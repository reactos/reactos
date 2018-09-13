//=--------------------------------------------------------------------------=
// CtlView.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of the IViewObjectEx interface, which is a moderately
// non-trivial bunch of code.
//
#include "IPServer.H"

#include "CtrlObj.H"
#include "Globals.H"
#include "Util.H"

// for ASSERT and FAIL
//
SZTHISFILE

// local functions we're going to find useful
//
HDC _CreateOleDC(DVTARGETDEVICE *ptd);

//=--------------------------------------------------------------------------=
// COleControl::Draw    [IViewObject2]
//=--------------------------------------------------------------------------=
// Draws a representation of an object onto the specified device context. 
//
// Parameters:
//    DWORD                - [in] draw aspect
//    LONG                 - [in] part of object to draw [not relevant]
//    void *               - NULL
//    DVTARGETDEVICE *     - [in] specifies the target device
//    HDC                  - [in] information context for target device
//    HDC                  - [in] target device context
//    LPCRECTL             - [in] rectangle in which the object is drawn
//    LPCRECTL             - [in] window extent and origin for metafiles
//    BOOL (*)(DWORD)      - [in] callback for continuing or cancelling drawing
//    DWORD                - [in] parameter to pass to callback.
//
// Output:
//    HRESULT
//
// Notes:
//    - we support the following OCX 96 extensions
//        a. flicker free drawing [multi-pass drawing]
//        b. pvAspect != NULL for optimized DC handling
//        c. prcBounds == NULL for windowless inplace active objects
//
STDMETHODIMP COleControl::Draw
(
    DWORD            dwDrawAspect,
    LONG             lIndex,
    void            *pvAspect,
    DVTARGETDEVICE  *ptd,
    HDC              hicTargetDevice,
    HDC              hdcDraw,
    LPCRECTL         prcBounds,
    LPCRECTL         prcWBounds,
    BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue),
    ULONG_PTR        dwContinue
)
{
    HRESULT hr;
    RECTL rc;
    POINT pVp, pW;
    BOOL  fOptimize = FALSE;
    int iMode;
    BYTE fMetafile = FALSE;
    BYTE fDeleteDC = FALSE;

    // support the aspects required for multi-pass drawing
    //
    switch (dwDrawAspect) {
        case DVASPECT_CONTENT:
        case DVASPECT_OPAQUE:
        case DVASPECT_TRANSPARENT:
            break;
        default:
            return DV_E_DVASPECT;
    }

    // first, have to do a little bit to support printing.
    //
    if (GetDeviceCaps(hdcDraw, TECHNOLOGY) == DT_METAFILE) {

        // We are dealing with a metafile.
        //
        fMetafile = TRUE;

        // If attributes DC is NULL, create one, based on ptd.
        //
        if (!hicTargetDevice) {

            // Does _CreateOleDC have to return an hDC
            // or can it be flagged to return an hIC 
            // for this particular case?
            //
            hicTargetDevice = _CreateOleDC(ptd);
            fDeleteDC = TRUE;
        }
    }

    // check to see if we have any flags passed in the pvAspect parameter.
    //
    if (pvAspect && ((DVASPECTINFO *)pvAspect)->cb == sizeof(DVASPECTINFO))
        fOptimize = (((DVASPECTINFO *)pvAspect)->dwFlags & DVASPECTINFOFLAG_CANOPTIMIZE) ? TRUE : FALSE;

    // if we are windowless, then we just pass this on to the end control code.
    //
    if (m_fInPlaceActive) {

        // give them a rectangle with which to draw
        //
        //ASSERT(!m_fInPlaceActive || !prcBounds, "Inplace active and somebody passed in prcBounds!!!");
        if (prcBounds)
		memcpy(&rc, prcBounds, sizeof(rc));
	else
		memcpy(&rc, &m_rcLocation, sizeof(rc));
    } else {

        // first -- convert the DC back to MM_TEXT mapping mode so that the
        // window proc and OnDraw can share the same painting code.  save
        // some information on it, so we can restore it later [without using
        // a SaveDC/RestoreDC]
        //
        rc = *prcBounds;

        // Don't do anything to hdcDraw if it's a metafile.
        // The control's Draw method must make the appropriate
        // accomodations for drawing to a metafile
        //
        if (!fMetafile) {
            LPtoDP(hdcDraw, (POINT *)&rc, 2);
            SetViewportOrgEx(hdcDraw, 0, 0, &pVp);
            SetWindowOrgEx(hdcDraw, 0, 0, &pW);
            iMode = SetMapMode(hdcDraw, MM_TEXT);
        }
    }

    // prcWBounds is NULL and not used if we are not dealing with a metafile.
    // For metafiles, we pass on rc as *prcBounds, we should also include
    // prcWBounds
    //
    hr = OnDraw(dwDrawAspect, hdcDraw, &rc, prcWBounds, hicTargetDevice, fOptimize);

    // clean up the DC when we're done with it, if appropriate.
    //
    if (!m_fInPlaceActive) {
        SetViewportOrgEx(hdcDraw, pVp.x, pVp.y, NULL);
        SetWindowOrgEx(hdcDraw, pW.x, pW.y, NULL);
        SetMapMode(hdcDraw, iMode);
    }

    // if we created a dc, blow it away now
    //
    if (fDeleteDC) DeleteDC(hicTargetDevice);
    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::DoSuperClassPaint
//=--------------------------------------------------------------------------=
// design time painting of a subclassed control.
//
// Parameters:
//    HDC                - [in]  dc to work with
//    LPCRECTL           - [in]  rectangle to paint to.  should be in pixels
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::DoSuperClassPaint
(
    HDC      hdc,
    LPCRECTL prcBounds
)
{
    HWND hwnd;
    RECT rcClient;
    int  iMapMode;
    POINT ptWOrg, ptVOrg;
    SIZE  sWOrg, sVOrg;

    // make sure we have a window.
    //
    hwnd = CreateInPlaceWindow(0,0, FALSE);
    if (!hwnd)
        return E_FAIL;

    GetClientRect(hwnd, &rcClient);

    // set up the DC for painting.  this code largely taken from the MFC CDK
    // DoSuperClassPaint() fn.  doesn't always get things like command
    // buttons quite right ...
    //
    // NOTE: there is a windows 95 problem in which the font instance manager
    // will leak a bunch of bytes in the global GDI pool whenever you 
    // change your extents and have an active font.  this code gets around
    // this for on-screen cases, but not for printing [which shouldn't be
    // too serious, because you're not often changing your control size and
    // printing rapidly in succession]
    //
    if ((rcClient.right - rcClient.left != prcBounds->right - prcBounds->left)
        && (rcClient.bottom - rcClient.top != prcBounds->bottom - prcBounds->top)) {

        iMapMode = SetMapMode(hdc, MM_ANISOTROPIC);
        SetWindowExtEx(hdc, rcClient.right, rcClient.bottom, &sWOrg);
        SetViewportExtEx(hdc, prcBounds->right - prcBounds->left, prcBounds->bottom - prcBounds->top, &sVOrg);
    }

    SetWindowOrgEx(hdc, 0, 0, &ptWOrg);
    SetViewportOrgEx(hdc, prcBounds->left, prcBounds->top, &ptVOrg);

#if STRICT
    CallWindowProc((WNDPROC)SUBCLASSWNDPROCOFCONTROL(m_ObjectType), hwnd, (g_fSysWin95Shell) ? WM_PRINT : WM_PAINT, (WPARAM)hdc, (LPARAM)(g_fSysWin95Shell ? PRF_CHILDREN | PRF_CLIENT : 0));
#else
    CallWindowProc((FARPROC)SUBCLASSWNDPROCOFCONTROL(m_ObjectType), hwnd, (g_fSysWin95Shell) ? WM_PRINT : WM_PAINT, (WPARAM)hdc, (LPARAM)(g_fSysWin95Shell ? PRF_CHILDREN | PRF_CLIENT : 0));
#endif // STRICT

    return S_OK;
}


//=--------------------------------------------------------------------------=
// COleControl::GetColorSet    [IViewObject2]
//=--------------------------------------------------------------------------=
// Returns the logical palette that the control will use for drawing in its
// IViewObject::Draw method with the corresponding parameters.
//
// Parameters:
//    DWORD                - [in]  how the object is to be represented
//    LONG                 - [in]  part of the object to draw [not relevant]
//    void *               - NULL
//    DVTARGETDEVICE *     - [in]  specifies the target device
//    HDC                  - [in]  information context for the target device
//    LOGPALETTE **        - [out] where to put palette
//
// Output:
//    S_OK                 - Control has a palette, and returned it through the out param.
//    S_FALSE              - Control does not currently have a palette.
//    E_NOTIMPL            - Control will never have a palette so optimize handling of this control.
//
// Notes:
//
STDMETHODIMP COleControl::GetColorSet
(
    DWORD            dwDrawAspect,
    LONG             lindex,
    void            *IgnoreMe,
    DVTARGETDEVICE  *ptd,
    HDC              hicTargetDevice,
    LOGPALETTE     **ppColorSet
)
{
    if (dwDrawAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    *ppColorSet = NULL;
    return (OnGetPalette(hicTargetDevice, ppColorSet)) ? ((*ppColorSet) ? S_OK : S_FALSE) : E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::Freeze    [IViewObject2]
//=--------------------------------------------------------------------------=
// Freezes a certain aspect of the object's presentation so that it does not
// change until the IViewObject::Unfreeze method is called.
//
// Parameters:
//    DWORD            - [in] aspect
//    LONG             - [in] part of object to draw
//    void *           - NULL
//    DWORD *          - [out] for Unfreeze
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Freeze
(
    DWORD   dwDrawAspect,
    LONG    lIndex,
    void   *IgnoreMe,
    DWORD  *pdwFreeze
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::Unfreeze    [IVewObject2]
//=--------------------------------------------------------------------------=
// Releases a previously frozen drawing. The most common use of this method
// is for banded printing.
//
// Parameters:
//    DWORD        - [in] cookie from freeze
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Unfreeze
(
    DWORD dwFreeze
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::SetAdvise    [IViewObject2]
//=--------------------------------------------------------------------------=
// Sets up a connection between the control and an advise sink so that the
// advise sink can be notified about changes in the control's view.
//
// Parameters:
//    DWORD            - [in] aspect
//    DWORD            - [in] info about the sink
//    IAdviseSink *    - [in] the sink
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::SetAdvise
(
    DWORD        dwAspects,
    DWORD        dwAdviseFlags,
    IAdviseSink *pAdviseSink
)
{
    // if it's not a content aspect, we don't support it.
    //
    if (!(dwAspects & DVASPECT_CONTENT)) {
        return DV_E_DVASPECT;
    }

    // set up some flags  [we gotta stash for GetAdvise ...]
    //
    m_fViewAdvisePrimeFirst = (dwAdviseFlags & ADVF_PRIMEFIRST) ? TRUE : FALSE;
    m_fViewAdviseOnlyOnce = (dwAdviseFlags & ADVF_ONLYONCE) ? TRUE : FALSE;

    RELEASE_OBJECT(m_pViewAdviseSink);
    m_pViewAdviseSink = pAdviseSink;
    ADDREF_OBJECT(m_pViewAdviseSink);

    // prime them if they want it [we need to store this so they can get flags later]
    //
    if (m_fViewAdvisePrimeFirst)
        ViewChanged();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetAdvise    [IViewObject2]
//=--------------------------------------------------------------------------=
// Retrieves the existing advisory connection on the control if there is one.
// This method simply returns the parameters used in the most recent call to
// the IViewObject::SetAdvise method.
//
// Parameters:
//    DWORD *            - [out]  aspects
//    DWORD *            - [out]  advise flags
//    IAdviseSink **     - [out]  the sink
//
// Output:
//    HRESULT
//
// Notes;
//
STDMETHODIMP COleControl::GetAdvise
(
    DWORD        *pdwAspects,
    DWORD        *pdwAdviseFlags,
    IAdviseSink **ppAdviseSink
)
{
    // if they want it, give it to them
    //
    if (pdwAspects)
        *pdwAspects = DVASPECT_CONTENT;

    if (pdwAdviseFlags) {
        *pdwAdviseFlags = 0;
        if (m_fViewAdviseOnlyOnce) *pdwAdviseFlags |= ADVF_ONLYONCE;
        if (m_fViewAdvisePrimeFirst) *pdwAdviseFlags |= ADVF_PRIMEFIRST;
    }

    if (ppAdviseSink) {
        *ppAdviseSink = m_pViewAdviseSink;
        ADDREF_OBJECT(*ppAdviseSink);
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetExtent    [IViewObject2]
//=--------------------------------------------------------------------------=
// Returns the size that the control will be drawn on the
// specified target device.
//
// Parameters:
//    DWORD            - [in] draw aspect
//    LONG             - [in] part of object to draw
//    DVTARGETDEVICE * - [in] information about target device
//    LPSIZEL          - [out] where to put the size
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetExtent
(
    DWORD           dwDrawAspect,
    LONG            lindex,
    DVTARGETDEVICE *ptd,
    LPSIZEL         psizel
)
{
    // we already have an implementation of this [from IOleObject]
    //
    return GetExtent(dwDrawAspect, psizel);
}


//=--------------------------------------------------------------------------=
// COleControl::OnGetPalette    [overridable]
//=--------------------------------------------------------------------------=
// called when the host wants palette information.  ideally, people should use
// this sparingly and carefully.
//
// Parameters:
//    HDC            - [in]  HIC for the target device
//    LOGPALETTE **  - [out] where to put the palette
//
// Output:
//    BOOL           - TRUE means we processed it, false means nope.
//
// Notes:
//
BOOL COleControl::OnGetPalette
(
    HDC          hicTargetDevice,
    LOGPALETTE **ppColorSet
)
{
    return FALSE;
}


//=--------------------------------------------------------------------------=
// COleControl::GetRect    [IViewObjectEx]
//=--------------------------------------------------------------------------=
// returns a rectnagle describing a given drawing aspect
//
// Parameters:
//    DWORD             - [in]  aspect
//    LPRECTL           - [out] region rectangle
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetRect
(
    DWORD    dvAspect,
    LPRECTL  prcRect
)
{
    RECTL rc;
    BOOL  f;

    // call the user routine and let them return the size
    //
    f = OnGetRect(dvAspect, &rc);
    if (!f) return DV_E_DVASPECT;

    // transform these dudes.
    //
    PixelToHiMetric((LPSIZEL)&rc, (LPSIZEL)prcRect);
    PixelToHiMetric((LPSIZEL)(LPBYTE)&rc + sizeof(SIZEL), (LPSIZEL)((LPBYTE)prcRect + sizeof(SIZEL)));

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetViewStatus    [IViewObjectEx]
//=--------------------------------------------------------------------------=
// returns information about the opactiy of the object and what drawing
// aspects are supported
//
// Parameters:
//    DWORD *            - [out] the status
//
/// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetViewStatus
(
    DWORD *pdwStatus
)
{
    // depending on the flag in the CONTROLOBJECTINFO structure, indicate our
    // transparency vs opacity.
    // OVERRIDE:  controls that wish to support multi-pass drawing should
    // override this routine and return, in addition to the flags indication
    // opacity, flags indicating what sort of drawing aspects they support.
    //
    *pdwStatus = FCONTROLISOPAQUE(m_ObjectType) ? VIEWSTATUS_OPAQUE : 0;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::QueryHitPoint    [IViewObjectEx]
//=--------------------------------------------------------------------------=
// indicates whether a point is within a given aspect of an object.
//
// Parameters:
//    DWORD                - [in]  aspect
//    LPCRECT              - [in]  Bounds rectangle
//    POINT                - [in]  hit location client coordinates
//    LONG                 - [in]  what the container considers close
//    DWORD *              - [out] info about the hit
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::QueryHitPoint
(
    DWORD    dvAspect,
    LPCRECT  prcBounds,
    POINT    ptLocation,
    LONG     lCloseHint,
    DWORD   *pdwHitResult
)
{
    // OVERRIDE: override me if you want to provide additional [non-opaque]
    // functionality
    //
    if (dvAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    *pdwHitResult = PtInRect(prcBounds, ptLocation) ? HITRESULT_HIT : HITRESULT_OUTSIDE;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::QueryHitRect    [IViewObjectEx]
//=--------------------------------------------------------------------------=
// indicates wheter any point in a rectangle is within a given drawing aspect
// of an object.
//
// Parameters:
//    DWORD            - [in]  aspect
//    LPCRECT          - [in]  bounds
//    LPCRECT          - [in]  location
//    LONG             - [in]  what host considers close
//    DWORD *          - [out] hit result
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::QueryHitRect
(
    DWORD     dvAspect,
    LPCRECT   prcBounds,
    LPCRECT   prcLocation,
    LONG      lCloseHint,
    DWORD    *pdwHitResult
)
{
    RECT rc;

    // OVERRIDE: override this for additional behaviour
    //
    if (dvAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    *pdwHitResult = IntersectRect(&rc, prcBounds, prcLocation) ? HITRESULT_HIT : HITRESULT_OUTSIDE;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetNaturalExtent    [IViewObjectEx]
//=--------------------------------------------------------------------------=
// supports two types of control sizing, content and integral.
//
// Parameters:
//    DWORD            - [in]  aspect
//    LONG             - [in]  index
//    DVTARGETDEVICE * - [in]  target device information
//    HDC              - [in]  HIC
//    DVEXTENTINFO *   - [in]  sizing data
//    LPSIZEL          - [out] sizing data retunred by control
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetNaturalExtent
(
    DWORD           dvAspect,
    LONG            lIndex,
    DVTARGETDEVICE *ptd,
    HDC             hicTargetDevice,
    DVEXTENTINFO   *pExtentInfo,
    LPSIZEL         pSizel
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::OnGetRect    [overridable
//=--------------------------------------------------------------------------=
// returns our rectangle
//
// Parameters:
//    DWORD              - [in]  aspect they want the rect for
//    RECTL *            - [out] the rectangle that matches this aspect
//
// Output:
//    BOOL               - false means we don't like the aspect
//
// Notes:
//
BOOL COleControl::OnGetRect
(
    DWORD   dvAspect,
    RECTL  *pRect
)
{
    // by default, we only support content drawing.
    //
    if (dvAspect != DVASPECT_CONTENT)
        return FALSE;

    // just give them our bounding rectangle
    //
    *((LPRECT)pRect) = m_rcLocation;
    return TRUE;
}

//=--------------------------------------------------------------------------=
// _CreateOleDC
//=--------------------------------------------------------------------------=
// creates an HDC given a DVTARGETDEVICE structure.
//
// Parameters:
//    DVTARGETDEVICE *              - [in] duh.
//
// Output:
//    HDC
//
// Notes:
//
HDC _CreateOleDC
(
    DVTARGETDEVICE *ptd
)
{
    LPDEVMODEW   pDevModeW;
    DEVMODEA     DevModeA, *pDevModeA;
    LPOLESTR     lpwszDriverName;
    LPOLESTR     lpwszDeviceName;
    LPOLESTR     lpwszPortName;
    HDC          hdc;

    // return screen DC for NULL target device
    //
    if (!ptd)
        return CreateDC("DISPLAY", NULL, NULL, NULL);

    if (ptd->tdExtDevmodeOffset == 0)
        pDevModeW = NULL;
    else
        pDevModeW = (LPDEVMODEW)((LPSTR)ptd + ptd->tdExtDevmodeOffset);

    lpwszDriverName = (LPOLESTR)((BYTE*)ptd + ptd->tdDriverNameOffset);
    lpwszDeviceName = (LPOLESTR)((BYTE*)ptd + ptd->tdDeviceNameOffset);
    lpwszPortName   = (LPOLESTR)((BYTE*)ptd + ptd->tdPortNameOffset);

    MAKE_ANSIPTR_FROMWIDE(pszDriverName, lpwszDriverName);
    MAKE_ANSIPTR_FROMWIDE(pszDeviceName, lpwszDeviceName);
    MAKE_ANSIPTR_FROMWIDE(pszPortName,   lpwszPortName);

    // wow, this sucks.
    //
    if (pDevModeW) {
        WideCharToMultiByte(CP_ACP, 0, pDevModeW->dmDeviceName, -1, (LPSTR)DevModeA.dmDeviceName, CCHDEVICENAME, NULL, NULL);
	memcpy(&DevModeA.dmSpecVersion, &pDevModeW->dmSpecVersion,
		offsetof(DEVMODEA, dmFormName) - offsetof(DEVMODEA, dmSpecVersion));
        WideCharToMultiByte(CP_ACP, 0, pDevModeW->dmFormName, -1, (LPSTR)DevModeA.dmFormName, CCHFORMNAME, NULL, NULL);
	memcpy(&DevModeA.dmLogPixels, &pDevModeW->dmLogPixels, sizeof(DEVMODEA) - offsetof(DEVMODEA, dmLogPixels));
        if (pDevModeW->dmDriverExtra) {
            pDevModeA = (DEVMODEA *)HeapAlloc(g_hHeap, 0, sizeof(DEVMODEA) + pDevModeW->dmDriverExtra);
            if (!pDevModeA) return NULL;
            memcpy(pDevModeA, &DevModeA, sizeof(DEVMODEA));
            memcpy(pDevModeA + 1, pDevModeW + 1, pDevModeW->dmDriverExtra);
        } else
            pDevModeA = &DevModeA;

	DevModeA.dmSize = sizeof(DEVMODEA);
    } else
        pDevModeA = NULL;

    hdc = CreateDC(pszDriverName, pszDeviceName, pszPortName, pDevModeA);
    if (pDevModeA != &DevModeA) HeapFree(g_hHeap, 0, pDevModeA);
    return hdc;
}
