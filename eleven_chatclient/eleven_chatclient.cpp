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


using namespace eleven;


chatclient::chatclient( const char* inIPAddress, in_port_t inPortNumber, const char* inSettingsFolderPath )
	: mSocket(-1), mSession(NULL), mIPAddress(inIPAddress), mDesiredPort(inPortNumber)
{
	std::string		settingsFilePath( inSettingsFolderPath );
	settingsFilePath.append("/settings.ini");
	mSettingsFilePath = settingsFilePath;
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

	mSession = session_ptr( new session( mSocket, "", mSettingsFilePath.c_str(), SOCKET_TYPE_CLIENT ) );
	if( !mSession->valid() )
	{
		mSession = session_ptr();
		close(mSocket);
		mSocket = -1;
		return false;
	}
	
	return true;
}


void	chatclient::listen_for_messages_thread( chatclient* self )
{
	char	data[1024] = {0};
	int		dataRead = 0;
	
	while( self->mSession->keep_running() )
	{
		if( sizeof(data) == dataRead )
		{
			self->mSession->disconnect();
			break;
		}
		
		int	amountRead = SSL_read( self->mSession->mSSLSocket, data +dataRead, 1 );
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
					foundHandlerItty->second( self->mSession, dataStr, self );
			}
			dataRead = 0;
		}
		else if( amountRead < 0 )
		{
			int		sslerr = SSL_get_error( self->mSession->mSSLSocket, amountRead );
			printf( "err = %d\n", sslerr );
			if( sslerr == SSL_ERROR_SYSCALL )
				printf( "\terrno = %d\n", errno );
		}
		else if( amountRead == 0 )
		{
			self->mSession->disconnect();
		}
		if( SSL_get_shutdown( self->mSession->mSSLSocket ) )
		{
			self->mSession->disconnect();
		}
	}
	
	self->mSession = session_ptr();
	close(self->mSocket);
	self->mSocket = -1;
}


void    chatclient::register_message_handler( std::string command, message_handler handler )
{
	mHandlers[command] = handler;
}


void	chatclient::listen_for_messages()
{
	std::thread( &chatclient::listen_for_messages_thread, this ).detach();
}

