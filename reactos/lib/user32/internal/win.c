
#include <windows.h>
#include <user32/win.h>
#include <user32/class.h>
#include <user32/menu.h>
#include <user32/winpos.h>
#include <user32/hook.h>
#include <user32/property.h>
#include <user32/dce.h>
#include <user32/caret.h>
#include <user32/debug.h>
#include <user32/heapdup.h>
#include <user32/dialog.h>

WND *rootWnd;
//////////////////////////////////////////////////////////////////////////////////

WND *pWndDesktop;

HANDLE WIN_CreateWindowEx( CREATESTRUCTW *cs, ATOM classAtom)
{
    HANDLE hWnd;
    WND *wndPtr;
    STARTUPINFO StartupInfo;
    CLASS *classPtr = NULL;
    POINT maxSize, maxPos, minTrack, maxTrack;
    HWND  hWndLinkAfter = NULL;
 /* Create the window structure */

 

    classPtr = CLASS_FindClassByAtom(classAtom,cs->hInstance);
    if ( classPtr == NULL )
	return NULL;


    if (!(wndPtr = HeapAlloc(GetProcessHeap(),0, sizeof(WND) + classPtr->cbWndExtra
                                  - sizeof(classPtr->wExtra) )))
    {
	return NULL;
    }

    

    wndPtr->next        = rootWnd;
    rootWnd = wndPtr;

    wndPtr->class       = classPtr;

//    if ( CreatePipe(&(wndPtr->hmemTaskQ), &(wndPtr->hwndSelf),NULL,4096) == FALSE )
//	return -1;

    
    wndPtr->hwndSelf = wndPtr;
    hWnd = wndPtr->hwndSelf;

    /* Fill the window structure */

    wndPtr->next  = NULL;
    wndPtr->child = NULL;

    if ((cs->style & WS_CHILD) && cs->hWndParent)
    {
        wndPtr->parent = WIN_FindWndPtr( cs->hWndParent );
        wndPtr->owner  = NULL;
    }
    else {
	wndPtr->owner = NULL;
	wndPtr->parent = NULL;
    }
	
/*
    else
    {
        wndPtr->parent = pWndDesktop;
        if (!cs->hWndParent || (cs->hWndParent == pWndDesktop->hwndSelf))
            wndPtr->owner = NULL;
        else
            wndPtr->owner = WIN_GetTopParentPtr(WIN_FindWndPtr(cs->hWndParent));
    }
 */  
    
    wndPtr->winproc        = classPtr->winproc;
    wndPtr->dwMagic        = WND_MAGIC;
    //wndPtr->hwndSelf       = hWnd;
    wndPtr->hInstance      = cs->hInstance;
    wndPtr->text           = NULL;
    wndPtr->hmemTaskQ      = GetFastQueue();
    wndPtr->hrgnUpdate     = 0;
    wndPtr->hwndLastActive = wndPtr->hwndSelf;
    wndPtr->dwStyle        = cs->style & ~WS_VISIBLE;
    wndPtr->dwExStyle      = cs->dwExStyle;
    wndPtr->wIDmenu        = 0;
    wndPtr->helpContext    = 0;
    wndPtr->flags          = 0;
    wndPtr->pVScroll       = NULL;
    wndPtr->pHScroll       = NULL;
    wndPtr->pProp          = NULL;
    wndPtr->userdata       = 0;
    wndPtr->hSysMenu       = (wndPtr->dwStyle & WS_SYSMENU)
			     ? MENU_GetSysMenu( hWnd, 0 ) : 0;

    if (classPtr->cbWndExtra) 
	HEAP_memset( wndPtr->wExtra, 0, classPtr->cbWndExtra);


    /* Call the WH_CBT hook */

    hWndLinkAfter = ((cs->style & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD) ? HWND_BOTTOM : HWND_TOP;

    if (HOOK_IsHooked( WH_CBT ))
    {
	CBT_CREATEWNDW cbtc;
        LRESULT ret;

	cbtc.lpcs = cs;
	cbtc.hwndInsertAfter = hWndLinkAfter;
        ret = HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (INT)hWnd, (LPARAM)&cbtc,  classPtr->bUnicode);
        if (ret)
	{

	    HeapFree( GetProcessHeap(),0,wndPtr );
	    return NULL;
	}
    }

    /* Increment class window counter */

    classPtr->cWindows++;

    /* Correct the window style */

    if (!(cs->style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
    {
        wndPtr->dwStyle |= WS_CAPTION | WS_CLIPSIBLINGS;
        wndPtr->flags |= WIN_NEED_SIZE;
    }
    if (cs->dwExStyle & WS_EX_DLGMODALFRAME) 
	wndPtr->dwStyle &= ~WS_THICKFRAME;





    GetStartupInfoW((STARTUPINFO *)&StartupInfo);
    if (cs->x == CW_USEDEFAULT) 
    {
        if (   !(cs->style & (WS_CHILD | WS_POPUP))
            &&  (StartupInfo.dwFlags & STARTF_USEPOSITION) )
        {
            cs->x = StartupInfo.dwX;
            cs->y = StartupInfo.dwY;
        }
        else
        {
            cs->x = 0;
            cs->y = 0;
        }
    }
    if (cs->cx == CW_USEDEFAULT)
    {
        
        if (   !(cs->style & (WS_CHILD | WS_POPUP))
            &&  (StartupInfo.dwFlags & STARTF_USESIZE) )
        {
            cs->cx = StartupInfo.dwXSize;
            cs->cy = StartupInfo.dwYSize;
        }
        else
        {
   		cs->cx = SYSMETRICS_CXSCREEN;
    		cs->cy = SYSMETRICS_CYSCREEN;
        }
    }


 

    /* Send the WM_GETMINMAXINFO message and fix the size if needed and appropriate */

    if ( !(cs->style & (WS_POPUP | WS_CHILD) )) {

    	if ((cs->style & WS_THICKFRAME) )
    	{

        	WINPOS_GetMinMaxInfo( wndPtr, &maxSize, &maxPos, &minTrack, &maxTrack);
        	if (maxSize.x < cs->cx) cs->cx = maxSize.x;
        	if (maxSize.y < cs->cy) cs->cy = maxSize.y;
        	if (cs->cx < minTrack.x ) cs->cx = minTrack.x;
        	if (cs->cy < minTrack.y ) cs->cy = minTrack.y;

    	}


    }
    if(cs->style & WS_CHILD)
    {
        if(cs->cx < 0) cs->cx = 0;
        if(cs->cy < 0) cs->cy = 0;
    }
    else
    {
        if (cs->cx <= 0) cs->cx = 1;
        if (cs->cy <= 0) cs->cy = 1;
    }

    wndPtr->rectWindow.left   = cs->x;
    wndPtr->rectWindow.top    = cs->y;
    wndPtr->rectWindow.right  = cs->x + cs->cx;
    wndPtr->rectWindow.bottom = cs->y + cs->cy;

    wndPtr->rectClient= wndPtr->rectWindow;


    printf(":%d %d %d %d\n", wndPtr->rectWindow.left, wndPtr->rectWindow.top,
		wndPtr->rectWindow.right, wndPtr->rectWindow.bottom);

        /* Get class or window DC if needed */

    if (classPtr->style & CS_OWNDC) 
	wndPtr->dce = DCE_AllocDCE(wndPtr,DCE_WINDOW_DC);
    else if (classPtr->style & CS_CLASSDC) 
	wndPtr->dce = classPtr->dce;
    else if ( classPtr->style & CS_PARENTDC)
	wndPtr->dce = wndPtr->parent->dce;
    else wndPtr->dce = DCE_AllocDCE(wndPtr,DCE_WINDOW_DC);;


    //wndPtr->rectClient.top = wndPtr->rectClient.left = 0;
    //wndPtr->rectClient.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    //wndPtr->rectClient.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;


     /* Set the window menu */

    if ((wndPtr->dwStyle & (WS_CAPTION | WS_CHILD)) == WS_CAPTION )
    {
        if (cs->hMenu) 
		SetMenu(hWnd, cs->hMenu);
        else
        {
            if (classPtr->menuName) {
		if ( classPtr->bUnicode == FALSE )
                	cs->hMenu =  LoadMenuA(cs->hInstance,classPtr->menuName);
		else
			cs->hMenu =  LoadMenuW(cs->hInstance,classPtr->menuName);
	    }
        }
    }
    else 
	wndPtr->wIDmenu = (UINT)cs->hMenu;

    /* Send the WM_CREATE message 
     * Perhaps we shouldn't allow width/height changes as well. 
     * See p327 in "Internals". 
     */

    maxPos.x = wndPtr->rectWindow.left;
    maxPos.y = wndPtr->rectWindow.top;


    if( MSG_SendMessage( wndPtr, WM_NCCREATE, 0, (LPARAM)cs) == 0)
    {
	    /* Abort window creation */
	WIN_DestroyWindow( wndPtr );
	return NULL;
    }
   
	
    /* Insert the window in the linked list */

    WIN_LinkWindow( hWnd, hWndLinkAfter );

    WINPOS_SendNCCalcSize( hWnd, FALSE, &wndPtr->rectWindow,
                               NULL, NULL, 0, &wndPtr->rectClient );
    OffsetRect(&wndPtr->rectWindow, maxPos.x - wndPtr->rectWindow.left,
                                          maxPos.y - wndPtr->rectWindow.top);


    if( (MSG_SendMessage( wndPtr, WM_CREATE, 0, (LPARAM)cs)) == -1 )
    {
	WIN_UnlinkWindow( hWnd );
	WIN_DestroyWindow( wndPtr );
	return NULL;
    }
            /* Send the size messages */

    if (!(wndPtr->flags & WIN_NEED_SIZE))
    {
       /* send it anyway */
	if (((wndPtr->rectClient.right-wndPtr->rectClient.left) <0)
		    ||((wndPtr->rectClient.bottom-wndPtr->rectClient.top)<0))
		 
         MSG_SendMessage( wndPtr, WM_SIZE, SIZE_RESTORED,
                                MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                                         wndPtr->rectClient.bottom-wndPtr->rectClient.top));
         MSG_SendMessage( wndPtr, WM_MOVE, 0,
                                MAKELONG( wndPtr->rectClient.left,
                                          wndPtr->rectClient.top) );
    }

            /* Show the window, maximizing or minimizing if needed */

    if (wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE))
    {
	RECT newPos;
	UINT swFlag = (wndPtr->dwStyle & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;
               wndPtr->dwStyle &= ~(WS_MAXIMIZE | WS_MINIMIZE);
	WINPOS_MinMaximize( wndPtr, swFlag, &newPos );
                swFlag = ((wndPtr->dwStyle & WS_CHILD) || GetActiveWindow())
                    ? SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED
                    : SWP_NOZORDER | SWP_FRAMECHANGED;
        SetWindowPos( hWnd, 0, newPos.left, newPos.top,
                                newPos.right, newPos.bottom, swFlag );
    }

    if( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
    {
		/* Notify the parent window only */

		MSG_SendMessage( wndPtr->parent, WM_PARENTNOTIFY,
				MAKEWPARAM(WM_CREATE, wndPtr->wIDmenu), (LPARAM)hWnd );
		if( !IsWindow(hWnd) ) return 0;
    }

    if (cs->style & WS_VISIBLE) 
		ShowWindow( hWnd, SW_SHOW );

            /* Call WH_SHELL hook */

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
                HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWCREATED, (INT)hWnd, 0L,  classPtr->bUnicode);

    return hWnd;
 

}

WINBOOL WIN_IsWindow(HANDLE hWnd)
{	
        if (WIN_FindWndPtr( hWnd ) == NULL) return FALSE;
	return TRUE;
}

/***********************************************************************
 *           WIN_FindWinToRepaint
 *
 * Find a window that needs repaint.
 */
HWND WIN_FindWinToRepaint( HWND hwnd, HQUEUE hQueue )
{
    HWND hwndRet;
    WND *pWnd = pWndDesktop;

    /* Note: the desktop window never gets WM_PAINT messages 
     * The real reason why is because Windows DesktopWndProc
     * does ValidateRgn inside WM_ERASEBKGND handler.
     */

    pWnd = hwnd ? WIN_FindWndPtr( hwnd ) : pWndDesktop->child;

    for ( ; pWnd ; pWnd = pWnd->next )
    {
        if (!(pWnd->dwStyle & WS_VISIBLE))
        {
            DPRINT( "skipping window %04x\n",
                         pWnd->hwndSelf );
            continue;
        }
        if ((pWnd->hmemTaskQ == hQueue) &&
            (pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))) break;
        
        if (pWnd->child )
            if ((hwndRet = WIN_FindWinToRepaint( pWnd->child->hwndSelf, hQueue )) )
                return hwndRet;
    }
    
    if (!pWnd) return 0;
    
    hwndRet = pWnd->hwndSelf;

    /* look among siblings if we got a transparent window */
    while (pWnd && ((pWnd->dwExStyle & WS_EX_TRANSPARENT) ||
                    !(pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))))
    {
        pWnd = pWnd->next;
    }
    if (pWnd) hwndRet = pWnd->hwndSelf;
    DPRINT("found %04x\n",hwndRet);
    return hwndRet;
}

