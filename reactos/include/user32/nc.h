/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

//#include "config.h"

#define WIN95_LOOK 1
#define WIN31_LOOK 0
extern int TWEAK_WineLook;


#include <user32/win.h>
#include <user32/sysmetr.h>
#include <user32/msg.h>
#include <user32/menu.h>
#include <user32/winpos.h>
#include <user32/hook.h>
#include <user32/scroll.h>

#define SC_ARRANGE      0xf110

#define WM_SETVISIBLE           0x0009

  /* WM_NCHITTEST return codes */
#define HTERROR             (-2)
#define HTTRANSPARENT       (-1)



#define HTMINBUTTON         8
#define HTMAXBUTTON         9



#define HTBORDER            18

#define HTOBJECT            19
#define HTCLOSE             20
#define HTHELP              21
#define HTSIZEFIRST         HTLEFT
#define HTSIZELAST          HTBOTTOMRIGHT

  /* CallMsgFilter() values */

#define MSGF_MESSAGEBOX     1

#define MSGF_MOVE           3
#define MSGF_SIZE           4


#define MAKEINTRESOURCEA(i)  (LPSTR) ((DWORD) ((WORD) (i)))

#define DCX_USESTYLE         0x00010000

LONG   NC_HandleNCPaint( HWND hwnd , HRGN clip);
LONG   NC_HandleNCActivate( WND *pwnd, WPARAM wParam );
LONG   NC_HandleNCCalcSize( WND *pWnd, RECT *winRect );
LONG   NC_HandleNCHitTest( HWND hwnd, POINT pt );
LONG   NC_HandleNCLButtonDown( WND* pWnd, WPARAM wParam, LPARAM lParam );
LONG   NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM wParam, LPARAM lParam);
LONG   NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt );
LONG   NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );
void   NC_DrawSysButton( HWND hwnd, HDC hdc, WINBOOL down );
WINBOOL NC_DrawSysButton95( HWND hwnd, HDC hdc, WINBOOL down );
WINBOOL NC_GetSysPopupPos( WND* wndPtr, RECT* rect );
void NC_AdjustRect( LPRECT rect, DWORD style, WINBOOL menu,
                           DWORD exStyle );

//gdi
WINBOOL STDCALL FastWindowFrame(HDC,const RECT*,INT,INT,DWORD);
void NC_AdjustRectOuter95 (LPRECT rect, DWORD style, WINBOOL menu, DWORD exStyle);
void NC_AdjustRectInner95 (LPRECT rect, DWORD style, DWORD exStyle);


#endif /* __WINE_NONCLIENT_H */
