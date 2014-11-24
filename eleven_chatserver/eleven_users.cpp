//
//  eleven_users.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_users.h"
#include "eleven_log.h"
#include "libscrypt.h"
#include <sys/param.h>
#include <fstream>
 

using namespace eleven;


std::recursive_mutex				user_session::usersLock;	// Lock for users, namedUsers and loggedInUsers TOGETHER!
std::map<user_id,user>				user_session::users;
std::map<std::string,user_id>		user_session::namedUsers;
std::map<user_id,user_session_ptr>	user_session::loggedInUsers;


std::string	user_session::hash( std::string inPassword )
{
	char			outbuf[SCRYPT_MCF_LEN] = {0};
	libscrypt_hash(outbuf, inPassword.c_str(), SCRYPT_N, SCRYPT_r, 4);
	
	return std::string( outbuf, SCRYPT_MCF_LEN );
}


user_session::~user_session()
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	loggedInUsers.erase(mCurrentUser);
}

handler	user_session::login_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	size_t			currOffset = 0;
	std::string		commandName = session::next_word( inCommand, currOffset );
	std::string		userName = session::next_word( inCommand, currOffset );
	std::string		password = session::next_word( inCommand, currOffset );
	
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( loginInfo )
	{
		session->sendln( "/!already_logged_in You are already logged in." );
		return;
	}
	else
	{
		loginInfo = user_session_ptr( new user_session( session ) );
	}
	
	if( loginInfo->log_in( userName, password ) )
	{
		session->attach_sessiondata<user_session>( USER_SESSION_DATA_ID, loginInfo );
		session->sendln( "/logged_in Log-in successful." );
	}
	else
	{
		loginInfo = user_session_ptr();
		session->sendln( "/!could_not_log_in Unable to log in." );
	}
};


handler	user_session::adduser_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		password;
	std::string		passwordConfirm;
	std::string		flagOne;
	std::string		flagTwo;
	size_t			currOffset = 0;
	user_flags		theFlags = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	password = session::next_word( inCommand, currOffset );
	passwordConfirm = session::next_word( inCommand, currOffset );
	flagOne = session::next_word( inCommand, currOffset );
	flagTwo = session::next_word( inCommand, currOffset );
	
	if( password.compare(passwordConfirm) != 0 )
	{
		session->sendln( "/!password_confirmation_mismatch Password and password confirmation don't match." );
		return;
	}
	
	if( flagOne.compare("moderator") == 0 || flagTwo.compare("moderator") == 0 )
		theFlags |= USER_FLAG_MODERATOR;
	if( flagOne.compare("owner") == 0 || flagTwo.compare("owner") == 0 )
		theFlags |= USER_FLAG_SERVER_OWNER;
	
	if( !loginInfo->add_user( userName, password, theFlags ) )
	{
		session->sendln( "/!could_not_add_user Failed to add user." );
		return;
	}
	else
		session->sendln( "/user_added User added." );
};


handler	user_session::deleteuser_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		userNameConfirm;
	size_t			currOffset = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	userNameConfirm = session::next_word( inCommand, currOffset );
	
	if( userName.compare(userNameConfirm) != 0 )
	{
		session->sendln( "/!user_name_confirmation_mismatch User name and user name confirmation don't match." );
		return;
	}
	
	bool	success = false;
	{
		std::lock_guard<std::recursive_mutex>		lock(usersLock);
		success = loginInfo->delete_user( loginInfo->id_for_user_name(userName) );
	}
	
	if( !success )
	{
		session->sendln( "/!could_not_delete_user Failed to delete user." );
		return;
	}
	else
		session->printf( "/user_deleted User %s deleted.\r\n", userName.c_str() );
};


handler	user_session::retireuser_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		userNameConfirm;
	size_t			currOffset = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	userNameConfirm = session::next_word( inCommand, currOffset );
	
	if( userName.compare(userNameConfirm) != 0 )
	{
		session->sendln( "/!user_name_confirmation_mismatch User name and user name confirmation don't match." );
		return;
	}
	
	if( !loginInfo->retire_user( loginInfo->id_for_user_name(userName) ) )
	{
		session->sendln( "/!could_not_retire_user Failed to retire user." );
		return;
	}
	else
		session->printf( "/retired_user User %s was retired.\r\n", userName.c_str() );
};


