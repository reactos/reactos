/*
 * USER DCE definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_DCE_H
#define __WINE_DCE_H

#include <windows.h>

struct tagWND;
/* additional DCX flags 
 */

#define DCX_NORESETATTR		0x00000004
#define DCX_EXCLUDEUPDATE    	0x00000100
#define DCX_INTERSECTUPDATE  	0x00000200
//#define DCX_LOCKWINDOWUPDATE 	0x00000400
#define DCX_NORECOMPUTE      	0x00100000
//#define DCX_VALIDATE         	0x00200000

#define DCX_DCEEMPTY		0x00000800
#define DCX_DCEBUSY		0x00001000
#define DCX_DCEDIRTY		0x00002000
#define DCX_WINDOWPAINT		0x00020000
#define DCX_KEEPCLIPRGN		0x00040000
#define DCX_NOCLIPCHILDREN      0x00080000

typedef enum
{
    DCE_CACHE_DC,   /* This is a cached DC (allocated by USER) */
    DCE_CLASS_DC,   /* This is a class DC (style CS_CLASSDC) */
    DCE_WINDOW_DC   /* This is a window DC (style CS_OWNDC) */
} DCE_TYPE;


typedef struct tagDCE
{
    struct tagDCE *next;
    HDC          hDC;
    HWND         hwndCurrent;
    HWND         hwndDC;
    HRGN         hClipRgn;
    DCE_TYPE     type;
    DWORD        DCXflags;
} DCE;


void  DCE_Init(void);
DCE*  DCE_AllocDCE( HWND hWnd, DCE_TYPE type );
DCE*  DCE_FreeDCE( DCE *dce );
void  DCE_FreeWindowDCE( struct tagWND* );
INT DCE_ExcludeRgn( HDC, struct tagWND*, HRGN );
HRGN DCE_GetVisRgn( HWND, WORD );
WINBOOL DCE_InvalidateDCE( struct tagWND *, const RECT* );

#endif  /* __WINE_DCE_H */
