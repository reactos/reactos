/* $Id: stubs.c,v 1.45.12.3 2004/08/31 11:38:56 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native User stubs
 * FILE:             subsys/win32k/ntuser/stubs.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       04-06-2001  CSH  Created
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>

#ifdef UNIMPLEMENTED
#undef UNIMPLEMENTED
#define UNIMPLEMENTED DbgPrint("%s() in %s:%i UNIMPLEMENTED!\n", __FUNCTION__, __FILE__, __LINE__);
#endif

/*
 * Cursors and Icons
 */

DWORD
STDCALL
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
STDCALL
NtUserClipCursor(
  RECT *UnsafeRect)
{
   UNIMPLEMENTED;
   return TRUE;
}

BOOL
STDCALL
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL
STDCALL
NtUserSetCursorIconContents(HANDLE Handle,
                            PICONINFO IconInfo)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Accelerators
 */

int
STDCALL
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  int EntriesCount)
{
   UNIMPLEMENTED;
   return 0;
}

HACCEL
STDCALL
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount)
{
   UNIMPLEMENTED;
   return 0;
}

BOOLEAN
STDCALL
NtUserDestroyAcceleratorTable(
  HACCEL Table)
{
   UNIMPLEMENTED;
   return FALSE;
}


int
STDCALL
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message)
{
   UNIMPLEMENTED;
   return 0;
} 

/*
 * Carets
 */

BOOL
STDCALL
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight)
{
  return FALSE;
}

UINT
STDCALL
NtUserGetCaretBlinkTime(VOID)
{
  return 0;
}

BOOL
STDCALL
NtUserGetCaretPos(
  LPPOINT lpPoint)
{
  return FALSE;
}

BOOL
STDCALL
NtUserHideCaret(
  HWND hWnd)
{
  return FALSE;
}

BOOL
STDCALL
NtUserShowCaret(
  HWND hWnd)
{
  return FALSE;
}

/*
 * Classes
 */

DWORD STDCALL
NtUserGetClassName (
  HWND hWnd,
  LPWSTR lpClassName,
  ULONG nMaxCount)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
		  DWORD Unknown1)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD STDCALL
NtUserSetClassWord(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
   UNIMPLEMENTED;
   return(0);
}

BOOL STDCALL
NtUserUnregisterClass(
   LPCWSTR ClassNameOrAtom,
	 HINSTANCE hInstance,
	 DWORD Unknown)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Clipboard
 */

BOOL STDCALL
NtUserOpenClipboard(HWND hWnd, DWORD Unknown1)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL STDCALL
NtUserCloseClipboard(VOID)
{
   UNIMPLEMENTED;
   return FALSE;
}

HWND STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
   UNIMPLEMENTED;
   return 0;
}

BOOL STDCALL
NtUserChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD STDCALL
NtUserCountClipboardFormats(VOID)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD STDCALL
NtUserEmptyClipboard(VOID)
{
   UNIMPLEMENTED;
   return TRUE;
}

HANDLE STDCALL
NtUserGetClipboardData(UINT uFormat, DWORD Unknown1)
{
   UNIMPLEMENTED;
   return 0;
}

INT STDCALL
NtUserGetClipboardFormatName(UINT format, PUNICODE_STRING FormatName,
   INT cchMaxCount)
{
   UNIMPLEMENTED;
   return 0;
}

HWND STDCALL
NtUserGetClipboardOwner(VOID)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD STDCALL
NtUserGetClipboardSequenceNumber(VOID)
{
   UNIMPLEMENTED;
   return 0;
}

HWND STDCALL
NtUserGetClipboardViewer(VOID)
{
   UNIMPLEMENTED;
   return 0;
}

INT STDCALL
NtUserGetPriorityClipboardFormat(UINT *paFormatPriorityList, INT cFormats)
{
   UNIMPLEMENTED;
   return 0;
}

BOOL STDCALL
NtUserIsClipboardFormatAvailable(UINT format)
{
   UNIMPLEMENTED;
   return FALSE;
}

HANDLE STDCALL
NtUserSetClipboardData(UINT uFormat, HANDLE hMem, DWORD Unknown2)
{
   UNIMPLEMENTED;
   return 0;
}

