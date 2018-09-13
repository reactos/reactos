#include "priv.h"

#include <mluisupp.h>

// stuff that should be turned off while in here, but on when 
// it goes to commonsb or shbrows2.cpp
#define IN_BASESB2

#ifdef IN_BASESB2
#define _fFullScreen FALSE
#endif

#include "sccls.h"

#include "basesb.h"
#include <fsmenu.h>
#include "iedde.h"
#include "bindcb.h"
#include "inpobj.h"
#include "multimon.h"
#include "mmhelper.h"
#include "resource.h"
#include "security.h"
#include <urlmon.h>
#include "favorite.h"
#include "uemapp.h"


#ifdef FEATURE_PICS
#include <ratings.h>
#endif

#ifdef UNIX
#include "unixstuff.h"
#endif


#define DM_ACCELERATOR      0
#define DM_WEBCHECKDRT      0
#define DM_COCREATEHTML     0
#define DM_CACHEOLESERVER   0
#define DM_DOCCP            0
#define DM_PICS             0
#define DM_SSL              0
#define DM_MISC             DM_TRACE    // misc/tmp




//
// BUGBUG: Remove this #include by defining _bbd._pauto as IWebBrowserApp, just like
//  Explorer.exe.
//
#include "hlframe.h"

extern IUnknown* ClassHolder_Create(const CLSID* pclsid);
extern HRESULT VariantClearLazy(VARIANTARG *pvarg);


#define WMC_ASYNCOPERATION      (WMC_RESERVED_FIRST + 0x0000)

// _uActionQueued of WMC_ACYNCOPERATION specifies the operation.
#define ASYNCOP_NIL                 0
#define ASYNCOP_GOTO                1
#define ASYNCOP_ACTIVATEPENDING     2
#define ASYNCOP_CANCELNAVIGATION    3

void IEInitializeClassFactoryObject(IUnknown* punkAuto);
BOOL ParseRefreshContent(LPWSTR pwzContent,
    UINT * puiDelay, LPWSTR pwzUrlBuf, UINT cchUrlBuf);

#define VALIDATEPENDINGSTATE() ASSERT((_bbd._psvPending && _bbd._psfPending) || (!_bbd._psvPending && !_bbd._psfPending))

#define DM_HTTPEQUIV        TF_SHDNAVIGATE
#define DM_NAV              TF_SHDNAVIGATE
#define DM_ZONE             TF_SHDNAVIGATE
#define DM_IEDDE            DM_TRACE
#define DM_CANCELMODE       0
#define DM_UIWINDOW         0
#define DM_ENABLEMODELESS   TF_SHDNAVIGATE
#define DM_EXPLORERMENU     0
#define DM_BACKFORWARD      0
#define DM_PROTOCOL         0
#define DM_ITBAR            0
#define DM_STARTUP          0
#define DM_AUTOLIFE         0
#define DM_PALETTE          0
#define DM_PERSIST          0       // trace IPS::Load, ::Save, etc.
#define DM_VIEWSTREAM       DM_TRACE
#define DM_FOCUS            0
#define DM_FOCUS2           0           // like DM_FOCUS, but verbose

// these two MUST be in order because we peek them together

STDAPI SafeGetItemObject(LPSHELLVIEW psv, UINT uItem, REFIID riid, void **ppv);
extern HRESULT TargetQueryService(LPUNKNOWN punk, REFIID riid, void **ppvObj);
HRESULT CreateTravelLog(ITravelLog **pptl);


#ifdef MESSAGEFILTER
/*
 * CMessageFilter - implementation of IMessageFilter
 *
 * Used to help distribute WM_TIMER messages during OLE operations when 
 * we are busy.  If we don't install the CoRegisterMessageFilter
 * then OLE can PeekMessage(PM_NOREMOVE) the timers such that they pile up
 * and fill the message queue.
 *
 */
class CMessageFilter : public IMessageFilter {
public:
    // *** IUnknown methods ***
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObj)
    {
        // This interface doesn't get QI'ed.
        ASSERT(FALSE);
        return E_NOINTERFACE;
    };
    virtual ULONG __stdcall AddRef(void)    {   return ++_cRef; };
    virtual ULONG __stdcall Release(void)   {   ASSERT(_cRef > 0);
                                                _cRef--;
                                                if (_cRef > 0)
                                                    return _cRef;

                                                delete this;
                                                return 0;
                                            };

    // *** IMessageFilter specific methods ***
    virtual DWORD __stdcall HandleInComingCall(
        IN DWORD dwCallType,
        IN HTASK htaskCaller,
        IN DWORD dwTickCount,
        IN LPINTERFACEINFO lpInterfaceInfo)
    {
        if (_lpMFOld)
           return (_lpMFOld->HandleInComingCall(dwCallType, htaskCaller, dwTickCount, lpInterfaceInfo));
        else
           return SERVERCALL_ISHANDLED;
    };

    virtual DWORD __stdcall RetryRejectedCall(
        IN HTASK htaskCallee,
        IN DWORD dwTickCount,
        IN DWORD dwRejectType)
    {
        if (_lpMFOld)
            return (_lpMFOld->RetryRejectedCall(htaskCallee, dwTickCount, dwRejectType));
        else
            return 0xffffffff;
    };

    virtual DWORD __stdcall MessagePending(
        IN HTASK htaskCallee,
        IN DWORD dwTickCount,
        IN DWORD dwPendingType)
    {
        DWORD dw;
        MSG msg;

        // We can get released during the DispatchMessage call...
        // If it's our last release, we'll free ourselves and
        // fault when we dereference _lpMFOld... Make sure this
        // doesn't happen by increasing our refcount.
        //
        AddRef();

        while (PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE))
        {
#ifndef DISPATCH_IETIMERS
            TCHAR szClassName[40];
                
            GetClassName(msg.hwnd, szClassName, ARRAYSIZE(szClassName));


            if (StrCmpI(szClassName, TEXT("Internet Explorer_Hidden")) != 0)
            {
#endif
                DispatchMessage(&msg);
#ifndef DISPATCH_IETIMERS
            }
#endif
        }

        if (_lpMFOld)
            dw = (_lpMFOld->MessagePending(htaskCallee, dwTickCount, dwPendingType));
        else
            dw = PENDINGMSG_WAITDEFPROCESS;

        Release();

        return(dw);
    };

    CMessageFilter() : _cRef(1)
    {
        ASSERT(_lpMFOld == NULL);
    };

    BOOL Initialize()
    {
        return (CoRegisterMessageFilter((LPMESSAGEFILTER)this, &_lpMFOld) != S_FALSE);
    };

    void UnInitialize()
    {
        CoRegisterMessageFilter(_lpMFOld, NULL);

        // we shouldn't ever get called again, but after 30 minutes
        // of automation driving we once hit a function call above
        // and we dereferenced this old pointer and page faulted.

        ATOMICRELEASE(_lpMFOld);
    };

protected:
    int _cRef;
    LPMESSAGEFILTER _lpMFOld;
};
#endif

//--------------------------------------------------------------------------
// Detecting a memory leak
//--------------------------------------------------------------------------

HRESULT GetTopFrameOptions(IServiceProvider * psp, DWORD * pdwOptions)
{
    HRESULT hres;
    IServiceProvider * pspTop;

    if(SUCCEEDED(hres = psp->QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (void **)&pspTop)))
    {
        LPTARGETFRAME2      ptgf;
        if (SUCCEEDED(hres = pspTop->QueryService(SID_SContainerDispatch, IID_ITargetFrame2, (void **)&ptgf)))
        {
            hres = ptgf->GetFrameOptions(pdwOptions);
            ptgf->Release();
        }
        pspTop->Release();
    }

    return hres;
}

void UpdateDesktopComponentName(LPITEMIDLIST pidl, LPCWSTR lpcwszName)
{
    IActiveDesktop * piad;

    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_IActiveDesktop, (void **)&piad)))
    {
        WCHAR wzPath[MAX_URL_STRING];
        if (SUCCEEDED(::IEGetDisplayName(pidl, wzPath, SHGDN_FORPARSING)))
        {
            COMPONENT comp;
            comp.dwSize = SIZEOF(comp);

            if (SUCCEEDED(piad->GetDesktopItemBySource(wzPath, &comp, 0)) && !comp.wszFriendlyName[0])
            {
                StrCpyNW(comp.wszFriendlyName, lpcwszName, ARRAYSIZE(comp.wszFriendlyName));
                piad->ModifyDesktopItem(&comp, COMP_ELEM_FRIENDLYNAME);
                piad->ApplyChanges(AD_APPLY_SAVE);
            }
        }
        piad->Release();
    }
}

HRESULT CBaseBrowser2::_Initialize(HWND hwnd, IUnknown* pauto)
{
    if (pauto)
    {
        pauto->AddRef();
    }
    else
    {
        CIEFrameAuto_CreateInstance(NULL, &pauto);
    }

    // Grab _pauto interfaces we use throughout this code.
    //
    if (pauto)
    {
        pauto->QueryInterface(IID_IWebBrowser2, (void **)&_bbd._pautoWB2);
        ASSERT(_bbd._pautoWB2);

        pauto->QueryInterface(IID_IExpDispSupport, (void **)&_bbd._pautoEDS);
        ASSERT(_bbd._pautoEDS);

        pauto->QueryInterface(IID_IShellService, (void **)&_bbd._pautoSS);
        ASSERT(_bbd._pautoSS);

        pauto->QueryInterface(IID_ITargetFrame2, (void **)&_ptfrm);
        ASSERT(_ptfrm);

        pauto->QueryInterface(IID_IHlinkFrame, (void **)&_bbd._phlf);
        ASSERT(_bbd._phlf);

        IHTMLWindow2 *pWindow;
        if( SUCCEEDED(GetWindowFromUnknown( pauto, &pWindow )) )
        {
            pWindow->QueryInterface( IID_IShellHTMLWindowSupport, (void**)&_phtmlWS );
            pWindow->Release( );
        }
        ASSERT( _phtmlWS );

        _pauto = pauto;
    }

    // BUGBUG _psbOuter?
    if (NULL == _bbd._phlf)
    {
        Release();
        return E_FAIL;
    }
    else
    {
        _SetWindow(hwnd);
        return S_OK;
    }
}

HRESULT CBaseBrowser2_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CBaseBrowser2 *pbb = new CBaseBrowser2(pUnkOuter);
    if (pbb)
    {
        *ppunk = pbb->_GetInner();
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

CBaseBrowser2::CBaseBrowser2(IUnknown* punkAgg) :
       CAggregatedUnknown(punkAgg),
        _bptBrowser(BPT_DeferPaletteSupport)
{
    TraceMsg(TF_SHDLIFE, "ctor CBaseBrowser2 %x", this);

    _bbd._uActivateState = SVUIA_ACTIVATE_FOCUS;
    _InitComCtl32();

    ASSERT(S_FALSE == _DisableModeless());
    ASSERT(_cp == CP_ACP);
    ASSERT(!_fNoTopLevelBrowser);

#if 0 // split: here for context (ordering)
    EVAL(FDSA_Initialize(SIZEOF(TOOLBARITEM), ITB_CGROW, &_fdsaTBar, _aTBar, ITB_CSTATIC));
#endif


    _QueryOuterInterface(IID_IBrowserService2, (void **)&_pbsOuter);
    _QueryOuterInterface(IID_IShellBrowser, (void **)&_psbOuter);
    _QueryOuterInterface(IID_IServiceProvider, (void **)&_pspOuter);
    // The following are intercepted by CCommonBrowser, but we don't call 'em
    //_QueryOuterInterface(IID_IOleCommandTarget, (void **)&_pctOuter);
    //_QueryOuterInterface(IID_IInputObjectSite, (void **)&_piosOuter);

}


HRESULT CBaseBrowser2_Validate(HWND hwnd, void ** ppsb)
{
    CBaseBrowser2* psb = *(CBaseBrowser2**)ppsb;    // BUGBUG split: bogus!!!

    if (psb)
    {
        if (NULL == psb->_bbd._phlf)
        {
            ATOMICRELEASE(psb);
        }
        else
        {
            psb->_SetWindow(hwnd);
        }
    }

    *ppsb = psb;        // BUGBUG split: bogus!!!

    return (psb) ? S_OK : E_OUTOFMEMORY;
}

CBaseBrowser2::~CBaseBrowser2()
{
    TraceMsg(TF_SHDLIFE, "dtor CBaseBrowser2 %x", this);

    delete _ptrsite;
    _ptrsite = NULL;
    
    // BUGBUG are we releasing these too early (i.e. does anything in the
    // rest of this func rely on having the 'vtables' still be valid?)
    RELEASEOUTERINTERFACE(_pbsOuter);
    RELEASEOUTERINTERFACE(_psbOuter);
    RELEASEOUTERINTERFACE(_pspOuter);
    // The following are intercepted by CCommonBrowser, but we don't call 'em
    //RELEASEOUTERINTERFACE(_pctOuter);
    //RELEASEOUTERINTERFACE(_piosOuter);
    
    ASSERT(_hdpaDLM == NULL);    // subclass must free it.

#if 0 // split: here for context (ordering)
    _CloseAndReleaseToolbars(FALSE);

    FDSA_Destroy(&_fdsaTBar);
#endif

    // finish tracking here
    if (_ptracking) {
        delete _ptracking;
        _ptracking = NULL;
    }

    //
    // Notes: Unlike IE3.0, we release CIEFrameAuto pointers here.
    //
    ATOMICRELEASE(_bbd._pautoWB2);
    ATOMICRELEASE(_bbd._pautoEDS);
    ATOMICRELEASE(_bbd._pautoSS);
    ATOMICRELEASE(_phtmlWS);
    ATOMICRELEASE(_bbd._phlf);
    ATOMICRELEASE(_ptfrm);
    ATOMICRELEASE(_pauto);
    
    ATOMICRELEASE(_punkSFHistory);

    // clean up our palette by simulating a switch out of palettized mode
    _bptBrowser = BPT_NotPalettized;
    _QueryNewPalette();


    ASSERT(!_bbd._phlf);
    ASSERT(!_ptfrm);
    ASSERT(S_FALSE == _DisableModeless());
    ASSERT(_bbd._hwnd==NULL);

    ATOMICRELEASE(_pact);

    ATOMICRELEASE(_pIUrlHistoryStg);
    ATOMICRELEASE(_poleHistory);
    ATOMICRELEASE(_pstmHistory);
    ATOMICRELEASE(_bbd._ptl);

#ifdef MESSAGEFILTER
    if (_lpMF) {
        IMessageFilter* lpMF = _lpMF;
        _lpMF = NULL;
        ((CMessageFilter *)lpMF)->UnInitialize();
        EVAL(lpMF->Release() == 0);
    }
#endif

    // This is created during FileCabinet_CreateViewWindow2
    CShellViews_Delete(&_fldBase._cViews);


    //
    // If the class factory object has been cached, unlock it and release.
    //
    if (_pcfHTML) {
        _pcfHTML->LockServer(FALSE);
        _pcfHTML->Release();
    }

    ATOMICRELEASE(_pToolbarExt);
}

HRESULT CBaseBrowser2::v_InternalQueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CBaseBrowser2, IShellBrowser),         // IID_IShellBrowser
        QITABENTMULTI(CBaseBrowser2, IOleWindow, IShellBrowser), // IID_IOleWindow
        QITABENT(CBaseBrowser2, IOleInPlaceUIWindow),   // IID_IOleInPlaceUIWindow
        QITABENT(CBaseBrowser2, IOleCommandTarget),     // IID_IOleCommandTarget
        QITABENT(CBaseBrowser2, IDropTarget),           // IID_IDropTarget
        QITABENTMULTI(CBaseBrowser2, IBrowserService, IBrowserService2), // IID_IBrowserService
        QITABENT(CBaseBrowser2, IBrowserService2),      // IID_IBrowserService2
        QITABENT(CBaseBrowser2, IServiceProvider),      // IID_IServiceProvider
        QITABENT(CBaseBrowser2, IOleContainer),         // IID_IOleContainer
        QITABENT(CBaseBrowser2, IAdviseSink),           // IID_IAdviseSink
        QITABENT(CBaseBrowser2, IInputObjectSite),      // IID_IInputObjectSite
        QITABENT(CBaseBrowser2, IDocNavigate),          // IID_IDocNavigate
        QITABENT(CBaseBrowser2, IPersistHistory),       // IID_IPersistHistory
        QITABENT(CBaseBrowser2, IInternetSecurityMgrSite), // IID_IInternetSecurityMgrSite
        QITABENT(CBaseBrowser2, IVersionHost),          // IID_IVersionHost
#ifdef LIGHT_FRAMES
        QITABENT(CBaseBrowser2, IOleClientSite),        // IID_IOleClientSite
        QITABENT(CBaseBrowser2, IOleInPlaceSite),       // IID_IOleInPlaceSite
        QITABENT(CBaseBrowser2, IOleInPlaceFrame),       // IID_IOleInPlaceFrame
#endif
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

BOOL CBaseBrowser2::_IsViewMSHTML(IShellView *psv)
{
    BOOL fIsMSHTML = FALSE;
    
    if (psv)
    {
        IPersist *pPersist = NULL;
        HRESULT hres = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_IPersist, (void **)&pPersist);
        if (SUCCEEDED(hres))
        {
            CLSID clsid;
            hres = pPersist->GetClassID(&clsid);
            if (SUCCEEDED(hres) && IsEqualGUID(clsid, CLSID_HTMLDocument))
                fIsMSHTML = TRUE;
            pPersist->Release();
        }
    }
    return fIsMSHTML;
}

HRESULT CBaseBrowser2::ReleaseShellView()
{
    //  We're seeing some reentrancy here.  If _cRefUIActivateSV is non-zero, it means we're
    //  in the middle of UIActivating the shell view.
    //
    if (_cRefUIActivateSV)
    {
        TraceMsg(TF_WARNING, 
            "CBB(%x)::ReleaseShellView _cRefUIActivateSV(%d)!=0  _bbd._psv=%x ABORTING", 
            this, _cRefUIActivateSV, _bbd._psv);
        return S_OK;
    }
    
    BOOL fViewObjectChanged = FALSE;

    VALIDATEPENDINGSTATE();

    TraceMsg(DM_NAV, "CBaseBrowser2(%x)::ReleaseShellView(%x)", this, _bbd._psv);

    ATOMICRELEASE(_pdtView);

    if (_bbd._psv) {
        //
        //  Disable navigation while we are UIDeactivating/DestroyWindowing
        // the IShellView. Some OC/DocObject in it (such as ActiveMovie)
        // might have a message loop long enough to cause some reentrancy.
        //
        _psbOuter->EnableModelessSB(FALSE);

        // Tell the shell's HTML window we are releasing the document.
        if( _phtmlWS )
            _phtmlWS->ViewReleased( );

        //
        //  We need to cancel the menu mode so that unmerging menu won't
        // destroy the menu we are dealing with (which caused GPF in USER).
        // DocObject needs to do appropriate thing for context menus.
        // (02-03-96 SatoNa)
        //
        HWND hwndCapture = GetCapture();
        TraceMsg(DM_CANCELMODE, "ReleaseShellView hwndCapture=%x _bbd._hwnd=%x", hwndCapture, _bbd._hwnd);
        if (hwndCapture && hwndCapture==_bbd._hwnd) {
            TraceMsg(DM_CANCELMODE, "ReleaseShellView Sending WM_CANCELMODE");
            SendMessage(_bbd._hwnd, WM_CANCELMODE, 0, 0);
        }

        //
        //  We don't want to resize the previous view window while we are
        // navigating away from it.
        //
        TraceMsg(TF_SHDUIACTIVATE, "CSB::ReleaseShellView setting _fDontResizeView");
        _fDontResizeView = TRUE;

        // If the current view is still waiting for ReadyStateComplete,
        // and the view we're swapping in here does not support this property,
        // then we'll never go to ReadyStateComplete! Simulate it here:
        //
        // NOTE: ZekeL put this in _CancelNavigation which happened way too often.
        // I think this is the case he was trying to fix, but I don't remember
        // the bug number so I don't have a specific repro...
        //
        
        if (!_bbd._fIsViewMSHTML)
        {
            _fReleasingShellView = TRUE;
            OnReadyStateChange(_bbd._psv, READYSTATE_COMPLETE);
            _fReleasingShellView = FALSE;
        }


        // At one point during a LOR stress test, we got re-entered during
        // this UIActivate call (some rogue 3rd-party IShellView perhaps?)
        // which caused _psv to get freed, and we faulted during the unwind.
        // Gaurd against this by swapping the _psv out early.
        //
        IShellView* psv = _bbd._psv;
        _bbd._psv = NULL;
        if (psv)
        {
            psv->UIActivate(SVUIA_DEACTIVATE);
            if (_cRefUIActivateSV)
            {
                TraceMsg(TF_WARNING, "CBB(%x)::ReleaseShellView setting _bbd._psv = NULL (was %x) while _cRefUIActivateSV=%d",
                    this, psv, _cRefUIActivateSV);
            }

            ATOMICRELEASE(_bbd._pctView);

            if (_pvo)
            {
                IAdviseSink *pSink;

                // paranoia: only blow away the advise sink if it is still us
                if (SUCCEEDED(_pvo->GetAdvise(NULL, NULL, &pSink)) && pSink)
                {
                    if (pSink == (IAdviseSink *)this)
                        _pvo->SetAdvise(0, 0, NULL);

                    pSink->Release();
                }

                fViewObjectChanged = TRUE;
                ATOMICRELEASE(_pvo);
            }
            
            psv->SaveViewState();
            TraceMsg(DM_NAV, "ief NAV::%s %x",TEXT("ReleaseShellView Calling DestroyViewWindow"), psv);
            psv->DestroyViewWindow();
    
            UINT cRef = psv->Release();
            TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("ReleaseShellView called psv->Release"), psv, cRef);

            _bbd._hwndView = NULL;
            TraceMsg(TF_SHDUIACTIVATE, "CSB::ReleaseShellView resetting _fDontResizeView");
            _fDontResizeView = FALSE;

            if (_bbd._pidlCur) {
                ILFree(_bbd._pidlCur);
                _bbd._pidlCur = NULL;
            }
        }
        
        _psbOuter->EnableModelessSB(TRUE);

        //
        //  If there is any blocked async operation AND we can navigate now,
        // unblock it now. 
        //
        _MayUnblockAsyncOperation();

    }

    ATOMICRELEASE(_bbd._psf);

    if (fViewObjectChanged)
        _ViewChange(DVASPECT_CONTENT, -1);

    if (_bbd._pszTitleCur)
    {
        LocalFree(_bbd._pszTitleCur);
        _bbd._pszTitleCur = NULL;
    }

    // NOTES: (SatoNa)
    //
    //  This is the best time to clean up the left-over from UI-negotiation
    // from the previous DocObject. Excel 97, for some reason, takes 16
    // pixels from the top (for the formula bar) when we UI-deactivate it
    // by callin gIOleDocumentView::UIActivate(FALSE), which we call above.
    //
    SetRect(&_rcBorderDoc, 0, 0, 0, 0);
    return S_OK;
}

void CBaseBrowser2::_StopCurrentView()
{
    // send OLECMDID_STOP
    if (_bbd._pctView) // we must check!
    {
        _bbd._pctView->Exec(NULL, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
    }
}

//
// This function synchronously cancels the pending navigation if any.
//
HRESULT CBaseBrowser2::_CancelPendingNavigation(BOOL fDontReleaseState)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_CancelPendingNavigation called");
    
    _KillRefreshTimer();
    _StopAsyncOperation();
    
    HRESULT hres = S_FALSE;

    if (_bbd._psvPending) {
        if (_bbd._phlf && !fDontReleaseState) {
             // release our state
             _bbd._phlf->Navigate(0, NULL, NULL, NULL);
        }
        _CancelPendingView();
        hres = S_OK;
    }

//#ifdef DEBUG
//    if (hres == S_FALSE)
//        TraceMsg(TF_WARNING, "CBB::_CancelPendingNavigation returns S_FALSE");
//#endif
    return hres;
}

void CBaseBrowser2::_SendAsyncNavigationMsg(VARIANTARG *pvarargIn)
{
    if (EVAL(pvarargIn->vt == VT_BSTR && pvarargIn->bstrVal))
    {
        TCHAR szName[MAX_URL_STRING];
        LPITEMIDLIST pidl;

        OleStrToStrN(szName, MAX_URL_STRING, pvarargIn->bstrVal, -1);
    
        if (EVAL(SUCCEEDED(IECreateFromPath(szName, &pidl))))
        {
            _NavigateToPidlAsync(pidl, 0); // takes ownership of pidl
        }
    }
}


//
// NOTES: It does not cancel the pending view.
//
void CBaseBrowser2::_StopAsyncOperation(void)
{
    // Don't remove posted WMC_ASYNCOPERATION message. PeekMesssage removes
    // messages for children! (SatoNa)
    _uActionQueued = ASYNCOP_NIL;

    // Remove the pidl in the queue (single depth)
    _FreeQueuedPidl(&_pidlQueued);
}

//
//  This function checks if we have any asynchronous operation AND
// we no longer need to postpone. In that case, we unblock it by
// posting a WMC_ASYNCOPERATION.
//
void CBaseBrowser2::_MayUnblockAsyncOperation(void)
{
    if (_uActionQueued!=ASYNCOP_NIL && _CanNavigate()) {
        TraceMsg(TF_SHDNAVIGATE, "CBB::_MayUnblockAsyncOp posting WMC_ASYNCOPERATION");
        PostMessage(_bbd._hwnd, WMC_ASYNCOPERATION, 0, 0);
    }
}

BOOL CBaseBrowser2::_PostAsyncOperation(UINT uAction)
{
    _uActionQueued = uAction;
    return PostMessage(_bbd._hwnd, WMC_ASYNCOPERATION, 0, 0);
}

LRESULT CBaseBrowser2::_SendAsyncOperation(UINT uAction)
{
    _uActionQueued = uAction;
    return SendMessage(_bbd._hwnd, WMC_ASYNCOPERATION, 0, 0);
}

HRESULT CBaseBrowser2::_CancelPendingNavigationAsync(void)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_CancelPendingNavigationAsync called");

    _KillRefreshTimer();
    _StopAsyncOperation();
    _PostAsyncOperation(ASYNCOP_CANCELNAVIGATION);
    return S_OK;
}

HRESULT CBaseBrowser2::_CancelPendingView(void)
{
    if (_bbd._psvPending) {
        TraceMsg(DM_NAV, "ief NAV::%s %x",TEXT("_CancelPendingView Calling DestroyViewWindow"), _bbd._psvPending);
        _bbd._psvPending->DestroyViewWindow();

        ASSERT(_bbd._psfPending);

        // When cancelling a pending navigation, make sure we
        // think the pending operation is _COMPLETE otherwise
        // we may get stuck in a _LOADING state...
        //
        TraceMsg(TF_SHDNAVIGATE, "basesb(%x) Fake pending ReadyState_Complete", this);
        OnReadyStateChange(_bbd._psvPending, READYSTATE_COMPLETE);

        ATOMICRELEASE(_bbd._psvPending);

        // Paranoia
        ATOMICRELEASE(_bbd._psfPending);
        
        _bbd._hwndViewPending = NULL;

        _setDescendentNavigate(NULL);
        SetNavigateState(BNS_NORMAL);

        if (_bbd._pidlPending) {
            ILFree(_bbd._pidlPending);
            _bbd._pidlPending = NULL;
        }

        if (_bbd._pszTitlePending)
        {
            LocalFree(_bbd._pszTitlePending);
            _bbd._pszTitlePending = NULL;
        }

        // Pending navigation is canceled.
        // since the back button works as a stop on pending navigations, we
        // should check that here as well.
        _pbsOuter->UpdateBackForwardState();
        _NotifyCommandStateChange();

        _PauseOrResumeView(_fPausedByParent);
    }
    return S_OK;
}

void CBaseBrowser2::_UpdateTravelLog(void)
{
    //
    //  we update the travellog in two parts.  first we update
    //  the current entry with the current state info, 
    //  then we create a new empty entry.  UpdateEntry()
    //  and AddEntry() need to always be in pairs, with 
    //  identical parameters.
    //
    //  if this navigation came from a LoadHistory, the 
    //  _fDontAddTravelEntry will be set, and the update and 
    //  cursor movement will have been adjusted already.
    //  we also want to prevent new frames from updating 
    //  and adding stuff, so unless this is the top we
    //  wont add to the travellog if this is a new frame.
    //
    ASSERT(!(_grfHLNFPending & HLNF_CREATENOHISTORY));

    ITravelLog *ptl;
    GetTravelLog(&ptl);
    BOOL fTopFrameBrowser = IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *));
 
    if(ptl)
    {
        //  
        //  some times we are started by another app (MSWORD usually) that has HLink
        //  capability.  we detect this by noting that we are a new browser with an empty
        //  TravelLog, and then see if we can get a IHlinkBrowseContext.  if this is successful,
        //  we should add an entry and update it immediately with the external info.
        //
        IHlinkBrowseContext *phlbc = NULL;  // init to suppress bogus C4701 warning
        BOOL fExternalNavigate = (FAILED(ptl->GetTravelEntry(SAFECAST(this, IBrowserService *), 0, NULL)) &&
            fTopFrameBrowser && _bbd._phlf && SUCCEEDED(_bbd._phlf->GetBrowseContext(&phlbc)));

        if(fExternalNavigate)
        {
            ptl->AddEntry(SAFECAST(this, IBrowserService *), FALSE);
            ptl->UpdateExternal(SAFECAST(this, IBrowserService *), phlbc);
            phlbc->Release();
        }
        else if(_bbd._psv && !_fIsLocalAnchor)
            ptl->UpdateEntry(SAFECAST(this, IBrowserService *), FALSE);  //CAST for IUnknown

        if(!_fDontAddTravelEntry && (_bbd._psv || fTopFrameBrowser))
            ptl->AddEntry(SAFECAST(this, IBrowserService *), _fIsLocalAnchor);  //CAST for IUnknown

        ptl->Release();
    }

    _fDontAddTravelEntry = FALSE;
    _fIsLocalAnchor = FALSE;
}

void CBaseBrowser2::_OnNavigateComplete(LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    _pbsOuter->UpdateBackForwardState();
}


