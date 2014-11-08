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


int main()
{
    eleven::chatserver       server( 13762 );
    
    if( server.valid() )
    {
		if( !eleven::user_session::load_users( "accounts.txt" ) )
		{
			fprintf(stderr, "Can't find account database file.\n");
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
		server.register_command_handler( "/bye", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "YEAH:Nice to speak with you.\n" );
			
			session->log_out();
		} );
		// /howdy
		server.register_command_handler( "/howdy", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "YEAH:Welcome!\n" );
		} );
		// /join <channelName>
		server.register_command_handler( "/join", eleven::channel::join_channel_handler );
		// /leave [<channelName>]
		server.register_command_handler( "/leave", eleven::channel::leave_channel_handler );
		// /kick [<channelName>] <userName>
		server.register_command_handler( "/kick", eleven::channel::kick_handler );
		// <anything that's not a recognized command>
		server.register_command_handler( "*", eleven::channel::chat_handler );
		
        printf( "NOTE:Listening on port %d\n", server.port_number() );
        server.wait_for_connection();
    }

    return 0;
}