//
//  eleven_users.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_users.h"


using namespace eleven;


std::map<user_id,user>			user_session::users;
std::map<std::string,user_id>	user_session::namedUsers;


std::string	user_session::hash( std::string inPassword )
{
	return inPassword;
}


bool	user_session::log_in( std::string inUserName, std::string inPassword )
{
	auto foundUserID = namedUsers.find( inUserName );
	if( foundUserID == namedUsers.end() )
		return false;
	
	auto	foundUser = users.find( foundUserID->second );
	if( foundUser == users.end() )
		return false;
	
	if( foundUser->second.mUserFlags & USER_FLAG_BLOCKED )
		return false;
	
	if( foundUser->second.mUserFlags & USER_FLAG_RETIRED )
		return false;
	
	std::string		providedPasswordHash = hash(inPassword);
	if( foundUser->second.mPasswordHash.compare(providedPasswordHash) != 0 )
		return false;
	
	return true;
}


bool	user_session::valid_user_password( std::string inUserName, std::string inPassword )
{
	
}


bool	user_session::block_user( user_id inUserIDToBlock )
{
	if( mCurrentUser == 0 )
		return false;
	
	
}


bool	user_session::retire_user( user_id inUserIDToDelete )
{
	if( mCurrentUser == 0 )
		return false;
	
}


bool	user_session::delete_user( user_id inUserIDToDelete )
{
	if( mCurrentUser == 0 )
		return false;
	
}


user_flags	user_session::user_flags( user_id inUserID )
{
	if( mCurrentUser == 0 )
		return false;
	
}