//// CHEEBUGBUG: does only the top shbrowse need this?  or the top oc frame too?
HRESULT CBaseBrowser2::UpdateSecureLockIcon(int eSecureLock)
{
    // only the top boy should get to set his stuff
    if (!IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *)))
        return S_OK;

    if (eSecureLock != SECURELOCK_NOCHANGE)
        _bbd._eSecureLockIcon = eSecureLock;
    
    // 
    //  BUGBUG:  There is no mixed Security Icon - zekel 6-AUG-97
    //  right now we have no icon or TT for SECURELOCK_SET_MIXED, which 
    //  is set when the root page is secure but some of the other content
    //  or frames are not.  some PM needs to implement, probably 
    //  with consultation from TonyCi and DBau.  by default we currently
    //  only show for pages that are completely secure.
    //

    TraceMsg(DM_SSL, "CBB:UpdateSecureLockIcon() _bbd._eSecureLockIcon = %d", _bbd._eSecureLockIcon);

    //
    // BUGBUG  it looks like it doesnt matter what icon we select here,
    // the status bar always shows some lock icon that was cached there earlier
    // and it treats this HICON as a bool to indicat on or off  - zekel - 5-DEC-97
    //

    HICON hicon = NULL;
    TCHAR szText[MAX_TOOLTIP_STRING];

    szText[0] = 0;

    switch (_bbd._eSecureLockIcon)
    {
    case SECURELOCK_SET_UNSECURE:
    case SECURELOCK_SET_MIXED:
        hicon = NULL;
        break;

    case SECURELOCK_SET_SECUREUNKNOWNBIT:
        hicon = g_hiconSSL;
        break;

    case SECURELOCK_SET_SECURE40BIT:
        hicon = g_hiconSSL;
        MLLoadString(IDS_SSL40, szText, ARRAYSIZE(szText));
        break;

    case SECURELOCK_SET_SECURE56BIT:
        hicon = g_hiconSSL;
        MLLoadString(IDS_SSL56, szText, ARRAYSIZE(szText));
        break;

    case SECURELOCK_SET_SECURE128BIT:
        hicon = g_hiconSSL;
        MLLoadString(IDS_SSL128, szText, ARRAYSIZE(szText));
        break;

    case SECURELOCK_SET_FORTEZZA:
        hicon = g_hiconFortezza;
        MLLoadString(IDS_SSL_FORTEZZA, szText, ARRAYSIZE(szText));
        break;

    default:
        ASSERT(0);
        return E_FAIL;
    }

    VARIANTARG var = {0};
    if (_bbd._pctView && SUCCEEDED(_bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_SSL, NULL, &var))
        && V_UI4(&var) != PANE_NONE)
    {
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETICON, V_UI4(&var), (LPARAM)(hicon), NULL);
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTIPTEXT, V_UI4(&var), (LPARAM)(szText[0] ? szText : NULL), NULL);
    }    
    return S_OK;
}
//
//  This block of code simply prevents calling UIActivate of old
// extensions with a new SVUIA_ value.
//
HRESULT CBaseBrowser2::_UIActivateView(UINT uState)
{
    if (_bbd._psv) 
    {
        BOOL fShellView2;
        IShellView2* psv2;
        if (SUCCEEDED(_bbd._psv->QueryInterface(IID_IShellView2, (void **)&psv2)))
        {
            fShellView2 = TRUE;
            psv2->Release();
        }
        else
        {
            fShellView2 = FALSE;
        }

        if (uState == SVUIA_INPLACEACTIVATE && !fShellView2)
        {
            uState = SVUIA_ACTIVATE_NOFOCUS;        // map it to old one.
        }

        if (_cRefUIActivateSV)
        {
            TraceMsg(TF_WARNING, "CBB(%x)::_UIActivateView(%d) entered reentrantly!!!!!! _cRefUIActivate=%d",
                this, uState, _cRefUIActivateSV);
            if (uState == SVUIA_DEACTIVATE)
            {
                _fDeferredUIDeactivate = TRUE;
                return S_OK;
            }

            if (!_HeyMoe_IsWiseGuy())
            {
                if (_bbd._psv)
                    _bbd._psv->UIActivate(SVUIA_INPLACEACTIVATE);
                return S_OK;
            }
        }

        _cRefUIActivateSV++;

        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView(%d) about to call _bbd._psv(%x)->UIActivate",
            this, uState, _bbd._psv);

        _bbd._psv->UIActivate(uState);

        if (uState == SVUIA_ACTIVATE_FOCUS && !fShellView2)
        {
            // win95 defview expects a SetFocus on activation (nt5 bug#172210)
            if (_bbd._hwndView)
                SetFocus(_bbd._hwndView);
        }

        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView(%d) back from _bbd._psv(%x)->UIActivate",
            this, uState, _bbd._psv);

        _cRefUIActivateSV--;

        UpdateSecureLockIcon(SECURELOCK_NOCHANGE);
        
    }
    _bbd._uActivateState = uState;

    // If this is a pending view, set the focus to its window even though it's hidden.
    // In ActivatePendingView(), we check if this window still has focus and, if it does,
    // we will ui-activate the view. Fix for IE5 bug #70632 -- MohanB

    if (    SVUIA_ACTIVATE_FOCUS == uState
        &&  !_bbd._psv
        &&  !_bbd._hwndView
        &&  _bbd._psvPending
        &&  _bbd._hwndViewPending
       )
    {
        ::SetFocus(_bbd._hwndViewPending);
    }

    if (_fDeferredUIDeactivate)
    {
        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView processing deferred UIDeactivate, _bbd._psv=%x",
            this, _bbd._psv);
        _fDeferredUIDeactivate = FALSE;
        if (_bbd._psv)
            _bbd._psv->UIActivate(SVUIA_DEACTIVATE);
        UpdateSecureLockIcon(SECURELOCK_NOCHANGE);
        _bbd._uActivateState = SVUIA_DEACTIVATE;
    }

    if (_fDeferredSelfDestruction)
    {
        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView processing deferred OnDestroy",
            this);
        _fDeferredSelfDestruction = FALSE;
        _pbsOuter->OnDestroy();
    }

    return S_OK;
}


//Called from CShellBrowser::OnCommand
HRESULT CBaseBrowser2::Offline(int iCmd)
{
    HRESULT hresIsOffline = IsGlobalOffline() ? S_OK : S_FALSE;

    switch(iCmd){
    case SBSC_TOGGLE:
        hresIsOffline = (hresIsOffline == S_OK) ? S_FALSE : S_OK; // Toggle Property
        // Tell wininet that the user wants to go offline
        SetGlobalOffline(hresIsOffline == S_OK); 
        SendShellIEBroadcastMessage(WM_WININICHANGE,0,0, 1000); // Tell all browser windows to update their title   
        break;
        
    case SBSC_QUERY:
        break;
    default: // Treat like a query
        break;                   
    }
    return hresIsOffline;
}

void CBaseBrowser2::_GetNavigationInfo(WORD * pwNavTypeFlags)
{
    *pwNavTypeFlags = 0;

    for (;;)
    {
        BOOL fCurIsWeb = ILIsWeb(_bbd._pidlCur);
        BOOL fPendingIsWeb = ILIsWeb(_bbd._pidlPending);

        if ((_bbd._pidlCur == NULL) || (_bbd._pidlPending == NULL))
        {
            *pwNavTypeFlags |= (NAVTYPE_PageIsChanging
                                    | NAVTYPE_SiteIsChanging);

            // Make sure we don't do transitions when
            // just navigating between non-web PIDLs.
            if ((_bbd._pidlCur && !fCurIsWeb)
            || (_bbd._pidlPending && !fPendingIsWeb))
            {
                *pwNavTypeFlags |= NAVTYPE_ShellNavigate;
            }

            break;
        }

        // Make sure we don't do transitions when
        // just navigating between non-web PIDLs.
        if (!fCurIsWeb && !fPendingIsWeb)
        {
            *pwNavTypeFlags |= (NAVTYPE_ShellNavigate | NAVTYPE_PageIsChanging);
            break;
        }

        // Check to see if this is a local anchor href navigate on the same page.
        if (!IEILIsEqual(_bbd._pidlCur, _bbd._pidlPending, TRUE))
            *pwNavTypeFlags |= NAVTYPE_PageIsChanging;

        // Check to see if the internet site is changing.
        TCHAR szURL[MAX_URL_STRING];
        TCHAR szCurHostName[INTERNET_MAX_HOST_NAME_LENGTH];
        TCHAR szPendingHostName[INTERNET_MAX_HOST_NAME_LENGTH]; 
        DWORD cch = SIZECHARS(szCurHostName);

        if (SUCCEEDED(::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_FORPARSING, szURL, SIZECHARS(szURL), NULL))
        &&  SUCCEEDED(UrlGetPart(szURL, szCurHostName, &cch, URL_PART_HOSTNAME, 0))
        &&  SUCCEEDED(::IEGetNameAndFlags(_bbd._pidlPending, SHGDN_FORPARSING, szURL, SIZECHARS(szURL), NULL))
        &&  (cch = SIZECHARS(szPendingHostName))
        &&  SUCCEEDED(UrlGetPart(szURL, szPendingHostName, &cch, URL_PART_HOSTNAME, 0))
        &&  0 != StrCmpI(szCurHostName, szPendingHostName))
        {
            *pwNavTypeFlags |= NAVTYPE_SiteIsChanging;
        }

        break;
    }
}

void CBaseBrowser2::_MayPlayTransition(IShellView* psvNew, HWND hwndViewNew, BOOL bSiteChanging)
{
    ASSERT(_bbd._psvPending == NULL);
    ASSERT(_bbd._hwndViewPending == NULL);

    DWORD   dwEnabled;
    DWORD   dwSize = SIZEOF(dwEnabled);
    BOOL    bDefault = REGSTR_VAL_PAGETRANSITIONS_DEF;
    SHRegGetUSValue(REGSTR_PATH_MAIN,
                    REGSTR_VAL_PAGETRANSITIONS,
                    NULL,
                    (void *)&dwEnabled,
                    &dwSize,
                    FALSE,
                    (void *)&bDefault,
                    SIZEOF(bDefault));
    
    // Leave early if the user doesn't like transtions between pages.
    if (!dwEnabled)
        return;

    // We are not supposed to be called recursively during _PlayTransition.
    ASSERT(_ptrsite->_uState != CTransitionSite::TRSTATE_PAINTING);

    // Call _PlayTransition only if we have a current view.
    ASSERT(psvNew != NULL);
    ASSERT(_bbd._psv != NULL);

    _ptrsite->_psvNew = psvNew;
    _ptrsite->_psvNew->AddRef();
    _ptrsite->_hwndViewNew = hwndViewNew;

    HRESULT hres = _ptrsite->_ApplyTransition(bSiteChanging);

    if (SUCCEEDED(hres))
    {
        ASSERT(_ptrsite->_uState == CTransitionSite::TRSTATE_INITIALIZING);

        _pbsOuter->ReleaseShellView();
    
        hres = _ptrsite->_StartTransition();
        ASSERT(_ptrsite->_uState == CTransitionSite::TRSTATE_PAINTING);
    }
}

BOOL _TrackPidl(LPITEMIDLIST pidl, IUrlHistoryPriv *php, BOOL fIsOffline, LPTSTR pszUrl, DWORD cchUrl)
{
    BOOL fRet = FALSE;

    // BUGBUG: Should use IsBrowserFrameOptionsPidlSet(pidl, BFO_ENABLE_HYPERLINK_TRACKING)
    //     instead of IsURLChild() because it doesn't work in Folder Shortcuts and doesn't
    //     work in NSEs outside of the "IE" name space (like Web Folders).
    if (pidl && IsURLChild(pidl, FALSE))
    {
        if (SUCCEEDED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, pszUrl, cchUrl, NULL)))
        {
            PROPVARIANT vProp = {0};

            //  BUGBUG - DOES THIS EVER WORK??? - ZekeL - 6-JAN-98
            //  i see no place where we ever set the 
            php->GetProperty(pszUrl, PID_INTSITE_TRACKING, &vProp);

            if (vProp.vt == VT_UI4)
            {
                if (fIsOffline)
                    fRet = (vProp.ulVal & TRACK_OFFLINE_CACHE_ENTRY) ? TRUE : FALSE;
                else
                    fRet = (vProp.ulVal & TRACK_ONLINE_CACHE_ENTRY) ? TRUE : FALSE;
            }

            PropVariantClear(&vProp);
        }
    }

    return fRet;
}

// End tracking of previous page
// May start tracking of new page
// use SatoN's db to quick check tracking/tracking scope bits, so
// to eliminate call to CUrlTrackingStg::IsOnTracking
void CBaseBrowser2::_MayTrackClickStream(LPITEMIDLIST pidlNew)
{
    BOOL    fIsOffline = (Offline(SBSC_QUERY) != S_FALSE);
    IUrlHistoryStg*    phist;
    IUrlHistoryPriv*   phistp;
    PROPVARIANT vProp = {0};
    TCHAR szUrl[MAX_URL_STRING];

    ASSERT(_bbd._pautoWB2);
    if (FAILED(_pspOuter->QueryService(SID_STopLevelBrowser, IID_IUrlHistoryStg, (void **)&phist)))
        return;

    if (FAILED(phist->QueryInterface(IID_IUrlHistoryPriv,(void **)&phistp)))
        return;

    phist->Release();

    if (_TrackPidl(_bbd._pidlCur, phistp, fIsOffline, szUrl, SIZECHARS(szUrl)))
    {
        if (!_ptracking)
            return;

        _ptracking->OnUnload(szUrl);
    }

    if (_TrackPidl(pidlNew, phistp, fIsOffline, szUrl, SIZECHARS(szUrl)))
    {    
        // instance of object already exists
        BRMODE brMode = BM_NORMAL;
        DWORD dwOptions;

        if (!_ptracking) {
            _ptracking = new CUrlTrackingStg();
            if (!_ptracking)
                return;
        }

        if (SUCCEEDED(GetTopFrameOptions(_pspOuter, &dwOptions)))
        {
            //Is this a desktop component?                    
            if (dwOptions & FRAMEOPTIONS_DESKTOP)
                brMode = BM_DESKTOP;
            //Is it fullscreen?                    
            else if (dwOptions & (FRAMEOPTIONS_SCROLL_AUTO | FRAMEOPTIONS_NO3DBORDER))
                brMode = BM_THEATER;
        }

        ASSERT(_ptracking);
        _ptracking->OnLoad(szUrl, brMode, FALSE);
    }

    phistp->Release();

    return;
}


HRESULT CBaseBrowser2::_SwitchActivationNow()
{
    ASSERT(_bbd._psvPending);

    WORD wNavTypeFlags = 0;  // init to suppress bogus C4701 warning
    if (_ptrsite)
        _GetNavigationInfo(&wNavTypeFlags);

    IShellView* psvNew = _bbd._psvPending;
    IShellFolder* psfNew = _bbd._psfPending;
    HWND hwndViewNew = _bbd._hwndViewPending;
    LPITEMIDLIST pidlNew = _bbd._pidlPending;

    _bbd._fIsViewMSHTML = _IsViewMSHTML(psvNew);
    
    _bbd._psvPending = NULL;
    _bbd._psfPending = NULL;
    _bbd._hwndViewPending = NULL;
    _bbd._pidlPending = NULL;

    //
    // Note that we call _MayPlayTransition after we emptied _bbd._psvPending.
    // It will ensure that we don't re-enter this function as a result
    // of dispatched messsages from within _MayPlayTransition.
    //
    if (_ptrsite)
    {
        if (
            !(wNavTypeFlags & NAVTYPE_ShellNavigate)
            &&
            (wNavTypeFlags & NAVTYPE_PageIsChanging)
            &&
            (_bbd._psv && psvNew)
            )
        {
            _MayPlayTransition(psvNew, hwndViewNew, (wNavTypeFlags & NAVTYPE_SiteIsChanging));
        }
        else
        {
            // Make sure we update the navigation status for transitions
            // Don't update the event list for intra-page anchor href's.
            if (wNavTypeFlags & NAVTYPE_PageIsChanging)
                _ptrsite->_UpdateEventList();
        }
    }

    // Quickly check tracking prefix string on this page,
    // if turned on, log enter/exit events
    // BUGBUG: Should use IsBrowserFrameOptionsSet(_bbd._psf, BFO_ENABLE_HYPERLINK_TRACKING)
    //     instead of IsURLChild() because it doesn't work in Folder Shortcuts and doesn't
    //     work in NSEs outside of the "IE" name space (like Web Folders).
    if ((_bbd._pidlCur && IsURLChild(_bbd._pidlCur, FALSE)) ||
        (pidlNew && IsURLChild(pidlNew, FALSE)))
        _MayTrackClickStream(pidlNew);

    // nuke the old stuff
    _pbsOuter->ReleaseShellView();
    
    ASSERT(!_bbd._psv && !_bbd._psf && !_bbd._hwndView && !_bbd._pidlCur);

    // activate the new stuff
    if (_grfHLNFPending != (DWORD)-1) {
        _OnNavigateComplete(pidlNew, _grfHLNFPending);
    }

    VALIDATEPENDINGSTATE();

    // now do the actual switch

    // no need to addref because we're keeping the pointer and just chaning
    // it from the pending to the current member variables
    _bbd._psf = psfNew;
    _bbd._psv = psvNew; 
    _bbd._pidlCur = pidlNew;
    _bbd._hwndView = hwndViewNew;
    _dwReadyStateCur = _dwReadyStatePending;

    if (_bbd._pszTitleCur)
        LocalFree(_bbd._pszTitleCur);
    _bbd._pszTitleCur = _bbd._pszTitlePending;
    _bbd._pszTitlePending = NULL;

    if (_eSecureLockIconPending != SECURELOCK_NOCHANGE)
    {
        _bbd._eSecureLockIcon = _eSecureLockIconPending;
        _eSecureLockIconPending = SECURELOCK_NOCHANGE;
    }

    //
    //  This is the best time to resize the newone.
    //
    _pbsOuter->_UpdateViewRectSize();
    SetWindowPos(_bbd._hwndView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    // WARNING: Not all shellview supports IOleCommandTarget!!!
    _fUsesPaletteCommands = FALSE;
    
    if ( _bbd._psv )
    {
        _bbd._psv->QueryInterface(IID_IOleCommandTarget, (void **)&_bbd._pctView);

        // PALETTE: Exec down to see if they support the colors changes Command so that we don't have to 
        // PALETTE: wire ourselves into the OnViewChange mechanism just to get palette changes...
        if ( _bbd._pctView && 
             SUCCEEDED(_bbd._pctView->Exec( &CGID_ShellDocView, SHDVID_CANDOCOLORSCHANGE, 0, NULL, NULL)))
        {
            _fUsesPaletteCommands = TRUE;

            // force a colors dirty to make sure that we check for a new palette for each page...
            _ColorsDirty( BPT_UnknownPalette );
        }
    }

    // PALETTE: only register for the OnViewChange stuff if the above exec failed...
    if (SUCCEEDED(_bbd._psv->QueryInterface(IID_IViewObject, (void**)&_pvo)) && !_fUsesPaletteCommands )
        _pvo->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, this);

    _Exec_psbMixedZone();

    if (_bbd._pctView != NULL)
    {
        _bbd._pctView->Exec(&CGID_ShellDocView, SHDVID_RESETSTATUSBAR, 0, NULL, NULL);
    }

    return S_OK;
}


