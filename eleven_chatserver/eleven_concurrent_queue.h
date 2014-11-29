//
//  eleven_concurrent_queue.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-29.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __interconnectserver__eleven_concurrent_queue__
#define __interconnectserver__eleven_concurrent_queue__

#include "eleven_semaphore.h"
#include <mutex>
#include <functional>

namespace eleven
{
	
	template< class E >
	class concurrent_queue
	{
	public:
		struct element
		{
			element() : mNext(NULL) {};
			explicit element( const E& inPayload ) : mPayload(inPayload), mNext(NULL) {};
			
			E			mPayload;
			element*	mNext;
		};
		
		void	push( const E& inPayload )
		{
			element*	newElem = new element( inPayload );
			std::lock_guard<std::mutex>	lock(mHeadTailMutex);
			
			if( mTail )
			{
				mTail->mNext = newElem;
				mTail = newElem;
			}
			else
				mHead = mTail = newElem;
			
			mSemaphore.signal();
		}
		
		bool	pop( E& outPayload )
		{
			element* 	poppedElem  = NULL;
			{
				std::lock_guard<std::mutex>	lock(mHeadTailMutex);
				element* 					poppedElem = mHead;
				
				if( mHead )
				{
					mHead = poppedElem->mNext;
					if( !mHead )
						mTail = NULL;
				}
			}

			if( poppedElem )
			{
				outPayload = poppedElem->mPayload;
				return true;
			}
			else
				return false;
		}
		
		void	wait( std::function<bool(const E&)> newItemCallback )
		{
			bool	keepGoing = true;
			while( keepGoing )
			{
				mSemaphore.wait();
				
				E	payload;
				if( pop(payload) )
					keepGoing = newItemCallback( payload );
			}
		}
		
	protected:
		element*		mHead;
		element*		mTail;
		std::mutex		mHeadTailMutex;
		semaphore		mSemaphore;
	};
	
}


#endif /* defined(__interconnectserver__eleven_concurrent_queue__) */
