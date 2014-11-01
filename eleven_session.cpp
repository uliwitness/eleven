//
//  eleven_session.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_session.h"
#include <sys/socket.h>
#include <string>


using namespace eleven;


#define MAX_LINE_LENGTH 1024


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


ssize_t	session::send( const uint8_t *inData, size_t inLength )
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
