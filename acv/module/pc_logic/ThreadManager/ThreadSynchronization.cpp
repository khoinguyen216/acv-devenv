#pragma once

#include "ThreadSynchronization.h"

namespace BoatDetection{

ThreadSynchronization::ThreadSynchronization(void) 
#ifdef HAVE_THREADS
	:m_isReady(false), m_isInPreparation(false)
#endif
{
}

ThreadSynchronization::~ThreadSynchronization(void)
{
}

#ifdef HAVE_THREADS
boost::recursive_mutex& ThreadSynchronization::getMutex(void) {
	return m_mutex;
}
#endif

void ThreadSynchronization::WaitUntilReady(void) {
#ifdef HAVE_THREADS
	boost::recursive_mutex::scoped_lock lock(m_mutex);
	while (!m_isReady && m_isInPreparation)
		m_cond.wait(lock);
#else
	if (m_isInPreparation) 
		std::cout << "Still in preparation without multithreading ?" << std::endl;
	if (!m_isReady)
		std::cout << "Still not ready without multithreading ?" << std::endl;
#endif
}

void ThreadSynchronization::setInPreparation(void) {
#ifdef HAVE_THREADS
	boost::recursive_mutex::scoped_lock lock(m_mutex);
#endif
	m_isInPreparation = true;
	m_isReady=false;
#ifdef OT_HAVE_THREADS
	m_cond.notify_all();
#endif
}

bool ThreadSynchronization::isInPreparation(void) {	
#ifdef HAVE_THREADS
	boost::recursive_mutex::scoped_lock lock(m_mutex);
#endif
	return m_isInPreparation;
}

void ThreadSynchronization::setReady(void) {
#ifdef HAVE_THREADS
	boost::recursive_mutex::scoped_lock lock(m_mutex);
#endif
	m_isReady = true;
#ifdef HAVE_THREADS
	m_cond.notify_all();
#endif
}

void ThreadSynchronization::unsetReady(void) {
#ifdef HAVE_THREADS
	boost::recursive_mutex::scoped_lock lock(m_mutex);
#endif
	//m_isInPreparation = false;
	m_isReady=false;	
#ifdef HAVE_THREADS
	m_cond.notify_all();
#endif
}
} //namespace BoatDetection

