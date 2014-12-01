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
	time_t modificationTime = 0;
	size_t numChunks = 0;
	
	session::next_word( inCommand, currOffset );
	std::string filename = inCommand.substr( currOffset );
	sSharedServer->info( filename, numChunks, modificationTime );
	if( numChunks != 0 )
		inSession->printf( "/asset_info %d %d %s\r\n", numChunks, modificationTime, filename.c_str() );
	else
		inSession->printf( "/!no_such_asset -1 %s\r\n", filename.c_str() );
};


handler	asset_server::get_asset = [](session_ptr inSession,std::string inCommand,class chatserver* inServer)
{
	size_t	currOffset = 0;
	size_t	chunkIndex = 0;
	size_t	maxChunks = 0;
	uint8_t	buf[STREAMING_BLOCK_SIZE] = {0};
	
	session::next_word( inCommand, currOffset );
	std::string	currIndexStr = session::next_word( inCommand, currOffset );
	chunkIndex = strtoul( currIndexStr.c_str(), NULL, 10 );
	std::string filename = inCommand.substr( currOffset );
	size_t	amountRead = sSharedServer->get( filename, chunkIndex, buf, maxChunks );
	if( amountRead > 0 )
	{
		log( "Serving %luth (of %lu) %lu bytes of file %s\n", chunkIndex, maxChunks, amountRead, filename.c_str() );
		inSession->printf( "/asset_chunk %lu %lu %lu %s\r\n", amountRead, chunkIndex, maxChunks, filename.c_str() );
		inSession->send( buf, amountRead );
	}
	else
		inSession->printf( "/!no_such_asset %d %s\r\n", chunkIndex, filename.c_str() );
};
