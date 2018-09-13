//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       Timer.cxx
//
//  Contents:   Class implementation for CTimerMan, CTimer,
//              and CTimerSink, which support non-Windows based timers
//              for better service and granularity.
//              Timer runs on separate thread, posting messages to main
//              window proc. Message is not WM_TIMER, however, so that
//              there won't be any delay in posting the message.
//              See Timers and Synchronization spec for details.
//
//  Created:    Dec. 5, 1996
//
//--------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TIMER_HXX_
#define X_TiMER_HXX_
#include "timer.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

DeclareTag( tagExtTimer, "Timer trace", "Trace External Trident Timer" );
DeclareTag( tagExtTimerReentrant, "Timer reentrancy", "Test Timer code reentrancy" );
DeclareTag( tagExtTimerThrottle, "Timer Throttle", "Trace timer throttling" );

MtDefine(Timers, Mem, "Timer Manager")
MtDefine(CTimer, Timers, "CTimer")
MtDefine(CTimer_aryAdvises_pv, CTimer, "CTimer::_aryAdvises::_pv")
MtDefine(CTimerAdvise, Timers, "CTimerAdvise")
MtDefine(CTimerCtx, Timers, "CTimerCtx")
MtDefine(CTimerCtx_aryAdvises_pv, CTimerCtx, "CTimerCtx::_aryAdvises")
MtDefine(CTimerCtx_aryNamedTimers_pv, CTimerCtx, "CTimerCtx::_aryNamedTimers")
MtDefine(CTimerMan, Timers, "CTimerMan")
MtDefine(CTimerMan_aryTimerThreadAdvises_pv, CTimerMan, "CTimerMan::_aryTimerThreadAdvises::_pv")
MtDefine(CTimerSink, Timers, "CTimerSink")
MtDefine(CTimerSetRefTimer_arydw_pv, Locals, "CTimer::SetRefTimer arydw::_pv")

HRESULT VariantToTime(VARIANT *pvtime, DWORD *pdw)
{
    HRESULT hr = S_OK;

    Assert(pvtime);
    Assert(pdw);

    if (V_VT(pvtime) == VT_UI4)
    {
        *pdw = V_UI4(pvtime);
    }
    else
    {
        VARIANT vtimeOut;
        vtimeOut.vt = VT_EMPTY;
        hr = VariantChangeType(&vtimeOut, pvtime, 0, VT_UI4);
        if (SUCCEEDED(hr))
            *pdw = V_UI4(&vtimeOut);
    }
    RRETURN(hr);
}

#define DECLARE_VARIANT_INIT(name, dwInit)  \
    VARIANT name;           \
    VariantInit(&name);     \
    V_VT(&name) = VT_UI4;   \
    V_UI4(&name) = dwInit;

//+------------------------------------------------------------------------
//  Globals
//-------------------------------------------------------------------------

CCriticalSection    g_csTimerMan;
// CONSIDER: w/ g_pTimerMan, why have a _pTimerMan for each CTimerCtx?
#ifndef WIN16
CTimerMan          *g_pTimerMan = NULL;
#endif

HRESULT GetTimerManager( CTimerMan **ppTimerMan )
{
    USE_FAST_TASK_GLOBALS;
    // both writing and reading g_pTimerMan need to be in cs
    LOCK_SECTION(g_csTimerMan);

    if (FAST_TASK_GLOBAL(g_pTimerMan) == NULL)
    {
        FAST_TASK_GLOBAL(g_pTimerMan) = new CTimerMan;   // _ulRefs starts as 1

        if (FAST_TASK_GLOBAL(g_pTimerMan) == NULL)
            RRETURN(E_OUTOFMEMORY);
    }
    else
    {
        FAST_TASK_GLOBAL(g_pTimerMan)->AddRef();
    }

    *ppTimerMan = FAST_TASK_GLOBAL(g_pTimerMan);

    RRETURN(S_OK);
}


//+----------------------------------------------------------------------------
//
//  Function:   DeinitTimerCtx
//
//  Synopsis:   Delete Timer Context
//
//  Arguments:  pts - THREADSTATE for current thread
//
//-----------------------------------------------------------------------------
void
DeinitTimerCtx(THREADSTATE *pts)
{
    TraceTag((tagExtTimer, "DeinitTimerCtx"));
    Assert(pts);
    if ( pts->pTimerCtx )
    {
        pts->pTimerCtx->Release();
        pts->pTimerCtx = NULL;
    }
}

/******************************************************************************
                CTimerMan
******************************************************************************/

CTimerMan::CTimerMan()
    : CExecFT(&_cs), _aryTimerThreadAdvises(Mt(CTimerMan_aryTimerThreadAdvises_pv))
{
    Assert(!TASK_GLOBAL(g_pTimerMan));    // only one per process should exist
    _iFirstFree         = -1;
    _fIsLaunched        = FALSE;
    _fShutdown          = FALSE;
    _hevCheckAdvises    = NULL;
    InitializeCriticalSection(&_cs);
}

CTimerMan::~CTimerMan()
{
    TraceTag((tagExtTimer, "~CTimerMan"));
    DeleteCriticalSection(&_cs);
    TASK_GLOBAL(g_pTimerMan) = NULL;     // only one per process should exist
}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   IUnknown implementation, with a twist. This object should
//              exist during the life time of the thread, but is also doled
//              out as a COM object. Therefore, pointers to the outside
//              world are ref counted, but not the internally used object,
//              which treats this as a regular class object (i.e. uses the
//              TLS macro to obtain object). This allows the secondary object
//              count to be correct during shutdown
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CTimerMan::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimerService *)this, IUnknown)
        QI_INHERITS(this, ITimerService)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CreateTimer         [ITimerService]
