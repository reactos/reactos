//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       sinpl.cxx
//
//  Contents:   Implementation of the CServer inplace functionality
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

DeclareTag(tagCServer, "Server", "Server base class stuff")
DeclareTag(tagRects,   "ServerRects", "Position rect, clip rect, extent")
MtDefine(CInPlace, CDoc, "CInPlace")

extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);

#ifndef NO_IME
extern BOOL DIMMHandleDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
#endif

#ifndef WS_EX_LAYOUTRTL // winuser.h defines this for WINVER >= 0x0500
#define WS_EX_LAYOUTRTL 0x00400000L //Right-to-left mirroring, as used in NT5
#endif

//  Size of in-place border

#define CX_IPBORDER     4
#define CY_IPBORDER     4
#define CX_HANDLEINSET  1
#define CY_HANDLEINSET  1


//+---------------------------------------------------------------------------
//
//  Function:   DrawUIActiveBorder
//
//  Synopsis:   Draw the UI Active border
//
//  Arguments:  hdcDraw     Draw into this dc.
//              prc         Draw border inside this rectangle.
//              fHatch      If true, draw hatch border
//              fHandles    If true, draw grab handles
//
//----------------------------------------------------------------------------

void
DrawUIActiveBorder(HDC hdcDraw, RECT * prc, BOOL fHatch, BOOL fHandles)
{
    int     i;
    GDIRECT    rc;
    HBRUSH  hbr;

    if (fHatch)
    {
        hbr = GetCachedBmpBrush(IDR_HATCHBMP);

        SetTextColor(hdcDraw, RGB(0, 0, 0));
        SetBkColor(hdcDraw, RGB(255, 255, 255));

        for (i = 0; i < 4; i++)
        {
#ifdef WIN16
            CopyRect(&rc, prc);
#else
            rc = *prc;
#endif

            switch (i)
            {
                case 0:  rc.right  = rc.left   + CX_IPBORDER; break;
                case 1:  rc.left   = rc.right  - CX_IPBORDER; break;
                case 2:  rc.bottom = rc.top    + CY_IPBORDER; break;
                case 3:  rc.top    = rc.bottom - CY_IPBORDER; break;
            }

            FillRect(hdcDraw, &rc, hbr);
        }
    }

    if (fHandles)
    {
        hbr = (HBRUSH) GetStockObject(BLACK_BRUSH);

        //
        // Grab handle rectangles:
        //
        //   +-----+------+----+------+-----+
        //   |  0  |      | 1  |      |  2  |
        //   +---+-+      +----+      +-+---+
        //   | 3 |                      | 4 |
        //   +---+                      +---+
        //   |                              |
        //   |                              |
        //   +---+                      +---+
        //   | 5 |                      | 6 |
        //   +---+                      +---+
        //   |                              |
        //   |                              |
        //   +---+                      +---+
        //   | 7 |                      | 8 |
        //   +---+-+      +----+      +-+---+
        //   |  9  |      | 10 |      | 11  |
        //   +-----+------+----+------+-----+
        //

        for (i = 0; i < 12; i++)
        {

            // Compute left and right of rectangle.

            switch (i)
            {
                case 3:
                case 5:
                case 7:
                    rc.left = prc->left;
                    rc.right = rc.left + CX_IPBORDER;
                    break;

                case 0:
                case 9:
                    rc.left = prc->left;
                    rc.right = rc.left + CX_IPBORDER + CX_HANDLEINSET;
                    break;

                case 1:
                case 10:
                    rc.left = (prc->left + prc->right - CX_IPBORDER) / 2;
                    rc.right = rc.left + CX_IPBORDER + CX_HANDLEINSET;
                    break;

                case 2:
                case 11:
                    rc.right = prc->right;
                    rc.left = rc.right - CX_IPBORDER - CX_HANDLEINSET;
                    break;

                case 4:
                case 6:
                case 8:
                    rc.right = prc->right;
                    rc.left = rc.right - CX_IPBORDER;
                    break;
            }

            // Compute top and bottom of rectangle.

            switch (i)
            {
                case 0:
                case 1:
                case 2:
                    rc.top = prc->top;
                    rc.bottom = rc.top + CY_IPBORDER;
                    break;

                case 3:
                case 4:
                    rc.top = prc->top + CY_IPBORDER;
                    rc.bottom = rc.top + CY_HANDLEINSET;
                    break;

                case 5:
                case 6:
                    rc.top = (prc->top + prc->bottom - CY_IPBORDER) / 2;
                    rc.bottom = rc.top + CY_IPBORDER + CY_HANDLEINSET;
                    break;

                case 7:
                case 8:
                    rc.top = prc->bottom - CY_IPBORDER - CY_HANDLEINSET;
                    rc.bottom = rc.top + CY_HANDLEINSET;
                    break;

                case 9:
                case 10:
                case 11:
                    rc.bottom = prc->bottom;
                    rc.top = rc.bottom - CY_IPBORDER;
                    break;
            }

            FillRect(hdcDraw, &rc, hbr);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CInPlace
//
//  Synopsis:   CInPlace object constructor
//
//  Arguments:  [pServer] -- CServer object that owns us
//
//----------------------------------------------------------------------------

CInPlace::CInPlace()
{
    _fUIDown        = TRUE;
    _fFrameActive   = TRUE;
    _fDocActive     = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CInPlace
//
//  Synopsis:   CInPlace object destructor
//
//----------------------------------------------------------------------------

CInPlace::~CInPlace()
{
    ClearInterface(&_pInPlaceSite);
    ClearInterface(&_pFrame);
    ClearInterface(&_pDoc);
    ClearInterface(&_pDataObj);
    Assert(!_hwnd);
}

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
CServer::EnsureInPlaceObject()
{
    if (!_pInPlace)
    {
        _pInPlace = new CInPlace();
        if (!_pInPlace)
            RRETURN(E_OUTOFMEMORY);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::ActivateInPlace, CServer
//
//  Synopsis:   In-place activates the object
//
//  Returns:    Success if we in-place activated properly
//
//  Notes:      This method implements the standard in-place activation
//              protocol and creates the in-place window.
//
//----------------------------------------------------------------------------

HRESULT
CServer::ActivateInPlace(LPMSG lpmsg)
{
    HRESULT     hr;
    HWND        hWndSite;
    RECT        rcPos;
    RECT        rcVisible;
    BOOL        fUsingWindowlessSite = FALSE;
    BOOL        fNoRedraw = FALSE;
    WORD        wLockFlags;

    if (!_pClientSite)
        RRETURN(E_UNEXPECTED);

    hr = THR(EnsureInPlaceObject());
    if (hr)
        RRETURN(hr);

    Assert(_pInPlace);
    Assert(_pInPlace->_fWindowlessInplace == FALSE);
    _pInPlace->_fDeactivating = FALSE;

    // If we were not handed an inplace site through the
    // docobj interfaces, then negotiate for one now.

    if (!_pInPlace->_pInPlaceSite)
    {
        if (OK(_pClientSite->QueryInterface(
                    IID_IOleInPlaceSiteWindowless,
                    (void **)&_pInPlace->_pInPlaceSite)))
        {
            fUsingWindowlessSite = TRUE;
            _pInPlace->_fUseExtendedSite = TRUE;
            // CHROME
            if (IsChromeHosted())
            {
                // Set member variable denoting that we have found
                // a windowless site.
                _fWindowless = TRUE;
            }
        }
        else if (OK(_pClientSite->QueryInterface(
                IID_IOleInPlaceSiteEx,
                (void **)&_pInPlace->_pInPlaceSite)))
        {
            _pInPlace->_fUseExtendedSite = TRUE;
        }
        else
        {
            hr = THR(_pClientSite->QueryInterface(
                    IID_IOleInPlaceSite,
                    (void **) &(_pInPlace->_pInPlaceSite)));
            if (hr)
                goto Error;
        }
    }

    // CHROME
    // At this point we must have a windowless site if we are Chrome
    // hosted. If not we fail (can't be Chrome hosted and windowed). 
    if (IsChromeHosted())
    {
        Assert(_fWindowless);
        if (!_fWindowless)
        {
            hr = E_FAIL;
            goto Error;
        }
        fUsingWindowlessSite = TRUE;
    }

    Assert(_pInPlace->_pInPlaceSite != NULL);

    hr = THR(_pInPlace->_pInPlaceSite->CanInPlaceActivate());
    if (hr == S_FALSE)
    {
        TraceTag((tagError, "Container refused InPlace activation!"));

        hr = E_FAIL;
        goto Error;
    }
    else if (hr)
        goto Error;

    if (_fWindowless && fUsingWindowlessSite)
    {
        hr = ((IOleInPlaceSiteWindowless *)
                (_pInPlace->_pInPlaceSite))->CanWindowlessActivate();
        _pInPlace->_fWindowlessInplace = (hr == S_OK);
    }

    // CHROME
    // A Chrome hosted document should never have its site refuse
    // windowless activation.
    if (IsChromeHosted())
    {
        Assert(_pInPlace->_fWindowlessInplace);
        if (!_pInPlace->_fWindowlessInplace)
        {
            hr = E_FAIL;
            goto Error;
        }
    }

    // get information about position, size etc.

    _pInPlace->_frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);

    hr = THR(_pInPlace->_pInPlaceSite->GetWindowContext(
                                        &_pInPlace->_pFrame,
                                        &_pInPlace->_pDoc,
                                        ENSUREOLERECT(&_pInPlace->_rcPos),
                                        ENSUREOLERECT(&_pInPlace->_rcClip),
                                        &_pInPlace->_frameInfo));
    if (hr)
        goto Error;

    if (_fMsoDocMode)
    {
        // Our _sizel has to match our view rect

        SIZEL sizel;

        sizel.cx = HimetricFromHPix(_pInPlace->_rcPos.right - _pInPlace->_rcPos.left);
        sizel.cy = HimetricFromVPix(_pInPlace->_rcPos.bottom - _pInPlace->_rcPos.top);

        SetExtent(DVASPECT_CONTENT, &sizel);
    }

    // Check for the container requesting an infinite scale factor.

    if ((_sizel.cx == 0 && _pInPlace->_rcPos.right - _pInPlace->_rcPos.left != 0) ||
        (_sizel.cy == 0 && _pInPlace->_rcPos.bottom - _pInPlace->_rcPos.top != 0))
    {
        Assert(0 && "Host error: Infinite scale factor. Not a Forms³ error.");
        hr = E_FAIL;
        goto Error;
    }

    TraceTag((tagRects,
            "%08lx ActivateInPlace context pos=%ld %ld %ld %ld; clip=%ld %ld %ld %ld",
            this, _pInPlace->_rcPos, _pInPlace->_rcClip));

    DbgTrackItf(IID_IOleInPlaceFrame, "HostFrame", TRUE, (void **)&_pInPlace->_pFrame);

    TraceTag((tagRects, "%08lx ActivateInPlace extent=%ld %ld", this, _sizel.cx, _sizel.cy));

    if (!_pInPlace->_fWindowlessInplace)
    {
        rcPos = _pInPlace->_rcPos;

        _pInPlace->_ptWnd  = *(POINT *)&_pInPlace->_rcPos;
        OffsetRect(&_pInPlace->_rcPos,
                -_pInPlace->_ptWnd.x, -_pInPlace->_ptWnd.y);
        OffsetRect(&_pInPlace->_rcClip,
                -_pInPlace->_ptWnd.x, -_pInPlace->_ptWnd.y);

        hr = THR(_pInPlace->_pInPlaceSite->GetWindow(&hWndSite));
        if (hr)
            goto Error;

        TraceTag((tagRects, "%08lx ActivateInPlace > AttachWin %ld %ld %ld %ld", this, rcPos));

        hr = THR(AttachWin(hWndSite, &rcPos, &_pInPlace->_hwnd));
        if (hr)
            goto Error;

        IntersectRect(&rcVisible, &_pInPlace->_rcPos, &_pInPlace->_rcClip);
        if (!EqualRect(&rcVisible, &_pInPlace->_rcPos))
        {
            _pInPlace->_fUsingWindowRgn = TRUE;
            SetWindowRgn(_pInPlace->_hwnd, CreateRectRgnIndirect(&rcVisible), FALSE);
        }

    }
    else
    {
        _pInPlace->_ptWnd.x = _pInPlace->_ptWnd.y = 0;
    }

    //  Notify our container that we are going in-place active.
    //    Since the container may move us to some new state during
    //    the notification, we need to remember that we're already
    //    transitioning to OS_INPLACE.  If the container refuses
    //    the activation, then we unwind the partial transiton.

    _state = OS_INPLACE;

    wLockFlags = Unlock(SERVERLOCK_TRANSITION);

    if (_pInPlace->_fUseExtendedSite)
    {
        hr = THR(((IOleInPlaceSiteEx *)_pInPlace->_pInPlaceSite)->
                OnInPlaceActivateEx(&fNoRedraw,
                    _pInPlace->_fWindowlessInplace ? ACTIVATE_WINDOWLESS : 0));
    }
    else
    {
        hr = THR(_pInPlace->_pInPlaceSite->OnInPlaceActivate());
    }

    Relock(wLockFlags);

    if (hr)
    {
        if (_state == OS_INPLACE)
            _state = OS_RUNNING;

        goto Error;
    }

    if (!fNoRedraw)
    {
        InvalidateRect(NULL, TRUE);
    }

Cleanup:
    RRETURN(hr);

Error:
    IGNORE_HR(DeactivateInPlace());
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUpdateEnum
//
//  Synopsis:   Called from EnumChildWindows, used to determine if
//              there's an update region for a child window.
//
//----------------------------------------------------------------------------

static BOOL CALLBACK
GetUpdateEnum(HWND hwnd, LPARAM lparam)
{
    if (GetUpdateRect(hwnd, (RECT *)NULL, 0))
    {
        *(BOOL *)lparam = TRUE;
        return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     CServer::DeactivateInPlace, CServer
//
//  Synopsis:   In-place deactivates the object
//
//  Returns:    Success except for catastophic circumstances
//
//  Notes:      This method "undoes" everything done in ActivateInPlace
//              including destroying the inplace active window.
//
//---------------------------------------------------------------

HRESULT
CServer::DeactivateInPlace()
{
    IOleInPlaceSite *   pInPlaceSite;
    BOOL                fRedraw;
    WORD                wLockFlags;

    Assert(_pInPlace);

    if (_pInPlace->_fUseExtendedSite && _pInPlace->_hwnd)
    {
        Assert(IsWindow(_pInPlace->_hwnd));
        fRedraw = GetUpdateRect(_pInPlace->_hwnd, (RECT *)NULL, 0);
        if (!fRedraw)
        {
            EnumChildWindows(_pInPlace->_hwnd, GetUpdateEnum, (LPARAM)&fRedraw);
        }
        SetWindowPos(_pInPlace->_hwnd, NULL, 0, 0, 0, 0,
                SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE |
                SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
    }
    else
    {
        fRedraw = FALSE;
    }

    DetachWin();

    ClearInterface(&_pInPlace->_pFrame);
    ClearInterface(&_pInPlace->_pDoc);

    //  We use the in place site to send notification below, but
    //    we want to be fully deactivated before we make the call
    //    so that the container can reenter us.  To make this easier,
    //    we NULL out our member variable, but save the value for
    //    later use (and Release()'ing)

    pInPlaceSite = _pInPlace->_pInPlaceSite;

    //
    // Only do this if we were not activated as a docobject.
    // Is it ok to skip all the notifications down below?
    //
    
    if (!_fMsoDocMode)
    {
        _pInPlace->_pInPlaceSite = NULL;
    }

    _pInPlace->_fWindowlessInplace = FALSE;

    //  Notify our container that we're in-place deactivating

    Assert(_state == OS_INPLACE || _state == OS_RUNNING);
    if (_state == OS_INPLACE)
    {
        //  The container may reenter us, so need to remember that
        //    we've done almost all the transition to OS_RUNNING

        _state = OS_RUNNING;

        //  Errors from this notification are ignored (in the function
        //    which calls this one); we don't allow our container to stop
        //    us from in-place deactivating

        if (pInPlaceSite)
        {
            //  We allow the container to reenter us during this
            //    transition, so we need to unlock ourselves

            wLockFlags = Unlock(SERVERLOCK_TRANSITION);

            if (_pInPlace->_fUseExtendedSite)
            {
                IGNORE_HR(((IOleInPlaceSiteEx *)pInPlaceSite)->
                        OnInPlaceDeactivateEx(!fRedraw));
            }
            else
            {
                IGNORE_HR(pInPlaceSite->OnInPlaceDeactivate());
            }

            Relock(wLockFlags);
        }
    }

    if (!_fMsoDocMode)
    {
        ReleaseInterface(pInPlaceSite);
    }

    delete _pInPlace;
    _pInPlace = NULL;

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::ActivateUI, CServer
//
//  Synopsis:   Notifies container of U.I. activation and installs our
//              U.I. elements.
//
//  Returns:    Success if our container granted permission to U.I. activate.
//
//  Notes:      Installing our U.I. (border toolbars, floating palettes,
//              adornments) is accomplished via a virtual call to InstallUI.
//
//---------------------------------------------------------------

HRESULT
CServer::ActivateUI(LPMSG lpmsg)
{
    HRESULT     hr;
    WORD        wLockFlags;

    Assert(_pInPlace);
    Assert(_pInPlace->_pInPlaceSite);

    //  We must remember we're in the process of UI Activating,
    //    since the container may move us to some new state
    //    in its notification method

    _state = OS_UIACTIVE;

    //  We allow the container to reenter us during this notification,
    //    so we need to unlock ourselves

    wLockFlags = Unlock(SERVERLOCK_TRANSITION);

    hr = THR(_pInPlace->_pInPlaceSite->OnUIActivate());

    Relock(wLockFlags);

    if (hr)
    {
        //  If the container fails the OnUIActivate call, then we
        //    give up and stay IPA

        if (_state == OS_UIACTIVE)
            _state = OS_INPLACE;

        goto Cleanup;
    }

    //  BUGBUG this is almost (but not quite) robust enough.  If by
    //    some quirk of fate the container caused us to de-UIActivate,
    //    then re-UIActivate us in the OnUIActivate call, then we
    //    would mistakenly re-install our UI below

    if (_state == OS_UIACTIVE && !_pInPlace->_fChildActivating)
    {
        hr = THR(InstallUI());
        if (hr)
            goto Error;

        if (!_fMsoDocMode &&
            (GetAmbientBool(DISPID_AMBIENT_SHOWHATCHING, TRUE) ||
             GetAmbientBool(DISPID_AMBIENT_SHOWGRABHANDLES, TRUE)))
        {
            ShowUIActiveBorder(TRUE);
        }
    }

Cleanup:
    RRETURN(hr);

Error:
    DeactivateUI();
    goto Cleanup;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DeactivateUI, CServer
//
//  Synopsis:   Removes any U.I. we have installed and notifies our container
//              that we are no longer U.I. active
//
//  Returns:    Success except for catastrophic circumstances
//
//  Notes:      This method "undoes" everything done in ActivateUI.
//              U.I. elements are removed via a virtual call to RemoveUI
//
//---------------------------------------------------------------

HRESULT
CServer::DeactivateUI(void)
{
    WORD wLockFlags;

    //  Remove any UI that is up

    RemoveUI();
    ShowUIActiveBorder(FALSE);

    Assert(_pInPlace);

    _state = OS_INPLACE;

    //  Notify our container that we have deactivated.  Note that
    //    we don't let our container prevent us from UI Deactivating

    //  We need to unlock ourselves in case the container wants
    //    to reenter us during the OnUIDeactivate notification

    wLockFlags = Unlock(SERVERLOCK_TRANSITION);

    IGNORE_HR(_pInPlace->_pInPlaceSite->OnUIDeactivate(FALSE));

    Relock(wLockFlags);

    return S_OK; 
}


//+---------------------------------------------------------------
//
//  Member:     CServer::InstallUI, CServer
//
//  Synopsis:   Installs previously created U.I. so it is displayed to
//              the user.
//
//  Notes:      This method will call the installframeUI and InstallDocUI
//              methods to install those U.I. elements, respectively.
//
//---------------------------------------------------------------

HRESULT
CServer::InstallUI(
    BOOL    fSetFocus)
{
    HRESULT hr = S_OK;

    if (!_pInPlace->_fChildActivating && !_pInPlace->_fDeactivating)
    {
        _pInPlace->_fUIDown = FALSE;

#ifndef NO_OLEUI
        hr = THR(InstallFrameUI());
        if (hr)
            goto Error;

        hr = THR(InstallDocUI());
        if (hr)
            goto Error;
#endif // NO_OLEUI

        // Set focus if requested
        // BUGBUG: This could perhaps be better tracked by watching the activation
        //         state of our container rather than passing flags into this routine.
        //         Unfortunately, tracking activation is difficult in the 96 code base.
        if (fSetFocus)
        {
            SetFocus(TRUE);
        }
    }

Cleanup:
    RRETURN(hr);

Error:
    RemoveUI();
    goto Cleanup;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::RemoveUI, CServer
//
//  Synopsis:   Removes previously installed U.I. so it is hidden from the
//              the user.
//
//  Notes:      This method "undoes" everything done in InstallUI.  It calls
//              the RemoveFrameUI and RemoveDocUI methods.
//
//---------------------------------------------------------------

void
CServer::RemoveUI(void)
{
    if (!_pInPlace->_fUIDown)
    {
        _pInPlace->_fUIDown = TRUE;

#ifndef NO_OLEUI
        RemoveDocUI();
        RemoveFrameUI();
#endif // NO_OLEUI
    }
}



//+------------------------------------------------------------------------
//
//  Member:     CServer::AttachWin
//
//  Synopsis:   Creates the in-place window for the object.  The base
//              class does nothing here; any derived object which can
//              be in-place or UI Activated should override this method
//
//  Returns:    HWND
//
//-------------------------------------------------------------------------

HRESULT
CServer::AttachWin(HWND hWndParent, RECT * prc, HWND * phWnd)
{
    HRESULT hr      = S_OK;
    HWND    hwnd;
    TCHAR * pszBuf;


    if (!s_atomWndClass)
    {
        hr = THR(RegisterWindowClass(
                _T("Server"),
                CServer::WndProc,
                CS_DBLCLKS,
                NULL, NULL,
                &CServer::s_atomWndClass));
        if (hr)
        {
            hwnd = NULL;
            goto Cleanup;
        }
    }

    Assert(phWnd);

#ifdef WIN16
    char szBuf[128];
    GlobalGetAtomName(s_atomWndClass, szBuf, ARRAY_SIZE(szBuf));
    pszBuf = szBuf;
#else
    pszBuf = (TCHAR *)(DWORD_PTR)s_atomWndClass; 
#endif

    hwnd = CreateWindowEx(
            0,
            pszBuf,
            NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            prc->left, prc->top,
            prc->right - prc->left, prc->bottom - prc->top,
            hWndParent,
            0,              // no child identifier - shouldn't send WM_COMMAND
            g_hInstCore,
            this);
    if (!hwnd)
    {
        hr = GetLastWin32Error();
        DetachWin();

        goto Cleanup;
    }


    SetWindowPos(_pInPlace->_hwnd, NULL, 0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE |
            SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
 

Cleanup:
    Assert(hr || hwnd);

    *phWnd = hwnd;

    RRETURN(hr);
}


//+--------------------------------------------------------------
//
//  Member:     CServer::DetachWin, CServer
//
//  Synopsis:   Detaches the child's in-place
//              window from the current parent.
//
//  Notes:      This destroys the _hwnd of the server.
//              If the derived class does anything
//              other than create a Window on AttachWin,
//              it must over-ride this function.
//              If the derived class destroys the window
//              on detach, it must set _hwnd = NULL
//
//---------------------------------------------------------------

void
CServer::DetachWin()
{
    Assert(_pInPlace);

    if (_pInPlace->_hwnd)
    {
        Assert(IsWindow(_pInPlace->_hwnd));
        Verify(DestroyWindow(_pInPlace->_hwnd));
        _pInPlace->_hwnd = NULL;
    }
    else if (_pInPlace->_fWindowlessInplace)
    {
        // Do windowless equivalent of DestroyWindow here.

        if (_pInPlace->_fFocus)
        {
            IGNORE_HR(((IOleInPlaceSiteWindowless*)
                    (_pInPlace->_pInPlaceSite))->SetFocus(FALSE));
        }
    }
}

//+---------------------------------------------------------------
//
//  Member:     CServer::InstallFrameUI, CServer
//
//  Synopsis:   Installs the U.I. elements on the frame window.
//              This function assumes the server has does not
//              have any UI.  Derived classes should override
//              to provide their own UI.
//
//---------------------------------------------------------------

#ifndef NO_OLEUI
HRESULT
CServer::InstallFrameUI()
{
    THR_NOTRACE(_pInPlace->_pFrame->SetMenu(NULL, NULL, NULL));
    THR_NOTRACE(_pInPlace->_pFrame->SetBorderSpace(NULL));

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::RemoveFrameUI, CServer
//
//  Synopsis:   Removes the U.I. elements on the frame window
//
//  Notes:      This method "undoes" everything that was done in
//              InstallFrameUI -- it removes the shared menu from
//              the frame.
//
//              Servers that override the InstallFrameUI method will
//              also want to override this method.
//              This method is call by the RemoveUI method and on
//              document window deactivation for MDI-application purposes.
//
//---------------------------------------------------------------

void
CServer::RemoveFrameUI()
{
    if (_pInPlace->_pFrame)
    {
        IGNORE_HR(_pInPlace->_pFrame->SetMenu(NULL, NULL, NULL));
        THR_NOTRACE(_pInPlace->_pFrame->SetBorderSpace(NULL));
    };
}


//+---------------------------------------------------------------
//
//  Member:     CServer::InstallDocUI, protected
//
//  Synopsis:   Installs the U.I. elements on the document window
//
//  Notes:      This method notifies the document window that we are
//              the active object.  Otherwise, there are no standard U.I. elements
//              installed on the document window.
//
//              Servers that have document window tools should override this
//              method.
//
//---------------------------------------------------------------

HRESULT
CServer::InstallDocUI(void)
{
    HRESULT hr = S_OK;
    TCHAR   ach[MAX_USERTYPE_LEN + 1];
    IOleInPlaceActiveObject *pInPlaceActiveObject = NULL;

    Assert(_pInPlace);

    if (_pInPlace->_pDoc != NULL)
    {
        Verify(LoadString(
                GetResourceHInst(),
                IDS_USERTYPESHORT(BaseDesc()->_idrBase),
                ach,
                ARRAY_SIZE(ach)));

        hr = THR(PrivateQueryInterface(IID_IOleInPlaceActiveObject, (void **)&pInPlaceActiveObject));
        if (hr)
            goto Cleanup;

        hr = THR(_pInPlace->_pDoc->SetActiveObject(pInPlaceActiveObject, ach));
        if (hr)
            goto Error;

        IGNORE_HR(_pInPlace->_pDoc->SetBorderSpace(NULL));
    }

Cleanup:
    ReleaseInterface(pInPlaceActiveObject);
    RRETURN(hr);

Error:
    RemoveDocUI();
    goto Cleanup;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::RemoveDocUI, protected
//
//  Synopsis:   Removes the U.I. elements from the document window.
//
//  Notes:      This method "undoes" everything done in the InstallDocUI
//              method.
//
//              Servers that override the InstallDocUI method should
//              also override this method.
//
//---------------------------------------------------------------

void
CServer::RemoveDocUI(void)
{
    Assert(_pInPlace);

    if (_pInPlace->_pDoc != NULL)
    {
        _pInPlace->_pDoc->SetActiveObject(NULL, NULL);
    }
}
#endif // NO_OLEUI


//+---------------------------------------------------------------
//
//  Member:     CServer::GetHWND()
//
//  Synopsis:   Get window used by this CServer.
//
//---------------------------------------------------------------

HWND
CServer::GetHWND()
{
    // WARNING:  This code is very sensitive to the code
    // generator.  The alaising in the last else of the
    // ladder avoids the problem.  If you change this code,
    // please test it by right-clicking a UIActive MPC on
    // the "tab" in a ship build during design mode.  The
    // context menu should appear in the correct place and
    // be functional.  <rodc>

    HWND    hwnd;

    if (State() < OS_INPLACE)
    {
        hwnd = NULL;
    }
    else if (_pInPlace->_hwnd)
    {
        hwnd = _pInPlace->_hwnd;
    }
    else
    {
        HWND *  phwndtmp = &hwnd;

        if (_pInPlace->_pInPlaceSite->GetWindow(phwndtmp))
            hwnd = NULL;
    }

    return hwnd;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetWindow, IOleWindow
//
//  Synopsis:   Method of IOleWindow interface
//
//---------------------------------------------------------------

HRESULT
CServer::GetWindow(HWND FAR* lphwnd)
{
    if (_pInPlace == NULL)
        RRETURN_NOTRACE(E_FAIL);

    *lphwnd = _pInPlace->_hwnd;

#ifdef _MAC
	if (*lphwnd == NULL)
		*lphwnd = GetHWND();

    RRETURN_NOTRACE((*lphwnd ? S_OK : E_FAIL));
#else
    RRETURN_NOTRACE((_pInPlace->_hwnd ? S_OK : E_FAIL));
#endif
}


//+---------------------------------------------------------------
//
//  Member:     CServer::ContextSensitiveHelp, IOleWindow
//
//  Synopsis:   Method of IOleWindow interface
//
//  Notes:      This method sets or clears the _fCSHelpMode
//              member flag.  The window procedure needs to pay
//              attention to the value of this flag in implementing
//              context-sensitive help.
//
//---------------------------------------------------------------

HRESULT
CServer::ContextSensitiveHelp(BOOL fEnterMode)
{
    _pInPlace->_fCSHelpMode = fEnterMode;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::InPlaceDeactivate, IOleInPlaceObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method transitions the object to the loaded state
//              if the object is in the InPlace or U.I. active state.
//
//---------------------------------------------------------------

HRESULT
CServer::InPlaceDeactivate(void)
{
    HRESULT hr = S_OK;

    if (State() == OS_INPLACE || State() == OS_UIACTIVE)
    {
        _pInPlace->_fDeactivating = TRUE;
        hr = THR(TransitionTo(OS_RUNNING));

        //  If the in-place object is still around, clear the
        //    flag.  Note that the in-place object may have been
        //    destroyed by the transition to the OS_RUNNING state,
        //    or may not if the container is caching a pointer to
        //    any of the in-place interfaces. (chrisz)


        if (_pInPlace)
        {
            _pInPlace->_fDeactivating = FALSE;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::UIDeactivate, IOleInPlaceObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      The method transitions the object to the in-place state
//              if the object is in U.I. active state.
//
//---------------------------------------------------------------

HRESULT
CServer::UIDeactivate(void)
{
    HRESULT hr = S_OK; 
    if (State() == OS_UIACTIVE)
    {
        _pInPlace->_fDeactivating = TRUE;
        hr = TransitionTo(OS_INPLACE);
        _pInPlace->_fDeactivating = FALSE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::SetObjectRects, IOleInPlaceObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method does a Move window on the child
//              window to put it in its new position.
//
//---------------------------------------------------------------

HRESULT
#ifndef WIN16
CServer::SetObjectRects(LPCOLERECT prcPos, LPCOLERECT prcClip)
#else
CServer::SetObjectRects(LPCOLERECT prcPosS, LPCOLERECT prcClipS)
#endif
{
    HRESULT     hr = S_OK;
    RECT        rcWnd;
    RECT        rcVisible;

#ifdef  WIN16
    if (!prcPosS)
#else
    if (!prcPos)
#endif
        RRETURN(E_INVALIDARG);

#ifdef WIN16
    RECT        rcPos, *prcPos;
    RECT        rcClip, *prcClip;
    
    CopyRect(&rcPos,  prcPosS);
    prcPos = &rcPos;
    CopyRect(&rcClip, prcClipS);
    prcClip = &rcClip;
#endif

    Assert(State() >= OS_INPLACE);
    Assert(_pInPlace);
    Assert(_pInPlace->_pInPlaceSite);
    Assert(prcPos->left <= prcPos->right);
    Assert(prcPos->top <= prcPos->bottom);

    if (State() < OS_INPLACE)
        RRETURN(E_UNEXPECTED);

    // Handle bogus call from MFC container with NULL clip rectangle...
    if (!prcClip)
    {
        prcClip = prcPos;
    }

    TraceTag((tagRects,
            "%08lx SetObjectRects pos=%ld %ld %ld %ld; clip=%ld %ld %ld %ld",
            this, *prcPos, *prcClip));

    // Check for the container requesting an infinite scale factor.

    if ((_sizel.cx == 0 && prcPos->right - prcPos->left != 0) ||
        (_sizel.cy == 0 && prcPos->bottom - prcPos->top != 0))
    {
        Assert(0 && "Host error: Infinite scale factor. Not a Forms³ error.");
        RRETURN(E_FAIL);
    }

    //
    // Update the inplace RECTs and offsets
    //

    if (_pInPlace->_fWindowlessInplace)
    {
        CopyRect(&_pInPlace->_rcPos, prcPos);
        CopyRect(&_pInPlace->_rcClip,prcClip);
    }
    else
    {
        Assert(_pInPlace->_hwnd);

        // Because we set _pInPlace->_rcPos to match the extent
        // in IOleObject::SetExtent, a change in the size of the
        // positon rectangle here implies that the container is
        // scaling us.

/*
        if ((_pInPlace->_rcPos.right - _pInPlace->_rcPos.left !=
                prcPos->right - prcPos->left ||
            _pInPlace->_rcPos.bottom - _pInPlace->_rcPos.top !=
                prcPos->bottom - prcPos->top))
        {
            // Handle change in scaling.
            //::InvalidateRect ( _pInPlace->_hwnd, NULL, TRUE );
            RedrawWindow(_pInPlace->_hwnd,
                (GDIRECT *)NULL, NULL, RDW_ERASE | RDW_ALLCHILDREN | RDW_INVALIDATE);
        }
*/

        _pInPlace->_ptWnd = *(POINT *)&prcPos->left;

        CopyRect(&_pInPlace->_rcPos, prcPos);
        OffsetRect(&_pInPlace->_rcPos,  -_pInPlace->_ptWnd.x, -_pInPlace->_ptWnd.y);

        CopyRect(&_pInPlace->_rcClip, prcClip);
        OffsetRect(&_pInPlace->_rcClip, -_pInPlace->_ptWnd.x, -_pInPlace->_ptWnd.y);

        CopyRect(&rcWnd, prcPos);
        if (_pInPlace->_fShowBorder)
        {
            InflateRect(&rcWnd, CX_IPBORDER, CY_IPBORDER);
        }

        IntersectRect(&rcVisible, &rcWnd, prcClip);
        if (EqualRect(&rcVisible, &rcWnd))
        {
            if (_pInPlace->_fUsingWindowRgn)
            {
                SetWindowRgn(_pInPlace->_hwnd, NULL, TRUE);
                _pInPlace->_fUsingWindowRgn = FALSE;
            }
        }
        else 
        {
            _pInPlace->_fUsingWindowRgn = TRUE;
            OffsetRect(&rcVisible, -rcWnd.left, -rcWnd.top);
            SetWindowRgn(_pInPlace->_hwnd,
                    CreateRectRgnIndirect(&rcVisible),
                    TRUE);
        }

        // we go to some trouble here to make sure we aren't calling SetWindowPos
        // with invalid area in our window, because Windows sends a WM_ERASEBKGND
        // message to our window and all child windows if it finds a completely
        // invalid window at the time that SetWindowPos is called.  This causes
        // truly disastrous flashing to occur when we are resizing the window.
        HRGN hrgnUpdate = ::CreateRectRgnIndirect(&g_Zero.rc);
        if (hrgnUpdate)
        {
            int result = ::GetUpdateRgn(_pInPlace->_hwnd, hrgnUpdate, FALSE);
            if (result != ERROR && result != NULLREGION)
            {
                ::ValidateRgn(_pInPlace->_hwnd, hrgnUpdate);
            }
            else
            {
                ::DeleteObject(hrgnUpdate);
                hrgnUpdate = NULL;
            }
        }

        TraceTag((tagRects, "%08lx SetObjectRects > SetWindowPos %ld %ld %ld %ld", this, rcWnd));
        
        SetWindowPos(_pInPlace->_hwnd, NULL, rcWnd.left, rcWnd.top,
                rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top,
                SWP_NOZORDER | SWP_NOACTIVATE);

        // restore previously invalid area
        if (hrgnUpdate)
        {
            if (_pInPlace && _pInPlace->_hwnd)
            {
                ::InvalidateRgn(_pInPlace->_hwnd, hrgnUpdate, FALSE);
            }
            ::DeleteObject(hrgnUpdate);
        }
    }

    RRETURN (hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::ReactivateAndUndo, IOleInPlaceObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method activates us, but doesn't do anything else.
//              Anyone who supports undo should override this method.
//
//---------------------------------------------------------------

HRESULT
CServer::ReactivateAndUndo(void)
{
    HRESULT     hr;

    hr = THR(TransitionTo(OS_UIACTIVE));

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CServer::TranslateAccelerator, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceActiveObject interface
//
//---------------------------------------------------------------

HRESULT
CServer::TranslateAccelerator(LPMSG lpmsg)
{
    HRESULT hr = S_FALSE;
    CLock   Lock(this);
    VARIANT_BOOL    fEnabled;

    // if we are a form this will get called
    // because we are our own inplace object
    // but we want to bail out if we are disabled
    THR(GetEnabled(&fEnabled));
    if (!fEnabled)
    {
        // Call CServer::DoTranslateAccelerator to forward the key
        // up to the site.
        hr = THR(CServer::DoTranslateAccelerator(lpmsg));
    }
    else if (lpmsg->message >= WM_KEYFIRST && lpmsg->message <= WM_KEYLAST)
    {
        hr = THR_NOTRACE(DoTranslateAccelerator(lpmsg));
    }

    //
    // If we are merging menus, let the frame have a crack at 
    // translating accelerators.  The call to OleTranslateAccelerator
    // is necessary even though we're an in-proc server because 
    // the frame has no way of knowing where to route menu messages
    // otherwise.  Without this, things like Alt+E, C will not
    // work correctly.  (anandra)
    //
    
    if (S_FALSE == hr && 
        _pInPlace &&
        _pInPlace->_pFrame &&
        _pInPlace->_fMenusMerged)
    {
        hr = THR(OleTranslateAccelerator(
                _pInPlace->_pFrame, 
                &_pInPlace->_frameInfo, 
                lpmsg));
    }

    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DoTranslateAccelerator, CServer
//
//  Synopsis:   Helper method for TranslateAccelerator.
//              Derived control classes should override this method
//              rather than TranslateAccelerator
//
//---------------------------------------------------------------

HRESULT
CServer::DoTranslateAccelerator(LPMSG lpmsg)
{
    HRESULT hr = S_FALSE;
    IOleControlSite * pCtrlSite;

    if (_pClientSite &&
        !THR_NOTRACE(_pClientSite->QueryInterface(
                IID_IOleControlSite,
                (void**) &pCtrlSite)))
    {
            hr = THR(pCtrlSite->TranslateAccelerator(lpmsg, VBShiftState()));
            pCtrlSite->Release();
    }

    RRETURN1_NOTRACE(hr, S_FALSE);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::OnFrameWindowActivate, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//---------------------------------------------------------------

HRESULT
CServer::OnFrameWindowActivate(BOOL fActivate)
{
    if (_pInPlace)
    {
        if (fActivate && _state == OS_UIACTIVE && !_pInPlace->_fChildActive)
        {
            // Set focus unless a child window has focus already
            HWND hwndFocus = ::GetFocus();
            
            if (!(hwndFocus && _pInPlace->_hwnd && ::IsChild(_pInPlace->_hwnd, hwndFocus)))
            {
                SetFocus(TRUE);
            }
        }

        _pInPlace->_fFrameActive = fActivate;
    }
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CServer::OnDocWindowActivate, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method will install or remove the frame
//              U.I. elements using the InstallFrameUI or RemoveFrameUI
//              methods.  This is to properly handle the MDI application
//              case.  It also updates our shading color.
//
//---------------------------------------------------------------

HRESULT
CServer::OnDocWindowActivate(BOOL fActivate)
{
    HRESULT hr = S_OK;

#ifndef NO_OLEUI
    if (!_pInPlace)
        goto Cleanup;

    if (fActivate)
    {
        hr = THR(InstallFrameUI());
    }
    else
    {
        RemoveFrameUI();
    }
#endif // NO_OLEUI

    if (!hr)
    {
        Assert(_pInPlace);

        _pInPlace->_fDocActive = fActivate;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::ResizeBorder, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      There are no standard border adornments so we do
//              nothing in this method.  Servers that have additional
//              U.I. elements that are installed on the frame or
//              document windows should override this method.
//
//---------------------------------------------------------------

HRESULT
CServer::ResizeBorder(
        LPCOLERECT lprc,
        LPOLEINPLACEUIWINDOW pUIWindow,
        BOOL fFrameWindow)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::EnableModeless, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//---------------------------------------------------------------

HRESULT
CServer::EnableModeless(BOOL fEnable)
{
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CServer::ShowUIActiveBorder
//
//  Synopsis:   Shows or hides the UI Active border for this object
//
//  Arguments:  [fShowBorder]   --  TRUE to show it
//
//-------------------------------------------------------------------------

void
CServer::ShowUIActiveBorder(BOOL fShowBorder)
{
    RECT    rc;

    Assert(State() >= OS_INPLACE);

    if (!_pInPlace->_hwnd)
        return;

    if (_pInPlace->_fShowBorder == (unsigned) fShowBorder)
        return;

    _pInPlace->_fShowBorder = fShowBorder;

    rc = _pInPlace->_rcPos;

    if (fShowBorder)
    {
        InflateRect(&rc, CX_IPBORDER, CY_IPBORDER);
    }

    TraceTag((tagRects, "%08lx ShowUIActiveBorder > SetWindowPos %ld %ld %ld %ld",
            this,
            rc.left + _pInPlace->_ptWnd.x,
            rc.top + _pInPlace->_ptWnd.y,
            rc.right - rc.left + _pInPlace->_ptWnd.x,
            rc.bottom - rc.top + _pInPlace->_ptWnd.y));

    SetWindowPos(
            _pInPlace->_hwnd,
            NULL,
            rc.left + _pInPlace->_ptWnd.x,
            rc.top + _pInPlace->_ptWnd.y,
            rc.right - rc.left,
            rc.bottom - rc.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::OnPaint
//
//  Synopsis:   Draws the client area when a window is present
//
//-------------------------------------------------------------------------

void
CServer::OnPaint()
{
    HDC hdc;
    PAINTSTRUCT ps;

    Assert(_pInPlace);
    Assert(_pInPlace->_hwnd);

    if (TestLock(SERVERLOCK_BLOCKPAINT))
        return;

    CLock Lock(this, SERVERLOCK_BLOCKPAINT);

    hdc = ::BeginPaint(_pInPlace->_hwnd, &ps);

    if (!hdc)
        return;

    GetPalette(hdc);

    // NOTE: Here we pass a NULL in for the rect.  This is done becuase
    // this control has a window and must be in place, and the convention
    // is that a NULL is passed in to have the control draw in place, as
    // opposed to drawing it in another view.

    IGNORE_HR(Draw(DVASPECT_CONTENT, -1, 0, 0, 0, hdc, NULL, 0, 0, 0));

    SelectPalette(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
    EndPaint(_pInPlace->_hwnd, &ps);
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::OnEraseBkgnd
//
//  Synopsis:   Erases the background when a window is present
//
//-------------------------------------------------------------------------

BOOL
CServer::OnEraseBkgnd(HDC hdc)
{
    Assert( _pInPlace );
    Assert( _pInPlace->_hwnd );

    return TestLock(SERVERLOCK_BLOCKPAINT) ? TRUE : FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::OnNCSetCursor
//
//  Synopsis:   Set the cursor for the non-licent area.
//
//-------------------------------------------------------------------------

BOOL
CServer::OnNCSetCursor(HWND hwnd, int nHitTest, UINT msg)
{
    TCHAR * idc;

    // Set cursor for non-client area.  We don't let the default
    // window procedure handle this because it gives the container
    // a chance to override what we really want.

    if (hwnd != _pInPlace->_hwnd || msg != WM_MOUSEMOVE)
        return FALSE;

    switch (nHitTest)
    {
    case HTCAPTION:
        idc = IDC_SIZEALL;
        break;

    case HTLEFT:
    case HTRIGHT:
        idc = IDC_SIZEWE;
        break;

    case HTTOP:
    case HTBOTTOM:
        idc = IDC_SIZENS;
        break;

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
        idc = IDC_SIZENWSE;
        break;

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
        idc = IDC_SIZENESW;
        break;

    default:
        return FALSE;
    }

    SetCursorIDC(idc);

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::OnNCPaint
//
//  Synopsis:   Draws the non-client region for the given window
//
//  Arguments:  hWnd    Window to draw
//
//-------------------------------------------------------------------------

void
CServer::OnNCPaint()
{
    RECT        rcBorder;
    HDC         hdc;

    if (!_pInPlace->_fShowBorder)
        return;

    // Get rcPos and inflate it.  This rcPos is our client
    // coordinate system
    rcBorder = _pInPlace->_rcPos;
    InflateRect(&rcBorder, CX_IPBORDER, CY_IPBORDER);

    // Normalize rcBorder to our window coord. system.
    OffsetRect(&rcBorder, -rcBorder.left, -rcBorder.top);

    hdc = GetWindowDC(_pInPlace->_hwnd);

    DrawUIActiveBorder(
            hdc,
            &rcBorder,
            GetAmbientBool(DISPID_AMBIENT_SHOWHATCHING, TRUE),
            GetAmbientBool(DISPID_AMBIENT_SHOWGRABHANDLES, TRUE));

    ::ReleaseDC(_pInPlace->_hwnd, hdc);
}


//+------------------------------------------------------------------------
//
//  Member:     CServer::OnNCHitTest
//
//  Synopsis:   Returns the hit code for the border part at the given
//              point.
//
//-------------------------------------------------------------------------

LONG
CServer::OnNCHitTest(POINTS pts)
{
    RECT    rc;
    POINT   pt;
    int     dwHitBits;
    int     d;

    pt.x = pts.x;
    pt.y = pts.y;

    //
    // If it clearly is in the client area, return that fact.
    //
    GetClientRect( _pInPlace->_hwnd , &rc );
    ScreenToClient(_pInPlace->_hwnd, &pt);
    if (PtInRect(&rc, pt))
        return HTCLIENT;

    //
    // If we have no hatch border to test against then we can
    // give no better information.
    //
    if (!_pInPlace->_fShowBorder)
        return HTNOWHERE;

    // Set bits in a DWORD based on horizontal and vertical position
    // of the mouse.

    dwHitBits = 0;

    d = (rc.left + rc.right - CX_IPBORDER) / 2;
    if (pt.x >= d && pt.x < d + CX_IPBORDER + CX_HANDLEINSET)
        dwHitBits |= 0x04;
    else if (pt.x < rc.left + CX_IPBORDER + CX_HANDLEINSET)
        dwHitBits |= 0x01;
    else if (pt.x >= rc.right - CX_IPBORDER - CX_HANDLEINSET)
        dwHitBits |= 0x02;

    d = (rc.top + rc.bottom - CY_IPBORDER) / 2;
    if (pt.y >= d && pt.y < d + CY_IPBORDER + CY_HANDLEINSET)
        dwHitBits |= 0x40;
    else if (pt.y < rc.top + CY_IPBORDER + CY_HANDLEINSET)
        dwHitBits |= 0x10;
    else if (pt.y >= rc.bottom - CY_IPBORDER - CY_HANDLEINSET)
        dwHitBits |= 0x20;

    // Convert bits to hit test codes.

    switch (dwHitBits)
    {
    case 0x11:
        return HTTOPLEFT;
    case 0x21:
        return HTBOTTOMLEFT;
    case 0x41:
        return HTLEFT;
    case 0x12:
        return HTTOPRIGHT;
    case 0x22:
        return HTBOTTOMRIGHT;
    case 0x42:
        return HTRIGHT;
    case 0x14:
        return HTTOP;
    case 0x24:
        return HTBOTTOM;
    }

    return HTCAPTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::OnNCLButtonDown
//
//  Synopsis:   Handle WM_NCLBUTTONDOWN
//
//-------------------------------------------------------------------------

BOOL
CServer::OnNCLButtonDown(int ht, POINTS pts, RECT * prcCurrent)
{
    HRESULT hr;
    HDC     hdc;
    HWND    hwnd;
    RECT    rcStart;
    RECT    rcCurrent;
    RECT    rcClip;
    RECT    rc;
    MSG     msg;
    POINT   pt;
    POINT   ptStart;
    BOOL    fCallOnPosRectChange = FALSE;

    hr = THR(_pInPlace->_pInPlaceSite->GetWindow(&hwnd));
    if (hr)
        return FALSE;

    if (GetCapture() != NULL)
        return FALSE;

    if (!prcCurrent)
    {
        prcCurrent = &rcCurrent;
    }

    // Get container's window current and lock it so
    // the x-or drawing below will not be messed up.

    UpdateChildTree(hwnd);
    LockWindowUpdate(hwnd);

    // Get starting point in container window coordinates.

    ptStart.x = pts.x;
    ptStart.y = pts.y;
    ScreenToClient(hwnd, &ptStart);

    // Get clip rectangle in container window coordinates.

    rcClip = _pInPlace->_rcClip;
    OffsetRect(&rcClip, _pInPlace->_ptWnd.x, _pInPlace->_ptWnd.y);
    ::GetClientRect(hwnd, &rc);
    IntersectRect(&rcClip, &rcClip, &rc);

    // Get starting rectangle in container window coordinates.

    rcStart = _pInPlace->_rcPos;
    OffsetRect(&rcStart, _pInPlace->_ptWnd.x, _pInPlace->_ptWnd.y);

    // Setup DC for drawing.

    hdc = GetDCEx(hwnd, NULL,
            DCX_CACHE |
            DCX_CLIPSIBLINGS |
            DCX_LOCKWINDOWUPDATE);
    IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

    *prcCurrent = rcStart;

    ::SetCapture(hwnd);

    DrawDefaultFeedbackRect(hdc, prcCurrent);

    // Get messages until capture lost or cancelled/accepted

    for (;;)
    {
        #ifdef PRODUCT_PROF
        ::SuspendCAP();
        #endif

        GetMessage(&msg, NULL, 0, 0);

        #ifdef PRODUCT_PROF
        ::ResumeCAP();
        #endif

        if (::GetCapture() != hwnd)
            goto ExitLoop;

        switch (msg.message)
        {
        case WM_SETCURSOR:
            break;

        case WM_KEYDOWN:
            if (msg.wParam == VK_ESCAPE)
                goto ExitLoop;
            break;

        case WM_RBUTTONDOWN:
            goto ExitLoop;

        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:

            rc = *prcCurrent;
            *prcCurrent = rcStart;

            pts = MAKEPOINTS(msg.lParam);
            pt.x = pts.x;
            pt.y = pts.y;

            if (pt.x < rcClip.left)
                pt.x = rcClip.left;
            else if (pt.x > rcClip.right)
                pt.x = rcClip.right;

            if (pt.y < rcClip.top)
                pt.y = rcClip.top;
            else if (pt.y > rcClip.bottom)
                pt.y = rcClip.bottom;

            switch (ht)
            {
            case HTCAPTION:
                OffsetRect(prcCurrent, pt.x - ptStart.x, pt.y - ptStart.y);
                break;

            case HTLEFT:
            case HTTOPLEFT:
            case HTBOTTOMLEFT:
                prcCurrent->left += pt.x - ptStart.x;
                if (prcCurrent->left > rcStart.right)
                    prcCurrent->left = rcStart.right;
                break;

            case HTRIGHT:
            case HTTOPRIGHT:
            case HTBOTTOMRIGHT:
                prcCurrent->right += pt.x - ptStart.x;
                if (prcCurrent->right < rcStart.left)
                    prcCurrent->right = rcStart.left;
                break;
            }

            switch (ht)
            {
            case HTTOP:
            case HTTOPLEFT:
            case HTTOPRIGHT:
                prcCurrent->top += pt.y - ptStart.y;
                if (prcCurrent->top > rcStart.bottom)
                    prcCurrent->top = rcStart.bottom;
                break;

            case HTBOTTOM:
            case HTBOTTOMLEFT:
            case HTBOTTOMRIGHT:
                prcCurrent->bottom += pt.y - ptStart.y;
                if (prcCurrent->bottom < rcStart.top)
                    prcCurrent->bottom = rcStart.top;
                break;
            }

            if (memcmp(&rcCurrent, &rc, sizeof(rc)))
            {
                DrawDefaultFeedbackRect(hdc, &rc);
                DrawDefaultFeedbackRect(hdc, prcCurrent);
            }

            if (msg.message == WM_LBUTTONUP)
            {
                fCallOnPosRectChange = TRUE;
                goto ExitLoop;
            }

        default:

            // Anything can happen during DispatchMessage.
            // Insure that our feedback is not messed up.

            DrawDefaultFeedbackRect(hdc, prcCurrent);
            DispatchMessage(&msg);
            UpdateChildTree(hwnd);
            DrawDefaultFeedbackRect(hdc, prcCurrent);
            break;
        }
    }

ExitLoop:

    DrawDefaultFeedbackRect(hdc, prcCurrent);

    ::ReleaseDC(hwnd, hdc);
#if DBG==1
    Assert(!TLS(fHandleCaptureChanged));
#endif
    ReleaseCapture();
    LockWindowUpdate(NULL);

    return fCallOnPosRectChange;
}


//+---------------------------------------------------------------------------
//
//  Member:    CServer::OnDestroy
//
//  Synopsis:  Deregister Drag and Drop
//
//----------------------------------------------------------------------------

void
CServer::OnDestroy( )
{
    Assert(_pInPlace->_hwnd);
    RevokeDragDrop(_pInPlace->_hwnd);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::OnWindowMessage
//
//  Synopsis:   Handles windows messages.
//
//  Arguments:  msg     the message identifier
//              wParam  the first message parameter
//              lParam  the second message parameter
//
//  Returns:    LRESULT as in WNDPROCs
//
//----------------------------------------------------------------------------

HRESULT
CServer::OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr=S_OK;
    POINT pt;

    *plResult = 0;

    // We should always be stabilized when handling a window message.
    // This check insures that derived classes are doing the right thing.

    Assert(TestLock(SERVERLOCK_STABILIZED));

    // Events fired by the derived class implementation of OnWindowMessage
    // may have caused this object to leave the inplace state.  Bail out
    // here if it looks like this happened.  We check for _pInPlace instead
    // of _state < OS_INPLACE because the window procedure can be called
    // before we are officially in the inplace state.

    if (!_pInPlace)
    {
        return S_OK;
    }

    // Process the message.

    switch (msg)
    {
    case WM_DESTROY:
        OnDestroy();
        break;

// WINCEREVIEW -- senthilv - 01/30/97 ignoring all message handlers from nonclient area
#ifndef WINCE
    case WM_NCLBUTTONDOWN:
        OnNCLButtonDown(wParam, MAKEPOINTS(lParam));
        break;

    case WM_NCHITTEST:
        *plResult = OnNCHitTest(MAKEPOINTS(lParam));
        break;

    case WM_NCCALCSIZE:
        if (_pInPlace->_fShowBorder)
        {
            // Let the old client rect origin stay the same so shrink
            // incoming rectangle (which is total window size)
            InflateRect((RECT *)lParam, -CX_IPBORDER, -CY_IPBORDER);
        }

        if (wParam && TestServerDescFlag(SERVERDESC_INVAL_ON_RESIZE))
        {
            *plResult |= WVR_REDRAW;
        }
        break;

    case WM_NCPAINT:
        OnNCPaint();
        break;
#endif	// WINCE

    case WM_SETCURSOR:

        if (LOWORD(lParam) != HTCLIENT)
        {
            if (_pInPlace->_hwnd &&
                !OnNCSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam)))
            {
                *plResult = DefWindowProc(_pInPlace->_hwnd, msg, wParam, lParam);
            }
        }
        else
        {
            HWND hwnd;

            GetCursorPos(&pt);

            hwnd = GetHWND();
            if (hwnd)
            {
                ScreenToClient(hwnd, &pt);

                // First chance at setting the cursor

                if (THR(OnInactiveSetCursor(
                        &_pInPlace->_rcPos,
                        pt.x, pt.y,
                        HIWORD(wParam),
                        FALSE)) == S_FALSE)
                {
                    // Give the container a chance to set the cursor.

                    IGNORE_HR(OnDefWindowMessage(msg,
                                                 wParam ? wParam : (WPARAM)hwnd,
                                                 lParam,
                                                 plResult ));

                    // If the container did not set the cursor, then use the built in
                    // cursor.  Note that we do this for our window only.

                    if (!*plResult && hwnd == (HWND)wParam && !_pInPlace->_fBubbleInsideOut)
                    {
                        *plResult = (THR(OnInactiveSetCursor(
                            &_pInPlace->_rcPos,
                            pt.x, pt.y,
                            HIWORD(wParam),
                            TRUE)) == S_FALSE) ? FALSE : TRUE;
                    }
                }
                else
                {
                    *plResult = TRUE;
                }
            }
        }
        break;

    case WM_MOUSEMOVE:
        hr = THR(OnInactiveMouseMove(
                &_pInPlace->_rcPos,
                MAKEPOINTS(lParam).x,
                MAKEPOINTS(lParam).y,
                wParam));
        break;

    case WM_ERASEBKGND:
        *plResult = OnEraseBkgnd((HDC)wParam);
        break;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        {
            IOleControlSite * pControlSite;

            _pInPlace->_fFocus = (msg == WM_SETFOCUS);

            if (OK(THR_NOTRACE(_pClientSite->QueryInterface(IID_IOleControlSite,
                    (void **)&pControlSite))))
            {
                pControlSite->OnFocus(msg == WM_SETFOCUS);
                pControlSite->Release();
            }
        }
        break;

    // OCX containers (e.g. VB4) will forward WM_PALETTECHANGED and WM_QUERYNEWPALETTE
    // on to us to properly realize our palette for controls in cases where they are
    // windowed.  This is semantically equivalent to the code in MinFrameWndProc in
    // minfr.cxx.
    case WM_PALETTECHANGED:
        Assert(_pInPlace);
        if ((HWND)wParam == _pInPlace->_hwnd)
            break;
        // **** FALL THRU ****
    case WM_QUERYNEWPALETTE:
        {
            HDC         hdc;

            Assert(_pInPlace);
            hdc = ::GetDC(_pInPlace->_hwnd);
            if (hdc)
            {
                BOOL        fInvalidate = FALSE;
                HPALETTE    hpal;

                hpal = GetPalette();
                if (hpal)
                {
                    HPALETTE hpalOld = SelectPalette(hdc, hpal, (msg == WM_PALETTECHANGED));
                    fInvalidate = RealizePalette(hdc) || (msg == WM_PALETTECHANGED);
                    SelectPalette(hdc, hpalOld, TRUE);
                    
                    if (fInvalidate && (_state >= OS_INPLACE))
                    {
                        if (_pInPlace->_hwnd)
                            RedrawWindow(_pInPlace->_hwnd, (GDIRECT *)NULL, NULL,
                                         RDW_INVALIDATE | RDW_ALLCHILDREN);
                        else
                            InvalidateRect(NULL, TRUE);
                    }
                }
                ::ReleaseDC(_pInPlace->_hwnd, hdc);
                *plResult = !!hpal;
            }
            break;
        }

    default:
        hr = THR(OnDefWindowMessage(msg, wParam, lParam, plResult));
        break;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::OnDefWindowMessage
//
//  Synopsis:   Default handling for window messages.
//
//  Arguments:  msg      the message identifier
//              wParam   the first message parameter
//              lParam   the second message parameter
//              plResult return value from window proc
//
//----------------------------------------------------------------------------

HRESULT
CServer::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = S_OK;

    // Events fired by the derived class implementation of OnWindowMessage
    // may have caused this object to leave the inplace state.  Bail out
    // here if it looks like this happened.  We check for _pInPlace instead
    // of _state < OS_INPLACE because the window procedure can be called
    // before we are officially in the inplace state.

    if (!_pInPlace)
    {
        return S_OK;
    }

    if (_pInPlace->_fWindowlessInplace)
    {
        hr = THR(((IOleInPlaceSiteWindowless*)_pInPlace->_pInPlaceSite)->
                OnDefWindowMessage(msg, wParam, lParam, plResult));
    }
#ifndef NO_IME
    else if (DIMMHandleDefWindowProc(_pInPlace->_hwnd, msg, wParam, lParam, plResult))
    {
        // WM_IME* messages routed through here, so let the imm hook get a chance at them
        // plResult set in call above.
        
        // Note we only bother to hook if !_pInPlace->_fWindowlessInPlace, that is if
        // we have our own window.
    }
#endif // ndef NO_IME
    else if (_pInPlace->_hwnd)
    {
        // Events fired by the derived class implementation of OnWindowMessage
        // may have caused this object to leave the inplace state.  Bail out
        // here if it looks like this happened.  We check for _pInPlace instead
        // of _state < OS_INPLACE because the window procedure can be called
        // before we are officially in the inplace state.

#ifdef WIN16
        if ((msg == WM_RBUTTONUP) || (msg == WM_NCRBUTTONUP))
        {
            POINT pt;
            pt.x = MAKEPOINTS(lParam).x;
            pt.y = MAKEPOINTS(lParam).y;
            ClientToScreen(_pInPlace->_hwnd,&pt);
            SendMessage(_pInPlace->_hwnd,
                        WM_CONTEXTMENU,
                        (WPARAM)_pInPlace->_hwnd,
                        MAKELPARAM(pt.x,pt.y));
        }
        else
            *plResult = DefWindowProc(_pInPlace->_hwnd, msg, wParam, lParam);
#else            
        *plResult = DefWindowProc(_pInPlace->_hwnd, msg, wParam, lParam);
#endif        
    }
    else
    {
        *plResult = 0;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::WndProc
//
//  Synopsis:   Window procedure for use by derived class.
//              This function maintains the relationship between
//              CServer and HWND and delegates all other functionality
//              to the CServer::OnWindowMessage virtual method.
//
//  Arguments:  hwnd     the window
//              msg      the message identifier
//              wParam   the first message parameter
//              lParam   the second message parameter
//              plResult the window procedure return value
//
//  Returns:    S_FALSE if caller should delegate to the default window proc
//
//----------------------------------------------------------------------------

LRESULT CALLBACK
CServer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CServer *pServer;
    LRESULT lResult = 0;

    //
    // a-msadek; Trident window should not be mirrored if hosted by a mirrored process
    //

    if(msg == WM_NCCREATE)
    {
        DWORD dwExStyles;
        if ((dwExStyles = GetWindowLong(hwnd, GWL_EXSTYLE)) & WS_EX_LAYOUTRTL)
        {
             SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyles &~ WS_EX_LAYOUTRTL);
        }
    }

    //
    // If creating, then establish the connection between the HWND and
    // the CServer.  Otherwise, fetch the pointer to the CServer.
    //

#if defined(WINCE) && !defined(WINCE_NT)
    if (msg == WM_CREATE)
#else
    if (msg == WM_NCCREATE)
#endif // WINCE
    {
        pServer = (CServer *) ((LPCREATESTRUCTW)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pServer);
        pServer->_pInPlace->_hwnd = hwnd; 
        pServer->PrivateAddRef();
    }
    else
    {
        pServer = (CServer *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pServer)
    {
#ifdef WIN16
        // in case we were are on some other thread's stack (e.g. Java)... 
        // impersonate our own
        THREAD_HANDLE dwOldImpersonatedTId = w16ImpersonateThread(pServer->_dwThreadId);
#endif

        Assert(pServer->_pInPlace);
        Assert(pServer->_pInPlace->_hwnd == hwnd);

        //
        // Give the derived class a chance to handle the window message.
        //

        IGNORE_HR(pServer->OnWindowMessage(msg, wParam, lParam, &lResult));

        //
        // If destroying, break the connection between the HWND and CServer.
        // The call to release might destroy the server, so it must come
        // after the OnWindowMessage virtual function call.
        //

#if defined(WINCE) && !defined(WINCE_NT)
        if (msg == WM_DESTROY)
#else
        if (msg == WM_NCDESTROY)
#endif // WINCE
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            pServer->_pInPlace->_hwnd = NULL;
            pServer->PrivateRelease();
        }

#ifdef WIN16
        w16ImpersonateThread(dwOldImpersonatedTId);
#endif

    }

    return lResult;
}

