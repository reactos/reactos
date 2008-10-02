/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntuser.h
 * PURPOSE:         Win32 Shared USER Types for NtUser*
 * PROGRAMMER:      Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

/* DEFINES *******************************************************************/

/* NtUserQueryWindow */
#define QUERY_WINDOW_UNIQUE_PROCESS_ID  0x00
#define QUERY_WINDOW_UNIQUE_THREAD_ID   0x01
#define QUERY_WINDOW_ACTIVE             0x02
#define QUERY_WINDOW_FOCUS              0x03
#define QUERY_WINDOW_ISHUNG             0x04

// FNID's for NtUserSetWindowFNID, NtUserMessageCall
#define FNID_SCROLLBAR              0x029A
#define FNID_ICONTITLE              0x029B
#define FNID_MENU                   0x029C
#define FNID_DESKTOP                0x029D
#define FNID_DEFWINDOWPROC          0x029E
#define FNID_SWITCH                 0x02A0
#define FNID_BUTTON                 0x02A1
#define FNID_COMBOBOX               0x02A2
#define FNID_COMBOLBOX              0x02A3
#define FNID_DIALOG                 0x02A4
#define FNID_EDIT                   0x02A5
#define FNID_LISTBOX                0x02A6
#define FNID_MDICLIENT              0x02A7
#define FNID_STATIC                 0x02A8
#define FNID_IME                    0x02A9
#define FNID_CALLWNDPROC            0x02AA
#define FNID_CALLWNDPROCRET         0x02AB
#define FNID_SENDMESSAGE            0x02B0
// Kernel has option to use TimeOut or normal msg send, based on type of msg.
#define FNID_SENDMESSAGEWTOOPTION   0x02B1
#define FNID_SENDMESSAGETIMEOUT     0x02B2
#define FNID_BROADCASTSYSTEMMESSAGE 0x02B4
#define FNID_TOOLTIPS               0x02B5
#define FNID_UNKNOWN                0x02B6
#define FNID_SENDNOTIFYMESSAGE      0x02B7
#define FNID_SENDMESSAGECALLBACK    0x02B8

 
#define FNID_DDEML       0x2000 // Registers DDEML
#define FNID_DESTROY     0x4000 // This is sent when WM_NCDESTROY or in the support routine.
                                // Seen during WM_CREATE on error exit too.

// ICLS's for NtUserGetClassName FNID to ICLS, NtUserInitializeClientPfnArrays
#define ICLS_BUTTON       0
#define ICLS_EDIT         1
#define ICLS_STATIC       2
#define ICLS_LISTBOX      3
#define ICLS_SCROLLBAR    4
#define ICLS_COMBOBOX     5
#define ICLS_MDICLIENT    6
#define ICLS_COMBOLBOX    7
#define ICLS_DDEMLEVENT   8
#define ICLS_DDEMLMOTHER  9
#define ICLS_DDEML16BIT   10
#define ICLS_DDEMLCLIENTA 11
#define ICLS_DDEMLCLIENTW 12
#define ICLS_DDEMLSERVERA 13
#define ICLS_DDEMLSERVERW 14
#define ICLS_IME          15
#define ICLS_DESKTOP      16
#define ICLS_DIALOG       17
#define ICLS_MENU         18
#define ICLS_SWITCH       19
#define ICLS_ICONTITLE    20
#define ICLS_TOOLTIPS     21
#define ICLS_UNKNOWN      22
#define ICLS_NOTUSED      23
#define ICLS_END          31

// Server event activity bits.
#define SRV_EVENT_MENU            0x0001
#define SRV_EVENT_END_APPLICATION 0x0002
#define SRV_EVENT_RUNNING         0x0004
#define SRV_EVENT_NAMECHANGE      0x0008
#define SRV_EVENT_VALUECHANGE     0x0010
#define SRV_EVENT_STATECHANGE     0x0020
#define SRV_EVENT_LOCATIONCHANGE  0x0040
#define SRV_EVENT_CREATE          0x8000

/* ENUMERATIONS **************************************************************/

/* apfnSimpleCall indices from Windows XP SP 2 */
/* TODO: Check for differences in Windows 2000, 2003 and 2008 */
#define WIN32K_VERSION NTDDI_WINXPSP2 // FIXME: this should go somewhere else

