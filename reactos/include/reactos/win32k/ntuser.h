#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

struct _W32PROCESSINFO;
struct _W32THREADINFO;

typedef struct _DESKTOP
{
    HANDLE hKernelHeap;
    WCHAR szDesktopName[1];
    HWND hTaskManWindow;
    HWND hProgmanWindow;
} DESKTOP, *PDESKTOP;

typedef struct _CALLPROC
{
    struct _W32PROCESSINFO *pi;
    WNDPROC WndProc;
    UINT Unicode : 1;
} CALLPROC, *PCALLPROC;

typedef struct _WINDOWCLASS
{
    struct _WINDOWCLASS *Next;
    struct _WINDOWCLASS *Clone;
    struct _WINDOWCLASS *Base;
    PDESKTOP Desktop;
    RTL_ATOM Atom;
    ULONG Windows;

    UINT Style;
    WNDPROC WndProc;
    union
    {
        WNDPROC WndProcExtra;
        PCALLPROC CallProc;
    };
    PCALLPROC CallProc2;
    INT ClsExtra;
    INT WndExtra;
    HINSTANCE hInstance;
    HANDLE hIcon; /* FIXME - Use pointer! */
    HANDLE hIconSm; /* FIXME - Use pointer! */
    HANDLE hCursor; /* FIXME - Use pointer! */
    HBRUSH hbrBackground;
    HANDLE hMenu; /* FIXME - Use pointer! */
    PWSTR MenuName;
    PSTR AnsiMenuName;

    UINT Destroying : 1;
    UINT Unicode : 1;
    UINT System : 1;
    UINT Global : 1;
    UINT GlobalCallProc : 1;
    UINT GlobalCallProc2 : 1;
    UINT MenuNameIsString : 1;
} WINDOWCLASS, *PWINDOWCLASS;

typedef struct _W32PROCESSINFO
{
    PVOID UserHandleTable;
    HINSTANCE hModUser;
    PWINDOWCLASS LocalClassList;
    PWINDOWCLASS GlobalClassList;
    PWINDOWCLASS SystemClassList;
} W32PROCESSINFO, *PW32PROCESSINFO;

typedef struct _W32THREADINFO
{
    PW32PROCESSINFO pi; /* [USER] */
    PW32PROCESSINFO kpi; /* [KERNEL] */
    PDESKTOP Desktop;
    ULONG_PTR DesktopHeapDelta;
} W32THREADINFO, *PW32THREADINFO;

/* Window Client Information structure */
typedef struct _W32CLIENTINFO
{
    ULONG Win32ClientInfo0[2];
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;
    ULONG Win32ClientInfo1[5];
    HWND  hWND;
    PVOID pvWND;
    ULONG Win32ClientInfo2[50];
} W32CLIENTINFO, *PW32CLIENTINFO;

#define GetWin32ClientInfo() (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo)

PW32THREADINFO GetW32ThreadInfo(VOID);
PW32PROCESSINFO GetW32ProcessInfo(VOID);

DWORD
NTAPI
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserBuildMenuItemList(
 HMENU hMenu,
 PVOID Buffer,
 ULONG nBufSize,
 DWORD Reserved);

DWORD
NTAPI
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
NTAPI
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck);

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

BOOL
NTAPI
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags);

BOOL
NTAPI
NtUserDestroyMenu(
  HMENU hMenu);

DWORD
NTAPI
NtUserDrawMenuBarTemp(
  HWND hWnd,
  HDC hDC,
  PRECT hRect,
  HMENU hMenu,
  HFONT hFont);

UINT
NTAPI
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable);

UINT
NTAPI
NtUserEnumClipboardFormats(
  UINT format);

DWORD
NTAPI
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii);

BOOL
NTAPI
NtUserEndMenu(VOID);

UINT NTAPI
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags);

BOOL
NTAPI
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi);

UINT
NTAPI
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID);

BOOL
NTAPI
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem);

HMENU
NTAPI
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert);

BOOL
NTAPI
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite);

BOOL
NTAPI
NtUserMenuInfo(
 HMENU hmenu,
 PROSMENUINFO lpmi,
 BOOL fsog
);

int
NTAPI
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y);

