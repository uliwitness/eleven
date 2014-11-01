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


namespace eleven
{

	class session;


	typedef std::function<bool(session*,std::string)>	handler;
	typedef std::map<std::string,handler>				handler_map;


	class session
	{
	public:
		session( int sessionSocket ) : mSessionSocket(sessionSocket) {}
		
		/*! Send the given formatted output to the client as a string. *DO NOT* call this as printf( cStringVar ) because if cStringVar contains '%' signs you will crash, use send() below instead for that case, or printf("%s",cStringVar). */
		ssize_t		printf( const char* inFormatString, ... );
		
		/*! Preferred over printf() if all you want is send a single string without substitution of '%'-sequences. */
		ssize_t		send( std::string inString );
		/*! Send the given raw byte data. This is handy for sending back e.g. images. */
		ssize_t		send( const char* inData, size_t inLength );
		
		/*! Read a single line as a string from the session. Useful for back-and-forth conversation during a session. Returns TRUE on success, FALSE on failure. */
		bool		readln( std::string& outString );
		/*! Read outData.size() bytes of binary data from the session into outData. Returns TRUE on success, FALSE on failure. */
		bool		read( std::vector<uint8_t>& outData );
		
	private:
		int		mSessionSocket;
	};


	class chatserver
	{
	public:
		chatserver( in_port_t inPortNumber = 0 );	//!< 0 == open random port.
		
		bool        valid()         { return mIsOpen && mListeningSocket >= 0; }	//!< Has the constructor successfully initialized the socket?
		in_port_t   port_number()   { return mPortNumber; }	//!< Give port number we're actually listening on.
		
		void        wait_for_connection();	//!< Run the server loop and dispatch connections to handlers.
		
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
