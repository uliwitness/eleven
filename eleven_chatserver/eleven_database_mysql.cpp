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
#include <cppconn/prepared_statement.h>

using namespace std;
using namespace eleven;


// Default username and password used to set up a fresh database's tables:
#define PASSWORD_HASH		"$s1$0e0804$GJqUPV/GowGPc1myVgovRg==$7at14hpizppp5LWApvwC5DnUX5bxtjNYk79dutroHsu0lKc7/JXcjr3kHKY2JxUCu0r2JBMwFc0zUO51EnPwJQ=="	// "eleven"
#define USER_NAME			"admin"


database_mysql::database_mysql( std::string inSettingsFolderPath )
	: mConnection(NULL), mDriver(NULL)
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
	sql::PreparedStatement	*stmt = NULL;
	sql::ResultSet			*res = NULL;

	try
	{
		stmt = mConnection->prepareStatement( "SELECT * FROM users WHERE id=?" );
		stmt->setInt( 1, inUserID );
		res = stmt->executeQuery();
		while( res->next() )
		{
			user	foundUser;
			foundUser.mUserID = res->getInt("id");
			foundUser.mUserName = res->getString("name");
			foundUser.mPasswordHash = res->getString("password");
			foundUser.mUserFlags = (res->getInt("blocked") ? USER_FLAG_BLOCKED : 0) | (res->getInt("retired") ? USER_FLAG_RETIRED : 0) | (res->getInt("owner") ? USER_FLAG_SERVER_OWNER : 0) | (res->getInt("moderator") ? USER_FLAG_MODERATOR : 0);
			delete res;
			delete stmt;
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
				sql::Statement *stmt2 = mConnection->createStatement();
				stmt2->execute(	"CREATE TABLE users\n"
								"(\n"
								"id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,\n"
								"name CHAR(255) NOT NULL UNIQUE,\n"
								"password CHAR(255),\n"
								"blocked BOOL,\n"
								"retired BOOL,\n"
								"owner BOOL,\n"
								"moderator BOOL\n"
								");\n");
				delete stmt2;
				stmt = mConnection->prepareStatement(
								"INSERT INTO users (\n"
								"`name`,\n"
								"`password`,\n"
								"`blocked`,\n"
								"`retired`,\n"
								"`owner`,\n"
								"`moderator`\n"
								")\n"
								"VALUES (\n"
								"?, ?, false, false, true, false\n"
								");");
				stmt->setString( 1, USER_NAME );
				stmt->setString( 2, PASSWORD_HASH );
				stmt->execute();
				delete stmt;
				
				if( inUserID == 1 )
				{
					user newUser;
					newUser.mUserID = inUserID;
					newUser.mUserName = USER_NAME;
					newUser.mPasswordHash = PASSWORD_HASH;
					newUser.mUserFlags = USER_FLAG_SERVER_OWNER;
					return newUser;
				}
				else
					return user();
			}
			catch(sql::SQLException &e2)
			{
				log( "Error reading user database: %s (code=%d state=%s)\n", e2.what(), e2.getErrorCode(), e2.getSQLState().c_str() );
			}
		}
		else
		{
			log( "Error finding user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
		}
	}

	return user();
}


user	database_mysql::user_from_name( std::string inUserName )
{
	sql::PreparedStatement	*stmt = NULL;
	sql::ResultSet			*res = NULL;

	try
	{
		stmt = mConnection->prepareStatement( "SELECT * FROM users WHERE name=?" );
		stmt->setString( 1, inUserName );
		res = stmt->executeQuery();
		while( res->next() )
		{
			user	foundUser;
			foundUser.mUserID = res->getInt("id");
			foundUser.mUserName = res->getString("name");
			foundUser.mPasswordHash = res->getString("password");
			foundUser.mUserFlags = (res->getInt("blocked") ? USER_FLAG_BLOCKED : 0) | (res->getInt("retired") ? USER_FLAG_RETIRED : 0) | (res->getInt("owner") ? USER_FLAG_SERVER_OWNER : 0) | (res->getInt("moderator") ? USER_FLAG_MODERATOR : 0);
			delete res;
			delete stmt;
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
				sql::Statement *stmt2 = mConnection->createStatement();
				stmt2->execute(	"CREATE TABLE users\n"
								"(\n"
								"id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,\n"
								"name CHAR(255) NOT NULL UNIQUE,\n"
								"password CHAR(255),\n"
								"blocked BOOL,\n"
								"retired BOOL,\n"
								"owner BOOL,\n"
								"moderator BOOL\n"
								");\n");
				delete stmt2;
				stmt = mConnection->prepareStatement(
								"INSERT INTO users (\n"
								"name,\n"
								"password,\n"
								"blocked,\n"
								"retired,\n"
								"owner,\n"
								"moderator\n"
								")\n"
								"VALUES (\n"
								"?, ?, false, false, true, false\n"
								");");
				stmt->setString( 1, USER_NAME );
				stmt->setString( 2, PASSWORD_HASH );
				stmt->execute();
				delete stmt;
				
				if( inUserName.compare(USER_NAME) == 0 )
				{
					user newUser;
					newUser.mUserID = 1;
					newUser.mUserName = USER_NAME;
					newUser.mPasswordHash = PASSWORD_HASH;
					newUser.mUserFlags = USER_FLAG_SERVER_OWNER;
					return newUser;
				}
				else
					return user();
			}
			catch(sql::SQLException &e2)
			{
				log( "Error reading user database: %s (code=%d state=%s)\n", e2.what(), e2.getErrorCode(), e2.getSQLState().c_str() );
			}
		}
		else
		{
			log( "Error finding user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
		}
	}

	return user();
}


