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

VOID
UserSetupInternalPos(VOID);
BOOL UserDrawSysMenuButton( HWND hWnd, HDC hDC, LPRECT, BOOL down );
PINTERNALPOS
UserGetInternalPos(HWND hWnd);
ULONG
UserHasDlgFrameStyle(ULONG Style, ULONG ExStyle);
ULONG
UserHasThickFrameStyle(ULONG Style, ULONG ExStyle);
void
UserGetFrameSize(ULONG Style, ULONG ExStyle, SIZE *Size);

DWORD
SCROLL_HitTest( HWND hwnd, INT nBar, POINT pt, BOOL bDragging );

