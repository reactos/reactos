/****************************** Module Header ******************************\
* Module Name: ntuser.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains all kernel mode entry points
*
* History:
* 03-22-95 JimA         Created.
\***************************************************************************/

#ifndef _NTUSER_
#define _NTUSER_

#include "w32wow64.h"

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

#if DBG
    #define DBGHYD(m)                                           \
    {                                                           \
        KdPrint(("\nHYDRA %d : ", gSessionId));                 \
        KdPrint(m);                                             \
        KdPrint(("\n"));                                        \
    }
#else
    #define DBGHYD(m)
#endif

#include "usercall.h"

W32KAPI
UINT
NtUserHardErrorControl(
    IN HARDERRORCONTROL dwCmd,
    IN HANDLE handle OPTIONAL,
    OUT PDESKRESTOREDATA pdrdRestore OPTIONAL);

VOID
NtUserSetDebugErrorLevel(
    IN DWORD dwErrorLevel);

W32KAPI
BOOL
NtUserGetObjectInformation(
    IN HANDLE hObject,
    IN int nIndex,
    OUT PVOID pvInfo,
    IN DWORD nLength,
    OUT LPDWORD pnLengthNeeded);

W32KAPI
BOOL
NtUserSetObjectInformation(
    IN HANDLE hObject,
    IN int nIndex,
    IN LPCVOID pvInfo,
    IN DWORD nLength);

W32KAPI
BOOL
NtUserWin32PoolAllocationStats(
    IN  LPDWORD parrTags,
    IN  SIZE_T  tagsCount,
    OUT SIZE_T* lpdwMaxMem,
    OUT SIZE_T* lpdwCrtMem,
    OUT LPDWORD lpdwMaxAlloc,
    OUT LPDWORD lpdwCrtAlloc);

#if DBG

W32KAPI
VOID
NtUserDbgWin32HeapFail(
    IN DWORD dwFlags,
    IN BOOL  bFail);

W32KAPI
DWORD
NtUserDbgWin32HeapStat(
    PDBGHEAPSTAT phs,
    DWORD   dwLen);

#endif // DBG

W32KAPI
NTSTATUS
NtUserConsoleControl(
    IN CONSOLECONTROL ConsoleCommand,
    IN PVOID ConsoleInformation,
    IN DWORD ConsoleInformationLength);

W32KAPI
HWINSTA
NtUserCreateWindowStation(
    IN POBJECT_ATTRIBUTES   pObja,
    IN ACCESS_MASK          amRequest,
    IN HANDLE               hKbdLayoutFile,
    IN DWORD                offTable,
    IN PUNICODE_STRING      pstrKLID,
    IN UINT                 uKbdInputLocale);

W32KAPI
HWINSTA
NtUserOpenWindowStation(
    IN POBJECT_ATTRIBUTES pObja,
    IN ACCESS_MASK amRequest);

W32KAPI
BOOL
NtUserCloseWindowStation(
    IN HWINSTA hwinsta);

W32KAPI
BOOL
NtUserSetProcessWindowStation(
    IN HWINSTA hwinsta);

W32KAPI
HWINSTA
NtUserGetProcessWindowStation(
    VOID);

W32KAPI
BOOL
NtUserLockWorkStation(
    VOID);

W32KAPI
HDESK
NtUserCreateDesktop(
    IN POBJECT_ATTRIBUTES pObja,
    IN PUNICODE_STRING pstrDevice,
    IN LPDEVMODEW pDevmode,
    IN DWORD dwFlags,
    IN ACCESS_MASK amRequest);

W32KAPI
HDESK
NtUserOpenDesktop(
    IN POBJECT_ATTRIBUTES pObja,
    IN DWORD dwFlags,
    IN ACCESS_MASK amRequest);

W32KAPI
HDESK
NtUserOpenInputDesktop(
    IN DWORD dwFlags,
    IN BOOL fInherit,
    IN DWORD amRequest);

W32KAPI
NTSTATUS
NtUserResolveDesktopForWOW (
    IN OUT PUNICODE_STRING pstrDesktop);

W32KAPI
HDESK
NtUserResolveDesktop(
    IN HANDLE hProcess,
    IN PUNICODE_STRING pstrDesktop,
    IN BOOL fInherit,
    OUT HWINSTA *phwinsta);

W32KAPI
BOOL
NtUserCloseDesktop(
    IN HDESK hdesk);

W32KAPI
BOOL
NtUserSetThreadDesktop(
    IN HDESK hdesk);

W32KAPI
HDESK
NtUserGetThreadDesktop(
    IN DWORD dwThreadId,
    IN HDESK hdeskConsole);

W32KAPI
BOOL
NtUserSwitchDesktop(
    IN HDESK hdesk);

W32KAPI
NTSTATUS
NtUserInitializeClientPfnArrays(
    IN CONST PFNCLIENT *ppfnClientA OPTIONAL,
    IN CONST PFNCLIENT *ppfnClientW OPTIONAL,
    IN CONST PFNCLIENTWORKER *ppfnClientWorker OPTIONAL,
    IN HANDLE hModUser);

W32KAPI
BOOL
NtUserWaitForMsgAndEvent(
    IN HANDLE hevent);

W32KAPI
DWORD
NtUserDragObject(
    IN HWND hwndParent,
    IN HWND hwndFrom,
    IN UINT wFmt,
    IN ULONG_PTR dwData,
    IN HCURSOR hcur);

W32KAPI
BOOL
NtUserGetIconInfo(
    IN  HICON hicon,
    OUT PICONINFO piconinfo,
    IN  OUT OPTIONAL PUNICODE_STRING pstrInstanceName,
    IN  OUT OPTIONAL PUNICODE_STRING pstrResName,
    OUT OPTIONAL LPDWORD pbpp,
    IN  BOOL fInternal);

