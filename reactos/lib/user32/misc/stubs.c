/* $Id: stubs.c,v 1.14 2002/09/08 10:23:10 chorns Exp $
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

WINBOOL STDCALL 
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
}

WINBOOL
STDCALL
AnyPopup(VOID)
{
  return FALSE;
}

WINBOOL
STDCALL
AttachThreadInput(
  DWORD idAttach,
  DWORD idAttachTo,
  WINBOOL fAttach)
{
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
  return FALSE;
}

WINBOOL
STDCALL
ClientToScreen(
  HWND hWnd,
  LPPOINT lpPoint)
{
  return FALSE;
}

WINBOOL
STDCALL
ClipCursor(
  CONST RECT *lpRect)
{
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
  return (HANDLE)0;
}




WINBOOL
STDCALL
DragDetect(
  HWND hwnd,
  POINT pt)
{
  return FALSE;
}



WINBOOL
STDCALL
EnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows)
{
  return FALSE;
}

WINBOOL
STDCALL
ExitWindowsEx(
  UINT uFlags,
  DWORD dwReserved)
{
  return FALSE;
}




HWND
STDCALL
GetActiveWindow(VOID)
{
  return (HWND)0;
}

HWND
STDCALL
GetCapture(VOID)
{
  return (HWND)0;
}


UINT
STDCALL
GetDoubleClickTime(VOID)
{
  return 0;
}

HWND
STDCALL
GetFocus(VOID)
{
  return (HWND)0;
}

DWORD
STDCALL
GetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
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
  return 0;
}




DWORD
STDCALL
GetQueueStatus(
  UINT flags)
{
  return 0;
}


DWORD
STDCALL
GetSysColor(
  int nIndex)
{
  return 0;
}

HMENU
STDCALL
GetSystemMenu(
  HWND hWnd,
  WINBOOL bRevert)
{
  return (HMENU)0;
}


















WINBOOL
STDCALL
IsWindowEnabled(
  HWND hWnd)
{
  return FALSE;
}







int
STDCALL
LoadStringW(
  HINSTANCE hInstance,
  UINT uID,
  LPWSTR lpBuffer,
  int nBufferMax)
{
  return 0;
}

WINBOOL
STDCALL
LockWindowUpdate(
  HWND hWndLock)
{
  return FALSE;
}

WINBOOL
STDCALL
LockWorkStation(VOID)
{
  return FALSE;
}



int
STDCALL
MapWindowPoints(
  HWND hWndFrom,
  HWND hWndTo,
  LPPOINT lpPoints,
  UINT cPoints)
{
  return 0;
}

WINBOOL
STDCALL
MessageBeep(
  UINT uType)
{
  return FALSE;
}



HMONITOR
STDCALL
MonitorFromPoint(
  POINT pt,
  DWORD dwFlags)
{
  return (HMONITOR)0;
}

HMONITOR
STDCALL
MonitorFromRect(
  LPRECT lprc,
  DWORD dwFlags)
{
  return (HMONITOR)0;
}

HMONITOR
STDCALL
MonitorFromWindow(
  HWND hwnd,
  DWORD dwFlags)
{
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
  return (HDEVNOTIFY)0;
}

HDEVNOTIFY
STDCALL
RegisterDeviceNotificationW(
  HANDLE hRecipient,
  LPVOID NotificationFilter,
  DWORD Flags)
{
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
  return FALSE;
}


WINBOOL
STDCALL
ReleaseCapture(VOID)
{
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
  return 0;
}



HWND
STDCALL
SetActiveWindow(
  HWND hWnd)
{
  return (HWND)0;
}

HWND
STDCALL
SetCapture(
  HWND hWnd)
{
  return (HWND)0;
}





WINBOOL
STDCALL
SetDoubleClickTime(
  UINT uInterval)
{
  return FALSE;
}

HWND
STDCALL
SetFocus(
  HWND hWnd)
{
  return (HWND)0;
}
VOID
STDCALL
SetLastErrorEx(
  DWORD dwErrCode,
  DWORD dwType)
{
}





WINBOOL
STDCALL
SetSysColors(
  int cElements,
  CONST INT *lpaElements,
  CONST COLORREF *lpaRgbValues)
{
  return FALSE;
}








WINBOOL
STDCALL
SwapMouseButton(
  WINBOOL fSwap)
{
  return FALSE;
}

WINBOOL
STDCALL
TrackMouseEvent(
  LPTRACKMOUSEEVENT lpEventTrack)
{
  return FALSE;
}







WINBOOL
STDCALL
UnregisterDeviceNotification(
  HDEVNOTIFY Handle)
{
  return FALSE;
}

WINBOOL
STDCALL
UnregisterHotKey(
  HWND hWnd,
  int id)
{
  return FALSE;
}





DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
  return 0;
}


/* EOF */
