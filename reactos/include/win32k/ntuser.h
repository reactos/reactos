#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

ULONG STDCALL
NtUserGetSystemMetrics(ULONG Index);
DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset);
DWORD STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index);
INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc);
BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect);
HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom);
BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point);

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

DWORD
STDCALL
NtUserBlockInput(
  DWORD Unknown0);

DWORD
STDCALL
NtUserBuildHwndList(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
STDCALL
NtUserBuildNameList(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserBuildPropList(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserCallHwnd(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserCallHwndLock(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserCallHwndOpt(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserCallNextHookEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserCallNoParam(
  DWORD Unknown0);

DWORD
STDCALL
NtUserCallOneParam(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserCallTwoParam(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserChangeClipboardChain(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserChangeDisplaySettings(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserCheckMenuItem(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD STDCALL
NtUserChildWindowFromPointEx(HWND Parent,
			     LONG x,
			     LONG y,
			     UINT Flags);

DWORD
STDCALL
NtUserClipCursor(
  DWORD Unknown0);

DWORD
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

DWORD
STDCALL
NtUserCopyAcceleratorTable(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserCountClipboardFormats(VOID);

DWORD
STDCALL
NtUserCreateAcceleratorTable(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserCreateCaret(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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
  DWORD Unknown12);

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

DWORD STDCALL
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     LONG x,
		     LONG y,
		     LONG cx,
		     LONG cy,
		     UINT Flags);
DWORD
STDCALL
NtUserDefSetText(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserDeleteMenu(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserDestroyAcceleratorTable(
  DWORD Unknown0);

DWORD
STDCALL
NtUserDestroyCursor(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserDestroyMenu(
  DWORD Unknown0);

BOOLEAN STDCALL
NtUserDestroyWindow(HWND Wnd);

LRESULT
STDCALL
NtUserDispatchMessage(CONST MSG* lpmsg);

DWORD
STDCALL
NtUserDragDetect(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserDragObject(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

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

DWORD
STDCALL
NtUserDrawIconEx(
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

DWORD
STDCALL
NtUserDrawMenuBarTemp(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserEmptyClipboard(VOID);

DWORD
STDCALL
NtUserEnableMenuItem(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserEnableScrollBar(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserEndMenu(VOID);

BOOL STDCALL
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs);

DWORD
STDCALL
NtUserEnumDisplayDevices(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserEnumDisplayMonitors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserEnumDisplaySettings(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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

DWORD
STDCALL
NtUserFindExistingCursorIcon(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HWND
STDCALL
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING  ucClassName,
  PUNICODE_STRING  ucWindowName,
  DWORD Unknown4);

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

DWORD
STDCALL
NtUserGetCaretBlinkTime(VOID);

DWORD
STDCALL
NtUserGetCaretPos(
  DWORD Unknown0);

DWORD STDCALL
NtUserGetClassInfo(IN LPWSTR ClassName,
		   IN ULONG InfoClass,
		   OUT PVOID Info,
		   IN ULONG InfoLength,
		   OUT PULONG ReturnedLength);

DWORD
STDCALL
NtUserGetClassName(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetClipboardData(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetClipboardFormatName(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetClipboardOwner(VOID);

DWORD
STDCALL
NtUserGetClipboardSequenceNumber(VOID);

DWORD
STDCALL
NtUserGetClipboardViewer(VOID);

DWORD
STDCALL
NtUserGetClipCursor(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserGetCursorInfo(
  DWORD Unknown0);

HDC STDCALL
NtUserGetDC(HWND hWnd);

HDC STDCALL NtUserGetDCEx(HWND hWnd, HANDLE hRegion, ULONG Flags);

DWORD
STDCALL
NtUserGetDoubleClickTime(VOID);

DWORD
STDCALL
NtUserGetForegroundWindow(VOID);

DWORD
STDCALL
NtUserGetGuiResources(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetGUIThreadInfo(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetIconInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserGetIconSize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetKeyNameText(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetKeyState(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetListBoxInfo(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetMenuBarInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserGetMenuIndex(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetMenuItemRect(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
STDCALL
NtUserGetMessage(
  LPMSG lpMsg,
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

DWORD
STDCALL
NtUserGetOpenClipboardWindow(VOID);

DWORD
STDCALL
NtUserGetPriorityClipboardFormat(
  DWORD Unknown0,
  DWORD Unknown1);

HWINSTA
STDCALL
NtUserGetProcessWindowStation(VOID);

DWORD
STDCALL
NtUserGetScrollBarInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetSystemMenu(
  DWORD Unknown0,
  DWORD Unknown1);

HDESK
STDCALL
NtUserGetThreadDesktop(
  DWORD dwThreadId,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetThreadState(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetTitleBarInfo(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetUpdateRect(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetUpdateRgn(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserGetWindowDC(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetWindowPlacement(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserHideCaret(
  DWORD Unknown0);

DWORD
STDCALL
NtUserHiliteMenuItem(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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

DWORD
STDCALL
NtUserInternalGetWindowText(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserInvalidateRect(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserInvalidateRgn(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserIsClipboardFormatAvailable(
  DWORD Unknown0);

DWORD
STDCALL
NtUserKillTimer(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserMapVirtualKeyEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserMenuItemFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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

DWORD
STDCALL
NtUserMinMaximize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

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

DWORD
STDCALL
NtUserMoveWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

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

DWORD
STDCALL
NtUserOpenClipboard(
  DWORD Unknown0,
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
  LPMSG lpMsg,
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

DWORD
STDCALL
NtUserQueryWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserRealChildWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserRedrawWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

RTL_ATOM
STDCALL
NtUserRegisterClassExWOW(
  LPWNDCLASSEX lpwcx,
  BOOL bUnicodeClass,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserRegisterHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserRegisterTasklist(
  DWORD Unknown0);

UINT STDCALL
NtUserRegisterWindowMessage(LPCWSTR MessageName);

DWORD
STDCALL
NtUserRemoveMenu(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

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
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
STDCALL
NtUserScrollWindowEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7);

DWORD
STDCALL
NtUserSendInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

LRESULT STDCALL
NtUserSendMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam);

BOOL
STDCALL
NtUserSendMessageCallback(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData);

BOOL
STDCALL
NtUserSendNotifyMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

DWORD
STDCALL
NtUserSetActiveWindow(
  DWORD Unknown0);

DWORD
STDCALL
NtUserSetCapture(
  DWORD Unknown0);

DWORD
STDCALL
NtUserSetClassLong(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetClassWord(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserSetClipboardData(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserSetClipboardViewer(
  DWORD Unknown0);

DWORD
STDCALL
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetCursor(
  DWORD Unknown0);

DWORD
STDCALL
NtUserSetCursorContents(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetCursorIconData(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetFocus(
  DWORD Unknown0);

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
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserSetMenu(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserSetMenuContextHelpId(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetMenuDefaultItem(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserSetMenuFlagRtoL(
  DWORD Unknown0);

BOOL
STDCALL
NtUserSetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength);

DWORD
STDCALL
NtUserSetParent(
  DWORD Unknown0,
  DWORD Unknown1);

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
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetShellWindowEx(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetSysColors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetSystemCursor(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetSystemMenu(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetSystemTimer(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
STDCALL
NtUserSetThreadDesktop(
  HDESK hDesktop);

DWORD
STDCALL
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetTimer(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetWindowFNID(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetWindowLong(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetWindowPlacement(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
STDCALL
NtUserSetWindowRgn(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserSetWindowsHookAW(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserSetWindowsHookEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserSetWindowStationUser(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetWindowWord(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

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

DWORD
STDCALL
NtUserShowCaret(
  DWORD Unknown0);

DWORD
STDCALL
NtUserShowScrollBar(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

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
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserThunkedMenuInfo(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserThunkedMenuItemInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserToUnicodeEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
STDCALL
NtUserTrackMouseEvent(
  DWORD Unknown0);

DWORD
STDCALL
NtUserTrackPopupMenuEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserTranslateAccelerator(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

BOOL
STDCALL
NtUserTranslateMessage(
  LPMSG lpMsg,
  DWORD Unknown1);

DWORD
STDCALL
NtUserUnhookWindowsHookEx(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserUnregisterClass(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserUnregisterHotKey(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserValidateRect(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserYieldTask(VOID);

#endif /* __WIN32K_NTUSER_H */

/* EOF */
