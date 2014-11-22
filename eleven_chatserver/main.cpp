//
//  main.cpp
//  eleven_chatserver
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatserver.h"
#include "eleven_users.h"
#include "eleven_channel.h"
#include <sys/param.h>


int main( int argc, char** argv )
{
	const char*		settingsFolderPath = ".";
	if( argc > 1 )
	{
		settingsFolderPath = argv[1];
	}
	
	eleven::chatserver       server( settingsFolderPath, 13762 );
    
    if( server.valid() )
    {
		if( !eleven::user_session::load_users( settingsFolderPath ) )
		{
			fprintf(stderr, "Can't find account database file accounts.txt at %s.\n", settingsFolderPath);
			return 100;
		}
		
		// /login <userName> <password>
		server.register_command_handler( "/login", eleven::user_session::login_handler );
		// /adduser <userName> <password> <confirmPassword> [moderator] [owner]
		server.register_command_handler( "/adduser", eleven::user_session::adduser_handler );
		// /deleteuser <userName> <confirmUserName>
		server.register_command_handler( "/deleteuser", eleven::user_session::deleteuser_handler );
		// /blockuser <userName> <confirmUserName>
		server.register_command_handler( "/blockuser", eleven::user_session::blockuser_handler );
		// /retireuser <userName> <confirmUserName>
		server.register_command_handler( "/retireuser", eleven::user_session::retireuser_handler );
		// /makemoderator <userName>
		server.register_command_handler( "/makemoderator", eleven::user_session::makemoderator_handler );
		// /makeowner <userName>
		server.register_command_handler( "/makeowner", eleven::user_session::makeowner_handler );
		// /bye
		server.register_command_handler( "/bye", []( eleven::session_ptr session, std::string currRequest, eleven::chatserver* server )
		{
			session->printf( "/bye Logging you out.\n" );
			
			session->log_out();
		} );
		// /howdy
		server.register_command_handler( "/howdy", []( eleven::session_ptr session, std::string currRequest, eleven::chatserver* server )
		{
			session->printf( "/welcome Welcome!\n" );
		} );
		server.register_command_handler( "/shutdown", eleven::user_session::shutdown_handler );
		// /join <channelName>
		server.register_command_handler( "/join", eleven::channel::join_channel_handler );
		// /leave [<channelName>]
		server.register_command_handler( "/leave", eleven::channel::leave_channel_handler );
		// /kick [<channelName>] <userName>
		server.register_command_handler( "/kick", eleven::channel::kick_handler );
		// <anything that's not a recognized command>
		server.register_command_handler( "*", eleven::channel::chat_handler );
		
        printf( "Listening on port %d\n", server.port_number() );
        server.wait_for_connection();
    }

    return 0;
}