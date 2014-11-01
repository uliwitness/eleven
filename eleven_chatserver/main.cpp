//
//  main.cpp
//  eleven_chatserver
//
//  Created by Uli Kusterer on 2014-11-01.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include <string>
#include <functional>
#include <stdarg.h>


#define MAX_LINE_LENGTH 1024


class eleven_session;


typedef std::function<bool(eleven_session*,std::string)>	eleven_handler;
typedef std::map<std::string,eleven_handler>				eleven_handler_map;


class eleven_session
{
public:
	eleven_session( int sessionSocket ) : mSessionSocket(sessionSocket) {}
	
	void	printf( const char* inFormatString, ... )
	{
		char	replyString[MAX_LINE_LENGTH];
		replyString[sizeof(replyString) -1] = 0;    // snprintf doesn't terminate if text length is >= buffer size, so terminate manually and give it one less byte of buffer to work with so it doesn't overwrite us.
		
		va_list		args;
		va_start( args, inFormatString );
		vsnprintf( replyString, sizeof(replyString) -1, inFormatString, args );
		va_end(args);
		write( mSessionSocket, replyString, strlen(replyString) );    // Send!
	}
	
private:
	int		mSessionSocket;
};


class eleven_chatserver
{
public:
    eleven_chatserver( in_port_t inPortNumber = 0 ) // 0 == open random port.
        : mIsOpen(false), mListeningSocket(0)
    {
        int                 status = 0;
        struct sockaddr_in  my_name;
        
        mListeningSocket = socket( AF_INET, SOCK_STREAM, 0 );
        if( mListeningSocket == -1 )
        {
            perror( "Couldn't create Socket." );
            return;
        }

        /* socket binding */
        my_name.sin_family = AF_INET;
        my_name.sin_addr.s_addr = INADDR_ANY;
        my_name.sin_port = htons(inPortNumber);
        
        status = bind( mListeningSocket, (struct sockaddr*)&my_name, sizeof(my_name) );
        if( status == -1 )
        {
            perror( "Couldn't bind socket to port." );
            return;
        }
        
        status = listen( mListeningSocket, 5 );
        if( status == -1 )
        {
            perror("Couldn't listen on port.");
            close( mListeningSocket );
            return;
        }
        
        mIsOpen = true;
        
        struct sockaddr_in boundAddress = {0};
        socklen_t len = sizeof(boundAddress);
        if( getsockname( mListeningSocket, (struct sockaddr *)&boundAddress, &len ) == -1 )
        {
            perror("Couldn't determine port socket was bound to.");
            close( mListeningSocket );
            return;
        }
        else
            mPortNumber = ntohs(boundAddress.sin_port);
    }
    
    bool        valid()         { return mIsOpen && mListeningSocket >= 0; }
    in_port_t   port_number()   { return mPortNumber; }
    
    void        wait_for_connection()
    {
        mKeepRunning = true;
        
        while( mKeepRunning )
        {
            struct sockaddr_in  clientAddress = {0};
            socklen_t           clientAddressLength = 0;
            int                 sessionSocket = -1;
            ssize_t             x = 0,
                                bytesRead = 0;
            char                requestString[MAX_LINE_LENGTH];
            
            // Block until someone "knocks" on our port:
            clientAddressLength = sizeof(clientAddress);
            sessionSocket = accept( mListeningSocket, (struct sockaddr*)&clientAddress, &clientAddressLength );
            if( sessionSocket == -1 )
            {
                perror( "Failed to accept connection." );
                return;
            }
			
			eleven_session		session(sessionSocket);
			
            // Now read messages line-wise from the client:
            bool        keepSessionRunning = true;
            while( keepSessionRunning )
            {
                x = 0;
                if( (bytesRead = read(sessionSocket, requestString + x, MAX_LINE_LENGTH -x)) > 0 )
                {
                    x += bytesRead;
                }
                
                if( bytesRead == -1 )
                {
                    perror("Couldn't read request.");
                    close( sessionSocket );
                    return;
                }
                
                // Trim off trailing line break:
                if( x >= 2 && requestString[x-2] == '\r' )    // Might be Windows-style /r/n like Mac OS X sends it.
                    requestString[x-2] = '\0';
                if( x >= 1 && requestString[x-1] == '\n' )
                    requestString[x-1] = '\0';
				
				// Find first word and look up the command handler for it:
                std::string     commandName;
				size_t  		commandLen = 0;
				for( x = 0; requestString[x] != 0 && requestString[x] != ' ' && requestString[x] != '\t' && requestString[x] != '\r' && requestString[x] != '\n'; x++ )
				{
					commandLen = x +1;
				}
				if( commandLen > 0 )
					commandName.assign( requestString, commandLen );
				
                auto    foundHandler = mRequestHandlers.find(commandName);
                
                if( foundHandler != mRequestHandlers.end() )
                {
                    keepSessionRunning = (*foundHandler).second( &session, requestString );
                }
                else if( (foundHandler = mRequestHandlers.find("*")) != mRequestHandlers.end() )
                {
                    keepSessionRunning = (*foundHandler).second( &session, requestString );
                }
                else
                {
                	session.printf("\n");	// So the other side doesn't wait for an answer forever.
                }
            }
            close( sessionSocket );
        }
    }
    
    void    register_command_handler( std::string command, eleven_handler handler )
    {
        mRequestHandlers[command] = handler;
    }
    
private:
    bool                mIsOpen;
    int                 mListeningSocket;
    bool                mKeepRunning;
    in_port_t           mPortNumber;
    eleven_handler_map	mRequestHandlers;
};


int main()
{
    eleven_chatserver       server( 13762 );
    
    if( server.valid() )
    {
        printf( "Listening on port %d\n", server.port_number() );
		server.register_command_handler( "/bye", []( eleven_session* session, std::string currRequest )
		{
			session->printf( "Nice to speak with you.\n" );
			return false;	// Terminate session!
		} );
		server.register_command_handler( "/howdy", []( eleven_session* session, std::string currRequest )
		{
			session->printf( "Welcome!\n" );
			return true;
		} );
		server.register_command_handler( "*", []( eleven_session* session, std::string currRequest )
		{
			session->printf( "JUNK:%s\n",currRequest.c_str() );
			return true;
		} );
        server.wait_for_connection();
    }

    return 0;
}