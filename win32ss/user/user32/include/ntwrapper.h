#pragma once

#if defined(__GNUC__)
#define EXTINLINE extern inline __attribute__((always_inline)) __attribute__((gnu_inline))
#elif defined(_MSC_VER)
#define EXTINLINE extern __forceinline
#else
#error
#endif

BOOL FASTCALL TestState(PWND, UINT);

#if 0
EXTINLINE BOOL WINAPI
GetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
    return NtUserGetScrollBarInfo(hWnd, idObject, psbi);
}
#endif

EXTINLINE BOOL WINAPI
ShowScrollBar(HWND hWnd, INT wBar, BOOL bShow)
{
    return NtUserShowScrollBar(hWnd, wBar, bShow);
}

EXTINLINE BOOL WINAPI
CloseDesktop(HDESK hDesktop)
{
    return NtUserCloseDesktop(hDesktop);
}

EXTINLINE HDESK WINAPI
OpenInputDesktop(DWORD dwFlags, BOOL fInherit, ACCESS_MASK dwDesiredAccess)
{
    return NtUserOpenInputDesktop(dwFlags, fInherit, dwDesiredAccess);
}

EXTINLINE BOOL WINAPI
PaintDesktop(HDC hdc)
{
    return NtUserPaintDesktop(hdc);
}

EXTINLINE BOOL WINAPI
SetThreadDesktop(HDESK hDesktop)
{
    return NtUserSetThreadDesktop(hDesktop);
}

EXTINLINE BOOL WINAPI
SwitchDesktop(HDESK hDesktop)
{
    return NtUserSwitchDesktop(hDesktop);
}

EXTINLINE BOOL WINAPI
SetShellWindowEx(HWND hwndShell, HWND hwndShellListView)
{
    return NtUserSetShellWindowEx(hwndShell, hwndShellListView);
}

EXTINLINE DWORD WINAPI
GetGuiResources(HANDLE hProcess, DWORD uiFlags)
{
    return NtUserGetGuiResources(hProcess, uiFlags);
}

EXTINLINE BOOL WINAPI
GetUserObjectInformationW(HANDLE hObj, int nIndex, PVOID pvInfo, DWORD nLength, LPDWORD lpnLengthNeeded)
{
    return NtUserGetObjectInformation(hObj, nIndex, pvInfo, nLength, lpnLengthNeeded);
}

EXTINLINE BOOL WINAPI
LockWindowUpdate(HWND hWndLock)
{
    return NtUserLockWindowUpdate(hWndLock);
}

EXTINLINE BOOL WINAPI
LockWorkStation(VOID)
{
    return NtUserLockWorkStation();
}

EXTINLINE DWORD WINAPI
RegisterTasklist(DWORD x)
{
    return NtUserRegisterTasklist(x);
}

EXTINLINE DWORD WINAPI
DragObject(HWND hwnd1, HWND hwnd2, UINT u1, ULONG_PTR dw1, HCURSOR hc1)
{
    return NtUserDragObject(hwnd1, hwnd2, u1, dw1, hc1);
}

EXTINLINE BOOL WINAPI
KillTimer(HWND hWnd, UINT_PTR IDEvent)
{
    return NtUserKillTimer(hWnd, IDEvent);
}

EXTINLINE UINT_PTR WINAPI
SetSystemTimer(HWND hWnd, UINT_PTR IDEvent, UINT Period, TIMERPROC TimerFunc)
{
    return NtUserSetSystemTimer(hWnd, IDEvent, Period, TimerFunc);
}

EXTINLINE UINT_PTR WINAPI
SetTimer(HWND hWnd, UINT_PTR IDEvent, UINT Period, TIMERPROC TimerFunc)
{
    return NtUserSetTimer(hWnd, IDEvent, Period, TimerFunc);
}

EXTINLINE BOOL WINAPI
CloseWindowStation(HWINSTA hWinSta)
{
    return NtUserCloseWindowStation(hWinSta);
}

EXTINLINE HWINSTA WINAPI
GetProcessWindowStation(VOID)
{
    return NtUserGetProcessWindowStation();
}

EXTINLINE BOOL WINAPI
SetProcessWindowStation(HWINSTA hWinSta)
{
    return NtUserSetProcessWindowStation(hWinSta);
}

