///////////////////////////////////////////////////////////////////////////////
/*  File: thdsync.cpp

    Description: Contains classes for managing thread synchronization in 
        Win32 programs.  Most of the work is to provide automatic unlocking
        of synchronization primities on object destruction.  The work on 
        monitors and condition variables is strongly patterned after 
        work in "Multithreaded Programming with Windows NT" by Pham and Garg.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "thdsync.h"


CSemaphore::CSemaphore(
    DWORD dwInitialCount,
	DWORD dwMaxCount
    ) : CWin32SyncObj(CreateSemaphore(NULL, dwInitialCount, dwMaxCount, NULL))
{
    if (NULL == Handle())
        throw CSyncException(CSyncException::semaphore, CSyncException::create);
}



CSemaphoreList::~CSemaphoreList(
    void
    )
{
    for (Item *pItem = m_pFirst; NULL != pItem; pItem = m_pFirst)
    {
        m_pFirst = m_pFirst->m_pNext;
        delete pItem->m_pSem;
        delete pItem;
    }
}


void
CSemaphoreList::Dump(
    void
    )
{
    DBGPRINT((TEXT("List: 0x%08X  ------------------------------"), this));
    DBGPRINT((TEXT("m_pFirst = 0x%08X"), m_pFirst));
    DBGPRINT((TEXT("m_pLast  = 0x%08X"), m_pLast));
    for (Item *pItem = m_pFirst; NULL != pItem; pItem = pItem->m_pNext)
    {
        DBGPRINT((TEXT("\tpItem = 0x%08X"), pItem));
        DBGPRINT((TEXT("\t\tm_pNext = 0x%08X"), pItem->m_pNext));
        DBGPRINT((TEXT("\t\tm_pSem  = 0x%08X"), pItem->m_pSem));
    }
}

//
// Insert a semaphore at the head of the list.
//
void
CSemaphoreList::Prepend(
    CSemaphore *pSem
    )
{
    m_pFirst = new Item(pSem, m_pFirst);
    if (NULL == m_pLast)
    {
        m_pLast = m_pFirst;
    }
}


//
// Append a semaphore to the tail of the list.
//
void 
CSemaphoreList::Append(
    CSemaphore *pSem
    )
{
    if (NULL == m_pFirst)
    {
        //
        // Empty list.  Append is same as prepend.
        //
        Prepend(pSem);
    }
    else
    {
        //
        // Create and append the new item.
        //
        m_pLast->m_pNext = new Item(pSem);
        m_pLast = m_pLast->m_pNext;
    }
}

//
// Remove item from the head of the list and return the
// semaphore pointer held in the item.  Delete the item.
//
CSemaphore *
CSemaphoreList::Head(
    void
    )
{
    CSemaphore *pSem = NULL;
    Item *pHead = m_pFirst;             // Get first item.
    if (NULL != pHead)
    {
        pSem = pHead->m_pSem;           // Get first item's semaphore ptr.
        m_pFirst = m_pFirst->m_pNext;   // Unlink item from list.

        delete pHead;                   // Delete item.

        if (NULL == m_pFirst)
            m_pLast = NULL;             //  Adjust "last" ptr if necessary.
    }
    return pSem;                        // Return semaphore ptr.
}


CMutex::CMutex(
    BOOL bInitialOwner
    ) : CWin32SyncObj(CreateMutex(NULL, bInitialOwner, NULL))
{
    if (NULL == Handle())
        throw CSyncException(CSyncException::mutex, CSyncException::create);
}

CEvent::CEvent(
    BOOL bManualReset,
    BOOL bInitialState
    ) : CWin32SyncObj(CreateEvent(NULL, bManualReset, bInitialState, NULL))
{
    if (NULL == Handle())
        throw CSyncException(CSyncException::event, CSyncException::create);
}


//
// Release a "signal-unrgent" monitor.
//
void 
CMonitorSU::Release(
    void
    )
{
    if (0 != m_cUrgentSemCount)
    {
        //
        // There's more than one thread waiting on the "urgent" list.
        // Wake it up and let it run INSIDE of the monitor.
        //
        CSemaphore *pSem = m_UrgentSemList.Head();
        if (NULL != pSem)
        {
            pSem->Release();
        }
    }
    else
    {
        //
        // Exit the monitor.
        //
        m_Mutex.Release();
    }
}

//
// Wait on a "signal-return" condition variable.
//
void 
CConditionSR::Wait(
    void
    )
{
    m_cSemCount++;       // One more thread waiting.
    m_Monitor.Release(); // Release monitor's lock to prevent deadlock.
    m_Sem.Wait();        // Block until condition is signaled.
    m_cSemCount--;       // Have the lock, no longer waiting.
}

//
// Signal a "signal-return" condition variable.
//
void 
CConditionSR::Signal(
    void
    )
{
    //
    // If any threads blocked on the condition variable, release one 
    // of them.
    //
    if (0 < m_cSemCount)
        m_Sem.Release();        // Release thd's blocked on semaphore.
    else
        m_Monitor.Release();    // Release monitor's lock.
}

//
// Wait on a "signal-urgent" condition variable.
//
void 
CConditionSU::Wait(
    void
    )
{
    CSemaphore *pSem = new CSemaphore;
    if (NULL != pSem)
    {
        //
        // Add the semaphore to the list of waiting threads.
        //
        m_SemList.Append(pSem);
        m_cSemCount++;
        if (0 != m_Monitor.m_cUrgentSemCount)
        {
            //
            // At least 1 thread on the "urgent" list.  Get the one highest
            // priority and let it run while we wait.
            //
            CSemaphore *pUrgentItem = m_Monitor.m_UrgentSemList.Head();
            if (NULL != pUrgentItem)
            {
                pUrgentItem->Release();
            }
        }
        else
        {
            //
            // Exit the monitor.
            //
            m_Monitor.Release();
        }
        //
        // Wait for this condition variable to be signaled.
        //
        pSem->Wait();
        m_cSemCount--;
        delete pSem;
    }
}

//
// Signal a "signal-urgent" condition variable.
//
void 
CConditionSU::Signal(
    void
    )
{
    if (0 < m_cSemCount)
    {
        //
        // At least 1 thread waiting on this condition variable.
        //
        CSemaphore *pSemNew = new CSemaphore;
        if (NULL != pSemNew)
        {
            //
            // Add a new semaphore to the "urgent" list.
            //
            m_Monitor.m_UrgentSemList.Prepend(pSemNew);
            m_Monitor.m_cUrgentSemCount++;
            //
            // Retrieve the next semaphore from the condition list and
            // release it allowing it's thread to run.
            //
            CSemaphore *pSemFromList = m_SemList.Head();
            if (NULL != pSemFromList)
            {
                pSemFromList->Release();
            }
            //
            // Wait for this condition variable to be signaled.
            //
            pSemNew->Wait();
            m_Monitor.m_cUrgentSemCount--;
            delete pSemNew;
        }
    }
}

//
// Wait on a Win32 mutex object.
// Throw an exception if the mutex has been abandoned or the wait has timed out.
//
void
AutoLockMutex::Wait(
    DWORD dwTimeout
    )
{
    DWORD dwStatus = WaitForSingleObject(m_hMutex, dwTimeout);
    switch(dwStatus)
    {
        case WAIT_ABANDONED:
            throw CSyncException(CSyncException::mutex, CSyncException::abandoned);
            break;
        case WAIT_TIMEOUT:
            throw CSyncException(CSyncException::mutex, CSyncException::timeout);
            break;
        default:
            break;
    }
}