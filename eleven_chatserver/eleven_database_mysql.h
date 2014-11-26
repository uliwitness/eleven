//
//  eleven_database_mysql.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-25.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectserver__eleven_database_mysql__
#define __interconnectserver__eleven_database_mysql__


#include "eleven_ini_file.h"
#include "eleven_database.h"
#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"
#include <string>


namespace eleven
{

	class database_mysql : public database
	{
	public:
		database_mysql( std::string inSettingsFolderPath );
		
		user	user_from_id( user_id inUserID );
		user	user_from_name( std::string inUserName );
		bool	add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags = 0 );
		user_id	id_for_user_name( std::string inUserName );
		void	change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags );
		
	protected:
		ini_file			mSettingsFile;
		sql::Driver*		mDriver;
		sql::Connection*	mConnection;
	};

}

#endif /* defined(__interconnectserver__eleven_database_mysql__) */
