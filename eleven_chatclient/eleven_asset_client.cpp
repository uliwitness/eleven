//
//  eleven_asset_client.cpp
//  interconnectgame
//
//  Created by Uli Kusterer on 2014-11-30.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_asset_client.h"


using namespace eleven;


asset_client*	sSharedAssetClient = NULL;


asset_client::asset_client( std::string inAssetsFolderPath )
	: mAssetsCachePath(inAssetsFolderPath)
{
	sSharedAssetClient = this;
}


message_handler	asset_client::asset_info = []( session_ptr inSession, std::string inLine, chatclient* inSender)
{
	size_t	currOffset = 0;
	session::next_word( inLine, currOffset );
	std::string	chunkCountStr = session::next_word( inLine, currOffset );
	std::string	changeTimeStr = session::next_word( inLine, currOffset );
	std::string	filename = inLine.substr( currOffset );
	long	cunkCount = strtol(chunkCountStr.c_str(),NULL,10);

	std::string	filePath( sSharedAssetClient->mAssetsCachePath );
	filePath.append( 1, '/' );
	filePath.append( filename );
	std::string	metadataFilePath( filePath );
	metadataFilePath.append( "..metadata" );	// We reject file names with .. in them for safety reasons (going up the path) so this name can never happen in our cache.
	FILE*		theMetadataFile = fopen(metadataFilePath.c_str(),"r");
	std::string	currentFileVersion;
	long		nextChunkNum = -1;
	if( theMetadataFile )
	{
		size_t	len = 0;
		const char* bytes = fgetln( theMetadataFile, &len );
		while( bytes[len-1] == '\n' || bytes[len-1] == '\r' )
			len--;
		currentFileVersion = std::string( bytes, len );

		int x = 0;
		while( true )
		{
			size_t len = 0;
			const char* theLine = fgetln( theMetadataFile, &len );
			if( !theLine || (theLine[0] != '0' && theLine[0] != '1') )
				break;
			if( len == 2 && theLine[0] == '0' && theLine[1] == '\n' )
			{
				nextChunkNum = x;
				break;
			}
			x++;
		}

		fclose(theMetadataFile);
	}
	
	if( currentFileVersion.compare(changeTimeStr) == 0 && nextChunkNum == -1 )	// Current version & no missing segments:
	{
		sSharedAssetClient->mFileFinishedCallback( filename, true );	// Nothing more to do to get this file.
	}
	else
	{
		if( nextChunkNum == -1 )	// No missing segments? Either wrong version, or not d/led yet.
		{
			nextChunkNum = 0;
			theMetadataFile = fopen(metadataFilePath.c_str(),"w");	// Create metadata file/reset it.
			fprintf( theMetadataFile, "%s\n", changeTimeStr.c_str() );
			for( int x = 0; x < cunkCount; x++ )
			{
				fputs( "0\n", theMetadataFile );	// Ensure we have a line for each chunk.
			}
			fclose(theMetadataFile);
		}
		
		inSession->printf( "/get_asset %d %s\r\n", nextChunkNum, filename.c_str() );
	}
};


message_handler	asset_client::asset_chunk = []( session_ptr inSession, std::string inLine, chatclient* inSender)
{
	size_t	currOffset = 0;
	session::next_word( inLine, currOffset );
	std::string	dataSizeStr = session::next_word( inLine, currOffset );
	std::string	chunkNumStr = session::next_word( inLine, currOffset );
	std::string	maxChunksStr = session::next_word( inLine, currOffset );
	std::string	filename = inLine.substr( currOffset );
	size_t	chunkNum = strtoul( chunkNumStr.c_str(), NULL, 10 );
	size_t	maxChunks = strtoul( maxChunksStr.c_str(), NULL, 10 );
	size_t	dataSize = strtoul( dataSizeStr.c_str(), NULL, 10 );
	uint8_t buf[4096] = {0};
	inSession->read( buf, 1 );	// Skip the \n.
	
	if( filename.find("..") != std::string::npos )	// Don't let anyone walk up the directory tree.
		return;
	
	std::string	filePath( sSharedAssetClient->mAssetsCachePath );
	filePath.append( 1, '/' );
	filePath.append( filename );
	FILE* theFile = fopen( filePath.c_str(), "r+" );
	if( theFile == NULL )
		theFile = fopen( filePath.c_str(), "w" );
	fseek( theFile, chunkNum * 4096, SEEK_SET );
	if( inSession->read( buf, dataSize ) != dataSize )
		printf( "Couldn't read chunk from the network.\n" );
	fwrite( buf, 1, dataSize, theFile );
	fclose( theFile );
	
	std::string	metadataFilePath( filePath );
	metadataFilePath.append( "..metadata" );	// We reject file names with .. in them for safety reasons (going up the path) so this name can never happen in our cache.
	FILE*		theMetadataFile = fopen(metadataFilePath.c_str(),"r+");
	std::string	currentFileVersion;
	if( theMetadataFile )
	{
		long	nextChunkNum = -1;
		size_t	len = 0;
		fgetln( theMetadataFile, &len );	// Skip first line with version.
		fseek( theMetadataFile, 2 * chunkNum, SEEK_CUR);
		fputs( "1", theMetadataFile );
		fseek( theMetadataFile, len, SEEK_SET );
		int x = 0;
		while( true )
		{
			const char* theLine = fgetln( theMetadataFile, &len );
			if( !theLine || (theLine[0] != '0' && theLine[0] != '1') )
				break;
			if( len == 2 && theLine[0] == '0' && theLine[1] == '\n' )
			{
				nextChunkNum = x;
				break;
			}
			x++;
		}
		fclose(theMetadataFile);
		
		printf("download %lu -> %ld (of %lu)\n", chunkNum, nextChunkNum, maxChunks);
		if( nextChunkNum != -1 )
			inSession->printf( "/get_asset %lu %s\r\n", nextChunkNum, filename.c_str() );
		else
			sSharedAssetClient->mFileFinishedCallback( filename, true );
	}
	else
		sSharedAssetClient->mFileFinishedCallback( filename, false );
};
