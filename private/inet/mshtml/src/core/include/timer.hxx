//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       Timer.hxx
//
//  Contents:   Non-Windows based timer declarations.
//
//              For now, this include file contains information
//              that should be exposed to the outside world
//              and stuff that should be private to Trident.
//              This should be
//              separated out. Notably, interface and certain typedefs,
//              and GUIDs should be made be made publicly available
//
//----------------------------------------------------------------------------

#ifndef I_TIMER_HXX_
#define I_TIMER_HXX_
#pragma INCMSG("--- Beg 'timer.hxx'")

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_DOWNBASE_HXX_
#define X_DOWNBASE_HXX_
#include "downbase.hxx"
#endif

MtExtern(CTimer)
MtExtern(CTimerAdvise)
MtExtern(CTimerCtx)
MtExtern(CTimerMan)
MtExtern(CTimerSink)

class CTimerMan;
class CTimerCtx;
class CTimer;
class CTimerSink;
class CTimerAdvise;
typedef CPtrAry<CTimer *>             CTimerPtrAry;
typedef CPtrAry<CTimerAdvise *>       CTimerAdvisePtrAry;

#define MAX_TIME                        (~0U)

typedef struct TIMERTHREADADVISE
{
    DWORD       timeFire;           // when to fire the advise, matches NextTick
    DWORD       timeExpire;         // time Advise expires, matches timeMax
    CTimerCtx  *pTimerCtx;          // so we know what thread this advise belongs to
    BOOL        fIsFree;            // This cell is free
    int         NextFree;           // Next free cell in array, -1 if none
} TIMERTHREADADVISE, *PTIMERTHREADADVISE;

typedef struct TIMERNAMEDTIMER
{
    GUID        guidName;
    CTimer      *pTimer;
} TIMERNAMEDTIMER, *PTIMERNAMEDTIMER;

typedef CStackDataAry<TIMERTHREADADVISE, 12> TimerThreadAdviseAry;
typedef CStackDataAry<TIMERNAMEDTIMER, 4>    NamedTimerAry;

HRESULT GetTimerManager( CTimerMan **ppTimerMan );

//+------------------------------------------------------------------------
//
//  Class:      CTimerMan
//
//  Synopsis:   Allocation object of timers for external controls. Exposed
//              through IServiceProvider. This object also is the default
//              Reference timer, so it spawns a separate thread for the
//              clock and exposes ITimerService. ITimer is supported, but
//              not exposed through QueryInterface.
//              that this object does not support ITimerSink, since it is
//              a reference timer.
//              This is a per Process object, CTimerCtx is the per Thread
//              companion.
//
//-------------------------------------------------------------------------
class CTimerMan : public CExecFT, public ITimerService
{

    friend class CTimerCtx;
    typedef CExecFT super;

 public :

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTimerMan))

    CTimerMan (void);
    ~CTimerMan (void);

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    STDMETHODIMP_(ULONG) AddRef ( void ) {return(CBaseFT::AddRef());}
    STDMETHODIMP_(ULONG) Release( void ) {return(CBaseFT::Release());}

    // ITimerService methods
    STDMETHODIMP CreateTimer    ( ITimer *pReferenceTimer, ITimer **ppNewTimer );
    STDMETHODIMP GetNamedTimer  ( REFGUID rguidName, ITimer **ppTimer );
    STDMETHODIMP SetNamedTimerReference ( REFGUID rguidName, ITimer *pReferenceTimer );

    // CExecFT overrides
    void                Shutdown();
    void                Passivate();

protected:

    // CExecFT overrides
    virtual HRESULT     ThreadInit();
    virtual void        ThreadExec();
    virtual void        ThreadTerm();

    HRESULT      CreateCTimer   ( ITimer *pReferenceTimer, CTimer **ppNewTimer,
                                  REFGUID rguidName=GUID_NULL );
    HRESULT      EnsureTimerThread();
    HRESULT      AddAdvise      ( CTimerAdvise *pTimerAdvise,
                                  CTimerCtx *pTimerCtx,
                                  BOOL fReschduling=FALSE );
    void         RemoveAdvise   ( int index );
    // Call Signal Changes any time an AddAdvise is done. Not necessary for RemoveAdvise
    void         SignalChanges  (void) {Verify( SetEvent( _hevCheckAdvises ) );}