enum SimpleCallRoutines
{
	NOPARAM_ROUTINE_CREATEMENU,
	NOPARAM_ROUTINE_CREATEMENUPOPUP,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_ALLOWFOREGNDACTIVATION,
	NOPARAM_ROUTINE_MSQCLEARWAKEMASK,
	NOPARAM_ROUTINE_CREATESYSTEMTHREADS,
	NOPARAM_ROUTINE_DESTROY_CARET,
#endif
	NOPARAM_ROUTINE_ENABLEPROCWNDGHSTING,
#if (WIN32K_VERSION < NTDDI_VISTA)
	NOPARAM_ROUTINE_MSQCLEARWAKEMASK,
	NOPARAM_ROUTINE_ALLOWFOREGNDACTIVATION,
	NOPARAM_ROUTINE_DESTROY_CARET,
#endif
	NOPARAM_ROUTINE_GETDEVICECHANGEINFO,
	NOPARAM_ROUTINE_GETIMESHOWSTATUS,
	NOPARAM_ROUTINE_GETINPUTDESKTOP,
	NOPARAM_ROUTINE_GETMSESSAGEPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_HANDLESYSTHRDCREATFAIL,
#else
	NOPARAM_ROUTINE_GETREMOTEPROCID,
#endif
	NOPARAM_ROUTINE_HIDECURSORNOCAPTURE,
	NOPARAM_ROUTINE_LOADCURSANDICOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_LOADUSERAPIHOOK,
	NOPARAM_ROUTINE_PREPAREFORLOGOFF, /* 0x0f */
#endif
	NOPARAM_ROUTINE_RELEASECAPTURE,
	NOPARAM_ROUTINE_RESETDBLCLICK,
	NOPARAM_ROUTINE_ZAPACTIVEANDFOUS,
	NOPARAM_ROUTINE_REMOTECONSHDWSTOP,
	NOPARAM_ROUTINE_REMOTEDISCONNECT,
	NOPARAM_ROUTINE_REMOTELOGOFF,
	NOPARAM_ROUTINE_REMOTENTSECURITY,
	NOPARAM_ROUTINE_REMOTESHDWSETUP,
	NOPARAM_ROUTINE_REMOTESHDWSTOP,
	NOPARAM_ROUTINE_REMOTEPASSTHRUENABLE,
	NOPARAM_ROUTINE_REMOTEPASSTHRUDISABLE,
	NOPARAM_ROUTINE_REMOTECONNECTSTATE,
	NOPARAM_ROUTINE_UPDATEPERUSERIMMENABLING,
	NOPARAM_ROUTINE_USERPWRCALLOUTWORKER,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_WAKERITFORSHTDWN,
#endif
	NOPARAM_ROUTINE_INIT_MESSAGE_PUMP,
	NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP,
#if (WIN32K_VERSION < NTDDI_VISTA)
	NOPARAM_ROUTINE_LOADUSERAPIHOOK,
#endif
	ONEPARAM_ROUTINE_BEGINDEFERWNDPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_GETSENDMSGRECVR,
#endif
	ONEPARAM_ROUTINE_WINDOWFROMDC,
	ONEPARAM_ROUTINE_ALLOWSETFOREGND,
	ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_CREATESYSTEMTHREADS,
#endif
	ONEPARAM_ROUTINE_CSDDEUNINITIALIZE,
	ONEPARAM_ROUTINE_DIRECTEDYIELD,
	ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_GETCURSORPOS,
#endif
	ONEPARAM_ROUTINE_GETINPUTEVENT,
	ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT,
	ONEPARAM_ROUTINE_GETKEYBOARDTYPE,
	ONEPARAM_ROUTINE_GETPROCDEFLAYOUT,
	ONEPARAM_ROUTINE_GETQUEUESTATUS,
	ONEPARAM_ROUTINE_GETWINSTAINFO,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_HANDLESYSTHRDCREATFAIL,
#endif
	ONEPARAM_ROUTINE_LOCKFOREGNDWINDOW,
	ONEPARAM_ROUTINE_LOADFONTS,
	ONEPARAM_ROUTINE_MAPDEKTOPOBJECT,
	ONEPARAM_ROUTINE_MESSAGEBEEP,
	ONEPARAM_ROUTINE_PLAYEVENTSOUND,
	ONEPARAM_ROUTINE_POSTQUITMESSAGE,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_PREPAREFORLOGOFF,
#endif
	ONEPARAM_ROUTINE_REALIZEPALETTE,
	ONEPARAM_ROUTINE_REGISTERLPK,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_REGISTERSYSTEMTHREAD,
#endif
	ONEPARAM_ROUTINE_REMOTERECONNECT,
	ONEPARAM_ROUTINE_REMOTETHINWIRESTATUS,
	ONEPARAM_ROUTINE_RELEASEDC,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_REMOTENOTIFY,
#endif
	ONEPARAM_ROUTINE_REPLYMESSAGE,
	ONEPARAM_ROUTINE_SETCARETBLINKTIME,
	ONEPARAM_ROUTINE_SETDBLCLICKTIME,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_SETIMESHOWSTATUS,
#endif
	ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO,
	ONEPARAM_ROUTINE_SETPROCDEFLAYOUT,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_SETWATERMARKSTRINGS,
#endif
	ONEPARAM_ROUTINE_SHOWCURSOR,
	ONEPARAM_ROUTINE_SHOWSTARTGLASS,
	ONEPARAM_ROUTINE_SWAPMOUSEBUTTON,
	X_ROUTINE_WOWMODULEUNLOAD,
#if (WIN32K_VERSION < NTDDI_VISTA)
	X_ROUTINE_REMOTENOTIFY,
#endif
	HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW,
	HWND_ROUTINE_DWP_GETENABLEDPOPUP,
	HWND_ROUTINE_GETWNDCONTEXTHLPID,
	HWND_ROUTINE_REGISTERSHELLHOOKWINDOW,
	HWND_ROUTINE_SETMSGBOX,
	HWNDOPT_ROUTINE_SETPROGMANWINDOW,
	HWNDOPT_ROUTINE_SETTASKMANWINDOW,
	HWNDPARAM_ROUTINE_GETCLASSICOCUR,
	HWNDPARAM_ROUTINE_CLEARWINDOWSTATE,
	HWNDPARAM_ROUTINE_KILLSYSTEMTIMER,
	HWNDPARAM_ROUTINE_SETDIALOGPOINTER,
	HWNDPARAM_ROUTINE_SETVISIBLE,
	HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID,
	HWNDPARAM_ROUTINE_SETWINDOWSTATE,
	HWNDLOCK_ROUTINE_WINDOWHASSHADOW, /* correct prefix ? */
	HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS,
	HWNDLOCK_ROUTINE_DRAWMENUBAR,
	HWNDLOCK_ROUTINE_CHECKIMESHOWSTATUSINTHRD,
	HWNDLOCK_ROUTINE_GETSYSMENUHANDLE,
	HWNDLOCK_ROUTINE_REDRAWFRAME,
	HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK,
	HWNDLOCK_ROUTINE_SETDLGSYSMENU,
	HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW,
	HWNDLOCK_ROUTINE_SETSYSMENU,
	HWNDLOCK_ROUTINE_UPDATECKIENTRECT,
	HWNDLOCK_ROUTINE_UPDATEWINDOW,
	X_ROUTINE_IMESHOWSTATUSCHANGE,
	TWOPARAM_ROUTINE_ENABLEWINDOW,
	TWOPARAM_ROUTINE_REDRAWTITLE,
	TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS,
	TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW,
	TWOPARAM_ROUTINE_UPDATEWINDOWS,
	TWOPARAM_ROUTINE_VALIDATERGN,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_CHANGEWNDMSGFILTER,
	TWOPARAM_ROUTINE_GETCURSORPOS,
#endif
	TWOPARAM_ROUTINE_GETHDEVNAME,
	TWOPARAM_ROUTINE_INITANSIOEM,
	TWOPARAM_ROUTINE_NLSSENDIMENOTIFY,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_REGISTERGHSTWND,
#endif
	TWOPARAM_ROUTINE_REGISTERLOGONPROCESS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_REGISTERSBLFROSTWND,
#else
	TWOPARAM_ROUTINE_REGISTERSYSTEMTHREAD,
#endif
	TWOPARAM_ROUTINE_REGISTERUSERHUNGAPPHANDLERS,
	TWOPARAM_ROUTINE_SHADOWCLEANUP,
	TWOPARAM_ROUTINE_REMOTESHADOWSTART,
	TWOPARAM_ROUTINE_SETCARETPOS,
	TWOPARAM_ROUTINE_SETCURSORPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_SETPHYSCURSORPOS,
#endif
	TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK,
	TWOPARAM_ROUTINE_WOWCLEANUP
};

