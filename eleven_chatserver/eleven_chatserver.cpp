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
#include <thread>


#define MAX_LINE_LENGTH 1024


using namespace eleven;


void	session_thread( chatserver* server, int sessionSocket );


ssize_t	session::printf( const char* inFormatString, ... )
{
	char	replyString[MAX_LINE_LENGTH];
	replyString[sizeof(replyString) -1] = 0;    // snprintf doesn't terminate if text length is >= buffer size, so terminate manually and give it one less byte of buffer to work with so it doesn't overwrite us.
	
	va_list		args;
	va_start( args, inFormatString );
	vsnprintf( replyString, sizeof(replyString) -1, inFormatString, args );
	va_end(args);
	return write( mSessionSocket, replyString, strlen(replyString) );    // Send!
}


ssize_t	session::send( std::string inString )
{
	return write( mSessionSocket, inString.c_str(), inString.size() );    // Send!
}


ssize_t	session::send( const char *inData, size_t inLength )
{
	return write( mSessionSocket, inData, inLength );    // Send!
}


std::string	session::readln()
{
	ssize_t             x = 0,
						bytesRead = 0;
	char                requestString[MAX_LINE_LENGTH];

	if( (bytesRead = read(mSessionSocket, requestString + x, MAX_LINE_LENGTH -x)) > 0 )
	{
		x += bytesRead;
	}
	
	if( bytesRead == -1 )
	{
		perror("Couldn't read request.");
		return "";
	}
	
	// Trim off trailing line break:
	if( x >= 2 && requestString[x-2] == '\r' )    // Might be Windows-style /r/n like Mac OS X sends it.
		requestString[x-2] = '\0';
	if( x >= 1 && requestString[x-1] == '\n' )
		requestString[x-1] = '\0';
	
	return requestString;
}


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


void	session_thread( chatserver* server, int sessionSocket )
{
	session		session(sessionSocket);
	
	// Now read messages line-wise from the client:
	bool        keepSessionRunning = true;
	while( keepSessionRunning )
	{
		ssize_t             x = 0;

		std::string	requestString = session.readln();
		
		// Find first word and look up the command handler for it:
		std::string     commandName;
		size_t  		commandLen = 0;
		for( x = 0; requestString[x] != 0 && requestString[x] != ' ' && requestString[x] != '\t' && requestString[x] != '\r' && requestString[x] != '\n'; x++ )
		{
			commandLen = x +1;
		}
		if( commandLen > 0 )
			commandName = requestString.substr( 0, commandLen );
		
		handler    foundHandler = server->handler_for_command(commandName);
		
		keepSessionRunning = foundHandler( &session, requestString );
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
			return []( session* session, std::string currRequest ){ session->send("\n", 1); return true; };
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
		
		std::thread( &session_thread, this, sessionSocket ).detach();
	}
}
    
void    chatserver::register_command_handler( std::string command, handler handler )
{
	mRequestHandlers[command] = handler;
}
