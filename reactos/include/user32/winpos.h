/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_WINPOS_H
#define __WINE_WINPOS_H

#include <user32/win.h>

#define DWP_MAGIC  ((INT)('W' | ('P' << 8) | ('O' << ) | ('S' << 24)))

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

#define SWP_DEFERERASE      0x2000


#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_BORDER)))

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

#define EMPTYPOINT(pt)          ((*(LONG*)&(pt)) == -1)

#define PLACE_MIN		0x0001
#define PLACE_MAX		0x0002
#define PLACE_RECT		0x0004

#define SMC_NOCOPY		0x0001
#define SMC_NOPARENTERASE	0x0002
#define SMC_DRAWFRAME		0x0004
#define SMC_SETXPOS		0x0008

typedef struct
{
    INT       actualCount;
    INT       suggestedCount;
    WINBOOL      valid;
    INT       wMagic;
    HWND      hwndParent;
    WINDOWPOS winPos[1];
} DWP;

WINBOOL WINPOS_RedrawIconTitle( HWND hWnd );
WINBOOL WINPOS_ShowIconTitle( WND* pWnd, WINBOOL bShow );
void   WINPOS_GetMinMaxInfo( WND* pWnd, POINT *maxSize,
                                    POINT *maxPos, POINT *minTrack,
                                    POINT *maxTrack );
UINT WINPOS_MinMaximize( WND* pWnd, UINT cmd, LPRECT lpPos);
WINBOOL WINPOS_SetActiveWindow( HWND hWnd, WINBOOL fMouse,
                                      WINBOOL fChangeFocus );
WINBOOL WINPOS_ChangeActiveWindow( HWND hwnd, WINBOOL mouseMsg );
LONG WINPOS_SendNCCalcSize(HWND hwnd, WINBOOL calcValidRect,
                                  RECT *newWindowRect, RECT *oldWindowRect,
                                  RECT *oldClientRect, WINDOWPOS *winpos,
                                  RECT *newClientRect );
LONG WINPOS_HandleWindowPosChanging(WND *wndPtr, WINDOWPOS *winpos);
LONG WINPOS_HandleWindowPosChanging(WND *wndPtr, WINDOWPOS *winpos);
INT WINPOS_WindowFromPoint( WND* scopeWnd, POINT pt, WND **ppWnd );
void WINPOS_CheckInternalPos( HWND hwnd );
WINBOOL WINPOS_ActivateOtherWindow(WND* pWnd);
WINBOOL WINPOS_CreateInternalPosAtom(void);

void WINPOS_GetWinOffset( HWND hwndFrom, HWND hwndTo,
                                 POINT *offset );
WINBOOL WINPOS_CanActivate(WND* pWnd);
LPINTERNALPOS WINPOS_InitInternalPos( WND* wnd, POINT pt, 
					     LPRECT restoreRect );
WINBOOL WINPOS_SetPlacement( HWND hwnd, const WINDOWPLACEMENT *wndpl,
						UINT flags );

void WINPOS_MoveWindowZOrder( HWND hwnd, HWND hwndAfter );

UINT WINPOS_SizeMoveClean( WND* Wnd, HRGN oldVisRgn,
                                    LPRECT lpOldWndRect,
                                    LPRECT lpOldClientRect, UINT uFlags );

HWND WINPOS_ReorderOwnedPopups(HWND hwndInsertAfter,WND* wndPtr,WORD flags);

#endif  /* __WINE_WINPOS_H */