W32KAPI
BOOL
NtUserGetIconSize(
    IN HICON hIcon,
    IN UINT istepIfAniCur,
    OUT int *pcx,
    OUT int *pcy);

W32KAPI
BOOL
NtUserDrawIconEx(
    IN HDC hdc,
    IN int x,
    IN int y,
    IN HICON hicon,
    IN int cx,
    IN int cy,
    IN UINT istepIfAniCur,
    IN HBRUSH hbrush,
    IN UINT diFlags,
    IN BOOL fMeta,
    OUT DRAWICONEXDATA *pdid);

W32KAPI
HANDLE
NtUserDeferWindowPos(
    IN HDWP hWinPosInfo,
    IN HWND hwnd,
    IN HWND hwndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT wFlags);

W32KAPI
BOOL
NtUserEndDeferWindowPosEx(
    IN HDWP hWinPosInfo,
    IN BOOL fAsync);

W32KAPI
BOOL
NtUserGetMessage(
    OUT LPMSG pmsg,
    IN HWND hwnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax);

W32KAPI
BOOL
NtUserMoveWindow(
    IN HWND hwnd,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN BOOL fRepaint);

W32KAPI
int
NtUserTranslateAccelerator(
    IN HWND hwnd,
    IN HACCEL hAccTable,
    IN LPMSG lpMsg);

W32KAPI
LONG
NtUserSetClassLong(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG dwNewLong,
    IN BOOL bAnsi);

#ifdef _WIN64
W32KAPI
LONG_PTR
NtUserSetClassLongPtr(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG_PTR dwNewLong,
    IN BOOL bAnsi);
#else
#define NtUserSetClassLongPtr   NtUserSetClassLong
#endif

W32KAPI
BOOL
NtUserSetKeyboardState(
    IN CONST BYTE *lpKeyState);

W32KAPI
BOOL
NtUserSetWindowPos(
    IN HWND hwnd,
    IN HWND hwndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT dwFlags);

W32KAPI
BOOL
NtUserSetShellWindowEx(
    IN HWND hwnd,
    IN HWND hwndBkGnd);

W32KAPI
BOOL
NtUserSystemParametersInfo(
    IN UINT wFlag,
    IN DWORD wParam,
    IN OUT LPVOID lpData,
    IN UINT flags);

W32KAPI
BOOL
NtUserUpdatePerUserSystemParameters(
    IN HANDLE hToken,
    IN BOOL   bUserLoggedOn);

W32KAPI
DWORD
NtUserDdeInitialize(
    OUT PHANDLE phInst,
    OUT HWND *phwnd,
    OUT LPDWORD pMonFlags,
    IN DWORD afCmd,
    IN PVOID pcii);

W32KAPI
DWORD
NtUserUpdateInstance(
    IN HANDLE hInst,
    OUT LPDWORD pMonFlags,
    IN DWORD afCmd);

W32KAPI
DWORD
NtUserEvent(
    IN PEVENT_PACKET pep);

W32KAPI
BOOL
NtUserFillWindow(
    IN HWND hwndBrush,
    IN HWND hwndPaint,
    IN HDC hdc,
    IN HBRUSH hbr);

W32KAPI
PCLS
NtUserGetWOWClass(
    IN HINSTANCE hInstance,
    IN PUNICODE_STRING pString);

W32KAPI
UINT
NtUserGetInternalWindowPos(
    IN HWND hwnd,
    OUT LPRECT lpRect OPTIONAL,
    OUT LPPOINT lpPoint OPTIONAL);

W32KAPI
NTSTATUS
NtUserInitTask(
    IN UINT dwExpWinVer,
    IN DWORD dwAppCompatFlags,
    IN PUNICODE_STRING pstrModName,
    IN PUNICODE_STRING pstrBaseFileName,
    IN DWORD hTaskWow,
    IN DWORD dwHotkey,
    IN DWORD idTask,
    IN DWORD dwX,
    IN DWORD dwY,
    IN DWORD dwXSize,
    IN DWORD dwYSize);