//
// This member is called when we about to destroy the current shell view.
// Returning S_FALSE indicate that the user hit CANCEL when it is prompted
// to save the changes (if any).
//
HRESULT CBaseBrowser2::_MaySaveChanges(void)
{
    HRESULT hres = S_OK;
    if (_bbd._pctView) // we must check!
    {
        hres = _bbd._pctView->Exec(&CGID_Explorer, SBCMDID_MAYSAVECHANGES,
                            OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
    }
    return hres;
}

HRESULT CBaseBrowser2::_DisableModeless(void)
{
    if (_cRefCannotNavigate == 0)
    {
        OLECMD rgCmd;
        BOOL fPendingInScript = FALSE;

        //  if pending shell view supports it, give it a chance to tell us it's not ready
        //  to deactivate [eg executing a script].  normally scripts should not be run
        //  before inplace activation, but TRIDENT sometimes has to do this when parsing.
        //
        rgCmd.cmdID = SHDVID_CANDEACTIVATENOW;
        rgCmd.cmdf = 0;

        if (SUCCEEDED(IUnknown_QueryStatus(_bbd._psvPending, &CGID_ShellDocView, 1, &rgCmd, NULL)) &&
            (rgCmd.cmdf & MSOCMDF_SUPPORTED) &&
            !(rgCmd.cmdf & MSOCMDF_ENABLED))
        {
            fPendingInScript = TRUE;
        }

        if (!fPendingInScript) 
        {
            return S_FALSE;
        }
    }
    return S_OK;
}

BOOL CBaseBrowser2::_CanNavigate(void)
{
    return !((_DisableModeless() == S_OK) || (! IsWindowEnabled(_bbd._hwnd)));
}

HRESULT CBaseBrowser2::CanNavigateNow(void)
{
    return _CanNavigate() ? S_OK : S_FALSE;
}

HRESULT CBaseBrowser2::_PauseOrResumeView(BOOL fPaused)
{
    //
    // If fPaused (it's minimized or the parent is minimized) or
    // _bbd._psvPending is non-NULL, we need to pause.
    //
    if (_bbd._pctView) {
        VARIANT var = { 0 };
        var.vt = VT_I4;
        var.lVal = (_bbd._psvPending || fPaused) ? FALSE : TRUE;
        _bbd._pctView->Exec(NULL, OLECMDID_ENABLE_INTERACTION, OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
    }
    return S_OK;
}

HRESULT CBaseBrowser2::CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd)
{
    _fCreateViewWindowPending = TRUE;
    _pbsOuter->GetFolderSetData(&(_fldBase._fld)); // it's okay to stomp on this every time
    HRESULT hres = FileCabinet_CreateViewWindow2(_psbOuter, &_fldBase, psvNew, psvOld, prcView, phwnd);
    _fCreateViewWindowPending = FALSE;
    return hres;
}


//
// grfHLNF == (DWORD)-1 means don't touch the history at all.
//
// NOTE:
// if _fCreateViewWindowPending == TRUE, it means we came through here once
// already, but we are activating a synchronous view and the previous view would
// not deactivate immediately...
// It is used to delay calling IShellView::CreateViewWindow() for shell views until we know that
// we can substitute psvNew for _bbd._psv.
//
HRESULT CBaseBrowser2::_CreateNewShellView(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    BOOL fActivatePendingView = FALSE;
    IShellView *psvNew = NULL;

    // Bail Out of Navigation if modal windows are up from our view
    //BUGBUG Should we restart navigation on next EnableModeless(TRUE)?
    if (!_CanNavigate())
    {
        TraceMsg(DM_ENABLEMODELESS, "CSB::_CreateNewShellView returning ERROR_BUSY");
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
        
    HRESULT hres = _MaySaveChanges();
    if (hres == S_FALSE)
    {
        TraceMsg(DM_WARNING, "CBB::_CreateNewShellView _MaySaveChanges returned S_FALSE. Navigation canceled");
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    VALIDATEPENDINGSTATE();

    _CancelPendingView();
    ASSERT (_fCreateViewWindowPending == FALSE );

    VALIDATEPENDINGSTATE();

    hres = psf->CreateViewObject(_bbd._hwnd, IID_IShellView, (void **)&psvNew);
    if (SUCCEEDED(hres))
    {
        _bbd._fCreatingViewWindow = TRUE;

        _psbOuter->EnableModelessSB(FALSE);
    
        HWND hwndViewNew = NULL;
        RECT rcView;
        //
        // NOTES: SatoNa
        //
        //  Notice that we explicitly call _GetViewBorderRect (non-virtual)
        // instead of virtual _GetShellView, which CShellBrowser override.
        // BUGBUG split: we now call thru (virtual) _pbsOuter, is this o.k.?
        //
        _pbsOuter->_GetViewBorderRect(&rcView);

        ASSERT(_bbd._psvPending == NULL );


        // It is ncecessary for _bbd._pidlPending and _bbd._psvPending to both be set together.
        // they're  a pair
        // previously _bbd._pidlPending was being set after the call to
        // FileCabinet_CreateViewWindow and when messages were pumped there
        // a redirect would be nofied in the bind status callback.
        // this meant that a valid _bbd._pidlPending was actually available BUT
        // then we would return and blow away that _bbd._pidlPending
        
        _bbd._psvPending = psvNew;
        ASSERT(_bbd._psfPending == NULL);
        ASSERT(_bbd._pidlPending == NULL);
        psvNew->AddRef();
        _bbd._psfPending = psf;
        psf->AddRef();
        _bbd._pidlPending = ILClone(pidl);

            
        //Initialize _bbd._pidlNewShellView which will be used by GetViewStateStream
        _bbd._pidlNewShellView = pidl;
        _grfHLNFPending = grfHLNF;
        
        // Start at _COMPLETE just in case the object we connect
        // to doesn't notify us of ReadyState changes
        //
        _dwReadyStatePending = READYSTATE_COMPLETE;


        hres = _pbsOuter->CreateViewWindow(psvNew, _bbd._psv, &rcView, &hwndViewNew);

        _bbd._pidlNewShellView = NULL;

        TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_CreateNewShellView(3) Called CreateViewWindow"), psvNew, hres);

        if (SUCCEEDED(hres))
        {
            // we defer the _PauseOrResumeView until here when we have enough
            // info to know if it's a new page or not.  o.w. we end up (e.g.)
            // stopping bgsounds etc. on local links (nash:32270).
            _PauseOrResumeView(_fPausedByParent);
            
            // we stop the current view because we need to flush away any image stuff that
            // is in queue so that the actual html file can get downloaded
            _StopCurrentView();
            
                
            
            
            _bbd._hwndViewPending = hwndViewNew;

            /// BEGIN-CHC- Security fix for viewing non shdocvw ishellviews
            _CheckDisableViewWindow();
            /// END-CHC- Security fix for viewing non shdocvw ishellviews
        
            // BUGBUG chrisfra - if hres == S_FALSE this (calling ActivatePendingViewAsync
            // when _bbd._psv==NULL) will break async URL download
            // as it will cause _bbd._psvPending to be set to NULL prematurely.  this should
            // be deferred until CDocObjectView::CBindStatusCallback::OnObjectAvailable
            //if (hres==S_OK || _bbd._psv==NULL)
            
            ASSERT(( hres == S_OK ) || ( hres == S_FALSE ));
            
            if (hres==S_OK)
            {
                // We should activate synchronously.
                //
                // NOTE: This used to be ActivatePendingViewAsyc(), but that causes a
                // fault if you navigated to C:\ and click A:\ as soon as it appears. This
                // puts the WM_LBUTTONDOWN in FRONT of the WMC_ASYNCOPERATION message. If
                // there's no disk in drive A: then a message box appears while in the
                // middle of the above FileCabinet_CreateViewWindow call and we pull off
                // the async activate and activate the view we're in the middle of
                // creating! Don't do that.
                //
                fActivatePendingView = TRUE;
            }
            else
            {
                // Activation is pending.
                // since the back button works as a stop on pending navigations, we
                // should check that here as well.
                _pbsOuter->UpdateBackForwardState();
            }
        }
        else
        {
            TraceMsg(DM_WARNING, "ief _CreateNewShellView psvNew->CreateViewWindow failed %x", hres);
            _CancelPendingView();
        }
        psvNew->Release();

        _psbOuter->EnableModelessSB(TRUE);
    }
    else
    {
        TraceMsg(TF_WARNING, "ief _BrowseTo psf->CreateViewObject failed %x", hres);
    }

    //
    //  If there is any blocked async operation AND we can navigate now,
    // unblock it now. 
    //
    _MayUnblockAsyncOperation();

    _bbd._fCreatingViewWindow = FALSE;

    VALIDATEPENDINGSTATE();

    if (fActivatePendingView)
    {
        _PreActivatePendingViewAsync(); // so we match old code
        hres = _pbsOuter->ActivatePendingView();
        if (FAILED(hres))
            TraceMsg(DM_WARNING, "CBB::_CNSV ActivatePendingView failed");
    }

    TraceMsg(DM_STARTUP, "ief _CreateNewShellView returning %x", hres);
    return hres;
}

//  private bind that is very loose in its bind semantics.
HRESULT IEBindToObjectForNavigate(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsfOut);

// this binds to the pidl folder then hands off to CreateNewShellView
// if you have anything you need to do like checking before we allow the navigate, it
// should go into _NavigateToPidl
HRESULT CBaseBrowser2::_CreateNewShellViewPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD fSBSP)
{
    SetNavigateState(BNS_BEGIN_NAVIGATE);

    TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_CreateNewShellViewPidl not same pidl"), pidl, _bbd._pidlCur);

    // BUGBUG: Check for URL-pidl

    // We will allow UI to be displayed by passing this IBindCtx to IShellFolder::BindToObject().
    IBindCtx * pbc = NULL;
    IShellFolder* psf;
    HRESULT hres;

    // shell32 v4 (IE4 w/ActiveDesktop) had a bug that juntion points from the
    // desktop didn't work if a IBindCtx is passed and causes crashing on debug.
    // The fix is to not pass bindctx on that shell32.  We normally want to pass
    // an IBindCtx so shell extensions can enable the browser modal during UI.
    // The Internet name space does this when InstallOnDemand(TM) installs delegate
    // handles, like FTP.  A junction point is a folder that contains a desktop.ini
    // that gives the CLSID of the shell extension to use for the Shell Extension.
    if (4 != GetUIVersion())
    {
        pbc = CreateBindCtxForUI(SAFECAST(this, IShellBrowser*));    // I'm safecasting to IUnknown.  IShellBrowser is only for disambiguation.
    }
    hres = IEBindToObjectForNavigate(pidl, pbc, &psf);   // If pbc is NULL, we will survive.

    if (SUCCEEDED(hres))
    {
        hres = _CreateNewShellView(psf, pidl, grfHLNF);
        TraceMsg(DM_STARTUP, "CSB::_CreateNewShellViewPidl _CreateNewShellView(3) returned %x", hres);
        psf->Release();
    }
    else
    {
        // This will happen when a user tries to navigate to a directory past
        // MAX_PATH by double clicking on a subdirectory in the shell.
        TraceMsg(DM_TRACE, "CSB::_CreateNSVP BindToOject failed %x", hres);
    }

    // If _CreateNewShellView (or IEBindToObject) fails or the user cancels
    // the MayOpen dialog (hres==S_FALSE), we should restore the navigation
    // state to NORMAL (to stop animation). 
    if (FAILED(hres))
    {
        TraceMsg(TF_SHDNAVIGATE, "CSB::_CreateNSVP _CreateNewShellView FAILED (%x). SetNavigateState to NORMAL", hres);
        SetNavigateState(BNS_NORMAL);
    }

    ATOMICRELEASE(pbc);
    TraceMsg(DM_STARTUP, "CSB::_CreateNewShellViewPidl returning %x", hres);
    return hres;
}

//
// Returns the border rectangle for the shell view.
//
HRESULT CBaseBrowser2::_GetViewBorderRect(RECT* prc)
{
    _pbsOuter->_GetEffectiveClientArea(prc, NULL);  // BUGBUG hmon?
    // (derived class subtracts off border taken by all "frame" toolbars)
    return S_OK;
}

//
// Returns the window rectangle for the shell view window.
//
HRESULT CBaseBrowser2::GetViewRect(RECT* prc)
{
    //
    // By default (when _rcBorderDoc is empty), ShellView's window
    // rectangle is the same as its border rectangle.
    //
    _pbsOuter->_GetViewBorderRect(prc);

    // Subtract document toolbar margin
    prc->left += _rcBorderDoc.left;
    prc->top += _rcBorderDoc.top;
    prc->right -= _rcBorderDoc.right;
    prc->bottom -= _rcBorderDoc.bottom;

    TraceMsg(DM_UIWINDOW, "ief GetViewRect _rcBorderDoc=%x,%x,%x,%x",
             _rcBorderDoc.left, _rcBorderDoc.top, _rcBorderDoc.right, _rcBorderDoc.bottom);
    TraceMsg(DM_UIWINDOW, "ief GetViewRect prc=%x,%x,%x,%x",
             prc->left, prc->top, prc->right, prc->bottom);

    return S_OK;
}

void CBaseBrowser2::_PositionViewWindow(HWND hwnd, LPRECT prc)
{
    SetWindowPos(hwnd, NULL,
                 prc->left, prc->top, 
                 prc->right - prc->left, 
                 prc->bottom - prc->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

HRESULT CBaseBrowser2::_UpdateViewRectSize(void)
{
    RECT rc;

    TraceMsg(TF_SHDUIACTIVATE, "CSB::_UpdateViewRectSize called when _fDontReszeView=%d, _bbd._hwndV=%x, _bbd._hwndVP=%x",
             _fDontResizeView, _bbd._hwndView, _bbd._hwndViewPending);

    _pbsOuter->GetViewRect(&rc);

    if (_bbd._hwndView && !_fDontResizeView) {   
        TraceMsg(TF_SHDUIACTIVATE, "CSB::_UpdateViewRectSize resizing _bbd._hwndView(%x)", _bbd._hwndView);
        _PositionViewWindow(_bbd._hwndView, &rc);
    }

    if (_bbd._hwndViewPending) {
        TraceMsg(TF_SHDUIACTIVATE, "CSB::_UpdateViewRectSize resizing _bbd._hwndViewPending(%x)", _bbd._hwndViewPending);
        _PositionViewWindow(_bbd._hwndViewPending, &rc);
    }
    return S_OK;
}

UINT g_idMsgGetAuto = 0;

// this stays in shdocvw because the OC requires drop target registration
void CBaseBrowser2::_RegisterAsDropTarget()
{
    // if it's okay to register and we haven't registered already
    // and we've processed WM_CREATE
    if (!_fNoDragDrop && !_fRegisteredDragDrop && _bbd._hwnd)
    {
        BOOL fAttemptRegister = _fTopBrowser ? TRUE : FALSE;

        // if we're not toplevel, we still try to register
        // if we have a proxy browser
        if (!fAttemptRegister)
        {
            IShellBrowser* psb;
            HRESULT hres = _pspOuter->QueryService(SID_SProxyBrowser, IID_IShellBrowser, (void **)&psb);
            if (SUCCEEDED(hres)) {
                fAttemptRegister = TRUE;
                psb->Release();
            }
        }

        if (fAttemptRegister)
        {
            HRESULT hr;
            IDropTarget *pdt;

            // SAFECAST(this, IDropTarget*), the hard way
            hr = THR(QueryInterface(IID_IDropTarget, (void **)&pdt));
            if (SUCCEEDED(hr)) 
            {
                hr = THR(RegisterDragDrop(_bbd._hwnd, pdt));
                if (SUCCEEDED(hr)) {
                    _fRegisteredDragDrop = TRUE;
                }
                pdt->Release();
            }
        }
    }
}

void CBaseBrowser2::_UnregisterAsDropTarget()
{
    if (_fRegisteredDragDrop)
    {
        _fRegisteredDragDrop = FALSE;
        
        THR(RevokeDragDrop(_bbd._hwnd));
    }
}


HRESULT CBaseBrowser2::OnCreate(LPCREATESTRUCT pcs)
{
    HRESULT hres;
    TraceMsg(DM_STARTUP, "_OnCreate called");

    if (g_idMsgGetAuto == 0)
        g_idMsgGetAuto = RegisterWindowMessage(TEXT("GetAutomationObject"));

    hres = InitPSFInternet();

    // do stuff that depends on window creation
    if (SUCCEEDED(hres))
    {
        // this must be done AFTER the ctor so that we get virtuals right
        // NOTE: only do this if we're actually creating the window, because
        //       the only time we SetOwner(NULL) is OnDestroy.
        //
        _bbd._pautoSS->SetOwner(SAFECAST(this, IShellBrowser*));
    
        _RegisterAsDropTarget();
    }

    TraceMsg(DM_STARTUP, "ief OnCreate returning %d (SUCCEEDED(%x))", SUCCEEDED(hres), hres);

    return SUCCEEDED(hres) ? S_OK : E_FAIL;
}

HRESULT CBaseBrowser2::OnDestroy()
{
    //  We're seeing some reentrancy here.  If _cRefCannotNavigate is non-zero, it means we're
    //  in the middle of something and shouldn't destroy ourselves.
    //

    //  Also check reentrant calls to OnDestroy().
    if(_fInDestroy)
    {
        // Already being destroyed -- bail out.
        return S_OK;
    }

    _fInDestroy = TRUE;

    if (_cRefUIActivateSV)
    {
        TraceMsg(TF_WARNING, 
            "CBB(%x)::OnDestroy _cRefUIActivateSV(%d)!=0", 
            this, _cRefUIActivateSV);

        // I need to defer my self-destruction.
        //
        _fDeferredSelfDestruction = TRUE;
        return S_OK;
    }

    _KillRefreshTimer();

    _CancelPendingView();
    _pbsOuter->ReleaseShellView();
    
    AssertMsg(S_FALSE == _DisableModeless(),
              TEXT("CBB::OnDestroy _cRefCannotNavigate!=0 (%d)"),
              _cRefCannotNavigate);

    ATOMICRELEASE(_bbd._ptl);

    // This should always be successful because the IDropTarget is registered 
    // in _OnCreate() and is the default one.
    // _pdtView should have already been released in ReleaseShellView
    ASSERT(_pdtView == NULL);

    _UnregisterAsDropTarget();

    //
    //  It is very important to call _bbd._pauto->SetOwner(NULL) here, which will
    // remove any reference from the automation object to us. Before doing
    // it, we always has cycled references and we never be released.
    //
    _bbd._pautoSS->SetOwner(NULL);

    _bbd._hwnd = NULL;

#ifdef DEBUG
    _fProcessed_WM_CLOSE = TRUE;
#endif
    _DLMDestroy();
    IUnknown_SetSite(_pToolbarExt,NULL); // destroy the toolbar extensions

    return S_OK;
}

HRESULT CBaseBrowser2::NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    HRESULT hr = S_OK;
    _KillRefreshTimer();  // BUGBUG: restart if nav fails

    LPITEMIDLIST pidlNew = (LPITEMIDLIST)pidl;

    //
    //  BUGBUGTODO - need to handle going back to an outside app - zekel 7MAY97
    //  i have dumped the code that did this, so now i need to put it
    //  into the CTravelLog implementation, so that it will be done properly
    //  without us.  but it shouldnt be done here regardless.
    //

    //  BUGBUGREMOVE - with the old Travellog code
    // special case hack for telling us to use the local history, not
    // the global history
    if (pidl && pidl != PIDL_LOCALHISTORY)
        pidlNew = ILClone(pidl);

    //
    // Fortunately the only callers of NavigateToPidl use HLNF_NAVIGATINGBACK/FORWARD
    // so that's the only mapping we need to do here.
    //
    DWORD dwSBSP = 0;
    if (grfHLNF != (DWORD)-1)
    {
        if (grfHLNF & SHHLNF_WRITENOHISTORY)
            dwSBSP |= SBSP_WRITENOHISTORY;
        if (grfHLNF & SHHLNF_NOAUTOSELECT)
            dwSBSP |= SBSP_NOAUTOSELECT;
    }
    if (grfHLNF & HLNF_NAVIGATINGBACK)
        dwSBSP = SBSP_NAVIGATEBACK;
    else if (grfHLNF & HLNF_NAVIGATINGFORWARD)
        dwSBSP = SBSP_NAVIGATEFORWARD;

    if (dwSBSP)
    {
        hr = _psbOuter->BrowseObject(pidlNew, dwSBSP);  // browse will do the nav here.
        ILFree(pidlNew);
    }
    else
        _NavigateToPidlAsync(pidlNew, dwSBSP, FALSE);  // takes ownership of the pidl
    
    return hr;
}

// BUGBUG: Bad-hack
#define HLNF_NAVIGATINGFORWARDEND (HLNF_NAVIGATINGFORWARD | 0x1000)

// S_OK means we found at least one valid connection point
//
HRESULT GetWBConnectionPoints(IUnknown* punk, IConnectionPoint **ppcp1, IConnectionPoint **ppcp2)
{
    HRESULT           hres = E_FAIL;
    IExpDispSupport*  peds;
    CConnectionPoint* pccp1 = NULL;
    CConnectionPoint* pccp2 = NULL;
    
    if (ppcp1)
        *ppcp1 = NULL;
    if (ppcp2)
        *ppcp2 = NULL;

    if (punk && SUCCEEDED(punk->QueryInterface(IID_IExpDispSupport, (void **)&peds)))
    {
        if (ppcp1 && SUCCEEDED(peds->FindCIE4ConnectionPoint(DIID_DWebBrowserEvents,
                                                reinterpret_cast<CIE4ConnectionPoint**>(&pccp1))))
        {
            *ppcp1 = pccp1->CastToIConnectionPoint();
            hres = S_OK;
        }

        if (ppcp2 && SUCCEEDED(peds->FindCIE4ConnectionPoint(DIID_DWebBrowserEvents2,
                                                reinterpret_cast<CIE4ConnectionPoint**>(&pccp2))))
        {
            *ppcp2 = pccp2->CastToIConnectionPoint();
            hres = S_OK;
        }
            
        peds->Release();
    }

    return hres;
}

void CBaseBrowser2::_UpdateBackForwardState()
{
    if (_fTopBrowser && !_fNoTopLevelBrowser) {
        IConnectionPoint *pccp1;
        IConnectionPoint *pccp2;

        if (S_OK == GetWBConnectionPoints(_bbd._pautoEDS, &pccp1, &pccp2))
        {
            HRESULT hresT;
            VARIANTARG va[2];
            DISPPARAMS dp;
            ITravelLog *ptl;

            GetTravelLog(&ptl);

            // if we've got a site or if we're trying to get to a site,
            // enable the back button
            BOOL fEnable = (ptl ? S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_BACK, NULL) : FALSE);
                
            VARIANT_BOOL bEnable = fEnable ? VARIANT_TRUE : VARIANT_FALSE;
            TraceMsg(TF_TRAVELLOG, "CBB::UpdateBackForward BACK = %d", fEnable);

            // We use SHPackDispParams once instead of calling DoInvokeParams multiple times...
            //
            hresT = SHPackDispParams(&dp, va, 2, VT_I4, CSC_NAVIGATEBACK, VT_BOOL, bEnable);
            ASSERT(S_OK==hresT);

            // Removed the following EnableModelessSB(FALSE) because VB5 won't run the event handler if
            // we're modal.
            // _psbOuter->EnableModelessSB(FALSE);

            IConnectionPoint_SimpleInvoke(pccp1, DISPID_COMMANDSTATECHANGE, &dp);
            IConnectionPoint_SimpleInvoke(pccp2, DISPID_COMMANDSTATECHANGE, &dp);

            fEnable = (ptl ? S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_FORE, NULL) : FALSE);
            bEnable = fEnable ? VARIANT_TRUE : VARIANT_FALSE;
            TraceMsg(TF_TRAVELLOG, "CBB::UpdateBackForward FORE = %d", fEnable);

            ATOMICRELEASE(ptl);
            // We know how SHPackDispParams fills in va[]
            ASSERT(VT_BOOL == va[0].vt);
            va[0].boolVal = bEnable;
            ASSERT(VT_I4 == va[1].vt);
            va[1].lVal = CSC_NAVIGATEFORWARD;

            IConnectionPoint_SimpleInvoke(pccp1, DISPID_COMMANDSTATECHANGE, &dp);
            IConnectionPoint_SimpleInvoke(pccp2, DISPID_COMMANDSTATECHANGE, &dp);
            ATOMICRELEASE(pccp1);
            ATOMICRELEASE(pccp2);

            // Removed the following _psbOuter->EnableModelessSB(TRUE) because VB5 won't run the event handler if
            // we're modal.
            // _psbOuter->EnableModelessSB(TRUE);
        }
    }
}

void CBaseBrowser2::_NotifyCommandStateChange()
{
    HRESULT hr;

    // I'm only firing these in the toplevel case
    // Why? Who cares about the frameset case
    // since nobody listens to these events on
    // the frameset.
    //
    if (_fTopBrowser && !_fNoTopLevelBrowser) {
        IConnectionPoint *pccp1;
        IConnectionPoint *pccp2;

        if (S_OK == GetWBConnectionPoints(_bbd._pautoEDS, &pccp1, &pccp2))
        {
            ASSERT(pccp1 || pccp2); // Should've gotten at least one

            VARIANTARG args[2];
            DISPPARAMS dp;
            hr = SHPackDispParams(&dp, args, 2,
                                  VT_I4,   CSC_UPDATECOMMANDS,
                                  VT_BOOL, FALSE);

            IConnectionPoint_SimpleInvoke(pccp1, DISPID_COMMANDSTATECHANGE, &dp);
            IConnectionPoint_SimpleInvoke(pccp2, DISPID_COMMANDSTATECHANGE, &dp);
            ATOMICRELEASE(pccp1);
            ATOMICRELEASE(pccp2);
        }
    }
}


LRESULT CBaseBrowser2::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    HWND hwndControl = GET_WM_COMMAND_HWND(wParam, lParam);
    if (IsInRange(idCmd, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
    {
        if (_bbd._hwndView)
            SendMessage(_bbd._hwndView, WM_COMMAND, wParam, lParam);
        else
            TraceMsg(0, "view cmd id with NULL view");

        /// REVIEW - how can we get FCIDM_FAVORITECMD... range if we're NOT toplevelapp?
        /// REVIEW - should RecentOnCommand be done this way too?

    }
    
    return S_OK;
}


LRESULT CBaseBrowser2::OnNotify(LPNMHDR pnm)
{
    // the id is from the view, probably one of the toolbar items

    if (IsInRange(pnm->idFrom, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
    {
        if (_bbd._hwndView)
            SendMessage(_bbd._hwndView, WM_NOTIFY, pnm->idFrom, (LPARAM)pnm);
    }
    return 0;
}

HRESULT CBaseBrowser2::OnSetFocus()
{
#if 0 // BUGBUG split: would be nice to assert
    ASSERT(_get_itbLastFocus() == ITB_VIEW);
#endif

    if (_bbd._hwndView) {
        SetFocus(_bbd._hwndView);
    } 
    return 0;
}

#define ABOUT_HOME L"about:home"
// This function is VERY focused on achieving what the
// caller wants.  That's why it has a very specific
// meaning to the return value.
BOOL IsAboutHomeOrNonAboutURL(LPITEMIDLIST pidl)
{
    BOOL fIsAboutHomeOrNonAboutURL = TRUE;
    WCHAR wzCur[MAX_URL_STRING];

    if (pidl && SUCCEEDED(IEGetDisplayName(pidl, wzCur, SHGDN_FORPARSING)))
    {        
        // Is it "about:home"?
        if (0 != StrCmpNICW(ABOUT_HOME, wzCur, ARRAYSIZE(ABOUT_HOME) - 1))
        {
            // No.  We also want to return TRUE if the scheme was NOT an ABOUT URL.
            fIsAboutHomeOrNonAboutURL = (URL_SCHEME_ABOUT != GetUrlSchemeW(wzCur));
        }
    }

    return fIsAboutHomeOrNonAboutURL;            
}

//
// This function activate the pending view synchronously.
//
HRESULT
CBaseBrowser2::ActivatePendingView(void)
{
    HRESULT hres = E_FAIL;
    BOOL bHadFocus;

    TraceMsg(TF_SHDNAVIGATE, "CBB::ActivatePendingView called");

    ASSERT(_bbd._psvPending);
    ASSERT(_bbd._psfPending);

#ifdef FEATURE_PICS
    if (S_FALSE == IUnknown_Exec(_bbd._psvPending, &CGID_ShellDocView, SHDVID_CANACTIVATENOW, NULL, NULL, NULL))
    {
        hres = S_OK;    // still waiting . . . but no failure.
        goto DoneWait;
    }
#endif
    
    //  if we are in modal loop, don't activate now
    if (_cRefCannotNavigate > 0)
        goto Done;

    // if _cRefCannotNavigate > 0 it is possible that _hwndViewPending has not been created so this assert 
    // should go after the check above
    ASSERT(_bbd._hwndViewPending);
    
    //  if shell view supports it, give it a chance to tell us it's not ready
    //  to deactivate [eg executing a script]
    //
    OLECMD rgCmd;
    rgCmd.cmdID = SHDVID_CANDEACTIVATENOW;
    rgCmd.cmdf = 0;
    if (_bbd._pctView &&
        SUCCEEDED(_bbd._pctView->QueryStatus(&CGID_ShellDocView, 1, &rgCmd, NULL)) &&
        (rgCmd.cmdf & MSOCMDF_SUPPORTED) &&
        !(rgCmd.cmdf & MSOCMDF_ENABLED)) {
        //
        //  The DocObject that reported MSOCMDF_SUPPORTED must send
        //  SHDVID_DEACTIVATEMENOW when we're out of scripts or whatever so that
        //  we retry the activate
        //
        TraceMsg(DM_WARNING, "CBB::ActivatePendingView DocObject says I can't deactivate it now");
        goto Done;
    }

    ASSERT(_bbd._psvPending);

    // Prevent any navigation while we have the pointers swapped and we're in
    // delicate state
    _psbOuter->EnableModelessSB(FALSE);

    //
    // Don't play sound for the first navigation (to avoid multiple
    // sounds to be played for a frame-set creation).
    //
    if (_bbd._psv && IsWindowVisible(_bbd._hwnd)) {
        IEPlaySound(TEXT("ActivatingDocument"), FALSE);
    }

    ASSERT(_bbd._psvPending);

    //  NOTE: if there are any other protocols that need to not be in 
    //  the travel log, it should probably implemented through UrlIs(URLIS_NOTRAVELLOG)
    //  right now, About: is the only one we care about
    if (!(_grfHLNFPending & HLNF_CREATENOHISTORY) && 
        IsAboutHomeOrNonAboutURL(_bbd._pidlCur))
     {
        _UpdateTravelLog();
    }

    //  WARNING - these will only fail if the UpdateTravelLog() - zekel - 7-AUG-97
    //  was skipped and these bits are set.

    //  alanau 5-may-98 -- I still hit this assert on a script-based navigate to the same page.
    //      iedisp.cpp sees StrCmpW("about:blank","about:blank?http://www.microsoft.com/ie/ie40/gallery/_main.htm") 
    //      (for example), but basesb.cpp sees two equal pidls (both "about:blank?http://...").
    //      Killing this assert.
    // ASSERT(!_fDontAddTravelEntry);
    ASSERT(!_fIsLocalAnchor);

    // before we destroy the window check if it or any of its childern has focus
    bHadFocus =     _bbd._hwndView && (IsChildOrSelf(_bbd._hwndView, GetFocus()) == S_OK)
                ||  _bbd._hwndViewPending && (IsChildOrSelf(_bbd._hwndViewPending, GetFocus()) == S_OK);

    _pbsOuter->_SwitchActivationNow();

    _psbOuter->EnableModelessSB(TRUE);

    _StartRefreshTimer();   /* If client pull on the new guy, start the countdown */

    TraceMsg(DM_NAV, "CBaseBrowser2(%x)::ActivatePendingView(%x)", this, _bbd._psv);

    // if some other app has focus, then don't uiactivate this navigate
    // or we'll steal focus away. we'll uiactivate when we next get activated
    //
    // ie4.01, bug#64630 and 64329
    // _fActive only gets set by WM_ACTIVATE on the TopBrowser.  so for subframes
    // we always defer setting the focus if they didnt have focus before navigation.
    // the parent frame should set the subframe as necessary when it gets 
    //  its UIActivate.   - ReljaI 4-NOV-97
    if (SVUIA_ACTIVATE_FOCUS == _bbd._uActivateState && !(_fActive || bHadFocus))
    {
        _bbd._uActivateState = SVUIA_INPLACEACTIVATE;
        _fUIActivateOnActive = TRUE;
    }
    
    _UIActivateView(_bbd._uActivateState);

    // Tell the shell's HTML window we have a new document.
    if( _phtmlWS )
        _phtmlWS->ViewActivated( );

    // this matches the _bbd._psvPending = NULL above.
    // we don't put this right beside there because the
    // _SwitchActivationNow could take some time, as well as the DoInvokePidl

    SetNavigateState(BNS_NORMAL);

    _pbsOuter->UpdateBackForwardState();
    _NotifyCommandStateChange();

    if (!_fNoDragDrop && _fTopBrowser) {
        ASSERT(_bbd._psv);
        // _SwitchActivationNow should have already released the old _pdtView and set it to NULL
        ASSERT(_pdtView == NULL);
        _bbd._psv->QueryInterface(IID_IDropTarget, (void **)&_pdtView);
    }

    // The pending view may have a title change stored up, so fire the TitleChange.
    // Also the pending view may not tell us about title changes, so simulate one.
    //
    if (_bbd._pszTitleCur)
    {
        FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, _bbd._pszTitleCur);
    }
    else if (_bbd._pidlCur)
    {
        WCHAR wzFullName[MAX_URL_STRING];

        hres = ::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_NORMAL, wzFullName, SIZECHARS(wzFullName), NULL);
        if (EVAL(SUCCEEDED(hres)))
            FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, wzFullName);
    }

    // We must fire this event LAST because the app can shut us down
    // in response to this event.
    FireEvent_NavigateComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur, _bbd._hwnd);

    hres = S_OK;

Done:
    OnReadyStateChange(NULL, READYSTATE_COMPLETE);
DoneWait:
    return hres;
}

LRESULT CBaseBrowser2::_DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // call the UNICODE/ANSI aware DefWindowProc
    //
    return ::SHDefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CBaseBrowser2::_ViewChange(DWORD dwAspect, LONG lindex)
{
    //
    // we are interested in content changes only
    //

    // NOTE: if we are registed for separate palette commands, then do not invalidate the colours here...
    if (dwAspect & DVASPECT_CONTENT && !_fUsesPaletteCommands )
    {
        //
        // recompute our palette
        //
        _ColorsDirty(BPT_UnknownPalette);
    }
    else
    {
        TraceMsg(DM_PALETTE, "cbb::_vc not interested in aspect(s) %08X", dwAspect);
    }
}

void CBaseBrowser2::_ColorsDirty(BrowserPaletteType bptNew)
{
    //
    // if we are not currently handling palette messages then get out
    //
    if (_bptBrowser == BPT_DeferPaletteSupport)
    {
        TraceMsg(DM_PALETTE, "cbb::_cd deferring palette support");
        return;
    }

    //
    // we only handle palette changes and display changes
    //
    if ((bptNew != BPT_UnknownPalette) && (bptNew != BPT_UnknownDisplay))
    {
        AssertMsg(FALSE, TEXT("CBaseBrowser2::_ColorsDirty: invalid BPT_ constant"));
        bptNew = BPT_UnknownPalette;
    }

    //
    // if we aren't on a palettized display we don't care about palette changes
    //
    if ((bptNew != BPT_UnknownDisplay) && (_bptBrowser == BPT_NotPalettized))
    {
        TraceMsg(DM_PALETTE, "cbb::_cd not on palettized display");
        return;
    }

    //
    // if we are already handling one of these then we're done
    //
    if ((_bptBrowser == BPT_PaletteViewChanged) ||
        (_bptBrowser == BPT_DisplayViewChanged))
    {
        TraceMsg(DM_PALETTE, "cbb::_cd coalesced");
        return;
    }

    //
    // unknown display implies unknown palette when the display is palettized
    //
    if (_bptBrowser == BPT_UnknownDisplay)
        bptNew = BPT_UnknownDisplay;

    //
    // post ourselves a WM_QUERYNEWPALETTE so we can pile up multiple advises
    // and handle them at once (we can see a lot of them sometimes...)
    // NOTE: the lParam is -1 so we can tell that WE posted it and that
    // NOTE: it doesn't necessarily mean we onw the foreground palette...
    //
    if (PostMessage(_bbd._hwnd, WM_QUERYNEWPALETTE, 0, (LPARAM) -1))
    {
        TraceMsg(DM_PALETTE, "cbb::_cd queued update");

        //
        // remember that we have already posted a WM_QUERYNEWPALETTE
        //
        _bptBrowser = (bptNew == BPT_UnknownPalette)?
            BPT_PaletteViewChanged : BPT_DisplayViewChanged;
    }
    else
    {
        TraceMsg(DM_PALETTE, "cbb::_cd FAILED!");

        //
        // at least remember that the palette is stale
        //
        _bptBrowser = bptNew;
    }
}

void CBaseBrowser2::v_PropagateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend)
{
    if (_bbd._hwnd)
        PropagateMessage(_bbd._hwnd, uMsg, wParam, lParam, fSend);
}

void CBaseBrowser2::_DisplayChanged(WPARAM wParam, LPARAM lParam)
{
    //
    // forward this on to our children
    //
    v_PropagateMessage(WM_DISPLAYCHANGE, wParam, lParam, TRUE);

    //
    // and mark our colors as dirty
    //
    _ColorsDirty(BPT_UnknownDisplay);
}

// 
// return results for _UpdateBrowserPaletteInPlace()
//    S_OK : BrowserPalette was successfully updated in place
//    S_FALSE : BrowserPalette is exactly the same, no need to update
//    E_FAIL : Unable to update palette in place at all, caller needs to create new palette 
// 
HRESULT CBaseBrowser2::_UpdateBrowserPaletteInPlace(LOGPALETTE *plp)
{
    if (!_hpalBrowser)
        return E_FAIL;

    WORD w;
    if (GetObject(_hpalBrowser, sizeof(w), &w) != sizeof(w))
        return E_FAIL;

    if (w != plp->palNumEntries)
        return E_FAIL;

    if (w > 256)
        return E_FAIL;

    //
    // GDI marks a palette as dirty if you update its colors
    // only replace the entries if the colors are actually different
    // this prevents excessive flashing
    //
    PALETTEENTRY ape[256];

    if (GetPaletteEntries(_hpalBrowser, 0, w, ape) != w)
        return E_FAIL;

    if (memcmp(ape, plp->palPalEntry, w * sizeof(PALETTEENTRY)) == 0)
    {
        TraceMsg(DM_PALETTE, "cbb::_ubpip %08x already had view object's colors", _hpalBrowser);
        return S_FALSE;
    }

    // make sure we don't reuse the global halftone palette that we are reusing across shdocvw....
    // do this after we've done the colour match 
    if ( _hpalBrowser == g_hpalHalftone )
    {
        return E_FAIL;
    }
    
    //
    // actually set up the colors
    //
    if (SetPaletteEntries(_hpalBrowser, 0, plp->palNumEntries,
        plp->palPalEntry) != plp->palNumEntries)
    {
        return E_FAIL;
    }

    TraceMsg(DM_PALETTE, "cbb::_ubpip updated %08x with view object's colors", _hpalBrowser);
    return S_OK;
}

void CBaseBrowser2::_RealizeBrowserPalette(BOOL fBackground)
{
    HPALETTE hpalRealize;

    //
    // get a palette to realize
    //
    if (_hpalBrowser)
    {
        TraceMsg(DM_PALETTE, "cbb::_rbp realizing %08x", _hpalBrowser);
        hpalRealize = _hpalBrowser;
    }
    else
    {
        TraceMsg(DM_PALETTE, "cbb::_rbp realizing DEFAULT_PALETTE");
        hpalRealize = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
    }

    if ( !_fOwnsPalette && !fBackground )
    {
        // NOTE: if we don't think we own the foreground palette, and we
        // NOTE: are being told to realize in the foreground, then ignore
        // NOTE: it because they are wrong...
        fBackground = TRUE;
    }
    
    //
    // get a DC to realize on and select our palette
    //
    HDC hdc = GetDC(_bbd._hwnd);
    HPALETTE hpalOld = SelectPalette(hdc, hpalRealize, fBackground);

    //
    // we don't paint any palettized stuff ourselves we're just a frame
    // eg. we don't need to repaint here if the realize returns nonzero
    //
    RealizePalette(hdc);

    //
    // since we create and delete our palette alot, don't leave it selected
    //
    SelectPalette(hdc, hpalOld, TRUE);
    ReleaseDC(_bbd._hwnd, hdc);
}

void CBaseBrowser2::_PaletteChanged(WPARAM wParam, LPARAM lParam)
{
    TraceMsg(DM_PALETTE, "cbb::_pc (%08X, %08X, %08X) begins -----------------------", this, wParam, lParam);

    //
    // cdturner: 08/03/97
    // we think that we currently own the foregorund palette, we need to make sure that 
    // the window that just realized in the foreground (and thus caused the system
    // to generate the WM_PALETTECHANGED) was us, otherwise, we no longer own the 
    // palette 
    // 
    if ( _fOwnsPalette )
    {
        // by default we do not own it.
        _fOwnsPalette = FALSE;
        
        // the wParam hwnd we get is the top-level window that cause it, so we need to walk the window
        // chain to find out if it is one of our parents...
        // start at _bbd._hwnd (incase we are the top-level :-))
        HWND hwndParent = _bbd._hwnd;
        while ( hwndParent != NULL )
        {
            if ( hwndParent == (HWND) wParam )
            {
                // we caused it, so therefore we must still own it...
                _fOwnsPalette = TRUE;
                break;
            }
            hwndParent = GetParent( hwndParent );
        }
    }
    
    //
    // should we realize now? (see _QueryNewPalette to understand _bptBrowser)
    //
    // NOTE: we realize in the background here on purpose!  This helps us be
    // compatible with Netscape plugins etc that think they can own the
    // palette from inside the browser.
    //
    if (((HWND)wParam != _bbd._hwnd) && (_bptBrowser == BPT_Normal))
        _RealizeBrowserPalette(TRUE);

    //
    // always forward the changes to the current view
    // let the toolbars know too
    //
    if (_bbd._hwndView)
        TraceMsg(DM_PALETTE, "cbb::_pc forwarding to view window %08x", _bbd._hwndView);
    _pbsOuter->_SendChildren(_bbd._hwndView, TRUE, WM_PALETTECHANGED, wParam, lParam);  // SendMessage

    TraceMsg(DM_PALETTE, "cbb::_pc (%08X) ends -------------------------", this);
}

