/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/window.h
 * PURPOSE:     Window management definitions
 */
#include <windows.h>

extern COLORREF SysColors[];
extern HPEN SysPens[];
extern HBRUSH SysBrushes[];

#define NUM_SYSCOLORS 31

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

#define UserHasAnyFrameStyle(Style, ExStyle)                                   \
  (((Style) & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) ||                    \
   ((ExStyle) & WS_EX_DLGMODALFRAME) ||                                        \
   (!((Style) & (WS_CHILD | WS_POPUP))))

#define UserHasDlgFrameStyle(Style, ExStyle)                                   \
 (((ExStyle) & WS_EX_DLGMODALFRAME) ||                                         \
  (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define UserHasThickFrameStyle(Style, ExStyle)                                 \
  (((Style) & WS_THICKFRAME) &&                                                \
   (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define UserHasThinFrameStyle(Style, ExStyle)                                  \
  (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define UserHasBigFrameStyle(Style, ExStyle)                                   \
  (((Style) & (WS_THICKFRAME | WS_DLGFRAME)) ||                                \
   ((ExStyle) & WS_EX_DLGMODALFRAME))


BOOL UserDrawSysMenuButton( HWND hWnd, HDC hDC, LPRECT, BOOL down );
void
UserGetFrameSize(ULONG Style, ULONG ExStyle, SIZE *Size);
void
UserGetInsideRectNC(PWINDOW Wnd, RECT *rect);

DWORD
SCROLL_HitTest( HWND hwnd, INT nBar, POINT pt, BOOL bDragging );

LRESULT FASTCALL IntCallWindowProcW(BOOL IsAnsiProc, WNDPROC WndProc,
                                    HWND hWnd, UINT Msg, WPARAM wParam,
                                    LPARAM lParam);

HWND STDCALL
User32CreateWindowEx(DWORD dwExStyle,
		LPCSTR lpClassName,
		LPCSTR lpWindowName,
		DWORD dwStyle,
		int x,
		int y,
		int nWidth,
		int nHeight,
		HWND hWndParent,
		HMENU hMenu,
		HINSTANCE hInstance,
		LPVOID lpParam,
		BOOL Unicode);
