/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995, 1996 Alex Korobka
 */

#include <ntos/minmax.h>
#define MIN min
#define MAX max

#include <windows.h>
#include <string.h>
#include <user32/sysmetr.h>
#include <user32/caret.h>
#include <user32/win.h>
#include <user32/queue.h>
#include <user32/winpos.h>
#include <user32/hook.h>
#include <user32/dce.h>
#include <user32/nc.h>
#include <user32/paint.h>
#include <user32/debug.h>


HWND ICONTITLE_Create( WND *pWnd );
HWND GetSysModalWindow(void);


/* ----- internal variables ----- */
HWND hwndActive      = 0;  /* Currently active window */
HWND hwndPrevActive  = 0;  /* Previously active window */
HWND hGlobalShellWindow=0; /*the shell*/

LPCSTR atomInternalPos;

HWND  WINPOS_GetActiveWindow(void)
{  
    return  hwndActive;
}


/***********************************************************************
 *           WINPOS_CreateInternalPosAtom
 */
WINBOOL WINPOS_CreateInternalPosAtom()
{
    LPSTR str = "SysIP";
atomInternalPos ="SysIP";
  //  atomInternalPos = (LPCSTR)(DWORD)GlobalAddAtomA(str);
    //if ( atomInternalPos == NULL )
//	atomInternalPos = (LPCSTR)(DWORD)GlobalFindAtom(str);
    return (atomInternalPos) ? TRUE : FALSE;
}

/***********************************************************************
 *           WINPOS_CheckInternalPos
 *
 * Called when a window is destroyed.
 */
void WINPOS_CheckInternalPos( HWND hwnd )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS) GetPropA( hwnd, atomInternalPos );

    if( hwnd == hwndPrevActive ) hwndPrevActive = 0;
    if( hwnd == hwndActive )
    {
	hwndActive = 0; 
	DPRINT("\tattempt to activate destroyed window!\n");
    }

    if( lpPos )
    {
	//if( IsWindow(lpPos->hwndIconTitle) ) 
	//    DestroyWindow( lpPos->hwndIconTitle );
	//HeapFree( GetProcessHeap(), 0, lpPos );
    }
}

/***********************************************************************
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 */
static POINT WINPOS_FindIconPos( WND* wndPtr, POINT pt )
{
    RECT rectParent;
    short x, y, xspacing, yspacing;

    if ( wndPtr->parent != NULL ) {
    	GetClientRect( wndPtr->parent->hwndSelf, &rectParent );
    }
    else {

	rectParent.left = 0;
	rectParent.right = SYSMETRICS_CXFULLSCREEN;
	
	rectParent.top = 0;
	rectParent.right = SYSMETRICS_CYFULLSCREEN;
    }
	
    if ((pt.x >= rectParent.left) && (pt.x + SYSMETRICS_CXICON < rectParent.right) &&
        (pt.y >= rectParent.top) && (pt.y + SYSMETRICS_CYICON < rectParent.bottom))
        return pt;  /* The icon already has a suitable position */

    xspacing = SYSMETRICS_CXICONSPACING;
    yspacing = SYSMETRICS_CYICONSPACING;

    y = rectParent.bottom;
    for (;;)
    {
        for (x = rectParent.left; x <= rectParent.right-xspacing; x += xspacing)
        {
              /* Check if another icon already occupies this spot */
	    WND *childPtr = NULL;
	    if ( wndPtr->parent )
            	childPtr = wndPtr->parent->child;
            while (childPtr)
            {
                if ((childPtr->dwStyle & WS_MINIMIZE) && (childPtr != wndPtr))
                {
                    if ((childPtr->rectWindow.left < x + xspacing) &&
                        (childPtr->rectWindow.right >= x) &&
                        (childPtr->rectWindow.top <= y) &&
                        (childPtr->rectWindow.bottom > y - yspacing))
                        break;  /* There's a window in there */
                }
                childPtr = childPtr->next;
            }
            if (!childPtr) /* No window was found, so it's OK for us */
            {
		pt.x = x + (xspacing - SYSMETRICS_CXICON) / 2;
		pt.y = y - (yspacing + SYSMETRICS_CYICON) / 2;
		return pt;
            }
        }
        y -= yspacing;
    }
}

/***********************************************************************
 *           WINPOS_WindowFromPoint
 *
 * Find the window and hittest for a given point.
 */