EXTINLINE BOOL WINAPI
LockWindowStation(HWINSTA hWinSta)
{
    return NtUserLockWindowStation(hWinSta);
}

EXTINLINE BOOL WINAPI
UnlockWindowStation(HWINSTA hWinSta)
{
    return NtUserUnlockWindowStation(hWinSta);
}

EXTINLINE int WINAPI
CopyAcceleratorTableW(HACCEL hAccelSrc, LPACCEL lpAccelDst, int cAccelEntries)
{
    return NtUserCopyAcceleratorTable(hAccelSrc, lpAccelDst, cAccelEntries);
}

EXTINLINE HACCEL WINAPI
CreateAcceleratorTableW(LPACCEL lpaccl, int cEntries)
{
    return NtUserCreateAcceleratorTable(lpaccl, cEntries);
}

EXTINLINE BOOL WINAPI
CreateCaret(HWND hWnd, HBITMAP hBitmap, int nWidth, int nHeight)
{
    return NtUserCreateCaret(hWnd, hBitmap, nWidth, nHeight);
}

EXTINLINE UINT WINAPI
GetCaretBlinkTime(VOID)
{
    return NtUserGetCaretBlinkTime();
}

EXTINLINE BOOL WINAPI
GetCaretPos(LPPOINT lpPoint)
{
    return NtUserGetCaretPos(lpPoint);
}

EXTINLINE BOOL WINAPI
CloseClipboard(VOID)
{
    return NtUserCloseClipboard();
}

EXTINLINE INT WINAPI
CountClipboardFormats(VOID)
{
    return NtUserCountClipboardFormats();
}

EXTINLINE BOOL WINAPI
EmptyClipboard(VOID)
{
    return NtUserEmptyClipboard();
}

EXTINLINE HWND WINAPI
GetClipboardOwner(VOID)
{
    return NtUserGetClipboardOwner();
}

EXTINLINE DWORD WINAPI
GetClipboardSequenceNumber(VOID)
{
    return NtUserGetClipboardSequenceNumber();
}

EXTINLINE HWND WINAPI
GetClipboardViewer(VOID)
{
    return NtUserGetClipboardViewer();
}

EXTINLINE HWND WINAPI
GetOpenClipboardWindow(VOID)
{
    return NtUserGetOpenClipboardWindow();
}

EXTINLINE INT WINAPI
GetPriorityClipboardFormat(UINT *paFormatPriorityList, INT cFormats)
{
    return NtUserGetPriorityClipboardFormat(paFormatPriorityList, cFormats);
}

EXTINLINE BOOL WINAPI
IsClipboardFormatAvailable(UINT format)
{
    return NtUserIsClipboardFormatAvailable(format);
}

EXTINLINE HWND WINAPI
SetClipboardViewer(HWND hWndNewViewer)
{
    return NtUserSetClipboardViewer(hWndNewViewer);
}

EXTINLINE BOOL WINAPI
ChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext)
{
    return NtUserChangeClipboardChain(hWndRemove, hWndNewNext);
}

EXTINLINE BOOL WINAPI
GetClipCursor(LPRECT lpRect)
{
    return NtUserGetClipCursor(lpRect);
}

EXTINLINE HBRUSH WINAPI GetControlBrush(HWND hwnd, HDC  hdc, UINT ctlType)
{
    return NtUserGetControlBrush(hwnd, hdc, ctlType);
}

EXTINLINE HBRUSH WINAPI GetControlColor(HWND hwndParent, HWND hwnd, HDC hdc, UINT CtlMsg)
{
    return NtUserGetControlColor(hwndParent, hwnd, hdc, CtlMsg);
}

EXTINLINE BOOL WINAPI
GetCursorInfo(PCURSORINFO pci)
{
    return NtUserGetCursorInfo(pci);
}

EXTINLINE BOOL WINAPI
ClipCursor(CONST RECT *lpRect)
{
    return NtUserClipCursor((RECT *)lpRect);
}

EXTINLINE HCURSOR WINAPI
SetCursor(HCURSOR hCursor)
{
    return NtUserSetCursor(hCursor);
}

