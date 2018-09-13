//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       border.cxx
//
//  Contents:   Border helper implementation
//
//  Functions:  DrawBorder
//
//  History:    18-Jul-95   SumitC      Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

// BUGBUG: Temporary.  When Trident is part of the IE build, remove these and
// #include <winuserp.h> for these definitions
#ifndef BDR_OUTER
#define BDR_OUTER       0x0003
#endif
#ifndef BDR_INNER
#define BDR_INNER       0x000c
#endif
// global const definitions

const int HAIRLINE_IN_HIMETRICS = 26;
const int BORDEREFFECT_IN_HIMETRICS = 52;


//+-------------------------------------------------------------------------
//
//  Method:     DrawEdge2
//
//  Synopsis:   This routine is functionally equivalent to the Win95
//              DrawEdge API, except the following are not supported:
//                  BF_SOFT, BF_ADJUST, BF_FLAT, BF_MONO, BF_DIAG*
//              Also, colors to be used are passed in by the "c3d"
//              (ThreeDColors object) reference, rather than assuming
//              and being limited to the system colors.
//
//              If BF_MONO flag is specified, only the inner border is
//              drawn. This is used to draw flat scrollbars.
//
//--------------------------------------------------------------------------

void DrawEdge2(HDC hdc,
        LPRECT lprc,
        UINT edge,
        UINT flags,
        ThreeDColors & c3d,
        COLORREF colorBorder,
        UINT borderXWidth,
        UINT borderYWidth)
{
    COLORREF    colorTL;
    COLORREF    colorBR;
    RECT        rc;
    RECT        rc2;
    UINT        bdrMask;
    HBRUSH      hbrOld = NULL;
    COLORREF    crNow  = (COLORREF)0xFFFFFFFF;

    rc = *lprc;
    BOOL foutEffect = !(flags & BF_MONO); // No outer border if BF_MONO

    int XIn, XOut = 0, YIn, YOut = 0; //init XOut, YOut for retail build to pass

    if (foutEffect)
    {
        // border width can be odd number of pixels, so we divide it between
        // the inner and outer effect.

        XIn = borderXWidth / 2;
        XOut = borderXWidth - XIn;

        YIn = borderYWidth / 2;
        YOut = borderYWidth - YIn;
    }
    else
    {
        XIn = borderXWidth;
        YIn = borderYWidth;
    }

    Assert((BDR_OUTER == 0x0003) && (BDR_INNER == 0x000C));

    Assert(rc.left <= rc.right);
    Assert(rc.top <= rc.bottom);
    if (! (flags & BF_FLAT))
    {
        bdrMask = (foutEffect) ? BDR_OUTER : BDR_INNER;
        for (; bdrMask <= BDR_INNER; bdrMask <<= 2)
        {
            switch (edge & bdrMask)
            {
                case BDR_RAISEDOUTER:
                    colorTL = (flags & BF_SOFT) ? c3d.BtnHighLight() : c3d.BtnLight();
                    colorBR = c3d.BtnDkShadow();
                    break;

                case BDR_RAISEDINNER:
                    colorTL = (flags & BF_SOFT) ? c3d.BtnLight() : c3d.BtnHighLight();
                    colorBR = c3d.BtnShadow();
                    break;

                case BDR_SUNKENOUTER:
                    //fButton should be wndframe
                    colorTL = (flags & BF_SOFT) ? c3d.BtnDkShadow() : c3d.BtnShadow();
                    colorBR = c3d.BtnHighLight();
                    break;

                case BDR_SUNKENINNER:
                    if (flags & BF_MONO)
                    {
                        // inversion of BDR_RAISEDINNER
                        colorTL = c3d.BtnShadow();
                        colorBR = (flags & BF_SOFT) ? c3d.BtnLight() : c3d.BtnHighLight();
                    }
                    else
                    {
                        colorTL = (flags & BF_SOFT) ? c3d.BtnShadow() :  c3d.BtnDkShadow();
                        colorBR = c3d.BtnLight();
                    }
                    break;

                default:
                    return;
            }

            if (flags & (BF_RIGHT | BF_BOTTOM))
            {
                if (colorBR != crNow)
                {
                    HBRUSH hbrNew;
                    SelectCachedBrush(hdc, colorBR, &hbrNew, &hbrOld, &crNow);
                }

                if (flags & BF_RIGHT)
                {
                    rc2 = rc;
                    rc2.left = (rc.right -= foutEffect ? XOut : XIn);
                    if (rc2.left < rc.left)
                        rc2.left = rc.left;
                    PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                        rc2.bottom - rc2.top, PATCOPY);
                }

                if (flags & BF_BOTTOM)
                {
                    rc2 = rc;
                    rc2.top = (rc.bottom -= foutEffect ? YOut : YIn);
                    if (rc2.top < rc.top)
                        rc2.top = rc.top;
                    PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                        rc2.bottom - rc2.top, PATCOPY);
                }
            }

            if (flags & (BF_LEFT | BF_TOP))
            {
                if (colorTL != crNow)
                {
                    HBRUSH hbrNew;
                    SelectCachedBrush(hdc, colorTL, &hbrNew, &hbrOld, &crNow);
                }
    
                if (flags & BF_LEFT)
                {
                    rc2 = rc;
                    rc2.right = (rc.left += foutEffect ? XOut : XIn);
                    if (rc2.right > rc.right)
                        rc2.right = rc.right;
                    PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                        rc2.bottom - rc2.top, PATCOPY);
                }

                if (flags & BF_TOP)
                {
                    rc2 = rc;
                    rc2.bottom = (rc.top += foutEffect ? YOut : YIn);
                    if (rc2.bottom > rc.bottom)
                        rc2.bottom = rc.bottom;
                    PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                        rc2.bottom - rc2.top, PATCOPY);
                }
            }
            if (foutEffect)
                foutEffect = FALSE;
        }
    }
    else
    {
        if (colorBorder != crNow)
        {
            HBRUSH hbrNew;
            SelectCachedBrush(hdc, colorBorder, &hbrNew, &hbrOld, &crNow);
        }

        if (flags & BF_RIGHT)
        {
            rc2 = rc;
            rc2.left = (rc.right -= borderXWidth);
            if (rc2.left < rc.left)
                rc2.left = rc.left;
            PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                rc2.bottom - rc2.top, PATCOPY);
        }

        if (flags & BF_BOTTOM)
        {
            rc2 = rc;
            rc2.top = (rc.bottom -= borderYWidth);
            if (rc2.top < rc.top)
                rc2.top = rc.top;
            PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                rc2.bottom - rc2.top, PATCOPY);
        }

        if (flags & BF_LEFT)
        {
            rc2 = rc;
            rc2.right = (rc.left += borderXWidth);
            if (rc2.right > rc.right)
                rc2.right = rc.right;
            PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                rc2.bottom - rc2.top, PATCOPY);
        }

        if (flags & BF_TOP)
        {
            rc2 = rc;
            rc2.bottom = (rc.top += borderYWidth);
            if (rc2.bottom > rc.bottom)
                rc2.bottom = rc.bottom;
            PatBlt(hdc, rc2.left, rc2.top, rc2.right - rc2.left,
                rc2.bottom - rc2.top, PATCOPY);
        }
    }

    if (flags & BF_MIDDLE)
    {
        if (c3d.BtnFace() != crNow)
        {
            HBRUSH hbrNew;
            SelectCachedBrush(hdc, c3d.BtnFace(), &hbrNew, &hbrOld, &crNow);
        }
        PatBlt(hdc, rc.left, rc.top, rc.right - rc.left,
            rc.bottom - rc.top, PATCOPY);
    }

    if (hbrOld)
        ReleaseCachedBrush((HBRUSH)SelectObject(hdc, hbrOld));
}

