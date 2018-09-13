/*
 * @(#)MT.hxx 1.0 2/26/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 * Multi-threading utilites
 */

#ifndef _MT_HXX
#define _MT_HXX

typedef PVOID (WINAPI * PFN_INTERLOCKEDCOMPAREEXCHANGE)(PVOID *Destination, PVOID Exchange, PVOID Comperand);
extern PFN_INTERLOCKEDCOMPAREEXCHANGE g_pfnInterlockedCompareExchange;
#ifdef InterlockedCompareExchange
#undef InterlockedCompareExchange
#endif

#ifdef _WIN64
#define InterlockedCompareExchange(D, E, C) InterlockedCompareExchangePointer(D, E, C)
#else
#define InterlockedCompareExchange(D, E, C) (*g_pfnInterlockedCompareExchange)(D, E, C)
#endif

typedef LONG (WINAPI * PFN_INTERLOCKEDEXCHANGEADD)(LPLONG Addend, LONG Value);
extern PFN_INTERLOCKEDEXCHANGEADD g_pfnInterlockedExchangeAdd;
#ifdef InterlockedExchangeAdd
#undef InterlockedExchangeAdd
#endif

#define InterlockedExchangeAdd(A, V) (*g_pfnInterlockedExchangeAdd)(A, V)

#ifndef _WIN64
#define INTERLOCKEDEXCHANGE_PTR(p, x) (ULONG_PTR)InterlockedExchange((LPLONG)(p), (LONG)(x))
#else // _WIN64
#define INTERLOCKEDEXCHANGE_PTR(p, x) (ULONG_PTR)InterlockedExchangePointer((PVOID *)(p), (PVOID)(ULONG_PTR)(x))
#endif


class CSLock
{
public:
    CSLock(CRITICAL_SECTION * pcs) 
    { 
        _pcs = pcs; 
        EnterCriticalSection(pcs);
    }

    ~CSLock()
    {
        LeaveCriticalSection(_pcs);
    }

private:
    CRITICAL_SECTION * _pcs;
};


DEFINE_CLASS(Mutex);

class NOVTABLE Mutex : public SimpleIUnknown
{
public:

    Mutex();

    virtual void Enter() = 0;
    virtual void Leave() = 0;
    virtual void EnterRead() { Enter(); }
    virtual void LeaveRead() { Leave(); }
};


DEFINE_CLASS(CSMutex);

class CSMutex : public Mutex
{
public:
    static CSMutex * newCSMutex();

    virtual void Enter();

    virtual void Leave();

protected:
    CSMutex();
    ~CSMutex();

private:
    CRITICAL_SECTION    _cs;
};


class MutexLock
{
public:
    MutexLock(Mutex * pMutex);

    ~MutexLock();

    void Release();

private:
    Mutex * _pMutex;
};


class MutexReadLock
{
public:
    MutexReadLock(Mutex * pMutex);

    ~MutexReadLock();

    void Release();

private:
    Mutex * _pMutex;
};


DEFINE_CLASS(ShareMutex);

class ShareMutex : public Mutex
{ 
public:

    static ShareMutex * newShareMutex();

    protected:

        // internally used constants

        enum Internal
        {
            //   The Windows NT kernel requires a maximum wakeup count when
            //   creating a semaphore.
            m_MaxShareLockUsers      = 256
        };

        //
        //   Private data.
        //
        volatile LONG                 m_lExclusive;
        volatile LONG                 m_lTotalUsers;

        long                          m_lMaxSpins;
        long                          m_lMaxUsers;
        HANDLE                        m_hSemaphore;
        volatile LONG                 m_lWaiting;

#if DBG == 1

        // check recursive entry on same thread
    public:
        TLSDATA *                     _ptlsdata;

        //
        //   Counters for debugging builds.
        //
        volatile LONG                 m_lTotalExclusiveLocks;
        volatile LONG                 m_lTotalShareLocks;
        volatile LONG                 m_lTotalSleeps;
        volatile LONG                 m_lTotalSpins;
        volatile LONG                 m_lTotalTimeouts;
        volatile LONG                 m_lTotalWaits;
#endif

    public:
        //
        //   Public functions.
        //
        ShareMutex( long lNewMaxSpins = 4096, long lNewMaxUsers = 256 );
        ~ShareMutex( void );

