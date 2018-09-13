//+---------------------------------------------------------------------
//
//   File:      inplace.cxx
//
//  Contents:   CDoc implementation (partial)
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef UNIX
#ifndef X_WINABLE_H_
#define X_WINABLE_H_
#include "winable.h"
#endif
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_BOOKMARK_HXX_
#define X_BOOKMARK_HXX_
#include "bookmark.hxx"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DRAGDROP_HXX_
#define X_DRAGDROP_HXX_
#include "dragdrop.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#define SCROLLPERCENT   125

static ATOM  s_atomFormOverlayWndClass;
UINT   g_msgMouseWheel = 0;

EXTERN_C const CLSID CLSID_HTMLDialog;

#ifndef NO_IME
extern HRESULT ActivateDIMM(); // imm32.cxx
extern HRESULT FilterClientWindowsDIMM(ATOM *aaWindowClasses, UINT uSize);
#endif // ndef NO_IME

CLayout * GetLayoutForDragDrop(CElement * pElement); // defined below

//+---------------------------------------------------------------------------
//
//  Member:     CServer::EnsureInPlaceObject, CServer
//
//  Synopsis:   Creates the InPlace object when needed.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

HRESULT
CDoc::EnsureInPlaceObject()
{
    if (!_pInPlace)
    {
        _pInPlace = new CFormInPlace();
        if (!_pInPlace)
            RRETURN(E_OUTOFMEMORY);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormInPlace::~CFormInplace
//
//  Synopsis:   Cleanup before destruction.
//
//----------------------------------------------------------------------------

CFormInPlace::~CFormInPlace()
{
    // Make sure we have released all window resources.
    ClearInterface(&_pHostShowUI);
}


HRESULT
CDoc::OnSelectChange(void)
{
    if (_state != OS_UIACTIVE)
        return S_OK;

    HRESULT                         hr = S_OK;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     FormOverlayWndProc
//
//  Synopsis:   Transparent window for use during drag-drop.
//
//-------------------------------------------------------------------------

static LRESULT CALLBACK
FormOverlayWndProc(HWND hWnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT     ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        return 0;

    case WM_ERASEBKGND:
        return TRUE;

    default:
        return DefWindowProc(hWnd, wm, wParam, lParam);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::CreateOverlayWindow
//
//  Synopsis:   Creates a transparent window for use during drag-drop.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HWND
CDoc::CreateOverlayWindow(HWND hwndCover)
{
    Assert(_pInPlace->_hwnd);
    HWND hwnd;
    TCHAR * pszBuf;

    if (!s_atomFormOverlayWndClass)
    {
        HRESULT     hr;

        hr = THR(RegisterWindowClass(
                _T("Overlay"),
                FormOverlayWndProc,
                CS_DBLCLKS,
                NULL, NULL,
                &s_atomFormOverlayWndClass));
        if (hr)
            return NULL;
    }

#if defined(WIN16) || defined(_MAC)
    TCHAR szBuf[128];
    GlobalGetAtomName(s_atomFormOverlayWndClass, szBuf, ARRAY_SIZE(szBuf));
    pszBuf = szBuf;
#else
    pszBuf = (TCHAR *)(DWORD_PTR)s_atomFormOverlayWndClass;
#endif


    // BUGBUG (garybu) Overlay window should be same size as hwndCover and
    // just above it in the zorder.

    hwnd = CreateWindowEx(
            WS_EX_TRANSPARENT,
            pszBuf,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0,
            SHRT_MAX / 2, SHRT_MAX / 2,
            _pInPlace->_hwnd,
            0,
            g_hInstCore,
            this);
    if (!hwnd)
        return NULL;

    SetWindowPos(
            hwnd,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    return hwnd;
}


//+---------------------------------------------------------------
//
//  Member:    CDoc::AttachWin
//
//  Synopsis:  Create our InPlace window
//
//  Arguments: [hwndParent] -- our container's hwnd
//
//  Returns:   hwnd of InPlace window, or NULL
//
//---------------------------------------------------------------

HRESULT
CDoc::AttachWin(HWND hwndParent, RECT * prc, HWND * phwnd)
{
    HRESULT     hr = S_OK;
    HWND        hwnd = NULL;
    const DWORD STYLE_ENABLED = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    const DWORD STYLE_DISABLED = STYLE_ENABLED | WS_DISABLED;

    //  Note: this code is duplicated in CServer::AttachWin.

    if (!s_atomWndClass)
    {
        hr = THR(RegisterWindowClass(
                _T("Server"),
                CServer::WndProc,
                CS_DBLCLKS,
                NULL, NULL,
                &CServer::s_atomWndClass));
        if (hr)
            goto Cleanup;
#ifndef NO_IME
        // if dimm is installed, we only want ime interaction with the server class
        FilterClientWindowsDIMM(&s_atomWndClass, 1);
#endif // ndef NO_IME
    }

    Assert(phwnd);

    if (_hwndCached)
    {
        hwnd = _hwndCached;
        _hwndCached = NULL; // this is necessary because if the window is destroyed later by WM_DESTROY,
                            // which we don't intercept, then _hwndCached should not be != NULL
        if (SetParent (hwnd, hwndParent))
        {
            _pInPlace->_hwnd = hwnd;
            PrivateAddRef();
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(CServer *) this); // connect this CDoc to the window
                                                                 // NOTE the cast to CServer for WIN16
            SetWindowPos (
                hwnd,
                HWND_TOP,
                prc->left, prc->top,
                prc->right - prc->left, prc->bottom - prc->top,
                SWP_SHOWWINDOW | SWP_NOREDRAW);
        }
        else
        {
            DestroyWindow(hwnd);
            hwnd = NULL; // this will cause it to create new window
        }
    }

    if (!hwnd)
    {
        TCHAR * pszBuf;
        TCHAR achClassName[256];

#ifdef WIN16
        char szBuf[128];
        GlobalGetAtomName(CServer::s_atomWndClass, szBuf, ARRAY_SIZE(szBuf));
        pszBuf = szBuf;
#else
        pszBuf = (TCHAR *)(DWORD_PTR)CServer::s_atomWndClass;
#endif

        hwnd = CreateWindowEx(
                0,
                pszBuf,
                NULL,
                (_fEnabled ||
                 _fDesignMode) ? STYLE_ENABLED : STYLE_DISABLED,
                prc->left, prc->top,
                prc->right - prc->left, prc->bottom - prc->top,
                hwndParent,
                0,              // no child identifier - shouldn't send WM_COMMAND
                g_hInstCore,
                (CServer *)this);

        ::GetClassName(hwndParent, achClassName, ARRAY_SIZE(achClassName));
        _fVB = (StrCmpIW(achClassName, _T("HTMLPageDesignerWndClass")) == 0) ? TRUE : FALSE;
    }

    if (!hwnd)
        goto Win32Error;

    // Lazily register/revoke drag-drop
#ifndef NO_DRAGDROP
    IGNORE_HR(GWPostMethodCall(this, ONCALL_METHOD(CDoc, EnableDragDrop, enabledragdrop), 0, FALSE, "CDoc::EnableDragDrop"));
#endif // NO_DRAGDROP

#ifndef WIN16
#ifndef NO_IME
    // if DIMM is installed, activate it
    ActivateDIMM();
#endif // ndef NO_IME

    if (g_msgMouseWheel == 0 && GetVersion() >= 0x80000000)
    {
        // special for Windows 95, equivalent to WM_MOUSEWHEEL
        //
        g_msgMouseWheel = RegisterWindowMessage(_T("MSWHEEL_ROLLMSG"));
    }

    // Initialize our _wUIState from the window.
    // BUGBUG: do we need to clear formats here? (jbeda)
    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT && g_dwPlatformVersion >= 0x00050000)
    {
        _wUIState = SendMessage(hwnd, WM_QUERYUISTATE, 0, 0);
    }
#endif // ndef WIN16

Cleanup:
    *phwnd = hwnd;

    UpdatePrintNotifyWindow(hwnd);

    RRETURN(hr);

Win32Error:
    hr = GetLastWin32Error();
    DetachWin();
    goto Cleanup;
}

//+---------------------------------------------------------------
//
// Local Helper: IsInIEBrowser
//
// Synopsis: Determine if Trident is in IE browser
//
//----------------------------------------------------------------
BOOL
IsInIEBrowser(CDoc * pDoc)
{
    BOOL             fResult     = FALSE;
    IOleClientSite * pClientSite = pDoc->_pClientSite;

    if ((!(pDoc->_fDesignMode))
            && (pDoc == pDoc->GetRootDoc())
            && ((pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG) == 0)
            && (pClientSite))
    {
        HRESULT hr;
        IServiceProvider * psp1, * psp2;
        IUnknown         * pUnk;

        hr = pClientSite->QueryInterface(IID_IServiceProvider, (void **) &psp1);
        if (!hr && psp1)
        {
            hr = psp1->QueryService(
                    SID_STopLevelBrowser,
                    IID_IServiceProvider,
                    (void **) &psp2);
            if (!hr && psp2)
            {
                hr = psp2->QueryInterface(SID_SShellDesktop, (void **) &pUnk);
                if (!hr && pUnk)
                    pUnk->Release();
                else
                    fResult = TRUE;

                psp2->Release();
            }
            psp1->Release();
        }
    }
    return fResult;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::RunningToInPlace
//
//  Synopsis:   Effects the running to inplace-active state transition
//
//  Returns:    SUCCESS if the object results in the in-place state
//
//  Notes:      This method in-place activates all inside-out embeddings
//              in addition to the normal CServer base processing.
//
//---------------------------------------------------------------

HRESULT
CDoc::RunningToInPlace(LPMSG lpmsg)
{
    HRESULT         hr;
    CNotification   nf;

    TraceTag((tagCDoc, "%lx CDoc::RunningToInPlace", this));

    _fFirstTimeTab = IsInIEBrowser(this);
    _fInPlaceActivating = TRUE;
    _fEnableInteraction = TRUE;

    //  Do the normal transition, creating our main window

    hr = THR(CServer::RunningToInPlace(lpmsg));
    if (hr)
        goto Cleanup; // Do not goto error because CServer has already
                      // performed all necessary cleanup.

    // Make sure that we have a current element before we set focus to one.
    // If we are parse done, and just becoming inplace, then we don't have an
    // active element yet.
    if (LoadStatus() >= LOADSTATUS_PARSE_DONE)	
        DeferSetCurrency(0);

    //
    // Prepare the view
    //

    _view.Activate();
    _view.SetViewSize(_dci._sizeDst);
    _view.SetFocus(_pElemCurrent, _lSubCurrent);

    //
    // Look for IDocHostShowUI
    //

    Assert(!InPlace()->_pHostShowUI);

    // first look on the DocHostUIHandler
    // If the handler is imposed on us through ICustomDoc,
    //   it has already happened
    // If we use CSmartDocHostUIHandler, it will also
    //   pass IDocHostShowUI calls to the super site.
    if(_pHostUIHandler)
    {
        hr = THR_NOTRACE(_pHostUIHandler->QueryInterface(
                                                IID_IDocHostShowUI,
                                                (void **)&(InPlace()->_pHostShowUI)));
        if(hr != S_OK)
        {
            InPlace()->_pHostShowUI = NULL;
        }
    }

    // next look on the client site
    if(_pClientSite && !InPlace()->_pHostShowUI)
    {
        hr = THR_NOTRACE(_pClientSite->QueryInterface(
                                           IID_IDocHostShowUI,
                                           (void **)&(InPlace()->_pHostShowUI)));
        if(hr != S_OK)
        {
            InPlace()->_pHostShowUI = NULL;
        }
    }

    // not having a DocHostShowUI is not a failing condition for this function
    hr = S_OK;

    //
    // Make sure that everything is laid out correctly
    // *before* doing the BroadcastNotify because with olesites,
    // we want to baseline them upon the broadcast.  When doing this,
    // we better know the olesite's position.
    //

    _view.EnsureView(LAYOUT_SYNCHRONOUS);

    nf.DocStateChange1(PrimaryRoot(), (void *)OS_RUNNING);
    BroadcastNotify(&nf);

    if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        // Now is the time to ask the sites to load any history
        // that they could not earlier (e.g. scroll/caret positions
        // because they require the doc to be recalced, the site
        // arrays built, etc.)
        if (!_fDelayLoadHistoryDone)
        {
            CNotification   nf;
            
            _fDelayLoadHistoryDone = TRUE;
            nf.DelayLoadHistory(PrimaryRoot());
            BroadcastNotify(&nf);
        }
    }

#if DBG == 1
    DisplayChildZOrder(_pInPlace->_hwnd);
#endif

    //
    // Show document window *after* all child windows have been
    // created, to reduce clipping region recomputations.  To avoid
    // WM_ERASEBKGND sent outside of a WM_PAINT, we show without
    // redraw and then invalidate.
    //

    SetWindowPos(_pInPlace->_hwnd, NULL, 0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE |
            SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);

    // Unblock any script execution/parsing that was waiting for inplace activation
    NotifyMarkupsInPlace();

    // schedule flushing the AttachPeerQueue. Flushing should be posted, not synchronous,
    // so to allow shdocvw to connect to the doc fully before we start attaching peers
    // and possible firing events to shdocvw (e.g. errors)
    IGNORE_HR(GWPostMethodCall(
        this,
        ONCALL_METHOD(CDoc, PeerDequeueTasks, peerdequeue),
        0, FALSE, "CDoc::PeerDequeueTasks"));

    // If we have a task to look for a certain scroll position, make sure
    // we have recalc'd at least to that point before display.

    // Supress scrollbits because we're about to inval the entire window
    {
        NavigateNow(FALSE);
    }

    // This inval does nothing if LoadStatus() < LOADSTATUS_INTERACTIVE
    Invalidate(NULL, NULL, NULL, INVAL_CHILDWINDOWS);

    // run scripts if not in  mode
    if (!_fDesignMode)
    {
        if (LoadStatus() >= LOADSTATUS_DONE)
        {
            if (_pScriptCollection)
            {
                hr = THR(_pScriptCollection->SetState(SCRIPTSTATE_CONNECTED));
                if (hr)
                    goto Error;
            }

            if (!_fFiredOnLoad)
            {
                _fFiredOnLoad = TRUE;
                if (_pOmWindow)
                {
                    CDoc::CLock Lock(this);

                    if (HtmCtx() && !(HtmCtx()->GetState() & (DWNLOAD_ERROR | DWNLOAD_STOPPED)))
                    {
                        // these memberdata will only be here if a peer was present to put them
                        // there.  thus we don't need to check for _fPeersPossible
                        if (_cstrHistoryUserData || _pShortcutUserData)
                            FirePersistOnloads();

                        _pOmWindow->Fire_onload();
                    }

                    // Let the client site know we are loaded
                    // Only HTMLDialog pays attention to this
                    if (_pClientSite && _fInHTMLDlg)
                    {
                        CTExec(_pClientSite, &CLSID_HTMLDialog,
                               0, 0, NULL, NULL);
                    }
                }
            }
        }
    }

    RefreshStatusUI();
    DeferUpdateUI();

Cleanup:

    _fInPlaceActivating = FALSE;
    RRETURN(hr);

Error:

    IGNORE_HR(InPlaceToRunning());
    goto Cleanup;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::InPlaceToRunning
//
//  Synopsis:   Effects the inplace-active to running state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//  Notes:      This method in-place deactivates all inside-out embeddings
//              in addition to normal CServer base processing.
//
//---------------------------------------------------------------

HRESULT
CDoc::InPlaceToRunning(void)
{
    HRESULT         hr = S_OK;
    CNotification   nf;
    
    TraceTag((tagCDoc, "%lx CDoc::InPlaceToRunning", this));

    {
        CElement *pElemFireTarget;
        pElemFireTarget = _pElemCurrent->GetFocusBlurFireTarget(_lSubCurrent);
        Assert(pElemFireTarget);

        CElement::CLock LockUpdate(pElemFireTarget, CElement::ELEMENTLOCK_UPDATE);
        _fInhibitFocusFiring = TRUE;
        IGNORE_HR(pElemFireTarget->RequestYieldCurrency(TRUE));

        if (pElemFireTarget->IsInMarkup())
        {
            hr = THR(pElemFireTarget->YieldCurrency(_pElementDefault));
            if (hr)
                goto Cleanup;

            if (!TestLock(FORMLOCK_CURRENT))
            {
                Assert(pElemFireTarget == _pElemCurrent || pElemFireTarget->Tag() == ETAG_AREA);
                pElemFireTarget->Fire_onblur(0);
            }
        }
        _fInhibitFocusFiring = FALSE;
    }

    if (_pElemUIActive)
    {
        _pElemUIActive->YieldUI(PrimaryRoot());
    }
    _pElemCurrent = _pElemUIActive = PrimaryRoot();

    SetMouseCapture (NULL, NULL);
    releaseCapture();
    ClearCachedDialogs();

    // stop scripts if not in design mode
    CleanupScriptTimers();

    if (_fFiredOnLoad)
    {
        _fFiredOnLoad = FALSE;
        if (_pOmWindow)
        {
            CDoc::CLock Lock(this);
            _pOmWindow->Fire_onunload();
        }
    }

    TerminateLookForBookmarkTask();

    // we also want to drop any modeless dialogs that are around.
    ClearCachedDialogs();

    //
    // Shutdown the view
    //

    _view.Deactivate();

    hr = THR(CServer::InPlaceToRunning());

    //
    // Clear these variables so we don't stumble over stale
    // values should we be inplace activated again.
    //

    FormsKillTimer(this, TIMER_ID_MOUSE_EXIT);
    _fMouseOverTimer = FALSE;
    if (_pNodeLastMouseOver)
    {
        CTreeNode * pNodeRelease = _pNodeLastMouseOver;
        _pNodeLastMouseOver = NULL;
        pNodeRelease->NodeRelease();
    }

    nf.DocStateChange1(PrimaryRoot(), (void *)OS_INPLACE);
    BroadcastNotify(&nf);

    if (_pScriptCollection)
    {
        _pScriptCollection->SetState(SCRIPTSTATE_DISCONNECTED);
    }


Cleanup:

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::InPlaceToUIActive, public
//
//  Synopsis:   Overridden helper method in CServer.  Called when we
//              transition from InPlace to UIActive.
//
//  Returns:    HRESULT
//
//  Notes:      If the form is in run mode, then the first control on the
//              tab-stop list is UI Activated in leu of the form.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::InPlaceToUIActive(LPMSG lpmsg)
{
    HRESULT         hr  = S_OK;
    CNotification   nf;
    
    //
    // Try to get into the UI active state by UI activating
    // a contained site. If this works, CServer::InPlaceToUIActivate
    // will be called from CDoc::SetUIActiveSite.  Do this as long as
    // a child within us is not activating.
    //

    if (!_pInPlace->_fChildActivating)
    {
        if (_fDefView && !_fActiveDesktop && _pInPlace->_hwnd)
        {
            // Bring to top of z-order (fix for #77063)
            // BUGBUG (MohanB) Ideally, we would like to bringourselves to the top
            // always, but this would break ActiveDesktop! They keep us behind the
            // transparent ListView which contains the desktop icons.
            SetWindowPos(
                    _pInPlace->_hwnd,
                    HWND_TOP,
                    0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        if (_pElemCurrent != PrimaryRoot())
        {
            hr = THR(_pElemCurrent->BecomeCurrentAndActive(NULL, 0, TRUE));
        }
        else
        {
            ActivateFirstObject(lpmsg);
        }
    }

    // If we couldn't get a site to activate, then activate the document.

    if (_state != OS_UIACTIVE)
    {
        hr = THR(CServer::InPlaceToUIActive(lpmsg));
    }

    _view.SetFocus(_pElemCurrent, _lSubCurrent);

    // Tell the caret to update
    if( _pCaret )
        _pCaret->DeferredUpdateCaret((DWORD_PTR) this);

    IGNORE_HR(OnSelectChange());
    
    if (_fHasOleSite)
    {
        nf.DocStateChange1(PrimaryRoot(), (void *)OS_INPLACE);
        BroadcastNotify(&nf);
    }
    
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::UIActiveToInPlace
//
//  Synopsis:   Effects the U.I. active to inplace-active state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//  Notes:      This method U.I. deactivates any U.I. active embedding
//              in addition to normal CServer base processing.
//
//---------------------------------------------------------------

HRESULT
CDoc::UIActiveToInPlace()
{
    HRESULT         hr;
    CNotification   nf;
    
    //
    //  Tell the editor to lose focus
    //

    hr = THR( NotifySelection( SELECT_NOTIFY_LOSE_FOCUS, NULL ));

    _state = OS_INPLACE;

    if (_fHasOleSite)
    {
        nf.DocStateChange1(PrimaryRoot(), (void *)OS_UIACTIVE);
        BroadcastNotify(&nf);
    }

    _view.SetFocus(NULL, 0);
    
    //  Clear ourselves as the active selection container

    hr = THR(CServer::UIActiveToInPlace());

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetMouseMoveTimer
//
//  Synopsis:   Setup timer to simulate mouse move events.
//
//  Arguments:  uTimeOut    Timer frequency in milliseconds.
//                          If zero, kill the existing timer.
//
//----------------------------------------------------------------------------

void
CDoc::SetMouseMoveTimer(UINT uTimeOut)
{
    Assert(State() >= OS_INPLACE || uTimeOut == 0);

    if (State() >= OS_INPLACE)
    {
        KillTimer(_pInPlace->_hwnd, 1);
        if (uTimeOut)
        {
            SetTimer(_pInPlace->_hwnd, 1, uTimeOut, 0);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DragEnter, IDropTarget
//
//  Synopsis:   Setup for possible drop
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::DragEnter(
        IDataObject * pDataObj,
        DWORD         grfKeyState,
        POINTL        ptlScreen,
        DWORD *       pdwEffect)
{
#ifdef UNIX
    if (_dwTID != GetCurrentThreadId())
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
#endif

    _fRightBtnDrag = (grfKeyState & MK_RBUTTON) ? 1 : 0;

    HRESULT hr = CServer::DragEnter(pDataObj, grfKeyState, ptlScreen, pdwEffect);
    if (hr)
    {
        RRETURN(hr);
    }

    BeginUndoUnit(_T("Drag & Drop"));
    
    Assert(!_pDragDropTargetInfo);
    _pDragDropTargetInfo = new CDragDropTargetInfo( this );
    if (!_pDragDropTargetInfo)
        return E_OUTOFMEMORY;

    RRETURN(THR(DragOver(grfKeyState, ptlScreen, pdwEffect)));
}

CLayout *
GetLayoutForDragDrop(CElement * pElement)
{
    CMarkup * pMarkup;
    CLayout * pLayout = pElement->GetUpdatedLayout();

    // Handle slave markups
    if (!pLayout)
    {
        pMarkup = pElement->GetMarkup();
        if (pMarkup->Master())
        {
            pLayout = pMarkup->Master()->GetUpdatedLayout();
        }
    }
    return pLayout;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DragOver, IDropTarget
//
//  Synopsis:   Handle scrolling and dispatch to site.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::DragOver(DWORD grfKeyState, POINTL ptScreen, DWORD *pdwEffect)
{

    HRESULT     hr              = S_OK;
    POINT       pt;
    CTreeNode * pNodeElement    = NULL;
    DWORD       dwLoopEffect;
    CMessage    msg;
    DWORD       dwScrollDir     = 0;
    BOOL        fRedrawFeedback;
    BOOL        fRet;
    BOOL        fDragEnter;

    Assert(_pInPlace->_pDataObj);

    if (!_pDragDropTargetInfo)
    {
        AssertSz(0, "DragDropTargetInfo is NULL. Possible cause - DragOver called without calling DragEnter");
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    pt = *(POINT *)&ptScreen;
    ScreenToClient(_pInPlace->_hwnd, &pt);

    msg.pt = pt;
    HitTestPoint(&msg, &pNodeElement, HT_IGNOREINDRAG);
    if ( !pNodeElement )
        goto Cleanup;
        
    // Point to the content
    if (pNodeElement->Element()->HasSlaveMarkupPtr())
    {
        pNodeElement = pNodeElement->Element()->GetSlaveMarkupPtr()->FirstElement()->GetFirstBranch();
    }

    // We should not change pdwEffect unless DragEnter succeeded, and always call
    // DragEnter with the original effect
    dwLoopEffect = *pdwEffect;

    if (pNodeElement->Element() != _pDragDropTargetInfo->_pElementHit)
    {
        fDragEnter = TRUE;
        _pDragDropTargetInfo->_pElementHit = pNodeElement->Element();
    }
    else
        fDragEnter = FALSE;

    while (pNodeElement)
    {
        CLayout * pLayout = GetLayoutForDragDrop(pNodeElement->Element());

        pNodeElement->NodeAddRef();

        if (pNodeElement->Element() == _pDragDropTargetInfo->_pElementTarget)
        {
            fRet = pNodeElement->Element()->Fire_ondragHelper(
                0,
                DISPID_EVMETH_ONDRAGOVER,
                DISPID_EVPROP_ONDRAGOVER,
                _T("dragover"),
                pdwEffect);

            if (pNodeElement->IsDead())
            {
                pNodeElement->NodeRelease();
                goto Cleanup;
            }
            if (fRet)
            {
                if (pLayout)
                    hr = THR(pLayout->DragOver(grfKeyState, ptScreen, pdwEffect));
                else
                    *pdwEffect = DROPEFFECT_NONE;
            }
            break;
        }

        if (fDragEnter)
        {
            fRet = pNodeElement->Element()->Fire_ondragHelper(
                0,
                DISPID_EVMETH_ONDRAGENTER,
                DISPID_EVPROP_ONDRAGENTER,
                _T("dragenter"),
                &dwLoopEffect);

            if (pNodeElement->IsDead())
            {
                pNodeElement->NodeRelease();
                goto Cleanup;
            }
            if (fRet)
            {
                if (pLayout)
                {
                    hr = THR(pLayout->DragEnter(_pInPlace->_pDataObj, grfKeyState, ptScreen, &dwLoopEffect));
                    if (hr != S_FALSE)
                        break;
                }
            }
            else
                break;
        }

        // BUGBUG: what if pNodeElement goes away on this first release?!?
        pNodeElement->NodeRelease();
        pNodeElement = pNodeElement->GetUpdatedParentLayoutNode();
        dwLoopEffect = *pdwEffect;
    }
    if ( pNodeElement && DifferentScope(pNodeElement, _pDragDropTargetInfo->_pElementTarget) )
    {
        if (_pDragDropTargetInfo->_pElementTarget && _pDragDropTargetInfo->_pElementTarget->GetFirstBranch())
        {
            CLayout * pLayout;

            _pDragDropTargetInfo->_pElementTarget->Fire_ondragleave();

            pLayout = GetLayoutForDragDrop(_pDragDropTargetInfo->_pElementTarget);
            if (pLayout)
                IGNORE_HR(pLayout->DragLeave());
        }

        _pDragDropTargetInfo->_pElementTarget = pNodeElement->Element();
        *pdwEffect = dwLoopEffect;
    }

    if (NULL == _pDragDropTargetInfo->_pElementTarget)
    {
        *pdwEffect = DROPEFFECT_NONE;
    }


    // Find the site to scroll and the direction, if any

    // BUGBUG (MohanB) GetSite/GetLayout may not appropriate in case of <AREA>.
    // Here, we really need to use the corresponding image, not the site that
    // contains the area/map in the source. We should try to fix this in the new
    // layout code.        
    if (pNodeElement)
    {
        // BUGBUG: what if pNodeElement goes away on this first release!?!?
        pNodeElement->NodeRelease();
        if (pNodeElement->IsDead())
            goto Cleanup;
    }

    {
        Assert(_view.IsActive());

        CDispScroller * pDispScroller = _view.HitScrollInset((CPoint *) &pt, &dwScrollDir);

        if (pDispScroller)
        {
            Assert(dwScrollDir);
            *pdwEffect |= DROPEFFECT_SCROLL;

            if (_pDragDropTargetInfo->_pDispScroller == pDispScroller)
            {
                if (IsTimePassed(_pDragDropTargetInfo->_uTimeScroll))
                {
                    CSize sizeOffset;
                    CSize sizePercent(0, 0);
                    CSize sizeDelta;
                    CRect rc;

                    // Hide drag feedback while scrolling
                    fRedrawFeedback = _fDragFeedbackVis;
                    if (_fDragFeedbackVis)
                    {
                        CLayout * pLayout;

                        Assert(_pDragDropTargetInfo->_pElementTarget);

                        pLayout = GetLayoutForDragDrop(_pDragDropTargetInfo->_pElementTarget);
                        if (pLayout)
                            pLayout->DrawDragFeedback();
                    }

                    // open display tree for scrolling
                    Verify(_view.OpenView());

                    pDispScroller->GetClientRect(&rc, CLIENTRECT_CONTENT);
                    pDispScroller->GetScrollOffset(&sizeOffset);

                    if (dwScrollDir & SCROLL_LEFT)
                        sizePercent.cx = -SCROLLPERCENT;
                    else if (dwScrollDir & SCROLL_RIGHT)
                        sizePercent.cx = SCROLLPERCENT;
                    if (dwScrollDir & SCROLL_UP)
                        sizePercent.cy = -SCROLLPERCENT;
                    else if (dwScrollDir & SCROLL_DOWN)
                        sizePercent.cy = SCROLLPERCENT;

                    sizeDelta.cx = (sizePercent.cx ? (rc.Width() * sizePercent.cx) / 1000L : 0);
                    sizeDelta.cy = (sizePercent.cy ? (rc.Height() * sizePercent.cy) / 1000L : 0);

                    sizeOffset += sizeDelta;
                    sizeOffset.Max(g_Zero.size);

                    if( _pCaret )
                    {
                        _pCaret->BeginPaint();
                    }
                    
                    pDispScroller->SetScrollOffset(sizeOffset, TRUE);
                    
                    if( _pCaret )
                    {
                        _pCaret->EndPaint();
                    }
                    
                    //  Ensure all deferred calls are executed
                    _view.EndDeferred();

                    // Show drag feedback after scrolling is finished
                    if (fRedrawFeedback)
                    {
                        CLayout * pLayout;

                        Assert(_pDragDropTargetInfo->_pElementTarget);

                        pLayout = GetLayoutForDragDrop(_pDragDropTargetInfo->_pElementTarget);
                        if (pLayout)
                            pLayout->DrawDragFeedback();
                    }

                    // Wait a while before scrolling again
                    _pDragDropTargetInfo->_uTimeScroll = NextEventTime(g_iDragScrollInterval / 2);
                }
            }
            else
            {
                _pDragDropTargetInfo->_pDispScroller = pDispScroller;
                _pDragDropTargetInfo->_uTimeScroll = NextEventTime(g_iDragScrollDelay);
            }
        }
        else
        {
            _pDragDropTargetInfo->_pDispScroller = NULL;
        }
    }

Cleanup:
    // S_FALSE from DragOver() does not make sense for OLE. Also, since we
    // call DragOver() from within DragEnter(), we DO NOT want to return
    // S_FALSE because that would result in OLE passing on DragOver() to
    // the parent drop target.
    if (hr == S_FALSE)
        hr = S_OK;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DragLeave, IDropTarget
//
//  Synopsis:   Remove any user feedback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::DragLeave(BOOL fDrop)
{
    if (_pDragDropTargetInfo)
    {
        if (_pDragDropTargetInfo->_pElementTarget)
        {
            CLayout * pLayout;

            //BUGBUG (t-jeffg) This makes sure that when dragging out of the source window
            //the dragged item will be deleted.  Will be removed when _pdraginfo is dealt
            //with properly on leave.  (Anandra)
            _fSlowClick = FALSE;

            if (!fDrop)
                _pDragDropTargetInfo->_pElementTarget->Fire_ondragleave();

            pLayout = GetLayoutForDragDrop(_pDragDropTargetInfo->_pElementTarget);
            if (pLayout)
                IGNORE_HR(pLayout->DragLeave());
        }
        if ( ! fDrop && _fDesignMode && _pDragDropTargetInfo )
        {
            _pDragDropTargetInfo->RestoreSelection();    
        }            
        delete _pDragDropTargetInfo;
        _pDragDropTargetInfo = NULL;
               
        if ( _pCaret )
        {
            _pCaret->LoseFocus();
        }        
    }

    EndUndoUnit();

    RRETURN(CServer::DragLeave(fDrop));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Drop, IDropTarget
//
//  Synopsis:   Handle the drop operation
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Drop(
    IDataObject *pDataObj,
    DWORD        grfKeyState,
    POINTL       ptScreen,
    DWORD *      pdwEffect)
{
    HRESULT   hr = S_OK;
    CLayout * pLayout;

    CCurs   curs(IDC_WAIT);
    DWORD   dwEffect = *pdwEffect;

    hr = THR(DragOver(grfKeyState, ptScreen, &dwEffect));

    // Continue only if drop can happen (i.e. at least one of DROPEFFECT_COPY,
    // DROPEFFECT_MOVE, etc. is set in dwEffect).
    if (hr || DROPEFFECT_NONE == dwEffect || DROPEFFECT_SCROLL == dwEffect)
        goto Cleanup;

    Assert(_pDragDropTargetInfo);
    if (!_pDragDropTargetInfo->_pElementTarget)
    {
        *pdwEffect = DROPEFFECT_NONE;
        goto Cleanup;
    }

    if (_pDragDropTargetInfo->_pElementTarget->Fire_ondragHelper(
                0,
                DISPID_EVMETH_ONDROP,
                DISPID_EVPROP_ONDROP,
                _T("drop"),
                pdwEffect))
    {
        if (!_pDragDropTargetInfo->_pElementTarget)
        {
            *pdwEffect = DROPEFFECT_NONE;
            goto Cleanup;
        }

        pLayout = GetLayoutForDragDrop(_pDragDropTargetInfo->_pElementTarget);
        if (pLayout)
            hr = THR(pLayout->Drop(pDataObj, grfKeyState, ptScreen, pdwEffect));
        else
            *pdwEffect = DROPEFFECT_NONE;
    }

    // Drop can change the current site.  Set the UI active
    // site to correspond to the current site.

    if (_pElemUIActive != _pElemCurrent)
    {
        _pElemCurrent->BecomeUIActive();
    }

Cleanup:
    IGNORE_HR(DragLeave( hr == S_OK ));    

    RRETURN1(hr,S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ReactivateAndUndo, public
//
//  Synopsis:   Transitions us to UI Active and performs an undo.  This is
//              called if we just deactivated and the user selected our
//              parent's Undo option.
//
//  Arguments:  (none)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ReactivateAndUndo(void)
{
    HRESULT      hr;
    CDoc::CLock Lock(this);

    _fNoUndoActivate = TRUE;

    TransitionTo(OS_UIACTIVE);

#ifdef NO_EDIT
    hr = S_OK;
#else
    hr = THR(EditUndo());
#endif // NO_EDIT

    _fNoUndoActivate = FALSE;

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     DoTranslateAccelerator, public
//
//  Synopsis:   Overridden method of IOleInPlaceActiveObject
//
//  Arguments:  [lpmsg] -- Message to translate
//
//  Returns:    S_OK if translated, S_FALSE if not.  Error otherwise
//
//  History:    07-Feb-94     LyleC    Created
//              08-Feb-95     AndrewL  Turned off noisy trace.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::DoTranslateAccelerator(LPMSG lpmsg)
{
    HRESULT  hr = S_FALSE;
    CMessage Message(lpmsg);

    Assert(_pElemCurrent && _pElemCurrent->GetFirstBranch());

    CTreeNode::CLock Lock(_pElemCurrent->GetFirstBranch());

    switch (lpmsg->message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        break;
    default:
        goto Cleanup;
    }

    //
    // Do not handle any messages if our window or one of our children
    // does not have focus.
    //

    // BUGBUG (sujalp): We should not be dispatching this message from here.
    // It should be dispatched only from CDoc::OnWindowMessage. When this
    // problem is fixed up, remove the following switch.
    switch (lpmsg->message)
    {
    case WM_KEYDOWN:
        _fGotKeyDown = TRUE;
        // if we are Tabbing into Trident the first time directly from address bar
        // do not tab into address bar again on next TAB. (sramani: see bug#28426)

        // CHROME
        // When Chrome hosted we have no HWND so use CServer::GetFocus() rather
        // than ::GetFocus()
        if (!IsChromeHosted())
        {
            if (lpmsg->wParam == VK_TAB && ::GetFocus() != _pInPlace->_hwnd)
                _fFirstTimeTab = FALSE;
        }
        else
        {
            if (lpmsg->wParam == VK_TAB && !GetFocus())
                _fFirstTimeTab = FALSE;
        }

        break;

    case WM_KEYUP:
        if (!_fGotKeyDown)
            return S_FALSE;
        break;
    }

    // If there was no captured object, or if the capture object did not
    // handle the message, then lets pass it to the current site
    hr = THR(PumpMessage(&Message, _pElemCurrent->GetFirstBranch(), TRUE));

Cleanup:
    RRETURN1_NOTRACE(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CallParentTA, public
//
//  Synopsis:   Calls the parent's TranslateAccelerator method and returns
//              TRUE if the parent handled the message.
//
//  Arguments:  pmsg    Message for parent to translate.  If NULL,
//                      then this function always returns FALSE.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CallParentTA(CMessage * pmsg)
{
    HRESULT             hr;
    IOleControlSite *   pCtrlSite;

    Assert(_pClientSite);

    //  If we didn't have a message to translate, no need to call
    //    to our parent.  It is OK for the parent to not
    //    support IOleControlSite. In that case we just indicate
    //    the event was not handled.

    if (pmsg &&
            _pClientSite &&
            !THR_NOTRACE(_pClientSite->QueryInterface(
                IID_IOleControlSite,
                (void **) &pCtrlSite)))
    {
        hr = THR(pCtrlSite->TranslateAccelerator(
                                pmsg,
                                VBShiftState(pmsg->dwKeyState)));
        pCtrlSite->Release();
        if (FAILED(hr))
            hr = S_FALSE;
    }
    else
    {
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     Form::ActivateFirstObject
//
//  Synopsis:   Activate the first object in the tab order.
//
//  Arguments:  lpmsg       Message which prompted this rotation, or
//                          NULL if no message is available
//              pSiteStart  Site to start the search at.  If null, start
//                          at beginning of tab order.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ActivateFirstObject(LPMSG lpmsg)
{
    HRESULT     hr              = S_OK;
    CElement *  pElementClient  = GetPrimaryElementTop();
    BOOL        fDeferActivate  = FALSE;

    if( pElementClient == NULL )
        goto Cleanup;


    if ( pElementClient && ( _fDesignMode || pElementClient->Tag() == ETAG_FRAMESET ))
    {
        // In design mode or in frameset case, just activate the ped.
        //
        hr = THR(pElementClient->BecomeCurrentAndActive(NULL, 0, TRUE));
        goto Cleanup;
    }

    if (_fInHTMLDlg || !_fMsoDocMode
                    || ((_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG) != 0))
    {
        // only if we are a dialog or no one is activating us as a doc obj

        if (LoadStatus() < LOADSTATUS_PARSE_DONE)
        {
            // We need to wait until the doc is fully parsed before we can
            // determine the first tabbable object (IE5 #73116).
            fDeferActivate = !_fCurrencySet;
        }
        else
        {
            CElement *      pElement    = NULL;
            long            lSubNext    = 0;
            FOCUS_DIRECTION dir         = DIRECTION_FORWARD;

            if (lpmsg && (lpmsg->message == WM_KEYDOWN ||
                          lpmsg->message == WM_SYSKEYDOWN))
            {
                if (GetKeyState(VK_SHIFT) & 0x8000)
                {
                    dir = DIRECTION_BACKWARD;
                }
            }

            FindNextTabOrder(dir, NULL, 0, &pElement, &lSubNext);
            if (pElement)
            {
                Assert(pElement->IsTabbable(lSubNext));
                hr = THR(pElement->BecomeCurrentAndActive(NULL, lSubNext, TRUE));
                if (hr)
                    goto Cleanup;

                IGNORE_HR(THR(pElement->ScrollIntoView()));
                _fFirstTimeTab = FALSE;                
                goto Cleanup;
            }
        }
    }

    hr = THR(GetPrimaryElementTop()->BecomeCurrentAndActive(NULL, 0, TRUE));
    // Although we have set the currency, if we have not completed the 
    // parsing yet, we were only able to set it to the element client. Once
    // the parsing is completed, it will be set on the proper tab-stop element.
    // We ignore the _fCurrencySet value set by the BecomeCurrentAndActive call.
    if (fDeferActivate)
    {
        _fCurrencySet = FALSE;
    }

    // Fire window onfocus here the first time if not fired before in
    // onloadstatus when it goes to LOADSTATUS_DONE
    Fire_onfocus();

Cleanup:
    RRETURN1(hr, S_FALSE) ;
}