BOOL
NTAPI
NtUserMenuItemInfo(
 HMENU hMenu,
 UINT uItem,
 BOOL fByPosition,
 PROSMENUITEMINFO lpmii,
 BOOL fsog
);

BOOL
NTAPI
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags);

BOOL
NTAPI
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint);

BOOL
NTAPI
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId);

BOOL
NTAPI
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos);

BOOL
NTAPI
NtUserSetMenuFlagRtoL(
  HMENU hMenu);

BOOL
NTAPI
NtUserSetSystemMenu(
  HWND hWnd,
  HMENU hMenu);

DWORD
NTAPI
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi);

DWORD
NTAPI
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption);

BOOL
NTAPI
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm);

ULONG NTAPI
NtUserGetSystemMetrics(ULONG Index);

ULONG_PTR NTAPI
NtUserGetClassLong(HWND hWnd, INT Offset, BOOL Ansi);

LONG NTAPI
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi);

BOOL NTAPI
NtUserGetWindowRect(HWND hWnd, LPRECT Rect);

BOOL NTAPI
NtUserGetClientRect(HWND hWnd, LPRECT Rect);

HANDLE NTAPI
NtUserGetProp(HWND hWnd, ATOM Atom);

BOOL NTAPI
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point);

HWND NTAPI
NtUserGetDesktopWindow();

NTSTATUS
NTAPI
NtUserAcquireOrReleaseInputOwnership(
  BOOLEAN Release);

HKL
NTAPI
NtUserActivateKeyboardLayout(
  HKL hKl,
  ULONG Flags);

DWORD
NTAPI
NtUserAlterWindowStyle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserAttachThreadInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HDC NTAPI
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs);

DWORD
NTAPI
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
NTAPI
NtUserBlockInput(
  BOOL BlockIt);

ULONG
NTAPI
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG nBufSize);

NTSTATUS NTAPI
NtUserBuildNameList(
   HWINSTA hWinSta,
   ULONG dwSize,
   PVOID lpBuffer,
   PULONG pRequiredSize);

NTSTATUS
NTAPI
NtUserBuildPropList(
  HWND hWnd,
  LPVOID Buffer,
  DWORD BufferSize,
  DWORD *Count);

enum {
	HWND_ROUTINE_REGISTERSHELLHOOKWINDOW,
	HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW
};

DWORD
NTAPI
NtUserCallHwnd(
  DWORD Unknown0,
  DWORD Unknown1);

#define HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS      0x54
#define HWNDLOCK_ROUTINE_DRAWMENUBAR               0x55
#define HWNDLOCK_ROUTINE_REDRAWFRAME               0x58
#define HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW       0x5B
#define HWNDLOCK_ROUTINE_UPDATEWINDOW              0x5E
BOOL
NTAPI
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Routine);

#define HWNDOPT_ROUTINE_SETPROGMANWINDOW       0x4A
#define HWNDOPT_ROUTINE_SETTASKMANWINDOW       0x4B
HWND
NTAPI
NtUserCallHwndOpt(
  HWND Param,
  DWORD Routine);

