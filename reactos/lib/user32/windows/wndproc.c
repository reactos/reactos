#include <windows.h>
#include <user32/win.h>
#include <user32/winproc.h>
#include <user32/nc.h>
//#include <user32/defwnd.h>

//#include <user32/heapdup.h>

#define MDICREATESTRUCTA MDICREATESTRUCT
#define MDICREATESTRUCTW MDICREATESTRUCT


/**********************************************************************
 *	     CallWindowProc    
 *
 * The CallWindowProc() function invokes the windows procedure _func_,
 * with _hwnd_ as the target window, the message specified by _msg_, and
 * the message parameters _wParam_ and _lParam_.
 *
 * Some kinds of argument conversion may be done, I'm not sure what.
 *
 * CallWindowProc() may be used for windows subclassing. Use
 * SetWindowLong() to set a new windows procedure for windows of the
 * subclass, and handle subclassed messages in the new windows
 * procedure. The new windows procedure may then use CallWindowProc()
 * with _func_ set to the parent class's windows procedure to dispatch
 * the message to the superclass.
 *
 * RETURNS
 *
 *    The return value is message dependent.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win32 
 */
LRESULT WINAPI CallWindowProcA( 
    WNDPROC func, 
    HWND hwnd,
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam    
) 
{
   INT ret;
   WND *w;
   LRESULT l;
   w = WIN_FindWndPtr( hwnd );
   if ( w == NULL )
	return 0;

   if ( w->class->bUnicode == FALSE ) 
	return func(hwnd,msg,wParam,lParam);


   //convert message
   ret = WINPROC_MapMsg32WTo32A( hwnd,  msg,  wParam, &lParam );
   if ( ret == -1 )
	return 0;
   l = func(hwnd,msg,wParam,lParam);
   if ( ret == 1 )
	WINPROC_UnmapMsg32WTo32A( hwnd, msg, wParam, lParam );
   return l;
	



}

LRESULT WINAPI CallWindowProcW( 
    WNDPROC func, 
    HWND hwnd,
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam    
) 
{
   INT ret;
   WND *w;
   LRESULT l;
   w = WIN_FindWndPtr( hwnd );
   if ( w == NULL )
	return 0;

   if ( w->class->bUnicode == TRUE ) 
	return func(hwnd,msg,wParam,lParam);


   //convert message
   ret = WINPROC_MapMsg32ATo32W( hwnd,  msg,  wParam, &lParam );
   if ( ret == -1 )
	return 0;
   l = func(hwnd,msg,wParam,lParam);
   if ( ret == 1 )
	WINPROC_UnmapMsg32ATo32W( hwnd, msg, wParam, lParam );
   return l;
}

/***********************************************************************
 * DefWindowProc  Calls default window message handler
 * 
 * Calls default window procedure for messages not processed 
 *  by application.
 *
 *  RETURNS
 *     Return value is dependent upon the message.
*/


/***********************************************************************
 *  DefWindowProcA [USER32.126] 
 *
 */
LRESULT WINAPI DefWindowProcA( HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    LRESULT result = 0;

    if (!wndPtr) return 0;
//    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
	    if (cs->lpszName) DEFWND_SetTextA( wndPtr, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        result = NC_HandleNCCalcSize( wndPtr, (RECT *)lParam );
        break;

    case WM_WINDOWPOSCHANGING:
	result = WINPOS_HandleWindowPosChanging( wndPtr,
                                                   (WINDOWPOS *)lParam );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
            DEFWND_HandleWindowPosChanged( wndPtr, winPos->flags );
	}
        break;

    case WM_GETTEXT:
        if (wParam && wndPtr->text)
        {
            lstrcpynA( (LPSTR)lParam, wndPtr->text, wParam );
            result = (LRESULT)lstrlenA( (LPSTR)lParam );
        }
        break;

    case WM_SETTEXT:
	DEFWND_SetTextA( wndPtr, (LPSTR)lParam );
	NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
        break;

    default:
        result = DEFWND_DefWinProc( wndPtr, msg, wParam, lParam );
        break;
    }

//    SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result );
    return result;
}