EXTINLINE HDC WINAPI
GetDC(HWND hWnd)
{
    return NtUserGetDC(hWnd);
}

EXTINLINE HDC WINAPI
GetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags)
{
    return NtUserGetDCEx(hWnd, hrgnClip, flags);
}

EXTINLINE HDC WINAPI
GetWindowDC(HWND hWnd)
{
    return NtUserGetWindowDC(hWnd);
}

EXTINLINE BOOL WINAPI
FlashWindowEx(PFLASHWINFO pfwi)
{
    return NtUserFlashWindowEx(pfwi);
}

EXTINLINE BOOL WINAPI
DrawAnimatedRects(HWND hWnd, int idAni, CONST RECT *lprcFrom, CONST RECT *lprcTo)
{
    return NtUserDrawAnimatedRects(hWnd, idAni, (RECT*)lprcFrom, (RECT*)lprcTo);
}

EXTINLINE BOOL WINAPI
BlockInput(BOOL fBlockIt)
{
    return NtUserBlockInput(fBlockIt);
}

EXTINLINE UINT WINAPI
GetDoubleClickTime(VOID)
{
    return NtUserGetDoubleClickTime();
}

EXTINLINE BOOL WINAPI
GetKeyboardState(PBYTE lpKeyState)
{
    return NtUserGetKeyboardState((LPBYTE)lpKeyState);
}

EXTINLINE BOOL WINAPI
RegisterHotKey(HWND hWnd, int id, UINT fsModifiers, UINT vk)
{
    return NtUserRegisterHotKey(hWnd, id, fsModifiers, vk);
}

EXTINLINE HWND WINAPI
SetFocus(HWND hWnd)
{
    return NtUserSetFocus(hWnd);
}

EXTINLINE BOOL WINAPI
SetKeyboardState(LPBYTE lpKeyState)
{
    return NtUserSetKeyboardState((LPBYTE)lpKeyState);
}

EXTINLINE UINT WINAPI
SendInput(UINT nInputs, LPINPUT pInputs, int cbSize)
{
    return NtUserSendInput(nInputs, pInputs, cbSize);
}

EXTINLINE BOOL WINAPI
WaitMessage(VOID)
{
    return NtUserWaitMessage();
}

EXTINLINE HDC WINAPI
BeginPaint(HWND hwnd, LPPAINTSTRUCT lpPaint)
{
    return NtUserBeginPaint(hwnd, lpPaint);
}

EXTINLINE BOOL WINAPI
EndPaint(HWND hWnd, CONST PAINTSTRUCT *lpPaint)
{
    return NtUserEndPaint(hWnd, lpPaint);
}

EXTINLINE int WINAPI
ExcludeUpdateRgn(HDC hDC, HWND hWnd)
{
    return NtUserExcludeUpdateRgn(hDC, hWnd);
}

EXTINLINE BOOL WINAPI
InvalidateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
    return NtUserInvalidateRgn(hWnd, hRgn, bErase);
}

EXTINLINE BOOL WINAPI
RedrawWindow(HWND hWnd, CONST RECT *lprcUpdate, HRGN hrgnUpdate, UINT flags)
{
    return NtUserRedrawWindow(hWnd, lprcUpdate, hrgnUpdate, flags);
}

EXTINLINE BOOL WINAPI
DestroyWindow(HWND hWnd)
{
    return NtUserDestroyWindow(hWnd);
}

EXTINLINE HWND WINAPI
GetForegroundWindow(VOID)
{
    return NtUserGetForegroundWindow();
}

EXTINLINE BOOL WINAPI
GetGUIThreadInfo(DWORD idThread, LPGUITHREADINFO lpgui)
{
    return NtUserGetGUIThreadInfo(idThread, lpgui);
}

EXTINLINE BOOL WINAPI
GetTitleBarInfo(HWND hwnd, PTITLEBARINFO pti)
{
    return NtUserGetTitleBarInfo(hwnd, pti);
}

EXTINLINE BOOL WINAPI
GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
    return NtUserGetWindowPlacement(hWnd, lpwndpl);
}

EXTINLINE BOOL WINAPI
MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
    return NtUserMoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}

EXTINLINE HWND WINAPI
SetParent(HWND hWndChild, HWND hWndNewParent)
{
    return NtUserSetParent(hWndChild, hWndNewParent);
}