DWORD
NTAPI
NtUserCallHwndParam(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserCallHwndParamLock(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

BOOL
NTAPI
NtUserCallMsgFilter(
  LPMSG msg,
  INT code);
 
LRESULT
NTAPI
NtUserCallNextHookEx(
  HHOOK Hook,
  int Code,
  WPARAM wParam,
  LPARAM lParam);

#define NOPARAM_ROUTINE_CREATEMENU            0x0
#define NOPARAM_ROUTINE_CREATEMENUPOPUP       0x1
#define NOPARAM_ROUTINE_MSQCLEARWAKEMASK      0x3
#define NOPARAM_ROUTINE_REGISTER_PRIMITIVE	  0xffff0001 /* Private ROS */
#define NOPARAM_ROUTINE_DESTROY_CARET         0xffff0002
#define NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP   0xffff0003
#define NOPARAM_ROUTINE_INIT_MESSAGE_PUMP     0xffff0004
#define NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO   0xffff0005
#define NOPARAM_ROUTINE_ANYPOPUP              0xffff0006
#define NOPARAM_ROUTINE_CSRSS_INITIALIZED     0xffff0007
DWORD
NTAPI
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
#define ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO  0x0a
#define ONEPARAM_ROUTINE_GETCURSORPOSITION    0x0b
#define ONEPARAM_ROUTINE_ISWINDOWINDESTROY    0x0c
#define ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING 0x0d
#define ONEPARAM_ROUTINE_GETWINDOWINSTANCE    0x10
#define ONEPARAM_ROUTINE_MSQSETWAKEMASK       0x27
#define ONEPARAM_ROUTINE_GETKEYBOARDTYPE      0x28
#define ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT    0x29
#define ONEPARAM_ROUTINE_SHOWCURSOR           0x30
#define ONEPARAM_ROUTINE_REGISTERUSERMODULE   0x31
#define ONEPARAM_ROUTINE_RELEASEDC            0x39
DWORD
NTAPI
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
#define TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID 0x58
#define TWOPARAM_ROUTINE_SETCARETPOS        0x60
#define TWOPARAM_ROUTINE_GETWINDOWINFO      0x61
#define TWOPARAM_ROUTINE_REGISTERLOGONPROC  0x62
#define TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES 0x63
#define TWOPARAM_ROUTINE_GETSYSCOLORPENS    0x64
#define TWOPARAM_ROUTINE_GETSYSCOLORS       0x65
#define TWOPARAM_ROUTINE_SETSYSCOLORS       0x66
#define TWOPARAM_ROUTINE_ROS_SHOWWINDOW     0x1000
#define TWOPARAM_ROUTINE_ROS_ISACTIVEICON   0x1001
#define TWOPARAM_ROUTINE_ROS_NCDESTROY      0x1002
DWORD
NTAPI
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine);

BOOL
NTAPI
NtUserChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext);

LONG
NTAPI
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam);

DWORD
STDCALL
NtUserCheckImeHotKey(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

HWND NTAPI
NtUserChildWindowFromPointEx(HWND Parent,
			     LONG x,
			     LONG y,
			     UINT Flags);

BOOL
NTAPI
NtUserClipCursor(
  RECT *lpRect);

BOOL
NTAPI
NtUserCloseClipboard(VOID);

BOOL
NTAPI
NtUserCloseDesktop(
  HDESK hDesktop);

BOOL
NTAPI
NtUserCloseWindowStation(
  HWINSTA hWinSta);

DWORD
NTAPI
NtUserConsoleControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1);

int
NTAPI
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  int EntriesCount);

DWORD
NTAPI
NtUserCountClipboardFormats(VOID);

HACCEL
NTAPI
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount);

BOOL
NTAPI
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight);

HANDLE
NTAPI
NtUserCreateCursorIconHandle(
  PICONINFO IconInfo,
  BOOL Indirect);

HDESK
NTAPI
NtUserCreateDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  HWINSTA hWindowStation);

DWORD
NTAPI
NtUserCreateInputContext(
    DWORD dwUnknown1);

DWORD
NTAPI
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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
  DWORD dwUnknown);

HWINSTA
NTAPI
NtUserCreateWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
NTAPI
NtUserDdeGetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
NTAPI
NtUserDdeSetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HDWP NTAPI
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     int x,
         int y,
         int cx,
         int cy,
		     UINT Flags);
BOOL NTAPI
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText);

BOOLEAN
NTAPI
NtUserDestroyAcceleratorTable(
  HACCEL Table);

BOOL
NTAPI
NtUserDestroyCursor(
  HANDLE Handle,
  DWORD Unknown);

DWORD
NTAPI
NtUserDestroyInputContext(
    DWORD dwUnknown1);

BOOLEAN NTAPI
NtUserDestroyWindow(HWND Wnd);

DWORD
NTAPI
NtUserDisableThreadIme(
    DWORD dwUnknown1);

typedef struct tagNTUSERDISPATCHMESSAGEINFO
{
  BOOL HandledByKernel;
  BOOL Ansi;
  WNDPROC Proc;
  MSG Msg;
} NTUSERDISPATCHMESSAGEINFO, *PNTUSERDISPATCHMESSAGEINFO;

LRESULT
NTAPI
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO MsgInfo);

BOOL
NTAPI
NtUserDragDetect(
  HWND hWnd,
  LONG x,
  LONG y);