BOOL CBaseBrowser2::_QueryNewPalette()
{
    BrowserPaletteType bptNew;
    HPALETTE hpalNew = NULL;
    BOOL fResult = TRUE;
    BOOL fSkipRealize = FALSE;
    HDC hdc;

    TraceMsg(DM_PALETTE, "cbb::_qnp (%08X) begins ==================================", this);

TryAgain:
    switch (_bptBrowser)
    {
    case BPT_Normal:
        TraceMsg(DM_PALETTE, "cbb::_qnp - normal realization");
        //
        // Normal Realization: realize _hpalBrowser in the foreground
        //

        // avoid realzing the palette into the display if we've been asked not to...
        if ( !fSkipRealize )
            _RealizeBrowserPalette(FALSE);
        break;

    case BPT_ShellView:
        TraceMsg(DM_PALETTE, "cbb::_qnp - forwarding to shell view");
        //
        // Win95 Explorer-compatible: forward the query to the shell view
        //
        if (_bbd._hwndView && SendMessage(_bbd._hwndView, WM_QUERYNEWPALETTE, 0, 0))
            break;

        TraceMsg(DM_PALETTE, "cbb::_qnp - no shell view or view didn't answer");

        //
        // we only manage our palette as a toplevel app
        //

        //
        // the view didn't handle it; fall through to use a generic palette
        //
    UseGenericPalette:
        TraceMsg(DM_PALETTE, "cbb::_qnp - using generic palette");
        //
        // Use a Halftone Palette for the device
        //
        hpalNew = g_hpalHalftone;
        bptNew = BPT_Normal;
        goto UseThisPalette;

    case BPT_UnknownPalette:
    case BPT_PaletteViewChanged:
        TraceMsg(DM_PALETTE, "cbb::_qnp - computing palette");
        //
        // Undecided: try to use IViewObject::GetColorSet to compose a palette
        //
        LOGPALETTE *plp;
        HRESULT hres;

        // default to forwarding to the view if something fails along the way
        hpalNew = NULL;
        bptNew = BPT_ShellView;

        //
        // if we have a view object then try to get its color set
        //
        if (!_pvo)
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - no view object");
            goto UseGenericPalette;
        }

        plp = NULL;
        hres = _pvo->GetColorSet(DVASPECT_CONTENT, -1, NULL, NULL, NULL, &plp);

        if (FAILED(hres))
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - view object's GetColorSet failed");
            goto UseThisPalette;
        }

        //
        // either a null color set or S_FALSE mean the view object doesn't care
        //
        if (!plp)
            hres = S_FALSE;

        if (hres != S_FALSE)
        {
            //
            // can we reuse the current palette object?
            //
            HRESULT hrLocal = _UpdateBrowserPaletteInPlace(plp);
            if (FAILED( hrLocal ))
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - creating new palette for view object's colors");
                hpalNew = CreatePalette(plp);
            }
            else
            {
                hpalNew = _hpalBrowser;

                // NOTE: if we got back the same palette, don't bother realizing it into the foreground.
                // NOTE: this has the (desirable) side effect of stops us flashing the display when a
                // NOTE: control on a page has (wrongly) realized its own palette...
                if ( hrLocal == S_FALSE )
                {
                    // ASSERT( GetActiveWindow() == _bbd._hwnd );
                    fSkipRealize = TRUE;
                }
            }

            //
            // did we succeed at setting up a palette?
            //
            if (hpalNew)
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - palette is ready to use");
                bptNew = BPT_Normal;
            }
            else
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - failed to create palette");
            }
        }

        //
        // free the logical palette from the GetColorSet above
        //
        if (plp)
            CoTaskMemFree(plp);

        //
        // if the view object responded that it didn't care then pick a palette
        //
        if (hres == S_FALSE)
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - view object doesn't care");
            goto UseGenericPalette;
        }

        //
        // fall through to use the palette we decided on
        //
    UseThisPalette:
        //
        // we get here when we've decided on a new palette strategy
        //
        TraceMsg(DM_PALETTE, "cbb::_qnp - chose palette %08x", hpalNew);
        //
        // do we have a new palette object to use?
        //
        if (hpalNew != _hpalBrowser)
        {
            if (_hpalBrowser && _hpalBrowser != g_hpalHalftone)
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - deleting old palette %08x", _hpalBrowser);
                DeletePalette(_hpalBrowser);
            }
            _hpalBrowser = hpalNew;
        }
        
        //
        // notify the hosted object that we just changed the palette......
        //
        if ( _bbd._pctView )
        {
            VARIANTARG varIn = {0};
            varIn.vt = VT_I4;
            varIn.intVal = DISPID_AMBIENT_PALETTE;
            
            _bbd._pctView->Exec( &CGID_ShellDocView, SHDVID_AMBIENTPROPCHANGE, 0, &varIn, NULL );
        }

        //
        // now loop back and use this new palette strategy
        //
        _bptBrowser = bptNew;
        goto TryAgain;

    case BPT_UnknownDisplay:
    case BPT_DisplayViewChanged:
    case BPT_DeferPaletteSupport:
        TraceMsg(DM_PALETTE, "cbb::_qnp - unknown display");
        //
        // Unknown Display: decide whether we need palette support or not
        //
        hdc = GetDC(NULL);
        bptNew = (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)?
            BPT_UnknownPalette : BPT_NotPalettized;
        ReleaseDC(NULL, hdc);

        //
        // Set the new mode and branch accordingly
        // NOTE: we don't do a UseThisPalette here because it is still unknown
        //
        if ((_bptBrowser = bptNew) == BPT_UnknownPalette)
            goto TryAgain;

        TraceMsg(DM_PALETTE, "cbb::_qnp - not in palettized display mode");

        //
        // fall through to non-palette case
        //
    case BPT_NotPalettized:
        //
        // Not in Palettized Mode: do nothing
        //
        // if we just switched from a palettized mode then free our palette
        //
        if (_hpalBrowser)
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - old palette still lying around");
            hpalNew = NULL;
            bptNew = BPT_NotPalettized;
            goto UseThisPalette;
        }

        //
        // and don't do anything else
        //
        fResult = FALSE;
        break;

    default:
        TraceMsg(DM_PALETTE, "cbb::_qnp - invalid BPT_ state!");
        //
        // we should never get here
        //
        ASSERT(FALSE);
        _bptBrowser = BPT_UnknownDisplay;
        goto TryAgain;
    }

    TraceMsg(DM_PALETTE, "cbb::_qnp (%08X) ends ====================================", this);

    return fResult;
}


HRESULT CBaseBrowser2::_TryShell2Rename(IShellView* psv, LPCITEMIDLIST pidlNew)
{
    HRESULT hres = E_FAIL;

    if (EVAL(psv))  // Winstone once found it to be NULL.
    {
        // BUGBUGCHANGE -  overloading the semantics of IShellExtInit
        IPersistFolder* ppf;
        hres = psv->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf));
        if (SUCCEEDED(hres)) 
        {
            hres = ppf->Initialize(pidlNew);
            if (SUCCEEDED(hres)) 
            {
                // we need to update what we're pointing to
                LPITEMIDLIST pidl = ILClone(pidlNew);
                if (pidl) 
                {
                    if (IsSameObject(psv, _bbd._psv)) 
                    {
                        ASSERT(_bbd._pidlCur);
                        ILFree(_bbd._pidlCur);
                        _bbd._pidlCur = pidl;

                        // If the current pidl is renamed, we need to fire a
                        // TITLECHANGE event. We don't need to do this in the
                        // pending case because the NavigateComplete provides
                        // a way to get the title.
                        //
                        WCHAR wzFullName[MAX_URL_STRING];

                        ::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_NORMAL, wzFullName, SIZECHARS(wzFullName), NULL);
            
                        FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, wzFullName);
                    } 
                    else if (IsSameObject(psv, _bbd._psvPending)) 
                    {
                        ASSERT(_bbd._pidlPending);
                        ILFree(_bbd._pidlPending);
                        _bbd._pidlPending = pidl;
                    } 
                    else 
                    {
                        // BUGBUG: It may be possible to get here during _MayPlayTransition!
                        //
                        ASSERT(!_bbd._psvPending); // this should be the case if we get here
                        ASSERT(FALSE); // we should never get here or we have a problem
                    }
                }
            }
            ppf->Release();
        }
    }

    return hres;
}

HRESULT CBaseBrowser2::OnSize(WPARAM wParam)
{
    // If we are playing a transition, stop it.
    if (_ptrsite && (_ptrsite->_uState == CTransitionSite::TRSTATE_PAINTING))
        _ptrsite->_StopTransition();

    if (wParam != SIZE_MINIMIZED) {
        _pbsOuter->v_ShowHideChildWindows(FALSE);
    }
    
    return S_OK;
}

BOOL CBaseBrowser2::v_OnSetCursor(LPARAM lParam)
{
    if (_fNavigate || _fDescendentNavigate) {

        switch (LOWORD(lParam)) {
        case HTBOTTOM:
        case HTTOP:
        case HTLEFT:
        case HTRIGHT:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        case HTTOPLEFT:
        case HTTOPRIGHT:
            break;

        default:
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        }
    }
    
    return FALSE;
}

const SA_BSTRGUID s_sstrSearchFlags = {
    38 * SIZEOF(WCHAR),
    L"{265b75c1-4158-11d0-90f6-00c04fd497ea}"
};

#define PROPERTY_VALUE_SEARCHFLAGS ((BSTR)s_sstrSearchFlags.wsz)

LRESULT CBaseBrowser2::_OnGoto(void)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_OnGoto called");

    //
    //  If we can't navigate right now, postpone it by restoring _uAction
    // and don't free pidlQueued. Subsequent _MayUnblockAsyncOperation call
    // will post WMC_ASYNCOPERATION (if we can navigate) and we come here
    // again.
    //
    if (!_CanNavigate()) 
    {
        TraceMsg(TF_SHDNAVIGATE, "CBB::_OnGoto can't do it now. Postpone!");
        _uActionQueued = ASYNCOP_GOTO;
        return S_FALSE;
    }

    LPITEMIDLIST pidl = _pidlQueued;
    DWORD        dwSBSP = _dwSBSPQueued;
    _dwSBSPQueued = 0;

    _pidlQueued = NULL;

    if (pidl && PIDL_NOTHING != pidl) {

        DWORD grfHLNF = 0;
        
        if (dwSBSP & SBSP_WRITENOHISTORY)
        {
            grfHLNF |= SHHLNF_WRITENOHISTORY;
        }
        if (dwSBSP & SBSP_NOAUTOSELECT)
        {
            grfHLNF |= SHHLNF_NOAUTOSELECT;
        }

        if (PIDL_LOCALHISTORY == pidl)
        {
            pidl = NULL;

            // For beta2 we need to do a better job mapping SBSP to HLNF values.
            // For beta1, this is the only case that's busted.
            //
            // This problem stems from converting _NavigateToPidl in ::NavigateToPidl
            // into a call to the Async version
            //
            if (dwSBSP & SBSP_NAVIGATEBACK)
                grfHLNF = HLNF_NAVIGATINGBACK;
            else if (dwSBSP & SBSP_NAVIGATEFORWARD)
                grfHLNF = HLNF_NAVIGATINGFORWARD;
        }
        else if (dwSBSP == (DWORD)-1)
        {
            // Same problem as above
            //
            // This problem stems from converting _NavigateToPidl in ::NavigateToTLItem
            // into a call to the Async version
            //
            grfHLNF = (DWORD)-1;
        }
        else
        {
            if (dwSBSP & SBSP_REDIRECT) {
                grfHLNF |= HLNF_CREATENOHISTORY;
            }
            {
                IWebBrowser2 *pWB2; 
                BOOL  bAllow = ((dwSBSP & SBSP_ALLOW_AUTONAVIGATE) ? TRUE : FALSE);
    
                if (bAllow)
                    grfHLNF |= HLNF_ALLOW_AUTONAVIGATE;
    
                if (SUCCEEDED(_pspOuter->QueryService(SID_SHlinkFrame, IID_IWebBrowser2, (void **) &pWB2))) {
                    if (pWB2) {
                        VARIANT v;
                        VariantInit (&v);
                        pWB2->GetProperty(PROPERTY_VALUE_SEARCHFLAGS, &v);
                        v.lVal &= (~ 0x00000001);   // Clear the allow flag before we try to set it.
                        if (v.vt == VT_I4) {
                                v.lVal |= (bAllow ? 0x01 : 0x00);
                        } else {
                            v.vt = VT_I4;
                            v.lVal = (bAllow ? 0x01 : 0x00);
                        }
                        pWB2->PutProperty(PROPERTY_VALUE_SEARCHFLAGS, v);
                        pWB2->Release();
                    }
                }
            }
        }


        TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_OnGoto called calling _NavigateToPidl"), pidl, _bbd._pidlCur);
        _pbsOuter->_NavigateToPidl(pidl, (DWORD)grfHLNF, dwSBSP);
    } else {
        // wParam=NULL means canceling the navigation.
        TraceMsg(DM_NAV, "NAV::_OnGoto calling _CancelPendingView");
        _CancelPendingView();

        if (PIDL_NOTHING == pidl)
        {
            // If we're being told to navigate to nothing, go there
            //
            // BUGBUG REVIEW: What should we do with the history??
            //
            _pbsOuter->ReleaseShellView();
        }
        else if (!_bbd._pidlCur)
        {
            //
            //  If the very first navigation failed, navigate to
            // a local html file so that the user will be able
            // to View->Options dialog.
            //
            TCHAR szPath[MAX_PATH]; // This is always local
            HRESULT hresT=_GetStdLocation(szPath, ARRAYSIZE(szPath), DVIDM_GOLOCALPAGE);
            if (FAILED(hresT) || !PathFileExists(szPath)) {
                StrCpyN(szPath, TEXT("shell:Desktop"), ARRAYSIZE(szPath));
            }
            BSTR bstr = SysAllocStringT(szPath);
            if (bstr) {
                TraceMsg(TF_SHDNAVIGATE, "CBB::_OnGoto Calling _bbd._pauto->Navigate(%s)", szPath);
                _bbd._pautoWB2->Navigate(bstr,NULL,NULL,NULL,NULL);
                SysFreeString(bstr);
            }
        }
    }

    _FreeQueuedPidl(&pidl);

    return(0);
}

void CBaseBrowser2::_FreeQueuedPidl(LPITEMIDLIST* ppidl)
{
    if (*ppidl && PIDL_NOTHING != *ppidl) {
        ILFree(*ppidl);
    }
    *ppidl = NULL;
}

HRESULT CBaseBrowser2::OnFrameWindowActivateBS(BOOL fActive)
{
    BOOL fOldActive = _fActive;
    
    if (_pact)
    {
        TraceMsg(TF_SHDUIACTIVATE, "OnFrameWindowActivateBS(%d)", fActive);
        _pact->OnFrameWindowActivate(fActive);
    }

    _fActive = fActive;
    
    if (fActive && !fOldActive && _fUIActivateOnActive)
    {
        _fUIActivateOnActive = FALSE;

        _UIActivateView(SVUIA_ACTIVATE_FOCUS);
    }

    return S_OK;
}

LRESULT CBaseBrowser2::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
#ifdef DEBUG
    // compile time assert to make sure we don't use these msgs
    // here since we must allow these to go to the subclasses 
    case CWM_GLOBALSTATECHANGE:
    case CWM_FSNOTIFY:
    case WMC_ACTIVATE:
        break;
#endif
        
    // UGLY: Win95/NT4 shell DefView code sends this msg and does not deal
    // with the failure case. other ISVs do the same so this needs to stay forever
    case CWM_GETISHELLBROWSER:
        return (LRESULT)_psbOuter;  // not ref counted!

    //  WM_COPYDATA is used to implement inter-window target'ed navigation
    //  Copy data contains target, URL, postdata and referring URL
    case WM_COPYDATA:
        return (LRESULT)FALSE;

    case WMC_ASYNCOPERATION:
        {
            UINT uAction = _uActionQueued;
            _uActionQueued = ASYNCOP_NIL;

            switch(uAction) {
            case ASYNCOP_GOTO:
                _OnGoto();
                break;
    
            case ASYNCOP_ACTIVATEPENDING:
                VALIDATEPENDINGSTATE();
                if (_bbd._psvPending) // paranoia
                {  
                    DebugMemLeak(DML_TYPE_NAVIGATE | DML_BEGIN);
                    if (FAILED(_pbsOuter->ActivatePendingView()) && _cRefCannotNavigate > 0)
                    {
                        _uActionQueued = ASYNCOP_ACTIVATEPENDING; // retry activation
                    }
                    DebugMemLeak(DML_TYPE_NAVIGATE | DML_END);
                }
                break;
    
            case ASYNCOP_CANCELNAVIGATION:
                _CancelPendingNavigation();
                break;

            case ASYNCOP_NIL:
                break;

            default:
                ASSERT(0);
                break;
            }
        }
        break;

    case WMC_ONREFRESHTIMER:
        _OnRefreshTimer();
        break;

    case WM_SIZE:
        _pbsOuter->OnSize(wParam);
        break;

#ifdef PAINTINGOPTS
    case WM_WINDOWPOSCHANGING:
        // Let's not waste any time blitting bits around, the viewer window
        // is really the guy that has the content so when it resizes itself
        // it can decide if it needs to blt or not.  This also makes resizing
        // look nicer.
        ((LPWINDOWPOS)lParam)->flags |= SWP_NOCOPYBITS;
        goto DoDefault;
#endif

    case WM_ERASEBKGND:
        if (!_bbd._hwndView)
            goto DoDefault;

        // Don't draw WM_ERASEBKGND if we are going to play transition.
        if (_ptrsite && (_ptrsite->_uState == CTransitionSite::TRSTATE_INITIALIZING))
            return TRUE;

        goto DoDefault;

    case WM_PAINT:
        if (_ptrsite && (_ptrsite->_uState == CTransitionSite::TRSTATE_PAINTING)) 
        {
            ASSERT(_ptrsite->_pTransition);

            PAINTSTRUCT ps;
            HDC         hdc = BeginPaint(hwnd, &ps);
            
            HPALETTE hpalPrev = NULL;   // init to suppress bogus C4701 warning
            if (_hpalBrowser)
            {
                hpalPrev = SelectPalette(hdc, _hpalBrowser, TRUE);
                RealizePalette(hdc);
            }

            RECT rc;
            //_pbsOuter->GetViewRect(&rc);
            GetBorder(&rc);

            _ptrsite->_pTransition->Draw(hdc, &rc);

            if (_hpalBrowser)
            {
                SelectPalette(hdc, hpalPrev, TRUE);
                RealizePalette(hdc);
            }

            EndPaint(hwnd, &ps);
        }
        goto DoDefault;

    case WM_SETFOCUS:
        return _pbsOuter->OnSetFocus();

    case WM_DISPLAYCHANGE:
        _DisplayChanged(wParam, lParam);
        break;

    case WM_PALETTECHANGED:
        _PaletteChanged(wParam, lParam);
        break;

    case WM_QUERYNEWPALETTE:
        // we always pass -1 as the LParam to show that we posted it to ourselves...
        if ( lParam != 0xffffffff )
        {
            // otherwise, it looks like the system or our parent has just sent a real honest to God,
            // system WM_QUERYNEWPALETTE, so we now own the Foreground palette and we have a license to
            // to SelectPalette( hpal, FALSE );
            _fOwnsPalette = TRUE;
        }
        return _QueryNewPalette();

    case WM_SYSCOLORCHANGE:
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    case WM_WININICHANGE:
    case WM_FONTCHANGE:
        v_PropagateMessage(uMsg, wParam, lParam, TRUE);
        break;

    case WM_PRINT:
        // Win95 explorer did this
        if (_bbd._hwndView)
            SendMessage(_bbd._hwndView, uMsg, wParam, lParam);
        break;

#ifdef DEBUG
    case WM_ACTIVATE:
        // do *not* do any toolbar stuff here.  it will screw up desktop.
        // override does that in shbrows2.cpp
        TraceMsg(DM_FOCUS, "cbb.wpbs(WM_ACT): => default");
        goto DoDefault;
#endif

    case WM_SETCURSOR:
        if (v_OnSetCursor(lParam))
            return TRUE;
        goto DoDefault;

    case WM_CREATE:
        if (S_OK != _pbsOuter->OnCreate((LPCREATESTRUCT)lParam))
        {
            _pbsOuter->OnDestroy();
            return -1;
        }
        return 0;

    case WM_NOTIFY:
        return _pbsOuter->OnNotify((LPNMHDR)lParam);

    case WM_COMMAND:
        _pbsOuter->OnCommand(wParam, lParam);
        break;

    case WM_DESTROY:
        _pbsOuter->OnDestroy();
        break;

    default:
        if (uMsg == g_idMsgGetAuto)
        {
            //
            //  According to LauraBu, using WM_GETOBJECT for our private
            // purpose will work, but will dramatically slow down
            // accessibility apps unless we actually implement one of
            // accessibility interfaces. Therefore, we use a registered
            // message to get the automation/frame interface out of
            // IE/Nashvile frame. (SatoNa)
            //
            IUnknown* punk;
            if (SUCCEEDED(_bbd._pautoSS->QueryInterface(*(IID*)wParam, (void **)&punk)))
                return (LRESULT)punk; // Note that it's AddRef'ed by QI.
            return 0;
        }
        else if (uMsg == GetWheelMsg()) 
        {
             // Forward the mouse wheel message on to the view window
            if (_bbd._hwndView) 
            {
                PostMessage(_bbd._hwndView, uMsg, wParam, lParam);
                return 1;
            }
            // Fall through...
        }
DoDefault:
        return _DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// *** IOleWindow methods ***
HRESULT CBaseBrowser2::GetWindow(HWND * lphwnd)
{
    *lphwnd = _bbd._hwnd;
    return S_OK;
}

HRESULT CBaseBrowser2::GetViewWindow(HWND * lphwnd)
{
    *lphwnd = _bbd._hwndView;
    return S_OK;
}

HRESULT CBaseBrowser2::ContextSensitiveHelp(BOOL fEnterMode)
{
    // BUGBUG: Visit here later.
    return E_NOTIMPL;
}

// *** IShellBrowser methods *** (same as IOleInPlaceFrame)
HRESULT CBaseBrowser2::InsertMenusSB(HMENU hmenuShared,
                            LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return S_OK;
}

HRESULT CBaseBrowser2::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuReserved,
            HWND hwndActiveObject)
{
    return S_OK;
}



/*----------------------------------------------------------
Purpose: Remove menus that are shared with other menus from 
         the given browser menu.


Returns: 
Cond:    --
*/
HRESULT CBaseBrowser2::RemoveMenusSB(HMENU hmenuShared)
{
    return S_OK;
}

HRESULT CBaseBrowser2::SetStatusTextSB(LPCOLESTR lpszStatusText)
{
    // Even if we're not toplevel, send this to SendControlMsg
    // so events get notified. (Also simplifies CVOCBrowser.)
    //
    HRESULT hres;
    
    // If we are asked to put some text into the status bar, first save off what is already in pane 0
    if (lpszStatusText)
    {
        LRESULT lIsSimple = FALSE;
        
        // If we have a menu down, then we are already in simple mode. So send the 
        // text to pane 255 (simple)
        _psbOuter->SendControlMsg(FCW_STATUS, SB_ISSIMPLE, 0, 0L, &lIsSimple);
        
        if (!_fHaveOldStatusText && !lIsSimple)
        {
            WCHAR wzStatusText[MAX_URL_STRING];
            LRESULT ret;

            // TODO: Put this into a wrapper function because iedisp.cpp does something similar.
            //       Great when we convert to UNICODE
            if (SUCCEEDED(_psbOuter->SendControlMsg(FCW_STATUS, SB_GETTEXTLENGTHW, 0, 0, &ret)) &&
                LOWORD(ret) < ARRAYSIZE(wzStatusText))
            {
                // SB_GETTEXTW is not supported by the status bar control in Win95. Hence, the thunk here.
                _psbOuter->SendControlMsg(FCW_STATUS, SB_GETTEXTW, STATUS_PANE_NAVIGATION, (LPARAM)wzStatusText, NULL);
                StrCpyNW(_szwOldStatusText, wzStatusText, ARRAYSIZE(_szwOldStatusText)-1);
                _fHaveOldStatusText = TRUE;
            }
        }   

        hres = _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, lIsSimple ? 255 | SBT_NOBORDERS : STATUS_PANE_NAVIGATION | SBT_NOTABPARSING, (LPARAM)lpszStatusText, NULL);
    }
    else if (_fHaveOldStatusText) 
    {
        VARIANTARG var = {0};
        if (_bbd._pctView && SUCCEEDED(_bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_NAVIGATION, NULL, &var))
             && V_UI4(&var) != PANE_NONE)
        {
            hres = _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, V_UI4(&var),(LPARAM)_szwOldStatusText, NULL);
        }
        else
        {
            hres = E_FAIL;
        }
        _fHaveOldStatusText = FALSE;
    }
    else
    {
        // No message, and no old status text, so clear what's there.
        hres = _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, STATUS_PANE_NAVIGATION | SBT_NOTABPARSING , (LPARAM)lpszStatusText, NULL);
    }
    return hres;
}

HRESULT CBaseBrowser2::EnableModelessSB(BOOL fEnable)
{
    //  We no longer call _CancelNavigation here, which causes some problems
    // when the object calls EnableModeless when we are navigating away
    // (see IE bug 4581). Instead, we either cancel or postpone asynchronous
    // event while _DisableModeless(). (SatoNa)

    //
    // If we're NOT top level, assume virtual EnableModelessSB
    // handled this request and forwarded it to us. (See CVOCBrowser.)
    //
    if (fEnable)
    {
        // Robust against random calls
        //
        // If this EVAL rips, somebody is calling EMSB(TRUE) without a
        // (preceeding) matching EMSB(FALSE).  Find and fix!
        //
        if (EVAL(_cRefCannotNavigate > 0))
        {
            _cRefCannotNavigate--;
        }

        // Tell the shell's HTML window to retry pending navigation.
        if (_cRefCannotNavigate == 0 && _phtmlWS)
        {
            _phtmlWS->CanNavigate();
        }
    }
    else
    {
        _cRefCannotNavigate++;
    }

    //
    //  If there is any blocked async operation AND we can navigate now,
    // unblock it now. 
    //
    _MayUnblockAsyncOperation();

    return S_OK;
}

HRESULT CBaseBrowser2::TranslateAcceleratorSB(LPMSG lpmsg, WORD wID)
{
    return S_FALSE;
}

//
//  This function starts the navigation to the navigation to the specified
// pidl asynchronously. It cancels the pending navigation synchronously
// if any.
//
// NOTE: This function takes ownership of the pidl -- caller does NOT free pidl!!!
//
void CBaseBrowser2::_NavigateToPidlAsync(LPITEMIDLIST pidl, DWORD dwSBSP, BOOL fDontCallCancel)
{
    BOOL fCanSend = FALSE;

    TraceMsg(TF_SHDNAVIGATE, "CBB::_NavigateToPidlAsync called");

    // _StopAsyncOperation(); 
    if (!fDontCallCancel)
        _CancelPendingNavigation(); // which calls _StopAsyncOperation too
    else {
        //
        //  I'm removing this assert because _ShowBlankPage calls this funcion
        // with fDontCallCancel==TRUE -- callin _CancelPendingNavigation here
        // causes GPF in CDocHostObject::_CancelPendingNavigation. (SatoNa)
        //
        // ASSERT(_bbd._pidlPending == NULL);
    }

    _pidlQueued = pidl;
    _dwSBSPQueued = dwSBSP;

    // Technically a navigate must be async or we have problems such as:
    //   1> object initiating the navigate (mshtml or an object on the page
    //      or script) gets destroyed when _bbd._psv is removed and then we return
    //      from this call into the just freed object.
    //   2> object initiating the navigate gets called back by an event
    //
    // In order for Netscape OM compatibility, we must ALWAY have a _bbd._psv or
    // _bbd._psvPending, so we go SYNC when we have neither. This avoids problem
    // <1> but not problem <2>. As we find faults, we'll work around them.
    //
    // Check _fAsyncNavigate to avoid navigate when persisting the WebBrowserOC
    // This avoids faults in Word97 and MSDN's new InfoViewer -- neither like
    // being reentered by an object they are in the middle of initializing.
    //
    if (_bbd._psv || _bbd._psvPending || _fAsyncNavigate)
    {
        _PostAsyncOperation(ASYNCOP_GOTO);
    }
    else
    {
        //  if we are just starting out, we can do this synchronously and
        //  reduce the window where the IHTMLWindow2 for the frame is undefined
        fCanSend = TRUE;
    }

    // Starting a navigate means we are loading someing...
    //
    OnReadyStateChange(NULL, READYSTATE_LOADING);

    //
    // Don't play sound for the first navigation (to avoid multiple
    // sounds to be played for a frame-set creation).
    //
    if (_bbd._psv && IsWindowVisible(_bbd._hwnd)) {
        IEPlaySound(TEXT("Navigating"), FALSE);
    }

    if (fCanSend)
    {
        _SendAsyncOperation(ASYNCOP_GOTO);
    }
}


// BUGBUG REVIEW: now that all navigation paths go through
// _NavigateToPidlAsync we probably don't need to activate async.
// Remove this code...
//
BOOL CBaseBrowser2::_ActivatePendingViewAsync(void)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_ActivatePendingViewAsync called");

    _PreActivatePendingViewAsync();

    //
    // _bbd._psvPending is for debugging purpose.
    //
    return _PostAsyncOperation(ASYNCOP_ACTIVATEPENDING);
}

HRESULT CBaseBrowser2::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    HRESULT hres;
    BOOL fCloning = FALSE;

    if (PIDL_NOTHING == pidl)
    {
        if (!_CanNavigate()) return HRESULT_FROM_WIN32(ERROR_BUSY);

        _NavigateToPidlAsync((LPITEMIDLIST)PIDL_NOTHING, wFlags);
        return S_OK;
    }

    if (!_CanNavigate()) return HRESULT_FROM_WIN32(ERROR_BUSY);


    LPITEMIDLIST pidlNew = NULL;
    int iTravel = 0;

    switch(wFlags & (SBSP_RELATIVE|SBSP_ABSOLUTE|SBSP_PARENT|SBSP_NAVIGATEBACK|SBSP_NAVIGATEFORWARD))
    {
    case SBSP_NAVIGATEBACK:
        ASSERT(pidl==NULL || pidl==PIDL_LOCALHISTORY);
        iTravel = TLOG_BACK;
        break;

    case SBSP_NAVIGATEFORWARD:
        ASSERT(pidl==NULL || pidl==PIDL_LOCALHISTORY);
        iTravel = TLOG_FORE;
        break;

    case SBSP_RELATIVE:
        if (ILIsEmpty(pidl) && IsFlagSet(wFlags, SBSP_NEWBROWSER))
            fCloning = TRUE;
        else if (_bbd._pidlCur)
            pidlNew = ILCombine(_bbd._pidlCur, pidl);
        break;

    case SBSP_PARENT:
        pidlNew = ILCloneParent(_bbd._pidlCur);
        break;

    default:
        ASSERT(FALSE);
        // fall through
    case SBSP_ABSOLUTE:
        pidlNew = ILClone(pidl);
        break;
    }

    if(iTravel)
    {
        HRESULT hrTravel;
        ITravelLog *ptl;
        if(SUCCEEDED(hrTravel = GetTravelLog(&ptl)))
            hrTravel = ptl->Travel(SAFECAST(this, IShellBrowser*), iTravel);

        _pbsOuter->UpdateBackForwardState();
        ATOMICRELEASE(ptl);
        return hrTravel;
    }

    // if block is needed for multi-window open.  if we're called to open  a new
    // window, but we're in the middle of navigating, we say we're busy.
    if (wFlags & SBSP_SAMEBROWSER)
    {
        if (wFlags & (SBSP_EXPLOREMODE | SBSP_OPENMODE))
        {
            // fail this if we're already navigating
            if (!_CanNavigate() || (_uActionQueued == ASYNCOP_GOTO))
            {
                return HRESULT_FROM_WIN32(ERROR_BUSY);
            }
        }
    }
    
    
    if (pidlNew || fCloning)
    {
        if ((wFlags & (SBSP_NEWBROWSER|SBSP_SAMEBROWSER|SBSP_DEFBROWSER)) == SBSP_NEWBROWSER)
        {
            _OpenNewFrame(pidlNew, wFlags); // takes ownership of pidl
        }
        else
        {
            _NavigateToPidlAsync(pidlNew, wFlags /* grfSBSP */); // takes ownership of pidl
        }
        hres = S_OK;
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}


HRESULT CBaseBrowser2::GetControlWindow(UINT id, HWND * lphwnd)
{
    return E_FAIL;
}

