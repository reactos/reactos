/* $Id: window.c,v 1.2 2001/07/06 00:05:05 rex Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/window.c
 * PURPOSE:         Window management
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */
#include <windows.h>
#include <user32.h>
#include <window.h>
#include <debug.h>


WINBOOL
STDCALL
AdjustWindowRect(
  LPRECT lpRect,
  DWORD dwStyle,
  WINBOOL bMenu)
{
  return FALSE;
}

WINBOOL
STDCALL
AdjustWindowRectEx(
  LPRECT lpRect, 
  DWORD dwStyle, 
  WINBOOL bMenu, 
  DWORD dwExStyle)
{
  return FALSE;
}

WINBOOL
STDCALL
AllowSetForegroundWindow(
  DWORD dwProcessId)
{
  return FALSE;
}

WINBOOL
STDCALL
AnimateWindow(
  HWND hwnd,
  DWORD dwTime,
  DWORD dwFlags)
{
  return FALSE;
}

UINT
STDCALL
ArrangeIconicWindows(
  HWND hWnd)
{
  return 0;
}

HDWP
STDCALL
BeginDeferWindowPos(
  int nNumWindows)
{
  return (HDWP)0;
}

WINBOOL
STDCALL
BringWindowToTop(
  HWND hWnd)
{
  return FALSE;
}

WORD
STDCALL
CascadeWindows(
  HWND hwndParent,
  UINT wHow,
  CONST RECT *lpRect,
  UINT cKids,
  const HWND *lpKids)
{
  return 0;
}

HWND
STDCALL
ChildWindowFromPoint(
  HWND hWndParent,
  POINT Point)
{
  return (HWND)0;
}

HWND
STDCALL
ChildWindowFromPointEx(
  HWND hwndParent,
  POINT pt,
  UINT uFlags)
{
  return (HWND)0;
}

WINBOOL
STDCALL
CloseWindow(
  HWND hWnd)
{
  return FALSE;
}

HWND
STDCALL
CreateWindowExA(
  DWORD dwExStyle,
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
  LPVOID lpParam)
{
  UNICODE_STRING WindowName;
  UNICODE_STRING ClassName;
  HWND Handle;

  if (IS_ATOM(lpClassName)) {
    RtlInitUnicodeString(&ClassName, NULL);
    ClassName.Buffer = (LPWSTR)lpClassName;
  } else {
    if (!RtlCreateUnicodeStringFromAsciiz(&(ClassName), (PCSZ)lpClassName)) {
      SetLastError(ERROR_OUTOFMEMORY);
      return (HWND)0;
    }
  }

  if (!RtlCreateUnicodeStringFromAsciiz(&WindowName, (PCSZ)lpWindowName)) {
    if (!IS_ATOM(lpClassName)) {
      RtlFreeUnicodeString(&ClassName);
    }
    SetLastError(ERROR_OUTOFMEMORY);
    return (HWND)0;
  }

  Handle = NtUserCreateWindowEx(
    dwExStyle,
    &ClassName,
    &WindowName,
    dwStyle,
    x,
    y,
    nWidth,
    nHeight,
    hWndParent,
    hMenu,
    hInstance,
    lpParam,
    0);

  RtlFreeUnicodeString(&WindowName);

  if (!IS_ATOM(lpClassName)) {
    RtlFreeUnicodeString(&ClassName);
  }

  return Handle;
}

HWND
STDCALL
CreateWindowExW(
  DWORD dwExStyle,
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName,
  DWORD dwStyle,
  int x,
  int y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam)
{
  UNICODE_STRING WindowName;
  UNICODE_STRING ClassName;
  HANDLE Handle;

  if (IS_ATOM(lpClassName)) {
    RtlInitUnicodeString(&ClassName, NULL);
    ClassName.Buffer = (LPWSTR)lpClassName;
  } else {
    RtlInitUnicodeString(&ClassName, lpClassName);
  }

  RtlInitUnicodeString(&WindowName, lpWindowName);

  Handle = NtUserCreateWindowEx(
    dwExStyle,
    &ClassName,
    &WindowName,
    dwStyle,
    x,
    y,
    nWidth,
    nHeight,
    hWndParent,
    hMenu,
    hInstance,
    lpParam,
    0);

  return (HWND)Handle;
}

