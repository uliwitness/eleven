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


static std::mutex		sLogLock;


using namespace eleven;


void	eleven::log( const char* inFormatString, ... )
{
	std::lock_guard<std::mutex>	lock(sLogLock);	// Ensure date and actual log message arrive in one piece in the log, even if another thread locks at the same time.
	
	char		dateStr[30] = {0};
	time_t		theTime = time(NULL);
	const char*	dateFmt = "%Y-%m-%d %H:%M:%S";
	strftime( dateStr, sizeof(dateStr) -1, dateFmt, gmtime( &theTime ) );

	printf( "%s ", dateStr );
	
	va_list		args;
	va_start( args, inFormatString );
	vprintf( inFormatString, args );
	va_end(args);
}