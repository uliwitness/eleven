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
	
	typedef std::shared_ptr<current_channel>	current_channel_ptr;
	typedef std::shared_ptr<class channel>		channel_ptr;

	class channel
	{
	public:
		channel( std::string inName ) : mChannelName(inName) {};
		
		bool				sendln( std::string inMessage );
		bool				printf( const char* inFormatString, ... );
		
		bool				join_channel( session_ptr inSession, user_id inUserID, user_session_ptr userSession );
		bool				leave_channel( session_ptr inSession, user_id inUserID, user_session_ptr userSession, std::string inBlockedForReason = std::string() );
		bool				kick_user( session_ptr inSession, user_id inTargetUserID, user_session_ptr userSession );
		bool				user_is_kicked( user_id inUserID, user_session_ptr userSession );
		
		static channel_ptr	find_channel( std::string inChannelName, user_session_ptr theUserSession );
		
		static handler		join_channel_handler;
		static handler		leave_channel_handler;
		static handler		chat_handler;
		static handler		kick_handler;
		
	protected:
		std::string				mChannelName;	// Immutable after creation! Otherwise threading would be screwed!
		std::recursive_mutex	mUserListLock;	// Lock you take out before accessing mUsers.
		std::vector<user_id>	mUsers;			// Users currently in this room.
		
		static std::map<std::string,channel_ptr>	channels;
		static std::recursive_mutex					channels_lock;
	};
}

#endif /* defined(__eleven__eleven_channel__) */
