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
#include <unistd.h>
 

using namespace eleven;


std::recursive_mutex				user_session::usersLock;	// Lock for users, namedUsers, loggedInUsers and shuttingDown TOGETHER!
std::map<user_id,user_session_ptr>	user_session::loggedInUsers;
bool								user_session::shuttingDown = false;
database*							user_session::userDatabase = NULL;



user_session::~user_session()
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	loggedInUsers.erase(mCurrentUser);
}

handler	user_session::login_handler = []( session_ptr session, std::string inCommand, chatserver* server )
{
	if( shuttingDown )
	{
		session->sendln( "/!shutting_down The server is shutting down." );
		session->disconnect();
		return;
	}
	
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
	if( shuttingDown )
	{
		session->sendln( "/!shutting_down The server is already shutting down." );
		session->disconnect();
		return;
	}
	
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
	
	int		shutdownTime = 30;
	
	// Remind users of impending shutdown (which you hopefully communicated with them before):
	loginInfo->log( "Sending %d second shutdown warning.\n", shutdownTime );
	{
		std::lock_guard<std::recursive_mutex>		lock(usersLock);
		shuttingDown = true;
		broadcast_printf( "/shutting_down %d Server is shutting down in %d seconds.\r\n", shutdownTime, shutdownTime );
	}
	
	// Give users some time to log out gracefully:
	if( shutdownTime > 10 )
	{
		sleep(shutdownTime -10);
	}
	
	for( int x = 10; x > 0; x-- )
	{
		broadcast_printf( "/shutting_down %d Server is shutting down in %d seconds.\r\n", x, x );
		sleep(1);
	}
	
	// Forcibly disconnect any stragglers (shuttingDown takes care nobody new can log on, so does the log):
	{
		std::lock_guard<std::recursive_mutex>		lock(usersLock);
		for( auto sessionItty : loggedInUsers )
		{
			session_ptr	theSession = sessionItty.second->current_session();
			theSession->disconnect();
		}
	}

	loginInfo->log( "Initiated server shutdown.\n" );
	server->shut_down();
};


void	user_session::broadcast_printf( const char* inFormatString, ... )
{
	va_list	args;
	va_start(args, inFormatString);
	char	msg[1024] = {0};
	size_t msgLength = vsnprintf(msg, sizeof(msg)-1, inFormatString, args );
	va_end(args);
	
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	
	for( auto sessionItty : loggedInUsers )
	{
		session_ptr	theSession = sessionItty.second->current_session();
		theSession->send( (uint8_t*) msg, msgLength );
	}
}


