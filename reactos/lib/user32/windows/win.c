#include <windows.h>
#include <user32/win.h>
#include <user32/class.h>
#include <user32/menu.h>
#include <user32/winpos.h>
#include <user32/hook.h>
#include <user32/property.h>
#include <user32/paint.h>
#include <user32/debug.h>


// change style on WS_OVERLAPPEDWINDOW

#undef CreateWindowA
HWND STDCALL CreateWindowA(LPCSTR  lpClassName, LPCSTR  lpWindowName,
    DWORD  dwStyle,int  x, int  y,	
    int  nWidth, int  nHeight,	HWND  hWndParent,
    HMENU  hMenu,  HANDLE  hInstance,	
    LPVOID  lpParam )
{
	return CreateWindowExA( 0, lpClassName, lpWindowName, dwStyle,x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam );
}

#undef CreateWindowW
HWND STDCALL CreateWindowW(LPCWSTR  lpClassName, LPCWSTR  lpWindowName,
    DWORD  dwStyle,int  x, int  y,	
    int  nWidth, int  nHeight,	HWND  hWndParent,
    HMENU  hMenu,  HANDLE  hInstance,	
    LPVOID  lpParam )
{
	return CreateWindowExW( 0, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam );
}

HWND STDCALL CreateWindowExA( DWORD exStyle, LPCSTR lpClassName,
                                 LPCSTR lpWindowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE hInstance, LPVOID data )
{

    CREATESTRUCT cs;
    CLASS *p;
    DWORD status;

//    int i;


    p = CLASS_FindClassByAtom( STRING2ATOMA(lpClassName), hInstance );
    if ( p == NULL )
		return NULL;

    
    //if(exStyle & WS_EX_MDICHILD)
    //    return MDI_CreateMDIWindowA(className, windowName, style, x, y, width, height, parent, instance, data);



   
    if ( style ==  WS_OVERLAPPEDWINDOW )
	style |= (WS_OVERLAPPED| WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX |WS_MAXIMIZEBOX  );


    /* Create the window */

    cs.lpCreateParams = data;
    cs.hInstance      = hInstance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = (LPWSTR)lpWindowName;
    cs.dwExStyle      = exStyle;



    return  WIN_CreateWindowEx( &cs, p->atomName  );
  
}


HWND STDCALL CreateWindowExW( DWORD exStyle, LPCWSTR lpClassName,
                                 LPCWSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE hInstance, LPVOID data )
{
    CLASS *p;
    DWORD status;
    CREATESTRUCT cs;

    

    p = CLASS_FindClassByAtom( STRING2ATOMW(lpClassName), hInstance );
    if ( p == NULL )
		return NULL;

    if ( style ==  WS_OVERLAPPEDWINDOW )
	style |= (WS_OVERLAPPED| WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX |WS_MAXIMIZEBOX  );

    /* Create the window */

    cs.lpCreateParams = data;
    cs.hInstance      = hInstance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = p->className;
    cs.dwExStyle      = exStyle;

    
    return WIN_CreateWindowEx( &cs, p->atomName   );
   
}


/***********************************************************************
 *           DestroyWindow   (USER.135)
 */