EXTINLINE BOOL WINAPI
SetWindowPlacement(HWND hWnd, CONST WINDOWPLACEMENT *lpwndpl)
{
    return NtUserSetWindowPlacement(hWnd, (WINDOWPLACEMENT *)lpwndpl);
}

EXTINLINE BOOL WINAPI
SetWindowPos(HWND hWnd, HWND hWndAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    return NtUserSetWindowPos(hWnd,hWndAfter, X, Y, cx, cy, uFlags);
}

EXTINLINE BOOL WINAPI
ShowWindow(HWND hWnd, int nCmdShow)
{
    return NtUserShowWindow(hWnd, nCmdShow);
}

EXTINLINE BOOL WINAPI
ShowWindowAsync(HWND hWnd, int nCmdShow)
{
    return NtUserShowWindowAsync(hWnd, nCmdShow);
}

EXTINLINE HWND WINAPI
SetActiveWindow(HWND hWnd)
{
    return NtUserSetActiveWindow(hWnd);
}

EXTINLINE DWORD WINAPI
GetListBoxInfo(HWND hwnd)
{
    return NtUserGetListBoxInfo(hwnd); // Do it right! Have the message org from kmode!
}

EXTINLINE BOOL WINAPI
DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
    return NtUserDeleteMenu(hMenu, uPosition, uFlags);
}

EXTINLINE BOOL WINAPI
DestroyMenu(HMENU hMenu)
{
    return NtUserDestroyMenu(hMenu);
}

/*EXTINLINE BOOL WINAPI
GetMenuBarInfo(HWND hwnd, LONG idObject, LONG idItem, PMENUBARINFO pmbi)
{
    return NtUserGetMenuBarInfo(hwnd, idObject, idItem, pmbi);
}
*/
EXTINLINE BOOL WINAPI
GetMenuItemRect(HWND hWnd, HMENU hMenu, UINT uItem, LPRECT lprcItem)
{
    return NtUserGetMenuItemRect(hWnd, hMenu, uItem, lprcItem);
}

EXTINLINE BOOL WINAPI
RemoveMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
    return NtUserRemoveMenu(hMenu, uPosition, uFlags);
}

EXTINLINE BOOL WINAPI
SetMenuDefaultItem(HMENU hMenu, UINT uItem, UINT fByPos)
{
  return NtUserSetMenuDefaultItem(hMenu, uItem, fByPos);
}

EXTINLINE BOOL WINAPI
SetMenuContextHelpId(HMENU hmenu, DWORD dwContextHelpId)
{
    return NtUserSetMenuContextHelpId(hmenu, dwContextHelpId);
}

EXTINLINE HWND WINAPI
SetCapture(HWND hWnd)
{
    return NtUserSetCapture(hWnd);
}

EXTINLINE BOOL WINAPI
InvalidateRect(HWND hWnd, CONST RECT* lpRect, BOOL bErase)
{
    return NtUserInvalidateRect(hWnd, lpRect, bErase);
}

EXTINLINE BOOL WINAPI ValidateRect(HWND hWnd, CONST RECT *lpRect)
{
    return NtUserValidateRect(hWnd, lpRect);
}

EXTINLINE BOOL WINAPI ShowCaret(HWND hWnd)
{
    return NtUserShowCaret(hWnd);
}

EXTINLINE BOOL WINAPI HideCaret(HWND hWnd)
{
    return NtUserHideCaret(hWnd);
}




/*
    Inline functions that make calling NtUserCall*** functions readable
    These functions are prepended with NtUserx because they are not
    real syscalls and they are inlined
*/

EXTINLINE BOOL NtUserxDestroyCaret(VOID)
{
    return (BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_DESTROY_CARET);
}

EXTINLINE VOID NtUserxMsqClearWakeMask(VOID)
{
    NtUserCallNoParam(NOPARAM_ROUTINE_MSQCLEARWAKEMASK);
}

EXTINLINE HMENU NtUserxCreateMenu(VOID)
{
    return (HMENU)NtUserCallNoParam(NOPARAM_ROUTINE_CREATEMENU);
}