handler	user_session::blockuser_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		userNameConfirm;
	size_t			currOffset = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	userNameConfirm = session::next_word( inCommand, currOffset );
	
	if( userName.compare(userNameConfirm) != 0 )
	{
		session->sendln( "/!user_name_confirmation_mismatch User name and user name confirmation don't match." );
		return;
	}
	
	if( !loginInfo->block_user( loginInfo->id_for_user_name(userName) ) )
	{
		session->sendln( "/!could_not_block_user Failed to block user." );
		return;
	}
	else
		session->printf( "/blocked_user User %s was blocked.\r\n", userName.c_str() );
};


handler	user_session::makemoderator_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		state;
	size_t			currOffset = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	state = session::next_word( inCommand, currOffset );
	
	if( !loginInfo->change_user_flags( loginInfo->id_for_user_name(userName), (state.compare("yes") == 0) ? USER_FLAG_MODERATOR : 0, (state.compare("yes") == 0) ? 0 : USER_FLAG_MODERATOR ) )
	{
		session->printf( "/!could_not_change_moderator_status Failed to change moderator status of user %s.\r\n", userName.c_str() );
		return;
	}
	else
		session->printf( "/moderator_status_changed Changed moderator status of %s.\r\n", userName.c_str() );
};


handler	user_session::makeowner_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		state;
	size_t			currOffset = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	state = session::next_word( inCommand, currOffset );
	
	bool	success = false;
	{
		std::lock_guard<std::recursive_mutex>		lock(usersLock);
		success = loginInfo->change_user_flags( loginInfo->id_for_user_name(userName), (state.compare("yes") == 0) ? USER_FLAG_SERVER_OWNER : 0, (state.compare("yes") == 0) ? 0 : USER_FLAG_SERVER_OWNER );
	}
	
	if( !success )
	{
		session->printf( "/!could_not_change_owner_status Failed to change owner status of user %s.\r\n", userName.c_str() );
		return;
	}
	else
		session->printf( "/owner_status_changed Changed owner status of %s.\r\n", userName.c_str() );
};


handler	user_session::shutdown_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	user_session_ptr	loginInfo = session->find_sessiondata<user_session>(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		::log( "%s Attempt to shut down server while not logged in.",session->sender_address_str().c_str() );
		session->sendln( "/!not_logged_in You must log in first." );
		return;
	}
	
	if( (loginInfo->my_user_flags() & USER_FLAG_SERVER_OWNER) == 0 )
	{
		loginInfo->log( "Not permitted to shut down server." );
		session->sendln( "/!only_owners_can_shut_down Only server owners may shut down the server." );
		return;
	}
	
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	for( auto sessionItty : loggedInUsers )
	{
		session_ptr	theSession = sessionItty.second->current_session();
		theSession->printf("/shutting_down Server is shutting down.\r\n");
	}
	
	loginInfo->log( "Initiated server shutdown." );
	server->shut_down();
};


bool	user_session::log_in( std::string inUserName, std::string inPassword )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);

	// What user ID does this user have?
	auto foundUserID = namedUsers.find( inUserName );
	if( foundUserID == namedUsers.end() )
	{
		log( "No such user %s.\n", inUserName.c_str() );
		return false;
	}
	
	// Find user entry for that ID:
	auto	foundUser = users.find( foundUserID->second );
	if( foundUser == users.end() )
	{
		log( "No entry for user %s.\n", inUserName.c_str() );
		return false;	// Should never happen, but better be safe than sorry.
	}
	
	// Don't let the user log in if (s)he's blocked:
	if( (foundUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundUser->second.mUserFlags & USER_FLAG_RETIRED) )
	{
		log( "Rejected because blocked: User %s (%d).\n", inUserName.c_str(), mCurrentUser );
		return false;
	}
	
	//printf( "%s %s 11 1\n", inUserName.c_str(), hash( inPassword ).c_str() );
	
	// Make sure the password matches:
	char			actualPasswordHash[SCRYPT_MCF_LEN] = {0};
	foundUser->second.mPasswordHash.copy(actualPasswordHash, SCRYPT_MCF_LEN);
	if( libscrypt_check( actualPasswordHash, inPassword.c_str() ) <= 0 )
	{
		log( "Wrong password for user %s (%d).\n", inUserName.c_str(), mCurrentUser );
		return false;
	}
	
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);
	mCurrentUser = foundUserID->second;
	
	// Only allow one session per user at a time:
	session_ptr	alreadyLoggedInSession = session_for_user( mCurrentUser );
	if( alreadyLoggedInSession )
		alreadyLoggedInSession->log_out();
	
	// Log in the new session:
	loggedInUsers[mCurrentUser] = shared_from_this();

	log( "Logged in as user %s (%d).\n", inUserName.c_str(), mCurrentUser );
	
	return true;
}


