#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

#include <ddk/ntapi.h>

#define WM_SYSTIMER 280

ULONG STDCALL
NtUserGetSystemMetrics(ULONG Index);

DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset, BOOL Ansi);

LONG STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi);

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc);

BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect);

BOOL STDCALL
NtUserGetClientRect(HWND hWnd, LPRECT Rect);

HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom);

BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point);

HWND STDCALL
NtUserGetDesktopWindow();

NTSTATUS
STDCALL
NtUserAcquireOrReleaseInputOwnership(
  BOOLEAN Release);

DWORD
STDCALL
NtUserActivateKeyboardLayout(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserAlterWindowStyle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserAttachThreadInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HDC STDCALL
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs);

DWORD
STDCALL
NtUserBitBltSysBmp(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7);

BOOL
STDCALL
NtUserBlockInput(
  BOOL BlockIt);

ULONG
STDCALL
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG nBufSize);

NTSTATUS STDCALL
NtUserBuildNameList(
   HWINSTA hWinSta,
   ULONG dwSize,
   PVOID lpBuffer,
   PULONG pRequiredSize);

NTSTATUS
STDCALL
NtUserBuildPropList(
  HWND hWnd,
  LPVOID Buffer,
  DWORD BufferSize,
  DWORD *Count);

DWORD
STDCALL
NtUserCallHwnd(
  DWORD Unknown0,
  DWORD Unknown1);

#define HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS      0x54
#define HWNDLOCK_ROUTINE_DRAWMENUBAR               0x55
#define HWNDLOCK_ROUTINE_REDRAWFRAME               0x58
#define HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW       0x5B
#define HWNDLOCK_ROUTINE_UPDATEWINDOW              0x5E
BOOL
STDCALL
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Routine);

#define HWNDOPT_ROUTINE_SETPROGMANWINDOW       0x4A
#define HWNDOPT_ROUTINE_SETTASKMANWINDOW       0x4B
HWND
STDCALL
NtUserCallHwndOpt(
  HWND Param,
  DWORD Routine);

DWORD
STDCALL
NtUserCallHwndParam(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserCallHwndParamLock(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserCallMsgFilter(
  DWORD Unknown0,
  DWORD Unknown1);

LRESULT
STDCALL
NtUserCallNextHookEx(
  HHOOK Hook,
  int Code,
  WPARAM wParam,
  LPARAM lParam);

#define NOPARAM_ROUTINE_REGISTER_PRIMITIVE 0xffff0001 /* Private ROS */
#define NOPARAM_ROUTINE_DESTROY_CARET      0xffff0002
#define NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP 0xffff0003
#define NOPARAM_ROUTINE_INIT_MESSAGE_PUMP  0xffff0004
#define NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO 0xffff0005
DWORD
STDCALL
NtUserCallNoParam(
  DWORD Routine);

#define ONEPARAM_ROUTINE_GETMENU              0x01
#define ONEPARAM_ROUTINE_ISWINDOWUNICODE      0x02
#define ONEPARAM_ROUTINE_WINDOWFROMDC         0x03
#define ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID   0x04
#define ONEPARAM_ROUTINE_SWAPMOUSEBUTTON      0x05
#define ONEPARAM_ROUTINE_SETCARETBLINKTIME    0x06
#define ONEPARAM_ROUTINE_GETCARETINFO         0x07
#define ONEPARAM_ROUTINE_SWITCHCARETSHOWING   0x08
#define ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS 0x09
#define ONEPARAM_ROUTINE_GETWINDOWINSTANCE    0x10
#define ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO  0x0a
DWORD
STDCALL
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine);

#define TWOPARAM_ROUTINE_SETDCPENCOLOR      0x45
#define TWOPARAM_ROUTINE_SETDCBRUSHCOLOR    0x46
#define TWOPARAM_ROUTINE_GETDCCOLOR         0x47
#define TWOPARAM_ROUTINE_GETWINDOWRGNBOX    0x48
#define TWOPARAM_ROUTINE_GETWINDOWRGN       0x49
#define TWOPARAM_ROUTINE_SETMENUBARHEIGHT   0x50
#define TWOPARAM_ROUTINE_SETMENUITEMRECT    0x51
#define TWOPARAM_ROUTINE_SETGUITHRDHANDLE   0x52
  #define MSQ_STATE_CAPTURE	0x1
  #define MSQ_STATE_ACTIVE	0x2
  #define MSQ_STATE_FOCUS	0x3
  #define MSQ_STATE_MENUOWNER	0x4
  #define MSQ_STATE_MOVESIZE	0x5
  #define MSQ_STATE_CARET	0x6
#define TWOPARAM_ROUTINE_ENABLEWINDOW       0x53
#define TWOPARAM_ROUTINE_UNKNOWN            0x54
#define TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS    0x55
#define TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW 0x56
#define TWOPARAM_ROUTINE_VALIDATERGN        0x57
#define TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID 0x58
#define TWOPARAM_ROUTINE_CURSORPOSITION     0x59
#define TWOPARAM_ROUTINE_SETCARETPOS        0x60
#define TWOPARAM_ROUTINE_GETWINDOWINFO      0x61
DWORD
STDCALL
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine);

