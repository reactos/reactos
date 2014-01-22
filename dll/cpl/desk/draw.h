#pragma once

BOOL
MyDrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState, COLOR_SCHEME *scheme);
BOOL
MyDrawEdge(HDC hDC, LPRECT rc, UINT edge, UINT flags, COLOR_SCHEME *scheme);
VOID
MyDrawCaptionButtons(HDC hdc, LPRECT lpRect, BOOL bMinMax, int x, COLOR_SCHEME *scheme);
VOID
MyDrawScrollbar(HDC hdc, LPRECT rc, HBRUSH hbrScrollbar, COLOR_SCHEME *scheme);
BOOL
MyDrawCaptionTemp(HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont, HICON hIcon, LPCWSTR str, UINT uFlags, COLOR_SCHEME *scheme);
DWORD
MyDrawMenuBarTemp(HWND Wnd, HDC DC, LPRECT Rect, HMENU Menu, HFONT Font, COLOR_SCHEME *scheme);

#define MY_BF_ACTIVEBORDER 0x1000000
#define MY_BF_INACTIVEBORDER 0x2000000