EXTINLINE HMENU NtUserxCreatePopupMenu(VOID)
{
    return (HMENU)NtUserCallNoParam(NOPARAM_ROUTINE_CREATEMENUPOPUP);
}

EXTINLINE DWORD NtUserxGetMessagePos(VOID)
{
    return (DWORD)NtUserCallNoParam(NOPARAM_ROUTINE_GETMSESSAGEPOS);
}

EXTINLINE BOOL NtUserxReleaseCapture(VOID)
{
    return (BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_RELEASECAPTURE);
}

EXTINLINE BOOL NtUserxInitMessagePump(VOID)
{
    return (BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_INIT_MESSAGE_PUMP);
}

EXTINLINE BOOL NtUserxUnInitMessagePump(VOID)
{
    return NtUserCallNoParam(NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP);
}

EXTINLINE HANDLE NtUserxMsqSetWakeMask(DWORD_PTR dwWaitMask)
{
    return (HANDLE)NtUserCallOneParam(dwWaitMask, ONEPARAM_ROUTINE_GETINPUTEVENT);
}

EXTINLINE BOOL NtUserxSetCaretBlinkTime(UINT uMSeconds)
{
    return (BOOL)NtUserCallOneParam(uMSeconds, ONEPARAM_ROUTINE_SETCARETBLINKTIME);
}

EXTINLINE HWND NtUserxWindowFromDC(HDC hDC)
{
    return (HWND)NtUserCallOneParam((DWORD_PTR)hDC, ONEPARAM_ROUTINE_WINDOWFROMDC);
}

EXTINLINE BOOL NtUserxSwapMouseButton(BOOL fSwap)
{
    return (BOOL)NtUserCallOneParam((DWORD_PTR)fSwap, ONEPARAM_ROUTINE_SWAPMOUSEBUTTON);
}

EXTINLINE LPARAM NtUserxSetMessageExtraInfo(LPARAM lParam)
{
    return (LPARAM)NtUserCallOneParam((DWORD_PTR)lParam, ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO);
}

EXTINLINE INT NtUserxShowCursor(BOOL bShow)
{
    return (INT)NtUserCallOneParam((DWORD_PTR)bShow, ONEPARAM_ROUTINE_SHOWCURSOR);
}

EXTINLINE UINT NtUserxEnumClipboardFormats(UINT format)
{
    return (UINT)NtUserCallOneParam((DWORD_PTR)format, ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS);
}

EXTINLINE HICON NtUserxCreateEmptyCurObject(DWORD_PTR Param)
{
    return (HICON)NtUserCallOneParam(Param, ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT);
}

EXTINLINE BOOL NtUserxMessageBeep(UINT uType)
{
    return (BOOL)NtUserCallOneParam(uType, ONEPARAM_ROUTINE_MESSAGEBEEP);
}

EXTINLINE HKL NtUserxGetKeyboardLayout(DWORD idThread)
{
    return (HKL)NtUserCallOneParam((DWORD_PTR)idThread,  ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT);
}

EXTINLINE INT NtUserxGetKeyboardType(INT nTypeFlag)
{
    return (INT)NtUserCallOneParam((DWORD_PTR)nTypeFlag,  ONEPARAM_ROUTINE_GETKEYBOARDTYPE);
}

EXTINLINE INT NtUserxReleaseDC(HDC hDC)
{
    return (INT)NtUserCallOneParam((DWORD_PTR)hDC, ONEPARAM_ROUTINE_RELEASEDC);
}

EXTINLINE UINT NtUserxRealizePalette(HDC hDC)
{
    return (UINT)NtUserCallOneParam((DWORD_PTR)hDC, ONEPARAM_ROUTINE_REALIZEPALETTE);
}

EXTINLINE VOID NtUserxCreateSystemThreads(BOOL bRemoteProcess)
{
    NtUserCallOneParam(bRemoteProcess, ONEPARAM_ROUTINE_CREATESYSTEMTHREADS);
}

EXTINLINE HDWP NtUserxBeginDeferWindowPos(INT nNumWindows)
{
    return (HDWP)NtUserCallOneParam((DWORD_PTR)nNumWindows, ONEPARAM_ROUTINE_BEGINDEFERWNDPOS);
}

