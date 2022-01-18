/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/drawing.h
 * PURPOSE:     The drawing functions used by the tools
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

void Line(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, int thickness);

void Rect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, int thickness, int style);

void Ellp(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, int thickness, int style);

void RRect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, int thickness, int style);

void Poly(HDC hdc, POINT *lpPoints, int nCount, COLORREF fg, COLORREF bg, int thickness, int style, BOOL closed, BOOL inverted);

void Bezier(HDC hdc, POINT p1, POINT p2, POINT p3, POINT p4, COLORREF color, int thickness);

void Fill(HDC hdc, LONG x, LONG y, COLORREF color);

void Erase(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG radius);

void Replace(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LONG radius);

void Airbrush(HDC hdc, LONG x, LONG y, COLORREF color, LONG r);

void Brush(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG style);

void RectSel(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2);

void SelectionFrame(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF system_selection_color);

void Text(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LPCTSTR lpchText, HFONT font, LONG style);
