#include <windows.h>
#include <user32/dce.h>
#include <user32/win.h>

DCE *firstDCE = 0;
HDC defaultDCstate = 0;

/***********************************************************************
 *           DCE_AllocDCE
 *
 * Allocate a new DCE.
 */
DCE *DCE_AllocDCE( HWND hWnd, DCE_TYPE type )
{
    DCE * dce;
    if (!(dce = HeapAlloc( GetProcessHeap(), 0, sizeof(DCE) ))) return NULL;
    if (!(dce->hDC = CreateDC( "DISPLAY", NULL, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, dce );
	return 0;
    }

    /* store DCE handle in DC hook data field */

    //SetDCHook( dce->hDC, (FARPROC)DCHook, (DWORD)dce );

    dce->hwndCurrent = hWnd;
    dce->hClipRgn    = 0;
    dce->next        = firstDCE;
    firstDCE = dce;

    if( type != DCE_CACHE_DC ) /* owned or class DC */
    {
	dce->DCXflags = DCX_DCEBUSY;
	if( hWnd )
	{
	    WND* wnd = WIN_FindWndPtr(hWnd);
	
	    if( wnd->dwStyle & WS_CLIPCHILDREN ) dce->DCXflags |= DCX_CLIPCHILDREN;
	    if( wnd->dwStyle & WS_CLIPSIBLINGS ) dce->DCXflags |= DCX_CLIPSIBLINGS;
	}
	//SetHookFlags(dce->hDC,DCHF_INVALIDATEVISRGN);
    }
    else dce->DCXflags = DCX_CACHE | DCX_DCEEMPTY;

    return dce;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
DCE* DCE_FreeDCE( DCE *dce )
{
    DCE **ppDCE = &firstDCE;

    if (!dce) return NULL;
    while (*ppDCE && (*ppDCE != dce)) ppDCE = &(*ppDCE)->next;
    if (*ppDCE == dce) *ppDCE = dce->next;

 //   SetDCHook(dce->hDC, NULL, 0L);

    DeleteDC( dce->hDC );
    if( dce->hClipRgn && !(dce->DCXflags & DCX_KEEPCLIPRGN) )
	DeleteObject(dce->hClipRgn);
    HeapFree( GetProcessHeap(), 0, dce );
    return *ppDCE;
}

/***********************************************************************
 *   DCE_DeleteClipRgn
 */
void DCE_DeleteClipRgn( DCE* dce )
{
    dce->DCXflags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

    if( dce->DCXflags & DCX_KEEPCLIPRGN )
	dce->DCXflags &= ~DCX_KEEPCLIPRGN;
    else
	if( dce->hClipRgn > 1 )
	    DeleteObject( dce->hClipRgn );

    dce->hClipRgn = 0;

    //TRACE(dc,"\trestoring VisRgn\n");

    RestoreVisRgn(dce->hDC);
   
}

/***********************************************************************
 *           DCE_GetVisRect
 *
 * Calculate the visible rectangle of a window (i.e. the client or
 * window area clipped by the client area of all ancestors) in the
 * corresponding coordinates. Return FALSE if the visible region is empty.
 */
WINBOOL DCE_GetVisRect( WND *wndPtr, WINBOOL clientArea, RECT *lprect )
{
    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
	INT xoffset = lprect->left;
	INT yoffset = lprect->top;

	while (wndPtr->dwStyle & WS_CHILD)
	{
	    wndPtr = wndPtr->parent;

	    if ( (wndPtr->dwStyle & (WS_ICONIC | WS_VISIBLE)) != WS_VISIBLE )
		goto fail;

	    xoffset += wndPtr->rectClient.left;
	    yoffset += wndPtr->rectClient.top;
	    OffsetRect( lprect, wndPtr->rectClient.left,
				  wndPtr->rectClient.top );

	    if( (wndPtr->rectClient.left >= wndPtr->rectClient.right) ||
		(wndPtr->rectClient.top >= wndPtr->rectClient.bottom) ||
		(lprect->left >= wndPtr->rectClient.right) ||
		(lprect->right <= wndPtr->rectClient.left) ||
		(lprect->top >= wndPtr->rectClient.bottom) ||
		(lprect->bottom <= wndPtr->rectClient.top) )
		goto fail;

	    lprect->left = max( lprect->left, wndPtr->rectClient.left );
	    lprect->right = min( lprect->right, wndPtr->rectClient.right );
	    lprect->top = max( lprect->top, wndPtr->rectClient.top );
	    lprect->bottom = min( lprect->bottom, wndPtr->rectClient.bottom );
	}
	OffsetRect( lprect, -xoffset, -yoffset );
	return TRUE;
    }

fail:
    SetRectEmpty( lprect );
    return FALSE;
}


/***********************************************************************
 *           DCE_AddClipRects
 *
 * Go through the linked list of windows from pWndStart to pWndEnd,
 * adding to the clip region the intersection of the target rectangle
 * with an offset window rectangle.
 */
WINBOOL DCE_AddClipRects( WND *pWndStart, WND *pWndEnd, 
			        HRGN hrgnClip, LPRECT lpRect, int x, int y )
{
    RECT rect;

//    if( pWndStart->pDriver->pIsSelfClipping( pWndStart ) )
  //      return TRUE; /* The driver itself will do the clipping */

    for (; pWndStart != pWndEnd; pWndStart = pWndStart->next)
    {
	if( !(pWndStart->dwStyle & WS_VISIBLE) ) continue;
	    
	rect.left = pWndStart->rectWindow.left + x;
	rect.top = pWndStart->rectWindow.top + y;
	rect.right = pWndStart->rectWindow.right + x;
	rect.bottom = pWndStart->rectWindow.bottom + y;

	if( IntersectRect( &rect, &rect, lpRect ))
	    if(!REGION_UnionRectWithRgn( hrgnClip, &rect ))
		break;
    }
    return (pWndStart == pWndEnd);
}



/***********************************************************************
 *           DCE_GetVisRgn
 *
 * Return the visible region of a window, i.e. the client or window area
 * clipped by the client area of all ancestors, and then optionally
 * by siblings and children.
 */
HRGN DCE_GetVisRgn( HWND hwnd, WORD flags )
{
    HRGN hrgnVis = 0;
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    /* Get visible rectangle and create a region with it. */

    if (wndPtr && DCE_GetVisRect(wndPtr, !(flags & DCX_WINDOW), &rect))
    {
	if((hrgnVis = CreateRectRgnIndirect( &rect )))
        {
	    HRGN hrgnClip = CreateRectRgn( 0, 0, 0, 0 );
	    INT xoffset, yoffset;

	    if( hrgnClip )
	    {
		/* Compute obscured region for the visible rectangle by 
		 * clipping children, siblings, and ancestors. Note that
		 * DCE_GetVisRect() returns a rectangle either in client
		 * or in window coordinates (for DCX_WINDOW request). */

		if( (flags & DCX_CLIPCHILDREN) && wndPtr->child )
		{
		    if( flags & DCX_WINDOW )
		    {
			/* adjust offsets since child window rectangles are 
			 * in client coordinates */

			xoffset = wndPtr->rectClient.left - wndPtr->rectWindow.left;
			yoffset = wndPtr->rectClient.top - wndPtr->rectWindow.top;
		    }
		    else 
			xoffset = yoffset = 0;

		    DCE_AddClipRects( wndPtr->child, NULL, hrgnClip, 
				      &rect, xoffset, yoffset );
		}

		/* sibling window rectangles are in client 
		 * coordinates of the parent window */

		if (flags & DCX_WINDOW)
		{
		    xoffset = -wndPtr->rectWindow.left;
		    yoffset = -wndPtr->rectWindow.top;
		}
		else
		{
		    xoffset = -wndPtr->rectClient.left;
		    yoffset = -wndPtr->rectClient.top;
		}

		if (flags & DCX_CLIPSIBLINGS && wndPtr->parent )
		    DCE_AddClipRects( wndPtr->parent->child,
				      wndPtr, hrgnClip, &rect, xoffset, yoffset );

		/* Clip siblings of all ancestors that have the
                 * WS_CLIPSIBLINGS style
		 */

		while (wndPtr->dwStyle & WS_CHILD)
		{
		    wndPtr = wndPtr->parent;
		    xoffset -= wndPtr->rectClient.left;
		    yoffset -= wndPtr->rectClient.top;
		    if(wndPtr->dwStyle & WS_CLIPSIBLINGS && wndPtr->parent)
		    {
			DCE_AddClipRects( wndPtr->parent->child, wndPtr,
					  hrgnClip, &rect, xoffset, yoffset );
		    }
		}

		/* Now once we've got a jumbo clip region we have
		 * to substract it from the visible rectangle.
	         */

		CombineRgn( hrgnVis, hrgnVis, hrgnClip, RGN_DIFF );
		DeleteObject( hrgnClip );
	    }
	    else
	    {
		DeleteObject( hrgnVis );
		hrgnVis = 0;
	    }
	}
    }
    else
	hrgnVis = CreateRectRgn(0, 0, 0, 0); /* empty */
    return hrgnVis;
}

/***********************************************************************
 *           DCE_OffsetVisRgn
 *
 * Change region from DC-origin relative coordinates to screen coords.
 */

void DCE_OffsetVisRgn( HDC hDC, HRGN hVisRgn )
{
/*
    DC *dc;
    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return;

    OffsetRgn( hVisRgn, dc->w.DCOrgX, dc->w.DCOrgY );

    GDI_HEAP_UNLOCK( hDC );
*/
}



/***********************************************************************
 *           DCE_ExcludeRgn
 * 
 *  Translate given region from the wnd client to the DC coordinates
 *  and add it to the clipping region.
 */
INT DCE_ExcludeRgn( HDC hDC, WND* wnd, HRGN hRgn )
{
  POINT  pt = {0, 0};
  DCE     *dce = firstDCE;

  while (dce && (dce->hDC != hDC)) dce = dce->next;
  if( dce )
  {
      MapWindowPoints( wnd->hwndSelf, dce->hwndCurrent, &pt, 1);
      if( dce->DCXflags & DCX_WINDOW )
      { 
	  wnd = WIN_FindWndPtr(dce->hwndCurrent);
	  pt.x += wnd->rectClient.left - wnd->rectWindow.left;
	  pt.y += wnd->rectClient.top - wnd->rectWindow.top;
      }
  }
  else return ERROR;
  OffsetRgn(hRgn, pt.x, pt.y);

  return ExtSelectClipRgn( hDC, hRgn, RGN_DIFF );
}