DWORD
NTAPI
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1);

DWORD
NTAPI
NtUserDrawAnimatedRects(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserDrawCaption(
   HWND hWnd,
   HDC hDc,
   LPCRECT lpRc,
   UINT uFlags);

BOOL
STDCALL
NtUserDrawCaptionTemp(
  HWND hWnd,
  HDC hDC,
  LPCRECT lpRc,
  HFONT hFont,
  HICON hIcon,
  const PUNICODE_STRING str,
  UINT uFlags);

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
  DWORD Unknown1);

DWORD
NTAPI
NtUserEmptyClipboard(VOID);

BOOL
NTAPI
NtUserEnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows);

DWORD
NTAPI
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL NTAPI
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs);

BOOL
NTAPI
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICE lpDisplayDevice, /* device information */
  DWORD dwFlags ); /* reserved */

/*BOOL
NTAPI
NtUserEnumDisplayMonitors (
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData );*/

INT
NTAPI
NtUserEnumDisplayMonitors(
  OPTIONAL IN HDC hDC,
  OPTIONAL IN LPCRECT pRect,
  OPTIONAL OUT HMONITOR *hMonitorList,
  OPTIONAL OUT LPRECT monitorRectList,
  OPTIONAL IN DWORD listSize );


BOOL
NTAPI
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags );

DWORD
NTAPI
NtUserEvent(
  DWORD Unknown0);

DWORD
NTAPI
NtUserExcludeUpdateRgn(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserFillWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

HICON
NTAPI
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  LONG cx,
  LONG cy);

HWND
NTAPI
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING  ucClassName,
  PUNICODE_STRING  ucWindowName,
  DWORD dwUnknown
  );

DWORD
NTAPI
NtUserFlashWindowEx(
  DWORD Unknown0);

DWORD
NTAPI
NtUserGetAltTabInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

HWND NTAPI
NtUserGetAncestor(HWND hWnd, UINT Flags);

DWORD
NTAPI
NtUserGetAppImeLevel(
    DWORD dwUnknown1);

DWORD
NTAPI
NtUserGetAsyncKeyState(
  DWORD Unknown0);

DWORD
NTAPI
NtUserGetAtomName(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

UINT
NTAPI
NtUserGetCaretBlinkTime(VOID);

BOOL
NTAPI
NtUserGetCaretPos(
  LPPOINT lpPoint);

BOOL NTAPI
NtUserGetClassInfo(HINSTANCE hInstance,
		   PUNICODE_STRING ClassName,
		   LPWNDCLASSEXW wcex,
		   BOOL Ansi);

INT
NTAPI
NtUserGetClassName(HWND hWnd,
		   PUNICODE_STRING ClassName,
                   BOOL Ansi);

HANDLE
NTAPI
NtUserGetClipboardData(
  UINT uFormat,
  DWORD Unknown1);

INT
NTAPI
NtUserGetClipboardFormatName(
  UINT format,
  PUNICODE_STRING FormatName,
  INT cchMaxCount);

HWND
NTAPI
NtUserGetClipboardOwner(VOID);

DWORD
NTAPI
NtUserGetClipboardSequenceNumber(VOID);

HWND
NTAPI
NtUserGetClipboardViewer(VOID);

BOOL
NTAPI
NtUserGetClipCursor(
  RECT *lpRect);

DWORD
NTAPI
NtUserGetComboBoxInfo(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserGetControlBrush(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserGetControlColor(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserGetCursorInfo(
  PCURSORINFO pci);

HDC
NTAPI
NtUserGetDC(
  HWND hWnd);

HDC
NTAPI
NtUserGetDCEx(
  HWND hWnd,
  HANDLE hRegion,
  ULONG Flags);

UINT
NTAPI
NtUserGetDoubleClickTime(VOID);

HWND
NTAPI
NtUserGetForegroundWindow(VOID);

DWORD
NTAPI
NtUserGetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags);

BOOL
NTAPI
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui);

BOOL
NTAPI
NtUserGetCursorIconInfo(
  HANDLE Handle,
  PICONINFO IconInfo);

BOOL
NTAPI
NtUserGetCursorIconSize(
  HANDLE Handle,
  BOOL *fIcon,
  SIZE *Size);

DWORD
NTAPI
NtUserGetIconInfo(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6);

DWORD
NTAPI
NtUserGetIconSize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

DWORD
NTAPI
NtUserGetInternalWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HKL
NTAPI 
NtUserGetKeyboardLayout(
  DWORD dwThreadid);

UINT
NTAPI
NtUserGetKeyboardLayoutList(
  INT nItems,
  HKL *pHklBuff);

BOOL
NTAPI
NtUserGetKeyboardLayoutName(
  LPWSTR lpszName);

DWORD
NTAPI
NtUserGetKeyboardState(
  LPBYTE Unknown0);

DWORD
NTAPI
NtUserGetKeyboardType(
  DWORD TypeFlag);

DWORD
NTAPI
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize );
  
DWORD
NTAPI
NtUserGetKeyState(
  DWORD Unknown0);

BOOL
NTAPI
NtUserGetLastInputInfo(
    PLASTINPUTINFO plii);

DWORD
NTAPI
NtUserGetLayeredWindowAttributes(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserGetListBoxInfo(
  DWORD Unknown0);

typedef struct tagNTUSERGETMESSAGEINFO
{
  MSG Msg;
  ULONG LParamSize;
} NTUSERGETMESSAGEINFO, *PNTUSERGETMESSAGEINFO;

BOOL
NTAPI
NtUserGetMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax);

BOOL
NTAPI
NtUserGetMonitorInfo(
  IN HMONITOR hMonitor,
  OUT LPMONITORINFO pMonitorInfo);

DWORD
NTAPI
NtUserGetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution);