/***********************************************************************
 * DefWindowProcW [USER32.127] Calls default window message handler
 * 
 * Calls default window procedure for messages not processed 
 *  by application.
 *
 *  RETURNS
 *     Return value is dependent upon the message.
*/
LRESULT WINAPI DefWindowProcW( HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    LRESULT result = 0;

    if (!wndPtr) return 0;
  //  SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
	    if (cs->lpszName) DEFWND_SetTextW( wndPtr, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        result = NC_HandleNCCalcSize( wndPtr, (RECT *)lParam );
        break;

    case WM_WINDOWPOSCHANGING:
	result = WINPOS_HandleWindowPosChanging( wndPtr,
                                                   (WINDOWPOS *)lParam );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
            DEFWND_HandleWindowPosChanged( wndPtr, winPos->flags );
	}
        break;

    case WM_GETTEXT:
        if (wParam && wndPtr->text)
        {
            lstrcpynW( (LPWSTR)lParam, wndPtr->text, wParam );
            result = (LRESULT)lstrlenW( (LPWSTR)lParam );
        }
        break;

    case WM_SETTEXT:
	DEFWND_SetTextW( wndPtr, (LPSTR)lParam );
	NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
        break;

    default:
        result = DEFWND_DefWinProc( wndPtr, msg, wParam, lParam );
        break;
    }

  //  SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result );
    return result;
}

/**********************************************************************
 *	     WINPROC_TestCBForStr
 *
 * Return TRUE if the lparam is a string
 */
WINBOOL WINPROC_TestCBForStr ( HWND hwnd )
{	WND * wnd = WIN_FindWndPtr(hwnd); 
	return ( !(LOWORD(wnd->dwStyle) & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) ||
	      (LOWORD(wnd->dwStyle) & CBS_HASSTRINGS) );
}
/**********************************************************************
 *	     WINPROC_TestLBForStr
 *
 * Return TRUE if the lparam is a string
 */
WINBOOL WINPROC_TestLBForStr ( HWND hwnd )
{	WND * wnd = WIN_FindWndPtr(hwnd); 
	return ( !(LOWORD(wnd->dwStyle) & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || 
	    (LOWORD(wnd->dwStyle) & LBS_HASSTRINGS) );
}


/**********************************************************************
 *	     WINPROC_MapMsg32ATo32W
 *
 * Map a message from Ansi to Unicode.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 *
 * FIXME:
 *  WM_CHAR, WM_CHARTOITEM, WM_DEADCHAR, WM_MENUCHAR, WM_SYSCHAR, WM_SYSDEADCHAR
 *
 * FIXME:
 *  WM_GETTEXT/WM_SETTEXT and static control with SS_ICON style:
 *  the first four bytes are the handle of the icon 
 *  when the WM_SETTEXT message has been used to set the icon
 */
INT WINPROC_MapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam, LPARAM *plparam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)HeapAlloc( GetProcessHeap(), 0,
                                     wParam * sizeof(WCHAR) + sizeof(LPARAM) );
            if (!ptr) 
		return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
        }
        return 1;

    case WM_SETTEXT:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        *plparam = (LPARAM)HEAP_strdupAtoW( GetProcessHeap(), 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);
    break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)HeapAlloc( GetProcessHeap(), 0,
                                                                sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(CREATESTRUCTW *)*plparam;
            if (HIWORD(cs->lpszName))
                cs->lpszName = HEAP_strdupAtoW( GetProcessHeap(), 0,
                                                (LPCSTR)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                cs->lpszClass = HEAP_strdupAtoW( GetProcessHeap(), 0,
                                                 (LPCSTR)cs->lpszClass );
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCTW *cs = HeapAlloc( GetProcessHeap(), 0, sizeof(MDICREATESTRUCT) );
            if (!cs) return -1;
            *cs = *(MDICREATESTRUCT *)*plparam;
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupAtoW( GetProcessHeap(), 0,
                                               (LPCSTR)cs->szClass );
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupAtoW( GetProcessHeap(), 0,
                                               (LPCSTR)cs->szTitle );
            *plparam = (LPARAM)cs;
        }
        return 1;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupAtoW( GetProcessHeap(), 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);

    case LB_GETTEXT:		    /* fixme: fixed sized buffer */
        { if ( WINPROC_TestLBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)HeapAlloc( GetProcessHeap(), 0, 256 * sizeof(WCHAR) + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupAtoW( GetProcessHeap(), 0, (LPCSTR)*plparam );
        return (*plparam ? 1 : -1);

    case CB_GETLBTEXT:    /* fixme: fixed sized buffer */
        { if ( WINPROC_TestCBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)HeapAlloc( GetProcessHeap(), 0, 256 * sizeof(WCHAR) + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Multiline edit */
    case EM_GETLINE:
        { WORD len = (WORD)*plparam;
	  LPARAM *ptr = (LPARAM *) HeapAlloc( GetProcessHeap(), 0, sizeof(LPARAM) + sizeof (WORD) + len*sizeof(WCHAR) );
          if (!ptr) return -1;
          *ptr++ = *plparam;  /* Store previous lParam */
	  (WORD)*ptr = len;   /* Store the lenght */
          *plparam = (LPARAM)ptr;
	}
        return 1;

    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
    case EM_SETPASSWORDCHAR:
  //      FIXME(msg, "message %s (0x%x) needs translation, please report\n", SPY_GetMsgName(msg), msg );
        return -1;
    default:  /* No translation needed */
        return 0;
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32ATo32W
 *
 * Unmap a message that was mapped from Ansi to Unicode.
 */
void WINPROC_UnmapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpynWtoA( (LPSTR)*ptr, (LPWSTR)lParam, wParam );
            HeapFree( GetProcessHeap(), 0, ptr );
        }
        break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
            if (HIWORD(cs->lpszName))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->lpszClass );
            HeapFree( GetProcessHeap(), 0, cs );
        }
        break;

    case WM_MDICREATE:
        {
            MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)lParam;
            if (HIWORD(cs->szTitle))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->szClass );
            HeapFree( GetProcessHeap(), 0, cs );
        }
        break;

    case WM_SETTEXT:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        HeapFree( GetProcessHeap(), 0, (void *)lParam );
        break;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          HeapFree( GetProcessHeap(), 0, (void *)lParam );
        break;

    case LB_GETTEXT:
        { if ( WINPROC_TestLBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)lParam - 1;
	    lstrcpyWtoA( (LPSTR)*ptr, (LPWSTR)(lParam) );
            HeapFree( GetProcessHeap(), 0, ptr );
	  }
        }
        break;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          HeapFree( GetProcessHeap(), 0, (void *)lParam );
        break;

    case CB_GETLBTEXT:
        { if ( WINPROC_TestCBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpyWtoA( (LPSTR)*ptr, (LPWSTR)(lParam) );
            HeapFree( GetProcessHeap(), 0, ptr );
	  }
        }
        break;