HRESULT CBaseBrowser2::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam,
            LPARAM lParam, LRESULT *pret)
{
    HWND hwndControl = NULL;

    if (pret)
    {
        *pret = 0;
    }

    // If this is statusbar and set text then signal event change.
    if ((id == FCW_STATUS) && (uMsg == SB_SETTEXT || uMsg == SB_SETTEXTW) && // trying to set status text
        (!(wParam & SBT_OWNERDRAW))) // we don't own the window -- this can't work
    {
        // When browser or java perf timing mode is enabled, use "Done" or "Applet Started" 
        // in the status bar to get load time.
        if(g_dwStopWatchMode && (g_dwStopWatchMode & (SPMODE_BROWSER | SPMODE_JAVA)))
        {
            StopWatch_MarkJavaStop((LPSTR)lParam, _bbd._hwnd, (uMsg == SB_SETTEXTW));
        }
        
        if (uMsg == SB_SETTEXTW)
        {
            FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_STATUSTEXTCHANGE, (LPWSTR)lParam);
        }
        else
        {
            FireEvent_DoInvokeString(_bbd._pautoEDS, DISPID_STATUSTEXTCHANGE, (LPSTR)lParam);
        }
    }

    HRESULT hres = _psbOuter->GetControlWindow(id, &hwndControl);
    if (SUCCEEDED(hres))
    {
        LRESULT ret = SendMessage(hwndControl, uMsg, wParam, lParam);
        if (pret)
        {
            *pret = ret;
        }
    }

    return hres;
}
 
HRESULT CBaseBrowser2::QueryActiveShellView(struct IShellView ** ppshv)
{
    IShellView * psvRet = _bbd._psv;

    if ( _fCreateViewWindowPending )
    {
        ASSERT( _bbd._psvPending );
        psvRet = _bbd._psvPending;
    }
    //
    // We have both psv and hwndView after the completion of view creation.
    //
    *ppshv = psvRet;
    if (psvRet)
    {
        psvRet->AddRef();
        return NOERROR;
    }

    return E_FAIL;
}

HRESULT CBaseBrowser2::OnViewWindowActive(struct IShellView * psv)
{
    AssertMsg((!_bbd._psv || IsSameObject(_bbd._psv, psv)),
              TEXT("CBB::OnViewWindowActive _bbd._psv(%x)!=psv(%x)"),
              psv, _bbd._psv);
    _pbsOuter->_OnFocusChange(ITB_VIEW);
    return S_OK;
}

//==========================================================================
//
//==========================================================================

HRESULT CBaseBrowser2::SetToolbarItems(LPTBBUTTON pViewButtons, UINT nButtons,
            UINT uFlags)
{
    return NOERROR;
}


//
// Notes: pidlNew will be freed
//
HRESULT CBaseBrowser2::_OpenNewFrame(LPITEMIDLIST pidlNew, UINT wFlags)
{
    UINT uFlags = COF_CREATENEWWINDOW;
    
    if (wFlags & SBSP_EXPLOREMODE) 
        uFlags |= COF_EXPLORE;
    else 
    {
        // maintain the same class if possible
        TCHAR sz[20];
        GetClassName(_bbd._hwnd, sz, ARRAYSIZE(sz));
        if (0 == StrCmpI(sz, TEXT("IEFrame")))
            uFlags |= COF_IEXPLORE;
    }

    IBrowserService *pbs;
    ITravelLog *ptlClone = NULL;
    DWORD bid = BID_TOPFRAMEBROWSER;

#ifdef UNIX
    if (wFlags & SBSP_HELPMODE) 
        uFlags |= COF_HELPMODE;

    /* v-sriran: 12/10/97
     * On UNIX, we do not want to transfer the travel log to the help browser
     * so that users cannot navigate back into the parent's history
     */
    if(!(wFlags & SBSP_NOTRANSFERHIST))
#endif
    if(SUCCEEDED(_pspOuter->QueryService(SID_STopFrameBrowser, IID_IBrowserService, (void **)&pbs)))
    {
        ITravelLog *ptl;

        if(SUCCEEDED(pbs->GetTravelLog(&ptl)))
        {
            ASSERT(ptl);
            if(SUCCEEDED(ptl->Clone(&ptlClone)))
            {
                ptlClone->UpdateEntry(pbs, FALSE);

                bid = pbs->GetBrowserIndex();
            }
            ptl->Release();
        }

        pbs->Release();
    }

    HRESULT hr = SHOpenNewFrame(pidlNew, ptlClone, bid, uFlags);
    
    if(ptlClone)
        ptlClone->Release();

    return hr;
}

//
//  This is a helper member of CBaseBroaser class (non-virtual), which
// returns the effective client area. We get this rectangle by subtracting
// the status bar area from the real client area.
//
HRESULT CBaseBrowser2::_GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon)
{
#if 0
    static const int s_rgnViews[] =  {1, 0, 1, FCIDM_STATUS, 0, 0};
    GetEffectiveClientRect(_bbd._hwnd, lprectBorder, (LPINT)s_rgnViews);
#else // BUGBUG split: untested!
    // (derived class overrides w/ GetEffectiveClientRect for FCIDM_STATUS etc.)
    //
    // This code should only be hit in the WebBrowserOC case, but I don't
    // have a convenient assert for that... [mikesh]
    //
    ASSERT(hmon == NULL);
    GetClientRect(_bbd._hwnd, lprectBorder);
#endif
    return NOERROR;
}

HRESULT CBaseBrowser2::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    TraceMsg(TF_SHDUIACTIVATE, "UIW::ReqestBorderSpace pborderwidths=%x,%x,%x,%x",
             pborderwidths->left, pborderwidths->top, pborderwidths->right, pborderwidths->bottom);
    return S_OK;
}

//
// This is an implementation of IOleInPlaceUIWindow::GetBorder.
//
//  This function returns the bounding rectangle for the active object.
// It gets the effective client area, then subtract border area taken by
// all "frame" toolbars.
//
HRESULT CBaseBrowser2::GetBorder(LPRECT lprectBorder)
{
    _pbsOuter->_GetViewBorderRect(lprectBorder);
    return S_OK;
}

//
// NOTES: We used to handle the border space negotiation in CShellBrowser
//  and block it for OC (in Beta-1 of IE4), but I've changed it so that
//  CBaseBrowser2 always handles it. It simplifies our implementation and
//  also allows a DocObject to put toolbars within the frameset, which is
//  requested by the Excel team. (SatoNa)
//
HRESULT CBaseBrowser2::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    if (pborderwidths) {
        TraceMsg(TF_SHDUIACTIVATE, "UIW::SetBorderSpace pborderwidths=%x,%x,%x,%x",
                 pborderwidths->left, pborderwidths->top, pborderwidths->right, pborderwidths->bottom);
        _rcBorderDoc = *pborderwidths;
    } else {
        TraceMsg(TF_SHDUIACTIVATE, "UIW::SetBorderSpace pborderwidths=NULL");
        SetRect(&_rcBorderDoc, 0, 0, 0, 0);
    }
    
    _pbsOuter->_UpdateViewRectSize();
    return S_OK;
}

HRESULT CBaseBrowser2::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    TraceMsg(TF_SHDUIACTIVATE, "UIW::SetActiveObject called %x", pActiveObject);

    ATOMICRELEASE(_pact);

    if (pActiveObject)
    {
        _pact = pActiveObject;
        _pact->AddRef();
    }

    return S_OK;
}


BOOL _IsLegacyFTP(IUnknown * punk)
{
    BOOL fIsLegacyFTP = FALSE;
    CLSID clsid;

    if ((4 == GetUIVersion()) &&
        SUCCEEDED(IUnknown_GetClassID(punk, &clsid)) &&
        IsEqualCLSID(clsid, CLSID_CDocObjectView))
    {
        fIsLegacyFTP = TRUE;
    }

    return fIsLegacyFTP;
}

/***********************************************************************\
    FUNCTION: _AddFolderOptionsSheets

    DESCRIPTION:
        Add the sheets for the "Folder Options" dialog.  These sheets
    come from the IShelLView object.
\***********************************************************************/
HRESULT CBaseBrowser2::_AddFolderOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh)
{
    // Add the normal Folder Option sheets.
    IShellPropSheetExt * ppsx;
    HRESULT hr = E_FAIL;
    
    if (!_IsLegacyFTP(_bbd._psv))
    {
        hr = _pbsOuter->CreateBrowserPropSheetExt(IID_IShellPropSheetExt, (void **)&ppsx);

        if (SUCCEEDED(hr))
        {
            hr = ppsx->AddPages(AddPropSheetPage, (LPARAM)ppsh);
            ppsx->Release();
        }
    }

    // Let the view add additional pages.  The exception will be FTP Folders because it exists to add
    // internet pages and we don't want them here.  However, if the above failed, then
    // we also want to fall back to this.  One of the cases this fixes if if the
    // browser fell back to legacy FTP support (web browser), then the above call will
    // fail on browser only, and we want to fall thru here to add the internet options.  Which
    // is appropriate for the fallback legacy FTP case because the menu will only have "Internet Options"
    // on it.
    if (FAILED(hr) || !IsBrowserFrameOptionsSet(_bbd._psf, BFO_BOTH_OPTIONS))
    {
        EVAL(SUCCEEDED(hr = _bbd._psv->AddPropertySheetPages(dwReserved, pfnAddPropSheetPage, (LPARAM)ppsh)));
    }

    return hr;
}


/***********************************************************************\
    FUNCTION: _AddInternetOptionsSheets

    DESCRIPTION:
        Add the sheets for the "Internet Options" dialog.  These sheets
    come from the browser.
\***********************************************************************/
HRESULT CBaseBrowser2::_AddInternetOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh)
{
    BOOL fPagesAdded = FALSE;
    HRESULT hr = E_FAIL;

    // HACK-HACK: On the original Win95/WinNT shell, shell32's
    //   CDefView::AddPropertySheetPages() didn't call shell
    //   extensions with SFVM_ADDPROPERTYPAGES so they could merge
    //   in additional property pages.  This works around that
    //   problem by having IShellPropSheetExt marge them in.
    //   This is only done for the FTP Folder options NSE.
    //   If other NSEs want both, we need to rewrite the menu merging
    //   code to generate the sheets by by-passing the NSE.  This
    //   is because we know what the menu says so but we don't have
    //   any way for the NSE to populate the pages with what the menu says.
    if ((WhichPlatform() != PLATFORM_INTEGRATED) &&
        IsBrowserFrameOptionsSet(_bbd._psf, (BFO_BOTH_OPTIONS)))
    {
        IShellPropSheetExt * pspse;

        hr = _bbd._psf->QueryInterface(IID_PPV_ARG(IShellPropSheetExt, &pspse));
        if (SUCCEEDED(hr))
        {
            hr = pspse->AddPages(pfnAddPropSheetPage, (LPARAM)ppsh);
            if (SUCCEEDED(hr))
                fPagesAdded = TRUE;
            pspse->Release();
        }
    }

    if (!fPagesAdded)
    {
        // Add the normal Internet Control Panel sheets. (This won't work when viewing FTP)
        hr = _bbd._psv->AddPropertySheetPages(dwReserved, pfnAddPropSheetPage, (LPARAM)ppsh);
    }

    return hr;
}

/***********************************************************************\
    FUNCTION: _DoOptions

    DESCRIPTION:
        The user selected either "Folder Options" or "Internet Options" from
    the View or Tools menu (or where ever it lives this week).  The logic
    in this function is a little strange because sometimes the caller doesn't
    tell us which we need to display in the pvar.  If not, we need to calculate
    what to use.
    1. If it's a URL pidl (HTTP, GOPHER, etc) then we assume it's the
       "Internet Options" dialog.  We then use psv->AddPropertySheetPages()
       to create the "Internet Options" property sheets.
    2. If it's in the shell (or FTP because it needs folder options), then
       we assume it's "Folder Options" the user selected.  In that case,
       we get the property sheets using _pbsOuter->CreateBrowserPropSheetExt().
     
    Now it gets weird.  The PMs want FTP to have both "Internet Options" and
    "Folder Options".  If the pvar param is NULL, assume it's "Folder Options".
    If it was "Internet Options" in the internet case, then I will pass an
    pvar forcing Internet Options.

    NOTE: SBO_NOBROWSERPAGES means "Folder Options".  I'm guessing browser refers
          to the original explorer browser.
\***********************************************************************/

HDPA    CBaseBrowser2::s_hdpaOptionsHwnd = NULL;

void CBaseBrowser2::_DoOptions(VARIANT* pvar)
{
    // Step 1. Determine what sheets to use.
    DWORD dwFlags = SBO_DEFAULT;
    TCHAR szCaption[MAX_PATH];
    
    if (!_bbd._psv)
        return;

    // Did the caller explicitly tell us which to use?
    if (pvar && pvar->vt == VT_I4)
        dwFlags = pvar->lVal;
    else if (_bbd._pidlCur)
    {
        // don't show the Folder Option pages if
        // 1. we're browsing the internet (excluding FTP), or
        // 2. if we're browsing a local file (not a folder), like a local .htm file.
        if (IsBrowserFrameOptionsSet(_bbd._psf, BFO_RENAME_FOLDER_OPTIONS_TOINTERNET))
        {
            // SBO_NOBROWSERPAGES means don't add the "Folder Options" pages.
            dwFlags = SBO_NOBROWSERPAGES;
        }
    }
    
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[MAX_PAGES];

    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_DEFAULT | PSH_USECALLBACK;
    psh.hInstance = MLGetHinst();
    psh.hwndParent = _bbd._hwnd;
    psh.pszCaption = szCaption;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;
    psh.pfnCallback = _OptionsPropSheetCallback;

    // Step 2. Now add "Internet Options" or "Folder Options" sheets.
    if (dwFlags == SBO_NOBROWSERPAGES)
    {
        // They don't want folder pages. (The used to refer to it as browser)
        EVAL(SUCCEEDED(_AddInternetOptionsSheets(0, AddPropSheetPage, &psh)));
        MLLoadString(IDS_INTERNETOPTIONS, szCaption, ARRAYSIZE(szCaption));
    }
    else
    {
        EVAL(SUCCEEDED(_AddFolderOptionsSheets(0, AddPropSheetPage, &psh)));
        MLLoadString(IDS_FOLDEROPTIONS, szCaption, ARRAYSIZE(szCaption));
    }

    if (psh.nPages == 0)
    {
        SHRestrictedMessageBox(_bbd._hwnd);
    }
    else
    {
        // Step 3. Display the dialog
        _bbd._psv->EnableModelessSV(FALSE);
        INT_PTR iPsResult = PropertySheet(&psh);
        _SyncDPA();
        _bbd._psv->EnableModelessSV(TRUE);

        if (ID_PSREBOOTSYSTEM == iPsResult)
        {
            // The "offline folders" prop page will request a reboot if the user
            // has enabled or disabled client-side-caching.
            RestartDialog(_bbd._hwnd, NULL, EWX_REBOOT);
        }
    }
}

// we're here because our prop sheet just closed
// we need to purge it from the hwnd list
// check all the hwnds because 1) there's probably
// only one anyway, 2) paranoia.
void CBaseBrowser2::_SyncDPA()
{
    ENTERCRITICAL;

    if (s_hdpaOptionsHwnd != NULL)
    {
        int i, cPtr = DPA_GetPtrCount(s_hdpaOptionsHwnd);
        ASSERT(cPtr >= 0);

        // remove handles for windows which aren't there anymore
        for (i = cPtr - 1; i >= 0; i--)
        {
            HWND hwnd = (HWND)DPA_GetPtr(s_hdpaOptionsHwnd, i);
            if (!IsWindow(hwnd))
            {
                DPA_DeletePtr(s_hdpaOptionsHwnd, i);
                cPtr--;
            }
        }

        // if there aren't any windows left then clean up the hdpa
        if (cPtr == 0)
        {
            DPA_Destroy(s_hdpaOptionsHwnd);
            s_hdpaOptionsHwnd = NULL;
        }
    }

    LEAVECRITICAL;
}

int CALLBACK
CBaseBrowser2::_OptionsPropSheetCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        {
            ENTERCRITICAL;

            if (s_hdpaOptionsHwnd == NULL)
            {
                // BUGBUG low mem -> Create failure -> don't track hwnd
                s_hdpaOptionsHwnd = DPA_Create(1);
            }

            if (s_hdpaOptionsHwnd != NULL)
            {
                // BUGBUG low mem -> AppendPtr array expansion failure -> don't track hwnd
                DPA_AppendPtr(s_hdpaOptionsHwnd, hwndDlg);
            }

            LEAVECRITICAL;
        }
        break;
    }

    return 0;
}

HRESULT CBaseBrowser2::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, 
                                  OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (rgCmds == NULL)
        return E_INVALIDARG;

    if (pguidCmdGroup == NULL)
    {
        for (ULONG i = 0 ; i < cCmds; i++)
        {
            rgCmds[i].cmdf = 0;

            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_SETDOWNLOADSTATE:
            case OLECMDID_UPDATECOMMANDS:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case OLECMDID_STOP:
            case OLECMDID_STOPDOWNLOAD:
                if (_bbd._psvPending) // pending views are stoppable
                {
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                }
                else if (_bbd._pctView) // current views may support stop also
                {
                    _bbd._pctView->QueryStatus(NULL, 1, &rgCmds[i], pcmdtext);
                }
                break;

            default:
                // set to zero above
                if (_bbd._pctView)
                {
                    // Recursion check.  Avoid looping for those command IDs where Trident bounces
                    // back up to us.
                    //
                    if (_fInQueryStatus)
                        break;
                    _fInQueryStatus = TRUE;
                    _bbd._pctView->QueryStatus(NULL, 1, &rgCmds[i], pcmdtext);
                    _fInQueryStatus = FALSE;
                }
                break;
            }
        }
        // BUGBUG: should fill pcmdtext as well
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        for (ULONG i=0 ; i < cCmds ; i++)
        {
            switch (rgCmds[i].cmdID)
            {            
            case SBCMDID_ADDTOFAVORITES:
            case SBCMDID_CREATESHORTCUT:
                rgCmds[i].cmdf = OLECMDF_ENABLED;   // support these unconditionally
                break;

            case SBCMDID_CANCELNAVIGATION:
                rgCmds[i].cmdf = _bbd._psvPending ? OLECMDF_ENABLED : 0;
                break;

            case SBCMDID_OPTIONS:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
        }
        // BUGBUG: should fill pcmdtext as well
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        for (ULONG i=0 ; i < cCmds ; i++)
        {
            ITravelLog *ptl;

            switch (rgCmds[i].cmdID)
            {
            case SHDVID_CANGOBACK:
                rgCmds[i].cmdf = FALSE; // Assume False 
                if (SUCCEEDED(GetTravelLog(&ptl)))
                {
                    ASSERT(ptl);
                    if (S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_BACK, NULL))
                        rgCmds[i].cmdf = TRUE;
                    ptl->Release();
                }
                break;

            case SHDVID_CANGOFORWARD:
                rgCmds[i].cmdf = FALSE; // Assume False 
                if (SUCCEEDED(GetTravelLog(&ptl)))
                {
                    ASSERT(ptl);
                    if (S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_FORE, NULL))
                        rgCmds[i].cmdf = TRUE;
                    ptl->Release();
                }
                break;

            case SHDVID_PRINTFRAME:
            case SHDVID_MIMECSETMENUOPEN:
            case SHDVID_FONTMENUOPEN:
                if (_bbd._pctView)
                    _bbd._pctView->QueryStatus(pguidCmdGroup, 1, &rgCmds[i], pcmdtext);
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
        }
        // BUGBUG: should fill pcmdtext as well
    }
    else
    {
        return OLECMDERR_E_UNKNOWNGROUP;
    }

    return S_OK;
}

HRESULT CBaseBrowser2::_ShowBlankPage(LPCTSTR pszAboutUrl, LPCITEMIDLIST pidlIntended)
{
    // Never execute this twice.
    if (_fNavigatedToBlank) 
    {
        TraceMsg(TF_WARNING, "Re-entered CBaseBrowser2::_ShowBlankPage");
        return E_FAIL;
    }

    _fNavigatedToBlank = TRUE;

    BSTR bstrURL;
    TCHAR szPendingURL[MAX_URL_STRING + 1];
    TCHAR *pszOldUrl = NULL;
    
    szPendingURL[0] = TEXT('#');
    HRESULT hres;
    

    if (pidlIntended)
    {
        hres = ::IEGetNameAndFlags(pidlIntended, SHGDN_FORPARSING, szPendingURL + 1, SIZECHARS(szPendingURL)-1, NULL);
        if (S_OK == hres)
            pszOldUrl = szPendingURL;
    }   

    hres = CreateBlankURL(&bstrURL, pszAboutUrl, pszOldUrl);
   
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidlTemp;

        hres = IECreateFromPathW(bstrURL, &pidlTemp);
        if (SUCCEEDED(hres)) 
        {
            //
            // Note that we pass TRUE as fDontCallCancel to asynchronously
            // cancel the current view. Otherwise, we hit GPF in CDocHostObject::
            // _CancelPendingNavigation.
            //
            _NavigateToPidlAsync(pidlTemp, 0, TRUE); // takes ownership of pidl
        }

        SysFreeString(bstrURL);
    }
    return hres;
}

int CALLBACK _PunkRelease(void * p, void * pData)
{
    IUnknown* punk = (IUnknown*)p;
    punk->Release();
    return 1;
}

void CBaseBrowser2::_DLMDestroy(void)
{
    if (_hdpaDLM) {
        DPA_DestroyCallback(_hdpaDLM, _PunkRelease, NULL);
        _hdpaDLM = NULL;
    }
}

HRESULT CBaseBrowser2::InitializeDownloadManager()
{
    _hdpaDLM = DPA_Create(4);
    return S_OK;
}

HRESULT CBaseBrowser2::InitializeTransitionSite()
{
    HRESULT hr;
    
    _ptrsite = new CTransitionSite(_psbOuter);
    if (_ptrsite == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

//
// DLM = DownLoad Manager
//
void CBaseBrowser2::_DLMUpdate(MSOCMD* prgCmd)
{
    ASSERT(prgCmd->cmdID == OLECMDID_STOPDOWNLOAD);
    for (int i=DPA_GetPtrCount(_hdpaDLM)-1; i>=0; i--) {
        IOleCommandTarget* pcmdt = (IOleCommandTarget*)DPA_GetPtr(_hdpaDLM, i);
        prgCmd->cmdf = 0;
        pcmdt->QueryStatus(NULL, 1, prgCmd, NULL);
        if (prgCmd->cmdf & MSOCMDF_ENABLED) {
            // We found one downloading guy, skip others. 
            break;
        } else {
            // This guy is no longer busy, remove it from the list,
            // and continue. 
            DPA_DeletePtr(_hdpaDLM, i);
            pcmdt->Release();
        }
    }
}

void CBaseBrowser2::_DLMRegister(IUnknown* punk)
{
    // Check if it's already registered. 
    for (int i=0; i<DPA_GetPtrCount(_hdpaDLM); i++) {
        IOleCommandTarget* pcmdt = (IOleCommandTarget*)DPA_GetPtr(_hdpaDLM, i);
        if (IsSameObject(pcmdt, punk)) {
            // Already registered, don't register.
            return;
        }
    }

    IOleCommandTarget* pcmdt;
    HRESULT hres = punk->QueryInterface(IID_IOleCommandTarget, (void **)&pcmdt);
    if (SUCCEEDED(hres)) {
        if (DPA_AppendPtr(_hdpaDLM, pcmdt) == -1) {
            pcmdt->Release();
        }
    }
}

//
// This function updates the _fDescendentNavigate flag.
//
// ALGORITHM:
//  If pvaragIn->lVal has some non-zero value, we set _fDescendentNavigate.
//  Otherwise, we ask the current view to see if it has something to stop.
// 
HRESULT CBaseBrowser2::_setDescendentNavigate(VARIANTARG *pvarargIn)
{
    ASSERT(!pvarargIn || pvarargIn->vt == VT_I4 || pvarargIn->vt == VT_BOOL || pvarargIn->vt == VT_UNKNOWN);
    if (!pvarargIn || !pvarargIn->lVal)
    {
        MSOCMD rgCmd;

        rgCmd.cmdID = OLECMDID_STOPDOWNLOAD;
        rgCmd.cmdf = 0;
        if (_bbd._pctView)
            _bbd._pctView->QueryStatus(NULL, 1, &rgCmd, NULL);

        //
        // If and only if the view says "I'm not navigating any more",
        // we'll ask the same question to each registered objects.
        //
        if (_hdpaDLM && !(rgCmd.cmdf & MSOCMDF_ENABLED)) {
            _DLMUpdate(&rgCmd);
        }

        _fDescendentNavigate = (rgCmd.cmdf & MSOCMDF_ENABLED) ? TRUE:FALSE;
    }
    else
    {
        if (_hdpaDLM && pvarargIn->vt == VT_UNKNOWN) {
            ASSERT(pvarargIn->punkVal);
            _DLMRegister(pvarargIn->punkVal);
        }
        _fDescendentNavigate = TRUE;
    }
    return S_OK;
}

void CBaseBrowser2::_CreateShortcutOnDesktop(BOOL fUI)
{
    if (!fUI || (MLShellMessageBox(
                                 _bbd._hwnd,
                                 MAKEINTRESOURCE(IDS_CREATE_SHORTCUT_MSG),
                                 MAKEINTRESOURCE(IDS_TITLE),
                                 MB_OKCANCEL) == IDOK))
    {
         TCHAR szPath[MAX_PATH];
         
         if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, TRUE))
         {
            TCHAR szName[MAX_URL_STRING];
            ISHCUT_PARAMS ShCutParams = {0};
            
            ShCutParams.pidlTarget = _bbd._pidlCur;
            if(_bbd._pszTitleCur)
            {
                StrCpyNW(szName, _bbd._pszTitleCur, ARRAYSIZE(szName));
                ShCutParams.pszTitle = szName;
            }
            else
            {
            	::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_INFOLDER, szName, SIZECHARS(szName), NULL);
            	ShCutParams.pszTitle = PathFindFileName(szName); 
            } 
            ShCutParams.pszDir = szPath; 
            ShCutParams.pszOut = NULL;
            ShCutParams.bUpdateProperties = FALSE;
            ShCutParams.bUniqueName = TRUE;
            ShCutParams.bUpdateIcon = TRUE;
            HRESULT hr;
            IWebBrowser *pwb;
            hr = QueryService(SID_SHlinkFrame, IID_IWebBrowser, (void **)&pwb);

            if(S_OK == hr)
            {
                IDispatch *pdisp = NULL;

                ASSERT(pwb);
                hr = pwb->QueryInterface(IID_IOleCommandTarget, (void **)(&(ShCutParams.pCommand)));
                ASSERT((S_OK == hr) && (BOOLIFY(ShCutParams.pCommand)));
                
                hr = pwb->get_Document(&pdisp);
                if(S_OK == hr)
                {
                    ASSERT(pdisp);
                    hr = pdisp->QueryInterface(IID_IHTMLDocument2, (void **)(&(ShCutParams.pDoc)));
                    pdisp->Release();
                }
                pwb->Release();
            }
            

            HRESULT hresT = CreateShortcutInDirEx(&ShCutParams);
            SAFERELEASE(ShCutParams.pDoc);
            SAFERELEASE(ShCutParams.pCommand);
            AssertMsg(SUCCEEDED(hresT), TEXT("CDOH::_CSOD CreateShortcutInDir failed %x"), hresT);
         } 
         else 
         {
             TraceMsg(DM_ERROR, "CSB::_CSOD SHGetSFP(DESKTOP) failed");
         }
    }
}


void CBaseBrowser2::_AddToFavorites(LPCITEMIDLIST pidl, LPCTSTR pszTitle, BOOL fDisplayUI)
{
    HRESULT hr;
    IWebBrowser *pwb = NULL;
    IOleCommandTarget *pcmdt = NULL;

    if(SHIsRestricted2W(_bbd._hwnd, REST_NoFavorites, NULL, 0))
        return;

    hr = QueryService(SID_SHlinkFrame, IID_IWebBrowser, (void **)&pwb);

    if(S_OK == hr)
    {
        ASSERT(pwb);

        hr = pwb->QueryInterface(IID_IOleCommandTarget, (void **)(&pcmdt));
        ASSERT((S_OK == hr) && (BOOLIFY(pcmdt)));
        
        pwb->Release();
    }

    //there's a small window where _pidlCur can be freed while AddToFavorites is coming up,
    // so use a local copy instead
    LPITEMIDLIST pidlCur = NULL;
    if (!pidl)
        pidlCur = ILClone(_bbd._pidlCur);

    if (pidl || pidlCur)
        AddToFavorites(_bbd._hwnd, pidl ? pidl : pidlCur, pszTitle, fDisplayUI, pcmdt, NULL);

    ILFree(pidlCur);

    SAFERELEASE(pcmdt);
}

HRESULT CBaseBrowser2::_OnCoCreateDocument(VARIANTARG *pvarargOut)
{
    HRESULT hres;

    //
    // Cache the class factory object and lock it (leave it loaded)
    //
    if (_pcfHTML == NULL) {
        TraceMsg(DM_COCREATEHTML, "CBB::_OnCoCreateDoc called first time (this=%x)", this);
        hres = CoGetClassObject(CLSID_HTMLDocument, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                    0, IID_IClassFactory, (void **)&_pcfHTML);
        if (SUCCEEDED(hres)) {
            hres = _pcfHTML->LockServer(TRUE);
            if (FAILED(hres)) {
                _pcfHTML->Release();
                _pcfHTML = NULL;
                return hres;
            }
        } else {
            return hres;
        }
    }

    TraceMsg(DM_COCREATEHTML, "CBB::_OnCoCreateDoc creating an instance (this=%x)", this);

    hres = _pcfHTML->CreateInstance(NULL, IID_IUnknown, (void **)&pvarargOut->punkVal);
    if (SUCCEEDED(hres)) {
        pvarargOut->vt = VT_UNKNOWN;
    } else {
        pvarargOut->vt = VT_EMPTY;
    }
    return hres;
}


// fill a buffer with a variant, return a pointer to that buffer on success of the conversion

LPCTSTR VariantToString(const VARIANT *pv, LPTSTR pszBuf, UINT cch)
{
    *pszBuf = 0;

    if (pv && pv->vt == VT_BSTR && pv->bstrVal)
    {
        StrCpyN(pszBuf, pv->bstrVal, cch);
        if (*pszBuf)
            return pszBuf;
    }
    return NULL;
}

