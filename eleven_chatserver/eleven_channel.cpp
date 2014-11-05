//
//  eleven_channel.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-02.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_channel.h"
#include "eleven_session.h"


using namespace eleven;


bool	channel::sendln( std::string inMessage )
{
	for( user_id currUser : mUsers )
	{
		session	*	currUserSession = session_for_user( currUser );
		currUserSession->sendln( inMessage );
	}
	
	return false;
}


