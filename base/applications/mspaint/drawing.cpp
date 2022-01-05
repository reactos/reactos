/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/drawing.cpp
 * PURPOSE:     The drawing functions used by the tools
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void
Line(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, int thickness)
{
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, thickness, color));
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
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
    LONG a, b;
    HPEN oldPen;
    HBRUSH oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(color));

    b = max(1, max(abs(x2 - x1), abs(y2 - y1)));
    oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, color));
    for(a = 0; a <= b; a++)
        Rectangle(hdc, (x1 * (b - a) + x2 * a) / b - radius + 1,
                  (y1 * (b - a) + y2 * a) / b - radius + 1, (x1 * (b - a) + x2 * a) / b + radius + 1,
                  (y1 * (b - a) + y2 * a) / b + radius + 1);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void
Replace(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LONG radius)
{
    LONG a, b, x, y;
    b = max(1, max(abs(x2 - x1), abs(y2 - y1)));

    for(a = 0; a <= b; a++)
        for(y = (y1 * (b - a) + y2 * a) / b - radius + 1;
            y < (y1 * (b - a) + y2 * a) / b + radius + 1; y++)
            for(x = (x1 * (b - a) + x2 * a) / b - radius + 1;
                x < (x1 * (b - a) + x2 * a) / b + radius + 1; x++)
                if (GetPixel(hdc, x, y) == fg)
                    SetPixel(hdc, x, y, bg);
}

void
Airbrush(HDC hdc, LONG x, LONG y, COLORREF color, LONG r)
{
    LONG a, b;

    for(b = -r; b <= r; b++)
        for(a = -r; a <= r; a++)
            if ((a * a + b * b <= r * r) && (rand() % 4 == 0))
                SetPixel(hdc, x + a, y + b, color);
}

void
Brush(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG style)
{
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, color));
    HBRUSH oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(color));
    LONG a, b;
    b = max(1, max(abs(x2 - x1), abs(y2 - y1)));
    switch (style)
    {
        case 0:
            for(a = 0; a <= b; a++)
                Ellipse(hdc, (x1 * (b - a) + x2 * a) / b - 3, (y1 * (b - a) + y2 * a) / b - 3,
                        (x1 * (b - a) + x2 * a) / b + 4, (y1 * (b - a) + y2 * a) / b + 4);
            break;
        case 1:
            for(a = 0; a <= b; a++)
                Ellipse(hdc, (x1 * (b - a) + x2 * a) / b - 1, (y1 * (b - a) + y2 * a) / b - 1,
                        (x1 * (b - a) + x2 * a) / b + 3, (y1 * (b - a) + y2 * a) / b + 3);
            break;
        case 2:
            MoveToEx(hdc, x1, y1, NULL);
            LineTo(hdc, x2, y2);
            SetPixel(hdc, x2, y2, color);
            break;
        case 3:
            for(a = 0; a <= b; a++)
                Rectangle(hdc, (x1 * (b - a) + x2 * a) / b - 3, (y1 * (b - a) + y2 * a) / b - 3,
                          (x1 * (b - a) + x2 * a) / b + 5, (y1 * (b - a) + y2 * a) / b + 5);
            break;
        case 4:
            for(a = 0; a <= b; a++)
                Rectangle(hdc, (x1 * (b - a) + x2 * a) / b - 2, (y1 * (b - a) + y2 * a) / b - 2,
                          (x1 * (b - a) + x2 * a) / b + 3, (y1 * (b - a) + y2 * a) / b + 3);
            break;
        case 5:
            for(a = 0; a <= b; a++)
                Rectangle(hdc, (x1 * (b - a) + x2 * a) / b - 1, (y1 * (b - a) + y2 * a) / b - 1,
                          (x1 * (b - a) + x2 * a) / b + 1, (y1 * (b - a) + y2 * a) / b + 1);
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        {
            POINT offsTop[] = {{4, -3}, {2, -2}, {0, 0},
                               {-3, -3}, {-2, -2}, {-1, 0}};
            POINT offsBtm[] = {{-3, 4}, {-2, 2}, {-1, 1},
                               {4, 4}, {2, 2}, {0, 1}};
            LONG idx = style - 6;
            POINT pts[4];
            pts[0].x = x1 + offsTop[idx].x;
            pts[0].y = y1 + offsTop[idx].y;
            pts[1].x = x1 + offsBtm[idx].x;
            pts[1].y = y1 + offsBtm[idx].y;
            pts[2].x = x2 + offsBtm[idx].x;
            pts[2].y = y2 + offsBtm[idx].y;
            pts[3].x = x2 + offsTop[idx].x;
            pts[3].y = y2 + offsTop[idx].y;
            Polygon(hdc, pts, 4);
            break;
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
SelectionFrame(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, DWORD system_selection_color)
{
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_DOT, 1, system_selection_color));

    logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = 0;
    logbrush.lbHatch = 0;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Rectangle(hdc, x1, y1, x2, y2); /* SEL BOX FRAME */
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, system_selection_color));
    oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(system_selection_color));
    Rectangle(hdc, x1 - 1, y1 - 1, x1 + 2, y1 + 2);
    Rectangle(hdc, x2 - 2, y1 - 1, x2 + 2, y1 + 2);
    Rectangle(hdc, x1 - 1, y2 - 2, x1 + 2, y2 + 1);
    Rectangle(hdc, x2 - 2, y2 - 2, x2 + 2, y2 + 1);
    Rectangle(hdc, (x1 + x2) / 2 - 1, y1 - 1, (x1 + x2) / 2 + 2, y1 + 2);
    Rectangle(hdc, (x1 + x2) / 2 - 1, y2 - 2, (x1 + x2) / 2 + 2, y2 + 1);
    Rectangle(hdc, x1 - 1, (y1 + y2) / 2 - 1, x1 + 2, (y1 + y2) / 2 + 2);
    Rectangle(hdc, x2 - 2, (y1 + y2) / 2 - 1, x2 + 1, (y1 + y2) / 2 + 2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void
Text(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LPCTSTR lpchText, HFONT font, LONG style)
{
    INT iSaveDC = SaveDC(hdc); // We will modify the clipping region. Save now.

    RECT rc;
    SetRect(&rc, x1, y1, x2, y2);

    if (style == 0) // Transparent
    {
        SetBkMode(hdc, TRANSPARENT);
        GetBkColor(hdc);
    }
    else // Opaque
    {
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, bg);

        HBRUSH hbr = CreateSolidBrush(bg);
        FillRect(hdc, &rc, hbr); // Fill the background
        DeleteObject(hbr);
    }

    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    HGDIOBJ hFontOld = SelectObject(hdc, font);
    SetTextColor(hdc, fg);
    const UINT uFormat = DT_LEFT | DT_TOP | DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP |
                         DT_EXPANDTABS | DT_WORDBREAK;
    DrawText(hdc, lpchText, -1, &rc, uFormat);
    SelectObject(hdc, hFontOld);

    RestoreDC(hdc, iSaveDC); // Restore
}
