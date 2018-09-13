//+---------------------------------------------------------------------------
//  
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995.
//  
//  File:       xform.cxx
//  
//  Contents:   CTransform class
//  
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifdef WIN16_NEVER
long WINAPI MulDivQuick(long nMultiplicand, long nMultiplier, long nDivisor)
{
    if ( !nDivisor ) return -1;
#ifdef USE_INT64
    __int64 num = nMultiplicand * nMultiplier;
    return (num/nDivisor);
#else
	return (nMultiplicand > nMultiplier) ? (nMultiplicand / nDivisor * nMultiplier + 
											(nMultiplicand % nDivisor) * nMultiplier/nDivisor)
										: (nMultiplier / nDivisor * nMultiplicand + 
											(nMultiplier % nDivisor) * nMultiplicand/nDivisor);
#endif
	
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::WindowFromDocument
//
//  Synopsis:   Convert from document space to window space.
//
//----------------------------------------------------------------------------

long
CTransform::WindowFromDocumentX(long xlDoc) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromHim(xlDoc);
    long lRetValueNew = _ptDst.x + DxzFromDxt(dxt);

#if DBG==1
    long lRetValueOld = _ptDst.x + MulDivQuick(xlDoc, _sizeDst.cx, _sizeSrc.cx);
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return _ptDst.x + MulDivQuick(xlDoc, _sizeDst.cx, _sizeSrc.cx);

#endif  // IE5_ZOOM
}

long
CTransform::WindowFromDocumentY(long ylDoc) const
{
#ifdef  IE5_ZOOM

    long dyt = DytFromHim(ylDoc);
    long lRetValueNew = _ptDst.y + DyzFromDyt(dyt);

#if DBG==1
    long lRetValueOld = _ptDst.y + MulDivQuick(ylDoc, _sizeDst.cy, _sizeSrc.cy);
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return _ptDst.y + MulDivQuick(ylDoc, _sizeDst.cy, _sizeSrc.cy);

#endif  // IE5_ZOOM
}

long
CTransform::WindowFromDocumentCX(long xlDoc) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromHim(xlDoc);
    long lRetValueNew = DxzFromDxt(dxt);

#if DBG==1
    long lRetValueOld = MulDivQuick(xlDoc, _sizeDst.cx, _sizeSrc.cx);
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick(xlDoc, _sizeDst.cx, _sizeSrc.cx);

#endif  // IE5_ZOOM
}

long
CTransform::WindowFromDocumentCY(long ylDoc) const
{
#ifdef  IE5_ZOOM

    long dyt = DytFromHim(ylDoc);
    long lRetValueNew = DyzFromDyt(dyt);

#if DBG==1
    long lRetValueOld = MulDivQuick(ylDoc, _sizeDst.cy, _sizeSrc.cy);
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick(ylDoc, _sizeDst.cy, _sizeSrc.cy);

#endif  // IE5_ZOOM
}