BOOL
NTAPI
NtUserGetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength,
  PDWORD nLengthNeeded);

HWND
NTAPI
NtUserGetOpenClipboardWindow(VOID);

INT
NTAPI
NtUserGetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  INT cFormats);

HWINSTA
NTAPI
NtUserGetProcessWindowStation(VOID);

DWORD
NTAPI
NtUserGetRawInputBuffer(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserGetRawInputData(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
NTAPI
NtUserGetRawInputDeviceInfo(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserGetRawInputDeviceList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

BOOL
NTAPI
NtUserGetScrollBarInfo(
  HWND hWnd, 
  LONG idObject, 
  PSCROLLBARINFO psbi);

BOOL
NTAPI
NtUserGetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPSCROLLINFO lpsi);

HDESK
NTAPI
NtUserGetThreadDesktop(
  DWORD dwThreadId,
  DWORD Unknown1);

#define THREADSTATE_GETTHREADINFO   (0)
#define THREADSTATE_FOCUSWINDOW (1)
#define THREADSTATE_INSENDMESSAGE       (2)
#define THREADSTATE_PROGMANWINDOW (3)
#define THREADSTATE_TASKMANWINDOW (4)
DWORD
NTAPI
NtUserGetThreadState(
  DWORD Routine);

DWORD
NTAPI
NtUserGetTitleBarInfo(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL NTAPI
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase);

int
NTAPI
NtUserGetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase);

DWORD
NTAPI
NtUserGetWindowDC(
  HWND hWnd);

BOOL
NTAPI
NtUserGetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl);

DWORD
NTAPI
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserImpersonateDdeClientWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserInitialize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserInitializeClientPfnArrays(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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
  DWORD Unknown10);

INT
NTAPI
NtUserInternalGetWindowText(
  HWND hWnd,
  LPWSTR lpString,
  INT nMaxCount);

BOOL
NTAPI
NtUserInvalidateRect(
    HWND hWnd,
    CONST RECT *lpRect,
    BOOL bErase);

BOOL
NTAPI
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase);

BOOL
NTAPI
NtUserIsClipboardFormatAvailable(
  UINT format);

BOOL
NTAPI
NtUserKillSystemTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
);

BOOL
NTAPI
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
);

HKL
STDCALL
NtUserLoadKeyboardLayoutEx( 
   IN HANDLE Handle,
   IN DWORD offTable,
   IN PUNICODE_STRING puszKeyboardName,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags);

BOOL
NTAPI
NtUserLockWindowStation(
  HWINSTA hWindowStation);

