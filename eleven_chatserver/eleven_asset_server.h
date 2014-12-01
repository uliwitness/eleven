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
#include "eleven_concurrent_queue.h"
#include "eleven_users.h"


namespace eleven
{
	enum
	{
		STREAMING_BLOCK_SIZE = (4 * 1024)
	};
	
	
	class asset_queue_entry
	{
	public:
		asset_queue_entry() : mChunkNum(0), mInfoOnly(true)	{};
		asset_queue_entry( bool infoOnly, std::string inFilename, size_t inChunkNum, user_session_ptr userSession )
			: mInfoOnly(infoOnly), mFilename(inFilename), mChunkNum(inChunkNum), mUserSession(userSession) {};
		
		std::string			mFilename;
		size_t				mChunkNum;
		bool				mInfoOnly;
		user_session_ptr	mUserSession;
	};
	
	class asset_server
	{
	public:
		asset_server( std::string inSettingsFolder );

		void	wait_for_assets();	// Spawns off thread that waits for new asset requests.
		
		void	info( std::string inName, size_t &outNumChunks, time_t &outModificationTime );
		size_t	get( std::string inName, size_t inChunkIndex, uint8_t* outBuffer, size_t &outNumChunks );
		
		static handler		asset_info;		// "/asset_info" command with which client can query size of an asset or whether it has changed.
		static handler		get_asset;		// "/get_asset" command with which client can retrieve a chunk of an asset.
		
	protected:
		std::string							mAssetsFolderPath;
		concurrent_queue<asset_queue_entry>	mQueuedAssetChunks;

		static void		assets_thread( asset_server* self );
	};
}

#endif /* defined(__interconnectserver__eleven_asset_server__) */