W32KAPI
BOOL
NtUserPostThreadMessage(
    IN DWORD id,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

W32KAPI
BOOL
NtUserRegisterTasklist(
    IN HWND hwndTasklist);

W32KAPI
BOOL
NtUserSetClipboardData(
    IN UINT wFmt,
    IN HANDLE hMem,
    IN PSETCLIPBDATA scd);

W32KAPI
BOOL
NtUserCloseClipboard(
    VOID);

W32KAPI
BOOL
NtUserEmptyClipboard(
    VOID);

W32KAPI
HANDLE
NtUserConvertMemHandle(
    IN LPBYTE lpData,
    IN UINT cbNULL);

W32KAPI
NTSTATUS
NtUserCreateLocalMemHandle(
    IN HANDLE hMem,
    OUT LPBYTE lpData OPTIONAL,
    IN UINT cbData,
    OUT PUINT lpcbNeeded OPTIONAL);

W32KAPI
HHOOK
NtUserSetWindowsHookEx(
    IN HANDLE hmod,
    IN PUNICODE_STRING pstrLib OPTIONAL,
    IN DWORD idThread,
    IN int nFilterType,
    IN PROC pfnFilterProc,
    IN DWORD dwFlags);

W32KAPI
BOOL
NtUserSetInternalWindowPos(
    IN HWND hwnd,
    IN UINT cmdShow,
    IN CONST RECT *lpRect,
    IN CONST POINT *lpPoint);

W32KAPI
BOOL
NtUserChangeClipboardChain(
    IN HWND hwndRemove,
    IN HWND hwndNewNext);

W32KAPI
DWORD
NtUserCheckMenuItem(
    IN HMENU hmenu,
    IN UINT wIDCheckItem,
    IN UINT wCheck);

W32KAPI
HWND
NtUserChildWindowFromPointEx(
    IN HWND hwndParent,
    IN POINT point,
    IN UINT flags);

W32KAPI
BOOL
NtUserClipCursor(
    IN CONST RECT *lpRect OPTIONAL);

W32KAPI
HACCEL
NtUserCreateAcceleratorTable(
    IN LPACCEL lpAccel,
    IN INT cAccel);

W32KAPI
BOOL
NtUserDeleteMenu(
    IN HMENU hmenu,
    IN UINT nPosition,
    IN UINT dwFlags);

W32KAPI
BOOL
NtUserDestroyAcceleratorTable(
    IN HACCEL hAccel);

W32KAPI
BOOL
NtUserDestroyCursor(
    IN HCURSOR hcurs,
    IN DWORD cmd);

W32KAPI
HANDLE
NtUserGetClipboardData(
    IN UINT fmt,
    OUT PGETCLIPBDATA pgcd);

W32KAPI
BOOL
NtUserDestroyMenu(
    IN HMENU hmenu);

W32KAPI
BOOL
NtUserDestroyWindow(
    IN HWND hwnd);

W32KAPI
LRESULT
NtUserDispatchMessage(
    IN CONST MSG *pmsg);

W32KAPI
BOOL
NtUserEnableMenuItem(
    IN HMENU hMenu,
    IN UINT wIDEnableItem,
    IN UINT wEnable);

W32KAPI
BOOL
NtUserAttachThreadInput(
    IN DWORD idAttach,
    IN DWORD idAttachTo,
    IN BOOL fAttach);

W32KAPI
BOOL
NtUserGetWindowPlacement(
    IN HWND hwnd,
    OUT PWINDOWPLACEMENT pwp);

W32KAPI
BOOL
NtUserSetWindowPlacement(
    IN HWND hwnd,
    IN CONST WINDOWPLACEMENT *lpwndpl);

W32KAPI
BOOL
NtUserLockWindowUpdate(
    IN HWND hwnd);

W32KAPI
BOOL
NtUserGetClipCursor(
    OUT LPRECT lpRect);

W32KAPI
BOOL
NtUserEnableScrollBar(
    IN HWND hwnd,
    IN UINT wSBflags,
    IN UINT wArrows);

W32KAPI
BOOL
NtUserDdeSetQualityOfService(
    IN HWND hwndClient,
    IN CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
    OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev OPTIONAL);

W32KAPI
BOOL
NtUserDdeGetQualityOfService(
    IN HWND hwndClient,
    IN HWND hwndServer,
    OUT PSECURITY_QUALITY_OF_SERVICE pqos);

W32KAPI
DWORD
NtUserGetMenuIndex(
    IN HMENU hMenu,
    IN HMENU hSubMenu);

W32KAPI
VOID
NtUserSetRipFlags(
    DWORD dwRipFlags, DWORD PID);

W32KAPI
VOID
NtUserSetDbgTag(
    int     tag,
    DWORD   dwBitFlags);

W32KAPI
BOOL
NtUserThunkedMenuItemInfo(
    IN HMENU hMenu,
    IN UINT nPosition,
    IN BOOL fByPosition,
    IN BOOL fInsert,
    IN LPMENUITEMINFOW lpmii,
    IN PUNICODE_STRING pstrItem OPTIONAL);

W32KAPI
BOOL
NtUserThunkedMenuInfo(
    IN HMENU hMenu,
    IN LPCMENUINFO lpmi);

W32KAPI
BOOL
NtUserSetMenuDefaultItem(
    IN HMENU hMenu,
    IN UINT wID,
    IN UINT fByPosition
    );

W32KAPI
BOOL
NtUserDrawAnimatedRects(
    IN HWND hwnd,
    IN int idAni,
    IN CONST RECT * lprcFrom,
    IN CONST RECT * lprcTo);

HANDLE
NtUserLoadIcoCur(
    HANDLE hIcon,
    DWORD cxNew,
    DWORD cyNew,
    DWORD LR_flags);

W32KAPI
BOOL
NtUserDrawCaption(
    IN HWND hwnd,
    IN HDC hdc,
    IN CONST RECT *lprc,
    IN UINT flags);

W32KAPI
BOOL
NtUserFlashWindowEx(
    IN PFLASHWINFO pfwi);

W32KAPI
BOOL
NtUserPaintDesktop(
    IN HDC hdc);

W32KAPI
SHORT
NtUserGetAsyncKeyState(
    IN int vKey);

W32KAPI
HBRUSH
NtUserGetControlBrush(
    IN HWND hwnd,
    IN HDC hdc,
    IN UINT msg);

W32KAPI
HBRUSH
NtUserGetControlColor(
    IN HWND hwndParent,
    IN HWND hwndCtl,
    IN HDC hdc,
    IN UINT msg);

W32KAPI
BOOL
NtUserEndMenu(
    VOID);

W32KAPI
int
NtUserCountClipboardFormats(
    VOID);

W32KAPI
DWORD
NtUserGetClipboardSequenceNumber(
    VOID);

W32KAPI
UINT
NtUserGetCaretBlinkTime(
    VOID);

W32KAPI
HWND
NtUserGetClipboardOwner(
    VOID);

DWORD
NtUserGetClipboardSerialNumber(
    VOID);

W32KAPI
HWND
NtUserGetClipboardViewer(
    VOID);

W32KAPI
UINT
NtUserGetDoubleClickTime(
    VOID);

W32KAPI
HWND
NtUserGetForegroundWindow(
    VOID);

W32KAPI
HWND
NtUserGetOpenClipboardWindow(
    VOID);

W32KAPI
int
NtUserGetPriorityClipboardFormat(
    OUT UINT *paFormatPriorityList,
    IN int cFormats);

W32KAPI
HMENU
NtUserGetSystemMenu(
    IN HWND hwnd,
    IN BOOL bRevert);

W32KAPI
BOOL
NtUserGetUpdateRect(
    IN HWND hwnd,
    IN LPRECT prect OPTIONAL,
    IN BOOL bErase);

W32KAPI
BOOL
NtUserHideCaret(
    IN HWND hwnd);

W32KAPI
BOOL
NtUserHiliteMenuItem(
    IN HWND hwnd,
    IN HMENU hMenu,
    IN UINT uIDHiliteItem,
    IN UINT uHilite);

W32KAPI
BOOL
NtUserInvalidateRect(
    IN HWND hwnd,
    IN CONST RECT *prect OPTIONAL,
    IN BOOL bErase);

W32KAPI
BOOL
NtUserIsClipboardFormatAvailable(
    IN UINT nFormat);

W32KAPI
BOOL
NtUserKillTimer(
    IN HWND hwnd,
    IN UINT_PTR nIDEvent);

W32KAPI
HWND
NtUserMinMaximize(
    IN HWND hwnd,
    IN UINT nCmdShow,
    IN BOOL fKeepHidden);

W32KAPI
BOOL
NtUserMNDragOver(
    IN POINT * ppt,
    OUT PMNDRAGOVERINFO pmndoi);

W32KAPI
BOOL
NtUserMNDragLeave(
    VOID);

W32KAPI
BOOL
NtUserOpenClipboard(
    IN HWND hwnd,
    OUT PBOOL pfEmptyClient);

W32KAPI
BOOL
NtUserPeekMessage(
    OUT LPMSG pmsg,
    IN HWND hwnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax,
    IN UINT wRemoveMsg);

W32KAPI
BOOL
NtUserPostMessage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

W32KAPI
BOOL
NtUserSendNotifyMessage(
    IN HWND hwnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam OPTIONAL);

W32KAPI
BOOL
NtUserSendMessageCallback(
    IN HWND hwnd,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN SENDASYNCPROC lpResultCallBack,
    IN ULONG_PTR dwData);

W32KAPI
BOOL
NtUserRegisterHotKey(
    IN HWND hwnd,
    IN int id,
    IN UINT fsModifiers,
    IN UINT vk);

W32KAPI
BOOL
NtUserRemoveMenu(
    IN HMENU hmenu,
    IN UINT nPosition,
    IN UINT dwFlags);

W32KAPI
BOOL
NtUserScrollWindowEx(
    IN HWND hwnd,
    IN int XAmount,
    IN int YAmount,
    IN CONST RECT *pRect OPTIONAL,
    IN CONST RECT *pClipRect OPTIONAL,
    IN HRGN hrgnUpdate,
    OUT LPRECT prcUpdate OPTIONAL,
    IN UINT flags);

W32KAPI
HWND
NtUserSetActiveWindow(
    IN HWND hwnd);

W32KAPI
HWND
NtUserSetCapture(
    IN HWND hwnd);

W32KAPI
WORD
NtUserSetClassWord(
    IN HWND hwnd,
    IN int nIndex,
    IN WORD wNewWord);

W32KAPI
HWND
NtUserSetClipboardViewer(
    IN HWND hwndNewViewer);

W32KAPI
HCURSOR
NtUserSetCursor(
    IN HCURSOR hCursor);

W32KAPI
HWND
NtUserSetFocus(
    IN HWND hwnd);

W32KAPI
BOOL
NtUserSetMenu(
    IN HWND  hwnd,
    IN HMENU hmenu,
    IN BOOL  fRedraw);

W32KAPI
BOOL
NtUserSetMenuContextHelpId(
    IN HMENU hMenu,
    IN DWORD dwContextHelpId);

W32KAPI
BOOL
NtUserSetMenuFlagRtoL(
    IN HMENU hMenu);

W32KAPI
HWND
NtUserSetParent(
    IN HWND hwndChild,
    IN HWND hwndNewParent);

W32KAPI
int
NtUserSetScrollInfo(
    IN HWND hwnd,
    IN int nBar,
    IN LPCSCROLLINFO pInfo,
    IN BOOL fRedraw);

W32KAPI
BOOL
NtUserSetSysColors(
    IN int cElements,
    IN CONST INT * lpaElements,
    IN CONST COLORREF * lpaRgbValues,
    IN UINT  uOptions);

W32KAPI
UINT_PTR
NtUserSetTimer(
    IN HWND hwnd,
    IN UINT_PTR nIDEvent,
    IN UINT wElapse,
    IN TIMERPROC pTimerFunc);

W32KAPI
LONG
NtUserSetWindowLong(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG dwNewLong,
    IN BOOL bAnsi);

#ifdef _WIN64
W32KAPI
LONG_PTR
NtUserSetWindowLongPtr(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG_PTR dwNewLong,
    IN BOOL bAnsi);
#else
#define NtUserSetWindowLongPtr  NtUserSetWindowLong
#endif

W32KAPI
WORD
NtUserSetWindowWord(
    IN HWND hwnd,
    IN int nIndex,
    IN WORD wNewWord);

W32KAPI
HHOOK
NtUserSetWindowsHookAW(
    IN int nFilterType,
    IN HOOKPROC pfnFilterProc,
    IN DWORD dwFlags);

W32KAPI
BOOL
NtUserShowCaret(
    IN HWND hwnd);

W32KAPI
BOOL
NtUserShowScrollBar(
    IN HWND hwnd,
    IN int iBar,
    IN BOOL fShow);

W32KAPI
BOOL
NtUserShowWindowAsync(
    IN HWND hwnd,
    IN int nCmdShow);

W32KAPI
BOOL
NtUserShowWindow(
    IN HWND hwnd,
    IN int nCmdShow);

W32KAPI
BOOL
NtUserTrackMouseEvent(
    IN OUT LPTRACKMOUSEEVENT lpTME
    );

W32KAPI
BOOL
NtUserTrackPopupMenuEx(
    IN HMENU hMenu,
    IN UINT uFlags,
    IN int x,
    IN int y,
    IN HWND hwnd,
    IN CONST TPMPARAMS *pparamst OPTIONAL);

W32KAPI
BOOL
NtUserTranslateMessage(
    IN CONST MSG *lpMsg,
    IN UINT flags);

W32KAPI
BOOL
NtUserUnhookWindowsHookEx(
    IN HHOOK hhk);

W32KAPI
BOOL
NtUserUnregisterHotKey(
    IN HWND hwnd,
    IN int id);

W32KAPI
BOOL
NtUserValidateRect(
    IN HWND hwnd,
    IN CONST RECT *lpRect OPTIONAL);

W32KAPI
DWORD
NtUserWaitForInputIdle(
    IN ULONG_PTR idProcess,
    IN DWORD dwMilliseconds,
    IN BOOL fSharedWow);

W32KAPI
HWND
NtUserWindowFromPoint(
    IN POINT Point);

W32KAPI
HDC
NtUserBeginPaint(
    IN HWND hwnd,
    OUT LPPAINTSTRUCT lpPaint);

W32KAPI
BOOL
NtUserCreateCaret(
    IN HWND hwnd,
    IN HBITMAP hBitmap,
    IN int nWidth,
    IN int nHeight);

W32KAPI
BOOL
NtUserEndPaint(
    IN HWND hwnd,
    IN CONST PAINTSTRUCT *lpPaint);

W32KAPI
int
NtUserExcludeUpdateRgn(
    IN HDC hDC,
    IN HWND hwnd);

W32KAPI
HDC
NtUserGetDC(
    IN HWND hwnd);

W32KAPI
HDC
NtUserGetDCEx(
    IN HWND hwnd,
    IN HRGN hrgnClip,
    IN DWORD flags);

W32KAPI
HDC
NtUserGetWindowDC(
    IN HWND hwnd);

W32KAPI
int
NtUserGetUpdateRgn(
    IN HWND hwnd,
    IN HRGN hRgn,
    IN BOOL bErase);

W32KAPI
BOOL
NtUserRedrawWindow(
    IN HWND hwnd,
    IN CONST RECT *lprcUpdate OPTIONAL,
    IN HRGN hrgnUpdate,
    IN UINT flags);

W32KAPI
BOOL
NtUserInvalidateRgn(
    IN HWND hwnd,
    IN HRGN hRgn,
    IN BOOL bErase);

W32KAPI
int
NtUserSetWindowRgn(
    IN HWND hwnd,
    IN HRGN hRgn,
    IN BOOL bRedraw);

W32KAPI
BOOL
NtUserScrollDC(
    IN HDC hDC,
    IN int dx,
    IN int dy,
    IN CONST RECT *lprcScroll OPTIONAL,
    IN CONST RECT *lprcClip OPTIONAL,
    IN HRGN hrgnUpdate,
    OUT LPRECT lprcUpdate OPTIONAL);

W32KAPI
int
NtUserInternalGetWindowText(
    IN HWND hwnd,
    OUT LPWSTR lpString,
    IN int nMaxCount);

W32KAPI
int
NtUserGetMouseMovePointsEx(
    IN UINT             cbSize,
    IN CONST MOUSEMOVEPOINT *lppt,
    OUT MOUSEMOVEPOINT *lpptBuf,
    IN UINT             nBufPoints,
    IN DWORD            resolution);

W32KAPI
int
NtUserToUnicodeEx(
    IN UINT wVirtKey,
    IN UINT wScanCode,
    IN CONST BYTE *lpKeyState,
    OUT LPWSTR lpszBuff,
    IN int cchBuff,
    IN UINT wFlags,
    IN HKL hKeyboardLayout);

W32KAPI
BOOL
NtUserYieldTask(
    VOID);

W32KAPI
BOOL
NtUserWaitMessage(
    VOID);

W32KAPI
UINT
NtUserLockWindowStation(
    IN HWINSTA hWindowStation);

W32KAPI
BOOL
NtUserUnlockWindowStation(
    IN HWINSTA hWindowStation);

W32KAPI
UINT
NtUserSetWindowStationUser(
    IN HWINSTA hWindowStation,
    IN PLUID pLuidUser,
    IN PSID pSidUser OPTIONAL,
    IN DWORD cbSidUser);

W32KAPI
BOOL
NtUserSetLogonNotifyWindow(
    IN HWND hwndNotify);

W32KAPI
BOOL
NtUserSetSystemCursor(
    IN HCURSOR hcur,
    IN DWORD id);

W32KAPI
HCURSOR
NtUserGetCursorFrameInfo(
    IN HCURSOR hcur,
    IN int iFrame,
    OUT LPDWORD pjifRate,
    OUT LPINT pccur);

W32KAPI
BOOL
NtUserSetCursorContents(
    IN HCURSOR hCursor,
    IN HCURSOR hCursorNew);

W32KAPI
HCURSOR
NtUserFindExistingCursorIcon(
    IN PUNICODE_STRING pstrModName,
    IN PUNICODE_STRING pstrResName,
    IN PCURSORFIND     pcfSearch);

W32KAPI
BOOL
NtUserSetCursorIconData(
    IN HCURSOR         hCursor,
    IN PUNICODE_STRING pstrModName,
    IN PUNICODE_STRING pstrResName,
    IN PCURSORDATA     pData);

BOOL
NtUserWOWModuleUnload(
    IN HANDLE hModule);

BOOL
NtUserWOWCleanup(
    IN HANDLE hInstance,
    IN DWORD hTaskWow);

W32KAPI
BOOL
NtUserGetMenuItemRect(
    IN HWND hwnd,
    IN HMENU hMenu,
    IN UINT uItem,
    OUT LPRECT lprcItem);

W32KAPI
int
NtUserMenuItemFromPoint(
    IN HWND hwnd,
    IN HMENU hMenu,
    IN POINT ptScreen);

W32KAPI
BOOL
NtUserGetCaretPos(
    OUT LPPOINT lpPoint);

W32KAPI
BOOL
NtUserDefSetText(
    IN HWND hwnd,
    IN PLARGE_STRING Text OPTIONAL);

W32KAPI
NTSTATUS
NtUserQueryInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    IN OUT PULONG ReturnLength OPTIONAL);

