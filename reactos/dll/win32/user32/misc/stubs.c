/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/stubs.c
 * PURPOSE:         User32.dll stubs
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      08-F05-2001  CSH  Created
 */

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

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
DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
// Need to call NtQueryInformationProcess and send ProcessId not hProcess.
  return NtUserWaitForInputIdle(hProcess, dwMilliseconds, FALSE);
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


/*
 * @implemented
 */
DWORD
STDCALL
GetAppCompatFlags(HTASK hTask)
{
    PCLIENTINFO pci = GetWin32ClientInfo();

    return pci->dwCompatFlags;
}

/*
 * @implemented
 */
DWORD
STDCALL
GetAppCompatFlags2(HTASK hTask)
{
    PCLIENTINFO pci = GetWin32ClientInfo();

    return pci->dwCompatFlags2;
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
 * @implemented
 */
UINT
STDCALL
UserRealizePalette ( HDC hDC )
{
  return NtUserCallOneParam((DWORD) hDC, ONEPARAM_ROUTINE_REALIZEPALETTE);
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
SetSysColorsTemp(
		 const COLORREF *pPens,
		 const HBRUSH   *pBrushes,
		 INT            n
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


/*
 * @unimplemented
 */
DWORD STDCALL User32InitializeImmEntryTable(PVOID p)
{
  UNIMPLEMENTED;
  return 0;
}