HDWP
STDCALL
DeferWindowPos(
  HDWP hWinPosInfo,
  HWND hWnd,
  HWND hWndInsertAfter,
  int x,
  int y,
  int cx,
  int cy,
  UINT uFlags)
{
  return (HDWP)0;
}

LRESULT
STDCALL
DefWindowProcA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (Msg)
  { 
    case WM_CREATE:
      return 0;

    case WM_DESTROY:
      return 0;

    default:
      return 0;
  }

  return 0;
}

LRESULT
STDCALL
DefWindowProcW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

WINBOOL
STDCALL
DestroyWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
EndDeferWindowPos(
  HDWP hWinPosInfo)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumChildWindows(
  HWND hWndParent,
  ENUMWINDOWSPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumThreadWindows(
  DWORD dwThreadId,
  ENUMWINDOWSPROC lpfn,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumWindows(
  ENUMWINDOWSPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

HWND
STDCALL
FindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName)
{
  //FIXME: FindWindow does not search children, but FindWindowEx does.
  //       what should we do about this?
  return FindWindowExA (NULL, NULL, lpClassName, lpWindowName);
}

HWND
STDCALL
FindWindowExA(
  HWND hwndParent,
  HWND hwndChildAfter,
  LPCSTR lpszClass,
  LPCSTR lpszWindow)
{
  return (HWND)0;
}

HWND
STDCALL
FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
  //FIXME: FindWindow does not search children, but FindWindowEx does.
  //       what should we do about this?
  return FindWindowExW (NULL, NULL, lpClassName, lpWindowName);
}

HWND
STDCALL
FindWindowExW(
  HWND hwndParent,
  HWND hwndChildAfter,
  LPCWSTR lpszClass,
  LPCWSTR lpszWindow)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetAltTabInfo(
  HWND hwnd,
  int iItem,
  PALTTABINFO pati,
  LPTSTR pszItemText,
  UINT cchItemText)
{
  return FALSE;
}

WINBOOL
STDCALL
GetAltTabInfoA(
  HWND hwnd,
  int iItem,
  PALTTABINFO pati,
  LPSTR pszItemText,
  UINT cchItemText)
{
  return FALSE;
}

WINBOOL
STDCALL
GetAltTabInfoW(
  HWND hwnd,
  int iItem,
  PALTTABINFO pati,
  LPWSTR pszItemText,
  UINT cchItemText)
{
  return FALSE;
}

HWND
STDCALL
GetAncestor(
  HWND hwnd,
  UINT gaFlags)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetClientRect(
  HWND hWnd,
  LPRECT lpRect)
{
  return FALSE;
}

HWND
STDCALL
GetDesktopWindow(VOID)
{
  return (HWND)0;
}

HWND
STDCALL
GetForegroundWindow(VOID)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui)
{
  return FALSE;
}

HWND
STDCALL
GetLastActivePopup(
  HWND hWnd)
{
  return (HWND)0;
}

HWND
STDCALL
GetParent(
  HWND hWnd)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetProcessDefaultLayout(
  DWORD *pdwDefaultLayout)
{
  return FALSE;
}

WINBOOL
STDCALL
GetTitleBarInfo(
  HWND hwnd,
  PTITLEBARINFO pti)
{
  return FALSE;
}

HWND
STDCALL
GetTopWindow(
  HWND hWnd)
{
  return (HWND)0;
}

HWND
STDCALL
GetWindow(
  HWND hWnd,
  UINT uCmd)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetWindowInfo(
  HWND hwnd,
  PWINDOWINFO pwi)
{
  return FALSE;
}

UINT
STDCALL
GetWindowModuleFileName(
  HWND hwnd,
  LPSTR lpszFileName,
  UINT cchFileNameMax)
{
  return 0;
}

UINT
STDCALL
GetWindowModuleFileNameA(
  HWND hwnd,
  LPSTR lpszFileName,
  UINT cchFileNameMax)
{
  return 0;
}

