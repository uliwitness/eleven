//
//  eleven_database_flatfile.cpp
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-25.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_database_flatfile.h"
#include "eleven_log.h"
#include "eleven_ini_file.h"
#include <fstream>

using namespace std;
using namespace eleven;


#define PASSWORD_HASH		"$s1$0e0804$GJqUPV/GowGPc1myVgovRg==$7at14hpizppp5LWApvwC5DnUX5bxtjNYk79dutroHsu0lKc7/JXcjr3kHKY2JxUCu0r2JBMwFc0zUO51EnPwJQ=="
#define USER_NAME			"admin"


database_flatfile::database_flatfile( std::string inSettingsFolderPath )
	: mSettingsFolderPath(inSettingsFolderPath)
{
	std::string		settingsFilename( inSettingsFolderPath );
	settingsFilename.append("/settings.ini");
	if( !mSettingsFile.open( settingsFilename ) )
		log( "Error: Couldn't find settings file for database.\n" );
}



user	database_flatfile::user_from_name( std::string inUserName )
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	auto	foundUserID = mNamedUsers.find( inUserName );
	if( foundUserID == mNamedUsers.end() )
		return user();
	
	auto	foundUser = mUsers.find( foundUserID->second );
	if( foundUser == mUsers.end() )
		return user();
	
	return foundUser->second;
}


user	database_flatfile::user_from_id( user_id inUserID )
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	auto	foundUser = mUsers.find( inUserID );
	if( foundUser == mUsers.end() )
		return user();
	
	return foundUser->second;
}


user	database_flatfile::add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags )
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);
	
	// User names must be unique:
	if( mNamedUsers.find(inUserName) != mNamedUsers.end() )
		return user();
	
	// All clear? Get a unique user ID and create a user:
	user_id		newUserID = 1;
	while( mUsers.find(newUserID) != mUsers.end() )
		newUserID++;
	
	if( newUserID == 0 )	// We wrapped around. No more IDs left.
		return user();
	
	user			theUser;
	theUser.mUserID = newUserID;
	theUser.mUserName = inUserName;
	theUser.mPasswordHash = hash(inPassword);
	theUser.mUserFlags = inUserFlags;
	
	mUsers[newUserID] = theUser;
	mNamedUsers[theUser.mUserName] = newUserID;

	log( "Created new user \"%s\" (%d).\n", inUserName.c_str(), newUserID );
	
	if( !save_users() )
		log("Failed to save changed user database.\n");
	
	return theUser;
}


user_id		database_flatfile::id_for_user_name( std::string inUserName )
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	auto	foundUserID = mNamedUsers.find( inUserName );
	if( foundUserID == mNamedUsers.end() )
		return 0;
	
	return foundUserID->second;
}


bool	database_flatfile::delete_user( user_id inUserID )
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	// And delete the user's name (all names for that user ID, in case there are doubles for some reason):
	for( auto currUser = mNamedUsers.begin(); currUser != mNamedUsers.end(); )
	{
		if( currUser->second == inUserID )
		{
			currUser = mNamedUsers.erase(currUser);
		}
		else
			currUser++;
	}
	
	// OK, all that cleared, delete the user's entry:
	auto foundUser = mUsers.find(inUserID);
	mUsers.erase(foundUser);
	
	return save_users();
}


bool	database_flatfile::change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags )
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	auto	foundUser = mUsers.find( inUserID );
	if( foundUser == mUsers.end() )
	{
		log("No user entry for flag change target user %d.\n", inUserID);
		return false;
	}

	std::string	foundUserName( foundUser->second.mUserName );
	
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
	
	return save_users();
}


bool	database_flatfile::kick_user_from_channel( user_id inUserID, std::string channelName )
{
	load_kicklist( channelName );

	std::lock_guard<std::recursive_mutex>		usersLock( mUsersLock );
	std::vector<user_id>&	kickedUsers = mKicklists[channelName];
	
	kickedUsers.push_back( inUserID );
	
	return save_kicklist( channelName );
}


bool	database_flatfile::is_user_kicked_from_channel( user_id inUserID, std::string channelName )
{
	load_kicklist( channelName );
	
	std::lock_guard<std::recursive_mutex>		usersLock( mUsersLock );
	std::vector<user_id>&	kickedUsers = mKicklists[channelName];
	
	for( user_id theUser : kickedUsers )
	{
		if( theUser == inUserID )
			return true;
	}
	
	return false;
}


