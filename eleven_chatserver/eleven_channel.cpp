//
//  eleven_channel.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-02.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_channel.h"
#include "eleven_session.h"


#define MAX_LINE_LENGTH 1024


using namespace eleven;


std::map<std::string,channel*>	channel::channels;

bool	channel::sendln( std::string inMessage )
{
	for( user_id currUser : mUsers )
	{
		session	*	currUserSession = user_session::session_for_user( currUser );
		currUserSession->sendln( inMessage );
	}
	
	return false;
}


bool	channel::printf( const char* inFormatString, ... )
{
	char	replyString[MAX_LINE_LENGTH];
	replyString[sizeof(replyString) -1] = 0;    // snprintf doesn't terminate if text length is >= buffer size, so terminate manually and give it one less byte of buffer to work with so it doesn't overwrite us.
	
	va_list		args;
	va_start( args, inFormatString );
	vsnprintf( replyString, sizeof(replyString) -1, inFormatString, args );
	va_end(args);
	
	return sendln( std::string(replyString) );	// Send!
}


bool	channel::join_channel( session* inSession, user_id inUserID, user_session* userSession )
{
	// Check whether user is still logged in and hasn't been blocked since login:
	if( userSession->current_user() == 0 )
		return false;
	
	user_flags		theFlags = userSession->my_user_flags();
	
	if( (theFlags & USER_FLAG_BLOCKED)
		|| (theFlags & USER_FLAG_RETIRED) )
		return false;
	
	// +++ Check whether user is blocked only for this room & reject in that case.
	
	// Take note of users that are already in a room:
	bool		alreadyInRoom = false;
	for( auto currUserID : mUsers )
	{
		if( currUserID == inUserID )
		{
			alreadyInRoom = true;
			break;
		}
	}
	
	// Now tell channel about us:
	if( !alreadyInRoom )
		mUsers.push_back(inUserID);
	
	// Remember in our session what channel is current:
	current_channel* channelInfo = new current_channel;
	channelInfo->mChannelName = mChannelName;
	inSession->attach_sessiondata( CHANNEL_SESSION_DATA_ID, channelInfo );
	
	// Tell everyone else in this channel that we're here:
	if( !alreadyInRoom )
		printf( "JOIN:User %s joined the channel.", userSession->name_for_user_id(inUserID).c_str() );
	
	return true;
}


void	channel::leave_channel( session* inSession, user_id inUserID, user_session* userSession )
{
	// If user's "current" channel is this one, make it no longer current:
	//	(This drops us back into what is effectively our IRC console)
	current_channel* channelInfo = (current_channel*)inSession->find_sessiondata(CHANNEL_SESSION_DATA_ID);
	if( channelInfo && channelInfo->mChannelName.compare(mChannelName) == 0 )
	{
		inSession->remove_sessiondata(CHANNEL_SESSION_DATA_ID);
	}
	
	// Remove the user from our list of users to broadcast to:
	for( auto currUserItty = mUsers.begin(); currUserItty != mUsers.end(); )
	{
		if( (*currUserItty) == inUserID )
		{
			currUserItty = mUsers.erase(currUserItty);
		}
		else
			currUserItty++;
	}
	
	// Tell everyone else it's now safe to poke fun at that user:
	printf( "GONE:User %s has left the channel.", userSession->name_for_user_id(inUserID).c_str() );
}


handler	channel::join_channel_handler = [](session* inSession, std::string inCommand)
{
	size_t currOffset = 0;
	session::next_word( inCommand, currOffset );
	std::string	channelName = session::next_word( inCommand, currOffset );
	
	if( channelName.size() == 0 )
	{
		inSession->sendln( "!NME:You need to give the name of a channel to join." );
		return;
	}
	
	user_session*	theUserSession = (user_session*)inSession->find_sessiondata( USER_SESSION_DATA_ID );
	if( !theUserSession )
	{
		inSession->sendln( "!AUT:You need to be logged in to join a channel." );
		return;
	}

	channel*	theChannel = NULL;
	auto channelItty = channels.find(channelName);
	if( channelItty == channels.end() )
	{
		theChannel = new channel( channelName );
		channels[channelName] = theChannel;
	}
	else
		theChannel = channelItty->second;

	theChannel->join_channel( inSession, theUserSession->current_user(), theUserSession );
};


handler	channel::leave_channel_handler = [](session* inSession, std::string inCommand)
{
	size_t currOffset = 0;
	session::next_word( inCommand, currOffset );
	std::string	channelName = session::next_word( inCommand, currOffset );
	
	user_session*	theUserSession = (user_session*)inSession->find_sessiondata( USER_SESSION_DATA_ID );
	if( !theUserSession )
	{
		inSession->sendln( "!AUT:You need to be logged in to leave a channel." );
		return;
	}

	// No channel name given? Leave the current channel:
	if( channelName.size() == 0 )
	{
		current_channel* channelInfo = (current_channel*)inSession->find_sessiondata(CHANNEL_SESSION_DATA_ID);
		if( channelInfo )
			channelName = channelInfo->mChannelName;
		else
		{
			inSession->sendln( "!NME:You need to give the name of a channel to leave." );
			return;
		}
	}
	
	// Look up channel and leave it:
	auto	channelItty = channels.find( channelName );
	if( channelItty != channels.end() )
	{
		channelItty->second->leave_channel( inSession, theUserSession->current_user(), theUserSession );
	}
};


handler	channel::chat_handler = [](session* inSession, std::string inCommand)
{
	user_session*	theUserSession = (user_session*)inSession->find_sessiondata( USER_SESSION_DATA_ID );
	if( !theUserSession )
	{
		inSession->sendln( "!AUT:You need to be logged in to chat on a channel." );
		return;
	}

	// Make sure we leave any previous channels:
	current_channel* channelInfo = (current_channel*)inSession->find_sessiondata(CHANNEL_SESSION_DATA_ID);
	if( !channelInfo )
	{
		inSession->printf("!WHU:%s\r\n",inCommand.c_str());
		return;
	}

	channel*	theChannel = NULL;
	auto channelItty = channels.find(channelInfo->mChannelName);
	if( channelItty == channels.end() )
	{
		theChannel = new channel( channelInfo->mChannelName );
		channels[channelInfo->mChannelName] = theChannel;
	}
	else
		theChannel = channelItty->second;

	theChannel->printf("MESG: %s %s %s", channelInfo->mChannelName.c_str(), theUserSession->my_user_name().c_str(), inCommand.c_str());
};

