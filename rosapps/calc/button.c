/*
 *  ReactOS calc
 *
 *  button.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
    
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "button.h"


COLORREF text_colour;
COLORREF background_colour;
COLORREF disabled_background_colour;
COLORREF light;
COLORREF highlight;
COLORREF shadow;
COLORREF dark_shadow;

const COLORREF CLR_BTN_WHITE    = RGB(255, 255, 255);
const COLORREF CLR_BTN_BLACK    = RGB(0, 0, 0);
const COLORREF CLR_BTN_DGREY    = RGB(128, 128, 128);
const COLORREF CLR_BTN_GREY     = RGB(192, 192, 192);
const COLORREF CLR_BTN_LLGREY   = RGB(223, 223, 223);

const int BUTTON_IN             = 0x01;
const int BUTTON_OUT            = 0x02;
const int BUTTON_BLACK_BORDER   = 0x04;


void InitColours(void)
{
//    text_colour                = GetSysColor(COLOR_BTNTEXT);
    text_colour                = RGB(255,0,0);

    background_colour          = GetSysColor(COLOR_BTNFACE); 
    disabled_background_colour = background_colour;
    light                      = GetSysColor(COLOR_3DLIGHT);
    highlight                  = GetSysColor(COLOR_BTNHIGHLIGHT);
    shadow                     = GetSysColor(COLOR_BTNSHADOW);
    dark_shadow                = GetSysColor(COLOR_3DDKSHADOW);
}

static void FillSolidRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clr)
{
    RECT rect;

    SetBkColor(hDC, clr);
    rect.left = x;
    rect.top = y;
    rect.right = x + cx;
    rect.bottom = y + cy;
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

void Draw3dRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    FillSolidRect(hDC, x, y, cx - 1, 1, clrTopLeft);
    FillSolidRect(hDC, x, y, 1, cy - 1, clrTopLeft);
    FillSolidRect(hDC, x + cx, y, -1, cy, clrBottomRight);
    FillSolidRect(hDC, x, y + cy, cx, -1, clrBottomRight);
}


void DrawFilledRect(HDC hdc, LPRECT r, COLORREF colour)
{ 
    HBRUSH hBrush;

    hBrush = CreateSolidBrush(colour);
    FillRect(hdc, r, hBrush);
}

void DrawLine(HDC hdc, long sx, long sy, long ex, long ey, COLORREF colour)
{ 
    HPEN new_pen;
    HPEN old_pen;

    new_pen = CreatePen(PS_SOLID, 1, colour);
    old_pen = SelectObject(hdc, &new_pen);
    MoveToEx(hdc, sx, sy, NULL);
    LineTo(hdc, ex, ey);
    SelectObject(hdc, old_pen);
    DeleteObject(new_pen);
}

void DrawFrame(HDC hdc, RECT r, int state)
{ 
    COLORREF colour;

    if (state & BUTTON_BLACK_BORDER) {
        colour = CLR_BTN_BLACK;
        DrawLine(hdc, r.left,  r.top,       r.right, r.top, colour);   // Across top
        DrawLine(hdc, r.left, r.top,        r.left, r.bottom, colour); // Down left
        DrawLine(hdc, r.left, r.bottom - 1, r.right, r.bottom - 1, colour); // Across bottom
        DrawLine(hdc, r.right - 1, r.top,   r.right - 1, r.bottom, colour); // Down right
        InflateRect(&r, -1, -1);
    }
    if (state & BUTTON_OUT) {
        colour = highlight;
        DrawLine(hdc, r.left, r.top,        r.right, r.top, colour);    // Across top
        DrawLine(hdc, r.left, r.top,        r.left,  r.bottom, colour); // Down left
        colour = dark_shadow;
        DrawLine(hdc, r.left, r.bottom - 1, r.right, r.bottom - 1, colour); // Across bottom
        DrawLine(hdc, r.right - 1, r.top,   r.right - 1, r.bottom, colour); // Down right
        InflateRect(&r, -1, -1);
        colour = light;
        DrawLine(hdc, r.left, r.top,        r.right, r.top, colour);   // Across top
        DrawLine(hdc, r.left, r.top,        r.left, r.bottom, colour); // Down left
        colour = shadow;
        DrawLine(hdc, r.left, r.bottom - 1, r.right, r.bottom - 1, colour); // Across bottom
        DrawLine(hdc, r.right - 1, r.top,   r.right - 1, r.bottom, colour); // Down right
    }
    if (state & BUTTON_IN) {
        colour = dark_shadow;
        DrawLine(hdc, r.left, r.top,        r.right, r.top, colour);   // Across top
        DrawLine(hdc, r.left, r.top,        r.left, r.bottom, colour); // Down left
        DrawLine(hdc, r.left, r.bottom - 1, r.right, r.bottom - 1, colour); // Across bottom
        DrawLine(hdc, r.right - 1, r.top,   r.right - 1, r.bottom, colour); // Down right
        InflateRect(&r, -1, -1);
        colour = shadow;
        DrawLine(hdc, r.left,  r.top,        r.right, r.top, colour);    // Across top
        DrawLine(hdc, r.left,  r.top,        r.left,  r.bottom, colour); // Down left
        DrawLine(hdc, r.left,  r.bottom - 1, r.right, r.bottom - 1, colour); // Across bottom
        DrawLine(hdc, r.right - 1, r.top,    r.right - 1, r.bottom, colour); // Down right
    }
}

void DrawButtonText(HDC hdc, LPRECT r, LPCTSTR Buf, COLORREF text_colour)
{
    COLORREF previous_colour;
   
    previous_colour = SetTextColor(hdc, text_colour);
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, Buf, _tcslen(Buf), r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(hdc, previous_colour);
}

#define bufSize 512

void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    HDC   hdc;
    RECT  focus_rect, button_rect, offset_button_rect;
    UINT  state;
    TCHAR buffer[bufSize];
    
    hdc = lpDrawItemStruct->hDC;
    state = lpDrawItemStruct->itemState;

    CopyRect(&focus_rect, &lpDrawItemStruct->rcItem); 
    CopyRect(&button_rect, &lpDrawItemStruct->rcItem); 

    // Set the focus rectangle to just past the border decoration
    focus_rect.left  += 4;
    focus_rect.right  -= 4;
    focus_rect.top    += 4;
    focus_rect.bottom -= 4;
      
    // Retrieve the button's caption
    GetWindowText(lpDrawItemStruct->hwndItem, buffer, bufSize);

    if (state & ODS_DISABLED) {
        DrawFilledRect(hdc, &button_rect, disabled_background_colour);
    } else {
        DrawFilledRect(hdc, &button_rect, background_colour);
    }
        
    if (state & ODS_SELECTED) { 
        DrawFrame(hdc, button_rect, BUTTON_IN);
    } else {
        if ((state & ODS_DEFAULT) || (state & ODS_FOCUS)) {
            DrawFrame(hdc, button_rect, BUTTON_OUT | BUTTON_BLACK_BORDER);          
        } else {
            DrawFrame(hdc, button_rect, BUTTON_OUT);
        }
    }

    if (state & ODS_DISABLED) {
        offset_button_rect = button_rect;
        OffsetRect(&offset_button_rect, 1, 1);
        DrawButtonText(hdc, &offset_button_rect, buffer, CLR_BTN_WHITE);
        DrawButtonText(hdc, &button_rect, buffer, CLR_BTN_DGREY);
    } else {
        if (lpDrawItemStruct->CtlID >= ID_RED_START && lpDrawItemStruct->CtlID <= ID_RED_FINISH) {
            DrawButtonText(hdc, &button_rect, buffer, RGB(255,0,0));
        } else
        if (lpDrawItemStruct->CtlID >= ID_PURPLE_START && lpDrawItemStruct->CtlID <= ID_PURPLE_FINISH) {
            DrawButtonText(hdc, &button_rect, buffer, RGB(0,128,128));
        } else
        if (lpDrawItemStruct->CtlID >= ID_BLUE_START && lpDrawItemStruct->CtlID <= ID_BLUE_FINISH) {
            DrawButtonText(hdc, &button_rect, buffer, RGB(0,0,255));
        } else {
            DrawButtonText(hdc, &button_rect, buffer, text_colour);
        }
    }

    if (state & ODS_FOCUS) {
        DrawFocusRect(lpDrawItemStruct->hDC, (LPRECT)&focus_rect);
    }
} 
