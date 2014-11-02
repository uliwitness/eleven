//
//  main.cpp
//  eleven_chatserver
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatserver.h"
#include "eleven_users.h"


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
		server.register_command_handler( "/login", eleven::user_session::login_handler );
		server.register_command_handler( "/adduser", eleven::user_session::adduser_handler );
		server.register_command_handler( "/deleteuser", eleven::user_session::deleteuser_handler );
		server.register_command_handler( "/blockuser", eleven::user_session::blockuser_handler );
		server.register_command_handler( "/retireuser", eleven::user_session::retireuser_handler );
		server.register_command_handler( "/makemoderator", eleven::user_session::makemoderator_handler );
		server.register_command_handler( "/makeowner", eleven::user_session::makeowner_handler );
		server.register_command_handler( "/bye", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "YEAH:Nice to speak with you.\n" );
			
			session->log_out();
		} );
		server.register_command_handler( "/howdy", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "YEAH:Welcome!\n" );
		} );
		server.register_command_handler( "*", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "!WHU:%s\n", currRequest.c_str() );
		} );
        printf( "NOTE:Listening on port %d\n", server.port_number() );
        server.wait_for_connection();
    }

    return 0;
}