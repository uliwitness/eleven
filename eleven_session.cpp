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
#include <thread>


using namespace eleven;


#define THREADED_SESSION_SENDS		0


#define MAX_LINE_LENGTH 1024


session::session( int sessionSocket, const char* senderAddressStr, std::string inSettingsFolderPath, socket_type socketType )
	: mSessionSocket(sessionSocket), mKeepRunningFlag(true), mSSLContext(NULL), mSSLSocket(NULL), mSenderAddressStr(senderAddressStr)
{
	std::string		settingsFilePath( inSettingsFolderPath );
	settingsFilePath.append("/settings.ini");
	
	if( !mIniFile.open(settingsFilePath) )
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

		std::string		certificateFilePath( inSettingsFolderPath );
		certificateFilePath.append("/ElevenServerCertificate.pem");
		
		if( 1 != SSL_CTX_use_certificate_file( mSSLContext, certificateFilePath.c_str(), SSL_FILETYPE_PEM ) )
		{
			::printf("Error: Failed to load certificate.\n");
			SSL_CTX_free( mSSLContext );
			mSSLContext = NULL;
			return;
		}

		if( 1 != SSL_CTX_use_PrivateKey_file( mSSLContext, certificateFilePath.c_str(), SSL_FILETYPE_PEM ) )
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
		std::string		certificateFilePath( inSettingsFolderPath );
		certificateFilePath.append("/ElevenServerPublicCertificate.pem");
		
		if( BIO_read_filename( cert, certificateFilePath.c_str() ) <= 0 )
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

	#if THREADED_SESSION_SENDS
	std::vector<uint8_t>	theData;
	size_t		replyStringLen = strlen(replyString);
	theData.resize( replyStringLen );
	memcpy( &(theData[0]), replyString, replyStringLen );
	queue_data( theData );
	
	return replyStringLen;
	#else
	return SSL_write( mSSLSocket, replyString, (int)strlen(replyString) );
	#endif
}


ssize_t	session::sendln( std::string inString )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return false;
	
	#if THREADED_SESSION_SENDS
	std::vector<uint8_t>	theData;
	theData.resize( inString.size() +2 );
	memcpy( &(theData[0]), inString.c_str(), inString.size() );
	memcpy( &(theData[inString.size()]), "\r\n", 2 );
	queue_data( theData );
	
	return inString.size();
	#else
	size_t amountSent = SSL_write( mSSLSocket, inString.c_str(), (int)inString.size() );	// Send!
	if( SSL_write( mSSLSocket, "\r\n", 2 ) != 2 )
		amountSent -= 1;	// Make sure caller notices sth. didn't go out.
	return amountSent;
	#endif
}


ssize_t	session::send( std::string inString )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return 0;
	
	#if THREADED_SESSION_SENDS
	std::vector<uint8_t>	theData;
	theData.resize( inString.size() );
	memcpy( &(theData[0]), inString.c_str(), inString.size() );
	queue_data( theData );
	
	return inString.size();
	#else
	return SSL_write( mSSLSocket, inString.c_str(), (int)inString.size() );	// Send!
	#endif
}


ssize_t	session::send( const uint8_t *inData, size_t inLength )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return 0;
	
	#if THREADED_SESSION_SENDS
	std::vector<uint8_t>	theData;
	theData.resize( inLength );
	memcpy( &(theData[0]), inData, inLength );
	queue_data( theData );
	
	return inLength;
	#else
	return SSL_write( mSSLSocket, inData, (int)inLength );    // Send!
	#endif
}


ssize_t	session::send_data_with_prefix_printf( const uint8_t *inData, size_t inLength, const char* inFormatString, ... )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return 0;
	
	char	replyString[MAX_LINE_LENGTH];
	replyString[sizeof(replyString) -1] = 0;    // snprintf doesn't terminate if text length is >= buffer size, so terminate manually and give it one less byte of buffer to work with so it doesn't overwrite us.
	
	va_list		args;
	va_start( args, inFormatString );
	vsnprintf( replyString, sizeof(replyString) -1, inFormatString, args );
	va_end(args);
	
	#if THREADED_SESSION_SENDS
	size_t	replyStringLen = strlen(replyString);
	std::vector<uint8_t>	theData;
	theData.resize( replyStringLen +inLength );
	memcpy( &(theData[0]), replyString, replyStringLen );
	memcpy( &(theData[replyStringLen]), inData, inLength );
	queue_data( theData );
	
	return replyStringLen +inLength;
	#else
	ssize_t	theAmount = SSL_write( mSSLSocket, replyString, (int)strlen(replyString) );
	theAmount += SSL_write( mSSLSocket, inData, (int)inLength );
	
	return theAmount;
	#endif
}


void	session::queue_data( const std::vector<uint8_t> inData )
{
	mQueuedDataBlocks.push( inData );
}


void	session::queued_data_sender_thread( session_ptr self )
{
	self->mQueuedDataBlocks.wait( [=](const std::vector<uint8_t>& inData)
	{
		std::lock_guard<std::recursive_mutex>		lock(self->mSessionLock);

		if( self->mSSLSocket )
			SSL_write( self->mSSLSocket, &(inData[0]), (int)inData.size() );
		return self->keep_running();
	} );
}


void	session::wait_for_queued_data()
{
#if THREADED_SESSION_SENDS
	std::thread( &session::queued_data_sender_thread, shared_from_this() ).detach();
#endif
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


ssize_t	session::read( uint8_t* bytes, size_t numBytes )
{
	std::lock_guard<std::recursive_mutex>		lock(mSessionLock);

	if( !mSSLSocket )
		return 0;
	
	ssize_t	bytesRead = SSL_read( mSSLSocket, bytes, (int)numBytes );	// +++ all SSL_read calls need MSG_WAITALL applied somehow. How does one do that with SSL_read?
	
	return bytesRead;
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






