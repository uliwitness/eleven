//
//  eleven_database_flatfile.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-25.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectserver__eleven_database_flatfile__
#define __interconnectserver__eleven_database_flatfile__


#include "eleven_ini_file.h"
#include "eleven_database.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>


namespace eleven
{

	class database_flatfile : public database
	{
	public:
		database_flatfile( std::string inSettingsFolderPath );
		
		virtual user	user_from_id( user_id inUserID );
		virtual user	user_from_name( std::string inUserName );
		virtual user	add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags = 0 );
		virtual user_id	id_for_user_name( std::string inUserName );
		virtual bool	change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags );
		virtual bool	delete_user( user_id inUserID );
		virtual bool	kick_user_from_channel( user_id inUserID, std::string channelName );
		virtual bool	is_user_kicked_from_channel( user_id inUserID, std::string channelName );

		bool	load_users();
		bool	save_users();
		bool	load_kicklist( std::string channelName );
		bool	save_kicklist( std::string channelName );
		
	protected:
		ini_file									mSettingsFile;
		std::string									mSettingsFolderPath;
		std::recursive_mutex						mUsersLock;
		std::map<user_id,user>						mUsers;
		std::map<std::string,user_id>				mNamedUsers;
		std::map<std::string,std::vector<user_id>>	mKicklists;
	};

}

#endif /* defined(__interconnectserver__eleven_database_flatfile__) */