HWND STDCALL
NtUserSetClipboardViewer(HWND hWndNewViewer)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * Hooks
 */

LRESULT
STDCALL
NtUserCallNextHookEx(
  HHOOK Hook,
  int Code,
  WPARAM wParam,
  LPARAM lParam)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD
STDCALL
NtUserSetWindowsHookAW(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
   UNIMPLEMENTED;
   return 0;
}

HHOOK
STDCALL
NtUserSetWindowsHookEx(
  HINSTANCE Mod,
  PUNICODE_STRING UnsafeModuleName,
  DWORD ThreadId,
  int HookId,
  HOOKPROC HookProc,
  BOOL Ansi)
{
   UNIMPLEMENTED;
   return 0;
}

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
  DWORD Unknown7)
{
   UNIMPLEMENTED;
   return 0;
}

BOOL
STDCALL
NtUserUnhookWindowsHookEx(
  HHOOK Hook)
{
   UNIMPLEMENTED;
   return FALSE;
}

DWORD
STDCALL
NtUserUnhookWinEvent(
  DWORD Unknown0)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * Hot keys
 */

BOOL STDCALL
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserUnregisterHotKey(HWND hWnd,
		       int id)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Menus
 */

DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 VOID* Buffer,
 ULONG nBufSize,
 DWORD Reserved)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck)
{
   UNIMPLEMENTED;
   return 0;
}


HMENU STDCALL
NtUserCreateMenu(BOOL PopupMenu)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserDestroyMenu(
  HMENU hMenu)
{
   UNIMPLEMENTED;
   return FALSE;
}


UINT STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW UnsafeItemInfo)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserEndMenu(VOID)
{
   UNIMPLEMENTED;
   return 0;
}

UINT STDCALL
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi)
{
   UNIMPLEMENTED;
   return 0;
}


UINT STDCALL
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL
STDCALL
NtUserMenuInfo(
 HMENU Menu,
 PROSMENUINFO UnsafeMenuInfo,
 BOOL SetOrGet)
{
   UNIMPLEMENTED;
   return 0;
}


int STDCALL
NtUserMenuItemFromPoint(
  HWND Wnd,
  HMENU Menu,
  DWORD X,
  DWORD Y)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL
STDCALL
NtUserMenuItemInfo(
 HMENU Menu,
 UINT Item,
 BOOL ByPosition,
 PROSMENUITEMINFO UnsafeItemInfo,
 BOOL SetOrGet)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserSetMenuFlagRtoL(
  HMENU hMenu)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
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


BOOL STDCALL
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Messages
 */


DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED;
  return 0;
}


DWORD STDCALL
NtUserQuerySendMessage(DWORD Unknown0)
{
   UNIMPLEMENTED;
   return 0;
}


LRESULT STDCALL
NtUserSendMessageTimeout(HWND hWnd,
			 UINT Msg,
			 WPARAM wParam,
			 LPARAM lParam,
			 UINT uFlags,
			 UINT uTimeout,
			 ULONG_PTR *uResult,
	                 PNTUSERSENDMESSAGEINFO UnsafeInfo)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserSendMessageCallback(HWND hWnd,
			  UINT Msg,
			  WPARAM wParam,
			  LPARAM lParam,
			  SENDASYNCPROC lpCallBack,
			  ULONG_PTR dwData)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserSendNotifyMessage(HWND hWnd,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserGetQueueStatus(BOOL ClearChanges)
{
   UNIMPLEMENTED;
   return 0;
}


/*
 * Painting
 */


BOOL STDCALL
NtUserUpdateWindow(HWND hWnd)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserRedrawWindow(HWND hWnd, CONST RECT *lprcUpdate, HRGN hrgnUpdate,
   UINT flags)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Scroll
 */

DWORD STDCALL
NtUserScrollDC(HDC hDC, INT dx, INT dy, const RECT *lprcScroll,
   const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL
STDCALL
NtUserGetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL
STDCALL
NtUserGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL
STDCALL
NtUserEnableScrollBar(
  HWND hWnd, 
  UINT wSBflags, 
  UINT wArrows)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL
STDCALL
NtUserSetScrollBarInfo(
  HWND hwnd,
  LONG idObject,
  SETSCROLLBARINFO *info)
{
   UNIMPLEMENTED;
   return FALSE;
}

DWORD
STDCALL
NtUserSetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPCSCROLLINFO lpsi, 
  BOOL bRedraw)
{
   UNIMPLEMENTED;
   return 0;
}

DWORD STDCALL
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * Properties
 */

NTSTATUS STDCALL
NtUserBuildPropList(HWND hWnd,
		    LPVOID Buffer,
		    DWORD BufferSize,
		    DWORD *Count)
{
   UNIMPLEMENTED;
   return FALSE;
}

HANDLE STDCALL
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
   UNIMPLEMENTED;
   return FALSE;
}

HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL STDCALL
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Timers
 */

UINT_PTR
STDCALL
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
STDCALL
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
   UNIMPLEMENTED;
   return FALSE;
}


UINT_PTR
STDCALL
NtUserSetSystemTimer(
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
STDCALL
NtUserKillSystemTimer(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * WinPos
 */

BOOL
STDCALL
NtUserGetMinMaxInfo(
  HWND hwnd,
  MINMAXINFO *MinMaxInfo,
  BOOL SendMessage)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * Windows
 */

DWORD STDCALL
NtUserAlterWindowStyle(DWORD Unknown0,
		       DWORD Unknown1,
		       DWORD Unknown2)
{
   UNIMPLEMENTED;
   return 0;
}


ULONG
STDCALL
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG nBufSize)
{
   UNIMPLEMENTED;
   return 0;
}


HWND STDCALL
NtUserChildWindowFromPointEx(HWND hwndParent,
			     LONG x,
			     LONG y,
			     UINT uiFlags)
{
   UNIMPLEMENTED;
   return 0;
}


HDWP STDCALL
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     int x,
         int y,
         int cx,
         int cy,
		     UINT Flags)
{
   UNIMPLEMENTED;
   return NULL;
}

DWORD
STDCALL
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


DWORD STDCALL
NtUserEndDeferWindowPosEx(DWORD Unknown0,
			  DWORD Unknown1)
{
   UNIMPLEMENTED; 
   return 0;
}


DWORD STDCALL
NtUserFillWindow(DWORD Unknown0,
		 DWORD Unknown1,
		 DWORD Unknown2,
		 DWORD Unknown3)
{
   UNIMPLEMENTED;
   return 0;
}


HWND STDCALL
NtUserFindWindowEx(HWND hwndParent,
		   HWND hwndChildAfter,
		   PUNICODE_STRING ucClassName,
		   PUNICODE_STRING ucWindowName)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserFlashWindowEx(DWORD Unknown0)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserPaintDesktop(HDC hDC)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserGetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2)
{
   UNIMPLEMENTED;
   return 0;
}


HWND STDCALL
NtUserGetLastActivePopup(HWND hWnd)
{
   UNIMPLEMENTED;
   return 0;
}


HMENU STDCALL
NtUserGetSystemMenu(HWND hWnd, BOOL bRevert)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserSetSystemMenu(HWND hWnd, HMENU hMenu)
{
   UNIMPLEMENTED;
   return FALSE;
}


BOOL STDCALL
NtUserGetWindowPlacement(HWND hWnd,
			 WINDOWPLACEMENT *lpwndpl)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserLockWindowUpdate(DWORD Unknown0)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserMoveWindow(      
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint)
{
   UNIMPLEMENTED;
   return FALSE;
}


/*
	QueryWindow based on KJK::Hyperion and James Tabor.

	0 = QWUniqueProcessId
	1 = QWUniqueThreadId
	4 = QWIsHung            Implements IsHungAppWindow found
                                by KJK::Hyperion.

        9 = QWKillWindow        When I called this with hWnd ==
                                DesktopWindow, it shutdown the system
                                and rebooted.
*/
DWORD STDCALL
NtUserQueryWindow(HWND hWnd, DWORD Index)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserRealChildWindowFromPoint(DWORD Unknown0,
			       DWORD Unknown1,
			       DWORD Unknown2)
{
   UNIMPLEMENTED;
   return 0;
}


