#ifndef _INC_CSCVIEW_THDSYNC_H
#define _INC_CSCVIEW_THDSYNC_H
///////////////////////////////////////////////////////////////////////////////
/*  File: thdsync.h

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
#ifndef _WINDOWS_
#   include <windows.h>
#endif
#ifndef _INC_DSKQUOTA_DEBUG_H
#   include "debug.h"
#endif

class CCriticalSection
{
    public:
        CCriticalSection(void)
            { InitializeCriticalSection(&m_cs); }
        ~CCriticalSection(void)
            { DeleteCriticalSection(&m_cs); }

        void Enter(void)
            { EnterCriticalSection(&m_cs); }

        void Leave(void)
            { LeaveCriticalSection(&m_cs); }

        operator CRITICAL_SECTION& ()
            { return m_cs; }

    private:
        CRITICAL_SECTION m_cs;
        //
        // Prevent copy.
        //
        CCriticalSection(const CCriticalSection& rhs);
        CCriticalSection& operator = (const CCriticalSection& rhs);
};


class CWin32SyncObj
{
    public:
        explicit CWin32SyncObj(HANDLE handle)
            : m_handle(handle) { }
        virtual ~CWin32SyncObj(void)
            { if (NULL != m_handle) CloseHandle(m_handle); }

        HANDLE Handle(void)
            { return m_handle; }

    protected:
        HANDLE m_handle;
};


class CSemaphore : public CWin32SyncObj
{
    public:
        explicit CSemaphore(DWORD dwInitialCount = 0, DWORD dwMaxCount = 1);
        ~CSemaphore(void) { };

        DWORD Wait(DWORD dwTimeout = INFINITE)
            { return WaitForSingleObject(m_handle, dwTimeout); }
        void Release(DWORD dwReleaseCount = 1)
            { ReleaseSemaphore(m_handle, dwReleaseCount, NULL); }

    private:
        //
        // Prevent copy.
        //
        CSemaphore(const CSemaphore& rhs);
        CSemaphore& operator = (const CSemaphore& rhs);
};


class CSemaphoreList
{
    public:
        CSemaphoreList(void)
            : m_pFirst(NULL),
              m_pLast(NULL) { }

        ~CSemaphoreList(void);

        void Append(CSemaphore *pSem);
        void Prepend(CSemaphore *pSem);
        CSemaphore *Head(void);
        bool bEmpty(void)
            { return (NULL == m_pFirst); }
        void Dump(void);

    private:
        class Item
        {
            public:
                Item(CSemaphore *pSem, Item *pNext = NULL)
                    : m_pSem(pSem), m_pNext(pNext) { }

                CSemaphore *m_pSem;
                Item       *m_pNext;
        };

        Item *m_pFirst;
        Item *m_pLast;
};        


class CMutex : public CWin32SyncObj
{
    public:
        explicit CMutex(BOOL InitialOwner = FALSE);
        ~CMutex(void) { };

        DWORD Wait(DWORD dwTimeout = INFINITE)
            { return WaitForSingleObject(m_handle, dwTimeout); }
        void Release(void)
            { ReleaseMutex(m_handle); }

    private:
        //
        // Prevent copy.
        //
        CMutex(const CMutex& rhs);
        CMutex& operator = (const CMutex& rhs);
};


class CEvent : public CWin32SyncObj
{
    public:
        explicit CEvent(BOOL bManualReset, BOOL bInitialState);
        ~CEvent(void) { };

        BOOL Set(void)
            { return SetEvent(m_handle); }
        BOOL Reset(void)
            { return ResetEvent(m_handle); }

    private:
        //
        // Prevent copy.
        //
        CEvent(const CEvent& rhs);
        CEvent& operator = (const CEvent& rhs);
};


class CMonitor
{
    public:
        CMonitor(void) { }
        ~CMonitor(void) { }

        virtual void Lock(void)
            { m_Mutex.Wait(); }
        virtual void Release(void)
            { m_Mutex.Release(); }

    protected:
        CMutex m_Mutex;
};


class CConditionSU;  // fwd decl for use in CMonitorSU.

//
// "Signal-Urgent" monitor.
// Signalling threads are guaranteed to run immediately after the signaled
// thread exits the monitor.
//
class CMonitorSU : public CMonitor
{
    public:
        CMonitorSU(void)
            : m_cUrgentSemCount(0) { }
        ~CMonitorSU(void) { }

        virtual void Release(void);

    protected:
        int            m_cUrgentSemCount;
        CSemaphoreList m_UrgentSemList;

    friend class CConditionSU;
};


//
// "Signal-Return" condition variable.
// Thread that is signaled is guaranteed to own the mutex lock following
// receipt of the signal.
//
class CConditionSR
{
    public:
        explicit CConditionSR(CMonitor& monitor)
            : m_Monitor(monitor),
              m_cSemCount(0) { }

        virtual ~CConditionSR(void) { }

        virtual void Wait(void);
        virtual void Signal(void);

    protected:
        CMonitor&  m_Monitor;
        CSemaphore m_Sem;
        int        m_cSemCount;
};

//
// "Signal-Urgent" condition variable.
// A signaling thread is guaranteed to run first when the
// signaled thread exits the monitor.  
//
class CConditionSU
{
    public:
        explicit CConditionSU(CMonitorSU& monitor)
            : m_Monitor(monitor),
              m_cSemCount(0) { }
        virtual ~CConditionSU(void) { }

        virtual void Wait(void);
        virtual void Signal(void);

    protected:
        CMonitorSU&    m_Monitor;
        CSemaphoreList m_SemList;
        int            m_cSemCount;
};

       

//
// An "auto lock" object based on a Win32 critical section.
// The constructor automatically calls EnterCriticalSection for the 
// specified critical section.  The destructor automatically calls
// LeaveCriticalSection.  Note that the critical section object may
// be specified as a Win32 CRITICAL_SECTION or a CCriticalSection object.
// If using a CRITICAL_SECTION object, initialization and deletion of 
// the CRITICALS_SECTION is the responsibility of the caller.
//
class AutoLockCs
{
    public:
        explicit AutoLockCs(CRITICAL_SECTION& cs)
            : m_cLock(0),
              m_pCS(&cs) { Lock(); }

        void Lock(void)
            { DBGASSERT((0 <= m_cLock)); EnterCriticalSection(m_pCS); m_cLock++; }

        void Release(void)
            { m_cLock--; LeaveCriticalSection(m_pCS); }

        ~AutoLockCs(void) { if (0 < m_cLock) Release(); }

    private:
        CRITICAL_SECTION *m_pCS;
        int               m_cLock;
};


//
// An "auto lock" object based on a Win32 Mutex object.
// The constructor automatically calls WaitForSingleObject for the 
// specified mutex.  The destructor automatically calls
// ReleaseMutex. 
//
class AutoLockMutex
{
    public:
        //
        // Attaches to an already-owned mutex to ensure release.
        //
        explicit AutoLockMutex(HANDLE hMutex)
            : m_hMutex(hMutex) { }

        explicit AutoLockMutex(CMutex& mutex)
            : m_hMutex(mutex.Handle()) { }

        AutoLockMutex(HANDLE hMutex, DWORD dwTimeout)
            : m_hMutex(hMutex) { Wait(dwTimeout); }

        AutoLockMutex(CMutex& mutex, DWORD dwTimeout)
            : m_hMutex(mutex.Handle()) { Wait(dwTimeout); }

        ~AutoLockMutex(void) { ReleaseMutex(m_hMutex); }

    private:
        HANDLE m_hMutex;

        void Wait(DWORD dwTimeout = INFINITE);
};


//
// Automatically locks a monitor on creation and releases the 
// lock on destruction.  Helps exception-safety of monitor functions.
//
class AutoLockMonitor
{
    public:
        explicit AutoLockMonitor(CMonitor& monitor)
            : m_Monitor(monitor)
            { m_Monitor.Lock(); }

        ~AutoLockMonitor(void)
            { m_Monitor.Release(); }

    private:
        CMonitor& m_Monitor;
};


#endif // _INC_CSCVIEW_THDSYNC_H


