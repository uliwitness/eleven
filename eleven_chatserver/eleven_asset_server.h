//
//  eleven_asset_server.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-30.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectserver__eleven_asset_server__
#define __interconnectserver__eleven_asset_server__

#include "eleven_chatserver.h"


namespace eleven
{
	enum
	{
		STREAMING_BLOCK_SIZE = (4 * 1024)
	};
	
	class asset_server
	{
	public:
		asset_server( std::string inSettingsFolder );

		void	info( std::string inName, size_t &outNumChunks, time_t &outModificationTime );
		size_t	get( std::string inName, size_t inChunkIndex, uint8_t* outBuffer );
		
		static handler		asset_info;		// "/asset_info" command with which client can query size of an asset or whether it has changed.
		static handler		get_asset;		// "/get_asset" command with which client can retrieve a chunk of an asset.
		
	protected:
		std::string		mAssetsFolderPath;
	};
}

#endif /* defined(__interconnectserver__eleven_asset_server__) */
