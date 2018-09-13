///+---------------------------------------------------------------------------
//
//  File:       Sem.Hxx
//
//  Contents:   Semaphore classes
//
//  Classes:    CMutexSem - Mutex semaphore class
//              CShareSem - Multiple Reader, Single Writer class
//              CEventSem - Event semaphore
//
//  History:    21-Jun-91   AlexT       Created.
//
//  Notes:      No 32-bit implementation exists yet for these classes, it
//              will be provided when we have a 32-bit development
//              environment.  In the meantime, the 16-bit implementations
//              provided here can be used to ensure your code not blocking
//              while you hold a semaphore.
//
//----------------------------------------------------------------------------

#ifndef __SEM32_HXX__
#define __SEM32_HXX__

extern "C"
{
#include <windows.h>
};
#include <except.hxx>

// This is temporary. To be moved to the appropriate error file
// BUGBUG: use NT error codes. Conversion is expensive! (BartoszM)

enum SEMRESULT
{
    SEMSUCCESS = 0,
    SEMTIMEOUT,
    SEMNOBLOCK,
    SEMERROR
};

enum SEMSTATE
{
    SEMSHARED,
    SEMSHAREDOWNED
};

// BUGBUG: inlcude winbase.h or some such
// infinite timeout when requesting a semaphore

#if !defined INFINITE
#define INFINITE 0xFFFFFFFF
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CMutexSem (mxs)
//
//  Purpose:    Mutex Semaphore services
//
//  Interface:  Init            - initializer (two-step)
//              Request         - acquire semaphore
//              Release         - release semaphore
//
//  History:    14-Jun-91   AlexT       Created.
//              30-oct-91   SethuR      32 bit implementation
//
//  Notes:      This class wraps a mutex semaphore.  Mutex semaphores protect
//              access to resources by only allowing one client through at a
//              time.  The client Requests the semaphore before accessing the
//              resource and Releases the semaphore when it is done.  The
//              same client can Request the semaphore multiple times (a nest
//              count is maintained).
//              The mutex semaphore is a wrapper around a critical section
//              which does not support a timeout mechanism. Therefore the
//              usage of any value other than INFINITE is discouraged. It
//              is provided merely for compatibility.
//
//----------------------------------------------------------------------------

class CMutexSem
{
public:
                CMutexSem();
    inline BOOL Init();
                ~CMutexSem();