/* Multiline edit */
    case EM_GETLINE:
        { LPARAM *ptr = (LPARAM *)lParam - 1;  /* get the old lParam */
	  WORD len = *(WORD *)ptr;
          lstrcpynWtoA( ((LPSTR)*ptr)+2, ((LPWSTR)(lParam + 1))+1, len );
          HeapFree( GetProcessHeap(), 0, ptr );
        }
        break;
    }
}


/**********************************************************************
 *	     WINPROC_MapMsg32WTo32A
 *
 * Map a message from Unicode to Ansi.
 * Return value is -1 on error, 0 if OK, 1 if an UnmapMsg call is needed.
 */
INT WINPROC_MapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam, LPARAM *plparam )
{   switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)HeapAlloc( GetProcessHeap(), 0,
                                               wParam + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
        }
        return 1;

    case WM_SETTEXT:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        *plparam = (LPARAM)HEAP_strdupWtoA( GetProcessHeap(), 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)HeapAlloc( GetProcessHeap(), 0,
                                                                sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(CREATESTRUCTA *)*plparam;
            if (HIWORD(cs->lpszName))
                cs->lpszName  = HEAP_strdupWtoA( GetProcessHeap(), 0,
                                                 (LPCWSTR)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                cs->lpszClass = HEAP_strdupWtoA( GetProcessHeap(), 0,
                                                 (LPCWSTR)cs->lpszClass);
            *plparam = (LPARAM)cs;
        }
        return 1;
    case WM_MDICREATE:
        {
            MDICREATESTRUCTA *cs =
                (MDICREATESTRUCTA *)HeapAlloc( GetProcessHeap(), 0, sizeof(*cs) );
            if (!cs) return -1;
            *cs = *(MDICREATESTRUCTA *)*plparam;
            if (HIWORD(cs->szTitle))
                cs->szTitle = HEAP_strdupWtoA( GetProcessHeap(), 0,
                                               (LPCWSTR)cs->szTitle );
            if (HIWORD(cs->szClass))
                cs->szClass = HEAP_strdupWtoA( GetProcessHeap(), 0,
                                               (LPCWSTR)cs->szClass );
            *plparam = (LPARAM)cs;
        }
        return 1;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupWtoA( GetProcessHeap(), 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);

    case LB_GETTEXT:			/* fixme: fixed sized buffer */
        { if ( WINPROC_TestLBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)HeapAlloc( GetProcessHeap(), 0, 256 + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          *plparam = (LPARAM)HEAP_strdupWtoA( GetProcessHeap(), 0, (LPCWSTR)*plparam );
        return (*plparam ? 1 : -1);

    case CB_GETLBTEXT:		/* fixme: fixed sized buffer */
        { if ( WINPROC_TestCBForStr( hwnd ))
	  { LPARAM *ptr = (LPARAM *)HeapAlloc( GetProcessHeap(), 0, 256 + sizeof(LPARAM) );
            if (!ptr) return -1;
            *ptr++ = *plparam;  /* Store previous lParam */
            *plparam = (LPARAM)ptr;
	  }
        }
        return 1;

/* Multiline edit */
    case EM_GETLINE:
        { WORD len = (WORD)*plparam;
	  LPARAM *ptr = (LPARAM *) HeapAlloc( GetProcessHeap(), 0, sizeof(LPARAM) + sizeof (WORD) + len*sizeof(CHAR) );
          if (!ptr) return -1;
          *ptr++ = *plparam;  /* Store previous lParam */
	  (WORD)*ptr = len;   /* Store the lenght */
          *plparam = (LPARAM)ptr;
	}
        return 1;

    case WM_ASKCBFORMATNAME:
    case WM_DEVMODECHANGE:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
    case EM_SETPASSWORDCHAR:
//        FIXME(msg, "message %s (%04x) needs translation, please report\n",SPY_GetMsgName(msg),msg );
        return -1;
    default:  /* No translation needed */
        return 0;
    }
}


/**********************************************************************
 *	     WINPROC_UnmapMsg32WTo32A
 *
 * Unmap a message that was mapped from Unicode to Ansi.
 */
void WINPROC_UnmapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpynAtoW( (LPWSTR)*ptr, (LPSTR)lParam, wParam );
            HeapFree( GetProcessHeap(), 0, ptr );
        }
        break;

    case WM_SETTEXT:
    case CB_DIR:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_DIR:
    case LB_ADDFILE:
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
    case EM_REPLACESEL:
        HeapFree( GetProcessHeap(), 0, (void *)lParam );
        break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
            if (HIWORD(cs->lpszName))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->lpszName );
            if (HIWORD(cs->lpszClass))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->lpszClass );
            HeapFree( GetProcessHeap(), 0, cs );
        }
        break;

    case WM_MDICREATE:
        {
            MDICREATESTRUCTA *cs = (MDICREATESTRUCTA *)lParam;
            if (HIWORD(cs->szTitle))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->szTitle );
            if (HIWORD(cs->szClass))
                HeapFree( GetProcessHeap(), 0, (LPVOID)cs->szClass );
            HeapFree( GetProcessHeap(), 0, cs );
        }
        break;

