//
//  eleven_log.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-24.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_log__
#define __eleven__eleven_log__

#include <stdarg.h>
#include <functional>
#include <string>

namespace eleven
{
	void	log( const char* inFormatString, ... );
	
	void	prefixed_logv( const char* extraString, const char* inFormatString, va_list args );	// extraString is useful for including e.g. IP address after connection in log messages implicitly.
	
	void	set_log_callback( std::function<void(const char*)> inCallback );
}

#endif /* defined(__eleven__eleven_log__) */