//
//  Synopsis:   Creates a CTimer object
//
//  Arguments:  pReferenceTimer - Timer to base new timer off of. This is so
//                                the behavior of the new timer is effected
//                                by the reference timer. E.g., if reference
//                                timer is frozen, so will the new timer.
//                                Can be NULL, to use default timer.
//              ppNewTimer      - Returned timer.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY: either ran out of memory or hit the max
//                             allowed
//              E_POINTER
//              E_INVALIDARG
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::CreateTimer( ITimer *pReferenceTimer, ITimer **ppNewTimer )
{
    CTimer *pTimer;
    HRESULT hr;

    TraceTag((tagExtTimer, "CTimerMan::CreateTimer"));

    if ( !ppNewTimer )
        RRETURN( E_POINTER );

    hr = THR(CreateCTimer( pReferenceTimer, &pTimer ));
    *ppNewTimer = (ITimer *)pTimer;
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     CreateCTimer
//
//  Synopsis:   Workshorse for CreateTimer and GetNamedTimer. 
//
//  Arguments:  pReferenceTimer - Timer to base new timer off of. This is so
//                                the behavior of the new timer is effected
//                                by the reference timer. E.g., if reference
//                                timer is frozen, so will the new timer.
//                                Can be NULL, to use default timer.
//              ppNewTimer      - Returned timer class object.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY: either ran out of memory or hit the max
//                             allowed
//              E_POINTER
//              E_INVALIDARG
//
//  Note: Assumes that GetNamedTimer will call this with a NULL ref timer. 
//        Otherwise, CTimerCtx needs to be ensured regardless of RefTimer.
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::CreateCTimer( ITimer *pReferenceTimer, CTimer **ppNewTimer,
                         REFGUID rguidName )
{
    ITimer *pTmp = NULL;

    TraceTag((tagExtTimer, "CTimerMan::CreateCTimer"));
    Assert( ppNewTimer );

    *ppNewTimer = NULL;

    // make sure Timer is set up properly for this thread 
    // CTimerCtx handles all thread specific portions of the timer mgr
    THREADSTATE *pts = GetThreadState();
    if ( !pts->pTimerCtx )
    {
        pts->pTimerCtx = new CTimerCtx( this, &_cs );   // _ulRefs starts as 1
        if ( !pts->pTimerCtx )
            RRETURN( E_OUTOFMEMORY );
    }
    
    if ( pReferenceTimer )
    {
        pReferenceTimer->QueryInterface( IID_ITimer, (void **)&pTmp );
        if ( !pTmp )
            RRETURN( E_INVALIDARG );
        pTmp->Release();

        *ppNewTimer = new CTimer( pReferenceTimer, pts->pTimerCtx, &_cs, 
                                  rguidName );
    }
    else
    {
        *ppNewTimer = new CTimer( pts->pTimerCtx, pts->pTimerCtx, &_cs, 
                                  rguidName );
    }

	if ( NULL == *ppNewTimer )
        RRETURN( E_OUTOFMEMORY );

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     GetNamedTimer           [ITimerService]
//
//  Synopsis:   Returns a named timer. This enables several controls to obtain
//              the same timer without passing it back and forth among 
//              themselves. This allows easier synchronization among disparate
//              controls. Also, Trident will use a timer called "Draw" for
//              Trident painting, so controls can request that one if they
//              want to be synchronized with Trident's painting.
//
//  Arguments:  pszName         - Name of the timer to return. 
//              ppNewTimer      - Returned timer.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY: either ran out of memory or hit the max
//                             allowed
//              E_POINTER
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::GetNamedTimer( REFGUID rguidName, ITimer **ppNewTimer)
{
    THREADSTATE        *pts = GetThreadState();
    CTimer             *pTimer = NULL;
    HRESULT             hr = S_OK;

    TraceTag((tagExtTimer, "CTimerMan::GetNamedTimer"));
    if ( !ppNewTimer )
        RRETURN( E_POINTER );

    if ( rguidName == GUID_NULL ) 
        RRETURN( E_INVALIDARG );

    EnterCriticalSection();

    *ppNewTimer = NULL;

    // check with current timers to see if we already have this one
    if ( pts->pTimerCtx )
    {
        hr = pts->pTimerCtx->GetNamedCTimer( rguidName, &pTimer );
        if ( SUCCEEDED(hr) )
        {
            *ppNewTimer = (ITimer *)pTimer;
            (*ppNewTimer)->AddRef();
            goto cleanup;
        }
    }

    // didn't find a timer by that name, so let's create a new one
    hr = THR(CreateCTimer( NULL, &pTimer, rguidName ));
    if ( FAILED(hr) )
        goto error;

    hr = THR(pts->pTimerCtx->AddNamedCTimer( rguidName, pTimer ));
    if ( FAILED(hr) )
        goto error;

    *ppNewTimer = (ITimer *)pTimer;

cleanup:
    LeaveCriticalSection();
    RRETURN(hr);
error:
    if ( pTimer )
        delete pTimer;
    goto cleanup;
}

//+----------------------------------------------------------------------------
//
//  Method:     SetNamedTimerReference      [ITimerService]
//
//  Synopsis:   Sets the reference timer for the named timer. All advises that
//              were on the existing reference timer for the named timer are
//              moved to the new reference timer.
//
//  Arguments:  pszName         - Name of the timer whose reference timer will 
//                                change
//              pTimer          - New reference timer
//
//  Returns:    S_OK
//              E_OUTOFMEMORY: either ran out of memory or hit the max
//                             allowed
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::SetNamedTimerReference( REFGUID rguidName, ITimer *pRefTimer )
{
    THREADSTATE        *pts = GetThreadState();
    CTimer             *pTimer = NULL;
    HRESULT             hr = S_OK;

    TraceTag((tagExtTimer, "CTimerMan::SetNamedTimerReference"));

    if ( !pts->pTimerCtx || 
         FAILED(pts->pTimerCtx->GetNamedCTimer( rguidName, &pTimer )) )
        RRETURN( E_INVALIDARG );

    if ( pTimer == pRefTimer )      // single-step circular ref check.
        RRETURN( E_INVALIDARG );    // Warning: does not check complete chain

    // avoid work if new ref timer is the same as existing ref timer.
    if ( pTimer->GetRefTimer() == pRefTimer || 
         (!pRefTimer && 
          pTimer->GetRefTimer() == static_cast<ITimer *>(pts->pTimerCtx)) )
        RRETURN( S_OK );

    if ( !pRefTimer )
        // reset to default ref clock
        pRefTimer = static_cast<ITimer *>(pts->pTimerCtx);

    // Get the timer to move its advises over
    hr = pTimer->SetRefTimer( pRefTimer );

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     AddAdvise
//
//  Synopsis:   Adds an advisement to the timer thread. Also sets cookie
//              to TimerAdvise passed in, using the array index.
//
//  Arguments:  pTimerAdvise - pointer of TimerAdvise to add
//              fRescheduling - TRUE if Adding the advise for next interval
//                              of a periodic advise
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::AddAdvise( CTimerAdvise *pTimerAdvise, CTimerCtx *pTimerCtx,
                      BOOL fRescheduling )
{
    HRESULT             hr = S_OK;
    PTIMERTHREADADVISE  pTTA;
    int                 index;

    Assert( pTimerAdvise && pTimerCtx );

    hr = EnsureTimerThread();
    if ( FAILED(hr) )
        goto error;

    EnterCriticalSection();
    TraceTag((tagExtTimer, "CTimerMan::AddAdvise %s", fRescheduling?"Rescheduling":" "));
    if ( fRescheduling )
    {
        // update advise
        DWORD index = pTimerAdvise->GetCookie()-1;      // convert back to index
        _aryTimerThreadAdvises[index].timeFire = pTimerAdvise->GetNextTick();
    }
    else
    {
        if ( _iFirstFree >= 0 )
        {
            // Free cells in the array point to the next free cell to use with
            // their NextFree element. -1 indicate there are no more free cells
            // within the array. A free cell is one which causes a discontinuity
            // between the first array element and the last one in use, i.e., the
            // tail end of the part of the array being used is not considered a
            // free cell. It is considered virgin array space to be appended onto.
            // _iFirstFree points to the first free cell in the list
            index = _iFirstFree;
            pTTA = &_aryTimerThreadAdvises[index];
            _iFirstFree = pTTA->NextFree;
        }
        else
        {
            index = _aryTimerThreadAdvises.Size();
            hr = THR(_aryTimerThreadAdvises.AppendIndirect(NULL, &pTTA));
        }

        if ( SUCCEEDED(hr) )
        {
            pTTA->timeFire = pTimerAdvise->GetNextTick();
            pTTA->timeExpire = pTimerAdvise->GetTimeMax();
            pTTA->NextFree = -1;
            pTTA->fIsFree = FALSE;
            pTTA->pTimerCtx = pTimerCtx;
            pTimerAdvise->SetCookie( index+1 );     // zero is an invalid cookie value
        }
    }
    LeaveCriticalSection();

    if ( FAILED(hr) )
        goto error;

cleanup:
    RRETURN(hr);
error:
    goto cleanup;

}

//+----------------------------------------------------------------------------
//
//  Method:     RemoveAdvise
//
//  Synopsis:   Removes an advisement from the array of advisement.
//
//  Arguments:  index - which one to remove
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerMan::RemoveAdvise( int index )
{
    EnterCriticalSection();
    Assert( _aryTimerThreadAdvises.Size() > 0 );       // otherwise, we should have shut down
    Assert( index < _aryTimerThreadAdvises.Size() && index >= 0 );

    TraceTag((tagExtTimer, "CTimerMan::RemoveAdvise"));

    if ( !_aryTimerThreadAdvises[index].fIsFree )
    {
        _aryTimerThreadAdvises[index].fIsFree = TRUE;
        _aryTimerThreadAdvises[index].NextFree = _iFirstFree;
        _iFirstFree = index;
    }
    LeaveCriticalSection();
}

//+----------------------------------------------------------------------------
//
//  Method:     EnsureTimerThread
//
//  Synopsis:   Makes sure the Timer thread is up and running
//
//  Arguments:  none
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::EnsureTimerThread()
{
    HRESULT             hr = S_OK;

    // make sure timer thread is active
    if ( !_fIsLaunched )
    {
        TraceTag((tagExtTimer, "CTimerMan::EnsureTimerThread. Launching..."));

        EnterCriticalSection();
        if (!_fIsLaunched)
        {
            _hevCheckAdvises = CreateEventA( NULL, FALSE, FALSE, NULL );
            if ( !_hevCheckAdvises )
            {
                hr = GetLastError();
            }
            if (hr == S_OK)
            {
                hr = THR(Launch(FALSE));
            }
            if (hr == S_OK)
                _fIsLaunched = TRUE;
        }
        LeaveCriticalSection();
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//    Methods which run on the Timer Thread
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:     ThreadInit
//
//  Synopsis:   Initializes Event object used to signal timer thread
//
//  Arguments:  none
//
//  Returns:    result from CreateEvent
//
//-----------------------------------------------------------------------------
HRESULT
CTimerMan::ThreadInit( )
{
    TraceTag((tagExtTimer, "CTimerMan::ThreadInit"));
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     ThreadExec
//
//  Synopsis:   Timer on separate thread, the real workhorse. The thread has
//              an event schedule, which is its own array of single shot advises.
//              Periodic timers should reschedule their advises to this thread
//              as they are processed by the TimerCtx on the UI threads.
//              This thread sleeps until the earliest advise should
//              be fired, or until there is something that changes its
//              scheduling queue. Each advise gets fired off, even if they occur
//              at the same time as other events. The signaling method
//              prevents undue notifications.
//
//  Arguments:  none
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerMan::ThreadExec( )
{
    DWORD              timeWait, timeNow;
    TIMERTHREADADVISE *pTTA;
    int                cTTA;

    TraceTag((tagExtTimer, "CTimerMan::ThreadExec"));
    Assert( _hevCheckAdvises );

    while ( !_fShutdown )
    {
        timeWait = INFINITE;

        // Get the next advise to fire off
        EnterCriticalSection();
        if ( _aryTimerThreadAdvises.Size() > 0 )
        {
            timeNow = GetCurrentTime();

            // loop through to see if we have any that can be fired off now.
            // and in the mean time keep track of the closest one to timeNow
            // that should be scheduled
            for ( cTTA=_aryTimerThreadAdvises.Size(), pTTA=_aryTimerThreadAdvises;
                  cTTA;
                  cTTA--, pTTA++)
            {
                if ( !(pTTA->fIsFree) )
                {
                    if ( pTTA->timeFire <= timeNow )
                    {
                        // fire off advise, even if it has expired to
                        // clear _aryAdvises of dead Advises
                        pTTA->pTimerCtx->Signal();
                    }
                    else
                    {
                        if ( pTTA->timeFire - timeNow < timeWait )
                        {
                            timeWait = pTTA->timeFire - timeNow;
                        }
                    }
                }
            }
        }
        LeaveCriticalSection();

        // wait until its either time to process the advise scheduled or until
        // the event has been set because _aryTimerThreadAdvises has changed
        WaitForSingleObject( _hevCheckAdvises, timeWait );
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     ThreadTerm
//
//  Synopsis:   Closes Event object
//
//  Arguments:  none
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerMan::ThreadTerm( )
{
    TraceTag((tagExtTimer, "CTimerMan::ThreadTerm"));
}

//+----------------------------------------------------------------------------
//
//  Method:     Passivate
//
//  Synopsis:   Frees all references held by our timer
//
//  Arguments:  none
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerMan::Passivate( )
{
    TraceTag((tagExtTimer, "CTimerMan Passivate (Enter)"));

    Shutdown();
    super::Passivate();
    CloseEvent( _hevCheckAdvises );

    TraceTag((tagExtTimer, "CTimerMan Passivate (Leave)"));
}

//+----------------------------------------------------------------------------
//
//  Method:     Shutdown
//
//  Synopsis:   takes down timer thread
//
//  Arguments:  none
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerMan::Shutdown( )
{
    TraceTag((tagExtTimer, "CTimerMan Shutdown (Enter)"));

    _fShutdown = TRUE;
    if ( _hevCheckAdvises )
        SetEvent( _hevCheckAdvises );   // tell timer thread to shutdown
    super::Shutdown();

    TraceTag((tagExtTimer, "CTimerMan Shutdown (Leave)"));
}

/******************************************************************************
                CTimerCtx
******************************************************************************/
CTimerCtx::CTimerCtx( CTimerMan *pTimerMan, CRITICAL_SECTION *pcs )
    : CBaseFT(pcs)
    , _aryAdvises(Mt(CTimerCtx_aryAdvises_pv))
    , _aryNamedTimers(Mt(CTimerCtx_aryNamedTimers_pv))
{
    TraceTag((tagExtTimer, "CTimerCtx::CTimerCtx"));
    _pTimerMan =            pTimerMan;
    _pTimerMan->AddRef();
    _pts =                  GetThreadState();
    _cFreezes =             0;
    _fProcessingAdvise =    FALSE;
    _fPendingUnadvise =     FALSE;
    _fPosting =             FALSE;
    _fSetTimer =            FALSE;
    _fSignalManager =       FALSE;
    _uTimerID =             0;
    _aryAdvises.EnsureSize(3);
#if DBG==1
    _threadID =             GetCurrentThreadId();
#endif
}

CTimerCtx::~CTimerCtx()
{
    TraceTag((tagExtTimer, "CTimerCtx::~CTimerCtx"));
    Assert( _pTimerMan );
    TraceTag(( tagExtTimer, "~CTimerCtx deleting %d advises", _aryAdvises.Size() ));
    for ( int i=0; i<_aryAdvises.Size(); i++ )
    {
        delete _aryAdvises[i];
    }
    GWKillMethodCallEx( _pts, this, NULL, 0 );
    if ( _uTimerID )
        FormsKillTimer( this, _uTimerID );
    _pTimerMan->Release();

}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   The typical IUnknown implementation.
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CTimerCtx::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimer *)this, IUnknown)
        QI_INHERITS(this, ITimer)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     Unadvise         [ITimer]
//
//  Synopsis:   Removes an advisement from the array of advisement by cookie.
//
//  Arguments:  dwCookie - cookie identifying which advise
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::Unadvise( DWORD dwCookie )
{
    HRESULT hr = E_INVALIDARG;
#if DBG==1
    // make sure unadvise is happening on the same thread timectx was created on
    Assert(_threadID == GetCurrentThreadId() );
#endif

    TraceTag((tagExtTimer, "CTimerCtx::Unadvise (Cookie=%d)", dwCookie));
    // loop thru to find cookie match. Cookie comes from CTimerMan, so cookies
    // may not be contiguous.
    int iArySize = _aryAdvises.Size();
    for ( int i=0; i<iArySize; i++ )
    {
        if ( _aryAdvises[i]->GetCookie() == dwCookie )
        {
            if ( _fProcessingAdvise )
            {
                _fPendingUnadvise = TRUE;
                _aryAdvises[i]->SetPendingDelete();
            }
            else
            {
                delete _aryAdvises[i];
                _aryAdvises.Delete(i);
                _pTimerMan->RemoveAdvise( dwCookie-1 );   // CTimerMan passes back index+1 as cookie
            }
            hr = S_OK;
            break;
        }
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     Freeze (and Thaw)         [ITimer]
//
//  Synopsis:   Halts the flow of time for this timer. No events fire until
//              a freze is "thawed". Freezes can be nested. Must be matched
//              by a call that thaws the clock.
//
//  Arguments:  fFreeze - Whether to freeze or thaw the clock.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::Freeze( BOOL fFreeze )
{
    TraceTag((tagExtTimer, "CTimerCtx::Freeze %s", fFreeze?"On":"Off"));

    HRESULT hr = S_OK;

    if ( fFreeze )
    {
        if ( 0 == _cFreezes++ )
        {
            VARIANT v;
            VariantInit(&v);

            hr = GetTime(&v);
            Assert(SUCCEEDED(hr));
            Assert(V_VT(&v) == VT_UI4);
            _timeFrozen = V_UI4(&v);
        }
    }
    else if ( _cFreezes > 0 )
    {
        if ( 0 == --_cFreezes )
        {
            hr = ProcessAdvise();
        }
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     GetTime                 [ITimer]
//
//  Synopsis:   returns the current time in milliseconds.
//
//  Arguments:  pTime - returned time
//
//  Returns:    S_OK
//              E_POINTER
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::GetTime( VARIANT *pvTime )
{
    TraceTag((tagExtTimer, "CTimerCtx::GetTime"));

    if ( !pvTime )
        RRETURN(E_POINTER);

    VariantClear(pvTime);

    V_VT(pvTime) = VT_UI4;
    if ( _cFreezes > 0 || _fProcessingAdvise )
        V_UI4(pvTime) = _timeFrozen;
    else
        V_UI4(pvTime) = ::GetCurrentTime();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     Advise
//
//  Synopsis:   Does the actual setting of the Advise. Parameter checks done
//              by other Advise* calls.
//
//  Arguments:  timeMin - event will not be fired before this time
//              timeMax - event will not be fired after this time (discarded)
//                        if set to zero (translated to TIME_MAX), this is 
//                        never discarded
//              timeInterval - the minimum time before the next event is fired
//              dwFlags - Hint Flags:
//                        TIMER_HINT_KEYFRAME -
//                        TIMER_HINT_INVALIDATE - Sink will cause a paint. Timer
//                                                should be frozen when container
//                                                paints, or at least event shouldn't
//                                                fire during a paint
//              pTimerSink - method to call when event fires
//              pdwCookie - cookie returned for call to Unadvise
//
//  Returns:    S_OK
//              E_POINTER
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              DISP_E_OVERFLOW
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::Advise( VARIANT vtimeMin, VARIANT vtimeMax, VARIANT vtimeInterval,
                      DWORD dwFlags, ITimerSink *pTimerSink, DWORD *pdwCookie )
{
    TraceTag((tagExtTimer, "CTimerCtx::Advise"));

    if ( !pTimerSink || !pdwCookie )
        RRETURN(E_POINTER);

    HRESULT             hr;
    CTimerAdvise       *pTA = NULL;


    DWORD timeMin, timeMax, timeInterval;

    hr = VariantToTime(&vtimeMin, &timeMin);
    if (FAILED(hr))
        RRETURN(hr);

    hr = VariantToTime(&vtimeMax, &timeMax);
    if (FAILED(hr))
        RRETURN(hr);
    //
    // REVIEW - michaelw: if someone uses a different type to pass in 0xffffffff
    //                    we really shouldn't treat it as MAX_TIME.
    if (timeMax == MAX_TIME)
        RRETURN(DISP_E_OVERFLOW);
    else if (timeMax == 0)
        timeMax = MAX_TIME;

    if (timeMax < timeMin)
        RRETURN(E_INVALIDARG);

    hr = VariantToTime(&vtimeInterval, &timeInterval);
    if (FAILED(hr))
        RRETURN(hr);

    // add advise to UI thread, which adds advise Timer-thread's list
    pTA = new CTimerAdvise( pTimerSink, timeMin, timeMax, timeInterval, dwFlags );
    if ( !pTA )
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = _aryAdvises.Append( pTA );
    if ( FAILED(hr) )
        goto error;

    // put this on the timer thread as well
    hr = _pTimerMan->AddAdvise( pTA, this );
    if ( FAILED(hr) )
        goto error;

    if ( _fProcessingAdvise )   
        _fSignalManager = TRUE;         // defer signalling to ProcessAdvise
    else
        _pTimerMan->SignalChanges();    // tell timerman to pick this up now

    *pdwCookie = pTA->GetCookie();
    TraceTag((tagExtTimer, "CTimerCtx::Advise cookie = %d", *pdwCookie));

cleanup:
    RRETURN(hr);

error:
    delete pTA;
    goto cleanup;

}

void
CTimerCtx::Signal()
{
    TraceTag((tagExtTimer, "CTimerCtx::Signal"));
    TraceTag((tagExtTimerThrottle, "CTimerCtx::Signal"));
    if ( _fPosting || _cFreezes > 0 || _fSetTimer )
        return;

    EnterCriticalSection();
    if ( _fProcessingAdvise )
    {
        TraceTag((tagExtTimerThrottle, "setting _fSetTime=TRUE"));
        _fSetTimer = TRUE;
        LeaveCriticalSection();
        return;
    }
    LeaveCriticalSection();

    TraceTag((tagExtTimer, "Signal got through"));
    TraceTag((tagExtTimerThrottle, "Signal got through"));
    _fPosting = TRUE;
    GWPostMethodCallEx( _pts, (void *)this,
                        ONCALL_METHOD(CTimerCtx, OnMethodCall, onmethodcall),
                        0, FALSE, "CTimerCtx::OnMethodCall");

}

//+----------------------------------------------------------------------------
//
//  Method:     OnMethodCall
//
//  Synopsis:   The Callback from the Global Window Proc after posting our
//              request. In turn, fires off OnTimer calls to clients who
//              have posted advises with this reference timer.
//
//  Arguments:  dwContext - not used
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerCtx::OnMethodCall( DWORD_PTR dwContext )
{
    TraceTag((tagExtTimer, "CTimerCtx::OnMethodCall"));
    _fPosting = FALSE;
    if ( _cFreezes > 0 )
        return;

    WHEN_DBG(HRESULT hr =) ProcessAdvise();
    Assert(SUCCEEDED(hr));
}

//+----------------------------------------------------------------------------
//
//  Method:     ProcessAdvise
//
//  Synopsis:   Goes through advises and sees if they should notify clients
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::ProcessAdvise()
{
    TraceTag((tagExtTimer, "CTimerCtx::ProcessAdvise"));

    DWORD timeNow, timeMax, timeSink, timeInt;
    BOOL fSetTimer;
    HRESULT hr;

    if ( _cFreezes > 0 )
        RRETURN(S_OK);

    if ( _fProcessingAdvise ) {
        // oh man, aux msg pump driving timer. Defer until later
        _fSignalManager = TRUE;
        RRETURN(S_OK);
    }

    VARIANT vtimeNow;
    VariantInit(&vtimeNow);
	
	hr = GetTime( &vtimeNow );
	if ( FAILED(hr) )
		RRETURN(hr);
	
    hr = VariantToTime( &vtimeNow, &timeNow );
    if ( FAILED(hr) )
        RRETURN(hr);

    _timeFrozen = timeNow;
    _fProcessingAdvise = TRUE;
    int i = _aryAdvises.Size()-1;
    while ( i >= 0)
    {
        CTimerAdvise *pTA = _aryAdvises[i];
        if ( (timeMax = pTA->GetTimeMax()) < timeNow )
        {
            _pTimerMan->RemoveAdvise( _aryAdvises[i]->GetCookie()-1 );
            delete _aryAdvises[i];
            _aryAdvises.Delete(i);
        }
        else if ( (timeSink = pTA->GetNextTick()) <= timeNow )
        {
            if ( 0 == (timeInt = pTA->GetTimeInterval()) )
            {
                DECLARE_VARIANT_INIT(vtimeSink, timeSink);

                pTA->GetTimerSink()->OnTimer( vtimeSink );
#if DBG==1  
                if ( IsTagEnabled(tagExtTimerReentrant) ) {
                    // make like a modal dialog came up, or something
                    ProcessAdvise();
                }
#endif
                _pTimerMan->RemoveAdvise( _aryAdvises[i]->GetCookie()-1 );
                delete _aryAdvises[i];  // trash advise if this was not periodic
                _aryAdvises.Delete(i);
            }
            else
            {
                // make sure we give the latest interval to the sink call
                timeSink += ((timeNow - timeSink)/timeInt) * timeInt;

                DECLARE_VARIANT_INIT(vtimeSink, timeSink);

                pTA->GetTimerSink()->OnTimer( vtimeSink );

                // schedule next period
                if ( MAX_TIME == timeMax || timeSink + timeInt < timeMax )
                {
                    // reset pTA in case memory moved from growing during OnTimer call 
                    CTimerAdvise *pTA = _aryAdvises[i];
                    pTA->SetNextTick( timeSink + timeInt );
                    _pTimerMan->AddAdvise( pTA, this, TRUE );
                    _fSignalManager = TRUE;
                }
                else
                {
                    _pTimerMan->RemoveAdvise( _aryAdvises[i]->GetCookie()-1 );
                    delete _aryAdvises[i];  // trash it, advise expired
                    _aryAdvises.Delete(i);
                }
            }
        }
        i--;
    }

    // Process any Unadvises that came as a result of calling OnTimer on our sinks.
    if ( _fPendingUnadvise )
    {
        for ( i=_aryAdvises.Size()-1; i>=0; i-- )
        {
            if ( _aryAdvises[i]->GetPendingDelete() )
            {
                _pTimerMan->RemoveAdvise( _aryAdvises[i]->GetCookie()-1 );  // CTimerMan passes back index+1 as cookie
                delete _aryAdvises[i];
                _aryAdvises.Delete(i);
            }
        }
        _fPendingUnadvise = FALSE;
    }

    // Now it's time to tell the Timer Manager to wake up and notices the changes
    if ( _fSignalManager ) {
        _pTimerMan->SignalChanges();
        _fSignalManager = FALSE;
    }

    // If processing advises took longer than the interval in which we are 
    // notified to process the advises, then we need to yield to other msgs
    // in the Windows msg queue, so we'll set up a timer to let things happen
    EnterCriticalSection();
    fSetTimer =  _fSetTimer;
    _fProcessingAdvise = FALSE;
    LeaveCriticalSection();

    if ( fSetTimer )
    {
        hr = THR(FormsSetTimer( this, ONTICK_METHOD(CTimerCtx, TimerCallback, timercallback),
                 ++_uTimerID, 0 ));
        TraceTag((tagExtTimerThrottle, "Setting up Windows Timer, ID=%d", _uTimerID));
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     TimerCallback   
//
//  Synopsis:   Function Windows calls when processing a WM_Timer for CTimerCtx
//
//  Arguments:  uTimerID        - ID of Windows timer
//
//  Returns:    S_OK
//              E_FAIL          - no timer by this name
//
//-----------------------------------------------------------------------------
HRESULT BUGCALL
CTimerCtx::TimerCallback( UINT uTimerID )
{
    TraceTag((tagExtTimerThrottle, "TimerCallback _ID=%d  ID=%d", _uTimerID, uTimerID));
    Assert( _uTimerID == uTimerID );
    Verify(FormsKillTimer( this, uTimerID ) == S_OK);
    _fSetTimer = FALSE;
    _uTimerID = 0;
    return ProcessAdvise();
}

//+----------------------------------------------------------------------------
//
//  Method:     GetNamedCTimer
//
//  Synopsis:   Returns a named timer. 
//
//  Arguments:  rguidName       - GUID name of the timer to return. 
//              ppNewTimer      - Returned CTimer object.
//
//  Returns:    S_OK
//              E_FAIL          - no timer by this name
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::GetNamedCTimer( REFGUID rguidName, CTimer **ppNewTimer)
{
    TraceTag((tagExtTimer, "CTimerCtx::GetNamedCTimer"));
    Assert( ppNewTimer );

    *ppNewTimer = NULL;

    for ( int i=0; i<_aryNamedTimers.Size(); i++ ) 
    { 
        if ( _aryNamedTimers[i].guidName == rguidName )
        {
            *ppNewTimer = _aryNamedTimers[i].pTimer;
            return S_OK;
        }
    }
    return E_FAIL;
}

//+----------------------------------------------------------------------------
//
//  Method:     AddNamedCTimer
//
//  Synopsis:   Adds a timer and Name to the array of named timers
//
//  Arguments:  rguidName       - GUID name of the timer 
//              pTimer          - timer.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
CTimerCtx::AddNamedCTimer( REFGUID rguidName, CTimer *pTimer)
{
    PTIMERNAMEDTIMER pNT;
    HRESULT hr;

    TraceTag((tagExtTimer, "CTimerCtx::AddNamedCTimer"));
    Assert( pTimer );

    hr = THR(_aryNamedTimers.AppendIndirect(NULL, &pNT));
    if ( FAILED(hr) )
        RRETURN(hr);

    pNT->guidName = rguidName;
    pNT->pTimer = pTimer;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     RemoveNamedCTimer
//
//  Synopsis:   Removes the name and corresponding timer from the array of 
//              named timers
//
//  Arguments:  rguidName       - GUID name of the timer 
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimerCtx::RemoveNamedCTimer( REFGUID rguidName )
{
    TraceTag((tagExtTimer, "CTimerCtx::RemoveNamedCTimer"));

    for ( int i=_aryNamedTimers.Size()-1; i >= 0; i-- )
    {
        if ( _aryNamedTimers[i].guidName == rguidName )
        {
            _aryNamedTimers.Delete(i);
            break;
        }
    }
}

/******************************************************************************
                CTimer
******************************************************************************/
CTimer::CTimer( ITimer *pRefTimer, CTimerCtx *pTimerCtx, 
                CRITICAL_SECTION *pcs, REFGUID rguidName )
    : CBaseFT(pcs), _aryAdvises(Mt(CTimer_aryAdvises_pv))
{
    TraceTag((tagExtTimer, "CTimer::CTimer"));
    _dwCurrentCookie    = 1;
    _cFreezes           = 0; 
    _fProcessingAdvise  = FALSE; 
    _fPendingUnadvise   = FALSE;
    _pTimerSink         = NULL;
    _guidName           = rguidName;
    _pTimerCtx          = pTimerCtx;
    _pTimerCtx->AddRef();
    _pRefTimer          = pRefTimer;
    _pRefTimer->AddRef();
    IncrementObjectCount(&_dwObjCnt);
}

CTimer::~CTimer()
{
    TraceTag((tagExtTimer, "CTimer::~CTimer"));
    if ( _pTimerSink )                  // timer sink is on its own
    {
        _pTimerSink->_pTimer = NULL;
    }

    if ( GUID_NULL != _guidName ) 
        _pTimerCtx->RemoveNamedCTimer( _guidName );
    _pTimerCtx->Release();

    for ( int i=0; i<_aryAdvises.Size(); i++ )
    {
        _pRefTimer->Unadvise(_aryAdvises[i]->GetRefCookie());
        delete _aryAdvises[i];
    }
    _pRefTimer->Release();
    DecrementObjectCount(&_dwObjCnt);
}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   class QueryInterface impl
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CTimer::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimer *)this, IUnknown)
        QI_INHERITS(this, ITimer)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     Unadvise         [ITimer]
//
//  Synopsis:   Removes a schedule event
//
//  Arguments:  dwCookie - cookie identifying which advise
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//-----------------------------------------------------------------------------
HRESULT
CTimer::Unadvise( DWORD dwCookie )
{
    TraceTag((tagExtTimer, "CTimer::Unadvise (Cookie=%d)", dwCookie));
    HRESULT hr = E_INVALIDARG;
    for ( int i=0; i<_aryAdvises.Size(); i++ )
    {
        if ( _aryAdvises[i]->GetCookie() == dwCookie )
        {
            if ( _fProcessingAdvise )
            {
                _fPendingUnadvise = TRUE;
                _aryAdvises[i]->SetPendingDelete();
            }
            else
            {
                RemoveAdvise( i );
            }
            hr = S_OK;
            break;
        }
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     Freeze (and Thaw)         [ITimer]
//
//  Synopsis:   Halts the flow of time for this timer. No events fire until
//              a freze is "thawed". Freezes can be nested. Must be matched
//              by a call that thaws the clock.
//
//  Arguments:  fFreeze - Whether to freeze or thaw the clock.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CTimer::Freeze( BOOL fFreeze )
{
    TraceTag((tagExtTimer, "CTimer::Freeze"));

    HRESULT hr = S_OK;

    if ( fFreeze ) {
        if ( 0 == _cFreezes )
        {
            VARIANT vtimeNow;
            VariantInit(&vtimeNow);

            hr = _pRefTimer->GetTime( &vtimeNow );
            if (SUCCEEDED(hr))
            {
                hr = VariantToTime(&vtimeNow, &_timeFrozen);
                if (SUCCEEDED(hr))
                    _cFreezes++;
            }
        }
        else
            _cFreezes++;
    }
    else if ( _cFreezes > 0 )
    {
        if ( 0 == --_cFreezes )
        {
            hr = ProcessAdvise();
        }
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     GetTime             [ITimer]
//
//  Synopsis:   returns the current time in miliseconds.
//
//  Arguments:  pTime - returned time
//
//  Returns:    S_OK
//              E_POINTER
//
//-----------------------------------------------------------------------------
HRESULT
CTimer::GetTime( VARIANT *pvTime )
{
    TraceTag((tagExtTimer, "CTimer::GetTime"));
    if ( !pvTime )
        RRETURN( E_POINTER );

    if ( _fProcessingAdvise || _cFreezes > 0 )
    {
        VariantClear(pvTime);
        V_VT(pvTime) = VT_UI4;
        V_UI4(pvTime) = _timeFrozen;
        return( S_OK );
    }
    RRETURN(_pRefTimer->GetTime( pvTime ));
}

//+----------------------------------------------------------------------------
//
//  Method:     Advise
//
//  Synopsis:   Does the actual setting of the Advise. Parameter checks done
//              by other Advise* calls.
//
//  Arguments:  timeMin - event will not be fired before this time
//              timeMax - event will not be fired after this time (discarded)
//                        if set to zero (translated to MAX_TIME), this is 
//                        never discarded
//              timeInterval - the minimum time before the next event is fired
//              dwFlags - Hint Flags:
//                        TIMER_HINT_KEYFRAME -
//                        TIMER_HINT_INVALIDATE - Sink will cause a paint. Timer
//                                                should be frozen when container
//                                                paints, or at least event shouldn't
//                                                fire during a paint
//              pTimerSink - method to call when event fires
//              pdwCookie - cookie returned for call to Unadvise
//
//  Returns:    S_OK
//              E_POINTER
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              E_FAIL
//
//-----------------------------------------------------------------------------
HRESULT
CTimer::Advise( VARIANT vtimeMin, VARIANT vtimeMax, VARIANT vtimeInterval, 
                   DWORD dwFlags, ITimerSink *pTimerSink, DWORD *pdwCookie )
{
    HRESULT         hr;
    CTimerAdvise   *pTA = NULL;
    DWORD           dwCookie;
    DWORD timeMin, timeMax, timeInterval;

    hr = VariantToTime(&vtimeMin, &timeMin);
    if (FAILED(hr))
        RRETURN(hr);

    hr = VariantToTime(&vtimeMax, &timeMax);
    if (FAILED(hr))
        RRETURN(hr);
    //
    // REVIEW - michaelw: if someone uses a different type to pass in 0xffffffff
    //                    we really shouldn't treat it as MAX_TIME.
    if (timeMax == MAX_TIME)
        RRETURN(DISP_E_OVERFLOW);
    else if (timeMax == 0)
        timeMax = MAX_TIME;

    if (timeMax < timeMin)
        RRETURN(E_INVALIDARG);

    hr = VariantToTime(&vtimeInterval, &timeInterval);
    if (FAILED(hr))
        RRETURN(hr);

    // create our sink object if not already there
    if ( !_pTimerSink )
    {
        _pTimerSink = new CTimerSink( this );
        if ( !_pTimerSink )
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    // create advise and add advise to our list
    pTA = new CTimerAdvise( pTimerSink, timeMin, timeMax, timeInterval, dwFlags );
    if ( !pTA )
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    // ask reference timer to tell us when to process this advise
    hr = _pRefTimer->Advise(vtimeMin, vtimeMax, vtimeInterval, dwFlags, (ITimerSink *)_pTimerSink, &dwCookie);
    if ( FAILED(hr) )
        goto error;

    // add advisement to our own list
    hr = _aryAdvises.Append( pTA );
    if ( FAILED(hr) )
        goto error;

    // set cookies. Need separate cookies because ref timer can be switched midstream.
    pTA->SetRefCookie( dwCookie );
    pTA->SetCookie( _dwCurrentCookie );

    if ( pdwCookie )
        *pdwCookie = _dwCurrentCookie;
    _dwCurrentCookie++;

    TraceTag((tagExtTimer, "CTimer::Advise cookie = %d", *pdwCookie));

cleanup:
    RRETURN(hr);

error:
    TraceTag((tagExtTimer, "CTimer::Advise ERROR"));
    delete pTA;
    goto cleanup;

}

/*-------------------------------Helper Functions----------------------------*/

//+----------------------------------------------------------------------------
//
//  Method:     RemoveAdvise
//
//  Synopsis:   Removes an advisement from the array of advisement.
//
//  Arguments:  index - which one to remove
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
CTimer::RemoveAdvise( int index )
{
    TraceTag((tagExtTimer, "CTimer::RemoveAdvise"));
    Assert( index < _aryAdvises.Size() && index >= 0 );

    _pRefTimer->Unadvise( _aryAdvises[index]->GetRefCookie() );
    delete _aryAdvises[index];
    _aryAdvises.Delete(index);
}

//+----------------------------------------------------------------------------
//
//  Method:     ProcessAdvise
//
//  Synopsis:   Goes through advises and sees if they should notify clients
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
HRESULT
CTimer::ProcessAdvise()
{
    TraceTag((tagExtTimer, "CTimer::ProcessAdvise"));

    DWORD timeNow, timeMax, timeSink, timeInt;
    HRESULT hr;

    if ( _cFreezes > 0 )
        RRETURN(S_OK);

    VARIANT vtimeNow;
    VariantInit(&vtimeNow);
	
	hr = GetTime( &vtimeNow );
	if ( FAILED(hr) )
		RRETURN(hr);
	
    hr = VariantToTime( &vtimeNow, &timeNow );
    if ( FAILED(hr) )
        RRETURN(hr);

    _timeFrozen = timeNow;
    AddRef();                   // protect against releases to zero
    Assert(_fProcessingAdvise == FALSE);
    _fProcessingAdvise = TRUE;  // protect against unadvises

    // Check all advises
    int i = _aryAdvises.Size()-1;
    while ( i >= 0)
    {
        CTimerAdvise *pTA = _aryAdvises[i];
        if ( (timeMax = pTA->GetTimeMax()) < timeNow )
        {
            RemoveAdvise( i );
        }
        else if ( (timeSink = pTA->GetNextTick()) <= timeNow )
        {
            VARIANT vtimeSink;
            VariantInit(&vtimeSink);
            V_VT(&vtimeSink) = VT_UI4;

            if ( 0 == (timeInt = pTA->GetTimeInterval()) )
            {
                V_UI4(&vtimeSink) = timeSink;

                pTA->GetTimerSink()->OnTimer(vtimeSink);
                RemoveAdvise( i );      // trash advise if this was a one off
            }
            else
            {
                // make sure we give the latest interval to the sink call
                timeSink += ((timeNow - timeSink)/timeInt) * timeInt;
                V_UI4(&vtimeSink) = timeSink;

                pTA->GetTimerSink()->OnTimer( vtimeSink );

                // schedule next period
                if ( MAX_TIME == timeMax || timeSink + timeInt < timeMax )
                    (_aryAdvises[i])->SetNextTick( timeSink + timeInt );
                else
                    RemoveAdvise( i );      // trash it, advise expired
            }
        }
        i--;
    }
    _fProcessingAdvise = FALSE;

    // Process any Unadvises that came as a result of calling OnTimer on our sinks.
    if ( _fPendingUnadvise )
    {
        for ( i=_aryAdvises.Size()-1; i>=0; i-- )
        {
            if ( _aryAdvises[i]->GetPendingDelete() )
                RemoveAdvise( i );
        }
        _fPendingUnadvise = FALSE;
    }

    Release();

    RRETURN(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Method:     SetRefTimer
//
//  Synopsis:   Set the reference timer. Advises on the old reference timer
//              are removed and replaced onto the new reference timer.
//
//  Arguments:  pRefTimer - new reference timer
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------

HRESULT
CTimer::SetRefTimer( ITimer *pRefTimer )
{
    CTimerAdvise    *pTA;
    DWORD           dwCookie;
    DWORD           timeOldRef, timeNewRef, timeDiff=0;
    DWORD           timeMin, timeMax;
    int             cAdvises = _aryAdvises.Size();
    CStackPtrAry<DWORD_PTR, 12> arydw(Mt(CTimerSetRefTimer_arydw_pv));
    HRESULT         hr;

    TraceTag((tagExtTimer, "CTimer::SetRefTimer: moving %d advises", cAdvises));

    pRefTimer->AddRef();
    Freeze( TRUE );

    // account for different starting points in reference timers
    VARIANT v;
    VariantInit(&v);
    hr = THR(pRefTimer->GetTime( &v ));
    if (FAILED(hr))
        goto error;

    hr = VariantToTime(&v, &timeNewRef);
    if (FAILED(hr))
        goto error;

    hr = GetTime(&v);
    if (FAILED(hr))
        goto error;

    hr = VariantToTime(&v, &timeOldRef);
    if (FAILED(hr))
        goto error;

    timeDiff = timeNewRef - timeOldRef;

    if ( cAdvises > 0 )
    {
        // first move all advises to new ref timer before trashing old one
        hr = THR(arydw.EnsureSize(cAdvises));
        if ( FAILED(hr) )
            goto error;

        int i;
        for ( i=0; i<cAdvises; i++ )
        {
            pTA = _aryAdvises[i];
            timeMin = pTA->GetTimeMin() + timeDiff;
            if ( pTA->GetTimeMax() != MAX_TIME )
                timeMax = pTA->GetTimeMax() + timeDiff;
            else 
                timeMax = 0;    // this is treated as MAX_TIME from the interface standpoint

            DECLARE_VARIANT_INIT(vtimeMin, timeMin);
            DECLARE_VARIANT_INIT(vtimeMax, timeMax);
            DECLARE_VARIANT_INIT(vtimeInterval, pTA->GetTimeInterval());

            hr = pRefTimer->Advise( vtimeMin, vtimeMax, vtimeInterval,
                                    pTA->GetHintFlags(),
                                    pTA->GetTimerSink(),
                                    &dwCookie);
            if ( FAILED(hr) )
                goto error;

            arydw[i] = dwCookie;
        }

        // everything safely on new ref timer, 
        // - get rid of advises on old timer, 
        // - adjust min and max time accordingly, 
        // - and expunge expired advises
        for ( i=cAdvises-1; i>=0; i-- )
        {
            pTA = _aryAdvises[i];

            _pRefTimer->Unadvise( pTA->GetRefCookie() );
            
            if ( 0 == arydw[i] )
            {
                delete _aryAdvises[i];
                _aryAdvises.Delete(i);
            }
            else
            {
                pTA->SetTimeMin( pTA->GetTimeMin() + timeDiff );
                if ( pTA->GetTimeMax() != MAX_TIME )
                    pTA->SetTimeMax( pTA->GetTimeMax() + timeDiff );
                pTA->SetRefCookie( (DWORD)arydw[i] );
            }
        }
    }
    _pRefTimer->Release();
    _pRefTimer = pRefTimer;     // already AddRef'ed above


cleanup:
    Freeze( FALSE );
    RRETURN(hr);

error:
    Assert( 0 && "Error in SetRefTimer" );
    pRefTimer->Release();
    goto cleanup;
}

/******************************************************************************
                CTimerSink
******************************************************************************/
CTimerSink::CTimerSink( CTimer *pTimer )
{
    TraceTag((tagExtTimer, "CTimerSink::CTimerSink"));
    _pTimer = pTimer;
    _ulRefs = 0;
}

CTimerSink::~CTimerSink( )
{
    TraceTag((tagExtTimer, "CTimerSink on it's way out!"));
    if ( _pTimer )
        _pTimer->_pTimerSink = NULL;
}

ULONG
CTimerSink::AddRef()
{
    return ++_ulRefs;
}

ULONG
CTimerSink::Release()
{
    TraceTag((tagExtTimer, "CTimerSink::Release(); New ref count = %d", _ulRefs-1));
    if ( 0 == --_ulRefs )
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   IUnknown implementation.
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CTimerSink::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimerSink *)this, IUnknown)
        QI_INHERITS(this, ITimerSink)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     OnTimer             [ITimerSink]
//
//  Synopsis:   Get notified of an event which we requested through CTimer's
//              ITimer::Advise*
//
//  Arguments:  timeAdvie - the time that the advise was set.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CTimerSink::OnTimer( VARIANT vtimeAdvise )
{
    TraceTag((tagExtTimer, "CTimerSink::OnTimer"));
    if ( _pTimer )
        _pTimer->ProcessAdvise();
    return S_OK;
}


/******************************************************************************
                CTimerAdvise
******************************************************************************/
CTimerAdvise::CTimerAdvise()
{
    memset( this, 0, sizeof(CTimerAdvise) );
}

CTimerAdvise::CTimerAdvise( ITimerSink *pTimerSink, DWORD timeMin, DWORD timeMax,
                            DWORD timeInterval, DWORD dwHintFlags )
{
    Assert( pTimerSink );
    _timeMin            = timeMin;
    _timeMax            = timeMax;
    _timeInterval       = timeInterval;
    _NextTick           = timeMin;
    _dwHintFlags        = dwHintFlags;
    _pTimerSink         = pTimerSink;
    _pTimerSink->AddRef();
    _dwCookie           = 0;
    _dwRefCookie        = 0;
    _fDeleteMe          = FALSE;

}

CTimerAdvise::~CTimerAdvise()
{
    int  i;
    i = _pTimerSink->Release();
}