INT WINPOS_WindowFromPoint( WND* wndScope, POINT pt, WND **ppWnd )
{
    WND *wndPtr;
    INT hittest = HTERROR;
    POINT xy = pt;

   *ppWnd = NULL;
    wndPtr = wndScope->child;
    //if( wndScope->flags & WIN_MANAGED )
    {
	/* this prevents mouse clicks from going "through" scrollbars in managed mode */
	if( pt.x < wndScope->rectClient.left || pt.x >= wndScope->rectClient.right ||
	    pt.y < wndScope->rectClient.top || pt.y >= wndScope->rectClient.bottom )
	    goto hittest;
    }
    MapWindowPoints( GetDesktopWindow(), wndScope->hwndSelf, &xy, 1 );

    for (;;)
    {
        while (wndPtr)
        {
            /* If point is in window, and window is visible, and it  */
            /* is enabled (or it's a top-level window), then explore */
            /* its children. Otherwise, go to the next window.       */

            if ((wndPtr->dwStyle & WS_VISIBLE) &&
                (!(wndPtr->dwStyle & WS_DISABLED) ||
                 ((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)) &&
                (xy.x >= wndPtr->rectWindow.left) &&
                (xy.x < wndPtr->rectWindow.right) &&
                (xy.y >= wndPtr->rectWindow.top) &&
                (xy.y < wndPtr->rectWindow.bottom))
            {
                *ppWnd = wndPtr;  /* Got a suitable window */

                /* If window is minimized or disabled, return at once */
                if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;
                if (wndPtr->dwStyle & WS_DISABLED) return HTERROR;

                /* If point is not in client area, ignore the children */
                if ((xy.x < wndPtr->rectClient.left) ||
                    (xy.x >= wndPtr->rectClient.right) ||
                    (xy.y < wndPtr->rectClient.top) ||
                    (xy.y >= wndPtr->rectClient.bottom)) break;

                xy.x -= wndPtr->rectClient.left;
                xy.y -= wndPtr->rectClient.top;
                wndPtr = wndPtr->child;
            }
            else wndPtr = wndPtr->next;
        }

hittest:
        /* If nothing found, try the scope window */
        if (!*ppWnd) *ppWnd = wndScope;

        /* Send the WM_NCHITTEST message (only if to the same task) */
        if ((*ppWnd)->hmemTaskQ == GetFastQueue())
	{
            hittest = (INT)SendMessageA( (*ppWnd)->hwndSelf, WM_NCHITTEST, 
						 0, MAKELONG( pt.x, pt.y ) );
	    if (hittest != HTTRANSPARENT) return hittest;  /* Found the window */
	}
	else return HTCLIENT;

        /* If no children found in last search, make point relative to parent */
        if (!wndPtr)
        {
            xy.x += (*ppWnd)->rectClient.left;
            xy.y += (*ppWnd)->rectClient.top;
        }

        /* Restart the search from the next sibling */
        wndPtr = (*ppWnd)->next;
        *ppWnd = (*ppWnd)->parent;
    }
}

/*******************************************************************
 *         WINPOS_GetWinOffset
 *
 * Calculate the offset between the origin of the two windows. Used
 * to implement MapWindowPoints.
 */
void WINPOS_GetWinOffset( HWND hwndFrom, HWND hwndTo,
                                 POINT *offset )
{
    WND * wndPtr;

    offset->x = offset->y = 0;
    if (hwndFrom == hwndTo ) return;

      /* Translate source window origin to screen coords */
    if (hwndFrom)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwndFrom )))
        {
            //ERR(win,"bad hwndFrom = %04x\n",hwndFrom);
            return;
        }
        while (wndPtr->parent)
        {
            offset->x += wndPtr->rectClient.left;
            offset->y += wndPtr->rectClient.top;
            wndPtr = wndPtr->parent;
        }
    }

      /* Translate origin to destination window coords */
    if (hwndTo)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwndTo )))
        {
            //ERR(win,"bad hwndTo = %04x\n", hwndTo );
            return;
        }
        while (wndPtr->parent)
        {
            offset->x -= wndPtr->rectClient.left;
            offset->y -= wndPtr->rectClient.top;
            wndPtr = wndPtr->parent;
        }    
    }
}

/*******************************************************************
 *         WINPOS_CanActivate
 */
WINBOOL WINPOS_CanActivate(WND* pWnd)
{
    if( pWnd && ((pWnd->dwStyle & (WS_DISABLED | WS_VISIBLE | WS_CHILD)) 
	== WS_VISIBLE) ) return TRUE;
    return FALSE;
}

/***********************************************************************
 *           WINPOS_InitInternalPos
 */
LPINTERNALPOS WINPOS_InitInternalPos( WND* wnd, POINT pt, 
					     LPRECT restoreRect )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS) GetPropA( wnd->hwndSelf,
                                                      atomInternalPos );
    if( !lpPos )
    {
	/* this happens when the window is minimized/maximized 
	 * for the first time (rectWindow is not adjusted yet) */

	lpPos = HeapAlloc( GetProcessHeap(), 0, sizeof(INTERNALPOS) );
	if( !lpPos ) return NULL;

	SetPropA( wnd->hwndSelf, atomInternalPos, (HANDLE)lpPos );
	lpPos->hwndIconTitle = 0; /* defer until needs to be shown */
        memcpy( &lpPos->rectNormal, &wnd->rectWindow ,sizeof(RECT));
	*(UINT*)&lpPos->ptIconPos = *(UINT*)&lpPos->ptMaxPos = 0xFFFFFFFF;
    }


    if( wnd->dwStyle & WS_MINIMIZE ) 
	memcpy( &lpPos->ptIconPos, &pt,sizeof(POINT) );
    else if( wnd->dwStyle & WS_MAXIMIZE ) 
	memcpy( &lpPos->ptMaxPos, &pt,sizeof(POINT) );
    else if( restoreRect ) 
	memcpy( &lpPos->rectNormal,  restoreRect, sizeof(RECT) );

    return lpPos;
}

/***********************************************************************
 *           WINPOS_RedrawIconTitle
 */
