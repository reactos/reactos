#include "shellprv.h"
#pragma hdrstop

#include "mrsw.h"


//
// #defines
//
#ifdef DEBUG
#define MRSW_WAIT_TIMEOUT 30*1000 // = 30 seconds in debug
#else
#define MRSW_WAIT_TIMEOUT 10*60*1000 //  = ten minutes
#endif


STDAPI_(PMRSW) MRSW_Create(LPCTSTR pszObjectName)
{
    //
    // explicitly ansi so it runs on win95
    //
    CHAR aszName[MAX_PATH - 5 - 10 - 1]; // we limit it to MAX_PATH - lstrlen("mrsw.") - lstrlen(".semaphore") - 1 for null
    CHAR aszObjectName[MAX_PATH];
    MRSW* pmrsw;
    LPSECURITY_ATTRIBUTES psa;


    SHTCharToAnsi(pszObjectName, aszObjectName, ARRAYSIZE(aszObjectName));
    pmrsw = LocalAlloc(LPTR, sizeof(MRSW));

    if (!pmrsw)
    {
        // EOM, so bail
        return NULL;
    }

    // the name of all the objects will all start with "MRSW.UserObjectName",
    lstrcpyA(aszName, "MRSW.");
    StrCatBuffA(aszName, aszObjectName, ARRAYSIZE(aszName));
    lstrcpyA(pmrsw->aszObjectName, aszName); // object name with enough space for ".semaphore" at the end

    psa = CreateAllAccessSecurityAttributes(NULL, NULL, NULL);

    lstrcpyA(aszObjectName, pmrsw->aszObjectName);
    StrCatBuffA(aszObjectName, ".Semaphore", ARRAYSIZE(aszObjectName));
    pmrsw->hSemaphoreNumReaders = CreateSemaphoreA(psa, 0, MAXLONG, aszObjectName);

    lstrcpyA(aszObjectName, pmrsw->aszObjectName);
    StrCatBuffA(aszObjectName, ".Mutex", ARRAYSIZE(aszObjectName));
    pmrsw->hMutexWrite = CreateMutexA(psa, FALSE, aszObjectName);

    lstrcpyA(aszObjectName, pmrsw->aszObjectName);
    StrCatBuffA(aszObjectName, ".Event", ARRAYSIZE(aszObjectName));
    pmrsw->hEventTryToWrite = CreateEventA(psa, FALSE, TRUE, aszObjectName);

    if (!pmrsw->hSemaphoreNumReaders || !pmrsw->hMutexWrite || !pmrsw->hEventTryToWrite)
    {
        // this shouldnt happen
        ASSERT(FALSE);
        
        // dump any objects that we already created
        if (pmrsw->hSemaphoreNumReaders)
            CloseHandle(pmrsw->hSemaphoreNumReaders);

        if (pmrsw->hMutexWrite)
            CloseHandle(pmrsw->hMutexWrite);

        if (pmrsw->hEventTryToWrite)
            CloseHandle(pmrsw->hEventTryToWrite);

        return NULL;
    }

    return pmrsw;
}


STDAPI_(BOOL) MRSW_EnterRead(PMRSW pmrsw)
{
    BOOL bRet;
    DWORD dwRet;

    ASSERT(pmrsw);
    ASSERT(pmrsw->hMutexWrite);
    ASSERT(pmrsw->hSemaphoreNumReaders);

    // we grab this because we cant read at the same time someone is writing, and
    // the mutex also synchronizes access to the semaphore.
    dwRet = WaitForSingleObject(pmrsw->hMutexWrite, INFINITE);
    ASSERT(dwRet != WAIT_ABANDONED);

    // increment the semaphore that represents the total # of readers since we are
    // a new reader
    bRet = ReleaseSemaphore(pmrsw->hSemaphoreNumReaders, 1, NULL);

    // we are done incrementing the # of readers, so release the mutex that protects it
    EVAL(ReleaseMutex(pmrsw->hMutexWrite));
    
    return bRet;
}


STDAPI_(BOOL) MRSW_LeaveRead(PMRSW pmrsw)
{
    ASSERT(pmrsw);
    ASSERT(pmrsw->hSemaphoreNumReaders);
    ASSERT(pmrsw->hEventTryToWrite);

    // we decrement the count of the # of readers semaphore by waiting on it since we are
    // done reading. This call should return immeadeately since we incremented it when we started.
    WaitForSingleObject(pmrsw->hSemaphoreNumReaders, INFINITE);

    // now that we finsihed reading, we will wake up any waiting writers and let them check
    // to see if we were the last reader and therefore they can start.
    EVAL(SetEvent(pmrsw->hEventTryToWrite));

    return TRUE;
}


STDAPI_(LONG) QuerySemaphoreCount(HANDLE hSemaphore)
{
    LONG lOldCount;
    DWORD dwRet;

    // increment the count by one to get the current count
    if (!ReleaseSemaphore(hSemaphore, 1, &lOldCount))
    {
        ASSERT(FALSE);
        return 1; // we return 1 to prevent any writing threads from starting
    }

    // this should return immeadeately since we just increased the count
    dwRet = WaitForSingleObject(hSemaphore, MRSW_WAIT_TIMEOUT);

    ASSERT((dwRet != WAIT_ABANDONED) || (dwRet != WAIT_TIMEOUT));

    return lOldCount;
}


STDAPI_(BOOL) MRSW_EnterWrite(PMRSW pmrsw)
{
    DWORD dwRet;

    ASSERT(pmrsw);
    ASSERT(pmrsw->hMutexWrite);
    ASSERT(pmrsw->hSemaphoreNumReaders);
    ASSERT(pmrsw->hEventTryToWrite);

    // we grab the write mutex since we want to write. This will block any new readers as well
    // as letting have exclusive access to the # of readers semaphore.
    dwRet = WaitForSingleObject(pmrsw->hMutexWrite, INFINITE);
    ASSERT(dwRet != WAIT_ABANDONED);

    while (QuerySemaphoreCount(pmrsw->hSemaphoreNumReaders) > 0)
    {
        // there are still readers active, we will wait on the event that is set every
        // time a reader finishes. After the readers are all done, we can start to write.
        WaitForSingleObject(pmrsw->hEventTryToWrite, INFINITE);
    }

    // if we got here, all the readers are done, so its ok to start writing. We hold the write
    // mutex until the user calls PMRSW_LeaveWrite(), to block everyone else.
    return TRUE;
}


STDAPI_(BOOL) MRSW_LeaveWrite(PMRSW pmrsw)
{
    ASSERT(pmrsw);
    ASSERT(pmrsw->hMutexWrite);

    // we are done writing, so release the write mutex
    EVAL(ReleaseMutex(pmrsw->hMutexWrite));

    return TRUE;
}


STDAPI_(BOOL) MRSW_Destroy(PMRSW pmrsw)
{
    if (!CloseHandle(pmrsw->hSemaphoreNumReaders)   ||
        !CloseHandle(pmrsw->hMutexWrite)            ||
        !CloseHandle(pmrsw->hEventTryToWrite))
    {
        ASSERTMSG(FALSE, "Failed to destroy the MSRW %s !", pmrsw->aszObjectName); 
    }

    LocalFree(pmrsw);

    pmrsw = NULL;

    return TRUE;
}
