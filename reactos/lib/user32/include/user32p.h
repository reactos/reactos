/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32p.h
 * PURPOSE:         Win32 User Library Private Headers
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef USER32_PRIVATE_H
#define USER32_PRIVATE_H

/* Private User32 Headers */
#include "accel.h"
#include "cursor.h"
#ifndef __WINE__
#include "debug.h"
#endif
#include "draw.h"
#include "menu.h"
#include "message.h"
#include "regcontrol.h"
#include "resource.h"
#include "scroll.h"
#include "strpool.h"
#include "window.h"
#include "winpos.h"
#include "winsta.h"

/* One/Two Param Functions */
#define NtUserMsqSetWakeMask(dwWaitMask) \
  (HANDLE)NtUserCallOneParam(dwWaitMask, ONEPARAM_ROUTINE_MSQSETWAKEMASK)

#define NtUserMsqClearWakeMask() \
  NtUserCallNoParam(NOPARAM_ROUTINE_MSQCLEARWAKEMASK)
  
#define NtUserAnyPopup() \
  (BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_ANYPOPUP)

#define NtUserValidateRgn(hWnd, hRgn) \
  (BOOL)NtUserCallTwoParam((DWORD)hWnd, (DWORD)hRgn, TWOPARAM_ROUTINE_VALIDATERGN)

#define NtUserSetWindowContextHelpId(hWnd, dwContextHelpId) \
  (BOOL)NtUserCallTwoParam((DWORD)hwnd, dwContextHelpId, TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID)

#define NtUserSetCaretPos(X, Y) \
  (BOOL)NtUserCallTwoParam((DWORD)X, (DWORD)Y, TWOPARAM_ROUTINE_SETCARETPOS)

#define NtUserSetGUIThreadHandle(field, hwnd) \
  (BOOL)NtUserCallTwoParam((DWORD)field, (DWORD)hwnd, TWOPARAM_ROUTINE_SETGUITHRDHANDLE)

#define NtUserSetMenuItemRect(menu, mir) \
  (BOOL)NtUserCallTwoParam((DWORD)menu, (DWORD)mir, TWOPARAM_ROUTINE_SETMENUITEMRECT)

#define NtUserSetMenuBarHeight(menu, height) \
  (BOOL)NtUserCallTwoParam((DWORD)menu, (DWORD)height, TWOPARAM_ROUTINE_SETMENUBARHEIGHT)

#define NtUserGetWindowInfo(hwnd, pwi) \
  (BOOL)NtUserCallTwoParam((DWORD)hwnd, (DWORD)pwi, TWOPARAM_ROUTINE_GETWINDOWINFO)

#define NtUserRegisterLogonProcess(hproc, x) \
  (BOOL)NtUserCallTwoParam((DWORD)hproc, (DWORD)x, TWOPARAM_ROUTINE_REGISTERLOGONPROC)

#define NtUserGetSysColorBrushes(HBrushes, count) \
  (BOOL)NtUserCallTwoParam((DWORD)(HBrushes), (DWORD)(count), TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES)

#define NtUserGetSysColorPens(HPens, count) \
  (BOOL)NtUserCallTwoParam((DWORD)(HPens), (DWORD)(count), TWOPARAM_ROUTINE_GETSYSCOLORPENS)

#define NtUserGetSysColors(ColorRefs, count) \
  (BOOL)NtUserCallTwoParam((DWORD)(ColorRefs), (DWORD)(count), TWOPARAM_ROUTINE_GETSYSCOLORS)

#define NtUserSetSysColors(ColorRefs, count) \
  (BOOL)NtUserCallTwoParam((DWORD)(ColorRefs), (DWORD)(count), TWOPARAM_ROUTINE_SETSYSCOLORS)

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

#define NtUserGetWindowInstance(hwnd) \
  (HINSTANCE)NtUserCallOneParam((DWORD)hwnd, ONEPARAM_ROUTINE_GETWINDOWINSTANCE)

#define NtUserGetCursorPos(lpPoint) \
  (BOOL)NtUserCallOneParam((DWORD)lpPoint, ONEPARAM_ROUTINE_GETCURSORPOSITION)

#define NtUserIsWindowInDestroy(hWnd) \
  (BOOL)NtUserCallOneParam((DWORD)hWnd, ONEPARAM_ROUTINE_ISWINDOWINDESTROY)

#define NtUserEnableProcessWindowGhosting(bEnable) \
  NtUserCallOneParam((DWORD)bEnable, ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING)

/* Internal Thread Data */
extern HINSTANCE User32Instance;

typedef struct _USER32_THREAD_DATA
{
    MSG LastMessage;
    HKL KeyboardLayoutHandle;
} USER32_THREAD_DATA, *PUSER32_THREAD_DATA;

PUSER32_THREAD_DATA User32GetThreadData();
  
#endif
/* EOF */