EXTINLINE BOOL NtUserxReplyMessage(LRESULT lResult)
{
    return NtUserCallOneParam(lResult, ONEPARAM_ROUTINE_REPLYMESSAGE);
}

EXTINLINE VOID NtUserxPostQuitMessage(int nExitCode)
{
    NtUserCallOneParam(nExitCode, ONEPARAM_ROUTINE_POSTQUITMESSAGE);
}

EXTINLINE DWORD NtUserxGetQueueStatus(UINT flags)
{
    return (DWORD)NtUserCallOneParam(flags, ONEPARAM_ROUTINE_GETQUEUESTATUS);
}

EXTINLINE BOOL NtUserxValidateRgn(HWND hWnd, HRGN hRgn)
{
    return (BOOL)NtUserCallHwndParamLock(hWnd, (DWORD_PTR)hRgn, TWOPARAM_ROUTINE_VALIDATERGN);
}

EXTINLINE BOOL NtUserxSetCursorPos(INT x, INT y)
{
    return (BOOL)NtUserCallTwoParam((DWORD)x, (DWORD)y, TWOPARAM_ROUTINE_SETCURSORPOS);
}

EXTINLINE BOOL NtUserxEnableWindow(HWND hWnd, BOOL bEnable)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)hWnd, (DWORD_PTR)bEnable, TWOPARAM_ROUTINE_ENABLEWINDOW);
}

EXTINLINE BOOL NtUserxUpdateUiState(HWND hWnd, DWORD Param)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)hWnd, (DWORD_PTR)Param, TWOPARAM_ROUTINE_ROS_UPDATEUISTATE);
}

EXTINLINE VOID NtUserxSwitchToThisWindow(HWND hWnd, BOOL fAltTab)
{
    NtUserCallTwoParam((DWORD_PTR)hWnd, (DWORD_PTR)fAltTab, TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW);
}

EXTINLINE BOOL NtUserxShowOwnedPopups(HWND hWnd, BOOL fShow)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)hWnd, fShow, TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS);
}

EXTINLINE BOOL NtUserxUnhookWindowsHook(int nCode, HOOKPROC pfnFilterProc)
{
    return (BOOL)NtUserCallTwoParam(nCode, (DWORD_PTR)pfnFilterProc, TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK);
}

EXTINLINE BOOL NtUserxSetWindowContextHelpId(HWND hWnd, DWORD_PTR dwContextHelpId)
{
    return (BOOL)NtUserCallHwndParam(hWnd, dwContextHelpId, HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID);
}

EXTINLINE BOOL NtUserxKillSystemTimer(HWND hWnd, UINT_PTR IDEvent)
{
    return (BOOL)NtUserCallHwndParam(hWnd, IDEvent, HWNDPARAM_ROUTINE_KILLSYSTEMTIMER);
}

EXTINLINE VOID NtUserxSetDialogPointer(HWND hWnd, PVOID dlgInfo)
{
    NtUserCallHwndParam(hWnd, (DWORD_PTR)dlgInfo, HWNDPARAM_ROUTINE_SETDIALOGPOINTER);
}

EXTINLINE VOID NtUserxNotifyWinEvent(HWND hWnd, PVOID ne)
{
    NtUserCallHwndParam(hWnd, (DWORD_PTR)ne, HWNDPARAM_ROUTINE_ROS_NOTIFYWINEVENT);
}

EXTINLINE DWORD NtUserxGetWindowContextHelpId(HWND hwnd)
{
    return (DWORD)NtUserCallHwnd(hwnd, HWND_ROUTINE_GETWNDCONTEXTHLPID);
}

EXTINLINE BOOL NtUserxDeregisterShellHookWindow(HWND hWnd)
{
    return (BOOL)NtUserCallHwnd(hWnd, HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW);
}

EXTINLINE BOOL NtUserxRegisterShellHookWindow(HWND hWnd)
{
    return (BOOL)NtUserCallHwnd(hWnd, HWND_ROUTINE_REGISTERSHELLHOOKWINDOW);
}

EXTINLINE BOOL NtUserxSetMessageBox(HWND hWnd)
{
    return (BOOL)NtUserCallHwnd(hWnd, HWND_ROUTINE_SETMSGBOX);
}

