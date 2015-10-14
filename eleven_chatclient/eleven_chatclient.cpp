//
//  eleven_chatclient.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatclient.h"
#include "eleven_session.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#include <thread>
#include <iostream>


using namespace eleven;


chatclient::chatclient( const char* inIPAddress, in_port_t inPortNumber, const char* inSettingsFolderPath )
	: mSocket(-1), mSession(NULL), mIPAddress(inIPAddress), mDesiredPort(inPortNumber), mSettingsFolderPath(inSettingsFolderPath)
{
}

chatclient::~chatclient()
{
	if( mSession )
		mSession = session_ptr();
	if( mSocket >= 0 )
	{
		close( mSocket );
		mSocket = -1;
	}
}


bool	chatclient::connect()
{
	if( mSocket != -1 )
		return false;

	if( mSession && mSession->valid() )
		mSession->disconnect();
	
	struct sockaddr_in		serverAddress = {0};

	mSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if( mSocket == -1 )
	{
		perror("Couldn't create socket");
		return false;
	}

	serverAddress.sin_family = AF_INET;
	inet_aton( mIPAddress.c_str(), &serverAddress.sin_addr);
	serverAddress.sin_port = htons(mDesiredPort);

	int	status = ::connect( mSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress) );
	if( status == -1 )
	{
		perror("Couldn't connect to server");
		close(mSocket);
		mSocket = -1;
		return false;
	}

	mSession = session_ptr( new session( mSocket, "", mSettingsFolderPath, SOCKET_TYPE_CLIENT ) );
	if( !mSession->valid() )
	{
		mSession = session_ptr();
		close(mSocket);
		mSocket = -1;
		return false;
	}
	
	mSession->wait_for_queued_data();
	
	return true;
}


void	chatclient::listen_for_messages_thread( chatclient* self )
{
	try
	{
		char	data[1024] = {0};
		int		dataRead = 0;
		
		session_ptr		thisSession( self->mSession );
		
		while( thisSession->keep_running() )
		{
			if( sizeof(data) == dataRead )
			{
				thisSession->disconnect();
				break;
			}
			
			int	amountRead = SSL_pending(thisSession->mSSLSocket);
			if( amountRead < 0 )
			{
				int		sslerr = SSL_get_error( thisSession->mSSLSocket, amountRead );
				if( sslerr == SSL_ERROR_NONE )
					continue;
				::printf("sslerr = %d\n", sslerr);
			}

			amountRead = SSL_read( thisSession->mSSLSocket, data +dataRead, 1 );
			if( amountRead == 1 )
				dataRead++;
			
			if( dataRead > 0 && (data[dataRead-1] == '\n' || data[dataRead-1] == '\r') )	// Read any leftover lines *before* checking for errors.
			{
				if( dataRead != 1 || (data[dataRead-1] != '\n' && data[dataRead-1] != '\r' && data[dataRead-1] != '\0') )
				{
					std::string		dataStr(data,dataRead-1);
					std::string		messageCode;
					size_t			currOffset = 0;
					
					messageCode = session::next_word( dataStr, currOffset );
					auto	foundHandlerItty = self->mHandlers.find( messageCode );
					if( foundHandlerItty == self->mHandlers.end() )
						foundHandlerItty = self->mHandlers.find( "*" );
					if( foundHandlerItty != self->mHandlers.end() )
						foundHandlerItty->second( thisSession, dataStr, self );
				}
				dataRead = 0;
			}
			else if( amountRead < 0 )
			{
				int		sslerr = SSL_get_error( thisSession->mSSLSocket, amountRead );
				printf( "err = %d\n", sslerr );
				if( sslerr == SSL_ERROR_SYSCALL )
					printf( "\terrno = %d\n", errno );
				if( sslerr == SSL_ERROR_SSL )
					thisSession->disconnect();
			}
			else if( amountRead == 0 )
			{
				thisSession->disconnect();
			}
			if( thisSession->mSSLSocket && SSL_get_shutdown( thisSession->mSSLSocket ) )
			{
				thisSession->disconnect();
			}
		}
		
		self->mSession = session_ptr();
		if( self->mSocket != -1 )
		{
			close(self->mSocket);
			self->mSocket = -1;
		}
	}
	catch( std::exception& exc )
	{
		std::cout << "Exception " << exc.what() << " terminating message listening thread." << std::endl;
	}
}


void    chatclient::register_message_handler( std::string command, message_handler handler )
{
	mHandlers[command] = handler;
}


void	chatclient::listen_for_messages()
{
	std::thread( &chatclient::listen_for_messages_thread, this ).detach();
}

