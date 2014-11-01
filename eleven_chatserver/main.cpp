//
//  main.cpp
//  eleven_chatserver
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatserver.h"


int main()
{
    eleven::chatserver       server( 13762 );
    
    if( server.valid() )
    {
        printf( "Listening on port %d\n", server.port_number() );
		server.register_command_handler( "/bye", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "Nice to speak with you.\n" );
			return false;	// Terminate session!
		} );
		server.register_command_handler( "/howdy", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "Welcome!\n" );
			return true;
		} );
		server.register_command_handler( "*", []( eleven::session* session, std::string currRequest )
		{
			session->printf( "JUNK:%s\n", currRequest.c_str() );
			return true;
		} );
        server.wait_for_connection();
    }

    return 0;
}