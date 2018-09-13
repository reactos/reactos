/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    color2.c

Abstract:

    This module implements the support for the Win32 color dialog.

Revision History:

--*/



//
//  Include Files.
//

#include "windows.h"
#include <port1632.h>
#include "privcomd.h"
#include "color.h"





////////////////////////////////////////////////////////////////////////////
//
//  ChangeColorSettings
//
//  Updates color shown.
//
////////////////////////////////////////////////////////////////////////////

VOID ChangeColorSettings(
    register PCOLORINFO pCI)
{
    register HDC hDC;
    HWND hDlg = pCI->hDialog;
    DWORD dwRGBcolor = pCI->currentRGB;

    RGBtoHLS(dwRGBcolor);
    if (L != pCI->currentLum)
    {
        hDC = GetDC(hDlg);
        EraseLumArrow(hDC, pCI);
        pCI->currentLum = L;
        HLStoHLSPos(COLOR_LUM, pCI);
        LumArrowPaint(hDC, pCI->nLumPos, pCI);
        ReleaseDC(hDlg, hDC);
    }
    if ((H != pCI->currentHue) || (S != pCI->currentSat))
    {
        pCI->currentHue = H;
        pCI->currentSat = S;
        InvalidateRect(hDlg, (LPRECT)&pCI->rLumPaint, FALSE);
        hDC = GetDC(hDlg);
        EraseCrossHair(hDC, pCI);
        HLStoHLSPos(COLOR_HUE, pCI);
        HLStoHLSPos(COLOR_SAT, pCI);
        CrossHairPaint(hDC, pCI->nHuePos, pCI->nSatPos, pCI);
        ReleaseDC(hDlg, hDC);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  LumArrowPaint
//
////////////////////////////////////////////////////////////////////////////

VOID LumArrowPaint(
    HDC hDC,
    SHORT y,
    PCOLORINFO pCI)
{
    HPEN hPen;
    HBRUSH hBrush;
    POINT Triang[3];

    Triang[0].x = pCI->rLumScroll.left + 2;
    Triang[0].y = pCI->nLumPos;
    Triang[1].x = Triang[2].x = pCI->rLumScroll.right - 1;
    Triang[1].y = pCI->nLumPos - (cyCaption >> 2);
    Triang[2].y = pCI->nLumPos + (cyCaption >> 2);

    hPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
    hBrush = SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    Polygon(hDC, (LPPOINT)Triang, 3);
    SelectObject(hDC, hPen);
    SelectObject(hDC, hBrush);

    return;
    y;
}


////////////////////////////////////////////////////////////////////////////
//
//  EraseLumArrow
//
////////////////////////////////////////////////////////////////////////////

VOID EraseLumArrow(
    HDC hDC,
    PCOLORINFO pCI)
{
    HBRUSH hBrush;
    RECT Rect;

    hBrush = CreateSolidBrush(GetNearestColor(hDC, rgbClient));
    Rect.left = pCI->rLumScroll.left + 1;
    Rect.top = pCI->nLumPos - (cyCaption >> 2) - 1;
    Rect.right = pCI->rLumScroll.right;
    Rect.bottom = pCI->nLumPos + (cyCaption >> 2) + 1;
    FillRect(hDC, (LPRECT)&Rect, hBrush);
    DeleteObject(hBrush);
}


////////////////////////////////////////////////////////////////////////////
//
//  EraseCrossHair
//
////////////////////////////////////////////////////////////////////////////

VOID EraseCrossHair(
    HDC hDC,
    PCOLORINFO pCI)
{
    HBITMAP hOldBitmap;
    WORD distancex, distancey;
    WORD topy, bottomy, leftx, rightx;
    RECT rRainbow;

    CopyRect(&rRainbow, &pCI->rRainbow);

    distancex = (WORD)(10 * cxBorder);
    distancey = (WORD)(10 * cyBorder);
    topy    = ((WORD)rRainbow.top > pCI->nSatPos - distancey)
                  ? (WORD)rRainbow.top
                  : pCI->nSatPos - distancey;
    bottomy = ((WORD)rRainbow.bottom < pCI->nSatPos + distancey)
                  ? (WORD)rRainbow.bottom
                  : pCI->nSatPos + distancey;
    leftx   = ((WORD)rRainbow.left > pCI->nHuePos - distancex)
                  ? (WORD)rRainbow.left
                  : pCI->nHuePos - distancex;
    rightx  = ((WORD)rRainbow.right < pCI->nHuePos + distancex)
                  ? (WORD)rRainbow.right
                  : pCI->nHuePos + distancex;

    hOldBitmap = SelectObject(hDCFastBlt, hRainbowBitmap);
    BitBlt( hDC,
            leftx,
            topy,
            rightx - leftx,
            bottomy - topy,
            hDCFastBlt,
            leftx - (WORD)rRainbow.left,
            topy - (WORD)rRainbow.top,
            SRCCOPY );
    SelectObject(hDCFastBlt, hOldBitmap);
}


////////////////////////////////////////////////////////////////////////////
//
//  CrossHairPaint
//
////////////////////////////////////////////////////////////////////////////

VOID CrossHairPaint(
    register HDC hDC,
    SHORT x,
    SHORT y,
    PCOLORINFO pCI)
{
    SHORT distancex, distancey;
    SHORT topy, bottomy, topy2, bottomy2;
    SHORT leftx, rightx, leftx2, rightx2;
    RECT rRainbow;

    CopyRect(&rRainbow, &pCI->rRainbow);
    distancex = (SHORT)(5 * cxBorder);
    distancey = (SHORT)(5 * cyBorder);
    topy     = (SHORT)((rRainbow.top > y - 2 * distancey)
                         ? rRainbow.top
                         : y - 2 * distancey);
    bottomy  = (SHORT)((rRainbow.bottom < y + 2 * distancey)
                         ? rRainbow.bottom
                         : y + 2 * distancey);
    leftx    = (SHORT)((rRainbow.left > x - 2 * distancex)
                         ? rRainbow.left
                         : x - 2 * distancex);
    rightx   = (SHORT)((rRainbow.right < x + 2 * distancex)
                         ? rRainbow.right
                         : x + 2 * distancex);
    topy2    = (SHORT)((rRainbow.top > y - distancey)
                         ? rRainbow.top
                         : y - distancey);
    bottomy2 = (SHORT)((rRainbow.bottom < y + distancey)
                         ? rRainbow.bottom
                         : y + distancey);
    leftx2 = (SHORT)((rRainbow.left > x - distancex)
                         ? rRainbow.left
                         : x - distancex);
    rightx2 = (SHORT)((rRainbow.right < x + distancex)
                         ? rRainbow.right
                         : x + distancex);
    if (rRainbow.top < topy2)
    {
        if ((x - 1) >= rRainbow.left)
        {
            MMoveTo(hDC, x - 1, topy2);
            LineTo(hDC, x - 1, topy);
        }
        if ((int)x < rRainbow.right)
        {
            MMoveTo(hDC, x, topy2);
            LineTo(hDC, x, topy);
        }
        if ((x + 1) < rRainbow.right)
        {
            MMoveTo(hDC, x + 1, topy2);
            LineTo(hDC, x + 1, topy);
        }
    }
    if (rRainbow.bottom > bottomy2)
    {
        if ((x - 1) >= rRainbow.left)
        {
            MMoveTo(hDC, x - 1, bottomy2);
            LineTo(hDC, x - 1, bottomy);
        }
        if ((int)x < rRainbow.right)
        {
            MMoveTo(hDC, x, bottomy2);
            LineTo(hDC, x, bottomy);
        }
        if ((x + 1) < rRainbow.right)
        {
            MMoveTo(hDC, x + 1, bottomy2);
            LineTo(hDC, x + 1, bottomy);
        }
    }
    if (rRainbow.left < leftx2)
    {
        if ((y - 1) >= rRainbow.top)
        {
            MMoveTo(hDC, leftx2, y - 1);
            LineTo(hDC, leftx, y - 1);
        }
        if ((int)y < rRainbow.bottom)
        {
            MMoveTo(hDC, leftx2, y);
            LineTo(hDC, leftx, y);
        }
        if ((y + 1) < rRainbow.bottom)
        {
            MMoveTo(hDC, leftx2, y + 1);
            LineTo(hDC, leftx, y + 1);
        }
    }
    if (rRainbow.right > rightx2)
    {
        if ((y - 1) >= rRainbow.top)
        {
            MMoveTo(hDC, rightx2, y - 1);
            LineTo(hDC, rightx, y - 1);
        }
        if ((int)y < rRainbow.bottom)
        {
            MMoveTo(hDC, rightx2, y);
            LineTo(hDC, rightx, y);
        }
        if ((y + 1) < rRainbow.bottom)
        {
            MMoveTo(hDC, rightx2, y + 1);
            LineTo(hDC, rightx, y + 1);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  NearestSolid
//
////////////////////////////////////////////////////////////////////////////

VOID NearestSolid(
    register PCOLORINFO pCI)
{
    register HDC hDC;
    HWND hDlg = pCI->hDialog;

    hDC = GetDC(hDlg);
    EraseCrossHair(hDC, pCI);
    EraseLumArrow(hDC, pCI);
    RGBtoHLS(pCI->currentRGB = GetNearestColor(hDC, pCI->currentRGB));
    pCI->currentHue = H;
    pCI->currentLum = L;
    pCI->currentSat = S;
    HLStoHLSPos(0, pCI);
    CrossHairPaint(hDC, pCI->nHuePos, pCI->nSatPos, pCI);
    LumArrowPaint(hDC, pCI->nLumPos, pCI);
    ReleaseDC(hDlg, hDC);
    SetHLSEdit(0, pCI);
    SetRGBEdit(0, pCI);
    InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
    InvalidateRect(hDlg, (LPRECT)&pCI->rLumPaint, FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  HLSPostoHLS
//
////////////////////////////////////////////////////////////////////////////

VOID HLSPostoHLS(
    SHORT nHLSEdit,
    register PCOLORINFO pCI)
{
    switch (nHLSEdit)
    {
        case COLOR_HUE:
        {
            pCI->currentHue = (WORD)((pCI->nHuePos - pCI->rRainbow.left) *
                                     (RANGE - 1) / (pCI->nHueWidth - 1));
            break;
        }
        case COLOR_SAT:
        {
            pCI->currentSat = (WORD)(RANGE -
                                     (pCI->nSatPos - pCI->rRainbow.top) *
                                     RANGE / (pCI->nSatHeight - 1));
            break;
        }
        case COLOR_LUM:
        {
            pCI->currentLum = (WORD)(RANGE -
                                     (pCI->nLumPos - pCI->rLumPaint.top) *
                                     RANGE / (pCI->nLumHeight - 1));
            break;
        }
        default:
        {
            pCI->currentHue = (WORD)((pCI->nHuePos - pCI->rRainbow.left) *
                                     (RANGE - 1) / pCI->nHueWidth);
            pCI->currentSat = (WORD)(RANGE -
                                     (pCI->nSatPos - pCI->rRainbow.top) *
                                     RANGE / pCI->nSatHeight);
            pCI->currentLum = (WORD)(RANGE -
                                     (pCI->nLumPos - pCI->rLumPaint.top) *
                                     RANGE / pCI->nLumHeight);
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HLStoHLSPos
//
////////////////////////////////////////////////////////////////////////////

VOID HLStoHLSPos(
    SHORT nHLSEdit,
    register PCOLORINFO pCI)
{
    switch (nHLSEdit)
    {
        case ( COLOR_HUE ) :
        {
            pCI->nHuePos = (WORD)(pCI->rRainbow.left + pCI->currentHue *
                                  pCI->nHueWidth / (RANGE - 1));
            break;
        }
        case COLOR_SAT:
        {
            pCI->nSatPos = (WORD)(pCI->rRainbow.top +
                                  (RANGE - pCI->currentSat) *
                                  (pCI->nSatHeight - 1) / RANGE);
            break;
        }
        case COLOR_LUM:
        {
            pCI->nLumPos = (WORD)(pCI->rLumPaint.top +
                                  (RANGE - pCI->currentLum) *
                                  (pCI->nLumHeight - 1) / RANGE);
            break;
        }
        default:
        {
            pCI->nHuePos = (WORD)(pCI->rRainbow.left + pCI->currentHue *
                                  pCI->nHueWidth / (RANGE - 1));
            pCI->nSatPos = (WORD)(pCI->rRainbow.top +
                                  (RANGE - pCI->currentSat) *
                                  (pCI->nSatHeight - 1) / RANGE);
            pCI->nLumPos = (WORD)(pCI->rLumPaint.top +
                                  (RANGE - pCI->currentLum) *
                                  (pCI->nLumHeight - 1) / RANGE);
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SetHLSEdit
//
////////////////////////////////////////////////////////////////////////////

VOID SetHLSEdit(
    SHORT nHLSEdit,
    register PCOLORINFO pCI)
{
    register HWND hRainbowDlg = pCI->hDialog;

    switch (nHLSEdit)
    {
        case ( COLOR_HUE ) :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_HUE, pCI->currentHue, FALSE);
            break;
        }
        case ( COLOR_SAT ) :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_SAT, pCI->currentSat, FALSE);
            break;
        }
        case ( COLOR_LUM ) :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_LUM, pCI->currentLum, FALSE);
            break;
        }
        default :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_HUE, pCI->currentHue, FALSE);
            SetDlgItemInt(hRainbowDlg, COLOR_SAT, pCI->currentSat, FALSE);
            SetDlgItemInt(hRainbowDlg, COLOR_LUM, pCI->currentLum, FALSE);
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SetRGBEdit
//
////////////////////////////////////////////////////////////////////////////

VOID SetRGBEdit(
    SHORT nRGBEdit,
    PCOLORINFO pCI)
{
    register HWND hRainbowDlg = pCI->hDialog;
    DWORD rainbowRGB = pCI->currentRGB;

    switch (nRGBEdit)
    {
        case ( COLOR_RED ) :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_RED, GetRValue(rainbowRGB), FALSE);
            break;
        }
        case ( COLOR_GREEN ) :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_GREEN, GetGValue(rainbowRGB), FALSE);
            break;
        }
        case ( COLOR_BLUE ) :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_BLUE, GetBValue(rainbowRGB), FALSE);
            break;
        }
        default :
        {
            SetDlgItemInt(hRainbowDlg, COLOR_RED, GetRValue(rainbowRGB), FALSE);
            SetDlgItemInt(hRainbowDlg, COLOR_GREEN, GetGValue(rainbowRGB), FALSE);
            SetDlgItemInt(hRainbowDlg, COLOR_BLUE, GetBValue(rainbowRGB), FALSE);
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InitRainbow
//
//  Returns TRUE iff we make it.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitRainbow(
    register PCOLORINFO pCI)
{
    HDC hDC;
    WORD Sat, Hue;
    HBITMAP hOldBitmap;
    RECT Rect;
    HBRUSH hbrSwipe, hbrTemp;
    WORD nHueWidth, nSatHeight;
    register HWND hRainbowDlg = pCI->hDialog;
    ULONG ulLimit;

    RGBtoHLS(pCI->currentRGB);

    SetupRainbowCapture(pCI);

    nHueWidth = pCI->nHueWidth = (WORD)(pCI->rRainbow.right -
                                        pCI->rRainbow.left);
    nSatHeight = pCI->nSatHeight = (WORD)(pCI->rRainbow.bottom -
                                          pCI->rRainbow.top);

    pCI->currentHue = H;
    pCI->currentSat = S;
    pCI->currentLum = L;

    HLStoHLSPos(0, pCI);
    SetRGBEdit(0, pCI);
    SetHLSEdit(0, pCI);

    if (!hRainbowBitmap)
    {
        hDC = GetDC(hRainbowDlg);
        hRainbowBitmap = CreateCompatibleBitmap(hDC, nHueWidth, nSatHeight);
        if (!hRainbowBitmap)
        {
            return (FALSE);
        }
    }

    hOldBitmap = SelectObject(hDCFastBlt, hRainbowBitmap);

    //
    //  NOTE: The final pass through this loop paints on and past the end
    //        of the selected bitmap.  Windows is a good product, and doesn't
    //        let such foolishness happen.
    //
    ulLimit = GdiSetBatchLimit(1000);
    hbrSwipe = CreateHatchBrush(HS_DITHEREDTEXTCLR, 0);
    hbrTemp = SelectObject(hDCFastBlt,hbrSwipe);

    Rect.bottom = 0;

    for (Sat = RANGE; Sat > 0; Sat -= SATINC)
    {
        Rect.top = Rect.bottom;
        Rect.bottom = (nSatHeight * RANGE - (Sat - SATINC) * nSatHeight) / RANGE;
        Rect.right = 0;

        for (Hue = 0; Hue < (RANGE - 1); Hue += HUEINC)
        {
            Rect.left = Rect.right;
            Rect.right = ((Hue + HUEINC) * nHueWidth) / RANGE;
            SetTextColor(hDCFastBlt, HLStoRGB(Hue, RANGE / 2, Sat));
            PatBlt( hDCFastBlt,
                    Rect.left,
                    Rect.top,
                    Rect.right - Rect.left,
                    Rect.bottom - Rect.top,
                    PATCOPY );
        }
    }

    SelectObject(hDCFastBlt, hbrTemp);
    DeleteObject(hbrSwipe);
    GdiSetBatchLimit(ulLimit);

    SelectObject(hDCFastBlt, hOldBitmap);
    ReleaseDC(hRainbowDlg, hDC);

    UpdateWindow(hRainbowDlg);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PaintRainbow
//
////////////////////////////////////////////////////////////////////////////

VOID PaintRainbow(
    HDC hDC,
    LPRECT lpRect,
    register PCOLORINFO pCI)
{
    HBITMAP hOldBitmap;

    if (!hRainbowBitmap)
    {
        return;
    }
    hOldBitmap = SelectObject(hDCFastBlt, hRainbowBitmap);
    BitBlt( hDC,
            lpRect->left,
            lpRect->top,
            lpRect->right - lpRect->left,
            lpRect->bottom - lpRect->top,
            hDCFastBlt,
            lpRect->left - pCI->rRainbow.left,
            lpRect->top - pCI->rRainbow.top,
            SRCCOPY );
    SelectObject(hDCFastBlt, hOldBitmap);
    CrossHairPaint(hDC, pCI->nHuePos, pCI->nSatPos, pCI);
    UpdateWindow(pCI->hDialog);
}


////////////////////////////////////////////////////////////////////////////
//
//  RainbowPaint
//
////////////////////////////////////////////////////////////////////////////

void RainbowPaint(
    register PCOLORINFO pCI,
    HDC hDC,
    LPRECT lpPaintRect)
{
    WORD Lum;
    RECT Rect;
    HBRUSH hbrSwipe;

    //
    //  Paint the Current Color Sample.
    //
    if (IntersectRect((LPRECT)&Rect, lpPaintRect, (LPRECT)&(pCI->rCurrentColor)))
    {
        hbrSwipe = CreateSolidBrush(pCI->currentRGB);
        FillRect(hDC, (LPRECT)&Rect, hbrSwipe);
        DeleteObject(hbrSwipe);
    }

    //
    //  Paint the Nearest Pure Color Sample.
    //
    if (IntersectRect((LPRECT)&Rect, lpPaintRect, (LPRECT)&(pCI->rNearestPure)))
    {
        hbrSwipe = CreateSolidBrush(GetNearestColor(hDC, pCI->currentRGB));
        FillRect(hDC, (LPRECT)&Rect, hbrSwipe);
        DeleteObject(hbrSwipe);
    }

    //
    //  Paint the Luminosity Range.
    //
    if (IntersectRect((LPRECT)&Rect, lpPaintRect, (LPRECT)&(pCI->rLumPaint)))
    {
        Rect.left = pCI->rLumPaint.left;
        Rect.right = pCI->rLumPaint.right;
        Rect.top = pCI->rLumPaint.bottom - LUMINC / 2;
        Rect.bottom = pCI->rLumPaint.bottom;
        hbrSwipe = CreateSolidBrush(HLStoRGB( pCI->currentHue,
                                              0,
                                              pCI->currentSat ));
        FillRect(hDC, (LPRECT)&Rect, hbrSwipe);
        DeleteObject(hbrSwipe);
        for (Lum = LUMINC; Lum < RANGE; Lum += LUMINC)
        {
            Rect.bottom = Rect.top;
            Rect.top = (((pCI->rLumPaint.bottom + LUMINC / 2) * (DWORD)RANGE -
                         (Lum + LUMINC) * pCI->nLumHeight) / RANGE);
            hbrSwipe = CreateSolidBrush(HLStoRGB( pCI->currentHue,
                                                  Lum,
                                                  pCI->currentSat ));
            FillRect(hDC, (LPRECT)&Rect, hbrSwipe);
            DeleteObject(hbrSwipe);
        }
        Rect.bottom = Rect.top;
        Rect.top = pCI->rLumPaint.top;
        hbrSwipe = CreateSolidBrush(HLStoRGB( pCI->currentHue,
                                              RANGE,
                                              pCI->currentSat ));
        FillRect(hDC, (LPRECT)&Rect, hbrSwipe);
        DeleteObject(hbrSwipe);

        //
        //  Paint the bounding rectangle only when it might be necessary.
        //
        if (!EqualRect(lpPaintRect, (LPRECT)&pCI->rLumPaint))
        {
            hbrSwipe = SelectObject(hDC, GetStockObject(NULL_BRUSH));
            Rectangle( hDC,
                       pCI->rLumPaint.left - 1,
                       pCI->rLumPaint.top - 1,
                       pCI->rLumPaint.right + 1,
                       pCI->rLumPaint.bottom + 1 );
            SelectObject(hDC, hbrSwipe);
        }
    }

    //
    //  Paint the Luminosity Arrow.
    //
    if (IntersectRect((LPRECT)&Rect, lpPaintRect, (LPRECT)&pCI->rLumScroll))
    {
        LumArrowPaint(hDC, pCI->nLumPos, pCI);
    }

    if (IntersectRect((LPRECT)&Rect, lpPaintRect, (LPRECT)&pCI->rRainbow))
    {
        PaintRainbow(hDC, (LPRECT)&Rect, pCI);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Color conversion routines --
//
//  RGBtoHLS() takes a DWORD RGB value, translates it to HLS, and stores the
//  results in the global vars H, L, and S.  HLStoRGB takes the current values
//  of H, L, and S and returns the equivalent value in an RGB DWORD.  The vars
//  H, L and S are written to only by 1) RGBtoHLS (initialization) or 2) the
//  scrollbar handlers.
//
//  A point of reference for the algorithms is Foley and Van Dam, pp. 618-19.
//  Their algorithm is in floating point.
//
//  There are potential roundoff errors lurking throughout here.
//     (0.5 + x/y) without floating point,
//     (x / y) phrased ((x + (y / 2)) / y) yields very small roundoff error.
//  This makes many of the following divisions look funny.
//
//
//  H,L, and S vary over 0 - HLSMAX.
//  R,G, and B vary over 0 - RGBMAX.
//  HLSMAX BEST IF DIVISIBLE BY 6.
//  RGBMAX, HLSMAX must each fit in a byte.
//
//  Hue is undefined if Saturation is 0 (grey-scale).
//  This value determines where the Hue scrollbar is initially set for
//  achromatic colors.
//
////////////////////////////////////////////////////////////////////////////

#define UNDEFINED (HLSMAX * 2 / 3)


////////////////////////////////////////////////////////////////////////////
//
//  RGBtoHLS
//
////////////////////////////////////////////////////////////////////////////

VOID RGBtoHLS(
    DWORD lRGBColor)
{
    WORD R, G, B;                 // input RGB values
    WORD cMax,cMin;               // max and min RGB values
    WORD cSum,cDif;
    SHORT Rdelta, Gdelta, Bdelta; // intermediate value: % of spread from max

    //
    //  get R, G, and B out of DWORD.
    //
    R = GetRValue(lRGBColor);
    G = GetGValue(lRGBColor);
    B = GetBValue(lRGBColor);

    //
    //  Calculate lightness.
    //
    cMax = max(max(R, G), B);
    cMin = min(min(R, G), B);
    cSum = cMax + cMin;
    L = (WORD)(((cSum * (DWORD)HLSMAX) + RGBMAX) / (2 * RGBMAX));

    cDif = cMax - cMin;
    if (!cDif)
    {
        //
        //  r = g = b --> Achromatic case.
        //
        S = 0;                         // saturation
        H = UNDEFINED;                 // hue
    }
    else
    {
        //
        //  Chromatic case.
        //

        //
        //  Saturation.
        //
        //  Note: Division by cSum is not a problem, as cSum can only
        //        be 0 if the RGB value is 0L, and that is achromatic.
        //
        if (L <= (HLSMAX / 2))
        {
            S = (WORD)(((cDif * (DWORD) HLSMAX) + (cSum / 2) ) / cSum);
        }
        else
        {
            S = (WORD)((DWORD)((cDif * (DWORD)HLSMAX) +
                               (DWORD)((2 * RGBMAX - cSum) / 2)) /
                       (2 * RGBMAX - cSum));
        }

        //
        //  Hue.
        //
        Rdelta = (SHORT)((((cMax - R) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);
        Gdelta = (SHORT)((((cMax - G) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);
        Bdelta = (SHORT)((((cMax - B) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);

        if (R == cMax)
        {
            H = Bdelta - Gdelta;
        }
        else if (G == cMax)
        {
            H = (WORD)((HLSMAX / 3) + Rdelta - Bdelta);
        }
        else  // (B == cMax)
        {
            H = (WORD)(((2 * HLSMAX) / 3) + Gdelta - Rdelta);
        }

        if ((short)H < 0)
        {
            //
            //  This can occur when R == cMax and G is > B.
            //
            H += HLSMAX;
        }
        if (H >= HLSMAX)
        {
            H -= HLSMAX;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HueToRGB
//
//  Utility routine for HLStoRGB.
//
////////////////////////////////////////////////////////////////////////////

WORD HueToRGB(
    WORD n1,
    WORD n2,
    WORD hue)
{
    if (hue >= HLSMAX)
    {
        hue -= HLSMAX;
    }

    //
    //  Return r, g, or b value from this tridrant.
    //
    if (hue < (HLSMAX / 6))
    {
        return ((WORD)(n1 + (((n2 - n1) * hue + (HLSMAX / 12)) / (HLSMAX / 6))));
    }
    if (hue < (HLSMAX/2))
    {
        return (n2);
    }
    if (hue < ((HLSMAX*2)/3))
    {
        return ((WORD)(n1 + (((n2 - n1) * (((HLSMAX * 2) / 3) - hue) +
                       (HLSMAX / 12)) / (HLSMAX / 6))));
    }
    else
    {
        return (n1);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HLStoRGB
//
////////////////////////////////////////////////////////////////////////////

DWORD HLStoRGB(
    WORD hue,
    WORD lum,
    WORD sat)
{
    WORD R, G, B;                      // RGB component values
    WORD Magic1, Magic2;               // calculated magic numbers

    if (sat == 0)
    {
        //
        //  Achromatic case.
        //
        R = G = B = (WORD)((lum * RGBMAX) / HLSMAX);
    }
    else
    {
        //
        //  Chromatic case
        //

        //
        //  Set up magic numbers.
        //
        if (lum <= (HLSMAX / 2))
        {
            Magic2 = (WORD)((lum * ((DWORD)HLSMAX + sat) + (HLSMAX / 2)) / HLSMAX);
        }
        else
        {
            Magic2 = lum + sat -
                     (WORD)(((lum * sat) + (DWORD)(HLSMAX / 2)) / HLSMAX);
        }
        Magic1 = (WORD)(2 * lum - Magic2);

        //
        //  Get RGB, change units from HLSMAX to RGBMAX.
        //
        R = (WORD)(((HueToRGB(Magic1, Magic2, (WORD)(hue + (HLSMAX / 3))) *
                     (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX);
        G = (WORD)(((HueToRGB(Magic1, Magic2, hue) *
                     (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX);
        B = (WORD)(((HueToRGB(Magic1, Magic2, (WORD)(hue - (HLSMAX / 3))) *
                     (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX);
    }
    return (RGB(R, G, B));
}


////////////////////////////////////////////////////////////////////////////
//
//  RGBEditChange
//
//  Checks the edit box for a valid entry and updates the Hue, Sat, and Lum
//  edit controls if appropriate.  Also updates Lum picture and current
//  color sample.
//
//  nDlgID - Dialog ID of Red, Green or Blue edit control.
//
////////////////////////////////////////////////////////////////////////////

SHORT RGBEditChange(
    SHORT nDlgID,
    PCOLORINFO pCI)
{
    BOOL bOK;               // check that value in edit control is uint
    BYTE *currentValue;     // pointer to byte in RGB to change (or reset)
    SHORT nVal;
    TCHAR cEdit[3];
    register HWND hDlg = pCI->hDialog;

    currentValue = (BYTE *)&pCI->currentRGB;
    switch (nDlgID)
    {
        case ( COLOR_GREEN ) :
        {
            currentValue++;
            break;
        }
        case ( COLOR_BLUE ) :
        {
            currentValue += 2;
            break;
        }
    }
    nVal = (SHORT)GetDlgItemInt(hDlg, nDlgID, (BOOL FAR *)&bOK, FALSE);
    if (bOK)
    {
        if (nVal > RGBMAX)
        {
            nVal = RGBMAX;
            SetDlgItemInt(hDlg, nDlgID, nVal, FALSE);
        }
        if (nVal != (SHORT) *currentValue)
        {
            *currentValue = LOBYTE(nVal);
            ChangeColorSettings(pCI);
            SetHLSEdit(nDlgID, pCI);
        }
    }
    else if (GetDlgItemText(hDlg, nDlgID, (LPTSTR)cEdit, 2))
    {
        SetRGBEdit(nDlgID, pCI);
        SendDlgItemMessage(hDlg, nDlgID, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    }
    return (SHORT)(bOK ? TRUE : FALSE);
}