HRESULT CBaseBrowser2::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, 
                           VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_NOTSUPPORTED;

    if (pguidCmdGroup == NULL) 
    {
        switch(nCmdID) 
        {
        // CBaseBrowser2 doesn't actually do the toolbar -- itbar does, forward this
        case OLECMDID_UPDATECOMMANDS:
            _NotifyCommandStateChange();
            hres = S_OK;
            break;

        case OLECMDID_SETDOWNLOADSTATE:

            ASSERT(pvarargIn);
            if (pvarargIn) 
            {
                _setDescendentNavigate(pvarargIn);
                hres = _updateNavigationUI();
            }
            else 
            {
                hres = E_INVALIDARG;
            }
            break;
            
        case OLECMDID_REFRESH:
            if (_bbd._pctView) // we must check!
                hres = _bbd._pctView->Exec(NULL, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            else if (_bbd._psv)
            {
                _bbd._psv->Refresh();
                hres = S_OK;
            }

            break;

        //
        //  When Exec(OLECMDID_STOP) is called either by the containee (the
        // current document) or the automation service object, we cancel
        // the pending navigation (if any), then tell the current document
        // to stop the go-going download in that page.
        //
        case OLECMDID_STOP:
            // If we are playing a transition, stop that instead of the navigate.
            if (_ptrsite && (_ptrsite->_uState == CTransitionSite::TRSTATE_PAINTING))
                _ptrsite->_StopTransition();
            else
            {
                // cant stop if we are modeless
                if(S_FALSE == _DisableModeless())
                {
                    LPITEMIDLIST pidlIntended = (_bbd._pidlPending) ? ILClone(_bbd._pidlPending) : NULL;
                    _CancelPendingNavigation();
                    // the _bbd._pctView gives us a _StopCurrentView()
                    _pbsOuter->_ExecChildren(_bbd._pctView, TRUE, NULL, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);   // Exec
                    if (!_bbd._pidlCur)
                    {
                        TCHAR   szResURL[MAX_URL_STRING];

                        hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                                 HINST_THISDLL,
                                                 ML_CROSSCODEPAGE,
                                                 TEXT("navcancl.htm"),
                                                 szResURL,
                                                 ARRAYSIZE(szResURL),
                                                 TEXT("shdocvw.dll"));
                        if (SUCCEEDED(hres))
                        {
                            _ShowBlankPage(szResURL, pidlIntended);
                        }
                    }
                    if(pidlIntended)
                    {
                        ILFree(pidlIntended);
                    }
                        
                }
            }

            hres = S_OK;
            break;

        // handled in basesb so IWebBrowser::ExecWB gets this
        // since this used to be in shbrowse, make sure we do
        // it only if _fTopBrowser
        case OLECMDID_FIND:
#ifndef UNIX

#define TBIDM_SEARCH            0x123 // defined in browseui\itbdrop.h

            // Check restriction here cuz Win95 didn't check in SHFindFiles like it now does.
            if (!SHRestricted(REST_NOFIND) && _fTopBrowser)
            {
                if (!_bbd._pctView || FAILED(_bbd._pctView->Exec(NULL, nCmdID, nCmdexecopt, pvarargIn, pvarargOut))) 
                {
                    if (GetUIVersion() >= 5 && pvarargIn)
                    {
                        if (pvarargIn->vt == VT_UNKNOWN)
                        {
                            ASSERT(pvarargIn->punkVal);

                            VARIANT  var = {0};
                            var.vt = VT_I4;
                            var.lVal = -1;
                            if (SUCCEEDED(IUnknown_Exec(pvarargIn->punkVal, &CLSID_CommonButtons, TBIDM_SEARCH, 0, NULL, &var)))
                                break;
                        }
                    }
                    SHFindFiles(_bbd._pidlCur, NULL);
                }
            }
#endif
            break;

        case OLECMDID_HTTPEQUIV_DONE:
        case OLECMDID_HTTPEQUIV:
            hres = OnHttpEquiv(_bbd._psv, (OLECMDID_HTTPEQUIV_DONE == nCmdID), pvarargIn, pvarargOut);
            break;

        // Binder prints by reflecting the print back down. do the same here
        // Note: we may want to do the same for _PRINTPREVIEW, _PROPERTIES, _STOP, etc.
        // The null command group should all go down, no need to stop these at the pass.
        default:
            if (_bbd._pctView) // we must check!
                hres = _bbd._pctView->Exec(NULL, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            else
                hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (IsEqualGUID(CGID_MSHTML, *pguidCmdGroup))
    {
        if (_bbd._pctView) // we must check!
            hres = _bbd._pctView->Exec(&CGID_MSHTML, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        else
            hres = OLECMDERR_E_NOTSUPPORTED;
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case SBCMDID_CREATESHORTCUT:
            _CreateShortcutOnDesktop(nCmdexecopt & OLECMDEXECOPT_PROMPTUSER);
            break;

        case SBCMDID_ADDTOFAVORITES:
            {
                LPITEMIDLIST pidl = NULL;

                //if someone doesn't pass a path in, _AddToFavorites will use the current page
                if ((pvarargIn != NULL) && (pvarargIn->vt == VT_BSTR))
                    IECreateFromPath(pvarargIn->bstrVal, &pidl);
                
                TCHAR szTitle[128];
                LPTSTR pszTitle = NULL;
                if (pvarargOut)
                    pszTitle = (LPTSTR)VariantToString(pvarargOut, szTitle, ARRAYSIZE(szTitle)); // may be NULL
                else
                {
                    if (_bbd._pszTitleCur)
                        pszTitle = StrCpyNW(szTitle, _bbd._pszTitleCur, ARRAYSIZE(szTitle));
                }

                _AddToFavorites(pidl, pszTitle, nCmdexecopt & OLECMDEXECOPT_PROMPTUSER);

                if (pidl)
                    ILFree(pidl);
                hres = S_OK;
            }
            break;

        case SBCMDID_OPTIONS:
            _DoOptions(pvarargIn);
            break;

        case SBCMDID_CANCELNAVIGATION:

            TraceMsg(DM_NAV, "ief NAV::%s called when _bbd._pidlCur==%x, _bbd._psvPending==%x",
                             TEXT("Exec(SBCMDID_CANCELNAV) called"),
                             _bbd._pidlCur, _bbd._psvPending);

            // Check if this is sync or async
            if (pvarargIn && pvarargIn->vt==VT_I4 && pvarargIn->lVal) {
                TraceMsg(DM_WEBCHECKDRT, "CBB::Exec calling _CancelPendingNavigation");
                _CancelPendingNavigation();
            } else {
                //
                //  We must call ASYNC version in this case because this call
                // is from the pending view itself.
                //
                LPITEMIDLIST pidlIntended = (_bbd._pidlPending) ? ILClone(_bbd._pidlPending) : NULL;
                _CancelPendingNavigationAsync();
                if (!_bbd._pidlCur)
                {
                    TCHAR   szResURL[MAX_URL_STRING];

                    if (IsGlobalOffline())
                    {
                        hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                                 HINST_THISDLL,
                                                 ML_CROSSCODEPAGE,
                                                 TEXT("offcancl.htm"),
                                                 szResURL,
                                                 ARRAYSIZE(szResURL),
                                                 TEXT("shdocvw.dll"));
                        if (SUCCEEDED(hres))
                        {
                            _ShowBlankPage(szResURL, pidlIntended);
                        }
                    }
                    else
                    {
                        hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                                 HINST_THISDLL,
                                                 ML_CROSSCODEPAGE,
                                                 TEXT("navcancl.htm"),
                                                 szResURL,
                                                 ARRAYSIZE(szResURL),
                                                 TEXT("shdocvw.dll"));
                        if (SUCCEEDED(hres))
                        {
                            _ShowBlankPage(szResURL, pidlIntended);
                        }
                    }
                }
                if(pidlIntended)
                    ILFree(pidlIntended);
            }
            hres = S_OK;
            break;

        case SBCMDID_ASYNCNAVIGATION:

            TraceMsg(DM_NAV, "ief NAV::%s called when _bbd._pidlCur==%x, _bbd._psvPending==%x",
                             TEXT("Exec(SBCMDID_ASYNCNAV) called"),
                             _bbd._pidlCur, _bbd._psvPending);

            //
            //  We must call ASYNC version in this case because this call
            // is from the pending view itself.
            //
            _SendAsyncNavigationMsg(pvarargIn);
            hres = S_OK;
            break;


        case SBCMDID_COCREATEDOCUMENT:
            hres = _OnCoCreateDocument(pvarargOut);
            break;

        case SBCMDID_HISTSFOLDER:
            if (pvarargOut) {
                VariantClearLazy(pvarargOut);
                if (NULL == _punkSFHistory)
                {
                    IHistSFPrivate *phsfHistory;

                    hres = LoadHistoryShellFolder(NULL, &phsfHistory);
                    if (SUCCEEDED(hres))
                    {
                        hres = phsfHistory->QueryInterface(IID_IUnknown, (void **) &_punkSFHistory);
                        phsfHistory->Release();
                    }
                }
                if (NULL != _punkSFHistory)
                {
                    pvarargOut->vt = VT_UNKNOWN;
                    pvarargOut->punkVal = _punkSFHistory;
                    _punkSFHistory->AddRef();
                }
            }
            break;

        case SBCMDID_UPDATETRAVELLOG:
            _UpdateTravelLog();
            // fall through

        case SBCMDID_REPLACELOCATION:
            if(pvarargIn && pvarargIn->vt == VT_BSTR)
            {
                WCHAR wzParsedUrl[MAX_URL_STRING];
                LPWSTR  pszUrl = pvarargIn->bstrVal;
                LPITEMIDLIST pidl;

                // BSTRs can be NULL.
                if (!pszUrl)
                    pszUrl = L"";

                // NOTE: This URL came from the user, so we need to clean it up.
                //       If the user entered "yahoo.com" or "Search Get Rich Quick",
                //       it will be turned into a search URL by ParseURLFromOutsideSourceW().
                DWORD cchParsedUrl = ARRAYSIZE(wzParsedUrl);
                if (!ParseURLFromOutsideSourceW(pszUrl, wzParsedUrl, &cchParsedUrl, NULL))
                {
                    StrCpyN(wzParsedUrl, pszUrl, ARRAYSIZE(wzParsedUrl));
                } 

                IEParseDisplayName(CP_ACP, wzParsedUrl, &pidl);
                if (pidl)
                {
                    NotifyRedirect(_bbd._psv, pidl, NULL);
                    ILFree(pidl);
                }
            }

            //  even if there was no url, still force no refresh.
            _fGeneratedPage = TRUE;
            //  force updating the back and forward buttons
            _pbsOuter->UpdateBackForwardState();
            hres = S_OK;
            break;
            
        default:
            hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case SHDVID_GOBACK:
            hres = _psbOuter->BrowseObject(NULL, SBSP_NAVIGATEBACK);
            break;

        case SHDVID_GOFORWARD:
            hres = _psbOuter->BrowseObject(NULL, SBSP_NAVIGATEFORWARD);
            break;

        // we reflect AMBIENTPROPCHANGE down because this is how iedisp notifies dochost
        // that an ambient property has changed. we don't need to reflect this down in
        // cwebbrowsersb because only the top-level iwebbrowser2 is allowed to change props
        case SHDVID_AMBIENTPROPCHANGE:
        case SHDVID_PRINTFRAME:
        case SHDVID_MIMECSETMENUOPEN:
        case SHDVID_FONTMENUOPEN:
        case SHDVID_DOCFAMILYCHARSET:
            if (_bbd._pctView) // we must check!
            {
                hres = _bbd._pctView->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            }
            else
                hres = E_FAIL;
            break;

        case SHDVID_DEACTIVATEMENOW:
            if (!_bbd._psvPending)
            {
                hres = S_OK;
                break;
            }
            //  fall through to activate new view
#ifdef FEATURE_PICS
        case SHDVID_ACTIVATEMENOW:
#endif
            _ActivatePendingViewAsync();
            hres = S_OK;
            break;


        case SHDVID_GETPENDINGOBJECT:
            ASSERT( pvarargOut);
            if (_bbd._psvPending && ((pvarargIn && pvarargIn->vt == VT_BOOL && pvarargIn->boolVal) || !_bbd._psv))
            {
                VariantClearLazy(pvarargOut);
                _bbd._psvPending->QueryInterface(IID_IUnknown, (void **) &(pvarargOut->punkVal));
                if (pvarargOut->punkVal) pvarargOut->vt = VT_UNKNOWN;
            }
            hres = (pvarargOut->punkVal == NULL) ? E_FAIL : S_OK;
            break;

 
        case SHDVID_SETPRINTSTATUS:
            if (pvarargIn && pvarargIn->vt == VT_BOOL)
            {
                VARIANTARG var = {0};
                if (_bbd._pctView && SUCCEEDED(_bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_PRINTER, NULL, &var))
                     && V_UI4(&var) != PANE_NONE)
                    _psbOuter->SendControlMsg(FCW_STATUS, SB_SETICON, V_UI4(&var), 
                                  (LPARAM)(pvarargIn->boolVal ? g_hiconPrinter : NULL), NULL);
            }                  
            break;

#ifdef FEATURE_PICS
        /* Dochost sends up this command to have us put up the PICS access
         * denied dialog.  This is done so that all calls to this ratings
         * API are modal to the top-level browser window;  that in turn
         * lets the ratings code coalesce denials for all subframes into
         * a single dialog.
         */
        case SHDVID_PICSBLOCKINGUI:
            {
                void * pDetails;
                if (pvarargIn && pvarargIn->vt == VT_INT_PTR)
                    pDetails = pvarargIn->byref;
                else
                    pDetails = NULL;
                TraceMsg(DM_PICS, "CBaseBrowser2::Exec calling RatingAccessDeniedDialog2");
                hres = RatingAccessDeniedDialog2(_bbd._hwnd, NULL, pDetails);
            }
            break;
#endif

       case SHDVID_ONCOLORSCHANGE:
            // PALETTE:
            // PALETTE: recompute our palette
            // PALETTE:
            _ColorsDirty(BPT_UnknownPalette);
            break;

        case SHDVID_GETOPTIONSHWND:
        {
            ASSERT(pvarargOut != NULL);
            ASSERT(V_VT(pvarargOut) == VT_BYREF);


            // return the hwnd for the inet options
            // modal prop sheet. we're tracking
            // this hwnd because if it's open
            // and plugUI shutdown needs to happen,
            // that dialog needs to receive a WM_CLOSE
            // before we can nuke it

            hres = E_FAIL;

            // is there a list of window handles?

            ENTERCRITICAL;

            if (s_hdpaOptionsHwnd != NULL)
            {
                int cPtr = DPA_GetPtrCount(s_hdpaOptionsHwnd);
                // is that list nonempty?
                if (cPtr > 0)
                {
                    HWND hwndOptions = (HWND)DPA_GetPtr(s_hdpaOptionsHwnd, 0);
                    ASSERT(hwndOptions != NULL);

                    pvarargOut->byref = hwndOptions;

                    // remove it from the list
                    // that hwnd is not our responsibility anymore
                    DPA_DeletePtr(s_hdpaOptionsHwnd, 0);

                    // successful hwnd retrieval
                    hres = S_OK;
                }
            }

            LEAVECRITICAL;
        }
        break;

        case SHDVID_DISPLAYSCRIPTERRORS:
        {
            HRESULT hr;

            hr = _bbd._pctView->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

            return hr;
        }
        break;

       default:
            hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (IsEqualGUID(CGID_ShortCut, *pguidCmdGroup))
    {
        if (_bbd._pctView) // we must check!
            hres = _bbd._pctView->Exec(&CGID_ShortCut, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        else
            hres = OLECMDERR_E_NOTSUPPORTED;
    } 
    else
    {
        hres = OLECMDERR_E_UNKNOWNGROUP;
    }
    return hres;
}


HRESULT CBaseBrowser2::ParseDisplayName(IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{

    TraceMsg(0, "sdv TR ::ParseDisplayName called");
    *ppmkOut = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::EnumObjects( DWORD grfFlags, IEnumUnknown **ppenum)
{
    TraceMsg(0, "sdv TR ::EnumObjects called");
    *ppenum = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::LockContainer( BOOL fLock)
{
    TraceMsg(0, "sdv TR ::LockContainer called");
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::SetTitle(IShellView* psv, LPCWSTR lpszName)
{
    LPWSTR *ppszName = NULL;
    BOOL fFireEvent = FALSE;
    LPITEMIDLIST pidl;

    // We need to forward title changes on to the automation interface.
    // But since title changes can technically occur at any time we need
    // to distinguish between current and pending title changes and only
    // pass on the current title change now. We'll pass on the pending
    // title change at NavigateComplete time. (This also lets us identify
    // when we navigate to non-SetTitle object (such as the shell) and
    // simulate a TitleChange event.)
    //
    // Since the DocObjectHost needs to retrieve the title later, we
    // hold onto the current view's title change so they don't have to.
    //

    // Figure out which object is changing.
    //
    if (IsSameObject(_bbd._psv, psv))
    {
        ppszName = &_bbd._pszTitleCur;
        pidl = _bbd._pidlCur;
        fFireEvent = TRUE;
    }
    else if (EVAL(IsSameObject(_bbd._psvPending, psv) || !_bbd._psvPending)) // no pending probably means we're in _MayPlayTransition
    {
        ppszName = &_bbd._pszTitlePending;
        pidl = _bbd._pidlPending;
        // If we have no current guy, might as well set the title early
        fFireEvent = !_bbd._psv;
    }
    else
    {
        ppszName = NULL;
        pidl = NULL;        // init pidl to suppress bogus C4701 warning
    }

    if (ppszName)
    {
        UINT cchLen = lstrlenW(lpszName) + 1; // +1 for NULL
        UINT cbAlloc;

        // For some reason we cap the length of this string. We can't cap
        // less than MAX_PATH because we need to handle filesys names.
        //
        if (cchLen > MAX_PATH)
            cchLen = MAX_PATH;

        // We want to allocate at least a medium size string because
        // many web pages script the title one character at a time.
        //
#define MIN_TITLE_ALLOC  64
        if (cchLen < MIN_TITLE_ALLOC)
            cbAlloc = MIN_TITLE_ALLOC * SIZEOF(*lpszName);
        else
            cbAlloc = cchLen * SIZEOF(*lpszName);
#undef  MIN_TITLE_ALLOC

        // Do we need to allocate?
        if (!(*ppszName) || LocalSize((HLOCAL)(*ppszName)) < cbAlloc)
        {
            // Free up Old Title
            if(*ppszName)
                LocalFree((void *)(*ppszName));
                
            *ppszName = (LPWSTR)LocalAlloc(LPTR, cbAlloc);
        }

        if (*ppszName)
        {
            StrCpyNW(*ppszName, lpszName, cchLen);

            if (fFireEvent)
            {
                DWORD dwOptions;

                FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, *ppszName);

                // If this is a desktop component, try to update the friendly name if necessary.
                if (!_fCheckedDesktopComponentName)
                {
                    _fCheckedDesktopComponentName = TRUE;
                    if (SUCCEEDED(GetTopFrameOptions(_pspOuter, &dwOptions)) && (dwOptions & FRAMEOPTIONS_DESKTOP))
                        UpdateDesktopComponentName(pidl, lpszName);
                }
            }
        }
    }

    return NOERROR;
}
HRESULT CBaseBrowser2::GetTitle(IShellView* psv, LPWSTR pszName, DWORD cchName)
{
    LPWSTR psz;

    if (!psv || IsSameObject(_bbd._psv, psv))
    {
        psz = _bbd._pszTitleCur;
    }
    else if (EVAL(IsSameObject(_bbd._psvPending, psv) || !_bbd._psvPending))
    {
        psz = _bbd._pszTitlePending;
    }
    else
    {
        psz = NULL;
    }

    if (psz)
    {
        StrCpyNW(pszName, psz, cchName);
        return(S_OK);
    }
    else
    {
        *pszName = 0;
        return(E_FAIL);
    }
}

HRESULT CBaseBrowser2::GetParentSite(struct IOleInPlaceSite** ppipsite)
{
    *ppipsite = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::GetOleObject(struct IOleObject** ppobjv)
{
    *ppobjv = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::NotifyRedirect(IShellView * psv, LPCITEMIDLIST pidlNew, BOOL *pfDidBrowse)
{
    HRESULT hres = E_FAIL;
    
    if (pfDidBrowse)
        *pfDidBrowse = FALSE;

    if (IsSameObject(psv, _bbd._psv) ||
        IsSameObject(psv, _bbd._psvPending))
    {
        hres = _pbsOuter->_TryShell2Rename(psv, pidlNew);
        if (FAILED(hres)) 
        {
            // if we weren't able to just swap it, we've got to browse to it
            // but pass redirect so that we don't add a navigation stack item
            //
            // NOTE: the above comment is a bit old since we don't pass
            // redirect here. If we ever start passing redirect here,
            // we'll confuse ISVs relying on the NavigateComplete event
            // exactly mirroring when navigations enter the navigation stack.
            //
            hres = _psbOuter->BrowseObject(pidlNew, SBSP_WRITENOHISTORY | SBSP_SAMEBROWSER);

            if(pfDidBrowse)
                *pfDidBrowse = TRUE;
        }
    }

    return hres;
}

HRESULT CBaseBrowser2::SetFlags(DWORD dwFlags, DWORD dwFlagMask)
{
    if (dwFlagMask & BSF_REGISTERASDROPTARGET)
    {
        _fNoDragDrop = (!(dwFlags & BSF_REGISTERASDROPTARGET)) ? TRUE : FALSE;
        if (!_fNoDragDrop)
            _RegisterAsDropTarget();
        else
            _UnregisterAsDropTarget();
    }
    return S_OK;
}

HRESULT CBaseBrowser2::GetFlags(DWORD *pdwFlags)
{
    DWORD dwFlags = 0;

    if (!_fNoDragDrop)
        dwFlags |= BSF_REGISTERASDROPTARGET;
    *pdwFlags = dwFlags;

    return S_OK;
}


HRESULT CBaseBrowser2::UpdateWindowList(void)
{
    // code used to assert, but in WebBrowserOC cases we can get here.
    return E_UNEXPECTED;
}

STDMETHODIMP CBaseBrowser2::IsControlWindowShown(UINT id, BOOL *pfShown)
{
    if (pfShown)
        *pfShown = FALSE;
    return E_NOTIMPL;
}

STDMETHODIMP CBaseBrowser2::ShowControlWindow(UINT id, BOOL fShow)
{
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags)
{
    return ::IEGetDisplayName(pidl, pwszName, uFlags);
}

HRESULT CBaseBrowser2::IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut)
{
    HRESULT hr;
    IBindCtx * pbc = NULL;    
    WCHAR wzParsedUrl[MAX_URL_STRING];

    // NOTE: This URL came from the user, so we need to clean it up.
    //       If the user entered "yahoo.com" or "Search Get Rich Quick",
    //       it will be turned into a search URL by ParseURLFromOutsideSourceW().
    DWORD cchParsedUrl = ARRAYSIZE(wzParsedUrl);
    if (!ParseURLFromOutsideSourceW(pwszPath, wzParsedUrl, &cchParsedUrl, NULL))
    {
        StrCpyN(wzParsedUrl, pwszPath, ARRAYSIZE(wzParsedUrl));
    } 

    // This is currently used for FTP, so we only do it for FTP for perf reasons.
    if (URL_SCHEME_FTP == GetUrlSchemeW(wzParsedUrl))
        pbc = CreateBindCtxForUI(SAFECAST(this, IOleContainer *));  // We really want to cast to (IUnknown *) but that's ambiguous.
    
    hr = IEParseDisplayNameWithBCW(uiCP, wzParsedUrl, pbc, ppidlOut);
    ATOMICRELEASE(pbc);

    return hr;
}

HRESULT _DisplayParseError(HWND hwnd, HRESULT hres, LPCWSTR pwszPath)
{
    if (FAILED(hres)
        && hres != E_OUTOFMEMORY
        && hres != HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        TCHAR szPath[MAX_URL_STRING];
        OleStrToStrN(szPath, ARRAYSIZE(szPath), pwszPath, (UINT)-1);
        MLShellMessageBox(hwnd,
                        MAKEINTRESOURCE(IDS_ERROR_GOTO),
                        MAKEINTRESOURCE(IDS_TITLE),
                        MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                        szPath);

        hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    return hres;
}

HRESULT CBaseBrowser2::DisplayParseError(HRESULT hres, LPCWSTR pwszPath)
{
    return _DisplayParseError(_bbd._hwnd, hres, pwszPath);
}

HRESULT CBaseBrowser2::_CheckZoneCrossing(LPCITEMIDLIST pidl)
{
    return _pbsOuter->v_CheckZoneCrossing(pidl);
}


// if in global offline mode and this item requires net access and it is
// not in the cache put up UI to go online.
//
// returns:
//      S_OK        URL is ready to be accessed
//      E_ABORT     user canceled the UI

HRESULT CBaseBrowser2::_CheckInCacheIfOffline(LPCITEMIDLIST pidl, BOOL fIsAPost)
{
    HRESULT hr = S_OK;      // assume it is
    VARIANT_BOOL fFrameIsSilent;
    VARIANT_BOOL fFrameHasAmbientOfflineMode;

    EVAL(SUCCEEDED(_bbd._pautoWB2->get_Silent(&fFrameHasAmbientOfflineMode)));    // should always work

    EVAL(SUCCEEDED(_bbd._pautoWB2->get_Offline(&fFrameIsSilent)));   
    if ((fFrameIsSilent == VARIANT_FALSE) &&
        (fFrameHasAmbientOfflineMode == VARIANT_FALSE)&&
        pidl && (pidl != PIDL_NOTHING) && (pidl != PIDL_LOCALHISTORY) && 
        IsBrowserFrameOptionsPidlSet(pidl, BFO_USE_IE_OFFLINE_SUPPORT) && 
        IsGlobalOffline()) 
    {
        TCHAR szURL[MAX_URL_STRING];
        EVAL(SUCCEEDED(::IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szURL, SIZECHARS(szURL), NULL)));

        if (UrlHitsNet(szURL) && ((!UrlIsMappedOrInCache(szURL)) || fIsAPost))
        {
            // UI to allow user to go on-line
            HWND hParentWnd = NULL; // init to suppress bogus C4701 warning

            hr = E_FAIL;
            if(!_fTopBrowser)
            {
               IOleWindow *pOleWindow;
               hr = _pspOuter->QueryService(SID_STopLevelBrowser, IID_IOleWindow, (void **)&pOleWindow);
               if(SUCCEEDED(hr))
               { 
                    ASSERT(pOleWindow);
                    hr = pOleWindow->GetWindow(&hParentWnd);
                    pOleWindow->Release();
               }
            }
            
            if(S_OK != hr)
            {
                hr = S_OK;
                hParentWnd = _bbd._hwnd;
            }


            _psbOuter->EnableModelessSB(FALSE);
            if (InternetGoOnline(szURL, hParentWnd, FALSE))
            {
                // Tell all browser windows to update their title and status pane
                SendShellIEBroadcastMessage(WM_WININICHANGE,0,0, 1000); 
            }    
            else
                hr = E_ABORT;   // user abort case...

            _psbOuter->EnableModelessSB(TRUE);
        }
    }

    return hr;
}


// this does all the preliminary checking of whether we can navigate to pidl or not.
// then if all is ok, we do the navigate with CreateNewShellViewPidl
HRESULT CBaseBrowser2::_NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD fSBSP)
{
    HRESULT hres;
    BOOL fCanceledDueToOffline = FALSE;
    BOOL fIsAPost = FALSE;  
    LPITEMIDLIST pidlFree = NULL;

    //
    // If we are processing a modal dialog, don't process it.
    //
    // NOTES: Checking _cRefCannotNavigate is supposed to be enough, but
    //  since we are dealing with random ActiveX objects, we'd better be
    //  robust. That's why we check IsWindowEnabled as well.
    //
    if ((S_OK ==_DisableModeless()) || !IsWindowEnabled(_bbd._hwnd)) {
        TraceMsg(DM_ENABLEMODELESS, "CSB::_NavigateToPidl returning ERROR_BUSY");
        hres = HRESULT_FROM_WIN32(ERROR_BUSY);
        goto Done;
    }

    TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_NavigateToPidl called"), pidl, grfHLNF);
    // used to be we would pull a NULL out of the 
    // the TravelLog, but i dont think that happens anymore
    ASSERT(pidl);  // Get ZEKEL

    // Sometimes we are navigated to the INTERNET shell folder
    // if this is the case, we really want to goto the Start Page.
    // This case only happens if you select "Internet Explorer" from the 
    // Folder Explorer Band.
    if (IsBrowserFrameOptionsPidlSet(pidl, BFO_SUBSTITUE_INTERNET_START_PAGE))
    {
        TCHAR szHome[MAX_URL_STRING];
        hres = _GetStdLocation(szHome, ARRAYSIZE(szHome), DVIDM_GOHOME);
        if (SUCCEEDED(hres))
        {
            hres = IECreateFromPath(szHome, &pidlFree);
            if (SUCCEEDED(hres))
                pidl = pidlFree;
        }

        if (FAILED(hres))
            goto Done;
    }

    hres = _FireBeforeNavigateEvent(pidl, &fIsAPost);
    if (hres == E_ABORT)
        goto Done;   // event handler told us to cancel

#ifdef UNIX_DISABLE_LOCAL_FOLDER
    BOOL fCanceledDueToLocalFolder = FALSE;
    if (IsLocalFolderPidl( pidl ))
    {
        hres = E_FAIL;
        fCanceledDueToLocalFolder = TRUE;
        goto Done;
    }
#endif

    // if we can't go here (?), cancel the navigation
    hres = _CheckZoneCrossing(pidl);
    if (hres != S_OK)
        goto Done;
        
    TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_CreateNewShellViewPidl called"), pidl, grfHLNF);

    //
    // Now that we are actually navigating...
    //

    // tell the frame to cancel the current navigation
    // and tell it about history navigate options as it will not be getting it
    // from subsequent call to Navigate
    //
    if (_bbd._phlf) {
        _bbd._phlf->Navigate(grfHLNF&(SHHLNF_WRITENOHISTORY|SHHLNF_NOAUTOSELECT), NULL, NULL, NULL);
    }

    hres = _CheckInCacheIfOffline(pidl, fIsAPost);
    if (hres != S_OK) 
    {
        fCanceledDueToOffline = TRUE;
        goto Done;
    }


    //
    //  if we goto the current page, we still do a full navigate
    //  but we dont want to create a new entry.
    //
    //  **EXCEPTIONS**
    //  if this was a generated page, ie Trident did doc.writes(),
    //  we need to always create a travel entry, because trident 
    //  can rename the pidl, but it wont actually be that page.
    //
    //  if this was a post then we need to create a travelentry.
    //  however if it was a travel back/fore, it will already have
    //  set the the bit, so we still wont create a new entry.
    //
    //
    //  NOTE: this is similar to a refresh, in that it reparses
    //  the entire page, but creates no travel entry.
    //
    if (   !_fDontAddTravelEntry                 // If the flag is already set, short circuit the rest
        && !fIsAPost                             // ...and not a Post
        && !_fGeneratedPage                      // ...and not a Generated Page
        && !(grfHLNF & HLNF_CREATENOHISTORY)     // ...and the CREATENOHISTORY flag is NOT set
        && pidl                                  // ...and we have a pidl to navigate to
        && _bbd._pidlCur                         // ...as well as a current pidl
        && ILIsEqual(pidl, _bbd._pidlCur)        // ...and the pidls are equal
        )
        _fDontAddTravelEntry = TRUE;             // Then set the DontAddTravelEntry flag.

    TraceMsg(TF_TRAVELLOG, "CBB:_NavToPidl() _fDontAddTravelEntry = %d", _fDontAddTravelEntry);


    hres = _CreateNewShellViewPidl(pidl, grfHLNF, fSBSP);

    //  this needs to be reset after the Create is finished, because it affects
    //  the behavior of SetHistoryObject() and prevents the reuse of the existing DOH
    _fGeneratedPage = FALSE;

Done:
    if (FAILED(hres))
    {
        TraceMsg(DM_WARNING, "CSB::_NavigateToPidl _CreateNewShellViewPidl failed %x", hres);

        // On failure we won't hit _ActivatePendingView
        OnReadyStateChange(NULL, READYSTATE_COMPLETE);

        //  if this was navigation via ITravelLog, 
        //  this will revert us to the original position
        if (_fDontAddTravelEntry)
        {
            ITravelLog *ptl;
        
            if(SUCCEEDED(GetTravelLog(&ptl)))
            {
                ptl->Revert();
                ptl->Release();
            }
            _fDontAddTravelEntry = FALSE;
            ASSERT(!_fIsLocalAnchor);

            ATOMICRELEASE(_poleHistory);
            ATOMICRELEASE(_pbcHistory);
            ATOMICRELEASE(_pstmHistory);
        }
        _pbsOuter->UpdateBackForwardState();

        //  we failed and have nothing to show for it...
        if (!_bbd._pidlCur && !_fNavigatedToBlank)
        {
            TCHAR szResURL[MAX_URL_STRING];

            if (fCanceledDueToOffline)
            {
                hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                         HINST_THISDLL,
                                         ML_CROSSCODEPAGE,
                                         TEXT("offcancl.htm"),
                                         szResURL,
                                         ARRAYSIZE(szResURL),
                                         TEXT("shdocvw.dll"));
                if (SUCCEEDED(hres))
                {
                    _ShowBlankPage(szResURL, pidl);
                }
            }
            else
            {
                // NT #274562: We only want to navigate to the
                //    about:NavigationCancelled page if it wasn't
                //    a navigation to file path. (UNC or Drive).
                //    The main reason for this is that if the user
                //    enters "\\unc\share" into Start->Run and
                //    the window can't successfully navigate to the
                //    share because permissions don't allow it, we
                //    want to close the window after the user hits
                //    [Cancel] in the [Retry][Cancel] dialog.  This
                //    is to prevent the shell from appearing to have
                //    shell integrated bugs and to be compatible with
                //    the old shell.
                DWORD dwAttrib = SFGAO_FILESYSTEM;
                if (SUCCEEDED(IEGetAttributesOf(pidl, &dwAttrib)) &&
                    !(SFGAO_FILESYSTEM & dwAttrib))
                {
                    hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                             HINST_THISDLL,
                                             ML_CROSSCODEPAGE,
                                             TEXT("navcancl.htm"),
                                             szResURL,
                                             ARRAYSIZE(szResURL),
                                             TEXT("shdocvw.dll"));
                    if (SUCCEEDED(hres))
                    {
                        _ShowBlankPage(szResURL, pidl);
                    }
                }
            }
        }

#ifdef  UNIX_DISABLE_LOCAL_FOLDER
    if (fCanceledDueToLocalFolder)
    {
        // IEUNIX : since we are not making use of the url in the
        // about folder browsing page, we should not append it to 
        // the url to the error url as it is not handled properly
        // by about:folderbrowsing
        _ShowBlankPage(FOLDERBROWSINGINFO_URL, NULL);
        _fNavigatedToBlank = FALSE;
    }
#endif
        
    }

    ILFree(pidlFree);
    
    return hres;
}

HRESULT CBaseBrowser2::OnReadyStateChange(IShellView* psvSource, DWORD dwReadyState)
{
    BOOL fChange = FALSE;

    if (psvSource)
    {
        if (IsSameObject(psvSource, _bbd._psv))
        {
            TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(Current, %d)", this, dwReadyState);
            fChange = (_dwReadyStateCur != dwReadyState);
            _dwReadyStateCur = dwReadyState;
            if ((READYSTATE_COMPLETE == dwReadyState) && !_fReleasingShellView)
                _Exec_psbMixedZone();
        }
        else if (IsSameObject(psvSource, _bbd._psvPending))
        {
            TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(Pending, %d)", this, dwReadyState);
            fChange = (_dwReadyStatePending != dwReadyState);
            _dwReadyStatePending = dwReadyState;
        }
        else if (EVAL(!_bbd._psvPending))
        {
            // Assume psvSource != _bbd._psv && NULL==_bbd._psvPending
            // means that _SwitchActivationNow is in the middle
            // of _MayPlayTransition's message loop and the
            // _bbd._psvPending dude is updating us.
            //
            // NOTE: We don't fire the event because get_ReadyState
            // can't figure this out. We know we will eventually
            // fire the event because CBaseBrowser2 will go to _COMPLETE
            // after _SwitchActivationNow.
            //
            TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(ASSUMED Pending, %d)", this, dwReadyState);
            _dwReadyStatePending = dwReadyState;
       }
    }
    else
    {
        // We use this function when our own simulated
        // ReadyState changes
        //
        TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(Self, %d)", this, dwReadyState);
        fChange = (_dwReadyState != dwReadyState);
        _dwReadyState = dwReadyState;
    }

    // No sense in firing events if nothing actually changed...
    //
    if (fChange && _bbd._pautoEDS)
    {
        DWORD dw;

        IUnknown_CPContainerOnChanged(_pauto, DISPID_READYSTATE);

        // if we at Complete, fire the event
        get_ReadyState(&dw);
        if (READYSTATE_COMPLETE == dw)
        {
            FireEvent_DocumentComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur);

            //  if we hit this, we have not picked up the history object we created.
            //  get ZekeL
            AssertMsg(_fDontAddTravelEntry || !_poleHistory, TEXT("CBB::OnDocComplete: nobody picked up _poleHistory - Get ZekeL"));

            if (g_dwProfileCAP & 0x00080000) {
                StopCAP();
            }
        }
    }

    return S_OK;
}

HRESULT CBaseBrowser2::get_ReadyState(DWORD * pdwReadyState)
{
    DWORD dw = _dwReadyState;

    if (_bbd._psvPending && _dwReadyStatePending < dw)
    {
        dw = _dwReadyStatePending;
    }

    if (_bbd._psv && _dwReadyStateCur < dw)
    {
        dw = _dwReadyStateCur;
    }

    *pdwReadyState = dw;

    return S_OK;
}

HRESULT CBaseBrowser2::_updateNavigationUI()
{

    if (_fNavigate || _fDescendentNavigate)
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        if (!_fDownloadSet)
        {
            FireEvent_DoInvokeDispid(_bbd._pautoEDS, DISPID_DOWNLOADBEGIN);
            _fDownloadSet = TRUE;
        }
    }
    else
    {
        if (_fDownloadSet)
        {
            FireEvent_DoInvokeDispid(_bbd._pautoEDS, DISPID_DOWNLOADCOMPLETE);
            _fDownloadSet = FALSE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    return S_OK;
}

HRESULT CBaseBrowser2::SetNavigateState(BNSTATE bnstate)
{

    switch (bnstate) {
    case BNS_BEGIN_NAVIGATE:
    case BNS_NAVIGATE:
        _KillRefreshTimer();   // halt any pending client pull operations
        _fNavigate = TRUE;
        _updateNavigationUI();
        break;

    case BNS_NORMAL:
        _fNavigate = FALSE;
        _updateNavigationUI();
        break;
    }
    return S_OK;
}


HRESULT CBaseBrowser2::GetNavigateState(BNSTATE *pbnstate)
{
    // Return Navigate if we are processing a navigation or if
    // we are processing a modal dialog.
    //
    // NOTES: Checking _cRefCannotNavigate is supposed to be enough, but
    //  since we are dealing with random ActiveX objects, we'd better be
    //  robust. That's why we check IsWindowEnabled as well.
    //
    *pbnstate = (_fNavigate || (S_OK ==_DisableModeless()) || _fDescendentNavigate ||
            !IsWindowEnabled(_bbd._hwnd)) ? BNS_NAVIGATE : BNS_NORMAL;
    return S_OK;
}

HRESULT CBaseBrowser2::UpdateBackForwardState(void)
{
    if (_fTopBrowser) {
        _UpdateBackForwardState();
    } else {
    
        // sigh, BrowserBand now makes this fire
        //ASSERT(_fTopBrowser);
        IBrowserService *pbs = NULL;
        TraceMsg(TF_SHDNAVIGATE, "cbb.ohlfn: !_fTopBrowser (BrowserBand?)");
        if(SUCCEEDED(_pspOuter->QueryService(SID_STopFrameBrowser, IID_IBrowserService, (void **)&pbs))) {

            ASSERT(pbs);
            BOOL fTopFrameBrowser = IsSameObject(pbs, SAFECAST(this, IShellBrowser *));
            if (!fTopFrameBrowser)
                pbs->UpdateBackForwardState();
            else 
                _UpdateBackForwardState();
            pbs->Release();        
        }
    }
    return S_OK;
}


HRESULT CBaseBrowser2::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL;

    //
    // NOTES: Notice that CBaseBrowser2 directly expose the automation
    //  service object via QueryService. CWebBrowserSB will appropriately
    //  dispatch those. See comments on CWebBrowserSB::QueryService for
    //  detail. (SatoNa)
    //
    if (IsEqualGUID(guidService, SID_SWebBrowserApp) || 
        IsEqualGUID(guidService, SID_SContainerDispatch) || 
        IsEqualGUID(guidService, IID_IExpDispSupport))
    {
        return _bbd._pautoSS->QueryInterface(riid, ppvObj);
    }

    if (IsEqualGUID(guidService, SID_SHlinkFrame) || 
        IsEqualGUID(guidService, IID_ITargetFrame2) || 
        IsEqualGUID(guidService, IID_ITargetFrame)) 
    {
        return _ptfrm->QueryInterface(riid, ppvObj);
    }
    else if (IsEqualGUID(guidService, SID_STopLevelBrowser)
        || IsEqualGUID(guidService, SID_STopFrameBrowser))
    {
        if (IsEqualGUID(riid, IID_IUrlHistoryStg))
        {
            ASSERT(_fTopBrowser);
            hres = S_OK;
            // create this object the first time it's asked for
            if (!_pIUrlHistoryStg)
            {
                hres = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER,
                        IID_IUrlHistoryStg, (void **)&_pIUrlHistoryStg);
                    
            }
            if (_pIUrlHistoryStg)
            {
                *ppvObj = _pIUrlHistoryStg;
                _pIUrlHistoryStg->AddRef();
                return S_OK;          
            }

            return hres;
        }

        // This code should all migrate to a helper object after IE5B2.  So this
        // should be temporary (stevepro).
        if (IsEqualGUID(riid, IID_IToolbarExt))
        {
            if (NULL == _pToolbarExt)
            {
                IUnknown* punk;
                if (SUCCEEDED(CBrowserExtension_CreateInstance(NULL, &punk, NULL)))
                {
                    IUnknown_SetSite(punk, _psbOuter);
                    punk->QueryInterface(riid, (LPVOID*)&_pToolbarExt);
                    punk->Release();
                }
            }
            if (_pToolbarExt)
            {
                *ppvObj = (LPVOID*)_pToolbarExt;
                _pToolbarExt->AddRef();
                hres = S_OK;
            }

            return hres;
        }

        return QueryInterface(riid, ppvObj);
    }
    else if (IsEqualGUID(guidService, SID_SUrlHistory)) //BUGBUG - Eliminate this service group -- BharatS
    {
        
        if (!_pIUrlHistoryStg)
        {
            IUrlHistoryStg *puhs = NULL;
            // Asking for it creates a copy in _pIUrlHistoryStg
            hres = _pspOuter->QueryService(SID_STopLevelBrowser, IID_IUrlHistoryStg, (void **)&puhs);
            if(SUCCEEDED(hres) && puhs){
                if(!_pIUrlHistoryStg)
                    _pIUrlHistoryStg = puhs;
                else
                    puhs->Release();
            }
        }
        
        if (_pIUrlHistoryStg)
            hres = _pIUrlHistoryStg->QueryInterface(riid, ppvObj);
        
        return hres;
    }
    else if (  IsEqualGUID(guidService, SID_SShellBrowser)
            || IsEqualGUID(guidService, SID_SVersionHost)
            ) 
    {
        if (IsEqualIID(riid, IID_IHlinkFrame)) 
        {
            // HACK: MSHTML uses IID_IShellBrowser instead of SID_SHlinkFrame
            return _pspOuter->QueryService(SID_SHlinkFrame, riid, ppvObj);
        } 
        else if (IsEqualIID(riid, IID_IBindCtx) && _bbd._phlf) 
        {
            //
            // HACK ALERT: Notice that we are using QueryService to the
            //  other direction here. We must make it absolutely sure
            //  that we'll never infinitly QueryService each other.
            //
            
            IUnknown_QueryService(_bbd._phlf, IID_IHlinkFrame, riid, ppvObj);
        } 
        else 
        {
            return QueryInterface(riid, ppvObj);
        }
    }
    else if (IsEqualGUID(guidService, SID_SOmWindow))
    {
        //
        // HACK ALERT: Notice that we are using QueryService to the
        //  other direction here. We must make it absolutely sure
        //  that we'll never infinitly QueryService each other.
        //
        hres = IUnknown_QueryService(_ptfrm, SID_SOmWindow, riid, ppvObj);
    }

    if (*ppvObj)
        return NOERROR;

    return E_FAIL;
}


void CBaseBrowser2::OnDataChange(FORMATETC *, STGMEDIUM *)
{
}

void CBaseBrowser2::OnViewChange(DWORD dwAspect, LONG lindex)
{
    _ViewChange(dwAspect, lindex);
}

void CBaseBrowser2::OnRename(IMoniker *)
{
}

void CBaseBrowser2::OnSave()
{
}

void CBaseBrowser2::OnClose()
{
    _ViewChange(DVASPECT_CONTENT, -1);
}


// *** IDropTarget ***

// These methods are defined in shdocvw.cpp
extern DWORD CommonDragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt);


