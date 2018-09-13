//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispsurface.cxx
//
//  Contents:   Drawing surface abstraction used by display tree.
//
//  Classes:    CDispSurface
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

#ifndef X_DDRAWEX_H_
#define X_DDRAWEX_H_
#include <ddrawex.h>
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifdef _MAC
#ifndef X_MACCONTROLS_HXX_
#define X_MACCONTROLS_HXX_
#include "maccontrols.h"
#endif
#endif


MtDefine(CDispSurface, DisplayTree, "CDispSurface")

#if !defined (NODD)
extern HRESULT InitSurface();
extern IDirectDraw * g_pDirectDraw;
extern DDPIXELFORMAT* PixelFormat(HDC hdc, long cBitsPixel);
#endif


CDispSurface::CDispSurface(HDC hdc)
{
    // there are a lot of checks to prevent this constructor from being called
    // with a NULL hdc.  If this ever happens, we will probably hang in
    // CDispRoot::DrawBands.

    Assert(hdc != NULL);
    SetRawDC(hdc);
}

void 
CDispSurface::SetRawDC(HDC hdc)
{
    ZeroInit();

    _hdc = hdc;

    if (_hdc == NULL)
        return;

#if DBG == 1
    _hpal = (HPALETTE)::GetCurrentObject(hdc, OBJ_PAL);

#if !defined(NODD)
    IDirectDrawSurface *pDDSurface = 0;

    HRESULT hr = GetSurfaceFromDC(&pDDSurface);
    if (!hr)
    {
        DDSURFACEDESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.dwSize = sizeof(desc);

        Assert(SUCCEEDED((pDDSurface)->GetSurfaceDesc(&desc)) && (desc.dwFlags & DDSD_CAPS));
        Assert((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == 0);

        ReleaseInterface(pDDSurface);
    }
#endif // !NODD
#endif // DBG == 1
}

CDispSurface::CDispSurface(IDirectDrawSurface *pDDSurface)
{
    Assert(pDDSurface != NULL);
    
    ZeroInit();
    _pDDSurface = pDDSurface;
    pDDSurface->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::CreateBuffer
//              
//  Synopsis:   Create a buffer surface compatible with this (and all the args!)
//              
//  Arguments:  pSurface        surface to clone
//              fTexture        should the dd surface be a texture surface?
//
//  Returns:    A CDispSurface* or NULL
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispSurface *
CDispSurface::CreateBuffer(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture)
{
    CDispSurface *pSurface = new CDispSurface();

    if (!pSurface)
        return 0;

    HRESULT hr;

#if !defined (NODD)
    if (fDirectDraw)
    {
        hr = THR(pSurface->InitDD(_hdc, width, height, bufferDepth, hpal, fTexture));
    }
    else
#endif
    {
        hr = THR(pSurface->InitGDI(_hdc, width, height, bufferDepth, hpal));
    }

    if (hr)
    {
        delete pSurface;
        return 0;
    }

    return pSurface;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::IsCompat
//              
//  Synopsis:   Checks if the surface is compatible with the arguments
//              
//  Arguments:  See CreateBuffer
//              
//  Returns:    TRUE if successful
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
BOOL
CDispSurface::IsCompat(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture)
{
    // BUGBUG (michaelw) It really isn't enough to compare bufferDepth because of the strange
    //                   16 bit formats that may be different from one display to the next.
    //                   This could only happen in a multi monitor situation when we move from
    //                   entirely on one monitor to entirely on the other.  In all other cases
    //                   we are (I believe) insulated from this crap.

    return (_sizeBitmap.cx >= width
    &&          _sizeBitmap.cy >= height
    &&          _bufferDepth == bufferDepth
    &&          (!hpal || _hpal == hpal)
    &&          (_pDDSurface != NULL) == fDirectDraw
    &&          _fTexture == fTexture);
}

#if !defined(NODD)
//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::InitDD
//              
//  Synopsis:   Create a DD surface.
//              
//  Arguments:  See CreateBuffer
//              
//  Returns:    A regular HRESULT
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT
CDispSurface::InitDD(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal, BOOL fTexture)
{

    IDirectDrawPalette* pDDPal = 0;

    Assert(_pDDSurface == 0);
    Assert(_hbm == 0);

    HRESULT hr = THR(InitSurface());
    if (FAILED(hr))
        RRETURN(hr);

    // Figure out the insanely stupid dd pixel format for our buffer depth and dc
    DDPIXELFORMAT* pPF = PixelFormat(hdc, bufferDepth);
    if (!pPF)
        RRETURN(E_FAIL);

    // Setup the surface description
    DDSURFACEDESC	ddsd;

    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat = *pPF;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_DATAEXCHANGE | DDSCAPS_OWNDC;
    if (fTexture)
        ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;

    // Actually create the surface
    hr = THR(g_pDirectDraw->CreateSurface(&ddsd, &_pDDSurface, NULL));
    if (FAILED(hr))
        goto Cleanup;

    // set color table
    if (bufferDepth <= 8)
    {
        extern PALETTEENTRY g_pal16[];
        extern PALETTEENTRY g_pal2[];
        PALETTEENTRY*       pPal;
        PALETTEENTRY        pal256[256];
        long                cEntries;
        DWORD		        pcaps;

        if (bufferDepth == 8)
        {
            cEntries = GetPaletteEntries(hpal, 0, 256, pal256);
            pPal = pal256;
            pcaps = DDPCAPS_8BIT;
        }
        else if (bufferDepth == 4)
        {
            cEntries = 16;
            pPal = g_pal16;
            pcaps = DDPCAPS_4BIT;
        }
        else if (bufferDepth == 1)
        {
            cEntries = 2;
            pPal = g_pal2;
            pcaps = DDPCAPS_1BIT;
        }
        else
        {
            Assert(0 && "invalid cBitsPerPixel");
            goto Cleanup;
        }
        
        // create and initialize a new DD palette
        hr = THR(g_pDirectDraw->CreatePalette(pcaps | DDPCAPS_INITIALIZE, pPal, &pDDPal, NULL));
        if (FAILED(hr))
            goto Cleanup;

        // attach the DD palette to the DD surface
        hr = THR(_pDDSurface->SetPalette(pDDPal));
        if (FAILED(hr))
            goto Cleanup;
    }

    hr = THR(_pDDSurface->GetDC(&_hdc));
    if (FAILED(hr))
        goto Cleanup;

    if (hpal)
    {
        ::SelectPalette(_hdc, hpal, TRUE);
        ::RealizePalette(_hdc);
    }


    Assert(VerifyGetSurfaceFromDC());

    _sizeBitmap.cx = width;
    _sizeBitmap.cy = height;
    _fTexture = fTexture;
    _hpal = hpal;
    _bufferDepth = bufferDepth;

Cleanup:
    ReleaseInterface(pDDPal);
    if (hr)
        ClearInterface(&_pDDSurface);

    return hr;
}
#endif //NODD

//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::InitDD
//              
//  Synopsis:   Create a compatible bitmap surface.
//              
//  Arguments:  See CreateBuffer
//              
//  Returns:    A regular HRESULT
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT
CDispSurface::InitGDI(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal)
{
    // Compatible bitmaps have to be the same bith depth as the display
    Assert(bufferDepth == GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL));

    _hbm = ::CreateCompatibleBitmap(hdc, width, height);
    if (!_hbm)
        RRETURN(GetLastError());

    _hdc = ::CreateCompatibleDC(hdc);
    if (!_hdc)
    {
        ::DeleteObject(_hbm);
        _hbm = NULL;
        RRETURN(GetLastError());
    }

    ::SelectObject(_hdc, _hbm);

    if (!hpal)
    {
        hpal = (HPALETTE) ::GetCurrentObject(hdc, OBJ_PAL);
        if (hpal == NULL)
            hpal = g_hpalHalftone;
    }

    if (hpal)
    {
        ::SelectPalette(_hdc, hpal, TRUE);
        ::RealizePalette(_hdc);
    }

    _sizeBitmap.cx = width;
    _sizeBitmap.cy = height;
    _fTexture = FALSE;
    _hpal = hpal;
    _bufferDepth = bufferDepth;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::~CDispSurface
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------


CDispSurface::~CDispSurface()
{
    if (_hbm != NULL)
    {
        // clear out old bit map
        ::DeleteDC(_hdc);
        ::DeleteObject(_hbm);
    }

    if (_pDDSurface)
    {
        _pDDSurface->ReleaseDC(_hdc);
        _pDDSurface->Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::Draw
//              
//  Synopsis:   Blit the contents of this surface into the destination surface.
//              
//  Arguments:  pDestinationSurface     destination surface
//              rc                      rect to transfer (destination coords.)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispSurface::Draw(CDispSurface* pDestinationSurface, const CRect& rc) const
{
    Assert(pDestinationSurface != NULL);
    
    Assert(_hdc != NULL && (_hbm != NULL || _pDDSurface != NULL) && pDestinationSurface->_hdc != NULL);
    Assert(_sizeBitmap.cx > 0 && _sizeBitmap.cy > 0);
    
    ::SetViewportOrgEx(_hdc, 0, 0, NULL);
    ::BitBlt(
        pDestinationSurface->_hdc,
        rc.left, rc.top,
        min(_sizeBitmap.cx, rc.Width()),
        min(_sizeBitmap.cy, rc.Height()),
        _hdc,
        0, 0,
        SRCCOPY);

#ifdef _MAC
    DrawMacScrollbars();
#endif

}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetDC
//
//  Synopsis:   Return an HDC for rendering, with view point origin and clip
//              region already incorporated.
//
//  Arguments:  phdc            pointer to returned HDC
//              rcBounds        bounds of item to render
//              rcWillDraw      area that will be drawn
//              fNeedsPhysicalClip  TRUE if physical clipping is requested
//
//  Returns:    S_OK if successful
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetDC(
        HDC* phdc,
        const RECT& rcBounds,
        const RECT& rcWillDraw,
        BOOL fNeedsPhysicalClip,
        BOOL fMustPhysicallyClip)
{
    Assert(_pOffset != NULL);
    Assert(_prcClip != NULL);
    Assert(_prgnClip != NULL);
    
    CSize deviceOffset = *_pOffset - _sizeBandOffset;
    
    // change the clip region on this DC if physical clipping requirement has
    // changed, or if our clip region has changed
    if (fMustPhysicallyClip ||
        (fNeedsPhysicalClip && ((const CRect&)rcBounds) != rcWillDraw) ||
        _fWasPhysicalClip ||
        _fClipRgnHasChanged)
    {
        _fClipRgnHasChanged = FALSE;
        _fWasPhysicalClip = fNeedsPhysicalClip;
        
        CRegion rgnClip(*_prgnClip);
        if (_sizeBandOffset != g_Zero.size)
        {
            rgnClip.Offset(-_sizeBandOffset);
        }
        
        if (fNeedsPhysicalClip)
        {
            CRect rcClipGlobal(*_prcClip);
            rcClipGlobal.IntersectRect(rcWillDraw);
            rcClipGlobal.OffsetRect(deviceOffset);
            rgnClip.Intersect(rcClipGlobal);
        }
        
        if (rgnClip.IsRegion())
        {
            ::SelectClipRgn(_hdc,rgnClip.GetRegionAlias());
        }
        else
        {
            HRGN hrgnClip = ::CreateRectRgnIndirect(&rgnClip.AsRect());
            ::SelectClipRgn(_hdc,hrgnClip);
            ::DeleteObject(hrgnClip);
        }
    }
    
    // set DC origin
    ::SetViewportOrgEx(
        _hdc,
        deviceOffset.cx, 
        deviceOffset.cy,
        NULL);
    
    *phdc = _hdc;

#if DBG == 1
    if ((GetDeviceCaps(_hdc, PLANES) * GetDeviceCaps(_hdc, BITSPIXEL)) == 8)
    {
        Assert(_hpal == (HPALETTE)::GetCurrentObject(_hdc, OBJ_PAL));
    }
#endif
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetGlobalDC
//              
//  Synopsis:   Return a global HDC for rendering, with clip
//              region already incorporated.
//
//  Arguments:  phdc            pointer to returned HDC
//              prcBounds       in: bounds in local coordinates
//                              out: bounds in global coordinates
//              prcRedraw       in: rect to redraw in local coordinates
//                              out: rect to redraw in global coordinates
//              fNeedsPhysicalClip  TRUE if physical clipping is requested
//
//  Returns:    S_OK if successful
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetGlobalDC(
        HDC* phdc,
        RECT* prcBounds,
        RECT* prcRedraw,
        BOOL fNeedsPhysicalClip)
{
    HRESULT hr = GetDC(phdc, *prcBounds, *prcRedraw, fNeedsPhysicalClip, fNeedsPhysicalClip);
    
    if (SUCCEEDED(hr))
    {
        // set global DC origin
        ::SetViewportOrgEx(
            *phdc,
            -_sizeBandOffset.cx,
            -_sizeBandOffset.cy,
            NULL);
    }
    
    // return rects in local coords.
    ((CRect*)prcBounds)->OffsetRect(*_pOffset);
    ((CRect*)prcRedraw)->OffsetRect(*_pOffset);
    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetDirectDrawSurface
//
//  Synopsis:   Return a DirectDraw surface for rendering.
//
//  Arguments:  ppSurface       pointer to IDirectDrawSurface* for result
//              pOffset         offset from global coordinates
//
//  Returns:    S_OK if successful
//
//  Notes:      If the client didn't specify a desire for a DirectDraw
//              interface when inserted in the tree, he may not get one now.
//
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetDirectDrawSurface(
        IDirectDrawSurface** ppSurface,
        SIZE* pOffset)
{
    *ppSurface = _pDDSurface;
    *pOffset = *_pOffset;
    return (_pDDSurface == NULL) ? E_FAIL : S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetSurface
//
//  Synopsis:   Get a surface using a general IID-based interface.
//
//  Arguments:  iid         IID of surface interface to be returned
//              ppv         interface pointer returned
//              pOffset     offset to global coordinates
//
//  Returns:    S_OK if successful, E_NOINTERFACE if we don't have the
//              requested interface, E_FAIL for other problems
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetSurface(REFIID iid, void** ppv, SIZE* pOffset)
{
    if (iid == IID_IDirectDrawSurface)
    {
        return GetDirectDrawSurface((IDirectDrawSurface**)ppv,pOffset);
    }
    return E_NOINTERFACE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetClipRect
//              
//  Synopsis:   Get clip region bounds.
//              
//  Arguments:  prc     RECT which returns the result
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispSurface::GetClipRect(RECT* prc)
{
    *prc = *_prcClip;
    ::OffsetRect(prc,_pOffset->cx,_pOffset->cy);
    ((CRect*)prc)->IntersectRect(_prgnClip->GetBounds());
    ::OffsetRect(prc,-_pOffset->cx,-_pOffset->cy);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetOffset
//              
//  Synopsis:   Return the current offset to local coordinates.
//              
//  Arguments:  pOffset     returns the offset
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispSurface::GetOffset(SIZE* pOffset) const
{
    *pOffset = *_pOffset - _sizeBandOffset;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetSurface
//
//  Synopsis:   Return current DC or DirectDraw rendering surface.
//
//  Arguments:  hdc         returns DC
//              ppSurface   returns DirectDraw surface
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispSurface::GetSurface(HDC *hdc, IDirectDrawSurface** ppSurface)
{
    if (_pDDSurface)
    {
        *hdc = NULL;
        _pDDSurface->AddRef();
        *ppSurface = _pDDSurface;
    }
    else
    {
        *ppSurface = NULL;
        // BUGBUG: ill-defined ownership... should be a copy?
        *hdc = _hdc;
    }
}

HRESULT
CDispSurface::GetSurfaceFromDC(HDC hdc, IDirectDrawSurface **ppDDSurface)
{
#if !defined(NODD)
    IDirectDraw3 *pDD3 = 0;

    HRESULT hr = THR(InitSurface());
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(g_pDirectDraw->QueryInterface(IID_IDirectDraw3, (LPVOID *)&pDD3));
    if (FAILED(hr))
        goto Cleanup;

    Assert(pDD3);
    hr = THR(pDD3->GetSurfaceFromDC(hdc, ppDDSurface));

    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pDD3);

#else
    HRESULT hr = E_FAIL;
#endif
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::VerifyGetSurfaceFromDC
//
//  Synopsis:   Verifies that it is possible to GetSurfaceFromDC
//
//  Arguments:  hdc
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CDispSurface::VerifyGetSurfaceFromDC(HDC hdc)
{
#if !defined(NODD)
    IDirectDrawSurface *pDDSurface;

    HRESULT hr = THR(GetSurfaceFromDC(hdc, &pDDSurface));

    if (SUCCEEDED(hr))
    {
        pDDSurface->Release();
    }

    return hr == S_OK;
#else
    return FALSE;
#endif
}
