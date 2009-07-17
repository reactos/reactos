/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *	     1995 Alex Korobka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/unicode.h"
#include "wine/winuser16.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

  /* bits in the dwKeyData */
#define KEYDATA_ALT             0x2000
#define KEYDATA_PREVSTATE       0x4000

static short iF10Key = 0;
static short iMenuSysKey = 0;
static const WCHAR imm32W[] = { 'i','m','m','3','2','\0' };

/***********************************************************************
 *           DEFWND_HandleWindowPosChanged
 *
 * Handle the WM_WINDOWPOSCHANGED message.
 */
static void DEFWND_HandleWindowPosChanged( HWND hwnd, const WINDOWPOS *winpos )
{
    RECT rect;
    WND *wndPtr = WIN_GetPtr( hwnd );

    rect = wndPtr->rectClient;
    WIN_ReleasePtr( wndPtr );

    if (!(winpos->flags & SWP_NOCLIENTMOVE))
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG(rect.left, rect.top));

    if (!(winpos->flags & SWP_NOCLIENTSIZE) || (winpos->flags & SWP_STATECHANGED))
    {
        WPARAM wp = SIZE_RESTORED;
        if (IsZoomed(hwnd)) wp = SIZE_MAXIMIZED;
        else if (IsIconic(hwnd)) wp = SIZE_MINIMIZED;

        SendMessageW( hwnd, WM_SIZE, wp, MAKELONG(rect.right-rect.left, rect.bottom-rect.top) );
    }
}


/***********************************************************************
 *           DEFWND_SetTextA
 *
 * Set the window text.
 */
static void DEFWND_SetTextA( HWND hwnd, LPCSTR text )
{
    int count;
    WCHAR *textW;
    WND *wndPtr;

    if (!text) text = "";
    count = MultiByteToWideChar( CP_ACP, 0, text, -1, NULL, 0 );

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return;
    if ((textW = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR))))
    {
        HeapFree(GetProcessHeap(), 0, wndPtr->text);
        wndPtr->text = textW;
        MultiByteToWideChar( CP_ACP, 0, text, -1, textW, count );
        SERVER_START_REQ( set_window_text )
        {
            req->handle = wine_server_user_handle( hwnd );
            wine_server_add_data( req, textW, (count-1) * sizeof(WCHAR) );
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else
        ERR("Not enough memory for window text\n");
    WIN_ReleasePtr( wndPtr );

    USER_Driver->pSetWindowText( hwnd, textW );
}

/***********************************************************************
 *           DEFWND_SetTextW
 *
 * Set the window text.
 */
static void DEFWND_SetTextW( HWND hwnd, LPCWSTR text )
{
    static const WCHAR empty_string[] = {0};
    WND *wndPtr;
    int count;

    if (!text) text = empty_string;
    count = strlenW(text) + 1;

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return;
    HeapFree(GetProcessHeap(), 0, wndPtr->text);
    if ((wndPtr->text = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR))))
    {
        strcpyW( wndPtr->text, text );
        SERVER_START_REQ( set_window_text )
        {
            req->handle = wine_server_user_handle( hwnd );
            wine_server_add_data( req, wndPtr->text, (count-1) * sizeof(WCHAR) );
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else
        ERR("Not enough memory for window text\n");
    text = wndPtr->text;
    WIN_ReleasePtr( wndPtr );

    USER_Driver->pSetWindowText( hwnd, text );
}

/***********************************************************************
 *           DEFWND_ControlColor
 *
 * Default colors for control painting.
 */
HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType )
{
    if( ctlType == CTLCOLOR_SCROLLBAR)
    {
        HBRUSH hb = GetSysColorBrush(COLOR_SCROLLBAR);
        COLORREF bk = GetSysColor(COLOR_3DHILIGHT);
        SetTextColor( hDC, GetSysColor(COLOR_3DFACE));
        SetBkColor( hDC, bk);

        /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
         * we better use 0x55aa bitmap brush to make scrollbar's background
         * look different from the window background.
         */
        if (bk == GetSysColor(COLOR_WINDOW))
            return SYSCOLOR_55AABrush;

        UnrealizeObject( hb );
        return hb;
    }

    SetTextColor( hDC, GetSysColor(COLOR_WINDOWTEXT));

    if ((ctlType == CTLCOLOR_EDIT) || (ctlType == CTLCOLOR_LISTBOX))
        SetBkColor( hDC, GetSysColor(COLOR_WINDOW) );
    else {
        SetBkColor( hDC, GetSysColor(COLOR_3DFACE) );
        return GetSysColorBrush(COLOR_3DFACE);
    }
    return GetSysColorBrush(COLOR_WINDOW);
}