/**********************************************************************
 *	     WIN_GetWindowLong
 *
 * Helper function for GetWindowLong().
 */
LONG WIN_GetWindowLong( HWND hwnd, INT offset )
{
    LONG retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(LONG) > wndPtr->class->cbWndExtra)
        {
            DPRINT( "Invalid offset %d\n", offset );
            return 0;
        }
        retval = *(LONG *)(((char *)wndPtr->wExtra) + offset);

        return retval;
    }
    switch(offset)
    {
        case GWL_USERDATA:   return wndPtr->userdata;
        case GWL_STYLE:      return wndPtr->dwStyle;
        case GWL_EXSTYLE:    return wndPtr->dwExStyle;
        case GWL_ID:         return (LONG)wndPtr->wIDmenu;
        case GWL_WNDPROC:    return (LONG)wndPtr->winproc;
        case GWL_HWNDPARENT: return wndPtr->parent ? (LONG)wndPtr->parent->hwndSelf : (LONG)0;
        case GWL_HINSTANCE:  return (LONG)wndPtr->hInstance;
        default:
            DPRINT( "Unknown offset %d\n", offset );
    }
    return 0;
}


/**********************************************************************
 *	     WIN_SetWindowLong
 *
 * Helper function for SetWindowLong().
 *
 * 0 is the failure code. However, in the case of failure SetLastError
 * must be set to distinguish between a 0 return value and a failure.
 *
 * FIXME: The error values for SetLastError may not be right. Can
 *        someone check with the real thing?
 */
