// ReliMT.cpp
// lots of code here is (c) Bartosz Milewski, 1996, www.relisoft.com
// The rest is (C) 2002-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  define snprintf _snprintf
#elif defined(UNIX)
#  include <errno.h>
#  include <sys/sem.h>
#else
#  error unrecognized target
#endif//WIN32|UNIX
#include "verify.h"
#include "ReliMT.h"

////////////////////////////////////////////////////////////////////////////////
// Assert

void _wassert ( char* szExpr, char* szFile, int line )
{
	fprintf ( stderr, "Assertion Failure: \"%s\" in file %s, line %d", szExpr, szFile, line );
	exit (-1);
}


////////////////////////////////////////////////////////////////////////////////
// Thread

Thread::Thread ( long (THREADAPI * pFun) (void* arg), void* pArg )
{
#ifdef WIN32
	verify ( _handle = CreateThread (
		0, // Security attributes
		0, // Stack size
		(DWORD (WINAPI*)(void*))pFun,
		pArg,
		0, // don't create suspended.
		&_tid ));
#elif defined(UNIX)
	// set up the thread attribute: right now, we do nothing with it.
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// this will make the threads created by this process really concurrent
	verify ( !pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) );

	// create the new OS thread object
	verify ( !pthread_create ( &_threadId, &attr, (void* (*) (void*))pFun, pArg ) );

	verify ( !pthread_attr_destroy(&attr) );
#else
#  error unrecognized target
#endif//WIN32|UNIX
}

Thread::~Thread()
{
#ifdef WIN32
	verify ( CloseHandle ( _handle ) );
#elif defined(UNIX)
	verify ( !pthread_cancel ( _threadId ) );
#else
#  error unrecognized target
#endif//WIN32|UNIX
}

/*void Thread::Resume()
{
#ifdef WIN32
	ResumeThread (_handle);
#elif defined(UNIX)
#  error how to resume thread in unix?
#else
#  error unrecognized target
#endif//WIN32|UNIX
}*/

void Thread::WaitForDeath()
{
#ifdef WIN32
	DWORD dw = WaitForSingleObject ( _handle, 2000 );
	ASSERT ( dw != WAIT_FAILED );
#elif defined(UNIX)
	verify ( !pthread_join ( _threadId, (void**)NULL ) );
#else
#  error unrecognized target
#endif//WIN32|UNIX
}


////////////////////////////////////////////////////////////////////////////////
// ActiveObject

// The constructor of the derived class
// should call
//    _thread.Resume();
// at the end of construction

ActiveObject::ActiveObject() : _isDying (0), _thread (0)
{
}

ActiveObject::~ActiveObject()
{
	ASSERT ( !_thread ); // call Kill() from subclass's dtor
	// Kill() - // You can't call a virtual function from a dtor, EVEN INDIRECTLY
	// so, you must call Kill() in the subclass's dtor
}

// FlushThread must reset all the events on which the thread might be waiting.
void ActiveObject::Kill()
{
	if ( _thread )
	{
		_isDying++;
		FlushThread();
		// Let's make sure it's gone
		_thread->WaitForDeath();
		delete _thread;
		_thread = 0;
	}
}

void ActiveObject::Start()
{
	ASSERT ( !_thread );
	_thread = new Thread ( ThreadEntry, this );
}