/***********************************************************************
 *           DEFWND_Print
 *
 * This method handles the default behavior for the WM_PRINT message.
 */
static void DEFWND_Print( HWND hwnd, HDC hdc, ULONG uFlags)
{
  /*
   * Visibility flag.
   */
  if ( (uFlags & PRF_CHECKVISIBLE) &&
       !IsWindowVisible(hwnd) )
      return;

  /*
   * Unimplemented flags.
   */
  if ( (uFlags & PRF_CHILDREN) ||
       (uFlags & PRF_OWNED)    ||
       (uFlags & PRF_NONCLIENT) )
  {
    WARN("WM_PRINT message with unsupported flags\n");
  }

  /*
   * Background
   */
  if ( uFlags & PRF_ERASEBKGND)
    SendMessageW(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);

  /*
   * Client area
   */
  if ( uFlags & PRF_CLIENT)
    SendMessageW(hwnd, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);
}


/*
 * helpers for calling IMM32
 *
 * WM_IME_* messages are generated only by IMM32,
 * so I assume imm32 is already LoadLibrary-ed.
 */
static HWND DEFWND_ImmGetDefaultIMEWnd( HWND hwnd )
{
    HINSTANCE hInstIMM = GetModuleHandleW( imm32W );
    HWND (WINAPI *pFunc)(HWND);
    HWND hwndRet = 0;

    if (!hInstIMM)
    {
        ERR( "cannot get IMM32 handle\n" );
        return 0;
    }

    pFunc = (void*)GetProcAddress(hInstIMM,"ImmGetDefaultIMEWnd");
    if ( pFunc != NULL )
        hwndRet = (*pFunc)( hwnd );

    return hwndRet;
}

static BOOL DEFWND_ImmIsUIMessageA( HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HINSTANCE hInstIMM = GetModuleHandleW( imm32W );
    BOOL (WINAPI *pFunc)(HWND,UINT,WPARAM,LPARAM);
    BOOL fRet = FALSE;

    if (!hInstIMM)
    {
        ERR( "cannot get IMM32 handle\n" );
        return FALSE;
    }

    pFunc = (void*)GetProcAddress(hInstIMM,"ImmIsUIMessageA");
    if ( pFunc != NULL )
        fRet = (*pFunc)( hwndIME, msg, wParam, lParam );

    return fRet;
}

static BOOL DEFWND_ImmIsUIMessageW( HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HINSTANCE hInstIMM = GetModuleHandleW( imm32W );
    BOOL (WINAPI *pFunc)(HWND,UINT,WPARAM,LPARAM);
    BOOL fRet = FALSE;

    if (!hInstIMM)
    {
        ERR( "cannot get IMM32 handle\n" );
        return FALSE;
    }

    pFunc = (void*)GetProcAddress(hInstIMM,"ImmIsUIMessageW");
    if ( pFunc != NULL )
        fRet = (*pFunc)( hwndIME, msg, wParam, lParam );

    return fRet;
}



/***********************************************************************
 *           DEFWND_DefWinProc
 *
 * Default window procedure for messages that are the same in Ansi and Unicode.
 */
