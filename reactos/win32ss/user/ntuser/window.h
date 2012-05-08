#pragma once

extern ATOM AtomMessage;
extern ATOM AtomWndObj; /* WNDOBJ list */
extern ATOM AtomLayer;
extern ATOM AtomFlashWndState;

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define HAS_MENU(pWnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && pWnd->IDMenu)

#define IntIsDesktopWindow(WndObj) \
  (WndObj->spwndParent == NULL)

#define IntIsBroadcastHwnd(hWnd) \
  (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST)


#define IntWndBelongsToThread(WndObj, W32Thread) \
  ((WndObj->head.pti) && (WndObj->head.pti == W32Thread))

#define IntGetWndThreadId(WndObj) \
  PsGetThreadId(WndObj->head.pti->pEThread)

#define IntGetWndProcessId(WndObj) \
  PsGetProcessId(WndObj->head.pti->ppi->peProcess)

  
BOOL FASTCALL UserUpdateUiState(PWND Wnd, WPARAM wParam);
BOOL FASTCALL IntIsWindow(HWND hWnd);
HWND* FASTCALL IntWinListChildren(PWND Window);
VOID FASTCALL IntGetClientRect (PWND WindowObject, RECTL *Rect);
BOOL FASTCALL IntIsChildWindow (PWND Parent, PWND Child);
VOID FASTCALL IntUnlinkWindow(PWND Wnd);
VOID FASTCALL IntLinkHwnd(PWND Wnd, HWND hWndPrev);
PWND FASTCALL IntGetParent(PWND Wnd);
VOID FASTCALL IntGetWindowBorderMeasures(PWND WindowObject, UINT *cx, UINT *cy);
BOOL FASTCALL IntShowOwnedPopups( PWND owner, BOOL fShow );
LRESULT FASTCALL IntDefWindowProc( PWND Window, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL Ansi);
VOID FASTCALL IntNotifyWinEvent(DWORD, PWND, LONG, LONG, DWORD);
PWND FASTCALL co_UserCreateWindowEx(CREATESTRUCTW*, PUNICODE_STRING, PLARGE_STRING, PVOID);
BOOL FASTCALL IntEnableWindow(HWND,BOOL);
DWORD FASTCALL GetNCHitEx(PWND pWnd, POINT pt);

/* EOF */
