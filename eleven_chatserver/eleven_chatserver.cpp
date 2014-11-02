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


chatserver::chatserver( in_port_t inPortNumber ) // 0 == open random port.
	: mIsOpen(false), mListeningSocket(0)
{
	int                 status = 0;
	struct sockaddr_in  my_name;
	
	mListeningSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if( mListeningSocket == -1 )
	{
		perror( "Couldn't create Socket." );
		return;
	}

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
	session		session(sessionSocket);
	
	// Now read messages line-wise from the client:
	while( session.keep_running() )
	{
		std::string	requestString;
		if( !session.readln( requestString ) )
			break;
		
		// Find first word and look up the command handler for it:
		size_t			currOffset = 0;
		std::string     commandName = session::next_word( requestString, currOffset );
		handler    		foundHandler = server->handler_for_command(commandName);
		
		foundHandler( &session, requestString );
	}
	
	close( sessionSocket );
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
			return []( session* session, std::string currRequest ){ session->send((uint8_t*)"\n", 1); };
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
