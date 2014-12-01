//
//  eleven_asset_server.cpp
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-30.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_asset_server.h"
#include "eleven_ini_file.h"
#include "eleven_log.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>


using namespace eleven;


static asset_server*	sSharedServer = NULL;


asset_server::asset_server( std::string inSettingsFolder )
{
	std::string	settingsFilePath( inSettingsFolder );
	settingsFilePath.append( "/settings.ini" );
	ini_file	iniFile;
	if( !iniFile.open(settingsFilePath) )
	{
		log( "Can't find settings.ini file." );
		return;
	}
	mAssetsFolderPath = inSettingsFolder;
	mAssetsFolderPath.append( "/assets/" );
	
	sSharedServer = this;
}


void	asset_server::assets_thread( asset_server* self )
{
	self->mQueuedAssetChunks.wait( []( const asset_queue_entry& inEntry )
	{
		if( inEntry.mInfoOnly )
		{
			time_t modificationTime = 0;
			size_t numChunks = 0;
			
			sSharedServer->info( inEntry.mFilename, numChunks, modificationTime );
			log( "/asset_info %d %d %s\r\n", numChunks, modificationTime, inEntry.mFilename.c_str() );
			if( numChunks != 0 )
			{
				inEntry.mUserSession->current_session()->printf( "/asset_info %d %d %s\r\n", numChunks, modificationTime, inEntry.mFilename.c_str() );
			}
			else
			{
				inEntry.mUserSession->current_session()->printf( "/!no_such_asset -1 %s\r\n", inEntry.mFilename.c_str() );
			}
		}
		else
		{
			size_t	maxChunks = 0;
			uint8_t	buf[STREAMING_BLOCK_SIZE] = {0};
			
			size_t	amountRead = sSharedServer->get( inEntry.mFilename, inEntry.mChunkNum, buf, maxChunks );
			if( amountRead > 0 )
			{
				log( "Serving %luth (of %lu) %lu bytes of file %s\n", inEntry.mChunkNum, maxChunks, amountRead, inEntry.mFilename.c_str() );
				inEntry.mUserSession->current_session()->send_data_with_prefix_printf( buf, amountRead, "/asset_chunk %lu %lu %lu %s\r\n", amountRead, inEntry.mChunkNum, maxChunks, inEntry.mFilename.c_str() );
			}
			else
				inEntry.mUserSession->current_session()->printf( "/!no_such_asset %d %s\r\n", inEntry.mChunkNum, inEntry.mFilename.c_str() );
		}
		
		return true;
	} );
}


void	asset_server::wait_for_assets()
{
	std::thread( &asset_server::assets_thread, this ).detach();
}


void	asset_server::info( std::string inName, size_t &outNumChunks, time_t &outModificationTime )
{
	if( inName.find("..") != std::string::npos )	// Don't let anyone walk up the hierarchy.
	{
		outNumChunks = 0;
		return;
	}
	
	std::string	filePath( mAssetsFolderPath );
	filePath.append( inName );
	struct stat fileStat = {0};
	if( stat( filePath.c_str(), &fileStat ) < 0 )
	{
		outNumChunks = 0;
		return;
	}
	
	outModificationTime = fileStat.st_mtime;
	outNumChunks = (fileStat.st_size +STREAMING_BLOCK_SIZE -1) / STREAMING_BLOCK_SIZE;
}


size_t	asset_server::get( std::string inName, size_t inChunkIndex, uint8_t* outBuffer, size_t &outNumChunks )
{
	if( inName.find("..") != std::string::npos )	// Don't let anyone walk up the hierarchy.
		return 0;
	
	std::string	filePath( mAssetsFolderPath );
	filePath.append( inName );
	
	FILE*	theFile = fopen( filePath.c_str(), "r" );
	if( !theFile )
		return 0;

	struct stat fileStat = {0};
	int	fd = fileno( theFile );
	if( fstat( fd, &fileStat ) < 0 )
	{
		fclose(theFile);
		return 0;
	}
	outNumChunks = (fileStat.st_size +STREAMING_BLOCK_SIZE -1) / STREAMING_BLOCK_SIZE;

	fseek( theFile, inChunkIndex * STREAMING_BLOCK_SIZE, SEEK_SET );
	size_t itemsRead = fread( outBuffer, 1, STREAMING_BLOCK_SIZE, theFile );
	fclose(theFile);
	
	return itemsRead;
}


handler	asset_server::asset_info = [](session_ptr inSession, std::string inCommand, class chatserver* inServer)
{
	size_t currOffset = 0;
	
	user_session_ptr	theUserSession = inSession->find_sessiondata<user_session>( USER_SESSION_DATA_ID );
	if( !theUserSession )
	{
		inSession->sendln( "/!asset_not_logged_in You need to be logged in to download assets." );
		return;
	}
	
	session::next_word( inCommand, currOffset );
	std::string filename = inCommand.substr( currOffset );
	sSharedServer->mQueuedAssetChunks.push( asset_queue_entry( true, filename, 0, theUserSession ) );
};


handler	asset_server::get_asset = [](session_ptr inSession,std::string inCommand,class chatserver* inServer)
{
	size_t	currOffset = 0;
	size_t	chunkIndex = 0;

	user_session_ptr	theUserSession = inSession->find_sessiondata<user_session>( USER_SESSION_DATA_ID );
	if( !theUserSession )
	{
		inSession->sendln( "/!asset_not_logged_in You need to be logged in to download assets." );
		return;
	}
	
	session::next_word( inCommand, currOffset );
	std::string	currIndexStr = session::next_word( inCommand, currOffset );
	chunkIndex = strtoul( currIndexStr.c_str(), NULL, 10 );
	std::string filename = inCommand.substr( currOffset );

	sSharedServer->mQueuedAssetChunks.push( asset_queue_entry( false, filename, chunkIndex, theUserSession ) );
};