BOOL
STDCALL
NtUserChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext);

LONG
STDCALL
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam);

HWND STDCALL
NtUserChildWindowFromPointEx(HWND Parent,
			     LONG x,
			     LONG y,
			     UINT Flags);

BOOL
STDCALL
NtUserClipCursor(
  RECT *lpRect);

BOOL
STDCALL
NtUserCloseClipboard(VOID);

BOOL
STDCALL
NtUserCloseDesktop(
  HDESK hDesktop);

BOOL
STDCALL
NtUserCloseWindowStation(
  HWINSTA hWinSta);

DWORD
STDCALL
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1);

int
STDCALL
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  int EntriesCount);

DWORD
STDCALL
NtUserCountClipboardFormats(VOID);

HACCEL
STDCALL
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount);

BOOL
STDCALL
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight);

HANDLE
STDCALL
NtUserCreateCursorIconHandle(
  PICONINFO IconInfo,
  BOOL Indirect);

HDESK
STDCALL
NtUserCreateDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  HWINSTA hWindowStation);

DWORD
STDCALL
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

HWND
STDCALL
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
  BOOL bUnicodeWindow);

HWINSTA
STDCALL
NtUserCreateWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserDdeGetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserDdeSetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HDWP STDCALL
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     int x,
         int y,
         int cx,
         int cy,
		     UINT Flags);
BOOL STDCALL
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText);

BOOLEAN
STDCALL
NtUserDestroyAcceleratorTable(
  HACCEL Table);

BOOL
STDCALL
NtUserDestroyCursorIcon(
  HANDLE Handle,
  DWORD Unknown);

BOOLEAN STDCALL
NtUserDestroyWindow(HWND Wnd);

typedef struct tagNTUSERDISPATCHMESSAGEINFO
{
  BOOL HandledByKernel;
  BOOL Ansi;
  WNDPROC Proc;
  MSG Msg;
} NTUSERDISPATCHMESSAGEINFO, *PNTUSERDISPATCHMESSAGEINFO;

LRESULT
STDCALL
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO MsgInfo);

BOOL
STDCALL
NtUserDragDetect(
  HWND hWnd,
  LONG x,
  LONG y);

DWORD
STDCALL
NtUserDragObject(
	   HWND    hwnd1,
	   HWND    hwnd2,
	   UINT    u1,
	   DWORD   dw1,
	   HCURSOR hc1
	   );

DWORD
STDCALL
NtUserDrawAnimatedRects(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserDrawCaption(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserDrawCaptionTemp(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

BOOL
STDCALL
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
  DWORD Unknown1);

DWORD
STDCALL
NtUserEmptyClipboard(VOID);

BOOL
STDCALL
NtUserEnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows);

DWORD
STDCALL
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL STDCALL
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs);

BOOL
STDCALL
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICE lpDisplayDevice, /* device information */
  DWORD dwFlags ); /* reserved */

BOOL
STDCALL
NtUserEnumDisplayMonitors (
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData );

BOOL
STDCALL
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags );

DWORD
STDCALL
NtUserEvent(
  DWORD Unknown0);

