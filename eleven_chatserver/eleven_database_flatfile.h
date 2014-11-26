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
#include <mutex>


namespace eleven
{

	class database_flatfile : public database
	{
	public:
		database_flatfile( std::string inSettingsFolderPath );
		
		user	user_from_id( user_id inUserID );
		user	user_from_name( std::string inUserName );
		bool	add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags = 0 );
		user_id	id_for_user_name( std::string inUserName );
		bool	change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags );

		bool	load_users();
		bool	save_users();
		
	protected:
		ini_file						mSettingsFile;
		std::string						mSettingsFolderPath;
		std::recursive_mutex			mUsersLock;
		std::map<user_id,user>			mUsers;
		std::map<std::string,user_id>	mNamedUsers;
	};

}

#endif /* defined(__interconnectserver__eleven_database_flatfile__) */