void
CTransform::WindowFromDocument(RECT *prcOut, const RECTL *prclIn) const
{
#ifdef  IE5_ZOOM

    long xtLeft = DxtFromHim(prclIn->left);
    long ytTop = DytFromHim(prclIn->top);
    long xtRight = DxtFromHim(prclIn->right);
    long ytBottom = DytFromHim(prclIn->bottom);

    prcOut->left = _ptDst.x + DxzFromDxt(xtLeft);
    prcOut->top = _ptDst.y + DyzFromDyt(ytTop);
    prcOut->right = _ptDst.x + DxzFromDxt(xtRight);
    prcOut->bottom = _ptDst.y + DyzFromDyt(ytBottom);

#if DBG==1
    RECT rcOld;

    rcOld.left = _ptDst.x +
            MulDivQuick(prclIn->left, _sizeDst.cx, _sizeSrc.cx);

    rcOld.top = _ptDst.y +
            MulDivQuick(prclIn->top, _sizeDst.cy, _sizeSrc.cy);

    rcOld.right = _ptDst.x +
            MulDivQuick(prclIn->right, _sizeDst.cx, _sizeSrc.cx);

    rcOld.bottom = _ptDst.y +
            MulDivQuick(prclIn->bottom, _sizeDst.cy, _sizeSrc.cy);

    Assert(IsWithinFive(prcOut->left, rcOld.left) || IsZoomed());
    Assert(IsWithinFive(prcOut->top, rcOld.top) || IsZoomed());
    Assert(IsWithinFive(prcOut->right, rcOld.right) || IsZoomed());
    Assert(IsWithinFive(prcOut->bottom, rcOld.bottom) || IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

    prcOut->left = _ptDst.x +
            MulDivQuick(prclIn->left, _sizeDst.cx, _sizeSrc.cx);

    prcOut->top = _ptDst.y +
            MulDivQuick(prclIn->top, _sizeDst.cy, _sizeSrc.cy);

    prcOut->right = _ptDst.x +
            MulDivQuick(prclIn->right, _sizeDst.cx, _sizeSrc.cx);

    prcOut->bottom = _ptDst.y +
            MulDivQuick(prclIn->bottom, _sizeDst.cy, _sizeSrc.cy);

#endif  // IE5_ZOOM
}

void
CTransform::WindowFromDocument(POINT *pptOut, long xl, long yl) const
{
#ifdef  IE5_ZOOM

    long xt = DxtFromHim(xl);
    long yt = DytFromHim(yl);

    pptOut->x = _ptDst.x + DxzFromDxt(xt);
    pptOut->y = _ptDst.y + DyzFromDyt(yt);

#if DBG==1
    POINT ptOld;

    ptOld.x = _ptDst.x + MulDivQuick(xl, _sizeDst.cx, _sizeSrc.cx);
    ptOld.y = _ptDst.y + MulDivQuick(yl, _sizeDst.cy, _sizeSrc.cy);

    Assert(IsWithinFive(pptOut->x, ptOld.x) || IsZoomed());
    Assert(IsWithinFive(pptOut->y, ptOld.y) || IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

    pptOut->x = _ptDst.x + MulDivQuick(xl, _sizeDst.cx, _sizeSrc.cx);
    pptOut->y = _ptDst.y + MulDivQuick(yl, _sizeDst.cy, _sizeSrc.cy);

#endif  // IE5_ZOOM
}

void 
CTransform::WindowFromDocument(SIZE *psizeOut, long cxl, long cyl) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromHim(cxl);
    long dyt = DytFromHim(cyl);

    psizeOut->cx = DxzFromDxt(dxt);
    psizeOut->cy = DyzFromDyt(dyt);

#if DBG==1
    SIZE sizeOld;

    sizeOld.cx = MulDivQuick(cxl, _sizeDst.cx, _sizeSrc.cx);
    sizeOld.cy = MulDivQuick(cyl, _sizeDst.cy, _sizeSrc.cy);

    Assert(IsWithinFive(psizeOut->cx, sizeOld.cx) || IsZoomed());
    Assert(IsWithinFive(psizeOut->cy, sizeOld.cy) || IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

    psizeOut->cx = MulDivQuick(cxl, _sizeDst.cx, _sizeSrc.cx);
    psizeOut->cy = MulDivQuick(cyl, _sizeDst.cy, _sizeSrc.cy);

#endif  // IE5_ZOOM
}

// NB (cthrash) fRelative should be set to TRUE if lValue is relative to
// a pixel value (e.g. CUnitValue::UNIT_PERCENT) and shall be scaled only
// by the zoom factor.  Otherwise we should scale by the current DPI rather
// than the screen DPI.
//
// For example, let's say you have a 100x100 image.  On the screen you want
// it to be 100x100 scaled by any zooming factor.  When printing, you want
// it scaled by zooming factor (which should be unity) plus the relative
// DPI scaling between the screen and the printer.  Generally speaking, this
// would be, in pixel dimensions, larger on the printer than on the screen.
//
// If, on the other hand, we have an image whose width is designated to be
// a percentage of some other value, we don't want to scale by the DPI ratio
// when printing.

long
CTransform::DocPixelsFromWindowX ( long lValue, BOOL fRelative )
{
#ifdef  IE5_ZOOM

    // treat the incoming pixels as if they were in target units
    long lRetValueNew = fRelative ? lValue : DxzFromDxt(lValue, fRelative);

#if DBG==1
    long lRetValueOld = MulDivQuick ( lValue, 2540 * _sizeDst.cx,
                     (fRelative ? _sizeInch.cx : g_sizePixelsPerInch.cx) *
                     _sizeSrc.cx );
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick ( lValue, 2540 * _sizeDst.cx,
                         (fRelative ? _sizeInch.cx : g_sizePixelsPerInch.cx) *
                         _sizeSrc.cx );

#endif  // IE5_ZOOM
}


long
CTransform::DocPixelsFromWindowY ( long lValue, BOOL fRelative )
{
#ifdef  IE5_ZOOM

    // treat the incoming pixels as if they were in target units
    long lRetValueNew = fRelative ? lValue : DyzFromDyt(lValue, fRelative);

#if DBG==1
    long lRetValueOld = MulDivQuick ( lValue, 2540*_sizeDst.cy,
                         (fRelative ? _sizeInch.cy : g_sizePixelsPerInch.cy) *
                         _sizeSrc.cy );
    Assert(IsWithinN(lRetValueNew, lRetValueOld, 10) || IsZoomed());
#endif // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick ( lValue, 2540*_sizeDst.cy,
                         (fRelative ? _sizeInch.cy : g_sizePixelsPerInch.cy) *
                         _sizeSrc.cy );

#endif  // IE5_ZOOM
}


void
CTransform::WindowFromDocPixels ( POINT *ppt, POINT p, BOOL fRelative )
{
#ifdef  IE5_ZOOM

    ppt->x = _ptDst.x + WindowXFromDocPixels( p.x, fRelative );
    ppt->y = _ptDst.y + WindowYFromDocPixels( p.y, fRelative );

#else   // !IE5_ZOOM

    ppt->x = _ptDst.x + MulDivQuick( p.x,
                                     (fRelative ? _sizeInch.cx
                                                : g_sizePixelsPerInch.cx) *
                                     _sizeSrc.cx,
                                     2540 * _sizeDst.cx );
    ppt->y = _ptDst.y + MulDivQuick( p.y,
                                     (fRelative ? _sizeInch.cy
                                                : g_sizePixelsPerInch.cy) *
                                     _sizeSrc.cy,
                                     2540 * _sizeDst.cy );

#endif  // IE5_ZOOM
}


long
CTransform::WindowXFromDocPixels ( long lPixels, BOOL fRelative )
{
#ifdef  IE5_ZOOM

    long lRetValueNew = DxrFromDxz(lPixels, fRelative);

#if DBG==1
    long lRetValueOld = MulDivQuick( lPixels, (fRelative ? _sizeInch.cx
                                            :  g_sizePixelsPerInch.cx) * 
                        _sizeSrc.cx,
                        2540 * _sizeDst.cx );
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    // note this is not encounting _ptDst
    return MulDivQuick( lPixels, (fRelative ? _sizeInch.cx
                                            :  g_sizePixelsPerInch.cx) * 
                        _sizeSrc.cx,
                        2540 * _sizeDst.cx );

#endif  // IE5_ZOOM
}

long
CTransform::WindowYFromDocPixels ( long lPixels, BOOL fRelative )
{
#ifdef  IE5_ZOOM

    long lRetValueNew = DyrFromDyz(lPixels, fRelative);

#if DBG==1
    long lRetValueOld = MulDivQuick( lPixels, (fRelative ? _sizeInch.cy
                                            : g_sizePixelsPerInch.cy) *
                       _sizeSrc.cy,
                       2540 * _sizeDst.cy );
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick( lPixels, (fRelative ? _sizeInch.cy
                                            : g_sizePixelsPerInch.cy) *
                       _sizeSrc.cy,
                       2540 * _sizeDst.cy );

#endif  // IE5_ZOOM
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::DocumentFromWindow
//
//  Synopsis:   Convert from document space to window space.
//
//----------------------------------------------------------------------------

void
CTransform::DocumentFromWindow(RECTL *prclOut, const RECT *prcIn) const
{
#ifdef  IE5_ZOOM

    long xtLeft = DxtFromDxz(prcIn->left - _ptDst.x);
    long xtRight = DxtFromDxz(prcIn->right - _ptDst.x);
    long ytTop = DytFromDyz(prcIn->top - _ptDst.y);
    long ytBottom = DytFromDyz(prcIn->bottom - _ptDst.y);

    prclOut->left = HimFromDxt(xtLeft);
    prclOut->right = HimFromDxt(xtRight);
    prclOut->top = HimFromDyt(ytTop);
    prclOut->bottom = HimFromDyt(ytBottom);

#if DBG==1
    RECTL rclOld;

    if (_sizeDst.cx != 0)
    {
        rclOld.left = 
                MulDivQuick(prcIn->left - _ptDst.x, _sizeSrc.cx, _sizeDst.cx);

        rclOld.right =
                MulDivQuick(prcIn->right - _ptDst.x, _sizeSrc.cx, _sizeDst.cx);
    }
    else
    {
        rclOld.left = 0;
        rclOld.right = 0;
    }

    if (_sizeDst.cy != 0)
    {
        rclOld.top = 
                MulDivQuick(prcIn->top - _ptDst.y, _sizeSrc.cy, _sizeDst.cy);

        rclOld.bottom =
                MulDivQuick(prcIn->bottom - _ptDst.y, _sizeSrc.cy, _sizeDst.cy);
    }
    else
    {
        rclOld.top = 0;
        rclOld.bottom = 0;
    }

    Assert(IsWithinN(prclOut->left, rclOld.left, HimFromDxt(1)));
    Assert(IsWithinN(prclOut->right, rclOld.right, HimFromDxt(1)));
    Assert(IsWithinN(prclOut->top, rclOld.top, HimFromDyt(1)));
    Assert(IsWithinN(prclOut->bottom, rclOld.bottom, HimFromDyt(1)));
#endif  // DBG==1

#else   // IE5_ZOOM

    if (_sizeDst.cx != 0)
    {
        prclOut->left = 
                MulDivQuick(prcIn->left - _ptDst.x, _sizeSrc.cx, _sizeDst.cx);

        prclOut->right =
                MulDivQuick(prcIn->right - _ptDst.x, _sizeSrc.cx, _sizeDst.cx);
    }
    else
    {
        prclOut->left = 0;
        prclOut->right = 0;
    }

    if (_sizeDst.cy != 0)
    {
        prclOut->top = 
                MulDivQuick(prcIn->top - _ptDst.y, _sizeSrc.cy, _sizeDst.cy);

        prclOut->bottom =
                MulDivQuick(prcIn->bottom - _ptDst.y, _sizeSrc.cy, _sizeDst.cy);
    }
    else
    {
        prclOut->top = 0;
        prclOut->bottom = 0;
    }

#endif  // IE5_ZOOM
}

void
CTransform::DocumentFromWindow(POINTL *pptlOut, long x, long y) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromDxz(x - _ptDst.x);
    long dyt = DytFromDyz(y - _ptDst.y);

    pptlOut->x = HimFromDxt(dxt);
    pptlOut->y = HimFromDyt(dyt);

#if DBG==1
    POINTL ptlOld;

    ptlOld.x = _sizeDst.cx ? MulDivQuick(x - _ptDst.x, _sizeSrc.cx, _sizeDst.cx) : 0;
    ptlOld.y = _sizeDst.cy ? MulDivQuick(y - _ptDst.y, _sizeSrc.cy, _sizeDst.cy) : 0;

    Assert(IsWithinN(pptlOut->x, ptlOld.x, HimFromDxt(1)));
    Assert(IsWithinN(pptlOut->y, ptlOld.y, HimFromDyt(1)));
#endif  // DBG==1

#else   // IE5_ZOOM

    pptlOut->x = _sizeDst.cx ? MulDivQuick(x - _ptDst.x, _sizeSrc.cx, _sizeDst.cx) : 0;
    pptlOut->y = _sizeDst.cy ? MulDivQuick(y - _ptDst.y, _sizeSrc.cy, _sizeDst.cy) : 0;

#endif  // IE5_ZOOM
}

void 
CTransform::DocumentFromWindow(SIZEL *psizelOut, long cx, long cy) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromDxz(cx);
    long dyt = DytFromDyz(cy);

    psizelOut->cx = HimFromDxt(dxt);
    psizelOut->cy = HimFromDyt(dyt);

#if DBG==1
    SIZEL sizelOld;

    sizelOld.cx = _sizeDst.cx ? MulDivQuick(cx, _sizeSrc.cx, _sizeDst.cx) : 0;
    sizelOld.cy = _sizeDst.cy ? MulDivQuick(cy, _sizeSrc.cy, _sizeDst.cy) : 0;

    Assert(IsWithinN(psizelOut->cx, sizelOld.cx, HimFromDxt(1)));
    Assert(IsWithinN(psizelOut->cy, sizelOld.cy, HimFromDyt(1)));
#endif  // DBG==1

#else   // IE5_ZOOM

    psizelOut->cx = _sizeDst.cx ? MulDivQuick(cx, _sizeSrc.cx, _sizeDst.cx) : 0;
    psizelOut->cy = _sizeDst.cy ? MulDivQuick(cy, _sizeSrc.cy, _sizeDst.cy) : 0;

#endif  // IE5_ZOOM
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::DeviceFromTwips
//              CTransform::TwipsFromDevice
//
//  Synopsis:   Convert twips to device coordinates
//
//----------------------------------------------------------------------------

long
CTransform::DeviceFromTwipsCY(long twip) const
{
#ifdef  IE5_ZOOM

    long dyt = DytFromTwp(twip);
    long lRetValueNew = DyzFromDyt(dyt);

#if DBG==1
    long lRetValueOld = MulDivQuick(twip, _sizeDst.cy * 127, _sizeSrc.cy * 72);
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick(twip, _sizeDst.cy * 127, _sizeSrc.cy * 72);

#endif  // IE5_ZOOM
}

long 
CTransform::DeviceFromTwipsCX(long twip) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromTwp(twip);
    long lRetValueNew = DxzFromDxt(dxt);

#if DBG==1
    long lRetValueOld = MulDivQuick(twip, _sizeDst.cx * 127, _sizeSrc.cx * 72);
    Assert(IsWithinFive(lRetValueNew, lRetValueOld) || IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

	return MulDivQuick(twip, _sizeDst.cx * 127, _sizeSrc.cx * 72);

#endif  // IE5_ZOOM
}

long
CTransform::TwipsFromDeviceCY(long pix) const
{
#ifdef  IE5_ZOOM

    long dyt = DytFromDyz(pix);
    long lRetValueNew = TwpFromDyt(dyt);

#if DBG==1
    long lRetValueOld = MulDivQuick(pix, _sizeSrc.cy * 72, _sizeDst.cy * 127);
    Assert(IsWithinN(lRetValueNew, lRetValueOld, TwpFromDyt(2))
            || IsZoomed()
            || pix == MAX_MEASURE_WIDTH);
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick(pix, _sizeSrc.cy * 72, _sizeDst.cy * 127);

#endif  // IE5_ZOOM
}

long 
CTransform::TwipsFromDeviceCX(long pix) const
{
#ifdef  IE5_ZOOM

    long dxt = DxtFromDxz(pix);
    long lRetValueNew = TwpFromDxt(dxt);

#if DBG==1
    long lRetValueOld = MulDivQuick(pix, _sizeSrc.cy * 72, _sizeDst.cy * 127);
#ifdef REVIEW   // sidda: don't even bother asserting here; it fires too often
                // That's OK since the only caller is CLSMeasurer::LSDoCreateLine
    Assert(IsWithinN(lRetValueNew, lRetValueOld, TwpFromDxt(2))
            || IsZoomed()
            || pix == MAX_MEASURE_WIDTH);
#endif  // REVIEW
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick(pix, _sizeSrc.cx * 72, _sizeDst.cx * 127);

#endif  // IE5_ZOOM
}

#ifdef  IE5_ZOOM

X
CTransform::GetScaledDxpInch()
{
	return MulDivQuick(_wNumerX, _dxrInch, _wDenom);
}

Y
CTransform::GetScaledDypInch()
{
	return MulDivQuick(_wNumerY, _dyrInch, _wDenom);
}

//+---------------------------------------------------------------------------
//
//  Pixel scaling routines
//
//----------------------------------------------------------------------------

X
CTransform::DxrFromDxt(X dxt) const
{
    // target units ==> unzoomed render units
    return (_fDiff ?    MulDivQuick(dxt, _dxrInch, _dxtInch) :
                        dxt);
}

Y
CTransform::DyrFromDyt(Y dyt) const
{
    return (_fDiff ?    MulDivQuick(dyt, _dyrInch, _dytInch) :
                        dyt);
}

X
CTransform::DxzFromDxr(X dxr, BOOL fRelative) const
{
    // unzoomed render units ==> zoomed render units
    return ((_fScaled && !fRelative) ?  MulDivQuick(dxr, _wNumerX, _wDenom) :
                                        dxr);
}

Y
CTransform::DyzFromDyr(Y dyr, BOOL fRelative) const
{
    return ((_fScaled && !fRelative) ?  MulDivQuick(dyr, _wNumerY, _wDenom) :
                                        dyr);
}

X
CTransform::DxzFromDxt(X dxt, BOOL fRelative) const
{
    // wrapper function
    X   dxr = DxrFromDxt(dxt);
    return DxzFromDxr(dxr, fRelative);
}

Y
CTransform::DyzFromDyt(Y dyt, BOOL fRelative) const
{
    Y   dyr = DyrFromDyt(dyt);
    return DyzFromDyr(dyr, fRelative);
}

//+---------------------------------------------------------------------------
//
//  Function:   RectzFromRectr
//
//  Synopsis:   Scale a rectangle according to the current zoom factor.
//	
//  Arguments:  pRectZ      destination scaled rectangle
//              pRectR      source un-scaled rectangle
//
//----------------------------------------------------------------------------

void
CTransform::RectzFromRectr(LPRECT pRectZ, LPRECT pRectR)
{
    if (!_fScaled)
    {
        *pRectZ = *pRectR;
        return;
    }

    pRectZ->left    = DxzFromDxr(pRectR->left);
    pRectZ->right   = pRectZ->left;
    if (pRectR->right > pRectR->left)
    {
        pRectZ->right += max((long)DxzFromDxr(pRectR->right - pRectR->left), 1L);
    }

    pRectZ->top     = DyzFromDyr(pRectR->top);
    pRectZ->bottom  = pRectZ->top;
    if (pRectR->bottom > pRectR->top)
    {
        pRectZ->bottom += max((long)DyzFromDyr(pRectR->bottom - pRectR->top), 1L);
    }
}

//+---------------------------------------------------------------------------
//
//  Pixel measurement routines
//
//----------------------------------------------------------------------------

X
CTransform::DxtFromPts(float pts) const
{
    // points ==> target units
    return MulDivQuick(pts, _dxtInch, ptsInch);
}

Y
CTransform::DytFromPts(float pts) const
{
    return MulDivQuick(pts, _dytInch, ptsInch);
}

X
CTransform::DxtFromTwp(long twp) const
{
    // twips ==> target units
    return MulDivQuick(twp, _dxtInch, twpInch);
}

Y
CTransform::DytFromTwp(long twp) const
{
    return MulDivQuick(twp, _dytInch, twpInch);
}

X
CTransform::DxtFromHim(long him) const
{
    // himetric ==> target units
    return MulDivQuick(him, _dxtInch, himInch);
}

Y
CTransform::DytFromHim(long him) const
{
    return MulDivQuick(him, _dytInch, himInch);
}

//+---------------------------------------------------------------------------
//
//  Reverse pixel scaling routines
//
//----------------------------------------------------------------------------

X
CTransform::DxtFromDxr(X dxr) const
{
    // unzoomed render units ==> target units
    return (_fDiff ?    MulDivQuick(dxr, _dxtInch, _dxrInch) :
                        dxr);
}

Y
CTransform::DytFromDyr(Y dyr) const
{
    return (_fDiff ?    MulDivQuick(dyr, _dytInch, _dyrInch) :
                        dyr);
}

X
CTransform::DxrFromDxz(X dxz, BOOL fRelative) const
{
    // zoomed render units ==> unzoomed render units
    Assert(_wNumerX);
    return ((_fScaled && !fRelative) ?  MulDivQuick(dxz, _wDenom, _wNumerX) :
                                        dxz);
}

Y
CTransform::DyrFromDyz(Y dyz, BOOL fRelative) const
{
    Assert(_wNumerY);
    return ((_fScaled && !fRelative) ?  MulDivQuick(dyz, _wDenom, _wNumerY) :
                                        dyz);
}

X
CTransform::DxtFromDxz(X dxz, BOOL fRelative) const
{
    // wrapper function
    X dxr = DxrFromDxz(dxz, fRelative);
    return DxtFromDxr(dxr);
}

Y
CTransform::DytFromDyz(Y dyz, BOOL fRelative) const
{
    Y dyr = DyrFromDyz(dyz, fRelative);
    return DytFromDyr(dyr);
}

X
CTransform::HimFromDxt(long dxt) const
{
    // target units ==> himetric
    return MulDivQuick(dxt, himInch, _dxtInch);
}

Y
CTransform::HimFromDyt(long dyt) const
{
    return MulDivQuick(dyt, himInch, _dytInch);
}

X
CTransform::TwpFromDxt(long dxt) const
{
    // target units ==> twips
    return MulDivQuick(dxt, twpInch, _dxtInch);
}

Y
CTransform::TwpFromDyt(long dyt) const
{
    return MulDivQuick(dyt, twpInch, _dytInch);
}

#endif  // IE5_ZOOM

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::Init
//
//  Synopsis:   Initialize
//
//----------------------------------------------------------------------------

void
CTransform::Init(SIZEL sizelSrc)
{
    _sizeSrc = *(SIZE *)&sizelSrc;

    // initialize to 100% zoom factor by using actual screen resolution
    _sizeInch = g_sizePixelsPerInch;

    // assume destination size is the same as the source
    _ptDst      = g_Zero.pt;
    _sizeDst.cx = MulDivQuick(_sizeSrc.cx, _sizeInch.cx, 2540L);
    _sizeDst.cy = MulDivQuick(_sizeSrc.cy, _sizeInch.cy, 2540L);

    // We never have zero size. These are used for transforms,
    // and sizeSrc will be used as a denominator in a MulDiv,
    // which will return -1 for a zero divide.
    Assert( _sizeSrc.cx && _sizeDst.cx );
    Assert( _sizeSrc.cy && _sizeDst.cy );

#ifdef IE5_ZOOM

    SetScaleFraction(1, 1, 1);
    SetResolutions(&g_sizePixelsPerInch, &g_sizePixelsPerInch);

#endif  // IE5_ZOOM
}

void
CTransform::Init(
    const RECT *    prcDst,
    SIZEL           sizelSrc,
    const SIZE *    psizeInch )
{
    _sizeSrc = *(SIZE *)&sizelSrc;
    _ptDst   = *(POINT *)prcDst;
    _sizeDst.cx = prcDst->right - prcDst->left;
    _sizeDst.cy = prcDst->bottom - prcDst->top;

    // if provided a DPI value, go with it, otherwise assume unchanged

#ifdef IE5_ZOOM

    if (psizeInch)
    {
        // ASSUME: this happens only when printing

        _sizeInch = *psizeInch;

        // REVIEW sidda:    hack to force scaling from display pixels to printer pixels when printing
        //                  In future this should be done via explicit conversions.
        SetScaleFraction(1, 1, 1);
        SetResolutions(&g_sizePixelsPerInch, psizeInch);
    }
    else if (_sizeInch.cx == 0)
    {
        _sizeInch = g_sizePixelsPerInch;    // reset to something reasonable if this became zero for some reason
        SetResolutions(&g_sizePixelsPerInch, &g_sizePixelsPerInch);
    }

#else   // !IE5_ZOOM

    _sizeInch = psizeInch
                    ? *psizeInch
                    : g_sizePixelsPerInch;

#endif  // IE5_ZOOM

    // We never have zero size. These are used for transforms,
    // and sizeSrc will be used as a denominator in a MulDiv,
    // which will return -1 for a zero divide.
    Assert( _sizeSrc.cx && _sizeDst.cx );
    Assert( _sizeSrc.cy && _sizeDst.cy );
}

void
CTransform::Init(const CTransform * pTransform)
{
    _sizeSrc  = pTransform->_sizeSrc;
    _ptDst    = pTransform->_ptDst;
    _sizeDst  = pTransform->_sizeDst;
    _sizeInch = pTransform->_sizeInch;

#ifdef  IE5_ZOOM

    _wNumerX  = pTransform->_wNumerX;
    _wNumerY  = pTransform->_wNumerY;
    _wDenom   = pTransform->_wDenom;
    _fScaled  = pTransform->_fScaled;

#ifdef  IE6_ROTATE

    _ang        = pTransform->_ang;
    _mat        = pTransform->_mat;
    _matInverse = pTransform->_matInverse;

#endif  // IE6_ROTATE

    _dresLayout = pTransform->_dresLayout;
    _dresRender = pTransform->_dresRender;
    _fDiff      = pTransform->_fDiff;

#endif  // IE5_ZOOM
}


#ifdef IE5_ZOOM

//+---------------------------------------------------------------------------
//
//  Function:     GCD
//
//  Synopsis:   Compute GCD
//
//----------------------------------------------------------------------------

int GCD(int w1, int w2)
{
	w1 = abs(w1);
	w2 = abs(w2);

	if (w2 > w1)
		{
        int wT = w1;
		w1 = w2;
		w2 = wT;
		}
	if (w2 == 0)
		return w2;

	for (;;)
		{
		if ((w1 %= w2) == 0)
			return w2;

		if ((w2 %= w1) == 0)
			return w1;
		}
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::zoom
//
//  Synopsis:	Adjust internal data structures as necessary to apply zoom.
//
//  Arguments:  wNumerX     X fraction numerator
//              wNumerY     Y fraction numerator
//              wDenom      fraction denominator
//----------------------------------------------------------------------------

void
CTransform::zoom(int wNumerX, int wNumerY, int wDenom)
{
    // update view scaling information
    SetScaleFraction(wNumerX, wNumerY, wDenom);
    _sizeInch.cx = GetScaledDxpInch();
    _sizeInch.cy = GetScaledDypInch();

    _sizeDst.cx = DxzFromDxt(DxtFromHim(_sizeSrc.cx));
    _sizeDst.cy = DyzFromDyt(DytFromHim(_sizeSrc.cy));
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::SetScaleFraction
//
//  Synopsis:	Setup a scaling fraction. Only isotropic scaling is supported.
//              This method is meant to do away with the complex SetFrt() method.
//
//  Arguments:  wNumer      fraction numerator
//              wDenom      fraction denominator
//----------------------------------------------------------------------------

void
CTransform::SetScaleFraction(int wNumerX, int wNumerY, int wDenom)
{
	Assert(wNumerX >= 0);
    Assert(wNumerY >= 0);
	Assert(wDenom > 0);

    _fScaled = ((wNumerX != wDenom || wNumerY != wDenom) ? TRUE : FALSE);
    _wNumerX = wNumerX;
    _wNumerY = wNumerY;
    _wDenom = wDenom;
    SimplifyScaleFraction();
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::SimplifyScaleFraction
//
//  Synopsis:	Simplify the scaling fraction stored internally.
//----------------------------------------------------------------------------

void
CTransform::SimplifyScaleFraction()
{
	int   gcd;

	if ((gcd = GCD(_wNumerX, _wDenom)) > 1 &&
		(gcd = GCD(_wNumerY, gcd)) > 1)
		{
		_wNumerX /= gcd;
		_wNumerY /= gcd;
		_wDenom /= gcd;
		}
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransform::SetResolutions
//
//  Synopsis:	Initialize the target and rendering device resolutions.
//
//----------------------------------------------------------------------------

void
CTransform::SetResolutions(const SIZE * psizeInchTarget, const SIZE * psizeInchRender)
{
    _dresLayout.dxuInch = psizeInchTarget->cx;
    _dresLayout.dyuInch = psizeInchTarget->cy;
    _dresRender.dxuInch = psizeInchRender->cx;
    _dresRender.dyuInch = psizeInchRender->cy;

    _fDiff = ((_dxtInch != _dxrInch || _dytInch != _dyrInch) ? TRUE : FALSE);

}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::SetFrt
//
//  Synopsis:	Set wNumerX or wNumerY and wDenom for the given FRT.
//              Scale wNumerY or wNumerX by the amount wNumerX or wNumerY / wDenom
//              gets scaled by.
//
//              If fRestrict is set, FRT values are rounded to combinations
//              represntable in MGE.
//
//----------------------------------------------------------------------------

void
CFrt::SetFrt(int wNumer, int wDenom, BOOL fScaleY, BOOL fRestrict)
{
	Assert(wNumer >= 0);
	Assert(wDenom > 0);

	if (fScaleY)
		{
		_wNumerY = _wNumerX == 0 ?
			wNumer : MulDivQuick(_wNumerY, wNumer, _wNumerX);
		_wNumerX = wNumer;
		}
	else
		{
		_wNumerX = _wNumerY == 0 ?
			wNumer : MulDivQuick(_wNumerX, wNumer, _wNumerY);
		_wNumerY = wNumer;
		}
	_wDenom = wDenom;

	if (fRestrict)
		RestrictFrt();
	else
		SimplifyFrt();	// SimplifyFrt is called from RestrictFrt too
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::SimplifyFrt
//
//  Synopsis:   Factor the GCD out of the given FRT.
//              Make sure it corresponds to a round number of our units per inch.
//
//----------------------------------------------------------------------------

void
CFrt::SimplifyFrt()
{
	int   gcd;

	if ((gcd = GCD(_wNumerX, _wDenom)) > 1 &&
		(gcd = GCD(_wNumerY, gcd)) > 1)
		{
		_wNumerX /= gcd;
		_wNumerY /= gcd;
		_wDenom /= gcd;
		}
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::RestrictFrt
//
//  Synopsis:   Round FRT to what can be represented as (dxpInch,dypInch).
//	
//              Also simplifies FRT.
//
//----------------------------------------------------------------------------

void
CFrt::RestrictFrt()
{
#ifdef REVIEW   // sidda: this is the original Quill code
	// note: use MulDivT to ensure that whole-page view fits in a window
	_wNumerX = MulDivT(_wNumerX, dzlInch, _wDenom);
	_wNumerY = MulDivT(_wNumerY, dzlInch, _wDenom);
	_wDenom = dzlInch;
#endif  // REVIEW

	_wNumerX = MulDivQuick(_wNumerX, dzlInch, _wDenom);
	_wNumerY = MulDivQuick(_wNumerY, dzlInch, _wDenom);
	_wDenom = dzlInch;

	SimplifyFrt();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::RestrictFrtForPda
//
//  Synopsis:   Recalculate frt to be based on emus:
//              100% == FRT(dxpScreen, dypScreen, dzlInch)
//	
//----------------------------------------------------------------------------

void
CFrt::RestrictFrtForPda()
{
	// first simplify to reduce numbers
	SimplifyFrt();
		
	// however scary this looks, it is a simple scaling blown up to avoid 32-bit overflow
	__int64 i64NumerX = (_int64) _wNumerX * g_sizePixelsPerInch.cx;
	__int64 i64NumerY = (_int64) _wNumerY * g_sizePixelsPerInch.cy;
	__int64 i64Denom  = (_int64) _wDenom  * dzlInch;

	Assert(i64NumerX >= 0);
	Assert(i64NumerY >= 0);
	Assert(i64Denom > 0);

	// reduce FRT to 32-bit values. That's enough precision
	if (i64Denom > lwMaxInt)
		{
		int nDiv = i64Denom / lwMaxInt * 2;
		i64NumerX /= nDiv;
		i64NumerY /= nDiv;
		i64Denom  /= nDiv;
		}

	Assert(i64NumerX < lwMaxInt);
	Assert(i64NumerY < lwMaxInt);
	Assert(i64Denom < lwMaxInt);

	_wNumerX = i64NumerX;
	_wNumerY = i64NumerY;
	_wDenom  = i64Denom; 

	Assert(_wNumerX < lwMaxInt);
	Assert(_wDenom < lwMaxInt);

	// restrict to ppi
	RestrictFrt();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::GetDxpInchFromFrt
//
//  Synopsis:   Returns the X resolution per inch given a scaling fraction.
//	
//----------------------------------------------------------------------------

X
CFrt::GetDxpInchFromFrt()
{
	return (MulDiv(_wNumerX, dzlInch, _wDenom));
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::GetDypInchFromFrt
//
//  Synopsis:   Returns the Y resolution per inch given a scaling fraction.
//	
//----------------------------------------------------------------------------

Y
CFrt::GetDypInchFromFrt()
{
	return (MulDiv(_wNumerY, dzlInch, _wDenom));
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrt::GetFrt
//
//  Synopsis:   Returns the original, simplified scaling fraction, by applying
//              the opposite of RestrictFrtForPda().
//	
//----------------------------------------------------------------------------

void CFrt::GetFrt(int *pNumer, int *pDenom)
{
    CFrt    cfrtJunk;

    memset(&cfrtJunk, 0, sizeof(CFrt));

	__int64 i64NumerX = (_int64) _wNumerX * dzlInch;
	__int64 i64Denom  = (_int64) _wDenom  * g_sizePixelsPerInch.cx;

	Assert(i64NumerX >= 0);
	Assert(i64Denom > 0);

	// reduce FRT to 32-bit values. That's enough precision
	if (i64Denom > lwMaxInt)
		{
		int nDiv = i64Denom / lwMaxInt * 2;
		i64NumerX /= nDiv;
		i64Denom  /= nDiv;
		}

	Assert(i64NumerX < lwMaxInt);
	Assert(i64Denom < lwMaxInt);

    cfrtJunk.SetFrt(i64NumerX, i64Denom, TRUE, FALSE /* fRestrict */);

    if (pNumer)
        *pNumer = cfrtJunk.GetNumerX();

    if (pDenom)
        *pDenom = cfrtJunk.GetDenom();
}

#endif  // IE5_ZOOM
