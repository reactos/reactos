/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/usrstubs.c
 * PURPOSE:         Syscall stubs
 * PROGRAMMERS:     Olaf Siejka
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
NTAPI
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserDestroyMenu(
  HMENU hMenu)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserDrawMenuBarTemp(
  HWND hWnd,
  HDC hDC,
  PRECT hRect,
  HMENU hMenu,
  HFONT hFont)
{
    UNIMPLEMENTED;
	return 0;
}

UINT
NTAPI
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserEndMenu(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi)
{
    UNIMPLEMENTED;
	return FALSE;
}

UINT
NTAPI
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
    UNIMPLEMENTED;
	return FALSE;
}

HMENU
NTAPI
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
    UNIMPLEMENTED;
	return FALSE;
}

INT
NTAPI
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetMenuFlagRtoL(
  HMENU hMenu)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetSystemMenu(
  HWND hWnd,
  HMENU hMenu)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  INT x,
  INT y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
    UNIMPLEMENTED;
	return FALSE;
}

HKL
NTAPI
NtUserActivateKeyboardLayout(
  HKL hKl,
  ULONG Flags)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserAlterWindowStyle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserAttachThreadInput(
  IN DWORD idAttach,
  IN DWORD idAttachTo,
  IN BOOL fAttach)
{
    UNIMPLEMENTED;
	return FALSE;
}

HDC NTAPI
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserBitBltSysBmp(
  HDC hdc,
  INT nXDest,
  INT nYDest,
  INT nWidth,
  INT nHeight,
  INT nXSrc,
  INT nYSrc,
  DWORD dwRop )
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserBlockInput(
  BOOL BlockIt)
{
    UNIMPLEMENTED;
	return FALSE;
}

