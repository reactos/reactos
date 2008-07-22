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
    DWORD dwUnknown3) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDestroyMenu(
  HMENU hMenu) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserDrawMenuBarTemp(
  HWND hWnd,
  HDC hDC,
  PRECT hRect,
  HMENU hMenu,
  HFONT hFont) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserEndMenu(VOID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem) { UNIMPLEMENTED; }

HMENU
NTAPI
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite) { UNIMPLEMENTED; }

int
NTAPI
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetMenuFlagRtoL(
  HMENU hMenu) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetSystemMenu(
  HWND hWnd,
  HMENU hMenu) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm) { UNIMPLEMENTED; }

HKL
NTAPI
NtUserActivateKeyboardLayout(
  HKL hKl,
  ULONG Flags) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserAlterWindowStyle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserAttachThreadInput(
  IN DWORD idAttach,
  IN DWORD idAttachTo,
  IN BOOL fAttach) { UNIMPLEMENTED; }

HDC NTAPI
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs) { UNIMPLEMENTED; }

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
  DWORD dwRop ) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserBlockInput(
  BOOL BlockIt) { UNIMPLEMENTED; }

NTSTATUS
NTAPI
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG* pBufSize) { UNIMPLEMENTED; }

NTSTATUS NTAPI
NtUserBuildNameList(
   HWINSTA hWinSta,
   ULONG dwSize,
   PVOID lpBuffer,
   PULONG pRequiredSize) { UNIMPLEMENTED; }

NTSTATUS
NTAPI
NtUserBuildPropList(
  HWND hWnd,
  LPVOID Buffer,
  DWORD BufferSize,
  DWORD *Count) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCallHwnd(
  HWND hWnd,
  DWORD Routine) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Routine) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserCallHwndOpt(
  HWND hWnd,
  DWORD Routine) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCallHwndParam(
  HWND hWnd,
  DWORD Param,
  DWORD Routine) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCallHwndParamLock(
  HWND hWnd,
  DWORD Param,
  DWORD Routine) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserCallMsgFilter(
  LPMSG msg,
  INT code) { UNIMPLEMENTED; }

LRESULT
NTAPI
NtUserCallNextHookEx(
  HHOOK Hook,
  int Code,
  WPARAM wParam,
  LPARAM lParam) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCallNoParam(
  DWORD Routine) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext) { UNIMPLEMENTED; }

LONG
NTAPI
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam) { UNIMPLEMENTED; }

DWORD
STDCALL
NtUserCheckImeHotKey(
  DWORD dwUnknown1,
  DWORD dwUnknown2) { UNIMPLEMENTED; }

HWND NTAPI
NtUserChildWindowFromPointEx(
  HWND Parent,
  LONG x,
  LONG y,
  UINT Flags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserClipCursor(
    RECT *lpRect) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserCloseClipboard(VOID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserCloseDesktop(
  HDESK hDesktop) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserCloseWindowStation(
  HWINSTA hWinSta) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserConsoleControl(
  DWORD dwUnknown1,
  DWORD dwUnknown2,
  DWORD dwUnknown3) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

int
NTAPI
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  int EntriesCount) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCountClipboardFormats(VOID) { UNIMPLEMENTED; }

HACCEL
NTAPI
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight) { UNIMPLEMENTED; }

HDESK
NTAPI
NtUserCreateDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  HWINSTA hWindowStation) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCreateInputContext(
    DWORD dwUnknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3) { UNIMPLEMENTED; }

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
  DWORD dwUnknown) { UNIMPLEMENTED; }

HWINSTA
NTAPI
NtUserCreateWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDdeGetQualityOfService(
  IN HWND hwndClient,
  IN HWND hWndServer,
  OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDdeSetQualityOfService(
  IN  HWND hwndClient,
  IN  PSECURITY_QUALITY_OF_SERVICE pqosNew,
  OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev) { UNIMPLEMENTED; }

HDWP NTAPI
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     int x,
         int y,
         int cx,
         int cy,
		     UINT Flags) { UNIMPLEMENTED; }
BOOL NTAPI
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText) { UNIMPLEMENTED; }

BOOLEAN
NTAPI
NtUserDestroyAcceleratorTable(
  HACCEL Table) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDestroyCursor(
  HANDLE Handle,
  DWORD Unknown) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserDestroyInputContext(
    DWORD dwUnknown1) { UNIMPLEMENTED; }

