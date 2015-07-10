#pragma once
/** @defgroup multithreading Multithreading
 *  All multithreaded calls
 */

// Define a macro for exporting and importing DLL.
// THREAD_MANAGER_API is used for exporting DLL, when THREAD_MANAGER_EXPORT has been defined.
// THREAD_MANAGER_API is used for importing DLL, when THREAD_MANAGER_EXPORT hasn't been defined.
#ifdef WIN32
#ifdef THREAD_MANAGER_EXPORT
#define THREAD_MANAGER_API __declspec(dllexport)
#else
#define THREAD_MANAGER_API __declspec(dllimport)
#endif
#else
//For cygwin
#define THREAD_MANAGER_API
#endif

#ifdef THREAD_MANAGER_API_STATIC
#define THREAD_MANAGER_API
#endif

#define THREAD_MANAGER_MESSAGE_LEVEL 3
//#define HAVE_THREAD

#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
//#include "../MessageManager/MessageManager.h"
#include <boost/exception/all.hpp>
#include <boost/function.hpp>
#include <functional>


namespace BoatDetection {
//! Change the default number of threads.
THREAD_MANAGER_API void setMaxNbThreads(int nb);
THREAD_MANAGER_API int getMaxNbThreads(void);

THREAD_MANAGER_API void SetChildThreadException(const boost::exception_ptr &error);
THREAD_MANAGER_API boost::thread::id GetMainThreadId(void);

/**
 * Provides simple thread management using boost library.
 */
class ThreadManager
{
private:
	/**
	 * Disable the default copy constructor since we still don't want to implement it
	 */
	THREAD_MANAGER_API ThreadManager(const ThreadManager&);
	/**
	 * Disable the default operator "=" since we still don't want to implement it
	 */
	THREAD_MANAGER_API ThreadManager& operator = (const ThreadManager&);
protected:
	/**
	 * Group of threads.
	 */
	boost::thread *m_threads;
	/**
	 * Maximum number of simultaneous threads.
	 */
	int m_maxNbThreads;
	/**
	 * Number of active threads (0..m_maxNbThreads)
	 */
	int m_nbActiveThreads;
	/**
	 * Location in the m_threads of the next inserted thread. (0..m_maxNbThreads-1)
	 */
	int m_nextThread;
	/**
	 * Make sure that RequestNewThread and LaunchNewThread are called alternatively
	 */
	bool m_readyToLaunch;

	/**
	 * The number of exceptions thrown by the threads
	 */
	static unsigned int sm_nExceptions;

	/**
	 * The exception thrown by the child thread
	 */
	static boost::exception_ptr sm_childThreadException;

	/**
	 * The number of exceptions thrown by the threads
	 */
	static boost::recursive_mutex sm_exceptionMutex;

	/**
	 *
	 */
	static boost::thread::id sm_mainThreadId;

public:
	/**
	 * Constructor
	 */
	THREAD_MANAGER_API ThreadManager(void);
	/**
	 * Destructor
	 */
	THREAD_MANAGER_API ~ThreadManager(void);

	/**
	 * Wait until an available slot is available.
	 *
	 * Immediately returns if m_nbActiveThreads<m_maxNbThreads
	 * Or perform a timed join on any active thread if the array is full.
	 *
	 * @param hasBeenJoined True if the thread has been joined. You might want to collect the data with your main thread.
	 * @return next available thread index.
	 */
	THREAD_MANAGER_API int RequestNewThread(bool &hasBeenJoined);

	/**
	 * Creates a new thread.
	 *
	 * @param func Instructions to be launched (ex: boost::bind( &Class::Method, object, arg1, arg2...) )
	 */
	template<class Callable>
	void LaunchNewThread(Callable func);

	/**
	 * Wrap the target thread entry point in a try/catch block. Upon exception catching, give the exception pointer to the main thread.
	 *
	 * @param func Instructions to be launched (ex: boost::bind( &Class::Method, object, arg1, arg2...) )
	 */
	template<class Callable>
	void DoWork(Callable func);

	/**
	 * Join all threads.
	 */
	THREAD_MANAGER_API void JoinAll(void);

	/**
	 * Maximum number of threads for the system
	 */
	int getMaxNbThreads(void) {return m_maxNbThreads;}

	/**
	 * Current number of threads in this manager (not yet joined)
	 */
	int getNbActiveThreads(void) {return m_nbActiveThreads;}

	/**
	 * Incease the number of exceptions
	 */
	THREAD_MANAGER_API static void IncreaseExceptionNumber(void);

	/**
	 * Set the exception thrown by the child thread
	 *
	 * To let the main thread know there is at lease one exception thrown by the child threads
	 */
	THREAD_MANAGER_API static void SetChildThreadException(const boost::exception_ptr &error);
};

} //namespace cshot

namespace BoatDetection {
template<class Callable>
void ThreadManager::LaunchNewThread(Callable func) {
	// This is the main thread here.
	if (!m_readyToLaunch) {
		bool b;
		this->RequestNewThread(b);
	}
	// Call destructor and spawn a new thread.
	try {
		m_threads[m_nextThread] = boost::thread(
			boost::bind(
				&ThreadManager::DoWork< boost::_bi::protected_bind_t<Callable> >,
				this,
				boost::protect(func) // Prevent expansion of bind. Nest it in boost::protect
			));
		//m_threads[m_nextThread] = boost::thread(func);
		//this->DoWork<>(func);
	} catch(...) {
		std::cout << "Thread launch failed." << std::endl;
		exit(0);
	}
	m_nbActiveThreads++;
	m_readyToLaunch = false;

}

template<class Callable>
void ThreadManager::DoWork(Callable func) {
	// This is the child thread here.
	try {
		func(); //Un-protect and then call.
	} catch(...) {
		ThreadManager::SetChildThreadException( boost::current_exception());
		// NOTE: Expection is not thrown again, but thread should exit at the end of this method.
	}
}
}
