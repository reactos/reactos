/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/user32.h
 * PURPOSE:     Global user32 definitions
 */
#include <windows.h>
#include <win32k/win32k.h>

typedef struct _USER32_THREAD_DATA
{
  MSG LastMessage;
  HKL KeyboardLayoutHandle;
} USER32_THREAD_DATA, *PUSER32_THREAD_DATA;

PUSER32_THREAD_DATA User32GetThreadData();

/* a copy of this structure is in subsys/win32k/include/caret.h */
typedef struct _THRDCARETINFO
{
  HWND hWnd;
  HBITMAP Bitmap;
  POINT Pos;
  SIZE Size;
  BYTE Visible;
  BYTE Showing;
} THRDCARETINFO, *PTHRDCARETINFO;

VOID CreateFrameBrushes(VOID);
VOID DeleteFrameBrushes(VOID);
void DrawCaret(HWND hWnd, PTHRDCARETINFO CaretInfo);


#define NtUserValidateRgn(hWnd, hRgn) \
  (BOOL)NtUserCallTwoParam((DWORD)hWnd, (DWORD)hRgn, TWOPARAM_ROUTINE_VALIDATERGN)

#define NtUserSetWindowContextHelpId(hWnd, dwContextHelpId) \
  (BOOL)NtUserCallTwoParam((DWORD)hwnd, dwContextHelpId, TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID)

#define NtUserSetCaretPos(X, Y) \
  (BOOL)NtUserCallTwoParam((DWORD)X, (DWORD)Y, TWOPARAM_ROUTINE_SETCARETPOS)

#define NtUserGetCursorPos(lpPoint) \
  (BOOL)NtUserCallTwoParam((DWORD)lpPoint, (DWORD)FALSE, TWOPARAM_ROUTINE_CURSORPOSITION)

#define NtUserSetCursorPos(lpPoint) \
  (BOOL)NtUserCallTwoParam((DWORD)lpPoint, (DWORD)TRUE, TWOPARAM_ROUTINE_CURSORPOSITION)

#define NtUserSetGUIThreadHandle(field, hwnd) \
  (BOOL)NtUserCallTwoParam((DWORD)field, (DWORD)hwnd, TWOPARAM_ROUTINE_SETGUITHRDHANDLE)

#define NtUserSetMenuItemRect(menu, mir) \
  (BOOL)NtUserCallTwoParam((DWORD)menu, (DWORD)mir, TWOPARAM_ROUTINE_SETMENUITEMRECT)

#define NtUserSetMenuBarHeight(menu, height) \
  (BOOL)NtUserCallTwoParam((DWORD)menu, (DWORD)height, TWOPARAM_ROUTINE_SETMENUBARHEIGHT)

#define NtUserGetWindowInfo(hwnd, pwi) \
  (BOOL)NtUserCallTwoParam((DWORD)hwnd, (DWORD)pwi, TWOPARAM_ROUTINE_GETWINDOWINFO)

#define NtUserSetCaretBlinkTime(uMSeconds) \
  (BOOL)NtUserCallOneParam((DWORD)uMSeconds, ONEPARAM_ROUTINE_SETCARETBLINKTIME)

#define NtUserEnumClipboardFormats(format) \
  (UINT)NtUserCallOneParam(format, ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS)

#define NtUserWindowFromDC(hDC) \
  (HWND)NtUserCallOneParam((DWORD)hDC, ONEPARAM_ROUTINE_WINDOWFROMDC)

#define NtUserSwitchCaretShowing(CaretInfo) \
  (BOOL)NtUserCallOneParam((DWORD)CaretInfo, ONEPARAM_ROUTINE_SWITCHCARETSHOWING)

#define NtUserSwapMouseButton(fSwap) \
  (BOOL)NtUserCallOneParam((DWORD)fSwap, ONEPARAM_ROUTINE_SWAPMOUSEBUTTON)

#define NtUserGetMenu(hWnd) \
  (HMENU)NtUserCallOneParam((DWORD)hWnd, ONEPARAM_ROUTINE_GETMENU)

#define NtUserSetMessageExtraInfo(lParam) \
  (LPARAM)NtUserCallOneParam((DWORD)lParam, ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO)

#define NtUserIsWindowUnicode(hWnd) \
  (BOOL)NtUserCallOneParam((DWORD)hWnd, ONEPARAM_ROUTINE_ISWINDOWUNICODE)

#define NtUserGetWindowContextHelpId(hwnd) \
  NtUserCallOneParam((DWORD)hwnd, ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID)

LONG WINAPI RegCloseKey(HKEY);
LONG WINAPI RegOpenKeyExW(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);
LONG WINAPI RegQueryValueExW(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
