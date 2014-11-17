//
//  eleven_session.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_session.h"
#include <sys/socket.h>
#include <string>




using namespace eleven;


#define MAX_LINE_LENGTH 1024


session::session( int sessionSocket, socket_type socketType )
	: mSessionSocket(sessionSocket), mKeepRunningFlag(true), mSSLContext(NULL), mSSLSocket(NULL)
{
	SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
	
	const char*		certificateFilePath = (socketType == SOCKET_TYPE_SERVER) ? "ElevenServerCertificate.pem" : "ElevenClientCertificate.pem";
	
	mSSLContext = SSL_CTX_new( (socketType == SOCKET_TYPE_SERVER) ? TLSv1_2_server_method() : TLSv1_2_client_method() );
	if( !mSSLContext )
	{
		::printf("Error: Couldn't create SSL context.\n");
		return;
	}
	SSL_CTX_set_options(mSSLContext, SSL_OP_SINGLE_DH_USE);

	SSL_CTX_set_default_passwd_cb( mSSLContext, NULL );
	SSL_CTX_set_default_passwd_cb_userdata( mSSLContext, (void*)"eleven" );

	if( 1 != SSL_CTX_use_certificate_file( mSSLContext, certificateFilePath, SSL_FILETYPE_PEM ) )
	{
		::printf("Error: Failed to load certificate.\n");
		SSL_CTX_free( mSSLContext );
		mSSLContext = NULL;
		return;
	}

	if( 1 != SSL_CTX_use_PrivateKey_file( mSSLContext, certificateFilePath, SSL_FILETYPE_PEM ) )
	{
		::printf("Error: Failed to load private certificate.\n");
		SSL_CTX_free( mSSLContext );
		mSSLContext = NULL;
		return;
	}

	mSSLSocket = SSL_new( mSSLContext );
	if( !mSSLSocket )
	{
		::printf("Error: Couldn't create SSL socket.\n");
		SSL_CTX_free( mSSLContext );
		mSSLContext = NULL;
		return;
	}
	SSL_set_fd( mSSLSocket, sessionSocket );
	
	if( socketType == SOCKET_TYPE_SERVER )
	{
		int	err = SSL_accept( mSSLSocket );
		if( err <= 0 )
		{
			::printf("Error: Couldn't accept SSL socket.\n");
			SSL_shutdown( mSSLSocket );
			SSL_free( mSSLSocket );
			mSSLSocket = NULL;
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}
	}
	
	int	newValue = true;
	setsockopt( mSessionSocket, SOL_SOCKET, SO_NOSIGPIPE, &newValue, sizeof(newValue) );
}


ssize_t	session::reply_from_printfln( std::string& outString, const char* inFormatString, ... )
{
	char	replyString[MAX_LINE_LENGTH];
	replyString[sizeof(replyString) -1] = 0;    // snprintf doesn't terminate if text length is >= buffer size, so terminate manually and give it one less byte of buffer to work with so it doesn't overwrite us.
	
	va_list		args;
	va_start( args, inFormatString );
	vsnprintf( replyString, sizeof(replyString) -1, inFormatString, args );
	va_end(args);
	ssize_t writtenAmount = SSL_write( mSSLSocket, replyString, (int)strlen(replyString) );	// Send!
	
	if( writtenAmount <= 0 )
		return 0;
	
	if( send( (uint8_t*)"\r\n", 2 ) <= 0 )
		return 0;
	
	return readln( outString ) ? writtenAmount : 0;
}


ssize_t	session::printf( const char* inFormatString, ... )
{
	if( !mSSLSocket )
		return false;
	
	char	replyString[MAX_LINE_LENGTH];
	replyString[sizeof(replyString) -1] = 0;    // snprintf doesn't terminate if text length is >= buffer size, so terminate manually and give it one less byte of buffer to work with so it doesn't overwrite us.
	
	va_list		args;
	va_start( args, inFormatString );
	vsnprintf( replyString, sizeof(replyString) -1, inFormatString, args );
	va_end(args);
	return SSL_write( mSSLSocket, replyString, (int)strlen(replyString) );
}


ssize_t	session::sendln( std::string inString )
{
	if( !mSSLSocket )
		return false;
	
	size_t amountSent = SSL_write( mSSLSocket, inString.c_str(), (int)inString.size() );	// Send!
	if( SSL_write( mSSLSocket, "\r\n", 2 ) == 2 )
		amountSent -= 1;
	return amountSent;
}


ssize_t	session::send( std::string inString )
{
	if( !mSSLSocket )
		return false;
	
	return SSL_write( mSSLSocket, inString.c_str(), (int)inString.size() );	// Send!
}


ssize_t	session::send( const uint8_t *inData, size_t inLength )
{
	if( !mSSLSocket )
		return false;
	
	return SSL_write( mSSLSocket, inData, (int)inLength );    // Send!
}


bool	session::readln( std::string& outString )
{
	char                requestString[MAX_LINE_LENGTH];
	
	if( !mSSLSocket )
		return false;
	
	for( ssize_t x = 0; x < MAX_LINE_LENGTH; )
	{
		ssize_t	bytesRead = SSL_read( mSSLSocket, requestString +x, 1 );
		if( bytesRead != 1 )
			return false;
		if( requestString[x] == '\r' )
			requestString[x] = 0;	// Just terminate, don't break yet as we will have a \n as the next character.
		if( requestString[x] == '\n' )
		{
			requestString[x] = 0;
			break;
		}
		x += bytesRead;
	}
	
	requestString[MAX_LINE_LENGTH-1] = 0;
	
	outString = requestString;
	
	return true;
}


bool	session::read( std::vector<uint8_t> &outData )
{
	if( !mSSLSocket )
		return false;
	
	uint8_t*	bytes = &outData[0];
	ssize_t		numBytes = outData.size();
	
	ssize_t	bytesRead = SSL_read( mSSLSocket, bytes, (int)numBytes );	// +++ all SSL_read calls need MSG_WAITALL applied somehow. How does one do that with SSL_read?
	if( bytesRead != numBytes )
		return false;
	
	return true;
}


std::string	session::next_word( std::string inString, size_t &currOffset )
{
	if( currOffset == std::string::npos )
		return std::string();
	
	std::string	result;
	size_t		currWordEnd = inString.find_first_of( " \r\n\t", currOffset );
	size_t		nextWordStart = std::string::npos;
	if( currWordEnd != std::string::npos )
		nextWordStart = inString.find_first_not_of( " \r\n\t", currWordEnd );
	
	if( currWordEnd == std::string::npos )
	{
		result = inString.substr(currOffset);
		currOffset = std::string::npos;
	}
	else
	{
		result = inString.substr(currOffset,currWordEnd-currOffset);
		currOffset = nextWordStart;
	}
	return result;
}


void	session::attach_sessiondata( sessiondata_id inID, sessiondata* inData )
{
	remove_sessiondata(inID);	// Make sure any old data is deleted.
	mSessionData[inID] = inData;
}


sessiondata*	session::find_sessiondata( sessiondata_id inID )
{
	auto foundData = mSessionData.find(inID);
	
	if( foundData == mSessionData.end() )
		return NULL;
	else
		return foundData->second;
}


void	session::remove_sessiondata( sessiondata_id inID )
{
	auto foundData = mSessionData.find(inID);
	
	if( foundData != mSessionData.end() )
	{
		delete foundData->second;
		mSessionData.erase(foundData);
	}
}






