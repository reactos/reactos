/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/window.h
 * PURPOSE:     Window management definitions
 */
#include <windows.h>
#include <user32/wininternal.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

BOOL UserDrawSysMenuButton( HWND hWnd, HDC hDC, LPRECT, BOOL down );
ULONG
UserHasDlgFrameStyle(ULONG Style, ULONG ExStyle);
ULONG
UserHasThickFrameStyle(ULONG Style, ULONG ExStyle);
void
UserGetFrameSize(ULONG Style, ULONG ExStyle, SIZE *Size);
void
UserGetInsideRectNC(HWND hWnd, RECT *rect);

DWORD
SCROLL_HitTest( HWND hwnd, INT nBar, POINT pt, BOOL bDragging );

LRESULT FASTCALL IntCallWindowProcW(BOOL IsAnsiProc, WNDPROC WndProc,
                                    HWND hWnd, UINT Msg, WPARAM wParam,
                                    LPARAM lParam);
