/* $Id: stubs.c,v 1.62 2004/05/02 17:25:20 weiden Exp $
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
#include <mmsystem.h>
#include <user32.h>
#ifdef __USE_W32API
typedef PVOID LPIMEPROW, LPIMEPROA;
#endif

/*
 * @unimplemented
 */
BOOL
STDCALL
AttachThreadInput(
  DWORD idAttach,
  DWORD idAttachTo,
  BOOL fAttach)
{
  UNIMPLEMENTED;
  return FALSE;
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
BOOL
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
BOOL
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
  CONST HANDLE *pHandles,
  BOOL fWaitAll,
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
RealMsgWaitForMultipleObjectsEx(
  DWORD nCount,
  LPHANDLE pHandles,
  DWORD dwMilliseconds,
  DWORD dwWakeMask,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
BOOL
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
BOOL
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
BOOL
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
DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
  UNIMPLEMENTED;
  return 0;
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
 * @implemented
 */
BOOL
STDCALL
EndTask(
	HWND    hWnd,
	BOOL fShutDown,
	BOOL fForce)
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
VOID
STDCALL
SwitchToThisWindow ( HWND hwnd, BOOL fUnknown )
{
  UNIMPLEMENTED;
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
  return NtUserDragObject(hwnd1, hwnd2, u1, dw1, hc1);
}

/*
 * @unimplemented
 */
BOOL
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
BOOL
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
BOOL
STDCALL
GetAccCursorInfo ( PCURSORINFO pci )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ClientThreadSetup ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HDEVNOTIFY
STDCALL
RegisterDeviceNotificationW(
    HANDLE hRecipient,
    LPVOID NotificationFilter,
    DWORD Flags
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
GetRawInputDeviceInfoW(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LONG
STDCALL
BroadcastSystemMessageExW(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LONG
STDCALL
CsrBroadcastSystemMessageExW(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HDEVNOTIFY
STDCALL
RegisterDeviceNotificationA(
    HANDLE hRecipient,
    LPVOID NotificationFilter,
    DWORD Flags
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
GetRawInputDeviceInfoA(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LONG
STDCALL
BroadcastSystemMessageExA(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
DefRawInputProc(
    PRAWINPUT* paRawInput,
    INT nInput,
    UINT cbSizeHeader)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL 
DisableProcessWindowsGhosting(VOID)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputBuffer(
    PRAWINPUT   pData,
    PUINT    pcbSize,
    UINT         cbSizeHeader)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputData(
    HRAWINPUT    hRawInput,
    UINT         uiCommand,
    LPVOID      pData,
    PUINT    pcbSize,
    UINT         cbSizeHeader)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsGUIThread(
    BOOL bConvert)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
PrintWindow(
    HWND hwnd,
    HDC hdcBlt,
    UINT nFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterRawInputDevices(
    PCRAWINPUTDEVICE pRawInputDevices,
    UINT uiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
WINNLSGetIMEHotkey( HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
WINNLSEnableIME( HWND hwnd, BOOL enable)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
WINNLSGetEnableStatus( HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPSetIMEW( HWND hwnd, LPIMEPROW ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPQueryIMEW( LPIMEPROW ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPGetIMEW( HWND hwnd, LPIMEPROW ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPSetIMEA( HWND hwnd, LPIMEPROA ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPQueryIMEA( LPIMEPROA ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPGetIMEA( HWND hwnd, LPIMEPROA ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT 
STDCALL
SendIMEMessageExW(HWND hwnd,LPARAM lparam)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT 
STDCALL
SendIMEMessageExA(HWND hwnd, LPARAM lparam)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL DisplayExitWindowsWarnings(ULONG flags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL ReasonCodeNeedsBugID(ULONG reasoncode)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL ReasonCodeNeedsComment(ULONG reasoncode)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL CtxInitUser32(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL EnterReaderModeHelper(HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetAppCompatFlags2(HTASK hTask)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID STDCALL InitializeLpkHooks(FARPROC *hookfuncs)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
WORD STDCALL InitializeWin32EntryTable(UCHAR* EntryTablePlus0x1000)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL IsServerSideWindow(HWND wnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

typedef BOOL (CALLBACK *THEME_HOOK_FUNC) (DWORD state,PVOID arg2); //return type and 2nd parameter unknown
/*
 * @unimplemented
 */
BOOL STDCALL RegisterUserApiHook(HINSTANCE instance,THEME_HOOK_FUNC proc)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL UnregisterUserApiHook(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HKL STDCALL LoadKeyboardLayoutEx(DWORD unknown,LPCWSTR pwszKLID,UINT Flags) //1st parameter unknown
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID STDCALL AllowForegroundActivation(VOID)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID STDCALL ShowStartGlass(DWORD unknown)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdeGetQualityOfService(HWND hWnd, DWORD Reserved, PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  UNIMPLEMENTED;
  return FALSE;
}
