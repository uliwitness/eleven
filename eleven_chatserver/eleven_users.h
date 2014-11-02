//
//  eleven_users.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_users__
#define __eleven__eleven_users__

#include <string>
#include <map>
#include "eleven_chatserver.h"


namespace eleven
{
	/*! This ID is what the user_session is saved as in the server's session. */
	const sessiondata_id		USER_SESSION_DATA_ID = 0x55534552; // 'USER' in Hex.
	
	enum user_flags_enum
	{
		USER_FLAG_SERVER_OWNER	= (1 << 0),	//! An owner of the server. Can do everything.
		USER_FLAG_MODERATOR		= (1 << 1),	//! A moderator. Can do owner-y things to everyone but other owners or moderators.
		USER_FLAG_BLOCKED		= (1 << 2),	//! A blocked user that's not permitted to lock in (banned from the server, but account not deleted).
		USER_FLAG_RETIRED		= (1 << 3)	//! A user whose account still exists but is no longer permitted to log in. Might be used as a grace phase before actual deletion.
	};
	typedef uint32_t	user_flags;	//! A bitfield of the values in the user_flags_enum.
	
	typedef uint32_t	user_id;	//! Unique key by which users are identified. 0 is invalid.
	
	
	/*! All global information we have about a user as an account, i.e. what's needed for
		login and access control. */
	class user
	{
	public:
		std::string		mUserName;
		std::string		mPasswordHash;
		user_flags		mUserFlags;
	};
	
	
	/*! The current session login information. */
	class user_session : sessiondata
	{
	public:
		bool		log_in( std::string inUserName, std::string inPassword );
		
		bool		block_user( user_id inUserIDToBlock );
		bool		retire_user( user_id inUserIDToDelete );
		bool		delete_user( user_id inUserIDToDelete );
		
		bool		add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags = 0 );
		user_id		id_for_user_name( std::string inUserName );
		
		user_flags	find_user_flags( user_id inUserID );
		user_flags	my_user_flags();
		bool		change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags );

		std::string	hash( std::string inPassword );
	
		static	bool	load_users( const char* filePath );
		static	bool	save_users( const char* filePath );
		
		static handler	login_handler;
		static handler	adduser_handler;
		static handler	deleteuser_handler;
		static handler	retireuser_handler;
		static handler	blockuser_handler;
		static handler	makemoderator_handler;
		static handler	makeowner_handler;
		
	private:
		user_id		mCurrentUser;
		
		static std::map<user_id,user>			users;
		static std::map<std::string,user_id>	namedUsers;
	};
}

#endif /* defined(__eleven__eleven_users__) */