/* TYPES *********************************************************************/

/* FUNCTIONS *****************************************************************/

/* FIXME: UserHMGetHandle needs to be updated once the new handle manager is implemented */
//#define UserHMGetHandle(obj) ((obj)->hdr.Handle)
//PW32THREADINFO GetW32ThreadInfo(VOID);
//PW32PROCESSINFO GetW32ProcessInfo(VOID);
//#define GetWin32ClientInfo() (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo)






//
// NtUser* routines -- sorted by service call position for now
//

HKL
NTAPI
NtUserActivateKeyboardLayout(
    IN HKL hKL,
    IN ULONG Flags
);

VOID
NTAPI
NtUserAlterWindowStyle(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2
);

DWORD
NTAPI
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

BOOL
NTAPI
NtUserAttachThreadInput(
    IN DWORD idAttach,
    IN DWORD idAttachTo,
    IN BOOL fAttach
);

HDC
NTAPI
NtUserBeginPaint(
    HWND hWnd,
    PAINTSTRUCT* lPs
);

BOOL
NTAPI
NtUserBitBltSysBmp(
    HDC hDC,
    INT nXDest,
    INT nYDest,
    INT nWidth,
    INT nHeight,
    INT nXSrc,
    INT nYSrc,
    DWORD dwRop
);

BOOL
NTAPI
NtUserBlockInput(
    BOOL fBlockIt
);

DWORD
NTAPI
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4
);

/* VERIFY */
NTSTATUS
NTAPI
NtUserBuildHwndList(
    HDESK hDesktop,
    HWND hwndParent,
    BOOL bChildren, //VERIFY -- BOOL or BOOLEAN=
    ULONG dwThreadId,
    ULONG lParam,
    HWND* pWnd,
    ULONG* pBufSize
);

/* VERIFY */
NTSTATUS
NTAPI
NtUserBuildNameList(
    HWINSTA hWinSta,
    ULONG dwSize,
    PVOID lpBuffer,
    PULONG pRequiredSize
);

/* VERIFY */
NTSTATUS
NTAPI
NtUserBuildPropList(
    HWND hWnd,
    PVOID Buffer,
    DWORD BufferSize,
    DWORD *Count
);

DWORD
NTAPI
NtUserCallHwnd(
    HWND hWnd,
    DWORD Routine
);

BOOL // VERIFY
NTAPI
NtUserCallHwndLock(
    HWND hWnd,
    DWORD Routine
);

HWND // VERIFY
NTAPI
NtUserCallHwndOpt(
    HWND hWnd,
    DWORD Routine
);

DWORD
NTAPI
NtUserCallHwndParam(
    HWND hWnd,
    DWORD Param,
    DWORD Routine
);

DWORD
NTAPI
NtUserCallHwndParamLock(
    HWND hWnd,
    DWORD Param,
    DWORD Routine
);

BOOL
NTAPI
NtUserCallMsgFilter(
    PMSG pMsg,
    INT nCode
);

LRESULT
NTAPI
NtUserCallNextHookEx(
  INT nCode,
  WPARAM wParam,
  LPARAM lParam,
  BOOL bAnsi
);

DWORD
NTAPI
NtUserCallNoParam(
   DWORD Routine
);

DWORD
NTAPI
NtUserCallOneParam(
    DWORD Param,
    DWORD Routine
);

DWORD
NTAPI
NtUserCallTwoParam(
    DWORD Param1,
    DWORD Param2,
    DWORD Routine
);

BOOL
NTAPI
NtUserChangeClipboardChain(
    HWND hWndRemove,
    HWND hWndNewNext
);

LONG
NTAPI
NtUserChangeDisplaySettings(
    PUNICODE_STRING lpszDeviceName,
    LPDEVMODEW lpDevMode,
    DWORD dwflags,
    PVOID lParam
);