W32KAPI
NTSTATUS
NtUserSetInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength);

W32KAPI
NTSTATUS
NtUserSetInformationProcess(
    IN HANDLE hProcess,
    IN USERPROCESSINFOCLASS ProcessInfoClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength);

W32KAPI
BOOL
NtUserNotifyProcessCreate(
    IN DWORD dwProcessId,
    IN DWORD dwParentThreadId,
    IN ULONG_PTR dwData,
    IN DWORD dwFlags);

W32KAPI
NTSTATUS
NtUserTestForInteractiveUser(
    IN PLUID pluidCaller);

W32KAPI
BOOL
NtUserSetConsoleReserveKeys(
    IN HWND hwnd,
    IN DWORD fsReserveKeys);

W32KAPI
VOID
NtUserModifyUserStartupInfoFlags(
    IN DWORD dwMask,
    IN DWORD dwFlags);

W32KAPI
BOOL
NtUserSetWindowFNID(
    IN HWND hwnd,
    IN WORD fnid);

W32KAPI
VOID
NtUserAlterWindowStyle(
    IN HWND hwnd,
    IN DWORD mask,
    IN DWORD flags);

W32KAPI
VOID
NtUserSetThreadState(
    IN DWORD dwFlags,
    IN DWORD dwMask);