//+---------------------------------------------------------------------------
//
//  Function:   BRGetBorderWidth
//
//  Synopsis:   Return the border size in himetrics given border attributes
//
//  Arguments:  [BorderStyle] -- borderstyle
//              [Effect]      -- border effect
//
//  Returns:    int          -- -1 is an error value
//
//  History:    9-13-95 created [gideons]
//
//----------------------------------------------------------------------------

int
BRGetBorderWidth( fmBorderStyle BorderStyle )
{
    int uBorderWidth;

    // count for the beveled edge

    if (BorderStyle > fmBorderStyleSingle)
    {
        uBorderWidth = BORDEREFFECT_IN_HIMETRICS;
    }
    else // if no effect count for a single (for now) border line
    if (BorderStyle == fmBorderStyleSingle)
    {
        uBorderWidth = HAIRLINE_IN_HIMETRICS;
    }
    else
    if (BorderStyle == fmBorderStyleNone)
    {
        uBorderWidth = 0;
    }
    else
    {
        uBorderWidth = -1;
    }

    return uBorderWidth;
}






//+---------------------------------------------------------------------------
//
//  Function:   BRAdjustRectForBorder
//
//  Synopsis:   adjusts a given rectangle's client area given border attributes
//              taking zooming into account
//
//  Arguments:  [pDI]         -- CDrawInfo
//              [prcl]        -- rectangle to adjust
//              [BorderStyle] -- borderstyle
//              [Effect]      -- border effect
//
//  Returns:    HRESULT
//
//  History:    05-Sep-95   SumitC      Created
//              09-12-95    gideons     modified to support zooming
//
//----------------------------------------------------------------------------