DWORD
NTAPI
NtUserLockWindowUpdate(
  DWORD Unknown0);

DWORD
NTAPI
NtUserLockWorkStation(VOID);

UINT
NTAPI
NtUserMapVirtualKeyEx( UINT keyCode,
		       UINT transType,
		       DWORD keyboardId,
		       HKL dwhkl );

DWORD
NTAPI
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
NTAPI
NtUserMinMaximize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

BOOL
NTAPI
NtUserGetMinMaxInfo(
  HWND hwnd,
  MINMAXINFO *MinMaxInfo,
  BOOL SendMessage);

DWORD
NTAPI
NtUserMNDragLeave(VOID);

DWORD
NTAPI
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1);

HMONITOR
NTAPI
NtUserMonitorFromPoint(
  IN POINT point,
  IN DWORD dwFlags);

HMONITOR
NTAPI
NtUserMonitorFromRect(
  IN LPCRECT pRect,
  IN DWORD dwFlags);

HMONITOR
NTAPI
NtUserMonitorFromWindow(
  IN HWND hWnd,
  IN DWORD dwFlags);


BOOL
NTAPI
NtUserMoveWindow(      
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint
);

DWORD
NTAPI
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserNotifyProcessCreate(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserNotifyWinEvent(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserOpenClipboard(
  HWND hWnd,
  DWORD Unknown1);

HDESK
NTAPI
NtUserOpenDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess);

HDESK
NTAPI
NtUserOpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess);

HWINSTA
NTAPI
NtUserOpenWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess);

BOOL
NTAPI
NtUserPaintDesktop(
  HDC hDC);

DWORD
NTAPI
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6);

BOOL
NTAPI
NtUserPeekMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg);

BOOL
NTAPI
NtUserPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

BOOL
NTAPI
NtUserPostThreadMessage(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

DWORD
NTAPI
NtUserPrintWindow(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserProcessConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserQueryInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
NTAPI
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

DWORD
NTAPI
NtUserQuerySendMessage(
  DWORD Unknown0);

DWORD
NTAPI
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
NTAPI
NtUserQueryWindow(
  HWND hWnd,
  DWORD Index);

DWORD
NTAPI
NtUserRealInternalGetMessage(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6);

UINT
NTAPI
NtUserRealizePalette(HDC hDC);

DWORD
NTAPI
NtUserRealChildWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

BOOL
NTAPI
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
);

HWINSTA
NTAPI
NtUserRegisterClassExWOW(
    CONST WNDCLASSEXW* lpwcx,
    BOOL bUnicodeClass,
    WNDPROC wpExtra,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6,
    DWORD dwUnknown7);

DWORD
NTAPI
NtUserRegisterRawInputDevices(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

/* FIXME: These flag constans aren't what Windows uses. */
#define REGISTERCLASS_ANSI	2
#define REGISTERCLASS_SYSTEM	4
#define REGISTERCLASS_ALL	(REGISTERCLASS_ANSI | REGISTERCLASS_SYSTEM)

RTL_ATOM NTAPI
NtUserRegisterClassEx(
   CONST WNDCLASSEXW* lpwcx,
   PUNICODE_STRING ClassName,
   PUNICODE_STRING MenuName,
   WNDPROC wpExtra,
   DWORD Flags,
   HMENU hMenu);
   
UINT
NTAPI
NtUserRegisterClipboardFormat(
    PUNICODE_STRING format);

BOOL
NTAPI
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk);

BOOL
NTAPI
NtUserGetLastInputInfo(
    PLASTINPUTINFO plii);

DWORD
NTAPI
NtUserRegisterTasklist(
  DWORD Unknown0);

UINT NTAPI
NtUserRegisterWindowMessage(PUNICODE_STRING MessageName);

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserRemoteRedrawScreen(VOID);

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(VOID);

HANDLE NTAPI
NtUserRemoveProp(HWND hWnd, ATOM Atom);

DWORD
NTAPI
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserResolveDesktopForWOW(
  DWORD Unknown0);

DWORD
NTAPI
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip ,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate);

DWORD NTAPI
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags);

UINT
NTAPI
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

LRESULT NTAPI
NtUserSendMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam,
                  PNTUSERSENDMESSAGEINFO Info);