W32KAPI
ULONG_PTR
NtUserGetThreadState(
    IN USERTHREADSTATECLASS ThreadState);


LRESULT
NtUserGetListboxString(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN PLARGE_STRING pString,
    IN ULONG_PTR xParam,
    IN DWORD xpfn,
    IN PBOOL pbNotString);

W32KAPI
BOOL
NtUserValidateHandleSecure(
    IN HANDLE h);

W32KAPI
BOOL
NtUserUserHandleGrantAccess(
    IN HANDLE hUserHandle,
    IN HANDLE hJob,
    IN BOOL   bGrant);

W32KAPI
HWND
NtUserCreateWindowEx(
    IN DWORD dwExStyle,
    IN PLARGE_STRING pstrClassName,
    IN PLARGE_STRING pstrWindowName OPTIONAL,
    IN DWORD dwStyle,
    IN int x,
    IN int y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hwndParent,
    IN HMENU hmenu,
    IN HANDLE hModule,
    IN LPVOID pParam,
    IN DWORD dwFlags);

W32KAPI
NTSTATUS
NtUserBuildHwndList(
    IN HDESK hdesk,
    IN HWND hwndNext,
    IN BOOL fEnumChildren,
    IN DWORD idThread,
    IN UINT cHwndMax,
    OUT HWND *phwndFirst,
    OUT PUINT pcHwndNeeded);

