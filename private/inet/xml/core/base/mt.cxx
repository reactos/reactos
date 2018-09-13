/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop 

#undef DEBUGINRETAIL

#ifdef DEBUGINRETAIL
void Print(TCHAR * pch)
{
    TCHAR buf[1024];

    TCHAR * pBuf = buf;
    DWORD dw = ::GetCurrentThreadId();
    *pBuf++ = _T('[');
    _itot(dw, pBuf, 10);
    long l = _tcslen(pBuf);
    pBuf += l;
    *pBuf++ = _T(']');
    l = _tcslen(pch);
    memcpy(pBuf, pch, l * sizeof(TCHAR));
    pBuf[l] = 0;
    OutputDebugString(buf);
}
#endif


DeclareTag(tagShareMutex, "ShareMutex", "ShareMutex::EnterRead");
DeclareTag(tagMutex, "Mutex", "trace mutex activity");

CSMutex * g_pMutex;
ShareMutex * g_pMutexPointer;
ShareMutex * g_pMutexGC;
ShareMutex * g_pMutexFullGC;
ShareMutex * g_pMutexSR;
ApartmentMutex * g_pMutexName;
ApartmentMutex * g_pMutexAtom;
bool g_fMultiProcessor;
HANDLE g_hEventGC;

void MTExit();

BOOL
MTInit()
{
    BOOL fOk = TRUE;
    EnableTag(tagShareMutex, TRUE);

#if MPHEAP
    // MpHeapInit will do this
#else
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_fMultiProcessor = si.dwNumberOfProcessors > 1;
#endif
    g_pMutex = null;
    g_pMutexPointer = null;
    g_pMutexGC = null;
    g_pMutexFullGC = null;
    g_pMutexSR = null;
    g_pMutexName = null;
    g_pMutexAtom = null;
    TRY
    {
        g_pMutex = CSMutex::newCSMutex();
        g_pMutexPointer = ShareMutex::newShareMutex();
        g_pMutexGC = ShareMutex::newShareMutex();
        g_pMutexFullGC = ShareMutex::newShareMutex();
        g_pMutexSR = ShareMutex::newShareMutex();
        g_hEventGC = CreateEvent(NULL, TRUE, TRUE, NULL);
        g_pMutexName = ApartmentMutex::newApartmentMutex();
        g_pMutexAtom = ApartmentMutex::newApartmentMutex();
    }
    CATCH
    {
        fOk = FALSE;
    }
    ENDTRY
    if ( (fOk == FALSE) || (g_hEventGC == NULL) )
    {
        MTExit();
        return FALSE;
    }
    return TRUE;
}


void
MTExit()
{
    ::release(&g_pMutex);
    ::release(&g_pMutexPointer);
    ::release(&g_pMutexGC);
    ::release(&g_pMutexFullGC);
    ::release(&g_pMutexSR);
    ::release(&g_pMutexName);
    ::release(&g_pMutexAtom);
    CloseHandle(g_hEventGC);
}


CSMutex * 
CSMutex::newCSMutex()
{
    return new CSMutex();
}


CSMutex::CSMutex()
{
    InitializeCriticalSection(&_cs);
}


CSMutex::~CSMutex()
{
    DeleteCriticalSection(&_cs);
}


void
CSMutex::Enter()
{
    EnterCriticalSection(&_cs);
}


void
CSMutex::Leave()
{
    LeaveCriticalSection(&_cs);
}


Mutex::Mutex() : SimpleIUnknown(MultiThread)
{
}

MutexLock::MutexLock(Mutex * pMutex)
{
    _pMutex = pMutex;
    if (pMutex)
    {
        _pMutex->AddRef();
        _pMutex->Enter();
    }
}

MutexLock::~MutexLock()
{
    Release();
}

void MutexLock::Release()
{
    if (_pMutex)
    {
        _pMutex->Leave();
        _pMutex->Release();
        _pMutex = null;
    }
}


MutexReadLock::MutexReadLock(Mutex * pMutex)
{
    _pMutex = pMutex;
    if (_pMutex)
    {
        _pMutex->AddRef();
        _pMutex->EnterRead();
    }
}

MutexReadLock::~MutexReadLock()
{
    Release();
}

void MutexReadLock::Release()
{
    if (_pMutex)
    {
        _pMutex->LeaveRead();
        _pMutex->Release();
        _pMutex = null;
    }
}

ShareMutex *
ShareMutex::newShareMutex()
{
    return new ShareMutex();
}