WINBOOL WINPOS_RedrawIconTitle( HWND hWnd )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS)GetPropA( hWnd, atomInternalPos );
    if( lpPos )
    {
	if( lpPos->hwndIconTitle )
	{
	    SendMessageA( lpPos->hwndIconTitle, WM_SHOWWINDOW, TRUE, 0);
	    InvalidateRect( lpPos->hwndIconTitle, NULL, TRUE );
	    return TRUE;
	}
    }
    return FALSE;
}

/***********************************************************************
 *           WINPOS_ShowIconTitle
 */
WINBOOL WINPOS_ShowIconTitle( WND* pWnd, WINBOOL bShow )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS)GetPropA( pWnd->hwndSelf, atomInternalPos );

    if( lpPos )
    {
	HWND hWnd = lpPos->hwndIconTitle;

	//DPRINT("0x%04x %i\n", pWnd->hwndSelf, (bShow != 0) );

	if( !hWnd )
	    lpPos->hwndIconTitle = hWnd = ICONTITLE_Create( pWnd );
	if( bShow )
        {
	    pWnd = WIN_FindWndPtr(hWnd);

	    if( !(pWnd->dwStyle & WS_VISIBLE) )
	    {
		SendMessageA( hWnd, WM_SHOWWINDOW, TRUE, 0 );
		SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
			        SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
	    }
	}
	else ShowWindow( hWnd, SW_HIDE );
    }
    return FALSE;
}

/*******************************************************************
 *           WINPOS_GetMinMaxInfo
 *
 * Get the minimized and maximized information for a window.
 */
void WINPOS_GetMinMaxInfo( WND *wndPtr, POINT *maxSize, POINT *maxPos,
			   POINT *minTrack, POINT *maxTrack )
{
    LPINTERNALPOS lpPos;
    MINMAXINFO MinMax;
    INT xinc, yinc;

    /* Compute default values */

    MinMax.ptMaxSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxSize.y = SYSMETRICS_CYSCREEN;
    MinMax.ptMinTrackSize.x = SYSMETRICS_CXMINTRACK;
    MinMax.ptMinTrackSize.y = SYSMETRICS_CYMINTRACK;
    MinMax.ptMaxTrackSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxTrackSize.y = SYSMETRICS_CYSCREEN;

    //if (wndPtr->flags & WIN_MANAGED) 
	xinc = yinc = 0;
    //else 
    if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        xinc = SYSMETRICS_CXDLGFRAME;
        yinc = SYSMETRICS_CYDLGFRAME;
    }
    else
    {
        xinc = yinc = 0;
        if (HAS_THICKFRAME(wndPtr->dwStyle))
        {
            xinc += SYSMETRICS_CXFRAME;
            yinc += SYSMETRICS_CYFRAME;
        }
        if (wndPtr->dwStyle & WS_BORDER)
        {
            xinc += SYSMETRICS_CXBORDER;
            yinc += SYSMETRICS_CYBORDER;
        }
    }
    MinMax.ptMaxSize.x += 2 * xinc;
    MinMax.ptMaxSize.y += 2 * yinc;

    lpPos = (LPINTERNALPOS)GetPropA( wndPtr->hwndSelf, atomInternalPos );
    if( lpPos && !EMPTYPOINT(lpPos->ptMaxPos) )
	memcpy( &MinMax.ptMaxPosition, &lpPos->ptMaxPos,sizeof(POINT) );
    else
    {
        MinMax.ptMaxPosition.x = -xinc;
        MinMax.ptMaxPosition.y = -yinc;
    }

    MSG_SendMessage( wndPtr, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax );

      /* Some sanity checks */

   
    MinMax.ptMaxTrackSize.x = MAX( MinMax.ptMaxTrackSize.x,
                                   MinMax.ptMinTrackSize.x );
    MinMax.ptMaxTrackSize.y = MAX( MinMax.ptMaxTrackSize.y,
                                   MinMax.ptMinTrackSize.y );

    if (maxSize) *maxSize = MinMax.ptMaxSize;
    if (maxPos) *maxPos = MinMax.ptMaxPosition;
    if (minTrack) *minTrack = MinMax.ptMinTrackSize;
    if (maxTrack) *maxTrack = MinMax.ptMaxTrackSize;
}

/***********************************************************************
 *           WINPOS_MinMaximize
 *
 * Fill in lpRect and return additional flags to be used with SetWindowPos().
 * This function assumes that 'cmd' is different from the current window
 * state.
 */