BOOLEAN NTAPI
NtUserDestroyWindow(HWND Wnd) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserDisableThreadIme(
    DWORD dwUnknown1) { UNIMPLEMENTED; }

LRESULT
NTAPI
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO MsgInfo) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDragDetect(
  HWND hWnd,
  POINT pt) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDrawAnimatedRects(
  HWND hwnd,
  INT idAni,
  RECT *lprcFrom,
  RECT *lprcTo) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserDrawCaption(
   HWND hWnd,
   HDC hDc,
   LPCRECT lpRc,
   UINT uFlags) { UNIMPLEMENTED; }

BOOL
STDCALL
NtUserDrawCaptionTemp(
  HWND hWnd,
  HDC hDC,
  LPCRECT lpRc,
  HFONT hFont,
  HICON hIcon,
  const PUNICODE_STRING str,
  UINT uFlags) { UNIMPLEMENTED; }

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
  DWORD Unknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserEmptyClipboard(VOID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserEnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

BOOL NTAPI
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICEW lpDisplayDevice, /* device information */
  DWORD dwFlags ) { UNIMPLEMENTED; } /* reserved */

/*BOOL
NTAPI
NtUserEnumDisplayMonitors (
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData ) { UNIMPLEMENTED; }*/
/* FIXME:  The call below is ros-specific and should be rewritten to use the same params as the correct call above.  */
INT
NTAPI
NtUserEnumDisplayMonitors(
  OPTIONAL IN HDC hDC,
  OPTIONAL IN LPCRECT pRect,
  OPTIONAL OUT HMONITOR *hMonitorList,
  OPTIONAL OUT LPRECT monitorRectList,
  OPTIONAL IN DWORD listSize ) { UNIMPLEMENTED; }


NTSTATUS
NTAPI
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags ) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserEvent(
  DWORD Unknown0) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserFillWindow(
  HWND hWndPaint,
  HWND hWndPaint1,
  HDC  hDC,
  HBRUSH hBrush) { UNIMPLEMENTED; }

HICON
NTAPI
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  LONG cx,
  LONG cy) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING  ucClassName,
  PUNICODE_STRING  ucWindowName,
  DWORD dwUnknown
  ) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserFlashWindowEx(
  IN PFLASHWINFO pfwi) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetAltTabInfo(
   HWND hwnd,
   INT  iItem,
   PALTTABINFO pati,
   LPTSTR pszItemText,
   UINT   cchItemText,
   BOOL   Ansi) { UNIMPLEMENTED; }

HWND NTAPI
NtUserGetAncestor(HWND hWnd, UINT Flags) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetAppImeLevel(
    DWORD dwUnknown1) { UNIMPLEMENTED; }

