#pragma once

struct _PROPERTY;
struct _WINDOW_OBJECT;
typedef struct _WINDOW_OBJECT *PWINDOW_OBJECT;

#include <include/object.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/winsta.h>
#include <include/dce.h>
#include <include/prop.h>
#include <include/scroll.h>

extern ATOM AtomMessage;
extern ATOM AtomWndObj; /* WNDOBJ list */

BOOL FASTCALL UserUpdateUiState(PWND Wnd, WPARAM wParam);

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define HAS_BIGFRAME(style,exStyle) \
            (((style) & (WS_THICKFRAME | WS_DLGFRAME)) || \
            ((exStyle) & WS_EX_DLGMODALFRAME))

#define HAS_STATICOUTERFRAME(style,exStyle) \
            (((exStyle) & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == \
            WS_EX_STATICEDGE)

#define HAS_ANYFRAME(style,exStyle) \
            (((style) & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) || \
            ((exStyle) & WS_EX_DLGMODALFRAME) || \
            !((style) & (WS_CHILD | WS_POPUP)))

#define HAS_MENU(pWnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && pWnd->IDMenu)

#define IntIsDesktopWindow(WndObj) \
  (WndObj->spwndParent == NULL)

#define IntIsBroadcastHwnd(hWnd) \
  (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST)


#define IntWndBelongsToThread(WndObj, W32Thread) \
  ((WndObj->head.pti) && (WndObj->head.pti == W32Thread))

#define IntGetWndThreadId(WndObj) \
  WndObj->head.pti->pEThread->Cid.UniqueThread

#define IntGetWndProcessId(WndObj) \
  WndObj->head.pti->pEThread->ThreadsProcess->UniqueProcessId

BOOL FASTCALL
IntIsWindow(HWND hWnd);

HWND* FASTCALL
IntWinListChildren(PWND Window);

INIT_FUNCTION
NTSTATUS
NTAPI
InitWindowImpl (VOID);

NTSTATUS FASTCALL
CleanupWindowImpl (VOID);

VOID FASTCALL
IntGetClientRect (PWND WindowObject, RECTL *Rect);

HWND FASTCALL
IntGetActiveWindow (VOID);

BOOL FASTCALL
IntIsWindowVisible (PWND Window);

BOOL FASTCALL
IntIsChildWindow (PWND Parent, PWND Child);

VOID FASTCALL
IntUnlinkWindow(PWND Wnd);

VOID FASTCALL
IntLinkWindow(PWND Wnd, PWND WndPrevSibling);

VOID FASTCALL 
IntLinkHwnd(PWND Wnd, HWND hWndPrev);

PWND FASTCALL
IntGetAncestor(PWND Wnd, UINT Type);

PWND FASTCALL
IntGetParent(PWND Wnd);

INT FASTCALL
IntGetWindowRgn(PWND Window, HRGN hRgn);

INT FASTCALL
IntGetWindowRgnBox(PWND Window, RECTL *Rect);

BOOL FASTCALL
IntGetWindowInfo(PWND WindowObject, PWINDOWINFO pwi);

VOID FASTCALL
IntGetWindowBorderMeasures(PWND WindowObject, UINT *cx, UINT *cy);

BOOL FASTCALL
IntIsWindowInDestroy(PWND Window);

BOOL FASTCALL
IntShowOwnedPopups( PWND owner, BOOL fShow );

LRESULT FASTCALL
IntDefWindowProc( PWND Window, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL Ansi);

VOID FASTCALL IntNotifyWinEvent(DWORD, PWND, LONG, LONG, DWORD);

PWND FASTCALL co_UserCreateWindowEx(CREATESTRUCTW*, PUNICODE_STRING, PLARGE_STRING);
WNDPROC FASTCALL IntGetWindowProc(PWND,BOOL);

BOOL FASTCALL IntEnableWindow(HWND,BOOL);
DWORD FASTCALL GetNCHitEx(PWND pWnd, POINT pt);

/* EOF */