bool	user_session::log_in( std::string inUserName, std::string inPassword )
{
	if( shuttingDown )
	{
		current_session()->disconnect();
		return false;
	}
	
	std::lock_guard<std::recursive_mutex>		lock(usersLock);

	// Find user entry for that ID:
	auto	foundUser = userDatabase->user_from_name( inUserName );
	if( foundUser.mUserID == 0 )
	{
		log( "No entry for user %s.\n", inUserName.c_str() );
		return false;	// Should never happen, but better be safe than sorry.
	}
	
	// Don't let the user log in if (s)he's blocked:
	if( (foundUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundUser.mUserFlags & USER_FLAG_RETIRED) )
	{
		log( "Rejected because blocked: User %s (%d).\n", inUserName.c_str(), mCurrentUser );
		return false;
	}
	
	//printf( "%s %s 11 1\n", inUserName.c_str(), hash( inPassword ).c_str() );
	
	// Make sure the password matches:
	char			actualPasswordHash[SCRYPT_MCF_LEN] = {0};
	foundUser.mPasswordHash.copy(actualPasswordHash, SCRYPT_MCF_LEN);
	if( userDatabase->hash_password_equal( foundUser.mPasswordHash, inPassword ) )
	{
		log( "Wrong password for user %s (%d).\n", inUserName.c_str(), mCurrentUser );
		return false;
	}
	
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);
	mCurrentUser = foundUser.mUserID;
	
	// Only allow one session per user at a time:
	session_ptr	alreadyLoggedInSession = session_for_user( mCurrentUser );
	if( alreadyLoggedInSession )
		alreadyLoggedInSession->disconnect();
	
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
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
	{
		log("Can't find own user in trying to block user %d.\n", inUserIDToBlock);
		return false;
	}
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to block user %d.\n", inUserIDToBlock);
		return false;
	}
	
	// Find the target:
	auto	foundUserToBlock = userDatabase->user_from_id( inUserIDToBlock );
	if( foundUserToBlock.mUserID == 0 )
	{
		log("No user entry for block target user %d.\n", inUserIDToBlock);
		return false;
	}

	std::string	foundUserToBlockName( foundUserToBlock.mUserName );
	
	// Only owners and moderators may block users:
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to block user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToBlock);
		return false;
	}
	
	// A mere moderator may not block a server owner:
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock.mUserFlags & USER_FLAG_SERVER_OWNER) )
	{
		log("Not permitted to block moderator/owner user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToBlock);
		return false;
	}
	
	if( userDatabase->change_user_flags( foundUserToBlock.mUserID, USER_FLAG_BLOCKED, 0 ) )
	{
		log("Blocked user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToBlock);
	}
	else
		return false;
	
	// Log out that user if they're currently logged in:
	session_ptr blockedUserSession = session_for_user(inUserIDToBlock);
	if( blockedUserSession )
		blockedUserSession->disconnect();
	
	return true;
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
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
	{
		log("Can't find own user in trying to retire user %d.\n", inUserIDToDelete);
		return false;
	}
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to retire user %d.\n", inUserIDToDelete);
		return false;
	}
	
	auto	foundUserToBlock = userDatabase->user_from_id( inUserIDToDelete );
	if( foundUserToBlock.mUserID == 0 )
	{
		log("No user entry for retire target user %d.\n", inUserIDToDelete);
		return false;
	}
	
	std::string	foundUserToBlockName( foundUserToBlock.mUserName );
	
	// Only owners and moderators may block users:
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to retire user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// A mere moderator may not retire a server owner or another moderator:
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock.mUserFlags & USER_FLAG_SERVER_OWNER) )
	{
		log("Not permitted to retire moderator/owner user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// In fact, nobody may retire a server owner, you have to remove that flag first:
	if( foundUserToBlock.mUserFlags & USER_FLAG_SERVER_OWNER )
	{
		log("Can't retire an owner user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
		return false;
	}
	
	if( userDatabase->change_user_flags( foundUserToBlock.mUserID, USER_FLAG_RETIRED, 0 ) )
	{
		log("Retired user %s (%d).\n", foundUserToBlockName.c_str(), inUserIDToDelete);
	}
	else
		return false;
	
	// Log out that user if they're currently logged in:
	session_ptr blockedUserSession = session_for_user(inUserIDToDelete);
	if( blockedUserSession )
		blockedUserSession->disconnect();
	
	return true;
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
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
	{
		log("Can't find own user in trying to delete user %d.\n", inUserIDToDelete);
		return false;
	}
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
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
	auto	foundUserToBlock = userDatabase->user_from_id( inUserIDToDelete );
	if( foundUserToBlock.mUserID == 0 )
	{
		log("No user entry for delete target user %d.\n", inUserIDToDelete);
		return false;
	}
	
	// Only owners and moderators may delete users:
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to delete user %s (%d).\n", foundUserToBlock.mUserName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// A mere moderator may not delete a server owner:
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock.mUserFlags & USER_FLAG_SERVER_OWNER) )
	{
		log("Not permitted to delete moderator/owner user %s (%d).\n", foundUserToBlock.mUserName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// In fact, nobody may delete a server owner, you have to remove that flag first:
	if( foundUserToBlock.mUserFlags & USER_FLAG_SERVER_OWNER )
	{
		log("Can't delete an owner user %s (%d).\n", foundUserToBlock.mUserName.c_str(), inUserIDToDelete);
		return false;
	}
	
	// And delete the user's name (all names for that user ID, in case there are doubles for some reason):
	log("Deleting user %s (%d).\n", foundUserToBlock.mUserName.c_str(), inUserIDToDelete);
	userDatabase->delete_user( inUserIDToDelete );
	
	// Log out that user if they're currently logged in:
	session_ptr blockedUserSession = session_for_user(inUserIDToDelete);
	if( blockedUserSession )
		blockedUserSession->disconnect();
	
	return true;
}


user_flags	user_session::find_user_flags( user_id inUserID )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return USER_FLAG_RETIRED;
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
		return USER_FLAG_RETIRED;
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
		return USER_FLAG_RETIRED;
	
	auto	foundUser = userDatabase->user_from_id( inUserID );
	if( foundUser.mUserID == 0 )
		return USER_FLAG_RETIRED;
	
	return foundUser.mUserFlags;
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
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
	{
		log("Can't find own user in trying to change flags for user %d.\n", inUserID);
		return false;
	}
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
	{
		log("Blocked and trying to change flags for user %d.\n", inUserID);
		return false;
	}
	
	auto	foundUser = userDatabase->user_from_id( inUserID );
	if( foundUser.mUserID == 0 )
	{
		log("No user entry for flag change target user %d.\n", inUserID);
		return false;
	}

	std::string	foundUserName( foundUser.mUserName );
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;

	if( ((inSetFlags & USER_FLAG_SERVER_OWNER) != 0 || (inClearFlags & USER_FLAG_SERVER_OWNER) != 0)
		&& (foundCurrentUser.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to change owner flag on user %s (%d).\n", foundUserName.c_str(), inUserID);
		return false;
	}
	
	if( ((inSetFlags & USER_FLAG_MODERATOR) != 0 || (inClearFlags & USER_FLAG_MODERATOR) != 0)
		&& (foundCurrentUser.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log("Not permitted to change moderator flag user %s (%d).\n", foundUserName.c_str(), inUserID);
		return false;
	}
	
	if( !userDatabase->change_user_flags( inUserID, inSetFlags, inClearFlags ) )
	{
		log("Couldn't change flags on user %s (%d).\n", foundUserName.c_str(), inUserID);
		return false;
	}
	
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
	
	return true;
}


user_flags	user_session::my_user_flags()
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return USER_FLAG_RETIRED;
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
		return USER_FLAG_RETIRED;
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED) )
		return USER_FLAG_BLOCKED;
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
		return USER_FLAG_RETIRED;
	
	return foundCurrentUser.mUserFlags;
}


