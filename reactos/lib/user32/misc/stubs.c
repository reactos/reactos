/* $Id: stubs.c,v 1.39 2003/08/28 16:33:22 weiden Exp $
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
#include <string.h>
typedef UINT *LPUINT;
#include <mmsystem.h>


/*
 * @unimplemented
 */
WINBOOL
STDCALL
AnyPopup(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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




/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HWND
STDCALL
GetFocus(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
WINBOOL
STDCALL
LockWindowUpdate(
  HWND hWndLock)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
LockWorkStation(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HMONITOR
STDCALL
MonitorFromPoint(
  POINT pt,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return (HMONITOR)0;
}


/*
 * @unimplemented
 */
HMONITOR
STDCALL
MonitorFromRect(
  LPRECT lprc,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return (HMONITOR)0;
}


/*
 * @unimplemented
 */
HMONITOR
STDCALL
MonitorFromWindow(
  HWND hwnd,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return (HMONITOR)0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
WINBOOL
STDCALL
TrackMouseEvent(
  LPTRACKMOUSEEVENT lpEventTrack)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnregisterDeviceNotification(
  HDEVNOTIFY Handle)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnregisterHotKey(
  HWND hWnd,
  int id)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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
 *
 * @unimplemented
 */
VOID
STDCALL
SetDebugErrorLevel( DWORD dwLevel )
{
    DbgPrint("(%ld): stub\n", dwLevel);
}

/* EOF */

/*
 * @unimplemented
 */
WINBOOL
STDCALL
ChangeMenuW(
    HMENU hMenu,
    UINT cmd,
    LPCWSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
ChangeMenuA(
    HMENU hMenu,
    UINT cmd,
    LPCSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
DeregisterShellHookWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
RegisterShellHookWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @implemented
 */
WINBOOL
STDCALL
EndTask(
	HWND    hWnd,
	WINBOOL fShutDown,
	WINBOOL fForce)
{
    SendMessageW(hWnd, WM_CLOSE, 0, 0);
    
    if (IsWindow(hWnd))
    {
        if (fForce)
            return DestroyWindow(hWnd);
        else
            return FALSE;
    }
    
    return TRUE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
PrivateExtractIconsW(
		     LPCWSTR szFileName,
		     int     nIconIndex,
		     int     cxIcon,
		     int     cyIcon,
		     HICON  *phicon,
		     UINT   *piconid,
		     UINT    nIcons,
		     UINT    flags
		     )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
PrivateExtractIconsA(
		     LPCSTR szFileName,
		     int    nIconIndex,
		     int    cxIcon,
		     int    cyIcon,
		     HICON *phicon,
		     UINT  *piconid,
		     UINT   nIcons,
		     UINT   flags
		     )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
MenuWindowProcA(
		HWND   hWnd,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
MenuWindowProcW(
		HWND   hWnd,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
DrawCaptionTempW(
		 HWND        hwnd,
		 HDC         hdc,
		 const RECT *rect,
		 HFONT       hFont,
		 HICON       hIcon,
		 LPCWSTR     str,
		 UINT        uFlags
		 )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
DrawCaptionTempA(
		 HWND        hwnd,
		 HDC         hdc,
		 const RECT *rect,
		 HFONT       hFont,
		 HICON       hIcon,
		 LPCSTR      str,
		 UINT        uFlags
		 )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookW ( int idHook, HOOKPROC lpfn )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookA ( int idHook, HOOKPROC lpfn )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HRESULT
STDCALL
PrivateExtractIconExW(
		      DWORD u,
		      DWORD v,
		      DWORD w,
		      DWORD x,
		      DWORD y
		      )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HRESULT
STDCALL
PrivateExtractIconExA(
		      DWORD u,
		      DWORD v,
		      DWORD w,
		      DWORD x,
		      DWORD y
		      )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
NotifyWinEvent(
	       DWORD event,
	       HWND  hwnd,
	       LONG  idObject,
	       LONG  idChild
	       )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
HWINEVENTHOOK
STDCALL
SetWinEventHook(
		DWORD        eventMin,
		DWORD        eventMax,
		HMODULE      hmodWinEventProc,
		WINEVENTPROC pfnWinEventProc,
		DWORD        idProcess,
		DWORD        idThread,
		DWORD        dwFlags
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SwitchToThisWindow ( HWND hwnd, WINBOOL fUnknown )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnhookWinEvent ( HWINEVENTHOOK hWinEventHook )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
EditWndProc(
	    HWND   hWnd,
	    UINT   Msg,
	    WPARAM wParam,
	    LPARAM lParam
	    )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetAppCompatFlags ( HTASK hTask )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetInternalWindowPos(
		     HWND hwnd,
		     LPRECT rectWnd,
		     LPPOINT ptIcon
		     )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HRESULT
STDCALL
GetProgmanWindow ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HRESULT
STDCALL
GetTaskmanWindow ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
SetWindowStationUser ( DWORD x1, DWORD x2 )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
SetSystemTimer(
	       HWND      hwnd,
	       UINT      id,
	       UINT      timeout,
	       TIMERPROC proc
	       )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HRESULT
STDCALL
SetTaskmanWindow ( DWORD x )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HRESULT
STDCALL
SetProgmanWindow ( DWORD x )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ScrollChildren(
	       HWND   hWnd,
	       UINT   uMsg,
	       WPARAM wParam,
	       LPARAM lParam
	       )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
LoadLocalFonts ( VOID )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
LoadRemoteFonts ( VOID )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SetInternalWindowPos(
		     HWND    hwnd,
		     UINT    showCmd,
		     LPRECT  rect,
		     LPPOINT pt
		     )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
RegisterSystemThread ( DWORD flags, DWORD reserved )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
RegisterLogonProcess ( HANDLE hprocess, BOOL x )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
KillSystemTimer ( HWND hwnd, UINT id )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
RegisterTasklist ( DWORD x )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
SetLogonNotifyWindow ( HWINSTA hwinsta, HWND hwnd )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
DragObject(
	   HWND    hwnd1,
	   HWND    hwnd2,
	   UINT    u1,
	   DWORD   dw1,
	   HCURSOR hc1
	   )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetUserObjectSecurity(
		      HANDLE                hObj,
		      PSECURITY_INFORMATION pSIRequested,
		      PSECURITY_DESCRIPTOR  pSID,
		      DWORD                 nLength,
		      LPDWORD               lpnLengthNeeded
		      )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetUserObjectSecurity(
		      HANDLE                hObj,
		      PSECURITY_INFORMATION pSIRequested,
		      PSECURITY_DESCRIPTOR  pSID
		      )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnhookWindowsHook ( int nCode, HOOKPROC pfnFilterProc )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
UserRealizePalette ( HDC hDC )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
DrawMenuBarTemp(
		HWND   hwnd,
		HDC    hDC,
		LPRECT lprect,
		HMENU  hMenu,
		HFONT  hFont
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
SetSysColorsTemp(
		 const COLORREF *pPens,
		 const HBRUSH   *pBrushes,
		 DWORD           n
		 )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WORD
STDCALL
CascadeChildWindows ( HWND hWndParent, WORD wFlags )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WORD
STDCALL
TileChildWindows ( HWND hWndParent, WORD wFlags )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HDESK
STDCALL
GetInputDesktop ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
LockWindowStation ( HWINSTA hWinSta )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnlockWindowStation ( HWINSTA hWinSta )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetAccCursorInfo ( PCURSORINFO pci )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
ClientThreadSetup ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}
