/* $Id: stubs.c,v 1.24 2003/07/05 16:04:01 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/stubs.c
 * PURPOSE:         User32.dll stubs
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      08-05-2001  CSH  Created
 */
#include <windows.h>
#include <debug.h>

WINBOOL
STDCALL
AnyPopup(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
AttachThreadInput(
  DWORD idAttach,
  DWORD idAttachTo,
  WINBOOL fAttach)
{
  UNIMPLEMENTED;
  return FALSE;
}

long
STDCALL
BroadcastSystemMessage(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}

long
STDCALL
BroadcastSystemMessageA(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}

long
STDCALL
BroadcastSystemMessageW(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL
STDCALL
CheckRadioButton(
  HWND hDlg,
  int nIDFirstButton,
  int nIDLastButton,
  int nIDCheckButton)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
ClientToScreen(
  HWND hWnd,
  LPPOINT lpPoint)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
ClipCursor(
  CONST RECT *lpRect)
{
  UNIMPLEMENTED;
  return FALSE;
}

HANDLE
STDCALL
CopyImage(
  HANDLE hImage,
  UINT uType,
  int cxDesired,
  int cyDesired,
  UINT fuFlags)
{
  UNIMPLEMENTED;
  return (HANDLE)0;
}

WINBOOL
STDCALL
DragDetect(
  HWND hwnd,
  POINT pt)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
EnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND
STDCALL
GetActiveWindow(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND
STDCALL
GetCapture(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

UINT
STDCALL
GetDoubleClickTime(VOID)
{
  UNIMPLEMENTED;
  return 0;
}

HWND
STDCALL
GetFocus(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

DWORD
STDCALL
GetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
  UNIMPLEMENTED;
  return 0;
}

int
STDCALL
GetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
STDCALL
GetQueueStatus(
  UINT flags)
{
  UNIMPLEMENTED;
  return 0;
}

HMENU
STDCALL
GetSystemMenu(
  HWND hWnd,
  WINBOOL bRevert)
{
  UNIMPLEMENTED;
  return (HMENU)0;
}

WINBOOL
STDCALL
IsWindowEnabled(
  HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
LockWindowUpdate(
  HWND hWndLock)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
LockWorkStation(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
MessageBeep(
  UINT uType)
{
  UNIMPLEMENTED;
  return FALSE;
}

HMONITOR
STDCALL
MonitorFromPoint(
  POINT pt,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return (HMONITOR)0;
}

HMONITOR
STDCALL
MonitorFromRect(
  LPRECT lprc,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return (HMONITOR)0;
}

HMONITOR
STDCALL
MonitorFromWindow(
  HWND hwnd,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return (HMONITOR)0;
}

DWORD
STDCALL
MsgWaitForMultipleObjects(
  DWORD nCount,
  CONST LPHANDLE pHandles,
  WINBOOL fWaitAll,
  DWORD dwMilliseconds,
  DWORD dwWakeMask)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
STDCALL
MsgWaitForMultipleObjectsEx(
  DWORD nCount,
  CONST HANDLE pHandles,
  DWORD dwMilliseconds,
  DWORD dwWakeMask,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return 0;
}

#if 0
HDEVNOTIFY
STDCALL
RegisterDeviceNotificationA(
  HANDLE hRecipient,
  LPVOID NotificationFilter,
  DWORD Flags)
{
  UNIMPLEMENTED;
  return (HDEVNOTIFY)0;
}

HDEVNOTIFY
STDCALL
RegisterDeviceNotificationW(
  HANDLE hRecipient,
  LPVOID NotificationFilter,
  DWORD Flags)
{
  UNIMPLEMENTED;
  return (HDEVNOTIFY)0;
}
#endif
WINBOOL
STDCALL
RegisterHotKey(
  HWND hWnd,
  int id,
  UINT fsModifiers,
  UINT vk)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
ReleaseCapture(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
ScrollWindow(
  HWND hWnd,
  int XAmount,
  int YAmount,
  CONST RECT *lpRect,
  CONST RECT *lpClipRect)
{
  UNIMPLEMENTED;
  return FALSE;
}

int
STDCALL
ScrollWindowEx(
  HWND hWnd,
  int dx,
  int dy,
  CONST RECT *prcScroll,
  CONST RECT *prcClip,
  HRGN hrgnUpdate,
  LPRECT prcUpdate,
  UINT flags)
{
  UNIMPLEMENTED;
  return 0;
}

HWND
STDCALL
SetActiveWindow(
  HWND hWnd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND
STDCALL
SetCapture(
  HWND hWnd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

WINBOOL
STDCALL
SetDoubleClickTime(
  UINT uInterval)
{
  UNIMPLEMENTED;
  return FALSE;
}

VOID
STDCALL
SetLastErrorEx(
  DWORD dwErrCode,
  DWORD dwType)
{
  UNIMPLEMENTED;
}

WINBOOL
STDCALL
SetSysColors(
  int cElements,
  CONST INT *lpaElements,
  CONST COLORREF *lpaRgbValues)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
SwapMouseButton(
  WINBOOL fSwap)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
TrackMouseEvent(
  LPTRACKMOUSEEVENT lpEventTrack)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
UnregisterDeviceNotification(
  HDEVNOTIFY Handle)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL
STDCALL
UnregisterHotKey(
  HWND hWnd,
  int id)
{
  UNIMPLEMENTED;
  return FALSE;
}

DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
  UNIMPLEMENTED;
  return 0;
}



HHOOK
STDCALL
SetWindowsHookExA(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  UNIMPLEMENTED;
  return 0;
}

HHOOK
STDCALL
SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  UNIMPLEMENTED;
  return 0;
}

VOID
STDCALL
keybd_event(
	    BYTE bVk,
	    BYTE bScan,
	    DWORD dwFlags,
	    DWORD dwExtraInfo)


{
  UNIMPLEMENTED
}

VOID
STDCALL
mouse_event(
	    DWORD dwFlags,
	    DWORD dx,
	    DWORD dy,
	    DWORD cButtons,
	    DWORD dwExtraInfo)
{
  UNIMPLEMENTED
}

/******************************************************************************
 * SetDebugErrorLevel [USER32.@]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 */
VOID
STDCALL
SetDebugErrorLevel( DWORD dwLevel )
{
    DbgPrint("(%ld): stub\n", dwLevel);
}

/* EOF */
