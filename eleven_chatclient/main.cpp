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
	
	if( client.valid() )
	{
		client.current_session()->sendln( "/howdy" );
		client.current_session()->printf( "/login %s %s\r\n", usernameForLogin, passwordForLogin );
		client.current_session()->sendln( "/join myfavoriteroom" );
		client.current_session()->sendln( "Hello everyone in this room!" );
		sleep(5);	// Wait so we get messages from other users in this room queued up.
		client.current_session()->sendln( "/leave myfavoriteroom" );
//		client.current_session()->sendln( "/shutdown" );	// Just for testing the shutdown command.
//		client.current_session()->sendln( "/bye" );
		
		client.listen_for_messages( []( eleven::session_ptr inSession, std::string currLine, eleven::chatclient* inClient )
		{
			printf( "Answer received: %s\n", currLine.c_str() );
		});
		
		
		while( client.current_session()->keep_running() )
			;
		
		printf("No more messages, quitting\n");
	}
	
	return 0;
}