SHORT
NTAPI
NtUserGetAsyncKeyState(
  INT Key) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetAtomName(
    ATOM nAtom,
    LPWSTR lpBuffer) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserGetCaretBlinkTime(VOID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetCaretPos(
  LPPOINT lpPoint) { UNIMPLEMENTED; }

BOOL NTAPI
NtUserGetClassInfo(HINSTANCE hInstance,
		   PUNICODE_STRING ClassName,
		   LPWNDCLASSEXW wcex,
		   LPWSTR *ppszMenuName,
		   BOOL Ansi) { UNIMPLEMENTED; }

INT
NTAPI
NtUserGetClassName(HWND hWnd,
		   PUNICODE_STRING ClassName,
                   BOOL Ansi) { UNIMPLEMENTED; }
#if 0 // Real NtUserGetClassName
INT
NTAPI
NtUserGetClassName(HWND hWnd,
                   BOOL Real, // 0 GetClassNameW, 1 RealGetWindowClassA/W
                   PUNICODE_STRING ClassName) { UNIMPLEMENTED; }
#endif

HANDLE
NTAPI
NtUserGetClipboardData(
  UINT uFormat,
  PVOID pBuffer) { UNIMPLEMENTED; }

INT
NTAPI
NtUserGetClipboardFormatName(
  UINT format,
  PUNICODE_STRING FormatName,
  INT cchMaxCount) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserGetClipboardOwner(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetClipboardSequenceNumber(VOID) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserGetClipboardViewer(VOID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetClipCursor(
  RECT *lpRect) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetComboBoxInfo(
  HWND hWnd,
  PCOMBOBOXINFO pcbi) { UNIMPLEMENTED; }

HBRUSH
NTAPI
NtUserGetControlBrush(
  HWND hwnd,
  HDC  hdc,
  UINT ctlType) { UNIMPLEMENTED; }

HBRUSH
NTAPI
NtUserGetControlColor(
   HWND hwndParent,
   HWND hwnd,
   HDC hdc,
   UINT CtlMsg) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetCursorInfo(
  PCURSORINFO pci) { UNIMPLEMENTED; }

HDC
NTAPI
NtUserGetDC(
  HWND hWnd) { UNIMPLEMENTED; }

HDC
NTAPI
NtUserGetDCEx(
  HWND hWnd,
  HANDLE hRegion,
  ULONG Flags) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserGetDoubleClickTime(VOID) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserGetForegroundWindow(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetIconInfo(
   HANDLE hCurIcon,
   PICONINFO IconInfo,
   PUNICODE_STRING lpInstName,
   PUNICODE_STRING lpResName,
   LPDWORD pbpp,
   BOOL bInternal) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetIconSize(
    HANDLE Handle,
    UINT istepIfAniCur,
    LONG  *plcx,
    LONG  *plcy) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetInternalWindowPos(
  HWND hwnd,
  LPRECT rectWnd,
  LPPOINT ptIcon) { UNIMPLEMENTED; }

HKL
NTAPI
NtUserGetKeyboardLayout(
  DWORD dwThreadid) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserGetKeyboardLayoutList(
  INT nItems,
  HKL *pHklBuff) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetKeyboardLayoutName(
  LPWSTR lpszName) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetKeyboardState(
  LPBYTE Unknown0) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetKeyboardType(
  DWORD TypeFlag) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize ) { UNIMPLEMENTED; }

SHORT
NTAPI
NtUserGetKeyState(
  INT VirtKey) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetListBoxInfo(
  HWND hWnd) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength,
  PDWORD nLengthNeeded) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserGetOpenClipboardWindow(VOID) { UNIMPLEMENTED; }

INT
NTAPI
NtUserGetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  INT cFormats) { UNIMPLEMENTED; }

HWINSTA
NTAPI
NtUserGetProcessWindowStation(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetRawInputData(
  HRAWINPUT hRawInput,
  UINT uiCommand,
  LPVOID pData,
  PUINT pcbSize,
  UINT cbSizeHeader) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetScrollBarInfo(
  HWND hWnd,
  LONG idObject,
  PSCROLLBARINFO psbi) { UNIMPLEMENTED; }

HDESK
NTAPI
NtUserGetThreadDesktop(
  DWORD dwThreadId,
  DWORD Unknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetThreadState(
  DWORD Routine) { UNIMPLEMENTED; }

BOOLEAN
NTAPI
NtUserGetTitleBarInfo(
  HWND hwnd,
  PTITLEBARINFO pti) { UNIMPLEMENTED; }

BOOL NTAPI
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase) { UNIMPLEMENTED; }

int
NTAPI
NtUserGetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase) { UNIMPLEMENTED; }

HDC
NTAPI
NtUserGetWindowDC(
  HWND hWnd) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserGetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserImpersonateDdeClientWindow(
  HWND hWndClient,
  HWND hWndServer) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserInitialize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3) { UNIMPLEMENTED; }

NTSTATUS
NTAPI
NtUserInitializeClientPfnArrays(
  PPFNCLIENT pfnClientA, 
  PPFNCLIENT pfnClientW,
  PPFNCLIENTWORKER pfnClientWorker,
  HINSTANCE hmodUser) { UNIMPLEMENTED; }

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
  DWORD Unknown11) { UNIMPLEMENTED; }

INT
NTAPI
NtUserInternalGetWindowText(
  HWND hWnd,
  LPWSTR lpString,
  INT nMaxCount) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserInvalidateRect(
    HWND hWnd,
    CONST RECT *lpRect,
    BOOL bErase) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserIsClipboardFormatAvailable(
  UINT format) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
) { UNIMPLEMENTED; }

HKL
STDCALL
NtUserLoadKeyboardLayoutEx(
   IN HANDLE Handle,
   IN DWORD offTable,
   IN PUNICODE_STRING puszKeyboardName,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserLockWindowStation(
  HWINSTA hWindowStation) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserLockWindowUpdate(
  DWORD Unknown0) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserLockWorkStation(VOID) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserMapVirtualKeyEx( UINT keyCode,
		       UINT transType,
		       DWORD keyboardId,
		       HKL dwhkl ) { UNIMPLEMENTED; }
LRESULT
NTAPI
NtUserMessageCall(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  ULONG_PTR ResultInfo,
  DWORD dwType, // FNID_XX types
  BOOL Ansi) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserMNDragLeave(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserMoveWindow(
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint
) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserNotifyProcessCreate(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4) { UNIMPLEMENTED; }

VOID
NTAPI
NtUserNotifyWinEvent(
  DWORD Event,
  HWND  hWnd,
  LONG  idObject,
  LONG  idChild) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserOpenClipboard(
  HWND hWnd,
  DWORD Unknown1) { UNIMPLEMENTED; }

HDESK
NTAPI
NtUserOpenDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess) { UNIMPLEMENTED; }

HDESK
NTAPI
NtUserOpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess) { UNIMPLEMENTED; }