UINT WINPOS_MinMaximize( WND* wndPtr, UINT cmd, LPRECT lpRect )
{
    UINT swpFlags = 0;
    POINT size = { wndPtr->rectWindow.left, wndPtr->rectWindow.top };
    LPINTERNALPOS lpPos = WINPOS_InitInternalPos( wndPtr, size,
                                                  &wndPtr->rectWindow );

    
    //DPRINT("0x%04x %u\n", wndPtr->hwndSelf, cmd );


    if (HOOK_CallHooks(WH_CBT, HCBT_MINMAX, (INT)wndPtr->hwndSelf, cmd, wndPtr->class->bUnicode )) {
	swpFlags |= SWP_NOSIZE | SWP_NOMOVE;
        return swpFlags;
    }
   
  		

    if (lpPos)
    {
	if( wndPtr->dwStyle & WS_MINIMIZE )
	{
	    if( !MSG_SendMessage( wndPtr->hwndSelf, WM_QUERYOPEN, 0, 0L ) )
		return (SWP_NOSIZE | SWP_NOMOVE);
	    swpFlags |= SWP_NOCOPYBITS;
	}
	switch( cmd )
	{
	    case SW_MINIMIZE:
		 if( wndPtr->dwStyle & WS_MAXIMIZE)
		 {
		     wndPtr->flags |= WIN_RESTORE_MAX;
		     wndPtr->dwStyle &= ~WS_MAXIMIZE;
                 }
                 else
		     wndPtr->flags &= ~WIN_RESTORE_MAX;
		 wndPtr->dwStyle |= WS_MINIMIZE;

		 lpPos->ptIconPos = WINPOS_FindIconPos( wndPtr, lpPos->ptIconPos );

		 SetRect( lpRect, lpPos->ptIconPos.x, lpPos->ptIconPos.y,
				    SYSMETRICS_CXICON, SYSMETRICS_CYICON );
		 swpFlags |= SWP_NOCOPYBITS;
		 break;

	    case SW_MAXIMIZE:

                WINPOS_GetMinMaxInfo( wndPtr, &size, &lpPos->ptMaxPos, NULL, NULL );

		 if( wndPtr->dwStyle & WS_MINIMIZE )
		 {
		     WINPOS_ShowIconTitle( wndPtr, FALSE );
		     wndPtr->dwStyle &= ~WS_MINIMIZE;
		 }
                 wndPtr->dwStyle |= WS_MAXIMIZE;

		 SetRect( lpRect, lpPos->ptMaxPos.x, lpPos->ptMaxPos.y,
				    size.x, size.y );
		 break;

	    case SW_RESTORE:
		 if( wndPtr->dwStyle & WS_MINIMIZE )
		 {
		     wndPtr->dwStyle &= ~WS_MINIMIZE;
		     WINPOS_ShowIconTitle( wndPtr, FALSE );
		     if( wndPtr->flags & WIN_RESTORE_MAX)
		     {
			 /* Restore to maximized position */
                         WINPOS_GetMinMaxInfo( wndPtr, &size,&lpPos->ptMaxPos, NULL, NULL);
			 wndPtr->dwStyle |= WS_MAXIMIZE;
			 SetRect( lpRect, lpPos->ptMaxPos.x, lpPos->ptMaxPos.y, size.x, size.y );
			 break;
		     }
		 } 
		 else 
		     if( !(wndPtr->dwStyle & WS_MAXIMIZE) ) return (UINT)(-1);
 		     else wndPtr->dwStyle &= ~WS_MAXIMIZE;

		 /* Restore to normal position */

		*lpRect = lpPos->rectNormal; 
		 lpRect->right -= lpRect->left; 
		 lpRect->bottom -= lpRect->top;

		 break;
	}
    } else swpFlags |= SWP_NOSIZE | SWP_NOMOVE;
    return swpFlags;
}

/***********************************************************************
 *           WINPOS_SetPlacement
 */
