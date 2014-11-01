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
		
		ssize_t	printf( const char* inFormatString, ... );
		ssize_t	send( const char* inData, size_t inLength );
		
	private:
		int		mSessionSocket;
	};


	class chatserver
	{
	public:
		chatserver( in_port_t inPortNumber = 0 );	//! 0 == open random port.
		
		bool        valid()         { return mIsOpen && mListeningSocket >= 0; }	// Has the constructor successfully initialized the socket?
		in_port_t   port_number()   { return mPortNumber; }	//! Give port number we're actually listening on.
		
		void        wait_for_connection();	//! Run the server loop and dispatch connections to handlers.
		
		/*! Register a handler for the given first word of a line. This word can start with any character
			you like, though usually people use IRC-style slashes so user communication can be distinguished
			from commands to the server. */
		void    	register_command_handler( std::string command, handler handler );

		handler		handler_for_command( std::string command );	// Used internally to look up handlers.
		
	private:
		bool                mIsOpen;
		int                 mListeningSocket;
		bool                mKeepRunning;
		in_port_t			mPortNumber;
		handler_map			mRequestHandlers;
	};

}

#endif /* defined(__eleven__eleven_chatserver__) */
