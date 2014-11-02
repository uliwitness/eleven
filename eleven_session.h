//
//  eleven_session.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_session__
#define __eleven__eleven_session__

#include <vector>
#include <map>


namespace eleven
{
	class sessiondata
	{
	public:
	};
	
	typedef	uint32_t	sessiondata_id;
	
	
	class session
	{
	public:
		/*! */
		ssize_t		reply_from_printfln( std::string& outString, const char* inFormatString, ... );

		/*! Send the given formatted output to the client as a string. *DO NOT* call this as printf( cStringVar ) because if cStringVar contains '%' signs you will crash, use send() below instead for that case, or printf("%s",cStringVar). */
		ssize_t		printf( const char* inFormatString, ... );
		
		/*! Same as send(), but appends \r\n to it: */
		ssize_t		sendln( std::string inString );
		/*! Preferred over printf() if all you want is send a single string without substitution of '%'-sequences. */
		ssize_t		send( std::string inString );
		/*! Send the given raw byte data. This is handy for sending back e.g. images. */
		ssize_t		send( const uint8_t* inData, size_t inLength );
		
		/*! Read a single line as a string from the session. Useful for back-and-forth conversation during a session. Returns TRUE on success, FALSE on failure. */
		bool		readln( std::string& outString );
		/*! Read outData.size() bytes of binary data from the session into outData. Returns TRUE on success, FALSE on failure. */
		bool		read( std::vector<uint8_t>& outData );
		
		/*! For a server session, this allows you to exit the loop that dispatches commands and terminate the session/connection. */
		void		log_out()		{ mKeepRunningFlag = false; };
		
		bool		keep_running()	{ return mKeepRunningFlag; };
		
		void			attach_sessiondata( sessiondata_id inID, sessiondata* inData );	//! Takes over ownership of inData.
		sessiondata*	find_sessiondata( sessiondata_id inID );
		void			remove_sessiondata( sessiondata_id inID );	//! deletes the data stored under inID.
		
		/*! Parses the next word out of a string. */
		static std::string	next_word( std::string inString, size_t &currOffset );

	protected:
		session( int sessionSocket ) : mSessionSocket(sessionSocket), mKeepRunningFlag(true) {}
		
		friend class chatclient;
		friend class chatserver;
		
	private:
		int										mSessionSocket;
		bool									mKeepRunningFlag;
		std::map<sessiondata_id,sessiondata*>	mSessionData;
	};

}

#endif /* defined(__eleven__eleven_session__) */
