/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    The drawing functions used by the tools
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void
Line(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, int thickness)
{
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, color));
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
    SetPixelV(hdc, x2, y2, color);
    DeleteObject(SelectObject(hdc, oldPen));
}

void
Rect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg,  COLORREF bg, int thickness, int style)
{
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    logbrush.lbStyle = (style == 0) ? BS_HOLLOW : BS_SOLID;
    logbrush.lbColor = (style == 2) ? fg : bg;
    logbrush.lbHatch = 0;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Rectangle(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void
Ellp(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg,  COLORREF bg, int thickness, int style)
{
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    logbrush.lbStyle = (style == 0) ? BS_HOLLOW : BS_SOLID;
    logbrush.lbColor = (style == 2) ? fg : bg;
    logbrush.lbHatch = 0;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Ellipse(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void
RRect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg,  COLORREF bg, int thickness, int style)
{
    LOGBRUSH logbrush;
    HBRUSH oldBrush;
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    logbrush.lbStyle = (style == 0) ? BS_HOLLOW : BS_SOLID;
    logbrush.lbColor = (style == 2) ? fg : bg;
    logbrush.lbHatch = 0;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    RoundRect(hdc, x1, y1, x2, y2, 16, 16);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void
Poly(HDC hdc, POINT * lpPoints, int nCount,  COLORREF fg,  COLORREF bg, int thickness, int style, BOOL closed, BOOL inverted)
{
    LOGBRUSH logbrush;
    HBRUSH oldBrush;
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    UINT oldRop = GetROP2(hdc);

    if (inverted)
      SetROP2(hdc, R2_NOTXORPEN);

    logbrush.lbStyle = (style == 0) ? BS_HOLLOW : BS_SOLID;
    logbrush.lbColor = (style == 2) ? fg : bg;
    logbrush.lbHatch = 0;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    if (closed)
        Polygon(hdc, lpPoints, nCount);
    else
        Polyline(hdc, lpPoints, nCount);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));

    SetROP2(hdc, oldRop);
}

void
Bezier(HDC hdc, POINT p1, POINT p2, POINT p3, POINT p4, COLORREF color, int thickness)
{
    HPEN oldPen;
    POINT fourPoints[4];
    fourPoints[0] = p1;
    fourPoints[1] = p2;
    fourPoints[2] = p3;
    fourPoints[3] = p4;
    oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, color));
    PolyBezier(hdc, fourPoints, 4);
    DeleteObject(SelectObject(hdc, oldPen));
}

void
Fill(HDC hdc, LONG x, LONG y, COLORREF color)
{
    HBRUSH oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(color));
    ExtFloodFill(hdc, x, y, GetPixel(hdc, x, y), FLOODFILLSURFACE);
    DeleteObject(SelectObject(hdc, oldBrush));
}

void
Erase(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG radius)
{
    LONG b = max(1, max(labs(x2 - x1), labs(y2 - y1)));
    HBRUSH hbr = ::CreateSolidBrush(color);

    for (LONG a = 0; a <= b; a++)
    {
        LONG cx = (x1 * (b - a) + x2 * a) / b;
        LONG cy = (y1 * (b - a) + y2 * a) / b;
        RECT rc = { cx - radius, cy - radius, cx + radius, cy + radius };
        ::FillRect(hdc, &rc, hbr);
    }

    ::DeleteObject(hbr);
}

void
Replace(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LONG radius)
{
    LONG b = max(1, max(labs(x2 - x1), labs(y2 - y1)));

    for (LONG a = 0; a <= b; a++)
    {
        LONG cx = (x1 * (b - a) + x2 * a) / b;
        LONG cy = (y1 * (b - a) + y2 * a) / b;
        RECT rc = { cx - radius, cy - radius, cx + radius, cy + radius };
        for (LONG y = rc.top; y < rc.bottom; ++y)
        {
            for (LONG x = rc.left; x < rc.right; ++x)
            {
                if (::GetPixel(hdc, x, y) == fg)
                    ::SetPixelV(hdc, x, y, bg);
            }
        }
    }
}