WINBOOL STDCALL DestroyWindow( HWND hwnd )
{
    WND * wndPtr;

      /* Initialization */

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    //if (wndPtr == pWndDesktop) return FALSE; /* Can't destroy desktop */


    if ( hwnd == NULL )
	return FALSE;

 
      /* Call hooks */
	

    if( HOOK_CallHooks( WH_CBT, HCBT_DESTROYWND, hwnd, 0L, wndPtr->class->bUnicode ) )
        return FALSE;

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
    {
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, hwnd, 0L,  wndPtr->class->bUnicode  );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

    if( !QUEUE_IsExitingQueue(wndPtr->hmemTaskQ) )
	if( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	{
	    /* Notify the parent window only */
	    if ( wndPtr->parent  != NULL )
	    	SendMessageA( wndPtr->parent->hwndSelf, WM_PARENTNOTIFY,
			    MAKEWPARAM(WM_DESTROY, wndPtr->wIDmenu), (LPARAM)hwnd );
	    if( !IsWindow(hwnd) ) return TRUE;
	}

 

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
        SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW |
		        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|
		        ((QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))?SWP_DEFERERASE:0) );
	if (!IsWindow(hwnd)) return TRUE;
    }

      /* Recursively destroy owned windows */

    if( !(wndPtr->dwStyle & WS_CHILD) )
    {
      /* make sure top menu popup doesn't get destroyed */
      MENU_PatchResidentPopup( (HQUEUE)0xFFFF, wndPtr );

      for (;;)
      {

        WND *siblingPtr = NULL;
        if ( wndPtr->parent != NULL )
		siblingPtr = wndPtr->parent->child;  /* First sibling */
        while (siblingPtr)
        {
            if (siblingPtr->owner == wndPtr)
	    {
               if (siblingPtr->hmemTaskQ == wndPtr->hmemTaskQ)
                   break;
               else 
                   siblingPtr->owner = NULL;
	    }
            siblingPtr = siblingPtr->next;
        }
        if (siblingPtr) DestroyWindow( siblingPtr->hwndSelf );
        else break;
      }
// !Options.managed ||
      if(  EVENT_CheckFocus() )
          WINPOS_ActivateOtherWindow(wndPtr);

      if( wndPtr->owner &&
	  wndPtr->owner->hwndLastActive == wndPtr->hwndSelf )
	  wndPtr->owner->hwndLastActive = wndPtr->owner->hwndSelf;
    }

      /* Send destroy messages */

    WIN_SendDestroyMsg( wndPtr );
    if (!IsWindow(hwnd)) return TRUE;

      /* Unlink now so we won't bother with the children later on */

    if( wndPtr->parent ) WIN_UnlinkWindow(hwnd);

      /* Destroy the window storage */

    WIN_DestroyWindow( wndPtr );
    return TRUE;
}


/*******************************************************************
 *           EnableWindow   
 */
WINBOOL STDCALL EnableWindow( HWND hWnd, WINBOOL enable )
{
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hWnd ))) return FALSE;
    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Enable window */
	wndPtr->dwStyle &= ~WS_DISABLED;
	SendMessageA( hWnd, WM_ENABLE, TRUE, 0 );
	return TRUE;
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Disable window */
	wndPtr->dwStyle |= WS_DISABLED;
	if ((hWnd == GetFocus()) || IsChild( hWnd, GetFocus() ))
	    SetFocus( 0 );  /* A disabled window can't have the focus */
	if ((hWnd == GetCapture()) || IsChild( hWnd, GetCapture() ))
	    ReleaseCapture();  /* A disabled window can't capture the mouse */
	SendMessageA( hWnd, WM_ENABLE, FALSE, 0 );
	return FALSE;
    }
    return ((wndPtr->dwStyle & WS_DISABLED) != 0);
}



HWND STDCALL GetDesktopWindow(VOID)
{
	return WIN_GetDesktop()->hwndSelf;
}



HWND STDCALL GetWindow( HWND hwnd, UINT rel )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    switch(rel)
    {
    case GW_HWNDFIRST:
        if (wndPtr->parent) return wndPtr->parent->child->hwndSelf;
	else return 0;
	
    case GW_HWNDLAST:
	if (!wndPtr->parent) return 0;  /* Desktop window */
	while (wndPtr->next) wndPtr = wndPtr->next;
        return wndPtr->hwndSelf;
	
    case GW_HWNDNEXT:
        if (!wndPtr->next) return 0;
	return wndPtr->next->hwndSelf;
	
    case GW_HWNDPREV:
        if (!wndPtr->parent) return 0;  /* Desktop window */
        wndPtr = wndPtr->parent->child;  /* First sibling */
        if (wndPtr->hwndSelf == hwnd) return 0;  /* First in list */
        while (wndPtr->next)
        {
            if (wndPtr->next->hwndSelf == hwnd) return wndPtr->hwndSelf;
            wndPtr = wndPtr->next;
        }
        return 0;
	
    case GW_OWNER:
	return wndPtr->owner ? wndPtr->owner->hwndSelf : 0;

    case GW_CHILD:
	return wndPtr->child ? wndPtr->child->hwndSelf : 0;
    }
    return 0;
}


