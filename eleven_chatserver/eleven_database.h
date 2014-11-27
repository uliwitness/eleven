//
//  eleven_database.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-25.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectserver__eleven_database__
#define __interconnectserver__eleven_database__

#include "eleven_ini_file.h"
#include <string>


namespace eleven
{
	enum user_flags_enum
	{
		USER_FLAG_SERVER_OWNER	= (1 << 0),	//!< An owner of the server. Can do everything.
		USER_FLAG_MODERATOR		= (1 << 1),	//!< A moderator. Can do owner-y things to everyone but other owners or moderators.
		USER_FLAG_BLOCKED		= (1 << 2),	//!< A blocked user that's not permitted to log in (banned from the server, but account not deleted).
		USER_FLAG_RETIRED		= (1 << 3)	//!< A user whose account still exists but is no longer permitted to log in. Might be used as a grace phase before actual deletion.
	};
	typedef uint32_t	user_flags;	//!< A bitfield of the values in the user_flags_enum.
	
	typedef uint32_t	user_id;	//!< Unique number identifying a user. 0 is invalid.
	
	
	/*! All global information we have about a user as an account, i.e. what's needed for
		login and access control. */
	class user
	{
	public:
		user() : mUserID(0), mUserFlags(0) {}
		
		user_id			mUserID;
		std::string		mUserName;
		std::string		mPasswordHash;
		user_flags		mUserFlags;
	};
	
	
	/*! Abstract base class for a user database. Doesn't care whether
		the actual storage is SQLite, MySQL, flat files or hamsters in
		a wheel. */
	class database
	{
	public:
		virtual ~database()	{};
	
		/*! Gives the user entry for the specified ID. If the result's
			mUserID is 0, there is no user with ID inUserID. */
		virtual user		user_from_id( user_id inUserID ) = 0;
		/*! Gives the user entry with the specified name. If the result's
			mUserID is 0, there is no user with name inUserName. */
		virtual user		user_from_name( std::string inUserName ) = 0;
		/*! Create a new user with the given name, un-hashed password and flags. User is assigned a new unique user ID. The user's info is returned from this call. If the result's mUserID == 0, an error occurred. */
		virtual user		add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags = 0 ) = 0;
		/*! Return the user ID associated with the given user name. */
		virtual user_id		id_for_user_name( std::string inUserName ) = 0;
		/*! Change a given user's permission flags. */
		virtual bool		change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags ) = 0;
		/*! Prevent a user from ever logging in again in the given channel by adding it to that channel's kick list. */
		virtual bool		kick_user_from_channel( user_id inUserID, std::string channelName ) = 0;
		/*! Query whether a user has been banned from ever logging into the given channel again by querying that channel's kick list. */
		virtual bool		is_user_kicked_from_channel( user_id inUserID, std::string channelName ) = 0;
		/*! Permanently delete the user with the given ID from the database. */
		virtual bool		delete_user( user_id inUserID ) = 0;
		
		/*! Create a hash for a given password. */
		static std::string	hash( std::string inPassword );
		/*! Compare a password hash with an un-hashed password, returning whether they are equal. */
		static bool			hash_password_equal( std::string inHash, std::string inPassword );
	};

}

#endif /* defined(__interconnectserver__eleven_database__) */
