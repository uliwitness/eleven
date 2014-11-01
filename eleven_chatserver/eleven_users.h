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


namespace eleven
{
	enum
	{
		USER_FLAG_SERVER_OWNER	= (1 << 0),
		USER_FLAG_MODERATOR		= (1 << 1),
		USER_FLAG_BLOCKED		= (1 << 2),
		USER_FLAG_RETIRED		= (1 << 3)
	};
	typedef uint32_t	user_flags;
	
	typedef uint32_t	user_id;
	
	
	class user
	{
	public:
		std::string		mUserName;
		std::string		mPasswordHash;
		user_flags		mUserFlags;
	};
	
	
	class user_session
	{
	public:
		bool		log_in( std::string inUserName, std::string inPassword );
		
		bool		valid_user_password( std::string inUserName, std::string inPassword );
		bool		block_user( user_id inUserIDToBlock );
		bool		retire_user( user_id inUserIDToDelete );
		bool		delete_user( user_id inUserIDToDelete );
		
		user_flags	user_flags( user_id inUserID );

		std::string	hash( std::string inPassword );
		
	private:
		user_id		mCurrentUser;
		
		static std::map<user_id,user>			users;
		static std::map<std::string,user_id>	namedUsers;
	};
}

#endif /* defined(__eleven__eleven_users__) */
