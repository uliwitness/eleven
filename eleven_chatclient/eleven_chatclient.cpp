//
//  eleven_chatclient.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_chatclient.h"
#include "eleven_session.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


using namespace eleven;


chatclient::chatclient( const char* inIPAddress, in_port_t inPortNumber )
	: mSocket(-1), mSession(NULL)
{
	struct sockaddr_in		serverAddress = {0};

	mSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if( mSocket == -1 )
	{
		perror("Couldn't create socket");
		return;
	}

	serverAddress.sin_family = AF_INET;
	inet_aton( inIPAddress, &serverAddress.sin_addr);
	serverAddress.sin_port = htons(inPortNumber);

	int	status = connect( mSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress) );
	if( status == -1 )
	{
		perror("Couldn't connect to server");
		close(mSocket);
		mSocket = NULL;
		return;
	}
	
	mSession = new session( mSocket, SOCKET_TYPE_CLIENT );
	if( !mSession->valid() )
	{
		delete mSession;
		mSession = NULL;
		close(mSocket);
		mSocket = NULL;
		return;
	}
}

chatclient::~chatclient()
{
	if( mSession )
		delete mSession;
	if( mSocket >= 0 )
		close( mSocket );
}
