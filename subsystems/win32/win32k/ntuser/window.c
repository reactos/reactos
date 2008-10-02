/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/window.c
 * PURPOSE:         Window Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
NtUserAlterWindowStyle(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2)
{
    UNIMPLEMENTED;
    return;
}

HWND
APIENTRY
NtUserChildWindowFromPointEx(HWND Parent,
                             LONG x,
                             LONG y,
                             UINT Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
APIENTRY
NtUserCreateWindowEx(DWORD dwExStyle,
                     PUNICODE_STRING lpClassName,
                     PUNICODE_STRING lpWindowName,
                     DWORD dwStyle,
                     LONG x,
                     LONG y,
                     LONG nWidth,
                     LONG nHeight,
                     HWND hWndParent,
                     HMENU hMenu,
                     HINSTANCE hInstance,
                     LPVOID lpParam,
                     DWORD dwShowMode,
                     BOOL bUnicodeWindow,
                     DWORD dwUnknown)
{
    UNIMPLEMENTED;
    return NULL;
}

HDWP
APIENTRY
NtUserDeferWindowPos(HDWP WinPosInfo,
		             HWND Wnd,
		             HWND WndInsertAfter,
		             INT x,
                     INT y,
                     INT cx,
                     INT cy,
		             UINT Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserDefSetText(HWND WindowHandle,
                 PUNICODE_STRING WindowText)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDestroyWindow(HWND Wnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserEndDeferWindowPosEx(DWORD Unknown0,
                         DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserFillWindow(HWND hWndPaint,
                 HWND hWndPaint1,
                 HDC hDC,
                 HBRUSH hBrush)
{
    UNIMPLEMENTED;
    return FALSE;
}

HWND
APIENTRY
NtUserFindWindowEx(HWND hwndParent,
                   HWND hwndChildAfter,
                   PUNICODE_STRING ucClassName,
                   PUNICODE_STRING ucWindowName,
                   DWORD dwUnknown)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserFlashWindowEx(IN PFLASHWINFO pfwi)
{
    UNIMPLEMENTED;
    return FALSE;
}

HWND
APIENTRY
NtUserGetForegroundWindow(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtUserGetInternalWindowPos(HWND hwnd,
                           LPRECT rectWnd,
                           LPPOINT ptIcon)
{
    UNIMPLEMENTED;
    return 0;
}

HDC
APIENTRY
NtUserGetWindowDC(HWND hWnd)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserGetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtUserInternalGetWindowText(HWND hWnd,
                            LPWSTR lpString,
                            INT nMaxCount)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserLockWindowUpdate(HWND hWndLock)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserMoveWindow(HWND hWnd,
                 INT X,
                 INT Y,
                 INT nWidth,
                 INT nHeight,
                 BOOL bRepaint)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserPrintWindow(HWND hwnd,
                  HDC hdcBlt,
                  UINT nFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserRealChildWindowFromPoint(DWORD Unknown0,
                               DWORD Unknown1,
                               DWORD Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserRedrawWindow(HWND hWnd,
                   PRECT prcUpdate,
                   HRGN hrgnUpdate,
                   UINT flags)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserQueryWindow(HWND hWnd,
                  DWORD Index)
{
    UNIMPLEMENTED;
    return 0;
}

HWND
APIENTRY
NtUserSetActiveWindow(HWND hWnd)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtUserSetInternalWindowPos(HWND hwnd,
                           UINT showCmd,
                           LPRECT rect,
                           LPPOINT pt)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserSetLogonNotifyWindow(HWND hWnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetShellWindowEx(HWND hwndShell,
                       HWND hwndShellListView)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetWindowFNID(HWND hWnd,
                    WORD fnID)
{
    UNIMPLEMENTED;
    return FALSE;
}

LONG_PTR
APIENTRY
NtUserSetWindowLong(HWND hWnd,
                    INT Index,
                    LONG_PTR NewValue,
                    BOOL Ansi)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserSetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetWindowPos(HWND hWnd,
                   HWND hWndInsertAfter,
                   int X,
                   int Y,
                   int cx,
                   int cy,
                   UINT uFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtUserSetWindowRgn(HWND hWnd,
                   HRGN hRgn,
                   BOOL bRedraw)
{
    UNIMPLEMENTED;
    return 0;
}

WORD
APIENTRY
NtUserSetWindowWord(HWND hWnd,
                    INT Index,
                    WORD NewVal)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserShowWindow(HWND hWnd,
                 LONG nCmdShow)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserShowWindowAsync(HWND hWnd,
                      LONG nCmdShow)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserUpdateLayeredWindow(HWND hwnd,
                          HDC hdcDst,
                          POINT *pptDst,
                          SIZE *psize,
                          HDC hdcSrc,
                          POINT *pptSrc,
                          COLORREF crKey,
                          BLENDFUNCTION *pblend,
                          DWORD dwFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetLayeredWindowAttributes(HWND hwnd,
                                 COLORREF *pcrKey,
                                 BYTE *pbAlpha,
                                 DWORD *pdwFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetLayeredWindowAttributes(HWND hwnd,
                                 COLORREF crKey,
                                 BYTE bAlpha,
                                 DWORD dwFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

HWND
APIENTRY
NtUserWindowFromPoint(LONG X,
                      LONG Y)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtUserGetCPD(DWORD Unknown0,
             DWORD Unknown1,
             DWORD Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

HDC
APIENTRY
NtUserGetDC(HWND hWnd)
{
    UNIMPLEMENTED;
    return NULL;
}

HDC
APIENTRY
NtUserGetDCEx(HWND hWnd,
              HANDLE hRegion,
              ULONG Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
APIENTRY
NtUserGetAncestor(HWND hWnd,
                  UINT Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
APIENTRY
NtUserSetParent(HWND hWndChild,
                HWND hWndNewParent)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserGetAltTabInfo(HWND hwnd,
                    INT iItem,
                    PALTTABINFO pati,
                    LPTSTR pszItemText,
                    UINT cchItemText,
                    BOOL Ansi)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetComboBoxInfo(HWND hWnd,
                      PCOMBOBOXINFO pcbi)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserGetListBoxInfo(HWND hWnd)
{
    UNIMPLEMENTED;
    return 0;
}


BOOL
APIENTRY
NtUserGetTitleBarInfo(HWND hwnd,
                      PTITLEBARINFO pti)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserMinMaximize(HWND hWnd,
                  UINT cmd,
                  BOOL Hide)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserMNDragLeave(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserMNDragOver(DWORD Unknown0,
                 DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}
