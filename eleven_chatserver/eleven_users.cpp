//
//  eleven_users.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_users.h"
#include <openssl/sha.h>
#include <fstream>
 

using namespace eleven;


std::map<user_id,user>			user_session::users;
std::map<std::string,user_id>	user_session::namedUsers;


std::string	user_session::hash( std::string inPassword )
{
	unsigned char	buf[SHA256_DIGEST_LENGTH];
	SHA256( (const unsigned char*)inPassword.c_str(), inPassword.size(), buf );
	
	uint32_t*		longBuf = (uint32_t*) buf;
	char			str[SHA256_DIGEST_LENGTH * 2 +1] = {0};
	char*			currStrPtr = str;
	
	for( int x = 0; x < (SHA256_DIGEST_LENGTH / 4); x++ )
	{
		snprintf( currStrPtr, 9, "%0X", ntohl(longBuf[x]) );
		currStrPtr += 8;
	}
	
	return std::string( str, SHA256_DIGEST_LENGTH * 2 );
}


handler	user_session::login_handler = []( session* session, std::string inCommand )
{
	size_t			currOffset = 0;
	std::string		commandName = session::next_word( inCommand, currOffset );
	std::string		userName = session::next_word( inCommand, currOffset );
	std::string		password = session::next_word( inCommand, currOffset );
	
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( loginInfo )
	{
		session->sendln( "!NOO:You are already logged in." );
		return;
	}
	else
	{
		loginInfo = new user_session;
	}
	
	if( loginInfo->log_in( userName, password ) )
	{
		session->attach_sessiondata( USER_SESSION_DATA_ID, loginInfo );
		session->sendln( "YEAH:Log-in successful." );
	}
	else
	{
		delete loginInfo;
		loginInfo = NULL;
		session->sendln( "!BAD:Unable to log in." );
	}
};


handler	user_session::adduser_handler = []( session* session, std::string inCommand )
{
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "!PAS:You must log in first." );
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
		session->sendln( "!NEQ:Password and password confirmation don't match." );
		return;
	}
	
	if( flagOne.compare("moderator") == 0 || flagTwo.compare("moderator") == 0 )
		theFlags |= USER_FLAG_MODERATOR;
	if( flagOne.compare("owner") == 0 || flagTwo.compare("owner") == 0 )
		theFlags |= USER_FLAG_SERVER_OWNER;
	
	if( !loginInfo->add_user( userName, password, theFlags ) )
	{
		session->sendln( "!NOO:Failed to add user." );
		return;
	}
	else
		session->sendln( "YEAH:User added." );
};


handler	user_session::deleteuser_handler = []( session* session, std::string inCommand )
{
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "!PAS:You must log in first." );
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
		session->sendln( "!NEQ:User name and user name confirmation don't match." );
		return;
	}
	
	if( !loginInfo->delete_user( loginInfo->id_for_user_name(userName) ) )
	{
		session->sendln( "!NOO:Failed to delete user." );
		return;
	}
	else
		session->sendln( "YEAH:User deleted." );
};


handler	user_session::retireuser_handler = []( session* session, std::string inCommand )
{
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "!PAS:You must log in first." );
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
		session->sendln( "!NEQ:User name and user name confirmation don't match." );
		return;
	}
	
	if( !loginInfo->retire_user( loginInfo->id_for_user_name(userName) ) )
	{
		session->sendln( "!NOO:Failed to retire user." );
		return;
	}
	else
		session->sendln( "YEAH:User retired." );
};


handler	user_session::blockuser_handler = []( session* session, std::string inCommand )
{
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "!PAS:You must log in first." );
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
		session->sendln( "!NEQ:User name and user name confirmation don't match." );
		return;
	}
	
	if( !loginInfo->block_user( loginInfo->id_for_user_name(userName) ) )
	{
		session->sendln( "!NOO:Failed to block user." );
		return;
	}
	else
		session->sendln( "YEAH:User blocked." );
};


handler	user_session::makemoderator_handler = []( session* session, std::string inCommand )
{
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "!PAS:You must log in first." );
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
		session->printf( "!NOO:Failed to change moderator status of user %s.\r\n", userName.c_str() );
		return;
	}
	else
		session->printf( "YEAH:Changed moderator status of %s.", userName.c_str() );
};


handler	user_session::makeowner_handler = []( session* session, std::string inCommand )
{
	user_session*	loginInfo = (user_session*) session->find_sessiondata(USER_SESSION_DATA_ID);
	if( !loginInfo )
	{
		session->sendln( "!PAS:You must log in first." );
		return;
	}
	
	std::string		userName;
	std::string		state;
	size_t			currOffset = 0;
	
	session::next_word( inCommand, currOffset );
	userName = session::next_word( inCommand, currOffset );
	state = session::next_word( inCommand, currOffset );
	
	if( !loginInfo->change_user_flags( loginInfo->id_for_user_name(userName), (state.compare("yes") == 0) ? USER_FLAG_SERVER_OWNER : 0, (state.compare("yes") == 0) ? 0 : USER_FLAG_SERVER_OWNER ) )
	{
		session->printf( "!NOO:Failed to change owner status of user %s.\r\n", userName.c_str() );
		return;
	}
	else
		session->printf( "YEAH:Changed owner status of %s.", userName.c_str() );
};