//////////////////////////////////////////////////////////////////////
//
//   Class constructor.
//
//   Create a new lock and initialize it.  This call is not
//   thread safe and should only be made in a single thread
//   environment.
//
//////////////////////////////////////////////////////////////////////

ShareMutex::ShareMutex(
        long lNewMaxSpins,
        long lNewMaxUsers )
{
    //
    //   Set the initial state.
    //
    m_lExclusive = 0;
    m_lTotalUsers = 0;
    m_lWaiting = 0;



    //
    //   Check the configurable values.
    //
    Assert(lNewMaxSpins > 0);
    Assert((lNewMaxUsers > 0) && (lNewMaxUsers <= m_MaxShareLockUsers));

    m_lMaxSpins = lNewMaxSpins; 

    m_lMaxUsers = lNewMaxUsers; 
    
    //
    //   Create a semaphore to sleep on when the spin count exceeds
    //   its maximum.
    //
    if ( (m_hSemaphore = CreateSemaphoreA( NULL, 0, m_MaxShareLockUsers, NULL )) == NULL)
        Exception::throwLastError();

#if DBG == 1

    _ptlsdata = null;

    //
    //   Set the initial state of any debug variables.
    //
    m_lTotalExclusiveLocks = 0;
    m_lTotalShareLocks = 0;
    m_lTotalSleeps = 0;
    m_lTotalSpins = 0;
    m_lTotalTimeouts = 0;
    m_lTotalWaits = 0;
#endif
}

//////////////////////////////////////////////////////////////////////
//
//   Class destructor.
//
//   Destory a lock.  This call is not thread safe and should
//   only be made in a single thread environment.
//
//////////////////////////////////////////////////////////////////////

ShareMutex::~ShareMutex( void )
{
    if ( ! CloseHandle( m_hSemaphore ) )
        Exception::throwLastError();
}

void
ShareMutex::Enter()
{
#if DBG == 1
    Assert((!_ptlsdata || _ptlsdata != GetTlsData()) && "Recursive enter on ShareMutex");
#endif
    ClaimExclusiveLock();
    TraceTag((tagMutex, "%p +Write tid %x  w: %d  r: %d",
                            this, GetTlsData()->_dwTID,
                            m_lExclusive, m_lTotalUsers - m_lExclusive));
}

void 
ShareMutex::Leave()
{
    TraceTag((tagMutex, "%p -Write tid %x  w: %d  r: %d",
                            this, GetTlsData()->_dwTID,
                            m_lExclusive - 1, m_lTotalUsers - m_lExclusive));
    ReleaseExclusiveLock();
}

void 
ShareMutex::EnterRead()
{
#if DBG == 1
    if (_ptlsdata == GetTlsData())
    {
        TraceTag((tagShareMutex, "Perf WARNING - Recursive EnterRead on ShareMutex _ptlsdata = %p", _ptlsdata)); 
    }
#endif
    ClaimShareLock();
#if DBG == 1
    _ptlsdata = GetTlsData();
#endif
    TraceTag((tagMutex, "%p +Read  tid %x  w: %d  r: %d",
                            this, GetTlsData()->_dwTID,
                            m_lExclusive, m_lTotalUsers - m_lExclusive));
}

void
ShareMutex::LeaveRead()
{
    TraceTag((tagMutex, "%p -Read  tid %x  w: %d  r: %d",
                            this, GetTlsData()->_dwTID,
                            m_lExclusive, m_lTotalUsers - 1 - m_lExclusive));
#if DBG == 1
    _ptlsdata = null;
#endif
    ReleaseShareLock();
}

BOOLEAN
ShareMutex::TryEnter()
{
    (void) InterlockedIncrement( (LPLONG) & m_lExclusive );
    (void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );

    if ( m_lTotalUsers == 1 )
    {
#if DBG == 1

        InterlockedIncrement( (LPLONG) & m_lTotalExclusiveLocks );
#endif
        return true; 
    }
    else
    {
        (void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );
        (void) InterlockedDecrement( (LPLONG) & m_lExclusive );

        if ( m_lWaiting > 0 )
        { 
            WakeAllSleepers(); 
        }
        return false;
    }
}


