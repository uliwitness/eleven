//
//  eleven_log.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-24.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_log.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <mutex>
#include <string>
#include <stdlib.h>


static std::mutex		sLogLock;


static std::function<void(const char*)> 	sLogCallback = []( const char* inLogMessage )
{
	fwrite( inLogMessage, sizeof(char), strlen(inLogMessage), stdout );
};


using namespace eleven;




void	eleven::log( const char* inFormatString, ... )
{
	va_list		args;
	va_start( args, inFormatString );
	prefixed_logv( "", inFormatString, args );
	va_end(args);
}


void	eleven::prefixed_logv( const char* extraString, const char* inFormatString, va_list args )
{
	char		dateStr[1024] = {0};
	time_t		theTime = time(NULL);
	const char*	dateFmt = "%Y-%m-%d %H:%M:%S ";
	strftime( dateStr, sizeof(dateStr) -1, dateFmt, gmtime( &theTime ) );

	std::lock_guard<std::mutex>	lock(sLogLock);	// Ensure date and actual log message arrive in one piece in the log, even if another thread locks at the same time.
	
	size_t	dateLen = strlen(dateStr);
	size_t	extraLen = strlen(extraString);
	size_t	padCharLen = (extraLen > 0)? 1 : 0;
	if( padCharLen > 0 )
	{
		strcat( dateStr, extraString );
		strcat( dateStr, " " );
	}
	vsnprintf( dateStr +dateLen +extraLen +padCharLen, sizeof(dateStr) -dateLen -extraLen -padCharLen -1, inFormatString, args );
	
	sLogCallback( dateStr );
}


void	eleven::set_log_callback( std::function<void(const char*)> inCallback )
{
	sLogCallback = inCallback;
}