EXTINLINE VOID NtUserxClearWindowState(PWND pWnd, UINT Flag)
{
    if (!TestState(pWnd, Flag)) return;
    NtUserCallHwndParam(UserHMGetHandle(pWnd), (DWORD_PTR)Flag, HWNDPARAM_ROUTINE_CLEARWINDOWSTATE);
}

EXTINLINE VOID NtUserxSetWindowState(PWND pWnd, UINT Flag)
{
    if (TestState(pWnd, Flag)) return;
    NtUserCallHwndParam(UserHMGetHandle(pWnd), (DWORD_PTR)Flag, HWNDPARAM_ROUTINE_SETWINDOWSTATE);
}

EXTINLINE HWND NtUserxSetTaskmanWindow(HWND hWnd)
{
    return NtUserCallHwndOpt(hWnd, HWNDOPT_ROUTINE_SETTASKMANWINDOW);
}

EXTINLINE HWND NtUserxSetProgmanWindow(HWND hWnd)
{
    return NtUserCallHwndOpt(hWnd, HWNDOPT_ROUTINE_SETPROGMANWINDOW);
}

EXTINLINE UINT NtUserxArrangeIconicWindows(HWND hWnd)
{
    return (UINT)NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS);
}

EXTINLINE BOOL NtUserxUpdateWindow(HWND hWnd)
{
    return NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_UPDATEWINDOW);
}

EXTINLINE BOOL NtUserxDrawMenuBar(HWND hWnd)
{
    return (BOOL)NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_DRAWMENUBAR);
}

EXTINLINE BOOL NtUserxMDIRedrawFrame(HWND hWnd)
{
  return (BOOL)NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_REDRAWFRAME);
}

EXTINLINE BOOL NtUserxSetForegroundWindow(HWND hWnd)
{
    return NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW);
}


/* ReactOS-specific definitions */

EXTINLINE LPARAM NtUserxGetMessageExtraInfo(VOID)
{
    return (LPARAM)NtUserGetThreadState(THREADSTATE_GETMESSAGEEXTRAINFO);
}

EXTINLINE VOID NtUserxEnableProcessWindowGhosting(BOOL bEnable)
{
    NtUserCallOneParam((DWORD_PTR)bEnable, ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING);
}

EXTINLINE PVOID NtUserxGetDesktopMapping(PVOID ptr)
{
    return (PVOID)NtUserCallOneParam((DWORD_PTR)ptr, ONEPARAM_ROUTINE_GETDESKTOPMAPPING);
}

EXTINLINE BOOL NtUserxGetCursorPos(POINT* lpPoint)
{
    return (BOOL)NtUserCallOneParam((DWORD_PTR)lpPoint, ONEPARAM_ROUTINE_GETCURSORPOS);
}

EXTINLINE BOOL NtUserxSetMenuBarHeight(HMENU menu, INT height)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)menu, (DWORD_PTR)height, TWOPARAM_ROUTINE_SETMENUBARHEIGHT);
}

EXTINLINE BOOL NtUserxSetGUIThreadHandle(DWORD_PTR field, HWND hwnd)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)field, (DWORD_PTR)hwnd, TWOPARAM_ROUTINE_SETGUITHRDHANDLE);
}

EXTINLINE BOOL NtUserxSetCaretPos(INT x, INT y)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)x, (DWORD_PTR)y, TWOPARAM_ROUTINE_SETCARETPOS);
}

EXTINLINE BOOL NtUserxRegisterLogonProcess(DWORD dwProcessId, BOOL bRegister)
{
    return (BOOL)NtUserCallTwoParam((DWORD_PTR)dwProcessId, (DWORD_PTR)bRegister, TWOPARAM_ROUTINE_REGISTERLOGONPROCESS);
}

EXTINLINE BOOL NtUserxAllowSetForegroundWindow(DWORD dwProcessId)
{
    return (BOOL)NtUserCallOneParam((DWORD_PTR)dwProcessId, ONEPARAM_ROUTINE_ALLOWSETFOREGND);
}

EXTINLINE BOOL NtUserxLockSetForegroundWindow(UINT uLockCode)
{
    return (BOOL)NtUserCallOneParam((DWORD_PTR)uLockCode, ONEPARAM_ROUTINE_LOCKFOREGNDWINDOW);
}