// Use the ShellView IDropTarget functions whenever they are implemented


/*----------------------------------------------------------
Purpose: IDropTarget::DragEnter

*/
HRESULT CBaseBrowser2::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if (_pdtView)
        return _pdtView->DragEnter(pdtobj, grfKeyState, ptl, pdwEffect);
    else {
        if (_fNoDragDrop)
        {
            _dwDropEffect = DROPEFFECT_NONE;
        }
        else
        {
            _dwDropEffect = CommonDragEnter(pdtobj, grfKeyState, ptl);
        }
        *pdwEffect &= _dwDropEffect;
    }
    return S_OK;
}

/*----------------------------------------------------------
Purpose: IDropTarget::DragOver

*/
HRESULT CBaseBrowser2::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if (S_OK == _DisableModeless()) {
        *pdwEffect = 0;
        return S_OK;
    }

    if (_pdtView)
        return _pdtView->DragOver(grfKeyState, ptl, pdwEffect);

    *pdwEffect &= _dwDropEffect;
    return S_OK;    
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragLeave

*/
HRESULT CBaseBrowser2::DragLeave(void)
{
    if (_pdtView)
        return _pdtView->DragLeave();
    return S_OK;
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragDrop

*/
HRESULT CBaseBrowser2::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (S_OK == _DisableModeless()) {
        *pdwEffect = 0;
        return S_OK;
    }
    BOOL fNavigateDone = FALSE;
    HRESULT hr = E_FAIL;
    // If this is a shortcut - we want it to go via _NavIEShortcut
    // First check if it indeed a shortcut

    STGMEDIUM medium;

    if ((_ptfrm) && (DataObj_GetDataOfType(pdtobj, CF_HDROP, &medium)))
    {
                   
        WCHAR wszPath[MAX_PATH];

        if (DragQueryFileW((HDROP)medium.hGlobal, 0, wszPath, ARRAYSIZE(wszPath)))
        {
            LPWSTR pwszExtension = PathFindExtensionW(wszPath);
            // Check the extension to see if it is a .URL file

            if (0 == StrCmpIW(pwszExtension, L".url"))
            {
                // It is an internet shortcut 
                VARIANT varShortCutPath = {0};
                VARIANT varFlag = {0};
                SA_BSTR     bstr;
                varFlag.vt = VT_BOOL;
                varFlag.boolVal = VARIANT_TRUE;
                
                varShortCutPath.vt = VT_BSTR;
                StrCpyNW(bstr.wsz, wszPath, ARRAYSIZE(bstr.wsz));
                bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);
                varShortCutPath.bstrVal = bstr.wsz;
                
                hr = IUnknown_Exec(_ptfrm, &CGID_Explorer, SBCMDID_IESHORTCUT, 0, &varShortCutPath, &varFlag);
                fNavigateDone = SUCCEEDED(hr);   
                if(fNavigateDone)
                    DragLeave();
            }
        }

        // must call ReleaseStgMediumHGLOBAL since DataObj_GetDataOfType added an extra GlobalLock
        ReleaseStgMediumHGLOBAL(&medium);
    }

    if (FALSE == fNavigateDone)
    {
        if (_pdtView)
        {
            hr = _pdtView->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        }
        else 
        {
            LPITEMIDLIST pidlTarget;
            hr = SHPidlFromDataObject(pdtobj, &pidlTarget, NULL, 0);
            if (SUCCEEDED(hr)) {
                ASSERT(pidlTarget);
                hr = _psbOuter->BrowseObject(pidlTarget, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
                ILFree(pidlTarget);
            }
        }
    }

    return hr;
}

HRESULT CBaseBrowser2::_FireBeforeNavigateEvent(LPCITEMIDLIST pidl, BOOL* pfIsPost)
{
    HRESULT hres = S_OK;

    IBindStatusCallback * pBindStatusCallback;
    LPTSTR pszHeaders = NULL;
    LPBYTE pPostData = NULL;
    DWORD cbPostData = 0;
    BOOL fCancelled=FALSE;
    STGMEDIUM stgPostData;
    BOOL fHavePostStg = FALSE;

    *pfIsPost = FALSE;
    
    // get the bind status callback for this browser and ask it for
    // headers and post data
    if (SUCCEEDED(GetTopLevelPendingBindStatusCallback(this,&pBindStatusCallback))) {
        GetHeadersAndPostData(pBindStatusCallback,&pszHeaders,&stgPostData,&cbPostData, pfIsPost);
        pBindStatusCallback->Release();
        fHavePostStg = TRUE;

        if (stgPostData.tymed == TYMED_HGLOBAL) {
            pPostData = (LPBYTE) stgPostData.hGlobal;
        }
    }


    // Fire a BeforeNavigate event to inform container that we are about
    // to navigate and to give it a chance to cancel.  We have to ask
    // for post data and headers to pass to event, so only do this if someone
    // is actually hooked up to the event (HasSinks() is TRUE).
    //if (_bbd._pautoEDS->HasSinks()) 
    {
        TCHAR szFrameName[MAX_URL_STRING];
        LPTARGETFRAME2 pOurTargetFrame;
        SHSTR strHeaders;

        szFrameName[0] = 0;

        // get our frame name
        hres = TargetQueryService((IShellBrowser *)this, IID_ITargetFrame2, (void **) &pOurTargetFrame);
        if (SUCCEEDED(hres))
        {
            LPOLESTR pwzFrameName = NULL;
            pOurTargetFrame->GetFrameName(&pwzFrameName);
            pOurTargetFrame->Release();

            if (pwzFrameName) 
            {
                SHUnicodeToTChar(pwzFrameName, szFrameName, ARRAYSIZE(szFrameName));
                CoTaskMemFree(pwzFrameName);            
            }               
        }

        strHeaders.SetStr(pszHeaders);

        // fire the event!
        FireEvent_BeforeNavigate(_bbd._pautoEDS, _bbd._hwnd, _bbd._pautoWB2,
            pidl, NULL, 0, szFrameName[0] ? szFrameName : NULL,
            pPostData, cbPostData, strHeaders.GetStr(), &fCancelled);

        // free anything we alloc'd above
        if (pszHeaders)
            LocalFree(pszHeaders);

        if (fCancelled) {
            // container told us to cancel
            hres = E_ABORT;
        }
    }
    if (fHavePostStg) {
        ReleaseStgMedium(&stgPostData);
    }
    return hres;
}

HRESULT CBaseBrowser2::SetTopBrowser()
{
    _fTopBrowser = TRUE;

#ifdef MESSAGEFILTER
    if (!_lpMF) {
        /*
         * Create a message filter here to pull out WM_TIMER's while we are
         * busy.  The animation timer, along with other timers, can get
         * piled up otherwise which can fill the message queue and thus USER's
         * heap.
         */
        _lpMF = new CMessageFilter();

        if (_lpMF && !(((CMessageFilter *)_lpMF)->Initialize()))
        {
            ATOMICRELEASE(_lpMF);
        }
    }
#endif
    return S_OK;
}

HRESULT CBaseBrowser2::_ResizeView()
{
    RECT rc;

    _pbsOuter->_UpdateViewRectSize();
    if (_pact) {
        GetBorder(&rc);
        TraceMsg(TF_SHDUIACTIVATE, "UIW::SetBorderSpaceDW calling _pact->ResizeBorder");
        _pact->ResizeBorder(&rc, this, TRUE);
    }
    return S_OK;
} 

HRESULT CBaseBrowser2::_ResizeNextBorder(UINT itb)
{
    // (derived class resizes inner toolbar if any)
    return _ResizeView();
}

HRESULT CBaseBrowser2::_OnFocusChange(UINT itb)
{
#if 0
    // the OC *does* get here (it's not aggregated so _pbsOuter->_OnFocusChange
    // ends up here not in commonsb).  not sure if E_NOTIMPL is o.k., but
    // for now it's what we'll do...
    ASSERT(0);
#endif
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus)
{
    ASSERT(0);          // BUGBUG split: untested!
    // BUGBUG do we need to do _UIActivateView?
    ASSERT(fSetFocus);  // BUGBUG i think?

#if 0 // BUGBUG split: see commonsb.cpp, view/ONTOOLBARACTIVATED stuff
    // do we need something here?
#endif

    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::v_ShowHideChildWindows(BOOL fChildOnly)
{
    // (derived class does ShowDW on all toolbars)

    if (!fChildOnly) {
        _pbsOuter->_ResizeNextBorder(0);
        // This is called in ResizeNextBorder 
        // _UpdateViewRectSize();
    }

    return S_OK;
}

//***   _ExecChildren -- broadcast Exec to view and toolbars
// NOTES
//  we might do *both* punkBar and fBroadcast if we want to send stuff
//  to both the view and to all toolbars, e.g. 'stop' or 'refresh'.
//
//  BUGBUG n.b. the tray isn't a real toolbar, so it won't get called (sigh...).
HRESULT CBaseBrowser2::_ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt,
    VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    //ASSERT(!fBroadcast);    // only derived class supports this
    // alanau: but CWebBrowserSB doesn't override this method, so we hit this assert.

    // 1st, send to specified guy (if requested)
    if (punkBar != NULL) {
        // send to specified guy
        IUnknown_Exec(punkBar, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }

    // (derived class broadcasts to toolbars)

    return S_OK;
}

HRESULT CBaseBrowser2::_SendChildren(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    // the OC *does* get here (it's not aggregated so _pbsOuter->_SendChildren
    // ends up here not in commonsb).  since there are no kids i guess it's
    // ok to just drop the fBroadcast on the floor.
    ASSERT(!fBroadcast);    // only derived class supports this
#endif

    // 1st, send to specified guy (if requested)
    if (hwndBar != NULL) {
        // send to specified guy
        SendMessage(hwndBar, uMsg, wParam, lParam);
    }

    // (derived class broadcasts to toolbars)

    return S_OK;
}

void CBaseBrowser2::_StartRefreshTimer(void)
{
    if (_iRefreshTimeoutSet) {
        // This works even when iTimeout is 0
        TraceMsg(DM_WARNING, "CBB::_StartRefreshTimer start the timer");
        _iRefreshTimeoutSet = FALSE;
        _iRefreshTimerID = SetTimer(_bbd._hwnd, (UINT_PTR)this, _iRefreshTimeout*1000, _RefreshTimerProc);
    }
}


//
//  Handle <META HTTP-EQUIV ...> headers for lower components
//


HRESULT CBaseBrowser2::OnHttpEquiv(IShellView* psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut)
{
    //
    // We don't want to process any HTTP-EQUIV from the 'current' shellview
    // if there is any pending navigation. 
    //
    if (_bbd._psvPending && IsSameObject(_bbd._psv, psv)) {
        return E_FAIL;
    }

    typedef struct _HTTPEQUIVHANDLER {
        LPCWSTR pcwzHeaderName;
        HRESULT (*pfnHandler)(HWND, LPWSTR, LPWSTR, CBaseBrowser2 *, BOOL, LPARAM);
        LPARAM  lParam;
    } HTTPEQUIVHANDLER;

    const static HTTPEQUIVHANDLER HandlerList[] = {
                                           {L"Refresh",     _HandleRefresh,         0},
                                           {L"PICS-Label",  _HandlePICS,            0},
                                           {L"Page-Enter",  _HandleViewTransition,  tePageEnter},
                                           {L"Page-Exit",   _HandleViewTransition,  tePageExit},
                                           {L"Site-Enter",  _HandleViewTransition,  teSiteEnter},
                                           {L"Site-Exit",   _HandleViewTransition,  teSiteExit}
                                           };
    DWORD   i = 0;
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    LPWSTR  pwzEquivString = pvarargIn? pvarargIn->bstrVal : NULL;
    BOOL    fHasHeader = (pwzEquivString!=NULL);

    if (pvarargIn && pvarargIn->vt != VT_BSTR)
        return OLECMDERR_E_NOTSUPPORTED;   // BUGBUG should be invalidparam

#ifdef DEBUG
    TCHAR szDebug[MAX_URL_STRING] = TEXT("(empty)");
    if (pwzEquivString) {
        UnicodeToTChar(pwzEquivString, szDebug, ARRAYSIZE(szDebug));
    }
    TraceMsg(DM_HTTPEQUIV, "CBB::_HandleHttpEquiv got %s (fDone=%d)", szDebug, fDone);
#endif

    for ( ; i < ARRAYSIZE(HandlerList); i++) {

        // BUGBUG: do we care about matching "Refresh" on "Refreshfoo"?
        // If there is no header sent, we will call all handlers; this allows
        // us to pass the fDone flag to all handlers

        if (!fHasHeader || StrCmpNIW(HandlerList[i].pcwzHeaderName, pwzEquivString, lstrlenW(HandlerList[i].pcwzHeaderName)) == 0) {

            // Hit.  Now do the right thing for this header
            // We pass both the header and a pointer to the first char after
            // ':', which is usually the delimiter handlers will look for.

            LPWSTR pwzColon = fHasHeader? StrChrW(pwzEquivString, ':') : NULL;
      
            // Enforce the : at the end of the header
            if (fHasHeader && !pwzColon) {
                return OLECMDERR_E_NOTSUPPORTED;
            }
             
            hr = HandlerList[i].pfnHandler(_bbd._hwnd, pwzEquivString, pwzColon?(pwzColon+1):NULL, this, fDone, HandlerList[i].lParam);
            // We do not break here intentionally
        }
    }

    return hr;
} // _HandleHttpEquiv

STDMETHODIMP CBaseBrowser2::GetPalette( HPALETTE * phpal )
{
    BOOL fRes = FALSE;
    if ( _hpalBrowser )
    {
       *phpal = _hpalBrowser;
       fRes = TRUE;
    }
    return fRes ? NOERROR : E_FAIL;
}

HRESULT _HandleRefresh(HWND hwnd, LPWSTR pwz, LPWSTR pwzContent, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam)
{
    WCHAR        awch[INTERNET_MAX_URL_LENGTH];
    unsigned int uiTimeout = 0;

    ASSERT (pbb);

    awch[0] = 0;

    // Note - we *will not* fire the timer until we recieve fDone == TRUE
    // If fDone is set, we assume there is no content to deal with.
    // BUGBUG: should I allow content to be sent the same time as fDone? 19feb97 tonyci

    TraceMsg(DM_HTTPEQUIV, "_HandleRefresh called fDone=%d, pbb->i=%d",
             fDone, pbb->_iRefreshTimeoutSet);

    if (fDone && pbb->_iRefreshTimeoutSet) {

        // If we're in the middle of navigating to a new page and it's still
        // pending, defer starting the refresh timer until we actually activate
        // the pending view (to handle things like ratings blocking, etc.).
        // If there is no pending view, then we've already been activated,
        // or this is a refresh of the object already being displayed, etc.
        // In that case we start the timer off now.

        if (!pbb->_bbd._psvPending)
            pbb->_StartRefreshTimer();

        return S_OK;
    }

    // NSCompat: we only honor the first successfully parsed Refresh
    if (pbb->_pwzRefreshURL) {
        TraceMsg(DM_WARNING, "_HandleRefresh ignore second one");
        return S_OK;
    }

    if (    !pwzContent
        ||  !ParseRefreshContent(pwzContent, &uiTimeout, awch, INTERNET_MAX_URL_LENGTH)) {

        // Trident always calls this. Don't print a warning. 
        // TraceMsg(DM_WARNING, "_HandleRefresh ERROR no timeout");
        return OLECMDERR_E_NOTSUPPORTED;   // cannot handle refresh w/o timeout
    }

    // The next line kills any old timer, AND frees any old refresh URL
    pbb->_KillRefreshTimer();  // In case of multiple <meta ... refresh> tags

    if (awch[0]) {
        TraceMsg(DM_HTTPEQUIV, "_HandleRefresh handling %s", awch);
        pbb->_pwzRefreshURL = StrDupW(awch);

        // If we can't copy the URL, don't set the timer or else we'll
        // keep reloading the same page.

        if (pbb->_pwzRefreshURL == NULL)
            return OLECMDERR_E_NOTSUPPORTED;
    }

    pbb->_iRefreshTimeout = uiTimeout;
    pbb->_iRefreshTimeoutSet = TRUE;

    return S_OK;
} // _HandleRefresh

HRESULT _HandlePICS(HWND hwnd, LPWSTR pwz, LPWSTR pszColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam)
{
    return OLECMDERR_E_NOTSUPPORTED;
} // _HandlePICS

VOID CALLBACK _RefreshTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    TraceMsg(DM_HTTPEQUIV, "_RefreshTimerProc called");
    SendMessage(hwnd, WMC_ONREFRESHTIMER, 0, 0);
}

VOID CBaseBrowser2::_OnRefreshTimer(void)
{
    TraceMsg(DM_HTTPEQUIV, "_OnRefreshTimer called");

    // BUGBUG: if we have a modeless dialog up, should we retry again later?  tonyci 6jan97

    // Get the URL and navigate to it.
    // if the URL is NULL, do a refresh on this page.
    if (_bbd._pautoWB2)
    {
        if (_pwzRefreshURL) {
            VARIANT v1 = {0};
            v1.vt = VT_I4;
            v1.lVal = navNoReadFromCache;

            //  Netscape COMPAT.  they add HTTP-EQUIV refreshes to the history
            //  if they are non zero.  this allows users to go 
            //  back through the pages even if they dont get to stay 
            //  there that long.        zekel - 23-JUL-97
            if (!_iRefreshTimeout)
                v1.lVal |= navNoHistory;
            else
            {
                //  also, if we are refreshed to the same spot,
                //  we need to do NoHistory, because otherwise
                //  it will just fill up the navigation stack
                WCHAR wzCur[MAX_URL_STRING];
                if(SUCCEEDED(IEGetDisplayName(_bbd._pidlCur, wzCur, SHGDN_FORPARSING)) &&
                    0 == UrlCompareW(_pwzRefreshURL, wzCur, TRUE))
                    v1.lVal |= navNoHistory;
            }
    
            // We always want to do a full navigate for the URL, since it could be
            // a file of any type, .doc, .xls, .htm, etc.
    
            TraceMsg(DM_HTTPEQUIV, "_OnRefreshTimer calling Navigate");
#if (defined(UNIX) || defined(SPVER))
            // kill timer before navigate so navigate won't be envoked multipule times
	    _iRefreshTimeoutSet = FALSE;
	    KillTimer(_bbd._hwnd, _iRefreshTimerID);
#endif // UNIX
            _bbd._pautoWB2->Navigate(_pwzRefreshURL, &v1, PVAREMPTY, PVAREMPTY, PVAREMPTY);

            // Notes: _KillRefreshTimer will do the clean-up (_pwszRefreshURL).
        } else {
            TraceMsg(DM_HTTPEQUIV, "_OnRefreshTimer calling Refresh");
            VARIANT v = {0};
            v.vt = VT_I4;
            v.lVal = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_CLEARUSERINPUT;
            _bbd._pautoWB2->Refresh2(&v);
        }
    }

    _KillRefreshTimer();
    ASSERT(_pwzRefreshURL == NULL);
} 