DWORD
NTAPI
NtUserCheckImeHotKey(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

DWORD
NTAPI
NtUserCheckMenuItem(
    HMENU hmenu,
    UINT uIDCheckItem,
    UINT uCheck
);

HWND
NTAPI
NtUserChildWindowFromPointEx(
    HWND hWndParent,
    LONG x,
    LONG y,
    UINT Flags
);

BOOL
NTAPI
NtUserClipCursor(
    RECT *lpRect
);

BOOL
NTAPI
NtUserCloseClipboard(
    VOID
);

BOOL
NTAPI
NtUserCloseDesktop(
    HDESK hDesktop
);

BOOL
NTAPI
NtUserCloseWindowStation(
    HWINSTA hWinSta
);

DWORD
NTAPI
NtUserConsoleControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

DWORD
NTAPI
NtUserConvertMemHandle(
    DWORD Unknown0,
    DWORD Unknown1
);

INT
NTAPI
NtUserCopyAcceleratorTable(
    HACCEL hAccelTable,
    ACCEL* pAccelEntries,
    INT EntriesCount
);

INT
NTAPI
NtUserCountClipboardFormats(
    VOID
);

HACCEL
NTAPI
NtUserCreateAcceleratorTable(
    ACCEL* AccelEntries,
    INT EntriesCount
);

BOOL
NTAPI
NtUserCreateCaret(
    HWND hWnd,
    HBITMAP hBitmap,
    INT nWidth,
    INT nHeight
);

HDESK
NTAPI
NtUserCreateDesktop(
    PUNICODE_STRING lpszDesktopName,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpSecurity,
    HWINSTA hWindowStation
);

DWORD
NTAPI
NtUserCreateInputContext(
    DWORD dwUnknown1
);

DWORD
NTAPI
NtUserCreateLocalMemHandle(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

/* VERIFY */
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
    DWORD dwUnknown
);

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

BOOL
NTAPI
NtUserDdeGetQualityOfService(
    IN HWND hWndClient,
    IN HWND hWndServer,
    OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev
);

DWORD
NTAPI
NtUserDdeInitialize(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4
);

BOOL
NTAPI
NtUserDdeSetQualityOfService(
    IN HWND hWndClient,
    IN PSECURITY_QUALITY_OF_SERVICE pqosNew,
    OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev);

HDWP
NTAPI
NtUserDeferWindowPos(
    HDWP WinPosInfo,
    HWND Wnd,
    HWND WndInsertAfter,
    INT x,
    INT y,
    INT cx,
    INT cy,
    UINT Flags);

BOOL
NTAPI
NtUserDefSetText(
    HWND hWnd,
    PUNICODE_STRING WindowText);

BOOL
NTAPI
NtUserDeleteMenu(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags
);

BOOL
NTAPI
NtUserDestroyAcceleratorTable(
    HACCEL hAccelTable
);

BOOL
NTAPI
NtUserDestroyCursor(
    HCURSOR hCursor,
    DWORD Unknown
);

DWORD
NTAPI
NtUserDestroyInputContext(
    DWORD dwUnknown1
);

BOOL
NTAPI
NtUserDestroyMenu(
    HMENU hMenu
);

BOOL
NTAPI
NtUserDestroyWindow(
    HWND hWnd
);

DWORD
NTAPI
NtUserDisableThreadIme(
    DWORD dwUnknown1
);

LRESULT
NTAPI
NtUserDispatchMessage(
    PMSG Msg
);

BOOL
NTAPI
NtUserDragDetect(
  HWND hWnd,
  POINT pt
);

DWORD
NTAPI
NtUserDragObject(
    HWND hWndScope,
    HWND hWnd,
    UINT WndObj,
    DWORD hList,
    HCURSOR hCursor);

BOOL
NTAPI
NtUserDrawAnimatedRects(
    HWND hWnd,
    INT idAni,
    PRECT prcFrom,
    PRECT prcTo
);

BOOL
NTAPI
NtUserDrawCaption(
    HWND hWnd,
    HDC hDc,
    PRECT pRect,
    UINT uFlags
);

BOOL
NTAPI
NtUserDrawCaptionTemp(
    HWND hWnd,
    HDC hDC,
    PRECT pRect,
    HFONT hFont,
    HICON hIcon,
    PUNICODE_STRING pString,
    UINT uFlags);

BOOL
NTAPI
NtUserDrawIconEx(
    HDC hdc,
    INT xLeft,
    INT yTop,
    HICON hIcon,
    INT cxWidth,
    INT cyWidth,
    UINT istepIfAniCur,
    HBRUSH hbrFlickerFreeDraw,
    UINT diFlags, // VERIFY
    DWORD Unknown0,
    DWORD Unknown1
);

DWORD
NTAPI
NtUserDrawMenuBarTemp(
    HWND hWnd,
    HDC hDC,
    PRECT pRect,
    HMENU hMenu,
    HFONT hFont
);

BOOL
NTAPI
NtUserEmptyClipboard(
    VOID
);

BOOL
NTAPI
NtUserEnableMenuItem(
    HMENU hMenu,
    UINT uIDEnableItem,
    UINT uEnable
);

BOOL
NTAPI
NtUserEnableScrollBar(
    HWND hWnd,
    UINT wSBflags,
    UINT wArrows
);

DWORD
NTAPI
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1
);

BOOL
NTAPI
NtUserEndMenu(
    VOID
);

BOOL
NTAPI
NtUserEndPaint(
    HWND hWnd,
    PAINTSTRUCT* lPs
);

BOOL
NTAPI
NtUserEnumDisplayDevices(
    PUNICODE_STRING lpDevice,
    DWORD iDevNum,
    PDISPLAY_DEVICEW lpDisplayDevice,
    DWORD dwFlags
);

BOOL
NTAPI
NtUserEnumDisplayMonitors(
    HDC hDC,
    PRECT prcClip,
    MONITORENUMPROC pfnEnum,
    LPARAM dwData
);

NTSTATUS /*VERIFY */
NTAPI
NtUserEnumDisplaySettings(
    PUNICODE_STRING lpszDeviceName,
    DWORD iModeNum,
    PDEVMODEW pDevMode, /*VERIFY */
    DWORD dwFlags
);

DWORD
NTAPI
NtUserEvent(
    DWORD Unknown0
);

INT
NTAPI
NtUserExcludeUpdateRgn(
    HDC hDC,
    HWND hWnd
);

BOOL
NTAPI
NtUserFillWindow(
    HWND hWndPaint1,
    HWND hWndPaint2,
    HDC  hDC,
    HBRUSH hBrush
);

HICON
NTAPI
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  PPOINT Point
);

HWND
NTAPI
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING ucClassName,
  PUNICODE_STRING ucWindowName,
  DWORD dwUnknown
);

BOOL
NTAPI
NtUserFlashWindowEx(
    IN PFLASHWINFO pfwi
);

BOOL
NTAPI
NtUserGetAltTabInfo(
    HWND hWnd,
    INT iItem,
    PALTTABINFO pati,
    PTSTR pszItemText,
    UINT cchItemText,
    BOOL bAnsi
);

HWND
NTAPI
NtUserGetAncestor(
    HWND hWnd,
    UINT Flags
);

DWORD
NTAPI
NtUserGetAppImeLevel(
    DWORD dwUnknown
);

SHORT
NTAPI
NtUserGetAsyncKeyState(
    INT Key
);

DWORD
NTAPI
NtUserGetAtomName(
    ATOM nAtom,
    PWSTR pBuffer
);

UINT
NTAPI
NtUserGetCaretBlinkTime(
    VOID
);

BOOL
NTAPI
NtUserGetCaretPos(
    PPOINT pPoint
);

#if 0
/* XP */
BOOL
NTAPI
NtUserGetClassInfo(HINSTANCE hInstance,
		   PUNICODE_STRING ClassName,
		   PWNDCLASSEXW wcex,
		   PWSTR *ppszMenuName,
		   BOOL Ansi);
#endif

/* 2K3 */
/* VERIFY */
BOOL
NTAPI
NtUserGetClassInfoEx(
    HINSTANCE hInstance,
    PUNICODE_STRING ClassName,
    PWNDCLASSEXW wcex,
    PWSTR *ppszMenuName,
    BOOL bUnknown
);

INT
NTAPI
NtUserGetClassName(
    HWND hWnd,
    BOOL Real,
    PUNICODE_STRING ClassName
);

