/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    The drawing functions used by the tools
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

/* FUNCTIONS ********************************************************/

// WidenPath requires a geometric pen
HPEN CreateGeometricPen(COLORREF rgbColor, INT thickness)
{
    LOGBRUSH logbrush;
    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = rgbColor;
    logbrush.lbHatch = 0;
    return ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, thickness, &logbrush, 0, NULL);
}

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
Line(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hBrush, INT thickness)
{
    HGDIOBJ oldPen = SelectObject(hdc, CreateGeometricPen(0, thickness));
    BeginPath(hdc);
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
    EndPath(hdc);
    SetPolyFillMode(hdc, WINDING);
    WidenPath(hdc);
    DeleteObject(SelectObject(hdc, oldPen));

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hBrush);
    DeleteObject(hRgn);

    RECT rc = { x2, y2, x2 + 1, y2 + 1 };
    FillRect(hdc, &rc, hBrush);
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
Rect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style)
{
    HGDIOBJ hPenOld;
    if (style != 0)
    {
        HGDIOBJ hBrushOld = SelectObject(hdc, (style == 2) ? hFgBrush : hBgBrush);
        hPenOld = SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, x1, y1, x2, y2);
        SelectObject(hdc, hBrushOld);
        SelectObject(hdc, hPenOld);
    }

    HPEN hPen = CreateGeometricPen(0, thickness);
    hPenOld = SelectObject(hdc, hPen);
    BeginPath(hdc);
    Rectangle(hdc, x1, y1, x2, y2);
    EndPath(hdc);
    SetPolyFillMode(hdc, WINDING);
    WidenPath(hdc);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hFgBrush);
    DeleteObject(hRgn);
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
Ellp(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style)
{
    HGDIOBJ hPenOld;
    if (style != 0)
    {
        HGDIOBJ hBrushOld = SelectObject(hdc, (style == 2) ? hFgBrush : hBgBrush);
        hPenOld = SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, x1, y1, x2, y2);
        SelectObject(hdc, hBrushOld);
        SelectObject(hdc, hPenOld);
    }

    HPEN hPen = CreateGeometricPen(0, thickness);
    hPenOld = SelectObject(hdc, hPen);
    BeginPath(hdc);
    Ellipse(hdc, x1, y1, x2, y2);
    EndPath(hdc);
    SetPolyFillMode(hdc, WINDING);
    WidenPath(hdc);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hFgBrush);
    DeleteObject(hRgn);
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
RRect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style)
{
    HGDIOBJ hPenOld;
    if (style != 0)
    {
        HGDIOBJ hBrushOld = SelectObject(hdc, (style == 2) ? hFgBrush : hBgBrush);
        hPenOld = SelectObject(hdc, GetStockObject(NULL_PEN));
        RoundRect(hdc, x1, y1, x2, y2, 16, 16);
        SelectObject(hdc, hBrushOld);
        SelectObject(hdc, hPenOld);
    }

    HPEN hPen = CreateGeometricPen(0, thickness);
    hPenOld = SelectObject(hdc, hPen);
    BeginPath(hdc);
    RoundRect(hdc, x1, y1, x2, y2, 16, 16);
    EndPath(hdc);
    SetPolyFillMode(hdc, WINDING);
    WidenPath(hdc);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hFgBrush);
    DeleteObject(hRgn);
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
Poly(HDC hdc, POINT *lpPoints, INT nCount, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style, BOOL closed, BOOL inverted)
{
    UINT oldRop = GetROP2(hdc);
    if (inverted)
        SetROP2(hdc, R2_NOTXORPEN);

    HGDIOBJ hPenOld;
    if (style != 0)
    {
        HGDIOBJ hBrushOld = SelectObject(hdc, (style == 2) ? hFgBrush : hBgBrush);
        hPenOld = SelectObject(hdc, GetStockObject(NULL_PEN));
        if (closed)
            Polygon(hdc, lpPoints, nCount);
        else
            Polyline(hdc, lpPoints, nCount);
        SelectObject(hdc, hBrushOld);
        SelectObject(hdc, hPenOld);
    }

    HPEN hPen = CreateGeometricPen(0, thickness);
    hPenOld = SelectObject(hdc, hPen);
    BeginPath(hdc);
    if (closed)
        Polygon(hdc, lpPoints, nCount);
    else
        Polyline(hdc, lpPoints, nCount);
    EndPath(hdc);
    SetPolyFillMode(hdc, WINDING);
    WidenPath(hdc);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hFgBrush);
    DeleteObject(hRgn);

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
Bezier(HDC hdc, POINT p1, POINT p2, POINT p3, POINT p4, HBRUSH hBrush, INT thickness)
{
    HPEN oldPen;
    POINT fourPoints[4];
    fourPoints[0] = p1;
    fourPoints[1] = p2;
    fourPoints[2] = p3;
    fourPoints[3] = p4;
    oldPen = (HPEN) SelectObject(hdc, CreateGeometricPen(0, thickness));
    BeginPath(hdc);
    PolyBezier(hdc, fourPoints, 4);
    EndPath(hdc);
    SetPolyFillMode(hdc, WINDING);
    WidenPath(hdc);

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hBrush);
    DeleteObject(hRgn);

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
Fill(HDC hdc, LONG x, LONG y, HBRUSH hBrush)
{
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    ExtFloodFill(hdc, x, y, GetPixel(hdc, x, y), FLOODFILLSURFACE);
    SelectObject(hdc, oldBrush);
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
Erase(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hBrush, LONG radius)
{
    LONG b = max(1, max(labs(x2 - x1), labs(y2 - y1)));

    for (LONG a = 0; a <= b; a++)
    {
        LONG cx = (x1 * (b - a) + x2 * a) / b;
        LONG cy = (y1 * (b - a) + y2 * a) / b;
        RECT rc = { cx - radius, cy - radius, cx + radius, cy + radius };
        ::FillRect(hdc, &rc, hBrush);
    }
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
Replace(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, HBRUSH hBgBrush, LONG radius)
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
                {
                    RECT rc = { x, y, x + 1, y + 1 };
                    FillRect(hdc, &rc, hBgBrush);
                }
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
Airbrush(HDC hdc, LONG x, LONG y, HBRUSH hBrush, LONG r)
{
    for (LONG dy = -r; dy <= r; dy++)
    {
        for (LONG dx = -r; dx <= r; dx++)
        {
            if ((dx * dx + dy * dy <= r * r) && (rand() % r == 0))
            {
                RECT rc = { x + dx, y + dy, x + dx + 1, y + dy + 1 };
                FillRect(hdc, &rc, hBrush);
            }
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
Brush(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hBrush, LONG style, INT thickness)
{
    HPEN oldPen = (HPEN) SelectObject(hdc, GetStockObject(NULL_PEN));
    HBRUSH oldBrush = (HBRUSH) SelectObject(hdc, hBrush);

    if (thickness <= 1)
    {
        Line(hdc, x1, y1, x2, y2, hBrush, thickness);
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
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
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

void
Text(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, LPCWSTR lpchText, HFONT font, LONG style)
{
    INT iSaveDC = ::SaveDC(hdc); // We will modify the clipping region. Save now.

    CRect rc = { x1, y1, x2, y2 };

    ::SetBkMode(hdc, TRANSPARENT);
    if (style != 0) // Not Transparent
        ::FillRect(hdc, &rc, hBgBrush); // Fill the background

    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    HGDIOBJ hFontOld = ::SelectObject(hdc, font);
    const UINT uFormat = DT_LEFT | DT_TOP | DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP |
                         DT_EXPANDTABS | DT_WORDBREAK;
    ::BeginPath(hdc);
    ::DrawTextW(hdc, lpchText, -1, &rc, uFormat);
    ::EndPath(hdc);

    HRGN hRgn = PathToRegion(hdc);
    FillRgn(hdc, hRgn, hFgBrush);
    DeleteObject(hRgn);

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

HBRUSH CreateDitherBrush(COLORREF color, COLORREF monoColor0, COLORREF monoColor1)
{
    // 8x8 Bayer ordered dithering matrix (0 to 63)
    static const BYTE s_bayerMatrix[8][8] =
    {
        {  0, 32,  8, 40,  2, 34, 10, 42 },
        { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 },
        { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 },
        { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 },
        { 63, 31, 55, 23, 61, 29, 53, 21 },
    };
    INT sum = GetRValue(color) + GetGValue(color) + GetBValue(color);
    INT brightness = sum / 3;
    if (brightness < 0)
        brightness = 0;
    if (brightness >= 255)
        brightness = 256; // White out

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth    = 8;
    bmi.bmiHeader.biHeight   = -8; // Top-down
    bmi.bmiHeader.biPlanes   = 1;
    bmi.bmiHeader.biBitCount = 24;

    const BYTE b0 = GetBValue(monoColor0), g0 = GetGValue(monoColor0), r0 = GetRValue(monoColor0);
    const BYTE b1 = GetBValue(monoColor1), g1 = GetGValue(monoColor1), r1 = GetRValue(monoColor1);

    BYTE pixels[8 * 8 * 3];
    for (INT y = 0; y < 8; ++y)
    {
        INT index = y * (3 * CHAR_BIT);
        for (INT x = 0; x < 8; ++x)
        {
            const INT threshold = s_bayerMatrix[y][x] * 255 / 63;
            if (brightness > threshold)
            {
                pixels[index++] = b1; // Blue
                pixels[index++] = g1; // Green
                pixels[index++] = r1; // Red
            }
            else
            {
                pixels[index++] = b0; // Blue
                pixels[index++] = g0; // Green
                pixels[index++] = r0; // Red
            }
        }
    }

    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, pixels, &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    if (!hBitmap)
        return NULL;

    HBRUSH hBrush = CreatePatternBrush(hBitmap);
    DeleteObject(hBitmap);

    return hBrush;
}
