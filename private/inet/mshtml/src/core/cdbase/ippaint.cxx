//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ippain.cxx
//
//  Contents:   IOleInPlaceSiteWindowless helper methods
//
//----------------------------------------------------------------------------

#include "headers.hxx"


//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetDC
//
//  Synopsis:   Gets a DC for the caller, type of DC depends on flags.
//
//  Arguments:  [prc]     -- param as per IOleInPlaceSiteWindowless::GetDC
//              [dwFlags] --                -do-
//              [phDC]    --                -do-
//
//  Returns:    HRESULT
//
//  History:    28-Mar-95   SumitC      Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CServer::GetDC(LPRECT prc, DWORD dwFlags, HDC * phDC)
{
    HRESULT hr = S_OK;

    if (phDC == NULL)
        return E_POINTER;

    *phDC = NULL;

    if (_pInPlace == NULL)
        return E_FAIL;

    _pInPlace->_fIPNoDraw =
    _pInPlace->_fIPPaintBkgnd =
    _pInPlace->_fIPOffScreen = FALSE;

    if (_pInPlace->_hwnd)
    {
        // get the window dc
        *phDC = ::GetDC(_pInPlace->_hwnd);
        if (*phDC == NULL)
            RRETURN(GetLastWin32Error());

        HPALETTE hpal = GetPalette(*phDC);

        if (dwFlags & OLEDC_OFFSCREEN)
        {
            // build an offscreen buffer to return
            Assert(_pInPlace->_pOSC == NULL);
            _pInPlace->_pOSC = new COffScreenContext
                (
                *phDC,
                prc->right - prc->left,
                prc->bottom - prc->top,
                hpal,
                ((dwFlags >> 16) & OFFSCR_BPP)
                    | (dwFlags & OFFSCR_SURFACE)
                    | (dwFlags & OFFSCR_3DSURFACE)
                );
            
            if (_pInPlace->_pOSC == NULL)
            {
                ::ReleaseDC(_pInPlace->_hwnd, *phDC);
                RRETURN(E_OUTOFMEMORY);
            }

            *phDC = _pInPlace->_pOSC->GetDC(prc);
            _pInPlace->_fIPOffScreen = TRUE;
        }

        if (dwFlags & OLEDC_NODRAW)
        {
            _pInPlace->_fIPNoDraw = TRUE;
        }
        else if (dwFlags & OLEDC_PAINTBKGND)
        {
            _pInPlace->_fIPPaintBkgnd = TRUE;
        }
        _pInPlace->_rcPaint = *prc;

    }
    else if (_pInPlace->_fWindowlessInplace)
    {
        hr = ((IOleInPlaceSiteWindowless *)_pInPlace->_pInPlaceSite)->
                GetDC(prc, dwFlags, phDC);
        if ((hr == S_OK) && *phDC)
            GetPalette(*phDC);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::ReleaseDC
//
//  Synopsis:   Releases a DC obtained via GetDC above.
//
//  Arguments:  [hDC] -- param as per IOleInPlaceSiteWindowless::ReleaseDC
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::ReleaseDC(HDC hDC)
{
    HRESULT hr = S_OK;

    Assert(_pInPlace);

    // Get our palette out of the DC so that it doesn't stay locked
    SelectPalette(hDC, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);

    if (_pInPlace->_hwnd)
    {
        if (_pInPlace->_fIPOffScreen)
        {
            Assert(_pInPlace->_pOSC);

            ::ReleaseDC(_pInPlace->_hwnd, _pInPlace->_pOSC->ReleaseDC(_pInPlace->_hwnd, !_pInPlace->_fIPNoDraw));

            delete _pInPlace->_pOSC;
            _pInPlace->_pOSC = NULL;
        }
        else
        {
            if (::ReleaseDC(_pInPlace->_hwnd, hDC) == 0)
                hr = GetLastWin32Error();
        }
    }
    else if (_pInPlace->_fWindowlessInplace)
    {
        hr = ((IOleInPlaceSiteWindowless *)_pInPlace->_pInPlaceSite)->ReleaseDC(hDC);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::InvalidateRect
//
//  Synopsis:
//
//  Arguments:  [prc]    -- param as per IOleInPlaceSiteWindowless::InvalidateRect
//              [fErase] --             -do-
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::InvalidateRect(LPCRECT prc, BOOL fErase)
{
    HRESULT hr = S_OK;

    if (_state >= OS_INPLACE)
    {
        Assert(_pInPlace);

        if (_pInPlace->_hwnd)
        {
#ifdef WIN16
            ::InvalidateRect(_pInPlace->_hwnd, prc, fErase);
#else
            if (::InvalidateRect(_pInPlace->_hwnd, prc, fErase) == 0)
                hr = GetLastWin32Error();
#endif                
        }
        else
        {
            Assert(_pInPlace->_fWindowlessInplace);
            hr = ((IOleInPlaceSiteWindowless *)_pInPlace->_pInPlaceSite)->
                    InvalidateRect(prc, fErase);
        }
    }

    RRETURN(hr);

}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::InvalidateRgn
//
//  Synopsis:
//
//  Arguments:  [hrgn]   -- param as per IOleInPlaceSiteWindowless::InvalidateRgn
//              [fErase] --                 -do-
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::InvalidateRgn(HRGN hrgn, BOOL fErase)
{
    HRESULT hr = S_OK;

    Assert(_pInPlace);

    if (_pInPlace->_hwnd)
    {
        ::InvalidateRgn(_pInPlace->_hwnd, hrgn, fErase);    // always returns TRUE
    }
    else
    {
        Assert(_pInPlace->_fWindowlessInplace);
        hr = ((IOleInPlaceSiteWindowless *)_pInPlace->_pInPlaceSite)->
                InvalidateRgn(hrgn, fErase);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::Scroll
//
//  Synopsis:
//
//  Arguments:  [dx]        --
//              [dy]        --
//              [prcScroll] --
//              [prcClip]   --
//
//  Returns:    HRESULT
//
//  History:    28-Mar-95   SumitC      Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CServer::Scroll(int dx, int dy, LPCRECT prcScroll, LPCRECT prcClip)
{
    HRESULT hr      = S_OK;

    Assert(_pInPlace);

    if (_pInPlace->_hwnd)
    {
        if (::ScrollWindowEx(_pInPlace->_hwnd,
                             dx,
                             dy,
                             prcScroll,
                             prcClip,
                             NULL,
                             NULL,
                             SW_INVALIDATE) == ERROR)
        {
            hr = GetLastWin32Error();
        }
    }
    else
    {
        Assert(_pInPlace->_fWindowlessInplace);
        hr = ((IOleInPlaceSiteWindowless *)_pInPlace->_pInPlaceSite)->
                ScrollRect(dx, dy, prcScroll, prcClip);
    }

    RRETURN(hr);

}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::AdjustRect
//
//  Synopsis:
//
//  Arguments:  [prc] -- param as per IOleInPlaceSiteWindowless::AdjustRect
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::AdjustRect(LPRECT prc)
{
    HRESULT hr = S_OK;

    Assert(_pInPlace);

    if (_pInPlace->_hwnd)
    {
        IntersectRect(prc, prc, &_pInPlace->_rcClip);
    }
    else
    {
        Assert(_pInPlace->_fWindowlessInplace);
        hr = ((IOleInPlaceSiteWindowless *)_pInPlace->_pInPlaceSite)->
                AdjustRect(prc);
    }

    RRETURN(hr);
}