NTSTATUS
NTAPI
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG* pBufSize)
{
    UNIMPLEMENTED;
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS NTAPI
NtUserBuildNameList(
   HWINSTA hWinSta,
   ULONG dwSize,
   PVOID lpBuffer,
   PULONG pRequiredSize)
{
    UNIMPLEMENTED;
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtUserBuildPropList(
  HWND hWnd,
  LPVOID Buffer,
  DWORD BufferSize,
  DWORD *Count)
{
    UNIMPLEMENTED;
	return STATUS_UNSUCCESSFUL;
}

DWORD
NTAPI
NtUserCallHwnd(
  HWND hWnd,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return FALSE;
}

HWND
NTAPI
NtUserCallHwndOpt(
  HWND hWnd,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserCallHwndParam(
  HWND hWnd,
  DWORD Param,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCallHwndParamLock(
  HWND hWnd,
  DWORD Param,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserCallMsgFilter(
  LPMSG msg,
  INT code)
{
    UNIMPLEMENTED;
	return FALSE;
}

LRESULT
NTAPI
NtUserCallNextHookEx(
  HHOOK Hook,
  INT Code,
  WPARAM wParam,
  LPARAM lParam)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCallNoParam(
  DWORD Routine)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext)
{
    UNIMPLEMENTED;
	return FALSE;
}

LONG
NTAPI
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
STDCALL
NtUserCheckImeHotKey(
  DWORD dwUnknown1,
  DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

HWND NTAPI
NtUserChildWindowFromPointEx(
  HWND Parent,
  LONG x,
  LONG y,
  UINT Flags)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserClipCursor(
    RECT *lpRect)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserCloseClipboard(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserCloseDesktop(
  HDESK hDesktop)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserCloseWindowStation(
  HWINSTA hWinSta)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserConsoleControl(
  DWORD dwUnknown1,
  DWORD dwUnknown2,
  DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

INT
NTAPI
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  INT EntriesCount)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCountClipboardFormats(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

HACCEL
NTAPI
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  INT nWidth,
  INT nHeight)
{
    UNIMPLEMENTED;
	return FALSE;
}

HDESK
NTAPI
NtUserCreateDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserCreateInputContext(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
    UNIMPLEMENTED;
	return 0;
}

HWND
NTAPI
NtUserCreateWindowEx(
  DWORD dwExStyle,
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

HWINSTA
NTAPI
NtUserCreateWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserDdeGetQualityOfService(
  IN HWND hwndClient,
  IN HWND hWndServer,
  OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserDdeSetQualityOfService(
  IN  HWND hwndClient,
  IN  PSECURITY_QUALITY_OF_SERVICE pqosNew,
  OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
    UNIMPLEMENTED;
	return FALSE;
}

HDWP NTAPI
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
BOOL NTAPI
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOLEAN
NTAPI
NtUserDestroyAcceleratorTable(
  HACCEL Table)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserDestroyCursor(
  HANDLE Handle,
  DWORD Unknown)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserDestroyInputContext(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
	return 0;
}

BOOLEAN NTAPI
NtUserDestroyWindow(HWND Wnd)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserDisableThreadIme(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
	return 0;
}

LRESULT
NTAPI
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO MsgInfo)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserDragDetect(
  HWND hWnd,
  POINT pt)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserDrawAnimatedRects(
  HWND hwnd,
  INT idAni,
  RECT *lprcFrom,
  RECT *lprcTo)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserDrawCaption(
   HWND hWnd,
   HDC hDc,
   LPCRECT lpRc,
   UINT uFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
STDCALL
NtUserDrawCaptionTemp(
  HWND hWnd,
  HDC hDC,
  LPCRECT lpRc,
  HFONT hFont,
  HICON hIcon,
  const PUNICODE_STRING str,
  UINT uFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserDrawIconEx(
  HDC hdc,
  int xLeft,
  int yTop,
  HICON hIcon,
  int cxWidth,
  int cyWidth,
  UINT istepIfAniCur,
  HBRUSH hbrFlickerFreeDraw,
  UINT diFlags,
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserEmptyClipboard(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserEnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL NTAPI
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICEW lpDisplayDevice, /* device information */
  DWORD dwFlags ) /* reserved */
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserEnumDisplayMonitors (
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData )
{
    UNIMPLEMENTED;
	return FALSE;
}

NTSTATUS
NTAPI
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags )
{
    UNIMPLEMENTED;
	return STATUS_UNSUCCESSFUL;
}

DWORD
NTAPI
NtUserEvent(
  DWORD Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserFillWindow(
  HWND hWndPaint,
  HWND hWndPaint1,
  HDC  hDC,
  HBRUSH hBrush)
{
    UNIMPLEMENTED;
	return FALSE;
}

HICON
NTAPI
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  LONG cx,
  LONG cy) 
{
    UNIMPLEMENTED;
	return NULL;
}

HWND
NTAPI
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING  ucClassName,
  PUNICODE_STRING  ucWindowName,
  DWORD dwUnknown
  )
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserFlashWindowEx(
  IN PFLASHWINFO pfwi)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserGetAltTabInfo(
   HWND hwnd,
   INT  iItem,
   PALTTABINFO pati,
   LPTSTR pszItemText,
   UINT   cchItemText,
   BOOL   Ansi)
{
    UNIMPLEMENTED;
	return FALSE;
}

HWND NTAPI
NtUserGetAncestor(HWND hWnd, UINT Flags)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserGetAppImeLevel(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
	return 0;
}

SHORT
NTAPI
NtUserGetAsyncKeyState(
  INT Key)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetAtomName(
    ATOM nAtom,
    LPWSTR lpBuffer)
{
    UNIMPLEMENTED;
	return 0;
}

UINT
NTAPI
NtUserGetCaretBlinkTime(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetCaretPos(
  LPPOINT lpPoint)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL NTAPI
NtUserGetClassInfo(HINSTANCE hInstance,
		   PUNICODE_STRING ClassName,
		   LPWNDCLASSEXW wcex,
		   LPWSTR *ppszMenuName,
		   BOOL Ansi)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
NTAPI
NtUserGetClassInfoEx(DWORD Param1, DWORD Param2,
                     DWORD Param3, DWORD Param4,
                     DWORD Param5)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
NTAPI
NtUserGetClassName(HWND hWnd,
                   BOOL Real, // 0 GetClassNameW, 1 RealGetWindowClassA/W
                   PUNICODE_STRING ClassName)
{
    UNIMPLEMENTED;
	return 0;
}

				   
HANDLE
NTAPI
NtUserGetClipboardData(
  UINT uFormat,
  PVOID pBuffer)
{
    UNIMPLEMENTED;
	return NULL;
}

INT
NTAPI
NtUserGetClipboardFormatName(
  UINT format,
  PUNICODE_STRING FormatName,
  INT cchMaxCount)
{
    UNIMPLEMENTED;
	return 0;
}

HWND
NTAPI
NtUserGetClipboardOwner(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserGetClipboardSequenceNumber(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

HWND
NTAPI
NtUserGetClipboardViewer(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserGetClipCursor(
  RECT *lpRect)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserGetComboBoxInfo(
  HWND hWnd,
  PCOMBOBOXINFO pcbi)
{
    UNIMPLEMENTED;
	return FALSE;
}

HBRUSH
NTAPI
NtUserGetControlBrush(
  HWND hwnd,
  HDC  hdc,
  UINT ctlType)
{
    UNIMPLEMENTED;
	return NULL;
}

HBRUSH
NTAPI
NtUserGetControlColor(
   HWND hwndParent,
   HWND hwnd,
   HDC hdc,
   UINT CtlMsg)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetCursorInfo(
  PCURSORINFO pci)
{
    UNIMPLEMENTED;
	return FALSE;
}

HDC
NTAPI
NtUserGetDC(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return NULL;
}

HDC
NTAPI
NtUserGetDCEx(
  HWND hWnd,
  HANDLE hRegion,
  ULONG Flags)
{
    UNIMPLEMENTED;
	return NULL;
}

UINT
NTAPI
NtUserGetDoubleClickTime(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

HWND
NTAPI
NtUserGetForegroundWindow(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserGetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserGetIconInfo(
   HANDLE hCurIcon,
   PICONINFO IconInfo,
   PUNICODE_STRING lpInstName,
   PUNICODE_STRING lpResName,
   LPDWORD pbpp,
   BOOL bInternal)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserGetIconSize(
    HANDLE Handle,
    UINT istepIfAniCur,
    LONG  *plcx,
    LONG  *plcy)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetInternalWindowPos(
  HWND hwnd,
  LPRECT rectWnd,
  LPPOINT ptIcon)
{
    UNIMPLEMENTED;
	return 0;
}

HKL
NTAPI
NtUserGetKeyboardLayout(
  DWORD dwThreadid)
{
    UNIMPLEMENTED;
	return NULL;
}

UINT
NTAPI
NtUserGetKeyboardLayoutList(
  INT nItems,
  HKL *pHklBuff)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetKeyboardLayoutName(
  LPWSTR lpszName)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserGetKeyboardState(
  LPBYTE Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetKeyboardType(
  DWORD TypeFlag)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize )
{
    UNIMPLEMENTED;
	return 0;
}

SHORT
NTAPI
NtUserGetKeyState(
  INT VirtKey)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserGetListBoxInfo(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserGetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength,
  PDWORD nLengthNeeded)
{
    UNIMPLEMENTED;
	return FALSE;
}

HWND
NTAPI
NtUserGetOpenClipboardWindow(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

INT
NTAPI
NtUserGetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  INT cFormats)
{
    UNIMPLEMENTED;
	return 0;
}

HWINSTA
NTAPI
NtUserGetProcessWindowStation(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetRawInputData(
  HRAWINPUT hRawInput,
  UINT uiCommand,
  LPVOID pData,
  PUINT pcbSize,
  UINT cbSizeHeader)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserGetScrollBarInfo(
  HWND hWnd,
  LONG idObject,
  PSCROLLBARINFO psbi)
{
    UNIMPLEMENTED;
	return FALSE;
}

HDESK
NTAPI
NtUserGetThreadDesktop(
  DWORD dwThreadId,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOLEAN
NTAPI
NtUserGetTitleBarInfo(
  HWND hwnd,
  PTITLEBARINFO pti)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL NTAPI
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase)
{
    UNIMPLEMENTED;
	return 0;
}

int
NTAPI
NtUserGetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase)
{
    UNIMPLEMENTED;
	return 0;
}

HDC
NTAPI
NtUserGetWindowDC(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserGetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserImpersonateDdeClientWindow(
  HWND hWndClient,
  HWND hWndServer)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserInitialize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

NTSTATUS
NTAPI
NtUserInitializeClientPfnArrays(
  PPFNCLIENT pfnClientA, 
  PPFNCLIENT pfnClientW,
  PPFNCLIENTWORKER pfnClientWorker,
  HINSTANCE hmodUser)
{
    UNIMPLEMENTED;
	return STATUS_UNSUCCESSFUL;
}

DWORD
NTAPI
NtUserInitTask(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7,
  DWORD Unknown8,
  DWORD Unknown9,
  DWORD Unknown10,
  DWORD Unknown11)
{
    UNIMPLEMENTED;
	return 0;
}

INT
NTAPI
NtUserInternalGetWindowText(
  HWND hWnd,
  LPWSTR lpString,
  INT nMaxCount)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserInvalidateRect(
    HWND hWnd,
    CONST RECT *lpRect,
    BOOL bErase)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserIsClipboardFormatAvailable(
  UINT format)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
    UNIMPLEMENTED;
	return FALSE;
}

HKL
STDCALL
NtUserLoadKeyboardLayoutEx(
   IN HANDLE Handle,
   IN DWORD offTable,
   IN PUNICODE_STRING puszKeyboardName,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserLockWindowStation(
  HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserLockWindowUpdate(
  DWORD Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserLockWorkStation(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

UINT
NTAPI
NtUserMapVirtualKeyEx( UINT keyCode,
		       UINT transType,
		       DWORD keyboardId,
		       HKL dwhkl )
{
    UNIMPLEMENTED;
	return 0;
}

LRESULT
NTAPI
NtUserMessageCall(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  ULONG_PTR ResultInfo,
  DWORD dwType, // FNID_XX types
  BOOL Ansi)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserMNDragLeave(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserMoveWindow(
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint
)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserNotifyProcessCreate(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
	return 0;
}

VOID
NTAPI
NtUserNotifyWinEvent(
  DWORD Event,
  HWND  hWnd,
  LONG  idObject,
  LONG  idChild)
{
    UNIMPLEMENTED;
}

BOOL
NTAPI
NtUserOpenClipboard(
  HWND hWnd,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return FALSE;
}

HDESK
NTAPI
NtUserOpenDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess)
{
    UNIMPLEMENTED;
	return NULL;
}

HDESK
NTAPI
NtUserOpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
    UNIMPLEMENTED;
	return NULL;
}

HWINSTA
NTAPI
NtUserOpenWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserPaintDesktop(
  HDC hDC)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserPeekMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserPostThreadMessage(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserPrintWindow(
    HWND hwnd,
    HDC  hdcBlt,
    UINT nFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserProcessConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserQueryInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserQuerySendMessage(
  DWORD Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserQueryUserCounters(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserQueryWindow(
  HWND hWnd,
  DWORD Index)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRealInternalGetMessage(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRealChildWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
)
{
    UNIMPLEMENTED;
	return FALSE;
}

RTL_ATOM
NTAPI
NtUserRegisterClassExWOW(
    WNDCLASSEXW* lpwcx,
    PUNICODE_STRING pustrClassName,
    PUNICODE_STRING pustrCNVersion,
    PCLSMENUNAME pClassMenuName,
    DWORD fnID,
    DWORD Flags,
    LPDWORD pWow)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserRegisterTasklist(
  DWORD Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

UINT NTAPI
NtUserRegisterWindowMessage(PUNICODE_STRING MessageName)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRemoteRedrawScreen(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

HANDLE NTAPI
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserResolveDesktopForWOW(
  DWORD Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip ,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD NTAPI
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags)
{
    UNIMPLEMENTED;
	return 0;
}

UINT
NTAPI
NtUserSendInput(
  UINT nInputs,
  LPINPUT pInput,
  INT cbSize)
{
    UNIMPLEMENTED;
	return 0;
}

HWND NTAPI
NtUserSetActiveWindow(HWND Wnd)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

HWND NTAPI
NtUserSetCapture(HWND Wnd)
{
    UNIMPLEMENTED;
	return NULL;
}

ULONG_PTR NTAPI
NtUserSetClassLong(
  HWND  hWnd,
  INT Offset,
  ULONG_PTR  dwNewLong,
  BOOL  Ansi )
{
    UNIMPLEMENTED;
	return 0;
}

WORD
NTAPI
NtUserSetClassWord(
  HWND hWnd,
  INT nIndex,
  WORD wNewWord)
{
    UNIMPLEMENTED;
	return 0;
}

HANDLE
NTAPI
NtUserSetClipboardData(
  UINT uFormat,
  HANDLE hMem,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return NULL;
}

HWND
NTAPI
NtUserSetClipboardViewer(
  HWND hWndNewViewer)
{
    UNIMPLEMENTED;
	return NULL;
}

HPALETTE
STDCALL
NtUserSelectPalette(
    HDC hDC,
    HPALETTE  hpal,
    BOOL  ForceBackground
)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

HCURSOR
NTAPI
NtUserSetCursor(
  HCURSOR hCursor)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserSetCursorContents(
  HANDLE Handle,
  PICONINFO IconInfo)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetCursorIconData(
  HANDLE Handle,
  PBOOL fIcon,
  POINT *Hotspot,
  HMODULE hModule,
  HRSRC hRsrc,
  HRSRC hGroupRsrc)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

HWND
NTAPI
NtUserSetFocus(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetImeInfoEx(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetImeOwnerWindow(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetInternalWindowPos(
  HWND    hwnd,
  UINT    showCmd,
  LPRECT  rect,
  LPPOINT pt)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserSetKeyboardState(
  LPBYTE lpKeyState)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetLayeredWindowAttributes(
  HWND hwnd,
  COLORREF crKey,
  BYTE bAlpha,
  DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetLogonNotifyWindow(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength)
{
    UNIMPLEMENTED;
	return FALSE;
}

HWND
NTAPI
NtUserSetParent(
  HWND hWndChild,
  HWND hWndNewParent)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserSetProcessWindowStation(
  HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL NTAPI
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetScrollInfo(
  HWND hwnd,
  int fnBar,
  LPCSCROLLINFO lpsi,
  BOOL bRedraw)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserSetShellWindowEx(
  HWND hwndShell,
  HWND hwndShellListView)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetSysColors(
  int cElements,
  IN CONST INT *lpaElements,
  IN CONST COLORREF *lpaRgbValues,
  FLONG Flags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSetThreadDesktop(
  HDESK hDesktop)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1)
{
    UNIMPLEMENTED;
	return 0;
}

UINT_PTR
NTAPI
NtUserSetSystemTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return 0;
}

UINT_PTR
NTAPI
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserSetWindowFNID(
  HWND hWnd,
  WORD fnID)
{
    UNIMPLEMENTED;
	return FALSE;
}

LONG
NTAPI
NtUserSetWindowLong(
  HWND hWnd,
  DWORD Index,
  LONG NewValue,
  BOOL Ansi)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserSetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI NtUserSetWindowPos(
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags
)
{
    UNIMPLEMENTED;
	return FALSE;
}

INT
NTAPI
NtUserSetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw)
{
    UNIMPLEMENTED;
	return 0;
}

HHOOK
NTAPI
NtUserSetWindowsHookAW(
  int idHook,
  HOOKPROC lpfn,
  BOOL Ansi)
{
    UNIMPLEMENTED;
	return NULL;
}

HHOOK
NTAPI
NtUserSetWindowsHookEx(
  HINSTANCE Mod,
  PUNICODE_STRING ModuleName,
  DWORD ThreadId,
  int HookId,
  HOOKPROC HookProc,
  BOOL Ansi)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserSetWindowStationUser(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
    UNIMPLEMENTED;
	return 0;
}

WORD NTAPI
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal)
{
    UNIMPLEMENTED;
	return 0;
}

HWINEVENTHOOK
NTAPI
NtUserSetWinEventHook(
  UINT eventMin,
  UINT eventMax,
  HMODULE hmodWinEventProc,
  PUNICODE_STRING puString,
  WINEVENTPROC lpfnWinEventProc,
  DWORD idProcess,
  DWORD idThread,
  UINT dwflags)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
NTAPI
NtUserShowCaret(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserHideCaret(
  HWND hWnd)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserShowWindow(
  HWND hWnd,
  LONG nCmdShow)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserShowWindowAsync(
  HWND hWnd,
  LONG nCmdShow)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSoundSentry(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSwitchDesktop(
  HDESK hDesktop)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
	return 0;
}

INT
NTAPI
NtUserToUnicodeEx(
		  UINT wVirtKey,
		  UINT wScanCode,
		  PBYTE lpKeyState,
		  LPWSTR pwszBuff,
		  int cchBuff,
		  UINT wFlags,
		  HKL dwhkl )
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserTrackMouseEvent(
  LPTRACKMOUSEEVENT lpEventTrack)
{
    UNIMPLEMENTED;
	return FALSE;
}

int
NTAPI
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserTranslateMessage(
  LPMSG lpMsg,
  HKL dwhkl )
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUnhookWindowsHookEx(
  HHOOK Hook)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUnhookWinEvent(
  HWINEVENTHOOK hWinEventHook)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUnloadKeyboardLayout(
  HKL hKl)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUnlockWindowStation(
  HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUnregisterClass(
  PUNICODE_STRING ClassNameOrAtom,
  HINSTANCE hInstance,
  PCLSMENUNAME pClassMenuName)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUnregisterHotKey(HWND hWnd,
		       int id)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserUnregisterUserApiHook(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserUpdateLayeredWindow(
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
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUpdatePerUserSystemParameters(
  DWORD dwReserved,
  BOOL bEnable)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserUserHandleGrantAccess(
  IN HANDLE hUserHandle,
  IN HANDLE hJob,
  IN BOOL bGrant)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserValidateHandleSecure(
  HANDLE hHdl,
  BOOL Restricted)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
NTAPI
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserValidateTimerCallback(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserVkKeyScanEx(
  WCHAR wChar,
  HKL KeyboardLayout,
  DWORD Unknown2)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserWaitForInputIdle(
  IN HANDLE hProcess,
  IN DWORD dwMilliseconds,
  IN BOOL Unknown2) // Always FALSE
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserWaitForMsgAndEvent(
  DWORD Unknown0)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
NTAPI
NtUserWaitMessage(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
NTAPI
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
    UNIMPLEMENTED;
	return 0;
}

HWND
NTAPI
NtUserWindowFromPoint(
  LONG X,
  LONG Y)
{
    UNIMPLEMENTED;
	return NULL;
}

DWORD
NTAPI
NtUserYieldTask(VOID)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NTAPI
NtUserBuildMenuItemList(
 HMENU hMenu,
 PVOID Buffer,
 ULONG nBufSize,
 DWORD Reserved)
{
    UNIMPLEMENTED;
	return 0;
}
