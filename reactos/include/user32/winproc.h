/*
 * Window procedure callbacks definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINPROC_H
#define __WINE_WINPROC_H

#include <windows.h>

typedef enum
{  
	WIN_PROC_A,
	WIN_PROC_W
} WINDOWPROCTYPE;

typedef enum
{
    WIN_PROC_CLASS,
    WIN_PROC_WINDOW,
    WIN_PROC_TIMER
} WINDOWPROCUSER;


typedef HANDLE  HWINDOWPROC;
// base.h defines WNDPROC

typedef struct _MSGPARAM
{
    WPARAM      wParam;
    LPARAM	lParam;
    LRESULT	lResult;
} MSGPARAM;


WINBOOL WINPROC_Init(void);
WNDPROC WINPROC_GetProc( HWINDOWPROC proc, WINDOWPROCTYPE type );
WINBOOL WINPROC_SetProc( HWINDOWPROC *pFirst, WNDPROC func,
                               WINDOWPROCTYPE type, WINDOWPROCUSER user );
void WINPROC_FreeProc( HWINDOWPROC proc, WINDOWPROCUSER user );
WINDOWPROCTYPE WINPROC_GetProcType( HWINDOWPROC proc );

INT WINPROC_MapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam, LPARAM *plparam );
void WINPROC_UnmapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
INT WINPROC_MapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam, LPARAM *plparam );
void WINPROC_UnmapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );


#endif  /* __WINE_WINPROC_H */
