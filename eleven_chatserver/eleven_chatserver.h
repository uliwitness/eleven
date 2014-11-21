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
#include <vector>
#include "eleven_session.h"
#include "eleven_ini_file.h"


namespace eleven
{
	typedef std::function<void(session_ptr,std::string,class chatserver*)>	handler;
	typedef std::map<std::string,handler>								handler_map;


	class chatserver
	{
	public:
		explicit chatserver( const char* inSettingsFolder, in_port_t inPortNumber = 0 );	//!< 0 == open random port.
		
		bool        valid()         { return mIsOpen && mListeningSocket >= 0; }	//!< Has the constructor successfully initialized the socket?
		in_port_t   port_number()   { return mPortNumber; }	//!< Give port number we're actually listening on.
		
		void        wait_for_connection();	//!< Run the server loop and dispatch connections to handlers.
		
		void		shut_down()			{ mKeepRunning = false; };
		
		/*! Register a handler for the given first word of a line. This word can start with any character
			you like, though usually people use IRC-style slashes so user communication can be distinguished
			from commands to the server. */
		void    	register_command_handler( std::string command, handler handler );

		handler		handler_for_command( std::string command );	// Used internally to look up handlers.
		
	protected:
		static void	session_thread( chatserver* server, int sessionSocket );
	
	private:
		bool                mIsOpen;
		int                 mListeningSocket;
		bool                mKeepRunning;
		in_port_t			mPortNumber;
		handler_map			mRequestHandlers;
		std::string			mSettingsFolderPath;
	};

}

#endif /* defined(__eleven__eleven_chatserver__) */