HWINSTA
NTAPI
NtUserOpenWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserPaintDesktop(
  HDC hDC) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserPeekMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserPostThreadMessage(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserPrintWindow(
    HWND hwnd,
    HDC  hdcBlt,
    UINT nFlags) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserProcessConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserQueryInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserQuerySendMessage(
  DWORD Unknown0) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserQueryUserCounters(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserQueryWindow(
  HWND hWnd,
  DWORD Index) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRealInternalGetMessage(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRealChildWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
) { UNIMPLEMENTED; }

RTL_ATOM
NTAPI
NtUserRegisterClassExWOW(
    WNDCLASSEXW* lpwcx,
    PUNICODE_STRING pustrClassName,
    PUNICODE_STRING pustrCNVersion,
    PCLSMENUNAME pClassMenuName,
    DWORD fnID,
    DWORD Flags,
    LPDWORD pWow) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRegisterTasklist(
  DWORD Unknown0) { UNIMPLEMENTED; }

UINT NTAPI
NtUserRegisterWindowMessage(PUNICODE_STRING MessageName) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRemoteRedrawScreen(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(VOID) { UNIMPLEMENTED; }

HANDLE NTAPI
NtUserRemoveProp(HWND hWnd, ATOM Atom) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserResolveDesktopForWOW(
  DWORD Unknown0) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip ,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate) { UNIMPLEMENTED; }

DWORD NTAPI
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags) { UNIMPLEMENTED; }

UINT
NTAPI
NtUserSendInput(
  UINT nInputs,
  LPINPUT pInput,
  INT cbSize) { UNIMPLEMENTED; }

HWND NTAPI
NtUserSetActiveWindow(HWND Wnd) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2) { UNIMPLEMENTED; }

HWND NTAPI
NtUserSetCapture(HWND Wnd) { UNIMPLEMENTED; }

ULONG_PTR NTAPI
NtUserSetClassLong(
  HWND  hWnd,
  INT Offset,
  ULONG_PTR  dwNewLong,
  BOOL  Ansi ) { UNIMPLEMENTED; }

WORD
NTAPI
NtUserSetClassWord(
  HWND hWnd,
  INT nIndex,
  WORD wNewWord) { UNIMPLEMENTED; }

HANDLE
NTAPI
NtUserSetClipboardData(
  UINT uFormat,
  HANDLE hMem,
  DWORD Unknown2) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserSetClipboardViewer(
  HWND hWndNewViewer) { UNIMPLEMENTED; }

HPALETTE
STDCALL
NtUserSelectPalette(
    HDC hDC,
    HPALETTE  hpal,
    BOOL  ForceBackground
) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

HCURSOR
NTAPI
NtUserSetCursor(
  HCURSOR hCursor) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetCursorContents(
  HANDLE Handle,
  PICONINFO IconInfo) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetCursorIconData(
  HANDLE Handle,
  PBOOL fIcon,
  POINT *Hotspot,
  HMODULE hModule,
  HRSRC hRsrc,
  HRSRC hGroupRsrc) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserSetFocus(
  HWND hWnd) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetImeInfoEx(
    DWORD dwUnknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetImeOwnerWindow(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetInternalWindowPos(
  HWND    hwnd,
  UINT    showCmd,
  LPRECT  rect,
  LPPOINT pt) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetKeyboardState(
  LPBYTE lpKeyState) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetLayeredWindowAttributes(
  HWND hwnd,
  COLORREF crKey,
  BYTE bAlpha,
  DWORD dwFlags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetLogonNotifyWindow(
  HWND hWnd) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserSetParent(
  HWND hWndChild,
  HWND hWndNewParent) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetProcessWindowStation(
  HWINSTA hWindowStation) { UNIMPLEMENTED; }

BOOL NTAPI
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetScrollInfo(
  HWND hwnd,
  int fnBar,
  LPCSCROLLINFO lpsi,
  BOOL bRedraw) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetShellWindowEx(
  HWND hwndShell,
  HWND hwndShellListView) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetSysColors(
  int cElements,
  IN CONST INT *lpaElements,
  IN CONST COLORREF *lpaRgbValues,
  FLONG Flags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetThreadDesktop(
  HDESK hDesktop) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1) { UNIMPLEMENTED; }

UINT_PTR
NTAPI
NtUserSetSystemTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2) { UNIMPLEMENTED; }

UINT_PTR
NTAPI
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetWindowFNID(
  HWND hWnd,
  WORD fnID) { UNIMPLEMENTED; }

LONG
NTAPI
NtUserSetWindowLong(
  HWND hWnd,
  DWORD Index,
  LONG NewValue,
  BOOL Ansi) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl) { UNIMPLEMENTED; }

BOOL
NTAPI NtUserSetWindowPos(
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags
) { UNIMPLEMENTED; }

INT
NTAPI
NtUserSetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw) { UNIMPLEMENTED; }

