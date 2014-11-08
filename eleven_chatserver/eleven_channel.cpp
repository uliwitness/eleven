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
	
	// Check whether user is blocked only for this room:
	for( auto currUserID : mKickedUsers )
	{
		if( currUserID == inUserID )
			return false;
	}
	
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


bool	channel::leave_channel( session* inSession, user_id inUserID, user_session* userSession, std::string inBlockedForReason )
{
	// If user's "current" channel is this one, make it no longer current:
	//	(This drops us back into what is effectively our IRC console)
	session*		targetSession = user_session::session_for_user(inUserID);
	if( targetSession )
	{
		current_channel* channelInfo = (current_channel*)targetSession->find_sessiondata(CHANNEL_SESSION_DATA_ID);
		if( channelInfo && channelInfo->mChannelName.compare(mChannelName) == 0 )
		{
			targetSession->remove_sessiondata(CHANNEL_SESSION_DATA_ID);
		}
	}
	
	// Remove the user from our list of users to broadcast to:
	bool		wasInChannel = false;
	for( auto currUserItty = mUsers.begin(); currUserItty != mUsers.end(); )
	{
		if( (*currUserItty) == inUserID )
		{
			currUserItty = mUsers.erase(currUserItty);
			wasInChannel = true;
		}
		else
			currUserItty++;
	}
	
	if( !wasInChannel )
		return false;
	
	// Tell everyone else it's now safe to poke fun at that user:
	if( inBlockedForReason.size() > 0 )
	{
		printf( "BLOK:User %s has been kicked from the channel: %s", userSession->name_for_user_id(inUserID).c_str(), inBlockedForReason.c_str() );
		if( targetSession )
			targetSession->printf("BLOK:You have been blocked: %s\r\n", inBlockedForReason.c_str());
	}
	else
	{
		printf( "GONE:User %s has left the channel.", userSession->name_for_user_id(inUserID).c_str() );
	}
	
	return true;
}


bool	channel::kick_user( session* inSession, user_id inTargetUserID, user_session* userSession )
{
	// Check whether user is still logged in and hasn't been blocked since login:
	if( userSession->current_user() == 0 )
		return false;
	
	user_flags		theFlags = userSession->my_user_flags();
	
	if( (theFlags & USER_FLAG_BLOCKED)
		|| (theFlags & USER_FLAG_RETIRED) )
		return false;
	
	// Check whether user is blocked only for this room:
	for( auto currUserID : mKickedUsers )
	{
		if( currUserID == userSession->current_user() )
			return false;
	}
	
	// If the user name was mis-typed (or does not exist), we may get 0 as the user ID:
	if( inTargetUserID == 0 )
		return false;
	
	// Only owners and moderators may kick a user:
	if( (theFlags & USER_FLAG_SERVER_OWNER) == 0
		&& (theFlags & USER_FLAG_MODERATOR) == 0 )
		return false;
	
	// Moderators may not kick server owners:
	user_flags	targetFlags = userSession->find_user_flags(inTargetUserID);
	if( (theFlags & USER_FLAG_MODERATOR) && (targetFlags & USER_FLAG_SERVER_OWNER) )
		return false;

	// Moderators may not kick each other:
	if( (theFlags & USER_FLAG_MODERATOR) && (targetFlags & USER_FLAG_MODERATOR) )
		return false;
	
	// If this user has already been kicked, do nothing:
	for( auto currUserID : mKickedUsers )
	{
		if( currUserID == inTargetUserID )
			return true;
	}
	
	mKickedUsers.push_back( inTargetUserID );
	
	leave_channel( inSession, inTargetUserID, userSession, userSession->name_for_user_id(userSession->current_user()) );
	
	return true;
}


bool	channel::user_is_kicked( user_id inUserID )
{
	// If this user has already been kicked, do nothing:
	for( auto currUserID : mKickedUsers )
	{
		if( currUserID == inUserID )
			return true;
	}
	
	return false;
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

	if( !theChannel->join_channel( inSession, theUserSession->current_user(), theUserSession ) )
	{
		inSession->sendln( "!JOI:Couldn't join channel." );
		return;
	}
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
		if( !channelItty->second->leave_channel( inSession, theUserSession->current_user(), theUserSession ) )
		{
			inSession->printf( "!JOI:Couldn't leave channel %s.\r\n", channelName.c_str() );
			return;
		}
	}
	else
	{
		inSession->printf( "!BDN:No channel named %s.\r\n", channelName.c_str() );
		return;
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

	// Find the current channel so we can send the message to its subscribers:
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

	// Check whether user is blocked only for this room:
	if( theChannel->user_is_kicked( theUserSession->current_user() ) )
		return;
	
	theChannel->printf("MESG: %s %s %s", channelInfo->mChannelName.c_str(), theUserSession->my_user_name().c_str(), inCommand.c_str());
};


handler	channel::kick_handler = [](session* inSession, std::string inCommand)
{
	size_t currOffset = 0;
	session::next_word( inCommand, currOffset );
	std::string	channelName = session::next_word( inCommand, currOffset );
	std::string	userName = session::next_word( inCommand, currOffset );
	
	// If only one argument given, that's the user name, so swap user & channel:
	//	(we also use the current channel as the channelName)
	if( userName.size() == 0 )
	{
		userName = channelName;
		current_channel* channelInfo = (current_channel*)inSession->find_sessiondata(CHANNEL_SESSION_DATA_ID);
		if( channelInfo )
			channelName = channelInfo->mChannelName;
		else
		{
			inSession->sendln( "!NME:You need to give the name of a channel to kick the user from." );
			return;
		}
	}
	
	if( userName.size() == 0 )
	{
		inSession->sendln( "!UNM:You need to give the name of a user to kick." );
		return;
	}
	
	user_session*	theUserSession = (user_session*)inSession->find_sessiondata( USER_SESSION_DATA_ID );
	if( !theUserSession )
	{
		inSession->sendln( "!AUT:You need to be logged in to kick a user." );
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
	
	theChannel->kick_user( inSession, theUserSession->id_for_user_name(userName), theUserSession );
};