HRESULT
BRAdjustRectForBorderActual(
        CTransform *    pTransform,
        RECT *          prc,
        fmBorderStyle   BorderStyle,
        BOOL            fInflateForBorder)
{
    int uInflateXBy, uInflateYBy;
    UINT inflateUnit;

    Assert(prc);

    // count for the beveled edge

    if (BorderStyle > fmBorderStyleSingle)
    {
        // inflateUnit is 52 HiMetrics
        inflateUnit =  BORDEREFFECT_IN_HIMETRICS;
    }
    else // if no effect count for a single (for now) border line
    if (BorderStyle == fmBorderStyleSingle)
    {
        // inflateUnit = 26 HiMetrics.
        inflateUnit = HAIRLINE_IN_HIMETRICS;
    }
    else
        goto Cleanup;

    // compute the actually border dimantions
    uInflateXBy =  pTransform->WindowFromDocumentCX(inflateUnit);
          //MulDivQuick(inflateUnit, prcZ->right - prcZ->left, psizelZoom->cx);
    uInflateYBy =  pTransform->WindowFromDocumentCY(inflateUnit);
          //MulDivQuick(inflateUnit, prcZ->bottom - prcZ->top, psizelZoom->cy);

    // Border width can't go below 1 pixel
    if (uInflateXBy == 0)
       uInflateXBy = 1;

    if (uInflateYBy == 0)
        uInflateYBy = 1;

    // Compute border size with zooming.
    if (fInflateForBorder)
        InflateRect((RECT *)prc, uInflateXBy, uInflateYBy );
    else
        InflateRect((RECT *)prc, -uInflateXBy, -uInflateYBy );

   if (prc->left > prc->right)
        prc->left = prc->right;
    if (prc->top > prc->bottom)
        prc->top = prc->bottom;

Cleanup:
    return S_OK;
}

HRESULT
BRAdjustRectForBorder(
        CTransform *     pTransform,
        RECT *          prc,
        fmBorderStyle   BorderStyle)
{
    return BRAdjustRectForBorderActual(
                    pTransform,
                    prc,
                    BorderStyle,
                    FALSE);
}

