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
#include <mutex>
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
		
		bool				sendln( std::string inMessage );
		bool				printf( const char* inFormatString, ... );
		
		bool				join_channel( session* inSession, user_id inUserID, user_session* userSession );
		bool				leave_channel( session* inSession, user_id inUserID, user_session* userSession, std::string inBlockedForReason = std::string() );
		bool				kick_user( session* inSession, user_id inTargetUserID, user_session* userSession );
		bool				user_is_kicked( user_id inUserID );
		
		bool				save_kicklist( user_session* userSession );
		bool				load_kicklist( user_session* userSession );
		
		static channel*		find_channel( std::string inChannelName, user_session* theUserSession );
		
		static handler		join_channel_handler;
		static handler		leave_channel_handler;
		static handler		chat_handler;
		static handler		kick_handler;
		
	protected:
		std::string				mChannelName;
		std::vector<user_id>	mKickedUsers;	// Users forbidden from joining this room.
		std::vector<user_id>	mUsers;			// Users currently in this room.
		
		static std::map<std::string,channel*>	channels;
		static std::mutex						channels_lock;
	};

}

#endif /* defined(__eleven__eleven_channel__) */
