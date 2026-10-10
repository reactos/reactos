/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    The drawing functions used by the tools
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

void Line(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, int thickness);
void Line(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hBrush, INT thickness);

void Rect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, int thickness, int style);
void Rect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style);

void Ellp(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, int thickness, int style);
void Ellp(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style);

void RRect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, int thickness, int style);
void RRect(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style);

void Poly(HDC hdc, POINT *lpPoints, int nCount, COLORREF fg, COLORREF bg, int thickness, int style, BOOL closed, BOOL inverted);
void Poly(HDC hdc, POINT *lpPoints, INT nCount, HBRUSH hFgBrush, HBRUSH hBgBrush, INT thickness, INT style, BOOL closed, BOOL inverted);

void Bezier(HDC hdc, POINT p1, POINT p2, POINT p3, POINT p4, COLORREF color, int thickness);
void Bezier(HDC hdc, POINT p1, POINT p2, POINT p3, POINT p4, HBRUSH hBrush, INT thickness);

void Fill(HDC hdc, LONG x, LONG y, COLORREF color);
void Fill(HDC hdc, LONG x, LONG y, HBRUSH hBrush);

void Erase(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG radius);
void Erase(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hBrush, LONG radius);

void Replace(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LONG radius);
void Replace(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, HBRUSH hBgBrush, LONG radius);

void Airbrush(HDC hdc, LONG x, LONG y, COLORREF color, LONG r);
void Airbrush(HDC hdc, LONG x, LONG y, HBRUSH hBrush, LONG r);

void Brush(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF color, LONG style, INT thickness);
void Brush(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hBrush, LONG style, INT thickness);

void RectSel(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2);

void Text(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, COLORREF fg, COLORREF bg, LPCWSTR lpchText, HFONT font, LONG style);
void Text(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, HBRUSH hFgBrush, HBRUSH hBgBrush, LPCWSTR lpchText, HFONT font, LONG style);

BOOL
ColorKeyedMaskBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
                  HDC hdcSrc, int nXSrc, int nYSrc, int nSrcWidth, int nSrcHeight,
                  HBITMAP hbmMask, COLORREF keyColor);

void DrawXorRect(HDC hdc, const RECT *prc);

HBRUSH CreateDitherBrush(COLORREF color, COLORREF monoColor0, COLORREF monoColor1);