HRESULT
BRAdjustRectForBorderRev(
        CTransform *     pTransform,
        RECT *          prc,
        fmBorderStyle   BorderStyle)
{
    return BRAdjustRectForBorderActual(
                    pTransform,
                    prc,
                    BorderStyle,
                    TRUE);
}






//+---------------------------------------------------------------------------
//
//  Function:   BRAdjustRectlForBorder
//
//  Synopsis:   adjusts a given rectangle's client area given border attributes
//              WITHOUT taking zooming into account
//
//  Arguments:  [prcl]        -- rectangle to adjust
//              [BorderStyle] -- borderstyle
//              [Effect]      -- border effect
//
//  Returns:    HRESULT
//
//  History:    05-Sep-95   SumitC      Created
//              09-12-95    gideons     modified to support zooming
//
//----------------------------------------------------------------------------

HRESULT
BRAdjustRectlForBorder
(
        RECTL * prcl,
        fmBorderStyle BorderStyle)
{

    if (BorderStyle > fmBorderStyleSingle)
    {
        // Compute border size with zooming.

        InflateRect((RECT *)prcl,
                    -BORDEREFFECT_IN_HIMETRICS, -BORDEREFFECT_IN_HIMETRICS);
    }
    else // if no effect count for a single (for now) border line
    if (BorderStyle == fmBorderStyleSingle)
    {
        // Compute border size with zooming.
        InflateRect((RECT *)prcl,
                    -HAIRLINE_IN_HIMETRICS, -HAIRLINE_IN_HIMETRICS );
    }


    return S_OK;
}





//+---------------------------------------------------------------------------
//
//  Function:   BRAdjustSizelForBorder
//
//  Synopsis:   adjusts a given rectangle's client area given border attributes
//              WITHOUT taking zooming into account
//
//  Arguments:  [psizel]      -- sizel to adjust
//              [BorderStyle] -- borderstyle
//              [Effect]      -- border effect
//              [fSubtractAdd]-- add or subtract borders from the size
//
//  Returns:    HRESULT
//
//  History:    09-19-95    gideons     created
//
//----------------------------------------------------------------------------

HRESULT
BRAdjustSizelForBorder
(
        SIZEL * psizel,
        fmBorderStyle BorderStyle,
        BOOL fSubtractAdd)
{

    int iSubtractAdd = fSubtractAdd? 1: -1;

    if (BorderStyle > fmBorderStyleSingle)
    {
        psizel->cx += iSubtractAdd * 2 * BORDEREFFECT_IN_HIMETRICS;
        psizel->cy += iSubtractAdd * 2 * BORDEREFFECT_IN_HIMETRICS;
    }
    else // if no effect count for a single (for now) border line
    if (BorderStyle == fmBorderStyleSingle)
    {
        psizel->cx += iSubtractAdd * 2 * HAIRLINE_IN_HIMETRICS;
        psizel->cy += iSubtractAdd * 2 * HAIRLINE_IN_HIMETRICS;
    }

    // Don't allow a negative size
    psizel->cx = max(psizel->cx, 0L);
    psizel->cy = max(psizel->cy, 0L);

    return S_OK;
}




void DrawEdge2(HDC hdc,
        LPRECT lprc,
        UINT edge,
        UINT flags,
        ThreeDColors & c3d,
        COLORREF colorBorder,
        UINT borderXWidth,
        UINT borderYWidth);






//+---------------------------------------------------------------------------
//
//  Function:   BRDrawBorder
//
//  Synopsis:   Draws a border for a Forms3 control, or the form
//
//  Arguments:  [pDI]         -- [in] CDrawInfo *
//              [prc]         -- [in,out] rect in pixels
//              [BorderStyle] -- border style
//              [Effect]      -- border effect
//              [colorBorder] -- color of Border
//              [dwFlags]     -- BRFLAGS_BUTTON: if set, use "hard" edges.  usually use "soft"
//                               BRFLAGS_ADJUSTRECT: if set, adjust the rect to reflect the
//                               border drawn.
//                               BRFLAGS_DEFAULT: if set draw default button retangle around
//                               drawing rect and insert everything else accordingly
//
//  Returns:    HRESULT
//
//  History:    05-Sep-95   SumitC      Created
//              09-12-95    gideons     modified to support zooming
//
//----------------------------------------------------------------------------