bool	user_session::block_user( user_id inUserIDToBlock )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
	{
		log("Not logged in trying to block user %d.\n", inUserIDToBlock);
		return false;
	}
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
	{
		log("Can't find own user in trying to block user %d.\n", inUserIDToBlock);
		return false;
	}
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to block user %d.\n", inUserIDToBlock);
		return false;
	}
	
	// Find the target:
	auto	foundUserToBlock = users.find( inUserIDToBlock );
	if( foundUserToBlock == users.end() )
	{
		log("No user entry for block target user %d.\n", inUserIDToBlock);
		return false;
	}

	std::string	foundUserToBlockName( foundUserToBlock->second.mUserName );
	
	// Only owners and moderators may block users:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to block user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToBlock);
		return false;
	}
	
	// A mere moderator may not block a server owner:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER) )
	{
		log("Not permitted to block moderator/owner user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToBlock);
		return false;
	}
	
	foundUserToBlock->second.mUserFlags |= USER_FLAG_BLOCKED;
	log("Blocked user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToBlock);
	
	// Log out that user if they're currently logged in:
	session_ptr blockedUserSession = session_for_user(inUserIDToBlock);
	if( blockedUserSession )
		blockedUserSession->log_out();
	
	return save_users(NULL);
}


bool	user_session::retire_user( user_id inUserIDToDelete )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
	{
		log("Not logged in trying to retire user %d.\n", inUserIDToDelete);
		return false;
	}
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
	{
		log("Can't find own user in trying to retire user %d.\n", inUserIDToDelete);
		return false;
	}
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to retire user %d.\n", inUserIDToDelete);
		return false;
	}
	
	auto	foundUserToBlock = users.find( inUserIDToDelete );
	if( foundUserToBlock == users.end() )
	{
		log("No user entry for retire target user %d.\n", inUserIDToDelete);
		return false;
	}
	
	std::string	foundUserToBlockName( foundUserToBlock->second.mUserName );
	
	// Only owners and moderators may block users:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to retire user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// A mere moderator may not retire a server owner or another moderator:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER) )
	{
		log("Not permitted to retire moderator/owner user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// In fact, nobody may retire a server owner, you have to remove that flag first:
	if( foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER )
	{
		log("Can't retire an owner user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
		return false;
	}
	
	foundUserToBlock->second.mUserFlags |= USER_FLAG_RETIRED;
	log("Retired user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
	
	// Log out that user if they're currently logged in:
	session_ptr blockedUserSession = session_for_user(inUserIDToDelete);
	if( blockedUserSession )
		blockedUserSession->log_out();
	
	return save_users(NULL);
}


user_id	user_session::id_for_user_name( std::string inUserName )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return 0;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return 0;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return 0;
	
	// Find target:
	auto	foundUser = namedUsers.find( inUserName );
	if( foundUser == namedUsers.end() )
		return 0;
	
	return foundUser->second;
}


std::string	user_session::name_for_user_id( user_id inUser )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return std::string();
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return std::string();
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return std::string();
	
	// Find target:
	auto	foundUser = users.find( inUser );
	if( foundUser == users.end() )
		return std::string();
	
	return foundUser->second.mUserName;
}


bool	user_session::delete_user( user_id inUserIDToDelete )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
	{
		log("Not logged in trying to delete user %d.\n", inUserIDToDelete);
		return false;
	}
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
	{
		log("Can't find own user in trying to delete user %d.\n", inUserIDToDelete);
		return false;
	}
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to delete user %d.\n", inUserIDToDelete);
		return false;
	}
	
	// Find target:
	if( inUserIDToDelete == 0 )	// Invalid user ID.
	{
		log("Trying to delete invalid user ID %d.\n", inUserIDToDelete);
		return false;
	}
	auto	foundUserToBlock = users.find( inUserIDToDelete );
	if( foundUserToBlock == users.end() )
	{
		log("No user entry for delete target user %d.\n", inUserIDToDelete);
		return false;
	}
	
	// Only owners and moderators may delete users:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to delete user %s (%d).\n", foundUserToBlock->second.mUserName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// A mere moderator may not delete a server owner:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER) )
	{
		log("Not permitted to delete moderator/owner user %s (%d).\n", foundUserToBlock->second.mUserName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// In fact, nobody may delete a server owner, you have to remove that flag first:
	if( foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER )
	{
		log("Can't delete an owner user %s (%d).\n", foundUserToBlock->second.mUserName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// And delete the user's name (all names for that user ID, in case there are doubles for some reason):
	log("Deleting user %s (%d).\n", foundUserToBlock->second.mUserName.c_str(), inUserIDToDelete);
	for( auto currUser = namedUsers.begin(); currUser != namedUsers.end(); )
	{
		if( currUser->second == inUserIDToDelete )
		{
			currUser = namedUsers.erase(currUser);
		}
		else
			currUser++;
	}
	
	// OK, all that cleared, delete the user's entry:
	users.erase(foundUserToBlock);
	
	// Log out that user if they're currently logged in:
	session_ptr blockedUserSession = session_for_user(inUserIDToDelete);
	if( blockedUserSession )
		blockedUserSession->log_out();
	
	return save_users(NULL);
}


user_flags	user_session::find_user_flags( user_id inUserID )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return USER_FLAG_RETIRED;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return USER_FLAG_RETIRED;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return USER_FLAG_RETIRED;
	
	auto	foundUser = users.find( inUserID );
	if( foundUser == users.end() )
		return USER_FLAG_RETIRED;
	
	return foundUser->second.mUserFlags;
}


bool	user_session::change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
	{
		log("Not logged in trying to change flags for user %d.\n", inUserID);
		return false;
	}
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
	{
		log("Can't find own user in trying to change flags for user %d.\n", inUserID);
		return false;
	}
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to change flags for user %d.\n", inUserID);
		return false;
	}
	
	auto	foundUser = users.find( inUserID );
	if( foundUser == users.end() )
	{
		log("No user entry for flag change target user %d.\n", inUserID);
		return false;
	}

	std::string	foundUserName( foundUser->second.mUserName );
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;

	if( ((inSetFlags & USER_FLAG_SERVER_OWNER) != 0 || (inClearFlags & USER_FLAG_SERVER_OWNER) != 0)
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to change owner flag on user %s (%d).\n", foundUserName.c_str(), inUserID);
		return false;
	}
	
	if( ((inSetFlags & USER_FLAG_MODERATOR) != 0 || (inClearFlags & USER_FLAG_MODERATOR) != 0)
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to change moderator flag user %s (%d).\n", foundUserName.c_str(), inUserID);
		return false;
	}
	
	foundUser->second.mUserFlags &= ~inClearFlags;
	foundUser->second.mUserFlags |= inSetFlags;
	
	if( inClearFlags & USER_FLAG_MODERATOR )
	{
		log("Cleared moderator flag on user %s (%d).\n", foundUserName.c_str(), inUserID);
	}
	if( inClearFlags & USER_FLAG_SERVER_OWNER )
	{
		log("Cleared owner flag on user %s (%d).\n", foundUserName.c_str(), inUserID);
	}
	if( inSetFlags & USER_FLAG_MODERATOR )
	{
		log("Set moderator flag on user %s (%d).\n", foundUserName.c_str(), inUserID);
	}
	if( inSetFlags & USER_FLAG_SERVER_OWNER )
	{
		log("Set owner flag on user %s (%d).\n", foundUserName.c_str(), inUserID);
	}
	
	return save_users(NULL);
}


