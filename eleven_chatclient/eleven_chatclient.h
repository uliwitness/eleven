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
	class chatclient
	{
	public:
		chatclient( const char* inIPAddress, in_port_t inPortNumber, const char* inSettingsFolderPath );
		
		~chatclient();
		
		bool		valid()	{ return mSession && mSocket >= 0; };
		
		void		listen_for_messages( std::function<void(session_ptr)> inCallback );
		
		session_ptr	current_session()
		{
			return mSession;
		}
		
	private:
		int			mSocket;
		session_ptr	mSession;
	};
}

#endif /* defined(__eleven__eleven_chatclient__) */
