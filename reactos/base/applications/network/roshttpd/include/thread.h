/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/thread.h
 */
#ifndef __THREAD_H
#define __THREAD_H

#include <windows.h>

class CThread;

struct ThreadData {
	CThread *ClassPtr;
	HANDLE hFinished;
};

class CThread {
public:
	CThread();
	virtual ~CThread();
	BOOL PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	virtual void Execute();
	virtual void Terminate();
	BOOL Terminated();
protected:
	BOOL bTerminated;
	DWORD dwThreadId;
	HANDLE hThread;
	ThreadData Data;
};
typedef CThread *LPCThread;

#endif /* __THREAD_H */