private:
    TimerThreadAdviseAry    _aryTimerThreadAdvises;
    int                     _iFirstFree;
    BOOL                    _fIsLaunched;
    BOOL                    _fShutdown;
    EVENT_HANDLE            _hevCheckAdvises;
    CRITICAL_SECTION        _cs;

};

//+------------------------------------------------------------------------
//
//  Class:      CTimerCtx
//
//  Synopsis:   A per thread object which stradles the thread boundary
//              created by CTimerMan. Handles the notifications from
//              the separate thread and posts msgs to the UI thread's
//              global window to notify the appropriate ITimerSink object.
//              All per thread state is put here.
//              Created by CTimerMan, but destroyed by ThreadDetach
//
//-------------------------------------------------------------------------
class CTimerCtx : CBaseFT, public ITimer
{
    friend class CTimerMan;
    friend class CTimer;
    typedef CBaseFT super;

public :

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTimerCtx))

    CTimerCtx( CTimerMan *pTimerMan, CRITICAL_SECTION *pcs );
    ~CTimerCtx();

    // IUnknown methods
    STDMETHODIMP QueryInterface ( REFIID riid, LPVOID * ppv );
    STDMETHODIMP_(ULONG) AddRef ( void ) {return(super::AddRef());}
    STDMETHODIMP_(ULONG) Release( void ) {return(super::Release());}

    // ITimer methods
    STDMETHODIMP Advise      ( VARIANT vtimeMin, VARIANT vtimeMax, VARIANT vtimeInterval, 
                                  DWORD dwFlags, ITimerSink *pTimerSink, DWORD *pdwCookie);
    STDMETHODIMP Unadvise       ( DWORD dwCookie );
    STDMETHODIMP Freeze         ( BOOL fFreeze );
    STDMETHODIMP GetTime        ( VARIANT *pvtime );

    void                Signal      ( void );
    NV_DECLARE_ONCALL_METHOD(OnMethodCall, onmethodcall, ( DWORD_PTR dwContext ));
    NV_DECLARE_ONTICK_METHOD(TimerCallback, timercallback, ( UINT uTimerID ));

protected :

    HRESULT      ProcessAdvise  ( void );
    HRESULT      GetNamedCTimer ( REFGUID rguidName, CTimer **ppNewTimer );
    HRESULT      AddNamedCTimer ( REFGUID rguidName, CTimer *pTimer );
    void         RemoveNamedCTimer( REFGUID rguidName ); 

    CTimerMan          *_pTimerMan;
    CTimerAdvisePtrAry  _aryAdvises;
    THREADSTATE        *_pts;
    DWORD               _timeFrozen;
    int                 _cFreezes;
    BOOL                _fProcessingAdvise; // blocks unadvises during OnTimer calls
    BOOL                _fPendingUnadvise;  // an unadvise happened on an OnTimer call
    BOOL                _fPosting;          // blocks repeated signaling
    BOOL                _fSetTimer;         // not yielding to Window's msg queue, set timer
    BOOL                _fSignalManager;    // signal timer manager changes to its array
    UINT                _uTimerID;
    NamedTimerAry       _aryNamedTimers;    // keep track of named timer for this thread
#if DBG==1
    DWORD               _threadID;
#endif
};


//+------------------------------------------------------------------------
//
//  Class:      CTimer
//
//  Synopsis:   Timer object handed to the outside world. Implements ITimer.
//              Essentially a veneer to the real timer code. This object is
//              not Co-Creatable. Only created by ITimerService::CreateTimer.
//
//-------------------------------------------------------------------------
class CTimer : CBaseFT, public ITimer
{
    typedef CBaseFT super;
    friend class CTimerSink;
    friend class CTimerMan;
 
public :

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTimer))

	CTimer( ITimer *pRefTimer, CTimerCtx *pTimerCtx,
            CRITICAL_SECTION *pcs, REFGUID rguidName=GUID_NULL );
	~CTimer(void);

    // IUnknown
    STDMETHODIMP QueryInterface ( REFIID riid, LPVOID * ppv );
    STDMETHODIMP_(ULONG) AddRef ( void ) {return(super::AddRef());}
    STDMETHODIMP_(ULONG) Release( void ) {return(super::Release());}

    // ITimer methods
    STDMETHODIMP Advise      ( VARIANT vtimeMin, VARIANT vtimeMax, VARIANT vtimeInterval, 
                                  DWORD dwFlags, ITimerSink *pTimerSink, DWORD *pdwCookie );
    STDMETHODIMP Unadvise		( DWORD dwCookie );
    STDMETHODIMP Freeze			( BOOL fFreeze );
    STDMETHODIMP GetTime	    ( VARIANT *pTime );