        virtual void Enter();

        virtual void Leave();

        virtual void EnterRead();

        virtual void LeaveRead();

        BOOLEAN TryEnter();

        inline BOOLEAN ClaimExclusiveLock( long lSleep = INFINITE );

        inline BOOLEAN ClaimShareLock( long lSleep = INFINITE );

        inline void ReleaseExclusiveLock( void );

        inline void ReleaseShareLock( void );

        BOOLEAN UpdateMaxSpins( long lNewMaxSpins );

        BOOLEAN UpdateMaxUsers( long lNewMaxUsers );

        void UpgradeToExclusiveLock();

        void DowngradeToShareLock();

    protected:
        //
        //   Private functions.
        //
        BOOLEAN SleepWaitingForLock( long lSleep );

        BOOLEAN WaitForExclusiveLock( long lSleep );

        BOOLEAN WaitForShareLock( long lSleep );

        void WakeAllSleepers( void );      

    private:
        //
        //   Disabled operations.
        //
        ShareMutex( const ShareMutex & Copy );

        void operator=( const ShareMutex & Copy );
};

//////////////////////////////////////////////////////////////////////
//
//   Claim an exclusive lock.
//
//   Claim an exclusive lock if available else wait or exit.
//
//////////////////////////////////////////////////////////////////////

inline BOOLEAN ShareMutex::ClaimExclusiveLock( long lSleep )
{
    (void) InterlockedIncrement( (LPLONG) & m_lExclusive );
    (void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );

    if ( m_lTotalUsers != 1 )
    {
        if ( ! WaitForExclusiveLock( lSleep ) )
        { 
            return FALSE; 
        }
    }
#if DBG == 1
    _ptlsdata = GetTlsData();
    InterlockedIncrement( (LPLONG) & m_lTotalExclusiveLocks );
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Claim a shared lock.
//
//   Claim a shared lock if available else wait or exit.
//
//////////////////////////////////////////////////////////////////////

inline BOOLEAN ShareMutex::ClaimShareLock( long lSleep )
{
    (void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );

    if ( (m_lExclusive > 0) || (m_lTotalUsers > m_lMaxUsers) )
    {
        if ( ! WaitForShareLock( lSleep ) )
        { 
            return FALSE; 
        }
    }
#if DBG == 1

    InterlockedIncrement( (LPLONG) & m_lTotalShareLocks );
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Release an exclusive lock.
//
//   Release an exclusive lock and if needed wakeup any sleepers.
//
//////////////////////////////////////////////////////////////////////

inline void ShareMutex::ReleaseExclusiveLock( void )
{
#if DBG == 1
    _ptlsdata = null;
#endif
    (void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );
    Assert(m_lTotalUsers >= 0);
    (void) InterlockedDecrement( (LPLONG) & m_lExclusive );

    if ( m_lWaiting > 0 )
    { 
        WakeAllSleepers(); 
    }
}

//////////////////////////////////////////////////////////////////////
//
//   Release a shared lock.
//
//   Release a shared lock and if needed wakeup any sleepers.
//
//////////////////////////////////////////////////////////////////////

inline void ShareMutex::ReleaseShareLock( void )
{
    (void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );

    if ( m_lWaiting > 0 )
    { 
        WakeAllSleepers(); 
    }
}



DEFINE_CLASS(ApartmentMutex);

class ApartmentMutex : public ShareMutex
{
    typedef ShareMutex super;
    
public:
    //
    //   Public functions.
    //

    static ApartmentMutex * newApartmentMutex();
    
    ApartmentMutex( long lNewMaxSpins = 4096, long lNewMaxUsers = 256 );
    ~ApartmentMutex( void );

    // Mutex interface
    virtual void Enter();
    virtual void Leave();
    virtual void EnterRead();

    void Enter(TLSDATA * ptlsdata);
    void EnterRead(TLSDATA * ptlsdata);

    LONG users() { return _cApartmentUsers; }

    TLSDATA*    getOwner() { return _ptlsApartment; }

protected:
    // data
    TLSDATA *_ptlsApartment;    // identity of the apartment thread
    LONG    _cApartmentUsers;   // number of users inside the lock on the apartment thread
};

#endif _MT_HXX
