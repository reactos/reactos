/* MDI.H
 *
 * Copyright 1994, Bob Amstadt
 *           1995  Alex Korobka
 *
 * MDI structure definitions.
 */

#ifndef __WINE_MDI_H
#define __WINE_MDI_H

#include <windows.h>

#define MDI_MAXLISTLENGTH	0x40
#define MDI_MAXTITLELENGTH	0xA1

#define MDI_NOFRAMEREPAINT	0
#define MDI_REPAINTFRAMENOW	1
#define MDI_REPAINTFRAME	2

#define WM_MDICALCCHILDSCROLL   0x10AC /* this is exactly what Windows uses */

LRESULT WINAPI MDIClientWndProc( HWND hwnd, UINT message, 
                                        WPARAM wParam, LPARAM lParam );

typedef struct 
{
    UINT      nActiveChildren;
    HWND      hwndChildMaximized;
    HWND      hwndActiveChild;
    HMENU     hWindowMenu;
    UINT      idFirstChild;
    LPSTR       frameTitle;
    UINT      nTotalCreated;
    UINT      mdiFlags;
    UINT      sbRecalc;   /* SB_xxx flags for scrollbar fixup */
    HWND      self;
} MDICLIENTINFO;

extern HWND  MDI_CreateMDIWindowA(LPCSTR,LPCSTR,DWORD,INT,INT,
                                INT,INT,HWND,HINSTANCE,LPARAM);
extern HWND  MDI_CreateMDIWindowW(LPCWSTR,LPCWSTR,DWORD,INT,INT,
                                INT,INT,HWND,HINSTANCE,LPARAM);
#endif /* __WINE_MDI_H */

