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
#include "eleven_database.h"


namespace eleven
{
	/*! This ID is what the user_session is saved as in the server's session. */
	const sessiondata_id		USER_SESSION_DATA_ID = 0x55534552; // 'USER' in Hex.
	
	typedef std::shared_ptr<class user_session>		user_session_ptr;
	
	
	/*! The current session login information. */
	class user_session : public sessiondata, public std::enable_shared_from_this<user_session>
	{
	public:
		explicit user_session( session_ptr inSession ) : mCurrentSession(inSession) {};
		~user_session();
		
		bool		log_in( std::string inUserName, std::string inPassword );
		
		bool		block_user( user_id inUserIDToBlock );
		bool		retire_user( user_id inUserIDToDelete );
		bool		delete_user( user_id inUserIDToDelete );
		
		bool		add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags = 0 );
		user_id		id_for_user_name( std::string inUserName );
		std::string	name_for_user_id( user_id inUser );
		
		user_flags	find_user_flags( user_id inUserID );
		user_flags	my_user_flags();
		std::string	my_user_name()		{ std::lock_guard<std::recursive_mutex> my_lock(mUserSessionLock); return name_for_user_id(mCurrentUser); };
		bool		change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags );
		user_id		current_user()		{ std::lock_guard<std::recursive_mutex> my_lock(mUserSessionLock); return mCurrentUser; };
		session_ptr	current_session()	{ std::lock_guard<std::recursive_mutex> my_lock(mUserSessionLock); return mCurrentSession; };
		
		void		log( const char* inFormatString, ... );	// Log prefixed with current session's IP address.
		
		static session_ptr	session_for_user( user_id inUserID );
		static void			broadcast_printf( const char* inFormatString, ... );	// Send a message to all logged-in users.

		static void			set_user_database( database* inUserDatabase );
		
		static handler		login_handler;
		static handler		adduser_handler;
		static handler		deleteuser_handler;
		static handler		retireuser_handler;
		static handler		blockuser_handler;
		static handler		makemoderator_handler;
		static handler		makeowner_handler;
		static handler		shutdown_handler;
		
	private:
		user_id										mCurrentUser;
		session_ptr									mCurrentSession;
		std::recursive_mutex						mUserSessionLock;
		
		static std::recursive_mutex					usersLock;	// Lock for any of users, namedUsers and loggedInUsers.
		static std::map<user_id,user_session_ptr>	loggedInUsers;
		static bool									shuttingDown;
		static database*							userDatabase;
	};
}

#endif /* defined(__eleven__eleven_users__) */
