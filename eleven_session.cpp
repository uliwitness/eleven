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


session::session( int sessionSocket, const char* senderAddressStr, const char* inSettingsFilePath, socket_type socketType )
	: mSessionSocket(sessionSocket), mKeepRunningFlag(true), mSSLContext(NULL), mSSLSocket(NULL), mIniFile(inSettingsFilePath), mSenderAddressStr(senderAddressStr)
{
	if( !mIniFile.valid() )
	{
		::printf("Error: Failed to load settings file.\n");
		return;
	}
	
	SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
	
	mSSLContext = SSL_CTX_new( (socketType == SOCKET_TYPE_SERVER) ? TLSv1_2_server_method() : TLSv1_2_client_method() );
	if( !mSSLContext )
	{
		::printf("Error: Couldn't create SSL context.\n");
		return;
	}
	
	SSL_CTX_set_options(mSSLContext, SSL_OP_SINGLE_DH_USE);

	if( socketType == SOCKET_TYPE_SERVER )
	{
		std::string	pass( mIniFile.setting("cert_password") );
		SSL_CTX_set_default_passwd_cb( mSSLContext, NULL );
		SSL_CTX_set_default_passwd_cb_userdata( mSSLContext, (void*)pass.c_str() );

		if( 1 != SSL_CTX_use_certificate_file( mSSLContext, "ElevenServerCertificate.pem", SSL_FILETYPE_PEM ) )
		{
			::printf("Error: Failed to load certificate.\n");
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}

		if( 1 != SSL_CTX_use_PrivateKey_file( mSSLContext, "ElevenServerCertificate.pem", SSL_FILETYPE_PEM ) )
		{
			::printf("Error: Failed to load private certificate.\n");
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}
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
	
	int	err = 1;
	if( socketType == SOCKET_TYPE_SERVER )
	{
		err = SSL_accept( mSSLSocket );
	}
	else
	{
		err = SSL_connect( mSSLSocket );
	}
	if( err <= 0 )
	{
	}
	int	newValue = true;
	setsockopt( mSessionSocket, SOL_SOCKET, SO_NOSIGPIPE, &newValue, sizeof(newValue) );
	
	if( socketType == SOCKET_TYPE_CLIENT )
	{
		X509 *	desiredPublicCertificate = NULL;
		BIO *cert;

		if( (cert = BIO_new(BIO_s_file())) == NULL )
		{
			::printf("Error: Couldn't create desired server certificate.\n");
			SSL_shutdown( mSSLSocket );
			SSL_free( mSSLSocket );
			mSSLSocket = NULL;
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}
		if( BIO_read_filename( cert, "ElevenServerPublicCertificate.pem" ) <= 0 )
		{
			::printf("Error: Couldn't read desired server certificate.\n");
			BIO_free(cert);
			SSL_shutdown( mSSLSocket );
			SSL_free( mSSLSocket );
			mSSLSocket = NULL;
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}

		desiredPublicCertificate = PEM_read_bio_X509_AUX(cert, NULL, NULL /*password_callback*/, NULL);
		X509 *	serverPublicCertificate = SSL_get_peer_certificate(mSSLSocket);

		if( serverPublicCertificate == NULL || 0 != X509_cmp( serverPublicCertificate, desiredPublicCertificate ) )
		{
			X509_free( desiredPublicCertificate );
			X509_free( serverPublicCertificate );
			::printf("Error: Server did not present the correct certificate.\n");
			BIO_free(cert);
			SSL_shutdown( mSSLSocket );
			SSL_free( mSSLSocket );
			mSSLSocket = NULL;
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}
		
		X509_free( desiredPublicCertificate );
		X509_free( serverPublicCertificate );
	}
}


session::~session()
{
	if( mSSLSocket )
	{
		SSL_shutdown(mSSLSocket);
		SSL_free(mSSLSocket);
		mSSLSocket = NULL;
	}
	if( mSSLContext )
	{
		SSL_CTX_free( mSSLContext );
		mSSLContext = NULL;
	}
}


ssize_t	session::printf( const char* inFormatString, ... )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

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
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return false;
	
	size_t amountSent = SSL_write( mSSLSocket, inString.c_str(), (int)inString.size() );	// Send!
	if( SSL_write( mSSLSocket, "\r\n", 2 ) == 2 )
		amountSent -= 1;
	return amountSent;
}


ssize_t	session::send( std::string inString )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return false;
	
	return SSL_write( mSSLSocket, inString.c_str(), (int)inString.size() );	// Send!
}


ssize_t	session::send( const uint8_t *inData, size_t inLength )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return false;
	
	return SSL_write( mSSLSocket, inData, (int)inLength );    // Send!
}


bool	session::readln( std::string& outString )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

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


void	session::disconnect()
{
	std::lock_guard<std::recursive_mutex>	lock(mSessionLock);
	
	mKeepRunningFlag = false;
	if( mSSLSocket )
		SSL_shutdown(mSSLSocket);
}

bool	session::read( std::vector<uint8_t> &outData )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return false;
	
	uint8_t*	bytes = &outData[0];
	ssize_t		numBytes = outData.size();
	
	ssize_t	bytesRead = SSL_read( mSSLSocket, bytes, (int)numBytes );	// +++ all SSL_read calls need MSG_WAITALL applied somehow. How does one do that with SSL_read?
	if( bytesRead != numBytes )
		return false;
	
	return true;
}


std::string	session::next_word( std::string inString, size_t &currOffset, const char* delimiters )
{
	if( currOffset == std::string::npos )
		return std::string();
	
	std::string	result;
	size_t		currWordEnd = inString.find_first_of( delimiters, currOffset );
	size_t		nextWordStart = std::string::npos;
	if( currWordEnd != std::string::npos )
		nextWordStart = inString.find_first_not_of( delimiters, currWordEnd );
	
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


void	session::attach_sessiondata( sessiondata_id inID, sessiondata_ptr inData )
{
	remove_sessiondata(inID);	// Make sure any old data is deleted.
	std::lock_guard<std::recursive_mutex>	lock(mSessionDataLock);
	mSessionData[inID] = inData;
}


sessiondata_ptr	session::find_sessiondata( sessiondata_id inID )
{
	std::lock_guard<std::recursive_mutex>	lock(mSessionDataLock);
	
	auto foundData = mSessionData.find(inID);
	
	if( foundData == mSessionData.end() )
		return sessiondata_ptr();
	else
		return foundData->second;
}


void	session::remove_sessiondata( sessiondata_id inID )
{
	std::lock_guard<std::recursive_mutex>	lock(mSessionDataLock);

	auto foundData = mSessionData.find(inID);
	
	if( foundData != mSessionData.end() )
	{
		mSessionData.erase(foundData);
	}
}






