//
//  eleven_channel.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-02.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_channel__
#define __eleven__eleven_channel__


#include <string>
#include <vector>


namespace eleven
{

	#ifndef __eleven__eleven_users__
	typedef uint32_t	user_id;
	#endif


	class channel
	{
	public:
		channel( std::string inName ) : mChannelName(inName) {};
		
		bool	sendln( std::string inMessage );
		
	protected:
		std::string				mChannelName;
		std::vector<user_id>	mBannedUsers;
		std::vector<user_id>	mUsers;
	};

}

#endif /* defined(__eleven__eleven_channel__) */