DWORD
STDCALL
NtUserExcludeUpdateRgn(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserFillWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

HICON
STDCALL
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  LONG cx,
  LONG cy);

HWND
STDCALL
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING  ucClassName,
  PUNICODE_STRING  ucWindowName
  );

DWORD
STDCALL
NtUserFlashWindowEx(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetAltTabInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Flags);


DWORD
STDCALL
NtUserGetAsyncKeyState(
  DWORD Unknown0);

UINT
STDCALL
NtUserGetCaretBlinkTime(VOID);

BOOL
STDCALL
NtUserGetCaretPos(
  LPPOINT lpPoint);

DWORD STDCALL
NtUserGetClassInfo(HINSTANCE hInst,
		   LPCWSTR str,
		   LPWNDCLASSEXW wcex,
		   BOOL Ansi,
		   DWORD unknown3);

DWORD
STDCALL
NtUserGetClassName(HWND hWnd,
		   LPWSTR lpClassName,
		   ULONG nMaxCount);

HANDLE
STDCALL
NtUserGetClipboardData(
  UINT uFormat,
  DWORD Unknown1);

INT
STDCALL
NtUserGetClipboardFormatName(
  UINT format,
  PUNICODE_STRING FormatName,
  INT cchMaxCount);

HWND
STDCALL
NtUserGetClipboardOwner(VOID);

DWORD
STDCALL
NtUserGetClipboardSequenceNumber(VOID);

HWND
STDCALL
NtUserGetClipboardViewer(VOID);

BOOL
STDCALL
NtUserGetClipCursor(
  RECT *lpRect);

DWORD
STDCALL
NtUserGetComboBoxInfo(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetControlBrush(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetControlColor(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
STDCALL
NtUserGetCursorInfo(
  PCURSORINFO pci);

HDC
STDCALL
NtUserGetDC(
  HWND hWnd);

HDC
STDCALL
NtUserGetDCEx(
  HWND hWnd,
  HANDLE hRegion,
  ULONG Flags);

UINT
STDCALL
NtUserGetDoubleClickTime(VOID);

HWND
STDCALL
NtUserGetForegroundWindow(VOID);

DWORD
STDCALL
NtUserGetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags);

BOOL
STDCALL
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui);

BOOL
STDCALL
NtUserGetCursorIconInfo(
  HANDLE Handle,
  PICONINFO IconInfo);

BOOL
STDCALL
NtUserGetCursorIconSize(
  HANDLE Handle,
  BOOL *fIcon,
  SIZE *Size);

DWORD
STDCALL
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserGetInternalWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetKeyboardLayoutList(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetKeyboardLayoutName(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetKeyboardState(
  LPBYTE Unknown0);

DWORD
STDCALL
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize );

DWORD
STDCALL
NtUserGetKeyState(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetListBoxInfo(
  DWORD Unknown0);

typedef struct tagNTUSERGETMESSAGEINFO
{
  MSG Msg;
  ULONG LParamSize;
} NTUSERGETMESSAGEINFO, *PNTUSERGETMESSAGEINFO;

BOOL
STDCALL
NtUserGetMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax);

DWORD
STDCALL
NtUserGetMouseMovePointsEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

BOOL
STDCALL
NtUserGetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength,
  PDWORD nLengthNeeded);

HWND
STDCALL
NtUserGetOpenClipboardWindow(VOID);

INT
STDCALL
NtUserGetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  INT cFormats);

HWINSTA
STDCALL
NtUserGetProcessWindowStation(VOID);

BOOL
STDCALL
NtUserGetScrollBarInfo(
  HWND hWnd, 
  LONG idObject, 
  PSCROLLBARINFO psbi);

BOOL
STDCALL
NtUserGetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPSCROLLINFO lpsi);

HDESK
STDCALL
NtUserGetThreadDesktop(
  DWORD dwThreadId,
  DWORD Unknown1);

#define THREADSTATE_FOCUSWINDOW (1)
#define THREADSTATE_INSENDMESSAGE       (2)
DWORD
STDCALL
NtUserGetThreadState(
  DWORD Routine);

DWORD
STDCALL
NtUserGetTitleBarInfo(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL STDCALL
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase);