W32KAPI
NTSTATUS
NtUserBuildPropList(
    IN HWND hwnd,
    IN UINT cPropMax,
    OUT PPROPSET pPropSet,
    OUT PUINT pcPropNeeded);

W32KAPI
NTSTATUS
NtUserBuildNameList(
    IN HWINSTA hwinsta,
    IN UINT cbNameList,
    OUT PNAMELIST pNameList,
    OUT PUINT pcbNeeded);

W32KAPI
HKL
NtUserActivateKeyboardLayout(
    IN HKL hkl,
    IN UINT Flags);

W32KAPI
HKL
NtUserLoadKeyboardLayoutEx(
    IN HANDLE hFile,
    IN DWORD offTable,
    IN HKL hkl,
    IN PUNICODE_STRING pstrKLID,
    IN UINT KbdInputLocale,
    IN UINT Flags);

W32KAPI
BOOL
NtUserUnloadKeyboardLayout(
    IN HKL hkl);

W32KAPI
BOOL
NtUserSetSystemMenu(
    IN HWND hwnd,
    IN HMENU hmenu);

W32KAPI
BOOL
NtUserDragDetect(
    IN HWND hwnd,
    IN POINT pt);

W32KAPI
UINT_PTR
NtUserSetSystemTimer(
    IN HWND hwnd,
    IN UINT_PTR nIDEvent,
    IN DWORD dwElapse,
    IN WNDPROC pTimerFunc);

W32KAPI
BOOL
NtUserQuerySendMessage(
    OUT PMSG pmsg);

