//=--------------------------------------------------------------------------=
// CtlWrap.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// wrappers for various routines that have slightly different implementations
// for windowed and windowless controls.
//
#include "IPServer.H"

#include "CtrlObj.H"


// for ASSERT and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// COleControl::OcxGetFocus    [wrapper]
//=--------------------------------------------------------------------------=
// indicates whether or not we have the focus.
//
// Parameters:
//    none
//
// Output:
//    TRUE if we have focus, else false
//
// Notes:
//
BOOL COleControl::OcxGetFocus
(
    void
)
{
    // if we're windowless, the site provides this functionality
    //
    if (m_pInPlaceSiteWndless) {
        return (m_pInPlaceSiteWndless->GetFocus() == S_OK);
    } else {

        // we've got a window.  just let the APIs do our work
        //
        if (m_fInPlaceActive)
            return (GetFocus() == m_hwnd);
        else
            return FALSE;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::OcxGetWindowRect    [wrapper]
//=--------------------------------------------------------------------------=
// returns the current rectangle for this control, and correctly handles
// windowless vs windowed.
//
// Parameters:
//    LPRECT                - [out]  duh.
//
// Output:
//    BOOL                  - false means unexpected.
//
// Notes:
//
BOOL COleControl::OcxGetWindowRect
(
    LPRECT prc
)
{
    // if we're windowless, then we have this information already!
    //
    if (Windowless()) {
        *prc = m_rcLocation;
        return TRUE;
    } else
        return GetWindowRect(m_hwnd, prc);

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::OcxDefWindowProc    [wrapper]
//=--------------------------------------------------------------------------=
// default window processing
//
// Parameters:
//    UINT           - [in] duh.
//    WPARAM         - [in] duh.
//    LPARAM         - [in] DUH.
//
// Output:
//    LRESULT
//
// Notes:
//
LRESULT COleControl::OcxDefWindowProc
(
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LRESULT l;

    // if we're windowless, this is a site provided pointer
    //
    if (m_pInPlaceSiteWndless)
        m_pInPlaceSiteWndless->OnDefWindowMessage(msg, wParam, lParam, &l);
    else
        // we've got a window -- just pass it along
        //
        l = DefWindowProc(m_hwnd, msg, wParam, lParam);

    return l;
}

//=--------------------------------------------------------------------------=
// COleControl::OcxGetDC    [wrapper]
//=--------------------------------------------------------------------------=
// wraps the functionality of GetDC, and correctly handles windowless controls
//
// Parameters:
//    none
//
// Output:
//    HDC            - null means we couldn't get one
//
// Notes:
//    - we don't bother with a bunch of the IOleInPlaceSiteWindowless::GetDc
//      parameters, since the windows GetDC doesn't expose these either. users
//      wanting that sort of fine tuned control can call said routine
//      explicitly
//
HDC COleControl::OcxGetDC
(
    void
)
{
    HDC hdc = NULL;

    // if we're windowless, the site provides this functionality.
    //
    if (m_pInPlaceSiteWndless)
        m_pInPlaceSiteWndless->GetDC(NULL, 0, &hdc);
    else
        hdc = GetDC(m_hwnd);

    return hdc;
}

//=--------------------------------------------------------------------------=
// COleControl::OcxReleaseDC    [wrapper]
//=--------------------------------------------------------------------------=
// releases a DC returned by OcxGetDC
//
// Parameters:
//    HDC             - [in] release me
//
// Output:
//    none
//
// Notes:
//
void COleControl::OcxReleaseDC
(
    HDC hdc
)
{
    // if we're windowless, the site does this for us
    //
    if (m_pInPlaceSiteWndless)
        m_pInPlaceSiteWndless->ReleaseDC(hdc);
    else
        ReleaseDC(m_hwnd, hdc);
}

//=--------------------------------------------------------------------------=
// COleControl::OcxSetCapture    [wrapper]
//=--------------------------------------------------------------------------=
// provides a means for the control to get or release capture.
//
// Parameters:
//    BOOL            - [in] true means take, false release
//
// Output:
//    BOOL            - true means it's yours, false nuh-uh
//
// Notes:
//
BOOL COleControl::OcxSetCapture
(
    BOOL fGrab
)
{
    HRESULT hr;

    // the host does this for us if we're windowless [i'm getting really bored
    // of typing that]
    //
    if (m_pInPlaceSiteWndless) {
        hr = m_pInPlaceSiteWndless->SetCapture(fGrab);
        return (hr == S_OK);
    } else {
        // people shouldn't call this when they're not in-place active, but
        // just in case...
        //
        if (m_fInPlaceActive) {
            SetCapture(m_hwnd);
            return TRUE;
        } else
            return FALSE;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::OcxGetCapture    [wrapper]
//=--------------------------------------------------------------------------=
// tells you whether or not you have the capture.
//
// Parameters:
//    none
//
// Output:
//    BOOL         - true it's yours, false it's not
//
// Notes:
//
BOOL COleControl::OcxGetCapture
(
    void
)
{
    // host does this for windowless dudes
    //
    if (m_pInPlaceSiteWndless)
        return m_pInPlaceSiteWndless->GetCapture() == S_OK;
    else {
        // people shouldn't call this when they're not in-place active, but
        // just in case.
        //
        if (m_fInPlaceActive)
            return GetCapture() == m_hwnd;
        else
            return FALSE;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::OcxInvalidateRect    [wrapper]
//=--------------------------------------------------------------------------=
// invalidates the control's rectangle
//
// Parameters:
//    LPCRECT            - [in] rectangle to invalidate
//    BOOL               - [in] do we erase background first?
//
// Output:
//    BOOL
//
// Notes:
//
BOOL COleControl::OcxInvalidateRect
(
    LPCRECT prcInvalidate,
    BOOL    fErase
)
{
    // if we're windowless, then we need to get the site to do all this for
    // us
    if (m_pInPlaceSiteWndless)
        return m_pInPlaceSiteWndless->InvalidateRect(prcInvalidate, fErase) == S_OK;
    else {
        // otherwise do something different depending on whether or not we're
        // in place active or not
        //
        if (m_fInPlaceActive)
            return InvalidateRect(m_hwnd, prcInvalidate, TRUE);
        else
            ViewChanged();
    }

    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::OcxScrollRect    [wrapper]
//=--------------------------------------------------------------------------=
// does some window scrolling for the control
//
// Parameters:
//    LPCRECT             - [in] region to scroll
//    LPCRECT             - [in] region to clip
//    int                 - [in] dx to scroll
//    int                 - [in] dy to scroll
//
// Output:
//    BOOL
//
// Notes:
//
BOOL COleControl::OcxScrollRect
(
    LPCRECT  prcBounds,
    LPCRECT  prcClip,
    int      dx,
    int      dy
)
{
    // if we're windowless, the site provides this functionality, otherwise
    // APIs do the job
    //
    if (m_pInPlaceSiteWndless)
        return m_pInPlaceSiteWndless->ScrollRect(dx, dy, prcBounds, prcClip) == S_OK;
    else {
        if (m_fInPlaceActive) 
            ScrollWindowEx(m_hwnd, dx, dy, prcBounds, prcClip, NULL, NULL, SW_INVALIDATE);
        else
            return FALSE;
    }

    return TRUE;
}


