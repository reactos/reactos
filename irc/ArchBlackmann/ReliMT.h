// ReliMT.h
// lots of code here is (c) Bartosz Milewski, 1996, www.relisoft.com
// The rest is (C) 2003-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses


#ifndef __RELIMT_H
#define __RELIMT_H

#include "Reli.h"

#ifdef WIN32
#  ifndef _WINDOWS_
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#  endif
#  define THREADAPI WINAPI
#elif defined(UNIX)
#  include <pthread.h>
#  include <stdlib.h>
#  include "string.h"
#  include <sys/types.h> //Semaphore
#  include <sys/ipc.h> //Semaphore
#  include <sys/sem.h> //Semaphore
#  define THREADAPI
#else
#  error unrecognized target
#endif


////////////////////////////////////////////////////////////////////////////////
// Thread

class Thread : public Uncopyable
{
public:
	Thread ( long (THREADAPI * pFun) (void* arg), void* pArg );
	~Thread();
	//void Resume();
	void WaitForDeath();

	// platform-specific stuff:
private:
#ifdef WIN32
	HANDLE _handle;
	DWORD  _tid;     // thread id
#elif defined(UNIX)
	pthread_t _threadId;   // id of the thread
#else
#  error unrecognized target
#endif
	//DECLARE_PTR(Thread,(long (THREADAPI * pFun) (void* arg), void* pArg),(pFun,pArg));
}; //DECLARE_SPTR(Thread,(long (THREADAPI * pFun) (void* arg), void* pArg),(pFun,pArg));

////////////////////////////////////////////////////////////////////////////////
// ActiveObject

class ActiveObject : public Uncopyable
{
public:
	ActiveObject();
	virtual ~ActiveObject();
	void Kill();
	void Start();

protected:
	virtual void InitThread() = 0;
	virtual void Run() = 0;
	virtual void FlushThread() = 0;

	int             _isDying;

	static long THREADAPI ThreadEntry ( void *pArg );
	Thread         *_thread;

	//DECLARE_PTRV(ActiveObject);
}; //DECLARE_SPTRV(ActiveObject);

// Last thing in the constructor of a class derived from
// ActiveObject you must call
//    Start();
// Inside the loop the Run method you must keep checking _isDying
//    if (_isDying)
//         return;
// FlushThread must reset all the events on which the thread might be waiting.
// Example:
#if 0
// MyAsyncOutputter - class that outputs strings to a file asynchronously
class MyAsyncOutputter : public ActiveObject
{
public:
	MyAsyncOutputter ( const string& filename ) : _filename(filename), _currentBuf(0)
	{
		Start(); // start thread
	}
	void InitThread()
	{
		_f.open ( _filename, "wb" );
	}
	void Output ( const string& s )
	{
		{
			// acquire lock long enough to add the string to the active buffer
			Mutex::Lock lock ( _mutex );
			_buf[_currentBuf].push_back ( s );
		}
		_event.Release(); // don't need the lock fire the event
	}
	void Run()
	{
		while ( !_isDying )
		{
			// wait for signal from Output() or FlushThread()
			_event.Wait();
			{
				// acquire lock long enough to switch active buffers
				Mutex::Lock lock ( _mutex );
				_currentBuf = 1-_currentBuf;
				ASSERT ( !_buf[_currentBuf].size() );
			}
			// get a reference to the old buffer
			vector<string>& buf = _buf[1-_currentBuf];
			// write each string out to file and then empty the buffer
			for ( int i = 0; i < buf.size(); i++ )
				_f.write ( buf[i].c_str(), buf[i].size() );
			buf.resize(0);
		}
	}
	void FlushThread()
	{
		// _isDying is already set: signal thread so it can see that too
		_event.Release();
	}
private:
	string _filename;
	File _f;
	int _currentBuf;
	vector<string> _buf[2];
	Event _event;
	Mutex _mutex;
};
#endif

////////////////////////////////////////////////////////////////////////////////
// Mutex

class Mutex : public Uncopyable
{
public:
	Mutex();
	~Mutex();
private:
	void Acquire();
	bool TryAcquire();
	void Release();
public:
	// sub-class used to lock the Mutex
	class Lock : public Uncopyable
	{
	public:
		Lock ( Mutex& m );
		~Lock();

	private:
		// private data
		Mutex& _m;
	};
	friend class Mutex::Lock;


	// sub-class used to attempt to lock the mutex. Use operator bool()
	// to test if the lock was successful
	class TryLock : public Uncopyable
	{
	public:
		TryLock ( Mutex& m );
		~TryLock();
		operator bool () { return _bLocked; }

	private:
		// private data
		bool _bLocked;
		Mutex& _m;
	};
	friend class Mutex::TryLock;

private:
	// platform-specific stuff:
#ifdef WIN32
	HANDLE _h;
#elif defined(UNIX)
	pthread_mutex_t _mutex;
public: operator pthread_mutex_t* () { return &_mutex; }
#else
#  error unrecognized target
#endif
};

////////////////////////////////////////////////////////////////////////////////
// Event

class Event : public Uncopyable
{
public:
	Event();
	~Event();
	void Release(); // put into signaled state
	void Wait();

private:
#ifdef WIN32
	HANDLE _handle;
#elif defined(UNIX)
	//Sem util functions
	void sem_init();
	int sem_P();
	int sem_V();
	void sem_destroy();

	int sem_id;
	//pthread_cond_t _cond;
	//Mutex _mutex;
#else
#  error unrecognized target
#endif
	//DECLARE_PTR(Event,(),());
}; //DECLARE_SPTR(Event,(),());

#endif//__RELIWIN32_H
