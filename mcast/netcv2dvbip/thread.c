#include "defs.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef APPLE
#include <malloc.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <string.h>

#include "thread.h"
#include "misc.h"

// --- cCondWait -------------------------------------------------------------

cCondWait::cCondWait(void)
{
	signaled = false;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

cCondWait::~cCondWait()
{
	pthread_cond_broadcast(&cond); // wake up any sleepers
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

void cCondWait::SleepMs(int TimeoutMs)
{
	cCondWait w;
	// making sure the time is >2ms to avoid a possible busy wait
	// (no longer true for later 2.6 kernels)
	w.Wait(xmax(TimeoutMs, 3));
}

bool cCondWait::Wait(int TimeoutMs)
{
	pthread_mutex_lock(&mutex);
	if (!signaled) {
		if (TimeoutMs) {
			struct timespec abstime;
			if (GetAbsTime(&abstime, TimeoutMs)) {
				while (!signaled) {
					if (pthread_cond_timedwait(&cond, 
						&mutex, &abstime) == ETIMEDOUT)
					break;
				}
			}
		}
		else
			pthread_cond_wait(&cond, &mutex);
	}
	bool r = signaled;
	signaled = false;
	pthread_mutex_unlock(&mutex);
	return r;
}

void cCondWait::Signal(void)
{
	pthread_mutex_lock(&mutex);
	signaled = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}

// --- cCondVar --------------------------------------------------------------

cCondVar::cCondVar(void)
{
	pthread_cond_init(&cond, 0);
}

cCondVar::~cCondVar()
{
	pthread_cond_broadcast(&cond); // wake up any sleepers
	pthread_cond_destroy(&cond);
}

void cCondVar::Wait(cMutex &Mutex)
{
	if (Mutex.locked) {
		int locked = Mutex.locked;
		Mutex.locked = 0; // have to clear the locked count here,
				  // as pthread_cond_wait
				  // does an implicit unlock of the mutex
		pthread_cond_wait(&cond, &Mutex.mutex);
		Mutex.locked = locked;
	}
}

bool cCondVar::TimedWait(cMutex &Mutex, int TimeoutMs)
{
	bool r = true; // true = condition signaled, false = timeout

	if (Mutex.locked) {
		struct timespec abstime;
		if (GetAbsTime(&abstime, TimeoutMs)) {
			int locked = Mutex.locked;
			Mutex.locked = 0; // have to clear the locked count
					  // here, as pthread_cond_timedwait
					  // does an implicit mutex unlock.
			if (pthread_cond_timedwait(&cond, &Mutex.mutex, 
				&abstime) == ETIMEDOUT)
			r = false;
			Mutex.locked = locked;
		}
	}
	return r;
}

void cCondVar::Broadcast(void)
{
	pthread_cond_broadcast(&cond);
}



// --- cMutex ----------------------------------------------------------------

cMutex::cMutex(void)
{
	locked = 0;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
#ifndef APPLE
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
	pthread_mutex_init(&mutex, &attr);
}

cMutex::~cMutex()
{
	pthread_mutex_destroy(&mutex);
}

void cMutex::Lock(void)
{
	pthread_mutex_lock(&mutex);
	locked++;
}

void cMutex::Unlock(void)
{
	if (!--locked)
		pthread_mutex_unlock(&mutex);
}
//---cMutex--------------------------------------------------------------------


cThread::cThread(const char *Description)
{
	active = running = false;
#ifdef WIN32
	memset(&childTid, 0, sizeof(childTid));
#else
	childTid = 0;
#endif
	description = NULL;
	if (Description)
	{
		description = strdup(Description);
	}
}

cThread::~cThread()
{
	Cancel(); // just in case the derived class didn't call it
	free(description);
}

void *cThread::StartThread(cThread *Thread)
{
	if (Thread->description && !quiet) 
	printf("%s thread started.\n", Thread->description);
    
	Thread->Action();
	if (Thread->description && !quiet)
		printf("%s thread ended.\n", Thread->description);
	Thread->running = false;
	Thread->active = false;
	return NULL;
}

// ms to wait for a thread to stop before newly starting it
#define THREAD_STOP_TIMEOUT  3000 
// ms to sleep while waiting for a thread to stop
#define THREAD_STOP_SLEEP      30 

bool cThread::Start(void)
{
	if (!running) {
		if (active) {
			// Wait until the previous incarnation 
			// of this thread has completely ended
			// before starting it newly:
			cTimeMs RestartTimeout;
			while (!running && active && RestartTimeout.Elapsed() < 
			    THREAD_STOP_TIMEOUT)
				cCondWait::SleepMs(THREAD_STOP_SLEEP);
		}
		if (!active) {
			active = running = true;
			if (pthread_create(&childTid, NULL, (void *(*) (void *))
			    &StartThread, (void *)this) == 0) {
				pthread_detach(childTid); // auto-reap
			} else {
				printf("%s thread cannot be created.\n", 
					description);
				active = running = false;
				return false;
			}
		}
	}
	return true;
}


bool cThread::Active(void)
{
	if (active) {
		//
		// Single UNIX Spec v2 says:
		//
		// The pthread_kill() function is used to request
		// that a signal be delivered to the specified thread.
		//
		// As in kill(), if sig is zero, error checking is
		// performed but no signal is actually sent.
		//
		int err;
		if ((err = pthread_kill(childTid, 0)) != 0) {
			if (err != ESRCH)
			printf("%s thread cannot be killed.\n", description);
#ifdef WIN32
			memset(&childTid, 0, sizeof(childTid));
#else
			childTid = 0;
#endif
			active = running = false;
		} else
			return true;
	}
	return false;
}

void cThread::Cancel(int WaitSeconds)
{
	running = false;
	if (active && WaitSeconds > -1) {
		if (WaitSeconds > 0) {
			for (time_t t0 = time(NULL) + WaitSeconds; 
			    time(NULL) < t0; ) {
				if (!Active())
					return;
				cCondWait::SleepMs(10);
			}
			printf("ERROR: %s thread won't end (waited %d seconds)"
				" - canceling it...\n", description ? 
				description : "", WaitSeconds);
		}
		pthread_cancel(childTid);
#ifdef WIN32
		memset(&childTid, 0, sizeof(childTid));
#else
		childTid = 0;
#endif
		active = false;
	}
}

// --- cMutexLock ------------------------------------------------------------

cMutexLock::cMutexLock(cMutex *Mutex)
{
	mutex = NULL;
	locked = false;
	Lock(Mutex);
}

cMutexLock::~cMutexLock()
{
	if (mutex && locked)
		mutex->Unlock();
}

bool cMutexLock::Lock(cMutex *Mutex)
{
	if (Mutex && !mutex) {
		mutex = Mutex;
		Mutex->Lock();
		locked = true;
		return true;
	}
	return false;
}

// --- cThreadLock -----------------------------------------------------------

cThreadLock::cThreadLock(cThread *Thread)
{
	thread = NULL;
	locked = false;
	Lock(Thread);
}

cThreadLock::~cThreadLock()
{
	if (thread && locked)
		thread->Unlock();
}

bool cThreadLock::Lock(cThread *Thread)
{
	if (Thread && !thread) {
		thread = Thread;
		Thread->Lock();
		locked = true;
		return true;
	}
	return false;
}
