// ThreadPool.h
// This file is (C) 2003-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "ReliMT.h"

typedef void THREADAPI ThreadPoolFunc ( void* );

class ThreadPoolImpl;

class ThreadPool : public Uncopyable
{
public:
	static ThreadPool& Instance();
	~ThreadPool();

	void Launch ( ThreadPoolFunc* pFun, void* pArg );
	int IdleThreads();

private:
	ThreadPool();
	ThreadPoolImpl *_pimpl;
};

#endif// THREADPOOL_H