WINBOOL WINPOS_SetPlacement( HWND hwnd, const WINDOWPLACEMENT *wndpl,
						UINT flags )
{
    WND *pWnd = WIN_FindWndPtr( hwnd );
    if( pWnd )
    {
	LPINTERNALPOS lpPos = (LPINTERNALPOS)WINPOS_InitInternalPos( pWnd,
			     *(LPPOINT)&pWnd->rectWindow.left, &pWnd->rectWindow );

	if( flags & PLACE_MIN ) lpPos->ptIconPos = wndpl->ptMinPosition;
	if( flags & PLACE_MAX ) lpPos->ptMaxPos = wndpl->ptMaxPosition;
	if( flags & PLACE_RECT) lpPos->rectNormal = wndpl->rcNormalPosition;

	if( pWnd->dwStyle & WS_MINIMIZE )
	{
	    WINPOS_ShowIconTitle( pWnd, FALSE );
	    if( wndpl->flags & WPF_SETMINPOSITION && !EMPTYPOINT(lpPos->ptIconPos))
		SetWindowPos( hwnd, 0, lpPos->ptIconPos.x, lpPos->ptIconPos.y,
				0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	} 
	else if( pWnd->dwStyle & WS_MAXIMIZE )
	{
	    if( !EMPTYPOINT(lpPos->ptMaxPos) )
		SetWindowPos( hwnd, 0, lpPos->ptMaxPos.x, lpPos->ptMaxPos.y,
				0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	}
	else if( flags & PLACE_RECT )
		SetWindowPos( hwnd, 0, lpPos->rectNormal.left, lpPos->rectNormal.top,
				lpPos->rectNormal.right - lpPos->rectNormal.left,
				lpPos->rectNormal.bottom - lpPos->rectNormal.top,
				SWP_NOZORDER | SWP_NOACTIVATE );

	ShowWindow( hwnd, wndpl->showCmd );
	if( IsWindow(hwnd) && pWnd->dwStyle & WS_MINIMIZE )
	{
	    if( pWnd->dwStyle & WS_VISIBLE ) WINPOS_ShowIconTitle( pWnd, TRUE );

	    /* SDK: ...valid only the next time... */
	    if( wndpl->flags & WPF_RESTORETOMAXIMIZED ) pWnd->flags |= WIN_RESTORE_MAX;
	}
	return TRUE;
    }
    return FALSE;
}

/*******************************************************************
 *	   WINPOS_SetActiveWindow
 *
 * SetActiveWindow() back-end. This is the only function that
 * can assign active status to a window. It must be called only
 * for the top level windows.
 */
WINBOOL WINPOS_SetActiveWindow( HWND hWnd, WINBOOL fMouse, WINBOOL fChangeFocus)
{
    //CBTACTIVATESTRUCT* cbtStruct;
    WND*     wndPtr, *wndTemp;
    //HQUEUE hOldActiveQueue, hNewActiveQueue;
    WORD     wIconized = 0;

    /* paranoid checks */
    if( hWnd == GetDesktopWindow || hWnd == hwndActive ) return 0;

/*  if (wndPtr && (GetFastQueue() != wndPtr->hmemTaskQ))
 *	return 0;
 */
    wndPtr = WIN_FindWndPtr(hWnd);
    //hOldActiveQueue = (pActiveQueue)?pActiveQueue->self : 0;

    if( (wndTemp = WIN_FindWndPtr(hwndActive)) )
	wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
   

#if 0
    /* call CBT hook chain */
    if ((cbtStruct = SEGPTR_NEW(CBTACTIVATESTRUCT16)))
    {
        LRESULT wRet;
        cbtStruct->fMouse     = fMouse;
        cbtStruct->hWndActive = hwndActive;
        wRet = HOOK_CallHooks16( WH_CBT, HCBT_ACTIVATE, (WPARAM16)hWnd,
                                 (LPARAM)SEGPTR_GET(cbtStruct) );
        SEGPTR_FREE(cbtStruct);
        if (wRet) return wRet;
    }
#endif

    /* set prev active wnd to current active wnd and send notification */
    if ((hwndPrevActive = hwndActive) && IsWindow(hwndPrevActive))
    {
        if (!SendMessageA( hwndPrevActive, WM_NCACTIVATE, FALSE, 0 ))
        {
	    //if (GetSysModalWindow16() != hWnd) return 0;
	    /* disregard refusal if hWnd is sysmodal */
        }


	SendMessageA( hwndPrevActive, WM_ACTIVATE,
                        MAKEWPARAM( WA_INACTIVE, wIconized ),
                        (LPARAM)hWnd );


	/* check if something happened during message processing */
	if( hwndPrevActive != hwndActive ) return 0;
    }

    /* set active wnd */
    hwndActive = hWnd;

    /* send palette messages */
    if (hWnd && SendMessage( hWnd, WM_QUERYNEWPALETTE, 0, 0L))
	SendMessage((HWND)-1, WM_PALETTEISCHANGING, (WPARAM)hWnd, 0L );

    /* if prev wnd is minimized redraw icon title */
    if( IsIconic( hwndPrevActive ) ) WINPOS_RedrawIconTitle(hwndPrevActive);

#if DESKTOP
    /* managed windows will get ConfigureNotify event */  
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD) && !(wndPtr->flags & WIN_MANAGED))
    {
	/* check Z-order and bring hWnd to the top */
	for (wndTemp = WIN_GetDesktop()->child; wndTemp; wndTemp = wndTemp->next)
	    if (wndTemp->dwStyle & WS_VISIBLE) break;

	if( wndTemp != wndPtr )
	    SetWindowPos(hWnd, HWND_TOP, 0,0,0,0, 
			   SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
        if (!IsWindow(hWnd)) return 0;
    }

#endif

#if 0
    hNewActiveQueue = wndPtr ? wndPtr->hmemTaskQ : 0;

    /* send WM_ACTIVATEAPP if necessary */
    if (hOldActiveQueue != hNewActiveQueue)
    {
        WND **list, **ppWnd;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                if (!IsWindow( (*ppWnd)->hwndSelf )) continue;

                if ((*ppWnd)->hmemTaskQ == hOldActiveQueue)
                   SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                                   0, QUEUE_GetQueueTask(hNewActiveQueue) );
            }
            HeapFree( SystemHeap, 0, list );
        }

	pActiveQueue = (hNewActiveQueue)
		       ? (MESSAGEQUEUE*) GlobalLock16(hNewActiveQueue) : NULL;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                if (!IsWindow( (*ppWnd)->hwndSelf )) continue;

                if ((*ppWnd)->hmemTaskQ == hNewActiveQueue)
                   SendMessage( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                                  1, QUEUE_GetQueueTask( hOldActiveQueue ) );
            }
            HeapFree( SystemHeap, 0, list );
        }
	if (!IsWindow(hWnd)) return 0;
    }

#endif
    if (hWnd)
    {
        /* walk up to the first unowned window */
        wndTemp = wndPtr;
        while (wndTemp->owner) wndTemp = wndTemp->owner;
        /* and set last active owned popup */
        wndTemp->hwndLastActive = hWnd;

        wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
        SendMessageA( hWnd, WM_NCACTIVATE, TRUE, 0 );

        SendMessageA( hWnd, WM_ACTIVATE,
		 MAKEWPARAM( (fMouse) ? WA_CLICKACTIVE : WA_ACTIVE, wIconized),
		 (LPARAM)hwndPrevActive );


        if( !IsWindow(hWnd) ) return 0;
    }