UINT
STDCALL
GetWindowModuleFileNameW(
  HWND hwnd,
  LPWSTR lpszFileName,
  UINT cchFileNameMax)
{
  return 0;
}

WINBOOL
STDCALL
GetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl)
{
  return FALSE;
}

WINBOOL
STDCALL
GetWindowRect(
  HWND hWnd,
  LPRECT lpRect)
{
  return FALSE;
}

int
STDCALL
GetWindowTextA(
  HWND hWnd,
  LPSTR lpString,
  int nMaxCount)
{
  return 0;
}

int
STDCALL
GetWindowTextLengthA(
  HWND hWnd)
{
  return 0;
}

int
STDCALL
GetWindowTextLengthW(
  HWND hWnd)
{
  return 0;
}

int
STDCALL
GetWindowTextW(
  HWND hWnd,
  LPWSTR lpString,
  int nMaxCount)
{
  return 0;
}

DWORD
STDCALL
GetWindowThreadProcessId(
  HWND hWnd,
  LPDWORD lpdwProcessId)
{
  return 0;
}

WINBOOL
STDCALL
IsChild(
  HWND hWndParent,
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsIconic(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindowUnicode(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindowVisible(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsZoomed(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
LockSetForegroundWindow(
  UINT uLockCode)
{
  return FALSE;
}

WINBOOL
STDCALL
MoveWindow(
  HWND hWnd,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  WINBOOL bRepaint)
{
  return FALSE;
}

WINBOOL
STDCALL
OpenIcon(
  HWND hWnd)
{
  return FALSE;
}

HWND
STDCALL
RealChildWindowFromPoint(
  HWND hwndParent,
  POINT ptParentClientCoords)
{
  return (HWND)0;
}

UINT
RealGetWindowClass(
  HWND  hwnd,
  LPTSTR pszType,
  UINT  cchType)
{
  return 0;
}

WINBOOL
STDCALL
SetForegroundWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
SetLayeredWindowAttributes(
  HWND hwnd,
  COLORREF crKey,
  BYTE bAlpha,
  DWORD dwFlags)
{
  return FALSE;
}

HWND
STDCALL
SetParent(
  HWND hWndChild,
  HWND hWndNewParent)
{
  return (HWND)0;
}

WINBOOL
STDCALL
SetProcessDefaultLayout(
  DWORD dwDefaultLayout)
{
  return FALSE;
}

WINBOOL
STDCALL
SetWindowPlacement(
  HWND hWnd,
  CONST WINDOWPLACEMENT *lpwndpl)
{
  return FALSE;
}

WINBOOL
STDCALL
SetWindowPos(
  HWND hWnd,
  HWND hWndInsertAfter,
  int X,
  int Y,
  int cx,
  int cy,
  UINT uFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
SetWindowTextA(
  HWND hWnd,
  LPCSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
SetWindowTextW(
  HWND hWnd,
  LPCWSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
ShowOwnedPopups(
  HWND hWnd,
  WINBOOL fShow)
{
  return FALSE;
}

WINBOOL
STDCALL
ShowWindow(
  HWND hWnd,
  int nCmdShow)
{
  return NtUserShowWindow(hWnd, nCmdShow);
}

WINBOOL
STDCALL
ShowWindowAsync(
  HWND hWnd,
  int nCmdShow)
{
  return FALSE;
}

WORD
STDCALL
TileWindows(
  HWND hwndParent,
  UINT wHow,
  CONST RECT *lpRect,
  UINT cKids,
  const HWND *lpKids)
{
  return 0;
}

WINBOOL
STDCALL
UpdateLayeredWindow(
  HWND hwnd,
  HDC hdcDst,
  POINT *pptDst,
  SIZE *psize,
  HDC hdcSrc,
  POINT *pptSrc,
  COLORREF crKey,
  BLENDFUNCTION *pblend,
  DWORD dwFlags)
{
  return FALSE;
}

HWND
STDCALL
WindowFromPoint(
  POINT Point)
{
  return (HWND)0;
}

/* EOF */