static LRESULT DEFWND_DefWinProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_NCPAINT:
        return NC_HandleNCPaint( hwnd, (HRGN)wParam );

    case WM_NCHITTEST:
        {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            return NC_HandleNCHitTest( hwnd, pt );
        }

    case WM_NCCALCSIZE:
        return NC_HandleNCCalcSize( hwnd, (RECT *)lParam );

    case WM_WINDOWPOSCHANGING:
        return WINPOS_HandleWindowPosChanging( hwnd, (WINDOWPOS *)lParam );

    case WM_WINDOWPOSCHANGED:
        DEFWND_HandleWindowPosChanged( hwnd, (const WINDOWPOS *)lParam );
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        iF10Key = iMenuSysKey = 0;
        break;

    case WM_NCLBUTTONDOWN:
        return NC_HandleNCLButtonDown( hwnd, wParam, lParam );

    case WM_LBUTTONDBLCLK:
        return NC_HandleNCLButtonDblClk( hwnd, HTCLIENT, lParam );

    case WM_NCLBUTTONDBLCLK:
        return NC_HandleNCLButtonDblClk( hwnd, wParam, lParam );

    case WM_NCRBUTTONDOWN:
        /* in Windows, capture is taken when right-clicking on the caption bar */
        if (wParam==HTCAPTION)
        {
            SetCapture(hwnd);
        }
        break;

    case WM_RBUTTONUP:
        {
            POINT pt;

            if (hwnd == GetCapture())
                /* release capture if we took it on WM_NCRBUTTONDOWN */
                ReleaseCapture();

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            ClientToScreen(hwnd, &pt);
            SendMessageW( hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELPARAM(pt.x, pt.y) );
        }
        break;

    case WM_NCRBUTTONUP:
        /*
         * FIXME : we must NOT send WM_CONTEXTMENU on a WM_NCRBUTTONUP (checked
         * in Windows), but what _should_ we do? According to MSDN :
         * "If it is appropriate to do so, the system sends the WM_SYSCOMMAND
         * message to the window". When is it appropriate?
         */
        break;

    case WM_CONTEXTMENU:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
            SendMessageW( GetParent(hwnd), msg, wParam, lParam );
        else
        {
            LONG hitcode;
            POINT pt;
            WND *wndPtr = WIN_GetPtr( hwnd );
            HMENU hMenu = wndPtr->hSysMenu;
            WIN_ReleasePtr( wndPtr );
            if (!hMenu) return 0;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            hitcode = NC_HandleNCHitTest(hwnd, pt);

            /* Track system popup if click was in the caption area. */
            if (hitcode==HTCAPTION || hitcode==HTSYSMENU)
               TrackPopupMenu(GetSystemMenu(hwnd, FALSE),
                               TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                               pt.x, pt.y, 0, hwnd, NULL);
        }
        break;

    case WM_POPUPSYSTEMMENU:
        {
            /* This is an undocumented message used by the windows taskbar to
               display the system menu of windows that belong to other processes. */
            HMENU menu = GetSystemMenu(hwnd, FALSE);

            if (menu)
                TrackPopupMenu(menu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                               LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);
            return 0;
        }

    case WM_NCACTIVATE:
        return NC_HandleNCActivate( hwnd, wParam, lParam );

    case WM_NCDESTROY:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            if (!wndPtr) return 0;
            HeapFree( GetProcessHeap(), 0, wndPtr->text );
            wndPtr->text = NULL;
            HeapFree( GetProcessHeap(), 0, wndPtr->pScroll );
            wndPtr->pScroll = NULL;
            WIN_ReleasePtr( wndPtr );
            return 0;
        }

    case WM_PRINT:
        DEFWND_Print(hwnd, (HDC)wParam, lParam);
        return 0;

    case WM_PAINTICON:
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );
            if( hdc )
            {
              HICON hIcon;
              if (IsIconic(hwnd) && ((hIcon = (HICON)GetClassLongPtrW( hwnd, GCLP_HICON))) )
              {
                  RECT rc;
                  int x, y;

                  GetClientRect( hwnd, &rc );
                  x = (rc.right - rc.left - GetSystemMetrics(SM_CXICON))/2;
                  y = (rc.bottom - rc.top - GetSystemMetrics(SM_CYICON))/2;
                  TRACE("Painting class icon: vis rect=(%s)\n",
                        wine_dbgstr_rect(&ps.rcPaint));
                  DrawIcon( hdc, x, y, hIcon );
              }
              EndPaint( hwnd, &ps );
            }
            return 0;
        }

    case WM_SYNCPAINT:
        RedrawWindow ( hwnd, NULL, 0, RDW_ERASENOW | RDW_ERASE | RDW_ALLCHILDREN );
        return 0;

    case WM_SETREDRAW:
        if (wParam) WIN_SetStyle( hwnd, WS_VISIBLE, 0 );
        else
        {
            RedrawWindow( hwnd, NULL, 0, RDW_ALLCHILDREN | RDW_VALIDATE );
            WIN_SetStyle( hwnd, 0, WS_VISIBLE );
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow( hwnd );
        return 0;

    case WM_MOUSEACTIVATE:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
        {
            LONG ret = SendMessageW( GetParent(hwnd), WM_MOUSEACTIVATE, wParam, lParam );
            if (ret) return ret;
        }

        /* Caption clicks are handled by NC_HandleNCLButtonDown() */
        return MA_ACTIVATE;

    case WM_ACTIVATE:
        /* The default action in Windows is to set the keyboard focus to
         * the window, if it's being activated and not minimized */
        if (LOWORD(wParam) != WA_INACTIVE) {
            if (!IsIconic(hwnd)) SetFocus(hwnd);
        }
        break;

    case WM_MOUSEWHEEL:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
            return SendMessageW( GetParent(hwnd), WM_MOUSEWHEEL, wParam, lParam );
        break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
        {
            RECT rect;
            HDC hdc = (HDC)wParam;
            HBRUSH hbr = (HBRUSH)GetClassLongPtrW( hwnd, GCLP_HBRBACKGROUND );
            if (!hbr) return 0;

            if (GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC)
            {
                /* can't use GetClipBox with a parent DC or we fill the whole parent */
                GetClientRect( hwnd, &rect );
                DPtoLP( hdc, (LPPOINT)&rect, 2 );
            }
            else GetClipBox( hdc, &rect );
            FillRect( hdc, &rect, hbr );
            return 1;
        }

    case WM_GETDLGCODE:
        return 0;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORSCROLLBAR:
        return (LRESULT)DEFWND_ControlColor( (HDC)wParam, msg - WM_CTLCOLORMSGBOX );

    case WM_CTLCOLOR:
        return (LRESULT)DEFWND_ControlColor( (HDC)wParam, HIWORD(lParam) );

    case WM_SETCURSOR:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
        {
            /* with the exception of the border around a resizable wnd,
             * give the parent first chance to set the cursor */
            if ((LOWORD(lParam) < HTSIZEFIRST) || (LOWORD(lParam) > HTSIZELAST))
            {
                if (SendMessageW(GetParent(hwnd), WM_SETCURSOR, wParam, lParam)) return TRUE;
            }
        }
        NC_HandleSetCursor( hwnd, wParam, lParam );
        break;

    case WM_SYSCOMMAND:
        return NC_HandleSysCommand( hwnd, wParam, lParam );

    case WM_KEYDOWN:
        if(wParam == VK_F10) iF10Key = VK_F10;
        break;

    case WM_SYSKEYDOWN:
        if( HIWORD(lParam) & KEYDATA_ALT )
        {
            /* if( HIWORD(lParam) & ~KEYDATA_PREVSTATE ) */
              if ( (wParam == VK_MENU || wParam == VK_LMENU
                    || wParam == VK_RMENU) && !iMenuSysKey )
                iMenuSysKey = 1;
              else
                iMenuSysKey = 0;

            iF10Key = 0;

            if( wParam == VK_F4 )       /* try to close the window */
            {
                HWND top = GetAncestor( hwnd, GA_ROOT );
                if (!(GetClassLongW( top, GCL_STYLE ) & CS_NOCLOSE))
                    PostMessageW( top, WM_SYSCOMMAND, SC_CLOSE, 0 );
            }
        }
        else if( wParam == VK_F10 )
            iF10Key = 1;
        else if( wParam == VK_ESCAPE && (GetKeyState(VK_SHIFT) & 0x8000))
            SendMessageW( hwnd, WM_SYSCOMMAND, SC_KEYMENU, ' ' );
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        /* Press and release F10 or ALT */
        if (((wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU)
             && iMenuSysKey) || ((wParam == VK_F10) && iF10Key))
              SendMessageW( GetAncestor( hwnd, GA_ROOT ), WM_SYSCOMMAND, SC_KEYMENU, 0L );
        iMenuSysKey = iF10Key = 0;
        break;

    case WM_SYSCHAR:
    {
        iMenuSysKey = 0;
        if (wParam == '\r' && IsIconic(hwnd))
        {
            PostMessageW( hwnd, WM_SYSCOMMAND, SC_RESTORE, 0L );
            break;
        }
        if ((HIWORD(lParam) & KEYDATA_ALT) && wParam)
        {
            if (wParam == '\t' || wParam == '\x1b') break;
            if (wParam == ' ' && (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD))
                SendMessageW( GetParent(hwnd), msg, wParam, lParam );
            else
                SendMessageW( hwnd, WM_SYSCOMMAND, SC_KEYMENU, wParam );
        }
        else /* check for Ctrl-Esc */
            if (wParam != '\x1b') MessageBeep(0);
        break;
    }

    case WM_SHOWWINDOW:
        {
            LONG style = GetWindowLongW( hwnd, GWL_STYLE );
            WND *pWnd;
            if (!lParam) return 0; /* sent from ShowWindow */
            if ((style & WS_VISIBLE) && wParam) return 0;
            if (!(style & WS_VISIBLE) && !wParam) return 0;
            if (!GetWindow( hwnd, GW_OWNER )) return 0;
            if (!(pWnd = WIN_GetPtr( hwnd ))) return 0;
            if (pWnd == WND_OTHER_PROCESS) return 0;
            if (wParam)
            {
                if (!(pWnd->flags & WIN_NEEDS_SHOW_OWNEDPOPUP))
                {
                    WIN_ReleasePtr( pWnd );
                    return 0;
                }
                pWnd->flags &= ~WIN_NEEDS_SHOW_OWNEDPOPUP;
            }
            else pWnd->flags |= WIN_NEEDS_SHOW_OWNEDPOPUP;
            WIN_ReleasePtr( pWnd );
            ShowWindow( hwnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE );
            break;
        }

    case WM_CANCELMODE:
        iMenuSysKey = 0;
        MENU_EndMenu( hwnd );
        if (GetCapture() == hwnd) ReleaseCapture();
        break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        return -1;

    case WM_DROPOBJECT:
        return DRAG_FILE;

    case WM_QUERYDROPOBJECT:
        return (GetWindowLongA( hwnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES) != 0;

    case WM_QUERYDRAGICON:
        {
            UINT len;

            HICON hIcon = (HICON)GetClassLongPtrW( hwnd, GCLP_HICON );
            HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
            if (hIcon) return (LRESULT)hIcon;
            for(len=1; len<64; len++)
                if((hIcon = LoadIconW(instance, MAKEINTRESOURCEW(len))))
                    return (LRESULT)hIcon;
            return (LRESULT)LoadIconW(0, (LPWSTR)IDI_APPLICATION);
        }
        break;

    case WM_ISACTIVEICON:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            BOOL ret = (wndPtr->flags & WIN_NCACTIVATED) != 0;
            WIN_ReleasePtr( wndPtr );
            return ret;
        }

    case WM_NOTIFYFORMAT:
      if (IsWindowUnicode(hwnd)) return NFR_UNICODE;
      else return NFR_ANSI;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
        return 1;

    case WM_SETICON:
        {
            HICON ret;
            WND *wndPtr = WIN_GetPtr( hwnd );

            switch(wParam)
            {
            case ICON_SMALL:
                ret = wndPtr->hIconSmall;
                wndPtr->hIconSmall = (HICON)lParam;
                break;
            case ICON_BIG:
                ret = wndPtr->hIcon;
                wndPtr->hIcon = (HICON)lParam;
                break;
            default:
                ret = 0;
                break;
            }
            WIN_ReleasePtr( wndPtr );

            USER_Driver->pSetWindowIcon( hwnd, wParam, (HICON)lParam );

            if( (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION )
                NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */

            return (LRESULT)ret;
        }

    case WM_GETICON:
        {
            HICON ret;
            WND *wndPtr = WIN_GetPtr( hwnd );

            switch(wParam)
            {
            case ICON_SMALL:
                ret = wndPtr->hIconSmall;
                break;
            case ICON_BIG:
                ret = wndPtr->hIcon;
                break;
            case ICON_SMALL2:
                ret = wndPtr->hIconSmall;
                if (!ret) ret = (HICON)GetClassLongPtrW( hwnd, GCLP_HICONSM );
                /* FIXME: should have a default here if class icon is null */
                break;
            default:
                ret = 0;
                break;
            }
            WIN_ReleasePtr( wndPtr );
            return (LRESULT)ret;
        }

    case WM_HELP:
        SendMessageW( GetParent(hwnd), msg, wParam, lParam );
        break;

    case WM_APPCOMMAND:
        {
            HWND parent = GetParent(hwnd);
            if(!parent)
                HOOK_CallHooks(WH_SHELL, HSHELL_APPCOMMAND, wParam, lParam, TRUE);
            else
                SendMessageW( parent, msg, wParam, lParam );
            break;
        }
    case WM_KEYF1:
        {
            HELPINFO hi;

            hi.cbSize = sizeof(HELPINFO);
            GetCursorPos( &hi.MousePos );
            if (MENU_IsMenuActive())
            {
                hi.iContextType = HELPINFO_MENUITEM;
                hi.hItemHandle = MENU_IsMenuActive();
                hi.iCtrlId = MenuItemFromPoint( hwnd, hi.hItemHandle, hi.MousePos );
                hi.dwContextId = GetMenuContextHelpId( hi.hItemHandle );
            }
            else
            {
                hi.iContextType = HELPINFO_WINDOW;
                hi.hItemHandle = hwnd;
                hi.iCtrlId = GetWindowLongPtrA( hwnd, GWLP_ID );
                hi.dwContextId = GetWindowContextHelpId( hwnd );
            }
            SendMessageW( hwnd, WM_HELP, 0, (LPARAM)&hi );
            break;
        }

    case WM_INPUTLANGCHANGEREQUEST:
        ActivateKeyboardLayout( (HKL)lParam, 0 );
        break;

    case WM_INPUTLANGCHANGE:
        {
            int count = 0;
            HWND *win_array = WIN_ListChildren( hwnd );

            if (!win_array)
                break;
            while (win_array[count])
                SendMessageW( win_array[count++], WM_INPUTLANGCHANGE, wParam, lParam);
            HeapFree(GetProcessHeap(),0,win_array);
            break;
        }

    }

    return 0;
}