LONG WIN_SetWindowLong( HWND hwnd, INT offset, LONG newval )
{
    LONG *ptr, retval = 0;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    STYLESTRUCT style;
    
    //DPRINT("%x=%p %x %lx %x\n",hwnd, wndPtr, offset, newval, type);

    if (!wndPtr)
    {
       /* Is this the right error? */
       SetLastError( ERROR_INVALID_WINDOW_HANDLE );
       return 0;
    }

    if (offset >= 0)
    {
        if (offset + sizeof(LONG) > wndPtr->class->cbWndExtra)
        {
            DPRINT( "Invalid offset %d\n", offset );

            /* Is this the right error? */
            SetLastError( ERROR_OUTOFMEMORY );

            return 0;
        }
        ptr = (LONG *)(((char *)wndPtr->wExtra) + offset);


        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
        {
		retval = (LONG)ptr;
		*ptr = newval;
            	return (LONG)retval;
        }
    }
    else switch(offset)
    {
	case GWL_ID:
		ptr = (LONG*)&wndPtr->wIDmenu;
		break;
	case GWL_HINSTANCE:
		return (LONG)SetWindowWord( hwnd, offset, newval );
	case GWL_WNDPROC:
		retval = (LONG)wndPtr->winproc;
		wndPtr->winproc = (WNDPROC)newval;
		return retval;
	case GWL_STYLE:
	       	style.styleOld = wndPtr->dwStyle;
		newval &= ~(WS_VISIBLE | WS_CHILD);	/* Some bits can't be changed this way */
		style.styleNew = newval | (style.styleOld & (WS_VISIBLE | WS_CHILD));

		
		MSG_SendMessage(wndPtr,WM_STYLECHANGING,GWL_STYLE,(LPARAM)&style);
		wndPtr->dwStyle = style.styleNew;
		
		MSG_SendMessage(wndPtr,WM_STYLECHANGED,GWL_STYLE,(LPARAM)&style);
		return style.styleOld;
		    
        case GWL_USERDATA: 
		ptr = (LONG *)&wndPtr->userdata; 
		break;
        case GWL_EXSTYLE:  
	        style.styleOld = wndPtr->dwExStyle;
		style.styleNew = newval;
		MSG_SendMessage(wndPtr,WM_STYLECHANGING,GWL_EXSTYLE,(LPARAM)&style);
		wndPtr->dwExStyle = newval;		
		MSG_SendMessage(wndPtr,WM_STYLECHANGED,GWL_EXSTYLE,(LPARAM)&style);
		return style.styleOld;

	default:
            DPRINT( "Invalid offset %d\n", offset );

            /* Don't think this is right error but it should do */
            SetLastError( ERROR_OUTOFMEMORY );

            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 */
WINBOOL WIN_DestroyWindow( WND* wndPtr )
{
    HWND hWnd;
    WND *pWnd;

    if ( wndPtr == NULL )
	return FALSE;

    hWnd = wndPtr->hwndSelf;


	
    /* free child windows */

    while ((pWnd = wndPtr->child))
        if ( !WIN_DestroyWindow( pWnd ) )
		break;

    MSG_SendMessage( wndPtr, WM_NCDESTROY, 0, 0);

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    WINPOS_CheckInternalPos( hWnd );
    if( hWnd == GetCapture()) ReleaseCapture();

    /* free resources associated with the window */

//    TIMER_RemoveWindowTimers( wndPtr->hwndSelf );
    PROPERTY_RemoveWindowProps( wndPtr );

    wndPtr->dwMagic = 0;  /* Mark it as invalid */

    if ((wndPtr->hrgnUpdate) || (wndPtr->flags & WIN_INTERNAL_PAINT))
    {
        if ((UINT)wndPtr->hrgnUpdate > 1) 
		DeleteObject( wndPtr->hrgnUpdate );
        //QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
    }
// changed message queue implementation with pipes
    /* toss stale messages from the queue */
#if 0
    if( wndPtr->hmemTaskQ )
    {
        int           pos;
	WINBOOL	      bPostQuit = FALSE;
	WPARAM      wQuitParam = 0;
        MESSAGEQUEUE* msgQ = (MESSAGEQUEUE*) GlobalLock(wndPtr->hmemTaskQ);

	while( (pos = QUEUE_FindMsg(msgQ, hWnd, 0, 0)) != -1 )
	{
	    if( msgQ->messages[pos].msg.message == WM_QUIT ) 
	    {
		bPostQuit = TRUE;
		wQuitParam = msgQ->messages[pos].msg.wParam;
	    }
	    QUEUE_RemoveMsg(msgQ, pos);
	}
	/* repost WM_QUIT to make sure this app exits its message loop */
	if( bPostQuit ) PostQuitMessage(wQuitParam);
	wndPtr->hmemTaskQ = 0;
    }
#endif
    if (!(wndPtr->dwStyle & WS_CHILD))
       if (wndPtr->wIDmenu) DestroyMenu( (HMENU)wndPtr->wIDmenu );
    if (wndPtr->hSysMenu) DestroyMenu( wndPtr->hSysMenu );
   

  //  DeleteDC(wndPtr->dc)    /* Always do this to catch orphaned DCs */ 

    wndPtr->winproc = NULL;
    wndPtr->hwndSelf = NULL;
    wndPtr->class->cWindows--;
    wndPtr->class = NULL;
    pWnd = wndPtr->next;

    //wndPtr->pDriver->pFinalize(wndPtr);
    HeapFree( GetProcessHeap(),0,wndPtr );
    return 0;
}

/***********************************************************************
 *           WIN_ResetQueueWindows
 *
 * Reset the queue of all the children of a given window.
 * Return TRUE if something was done.
 */
WINBOOL WIN_ResetQueueWindows( WND* wnd, HQUEUE hQueue, HQUEUE hNew )
{
    WINBOOL ret = FALSE;

    if (hNew)  /* Set a new queue */
    {
        for (wnd = wnd->child; (wnd); wnd = wnd->next)
        {
            if (wnd->hmemTaskQ == hQueue)
            {
                wnd->hmemTaskQ = hNew;
                ret = TRUE;
            }
            if (wnd->child)
                ret |= WIN_ResetQueueWindows( wnd, hQueue, hNew );
        }
    }
    else  /* Queue is being destroyed */
    {
        while (wnd->child)
        {
            WND *tmp = wnd->child;
            ret = FALSE;
            while (tmp)
            {
                if (tmp->hmemTaskQ == hQueue)
                {
                    DestroyWindow( tmp->hwndSelf );
                    ret = TRUE;
                    break;
                }
                if (tmp->child && WIN_ResetQueueWindows(tmp->child,hQueue,0))
                    ret = TRUE;
                else
                    tmp = tmp->next;
            }
            if (!ret) break;
        }
    }
    return ret;
}

#if 0

WND * WIN_FindWndPtr( HWND hWnd )
{
	WND *wndPtr = rootWnd;

	while ( wndPtr != NULL ) {
		if ( wndPtr->hwndSelf == hWnd )
			return wndPtr;
		wndPtr->next = wndPtr;
	}
	return NULL;
}

#endif


#undef WIN_FindWndPtr
WND*   WIN_FindWndPtr( HWND hwnd );
WND * WIN_FindWndPtr( HWND hwnd )
{
    WND * ptr;
    
    if ( hwnd == NULL ) return NULL;
    ptr = (WND *)( hwnd );
    if (ptr->dwMagic != WND_MAGIC) return NULL;
    if (ptr->hwndSelf != hwnd)
    {
        DPRINT( "Can't happen: hwnd %04x self pointer is %04x\n",
                 hwnd, ptr->hwndSelf );
        return NULL;
    }
    return ptr;
}

WND*   WIN_GetDesktop(void)
{
	return NULL;
}

WND **WIN_BuildWinArray( WND *wndPtr, UINT bwaFlags, UINT* pTotal )
{
    WND **list, **ppWnd;
    WND *pWnd;
    UINT count, skipOwned, skipHidden;
    DWORD skipFlags;

    skipHidden = bwaFlags & BWA_SKIPHIDDEN;
    skipOwned = bwaFlags & BWA_SKIPOWNED;
    skipFlags = (bwaFlags & BWA_SKIPDISABLED) ? WS_DISABLED : 0;
    if( bwaFlags & BWA_SKIPICONIC ) skipFlags |= WS_MINIMIZE;

    /* First count the windows */

    if (!wndPtr) wndPtr = pWndDesktop;
    for (pWnd = wndPtr->child, count = 0; pWnd; pWnd = pWnd->next) 
    {
	if( (pWnd->dwStyle & skipFlags) || (skipOwned && pWnd->owner) ) continue;
	if( !skipHidden || pWnd->dwStyle & WS_VISIBLE ) count++;
    }

    if( count )
    {
	/* Now build the list of all windows */

	if ((list = (WND **)HeapAlloc( GetProcessHeap(), 0, sizeof(WND *) * (count + 1))))
	{
	    for (pWnd = wndPtr->child, ppWnd = list, count = 0; pWnd; pWnd = pWnd->next)
	    {
		if( (pWnd->dwStyle & skipFlags) || (skipOwned && pWnd->owner) ) continue;
		if( !skipHidden || pWnd->dwStyle & WS_VISIBLE )
		{
		   *ppWnd++ = pWnd;
		    count++;
		}
	    }
	   *ppWnd = NULL;
	}
	else count = 0;
    } else list = NULL;

    if( pTotal ) *pTotal = count;
    return list;
}
void WIN_DestroyList( WND **list )
{
	HeapFree(GetProcessHeap(),0,*list);
}






/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
WINBOOL WIN_UnlinkWindow( HWND hWnd )
{    
    WND *wndPtr, **ppWnd;

    if (!(wndPtr = WIN_FindWndPtr( hWnd )) || !wndPtr->parent) return FALSE;
    ppWnd = &wndPtr->parent->child;
    while (*ppWnd != wndPtr) ppWnd = &(*ppWnd)->next;
    *ppWnd = wndPtr->next;
    return TRUE;
}


/***********************************************************************
 *           WIN_LinkWindow
 *
 * Insert a window into the siblings linked list.
 * The window is inserted after the specified window, which can also
 * be specified as HWND_TOP or HWND_BOTTOM.
 */
WINBOOL WIN_LinkWindow( HWND hWnd, HWND hWndInsertAfter )
{    
    WND *wndPtr, **ppWnd;

    if (!(wndPtr = WIN_FindWndPtr( hWnd )) || !wndPtr->parent) return FALSE;

    if ((hWndInsertAfter == HWND_TOP) || (hWndInsertAfter == HWND_BOTTOM))
    {
        ppWnd = &wndPtr->parent->child;  /* Point to first sibling hWnd */
	if (hWndInsertAfter == HWND_BOTTOM)  /* Find last sibling hWnd */
	    while (*ppWnd) ppWnd = &(*ppWnd)->next;
    }
    else  /* Normal case */
    {
	WND * afterPtr = WIN_FindWndPtr( hWndInsertAfter );
	if (!afterPtr) return FALSE;
        ppWnd = &afterPtr->next;
    }
    wndPtr->next = *ppWnd;
    *ppWnd = wndPtr;
    return TRUE;
}



void WIN_UpdateNCArea(WND* wnd, BOOL bUpdate)
{
    POINT pt = {0, 0}; 
    HRGN hClip = (HRGN)1;

    /* desktop window doesn't have nonclient area */
    if(wnd == WIN_GetDesktop()) 
    {
        wnd->flags &= ~WIN_NEEDS_NCPAINT;
        return;
    }

    if( wnd->hrgnUpdate > (HRGN)1 )
    {
	ClientToScreen(wnd->hwndSelf, &pt);

        hClip = CreateRectRgn( 0, 0, 0, 0 );
        if (!CombineRgn( hClip, wnd->hrgnUpdate, 0, RGN_COPY ))
        {
            DeleteObject(hClip);
            hClip = (HRGN)1;
        }
	else
	    OffsetRgn( hClip, pt.x, pt.y );

        if (bUpdate)
        {
	    /* exclude non-client area from update region */
            HRGN hrgn = CreateRectRgn( 0, 0,
                                 wnd->rectClient.right - wnd->rectClient.left,
                                 wnd->rectClient.bottom - wnd->rectClient.top);

            if (hrgn && (CombineRgn( wnd->hrgnUpdate, wnd->hrgnUpdate,
                                       hrgn, RGN_AND) == NULLREGION))
            {
                DeleteObject( wnd->hrgnUpdate );
                wnd->hrgnUpdate = (HRGN)1;
            }

            DeleteObject( hrgn );
        }
    }

    wnd->flags &= ~WIN_NEEDS_NCPAINT;

#ifdef OPTIMIZE
    if ((wnd->hwndSelf == GetActiveWindow()) &&
        !(wnd->flags & WIN_NCACTIVATED))
    {
        wnd->flags |= WIN_NCACTIVATED;
        if( hClip > (HRGN)1) DeleteObject( hClip );
        hClip = (HRGN)1;
    }
#endif

    if (hClip) MSG_SendMessage( wnd, WM_NCPAINT, (WPARAM)hClip, 0L );

    if (hClip > (HRGN)1) DeleteObject( hClip );
}

/***********************************************************************
 *           WIN_IsWindowDrawable
 *
 * hwnd is drawable when it is visible, all parents are not
 * minimized, and it is itself not minimized unless we are
 * trying to draw its default class icon.
 */
WINBOOL WIN_IsWindowDrawable( WND* wnd, WINBOOL icon )
{

  return TRUE;

  if ( wnd == NULL )
	return FALSE;
  if( (wnd->dwStyle & WS_MINIMIZE &&
       icon && wnd->class->hIcon) ||
     !(wnd->dwStyle & WS_VISIBLE) ) return FALSE;
  for(wnd = wnd->parent; wnd; wnd = wnd->parent)
    if( wnd->dwStyle & WS_MINIMIZE ||
      !(wnd->dwStyle & WS_VISIBLE) ) break;
  return wnd == NULL;
}



WND* WIN_GetTopParentPtr( WND* pWnd )
{
    while( pWnd && (pWnd->dwStyle & WS_CHILD)) pWnd = pWnd->parent;
    return pWnd;
}


HWND WIN_GetTopParent( HWND hwnd )
{
    WND *wndPtr = WIN_GetTopParentPtr ( WIN_FindWndPtr( hwnd ) );
    return wndPtr ? wndPtr->hwndSelf : 0;
}


void WIN_CheckFocus( WND* pWnd )
{
    if( GetFocus() == pWnd->hwndSelf )
	SetFocus( (pWnd->dwStyle & WS_CHILD) ? pWnd->parent->hwndSelf : 0 ); 
}


void WIN_SendDestroyMsg( WND* pWnd )
{
    WIN_CheckFocus(pWnd);

    if( CARET_GetHwnd() == pWnd->hwndSelf ) DestroyCaret();

  
    MSG_SendMessage( pWnd, WM_DESTROY, 0, 0);

    if( IsWindow(pWnd->hwndSelf) )
    {
	WND* pChild = pWnd->child;
	while( pChild )
	{
	    WIN_SendDestroyMsg( pChild );
	    pChild = pChild->next;
	}
	WIN_CheckFocus(pWnd);
    }
    else
	DPRINT( "\tdestroyed itself while in WM_DESTROY!\n");
}


/***********************************************************************
 *           IsDialogMessage   (USER32.90)
 */
WINBOOL STDCALL WIN_IsDialogMessage( HWND hwndDlg, LPMSG msg )
{

    WINBOOL ret, translate, dispatch;
    INT dlgCode;

    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd ))
        return FALSE;

    dlgCode = SendMessage( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage( msg );
    if (dispatch) DispatchMessage( msg );
    return ret;
}


WINBOOL WIN_GetClientRect(WND *wndPtr, LPRECT lpRect )
{
    if ( lpRect == NULL )
	return FALSE;
    lpRect->left = lpRect->top = lpRect->right = lpRect->bottom = 0;
    if (wndPtr) 
    {
	lpRect->right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
	lpRect->bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    }
    return TRUE;	
}


/*******************************************************************
 *           GetSysModalWindow16   (USER.52)
 */
HWND  GetSysModalWindow(void)
{
    return NULL;
}
