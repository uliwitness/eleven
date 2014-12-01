//
//  eleven_asset_client.h
//  interconnectgame
//
//  Created by Uli Kusterer on 2014-11-30.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectgame__eleven_asset_client__
#define __interconnectgame__eleven_asset_client__

/*
	If you register the asset_info and asset_chunk handlers in here in
	your client and create an asset_client object, any file that you
	call /asset_info for will be downloaded if it hasn't already been or if it is no longer the current version.
*/

#include "eleven_chatclient.h"

namespace eleven
{
	class asset_client
	{
	public:
		asset_client( std::string assetsCachePath );
		
		void	set_file_finished_callback( std::function<void(std::string,bool)> inFileFinishedCallback )	{ mFileFinishedCallback = inFileFinishedCallback; };
		
		static message_handler		asset_info;
		static message_handler		asset_chunk;
		
		std::string								mAssetsCachePath;
		std::function<void(std::string,bool)>	mFileFinishedCallback;	// File name & whether successfully (TRUE) or error (FALSE).
	};
}

#endif /* defined(__interconnectgame__eleven_asset_client__) */