bool	user_session::log_in( std::string inUserName, std::string inPassword )
{
	// What user ID does this user have?
	auto foundUserID = namedUsers.find( inUserName );
	if( foundUserID == namedUsers.end() )
		return false;
	
	// Find user entry for that ID:
	auto	foundUser = users.find( foundUserID->second );
	if( foundUser == users.end() )
		return false;	// Should never happen, but better be safe than sorry.
	
	// Don't let the user log in if (s)he's blocked:
	if( (foundUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return false;
	
	// Make sure the password matches:
	std::string		providedPasswordHash = hash(inPassword);
	if( foundUser->second.mPasswordHash.compare(providedPasswordHash) != 0 )
		return false;
	
	mCurrentUser = foundUserID->second;
	
	return true;
}


bool	user_session::block_user( user_id inUserIDToBlock )
{
	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return false;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return false;
	
	// Find the target:
	auto	foundUserToBlock = users.find( inUserIDToBlock );
	if( foundUserToBlock == users.end() )
		return false;
	
	// Only owners and moderators may block users:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;

	// A mere moderator may not block a server owner:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER) )
		return false;
	
	foundUserToBlock->second.mUserFlags |= USER_FLAG_BLOCKED;
	
	return true;
}


bool	user_session::retire_user( user_id inUserIDToDelete )
{
	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return false;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return false;
	
	auto	foundUserToBlock = users.find( inUserIDToDelete );
	if( foundUserToBlock == users.end() )
		return false;
	
	// Only owners and moderators may block users:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;

	// A mere moderator may not retire a server owner:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER) )
		return false;
	
	// In fact, nobody may retire a server owner, you have to remove that flag first:
	if( foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER )
		return false;
	
	foundUserToBlock->second.mUserFlags |= USER_FLAG_RETIRED;
	
	return true;
}


user_id	user_session::id_for_user_name( std::string inUserName )
{
	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return 0;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return 0;
	
	// Find target:
	auto	foundUser = namedUsers.find( inUserName );
	if( foundUser == namedUsers.end() )
		return 0;
	
	return foundUser->second;
}


bool	user_session::delete_user( user_id inUserIDToDelete )
{
	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return false;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return false;
	
	// Find target:
	if( inUserIDToDelete == 0 )	// Invalid user ID.
		return false;
	auto	foundUserToBlock = users.find( inUserIDToDelete );
	if( foundUserToBlock == users.end() )
		return false;
	
	// Only owners and moderators may delete users:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;

	// A mere moderator may not delete a server owner:
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR)
		&& (foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER) )
		return false;
	
	// In fact, nobody may delete a server owner, you have to remove that flag first:
	if( foundUserToBlock->second.mUserFlags & USER_FLAG_SERVER_OWNER )
		return false;
	
	// OK, all that cleared, delete the user's entry:
	users.erase(foundUserToBlock);
	
	// And delete the user's name (all names for that user ID, in case there are doubles for some reason):
	for( auto currUser = namedUsers.begin(); currUser != namedUsers.end(); )
	{
		if( currUser->second == inUserIDToDelete )
		{
			currUser = namedUsers.erase(currUser);
		}
		else
			currUser++;
	}
	
	return true;
}


user_flags	user_session::find_user_flags( user_id inUserID )
{
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
	// Check whether user is still logged in and hasn't been blocked since login:
	if( mCurrentUser == 0 )
		return false;
	
	auto	foundCurrentUser = users.find( mCurrentUser );
	if( foundCurrentUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_BLOCKED)
		|| (foundCurrentUser->second.mUserFlags & USER_FLAG_RETIRED) )
		return false;
	
	auto	foundUser = users.find( inUserID );
	if( foundUser == users.end() )
		return false;
	
	if( (foundCurrentUser->second.mUserFlags & USER_FLAG_MODERATOR) == 0
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;

	if( ((inSetFlags & USER_FLAG_SERVER_OWNER) != 0 || (inClearFlags & USER_FLAG_SERVER_OWNER) != 0)
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;
	
	if( ((inSetFlags & USER_FLAG_MODERATOR) != 0 || (inClearFlags & USER_FLAG_MODERATOR) != 0)
		&& (foundCurrentUser->second.mUserFlags & USER_FLAG_SERVER_OWNER) == 0 )
		return false;
	
	foundUser->second.mUserFlags &= ~inClearFlags;
	foundUser->second.mUserFlags |= inSetFlags;
	
	return true;
}


user_flags	user_session::my_user_flags()
{
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


bool	user_session::load_users( const char* filePath )
{
	std::ifstream	file( filePath );
	if( !file.is_open() )
		return false;
	
	while( !file.eof() )
	{
		user			theUser;
		user_id			userID = 0;
		file >> theUser.mUserName;
		file >> theUser.mPasswordHash;
		file >> userID;
		file >> theUser.mUserFlags;
		
		if( userID == 0 || theUser.mUserName.size() == 0 || theUser.mPasswordHash.size() == 0 )
			return false;
		
		users[userID] = theUser;
		namedUsers[theUser.mUserName] = userID;
	}
	
	file.close();
	
	return true;
}


bool	user_session::add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags )
{
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
	
	return true;
}
