/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        thread.cpp
 * PURPOSE:     Generic thread class
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <debug.h>
#include <assert.h>
#include <windows.h>
#include <thread.h>

// This is the thread entry code
DWORD WINAPI ThreadEntry(LPVOID parameter)
{
	ThreadData *p = (ThreadData*) parameter;

    p->ClassPtr->Execute();

	SetEvent(p->hFinished);
	return 0;
}

// Default constructor
CThread::CThread()
{
	bTerminated = FALSE;
	// Points to the class that is executed within thread
	Data.ClassPtr = this;
	// Create synchronization event
	Data.hFinished = CreateEvent(NULL, TRUE, FALSE, NULL);

	// FIXME: Do some error handling
	assert(Data.hFinished != NULL);

	// Create thread
    hThread = CreateThread(NULL, 0, ThreadEntry, &Data, 0, &dwThreadId);

	// FIXME: Do some error handling
	assert(hThread != NULL);
}

// Default destructor
CThread::~CThread()
{
	if ((hThread != NULL) && (Data.hFinished != NULL)) {
		if (!bTerminated)
			Terminate();
		WaitForSingleObject(Data.hFinished, INFINITE);
		CloseHandle(Data.hFinished);
		CloseHandle(hThread);
		hThread = NULL;
	}
}

// Execute thread code
void CThread::Execute()
{
	while (!bTerminated) Sleep(0);
}

// Post a message to the thread's message queue
BOOL CThread::PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return PostThreadMessage(dwThreadId, Msg, wParam, lParam);
}

// Gracefully terminate thread
void CThread::Terminate()
{
	bTerminated = TRUE;
}

// Returns TRUE if thread is terminated, FALSE if not
BOOL CThread::Terminated()
{
	return bTerminated;
}
