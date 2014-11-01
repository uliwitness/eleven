//
//  eleven_chatserver.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_chatserver__
#define __eleven__eleven_chatserver__

#include <sys/socket.h>
#include <map>
#include <functional>
#include <string>


namespace eleven
{

	class session;


	typedef std::function<bool(session*,std::string)>	handler;
	typedef std::map<std::string,handler>				handler_map;


	class session
	{
	public:
		session( int sessionSocket ) : mSessionSocket(sessionSocket) {}
		
		void	printf( const char* inFormatString, ... );
		
	private:
		int		mSessionSocket;
	};


	class chatserver
	{
	public:
		chatserver( in_port_t inPortNumber = 0 );
		
		bool        valid()         { return mIsOpen && mListeningSocket >= 0; }
		in_port_t   port_number()   { return mPortNumber; }
		
		void        wait_for_connection();
		
		void    	register_command_handler( std::string command, handler handler );
		
	private:
		bool                mIsOpen;
		int                 mListeningSocket;
		bool                mKeepRunning;
		in_port_t			mPortNumber;
		handler_map			mRequestHandlers;
	};

}

#endif /* defined(__eleven__eleven_chatserver__) */
