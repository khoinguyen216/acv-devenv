#pragma once
#include "ThreadManager.h"

#ifdef HAVE_THREADS
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#endif

namespace BoatDetection{

/**
 * Frequently used tools to synchronize a main thread with a single background worker (reader thread)
 * Mainly used by Measurements class
 *
 * The mutex protects a member of Measurement Class while it is being initialized by a background thread.
 * Main thread tasks: 1- Lock the mutex, 2- Returns if isInPreparation(), 3- setInPreparation(), 4- Launch background thread, 5- Release lock and continue initializing other members.
 * Background thread tasks: 1- Lock the mutex, 2- Allocate or initialize the protected variable, 3- setReady(), 4- Release lock and continue
 * Main thread tasks to get access to the protected member: 1- WaitUntilReady() 2- Lock the mutex, 3- Return the protected member. It is now initialized and next operations can be performed safely.
 */
class ThreadSynchronization
{
private:
	/**
	 * Disable the default copy constructor since we still don't want to implement it
	 */
	THREAD_MANAGER_API ThreadSynchronization(const ThreadSynchronization&);
	/**
	 * Disable the default operator "=" since we still don't want to implement it
	 */
	THREAD_MANAGER_API ThreadSynchronization& operator = (const ThreadSynchronization&);
public:
	/**
	 * Constructor
	 */
	THREAD_MANAGER_API ThreadSynchronization(void);

	/**
	 * Destructor
	 */
	THREAD_MANAGER_API virtual ~ThreadSynchronization(void);

protected:
#ifdef HAVE_THREADS
	/**
	 * Mutex which protects the selected member in Measurements class
	 */
	boost::recursive_mutex m_mutex;

	/**
	 * Waiting condition until initialization is complete.
	 */
	boost::condition m_cond;
#endif
	
	/**
	 * Set to true by main thread when launching the thread. Released by calling setReady() from the background thread.
	 */
	bool m_isInPreparation;

	/**
	 * Set to true by background thread when member is ready
	 */
	bool m_isReady;

public:
#ifdef HAVE_THREADS
	THREAD_MANAGER_API boost::recursive_mutex& getMutex(void);
#endif
	THREAD_MANAGER_API void WaitUntilReady(void);
	THREAD_MANAGER_API void setInPreparation(void);
	THREAD_MANAGER_API bool isInPreparation(void);
	THREAD_MANAGER_API void setReady(void);
	THREAD_MANAGER_API void unsetReady(void);
};

}