/* Listbox */
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
	if ( WINPROC_TestLBForStr( hwnd ))
          HeapFree( GetProcessHeap(), 0, (void *)lParam );
        break;

    case LB_GETTEXT:
        { if ( WINPROC_TestLBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpyAtoW( (LPWSTR)*ptr, (LPSTR)(lParam) );
            HeapFree( GetProcessHeap(), 0, ptr );
	  }
        }
        break;

/* Combobox */
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
	if ( WINPROC_TestCBForStr( hwnd ))
          HeapFree( GetProcessHeap(), 0, (void *)lParam );
        break;

    case CB_GETLBTEXT:
        { if ( WINPROC_TestCBForStr( hwnd ))
          { LPARAM *ptr = (LPARAM *)lParam - 1;
            lstrcpyAtoW( (LPWSTR)*ptr, (LPSTR)(lParam) );
            HeapFree( GetProcessHeap(), 0, ptr );
	  }
        }
        break;

/* Multiline edit */
    case EM_GETLINE:
        { LPARAM *ptr = (LPARAM *)lParam - 1;  /* get the old lParam */
	  WORD len = *(WORD *)ptr;
          lstrcpynAtoW( ((LPWSTR)*ptr)+1, ((LPSTR)(lParam + 1))+2, len );
          HeapFree( GetProcessHeap(), 0, ptr );
        }
        break;
    }
}