int
STDCALL
NtUserGetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase);

DWORD
STDCALL
NtUserGetWindowDC(
  HWND hWnd);

BOOL
STDCALL
NtUserGetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl);

DWORD
STDCALL
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL
STDCALL
NtUserHideCaret(
  HWND hWnd);

DWORD
STDCALL
NtUserImpersonateDdeClientWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserInitializeClientPfnArrays(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
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
  DWORD Unknown10);

INT
STDCALL
NtUserInternalGetWindowText(
  HWND hWnd,
  LPWSTR lpString,
  INT nMaxCount);

DWORD
STDCALL
NtUserInvalidateRect(
HWND hWnd,
CONST RECT *lpRect,
BOOL bErase);

DWORD
STDCALL
  NtUserInvalidateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase);


BOOL
STDCALL
NtUserIsClipboardFormatAvailable(
  UINT format);

BOOL
STDCALL
NtUserKillSystemTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
);

BOOL
STDCALL
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
);

DWORD
STDCALL
NtUserLoadKeyboardLayoutEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

BOOL
STDCALL
NtUserLockWindowStation(
  HWINSTA hWindowStation);

DWORD
STDCALL
NtUserLockWindowUpdate(
  DWORD Unknown0);

DWORD
STDCALL
NtUserLockWorkStation(VOID);

UINT
STDCALL
NtUserMapVirtualKeyEx( UINT keyCode,
		       UINT transType,
		       DWORD keyboardId,
		       HKL dwhkl );

DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

BOOL
STDCALL
NtUserGetMinMaxInfo(
  HWND hwnd,
  MINMAXINFO *MinMaxInfo,
  BOOL SendMessage);

DWORD
STDCALL
NtUserMNDragLeave(VOID);

DWORD
STDCALL
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL
STDCALL
NtUserMoveWindow(      
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint
);

DWORD
STDCALL
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserNotifyWinEvent(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
STDCALL
NtUserOpenClipboard(
  HWND hWnd,
  DWORD Unknown1);

HDESK
STDCALL
NtUserOpenDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess);

HDESK
STDCALL
NtUserOpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess);

HWINSTA
STDCALL
NtUserOpenWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess);

BOOL
STDCALL
NtUserPaintDesktop(
  HDC hDC);

BOOL
STDCALL
NtUserPeekMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg);

BOOL
STDCALL
NtUserPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

BOOL
STDCALL
NtUserPostThreadMessage(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

DWORD
STDCALL
NtUserQuerySendMessage(
  DWORD Unknown0);

DWORD
STDCALL
NtUserQueryUserCounters(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

#define QUERY_WINDOW_UNIQUE_PROCESS_ID	0x00
#define QUERY_WINDOW_UNIQUE_THREAD_ID	0x01
#define QUERY_WINDOW_ISHUNG	0x04
DWORD
STDCALL
NtUserQueryWindow(
  HWND hWnd,
  DWORD Index);

DWORD
STDCALL
NtUserRealChildWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

BOOL
STDCALL
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
);

RTL_ATOM
STDCALL
NtUserRegisterClassExWOW(CONST WNDCLASSEXW* lpwcx,
			 BOOL bUnicodeClass,
			 WNDPROC wpExtra,
			 DWORD Unknown4,
			 DWORD Unknown5);

BOOL
STDCALL
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk);

DWORD
STDCALL
NtUserRegisterTasklist(
  DWORD Unknown0);

UINT STDCALL
NtUserRegisterWindowMessage(PUNICODE_STRING MessageName);

HANDLE STDCALL
NtUserRemoveProp(HWND hWnd, ATOM Atom);

DWORD
STDCALL
NtUserResolveDesktopForWOW(
  DWORD Unknown0);

DWORD
STDCALL
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip ,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate);

DWORD STDCALL
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags);

UINT
STDCALL
NtUserSendInput(
  UINT nInputs,
  LPINPUT pInput,
  INT cbSize);

typedef struct tagNTUSERSENDMESSAGEINFO
{
  BOOL HandledByKernel;
  BOOL Ansi;
  WNDPROC Proc;
} NTUSERSENDMESSAGEINFO, *PNTUSERSENDMESSAGEINFO;

