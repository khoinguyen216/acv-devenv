#include "ThreadManager.h"

namespace BoatDetection {

//! Global maximum number of threads. Initialised to boost::thread::hardware_concurrency().
int g_maxNbThreads = boost::thread::hardware_concurrency();


void setMaxNbThreads(int nb) {g_maxNbThreads = nb;}
int getMaxNbThreads(void) {return g_maxNbThreads;}

unsigned int ThreadManager::sm_nExceptions = 0;
boost::recursive_mutex ThreadManager::sm_exceptionMutex;
boost::exception_ptr ThreadManager::sm_childThreadException;
boost::thread::id ThreadManager::sm_mainThreadId = boost::thread::id();

ThreadManager::ThreadManager(void) :
	m_maxNbThreads(g_maxNbThreads), m_nbActiveThreads(0),
	m_nextThread(0), m_readyToLaunch(true)
{
	if (m_maxNbThreads<1) m_maxNbThreads=1;
	
	/*if (getVerbosityLevel()>THREAD_MANAGER_MESSAGE_LEVEL) {
		sprintf(g_message, "Multi-Thread (%d)", m_maxNbThreads);
		MESSAGE( g_message, THREAD_MANAGER_MESSAGE_LEVEL);
	}
	*/
	try {
		m_threads = new boost::thread[m_maxNbThreads];
	} catch (...) {exit(0);}
}

ThreadManager::~ThreadManager(void) {
	JoinAll();
	delete [] m_threads; m_threads=NULL;
}

void ThreadManager::IncreaseExceptionNumber(void) {
	//Thread management. One by one.
	boost::recursive_mutex::scoped_lock lock(sm_exceptionMutex);
	++sm_nExceptions;	
}

void ThreadManager::SetChildThreadException(const boost::exception_ptr &error) {
	boost::recursive_mutex::scoped_lock lock(sm_exceptionMutex);
	sm_childThreadException = error;
}

int ThreadManager::RequestNewThread(bool &hasBeenJoined) {
	hasBeenJoined = false;
	if (m_readyToLaunch) {
		// Nothing to do;
	} else {
		if (m_nbActiveThreads<m_maxNbThreads) {
			// List is not full.
			m_nextThread = m_nbActiveThreads;
		} else {
			// Need to wait for one thread to finish
			bool finished = false;
			while (!finished) {
				// Active join to find an available slot.
				m_nextThread = (m_nextThread+1)%m_maxNbThreads;
				try {
					finished = m_threads[m_nextThread].timed_join( boost::posix_time::millisec(1) );
				} catch (...) {
					std::cout << "Thread timed join failed. Continue." << std::endl;
				}
			};
			m_nbActiveThreads--;
			/*if (getVerbosityLevel() > THREAD_MANAGER_MESSAGE_LEVEL) {
				sprintf(g_message, "[%d] joined. %d active", m_nextThread, m_nbActiveThreads);
				MESSAGE( g_message, THREAD_MANAGER_MESSAGE_LEVEL);
			}*/
			hasBeenJoined = true;
		}
	}
	m_readyToLaunch = true;
	return m_nextThread;
}

void ThreadManager::JoinAll(void) {
	for (int i=0; i<m_nbActiveThreads; i++) {
		try {
			m_threads[i].join();
		} catch (...) {
			//TERMINATE(INVALID_INSTRUCTION, "Thread join failed..");
			exit (0);
		}
		
	}
	m_nbActiveThreads=0;
	m_nextThread=0;
	m_readyToLaunch=true;
	if (sm_childThreadException) {
		// To avoid the second time exception thrown since the destructor will call JoinAll one more time 
		sm_childThreadException = boost::exception_ptr();
		exit (0);
	}
}

} //namespace BoatDetection