HANDLE
NTAPI
NtUserGetClipboardData(
    UINT uFormat,
    PVOID pBuffer
);

INT
NTAPI
NtUserGetClipboardFormatName(
    UINT format,
    PUNICODE_STRING FormatName,
    INT cchMaxCount
);

HWND
NTAPI
NtUserGetClipboardOwner(
    VOID
);

DWORD
NTAPI
NtUserGetClipboardSequenceNumber(
    VOID
);

HWND
NTAPI
NtUserGetClipboardViewer(
    VOID
);

BOOL
NTAPI
NtUserGetClipCursor(
    RECT *pRect
);

BOOL
NTAPI
NtUserGetComboBoxInfo(
    HWND hWnd,
    PCOMBOBOXINFO pcbi
);

HBRUSH
NTAPI
NtUserGetControlBrush(
    HWND hWnd,
    HDC  hDC,
    UINT ctlType
);

HBRUSH
NTAPI
NtUserGetControlColor(
    HWND hwndParent,
    HWND hWnd,
    HDC hDC,
    UINT CtlMsg
);

DWORD
NTAPI
NtUserGetCPD(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2
);

DWORD
NTAPI
NtUserGetCursorFrameInfo(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

BOOL
NTAPI
NtUserGetCursorInfo(
    PCURSORINFO pci
);

HDC
NTAPI
NtUserGetDC(
  HWND hWnd
);

HDC
NTAPI
NtUserGetDCEx(
    HWND hWnd,
    HANDLE hRegion,
    ULONG Flags
);

UINT
NTAPI
NtUserGetDoubleClickTime(
    VOID
);

HWND
NTAPI
NtUserGetForegroundWindow(
    VOID
);

DWORD
NTAPI
NtUserGetGuiResources(
    HANDLE hProcess,
    DWORD uiFlags
);

BOOL
NTAPI
NtUserGetGUIThreadInfo(
    DWORD idThread,
    PGUITHREADINFO pgui
);

BOOL
NTAPI
NtUserGetIconInfo(
    HANDLE hCurIcon,
    PICONINFO IconInfo,
    PUNICODE_STRING lpInstName,
    PUNICODE_STRING lpResName,
    PDWORD pbpp,
    BOOL bInternal
);

BOOL
NTAPI
NtUserGetIconSize(
    HANDLE Handle,
    UINT istepIfAniCur,
    PLONG plcx,
    PLONG  plcy
);

DWORD
NTAPI
NtUserGetImeHotKey(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

DWORD
NTAPI
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

DWORD
NTAPI
NtUserGetInternalWindowPos(
    HWND hWnd,
    PRECT prcWnd,
    PPOINT ptIcon
);

UINT
NTAPI
NtUserGetKeyboardLayoutList(
    INT nItems,
    HKL *pHklBuff
);

BOOL
NTAPI
NtUserGetKeyboardLayoutName(
    LPWSTR lpszName
);

BOOL
NTAPI
NtUserGetKeyboardState(
    PBYTE pKeyState
);

INT
NTAPI
NtUserGetKeyNameText(
    LONG lParam,
    PWSTR pString,
    INT nSize
);

SHORT
NTAPI
NtUserGetKeyState(
    INT VirtKey
);

DWORD
NTAPI
NtUserGetListBoxInfo(
    HWND hWnd
);

BOOL
NTAPI
NtUserGetMenuBarInfo(
    HWND hWnd,
    LONG idObject,
    LONG idItem,
    PMENUBARINFO pmbi
);

UINT
NTAPI
NtUserGetMenuIndex(
    HMENU hMenu,
    UINT wID // VERIFY -- HMENU?
);

BOOL
NTAPI
NtUserGetMenuItemRect(
    HWND hWnd,
    HMENU hMenu,
    UINT uItem,
    PRECT prcItem
);

BOOL
NTAPI
NtUserGetMessage(
    PMSG pMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax
);

INT
NTAPI
NtUserGetMouseMovePointsEx(
    UINT cbSize,
    PMOUSEMOVEPOINT ppt,
    PMOUSEMOVEPOINT pptBuf,
    INT nBufPoints,
    DWORD dwRes
);

BOOL
NTAPI
NtUserGetObjectInformation(
    HANDLE hObject,
    INT nIndex,
    PVOID pvInformation,
    DWORD nLength,
    PDWORD nLengthNeeded
);

HWND
NTAPI
NtUserGetOpenClipboardWindow(
    VOID
);

INT
NTAPI
NtUserGetPriorityClipboardFormat(
    PUINT paFormatPriorityList,
    INT cFormats
);

HWINSTA
NTAPI
NtUserGetProcessWindowStation(
    VOID
);

UINT
NTAPI
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader
);

UINT
NTAPI
NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    PVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader
);

UINT
NTAPI
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    PVOID pData,
    PUINT pcbSize
);

UINT
NTAPI
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize
);

UINT
NTAPI
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize
);

BOOL
NTAPI
NtUserGetScrollBarInfo(
    HWND hWnd,
    LONG idObject,
    PSCROLLBARINFO psbi
);

HMENU
NTAPI
NtUserGetSystemMenu(
    HWND hWnd,
    BOOL bRevert
);


HDESK
NTAPI
NtUserGetThreadDesktop(
    DWORD dwThreadId,
    DWORD Unknown1
);


enum ThreadStateRoutines
{
    THREADSTATE_GETTHREADINFO,
    THREADSTATE_INSENDMESSAGE,
    THREADSTATE_FOCUSWINDOW,
    THREADSTATE_ACTIVEWINDOW,
    THREADSTATE_CAPTUREWINDOW,
    THREADSTATE_PROGMANWINDOW,
    THREADSTATE_TASKMANWINDOW
}; //0 to 18

DWORD
NTAPI
NtUserGetThreadState(
    DWORD Routine
);

BOOL
NTAPI
NtUserGetTitleBarInfo(
    HWND hWnd,
    PTITLEBARINFO pti
);

BOOL
NTAPI
NtUserGetUpdateRect(
    HWND hWnd,
    PRECT pRect,
    BOOL fErase
);

INT
NTAPI
NtUserGetUpdateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase
);

HDC
NTAPI
NtUserGetWindowDC(
    HWND hWnd
);

BOOL
NTAPI
NtUserGetWindowPlacement(
    HWND hWnd,
    WINDOWPLACEMENT *pwndpl
);