W32KAPI
UINT
NtUserSendInput(
    IN UINT    cInputs,
    IN CONST INPUT *pInputs,
    IN int     cbSize);

W32KAPI
BOOL
NtUserImpersonateDdeClientWindow(
    IN HWND hwndClient,
    IN HWND hwndServer);

W32KAPI
ULONG_PTR
NtUserGetCPD(
    IN HWND hwnd,
    IN DWORD options,
    IN ULONG_PTR dwData);

W32KAPI
int
NtUserCopyAcceleratorTable(
    IN HACCEL hAccelSrc,
    IN OUT LPACCEL lpAccelDst OPTIONAL,
    IN int cAccelEntries);

W32KAPI
HWND
NtUserFindWindowEx(
    IN HWND hwndParent,
    IN HWND hwndChild,
    IN PUNICODE_STRING pstrClassName OPTIONAL,
    IN PUNICODE_STRING pstrWindowName OPTIONAL,
    IN DWORD dwType);

W32KAPI
BOOL
NtUserGetClassInfo(
    IN HINSTANCE hInstance OPTIONAL,
    IN PUNICODE_STRING pstrClassName,
    IN OUT LPWNDCLASSEXW lpWndClass,
    OUT LPWSTR *ppszMenuName,
    IN BOOL bAnsi);

W32KAPI
int
NtUserGetClassName(
    IN HWND hwnd,
    IN BOOL bReal,
    IN OUT PUNICODE_STRING pstrClassName);

W32KAPI
int
NtUserGetClipboardFormatName(
    IN UINT format,
    OUT LPWSTR lpszFormatName,
    IN UINT chMax);

W32KAPI
int
NtUserGetKeyNameText(
    IN LONG lParam,
    OUT LPWSTR lpszKeyName,
    IN UINT chMax);

W32KAPI
BOOL
NtUserGetKeyboardLayoutName(
    IN OUT PUNICODE_STRING pstrKLID);

W32KAPI
UINT
NtUserGetKeyboardLayoutList(
    IN UINT nItems,
    OUT HKL *lpBuff);

W32KAPI
DWORD
NtUserGetGuiResources(
    HANDLE hProcess,
    DWORD dwFlags);

W32KAPI
UINT
NtUserMapVirtualKeyEx(
    IN UINT uCode,
    IN UINT uMapType,
    IN ULONG_PTR dwHKLorPKL,
    IN BOOL bHKL);

W32KAPI
ATOM
NtUserRegisterClassExWOW(
    IN WNDCLASSEX *lpWndClass,
    IN PUNICODE_STRING pstrClassName,
    IN PCLSMENUNAME pcmn,
    IN WORD fnid,
    IN DWORD dwFlags,
    IN LPDWORD pdwWOWstuff OPTIONAL);

W32KAPI
UINT
NtUserRegisterWindowMessage(
    IN PUNICODE_STRING pstrMessage);

W32KAPI
HANDLE
NtUserRemoveProp(
    IN HWND hwnd,
    IN DWORD dwProp);

W32KAPI
BOOL
NtUserSetProp(
    IN HWND hwnd,
    IN DWORD dwProp,
    IN HANDLE hData);

W32KAPI
BOOL
NtUserUnregisterClass(
    IN PUNICODE_STRING pstrClassName,
    IN HINSTANCE hInstance,
    OUT PCLSMENUNAME pcmn);

W32KAPI
SHORT
NtUserVkKeyScanEx(
    IN WCHAR ch,
    IN ULONG_PTR dwHKLorPKL,
    IN BOOL bHKL);

W32KAPI
NTSTATUS
NtUserEnumDisplayDevices(
    IN PUNICODE_STRING pstrDeviceName,
    IN DWORD iDevNum,
    IN OUT LPDISPLAY_DEVICEW lpDisplayDevice,
    IN DWORD dwFlags);

W32KAPI
HWINEVENTHOOK
NtUserSetWinEventHook(
    IN DWORD           eventMin,
    IN DWORD           eventMax,
    IN HMODULE         hmodWinEventProc,
    IN PUNICODE_STRING pstrLib OPTIONAL,
    IN WINEVENTPROC    pfnWinEventProc,
    IN DWORD           idEventProcess,
    IN DWORD           idEventThread,
    IN DWORD           dwFlags);

W32KAPI
BOOL
NtUserUnhookWinEvent(
    IN HWINEVENTHOOK hWinEventHook);

W32KAPI
VOID
NtUserNotifyWinEvent(
    IN DWORD event,
    IN HWND  hwnd,
    IN LONG  idObject,
    IN LONG  idChild);

W32KAPI
BOOL
NtUserGetGUIThreadInfo(
    IN DWORD idThread,
    IN OUT PGUITHREADINFO pgui);

W32KAPI
BOOL
NtUserGetTitleBarInfo(
    IN HWND hwnd,
    IN OUT PTITLEBARINFO ptbi);

W32KAPI
BOOL
NtUserGetScrollBarInfo(
    IN HWND hwnd,
    IN LONG idObject,
    IN OUT PSCROLLBARINFO ptbi);

W32KAPI
BOOL
NtUserGetComboBoxInfo(
    IN HWND hwnd,
    IN OUT PCOMBOBOXINFO pcbi
    );

W32KAPI
DWORD
NtUserGetListBoxInfo(
    IN HWND hwnd
    );

W32KAPI
HWND
NtUserGetAncestor(
    IN HWND hwnd,
    IN UINT gaFlags);

W32KAPI
BOOL
NtUserGetCursorInfo(
    IN OUT PCURSORINFO pci);

W32KAPI
HWND
NtUserRealChildWindowFromPoint(
    IN HWND hwndParent,
    IN POINT pt
    );