UINT STDCALL
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserSetImeOwnerWindow(DWORD Unknown0,
			DWORD Unknown1)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserSetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2,
			   DWORD Unknown3)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserSetLayeredWindowAttributes(DWORD Unknown0,
				 DWORD Unknown1,
				 DWORD Unknown2,
				 DWORD Unknown3)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserSetLogonNotifyWindow(DWORD Unknown0)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserSetMenu(
   HWND Wnd,
   HMENU Menu,
   BOOL Repaint)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserSetWindowFNID(
  DWORD Unknown0,
  DWORD Unknown1)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserSetWindowPlacement(HWND hWnd,
			 WINDOWPLACEMENT *lpwndpl)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserSetWindowPos(      
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags)
{
   UNIMPLEMENTED;
   return 0;
}


INT STDCALL
NtUserSetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw)
{
   UNIMPLEMENTED;
   return ERROR;
}


BOOL STDCALL
NtUserShowWindow(HWND hWnd,
		 LONG nCmdShow)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
		      DWORD Unknown1)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD STDCALL
NtUserUpdateLayeredWindow(DWORD Unknown0,
			  DWORD Unknown1,
			  DWORD Unknown2,
			  DWORD Unknown3,
			  DWORD Unknown4,
			  DWORD Unknown5,
			  DWORD Unknown6,
			  DWORD Unknown7,
			  DWORD Unknown8)
{
   UNIMPLEMENTED;
   return 0;
}


HWND STDCALL
NtUserWindowFromPoint(LONG X, LONG Y)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL STDCALL
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText)
{
   UNIMPLEMENTED;
   return FALSE;
}


INT STDCALL
NtUserInternalGetWindowText(HWND hWnd, LPWSTR lpString, INT nMaxCount)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * Miscellaneous
 */


DWORD
STDCALL
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL
STDCALL
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Routine)
{
   UNIMPLEMENTED;
   return FALSE;
}


HWND
STDCALL
NtUserCallHwndOpt(
  HWND Param,
  DWORD Routine)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD STDCALL
NtUserGetThreadState(
  DWORD Routine)
{
   UNIMPLEMENTED;
   return 0;
}


UINT
STDCALL
NtUserGetDoubleClickTime(VOID)
{
   UNIMPLEMENTED;
   return 0;
}


BOOL
STDCALL
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui)
{
   UNIMPLEMENTED;
   return FALSE;
}


DWORD
STDCALL
NtUserGetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
   UNIMPLEMENTED;
   return 0;
}


DWORD
STDCALL
NtUserActivateKeyboardLayout(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

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
  DWORD Unknown7)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallHwnd(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallHwndParam(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallHwndParamLock(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallMsgFilter(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

LONG
STDCALL
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDdeGetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDdeSetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDragObject(
	   HWND    hwnd1,
	   HWND    hwnd2,
	   UINT    u1,
	   DWORD   dw1,
	   HCURSOR hc1
	   )
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDrawAnimatedRects(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDrawCaption(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDrawCaptionTemp(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICE lpDisplayDevice, /* device information */
  DWORD dwFlags ) /* reserved */
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserEnumDisplayMonitors(
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags )
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserExcludeUpdateRgn(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetAltTabInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetAsyncKeyState(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetComboBoxInfo(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetControlBrush(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetControlColor(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetKeyboardLayoutList(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetKeyboardLayoutName(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetListBoxInfo(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetMouseMovePointsEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetTitleBarInfo(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserImpersonateDdeClientWindow(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserInitializeClientPfnArrays(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

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
  DWORD Unknown10)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserLoadKeyboardLayoutEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserLockWorkStation(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserMNDragLeave(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserNotifyWinEvent(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserQueryUserCounters(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserRegisterTasklist(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetSysColors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserTrackMouseEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUnloadKeyboardLayout(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUpdatePerUserSystemParameters(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUserHandleGrantAccess(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserValidateHandleSecure(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserVkKeyScanEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserWaitForInputIdle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserWaitForMsgAndEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserYieldTask(VOID)
{
  UNIMPLEMENTED

  return 0;
}

/*
 * Input
 */

BOOL
STDCALL
NtUserDragDetect(
  HWND hWnd,
  LONG x,
  LONG y)
{
  UNIMPLEMENTED
  return 0;
}

BOOL
STDCALL
NtUserBlockInput(
  BOOL BlockIt)
{
   UNIMPLEMENTED;
   return FALSE;
}

DWORD
STDCALL
NtUserAttachThreadInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
