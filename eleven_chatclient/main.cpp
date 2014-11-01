//
//  main.cpp
//  eleven_chatclient
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_session.h"
#include "eleven_chatclient.h"
#include <string>


int main()
{
	eleven::chatclient	client( "localhost", 13762 );
	
	if( client.valid() )
	{
		std::string outString;
		client.current_session()->reply_from_printfln( outString, "/howdy" );
		printf( "Answer to Howdy was: %s\n", outString.c_str() );
		client.current_session()->reply_from_printfln( outString, "/bye" );
		printf( "Answer to Bye was: %s\n", outString.c_str() );
	}
	
	return 0;
}