HRESULT
BRDrawBorder(
        CDrawInfo *pDI,
        RECT * prc,
        fmBorderStyle BorderStyle,
        COLORREF colorBorder,
        ThreeDColors * peffectColor,
        DWORD dwFlags)
{
    ThreeDColors    *ptdc;
    ThreeDColors    tdc;
    UINT            uBdrXWidth;
    UINT            uBdrYWidth;
    UINT            uEdgeStyle = 0, uEdgeFlags = 0;
    UINT            borderUnit;


    if (peffectColor == NULL)
    {
        ptdc = &tdc; // get the default colors
    }
    else
    {
        ptdc = peffectColor;
    }

    Assert(ptdc != NULL);
    Assert(pDI->_hdc);


    if(dwFlags & BRFLAGS_DEFAULT)
    {
        uBdrYWidth = pDI->WindowFromDocumentCY(HAIRLINE_IN_HIMETRICS);
        uBdrXWidth = pDI->WindowFromDocumentCX(HAIRLINE_IN_HIMETRICS);

        DrawEdge2(pDI->_hdc,
                  (RECT *)prc,
                  0,
                  BF_FLAT | BF_RECT,
                  *ptdc,
                  colorBorder,
                  uBdrXWidth,
                  uBdrYWidth);

        BRAdjustRectForBorder(pDI,
                              (RECT *) prc,
                              fmBorderStyleSingle);
    }

    // draw the beveled edge
    if (BorderStyle > fmBorderStyleSingle)
    {
        borderUnit =  (dwFlags & BRFLAGS_MONO) ?
                        HAIRLINE_IN_HIMETRICS :
                        BORDEREFFECT_IN_HIMETRICS;

        switch (BorderStyle)
        {
        case fmBorderStyleRaised:
            uEdgeStyle = EDGE_RAISED;
            break;

        case fmBorderStyleSunken:
            uEdgeStyle = EDGE_SUNKEN;
            break;

        case fmBorderStyleEtched:
            uEdgeStyle = EDGE_ETCHED;
            break;

        case fmBorderStyleBump:
            uEdgeStyle = EDGE_BUMP;
            break;

        default:
            Assert(0 && "CBorderHelper::Render, illegal effect value");
        }

        // Draw the border of the control

        uEdgeFlags = BF_RECT | ((dwFlags & BRFLAGS_BUTTON) ? BF_SOFT : 0);
        if (dwFlags & BRFLAGS_MONO)
        {
            uEdgeFlags |= BF_MONO;
        }
    }
    else if (BorderStyle == fmBorderStyleSingle)
    {
        borderUnit = HAIRLINE_IN_HIMETRICS;

        uEdgeFlags = BF_FLAT | BF_RECT;
    }
    else
        goto cleanup;   // nothing to do


    uBdrYWidth = pDI->WindowFromDocumentCY(borderUnit);
    uBdrXWidth = pDI->WindowFromDocumentCX(borderUnit);

    // border width can not go below 1 pixel
    if (uBdrYWidth == 0)
        uBdrYWidth = 1;

    if (uBdrXWidth == 0)
        uBdrXWidth = 1;

   //Draw the border
   //BUGBUG DrawEdge2  currently checks for underflow of prc
    DrawEdge2(pDI->_hdc,
              (RECT *)prc,
              uEdgeStyle,
              uEdgeFlags,
              *ptdc,
              colorBorder,
              uBdrXWidth,
              uBdrYWidth);

    if (dwFlags & BRFLAGS_ADJUSTRECT)
    {
        BRAdjustRectForBorder( pDI, prc, BorderStyle);
    }

cleanup:

    return S_OK;
}