DWORD
NTAPI
NtUserGetWOWClass(
    DWORD Unknown0,
    DWORD Unknown1
);

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

BOOL
NTAPI
NtUserHideCaret(
    HWND hWnd
);

BOOL
NTAPI
NtUserHiliteMenuItem(
    HWND hWnd,
    HMENU hMenu,
    UINT uItemHilite,
    UINT uHilite
);

BOOL
NTAPI
NtUserImpersonateDdeClientWindow(
    HWND hWndClient,
    HWND hWndServer
);

DWORD
NTAPI
NtUserInitialize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

NTSTATUS
NTAPI
NtUserInitializeClientPfnArrays(
    PPFNCLIENT pfnClientA,
    PPFNCLIENT pfnClientW,
    PPFNCLIENTWORKER pfnClientWorker,
    HINSTANCE hmodUser
);

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
    DWORD Unknown11
);

INT
NTAPI
NtUserInternalGetWindowText(
    HWND hWnd,
    LPWSTR lpString,
    INT nMaxCount
);

BOOL
NTAPI
NtUserInvalidateRect(
    HWND hWnd,
    RECT *pRect,
    BOOL bErase
);

BOOL
NTAPI
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase
);

BOOL
NTAPI
NtUserIsClipboardFormatAvailable(
    UINT format
);

BOOL
NTAPI
NtUserKillTimer(
    HWND hWnd,
    UINT_PTR uIDEvent
);

/* VERIFY */
HKL
NTAPI
NtUserLoadKeyboardLayoutEx(
    IN HANDLE Handle,
    IN DWORD offTable,
    IN PUNICODE_STRING puszKeyboardName,
    IN HKL hKL,
    IN PUNICODE_STRING puszKLID,
    IN DWORD dwKLID,
    IN UINT Flags
);

BOOL
NTAPI
NtUserLockWindowStation(
    HWINSTA hWindowStation
);

BOOL
NTAPI
NtUserLockWindowUpdate(
    HWND hWndLock
);

BOOL
NTAPI
NtUserLockWorkStation(
    VOID
);

UINT
NTAPI
NtUserMapVirtualKeyEx(
    UINT uCode,
    UINT uMapType,
    HKL dwHKL,
    BOOL bUnknown // VERIFY
);

INT
NTAPI
NtUserMenuItemFromPoint(
    HWND hWnd,
    HMENU hMenu,
    LONG X,
    LONG Y
);

LRESULT
NTAPI
NtUserMessageCall(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR ResultInfo,
    DWORD dwType, // FNID_XX types
    BOOL Ansi
);

DWORD // VERIFY
NTAPI
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide
);

DWORD
NTAPI
NtUserMNDragLeave(
    VOID
);

DWORD
NTAPI
NtUserMNDragOver(
    DWORD Unknown0,
    DWORD Unknown1
);

VOID
NTAPI
NtUserModifyUserStartupInfoFlags(
    DWORD Unknown0,
    DWORD Unknown1
);

BOOL
NTAPI
NtUserMoveWindow(
    HWND hWnd,
    INT X,
    INT Y,
    INT nWidth,
    INT nHeight,
    BOOL bRepaint
);

VOID
NTAPI
NtUserNotifyIMEStatus(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2
);

DWORD
NTAPI
NtUserNotifyProcessCreate(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4
);

VOID
NTAPI
NtUserNotifyWinEvent(
    DWORD Event,
    HWND  hWnd,
    LONG  idObject,
    LONG  idChild
);

BOOL
NTAPI
NtUserOpenClipboard(
    HWND hWnd,
    HWND *phWND
);

/* VERIFY */
HDESK
NTAPI
NtUserOpenDesktop(
    PUNICODE_STRING lpszDesktopName,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess
);

HDESK
NTAPI
NtUserOpenInputDesktop(
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess
);

HWINSTA
NTAPI
NtUserOpenWindowStation(
    PUNICODE_STRING lpszWindowStationName,
    ACCESS_MASK dwDesiredAccess
);

BOOL
NTAPI
NtUserPaintDesktop(
    HDC hDC
);

BOOL
NTAPI
NtUserPeekMessage(
    PMSG Msg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg
);

BOOL
NTAPI
NtUserPostMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
);

BOOL
NTAPI
NtUserPostThreadMessage(
    DWORD idThread,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
);

BOOL
NTAPI
NtUserPrintWindow(
    HWND hWnd,
    HDC  hdcBlt,
    UINT nFlags
);

DWORD
NTAPI
NtUserProcessConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

DWORD
NTAPI
NtUserQueryInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5
);

DWORD
NTAPI
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

DWORD
NTAPI
NtUserQuerySendMessage(
    DWORD Unknown0
);

#if 0
/* XP */
DWORD
NTAPI
NtUserQueryUserCounters(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4
);
#endif



DWORD
NTAPI
NtUserQueryWindow(
    HWND hWnd,
    DWORD Index
);

DWORD
NTAPI
NtUserRealChildWindowFromPoint(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2
);

DWORD
NTAPI
NtUserRealInternalGetMessage(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6
);

DWORD
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

BOOL
NTAPI
NtUserRedrawWindow(
    HWND hWnd,
    RECT *prcUpdate,
    HRGN hrgnUpdate,
    UINT uFlags
);

RTL_ATOM
NTAPI
NtUserRegisterClassExWOW(
    WNDCLASSEXW* pwcx,
    PUNICODE_STRING pustrClassName,
    PUNICODE_STRING pustrCNVersion,
    PCLSMENUNAME pClassMenuName,
    DWORD fnID,
    DWORD Flags,
    PDWORD pWow
);

DWORD
NTAPI
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

BOOL
NTAPI
NtUserRegisterHotKey(
    HWND hWnd,
    INT id,
    UINT fsModifiers,
    UINT vk
);

BOOL
NTAPI
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize
);

BOOL
NTAPI
NtUserRegisterTasklist(
    HWND hWnd
);

UINT
NTAPI
NtUserRegisterWindowMessage(
    PUNICODE_STRING MessageName
);

BOOL
NTAPI
NtUserRemoveMenu(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags
);

HANDLE
NTAPI
NtUserRemoveProp(
    HWND hWnd,
    ATOM Atom
);

DWORD
NTAPI
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4
);