BOOL
NTAPI
NtUserSendMessageCallback(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData);

LRESULT NTAPI
NtUserSendMessageTimeout(HWND hWnd,
			 UINT Msg,
			 WPARAM wParam,
			 LPARAM lParam,
			 UINT uFlags,
			 UINT uTimeout,
			 ULONG_PTR *uResult,
             PNTUSERSENDMESSAGEINFO Info);

BOOL
NTAPI
NtUserSendNotifyMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

HWND NTAPI
NtUserSetActiveWindow(HWND Wnd);

HWND NTAPI
NtUserGetActiveWindow(VOID);

DWORD
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

HWND NTAPI
NtUserSetCapture(HWND Wnd);

HWND NTAPI
NtUserGetCapture(VOID);

ULONG_PTR NTAPI
NtUserSetClassLong(
  HWND  hWnd,
  INT Offset,
  ULONG_PTR  dwNewLong,
  BOOL  Ansi );

DWORD
NTAPI
NtUserSetClassWord(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HANDLE
NTAPI
NtUserSetClipboardData(
  UINT uFormat,
  HANDLE hMem,
  DWORD Unknown2);

HWND
NTAPI
NtUserSetClipboardViewer(
  HWND hWndNewViewer);

HPALETTE
STDCALL
NtUserSelectPalette(
    HDC hDC,
    HPALETTE  hpal,
    BOOL  ForceBackground
);

DWORD
NTAPI
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1);

HCURSOR
NTAPI
NtUserSetCursor(
  HCURSOR hCursor);

BOOL
NTAPI
NtUserSetCursorContents(
  HANDLE Handle,
  PICONINFO IconInfo);

BOOL
NTAPI
NtUserSetCursorIconData(
  HANDLE Handle,
  PBOOL fIcon,
  POINT *Hotspot,
  HMODULE hModule,
  HRSRC hRsrc,
  HRSRC hGroupRsrc);

DWORD
NTAPI
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1);

HWND
NTAPI
NtUserSetFocus(
  HWND hWnd);

DWORD
NTAPI
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
NTAPI
NtUserSetImeInfoEx(
    DWORD dwUnknown1);

DWORD
NTAPI
NtUserSetImeOwnerWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserSetInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserSetInternalWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserSetKeyboardState(
  LPBYTE Unknown0);

DWORD
NTAPI
NtUserSetLayeredWindowAttributes(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserSetLogonNotifyWindow(
  DWORD Unknown0);

BOOL
NTAPI
NtUserSetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength);

HWND
NTAPI
NtUserSetParent(
  HWND hWndChild,
  HWND hWndNewParent);

BOOL
NTAPI
NtUserSetProcessWindowStation(
  HWINSTA hWindowStation);

BOOL NTAPI
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data);

DWORD
NTAPI
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserSetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPCSCROLLINFO lpsi, 
  BOOL bRedraw);

BOOL
NTAPI
NtUserSetShellWindowEx(
  HWND hwndShell,
  HWND hwndShellListView);

HWND
NTAPI
NtUserGetShellWindow();

DWORD
NTAPI
NtUserSetSysColors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id);

BOOL
NTAPI
NtUserSetThreadDesktop(
  HDESK hDesktop);

DWORD
NTAPI
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1);

UINT_PTR
NTAPI
NtUserSetSystemTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
);

DWORD
NTAPI
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

UINT_PTR
NTAPI
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
);

DWORD
NTAPI
NtUserSetWindowFNID(
  DWORD Unknown0,
  DWORD Unknown1);

LONG
NTAPI
NtUserSetWindowLong(
  HWND hWnd,
  DWORD Index,
  LONG NewValue,
  BOOL Ansi);

BOOL
NTAPI
NtUserSetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl);

BOOL 
NTAPI NtUserSetWindowPos(      
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags
);

INT
NTAPI
NtUserSetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw);

