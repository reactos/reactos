#include <windows.h>
#include <user32/nc.h>

WINBOOL 
STDCALL 
AdjustWindowRect( LPRECT lpRect, DWORD dwStyle, WINBOOL bMenu )
{
    return AdjustWindowRectEx( lpRect, dwStyle, bMenu, 0 );
}



WINBOOL
STDCALL AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, WINBOOL bMenu, DWORD dwExStyle)
{
        /* Correct the window style */

    if (!(dwStyle & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	dwStyle |= WS_CAPTION;
    dwStyle &= (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME | WS_CHILD);
    dwExStyle &= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE |
		WS_EX_STATICEDGE | WS_EX_TOOLWINDOW);
    if (dwExStyle & WS_EX_DLGMODALFRAME) dwStyle &= ~WS_THICKFRAME;

    if (TWEAK_WineLook == WIN31_LOOK)
	NC_AdjustRect( lpRect, dwStyle, bMenu, dwExStyle );
    else {
	NC_AdjustRectOuter95( lpRect, dwStyle, bMenu, dwExStyle );
	NC_AdjustRectInner95( lpRect, dwStyle, dwExStyle );
    }
   
    return TRUE;
}


WINBOOL STDCALL GetWindowRect( HWND hwnd, LPRECT rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return FALSE;
    
    *rect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD )
	MapWindowPoints( wndPtr->parent->hwndSelf, 0, (POINT *)rect, 2 );
    return TRUE;
}


WINBOOL
STDCALL
GetClientRect(HWND hWnd, LPRECT lpRect)
{
    WND * wndPtr = WIN_FindWndPtr( hWnd );
    if ( wndPtr == NULL || lpRect == NULL )
	return FALSE;

    lpRect->left = lpRect->top = lpRect->right = lpRect->bottom = 0;
    if (wndPtr) 
    {
	lpRect->right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
	lpRect->bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    }
    return TRUE;
}
