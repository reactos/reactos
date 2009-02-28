#define EXTINLINE extern inline __attribute__((always_inline)) 

EXTINLINE BOOL WINAPI
EnableScrollBar(HWND hWnd, UINT wSBflags, UINT wArrows)
{
    return NtUserEnableScrollBar(hWnd, wSBflags, wArrows);
}

EXTINLINE BOOL WINAPI
GetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
    return NtUserGetScrollBarInfo(hWnd, idObject, psbi);
}

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
DragObject(HWND hwnd1, HWND hwnd2, UINT u1, DWORD dw1, HCURSOR hc1)
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
    return NtUserGetKeyboardState((LPBYTE) lpKeyState);
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

EXTINLINE BOOL WINAPI
GetMenuBarInfo(HWND hwnd, LONG idObject, LONG idItem, PMENUBARINFO pmbi)
{
    return NtUserGetMenuBarInfo(hwnd, idObject, idItem, pmbi);
}

EXTINLINE BOOL WINAPI
GetMenuItemRect(HWND hWnd, HMENU hMenu, UINT uItem, LPRECT lprcItem)
{
    return NtUserGetMenuItemRect(hWnd, hMenu, uItem, lprcItem);
}

EXTINLINE BOOL WINAPI
HiliteMenuItem(HWND hwnd, HMENU hmenu, UINT uItemHilite, UINT uHilite)
{
    return NtUserHiliteMenuItem(hwnd, hmenu, uItemHilite, uHilite);
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
    return(NtUserSetCapture(hWnd));
}

EXTINLINE BOOL WINAPI
InvalidateRect(HWND hWnd, CONST RECT* lpRect, BOOL bErase)
{
    return NtUserInvalidateRect(hWnd, lpRect, bErase);
}