WORD STDCALL SetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->class->cbWndExtra)
        {
            DPRINT( "Invalid offset %d\n", offset );
            return 0;
        }
        ptr = (WORD *)(((char *)wndPtr->wExtra) + offset);
    }
    else switch(offset)
    {
	case GWW_ID:        ptr = (WORD *)&wndPtr->wIDmenu; break;
	case GWW_HINSTANCE: ptr = (WORD *)&wndPtr->hInstance; break;
	case GWW_HWNDPARENT: return SetParent( hwnd, newval );
	default:
            DPRINT("Invalid offset %d\n", offset );
            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}

LONG STDCALL GetWindowLongA( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset );
}


LONG STDCALL GetWindowLongW( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset );
}

/**********************************************************************
 *	     SetWindowLongW    (USER.518) Set window attribute
 *
 * SetWindowLong() alters one of a window's attributes or sets a -bit (long)
 * value in a window's extra memory. 
 *
 * The _hwnd_ parameter specifies the window.  is the handle to a
 * window that has extra memory. The _newval_ parameter contains the
 * new attribute or extra memory value.  If positive, the _offset_
 * parameter is the byte-addressed location in the window's extra
 * memory to set.  If negative, _offset_ specifies the window
 * attribute to set, and should be one of the following values:
 *
 * GWL_EXSTYLE      The window's extended window style
 *
 * GWL_STYLE        The window's window style. 
 *
 * GWL_WNDPROC      Pointer to the window's window procedure. 
 *
 * GWL_HINSTANCE    The window's pplication instance handle.
 *
 * GWL_ID           The window's identifier.
 *
 * GWL_USERDATA     The window's user-specified data. 
 *
 * If the window is a dialog box, the _offset_ parameter can be one of 
 * the following values:
 *
 * DWL_DLGPROC      The address of the window's dialog box procedure.
 *
 * DWL_MSGRESULT    The return value of a message 
 *                  that the dialog box procedure processed.
 *
 * DWL_USER         Application specific information.
 *
 * RETURNS
 *
 * If successful, returns the previous value located at _offset_. Otherwise,
 * returns 0.
 *
 * NOTES
 *
 * Extra memory for a window class is specified by a nonzero cbWndExtra 
 * parameter of the WNDCLASS structure passed to RegisterClass() at the
 * time of class creation.
 *  
 * Using GWL_WNDPROC to set a new window procedure effectively creates
 * a window subclass. Use CallWindowProc() in the new windows procedure
 * to pass messages to the superclass's window procedure.
 *
 * The user data is reserved for use by the application which created
 * the window.
 *
 * Do not use GWL_STYLE to change the window's WS_DISABLE style;
 * instead, call the EnableWindow() function to change the window's
 * disabled state.
 *
 * Do not use GWL_HWNDPARENT to reset the window's parent, use
 * SetParent() instead.
 *
 * Win95:
 * When offset is GWL_STYLE and the calling app's ver is 4.0,
 * it sends WM_STYLECHANGING before changing the settings
 * and WM_STYLECHANGED afterwards.
 * App ver 4.0 can't use SetWindowLong to change WS_EX_TOPMOST.
 *
 * BUGS
 *
 * GWL_STYLE does not dispatch WM_STYLE... messages.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win 
 *
 */

LONG STDCALL SetWindowLongA( HWND hwnd, INT offset, LONG newval )
{
    return WIN_SetWindowLong( hwnd, offset, newval );
}


LONG STDCALL SetWindowLongW( 
    HWND hwnd, 
    INT offset,
    LONG newval  
) {
    return WIN_SetWindowLong( hwnd, offset, newval );
}


HWND STDCALL GetTopWindow( HWND hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr && wndPtr->child) return wndPtr->child->hwndSelf;
    else return 0;
}

HWND STDCALL GetNextWindow( HWND hwnd, UINT flag )
{
    if ((flag != GW_HWNDNEXT) && (flag != GW_HWNDPREV)) return 0;
    return GetWindow( hwnd, flag );
}