    SEMRESULT   Request(DWORD dwMilliseconds = INFINITE);
    void        Release();

private:
    CRITICAL_SECTION _cs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CLock (lck)
//
//  Purpose:    Lock using a Mutex Semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the semaphor, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CLock INHERIT_UNWIND_IF_CAIRO
{
    EXPORTDEF DECLARE_UNWIND

public:
    CLock ( CMutexSem& mxs );
    ~CLock ();
private:
    CMutexSem&  _mxs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CEventSem (evs)
//
//  Purpose:    Event Semaphore services
//
//  Interface:  Wait            - wait for semaphore to be signalled
//              Set             - set signalled state
//              Reset           - clear signalled state
//              Pulse           - set and clear semaphore
//
//  History:    21-Jun-91   AlexT       Created.
//              27-Feb-92   BartoszM    Use exceptions for errors
//
//  Notes:      Used for communication between consumers and producers.
//              Consumer threads block by calling Wait. A producer
//              calls Set waking up all the consumers who go ahead
//              and consume until there's nothing left. They call
//              Reset, release whatever lock protected the resources,
//              and call Wait. There has to be a separate lock
//              to protect the shared resources.
//              Remember: call Reset under lock.
//                        don't call Wait under lock.
//
//----------------------------------------------------------------------------

class CEventSem
{
public:
    inline      CEventSem( BOOL fInitState=FALSE, const LPSECURITY_ATTRIBUTES lpsa=NULL );
    inline      CEventSem( HANDLE hEvent );
    inline     ~CEventSem();

    inline ULONG       Wait(DWORD dwMilliseconds = INFINITE,
                            BOOL fAlertable = FALSE );
    inline void        Set();
    inline void        Reset();
    inline void        Pulse();
    inline const HANDLE GetHandle() const { return _hEvent; }

private:
    HANDLE      _hEvent;
};

#if 0
// BUGBUG: This class is superceded by CResource, in resource.hxx and
//         resource.cxx.  It should be deleted by July 21, 1993
//  WadeR, July 8, 1883
//+---------------------------------------------------------------------------
//
//  Class:      CShareSem (shs)
//
//  Purpose:    Shared Semaphore services
//
//  Interface:  RequestExclusive   - acquire exclusive ownership
//              RequestShared      - acquire shared access
//              RequestSharedOwned - acquire ownership, allowing shared access
//              Release            - release semaphore
//              Upgrade            - upgrade from SharedOwned to Exclusive
//              Downgrade          - downgrade to SharedOwned or Shared
//
//  History:    21-Jun-91   AlexT       Created.
//
//  Notes:      Shared semaphores allow multiple readers/single writers
//              access to resources.  Readers bracket their use of a resource
//              with calls to RequestShared and Release.  Writers bracket
//              their use of a resource with calls to RequestExclusive and
//              Release.
//
//              RequestSharedOwned gives a client ownership of the semaphore
//              but still allows other Readers to share the semaphore.  At
//              some later point, the owning client can call Upgrade to get
//              Exclusive access to the semaphore.  This is useful when a
//              client needs to examine a data structure before modifying it,
//              as it allows other clients to continue viewing the data
//              until the owning client actually needs to modify it.
//
//              Downgrade allows a client to release ownership of a semaphore
//              without releasing access to it.
//
//              For now, this just uses a mutex semaphore.
//
//              BUGBUG -- Ownership related methods needs to be implemented.
//
//----------------------------------------------------------------------------

class CShareSem
{
public:
                CShareSem();
    BOOL        Init();
                ~CShareSem();

    SEMRESULT   RequestExclusive(DWORD dwMilliseconds);
    SEMRESULT   RequestShared(DWORD dwMilliseconds);
    SEMRESULT   RequestSharedOwned(DWORD dwMilliseconds);
    void        Release();
    SEMRESULT   Upgrade(DWORD dwMilliseconds);
    SEMRESULT   Downgrade(SEMSTATE fl);

private:

    // Ownership related methods....
    BOOL        ClaimOwnership();
    BOOL        ReleaseOwnership();

    // Private methods to facilitate code sharing
    void        EnableReaders();

    CMutexSem   _cmtx;
    CEventSem   _evsReaders;
    CEventSem   _evsWriters;
    BOOL        _fWrite;
    ULONG       _cReaders;
    ULONG       _cWaitingReaders;
    ULONG       _cWaitingWriters;
};

//+---------------------------------------------------------------------------
//
//  Class:      CShareSemObject
//
//  Purpose:    The Semaphore Object -- the constructor accquires the
//              semaphore and the destructor releases it.
//
//  Interface:
//
//  History:    05-July-91   SethuR         Created.
//
//
//----------------------------------------------------------------------------

class CShareSemObject INHERIT_UNWIND_IF_CAIRO
{
    DECLARE_UNWIND

public:
    inline CShareSemObject(CShareSem& shs);
    inline ~CShareSemObject();

    inline void      RequestExclusive(DWORD dw);
    inline void      RequestShared(DWORD dw);
    inline void      RequestSharedOwned(DWORD dw);
    inline SEMRESULT Upgrade(DWORD dw);
    inline void      Downgrade(SEMSTATE ss);

private:
    CShareSem* _pshs;
    BOOL       _fAccquired;
};


//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::CShareSemObject, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [shs] -- shared semaphore
//
//  History:    29-Aug-91   SethuR       Created.
//
//----------------------------------------------------------------------------

inline CShareSemObject::CShareSemObject(CShareSem& shs)
{
    _pshs =       &shs;
    _fAccquired = FALSE;
    END_CONSTRUCTION(CShareSemObject)
}

//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::RequestExclusive, public
//
//  Synopsis:   Get Exclusive acess
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    29-Aug-91   SethuR       Created.
//
//----------------------------------------------------------------------------

inline void CShareSemObject::RequestExclusive(DWORD dw)
{
    if (_pshs->RequestExclusive(dw) == SEMSUCCESS)
        _fAccquired = TRUE;
    else
        THROW (CException(Win4ErrSemaphoreInvalid));
}

//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::RequestShared, public
//
//  Synopsis:   Get allow shared access
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    29-Aug-91   SethuR       Created.
//
//----------------------------------------------------------------------------

inline void CShareSemObject::RequestShared(DWORD dw)
{
    if (_pshs->RequestSharedOwned(dw) == SEMSUCCESS)
        _fAccquired = TRUE;
    else
        THROW (CException(Win4ErrSemaphoreInvalid));
}

//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::RequestSharedOwned, public
//
//  Synopsis:   Get ownership but allow shared access
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    29-Aug-91   SethuR       Created.
//
//----------------------------------------------------------------------------

inline void CShareSemObject::RequestSharedOwned(DWORD dw)
{
    if (_pshs->RequestSharedOwned(dw) == SEMSUCCESS)
        _fAccquired = TRUE;
    else
        THROW (CException(Win4ErrSemaphoreInvalid));
}


//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::~CShareSemObject, public
//
//  Synopsis:   Destructor -- Releases the semaphore if accquired
//
//  History:    27-Aug-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CShareSemObject::~CShareSemObject()
{
    if (_fAccquired)
        _pshs->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::Upgrade, public
//
//  Synopsis:   Get exclusive access (must have ownership already)
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    21-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline SEMRESULT CShareSemObject::Upgrade(DWORD dw)
{
    if (_fAccquired)
        return _pshs->Upgrade(dw);
    THROW (CException(Win4ErrSemaphoreInvalid));
    return SEMTIMEOUT; // BUGBUG -- This is to satisfy compiler limitation
}

//+---------------------------------------------------------------------------
//
//  Member:     CShareSemObject::Downgrade, public
//
//  Synopsis:   Release exclusive access (but keep access)
//
//  Arguments:  [fl] -- SEMSHARED or SEMSHAREDOWNED
//
//  History:    21-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline void CShareSemObject::Downgrade(SEMSTATE ss)
{
    if (_fAccquired)
        _pshs->Downgrade(ss);
    else
        THROW (CException(Win4ErrSemaphoreInvalid));
    return;
}
#endif // 0  BUGBUG

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::CMutexSem, public
//
//  Synopsis:   Mutex semaphore constructor
//
//  Effects:    Initializes the semaphores data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CMutexSem::CMutexSem()
{
    Init();
}

inline CMutexSem::Init()
{
    InitializeCriticalSection(&_cs);
    return TRUE;
};

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::~CMutexSem, public
//
//  Synopsis:   Mutex semaphore destructor
//
//  Effects:    Releases semaphore data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CMutexSem::~CMutexSem()
{
    DeleteCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::Request, public
//
//  Synopsis:   Acquire semaphore
//
//  Effects:    Asserts correct owner
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    14-Jun-91   AlexT       Created.
//
//  Notes:      Uses GetCurrentTask to establish the semaphore owner, but
//              written to work even if GetCurrentTask fails.
//
//----------------------------------------------------------------------------

inline SEMRESULT CMutexSem::Request(DWORD dwMilliseconds)
{
    dwMilliseconds;

    EnterCriticalSection(&_cs);
    return(SEMSUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::Release, public
//
//  Synopsis:   Release semaphore
//
//  Effects:    Asserts correct owner
//
//  History:    14-Jun-91   AlexT       Created.
//
//  Notes:      Uses GetCurrentTask to establish the semaphore owner, but
//              written to work even if GetCurrentTask fails.
//
//----------------------------------------------------------------------------

inline void CMutexSem::Release()
{
    LeaveCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLock::CLock
//
//  Synopsis:   Acquire semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CLock::CLock ( CMutexSem& mxs )
: _mxs ( mxs )
{
    _mxs.Request ( INFINITE );
    END_CONSTRUCTION (CLock);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLock::~CLock
//
//  Synopsis:   Release semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CLock::~CLock ()
{
    _mxs.Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::CEventSem
//
//  Synopsis:   Creates an event
//
//  Arguments:  [bInitState] -- TRUE: signaled state, FALSE non-signaled
//              [lpsa]       -- security attributes
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CEventSem::CEventSem ( BOOL bInitState, const LPSECURITY_ATTRIBUTES lpsa )
{
    _hEvent = CreateEvent ( lpsa, TRUE, bInitState, 0 );
    if ( _hEvent == 0 )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::CEventSem
//
//  Synopsis:   Opens an event
//
//  Arguments:  [hEvent]     -- handle of event to open
//              [bInitState] -- TRUE: signaled state, FALSE non-signaled
//
//  History:    02-Jul-94   DwightKr    Created
//
//----------------------------------------------------------------------------

inline CEventSem::CEventSem ( HANDLE hEvent ) : _hEvent( hEvent )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::~CEventSem
//
//  Synopsis:   Releases event
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CEventSem::~CEventSem ()
{
    if ( !CloseHandle ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Set
//
//  Synopsis:   Set the state to signaled. Wake up waiting threads.
//              For manual events the state remains set
//              until Reset is called
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEventSem::Set()
{
    if ( !SetEvent ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Reset
//
//  Synopsis:   Reset the state to non-signaled. Threads will block.
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEventSem::Reset()
{
    if ( !ResetEvent ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Wait
//
//  Synopsis:   Block until event set
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline ULONG CEventSem::Wait( DWORD msec, BOOL fAlertable )
{
    DWORD res = WaitForSingleObjectEx ( _hEvent, msec, fAlertable );

    if ( res < 0 )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
    return(res);
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Pulse
//
//  Synopsis:   Set the state to signaled. Wake up waiting threads.
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEventSem::Pulse()
{
    if ( !PulseEvent ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

#endif /* __SEM32_HXX__ */
