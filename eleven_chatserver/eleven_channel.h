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


namespace eleven
{

	#ifndef __eleven__eleven_users__
	typedef uint32_t	user_id;
	#endif
	
	class session;
	

	class channel
	{
	public:
		channel( std::string inName ) : mChannelName(inName) {};
		
		bool	sendln( std::string inMessage );
		
		virtual session*	session_for_user( user_id inUserID ) = 0;
		
		bool				join_channel( user_id inUserID )	{ mUsers.push_back(inUserID); return true; };	// +++ Reject banned users and users that are already in the room.
		void				leave_channel( user_id inUserID )	{  };	// +++ Remove user from room and if it was last user close channel.
		
	protected:
		std::string				mChannelName;
		std::vector<user_id>	mBannedUsers;
		std::vector<user_id>	mUsers;
	};

}

#endif /* defined(__eleven__eleven_channel__) */
