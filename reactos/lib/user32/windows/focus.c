#include <windows.h>
#include <user32/win.h>

HWND hwndFocus;

//FIXME this a shared api by all procedures

HWND STDCALL SetFocus( HWND hwnd )
{
    HWND hWndPrevFocus, hwndTop = hwnd;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (wndPtr)
    {
	  /* Check if we can set the focus to this window */

	while ( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD  )
	{
	    if ( wndPtr->dwStyle & ( WS_MINIMIZE | WS_DISABLED) )
		 return 0;
            if (!(wndPtr = wndPtr->parent)) return 0;
	    hwndTop = wndPtr->hwndSelf;
	}

	if( hwnd == hwndFocus ) return hwnd;

	/* call hooks */
	if( HOOK_CallHooksA( WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd,
			      (LPARAM)hwndFocus) )
	    return 0;

        /* activate hwndTop if needed. */
	if (hwndTop != GetActiveWindow())
	{
	    if (!WINPOS_SetActiveWindow(hwndTop, 0, 0)) return 0;

	    if (!IsWindow( hwnd )) return 0;  /* Abort if window destroyed */
	}
    }
    else if( HOOK_CallHooksA( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)hwndFocus ) )
             return 0;

      /* Change focus and send messages */
    hWndPrevFocus = hwndFocus;

    FOCUS_SwitchFocus( hwndFocus , hwnd );

    return hWndPrevFocus;
}


HWND STDCALL GetFocus(void)
{
    return hwndFocus;
}

/*****************************************************************
 *	         FOCUS_SwitchFocus 
 */
void FOCUS_SwitchFocus( HWND hFocusFrom, HWND hFocusTo )
{
    WND *pFocusTo = WIN_FindWndPtr( hFocusTo );
    hwndFocus = hFocusTo;

#if 0
    if (hFocusFrom) SendMessageA( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );
#else
    /* FIXME: must be SendMessage16() because 32A doesn't do
     * intertask at this time */
    if (hFocusFrom) SendMessage( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );
#endif
    if( !pFocusTo || hFocusTo != hwndFocus )
	return;

    /* According to API docs, the WM_SETFOCUS message is sent AFTER the window
       has received the keyboard focus. */

//    pFocusTo->pDriver->pSetFocus(pFocusTo);

#if 0
    SendMessageA( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
#else
    SendMessageA( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
#endif
}