bool	user_session::add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags )
{
	std::lock_guard<std::recursive_mutex>		lock(usersLock);
	std::lock_guard<std::recursive_mutex>		my_lock(mUserSessionLock);

	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
	{
		log("Not logged in but trying to add a new user.\n");
		return false;
	}
	
	auto	foundCurrentUser = userDatabase->user_from_id( mCurrentUser );
	if( foundCurrentUser.mUserID == 0 )
	{
		log("No own user entry trying to add a new user.\n");
		return false;
	}
	
	if( (foundCurrentUser.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser.mUserFlags & USER_FLAG_RETIRED) )
	{
		log( "Blocked user %s (%d) trying to add a new user.\n", foundCurrentUser.mUserName.c_str(), mCurrentUser );
		return false;
	}
	if( inUserName.size() == 0 )
	{
		log( "User %s (%d) gave empty name for new user.\n", foundCurrentUser.mUserName.c_str(), mCurrentUser );
		return false;
	}
	
	if( inPassword.size() == 0 )
	{
		log( "User %s (%d) gave empty password for new user.\n", foundCurrentUser.mUserName.c_str(), mCurrentUser );
		return false;
	}
	
	// Only owners may add moderators or owners:
	if( ((inUserFlags & USER_FLAG_SERVER_OWNER) != 0
		|| (inUserFlags & USER_FLAG_MODERATOR) != 0)
		&& (my_user_flags() & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log( "User %s (%d) not permitted to make new user %s moderator/owner.\n", foundCurrentUser.mUserName.c_str(), mCurrentUser, inUserName.c_str() );
		return false;
	}
	
	// Only moderators and owners may add (regular) users:
	if( (my_user_flags() & USER_FLAG_MODERATOR) == 0
		&& (my_user_flags() & USER_FLAG_SERVER_OWNER) == 0 )
	{
		log( "User %s (%d) not permitted to make a new user \"%s\".\n", foundCurrentUser.mUserName.c_str(), mCurrentUser, inUserName.c_str() );
		return false;
	}
	
	// User names must be unique:
	if( userDatabase->user_from_name(inUserName).mUserID != 0 )
	{
		log( "User %s (%d) tried to make a new user with existing name \"%s\".\n", foundCurrentUser.mUserName.c_str(), mCurrentUser, inUserName.c_str() );
		return false;
	}
	
	user newUser = userDatabase->add_user( inUserName, inPassword, inUserFlags );
	if( newUser.mUserID != 0 )
	{
		log( "User %s (%d) created new user \"%s\" (%d).\n", foundCurrentUser.mUserName.c_str(), mCurrentUser, inUserName.c_str(), newUser.mUserID );
		return true;
	}
	else
		return false;
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


void	user_session::set_user_database( database* inUserDatabase )
{
	userDatabase = inUserDatabase;
}