/*****************************************************************
 *         GetParent   (USER.278)
 */
HWND STDCALL GetParent( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if ((!wndPtr) || (!(wndPtr->dwStyle & (WS_POPUP|WS_CHILD)))) return 0;
    wndPtr = (wndPtr->dwStyle & WS_CHILD) ? wndPtr->parent : wndPtr->owner;
    return wndPtr ? wndPtr->hwndSelf : NULL;
}

/*****************************************************************
 *         SetParent   (USER.495)
 */
HWND STDCALL SetParent( HWND hWndChild, HWND hWndNewParent )
{
  WND *wndPtr = WIN_FindWndPtr( hWndChild );
  WND *pWndNewParent = 
    (hWndNewParent) ? WIN_FindWndPtr( hWndNewParent ) : WIN_FindWndPtr(GetDesktopWindow());
  WND *pWndOldParent;

  //  (wndPtr)?(*wndPtr->pDriver->pSetParent)(wndPtr, pWndNewParent):NULL;

  return pWndOldParent?pWndOldParent->hwndSelf:NULL;
}

/*******************************************************************
 *	     GetWindowTextA    (USER.309)
 */
INT STDCALL GetWindowTextA( HWND hwnd, LPSTR lpString, INT nMaxCount )
{
    return (INT)SendMessageA( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}



/*******************************************************************
 *	     GetWindowTextW    (USER.312)
 */
INT STDCALL GetWindowTextW( HWND hwnd, LPWSTR lpString, INT nMaxCount )
{
    return (INT)SendMessageW( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}


WINBOOL STDCALL SetWindowTextA( HWND hwnd, LPCSTR lpString )
{
    return (WINBOOL)SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


WINBOOL STDCALL SetWindowTextW( HWND hwnd, LPCWSTR lpString )
{
    return (WINBOOL)SendMessageW( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


INT STDCALL GetWindowTextLengthA( HWND hwnd )
{
    return SendMessageA( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


INT STDCALL GetWindowTextLengthW( HWND hwnd )
{
    return SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


WINBOOL STDCALL IsChild( HWND parent, HWND child )
{
    WND * wndPtr = WIN_FindWndPtr( child );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        wndPtr = wndPtr->parent;
	if (wndPtr->hwndSelf == parent) return TRUE;
    }
    return FALSE;
}

WINBOOL STDCALL IsWindow(HANDLE hWnd)
{	
        if (WIN_FindWndPtr( hWnd ) == NULL) return FALSE;
	return TRUE;
}


/***********************************************************************
 *           IsWindowEnabled   
 */ 
WINBOOL STDCALL IsWindowEnabled(HWND hWnd)
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hWnd))) return FALSE;
    return !(wndPtr->dwStyle & WS_DISABLED);
}


/***********************************************************************
 *           IsWindowUnicode   
 */
WINBOOL STDCALL IsWindowUnicode( HWND hWnd )
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hWnd))) 
	return FALSE;
    return wndPtr->class->bUnicode;
}


WINBOOL STDCALL IsWindowVisible( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        if (!(wndPtr->dwStyle & WS_VISIBLE)) return FALSE;
        wndPtr = wndPtr->parent;
    }
    return (wndPtr && (wndPtr->dwStyle & WS_VISIBLE));
}





/*******************************************************************
 *            FlashWindow  (USER32.202)
 */
WINBOOL STDCALL FlashWindow( HWND hWnd, WINBOOL bInvert )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);


    if (!wndPtr) return FALSE;

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC hDC = GetDC(hWnd);
            
            if (!SendMessage( hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0 ))
                wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
            
            ReleaseDC( hWnd, hDC );
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            PAINT_RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE | RDW_ERASE |
					  RDW_UPDATENOW | RDW_FRAME, 0 );
            wndPtr->flags &= ~WIN_NCACTIVATED;
        }
        return TRUE;
    }
    else
    {
        WPARAM wparam;
        if (bInvert) wparam = !(wndPtr->flags & WIN_NCACTIVATED);
        else wparam = (hWnd == GetActiveWindow());

        SendMessage( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
        return wparam;
    }
}


