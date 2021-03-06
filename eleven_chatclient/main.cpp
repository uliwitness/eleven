//
//  main.cpp
//  eleven_chatclient
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

/*
	Syntax eleven_chatclient <settingsFolderPath> [<username> <password>]
	
	You can launch several instances of this program to see how chat
	communication works, by logging in as several users. You should then
	receive "Hello everyone in this room!" on both.
*/

#include "eleven_session.h"
#include "eleven_chatclient.h"
#include <string>
#include <unistd.h>


int main( int argc, const char** argv )
{
	const char*		usernameForLogin = "admin";
	const char*		passwordForLogin = "eleven";
	const char*		settingsFolderPath = ".";
	if( argc > 1 )
	{
		settingsFolderPath = argv[1];
	}
	if( argc > 3 )
	{
		usernameForLogin = argv[2];
		passwordForLogin = argv[3];
	}

	eleven::chatclient	client( "localhost", 13762, settingsFolderPath );
	
	if( client.connect() )
	{
		client.register_message_handler( "*", []( eleven::session_ptr inSession, std::string currLine, eleven::chatclient* inClient )
		{
			printf( "%s\n", currLine.c_str() );
		});
		client.register_message_handler( "/logged_out", []( eleven::session_ptr inSession, std::string currLine, eleven::chatclient* inClient )
		{
			inSession->disconnect();
		});
		client.listen_for_messages();
		
		eleven::session_ptr		theSession( client.current_session() );
		theSession->sendln( "/version" );
		theSession->printf( "/login %s %s\r\n", usernameForLogin, passwordForLogin );
		#if 0
		theSession->sendln( "/join myfavoriteroom" );
		theSession->sendln( "Hello everyone in this room!" );
		sleep(5);	// Wait so we get messages from other users in this room queued up.
		theSession->sendln( "/leave myfavoriteroom" );
//		theSession->sendln( "/shutdown" );	// Just for testing the shutdown command.
		theSession->sendln( "/logout" );
		
		while( theSession->keep_running() )	// Busy-wait until server cuts the connection in response to our "/logout".
			;
		#else
		while( theSession->keep_running() )
		{
			size_t	len = 0;
			char* msg = fgetln( stdin, &len );
			if( msg && len > 0 )
			{
				msg[len -1] = 0;
				theSession->sendln(msg);
			}
			clearerr(stdin);
		}
		#endif
	}
	
	printf("Terminate.\n");
	
	return 0;
}