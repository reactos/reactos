/*
 * Static-class extra info
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_STATIC_H
#define __WINE_STATIC_H

#include <windows.h>
#include <user32/win.h>

  /* Extra info for STATIC windows */
typedef struct
{
    HFONT  hFont;   /* Control font (or 0 for system font) */
    WORD   dummy;   /* Don't know what MS-Windows puts in there */
    HICON  hIcon;   /* Icon handle for SS_ICON controls */ 
} STATICINFO;

HICON STATIC_LoadIcon(WND *wndPtr,const void *name );
HICON STATIC_SetIcon( WND *wndPtr, HICON hicon );

HBITMAP STATIC_LoadBitmap(WND *wndPtr,const void *name );
HICON STATIC_SetBitmap( WND *wndPtr, HICON hicon );

LRESULT WINAPI StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam );

#endif  /* __WINE_STATIC_H */
