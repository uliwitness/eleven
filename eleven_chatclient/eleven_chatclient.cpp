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
	: mSocket(-1), mSession(NULL)
{
	struct sockaddr_in		serverAddress = {0};

	mSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if( mSocket == -1 )
	{
		perror("Couldn't create socket");
		return;
	}

	serverAddress.sin_family = AF_INET;
	inet_aton( inIPAddress, &serverAddress.sin_addr);
	serverAddress.sin_port = htons(inPortNumber);

	int	status = connect( mSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress) );
	if( status == -1 )
	{
		perror("Couldn't connect to server");
		close(mSocket);
		mSocket = -1;
		return;
	}
	
	std::string		settingsFilePath( inSettingsFolderPath );
	settingsFilePath.append("/settings.ini");
	mSession = session_ptr( new session( mSocket, settingsFilePath.c_str(), SOCKET_TYPE_CLIENT ) );
	if( !mSession->valid() )
	{
		mSession = session_ptr();
		close(mSocket);
		mSocket = -1;
		return;
	}
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


void	chatclient::listen_for_messages_thread( chatclient* self, std::function<void(session_ptr,std::string, eleven::chatclient*)> inCallback )
{
	char	data[1024] = {0};
	int		dataRead = 0;
	
	while( self->mSession->keep_running() )
	{
		if( sizeof(data) == dataRead )
		{
			self->mSession->log_out();
			break;
		}
		
		int	amountRead = SSL_read( self->mSession->mSSLSocket, data +dataRead, 1 );
		if( amountRead > 0 )
			dataRead++;
		
		if( dataRead > 0 && data[dataRead-1] == '\n' )	// Read any leftover lines *before* checking for errors.
		{
			inCallback( self->mSession, std::string(data,dataRead-1), self );
			dataRead = 0;
		}
		if( SSL_get_shutdown( self->mSession->mSSLSocket ) )
		{
			self->mSession->log_out();
		}
	}
}


void	chatclient::listen_for_messages( std::function<void(session_ptr,std::string, eleven::chatclient*)> inCallback )
{
	std::thread( &chatclient::listen_for_messages_thread, this, inCallback ).detach();
}