HHOOK
NTAPI
NtUserSetWindowsHookAW(
  int idHook,
  HOOKPROC lpfn,
  BOOL Ansi) { UNIMPLEMENTED; }

HHOOK
NTAPI
NtUserSetWindowsHookEx(
  HINSTANCE Mod,
  PUNICODE_STRING ModuleName,
  DWORD ThreadId,
  int HookId,
  HOOKPROC HookProc,
  BOOL Ansi) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserSetWindowStationUser(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3) { UNIMPLEMENTED; }

WORD NTAPI
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal) { UNIMPLEMENTED; }

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
  UINT dwflags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserShowCaret(
  HWND hWnd) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserHideCaret(
  HWND hWnd) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserShowWindow(
  HWND hWnd,
  LONG nCmdShow) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserShowWindowAsync(
  HWND hWnd,
  LONG nCmdShow) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSoundSentry(VOID) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSwitchDesktop(
  HDESK hDesktop) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1) { UNIMPLEMENTED; }

INT
NTAPI
NtUserToUnicodeEx(
		  UINT wVirtKey,
		  UINT wScanCode,
		  PBYTE lpKeyState,
		  LPWSTR pwszBuff,
		  int cchBuff,
		  UINT wFlags,
		  HKL dwhkl ) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserTrackMouseEvent(
  LPTRACKMOUSEEVENT lpEventTrack) { UNIMPLEMENTED; }

int
NTAPI
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserTranslateMessage(
  LPMSG lpMsg,
  HKL dwhkl ) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUnhookWindowsHookEx(
  HHOOK Hook) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUnhookWinEvent(
  HWINEVENTHOOK hWinEventHook) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUnloadKeyboardLayout(
  HKL hKl) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUnlockWindowStation(
  HWINSTA hWindowStation) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUnregisterClass(
  PUNICODE_STRING ClassNameOrAtom,
  HINSTANCE hInstance,
  PCLSMENUNAME pClassMenuName) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUnregisterHotKey(HWND hWnd,
		       int id) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserUnregisterUserApiHook(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2) { UNIMPLEMENTED; }

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
  DWORD dwFlags) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUpdatePerUserSystemParameters(
  DWORD dwReserved,
  BOOL bEnable) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserUserHandleGrantAccess(
  IN HANDLE hUserHandle,
  IN HANDLE hJob,
  IN BOOL bGrant) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserValidateHandleSecure(
  HANDLE hHdl,
  BOOL Restricted) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserValidateTimerCallback(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserVkKeyScanEx(
  WCHAR wChar,
  HKL KeyboardLayout,
  DWORD Unknown2) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserWaitForInputIdle(
  IN HANDLE hProcess,
  IN DWORD dwMilliseconds,
  IN BOOL Unknown2) { UNIMPLEMENTED; } // Always FALSE

DWORD
NTAPI
NtUserWaitForMsgAndEvent(
  DWORD Unknown0) { UNIMPLEMENTED; }

BOOL
NTAPI
NtUserWaitMessage(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5) { UNIMPLEMENTED; }

HWND
NTAPI
NtUserWindowFromPoint(
  LONG X,
  LONG Y) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserYieldTask(VOID) { UNIMPLEMENTED; }

DWORD
NTAPI
NtUserBuildMenuItemList(
 HMENU hMenu,
 PVOID Buffer,
 ULONG nBufSize,
 DWORD Reserved) { UNIMPLEMENTED; }