LRESULT STDCALL
NtUserSendMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam,
                  PNTUSERSENDMESSAGEINFO Info);

BOOL
STDCALL
NtUserSendMessageCallback(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData);

LRESULT STDCALL
NtUserSendMessageTimeout(HWND hWnd,
			 UINT Msg,
			 WPARAM wParam,
			 LPARAM lParam,
			 UINT uFlags,
			 UINT uTimeout,
			 ULONG_PTR *uResult,
	                 PNTUSERSENDMESSAGEINFO Info);

BOOL
STDCALL
NtUserSendNotifyMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

HWND STDCALL
NtUserSetActiveWindow(HWND Wnd);
HWND STDCALL
NtUserGetActiveWindow(VOID);

HWND STDCALL
NtUserSetCapture(HWND Wnd);
HWND STDCALL
NtUserGetCapture(VOID);

DWORD STDCALL
NtUserSetClassLong(
  HWND  hWnd,
  DWORD Offset,
  LONG  dwNewLong,
  BOOL  Ansi );


DWORD
STDCALL
NtUserSetClassWord(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HANDLE
STDCALL
NtUserSetClipboardData(
  UINT uFormat,
  HANDLE hMem,
  DWORD Unknown2);

HWND
STDCALL
NtUserSetClipboardViewer(
  HWND hWndNewViewer);

DWORD
STDCALL
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1);

HCURSOR
STDCALL
NtUserSetCursor(
  HCURSOR hCursor);

BOOL
STDCALL
NtUserSetCursorIconContents(
  HANDLE Handle,
  PICONINFO IconInfo);

BOOL
STDCALL
NtUserSetCursorIconData(
  HANDLE Handle,
  PBOOL fIcon,
  POINT *Hotspot,
  HMODULE hModule,
  HRSRC hRsrc,
  HRSRC hGroupRsrc);

DWORD
STDCALL
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1);

HWND
STDCALL
NtUserSetFocus(
  HWND hWnd);

DWORD
STDCALL
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserSetImeOwnerWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetInternalWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetKeyboardState(
  LPBYTE Unknown0);

DWORD
STDCALL
NtUserSetLayeredWindowAttributes(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetLogonNotifyWindow(
  DWORD Unknown0);

BOOL
STDCALL
NtUserSetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength);

HWND
STDCALL
NtUserSetParent(
  HWND hWndChild,
  HWND hWndNewParent);

BOOL
STDCALL
NtUserSetProcessWindowStation(
  HWINSTA hWindowStation);

BOOL STDCALL
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data);

DWORD
STDCALL
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPCSCROLLINFO lpsi, 
  BOOL bRedraw);

BOOL
STDCALL
NtUserSetShellWindowEx(
  HWND hwndShell,
  HWND hwndShellListView);

HWND
STDCALL
NtUserGetShellWindow();

DWORD
STDCALL
NtUserSetSysColors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
STDCALL
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id);

BOOL
STDCALL
NtUserSetThreadDesktop(
  HDESK hDesktop);

DWORD
STDCALL
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1);

UINT_PTR
STDCALL
NtUserSetSystemTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
);

UINT_PTR
STDCALL
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
);

DWORD
STDCALL
NtUserSetWindowFNID(
  DWORD Unknown0,
  DWORD Unknown1);

LONG
STDCALL
NtUserSetWindowLong(
  HWND hWnd,
  DWORD Index,
  LONG NewValue,
  BOOL Ansi);

BOOL
STDCALL
NtUserSetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl);

BOOL 
STDCALL NtUserSetWindowPos(      
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags
);

INT
STDCALL
NtUserSetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw);

DWORD
STDCALL
NtUserSetWindowsHookAW(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HHOOK
STDCALL
NtUserSetWindowsHookEx(
  HINSTANCE Mod,
  PUNICODE_STRING ModuleName,
  DWORD ThreadId,
  int HookId,
  HOOKPROC HookProc,
  BOOL Ansi);

DWORD
STDCALL
NtUserSetWindowStationUser(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

WORD STDCALL
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal);

DWORD
STDCALL
NtUserSetWinEventHook(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7);

BOOL
STDCALL
NtUserShowCaret(
  HWND hWnd);

DWORD
STDCALL
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow);

