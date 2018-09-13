//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       tracker.h
//
//  Contents:   CTracker -- class that implements link tracking for
//                          OLE monikers and shell links
//
//  Functions:  
//
//  History:    07-Aug-95   BillMo      Created.
//              20-Sep-95   MikeHill    Added Set- & Get-CreationFlags()
//              01-Dec-96   MikeHill    Update to NT5 implementation
//
//--------------------------------------------------------------------------

#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "lnktrack.h"
#include "resolve.h"
#include <trkwks.h>
#include <rpcasync.h>

//+-------------------------------------------------------------------------
//
//  Class:      CTracker
//
//  Purpose:    This class implements the object ID-based link
//              tracking (new to NT5).
//
//--------------------------------------------------------------------------

//
// Private interface used for testing purposes only
//

EXTERN_C const IID IID_ISLTracker; // 7c9e512f-41d7-11d1-8e2e-00c04fb9386d


#ifdef __cplusplus

class ISLTracker : public IUnknown
{
public:

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) ()  PURE;
    STDMETHOD_(ULONG,Release) () PURE;

    STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags, DWORD TrackerRestrictions) PURE;
    STDMETHOD(GetIDs)(CDomainRelativeObjId *pdroidBirth, CDomainRelativeObjId *pdroidLast, CMachineId *pmcid) PURE;
    STDMETHOD(CancelSearch)() PURE;

};  // interface ITracker

#endif // #ifdef __cplusplus


//
// CTrack declaration
//

struct CTracker
#ifdef __cplusplus
            : public ISLTracker
#endif
{

#ifdef __cplusplus

        //  ----------------
        //  IUnknown methods
        //  ----------------

        STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
        STDMETHOD_(ULONG,AddRef) ();
        STDMETHOD_(ULONG,Release) ();

        //  ------------------
        //  ISLTracker methods
        //  ------------------

        STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags, DWORD TrackerRestrictions);
        STDMETHOD(GetIDs)(CDomainRelativeObjId *pdroidBirth, CDomainRelativeObjId *pdroidLast, CMachineId *pmcid);

        //  -----------------------
        //  Custom CTracker methods
        //  -----------------------

        // Construction
        CTracker( IShellLink *pshlink ) : _pshlink(pshlink)
        {
            _fLoadedAtLeastOnce = _fLoaded = _fDirty = FALSE;
            _fCritsecInitialized = _fMendInProgress = FALSE;
            _hEvent = NULL;
            _pRpcAsyncState = NULL;
        };

        ~CTracker()
        {
            if( _fCritsecInitialized )
            {
                DeleteCriticalSection( &_cs );
                _fCritsecInitialized = FALSE;
            }

            if( NULL != _pRpcAsyncState )
            {
                delete[] _pRpcAsyncState;
                _pRpcAsyncState = NULL;
            }

            if( NULL != _hEvent )
            {
                CloseHandle( _hEvent );
                _hEvent = NULL;
            }
        }


        // Initialization.

        HRESULT     InitFromHandle( const HANDLE hFile, const TCHAR* ptszFile );
        HRESULT     InitNew( );
        VOID        UnInit();

        // Load and Save

        HRESULT     Load( BYTE *pb, ULONG cb );
        ULONG       GetSize()
        {
            return( sizeof(DWORD)   // To save the length
                    + sizeof(DWORD) // To save flags
                    + sizeof(_mcidLast) + sizeof(_droidLast) + sizeof(_droidBirth) );
        }

        VOID        Save( BYTE *pb, ULONG cb );

        // Search for a file

        HRESULT     Search( const DWORD dwTickCountDeadline,
                            const WIN32_FIND_DATA *pfdIn,
                            WIN32_FIND_DATA *pfdOut,
                            UINT uShlinkFlags,
                            DWORD TrackerRestrictions );
        STDMETHODIMP CancelSearch(); // Also in ISLTracker


        BOOL        IsDirty( )
        {
            return( _fDirty );
        }

        BOOL        IsLoaded( )
        {
            return( _fLoaded );
        }

        BOOL        WasLoadedAtLeastOnce( )
        {
            return( _fLoadedAtLeastOnce );
        }


private:

        // Call this from either InitNew or Load
        HRESULT     InitRPC();

#endif  // #ifdef __cplusplus

        BOOL                    _fDirty:1;

        // TRUE => InitNew has be called, but neither InitFromHandle nor Load
        // has been called since.
        BOOL                    _fLoaded:1;

        // TRUE => _cs has been intialized and must be deleted on destruction.
        BOOL                    _fCritsecInitialized:1;

        // TRUE => An async call to LnkMendLink is active
        BOOL                    _fMendInProgress:1;

        // Event used for the async RPC call LnkMendLink, and a critsec to
        // coordinate the search thread and UI thread.

        HANDLE                  _hEvent;
        CRITICAL_SECTION        _cs;

        // Either InitFromHandle or Load has been called at least once, though
        // InitNew may have been called since.
        BOOL                    _fLoadedAtLeastOnce:1;
        IShellLink             *_pshlink;
        PRPC_ASYNC_STATE        _pRpcAsyncState;

        CMachineId              _mcidLast;
        CDomainRelativeObjId    _droidLast;
        CDomainRelativeObjId    _droidBirth;

};  // struct CTracker


#define CTRACKER_VERSION    0

STDAPI Tracker_Search(struct CTracker *pThis,
                       DWORD dwTickCountDeadline,
                       const WIN32_FIND_DATA *pfdIn,
                       WIN32_FIND_DATA *pfdOut, 
                       UINT uShlinkFlags,
                       DWORD TrackerRestrictions);
STDAPI Tracker_CancelSearch(struct CTracker *pThis);

#endif // #ifndef _TRACKER_H_
