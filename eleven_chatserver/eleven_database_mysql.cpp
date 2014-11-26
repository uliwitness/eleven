//
//  eleven_database_mysql.cpp
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-25.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_database_mysql.h"
#include "eleven_log.h"
#include "eleven_ini_file.h"

using namespace std;
using namespace eleven;


#define PASSWORD_HASH		"$s1$0e0804$GJqUPV/GowGPc1myVgovRg==$7at14hpizppp5LWApvwC5DnUX5bxtjNYk79dutroHsu0lKc7/JXcjr3kHKY2JxUCu0r2JBMwFc0zUO51EnPwJQ=="
#define USER_NAME			"admin"


database_mysql::database_mysql( std::string inSettingsFolderPath )
{
	std::string		settingsFilename( inSettingsFolderPath );
	settingsFilename.append("/settings.ini");
	if( !mSettingsFile.open( settingsFilename ) )
		log( "Error: Couldn't find settings file for database.\n" );
	
	try
	{
		// Create a connection:
		mDriver = get_driver_instance();
		mConnection = mDriver->connect( mSettingsFile.setting("dbserver"), mSettingsFile.setting("dbuser"), mSettingsFile.setting("dbpassword") );

		// Connect to the database:
		mConnection->setSchema( mSettingsFile.setting("dbname") );
	}
	catch( sql::SQLException &e )
	{
		log( "Error opening database: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
	}
}


bool	database_mysql::change_user_flags( user_id inUserID, user_flags inSetFlags, user_flags inClearFlags )
{
	sql::Statement	*stmt = NULL;

	try
	{
		sql::SQLString	query( "UPDATE TABLE users SET" );
		bool			isFirst = true;
		
		if( inSetFlags & USER_FLAG_MODERATOR )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " moderator=true";
		}
		if( inSetFlags & USER_FLAG_SERVER_OWNER )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " owner=true";
		}
		if( inSetFlags & USER_FLAG_BLOCKED )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " blocked=true";
		}
		if( inSetFlags & USER_FLAG_RETIRED )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " retired=true";
		}

		if( inClearFlags & USER_FLAG_MODERATOR )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " moderator=false";
		}
		if( inClearFlags & USER_FLAG_SERVER_OWNER )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " owner=false";
		}
		if( inClearFlags & USER_FLAG_BLOCKED )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " blocked=false";
		}
		if( inClearFlags & USER_FLAG_RETIRED )
		{
			if( isFirst )
				isFirst = false;
			else
				query += ",";
			query += " retired=false";
		}
	
		query += " WHERE id=";
		query += std::to_string(inUserID);
		stmt = mConnection->createStatement();
		stmt->execute( query );
		delete stmt;
		
		return true;
	}
	catch (sql::SQLException &e)
	{
		log( "Error changing user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
		return false;
	}
}



user	database_mysql::user_from_id( user_id inUserID )
{
	sql::Statement	*stmt = NULL;
	sql::ResultSet	*res = NULL;

	try
	{
		stmt = mConnection->createStatement();
		res = stmt->executeQuery( "SELECT * FROM users" );
		while( res->next() )
		{
			user	foundUser;
			foundUser.mUserName = res->getString("name");
			foundUser.mPasswordHash = res->getString("password");
			foundUser.mUserFlags = (res->getInt("blocked") ? USER_FLAG_BLOCKED : 0) | (res->getInt("retired") ? USER_FLAG_RETIRED : 0) | (res->getInt("owner") ? USER_FLAG_SERVER_OWNER : 0) | (res->getInt("moderator") ? USER_FLAG_MODERATOR : 0);
			return foundUser;
		}
		delete res;
		delete stmt;
	}
	catch (sql::SQLException &e)
	{
		if( e.getErrorCode() == 1146 )	// No such table? Create it!
		{
			try
			{
				stmt = mConnection->createStatement();
				stmt->execute(	"CREATE TABLE users\n"
								"(\n"
								"id int,\n"
								"name CHAR(255),\n"
								"password CHAR(255),\n"
								"blocked BOOL,\n"
								"retired BOOL,\n"
								"owner BOOL,\n"
								"moderator BOOL\n"
								")\n");
				delete stmt;
				stmt = mConnection->createStatement();
				stmt->execute(	"INSERT INTO users (\n"
								"`id`,\n"
								"`name` ,\n"
								"`password`,\n"
								"`blocked`,\n"
								"`retired`,\n"
								"`owner`,\n"
								"`moderator`\n"
								")\n"
								"VALUES (\n"
								"'1', '" USER_NAME "', '" PASSWORD_HASH "', false, false, true, false\n"
								");");
				delete stmt;
				
				if( inUserID == 1 )
				{
					user newUser;
					newUser.mUserName = USER_NAME;
					newUser.mPasswordHash = PASSWORD_HASH;
					newUser.mUserFlags = USER_FLAG_SERVER_OWNER;
				}
				else
					return user();
			}
			catch(sql::SQLException &e2)
			{
				log( "Error reading user database: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
			}
		}
		else
		{
			log( "Error finding user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
		}
	}

	return user();
}