#if 0
    /* change focus if possible */
    if( fChangeFocus && GetFocus() )
	if( WIN_GetTopParent(GetFocus()) != hwndActive )
	    FOCUS_SwitchFocus( GetFocus(),
			       (wndPtr && (wndPtr->dwStyle & WS_MINIMIZE))?
			       0:
			       hwndActive
	    );
#endif

    /* if active wnd is minimized redraw icon title */
    if( IsIconic(hwndActive) ) WINPOS_RedrawIconTitle(hwndActive);

    return (hWnd == hwndActive);
}

/*******************************************************************
 *         WINPOS_ActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
WINBOOL WINPOS_ActivateOtherWindow(WND* pWnd)
{
  WINBOOL	bRet = 0;
  WND*  	pWndTo = NULL;

  if( pWnd->hwndSelf == hwndPrevActive )
      hwndPrevActive = 0;

  if( hwndActive != pWnd->hwndSelf && 
    ( hwndActive || QUEUE_IsExitingQueue(pWnd->hmemTaskQ)) )
      return 0;

  if( !(pWnd->dwStyle & WS_POPUP) || !(pWnd->owner) ||
      !WINPOS_CanActivate((pWndTo = WIN_GetTopParentPtr(pWnd->owner))) ) 
  {
      WND* pWndPtr = WIN_GetTopParentPtr(pWnd);

      pWndTo = WIN_FindWndPtr(hwndPrevActive);

      while( !WINPOS_CanActivate(pWndTo) ) 
      {
	 /* by now owned windows should've been taken care of */

	  pWndTo = pWndPtr->next;
	  pWndPtr = pWndTo;
	  if( !pWndTo ) break;
      }
  }

  bRet = WINPOS_SetActiveWindow( pWndTo ? pWndTo->hwndSelf : NULL, FALSE, TRUE );

  /* switch desktop queue to current active */
  if( pWndTo ) WIN_GetDesktop()->hmemTaskQ = pWndTo->hmemTaskQ;

  hwndPrevActive = 0;
  return bRet;  
}

/*******************************************************************
 *	   WINPOS_ChangeActiveWindow
 *
 */
WINBOOL WINPOS_ChangeActiveWindow( HWND hWnd, WINBOOL mouseMsg )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if (!hWnd) return WINPOS_SetActiveWindow( 0, mouseMsg, TRUE );

    if( !wndPtr ) return FALSE;

    /* child windows get WM_CHILDACTIVATE message */
    if( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD )
	return MSG_SendMessage(wndPtr, WM_CHILDACTIVATE, 0, 0L);

        /* owned popups imply owner activation - not sure */
    if ((wndPtr->dwStyle & WS_POPUP) && wndPtr->owner &&
        (wndPtr->owner->dwStyle & WS_VISIBLE ) &&
        !(wndPtr->owner->dwStyle & WS_DISABLED ))
    {
        if (!(wndPtr = wndPtr->owner)) return FALSE;
	hWnd = wndPtr->hwndSelf;
    }

    if( hWnd == hwndActive ) return FALSE;

    if( !WINPOS_SetActiveWindow(hWnd ,mouseMsg ,TRUE) )
	return FALSE;

#if DESKTOP
    /* switch desktop queue to current active */
    if( wndPtr->parent == WIN_GetDesktop())
        WIN_GetDesktop()->hmemTaskQ = wndPtr->hmemTaskQ;
#endif
    return TRUE;
}


/***********************************************************************
 *           WINPOS_SendNCCalcSize
 *
 * Send a WM_NCCALCSIZE message to a window.
 * All parameters are read-only except newClientRect.
 * oldWindowRect, oldClientRect and winpos must be non-NULL only
 * when calcValidRect is TRUE.
 */
