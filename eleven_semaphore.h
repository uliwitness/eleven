//
//  eleven_semaphore.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-29.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectserver__eleven_semaphore__
#define __interconnectserver__eleven_semaphore__

#include <mutex>
#include <condition_variable>


namespace eleven
{

	class semaphore
	{  
	public:
		semaphore() : mCount(0) {}
		
		void reset()
		{
			std::unique_lock<std::mutex> lock(mCountLock);
			mCount = 0;
		}
		
		void signal()
		{
			std::unique_lock<std::mutex> lock(mCountLock);
			
			++mCount;
			
			mCondition.notify_one();
		}
		
		void wait()
		{
			std::unique_lock<std::mutex> lock(mCountLock);
			
			while( !mCount )
				mCondition.wait(lock);
			
			--mCount;
		}
		
	protected:
		std::mutex				mCountLock;
		std::condition_variable	mCondition;
		unsigned int			mCount;
	};
 
 
}

#endif /* defined(__interconnectserver__eleven_semaphore__) */