user	database_mysql::add_user( std::string inUserName, std::string inPassword, user_flags inUserFlags )
{
	sql::PreparedStatement	*stmt = NULL;

	try
	{
		stmt = mConnection->prepareStatement(	"INSERT INTO users (\n"
												"`name` ,\n"
												"`password`,\n"
												"`blocked`,\n"
												"`retired`,\n"
												"`owner`,\n"
												"`moderator`\n"
												")\n"
												"VALUES (\n"
												"?,\n"
												"?,\n"
												"?, ?, ?, ?\n"
												");" );
		stmt->setString( 1, inUserName );
		stmt->setString( 2, hash(inPassword) );
		stmt->setBoolean( 3, (inUserFlags & USER_FLAG_BLOCKED) );
		stmt->setBoolean( 4, (inUserFlags & USER_FLAG_RETIRED) );
		stmt->setBoolean( 5, (inUserFlags & USER_FLAG_SERVER_OWNER) );
		stmt->setBoolean( 6, (inUserFlags & USER_FLAG_MODERATOR) );
		stmt->execute();
		delete stmt;
		
		return user_from_name( inUserName );
	}
	catch(sql::SQLException &e)
	{
		log( "Error creating user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
	}

	return user();
}


bool	database_mysql::delete_user( user_id inUserID )
{
	sql::PreparedStatement	*stmt = NULL;

	try
	{
		stmt = mConnection->prepareStatement( "DELETE FROM users WHERE id=?" );
		stmt->setInt( 1, inUserID );
		stmt->execute();
		delete stmt;
		
		return true;
	}
	catch (sql::SQLException &e)
	{
		log( "Error deleting user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
		return false;
	}
}


bool	database_mysql::kick_user_from_channel( user_id inUserID, std::string channelName )
{
	sql::PreparedStatement	*stmt = NULL;

	try
	{
		stmt = mConnection->prepareStatement( "INSERT INTO kickedfromchannel ( userid, channelname ) VALUES ( ?, ? );" );
		stmt->setInt( 1, inUserID );
		stmt->setString( 2, channelName );
		stmt->execute();
		delete stmt;
	}
	catch (sql::SQLException &e)
	{
		if( e.getErrorCode() == 1146 )	// No such table? Create it!
		{
			try
			{
				sql::Statement *stmt2 = mConnection->createStatement();
				stmt2->execute(	"CREATE TABLE kickedfromchannel\n"
								"(\n"
								"userid INT NOT NULL FOREIGN KEY,\n"
								"channelname CHAR(255) NOT NULL\n"
								")\n");
				delete stmt2;
				stmt = mConnection->prepareStatement( "INSERT INTO kickedfromchannel ( userid, channelname ) VALUES ( ?, ? );" );
				stmt->setInt( 1, inUserID );
				stmt->setString( 2, channelName );
				stmt->execute();
				delete stmt;
			}
			catch(sql::SQLException &e2)
			{
				log( "Error reading user database: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
				return false;
			}
		}
		else
		{
			log( "Error finding user: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
			return false;
		}
	}

	return true;
}


bool	database_mysql::is_user_kicked_from_channel( user_id inUserID, std::string channelName )
{
	sql::PreparedStatement	*stmt = NULL;
	sql::ResultSet			*res = NULL;

	try
	{
		stmt = mConnection->prepareStatement( "SELECT * FROM kickedfromchannel WHERE userid=? AND channelname=?" );
		stmt->setInt( 1, inUserID );
		stmt->setString( 2, channelName );
		res = stmt->executeQuery();
		while( res->next() )
		{
			delete res;
			delete stmt;
			return true;
		}
		delete res;
		delete stmt;
	}
	catch (sql::SQLException &e)
	{
		if( e.getErrorCode() == 1146 )	// No such table? Nobody blocked yet.
		{
			return false;
		}
		else
		{
			log( "Error finding if user was kicked: %s (code=%d state=%s)\n", e.what(), e.getErrorCode(), e.getSQLState().c_str() );
			return true;	// +++ On any other error, should we really just block even valid users, or should we let in even the bad guys?
		}
	}
	
	return false;
}