DWORD
NTAPI
NtUserResolveDesktopForWOW(
    DWORD Unknown0
);

DWORD
NTAPI
NtUserSBGetParms(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

BOOL
NTAPI
NtUserScrollDC(
    HDC hDC,
    INT dx,
    INT dy,
    RECT *lprcScroll,
    RECT *lprcClip ,
    HRGN hrgnUpdate,
    PRECT lprcUpdate
);

DWORD
NTAPI
NtUserScrollWindowEx(
    HWND hWnd,
    INT dx,
    INT dy,
    RECT *prcScroll,
    RECT *prcClip,
    HRGN hrgnUpdate,
    PRECT prcUpdate,
    UINT uFlags
);

HPALETTE
NTAPI
NtUserSelectPalette(
    HDC hDC,
    HPALETTE hPal,
    BOOL bForceBackground
);

UINT
NTAPI
NtUserSendInput(
    UINT nInputs,
    PINPUT pInput,
    INT cbSize
);

HWND
NTAPI
NtUserSetActiveWindow(
    HWND hWnd
);

DWORD
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

HWND
NTAPI
NtUserSetCapture(
    HWND hWnd
);

ULONG_PTR
NTAPI
NtUserSetClassLong(
    HWND hWnd,
    INT nIndex,
    ULONG_PTR dwNewLong,
    BOOL bAnsi
);

WORD
NTAPI
NtUserSetClassWord(
    HWND hWnd,
    INT nIndex,
    WORD wNewWord
);

HANDLE
NTAPI
NtUserSetClipboardData(
    UINT uFormat,
    HANDLE hMem,
    DWORD Unknown
);

HWND
NTAPI
NtUserSetClipboardViewer(
    HWND hWndNewViewer
);

DWORD
NTAPI
NtUserSetConsoleReserveKeys(
    DWORD Unknown0,
    DWORD Unknown1
);

HCURSOR
NTAPI
NtUserSetCursor(
    HCURSOR hCursor
);

/* VERIFY */
BOOL
NTAPI
NtUserSetCursorContents(
    HANDLE Handle,
    PICONINFO IconInfo
);

BOOL
NTAPI
NtUserSetCursorIconData(
    HANDLE Handle,
    HMODULE hModule,
    PUNICODE_STRING pstrResName,
    PICONINFO pIconInfo
);

#if 0
/* XP */
VOID
NTAPI
NtUserSetDbgTag(
    DWORD Unknown0,
    DWORD Unknown1
);
#endif

HWND
NTAPI
NtUserSetFocus(
    HWND hWnd
);

DWORD
NTAPI
NtUserSetImeHotKey(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4
);

DWORD
NTAPI
NtUserSetImeInfoEx(
    DWORD dwUnknown1
);

DWORD
NTAPI
NtUserSetImeOwnerWindow(
    DWORD Unknown0,
    DWORD Unknown1
);

DWORD
NTAPI
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4
);

DWORD
NTAPI
NtUserSetInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4
);

DWORD
NTAPI
NtUserSetInternalWindowPos(
    HWND hWnd,
    UINT showCmd,
    PRECT prcRect,
    PPOINT pt
);

BOOL
NTAPI
NtUserSetKeyboardState(
    PBYTE pKeyState
);

BOOL
NTAPI
NtUserSetLogonNotifyWindow(
    HWND hWnd
);

BOOL
NTAPI
NtUserSetMenu(
    HWND hWnd,
    HMENU hMenu,
    BOOL bRepaint
);

BOOL
NTAPI
NtUserSetMenuContextHelpId(
    HMENU hMenu,
    DWORD dwContextHelpId
);

BOOL
NTAPI
NtUserSetMenuDefaultItem(
    HMENU hMenu,
    UINT uItem,
    UINT fByPos
);

BOOL
NTAPI
NtUserSetMenuFlagRtoL(
    HMENU hMenu
);

/* VERIFY */
BOOL
NTAPI
NtUserSetObjectInformation(
    HANDLE hObject,
    DWORD nIndex,
    PVOID pvInformation,
    DWORD nLength
);

HWND
NTAPI
NtUserSetParent(
    HWND hWndChild,
    HWND hWndNewParent
);

BOOL
NTAPI
NtUserSetProcessWindowStation(
    HWINSTA hWindowStation
);

BOOL
NTAPI
NtUserSetProp(
    HWND hWnd,
    ATOM Atom,
    HANDLE hData
);

DWORD
NTAPI
NtUserSetScrollInfo(
    HWND hWnd,
    INT fnBar,
    SCROLLINFO* psi,
    BOOL bRedraw
);

BOOL
NTAPI
NtUserSetShellWindowEx(
    HWND hwndShell,
    HWND hwndShellListView
);

BOOL
NTAPI
NtUserSetSysColors(
    IN INT cElements,
    IN INT *lpaElements,
    IN COLORREF *lpaRgbValues,
    IN FLONG Flags
);

BOOL
NTAPI
NtUserSetSystemCursor(
    HCURSOR hCur,
    DWORD idCur
);

BOOL
NTAPI
NtUserSetSystemMenu(
    HWND hWnd,
    HMENU hMenu
);

/* VERIFY */
UINT_PTR
NTAPI
NtUserSetSystemTimer(
    HWND hWnd,
    UINT_PTR nIDEvent,
    UINT uElapse,
    TIMERPROC lpTimerFunc
);

BOOL
NTAPI
NtUserSetThreadDesktop(
    HDESK hDesktop
);

VOID
NTAPI
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2
);

VOID
NTAPI
NtUserSetThreadState(
    DWORD Unknown0,
    DWORD Unknown1
);

UINT_PTR
NTAPI
NtUserSetTimer(
    HWND hWnd,
    UINT_PTR nIDEvent,
    UINT uElapse,
    TIMERPROC lpTimerFunc
);

BOOL
NTAPI
NtUserSetWindowFNID(
    HWND hWnd,
    WORD fnID
);

/* VERIFY */
LONG_PTR
NTAPI
NtUserSetWindowLong(
    HWND hWnd,
    INT Index,
    LONG_PTR NewValue,
    BOOL Ansi // VERIFY
);

BOOL
NTAPI
NtUserSetWindowPlacement(
    HWND hWnd,
    WINDOWPLACEMENT *pwndpl
);