static LPARAM DEFWND_GetTextA( WND *wndPtr, LPSTR dest, WPARAM wParam )
{
    LPARAM result = 0;

    __TRY
    {
        if (wndPtr->text)
        {
            if (!WideCharToMultiByte( CP_ACP, 0, wndPtr->text, -1,
                                      dest, wParam, NULL, NULL )) dest[wParam-1] = 0;
            result = strlen( dest );
        }
        else dest[0] = '\0';
    }
    __EXCEPT_PAGE_FAULT
    {
        return 0;
    }
    __ENDTRY
    return result;
}

/***********************************************************************
 *              DefWindowProcA (USER32.@)
 *
 * See DefWindowProcW.
 */
LRESULT WINAPI DefWindowProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    LRESULT result = 0;
    HWND full_handle;

    if (!(full_handle = WIN_IsCurrentProcess( hwnd )))
    {
        if (!IsWindow( hwnd )) return 0;
        ERR( "called for other process window %p\n", hwnd );
        return 0;
    }
    hwnd = full_handle;

    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
        if (lParam)
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
            /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
             * may have child window IDs instead of window name */
            if (HIWORD(cs->lpszName))
                DEFWND_SetTextA( hwnd, cs->lpszName );
            result = 1;
        }
        break;

    case WM_GETTEXTLENGTH:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            if (wndPtr && wndPtr->text)
                result = WideCharToMultiByte( CP_ACP, 0, wndPtr->text, strlenW(wndPtr->text),
                                              NULL, 0, NULL, NULL );
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_GETTEXT:
        if (wParam)
        {
            LPSTR dest = (LPSTR)lParam;
            WND *wndPtr = WIN_GetPtr( hwnd );

            if (!wndPtr) break;
            result = DEFWND_GetTextA( wndPtr, dest, wParam );

            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_SETTEXT:
        DEFWND_SetTextA( hwnd, (LPCSTR)lParam );
        if( (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION )
            NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
        result = 1; /* success. FIXME: check text length */
        break;

    case WM_IME_CHAR:
        if (HIBYTE(wParam)) PostMessageA( hwnd, WM_CHAR, HIBYTE(wParam), lParam );
        PostMessageA( hwnd, WM_CHAR, LOBYTE(wParam), lParam );
        break;

    case WM_IME_KEYDOWN:
        result = PostMessageA( hwnd, WM_KEYDOWN, wParam, lParam );
        break;

    case WM_IME_KEYUP:
        result = PostMessageA( hwnd, WM_KEYUP, wParam, lParam );
        break;

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_SELECT:
    case WM_IME_NOTIFY:
        {
            HWND hwndIME;

            hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
            if (hwndIME)
                result = SendMessageA( hwndIME, msg, wParam, lParam );
        }
        break;
    case WM_IME_SETCONTEXT:
        {
            HWND hwndIME;

            hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
            if (hwndIME)
                result = DEFWND_ImmIsUIMessageA( hwndIME, msg, wParam, lParam );
        }
        break;

    case WM_SYSCHAR:
    {
        CHAR ch = LOWORD(wParam);
        WCHAR wch;
        MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
        wParam = MAKEWPARAM( wch, HIWORD(wParam) );
    }
    /* fall through */
    default:
        result = DEFWND_DefWinProc( hwnd, msg, wParam, lParam );
        break;
    }

    SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result, wParam, lParam );
    return result;
}


