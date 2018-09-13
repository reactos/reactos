//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       eventlst.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  InitializeListLock
//              InitializeListEvent
//              LockWaitToWrite
//
//              *** local functions ***
//              LockInitialize
//
//  History:    29-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include        "global.hxx"

#include        "eventlst.h"

#define PCB_LIST_DEBUG 0

BOOL LockInitialize(LIST_LOCK *pListLock, DWORD dwDebugMask);


BOOL InitializeListLock(LIST_LOCK *psListLock, DWORD dwDebugMask)
{
    return(LockInitialize(psListLock, dwDebugMask));
}

BOOL InitializeListEvent(HANDLE *phListEvent)
{
    if (!(*phListEvent = CreateEvent(NULL, TRUE, TRUE, NULL)))
    {
        return(FALSE);
    }
    
    return(TRUE);
}

BOOL EventFree(HANDLE hListEvent)
{
    if ((hListEvent) && (hListEvent != INVALID_HANDLE_VALUE))
    {
        CloseHandle(hListEvent);
        return(TRUE);
    }

    return(FALSE);
}

BOOL LockInitialize(LIST_LOCK *pListLock, DWORD dwDebugMask) 
{
    //
    // Initialize the variable that indicates the number of 
    // reader threads that are reading.
    // Initially no reader threads are reading.
    //

    pListLock->dwDebugMask  = dwDebugMask;

    pListLock->NumReaders   = 0;

    pListLock->hMutexNoWriter = CreateMutex(NULL, FALSE, NULL);

    if (!(pListLock->hMutexNoWriter))
    {
        return(FALSE);
    }

    //
    // Create the manual-reset event that is signalled when  
    // no reader threads are reading.  Initially no reader   
    // threads are reading.                                  
    //

    pListLock->hEventNoReaders = CreateEvent(NULL, TRUE, TRUE, NULL);

    if (pListLock->hEventNoReaders)
    {
        return(TRUE);
    }
    else 
    {
        CloseHandle(pListLock->hMutexNoWriter);

        pListLock->hMutexNoWriter = NULL;

        return(FALSE);
    }
}

BOOL LockFree(LIST_LOCK *pListLock)
{
    if (pListLock->hEventNoReaders)
    {
        CloseHandle(pListLock->hEventNoReaders);
        pListLock->hEventNoReaders = NULL;
    }

    if (pListLock->hMutexNoWriter)
    {
        CloseHandle(pListLock->hMutexNoWriter);
        pListLock->hMutexNoWriter = NULL;
    }

    return(TRUE);
}

void LockWaitToWrite(LIST_LOCK *pListLock)
{
    //
    // We can write if the following are true:
    //
    // 1. The mutex guard is available and no
    //    other threads are writing.             
    //
    // 2. No threads are reading.
    //
    // Note that, unlike an rtl resource, this attempt
    // to write does not lock out other readers.  We
    // just have to wait patiently for our turn.
    // 

    HANDLE  ahObjects[2];

    ahObjects[0]    = pListLock->hMutexNoWriter;
    ahObjects[1]    = pListLock->hEventNoReaders;

    WaitForMultipleObjects(2, ahObjects, TRUE, INFINITE);

#   if (DBG) && (PCB_LIST_DEBUG)

        DbgPrintf(pListLock->dwDebugMask, "Write Acquire: t:%04lX w:%p r:%p\n",
                    GetCurrentThreadId(), ahObjects[0], ahObjects[1]);

#   endif

    //
    // Exit with the mutex, so as to prevent any more readers or writers
    // from coming in.
    //
}

void LockDoneWriting(LIST_LOCK *pListLock) 
{
    //
    // We're done writing, release the mutex so that
    // readers or other writers may come in.
    //

#   if (DBG) && (PCB_LIST_DEBUG)

        DbgPrintf(pListLock->dwDebugMask, "Write Release: t:%04lX w:%p r:%p\n",
                    GetCurrentThreadId(), pListLock->hMutexNoWriter, pListLock->hEventNoReaders);

#   endif

    ReleaseMutex(pListLock->hMutexNoWriter);
}



void LockWaitToRead(LIST_LOCK *pListLock) 
{
    //
    // Acquire the mutex that protects the list data.
    //
    WaitForSingleObject(pListLock->hMutexNoWriter, INFINITE);

#   if (DBG) && (PCB_LIST_DEBUG)

        DbgPrintf(pListLock->dwDebugMask, "Read  Acquire: t:%04lX w:%p r:%p\n",
                    GetCurrentThreadId(), pListLock->hMutexNoWriter, pListLock->hEventNoReaders);

#   endif

    //
    // Now that we have the mutex, we can modify list data without
    // fear of corrupting anyone.
    //

    //
    // Increment the number of reader threads.
    //

    if (++pListLock->NumReaders == 1) 
    {
        //
        // If this is the first reader thread, set our event to   
        // reflect this.  This is so that anyone waiting to write 
        // will block until we're done.                           
        //
        ResetEvent(pListLock->hEventNoReaders);
    }

    //
    // Allow other writer/reader threads to use
    // the lock object.
    //
    ReleaseMutex(pListLock->hMutexNoWriter);
}



void LockDoneReading(LIST_LOCK *pListLock) 
{
    //
    // Acquire the mutex that guards the list data so we can
    // decrement the number of readers safely.
    //

    WaitForSingleObject(pListLock->hMutexNoWriter, INFINITE);

#   if (DBG) && (PCB_LIST_DEBUG)

        DbgPrintf(pListLock->dwDebugMask, "Read  Release: t:%04lX w:%p r:%p\n",
                    GetCurrentThreadId(), pListLock->hMutexNoWriter, pListLock->hEventNoReaders);

#   endif

    if (--pListLock->NumReaders == 0) 
    {
        //
        // We were the last reader.  Wake up any potential
        // writers.
        //
        SetEvent(pListLock->hEventNoReaders);
    }

    //
    // Allow other writer/reader threads to use
    // the lock object.
    //
    ReleaseMutex(pListLock->hMutexNoWriter);
}