BOOL
NTAPI
NtUserSetWindowPos(
    HWND hWnd,
    HWND hWndInsertAfter,
    INT X,
    INT Y,
    INT cx,
    INT cy,
    UINT uFlags
);

INT
NTAPI
NtUserSetWindowRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bRedraw
);

HHOOK
NTAPI
NtUserSetWindowsHookAW(
    INT idHook,
    HOOKPROC pfn,
    BOOL Ansi
);

/* VERIFY */
HHOOK
NTAPI
NtUserSetWindowsHookEx(
    HINSTANCE Mod,
    PUNICODE_STRING ModuleName,
    DWORD ThreadId,
    INT HookId,
    HOOKPROC HookProc,
    BOOL Ansi
);

BOOL
NTAPI
NtUserSetWindowStationUser(
    HWINSTA hWinSta,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

/* VERIFY */
WORD
NTAPI
NtUserSetWindowWord(
    HWND hWnd,
    INT Index,
    WORD NewVal
);

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
    UINT dwflags
);

BOOL
NTAPI
NtUserShowCaret(
    HWND hWnd
);

BOOL
NTAPI
NtUserShowScrollBar(
    HWND hWnd,
    INT wBar,
    BOOL bShow
);

BOOL
NTAPI
NtUserShowWindow(
    HWND hWnd,
    LONG nCmdShow
);

BOOL
NTAPI
NtUserShowWindowAsync(
    HWND hWnd,
    LONG nCmdShow
);

BOOL
NTAPI
NtUserSoundSentry(
    VOID
);

BOOL
NTAPI
NtUserSwitchDesktop(
    HDESK hDesktop
);

BOOL
NTAPI
NtUserSystemParametersInfo(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni
);

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1
);

/* VERIFY */
DWORD
NTAPI
NtUserThunkedMenuInfo(
    HMENU hMenu,
    MENUINFO* pcmi
);

/* VERIFY */
DWORD
NTAPI
NtUserThunkedMenuItemInfo(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    BOOL bInsert,
    MENUITEMINFOW* pmii,
    PUNICODE_STRING pszCaption
);

INT
NTAPI
NtUserToUnicodeEx(
    UINT wVirtKey,
    UINT wScanCode,
    PBYTE pKeyState,
    PWSTR pwszBuff,
    INT cchBuff,
    UINT dwFlags,
    HKL dwhkl
);

BOOL
NTAPI
NtUserTrackMouseEvent(
    TRACKMOUSEEVENT* pEventTrack
);

BOOL
NTAPI
NtUserTrackPopupMenuEx(
    HMENU hMenu,
    UINT fuFlags,
    INT x,
    INT y,
    HWND hWnd,
    TPMPARAMS* lptpm
);

DWORD
NTAPI
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5
);

DWORD
NTAPI
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6
);

INT
NTAPI
NtUserTranslateAccelerator(
    HWND Window,
    HACCEL Table,
    PMSG Msg
);

BOOL
NTAPI
NtUserTranslateMessage(
    PMSG pMsg,
    HKL dwhkl //VERIFY -- Flag?
);

BOOL
NTAPI
NtUserUnhookWindowsHookEx(
    HHOOK Hook
);

BOOL
NTAPI
NtUserUnhookWinEvent(
    HWINEVENTHOOK hWinEventHook
);

BOOL
NTAPI
NtUserUnloadKeyboardLayout(
    HKL hKL
);

BOOL
NTAPI
NtUserUnlockWindowStation(
    HWINSTA hWindowStation
);

BOOL
NTAPI
NtUserUnregisterClass(
    PUNICODE_STRING ClassNameOrAtom, // VERIFY
    HINSTANCE hInstance,
    PCLSMENUNAME pClassMenuName
);

DWORD
NTAPI
NtUserUnregisterUserApiHook(
    VOID
);

BOOL
NTAPI
NtUserUnregisterHotKey(
    HWND hWnd,
    INT nHotKeyId
);

DWORD
NTAPI
NtUserUpdateInputContext(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2
);

DWORD
NTAPI
NtUserUpdateInstance(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2
);

BOOL
NTAPI
NtUserUpdateLayeredWindow(
    HWND hWnd,
    HDC hdcDst,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags
);

BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    HWND hWnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags
);

BOOL
NTAPI
NtUserSetLayeredWindowAttributes(
    HWND hWnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags
);

BOOL
NTAPI
NtUserUpdatePerUserSystemParameters(
    DWORD dwReserved,
    BOOL bEnable // VERIFY
);

BOOL
NTAPI
NtUserUserHandleGrantAccess(
    IN HANDLE hUserHandle,
    IN HANDLE hJob,
    IN BOOL bGrant
);

BOOL
NTAPI
NtUserValidateHandleSecure(
    HANDLE hHdl
);

BOOL
NTAPI
NtUserValidateRect(
    HWND hWnd,
    PRECT pRect
);

DWORD
NTAPI
NtUserValidateTimerCallback(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

DWORD
NTAPI
NtUserVkKeyScanEx(
    WCHAR wChar,
    HKL KeyboardLayout,
    BOOL bUnknown
);

DWORD
NTAPI
NtUserWaitForInputIdle(
    IN HANDLE hProcess,
    IN DWORD dwMilliseconds,
    IN BOOL Unknown2
);

DWORD
NTAPI
NtUserWaitForMsgAndEvent(
    DWORD Unknown0
);

BOOL
NTAPI
NtUserWaitMessage(
    VOID
);

DWORD
NTAPI
NtUserWin32PoolAllocationStats(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5
);

HWND
NTAPI
NtUserWindowFromPoint(
    LONG X,
    LONG Y
);

DWORD
NTAPI
NtUserYieldTask(
    VOID
);

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

DWORD
NTAPI
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4
);

DWORD
NTAPI
NtUserRemoteRedrawScreen(
    VOID
);

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(
    VOID
);

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3
);

#if 0
/* 2K3 checked build */
DWORD
NTAPI
NtUserSetRipFlags(
    DWORD Unknown1,
);
#endif

#if 0
/* XP */
DWORD
NTAPI
NtUserSetRipFlags(
    DWORD Unknown1,
    DWORD Unknown2
);
#endif

#if 0
/* Vista */
DWORD
NTAPI
NtUserSetRipFlags(
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);
#endif

#endif /* __WIN32K_NTUSER_H */