user_flags	user_session::my_user_flags()
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return USER_FLAG_RETIRED;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return USER_FLAG_RETIRED;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED) )
		return USER_FLAG_BLOCKED;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return USER_FLAG_RETIRED;
	
	return foundCurrentUser->second.mUserFlags;
}


static char		sUsersFilePath[MAXPATHLEN +1] = {0};
static char		sSettingsFolderPath[MAXPATHLEN +1] = {0};


const char*	user_session::settings_folder_path()
{
	return sSettingsFolderPath;
}

bool	user_session::load_users( const char* settingsFolderPath )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);

	strncpy(sSettingsFolderPath, settingsFolderPath, sizeof(sSettingsFolderPath) -1 );
	strncpy(sUsersFilePath, sSettingsFolderPath, sizeof(sUsersFilePath) -1 );
	if( sUsersFilePath[0] != 0 )
		strncat(sUsersFilePath, "/accounts.txt", sizeof(sUsersFilePath) -1 );
	else
		strncat(sUsersFilePath, "accounts.txt", sizeof(sUsersFilePath) -1 );
	std::ifstream	file( sUsersFilePath );
	if( !file.is_open() )
		return false;
	
	while( !file.eof() )
	{
		user			theUser;
		user_id			userID = 0;
		file >> theUser.mUserName;
		
		if( theUser.mUserName.size() == 0 )
			break;
		
		file >> theUser.mPasswordHash;
		file >> userID;
		file >> theUser.mUserFlags;
		
		if( userID == 0 || theUser.mPasswordHash.size() == 0 )
			return false;
		
		users[userID] = theUser;
		namedUsers[theUser.mUserName] = userID;
	}
	
	file.close();
	
	return true;
}