void
Airbrush(HDC hdc, LONG x, LONG y, COLORREF color, LONG r)
{
    for (LONG dy = -r; dy <= r; dy++)
    {
        for (LONG dx = -r; dx <= r; dx++)
        {
            if ((dx * dx + dy * dy <= r * r) && (rand() % r == 0))
                ::SetPixelV(hdc, x + dx, y + dy, color);
        }
    }
}

void
Brush(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG style, INT thickness)
{
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, color));
    HBRUSH oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(color));

    if (thickness <= 1)
    {
        Line(hdc, x1, y1, x2, y2, color, thickness);
    }
    else
    {
        LONG a, b = max(1, max(labs(x2 - x1), labs(y2 - y1)));
        switch ((BrushStyle)style)
        {
            case BrushStyleRound:
                for (a = 0; a <= b; a++)
                {
                    Ellipse(hdc,
                            (x1 * (b - a) + x2 * a) / b - (thickness / 2),
                            (y1 * (b - a) + y2 * a) / b - (thickness / 2),
                            (x1 * (b - a) + x2 * a) / b + (thickness / 2),
                            (y1 * (b - a) + y2 * a) / b + (thickness / 2));
                }
                break;

            case BrushStyleSquare:
                for (a = 0; a <= b; a++)
                {
                    Rectangle(hdc,
                              (x1 * (b - a) + x2 * a) / b - (thickness / 2),
                              (y1 * (b - a) + y2 * a) / b - (thickness / 2),
                              (x1 * (b - a) + x2 * a) / b + (thickness / 2),
                              (y1 * (b - a) + y2 * a) / b + (thickness / 2));
                }
                break;

            case BrushStyleForeSlash:
            case BrushStyleBackSlash:
            {
                POINT offsetTop, offsetBottom;
                if ((BrushStyle)style == BrushStyleForeSlash)
                {
                    offsetTop    = { (thickness - 1) / 2, -(thickness - 1) / 2 };
                    offsetBottom = { -thickness      / 2,   thickness      / 2 };
                }
                else
                {
                    offsetTop =    { -thickness      / 2, -thickness      / 2 };
                    offsetBottom = { (thickness - 1) / 2, (thickness - 1) / 2 };
                }
                POINT points[4] =
                {
                    { x1 + offsetTop.x,    y1 + offsetTop.y    },
                    { x1 + offsetBottom.x, y1 + offsetBottom.y },
                    { x2 + offsetBottom.x, y2 + offsetBottom.y },
                    { x2 + offsetTop.x,    y2 + offsetTop.y    },
                };
                Polygon(hdc, points, _countof(points));
                break;
            }
        }
    }
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void
RectSel(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2)
{
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_DOT, 1, GetSysColor(COLOR_HIGHLIGHT)));
    UINT oldRop = GetROP2(hdc);

    SetROP2(hdc, R2_NOTXORPEN);

    logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = 0;
    logbrush.lbHatch = 0;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Rectangle(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));

    SetROP2(hdc, oldRop);
}

void
Text(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LPCWSTR lpchText, HFONT font, LONG style)
{
    INT iSaveDC = ::SaveDC(hdc); // We will modify the clipping region. Save now.

    CRect rc = { x1, y1, x2, y2 };

    if (style == 0) // Transparent
    {
        ::SetBkMode(hdc, TRANSPARENT);
    }
    else // Opaque
    {
        ::SetBkMode(hdc, OPAQUE);
        ::SetBkColor(hdc, bg);

        HBRUSH hbr = ::CreateSolidBrush(bg);
        ::FillRect(hdc, &rc, hbr); // Fill the background
        ::DeleteObject(hbr);
    }

    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    HGDIOBJ hFontOld = ::SelectObject(hdc, font);
    ::SetTextColor(hdc, fg);
    const UINT uFormat = DT_LEFT | DT_TOP | DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP |
                         DT_EXPANDTABS | DT_WORDBREAK;
    ::DrawTextW(hdc, lpchText, -1, &rc, uFormat);
    ::SelectObject(hdc, hFontOld);

    ::RestoreDC(hdc, iSaveDC); // Restore
}