//////////////////////////////////////////////////////////////////////
//
//   Sleep waiting for the lock.
//
//   We have decided it is time to sleep waiting for the lock
//   to become free.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN
ShareMutex::SleepWaitingForLock(
        long lSleep )
{
    //
    //   We have been spinning waiting for the lock but it
    //   has not become free.  Hence, it is now time to 
    //   give up and sleep for a while.
    //
    (void) InterlockedIncrement( (LPLONG) & m_lWaiting );

    //
    //   Just before we go to sleep we do one final check
    //   to make sure that the lock is still busy and that
    //   there is someone to wake us up when it becomes free.
    //
    if ( m_lTotalUsers > 0 )
    {
#if DBG == 1
        //
        //   Count the number of times we have slept on this lock.
        //
        (void) InterlockedIncrement( (LPLONG) & m_lTotalSleeps );

#endif
        //
        //   When we sleep we awoken when the lock becomes free
        //   or when we timeout.  If we timeout we simply exit
        //   after decrementing various counters.
        if (WaitForSingleObject( m_hSemaphore, lSleep ) != WAIT_OBJECT_0 )
        { 
#if DBG == 1
            //
            //   Count the number of times we have timed out 
            //   on this lock.
            //
            (void) InterlockedIncrement( (LPLONG) & m_lTotalTimeouts );

#endif
            return FALSE; 
        }
    }
    else
    {
        //
        //   Lucky - the lock was just freed so lets
        //   decrement the sleep count and exit without
        //   sleeping.
        // 
        (void) InterlockedDecrement( (LPLONG) & m_lWaiting );
    }
    
    return TRUE;
}

void
ShareMutex::UpgradeToExclusiveLock()
{
    Assert(m_lTotalUsers > 0);

    (void) InterlockedIncrement( (LPLONG) & m_lExclusive );

    if ( m_lTotalUsers != 1 )
    {
        WaitForExclusiveLock( INFINITE );
    }
#if DBG == 1

    InterlockedIncrement( (LPLONG) & m_lTotalExclusiveLocks );
#endif

}


void
ShareMutex::DowngradeToShareLock()
{
    Assert(m_lTotalUsers > 0);
    Assert(m_lExclusive > 0);

    (void) InterlockedDecrement( (LPLONG) & m_lExclusive );

    if ( m_lWaiting > 0 )
    { 
        WakeAllSleepers(); 
    }
}


//////////////////////////////////////////////////////////////////////
//
//   Update the spin limit.
//
//   Update the maximum number of spins while waiting for the lock.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN
ShareMutex::UpdateMaxSpins(
        long lNewMaxSpins )
{
    Assert(lNewMaxSpins > 0);
    
    if ( lNewMaxSpins > 0 )
    { 
        m_lMaxSpins = lNewMaxSpins; 

        return TRUE;
    }
    else
    { 
        return FALSE; 
    }
}

//////////////////////////////////////////////////////////////////////
//
//   Update the sharing limit.
//
//   Update the maximum number of users that can share the lock.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN
ShareMutex::UpdateMaxUsers(
        long lNewMaxUsers )
{
    Assert((lNewMaxUsers > 0) && (lNewMaxUsers <= m_MaxShareLockUsers));
    
    if ( (lNewMaxUsers > 0) && (lNewMaxUsers <= m_MaxShareLockUsers) )
    {
        ClaimExclusiveLock();

        m_lMaxUsers = lNewMaxUsers;
        
        ReleaseExclusiveLock();

        return TRUE;
    }
    else
    { 
        return FALSE; 
    }
}

