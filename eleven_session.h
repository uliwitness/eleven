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
#include "openssl/ssl.h"
#include "eleven_ini_file.h"
#include "eleven_concurrent_queue.h"
#include <memory>
#include <mutex>


namespace eleven
{
	typedef enum socket_type
	{
		SOCKET_TYPE_SERVER,
		SOCKET_TYPE_CLIENT
	} socket_type;
	
	
	class sessiondata
	{
	public:
		virtual ~sessiondata() {};
	};
	typedef std::shared_ptr<sessiondata>	sessiondata_ptr;
	
	typedef	uint32_t	sessiondata_id;
	
	class session;
	
	typedef std::shared_ptr<session>	session_ptr;
	
	class session : public std::enable_shared_from_this<session>
	{
	public:
		~session();
	
		bool		valid()	{ return mSSLSocket != NULL; };
		
		/*! Send the given formatted output to the client as a string. *DO NOT* call this as printf( cStringVar ) because if cStringVar contains '%' signs you will crash, use send() below instead for that case, or printf("%s",cStringVar). */
		ssize_t		printf( const char* inFormatString, ... );
		
		/*! Same as send(), but appends \r\n to it: */
		ssize_t		sendln( std::string inString );
		/*! Preferred over printf() if all you want is send a single string without substitution of '%'-sequences. */
		ssize_t		send( std::string inString );
		/*! Send the given raw byte data. This is handy for sending back e.g. images. */
		ssize_t		send( const uint8_t* inData, size_t inLength );
		/*! Like printf followed by send, but as an atomic, thread-safe unit. */
		ssize_t		send_data_with_prefix_printf( const uint8_t *inData, size_t inLength, const char* inFormatString, ... );
		
		void		queue_data( const std::vector<uint8_t> inData );
		void		wait_for_queued_data();
		
		/*! Read a single line as a string from the session. Useful for back-and-forth conversation during a session. Returns TRUE on success, FALSE on failure. */
		bool		readln( std::string& outString );
		/*! Read numBytes of binary data from the session into the buffer provided in bytes. Returns amount of bytes actually read. */
		size_t		read( uint8_t* bytes, size_t numBytes );
		
		/*! For a server session, this allows you to exit the loop that dispatches commands and terminate the session/connection. */
		void		disconnect();
		void		force_disconnect();
		bool		keep_running()	{ return mKeepRunningFlag; };	// Test if we're still connected.
		
		// Attach session-related data to a session. Used by the other modules on the server:
		template<class D>
		void				attach_sessiondata( sessiondata_id inID, std::shared_ptr<D> inData )	{ attach_sessiondata( inID, std::static_pointer_cast<sessiondata>(inData) ); };
		template<class D>
		std::shared_ptr<D>	find_sessiondata( sessiondata_id inID ) { return std::static_pointer_cast<D>(find_sessiondata(inID)); };	//!< D must be a sessiondata subclass.
		void				remove_sessiondata( sessiondata_id inID );
		
		std::string			sender_address_str()	{ return mSenderAddressStr; };
		
		ini_file&			settings()				{ return mIniFile; }
		
		/*! Parses the next word out of a string. */
		static std::string	next_word( std::string inString, size_t &currOffset, const char* delimiters = " \r\n\t" );
		/*! Use this after next_word to get the rest of a string. */
		static std::string	remainder_of_string( std::string inString, size_t currOffset );

	protected:
		session( int sessionSocket, const char* senderAddressStr, std::string inSettingsFolderPath, socket_type socketType );
		
		friend class chatclient;
		friend class chatserver;
		
		void				attach_sessiondata( sessiondata_id inID, sessiondata_ptr inData );
		sessiondata_ptr		find_sessiondata( sessiondata_id inID );
		static void			queued_data_sender_thread( session_ptr self );

	private:
		int											mSessionSocket;
		std::string									mSenderAddressStr;	// IP address as string for display in log messages (currently SOCKET_TYPE_SERVER only).
		bool										mKeepRunningFlag;
		std::map<sessiondata_id,sessiondata_ptr>	mSessionData;
		SSL*										mSSLSocket;
		SSL_CTX*									mSSLContext;
		ini_file									mIniFile;
		std::recursive_mutex						mSessionLock;
		std::recursive_mutex						mSessionDataLock;
		concurrent_queue<std::vector<uint8_t>>		mQueuedDataBlocks;
	};
}

#endif /* defined(__eleven__eleven_session__) */