BOOL
ColorKeyedMaskBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
                  HDC hdcSrc, int nXSrc, int nYSrc, int nSrcWidth, int nSrcHeight,
                  HBITMAP hbmMask, COLORREF keyColor)
{
    HDC hTempDC1, hTempDC2;
    HBITMAP hbmTempColor, hbmTempMask;
    HGDIOBJ hbmOld1, hbmOld2;

    if (hbmMask == NULL)
    {
        if (keyColor == CLR_INVALID)
        {
            ::StretchBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
                         hdcSrc, nXSrc, nYSrc, nSrcWidth, nSrcHeight, SRCCOPY);
        }
        else
        {
            ::GdiTransparentBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
                                hdcSrc, nXSrc, nYSrc, nSrcWidth, nSrcHeight, keyColor);
        }
        return TRUE;
    }
    else if (nWidth == nSrcWidth && nHeight == nSrcHeight && keyColor == CLR_INVALID)
    {
        ::MaskBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
                  hdcSrc, nXSrc, nYSrc, hbmMask, 0, 0, MAKEROP4(SRCCOPY, 0xAA0029));
        return TRUE;
    }

    hTempDC1 = ::CreateCompatibleDC(hdcDest);
    hTempDC2 = ::CreateCompatibleDC(hdcDest);
    hbmTempMask = ::CreateBitmap(nWidth, nHeight, 1, 1, NULL);
    hbmTempColor = CreateColorDIB(nWidth, nHeight, RGB(255, 255, 255));

    // hbmTempMask <-- hbmMask (stretched)
    hbmOld1 = ::SelectObject(hTempDC1, hbmMask);
    hbmOld2 = ::SelectObject(hTempDC2, hbmTempMask);
    ::StretchBlt(hTempDC2, 0, 0, nWidth, nHeight, hTempDC1, 0, 0, nSrcWidth, nSrcHeight, SRCCOPY);
    ::SelectObject(hTempDC2, hbmOld2);
    ::SelectObject(hTempDC1, hbmOld1);

    hbmOld1 = ::SelectObject(hTempDC1, hbmTempColor);
    if (keyColor == CLR_INVALID)
    {
        // hbmTempColor <-- hdcSrc (stretched)
        ::StretchBlt(hTempDC1, 0, 0, nWidth, nHeight,
                     hdcSrc, nXSrc, nYSrc, nSrcWidth, nSrcHeight, SRCCOPY);

        // hdcDest <-- hbmTempColor (masked)
        ::MaskBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hTempDC1, 0, 0,
                  hbmTempMask, 0, 0, MAKEROP4(SRCCOPY, 0xAA0029));
    }
    else
    {
        // hbmTempColor <-- hdcDest
        ::BitBlt(hTempDC1, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY);

        // hbmTempColor <-- hdcSrc (color key)
        ::GdiTransparentBlt(hTempDC1, 0, 0, nWidth, nHeight,
                            hdcSrc, nXSrc, nYSrc, nSrcWidth, nSrcHeight, keyColor);

        // hdcDest <-- hbmTempColor (masked)
        ::MaskBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hTempDC1, 0, 0,
                  hbmTempMask, 0, 0, MAKEROP4(SRCCOPY, 0xAA0029));
    }
    ::SelectObject(hTempDC1, hbmOld1);

    ::DeleteObject(hbmTempColor);
    ::DeleteObject(hbmTempMask);
    ::DeleteDC(hTempDC2);
    ::DeleteDC(hTempDC1);

    return TRUE;
}

void DrawXorRect(HDC hdc, const RECT *prc)
{
    HGDIOBJ oldPen = ::SelectObject(hdc, ::CreatePen(PS_SOLID, 0, RGB(0, 0, 0)));
    HGDIOBJ oldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
    INT oldRop2 = SetROP2(hdc, R2_NOTXORPEN);
    ::Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);
    ::SetROP2(hdc, oldRop2);
    ::SelectObject(hdc, oldBrush);
    ::DeleteObject(::SelectObject(hdc, oldPen));
}