W32KAPI
BOOL
NtUserGetAltTabInfo(
    IN HWND hwnd,
    IN int iItem,
    IN OUT PALTTABINFO pati,
    OUT LPWSTR lpszItemText,
    IN UINT cchItemText,
    IN BOOL bAnsi);

W32KAPI
BOOL
NtUserGetMenuBarInfo(
    IN HWND hwnd,
    IN long idObject,
    IN long idItem,
    IN OUT PMENUBARINFO pmbi);

W32KAPI
LONG
NtUserChangeDisplaySettings(
    IN PUNICODE_STRING pstrDeviceName,
    IN LPDEVMODEW lpDevMode,
    IN HWND hwnd,
    IN DWORD dwFlags,
    IN PVOID lParam);

W32KAPI
BOOL
NtUserCallMsgFilter(
    IN OUT LPMSG lpMsg,
    IN int nCode);

W32KAPI
int
NtUserDrawMenuBarTemp(
    IN HWND hwnd,
    IN HDC hdc,
    IN LPCRECT lprc,
    IN HMENU hMenu,
    IN HFONT hFont);

W32KAPI
BOOL
NtUserDrawCaptionTemp(
    IN HWND hwnd,
    IN HDC hdc,
    IN LPCRECT lprc,
    IN HFONT hFont,
    IN HICON hicon,
    IN PUNICODE_STRING pstrText,
    IN UINT flags);

W32KAPI
SHORT
NtUserGetKeyState(
    IN int vk);

W32KAPI
BOOL
NtUserGetKeyboardState(
    OUT PBYTE pb);

W32KAPI
HANDLE
NtUserQueryWindow(
    IN HWND hwnd,
    IN WINDOWINFOCLASS WindowInfo);

W32KAPI
BOOL
NtUserSBGetParms(
    IN HWND hwnd,
    IN int code,
    IN PSBDATA pw,
    IN OUT LPSCROLLINFO lpsi);

W32KAPI
BOOL
NtUserBitBltSysBmp(
    IN HDC hdc,
    IN int xDest,
    IN int yDest,
    IN int cxDest,
    IN int cyDest,
    IN int xSrc,
    IN int ySrc,
    IN DWORD dwRop);

W32KAPI
LRESULT
NtUserMessageCall(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN ULONG_PTR xParam,
    IN DWORD xpfnProc,
    IN BOOL bAnsi);

W32KAPI
LRESULT
NtUserCallNextHookEx(
    IN int nCode,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN BOOL bAnsi);

W32KAPI
BOOL
NtUserEnumDisplayMonitors(
    IN HDC             hdc,
    IN LPCRECT         lprcClip,
    IN MONITORENUMPROC lpfnEnum,
    IN LPARAM          dwData);

W32KAPI
NTSTATUS
NtUserEnumDisplaySettings(
    IN PUNICODE_STRING pstrDeviceName,
    IN DWORD           iModeNum,
    OUT LPDEVMODEW     lpDevMode,
    IN  DWORD          dwFlags);

LONG
NtUserChangeDisplaySettings(
    IN PUNICODE_STRING pstrDeviceName,
    IN LPDEVMODEW lpDevMode,
    IN HWND hwnd,
    IN DWORD dwFlags,
    IN PVOID lParam);

W32KAPI
BOOL
NtUserQueryUserCounters(
    IN  DWORD       dwQueryType,
    IN  LPVOID      pvIn,
    IN  DWORD       dwInSize,
    OUT LPVOID      pvResult,
    IN  DWORD       dwOutSize
    );

W32KAPI
BOOL
NtUserUpdateLayeredWindow(
    IN HWND hwnd,
    IN HDC hdcDst,
    IN POINT *pptDst,
    IN SIZE *psize,
    IN HDC hdcSrc,
    IN POINT *pptSrc,
    IN COLORREF crKey,
    IN BLENDFUNCTION *pblend,
    IN DWORD dwFlags);

W32KAPI
BOOL
NtUserSetLayeredWindowAttributes(
    IN HWND hwnd,
    IN COLORREF crKey,
    IN BYTE bAlpha,
    IN DWORD dwFlags);

W32KAPI
NTSTATUS
NtUserRemoteConnect(
    IN PDOCONNECTDATA pDoConnectData,
    IN ULONG DisplayDriverNameLength,
    IN PWCHAR DisplayDriverName );

W32KAPI
NTSTATUS
NtUserRemoteRedrawRectangle(
    IN WORD Left,
    IN WORD Top,
    IN WORD Right,
    IN WORD Bottom );

W32KAPI
NTSTATUS
NtUserRemoteRedrawScreen( VOID );

W32KAPI
NTSTATUS
NtUserRemoteStopScreenUpdates( VOID );

W32KAPI
NTSTATUS
NtUserCtxDisplayIOCtl(
    IN ULONG  DisplayIOCtlFlags,
    IN PUCHAR pDisplayIOCtlData,
    IN ULONG  cbDisplayIOCtlData);

W32KAPI
HPALETTE
NtUserSelectPalette(
    IN HDC hdc,
    IN HPALETTE hpalette,
    IN BOOL fForceBackground);

W32KAPI
NTSTATUS
NtUserProcessConnect(
    IN HANDLE    hProcess,
    IN OUT PVOID pConnectInfo,
    IN ULONG     cbConnectInfo);

W32KAPI
NTSTATUS
NtUserSoundSentry(VOID);

W32KAPI
NTSTATUS
NtUserInitialize(
    IN DWORD   dwVersion,
    IN HANDLE  hPowerRequestEvent,
    IN HANDLE  hMediaRequestEvent);

#endif  // _NTUSER_
