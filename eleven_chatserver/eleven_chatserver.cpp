//
//  eleven_chatserver.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatserver.h"
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <thread>


using namespace eleven;


void	session_thread( chatserver* server, int sessionSocket );


handler	chatserver::shutdown_handler = []( eleven::session_ptr session, std::string currRequest, chatserver* server )
{
	server->shut_down();
};


chatserver::chatserver( const char* inSettingsFolder, in_port_t inPortNumber ) // 0 == open random port.
	: mIsOpen(false), mListeningSocket(0), mSettingsFolderPath(inSettingsFolder)
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
		return;
	}
	
	mIsOpen = true;
	
	struct sockaddr_in boundAddress = {0};
	socklen_t len = sizeof(boundAddress);
	if( getsockname( mListeningSocket, (struct sockaddr *)&boundAddress, &len ) == -1 )
	{
		perror("Couldn't determine port socket was bound to.");
		close( mListeningSocket );
		return;
	}
	else
		mPortNumber = ntohs(boundAddress.sin_port);
}


void	chatserver::session_thread( chatserver* server, int sessionSocket )
{
	printf("New session started.\n");

	std::string	settingsFilePath( server->mSettingsFolderPath );
	settingsFilePath.append("/settings.ini");
	session_ptr		newSession( new session( sessionSocket, settingsFilePath.c_str(), SOCKET_TYPE_SERVER ) );
	if( !newSession->valid() )
	{
		close( sessionSocket );
		sessionSocket = 0;
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
	
	close( sessionSocket );
	
	printf("Session ended.\n");
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
		
		std::thread( &chatserver::session_thread, this, sessionSocket ).detach();
	}
}
    
void    chatserver::register_command_handler( std::string command, handler handler )
{
	mRequestHandlers[command] = handler;
}
