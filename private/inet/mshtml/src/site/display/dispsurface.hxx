//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispsurface.hxx
//
//  Contents:   Drawing surface abstraction used by display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPSURFACE_HXX_
#define I_DISPSURFACE_HXX_
#pragma INCMSG("--- Beg 'dispsurface.hxx'")

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif


MtExtern(CDispSurface)

//+---------------------------------------------------------------------------
//
//  Class:      CDispSurface
//
//  Synopsis:   Drawing surface abstraction used by display tree.
//
//----------------------------------------------------------------------------

class CDispSurface
{
    friend CDispLayerContext;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispSurface))

                            CDispSurface() {ZeroInit();}
                            ~CDispSurface();

                            CDispSurface(HDC hdc);
                            CDispSurface(IDirectDrawSurface *pDDSurface);

    CDispSurface *          CreateBuffer(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture);
    BOOL                    IsCompat(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture);

    static HRESULT          GetSurfaceFromDC(HDC hdc, IDirectDrawSurface **pDDSurface);
    static BOOL             VerifyGetSurfaceFromDC(HDC hdc);

    HRESULT                 GetSurfaceFromDC(IDirectDrawSurface **ppDDSurface) { return GetSurfaceFromDC(_hdc, ppDDSurface); }
    BOOL                    VerifyGetSurfaceFromDC() { return VerifyGetSurfaceFromDC(_hdc); }

    long                    Height() { return _sizeBitmap.cy; }
    long                    Width() { return _sizeBitmap.cx; }
    CSize                   Size() {return _sizeBitmap;}
    
    BOOL                    IsMetafile() { return GetDeviceCaps( _hdc, TECHNOLOGY ) == DT_METAFILE; }
    BOOL                    IsMemory() { return (_hbm != 0) || (_pDDSurface != 0) || (GetObjectType(_hdc) == OBJ_MEMDC); }

    void                    Draw(
                                CDispSurface* pDestinationSurface,
                                const CRect& rc) const;
    
    HRESULT                 GetDC(
                                HDC* phdc,
                                const RECT& rcBounds,
                                const RECT& rcWillDraw,
                                BOOL fNeedsPhysicalClip,
                                BOOL fMustPhysicallyClip = FALSE);
    
    HRESULT                 GetGlobalDC(
                                HDC* phdc,
                                RECT* prcBounds,    // in: local coords, out: global coords.
                                RECT* prcRedraw,    // in: local coords, out: global coords.
                                BOOL fNeedsPhysicalClip);
    
    HRESULT                 GetDirectDrawSurface(
                                    IDirectDrawSurface** ppSurface,
                                    SIZE* pOffset);
    HRESULT                 GetSurface(
                                    const IID& iid,
                                    void** ppv,
                                    SIZE* pOffset);

    void                    GetClipRect(RECT* prc);

    void                    GetOffset(SIZE* pOffset) const;

    void                    GetSurface(HDC *hdc, IDirectDrawSurface** ppSurface);

    void                    PrepareClientSurface(
                                CSize* pOffset,
                                CRect* prcClip)
                                    {_pOffset = pOffset; _prcClip = prcClip;}
    
    void                    SetClipRgn(CRegion* prgnClip)
                                    {_prgnClip = prgnClip;
                                     _fClipRgnHasChanged = TRUE;}
    
    void                    ForceClip()
                                    {_fClipRgnHasChanged = TRUE;}
    
    void                    SetBandOffset(const SIZE& sizeOffset)
                                    {_sizeBandOffset = sizeOffset;}
    const SIZE&             GetBandOffset() const
                                    {return _sizeBandOffset;}
    
    // internal methods
    void                    SetRawDC(HDC hdc);
    HDC                     GetRawDC() {return _hdc;}

public:

private:
    HDC                     _hdc;
    void					ZeroInit()
                                    {ZeroMemory(this, sizeof(CDispSurface));}

    HRESULT                 InitDD(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal, BOOL fTexture);
    HRESULT                 InitGDI(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal);

    CSize                   _sizeBandOffset;
    CSize                   _sizeBitmap;
    HBITMAP                 _hbm;               // NULL if we're using DD
    IDirectDrawSurface*     _pDDSurface;        // NULL if we're using GDI
    CSize*                  _pOffset;
    CRect*                  _prcClip;
    CRegion*                _prgnClip;
    BOOL                    _fClipRgnHasChanged;
    BOOL                    _fWasPhysicalClip;
    HPALETTE                _hpal;
    BOOL                    _fTexture;
    short                   _bufferDepth;
};

#pragma INCMSG("--- End 'dispsurface.hxx'")
#else
#pragma INCMSG("*** Dup 'dispsurface.hxx'")
#endif