bool	user_session::save_users( const char* filePath )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);

	if( !filePath )
		filePath = sUsersFilePath;
	std::ofstream	file( filePath, std::ios::trunc | std::ios::out );
	if( !file.is_open() )
		return false;
	
	for( auto currUser = users.begin(); currUser != users.end(); currUser++ )
	{
		file << currUser->second.mUserName
			<< " "
			<< currUser->second.mPasswordHash
			<< " "
			<< currUser->first
			<< " "
			<< currUser->second.mUserFlags
			<< std::endl;
	}
	
	file.close();
	
	return true;
}


bool	user_session::add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return false;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return false;
	
	if( inUserName.size() == 0 )
		return false;
	
	if( inPassword.size() == 0 )
		return false;
	
	// Only owners may add moderators or owners:
	if( ((inUserFlags & USER_FLAG_SERVER_OWNER) != 0
		|| (inUserFlags & USER_FLAG_MODERATOR) != 0)
		&& (my_user_flags() & USER_FLAG_SERVER_OWNER) == 0 )
		return false;
	
	// Only moderators and owners may add (regular) users:
	if( (my_user_flags() & USER_FLAG_MODERATOR) == 0
		&& (my_user_flags() & USER_FLAG_SERVER_OWNER) == 0 )
		return false;
	
	// User names must be unique:
	if( namedUsers.find(inUserName) != namedUsers.end() )
		return false;
	
	// All clear? Get a unique user ID and create a user:
	user_id		newUserID = 1;
	while( users.find(newUserID) != users.end() )
		newUserID++;
	
	if( newUserID == 0 )	// We wrapped around. No more IDs left.
		return false;
	
	user			theUser;
	theUser.mUserName = inUserName;
	theUser.mPasswordHash = hash(inPassword);
	theUser.mUserFlags = inUserFlags;
	
	users[newUserID] = theUser;
	namedUsers[theUser.mUserName] = newUserID;
	
	return save_users(NULL);
}


session_ptr	user_session::session_for_user( user_id inUserID )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);

	auto	sessionItty = loggedInUsers.find( inUserID );
	if( sessionItty == loggedInUsers.end() )
		return NULL;
	else
		return sessionItty->second->current_session();
}


void	user_session::log( const char* inFormatString, ... )
{
	va_list		args;
	va_start(args, inFormatString);
	prefixed_logv( current_session()->sender_address_str().c_str(), inFormatString, args );
	va_end(args);
}