LONG WINPOS_SendNCCalcSize( HWND hwnd, WINBOOL calcValidRect,
                            RECT *newWindowRect, RECT *oldWindowRect,
                            RECT *oldClientRect, WINDOWPOS *winpos,
                            RECT *newClientRect )
{
    NCCALCSIZE_PARAMS params;
    WINDOWPOS winposCopy;
    LONG result;

    params.rgrc[0] = *newWindowRect;
    if (calcValidRect)
    {
        winposCopy = *winpos;
	params.rgrc[1] = *oldWindowRect;
	params.rgrc[2] = *oldClientRect;
	params.lppos = &winposCopy;
    }
    result = SendMessageA( hwnd, WM_NCCALCSIZE, calcValidRect,
                             (LPARAM)&params );
    
    *newClientRect = params.rgrc[0];
    return result;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChangingA( WND *wndPtr, WINDOWPOS *winpos )
{
    POINT maxSize, minTrack;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	WINPOS_GetMinMaxInfo( wndPtr, &maxSize, NULL, &minTrack, NULL );
	if (maxSize.x < winpos->cx) winpos->cx = maxSize.x;
	if (maxSize.y < winpos->cy) winpos->cy = maxSize.y;
	if (!(wndPtr->dwStyle & WS_MINIMIZE))
	{
	    if (winpos->cx < minTrack.x ) winpos->cx = minTrack.x;
	    if (winpos->cy < minTrack.y ) winpos->cy = minTrack.y;
	}
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging( WND *wndPtr, WINDOWPOS *winpos )
{
    POINT maxSize;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	WINPOS_GetMinMaxInfo( wndPtr, &maxSize, NULL, NULL, NULL );
	winpos->cx = MIN( winpos->cx, maxSize.x );
	winpos->cy = MIN( winpos->cy, maxSize.y );
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_MoveWindowZOrder
 *
 * Move a window in Z order, invalidating everything that needs it.
 * Only necessary for windows without associated X window.
 */
void WINPOS_MoveWindowZOrder( HWND hwnd, HWND hwndAfter )
{
    WINBOOL movingUp;
    WND *pWndAfter, *pWndCur, *wndPtr = WIN_FindWndPtr( hwnd );

    /* We have two possible cases:
     * - The window is moving up: we have to invalidate all areas
     *   of the window that were covered by other windows
     * - The window is moving down: we have to invalidate areas
     *   of other windows covered by this one.
     */

    if (hwndAfter == HWND_TOP)
    {
        movingUp = TRUE;
    }
    else if (hwndAfter == HWND_BOTTOM)
    {
        if (!wndPtr->next) return;  /* Already at the bottom */
        movingUp = FALSE;
    }
    else
    {
        if (!(pWndAfter = WIN_FindWndPtr( hwndAfter ))) return;
        if (wndPtr->next == pWndAfter) return;  /* Already placed right */

          /* Determine which window we encounter first in Z-order */
        pWndCur = wndPtr->parent->child;
        while ((pWndCur != wndPtr) && (pWndCur != pWndAfter))
            pWndCur = pWndCur->next;
        movingUp = (pWndCur == pWndAfter);
    }

    if (movingUp)
    {
        WND *pWndPrevAfter = wndPtr->next;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        pWndCur = wndPtr->next;
        while (pWndCur != pWndPrevAfter)
        {
            RECT rect = { pWndCur->rectWindow.left,
			    pWndCur->rectWindow.top,
			    pWndCur->rectWindow.right,
			    pWndCur->rectWindow.bottom };
            OffsetRect( &rect, -wndPtr->rectClient.left,
                          -wndPtr->rectClient.top );
            PAINT_RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                              RDW_FRAME | RDW_ERASE, 0 );
            pWndCur = pWndCur->next;
        }
    }
    else  /* Moving down */
    {
        pWndCur = wndPtr->next;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        while (pWndCur != wndPtr)
        {
            RECT rect = { pWndCur->rectWindow.left,
                            pWndCur->rectWindow.top,
                            pWndCur->rectWindow.right,
                            pWndCur->rectWindow.bottom };
            OffsetRect( &rect, -pWndCur->rectClient.left,
                          -pWndCur->rectClient.top );
            PAINT_RedrawWindow( pWndCur->hwndSelf, &rect, 0, RDW_INVALIDATE |
                              RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE, 0 );
            pWndCur = pWndCur->next;
        }
    }
}

/***********************************************************************
 *           WINPOS_ReorderOwnedPopups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 */
HWND WINPOS_ReorderOwnedPopups(HWND hwndInsertAfter,WND* wndPtr,WORD flags)
{
 WND* 	w = WIN_GetDesktop()->child;

  if( wndPtr->dwStyle & WS_POPUP && wndPtr->owner )
  {
   /* implement "local z-order" between the top and owner window */

     HWND hwndLocalPrev = HWND_TOP;

     if( hwndInsertAfter != HWND_TOP )
     {
	while( w != wndPtr->owner )
	{
          if (w != wndPtr) hwndLocalPrev = w->hwndSelf;
	  if( hwndLocalPrev == hwndInsertAfter ) break;
	  w = w->next;
	}
	hwndInsertAfter = hwndLocalPrev;
     }

  }
  else if( wndPtr->dwStyle & WS_CHILD ) return hwndInsertAfter;

  w = WIN_GetDesktop()->child;
  while( w )
  {
    if( w == wndPtr ) break;

    if( w->dwStyle & WS_POPUP && w->owner == wndPtr )
    {
      SetWindowPos(w->hwndSelf, hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                     SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_DEFERERASE);
      hwndInsertAfter = w->hwndSelf;
    }
    w = w->next;
  }

  return hwndInsertAfter;
}

/***********************************************************************
 *	     WINPOS_SizeMoveClean
 *
 * Make window look nice without excessive repainting
 *
 * the pain:
 *
 * visible regions are in window coordinates
 * update regions are in window client coordinates
 * client and window rectangles are in parent client coordinates
 *
 * FIXME: Move visible and update regions to the same coordinate system
 *	 (either parent client or window). This is a lot of work though.
 */
UINT WINPOS_SizeMoveClean( WND* Wnd, HRGN oldVisRgn,
                                    LPRECT lpOldWndRect,
                                    LPRECT lpOldClientRect, UINT uFlags )
{
 HRGN newVisRgn = DCE_GetVisRgn(Wnd->hwndSelf,DCX_WINDOW | DCX_CLIPSIBLINGS);
 HRGN dirtyRgn = CreateRectRgn(0,0,0,0);
 int  other, my;

 

 if( (lpOldWndRect->right - lpOldWndRect->left) != (Wnd->rectWindow.right - Wnd->rectWindow.left) ||
     (lpOldWndRect->bottom - lpOldWndRect->top) != (Wnd->rectWindow.bottom - Wnd->rectWindow.top) )
     uFlags |= SMC_DRAWFRAME;

 CombineRgn( dirtyRgn, newVisRgn, 0, RGN_COPY);

 if( !(uFlags & SMC_NOCOPY) )
   CombineRgn( newVisRgn, newVisRgn, oldVisRgn, RGN_AND ); 

 /* map regions to the parent client area */
 
 OffsetRgn( dirtyRgn, Wnd->rectWindow.left, Wnd->rectWindow.top );
 OffsetRgn( oldVisRgn, lpOldWndRect->left, lpOldWndRect->top );

 /* compute invalidated region outside Wnd - (in client coordinates of the parent window) */

 other = CombineRgn(dirtyRgn, oldVisRgn, dirtyRgn, RGN_DIFF);

 /* map visible region to the Wnd client area */

 OffsetRgn( newVisRgn, Wnd->rectWindow.left - Wnd->rectClient.left,
                         Wnd->rectWindow.top - Wnd->rectClient.top );

 /* substract previously invalidated region from the Wnd visible region */

 my =  (Wnd->hrgnUpdate > 1) ? CombineRgn( newVisRgn, newVisRgn,
                                             Wnd->hrgnUpdate, RGN_DIFF)
                             : COMPLEXREGION;

 if( uFlags & SMC_NOCOPY )	/* invalidate Wnd visible region */
   {
     if (my != NULLREGION)
	 PAINT_RedrawWindow( Wnd->hwndSelf, NULL, newVisRgn, RDW_INVALIDATE |
	  RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
     else if(uFlags & SMC_DRAWFRAME)
	 Wnd->flags |= WIN_NEEDS_NCPAINT;
   }
 else			/* bitblt old client area */
   { 
     HDC hDC;
     int   update;
     HRGN updateRgn;
     int   xfrom,yfrom,xto,yto,width,height;

     if( uFlags & SMC_DRAWFRAME )
       {
	 /* copy only client area, frame will be redrawn anyway */

         xfrom = lpOldClientRect->left; yfrom = lpOldClientRect->top;
         xto = Wnd->rectClient.left; yto = Wnd->rectClient.top;
         width = lpOldClientRect->right - xfrom; height = lpOldClientRect->bottom - yfrom;
	 updateRgn = CreateRectRgn( 0, 0, width, height );
	 CombineRgn( newVisRgn, newVisRgn, updateRgn, RGN_AND );
	 SetRectRgn( updateRgn, 0, 0, Wnd->rectClient.right - xto,
                       Wnd->rectClient.bottom - yto );
       }
     else
       {
         xfrom = lpOldWndRect->left; yfrom = lpOldWndRect->top;
         xto = Wnd->rectWindow.left; yto = Wnd->rectWindow.top;
         width = lpOldWndRect->right - xfrom; height = lpOldWndRect->bottom - yfrom;
	 updateRgn = CreateRectRgn( xto - Wnd->rectClient.left,
				      yto - Wnd->rectClient.top,
				Wnd->rectWindow.right - Wnd->rectClient.left,
			        Wnd->rectWindow.bottom - Wnd->rectClient.top );
       }

     CombineRgn( newVisRgn, newVisRgn, updateRgn, RGN_AND );

     /* substract new visRgn from target rect to get a region that won't be copied */

     update = CombineRgn( updateRgn, updateRgn, newVisRgn, RGN_DIFF );

     /* Blt valid bits using parent window DC */

     if( Wnd->parent && my != NULLREGION && (xfrom != xto || yfrom != yto) )
       {
	 
	 /* compute clipping region in parent client coordinates */

	 OffsetRgn( newVisRgn, Wnd->rectClient.left, Wnd->rectClient.top );
	 CombineRgn( oldVisRgn, oldVisRgn, newVisRgn, RGN_OR );

// REMOVED DCX_KEEPCLIPRGN 

         hDC = GetDCEx( Wnd->parent->hwndSelf, oldVisRgn,
                           DCX_INTERSECTRGN | DCX_CACHE | DCX_CLIPSIBLINGS);
         
         BitBlt( hDC, xto, yto, width, height, hDC, xfrom, yfrom, SRCCOPY );
         ReleaseDC( Wnd->parent->hwndSelf, hDC); 

       }

     if( update != NULLREGION )
         PAINT_RedrawWindow( Wnd->hwndSelf, NULL, updateRgn, RDW_INVALIDATE |
                         RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
     else if( uFlags & SMC_DRAWFRAME ) Wnd->flags |= WIN_NEEDS_NCPAINT;
     DeleteObject( updateRgn );
   }

 /* erase uncovered areas */

 if( !(uFlags & SMC_NOPARENTERASE) && (other != NULLREGION ) )
      PAINT_RedrawWindow( Wnd->parent->hwndSelf, NULL, dirtyRgn,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
 DeleteObject(dirtyRgn);
 DeleteObject(newVisRgn);
 return uFlags;
}