BOOL
STDCALL
NtUserShowWindow(
  HWND hWnd,
  LONG nCmdShow);

DWORD
STDCALL
NtUserShowWindowAsync(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL
STDCALL
NtUserSwitchDesktop(
  HDESK hDesktop);

DWORD
STDCALL
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni);

int
STDCALL
NtUserToUnicodeEx(
		  UINT wVirtKey,
		  UINT wScanCode,
		  PBYTE lpKeyState,
		  LPWSTR pwszBuff,
		  int cchBuff,
		  UINT wFlags,
		  HKL dwhkl );

DWORD
STDCALL
NtUserTrackMouseEvent(
  DWORD Unknown0);

int
STDCALL
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message);

BOOL
STDCALL
NtUserTranslateMessage(
  LPMSG lpMsg,
  HKL dwhkl );

BOOL
STDCALL
NtUserUnhookWindowsHookEx(
  HHOOK Hook);

DWORD
STDCALL
NtUserUnhookWinEvent(
  DWORD Unknown0);

DWORD
STDCALL
NtUserUnloadKeyboardLayout(
  DWORD Unknown0);

BOOL
STDCALL
NtUserUnlockWindowStation(
  HWINSTA hWindowStation);

BOOL
STDCALL
NtUserUnregisterClass(
  LPCWSTR ClassNameOrAtom,
  HINSTANCE hInstance,
  DWORD Unknown);

BOOL
STDCALL
NtUserUnregisterHotKey(HWND hWnd,
		       int id);

DWORD
STDCALL
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

BOOL STDCALL
NtUserUpdateWindow( HWND hWnd );

DWORD
STDCALL
NtUserUpdateLayeredWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7,
  DWORD Unknown8);

DWORD
STDCALL
NtUserUpdatePerUserSystemParameters(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserUserHandleGrantAccess(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserValidateHandleSecure(
  DWORD Unknown0);

VOID STDCALL
NtUserValidateRect(HWND Wnd, const RECT* Rect);


DWORD
STDCALL
NtUserVkKeyScanEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserWaitForInputIdle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserWaitForMsgAndEvent(
  DWORD Unknown0);

BOOL
STDCALL
NtUserWaitMessage(VOID);

DWORD
STDCALL
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

HWND
STDCALL
NtUserWindowFromPoint(
  LONG X,
  LONG Y);

DWORD
STDCALL
NtUserYieldTask(VOID);

DWORD STDCALL
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid);

DWORD STDCALL
NtUserGetQueueStatus(BOOL ClearChanges);

HWND STDCALL
NtUserGetParent(HWND hWnd);

HWND STDCALL
NtUserGetWindow(HWND hWnd, UINT Relationship);

HWND STDCALL
NtUserGetLastActivePopup(HWND hWnd);
typedef struct _WndProcHandle
{
  WNDPROC WindowProc;
  BOOL IsUnicode;
  HANDLE ProcessID;
} WndProcHandle;
                                    
DWORD STDCALL
NtUserDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data);

VOID STDCALL
NtUserManualGuiCheck(LONG Check);

typedef struct _SETSCROLLBARINFO
{
  int nTrackPos; 
  int reserved;
  DWORD rgstate[CCHILDREN_SCROLLBAR+1];
} SETSCROLLBARINFO, *PSETSCROLLBARINFO;

BOOL
STDCALL
NtUserSetScrollBarInfo(
  HWND hwnd,
  LONG idObject,
  SETSCROLLBARINFO *info);

/* lParam of DDE messages */
typedef struct tagKMDDEEXECUTEDATA
{
  HWND Sender;
  HGLOBAL ClientMem;
  /* BYTE Data[DataSize] */
} KMDDEEXECUTEDATA, *PKMDDEEXECUTEDATA; 

typedef struct tagKMDDELPARAM
{
  BOOL Packed;
  union
    {
      struct
        {
          UINT uiLo;
          UINT uiHi;
        } Packed;
      LPARAM Unpacked;
    } Value;
} KMDDELPARAM, *PKMDDELPARAM;

#endif /* __WIN32K_NTUSER_H */

/* EOF */