protected :

    void    RemoveAdvise( int index );
    HRESULT ProcessAdvise( void );
    HRESULT SetRefTimer( ITimer *pRefTimer );
    ITimer *GetRefTimer( void ) { return _pRefTimer; }

protected :

    DWORD               _dwCurrentCookie;
    LONG                _cFreezes;
    DWORD               _timeFrozen;
    CTimerAdvisePtrAry  _aryAdvises;
    BOOL                _fProcessingAdvise;
    BOOL                _fPendingUnadvise;
    GUID                _guidName;
    ITimer             *_pRefTimer;
    CTimerCtx          *_pTimerCtx;
    CTimerSink         *_pTimerSink;

#ifdef OBJCNTCHK
    DWORD               _dwObjCnt;          // Cookie for verifying object count
#endif

};

//+------------------------------------------------------------------------
//
//  Class:      CTimerSink
//
//  Synopsis:   Sink object created by CTimer to receive notifications
//              from reference timer. Separate sink object breaks circular
//              link between timer/sink and Reference timer.
//              Do not subclass off this class.
//
//-------------------------------------------------------------------------
class CTimerSink : public ITimerSink
{
    friend class CTimer;

public :

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTimerSink))

    CTimerSink( CTimer *pTimer );
    ~CTimerSink(void);

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface (REFIID iid, void **ppvObj);

    // ITimerSink methods
    STDMETHODIMP OnTimer        ( VARIANT vtimeAdvise );

private :
    ULONG   _ulRefs;
    CTimer *_pTimer;
};

//+------------------------------------------------------------------------
//
//  Class:      CTimerAdvise
//
//  Synopsis:   Timer Advise object which holds information regarding an
//              event set through ITimer. Simple C++ class, no interfaces
//
//-------------------------------------------------------------------------
class CTimerAdvise
{

public :

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTimerAdvise))

    CTimerAdvise( void );
    CTimerAdvise( ITimerSink *pTimerSink, DWORD timeMin, DWORD timeMax, 
                  DWORD timeInterval=0, DWORD dwHintFlags=0 );
    ~CTimerAdvise( void );

    inline void  SetHintFlags( DWORD dwFlags )  {_dwHintFlags = dwFlags;}
    inline DWORD GetHintFlags( void )           {return _dwHintFlags;}
    inline void  SetCookie( DWORD dwCookie )    {_dwCookie = dwCookie;}
    inline DWORD GetCookie( void )              {return _dwCookie;}
    inline void  SetRefCookie( DWORD dwCookie ) {_dwRefCookie = dwCookie;}
    inline DWORD GetRefCookie( void )           {return _dwRefCookie;}
    inline DWORD GetTimeMin( void )             {return _timeMin;}
    inline void  SetTimeMin( DWORD time )       {_timeMin = time;}
    inline DWORD GetTimeMax( void )             {return _timeMax;}
    inline void  SetTimeMax( DWORD time )       {_timeMax = time;}
    inline DWORD GetTimeInterval( void )        {return _timeInterval;}
    inline void  SetTimeInterval( DWORD time )  {_timeInterval = time;}
    inline void  SetNextTick( DWORD tick )      {_NextTick = tick;}
    inline DWORD GetNextTick( void )            {return _NextTick;}
    inline ITimerSink *GetTimerSink( void )     {return _pTimerSink;}
    inline void  SetPendingDelete( BOOL fZap=TRUE )  {_fDeleteMe = fZap;}
    inline BOOL  GetPendingDelete( void )       {return _fDeleteMe;}

protected :

    DWORD       _timeMin, _timeMax;
    DWORD       _timeInterval;      // if not zero, then Advise is periodic
    DWORD       _NextTick;          // Next time we send Advise
    DWORD       _dwHintFlags;
    ITimerSink *_pTimerSink;
    DWORD       _dwCookie;
    DWORD       _dwRefCookie;
    BOOL        _fDeleteMe;

};

#pragma INCMSG("--- End 'timer.hxx'")
#else
#pragma INCMSG("*** Dup 'timer.hxx'")
#endif
