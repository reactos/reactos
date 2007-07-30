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
#include "controls.h"
#include "draw.h"
#include "dde_private.h"
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

#define NtUserRegisterUserModule(hInstance) \
  (BOOL)NtUserCallOneParam((DWORD)hInstance, ONEPARAM_ROUTINE_REGISTERUSERMODULE)

/*
#define NtUserEnumClipboardFormats(format) \
  (UINT)NtUserCallOneParam(format, ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS)
*/

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

#define NtUserShowCursor(bShow) \
  NtUserCallOneParam((DWORD)bShow, ONEPARAM_ROUTINE_SHOWCURSOR)

#define ShowCaret(hwnd) \
  NtUserShowCaret(hwnd)

#define HideCaret(hwnd) \
  NtUserHideCaret(hwnd)


/* Internal Thread Data */
extern HINSTANCE User32Instance;

typedef struct _USER32_TRACKINGLIST {
    TRACKMOUSEEVENT tme;
    POINT pos; /* center of hover rectangle */
    UINT_PTR timer;
} USER32_TRACKINGLIST,*PUSER32_TRACKINGLIST;


typedef struct _USER32_THREAD_DATA
{
    MSG LastMessage;
    HKL KeyboardLayoutHandle;
    USER32_TRACKINGLIST tracking_info; /* TrackMouseEvent stuff */
} USER32_THREAD_DATA, *PUSER32_THREAD_DATA;

PUSER32_THREAD_DATA User32GetThreadData();
  
DEVMODEW *
STDCALL
GdiConvertToDevmodeW(DEVMODEA *dm);

/* FIXME: Belongs to some header. */
BOOL STDCALL GdiDllInitialize(HANDLE, DWORD, LPVOID);
void InitStockObjects(void);
VOID DeleteFrameBrushes(VOID);

/* message spy definitions */
#define SPY_DISPATCHMESSAGE       0x0101
#define SPY_SENDMESSAGE           0x0103
#define SPY_DEFWNDPROC            0x0105

#define SPY_RESULT_OK             0x0001
#define SPY_RESULT_INVALIDHWND    0x0003
#define SPY_RESULT_DEFWND         0x0005

extern const char *SPY_GetMsgName(UINT msg, HWND hWnd);
extern const char *SPY_GetVKeyName(WPARAM wParam);
extern void SPY_EnterMessage(INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void SPY_ExitMessage(INT iFlag, HWND hwnd, UINT msg,
                            LRESULT lReturn, WPARAM wParam, LPARAM lParam);
extern int SPY_Init(void);


#endif
/* EOF */