//////////////////////////////////////////////////////////////////////
//
//   Wait for an exclusive lock.
//
//   Wait for the spinlock to become free and then claim it.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN
ShareMutex::WaitForExclusiveLock(
        long lSleep )
{
#if DBG == 1
    register long lSpins = 0;
    register long lWaits = 0;

#endif
    while ( m_lTotalUsers != 1 )
    {
        //
        //   The lock is busy so release it and spin waiting
        //   for it to become free.
        //
        (void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );
    
        //
        //  Find out if we are allowed to spin and sleep if
        //  necessary.
        //
        if ( (lSleep > 0) || (lSleep == INFINITE) )
        {
            register long lCount;

            //
            //   Wait by spinning and repeatedly testing the lock.
            //   We exit when the lock becomes free or the spin limit
            //   is exceeded.
            //
            for (lCount = m_lMaxSpins; (lCount > 0) && (m_lTotalUsers > 0);    lCount -- )
                ;
#if DBG == 1

            lSpins += (m_lMaxSpins - lCount);
            lWaits ++;
#endif

            //
            //   We have exhusted our spin count so it is time to
            //   sleep waiting for the lock to clear.
            //
            if ( lCount == 0 )
            {
                //
                //   We have decide that we need to sleep but are
                //   still holding an exclusive lock so lets drop it
                //   before sleeping.
                //
                (void) InterlockedDecrement( (LPLONG) & m_lExclusive );

                //
                //   We have decide to go to sleep.  If the sleep time
                //   is not 'INFINITE' then we must subtract the time
                //   we sleep from our maximum sleep time.  If the
                //   sleep time is 'INFINITE' then we can just skip
                //   this step.
                //
                if ( lSleep != INFINITE )
                {
                    register DWORD dwStartTime = GetTickCount();

                    if ( ! SleepWaitingForLock( lSleep ) )
                    { 
                        return FALSE; 
                    }

                    lSleep -= ((GetTickCount() - dwStartTime) + 1);
                    lSleep = (lSleep > 0) ? lSleep : 0;
                }
                else
                {
                    if ( ! SleepWaitingForLock( lSleep ) )
                    {
                        return FALSE; 
                    }
                }

                //
                //   We have woken up again so lets reclaim the
                //   exclusive lock we had earlier.
                //
                (void) InterlockedIncrement( (LPLONG) & m_lExclusive );
            }
        }
        else
        { 
            //
            //   We have decide that we need to exit but are still
            //   holding an exclusive lock.  so lets drop it and leave.
            //
            (void) InterlockedDecrement( (LPLONG) & m_lExclusive );

            return FALSE; 
        } 

        //
        //   Lets test the lock again.
        //
        InterlockedIncrement( (LPLONG) & m_lTotalUsers );
    }
#if DBG == 1

//    (void) InterlockedExchangeAdd( (LPLONG) & m_lTotalSpins, (LONG) lSpins );
//    (void) InterlockedExchangeAdd( (LPLONG) & m_lTotalWaits, (LONG) lWaits );
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Wait for a shared lock.
//
//   Wait for the lock to become free and then claim it.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN
ShareMutex::WaitForShareLock(
        long lSleep )
{
#if DBG == 1
    register long lSpins = 0;
    register long lWaits = 0;

#endif
    while ( (m_lExclusive > 0) || (m_lTotalUsers > m_lMaxUsers) )
    {
        //
        //   The lock is busy so release it and spin waiting
        //   for it to become free.
        //
        (void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );
        Assert(m_lTotalUsers >= 0);

        if ( (lSleep > 0) || (lSleep == INFINITE) )
        {
            register long lCount;

            //
            //   Wait by spinning and repeatedly testing the lock.
            //   We exit when the lock becomes free or the spin limit
            //   is exceeded.
            //
            for (lCount = m_lMaxSpins; (lCount > 0) && 
                     ((m_lExclusive > 0) || (m_lTotalUsers >= m_lMaxUsers)); lCount -- )
                ;
#if DBG == 1

            lSpins += (m_lMaxSpins - lCount);
            lWaits ++;
#endif

            //
            //   We have exhusted our spin count so it is time to
            //   sleep waiting for the lock to clear.
            //
            if ( lCount == 0 )
            { 
                //
                //   We have decide to go to sleep.  If the sleep time
                //   is not 'INFINITE' then we must subtract the time
                //   we sleep from our maximum sleep time.  If the
                //   sleep time is 'INFINITE' then we can just skip
                //   this step.
                //
                if ( lSleep != INFINITE )
                {
                    register DWORD dwStartTime = GetTickCount();

                    if ( ! SleepWaitingForLock( lSleep ) )
                    { 
                        return FALSE; 
                    }

                    lSleep -= ((GetTickCount() - dwStartTime) + 1);
                    lSleep = (lSleep > 0) ? lSleep : 0;
                }
                else
                {
                    if ( ! SleepWaitingForLock( lSleep ) )
                    {
                        return FALSE; 
                    }
                }
            }
        }
        else
        { 
            return FALSE; 
        }

        //
        //   Lets test the lock again.
        //
        (void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );
    }
#if DBG == 1


//    (void) InterlockedExchangeAdd( (LPLONG) & m_lTotalSpins, (LONG) lSpins );
//    (void) InterlockedExchangeAdd( (LPLONG) & m_lTotalWaits, (LONG) lWaits );
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Wake all sleepers.
//
//   Wake all the sleepers who are waiting for the spinlock.
//   All sleepers are woken because this is much more efficent
//   and it is known that the lock latency is short.
//
//////////////////////////////////////////////////////////////////////

void
ShareMutex::WakeAllSleepers( void )
{
    register ULONG_PTR  lWakeup = INTERLOCKEDEXCHANGE_PTR(&m_lWaiting, 0 );

    if ( lWakeup > 0 )
    {
        //
        //   Wake up all sleepers as the lock has just been freed.
        //   It is a straight race to decide who gets the lock next.
        //
        if ( ! ReleaseSemaphore( m_hSemaphore, PtrToLong((const void*)(lWakeup)), (LPLONG)NULL ) )
            Exception::throwLastError();
    }
    else
    {
        //
        //   When multiple threads pass through the critical section rapidly
        //   it is possible for the 'Waiting' count to become negative.
        //   This should be very rare but such a negative value needs to be
        //   preserved. 
        //
        for ( /* void */;lWakeup < 0;lWakeup ++ )
        { 
            InterlockedDecrement( (LPLONG) &m_lWaiting ); 
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//              ApartmentMutex
//
// This class is similar to ShareMutex (from which it derives), except that
// it allows any number of users on the "apartment" thread to enter the
// lock.  The first writer into the lock makes his thread the "apartment"
// thread, and it remains that way until all users from that thread have left.
//
// Apartment users obey the stack disipline, because they're on the same thread;
// i.e. the calls nest nicely.  This simplifies our job somewhat.
//
// This kind of mutex is necessary to avoid deadlock.  For example, consider
// this typical scenario:
//      1. script says:  node.text = 'new value';  inducing a call to put_text.
//      2. put_text changes the text, notifying the XML DSO about the change.
//      3. the DSO converts this to the OSP notification: cellChanged(row, col)
//      4. the OSP listener gets the new value by calling getVariant(row, col)
// Step 1 wants an exclusive lock, but step 4 wants a read lock.  A ShareMutex
// would deadlock, but an ApartmentMutex will allow this because both calls are
// on the same thread.  


ApartmentMutex *
ApartmentMutex::newApartmentMutex()
{
    return new ApartmentMutex();
}

//////////////////////////////////////////////////////////////////////
//
//   Class constructor.
//
//   Create a new lock and initialize it.  This call is not
//   thread safe and should only be made in a single thread
//   environment.
//
//////////////////////////////////////////////////////////////////////

ApartmentMutex::ApartmentMutex(long lNewMaxSpins, long lNewMaxUsers) :
    super(lNewMaxSpins, lNewMaxUsers),
    _ptlsApartment(NULL),
    _cApartmentUsers(0)
{
}


//////////////////////////////////////////////////////////////////////
//
//   Class destructor.
//
//   Destory a lock.  This call is not thread safe and should
//   only be made in a single thread environment.
//
//////////////////////////////////////////////////////////////////////

ApartmentMutex::~ApartmentMutex( void )
{
}


void
ApartmentMutex::Enter(TLSDATA * ptlsdata)
{
    if (_ptlsApartment != ptlsdata)
    {
        // Normal entry. Check, enter, and take ownership of the apartment.
        ClaimExclusiveLock();
        Assert(_ptlsApartment == NULL);
        _ptlsApartment = ptlsdata;
    }
    else
    {
        // Recursive entry on the apartment thread.  Enter without checking.
        (void) InterlockedIncrement( (LPLONG) & m_lExclusive );
        (void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );
    }

    // No matter how I enter, I own the apartment once I'm in.
    Assert(_ptlsApartment == ptlsdata);
    ++ _cApartmentUsers;

    TraceTag((tagMutex, "%p +Write tid %x  w: %d  r: %d  apt: %d",
                            this, GetTlsData()->_dwTID, m_lExclusive,
                            m_lTotalUsers - m_lExclusive, _cApartmentUsers));
}


void
ApartmentMutex::Leave()
{
    Assert(_ptlsApartment == GetTlsData());
    -- _cApartmentUsers;

    TraceTag((tagMutex, "%p -Write tid %x  w: %d  r: %d  apt: %d",
                            this, GetTlsData()->_dwTID, m_lExclusive - 1,
                            m_lTotalUsers - m_lExclusive, _cApartmentUsers));

    // If I'm the last (outermost) apartment user, give up ownership.
    if (_cApartmentUsers == 0)
    {
        _ptlsApartment = NULL;
    }

    // Leave the usual way.
    ReleaseExclusiveLock();
}


void
ApartmentMutex::EnterRead(TLSDATA * ptlsdata)
{
    if (_ptlsApartment != ptlsdata)
    {
        // Normal entry.  Check and enter.
        ClaimShareLock();
    }
    else
    {
        // Recursive entry on the apartment thread.  Enter without checking.
        (void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );
    }

    TraceTag((tagMutex, "%p +Read  tid %x  w: %d  r: %d  apt: %d",
                            this, GetTlsData()->_dwTID, m_lExclusive,
                            m_lTotalUsers - m_lExclusive, _cApartmentUsers));
}


void
ApartmentMutex::Enter()
{
    Enter(GetTlsData());
}


void
ApartmentMutex::EnterRead()
{
    EnterRead(GetTlsData());
}