bool	database_flatfile::load_users()
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	std::string		accountsFileName = mSettingsFile.setting("accountsfilename");
	if( accountsFileName.length() == 0 )
		accountsFileName = "accounts.txt";
	if( accountsFileName[0] != '/' )
		accountsFileName = mSettingsFolderPath + "/" + accountsFileName;
	std::ifstream	file( accountsFileName );
	if( !file.is_open() )
		return false;
	
	while( !file.eof() )
	{
		user			theUser;
		file >> theUser.mUserName;
		
		if( theUser.mUserName.size() == 0 )
			break;
		
		file >> theUser.mPasswordHash;
		file >> theUser.mUserID;
		file >> theUser.mUserFlags;
		
		if( theUser.mUserID == 0 || theUser.mPasswordHash.size() == 0 )
			return false;
		
		mUsers[theUser.mUserID] = theUser;
		mNamedUsers[theUser.mUserName] = theUser.mUserID;
	}
	
	file.close();
	
	return true;
}


bool	database_flatfile::save_users()
{
	std::lock_guard<std::recursive_mutex>		lock(mUsersLock);

	std::string		accountsFileName = mSettingsFile.setting("accountsfilename");
	if( accountsFileName.length() == 0 )
		accountsFileName = "accounts.txt";
	if( accountsFileName[0] != '/' )
		accountsFileName = mSettingsFolderPath + "/" + accountsFileName;
	std::ofstream	file( accountsFileName, std::ios::trunc | std::ios::out );
	if( !file.is_open() )
		return false;
	
	for( auto currUser = mUsers.begin(); currUser != mUsers.end(); currUser++ )
	{
		file << currUser->second.mUserName
			<< " "
			<< currUser->second.mPasswordHash
			<< " "
			<< currUser->second.mUserID
			<< " "
			<< currUser->second.mUserFlags
			<< std::endl;
	}
	
	file.close();
	
	return true;
}


bool	database_flatfile::save_kicklist( std::string channelName )
{
	std::string	settingsFilePath(mSettingsFolderPath);
	if( settingsFilePath.length() > 0 )
		settingsFilePath.append("/channel_");
	else
		settingsFilePath = "channel_";
	settingsFilePath.append( channelName );	// +++ Must filter out slashes!
	settingsFilePath.append( "_kicklist.txt" );
	
	std::ofstream	file( settingsFilePath.c_str(), std::ios::trunc | std::ios::out );
	if( !file.is_open() )
		return false;
	
	{
		std::lock_guard<std::recursive_mutex>		usersLock( mUsersLock );
		std::vector<user_id>&	kickedUsers = mKicklists[channelName];
		for( auto currUser = kickedUsers.begin(); currUser != kickedUsers.end(); currUser++ )
		{
			file << *currUser << std::endl;
		}
	}
	
	file.close();
	
	return true;
}


bool	database_flatfile::load_kicklist( std::string channelName )
{
	{
		std::lock_guard<std::recursive_mutex>		usersLock( mUsersLock );
		std::vector<user_id>&						kickedUsers = mKicklists[channelName];
		if( kickedUsers.size() != 0 )
			return true;	// Already loaded, nothing to do.
	}
	
	std::string	settingsFilePath(mSettingsFolderPath);
	if( settingsFilePath.length() > 0 )
		settingsFilePath.append("/channel_");
	else
		settingsFilePath = "channel_";
	settingsFilePath.append( channelName );	// +++ Must filter out slashes!
	settingsFilePath.append( "_kicklist.txt" );
	
	std::ifstream	file( settingsFilePath );
	if( !file.is_open() )
		return false;
	
	{
		std::lock_guard<std::recursive_mutex>		usersLock( mUsersLock );
		std::vector<user_id>&						kickedUsers = mKicklists[channelName];
		while( !file.eof() )
		{
			user_id			userID = 0;
			
			file >> userID;
			
			if( userID == 0 )
				return false;
			
			kickedUsers.push_back(userID);
		}
	}
	
	file.close();
	
	return true;
}