//
//   Called when some operation wants to make sure that the Client Pull timer
//   will not be firing.
//

HRESULT CBaseBrowser2::_KillRefreshTimer(void)
{
    // Called when a navigation or other event causes our client pull timer
    // to be terminated.

    if (_pwzRefreshURL) {
        LocalFree(_pwzRefreshURL);
        _pwzRefreshURL = NULL;
    }

    _iRefreshTimeoutSet = FALSE;

    return (KillTimer(_bbd._hwnd, _iRefreshTimerID))? S_OK : E_FAIL;

} // _KillRefreshTimer

BOOL ParseRefreshContent(LPWSTR pwzContent,
    UINT * puiDelay, LPWSTR pwzUrlBuf, UINT cchUrlBuf)
{
    // We are parsing the following string:
    //
    //  [ws]* [0-9]+ [ws]* ; [ws]* url [ws]* = [ws]* { ' | " } [any]* { ' | " }
    //
    // Netscape insists that the string begins with a delay.  If not, it
    // ignores the entire directive.  There can be more than one URL mentioned,
    // and the last one wins.  An empty URL is treated the same as not having
    // a URL at all.  An empty URL which follows a non-empty URL resets
    // the previous URL.

    enum { PRC_START, PRC_DIG, PRC_DIG_WS, PRC_SEMI, PRC_SEMI_URL,
        PRC_SEMI_URL_EQL, PRC_SEMI_URL_EQL_ANY };

    UINT uiState = PRC_START;
    UINT uiDelay = 0;
    LPWSTR pwz = pwzContent;
    LPWSTR pwzUrl = NULL;   // init to suppress bogus C4701 warning
    UINT   cchUrl = 0;      // init to suppress bogus C4701 warning    
    WCHAR  wch;
    WCHAR  wchDel = 0;      // init to suppress bogus C4701 warning

    *pwzUrlBuf = 0;

    do
    {
        wch = *pwz;

        switch (uiState)
        {
        case PRC_START:
            if (wch >= TEXT('0') && wch <= TEXT('9'))
            {
                uiState = PRC_DIG;
                uiDelay = wch - TEXT('0');
            }
            else if (!ISSPACE(wch))
                goto done;
            break;

        case PRC_DIG:
            if (wch >= TEXT('0') && wch <= TEXT('9'))
                uiDelay = uiDelay * 10 + wch - TEXT('0');
            else if (ISSPACE(wch))
                uiState = PRC_DIG_WS;
            else if (wch == TEXT(';'))
                uiState = PRC_SEMI;
            else
                goto done;
            break;

        case PRC_DIG_WS:
            if (wch == TEXT(';'))
                uiState = PRC_SEMI;
            else if (!ISSPACE(wch))
                goto done;
            break;

        case PRC_SEMI:
            if (    (wch == TEXT('u') || wch == TEXT('U'))
                &&  (pwz[1] == TEXT('r') || pwz[1] == TEXT('R'))
                &&  (pwz[2] == TEXT('l') || pwz[2] == TEXT('L')))
            {
                uiState = PRC_SEMI_URL;
                pwz += 2;
            }
            else if (!ISSPACE(wch) && wch != TEXT(';'))
                goto done;
            break;

        case PRC_SEMI_URL:
            if (wch == TEXT('='))
            {
                uiState = PRC_SEMI_URL_EQL;
                *pwzUrlBuf = 0;
            }
            else if (wch == TEXT(';'))
                uiState = PRC_SEMI;
            else if (!ISSPACE(wch))
                goto done;
            break;

        case PRC_SEMI_URL_EQL:
            if (wch == TEXT(';'))
                uiState = PRC_SEMI;
            else if (!ISSPACE(wch))
            {
                uiState = PRC_SEMI_URL_EQL_ANY;

                pwzUrl = pwzUrlBuf;
                cchUrl = cchUrlBuf;

                if (wch == TEXT('\'')|| wch == TEXT('\"'))
                    wchDel = wch;
                else
                {
                    wchDel = 0;
                    *pwzUrl++ = wch;
                    cchUrl--;
                }
            }
            break;
                    
        case PRC_SEMI_URL_EQL_ANY:
            if (    !wch
                ||  ( wchDel && wch == wchDel)
                ||  (!wchDel && (ISSPACE(wch) || wch == TEXT(';'))))
            {
                *pwzUrl = 0;
                uiState = wch == TEXT(';') ? PRC_SEMI : PRC_DIG_WS;
            }
            else if (cchUrl > 1)
            {
                *pwzUrl++ = wch;
                cchUrl--;
            }
            break;
        }

        ++pwz;

    } while (wch);

done:

    *puiDelay = uiDelay;

    return(uiState >= PRC_DIG);
} // ParseRefreshContent

/////////////////////////////////////////////////////////////////////////////
// _HandleViewTransition
/////////////////////////////////////////////////////////////////////////////
HRESULT _HandleViewTransition
(
    HWND            hwnd,
    LPWSTR          pwz,
    LPWSTR          pwzContent,
    CBaseBrowser2 *  pbb,
    BOOL            fDone,
    LPARAM          lParam
)
{
    if ((pwz == NULL) || (pwzContent == NULL) || (pbb == NULL))
        return E_INVALIDARG;

    // NOTE: If there is no _bbd._psvPending, then we have most likely
    // been refreshed, so there is no need to reparse the EQUIV.
    if ((pbb->_ptrsite == NULL) || (pbb->_bbd._psvPending == NULL))
        return OLECMDERR_E_NOTSUPPORTED;

    TRANSITIONINFO  tiTransitionInfo = { 0 };
    HRESULT         hrResult;

    if (ParseTransitionInfo(pwzContent, &tiTransitionInfo))
    {
        hrResult = pbb->_ptrsite->_SetTransitionInfo((TransitionEvent)lParam, &tiTransitionInfo);

        // If anything fails, make like we didn't do anything.
        if (FAILED(hrResult))
            hrResult = OLECMDERR_E_NOTSUPPORTED;
    }
    else
        hrResult = OLECMDERR_E_NOTSUPPORTED;

    SAFERELEASE(tiTransitionInfo.pPropBag);

    return hrResult;
}   // _HandleViewTransition


/// BEGIN-CHC- Security fix for viewing non shdocvw ishellviews
void CBaseBrowser2::_CheckDisableViewWindow()
{
    void * pdov;
    HRESULT hres;

    if (_fTopBrowser && !_fNoTopLevelBrowser)
        return;     // Checking is not needed.

    if (WhichPlatform() == PLATFORM_INTEGRATED)
        return;     // Checking is not needed.

    hres = _bbd._psvPending->QueryInterface(CLSID_CDocObjectView, &pdov);
    if (SUCCEEDED(hres)) {
        // this is asking for an internal interface...  which is NOT IUnknown.
        // so we release the psvPrev, not th interface we got
        _bbd._psvPending->Release();
        
        // if we got here, this is shdocvw, we don't need to do anything
        return;
    } 

    TCHAR szClass[80];
    if (GetClassName(_bbd._hwndViewPending, szClass, ARRAYSIZE(szClass))) {
        const TCHAR c_szDefViewClass[] = TEXT("SHELLDLL_DefView");
        
        if (!StrCmp(szClass, c_szDefViewClass))
        {            
            if (_SubclassDefview()) {
                return;
            }
        }
    }
    

    // otherwise disable the window
    EnableWindow(_bbd._hwndViewPending, FALSE);
}

#define ID_LISTVIEW     1
BOOL CBaseBrowser2::_SubclassDefview()
{
    HWND hwndLV = GetDlgItem(_bbd._hwndViewPending, ID_LISTVIEW);

    // validate the class is listview
    TCHAR szClass[80];
    if (GetClassName(hwndLV, szClass, ARRAYSIZE(szClass))) {
        if (StrCmp(szClass, WC_LISTVIEW)) {
            
            // if something went wrong, bail!
            return FALSE;
        }
    }

    // ALERT:
    // Do not use SHRevokeDragDrop in the call below. That will have the
    // un-intentional side effect of unloading OLE because when the Defview
    // window is destroyed, we call SHRevokeDragDrop once again.

    RevokeDragDrop(hwndLV);


    _pfnDefView = (WNDPROC) SetWindowLongPtr(_bbd._hwndViewPending, GWLP_WNDPROC, (LONG_PTR)DefViewWndProc);
    
    // hack hack...  we know that defview doesn't use USERDATA so we do it instead
    // of using properties.
    SetWindowLongPtr(_bbd._hwndViewPending, GWLP_USERDATA, (LONG_PTR)(void*)(CBaseBrowser2*)this);
    return TRUE;
}

BOOL _IsSafe(IUnknown * psp, DWORD dwFlags)
{
    BOOL IsSafe = TRUE; // Assume we will allow this.
    IInternetHostSecurityManager * pihsm;

    // We should never subclass in this case, because the new shell32.dll with Zone Checking security
    // is installed.
    ASSERT(WhichPlatform() != PLATFORM_INTEGRATED);

    // What we want to do is allow this to happen only if the author of the HTML that hosts
    // the DefView is safe.  It's OK if they point to something unsafe, because they are
    // trusted.
    if (EVAL(SUCCEEDED(IUnknown_QueryService(psp, IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pihsm))))
    {
        if (S_OK != ZoneCheckHost(pihsm, URLACTION_SHELL_VERB, dwFlags))
        {
            // This zone is not OK or the user choose to not allow this to happen,
            // so cancel the operation.
            IsSafe = FALSE;    // Turn off functionality.
        }

        pihsm->Release();
    }

    return IsSafe;
}

LRESULT CBaseBrowser2::DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBaseBrowser2* psb = (CBaseBrowser2*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    CALLWNDPROC cwp = (CALLWNDPROC)psb->_pfnDefView;

    switch (uMsg) {
    case WM_NOTIFY:
    {
        
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm->idFrom == ID_LISTVIEW) {
            switch (pnm->code) {
            case LVN_ITEMACTIVATE:
                // We should never subclass in this case, because the new shell32.dll with Zone Checking security
                // is installed.
                ASSERT(WhichPlatform() != PLATFORM_INTEGRATED);
                return 1; // abort what you were going to do
                break;
                
            case NM_RETURN:
            case NM_DBLCLK:
                if (!_IsSafe(SAFECAST(psb, IShellBrowser*), PUAF_DEFAULT | PUAF_WARN_IF_DENIED))
                    return 1;
                break;
            
            case LVN_BEGINDRAG:
                if (!_IsSafe(SAFECAST(psb, IShellBrowser*), PUAF_NOUI))
                    return 1;
            }
        }
    }    
    break;
        
    case WM_CONTEXTMENU:
        return 1;
        
    case WM_DESTROY:
        // unsubclass
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)cwp);
        break;
    }

    return CallWindowProc(cwp, hwnd, uMsg, wParam, lParam);
}


/// END-CHC- Security fix for viewing non shdocvw ishellviews




//
//  IPersist
//
HRESULT 
CBaseBrowser2::GetClassID(CLSID *pclsid)
{
    return E_NOTIMPL;
}

//
// IPersistHistory
//  
#ifdef DEBUG
#define c_szFrameMagic TEXT("IE4ViewStream")
#define c_cchFrameMagic SIZECHARS(c_szFrameMagic)
#endif

// NOTE this is almost the same kind of data that is 
//  stored in a TravelEntry.

typedef struct _PersistedFrame {
    DWORD cbSize;
    DWORD type;
    DWORD lock;
    DWORD bid;
    CLSID clsid;
    DWORD cbPidl;
} PERSISTEDFRAME, *PPERSISTEDFRAME;

#define PFTYPE_USEPIDL      1
#define PFTYPE_USECLSID     2

#define PFFLAG_SECURELOCK   0x00000001


HRESULT GetPersistedFrame(IStream *pstm, PPERSISTEDFRAME ppf, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    ASSERT(pstm);
    ASSERT(ppf);
    ASSERT(ppidl);

#ifdef DEBUG
    TCHAR szMagic[SIZECHARS(c_szFrameMagic)];
    DWORD cbMagic = CbFromCch(c_cchFrameMagic);

    ASSERT(SUCCEEDED(IStream_Read(pstm, (void *) szMagic, cbMagic)));
    ASSERT(!StrCmp(szMagic, c_szFrameMagic));
#endif //DEBUG

    // This is pointing to the stack, make sure it starts NULL
    *ppidl = NULL;

    if(SUCCEEDED(hr = IStream_Read(pstm, (void *) ppf, SIZEOF(PERSISTEDFRAME))))
    {
        if(ppf->cbSize == SIZEOF(PERSISTEDFRAME) && (ppf->type == PFTYPE_USECLSID || ppf->type == PFTYPE_USEPIDL))
        {
            //  i used SHAlloc() cuz its what all the IL functions use
            if(ppf->cbPidl)
                *ppidl = (LPITEMIDLIST) SHAlloc(ppf->cbPidl);
        
            if(*ppidl)
            {
                hr = IStream_Read(pstm, (void *) *ppidl, ppf->cbPidl);
                if(FAILED(hr))
                {
                    ILFree(*ppidl);
                    *ppidl = NULL;
                }
            }
            else 
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT
CBaseBrowser2::LoadHistory(IStream *pstm, IBindCtx *pbc)
{
    HRESULT hr = E_INVALIDARG;


    ASSERT(pstm);

    TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory entered pstm = %X, pbc = %d", pstm, pbc);

    ATOMICRELEASE(_poleHistory);
    ATOMICRELEASE(_pstmHistory);
    ATOMICRELEASE(_pbcHistory);

    if(pstm)
    {
        PERSISTEDFRAME pf;
        LPITEMIDLIST pidl;

        hr = GetPersistedFrame(pstm, &pf, &pidl);

        if(SUCCEEDED(hr))
        {
            //  need to restore the previous bid
            //  if this is a new window
            ASSERT(pf.bid == _dwBrowserIndex || !_bbd._pidlCur);
            _dwBrowserIndex = pf.bid;
            _eSecureLockIconPending = pf.lock;

            if(pf.type == PFTYPE_USECLSID)
            {
                //get the class and instantiate
                if(SUCCEEDED(hr = CoCreateInstance(pf.clsid, NULL, CLSCTX_ALL, IID_IOleObject, (void **)&_poleHistory)))
                {
                    DWORD dwFlags;

                    if(SUCCEEDED(hr = _poleHistory->GetMiscStatus(DVASPECT_CONTENT, &dwFlags)))
                    {
                        if(dwFlags & OLEMISC_SETCLIENTSITEFIRST)
                        {
                            pstm->AddRef();  
                            if(pbc)
                                pbc->AddRef();
                            // we need to addref because we will use it async
                            // whoever uses it needs to release.
                            _pstmHistory = pstm;
                            _pbcHistory = pbc;
                        }
                        else
                        {
                            IPersistHistory *pph;

                            if(SUCCEEDED(hr = _poleHistory->QueryInterface(IID_IPersistHistory, (void **) &pph)))
                            {
                                hr = pph->LoadHistory(pstm, pbc);
                                TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory called pole->LoadHistory, hr =%X", hr);

                                pph->Release();
                            }
                        }
                        //  if we made then set the prepared history object in 
                        //  _poleHistory
                        if(FAILED(hr))
                        {
                            ATOMICRELEASE(_poleHistory);
                            ATOMICRELEASE(_pstmHistory);
                            ATOMICRELEASE(_pbcHistory);
                        }
                    }
                }
            }
            
            //
            //  just browse the object
            //  if poleHistory is set, then when the dochost is created
            //  it will pick up the object and use it.
            //  other wise we will do a normal navigate.
            //
            if(pidl)
            {
                _fDontAddTravelEntry = TRUE;
                hr = _psbOuter->BrowseObject(pidl, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
                ILFree(pidl);
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory exiting, hr =%X", hr);
    _pbsOuter->UpdateBackForwardState();
    return hr;
}

HRESULT
CBaseBrowser2::SaveHistory(IStream *pstm)
{
    HRESULT hr = E_UNEXPECTED;
    TraceMsg(TF_TRAVELLOG, "CBB::SaveHistory entering, pstm =%X", pstm);
    ASSERT(pstm);

    if(_bbd._pidlCur)
    {
        PERSISTEDFRAME pf = {0};
        pf.cbSize = SIZEOF(pf);
        pf.bid = GetBrowserIndex();
        pf.cbPidl = ILGetSize(_bbd._pidlCur);
        pf.type = PFTYPE_USEPIDL;
        pf.lock = _bbd._eSecureLockIcon;

        ASSERT(SUCCEEDED(IStream_Write(pstm, (void *) c_szFrameMagic, CbFromCch(c_cchFrameMagic))));
    
        //
        //  in order to use IPersistHistory we need to get the CLSID of the Ole Object
        //  then we need to get IPersistHistory off that object
        //  then we can save the PERSISTEDFRAME and the pidl and then pass 
        //  the stream down into the objects IPersistHistory
        //

        //  BUGBUG - right now we circumvent the view object for history - zekel - 18-JUL-97
        //  right now we just grab the DocObj from the view object, and then query
        //  the Doc for IPersistHistory.  really what we should be doing is QI the view
        //  for pph, and then use it.  however this requires risky work with the
        //  navigation stack, and thus should be postponed to IE5.  looking to the future
        //  the view could persist all kinds of great state info
        //
        //  but now we just get the background object.  but check to make sure that it
        //  will be using the DocObjHost code by QI for IDocViewSite
        //

        //  _bbd._psv can be null in subframes that have not completed navigating
        //  before a refresh is called
        IPersistHistory *pph = NULL;
        if(_bbd._psv && SUCCEEDED(hr = SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_IPersistHistory, (void **)&pph)))
        {
            ASSERT(pph);
            IDocViewSite *pdvs;

            if(SUCCEEDED(_bbd._psv->QueryInterface(IID_IDocViewSite, (void **)&pdvs)) && 
               SUCCEEDED(hr = pph->GetClassID(&(pf.clsid))))
            {
                pf.type = PFTYPE_USECLSID;
                TraceMsg(TF_TRAVELLOG, "CBB::SaveHistory is PFTYPE_USECLSID");
            }

            ATOMICRELEASE(pdvs);
        }


        if(SUCCEEDED(hr = IStream_Write(pstm,(void *)&pf, pf.cbSize)))
            hr = IStream_Write(pstm,(void *)_bbd._pidlCur, pf.cbPidl);

        if(SUCCEEDED(hr) && pf.type == PFTYPE_USECLSID)
            hr = pph->SaveHistory(pstm);

        ATOMICRELEASE(pph);
    }
    
    TraceMsg(TF_TRAVELLOG, "CBB::SaveHistory exiting, hr =%X", hr);
    return hr;

}

HRESULT CBaseBrowser2::SetPositionCookie(DWORD dwPositionCookie)
{
    HRESULT hr = E_FAIL;
    //
    //  we force the browser to update its internal location and the address bar
    //  this depends on the fact that setposition cookie was always 
    //  started by a ptl->Travel(). so that the current position in the ptl
    //  is actually the correct URL for us to have.  zekel - 22-JUL-97
    //

    ITravelLog *ptl;
    GetTravelLog(&ptl);
    if(ptl)
    {
        ITravelEntry *pte;
        if(EVAL(SUCCEEDED(ptl->GetTravelEntry((IShellBrowser *)this, 0, &pte))))
        {
            LPITEMIDLIST pidl;
            ASSERT(pte);
            if(SUCCEEDED(pte->GetPidl(&pidl)))
            {
                BOOL fUnused;
                ASSERT(pidl);
                if(SUCCEEDED(hr = _FireBeforeNavigateEvent(pidl, &fUnused)))
                {
                    IPersistHistory *pph;
                    if(_bbd._psv && SUCCEEDED(hr = SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_IPersistHistory, (void **)&pph)))
                    {
                        ASSERT(pph);

                        //  now that we are certain that we are going to call into 
                        //   the document, we need to update the entry right before.
                        //  NOTE: after an update, we cannot revert if there was an
                        //  error in the Set...
                        ptl->UpdateEntry((IShellBrowser *)this, TRUE);

                        hr = pph->SetPositionCookie(dwPositionCookie);
                        pph->Release();

                        //  this updates the browser to the new pidl, 
                        //  and navigates there directly if necessary.
                        BOOL fDidBrowse;
                        NotifyRedirect(_bbd._psv, pidl, &fDidBrowse);

                        if(!fDidBrowse)
                        {
                            FireEvent_NavigateComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur, _bbd._hwnd);
                            FireEvent_DocumentComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur);
                        }
                    }

                }
                ILFree(pidl);
            }
            pte->Release();
        }
        ptl->Release();
    }

    TraceMsg(TF_TRAVELLOG, "CBB::SetPositionCookie exiting, cookie = %X, hr =%X", dwPositionCookie, hr);
    
    return hr;
}

HRESULT CBaseBrowser2::GetPositionCookie(DWORD *pdwPositionCookie)
{
    HRESULT hr = E_FAIL;
    IPersistHistory *pph;
    ASSERT(pdwPositionCookie);

    if(pdwPositionCookie && _bbd._psv && SUCCEEDED(hr = SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_IPersistHistory, (void **)&pph)))
    {
        ASSERT(pph);

        hr = pph->GetPositionCookie(pdwPositionCookie);
        pph->Release();
    }

    TraceMsg(TF_TRAVELLOG, "CBB::GetPositionCookie exiting, cookie = %X, hr =%X", *pdwPositionCookie, hr);

    return hr;
}

DWORD CBaseBrowser2::GetBrowserIndex()
{
    //  the first time we request the index, we init it.
    if (!_dwBrowserIndex)
    {
        //
        //  the topframe browser all have the same browser index so 
        //  that they can trade TravelEntries if necessary.  because we now
        //  trade around TravelEntries, then we need to make the bids relatively
        //  unique.  and avoid ever having a random frame be BID_TOPFRAMEBROWSER
        //
        if(IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *)))
            _dwBrowserIndex = BID_TOPFRAMEBROWSER;
        else do
        {
            _dwBrowserIndex = SHRandom();

        } while (!_dwBrowserIndex || _dwBrowserIndex == BID_TOPFRAMEBROWSER);
        // psp->Release();

        TraceMsg(TF_TRAVELLOG, "CBB::GetBrowserIndex() NewFrame BID = %X", _dwBrowserIndex);
    }

    return _dwBrowserIndex;
}

HRESULT CBaseBrowser2::GetHistoryObject(IOleObject **ppole, IStream **ppstm, IBindCtx **ppbc) 
{
    ASSERT(ppole);
    ASSERT(ppstm);
    ASSERT(ppbc);

    *ppole = _poleHistory;
    *ppstm = _pstmHistory;
    *ppbc = _pbcHistory;

    //  we dont need to release, because we are just giving away our
    //  reference.
    _poleHistory = NULL;
    _pstmHistory = NULL;
    _pbcHistory = NULL;

    if(*ppole)
        return NOERROR;

    ASSERT(!*ppstm);
    return E_FAIL;
}

HRESULT CBaseBrowser2::SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor)
{
    if(!_poleHistory && !_fGeneratedPage)
    {
        ASSERT(pole);

        _fIsLocalAnchor = fIsLocalAnchor;

        if(pole)
        {
            _poleHistory = pole;
            _poleHistory->AddRef();
            return NOERROR;
        }
    }
    
    return E_FAIL;
}

HRESULT CBaseBrowser2::CacheOLEServer(IOleObject *pole)
{
    TraceMsg(DM_CACHEOLESERVER, "CBB::CacheOLEServer called");
    HRESULT hres;
    IPersist* pps;

    // ISVs want to turn off this caching because it's "incovenient"
    // to have the browser hold onto their object. We can do that
    // with a quick registry check here, but first let's make sure
    // we don't have a real bug to fix...

    hres = pole->QueryInterface(IID_IPersist, (void **)&pps);
    if (FAILED(hres)) {
        return hres;
    }

    CLSID clsid = CLSID_NULL;
    hres = pps->GetClassID(&clsid);
    pps->Release();

    if (SUCCEEDED(hres)) {
        SA_BSTRGUID str;
        StringFromGUID2(clsid, str.wsz, ARRAYSIZE(str.wsz));
        str.cb = lstrlenW(str.wsz) * SIZEOF(WCHAR);

        VARIANT v;
        VariantInit (&v);
        hres = _bbd._pautoWB2->GetProperty(str.wsz, &v);

        if (SUCCEEDED(hres) && v.vt != VT_EMPTY) {
            // We already have it. We are fine.
            TraceMsg(DM_CACHEOLESERVER, "CBB::CacheOLEServer not first time");
            VariantClear(&v);
        } else {
            // We don't have it yet. Add it. 
            v.vt = VT_UNKNOWN;
            v.punkVal = ClassHolder_Create(&clsid);
            if (v.punkVal) {
                hres = _bbd._pautoWB2->PutProperty(str.wsz, v);
                TraceMsg(DM_CACHEOLESERVER, "CBB::CacheOLEServer first time %x", hres);
                v.punkVal->Release();
            } else {
                ASSERT(0);
            }
        }
    }

    return hres;
}


HRESULT CBaseBrowser2::GetSetCodePage(VARIANT* pvarIn, VARIANT* pvarOut)
{
    //
    //  Process the out parameter first so that the client can set and
    // get the previous value in a single call.
    //
    if (pvarOut) {
        pvarOut->vt = VT_I4;
        pvarOut->lVal = _cp;
    }

    if (pvarIn && pvarIn->vt==VT_I4) {
        TraceMsg(DM_DOCCP, "CBB::GetSetCodePage changing _cp from %d to %d",
                 _cp, pvarIn->lVal);
        _cp = pvarIn->lVal;
    }

    return S_OK;
}

HRESULT CBaseBrowser2::GetPidl(LPITEMIDLIST *ppidl)
{
    ASSERT(ppidl);

    *ppidl = ILClone(_bbd._pidlCur);

    return *ppidl ? S_OK : E_FAIL;
}

HRESULT CBaseBrowser2::SetReferrer(LPITEMIDLIST pidl)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::GetBrowserByIndex(DWORD dwID, IUnknown **ppunk)
{
    HRESULT hr = E_FAIL;
    ASSERT(ppunk);
    *ppunk = NULL;

    //gotta get the target frame ...
    ITargetFramePriv *ptf;
    if(SUCCEEDED(_ptfrm->QueryInterface(IID_ITargetFramePriv, (void **)&ptf)))
    {
        ASSERT(ptf);
        hr = ptf->FindBrowserByIndex(dwID, ppunk);
        ptf->Release();
    }

    return hr;
}

HRESULT CBaseBrowser2::GetTravelLog(ITravelLog **pptl)
{
    ASSERT(pptl);
    *pptl = NULL;

    if(!_bbd._ptl)
    {
        IBrowserService *pbs;

        if (SUCCEEDED(_pspOuter->QueryService(SID_STopFrameBrowser, IID_IBrowserService, (void **)&pbs))) 
        {
            ASSERT(pbs);

            if(IsSameObject(SAFECAST(this, IBrowserService *), pbs))
            {
                // we are it, so we need to make us a TravelLog
                CreateTravelLog(&_bbd._ptl);
            }
            else
            {
                pbs->GetTravelLog(&_bbd._ptl);
            }
            
            pbs->Release();
        }
    }

    if(_bbd._ptl)
    {
        _bbd._ptl->AddRef();
        *pptl = _bbd._ptl;
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CBaseBrowser2::InitializeTravelLog(ITravelLog* ptl, DWORD dwBrowserIndex)
{
    _bbd._ptl = ptl;
    _bbd._ptl->AddRef();
    _dwBrowserIndex =dwBrowserIndex;
    return S_OK;
}

// Let the top level browser know that it might need to update it's zones information
void CBaseBrowser2::_Exec_psbMixedZone()
{
    IShellBrowser *psbTop;
    if (SUCCEEDED(_pspOuter->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psbTop))) 
    {
        IUnknown_Exec(psbTop, &CGID_Explorer, SBCMDID_MIXEDZONE, 0, NULL, NULL);
         psbTop->Release();
    }
}

//+---------------------------------------------------------------
//
//  Member:     CBaseBrowser2::QueryUseLocalVersionVector
//
//---------------------------------------------------------------

STDMETHODIMP
CBaseBrowser2::QueryUseLocalVersionVector(BOOL *pfUseLocal)
{
    *pfUseLocal = FALSE;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CBaseBrowser2::QueryVersionVector
//
//---------------------------------------------------------------

STDMETHODIMP
CBaseBrowser2::QueryVersionVector(IVersionVector *pVersion)
{
    HRESULT hr;
    HKEY    hkey = NULL;

    hr = pVersion->SetVersion(L"IE", L"5.0.beta1");
    if (hr)
        goto Cleanup;

    // Enumerate through the HKLM\Software\Microsoft\Internet Explorer\Version Vector key
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Internet Explorer\\Version Vector"),
            &hkey) == ERROR_SUCCESS)
    {
        for (int iValue = 0; ;iValue++)
        {
            OLECHAR    wszValue[256];
            OLECHAR    wszVersion[256];
            DWORD      dwType;
            DWORD      cchValue = ARRAYSIZE(wszValue);
            DWORD      cchVersion = ARRAYSIZE(wszVersion);

            if (SHEnumValueW(hkey, iValue, wszValue, &cchValue, 
                                                     &dwType, wszVersion, &cchVersion)==ERROR_SUCCESS)
            {
                hr = pVersion->SetVersion(wszValue, wszVersion);
                if (hr)
                    goto Cleanup;
            }
            else
            {
                break;
            }
        }
    }


Cleanup:
    if (hkey != NULL)
        RegCloseKey(hkey);
    return hr;
}



#ifdef LIGHT_FRAMES
HRESULT
CBaseBrowser2::SaveObject( void)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::GetMoniker( DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::GetContainer( IOleContainer **ppContainer)
{
    // we're also a container.
    if (!ppContainer)
        return E_INVALIDARG;

    *ppContainer = (IOleContainer*)this;
    return S_OK;
}

HRESULT
CBaseBrowser2::ShowObject( void)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::OnShowWindow( BOOL fShow)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::RequestNewObjectLayout( void)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::CanInPlaceActivate( void)
{
    return S_OK;
}

HRESULT
CBaseBrowser2::OnInPlaceActivate( void)
{
    return S_OK;
}

HRESULT
CBaseBrowser2::OnUIActivate( void)
{
    return S_OK;
}

HRESULT
CBaseBrowser2::GetWindowContext( 
        /* [out] */ IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame,
        /* [out] */ IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc,
        /* [out] */ LPRECT lprcPosRect,
        /* [out] */ LPRECT lprcClipRect,
        /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    *ppFrame = (IOleInPlaceFrame*) this;
    *ppDoc = (IOleInPlaceUIWindow*) this;
    GetViewRect(lprcPosRect);
    GetViewRect(lprcClipRect);
    return S_OK;
}

HRESULT
CBaseBrowser2::Scroll( 
        /* [in] */ SIZE scrollExtant)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::OnUIDeactivate( 
        /* [in] */ BOOL fUndoable)
{
    return S_OK;
}

HRESULT
CBaseBrowser2::OnInPlaceDeactivate( void)
{
    return S_OK;
}

HRESULT
CBaseBrowser2::DiscardUndoState( void)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::DeactivateAndUndo( void)
{
    return E_NOTIMPL;
}

HRESULT
CBaseBrowser2::OnPosRectChange( 
        /* [in] */ LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}

#endif
