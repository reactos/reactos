/*****************************************************************************\
*                                                                             *
* skbapi.h -    Screen Keyboard Public Header File
*                                                                             *
*               
*                                                                             *
*               Copyright (c) 1992-1993, Microsoft Corp. All rights reserved. *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_WINDOWS
#include <windows.h>    /* windows.h must be preincluded */
#endif /* _INC_WINDOWS */

#ifndef _INC_SKBAPI     /* prevent multiple includes */
#define _INC_SKBAPI

/****** Screen Keyboard *****************************************************/

#ifndef WM_SKB                 /* also defined in penwin.h */
#define WM_SKB                 (WM_PENWINFIRST+4)
#endif

/*	wCommand values */
#define SKB_QUERY              0x0000
#define SKB_SHOW               0x0001
#define SKB_HIDE               0x0002
#define SKB_CENTER             0x0010
#define SKB_MOVE               0x0020
#define SKB_MINIMIZE           0x0040

/* wPad values */
#define SKB_FULL               0x0100
#define SKB_BASIC              0x0200
#define SKB_NUMPAD             0x0400
#define SKB_ATMPAD             0x0800
#define SKB_DEFAULT            SKB_FULL
#define SKB_CURRENT            0x0000

/* return values */
#define SKB_OK                 0x0000
#define SKB_ERR                0xFFFF

/* notification values */
#define SKN_CHANGED            1

#define SKN_POSCHANGED         1
#define SKN_PADCHANGED         2
#define SKN_MINCHANGED         4
#define SKN_VISCHANGED         8
#define SKN_TERMINATED         0xffff

typedef struct tagSKBINFO
   {
   HWND hwnd;
   UINT nPad;
   BOOL fVisible;
   BOOL fMinimized;
   RECT rect;
   DWORD dwReserved;
   }
   SKBINFO, FAR *LPSKBINFO;


UINT WINAPI ScreenKeyboard(HWND, UINT, UINT, LPPOINT, LPSKBINFO);	/* skb.dll */

#endif /* _INC_SKBAPI */
