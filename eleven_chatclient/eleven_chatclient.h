//
//  eleven_chatclient.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_chatclient__
#define __eleven__eleven_chatclient__

#include <sys/socket.h>
#include <functional>
#include "eleven_session.h"


namespace eleven
{
	typedef std::function<void(session_ptr,std::string,class chatclient*)> message_handler;
	
	class chatclient
	{
	public:
		chatclient( const char* inIPAddress, in_port_t inPortNumber, const char* inSettingsFolderPath );
		
		~chatclient();
		
		
		bool		connect();	//!< Connect to the server. Returns TRUE on success.
		bool		valid()	{ return mSession && mSocket >= 0; };	//!< Did connect() manage to connect?
		
		/*! Register a handler for the given first word of a line. This word can start with any character
			you like, though usually people use IRC-style slashes so user communication can be distinguished
			from commands to the server. */
		void    	register_message_handler( std::string command, message_handler handler );
		
		/*! Starts a thread that retrieves server replies and hands them off to your handlers. */
		void		listen_for_messages();
		
		session_ptr	current_session() { return mSession; }
		
	private:
		static void	listen_for_messages_thread( eleven::chatclient* self );
		
		int										mSocket;
		session_ptr								mSession;
		std::string								mIPAddress;
		in_port_t								mDesiredPort;
		std::string								mSettingsFilePath;
		std::map<std::string,message_handler>	mHandlers;
	};
}

#endif /* defined(__eleven__eleven_chatclient__) */
