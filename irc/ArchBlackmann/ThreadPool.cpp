// ThreadPool.cpp
// This file is (C) 2003-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses

#include <vector>
using std::vector;
#include "ThreadPool.h"
#include "QueueT.h"
#include "auto_vector.h"
#include "verify.h"
#include "ReliMT.h"

class PoolableThread : public ActiveObject
{
public:
	PoolableThread ( ThreadPoolImpl& );
	~PoolableThread()
	{
		Kill();
	}
	void InitThread();
	void Run();
	void FlushThread();

	ThreadPoolImpl& _pool;
};

class ThreadPoolLaunchData
{
public:
	ThreadPoolFunc* pFun;
	void* pArg;
};

template <class T>
class AtomicCounter : public Uncopyable
{
	Mutex _m;
	T _t;
public:
	AtomicCounter ( T init = 0 ) : _t(init)
	{
	}
	AtomicCounter ( const AtomicCounter<T>& t )
	{
		//Mutex::Lock l ( _m ); // don't need to lock since this is a ctor
		Mutex::Lock l2 ( t._m );
		_t = t._t;
	}
	const AtomicCounter<T>& operator = ( const AtomicCounter<T>& t )
	{
		Mutex::Lock l ( _m );
		Mutex::Lock l2 ( t._m );
		_t = t._t;
		return *this;
	}
	T operator ++ ()
	{
		Mutex::Lock l ( _m );
		T t = _t++;
		return t;
	}
	const AtomicCounter<T>& operator ++ ( int )
	{
		Mutex::Lock l ( _m );
		++_t;
		return *this;
	}
	T operator -- ()
	{
		Mutex::Lock l ( _m );
		T t = _t--;
		return t;
	}
	const AtomicCounter<T>& operator -- ( int )
	{
		Mutex::Lock l ( _m );
		--_t;
		return *this;
	}
	const AtomicCounter<T>& operator += ( T t )
	{
		Mutex::Lock l ( _m );
		return _t += t;
		return *this;
	}
	const AtomicCounter<T>& operator -= ( T t )
	{
		Mutex::Lock l ( _m );
		return _t -= t;
		return *this;
	}
	operator const T& () const
	{
		//Mutex::Lock l ( _m );
		return _t;
	}
	T operator !() const
	{
		//Mutex::Lock l ( _m );
		return !_t;
	}
};

class ThreadPoolImpl : public Uncopyable
{
public:
	ThreadPoolImpl() : _isDying(false), _idleThreads(0)
	{
	}

	~ThreadPoolImpl()
	{
	}

	void Shutdown()
	{
		_isDying = true;
		while ( _idleThreads )
		{
			_threadWaitEvent.Release();
			_threadStartEvent.Wait(); // let thread actually get a grip
		}
	}

	void Launch ( ThreadPoolFunc* pFun, void* pArg )
	{
		// this mutex is necessary to make sure we never have a conflict
		// between checking !_idleThreads and the call to _threadStartEvent.Wait()
		// basically if 2 threads call Launch() simultaneously, and there is only
		// 1 idle thread, it's possible that a new thread won't be created to
		// satisfy the 2nd request until an existing thread finishes.
		Mutex::Lock launchlock ( _launchMutex );

		ASSERT ( pFun );
		ThreadPoolLaunchData* data;
		{
			Mutex::Lock lock ( _vectorMutex );
			if ( !_spareData.size() )
				_spareData.push_back ( new ThreadPoolLaunchData() );
			data = _spareData.pop_back().release();
			if ( !_idleThreads )
				_threads.push_back ( new PoolableThread(*this) );
		}

		data->pFun = pFun;
		data->pArg = pArg;
		verify ( _pendingData.Add ( data ) );
		_threadWaitEvent.Release(); // tell a thread to do it's thing...
		_threadStartEvent.Wait(); // wait on a thread to pick up the request
	}

	// functions for threads to call...
	ThreadPoolLaunchData* GetPendingData()
	{
		ThreadPoolLaunchData* data = NULL;
		++_idleThreads;
		_threadWaitEvent.Wait(); // waits until there's a request
		--_idleThreads;
		_threadStartEvent.Release(); // tell requester we got it
		if ( _isDying )
			return NULL;
		_pendingData.Get ( data );
		ASSERT ( data );
		return data;
	}

	void RecycleData ( ThreadPoolLaunchData* data )
	{
		Mutex::Lock lock ( _vectorMutex );
		_spareData.push_back ( data );
	}

	bool _isDying;
	Mutex _vectorMutex, _launchMutex;
	auto_vector<PoolableThread> _threads;
	auto_vector<ThreadPoolLaunchData> _spareData;
	CQueueT<ThreadPoolLaunchData*> _pendingData;
	Event _threadWaitEvent, _threadStartEvent;
	AtomicCounter<int> _idleThreads;
};

///////////////////////////////////////////////////////////////////////////////
// ThreadPool

/*static*/ ThreadPool& ThreadPool::Instance()
{
	static ThreadPool tp;
	return tp;
}

ThreadPool::ThreadPool() : _pimpl ( new ThreadPoolImpl )
{
};

ThreadPool::~ThreadPool()
{
	_pimpl->Shutdown();
	delete _pimpl;
	_pimpl = 0;
}

void ThreadPool::Launch ( ThreadPoolFunc* pFun, void* pArg )
{
	_pimpl->Launch ( pFun, pArg );
}

int ThreadPool::IdleThreads()
{
	return _pimpl->_idleThreads;
}

///////////////////////////////////////////////////////////////////////////////
// PoolableThread

PoolableThread::PoolableThread ( ThreadPoolImpl& pool ) : _pool(pool)
{
	Start();
}

void PoolableThread::InitThread()
{
}

void PoolableThread::Run()
{
	ThreadPoolLaunchData* data;
	while ( !_isDying )
	{
		data = _pool.GetPendingData(); // enter wait state if none...
		if ( !data ) // NULL data means kill thread
			break;
		(*data->pFun) ( data->pArg ); // call the function
		_pool.RecycleData ( data );
	}
}

void PoolableThread::FlushThread()
{
}
