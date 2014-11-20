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
		mSocket = NULL;
		return;
	}
	
	std::string		settingsFilePath( inSettingsFolderPath );
	settingsFilePath.append("/settings.ini");
	mSession = new session( mSocket, settingsFilePath.c_str(), SOCKET_TYPE_CLIENT );
	if( !mSession->valid() )
	{
		delete mSession;
		mSession = NULL;
		close(mSocket);
		mSocket = NULL;
		return;
	}
}

chatclient::~chatclient()
{
	if( mSession )
		delete mSession;
	if( mSocket >= 0 )
		close( mSocket );
}


void	chatclient::listen_for_messages( std::function<void(session*)> inCallback )
{
	while( mSession->keep_running() )
	{
		// Sometimes SSL needs to process its own meta-data:
		int		err = -1, result = 0;
		while( err != 0 )
		{
			err = SSL_read( mSession->mSSLSocket, NULL, 0 );
			if( err == -1 )
			{
				if( SSL_get_error( mSession->mSSLSocket, err ) == SSL_ERROR_WANT_WRITE )
				{
					struct timeval	tv = { 1, 0 };
					fd_set			readfds;
					FD_ZERO( &readfds );
					FD_SET( mSocket, &readfds );
					fd_set			writefds;
					FD_ZERO( &writefds );
					FD_SET( mSocket, &writefds );
					
					result = select( mSocket +1, &readfds, &writefds, NULL, &tv );
				}
			}
			if( SSL_get_shutdown( mSession->mSSLSocket ) )
			{
				mSession->log_out();
			}
		}
		
		// Once there's no metadata left, try reading some data of our own:
		{
			struct timeval	tv = { 8, 0 };
			fd_set			readfds;
			FD_ZERO( &readfds );
			FD_SET( mSocket, &readfds );
			
			result = select( mSocket +1, &readfds, NULL, NULL, &tv );
			if( result > 0 && FD_ISSET(mSocket, &readfds) )
			{
				if( SSL_pending( mSession->mSSLSocket ) > 0 )
					inCallback( mSession );
			}
			else if( result == -1 )
			{
				perror( "Error listening to message from server" );
				mSession->log_out();
			}
			if( SSL_get_shutdown( mSession->mSSLSocket ) )
			{
				mSession->log_out();
			}
		}
	}
}