long THREADAPI ActiveObject::ThreadEntry ( void* pArg )
{
	ActiveObject * pActive = (ActiveObject*)pArg;
	pActive->InitThread();
	pActive->Run();
	pActive->Kill();
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Mutex

Mutex::Mutex()
{
#ifdef WIN32
	verify ( _h = CreateMutex ( NULL, FALSE, NULL ) );
#elif defined(UNIX)
	pthread_mutexattr_t attrib;
	verify ( !pthread_mutexattr_init( &attrib ) );
	// allow recursive locks
	verify ( !pthread_mutexattr_settype( &attrib, PTHREAD_MUTEX_RECURSIVE ) );
	verify ( !pthread_mutex_init ( &_mutex, &attrib ) );
#else
#  error unrecognized target
#endif
}

Mutex::~Mutex()
{
#ifdef WIN32
	verify ( CloseHandle ( _h ) );
#elif defined(UNIX)
	verify ( !pthread_mutex_destroy(&_mutex) );
#else
#  error unrecognized target
#endif
}

void Mutex::Acquire()
{
#ifdef WIN32
	DWORD dw = WaitForSingleObject ( _h, INFINITE );
	ASSERT ( dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED );
#elif defined(UNIX)
	verify ( !pthread_mutex_lock(&_mutex) );
#else
#  error unrecognized target
#endif
}

bool Mutex::TryAcquire()
{
#ifdef WIN32
	DWORD dw = WaitForSingleObject ( _h, 1 );
	ASSERT ( dw == WAIT_OBJECT_0 || dw == WAIT_TIMEOUT || dw == WAIT_ABANDONED );
	return (dw != WAIT_TIMEOUT);
#elif defined(UNIX)
	int err = pthread_mutex_trylock(&_mutex);
	ASSERT ( err == EBUSY || err == 0 );
	return (err == 0);
#else
#  error unrecognized target
#endif
}

void Mutex::Release()
{
#ifdef WIN32
	verify ( ReleaseMutex ( _h ) );
#elif defined(UNIX)
	verify ( !pthread_mutex_unlock(&_mutex) );
	// we could allow EPERM return value too, but we are forcing user into RIIA
#else
#  error unrecognized target
#endif
}

Mutex::Lock::Lock ( Mutex& m ) : _m(m)
{
	_m.Acquire();
}

Mutex::Lock::~Lock()
{
	_m.Release();
}

Mutex::TryLock::TryLock ( Mutex& m ) : _m(m)
{
	_bLocked = _m.TryAcquire();
}

Mutex::TryLock::~TryLock()
{
	if ( _bLocked )
		_m.Release();
}

///////////////////////////////////////////////////////////////////////////////
// Event

Event::Event()
{
#ifdef WIN32
	// start in non-signaled state (red light)
	// auto reset after every Wait
	verify ( _handle = CreateEvent ( 0, FALSE, FALSE, 0 ) );
#elif defined(UNIX)
	//verify ( !pthread_cond_init ( &_cond, NULL /* default attributes */) );
	sem_init();
	//verify(sem_init());
#else
#  error unrecognized target
#endif
}

Event::~Event()
{
#ifdef WIN32
	verify ( CloseHandle ( _handle ) );
#elif defined(UNIX)
	//verify ( !pthread_cond_destroy ( &_cond ) );
	sem_destroy();
#else
#  error unrecognized target
#endif
}

void Event::Release() // put into signaled state
{
#ifdef WIN32
	verify ( SetEvent ( _handle ) );
#elif defined(UNIX)
	//verify ( !pthread_cond_signal ( &_cond ) );
	verify(!sem_V());
#else
#  error unrecognized target
#endif
}

void Event::Wait()
{
#ifdef WIN32
	// Wait until event is in signaled (green) state
	DWORD dw = WaitForSingleObject ( _handle, INFINITE );
	ASSERT ( dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED );
#elif defined(UNIX)
	// According to docs: The pthread_cond_wait() and pthread_cond_timedwait()
	// functions are used to block on a condition variable. They are called
	// with mutex locked by the calling thread or undefined behaviour will
	// result.
	//Mutex::Lock lock ( _mutex );
	//verify ( !pthread_cond_wait ( &_cond, _mutex ) );
	verify(!sem_P());
#else
#  error unrecognized target
#endif
}

#ifdef UNIX
void Event::sem_init()
{
	sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
	ASSERT(sem_id != -1);
}

int Event::sem_P()
{
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	return semop(sem_id, &sb, 1);
}

int Event::sem_V()
{
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op =  1;
	sb.sem_flg = 0;
	return semop(sem_id, &sb, 1);
}

void Event::sem_destroy()
{
#ifdef MACOSX
	semun mactmp;
	mactmp.val = 0;
	semctl(sem_id, 0, IPC_RMID, mactmp);
#else
	semctl(sem_id, 0, IPC_RMID, 0);
#endif
}
#endif

