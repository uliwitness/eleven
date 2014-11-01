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
	return ::send( mSessionSocket, replyString, strlen(replyString), SO_NOSIGPIPE );	// Send!
}


ssize_t	session::send( std::string inString )
{
	return ::send( mSessionSocket, inString.c_str(), inString.size(), SO_NOSIGPIPE );	// Send!
}


ssize_t	session::send( const char *inData, size_t inLength )
{
	return ::send( mSessionSocket, inData, inLength, SO_NOSIGPIPE );    // Send!
}


bool	session::readln( std::string& outString )
{
	char                requestString[MAX_LINE_LENGTH];

	for( ssize_t x = 0; x < MAX_LINE_LENGTH; )
	{
		ssize_t	bytesRead = ::recv( mSessionSocket, requestString +x, 1, MSG_WAITALL );
		if( bytesRead != 1 )
			return false;
		if( requestString[x] == '\r' )
			requestString[x] = 0;	// Just terminate, don't break yet as we will have a \n as the next character.
		if( requestString[x] == '\n' )
		{
			requestString[x] = 0;
			break;
		}
		x += bytesRead;
	}
	
	requestString[MAX_LINE_LENGTH-1] = 0;
	
	outString = requestString;
	
	return true;
}


bool	session::read( std::vector<uint8_t> &outData )
{
	uint8_t*	bytes = &outData[0];
	ssize_t		numBytes = outData.size();
	
	ssize_t	bytesRead = ::recv( mSessionSocket, bytes, numBytes, MSG_WAITALL );
	if( bytesRead != numBytes )
		return false;
	
	return true;
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

		std::string	requestString;
		if( !session.readln( requestString ) )
			break;
		
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
