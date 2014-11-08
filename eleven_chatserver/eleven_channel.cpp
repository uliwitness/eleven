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
	// +++ Reject banned users and users that are already in the room.
	
	// Make sure we leave any previous channels:
	current_channel* channelInfo = (current_channel*)inSession->find_sessiondata(CHANNEL_SESSION_DATA_ID);
	if( channelInfo )
	{
		auto	channelItty = channels.find( channelInfo->mChannelName );
		channelItty->second->leave_channel( inSession, inUserID, userSession );
	}
	
	// Now tell channel about us and remember in our session what channel is current:
	mUsers.push_back(inUserID);
	
	channelInfo = new current_channel;
	channelInfo->mChannelName = mChannelName;
	inSession->attach_sessiondata( CHANNEL_SESSION_DATA_ID, channelInfo );
	
	// Tell everyone else in this channel that we're here:
	printf( "JOIN:User %s joined the channel.", userSession->name_for_user_id(inUserID).c_str() );
	
	return true;
}


void	channel::leave_channel( session* inSession, user_id inUserID, user_session* userSession )
{
	// +++ Remove user from room and if it was last user close channel.
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
		inSession->printf("!WHU:%s",inCommand.c_str());
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

	theChannel->sendln(inCommand);
};