static LPARAM DEFWND_GetTextW( WND *wndPtr, LPWSTR dest, WPARAM wParam )
{
    LPARAM result = 0;

    __TRY
    {
        if (wndPtr->text)
        {
            lstrcpynW( dest, wndPtr->text, wParam );
            result = strlenW( dest );
        }
        else dest[0] = '\0';
    }
    __EXCEPT_PAGE_FAULT
    {
        return 0;
    }
    __ENDTRY

    return result;
}

/***********************************************************************
 *              DefWindowProcW (USER32.@) Calls default window message handler
 *
 * Calls default window procedure for messages not processed
 *  by application.
 *
 *  RETURNS
 *     Return value is dependent upon the message.
*/
LRESULT WINAPI DefWindowProcW(
    HWND hwnd,      /* [in] window procedure receiving message */
    UINT msg,       /* [in] message identifier */
    WPARAM wParam,  /* [in] first message parameter */
    LPARAM lParam )   /* [in] second message parameter */
{
    LRESULT result = 0;
    HWND full_handle;

    if (!(full_handle = WIN_IsCurrentProcess( hwnd )))
    {
        if (!IsWindow( hwnd )) return 0;
        ERR( "called for other process window %p\n", hwnd );
        return 0;
    }
    hwnd = full_handle;
    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
        if (lParam)
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
            /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
             * may have child window IDs instead of window name */
            if (HIWORD(cs->lpszName))
                DEFWND_SetTextW( hwnd, cs->lpszName );
            result = 1;
        }
        break;

    case WM_GETTEXTLENGTH:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            if (wndPtr && wndPtr->text) result = (LRESULT)strlenW(wndPtr->text);
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_GETTEXT:
        if (wParam)
        {
            LPWSTR dest = (LPWSTR)lParam;
            WND *wndPtr = WIN_GetPtr( hwnd );

            if (!wndPtr) break;
            result = DEFWND_GetTextW( wndPtr, dest, wParam );
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_SETTEXT:
        DEFWND_SetTextW( hwnd, (LPCWSTR)lParam );
        if( (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION )
            NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
        result = 1; /* success. FIXME: check text length */
        break;

    case WM_IME_CHAR:
        PostMessageW( hwnd, WM_CHAR, wParam, lParam );
        break;

    case WM_IME_KEYDOWN:
        result = PostMessageW( hwnd, WM_KEYDOWN, wParam, lParam );
        break;

    case WM_IME_KEYUP:
        result = PostMessageW( hwnd, WM_KEYUP, wParam, lParam );
        break;

    case WM_IME_SETCONTEXT:
        {
            HWND hwndIME;

            hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
            if (hwndIME)
                result = DEFWND_ImmIsUIMessageW( hwndIME, msg, wParam, lParam );
        }
        break;

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_SELECT:
    case WM_IME_NOTIFY:
        {
            HWND hwndIME;

            hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
            if (hwndIME)
                result = SendMessageW( hwndIME, msg, wParam, lParam );
        }
        break;

    default:
        result = DEFWND_DefWinProc( hwnd, msg, wParam, lParam );
        break;
    }
    SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result, wParam, lParam );
    return result;
}
