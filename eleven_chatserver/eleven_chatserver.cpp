//
//  eleven_chatserver.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatserver.h"
#include "eleven_log.h"
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <thread>


using namespace eleven;


chatserver::chatserver( const char* inSettingsFolder, in_port_t inPortNumber ) // 0 == open random port.
	: mIsOpen(false), mListeningSocket(-1), mSettingsFolderPath(inSettingsFolder)
{
	int                 status = 0;
	struct sockaddr_in  my_name;
	
	mListeningSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if( mListeningSocket == -1 )
	{
		perror( "Couldn't create Socket." );
		return;
	}

	// Avoid the "Address already in use" error we get if we quickly quit and restart during debugging:
	int		yes = 1;
	setsockopt( mListeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes) );

	/* socket binding */
	my_name.sin_family = AF_INET;
	my_name.sin_addr.s_addr = INADDR_ANY;
	my_name.sin_port = htons(inPortNumber);
	
	status = bind( mListeningSocket, (struct sockaddr*)&my_name, sizeof(my_name) );
	if( status == -1 )
	{
		perror( "Couldn't bind socket to port." );
		return;
	}
	
	status = listen( mListeningSocket, 5 );
	if( status == -1 )
	{
		perror("Couldn't listen on port.");
		close( mListeningSocket );
		mListeningSocket = -1;
		return;
	}
	
	struct sockaddr_in boundAddress = {0};
	socklen_t len = sizeof(boundAddress);
	if( getsockname( mListeningSocket, (struct sockaddr *)&boundAddress, &len ) == -1 )
	{
		perror("Couldn't determine port socket was bound to.");
		close( mListeningSocket );
		mListeningSocket = -1;
		return;
	}
	else
		mPortNumber = ntohs(boundAddress.sin_port);
	
	mIsOpen = true;
}


void	chatserver::session_thread( chatserver* server, int sessionSocket, struct sockaddr_in senderAddress )
{
	char		senderAddressStr[INET_ADDRSTRLEN +1] = {0};
	
	try
	{
		inet_ntop( AF_INET, &senderAddress, senderAddressStr, sizeof(senderAddressStr) -1 );
		
		log("%s Session started.\n", senderAddressStr);

		session_ptr		newSession( new session( sessionSocket, senderAddressStr, server->mSettingsFolderPath, SOCKET_TYPE_SERVER ) );
		if( !newSession->valid() )
		{
			close( sessionSocket );
			sessionSocket = -1;
			return;
		}
		
		// Now read messages line-wise from the client:
		while( newSession->keep_running() )
		{
			std::string	requestString;
			if( !newSession->readln( requestString ) )
				break;
			
			// Find first word and look up the command handler for it:
			size_t			currOffset = 0;
			std::string     commandName = session::next_word( requestString, currOffset );
			handler    		foundHandler = server->handler_for_command(commandName);
			
			if( foundHandler )
				foundHandler( newSession, requestString, server );
		}
		
		if( newSession->mSSLSocket )
			newSession->force_disconnect();
		if( sessionSocket != -1 )
		{
			close( sessionSocket );
			sessionSocket = -1;
		}
	}
	catch( std::exception& exc )
	{
		log("%s Exception %s terminating session thread.\n", senderAddressStr, exc.what());
	}
	
	log("%s Session ended.\n", senderAddressStr);
}


handler	chatserver::handler_for_command( std::string commandName )
{
	auto    foundHandler = mRequestHandlers.find(commandName);
	
	if( foundHandler != mRequestHandlers.end() )
		return foundHandler->second;
	else
	{
		foundHandler = mRequestHandlers.find("*");
		if( foundHandler != mRequestHandlers.end() )
			return foundHandler->second;
		else
			return NULL;
	}
}


void	chatserver::wait_for_connection()
{
	mKeepRunning = true;
	
	while( mKeepRunning )
	{
		struct sockaddr_in  clientAddress = {0};
		socklen_t           clientAddressLength = 0;
		int                 sessionSocket = -1;
		
		// Block until someone "knocks" on our port:
		clientAddressLength = sizeof(clientAddress);
		sessionSocket = accept( mListeningSocket, (struct sockaddr*)&clientAddress, &clientAddressLength );
		if( sessionSocket == -1 )
		{
			perror( "Failed to accept connection." );
			return;
		}
		
		std::thread( &chatserver::session_thread, this, sessionSocket, clientAddress ).detach();
	}
}


void	chatserver::shut_down()
{
	mKeepRunning = false;
	
	if( mListeningSocket >= 0 )
	{
		close( mListeningSocket );
		mListeningSocket = -1;
	}
}

    
void    chatserver::register_command_handler( std::string command, handler handler )
{
	mRequestHandlers[command] = handler;
}
