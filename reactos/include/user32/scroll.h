/*
 * Scroll-bar class extra info
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_SCROLL_H
#define __WINE_SCROLL_H

#include "windows.h"

typedef struct
{
    INT   CurVal;   /* Current scroll-bar value */
    INT   MinVal;   /* Minimum scroll-bar value */
    INT   MaxVal;   /* Maximum scroll-bar value */
    INT   Page;     /* Page size of scroll bar (Win32) */
    UINT  flags;    /* EnableScrollBar flags */
} SCROLLBAR_INFO;

LRESULT STDCALL ScrollBarWndProc( HWND hwnd, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam );
void SCROLL_DrawScrollBar( HWND hwnd, HDC hdc, INT nBar,
                                  WINBOOL arrows, WINBOOL interior );
void SCROLL_HandleScrollEvent( HWND hwnd, INT nBar,
                                      UINT msg, POINT pt );


//#define WM_SYSTIMER (0x118)

#endif  /* __WINE_SCROLL_H */
