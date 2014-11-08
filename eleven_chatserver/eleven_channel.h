//
//  eleven_channel.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-02.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_channel__
#define __eleven__eleven_channel__


#include <string>
#include <vector>
#include "eleven_users.h"


namespace eleven
{
	class session;
	
	
	const sessiondata_id	CHANNEL_SESSION_DATA_ID = 0x4E414843;	// 'CHAN' in hex.
	
	
	class current_channel : public sessiondata
	{
	public:
		std::string		mChannelName;
	};
	

	class channel
	{
	public:
		channel( std::string inName ) : mChannelName(inName) {};
		
		bool	sendln( std::string inMessage );
		bool	printf( const char* inFormatString, ... );
		
		bool				join_channel( session* inSession, user_id inUserID, user_session* userSession );
		void				leave_channel( session* inSession, user_id inUserID, user_session* userSession );

		static handler		join_channel_handler;
		static handler		leave_channel_handler;
		static handler		chat_handler;
		
	protected:
		std::string				mChannelName;
		std::vector<user_id>	mBannedUsers;
		std::vector<user_id>	mUsers;
		
		static std::map<std::string,channel*>	channels;
	};

}

#endif /* defined(__eleven__eleven_channel__) */
