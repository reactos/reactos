#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

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

DWORD
STDCALL
NtUserBeginPaint(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserChildWindowFromPointEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserClipCursor(
  DWORD Unknown0);

DWORD
STDCALL
NtUserCloseClipboard(VOID);

DWORD
STDCALL
NtUserCloseDesktop(
  DWORD Unknown0);

DWORD
STDCALL
NtUserCloseWindowStation(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserCreateDesktop(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserCreateWindowEx(
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
  DWORD Unknown11,
  DWORD Unknown12);

DWORD
STDCALL
NtUserCreateWindowStation(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
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

DWORD
STDCALL
NtUserDeferWindowPos(
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

DWORD
STDCALL
NtUserDestroyWindow(
  DWORD Unknown0);

DWORD
STDCALL
NtUserDispatchMessage(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserEndPaint(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserFindWindowEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
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

DWORD
STDCALL
NtUserGetAncestor(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserGetClassInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

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

DWORD
STDCALL
NtUserGetDC(
  DWORD Unknown0);

DWORD
STDCALL
NtUserGetDCEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

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

DWORD
STDCALL
NtUserGetMessage(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserGetMouseMovePointsEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserGetObjectInformation(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserGetOpenClipboardWindow(VOID);

DWORD
STDCALL
NtUserGetPriorityClipboardFormat(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
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

DWORD
STDCALL
NtUserGetThreadDesktop(
  DWORD Unknown0,
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

DWORD
STDCALL
NtUserLockWindowStation(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserOpenDesktop(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserOpenInputDesktop(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserOpenWindowStation(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserPaintDesktop(
  DWORD Unknown0);

DWORD
STDCALL
NtUserPeekMessage(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
STDCALL
NtUserPostMessage(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserPostThreadMessage(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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

DWORD
STDCALL
NtUserRegisterWindowMessage(
  DWORD Unknown0);

DWORD
STDCALL
NtUserRemoveMenu(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
STDCALL
NtUserRemoveProp(
  DWORD Unknown0,
  DWORD Unknown1);

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

DWORD
STDCALL
NtUserSendMessageCallback(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

DWORD
STDCALL
NtUserSendNotifyMessage(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

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

DWORD
STDCALL
NtUserSetObjectInformation(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
STDCALL
NtUserSetParent(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSetProp(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

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

DWORD
STDCALL
NtUserSetThreadDesktop(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserShowWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserShowWindowAsync(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
STDCALL
NtUserSwitchDesktop(
  DWORD Unknown0);

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

DWORD
STDCALL
NtUserTranslateMessage(
  DWORD Unknown0,
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

DWORD
STDCALL
NtUserUnlockWindowStation(
  DWORD Unknown0);

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

DWORD
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