DWORD
NTAPI
NtUserSetWindowsHookAW(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HHOOK
NTAPI
NtUserSetWindowsHookEx(
  HINSTANCE Mod,
  PUNICODE_STRING ModuleName,
  DWORD ThreadId,
  int HookId,
  HOOKPROC HookProc,
  BOOL Ansi);

DWORD
NTAPI
NtUserSetWindowStationUser(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

WORD NTAPI
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal);

DWORD
NTAPI
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
NTAPI
NtUserShowCaret(
  HWND hWnd);

BOOL
NTAPI
NtUserHideCaret(
  HWND hWnd);

DWORD
NTAPI
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow);

BOOL
NTAPI
NtUserShowWindow(
  HWND hWnd,
  LONG nCmdShow);

BOOL
NTAPI
NtUserShowWindowAsync(
  HWND hWnd,
  LONG nCmdShow);

DWORD
NTAPI
NtUserSoundSentry(VOID);

BOOL
NTAPI
NtUserSwitchDesktop(
  HDESK hDesktop);

BOOL
NTAPI
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni);

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1);

INT
NTAPI
NtUserToUnicodeEx(
		  UINT wVirtKey,
		  UINT wScanCode,
		  PBYTE lpKeyState,
		  LPWSTR pwszBuff,
		  int cchBuff,
		  UINT wFlags,
		  HKL dwhkl );

DWORD
NTAPI
NtUserTrackMouseEvent(
  DWORD Unknown0);

int
NTAPI
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message);

BOOL
NTAPI
NtUserTranslateMessage(
  LPMSG lpMsg,
  HKL dwhkl );

BOOL
NTAPI
NtUserUnhookWindowsHookEx(
  HHOOK Hook);

DWORD
NTAPI
NtUserUnhookWinEvent(
  DWORD Unknown0);

BOOL
NTAPI
NtUserUnloadKeyboardLayout(
  HKL hKl);

BOOL
NTAPI
NtUserUnlockWindowStation(
  HWINSTA hWindowStation);

BOOL
NTAPI
NtUserUnregisterClass(
  PUNICODE_STRING ClassNameOrAtom,
  HINSTANCE hInstance);

BOOL
NTAPI
NtUserUnregisterHotKey(HWND hWnd,
		       int id);

DWORD
NTAPI
NtUserUnregisterUserApiHook(VOID);

DWORD
NTAPI
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
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

BOOL
NTAPI
NtUserUpdatePerUserSystemParameters(
  DWORD dwReserved,
  BOOL bEnable);

DWORD
NTAPI
NtUserUserHandleGrantAccess(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

BOOL
NTAPI
NtUserValidateHandleSecure(
  HANDLE hHdl,
  BOOL Restricted);

BOOL
NTAPI
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect);

DWORD
NTAPI
NtUserValidateTimerCallback(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserVkKeyScanEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserWaitForInputIdle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserWaitForMsgAndEvent(
  DWORD Unknown0);

BOOL
NTAPI
NtUserWaitMessage(VOID);

DWORD
NTAPI
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

HWND
NTAPI
NtUserWindowFromPoint(
  LONG X,
  LONG Y);

DWORD
NTAPI
NtUserYieldTask(VOID);

DWORD NTAPI
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid);

DWORD NTAPI
NtUserGetQueueStatus(BOOL ClearChanges);

HWND NTAPI
NtUserGetParent(HWND hWnd);

HWND NTAPI
NtUserGetWindow(HWND hWnd, UINT Relationship);

HWND NTAPI
NtUserGetLastActivePopup(HWND hWnd);

typedef struct _WNDPROC_INFO
{
    WNDPROC WindowProc;
    BOOL IsUnicode;
} WNDPROC_INFO, *PWNDPROC_INFO;
                                    
BOOL NTAPI
NtUserDereferenceWndProcHandle(IN HANDLE wpHandle, OUT PWNDPROC_INFO wpInfo);

VOID NTAPI
NtUserManualGuiCheck(LONG Check);

#define NtUserGetDCBrushColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_BRUSH, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserGetDCPenColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_PEN, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserSetDCBrushColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCBRUSHCOLOR)

#define NtUserSetDCPenColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCPENCOLOR)

typedef struct _SETSCROLLBARINFO
{
  int nTrackPos; 
  int reserved;
  DWORD rgstate[CCHILDREN_SCROLLBAR+1];
} SETSCROLLBARINFO, *PSETSCROLLBARINFO;

BOOL
NTAPI
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
