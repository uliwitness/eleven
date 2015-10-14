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
#include "eleven_log.h"
#include "eleven_database_flatfile.h"
#include <sys/param.h>

//void  INThandler(int sig)
//{
//	signal( sig, SIG_IGN );
//	printf ("Got a SIGINT\n" );
//	signal( SIGINT, INThandler );	// Install ourselves again so we get called next time.
//	exit(0);
//}
//
//
//void  ABRThandler(int sig)
//{
//	signal( sig, SIG_IGN );
//	printf ("Got a SIGABRT\n" );
//	signal( SIGABRT, ABRThandler );	// Install ourselves again so we get called next time.
//}
//
//
//void  PIPEhandler(int sig)
//{
//	signal( sig, SIG_IGN );
//	printf ("Got a SIGPIPE\n" );
//	signal( SIGPIPE, PIPEhandler );	// Install ourselves again so we get called next time.
//}


int main( int argc, char** argv )
{
	try
	{
//		signal( SIGINT, INThandler );
//		signal( SIGABRT, ABRThandler );
//		signal( SIGPIPE, PIPEhandler );
		
		const char*		settingsFolderPath = ".";
		if( argc > 1 )
		{
			settingsFolderPath = argv[1];
		}
		
		eleven::database_flatfile	theDatabase( settingsFolderPath );
		eleven::chatserver      	server( settingsFolderPath, 13762 );
		
		if( server.valid() )
		{
			eleven::user_session::set_user_database( &theDatabase );
			
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
			server.register_command_handler( "/logout", []( eleven::session_ptr session, std::string currRequest, eleven::chatserver* server )
			{
				session->printf( "/logged_out Logging you out.\n" );
				
				session->disconnect();
			} );
			// /howdy
			server.register_command_handler( "/version", []( eleven::session_ptr session, std::string currRequest, eleven::chatserver* server )
			{
				session->printf( "/version 1.0 eleven_chatserver_example\n" );
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
			
			eleven::log( "Listening on port %d\n", server.port_number() );
			server.wait_for_connection();
		}
	}
	catch( std::exception& exc )
	{
		eleven::log("Exception %s terminating server.\n", exc.what());
	}
	catch( ... )
	{
		eleven::log("Unknown exception terminating server.\n");
	}

    return 0;
}