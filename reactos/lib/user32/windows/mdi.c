/* MDI.C
 *
 * Copyright 1994, Bob Amstadt
 *           1995,1996 Alex Korobka
 *
 * This file contains routines to support MDI (Multiple Document
 * Interface) features .
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Notes: Fairly complete implementation.
 *        Also, Excel and WinWord do _not_ use MDI so if you're trying
 *	  to fix them look elsewhere.
 *
 * Notes on how the "More Windows..." is implemented:
 *
 *      When we have more than 9 opened windows, a "More Windows..."
 *      option appears in the "Windows" menu. Each child window has
 *      a WND* associated with it, accesible via the children list of
 *      the parent window. This WND* has a wIDmenu member, which reflects
 *      the position of the child in the window list. For example, with
 *      9 child windows, we could have the following pattern:
 *
 *
 *
 *                Name of the child window    pWndChild->wIDmenu
 *                     Doc1                       5000
 *                     Doc2                       5001
 *                     Doc3                       5002
 *                     Doc4                       5003
 *                     Doc5                       5004
 *                     Doc6                       5005
 *                     Doc7                       5006
 *                     Doc8                       5007
 *                     Doc9                       5008
 *
 *
 *       The "Windows" menu, as the "More windows..." dialog, are constructed
 *       in this order. If we add a child, we would have the following list:
 *
 *
 *               Name of the child window    pWndChild->wIDmenu
 *                     Doc1                       5000
 *                     Doc2                       5001
 *                     Doc3                       5002
 *                     Doc4                       5003
 *                     Doc5                       5004
 *                     Doc6                       5005
 *                     Doc7                       5006
 *                     Doc8                       5007
 *                     Doc9                       5008
 *                     Doc10                      5009
 *
 *       But only 5000 to 5008 would be displayed in the "Windows" menu. We want
 *       the last created child to be in the menu, so we swap the last child with
 *       the 9th... Doc9 will be accessible via the "More Windows..." option.
 *
 *                     Doc1                       5000
 *                     Doc2                       5001
 *                     Doc3                       5002
 *                     Doc4                       5003
 *                     Doc5                       5004
 *                     Doc6                       5005
 *                     Doc7                       5006
 *                     Doc8                       5007
 *                     Doc9                       5009
 *                     Doc10                      5008
 *
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#ifndef __REACTOS__
#include "wownt32.h"
#endif
#include "wine/unicode.h"
#ifndef __REACTOS__
#include "win.h"
#include "nonclient.h"
#include "controls.h"
#include "user.h"
#include "struct32.h"
#else
#include "user32/regcontrol.h"
#include <winnls.h>
#endif
#include "wine/debug.h"
#include "dlgs.h"

WINE_DEFAULT_DEBUG_CHANNEL(mdi);

#define MDI_MAXLISTLENGTH       0x40
#define MDI_MAXTITLELENGTH      0xa1

#define MDI_NOFRAMEREPAINT      0
#define MDI_REPAINTFRAMENOW     1
#define MDI_REPAINTFRAME        2

#define WM_MDICALCCHILDSCROLL   0x10ac /* this is exactly what Windows uses */

/* "More Windows..." definitions */
#define MDI_MOREWINDOWSLIMIT    9       /* after this number of windows, a "More Windows..."
                                           option will appear under the Windows menu */
#define MDI_IDC_LISTBOX         100
#define IDS_MDI_MOREWINDOWS     13

#define MDIF_NEEDUPDATE		0x0001

typedef struct
{
    UINT      nActiveChildren;
    HWND      hwndChildMaximized;
    HWND      hwndActiveChild;
    HMENU     hWindowMenu;
    UINT      idFirstChild;
    LPWSTR    frameTitle;
    UINT      nTotalCreated;
    UINT      mdiFlags;
    UINT      sbRecalc;   /* SB_xxx flags for scrollbar fixup */
} MDICLIENTINFO;

static HBITMAP hBmpClose   = 0;

/* ----------------- declarations ----------------- */
static void MDI_UpdateFrameText( HWND, HWND, BOOL, LPCWSTR);
static BOOL MDI_AugmentFrameMenu( HWND, HWND );
static BOOL MDI_RestoreFrameMenu( HWND, HWND );
static LONG MDI_ChildActivate( HWND, HWND );

static HWND MDI_MoreWindowsDialog(HWND);
static void MDI_SwapMenuItems(HWND, UINT, UINT);
static LRESULT WINAPI MDIClientWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI MDIClientWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

#ifdef __REACTOS__
void WINAPI ScrollChildren(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void WINAPI CalcChildScroll(HWND hwnd, INT scroll);

BOOL CALLBACK MDI_GetChildByID_EnumProc (HWND hwnd, LPARAM lParam )
{
    DWORD *control = (DWORD *)lParam;
    if(*control == GetWindowLongW( hwnd, GWL_ID ))
    {        
        *control = (DWORD)hwnd;        
        return FALSE;
    }
    return TRUE;
}
#endif

/* -------- Miscellaneous service functions ----------
 *
 *			MDI_GetChildByID
 */
static HWND MDI_GetChildByID(HWND hwnd, UINT id)
{
#ifdef __REACTOS__
    DWORD Control = id;
    if (hwnd && !EnumChildWindows(hwnd, (ENUMWINDOWSPROC)&MDI_GetChildByID_EnumProc, (LPARAM)&Control))
    {
        return (HWND)Control;
    }
    return 0;
#else
    HWND ret;
    HWND *win_array;
    int i;

    if (!(win_array = WIN_ListChildren( hwnd ))) return 0;
    for (i = 0; win_array[i]; i++)
    {
        if (GetWindowLongA( win_array[i], GWL_ID ) == id) break;
    }
    ret = win_array[i];
    HeapFree( GetProcessHeap(), 0, win_array );
    return ret;
#endif
}

static void MDI_PostUpdate(HWND hwnd, MDICLIENTINFO* ci, WORD recalc)
{
    if( !(ci->mdiFlags & MDIF_NEEDUPDATE) )
    {
	ci->mdiFlags |= MDIF_NEEDUPDATE;
	PostMessageA( hwnd, WM_MDICALCCHILDSCROLL, 0, 0);
    }
    ci->sbRecalc = recalc;
}


/*********************************************************************
 * MDIClient class descriptor
 */
const struct builtin_class_descr MDICLIENT_builtin_class =
{
    L"MDIClient",            /* name */
    0,                       /* style */
    MDIClientWndProcW,       /* procW */
    MDIClientWndProcA,       /* procA */
    sizeof(MDICLIENTINFO *), /* extra */
    IDC_ARROW,               /* cursor */
    (HBRUSH)(COLOR_APPWORKSPACE+1)    /* brush */
};


static MDICLIENTINFO *get_client_info( HWND client )
{
#ifdef __REACTOS__
    return (MDICLIENTINFO *)GetWindowLongPtr(client, 0);
#else
    MDICLIENTINFO *ret = NULL;
    WND *win = WIN_GetPtr( client );
    if (win)
    {
        if (win == WND_OTHER_PROCESS)
        {
            ERR( "client %p belongs to other process\n", client );
            return NULL;
        }
        if (win->cbWndExtra < sizeof(MDICLIENTINFO)) WARN( "%p is not an MDI client\n", client );
        else ret = (MDICLIENTINFO *)win->wExtra;
        WIN_ReleasePtr( win );
    }
    return ret;
#endif
}

/**********************************************************************
 *			MDI_MenuModifyItem
 */
static void MDI_MenuModifyItem( HWND client, HWND hWndChild )
{
    MDICLIENTINFO *clientInfo = get_client_info( client );
    WCHAR buffer[128];
    UINT n, id;

    if (!clientInfo || !clientInfo->hWindowMenu) return;

    id = GetWindowLongA( hWndChild, GWL_ID );
    if (id >= clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT) return;
    buffer[0] = '&';
    buffer[1] = '1' + id - clientInfo->idFirstChild;
    buffer[2] = ' ';
    GetWindowTextW( hWndChild, buffer + 3, sizeof(buffer)/sizeof(WCHAR) - 3 );

    n = GetMenuState(clientInfo->hWindowMenu, id, MF_BYCOMMAND);
    ModifyMenuW(clientInfo->hWindowMenu, id, MF_BYCOMMAND | MF_STRING, id, buffer );
    CheckMenuItem(clientInfo->hWindowMenu, id, n & MF_CHECKED);
}

/**********************************************************************
 *			MDI_MenuDeleteItem
 */
static BOOL MDI_MenuDeleteItem( HWND client, HWND hWndChild )
{
    WCHAR    	 buffer[128];
    static const WCHAR format[] = {'&','%','d',' ',0};
    MDICLIENTINFO *clientInfo = get_client_info( client );
    UINT	 index      = 0,id,n;

    if( !clientInfo->nActiveChildren || !clientInfo->hWindowMenu )
        return FALSE;

    id = GetWindowLongA( hWndChild, GWL_ID );
    DeleteMenu(clientInfo->hWindowMenu,id,MF_BYCOMMAND);

 /* walk the rest of MDI children to prevent gaps in the id
  * sequence and in the menu child list */

    for( index = id+1; index <= clientInfo->nActiveChildren +
				clientInfo->idFirstChild; index++ )
    {
        HWND hwnd = MDI_GetChildByID(client,index);
        if (!hwnd)
        {
            TRACE("no window for id=%i\n",index);
            continue;
        }

	/* set correct id */
        SetWindowLongW( hwnd, GWL_ID, GetWindowLongW( hwnd, GWL_ID ) - 1 );

	n = wsprintfW(buffer, format ,index - clientInfo->idFirstChild);
        GetWindowTextW( hwnd, buffer + n, sizeof(buffer)/sizeof(WCHAR) - n );

	/*  change menu if the current child is to be shown in the
         *  "Windows" menu
         */
        if (index <= clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT)
	ModifyMenuW(clientInfo->hWindowMenu ,index ,MF_BYCOMMAND | MF_STRING,
                      index - 1 , buffer );
    }

    /*  We must restore the "More Windows..." option if there are enough children
     */
    if (clientInfo->nActiveChildren - 1 > MDI_MOREWINDOWSLIMIT)
    {
        WCHAR szTmp[50];
        LoadStringW(GetModuleHandleA("USER32"), IDS_MDI_MOREWINDOWS, szTmp, sizeof(szTmp)/sizeof(szTmp[0]));
        AppendMenuW(clientInfo->hWindowMenu, MF_STRING, clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT, szTmp);
    }
    return TRUE;
}

/**********************************************************************
 * 			MDI_GetWindow
 *
 * returns "activateable" child different from the current or zero
 */
static HWND MDI_GetWindow(MDICLIENTINFO *clientInfo, HWND hWnd, BOOL bNext,
                            DWORD dwStyleMask )
{
#ifdef __REACTOS__
    /* FIXME */
    return 0;
#else
    int i;
    HWND *list;
    HWND last = 0;

    dwStyleMask |= WS_DISABLED | WS_VISIBLE;
    if( !hWnd ) hWnd = clientInfo->hwndActiveChild;

    if (!(list = WIN_ListChildren( GetParent(hWnd) ))) return 0;
    i = 0;
    /* start from next after hWnd */
    while (list[i] && list[i] != hWnd) i++;
    if (list[i]) i++;

    for ( ; list[i]; i++)
    {
        if (GetWindow( list[i], GW_OWNER )) continue;
        if ((GetWindowLongW( list[i], GWL_STYLE ) & dwStyleMask) != WS_VISIBLE) continue;
        last = list[i];
        if (bNext) goto found;
    }
    /* now restart from the beginning */
    for (i = 0; list[i] && list[i] != hWnd; i++)
    {
        if (GetWindow( list[i], GW_OWNER )) continue;
        if ((GetWindowLongW( list[i], GWL_STYLE ) & dwStyleMask) != WS_VISIBLE) continue;
        last = list[i];
        if (bNext) goto found;
    }
 found:
    HeapFree( GetProcessHeap(), 0, list );
    return last;
#endif
}

/**********************************************************************
 *			MDI_CalcDefaultChildPos
 *
 *  It seems that the default height is about 2/3 of the client rect
 */
static void MDI_CalcDefaultChildPos( HWND hwnd, WORD n, LPPOINT lpPos, INT delta)
{
    INT  nstagger;
    RECT rect;
    INT  spacing = GetSystemMetrics(SM_CYCAPTION) +
		     GetSystemMetrics(SM_CYFRAME) - 1;

    GetClientRect( hwnd, &rect );
    if( rect.bottom - rect.top - delta >= spacing )
	rect.bottom -= delta;

    nstagger = (rect.bottom - rect.top)/(3 * spacing);
    lpPos[1].x = (rect.right - rect.left - nstagger * spacing);
    lpPos[1].y = (rect.bottom - rect.top - nstagger * spacing);
    lpPos[0].x = lpPos[0].y = spacing * (n%(nstagger+1));
}

/**********************************************************************
 *            MDISetMenu
 */
static LRESULT MDISetMenu( HWND hwnd, HMENU hmenuFrame,
                           HMENU hmenuWindow)
{
    MDICLIENTINFO *ci;
    HWND hwndFrame = GetParent(hwnd);
    HMENU oldFrameMenu = GetMenu(hwndFrame);

    TRACE("%p %p %p\n", hwnd, hmenuFrame, hmenuWindow);

    if (hmenuFrame && !IsMenu(hmenuFrame))
    {
	WARN("hmenuFrame is not a menu handle\n");
	return 0L;
    }

    if (hmenuWindow && !IsMenu(hmenuWindow))
    {
	WARN("hmenuWindow is not a menu handle\n");
	return 0L;
    }

    if (!(ci = get_client_info( hwnd ))) return 0;

    if( ci->hwndChildMaximized && hmenuFrame && hmenuFrame!=oldFrameMenu )
        MDI_RestoreFrameMenu( GetParent(hwnd), ci->hwndChildMaximized );

    if( hmenuWindow && hmenuWindow != ci->hWindowMenu )
    {
        /* delete menu items from ci->hWindowMenu
         * and add them to hmenuWindow */
        /* Agent newsreader calls this function with  ci->hWindowMenu == NULL */
        if( ci->hWindowMenu && ci->nActiveChildren )
        {
            INT j;
            LPWSTR buffer = NULL;
	    MENUITEMINFOW mii;
            INT nbWindowsMenuItems; /* num of documents shown + "More Windows..." if present */
            INT i = GetMenuItemCount(ci->hWindowMenu) - 1;
            INT pos = GetMenuItemCount(hmenuWindow) + 1;

            AppendMenuA( hmenuWindow, MF_SEPARATOR, 0, NULL);

            if (ci->nActiveChildren <= MDI_MOREWINDOWSLIMIT)
                nbWindowsMenuItems = ci->nActiveChildren;
            else
                nbWindowsMenuItems = MDI_MOREWINDOWSLIMIT + 1;

            j = i - nbWindowsMenuItems + 1;

            for( ; i >= j ; i-- )
            {
		memset(&mii, 0, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE
		  | MIIM_SUBMENU | MIIM_TYPE | MIIM_BITMAP;

		GetMenuItemInfoW(ci->hWindowMenu, i, TRUE, &mii);
		if(mii.cch) { /* Menu is MFT_STRING */
		    mii.cch++; /* add room for '\0' */
		    buffer = HeapAlloc(GetProcessHeap(), 0,
				       mii.cch * sizeof(WCHAR));
		    mii.dwTypeData = buffer;
		    GetMenuItemInfoW(ci->hWindowMenu, i, TRUE, &mii);
		}
                DeleteMenu(ci->hWindowMenu, i, MF_BYPOSITION);
                InsertMenuItemW(hmenuWindow, pos, TRUE, &mii);
		if(buffer) {
		    HeapFree(GetProcessHeap(), 0, buffer);
		    buffer = NULL;
		}
            }
            /* remove separator */
            DeleteMenu(ci->hWindowMenu, i, MF_BYPOSITION);
        }
        ci->hWindowMenu = hmenuWindow;
    }

    if (hmenuFrame)
    {
        SetMenu(hwndFrame, hmenuFrame);
        if( hmenuFrame!=oldFrameMenu )
        {
            if( ci->hwndChildMaximized )
                MDI_AugmentFrameMenu( GetParent(hwnd), ci->hwndChildMaximized );
            return (LRESULT)oldFrameMenu;
        }
    }
    else
    {
        HMENU menu = GetMenu( GetParent(hwnd) );
	INT nItems = GetMenuItemCount(menu) - 1;
	UINT iId = GetMenuItemID(menu,nItems) ;

	if( !(iId == SC_RESTORE || iId == SC_CLOSE) )
	{
	    /* SetMenu() may already have been called, meaning that this window
	     * already has its menu. But they may have done a SetMenu() on
	     * an MDI window, and called MDISetMenu() after the fact, meaning
	     * that the "if" to this "else" wouldn't catch the need to
	     * augment the frame menu.
	     */
	    if( ci->hwndChildMaximized )
		MDI_AugmentFrameMenu( GetParent(hwnd), ci->hwndChildMaximized );
	}
    }
    return 0;
}

/**********************************************************************
 *            MDIRefreshMenu
 */
static LRESULT MDIRefreshMenu( HWND hwnd, HMENU hmenuFrame,
                           HMENU hmenuWindow)
{
    HWND hwndFrame = GetParent(hwnd);
    HMENU oldFrameMenu = GetMenu(hwndFrame);

    TRACE("%p %p %p\n", hwnd, hmenuFrame, hmenuWindow);

    FIXME("partially function stub\n");

    return (LRESULT)oldFrameMenu;
}


/* ------------------ MDI child window functions ---------------------- */


/**********************************************************************
 *					MDICreateChild
 */
static HWND MDICreateChild( HWND parent, MDICLIENTINFO *ci,
			    LPMDICREATESTRUCTA cs, BOOL unicode )
{
    POINT          pos[2];
    DWORD	     style = cs->style | (WS_CHILD | WS_CLIPSIBLINGS);
    HWND 	     hwnd, hwndMax = 0;
    UINT wIDmenu = ci->idFirstChild + ci->nActiveChildren;
#ifndef __REACTOS__
    WND *wndParent;
#endif
    static const WCHAR lpstrDef[] = {'j','u','n','k','!',0};

    TRACE("origin %i,%i - dim %i,%i, style %08lx\n",
                cs->x, cs->y, cs->cx, cs->cy, cs->style);
    /* calculate placement */
    MDI_CalcDefaultChildPos(parent, ci->nTotalCreated++, pos, 0);

#ifndef __REACTOS__
    if (cs->cx == CW_USEDEFAULT || cs->cx == CW_USEDEFAULT16 || !cs->cx) cs->cx = pos[1].x;
    if (cs->cy == CW_USEDEFAULT || cs->cy == CW_USEDEFAULT16 || !cs->cy) cs->cy = pos[1].y;

    if (cs->x == CW_USEDEFAULT || cs->x == CW_USEDEFAULT16)
#else
    if (cs->cx == CW_USEDEFAULT || !cs->cx) cs->cx = pos[1].x;
    if (cs->cy == CW_USEDEFAULT || !cs->cy) cs->cy = pos[1].y;

    if (cs->x == CW_USEDEFAULT)
#endif
    {
 	cs->x = pos[0].x;
	cs->y = pos[0].y;
    }

    /* restore current maximized child */
    if( (style & WS_VISIBLE) && ci->hwndChildMaximized )
    {
	TRACE("Restoring current maximized child %p\n", ci->hwndChildMaximized);
	if( style & WS_MAXIMIZE )
	    SendMessageW(parent, WM_SETREDRAW, FALSE, 0L);
	hwndMax = ci->hwndChildMaximized;
	ShowWindow( hwndMax, SW_SHOWNOACTIVATE );
	if( style & WS_MAXIMIZE )
	    SendMessageW(parent, WM_SETREDRAW, TRUE, 0L);
    }

    if (ci->nActiveChildren <= MDI_MOREWINDOWSLIMIT)
    /* this menu is needed to set a check mark in MDI_ChildActivate */
    if (ci->hWindowMenu != 0)
        AppendMenuW(ci->hWindowMenu, MF_STRING, wIDmenu, lpstrDef);

    ci->nActiveChildren++;

    /* fix window style */
#ifndef __REACTOS__
    wndParent = WIN_FindWndPtr( parent );
    if( !(wndParent->dwStyle & MDIS_ALLCHILDSTYLES) )
#else
    if( !(GetWindowLong(parent, GWL_STYLE) & MDIS_ALLCHILDSTYLES) )
#endif
    {
	TRACE("MDIS_ALLCHILDSTYLES is missing, fixing window style\n");
        style &= (WS_CHILD | WS_CLIPSIBLINGS | WS_MINIMIZE | WS_MAXIMIZE |
                  WS_CLIPCHILDREN | WS_DISABLED | WS_VSCROLL | WS_HSCROLL );
        style |= (WS_VISIBLE | WS_OVERLAPPEDWINDOW);
    }

#ifndef __REACTOS__
    if( wndParent->flags & WIN_ISWIN32 )
#endif
    {
#ifndef __REACTOS__
        WIN_ReleaseWndPtr( wndParent );
#endif
	if(unicode)
	{
	    MDICREATESTRUCTW *csW = (MDICREATESTRUCTW *)cs;
	    hwnd = CreateWindowW( csW->szClass, csW->szTitle, style,
                                csW->x, csW->y, csW->cx, csW->cy, parent,
                                (HMENU)wIDmenu, csW->hOwner, csW );
	}
	else
	    hwnd = CreateWindowA( cs->szClass, cs->szTitle, style,
                                cs->x, cs->y, cs->cx, cs->cy, parent,
                                (HMENU)wIDmenu, cs->hOwner, cs );
    }
#ifndef __REACTOS__
    else
    {
        MDICREATESTRUCT16 cs16;
        SEGPTR title, cls, seg_cs16;

        WIN_ReleaseWndPtr( wndParent );
        STRUCT32_MDICREATESTRUCT32Ato16( cs, &cs16 );
        cs16.szTitle = title = MapLS( cs->szTitle );
        cs16.szClass = cls = MapLS( cs->szClass );
        seg_cs16 = MapLS( &cs16 );
        hwnd = WIN_Handle32( CreateWindow16( cs->szClass, cs->szTitle, style,
                                             cs16.x, cs16.y, cs16.cx, cs16.cy,
                                             HWND_16(parent), (HMENU16)wIDmenu,
                                             cs16.hOwner, (LPVOID)seg_cs16 ));
        UnMapLS( seg_cs16 );
        UnMapLS( title );
        UnMapLS( cls );
    }
#endif

    /* MDI windows are WS_CHILD so they won't be activated by CreateWindow */

    if (hwnd)
    {
	/* All MDI child windows have the WS_EX_MDICHILD style */
        SetWindowLongW( hwnd, GWL_EXSTYLE, GetWindowLongW( hwnd, GWL_EXSTYLE ) | WS_EX_MDICHILD );

        /*  If we have more than 9 windows, we must insert the new one at the
         *  9th position in order to see it in the "Windows" menu
         */
        if (ci->nActiveChildren > MDI_MOREWINDOWSLIMIT)
            MDI_SwapMenuItems( parent, GetWindowLongW( hwnd, GWL_ID ),
                               ci->idFirstChild + MDI_MOREWINDOWSLIMIT - 1);

	MDI_MenuModifyItem(parent, hwnd);

        /* Have we hit the "More Windows..." limit? If so, we must
         * add a "More Windows..." option
         */
        if (ci->nActiveChildren == MDI_MOREWINDOWSLIMIT + 1)
        {
            WCHAR szTmp[50];
            LoadStringW(GetModuleHandleA("USER32"), IDS_MDI_MOREWINDOWS, szTmp, sizeof(szTmp)/sizeof(szTmp[0]));

            ModifyMenuW(ci->hWindowMenu,
                        ci->idFirstChild + MDI_MOREWINDOWSLIMIT,
                        MF_BYCOMMAND | MF_STRING,
                        ci->idFirstChild + MDI_MOREWINDOWSLIMIT,
                        szTmp);
        }

        if( IsIconic(hwnd) && ci->hwndActiveChild )
	{
	    TRACE("Minimizing created MDI child %p\n", hwnd);
	    ShowWindow( hwnd, SW_SHOWMINNOACTIVE );
	}
	else
	{
            /* WS_VISIBLE is clear if a) the MDI client has
             * MDIS_ALLCHILDSTYLES style and 2) the flag is cleared in the
             * MDICreateStruct. If so the created window is not shown nor
             * activated.
             */
            if (IsWindowVisible(hwnd)) ShowWindow(hwnd, SW_SHOW);
	}
        TRACE("created child - %p\n",hwnd);
    }
    else
    {
	ci->nActiveChildren--;
	DeleteMenu(ci->hWindowMenu,wIDmenu,MF_BYCOMMAND);
	if( IsWindow(hwndMax) )
	    ShowWindow(hwndMax, SW_SHOWMAXIMIZED);
    }

    return hwnd;
}

/**********************************************************************
 *			MDI_ChildGetMinMaxInfo
 *
 * Note: The rule here is that client rect of the maximized MDI child
 *	 is equal to the client rect of the MDI client window.
 */
static void MDI_ChildGetMinMaxInfo( HWND client, HWND hwnd, MINMAXINFO* lpMinMax )
{
    RECT rect;

    GetClientRect( client, &rect );
    AdjustWindowRectEx( &rect, GetWindowLongW( hwnd, GWL_STYLE ),
                        0, GetWindowLongW( hwnd, GWL_EXSTYLE ));

    lpMinMax->ptMaxSize.x = rect.right -= rect.left;
    lpMinMax->ptMaxSize.y = rect.bottom -= rect.top;

    lpMinMax->ptMaxPosition.x = rect.left;
    lpMinMax->ptMaxPosition.y = rect.top;

    TRACE("max rect (%ld,%ld - %ld, %ld)\n",
                        rect.left,rect.top,rect.right,rect.bottom);
}

/**********************************************************************
 *			MDI_SwitchActiveChild
 *
 * Note: SetWindowPos sends WM_CHILDACTIVATE to the child window that is
 *       being activated
 */
static void MDI_SwitchActiveChild( HWND clientHwnd, HWND childHwnd,
                                   BOOL bNextWindow )
{
    HWND	   hwndTo    = 0;
    HWND	   hwndPrev  = 0;
    MDICLIENTINFO *ci = get_client_info( clientHwnd );

    hwndTo = MDI_GetWindow(ci, childHwnd, bNextWindow, 0);

    TRACE("from %p, to %p\n",childHwnd,hwndTo);

    if ( !hwndTo ) return; /* no window to switch to */

    hwndPrev = ci->hwndActiveChild;

    if ( hwndTo != hwndPrev )
    {
	SetWindowPos( hwndTo, HWND_TOP, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE );

	if( bNextWindow && hwndPrev )
	    SetWindowPos( hwndPrev, HWND_BOTTOM, 0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
    }
}


/**********************************************************************
 *                                      MDIDestroyChild
 */
static LRESULT MDIDestroyChild( HWND parent, MDICLIENTINFO *ci,
                                HWND child, BOOL flagDestroy )
{
    if( child == ci->hwndActiveChild )
    {
        MDI_SwitchActiveChild(parent, child, TRUE);

        if( child == ci->hwndActiveChild )
        {
            ShowWindow( child, SW_HIDE);
            if( child == ci->hwndChildMaximized )
            {
                HWND frame = GetParent(parent);
                MDI_RestoreFrameMenu( frame, child );
                ci->hwndChildMaximized = 0;
                MDI_UpdateFrameText( frame, parent, TRUE, NULL);
            }

            MDI_ChildActivate(parent, 0);
        }
    }

    MDI_MenuDeleteItem(parent, child);

    ci->nActiveChildren--;

    TRACE("child destroyed - %p\n",child);

    if (flagDestroy)
    {
        MDI_PostUpdate(GetParent(child), ci, SB_BOTH+1);
        DestroyWindow(child);
    }
    return 0;
}


/**********************************************************************
 *					MDI_ChildActivate
 *
 * Note: hWndChild is NULL when last child is being destroyed
 */
static LONG MDI_ChildActivate( HWND client, HWND child )
{
    MDICLIENTINFO *clientInfo = get_client_info( client );
    HWND prevActiveWnd = clientInfo->hwndActiveChild;
    BOOL isActiveFrameWnd;

    if (child && (!IsWindowEnabled( child ))) return 0;

    /* Don't activate if it is already active. Might happen
       since ShowWindow DOES activate MDI children */
    if (clientInfo->hwndActiveChild == child) return 0;

    TRACE("%p\n", child);

    isActiveFrameWnd = (GetActiveWindow() == GetParent(client));

    /* deactivate prev. active child */
    if(prevActiveWnd)
    {
        SetWindowLongA( prevActiveWnd, GWL_STYLE,
                        GetWindowLongA( prevActiveWnd, GWL_STYLE ) | WS_SYSMENU );
	SendMessageA( prevActiveWnd, WM_NCACTIVATE, FALSE, 0L );
        SendMessageA( prevActiveWnd, WM_MDIACTIVATE, (WPARAM)prevActiveWnd, (LPARAM)child);
        /* uncheck menu item */
       	if( clientInfo->hWindowMenu )
        {
            UINT prevID = GetWindowLongA( prevActiveWnd, GWL_ID );

            if (prevID - clientInfo->idFirstChild < MDI_MOREWINDOWSLIMIT)
                CheckMenuItem( clientInfo->hWindowMenu, prevID, 0);
            else
       	        CheckMenuItem( clientInfo->hWindowMenu,
                               clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT - 1, 0);
        }
    }

    /* set appearance */
    if (clientInfo->hwndChildMaximized && clientInfo->hwndChildMaximized != child)
    {
        INT cmd = SW_SHOWNORMAL;

        if( child )
        {
            UINT state = GetMenuState(GetSystemMenu(child, FALSE), SC_MAXIMIZE, MF_BYCOMMAND);
            if (state != 0xFFFFFFFF && (state & (MF_DISABLED | MF_GRAYED)))
                SendMessageW(clientInfo->hwndChildMaximized, WM_SYSCOMMAND, SC_RESTORE, 0);
            else
                cmd = SW_SHOWMAXIMIZED;

            clientInfo->hwndActiveChild = child;
        }

        ShowWindow( clientInfo->hwndActiveChild, cmd );
    }

    clientInfo->hwndActiveChild = child;

    /* check if we have any children left */
    if( !child )
    {
	if( isActiveFrameWnd )
	    SetFocus( client );
        return 0;
    }

    /* check menu item */
    if( clientInfo->hWindowMenu )
    {
        UINT id = GetWindowLongA( child, GWL_ID );
        /* The window to be activated must be displayed in the "Windows" menu */
        if (id >= clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT)
        {
            MDI_SwapMenuItems( GetParent(child),
                               id, clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT - 1);
            id = clientInfo->idFirstChild + MDI_MOREWINDOWSLIMIT - 1;
            MDI_MenuModifyItem( GetParent(child), child );
        }

        CheckMenuItem(clientInfo->hWindowMenu, id, MF_CHECKED);
    }
    /* bring active child to the top */
    SetWindowPos( child, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    if( isActiveFrameWnd )
    {
	    SendMessageA( child, WM_NCACTIVATE, TRUE, 0L);
	    if( GetFocus() == client )
		SendMessageA( client, WM_SETFOCUS, (WPARAM)client, 0L );
	    else
		SetFocus( client );
    }
    SendMessageA( child, WM_MDIACTIVATE, (WPARAM)prevActiveWnd, (LPARAM)child );
    return TRUE;
}

/* -------------------- MDI client window functions ------------------- */

/**********************************************************************
 *				CreateMDIMenuBitmap
 */
static HBITMAP CreateMDIMenuBitmap(void)
{
 HDC 		hDCSrc  = CreateCompatibleDC(0);
 HDC		hDCDest	= CreateCompatibleDC(hDCSrc);
 HBITMAP	hbClose = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE) );
 HBITMAP	hbCopy;
 HBITMAP	hobjSrc, hobjDest;

 hobjSrc = SelectObject(hDCSrc, hbClose);
 hbCopy = CreateCompatibleBitmap(hDCSrc,GetSystemMetrics(SM_CXSIZE),GetSystemMetrics(SM_CYSIZE));
 hobjDest = SelectObject(hDCDest, hbCopy);

 BitBlt(hDCDest, 0, 0, GetSystemMetrics(SM_CXSIZE), GetSystemMetrics(SM_CYSIZE),
          hDCSrc, GetSystemMetrics(SM_CXSIZE), 0, SRCCOPY);

 SelectObject(hDCSrc, hobjSrc);
 DeleteObject(hbClose);
 DeleteDC(hDCSrc);

 hobjSrc = SelectObject( hDCDest, GetStockObject(BLACK_PEN) );

 MoveToEx( hDCDest, GetSystemMetrics(SM_CXSIZE) - 1, 0, NULL );
 LineTo( hDCDest, GetSystemMetrics(SM_CXSIZE) - 1, GetSystemMetrics(SM_CYSIZE) - 1);

 SelectObject(hDCDest, hobjSrc );
 SelectObject(hDCDest, hobjDest);
 DeleteDC(hDCDest);

 return hbCopy;
}

/**********************************************************************
 *				MDICascade
 */
static LONG MDICascade( HWND client, MDICLIENTINFO *ci )
{
#ifdef __REACTOS__
    /* FIXME */
    return 0;
#else
    HWND *win_array;
    BOOL has_icons = FALSE;
    int i, total;

    if (ci->hwndChildMaximized)
        SendMessageA( client, WM_MDIRESTORE,
                        (WPARAM)ci->hwndChildMaximized, 0);

    if (ci->nActiveChildren == 0) return 0;

    if (!(win_array = WIN_ListChildren( client ))) return 0;

    /* remove all the windows we don't want */
    for (i = total = 0; win_array[i]; i++)
    {
        if (!IsWindowVisible( win_array[i] )) continue;
        if (GetWindow( win_array[i], GW_OWNER )) continue; /* skip owned windows */
        if (IsIconic( win_array[i] ))
        {
            has_icons = TRUE;
            continue;
        }
        win_array[total++] = win_array[i];
    }
    win_array[total] = 0;

    if (total)
    {
        INT delta = 0, n = 0, i;
        POINT pos[2];
        if (has_icons) delta = GetSystemMetrics(SM_CYICONSPACING) + GetSystemMetrics(SM_CYICON);

        /* walk the list (backwards) and move windows */
        for (i = total - 1; i >= 0; i--)
        {
            TRACE("move %p to (%ld,%ld) size [%ld,%ld]\n",
                  win_array[i], pos[0].x, pos[0].y, pos[1].x, pos[1].y);

            MDI_CalcDefaultChildPos(client, n++, pos, delta);
            SetWindowPos( win_array[i], 0, pos[0].x, pos[0].y, pos[1].x, pos[1].y,
                          SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }
    HeapFree( GetProcessHeap(), 0, win_array );

    if (has_icons) ArrangeIconicWindows( client );
    return 0;
#endif
}

/**********************************************************************
 *					MDITile
 */
static void MDITile( HWND client, MDICLIENTINFO *ci, WPARAM wParam )
{
#ifdef __REACTOS__
    /* FIXME */
    return;
#else
    HWND *win_array;
    int i, total;
    BOOL has_icons = FALSE;

    if (ci->hwndChildMaximized)
        SendMessageA( client, WM_MDIRESTORE, (WPARAM)ci->hwndChildMaximized, 0);

    if (ci->nActiveChildren == 0) return;

    if (!(win_array = WIN_ListChildren( client ))) return;

    /* remove all the windows we don't want */
    for (i = total = 0; win_array[i]; i++)
    {
        if (!IsWindowVisible( win_array[i] )) continue;
        if (GetWindow( win_array[i], GW_OWNER )) continue; /* skip owned windows (icon titles) */
        if (IsIconic( win_array[i] ))
        {
            has_icons = TRUE;
            continue;
        }
        if ((wParam & MDITILE_SKIPDISABLED) && !IsWindowEnabled( win_array[i] )) continue;
        win_array[total++] = win_array[i];
    }
    win_array[total] = 0;

    TRACE("%u windows to tile\n", total);

    if (total)
    {
        HWND *pWnd = win_array;
        RECT rect;
        int x, y, xsize, ysize;
        int rows, columns, r, c, i;

        GetClientRect(client,&rect);
        rows    = (int) sqrt((double)total);
        columns = total / rows;

        if( wParam & MDITILE_HORIZONTAL )  /* version >= 3.1 */
        {
            i = rows;
            rows = columns;  /* exchange r and c */
            columns = i;
        }

        if (has_icons)
        {
            y = rect.bottom - 2 * GetSystemMetrics(SM_CYICONSPACING) - GetSystemMetrics(SM_CYICON);
            rect.bottom = ( y - GetSystemMetrics(SM_CYICON) < rect.top )? rect.bottom: y;
        }

        ysize   = rect.bottom / rows;
        xsize   = rect.right  / columns;

        for (x = i = 0, c = 1; c <= columns && *pWnd; c++)
        {
            if (c == columns)
            {
                rows  = total - i;
                ysize = rect.bottom / rows;
            }

            y = 0;
            for (r = 1; r <= rows && *pWnd; r++, i++)
            {
                SetWindowPos(*pWnd, 0, x, y, xsize, ysize,
                             SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
                y += ysize;
                pWnd++;
            }
            x += xsize;
        }
    }
    HeapFree( GetProcessHeap(), 0, win_array );
    if (has_icons) ArrangeIconicWindows( client );
#endif
}

/* ----------------------- Frame window ---------------------------- */


/**********************************************************************
 *					MDI_AugmentFrameMenu
 */
static BOOL MDI_AugmentFrameMenu( HWND frame, HWND hChild )
{
    HMENU menu = GetMenu( frame );
#ifndef __REACTOS__
    WND*	child = WIN_FindWndPtr(hChild);
#endif
    HMENU  	hSysPopup = 0;
    HBITMAP hSysMenuBitmap = 0;

    TRACE("frame %p,child %p\n",frame,hChild);

#ifndef __REACTOS__
    if( !menu || !child->hSysMenu )
    {
        WIN_ReleaseWndPtr(child);
        return 0;
    }
    WIN_ReleaseWndPtr(child);
#else
    if( !menu || !GetSystemMenu(hChild, FALSE) )
    {
        return 0;
    }
#endif

    /* create a copy of sysmenu popup and insert it into frame menu bar */

    if (!(hSysPopup = LoadMenuA(GetModuleHandleA("USER32"), "SYSMENU")))
	return 0;

    AppendMenuA(menu,MF_HELP | MF_BITMAP,
                   SC_MINIMIZE, (LPSTR)(DWORD)HBMMENU_MBAR_MINIMIZE ) ;
    AppendMenuA(menu,MF_HELP | MF_BITMAP,
                   SC_RESTORE, (LPSTR)(DWORD)HBMMENU_MBAR_RESTORE );

  /* In Win 95 look, the system menu is replaced by the child icon */
#ifndef __REACTOS__
  if(TWEAK_WineLook > WIN31_LOOK)
#endif
  {
    HICON hIcon = (HICON)GetClassLongA(hChild, GCL_HICONSM);
    if (!hIcon)
      hIcon = (HICON)GetClassLongA(hChild, GCL_HICON);
    if (hIcon)
    {
      HDC hMemDC;
      HBITMAP hBitmap, hOldBitmap;
      HBRUSH hBrush;
      HDC hdc = GetDC(hChild);

      if (hdc)
      {
        int cx, cy;
        cx = GetSystemMetrics(SM_CXSMICON);
        cy = GetSystemMetrics(SM_CYSMICON);
        hMemDC = CreateCompatibleDC(hdc);
        hBitmap = CreateCompatibleBitmap(hdc, cx, cy);
        hOldBitmap = SelectObject(hMemDC, hBitmap);
        SetMapMode(hMemDC, MM_TEXT);
        hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
        DrawIconEx(hMemDC, 0, 0, hIcon, cx, cy, 0, hBrush, DI_NORMAL);
        SelectObject (hMemDC, hOldBitmap);
        DeleteObject(hBrush);
        DeleteDC(hMemDC);
        ReleaseDC(hChild, hdc);
        hSysMenuBitmap = hBitmap;
      }
    }
  }
#ifndef __REACTOS__
  else
    hSysMenuBitmap = hBmpClose;
#endif

    if( !InsertMenuA(menu,0,MF_BYPOSITION | MF_BITMAP | MF_POPUP,
                     (UINT_PTR)hSysPopup, (LPSTR)hSysMenuBitmap))
    {
        TRACE("not inserted\n");
	DestroyMenu(hSysPopup);
	return 0;
    }

    /* The close button is only present in Win 95 look */
#ifndef __REACTOS__
    if(TWEAK_WineLook > WIN31_LOOK)
#endif
    {
        AppendMenuA(menu,MF_HELP | MF_BITMAP,
                       SC_CLOSE, (LPSTR)(DWORD)HBMMENU_MBAR_CLOSE );
    }

    EnableMenuItem(hSysPopup, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hSysPopup, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hSysPopup, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
    SetMenuDefaultItem(hSysPopup, SC_CLOSE, FALSE);

    /* redraw menu */
    DrawMenuBar(frame);

    return 1;
}

/**********************************************************************
 *					MDI_RestoreFrameMenu
 */
static BOOL MDI_RestoreFrameMenu( HWND frame, HWND hChild )
{
    MENUITEMINFOW menuInfo;
    HMENU menu = GetMenu( frame );
    INT nItems = GetMenuItemCount(menu) - 1;
    UINT iId = GetMenuItemID(menu,nItems) ;

    TRACE("frame %p,child %p,nIt=%d,iId=%d\n",frame,hChild,nItems,iId);

    if(!(iId == SC_RESTORE || iId == SC_CLOSE) )
	return 0;

    /*
     * Remove the system menu, If that menu is the icon of the window
     * as it is in win95, we have to delete the bitmap.
     */
    memset(&menuInfo, 0, sizeof(menuInfo));
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask  = MIIM_DATA | MIIM_TYPE;

    GetMenuItemInfoW(menu,
		     0,
		     TRUE,
		     &menuInfo);

    RemoveMenu(menu,0,MF_BYPOSITION);

#ifndef __REACTOS__
    if ( (menuInfo.fType & MFT_BITMAP)           &&
	 (LOWORD(menuInfo.dwTypeData)!=0)        &&
	 (LOWORD(menuInfo.dwTypeData)!=HBITMAP_16(hBmpClose)) )
    {
        DeleteObject(HBITMAP_32(LOWORD(menuInfo.dwTypeData)));
    }
#else
    if ( (menuInfo.fType & MFT_BITMAP)           &&
	 (LOWORD(menuInfo.dwTypeData)!=0))
    {
        DeleteObject((HBITMAP)(LOWORD(menuInfo.dwTypeData) & 0x50000));
    }
#endif

#ifndef __REACTOS__
    if(TWEAK_WineLook > WIN31_LOOK)
#endif
    {
        /* close */
        DeleteMenu(menu,GetMenuItemCount(menu) - 1,MF_BYPOSITION);
    }
    /* restore */
    DeleteMenu(menu,GetMenuItemCount(menu) - 1,MF_BYPOSITION);
    /* minimize */
    DeleteMenu(menu,GetMenuItemCount(menu) - 1,MF_BYPOSITION);

    DrawMenuBar(frame);

    return 1;
}


/**********************************************************************
 *				        MDI_UpdateFrameText
 *
 * used when child window is maximized/restored
 *
 * Note: lpTitle can be NULL
 */
static void MDI_UpdateFrameText( HWND frame, HWND hClient,
                                 BOOL repaint, LPCWSTR lpTitle )
{
    WCHAR   lpBuffer[MDI_MAXTITLELENGTH+1];
    MDICLIENTINFO *ci = get_client_info( hClient );

    TRACE("repaint %i, frameText %s\n", repaint, debugstr_w(lpTitle));

    if (!ci) return;

    if (!lpTitle && !ci->frameTitle)  /* first time around, get title from the frame window */
    {
        GetWindowTextW( frame, lpBuffer, sizeof(lpBuffer)/sizeof(WCHAR) );
        lpTitle = lpBuffer;
    }

    /* store new "default" title if lpTitle is not NULL */
    if (lpTitle)
    {
	if (ci->frameTitle) HeapFree( GetProcessHeap(), 0, ci->frameTitle );
	if ((ci->frameTitle = HeapAlloc( GetProcessHeap(), 0, (strlenW(lpTitle)+1)*sizeof(WCHAR))))
            strcpyW( ci->frameTitle, lpTitle );
    }

    if (ci->frameTitle)
    {
	if (ci->hwndChildMaximized)
	{
	    /* combine frame title and child title if possible */

	    static const WCHAR lpBracket[]  = {' ','-',' ','[',0};
	    static const WCHAR lpBracket2[]  = {']',0};
	    int	i_frame_text_length = strlenW(ci->frameTitle);

	    lstrcpynW( lpBuffer, ci->frameTitle, MDI_MAXTITLELENGTH);

	    if( i_frame_text_length + 6 < MDI_MAXTITLELENGTH )
            {
		strcatW( lpBuffer, lpBracket );
                if (GetWindowTextW( ci->hwndChildMaximized, lpBuffer + i_frame_text_length + 4,
                                    MDI_MAXTITLELENGTH - i_frame_text_length - 5 ))
                    strcatW( lpBuffer, lpBracket2 );
                else
                    lpBuffer[i_frame_text_length] = 0;  /* remove bracket */
            }
	}
	else
	{
            lstrcpynW(lpBuffer, ci->frameTitle, MDI_MAXTITLELENGTH+1 );
	}
    }
    else
	lpBuffer[0] = '\0';

    DefWindowProcW( frame, WM_SETTEXT, 0, (LPARAM)lpBuffer );
    if( repaint == MDI_REPAINTFRAME)
        SetWindowPos( frame, 0,0,0,0,0, SWP_FRAMECHANGED |
                      SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
}


/* ----------------------------- Interface ---------------------------- */


/**********************************************************************
 *		MDIClientWndProc_common
 */
static LRESULT MDIClientWndProc_common( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    MDICLIENTINFO *ci = NULL;

    if (WM_NCCREATE != message && WM_CREATE != message
        && NULL == (ci = get_client_info(hwnd)))
    {
        return 0;
    }

    switch (message)
    {
#ifdef __REACTOS__
      case WM_NCCREATE:
	if (!(ci = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ci))))
		return FALSE;
        SetWindowLongPtrW( hwnd, 0, (LONG_PTR)ci );
	return TRUE;
#endif
      
      case WM_CREATE:
      {
          RECT rect;
          /* Since we are using only cs->lpCreateParams, we can safely
           * cast to LPCREATESTRUCTA here */
          LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
#ifndef __REACTOS__
          WND *wndPtr = WIN_GetPtr( hwnd );
#endif
	ci = HeapAlloc(GetProcessHeap(), 0, sizeof(MDICLIENTINFO));
	if (NULL == ci)
	{
	    return -1;
	}
	SetWindowLongPtr(hwnd, 0, (LONG_PTR) ci);

	/* Translation layer doesn't know what's in the cs->lpCreateParams
	 * so we have to keep track of what environment we're in. */

#ifndef __REACTOS__
	if( wndPtr->flags & WIN_ISWIN32 )
#endif
	{
#define ccs ((LPCLIENTCREATESTRUCT)cs->lpCreateParams)
	    ci->hWindowMenu	= ccs->hWindowMenu;
	    ci->idFirstChild	= ccs->idFirstChild;
#undef ccs
	}
#ifndef __REACTOS__
        else
	{
	    LPCLIENTCREATESTRUCT16 ccs = MapSL((SEGPTR)cs->lpCreateParams);
	    ci->hWindowMenu	= HMENU_32(ccs->hWindowMenu);
	    ci->idFirstChild	= ccs->idFirstChild;
	}
        WIN_ReleasePtr( wndPtr );
#endif

	ci->hwndChildMaximized  = 0;
	ci->nActiveChildren	= 0;
	ci->nTotalCreated	= 0;
	ci->frameTitle		= NULL;
	ci->mdiFlags		= 0;
        SetWindowLongW( hwnd, GWL_STYLE, GetWindowLongW(hwnd,GWL_STYLE) | WS_CLIPCHILDREN );

	if (!hBmpClose) hBmpClose = CreateMDIMenuBitmap();

	if (ci->hWindowMenu != 0)
	    AppendMenuW( ci->hWindowMenu, MF_SEPARATOR, 0, NULL );

	GetClientRect( GetParent(hwnd), &rect);
        MoveWindow( hwnd, 0, 0, rect.right, rect.bottom, FALSE );

        MDI_UpdateFrameText( GetParent(hwnd), hwnd, MDI_NOFRAMEREPAINT, NULL);

        TRACE("Client created - hwnd = %p, idFirst = %u\n", hwnd, ci->idFirstChild );
        return 0;
      }

      case WM_DESTROY:
      {
          INT nItems;
          if( ci->hwndChildMaximized )
              MDI_RestoreFrameMenu( GetParent(hwnd), ci->hwndChildMaximized);
          if((ci->hWindowMenu != 0) &&
             (nItems = GetMenuItemCount(ci->hWindowMenu)) > 0)
          {
              ci->idFirstChild = nItems - 1;
              ci->nActiveChildren++;  /* to delete a separator */
              while( ci->nActiveChildren-- )
                  DeleteMenu(ci->hWindowMenu,MF_BYPOSITION,ci->idFirstChild--);
          }
          if (ci->frameTitle) HeapFree( GetProcessHeap(), 0, ci->frameTitle );
#ifdef __REACTOS__
          HeapFree( GetProcessHeap(), 0, ci );
          SetWindowLongPtrW( hwnd, 0, 0 );
#endif
          return 0;
      }

      case WM_MDIACTIVATE:
        if( ci->hwndActiveChild != (HWND)wParam )
	    SetWindowPos((HWND)wParam, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE);
        return 0;

      case WM_MDICASCADE:
        return MDICascade(hwnd, ci);

      case WM_MDICREATE:
        if (lParam)
            return (LRESULT)MDICreateChild( hwnd, ci, (MDICREATESTRUCTA *)lParam, unicode );
        return 0;

      case WM_MDIDESTROY:
#ifndef __REACTOS__
          return MDIDestroyChild( hwnd, ci, WIN_GetFullHandle( (HWND)wParam ), TRUE );
#else
          return MDIDestroyChild( hwnd, ci, (HWND)wParam, TRUE );
#endif

      case WM_MDIGETACTIVE:
          if (lParam) *(BOOL *)lParam = (ci->hwndChildMaximized != 0);
          return (LRESULT)ci->hwndActiveChild;

      case WM_MDIICONARRANGE:
	ci->mdiFlags |= MDIF_NEEDUPDATE;
        ArrangeIconicWindows( hwnd );
	ci->sbRecalc = SB_BOTH+1;
        SendMessageW( hwnd, WM_MDICALCCHILDSCROLL, 0, 0 );
        return 0;

      case WM_MDIMAXIMIZE:
	ShowWindow( (HWND)wParam, SW_MAXIMIZE );
        return 0;

      case WM_MDINEXT: /* lParam != 0 means previous window */
#ifndef __REACTOS__
	MDI_SwitchActiveChild( hwnd, WIN_GetFullHandle( (HWND)wParam ), !lParam );
#else
	MDI_SwitchActiveChild( hwnd, (HWND)wParam, !lParam );
#endif
	break;

      case WM_MDIRESTORE:
        SendMessageW( (HWND)wParam, WM_SYSCOMMAND, SC_RESTORE, 0);
        return 0;

      case WM_MDISETMENU:
          return MDISetMenu( hwnd, (HMENU)wParam, (HMENU)lParam );

      case WM_MDIREFRESHMENU:
          return MDIRefreshMenu( hwnd, (HMENU)wParam, (HMENU)lParam );

      case WM_MDITILE:
	ci->mdiFlags |= MDIF_NEEDUPDATE;
        ShowScrollBar( hwnd, SB_BOTH, FALSE );
        MDITile( hwnd, ci, wParam );
        ci->mdiFlags &= ~MDIF_NEEDUPDATE;
        return 0;

      case WM_VSCROLL:
      case WM_HSCROLL:
	ci->mdiFlags |= MDIF_NEEDUPDATE;
        ScrollChildren( hwnd, message, wParam, lParam );
	ci->mdiFlags &= ~MDIF_NEEDUPDATE;
        return 0;

      case WM_SETFOCUS:
          if (ci->hwndActiveChild && !IsIconic( ci->hwndActiveChild ))
              SetFocus( ci->hwndActiveChild );
          return 0;

      case WM_NCACTIVATE:
        if( ci->hwndActiveChild )
            SendMessageW(ci->hwndActiveChild, message, wParam, lParam);
	break;

      case WM_PARENTNOTIFY:
        if (LOWORD(wParam) == WM_LBUTTONDOWN)
        {
            HWND child;
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            child = ChildWindowFromPoint(hwnd, pt);

	    TRACE("notification from %p (%li,%li)\n",child,pt.x,pt.y);

            if( child && child != hwnd && child != ci->hwndActiveChild )
                SetWindowPos(child, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
        }
        return 0;

      case WM_SIZE:
        if( IsWindow(ci->hwndChildMaximized) )
	{
	    RECT	rect;

	    rect.left = 0;
	    rect.top = 0;
	    rect.right = LOWORD(lParam);
	    rect.bottom = HIWORD(lParam);

	    AdjustWindowRectEx(&rect, GetWindowLongA(ci->hwndChildMaximized,GWL_STYLE),
                               0, GetWindowLongA(ci->hwndChildMaximized,GWL_EXSTYLE) );
	    MoveWindow(ci->hwndChildMaximized, rect.left, rect.top,
			 rect.right - rect.left, rect.bottom - rect.top, 1);
	}
	else
            MDI_PostUpdate(hwnd, ci, SB_BOTH+1);

	break;

      case WM_MDICALCCHILDSCROLL:
	if( (ci->mdiFlags & MDIF_NEEDUPDATE) && ci->sbRecalc )
	{
            CalcChildScroll(hwnd, ci->sbRecalc-1);
	    ci->sbRecalc = 0;
	    ci->mdiFlags &= ~MDIF_NEEDUPDATE;
	}
        return 0;
    }
    return unicode ? DefWindowProcW( hwnd, message, wParam, lParam ) :
                     DefWindowProcA( hwnd, message, wParam, lParam );
}

/***********************************************************************
 *		MDIClientWndProcA
 */
static LRESULT WINAPI MDIClientWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow(hwnd)) return 0;
    return MDIClientWndProc_common( hwnd, message, wParam, lParam, FALSE );
}

/***********************************************************************
 *		MDIClientWndProcW
 */
static LRESULT WINAPI MDIClientWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow(hwnd)) return 0;
    return MDIClientWndProc_common( hwnd, message, wParam, lParam, TRUE );
}

#ifndef __REACTOS__
/***********************************************************************
 *		DefFrameProc (USER.445)
 */
LRESULT WINAPI DefFrameProc16( HWND16 hwnd, HWND16 hwndMDIClient,
                               UINT16 message, WPARAM16 wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_SETTEXT:
        lParam = (LPARAM)MapSL(lParam);
        /* fall through */
    case WM_COMMAND:
    case WM_NCACTIVATE:
    case WM_SETFOCUS:
    case WM_SIZE:
        return DefFrameProcA( WIN_Handle32(hwnd), WIN_Handle32(hwndMDIClient),
                              message, wParam, lParam );

    case WM_NEXTMENU:
        {
            MDINEXTMENU next_menu;
            DefFrameProcW( WIN_Handle32(hwnd), WIN_Handle32(hwndMDIClient),
                           message, wParam, (LPARAM)&next_menu );
            return MAKELONG( HMENU_16(next_menu.hmenuNext), HWND_16(next_menu.hwndNext) );
        }
    default:
        return DefWindowProc16(hwnd, message, wParam, lParam);
    }
}
#endif

/***********************************************************************
 *		DefFrameProcA (USER32.@)
 */
LRESULT WINAPI DefFrameProcA( HWND hwnd, HWND hwndMDIClient,
                                UINT message, WPARAM wParam, LPARAM lParam)
{
    if (hwndMDIClient)
    {
	switch (message)
	{
        case WM_SETTEXT:
            {
                DWORD len = MultiByteToWideChar( CP_ACP, 0, (LPSTR)lParam, -1, NULL, 0 );
                LPWSTR text = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
                MultiByteToWideChar( CP_ACP, 0, (LPSTR)lParam, -1, text, len );
                MDI_UpdateFrameText(hwnd, hwndMDIClient, MDI_REPAINTFRAME, text );
                HeapFree( GetProcessHeap(), 0, text );
            }
            return 1; /* success. FIXME: check text length */

        case WM_COMMAND:
        case WM_NCACTIVATE:
        case WM_NEXTMENU:
        case WM_SETFOCUS:
        case WM_SIZE:
            return DefFrameProcW( hwnd, hwndMDIClient, message, wParam, lParam );
        }
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *		DefFrameProcW (USER32.@)
 */
LRESULT WINAPI DefFrameProcW( HWND hwnd, HWND hwndMDIClient,
                                UINT message, WPARAM wParam, LPARAM lParam)
{
    MDICLIENTINFO *ci = get_client_info( hwndMDIClient );

    if (ci)
    {
	switch (message)
	{
        case WM_COMMAND:
            {
                WORD id = LOWORD(wParam);
                /* check for possible syscommands for maximized MDI child */
                if (id <  ci->idFirstChild || id >= ci->idFirstChild + ci->nActiveChildren)
                {
                    if( (id - 0xf000) & 0xf00f ) break;
                    if( !ci->hwndChildMaximized ) break;
                    switch( id )
                    {
                    case SC_SIZE:
                    case SC_MOVE:
                    case SC_MINIMIZE:
                    case SC_MAXIMIZE:
                    case SC_NEXTWINDOW:
                    case SC_PREVWINDOW:
                    case SC_CLOSE:
                    case SC_RESTORE:
                        return SendMessageW( ci->hwndChildMaximized, WM_SYSCOMMAND,
                                             wParam, lParam);
                    }
                }
                else
                {
                    HWND childHwnd;
                    if (id - ci->idFirstChild == MDI_MOREWINDOWSLIMIT)
                        /* User chose "More Windows..." */
                        childHwnd = MDI_MoreWindowsDialog(hwndMDIClient);
                    else
                        /* User chose one of the windows listed in the "Windows" menu */
                        childHwnd = MDI_GetChildByID(hwndMDIClient,id);

                    if( childHwnd )
                        SendMessageW( hwndMDIClient, WM_MDIACTIVATE, (WPARAM)childHwnd, 0 );
                }
            }
            break;

        case WM_NCACTIVATE:
	    SendMessageW(hwndMDIClient, message, wParam, lParam);
	    break;

        case WM_SETTEXT:
            MDI_UpdateFrameText(hwnd, hwndMDIClient, MDI_REPAINTFRAME, (LPWSTR)lParam );
	    return 1; /* success. FIXME: check text length */

        case WM_SETFOCUS:
	    SetFocus(hwndMDIClient);
	    break;

        case WM_SIZE:
            MoveWindow(hwndMDIClient, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            break;

        case WM_NEXTMENU:
            {
/* FIXME */
#ifndef __REACTOS__
                MDINEXTMENU *next_menu = (MDINEXTMENU *)lParam;

                if (!IsIconic(hwnd) && ci->hwndActiveChild && !ci->hwndChildMaximized)
                {
                    /* control menu is between the frame system menu and
                     * the first entry of menu bar */
                    WND *wndPtr = WIN_GetPtr(hwnd);

                    if( (wParam == VK_LEFT && GetMenu(hwnd) == next_menu->hmenuIn) ||
                        (wParam == VK_RIGHT && GetSubMenu(wndPtr->hSysMenu, 0) == next_menu->hmenuIn) )
                    {
                        WIN_ReleasePtr(wndPtr);
                        wndPtr = WIN_GetPtr(ci->hwndActiveChild);
                        next_menu->hmenuNext = GetSubMenu(wndPtr->hSysMenu, 0);
                        next_menu->hwndNext = ci->hwndActiveChild;
                    }
                    WIN_ReleasePtr(wndPtr);
                }
#endif
                return 0;
            }
	}
    }

    return DefWindowProcW( hwnd, message, wParam, lParam );
}


#ifndef __REACTOS__
/***********************************************************************
 *		DefMDIChildProc (USER.447)
 */
LRESULT WINAPI DefMDIChildProc16( HWND16 hwnd, UINT16 message,
                                  WPARAM16 wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_SETTEXT:
        return DefMDIChildProcA( WIN_Handle32(hwnd), message, wParam, (LPARAM)MapSL(lParam) );
    case WM_MENUCHAR:
    case WM_CLOSE:
    case WM_SETFOCUS:
    case WM_CHILDACTIVATE:
    case WM_SYSCOMMAND:
    case WM_SETVISIBLE:
    case WM_SIZE:
    case WM_SYSCHAR:
        return DefMDIChildProcW( WIN_Handle32(hwnd), message, wParam, lParam );
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi16 = (MINMAXINFO16 *)MapSL(lParam);
            MINMAXINFO mmi;
            STRUCT32_MINMAXINFO16to32( mmi16, &mmi );
            DefMDIChildProcW( WIN_Handle32(hwnd), message, wParam, (LPARAM)&mmi );
            STRUCT32_MINMAXINFO32to16( &mmi, mmi16 );
            return 0;
        }
    case WM_NEXTMENU:
        {
            MDINEXTMENU next_menu;
            DefMDIChildProcW( WIN_Handle32(hwnd), message, wParam, (LPARAM)&next_menu );
            return MAKELONG( HMENU_16(next_menu.hmenuNext), HWND_16(next_menu.hwndNext) );
        }
    default:
        return DefWindowProc16(hwnd, message, wParam, lParam);
    }
}
#endif


/***********************************************************************
 *		DefMDIChildProcA (USER32.@)
 */
LRESULT WINAPI DefMDIChildProcA( HWND hwnd, UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
    HWND client = GetParent(hwnd);
    MDICLIENTINFO *ci = get_client_info( client );

#ifndef __REACTOS__
    hwnd = WIN_GetFullHandle( hwnd );
#endif
    if (!ci) return DefWindowProcA( hwnd, message, wParam, lParam );

    switch (message)
    {
    case WM_SETTEXT:
	DefWindowProcA(hwnd, message, wParam, lParam);
        MDI_MenuModifyItem( client, hwnd );
	if( ci->hwndChildMaximized == hwnd )
	    MDI_UpdateFrameText( GetParent(client), client, MDI_REPAINTFRAME, NULL );
        return 1; /* success. FIXME: check text length */

    case WM_GETMINMAXINFO:
    case WM_MENUCHAR:
    case WM_CLOSE:
    case WM_SETFOCUS:
    case WM_CHILDACTIVATE:
    case WM_SYSCOMMAND:
/* FIXME */
#ifndef __REACTOS__
    case WM_SETVISIBLE:
#endif
    case WM_SIZE:
    case WM_NEXTMENU:
    case WM_SYSCHAR:
        return DefMDIChildProcW( hwnd, message, wParam, lParam );
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *		DefMDIChildProcW (USER32.@)
 */
LRESULT WINAPI DefMDIChildProcW( HWND hwnd, UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
    HWND client = GetParent(hwnd);
    MDICLIENTINFO *ci = get_client_info( client );

#ifndef __REACTOS__
    hwnd = WIN_GetFullHandle( hwnd );
#endif
    if (!ci) return DefWindowProcW( hwnd, message, wParam, lParam );

    switch (message)
    {
    case WM_SETTEXT:
        DefWindowProcW(hwnd, message, wParam, lParam);
        MDI_MenuModifyItem( client, hwnd );
        if( ci->hwndChildMaximized == hwnd )
            MDI_UpdateFrameText( GetParent(client), client, MDI_REPAINTFRAME, NULL );
        return 1; /* success. FIXME: check text length */

    case WM_GETMINMAXINFO:
        MDI_ChildGetMinMaxInfo( client, hwnd, (MINMAXINFO *)lParam );
        return 0;

    case WM_MENUCHAR:
        return 0x00010000; /* MDI children don't have menu bars */

    case WM_CLOSE:
        SendMessageW( client, WM_MDIDESTROY, (WPARAM)hwnd, 0 );
        return 0;

    case WM_SETFOCUS:
        if (ci->hwndActiveChild != hwnd) MDI_ChildActivate( client, hwnd );
        break;

    case WM_CHILDACTIVATE:
        MDI_ChildActivate( client, hwnd );
        return 0;

    case WM_SYSCOMMAND:
        switch( wParam )
        {
        case SC_MOVE:
            if( ci->hwndChildMaximized == hwnd) return 0;
            break;
        case SC_RESTORE:
        case SC_MINIMIZE:
            SetWindowLongW( hwnd, GWL_STYLE,
                            GetWindowLongW( hwnd, GWL_STYLE ) | WS_SYSMENU );
            break;
        case SC_MAXIMIZE:
            if (ci->hwndChildMaximized == hwnd)
                return SendMessageW( GetParent(client), message, wParam, lParam);
            SetWindowLongW( hwnd, GWL_STYLE,
                            GetWindowLongW( hwnd, GWL_STYLE ) & ~WS_SYSMENU );
            break;
        case SC_NEXTWINDOW:
            SendMessageW( client, WM_MDINEXT, 0, 0);
            return 0;
        case SC_PREVWINDOW:
            SendMessageW( client, WM_MDINEXT, 0, 1);
            return 0;
        }
        break;

/* FIXME */
#ifndef __REACTOS__
    case WM_SETVISIBLE:
        if( ci->hwndChildMaximized) ci->mdiFlags &= ~MDIF_NEEDUPDATE;
        else MDI_PostUpdate(client, ci, SB_BOTH+1);
        break;
#endif

    case WM_SIZE:
        if( ci->hwndActiveChild == hwnd && wParam != SIZE_MAXIMIZED )
        {
            ci->hwndChildMaximized = 0;
            MDI_RestoreFrameMenu( GetParent(client), hwnd );
            MDI_UpdateFrameText( GetParent(client), client, MDI_REPAINTFRAME, NULL );
        }

        if( wParam == SIZE_MAXIMIZED )
        {
            HWND hMaxChild = ci->hwndChildMaximized;

            if( hMaxChild == hwnd ) break;
            if( hMaxChild)
            {
                SendMessageW( hMaxChild, WM_SETREDRAW, FALSE, 0 );
                MDI_RestoreFrameMenu( GetParent(client), hMaxChild );
                ShowWindow( hMaxChild, SW_SHOWNOACTIVATE );
                SendMessageW( hMaxChild, WM_SETREDRAW, TRUE, 0 );
            }
            TRACE("maximizing child %p\n", hwnd );

            /* keep track of the maximized window. */
            ci->hwndChildMaximized = hwnd; /* !!! */

            /* The maximized window should also be the active window */
            MDI_ChildActivate( client, hwnd );
            MDI_AugmentFrameMenu( GetParent(client), hwnd );
            MDI_UpdateFrameText( GetParent(client), client, MDI_REPAINTFRAME, NULL );
        }

        if( wParam == SIZE_MINIMIZED )
        {
            HWND switchTo = MDI_GetWindow(ci, hwnd, TRUE, WS_MINIMIZE);

            if (switchTo) SendMessageW( switchTo, WM_CHILDACTIVATE, 0, 0);
        }
        MDI_PostUpdate(client, ci, SB_BOTH+1);
        break;

    case WM_NEXTMENU:
        {
/* FIXME */
#ifndef __REACTOS__
            MDINEXTMENU *next_menu = (MDINEXTMENU *)lParam;
            HWND parent = GetParent(client);

            if( wParam == VK_LEFT )  /* switch to frame system menu */
            {
                WND *wndPtr = WIN_GetPtr( parent );
                next_menu->hmenuNext = GetSubMenu( wndPtr->hSysMenu, 0 );
                WIN_ReleasePtr( wndPtr );
            }
            if( wParam == VK_RIGHT )  /* to frame menu bar */
            {
                next_menu->hmenuNext = GetMenu(parent);
            }
            next_menu->hwndNext = parent;
#endif
            return 0;
        }

    case WM_SYSCHAR:
        if (wParam == '-')
        {
            SendMessageW( hwnd, WM_SYSCOMMAND, (WPARAM)SC_KEYMENU, (DWORD)VK_SPACE);
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

/**********************************************************************
 *		CreateMDIWindowA (USER32.@) Creates a MDI child
 *
 * RETURNS
 *    Success: Handle to created window
 *    Failure: NULL
 */
HWND WINAPI CreateMDIWindowA(
    LPCSTR lpClassName,    /* [in] Pointer to registered child class name */
    LPCSTR lpWindowName,   /* [in] Pointer to window name */
    DWORD dwStyle,         /* [in] Window style */
    INT X,               /* [in] Horizontal position of window */
    INT Y,               /* [in] Vertical position of window */
    INT nWidth,          /* [in] Width of window */
    INT nHeight,         /* [in] Height of window */
    HWND hWndParent,     /* [in] Handle to parent window */
    HINSTANCE hInstance, /* [in] Handle to application instance */
    LPARAM lParam)         /* [in] Application-defined value */
{
    MDICLIENTINFO *pCi = get_client_info( hWndParent );
    MDICREATESTRUCTA cs;

    TRACE("(%s,%s,%ld,%d,%d,%d,%d,%p,%p,%ld)\n",
          debugstr_a(lpClassName),debugstr_a(lpWindowName),dwStyle,X,Y,
          nWidth,nHeight,hWndParent,hInstance,lParam);

    if (!pCi)
    {
        ERR("bad hwnd for MDI-client: %p\n", hWndParent);
        return 0;
    }
    cs.szClass=lpClassName;
    cs.szTitle=lpWindowName;
    cs.hOwner=hInstance;
    cs.x=X;
    cs.y=Y;
    cs.cx=nWidth;
    cs.cy=nHeight;
    cs.style=dwStyle;
    cs.lParam=lParam;

    return MDICreateChild(hWndParent, pCi, &cs, FALSE);
}

/***********************************************************************
 *		CreateMDIWindowW (USER32.@) Creates a MDI child
 *
 * RETURNS
 *    Success: Handle to created window
 *    Failure: NULL
 */
HWND WINAPI CreateMDIWindowW(
    LPCWSTR lpClassName,    /* [in] Pointer to registered child class name */
    LPCWSTR lpWindowName,   /* [in] Pointer to window name */
    DWORD dwStyle,         /* [in] Window style */
    INT X,               /* [in] Horizontal position of window */
    INT Y,               /* [in] Vertical position of window */
    INT nWidth,          /* [in] Width of window */
    INT nHeight,         /* [in] Height of window */
    HWND hWndParent,     /* [in] Handle to parent window */
    HINSTANCE hInstance, /* [in] Handle to application instance */
    LPARAM lParam)         /* [in] Application-defined value */
{
    MDICLIENTINFO *pCi = get_client_info( hWndParent );
    MDICREATESTRUCTW cs;

    TRACE("(%s,%s,%ld,%d,%d,%d,%d,%p,%p,%ld)\n",
          debugstr_w(lpClassName), debugstr_w(lpWindowName), dwStyle, X, Y,
          nWidth, nHeight, hWndParent, hInstance, lParam);

    if (!pCi)
    {
        ERR("bad hwnd for MDI-client: %p\n", hWndParent);
        return 0;
    }
    cs.szClass = lpClassName;
    cs.szTitle = lpWindowName;
    cs.hOwner = hInstance;
    cs.x = X;
    cs.y = Y;
    cs.cx = nWidth;
    cs.cy = nHeight;
    cs.style = dwStyle;
    cs.lParam = lParam;

    return MDICreateChild(hWndParent, pCi, (MDICREATESTRUCTA *)&cs, TRUE);
}

/**********************************************************************
 *		TranslateMDISysAccel (USER32.@)
 */
BOOL WINAPI TranslateMDISysAccel( HWND hwndClient, LPMSG msg )
{
    if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)
    {
        MDICLIENTINFO *ci = get_client_info( hwndClient );
        WPARAM wParam = 0;

        if (!ci || !IsWindowEnabled(ci->hwndActiveChild)) return 0;

        /* translate if the Ctrl key is down and Alt not. */

        if( (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
        {
            switch( msg->wParam )
            {
            case VK_F6:
            case VK_TAB:
                wParam = ( GetKeyState(VK_SHIFT) & 0x8000 ) ? SC_NEXTWINDOW : SC_PREVWINDOW;
                break;
            case VK_F4:
            case VK_RBUTTON:
                wParam = SC_CLOSE;
                break;
            default:
                return 0;
            }
            TRACE("wParam = %04x\n", wParam);
            SendMessageW(ci->hwndActiveChild, WM_SYSCOMMAND, wParam, (LPARAM)msg->wParam);
            return 1;
        }
    }
    return 0; /* failure */
}

/***********************************************************************
 *		CalcChildScroll (USER32.@)
 */
void WINAPI CalcChildScroll( HWND hwnd, INT scroll )
{
    SCROLLINFO info;
    RECT childRect, clientRect;
#ifndef __REACTOS__
    HWND *list;
#else
    HWND hWndCurrent;
#endif

    GetClientRect( hwnd, &clientRect );
    SetRectEmpty( &childRect );

#ifndef __REACTOS__
    if ((list = WIN_ListChildren( hwnd )))
    {
        int i;
        for (i = 0; list[i]; i++)
        {
            DWORD style = GetWindowLongW( list[i], GWL_STYLE );
            if (style & WS_MAXIMIZE)
            {
                HeapFree( GetProcessHeap(), 0, list );
                ShowScrollBar( hwnd, SB_BOTH, FALSE );
                return;
            }
            if (style & WS_VISIBLE)
            {
                WND *pWnd = WIN_FindWndPtr( list[i] );
                UnionRect( &childRect, &pWnd->rectWindow, &childRect );
                WIN_ReleaseWndPtr( pWnd );
            }
        }
        HeapFree( GetProcessHeap(), 0, list );
    }
#else
    hWndCurrent = GetWindow(hwnd, GW_CHILD);
    while (hWndCurrent != NULL)
    {
        DWORD style = GetWindowLongW( hWndCurrent, GWL_STYLE );
        if (style & WS_MAXIMIZE)
        {
            ShowScrollBar( hwnd, SB_BOTH, FALSE );
            return;
        }
        if (style & WS_VISIBLE)
        {
            RECT WindowRect;

            GetWindowRect( hWndCurrent, &WindowRect );
            UnionRect( &childRect, &WindowRect, &childRect );
        }
        hWndCurrent = GetWindow(hWndCurrent, GW_HWNDNEXT);
    }
#endif
    UnionRect( &childRect, &clientRect, &childRect );

    /* set common info values */
    info.cbSize = sizeof(info);
    info.fMask = SIF_POS | SIF_RANGE;

    /* set the specific */
    switch( scroll )
    {
	case SB_BOTH:
	case SB_HORZ:
			info.nMin = childRect.left;
			info.nMax = childRect.right - clientRect.right;
			info.nPos = clientRect.left - childRect.left;
			SetScrollInfo(hwnd, scroll, &info, TRUE);
			if (scroll == SB_HORZ) break;
			/* fall through */
	case SB_VERT:
			info.nMin = childRect.top;
			info.nMax = childRect.bottom - clientRect.bottom;
			info.nPos = clientRect.top - childRect.top;
			SetScrollInfo(hwnd, scroll, &info, TRUE);
			break;
    }
}


/***********************************************************************
 *		ScrollChildren (USER32.@)
 */
void WINAPI ScrollChildren(HWND hWnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam)
{
    INT newPos = -1;
    INT curPos, length, minPos, maxPos, shift;
    RECT rect;

    GetClientRect( hWnd, &rect );

    switch(uMsg)
    {
    case WM_HSCROLL:
	GetScrollRange(hWnd,SB_HORZ,&minPos,&maxPos);
	curPos = GetScrollPos(hWnd,SB_HORZ);
	length = (rect.right - rect.left) / 2;
	shift = GetSystemMetrics(SM_CYHSCROLL);
        break;
    case WM_VSCROLL:
	GetScrollRange(hWnd,SB_VERT,&minPos,&maxPos);
	curPos = GetScrollPos(hWnd,SB_VERT);
	length = (rect.bottom - rect.top) / 2;
	shift = GetSystemMetrics(SM_CXVSCROLL);
        break;
    default:
        return;
    }

    switch( wParam )
    {
	case SB_LINEUP:
		        newPos = curPos - shift;
			break;
	case SB_LINEDOWN:
			newPos = curPos + shift;
			break;
	case SB_PAGEUP:
			newPos = curPos - length;
			break;
	case SB_PAGEDOWN:
			newPos = curPos + length;
			break;

	case SB_THUMBPOSITION:
			newPos = LOWORD(lParam);
			break;

	case SB_THUMBTRACK:
			return;

	case SB_TOP:
			newPos = minPos;
			break;
	case SB_BOTTOM:
			newPos = maxPos;
			break;
	case SB_ENDSCROLL:
			CalcChildScroll(hWnd,(uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ);
			return;
    }

    if( newPos > maxPos )
	newPos = maxPos;
    else
	if( newPos < minPos )
	    newPos = minPos;

    SetScrollPos(hWnd, (uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ , newPos, TRUE);

    if( uMsg == WM_VSCROLL )
	ScrollWindowEx(hWnd ,0 ,curPos - newPos, NULL, NULL, 0, NULL,
			SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
    else
	ScrollWindowEx(hWnd ,curPos - newPos, 0, NULL, NULL, 0, NULL,
			SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
}


/******************************************************************************
 *		CascadeWindows (USER32.@) Cascades MDI child windows
 *
 * RETURNS
 *    Success: Number of cascaded windows.
 *    Failure: 0
 */
WORD WINAPI
CascadeWindows (HWND hwndParent, UINT wFlags, LPCRECT lpRect,
		UINT cKids, const HWND *lpKids)
{
    FIXME("(%p,0x%08x,...,%u,...): stub\n", hwndParent, wFlags, cKids);
    return 0;
}


/******************************************************************************
 *		TileWindows (USER32.@) Tiles MDI child windows
 *
 * RETURNS
 *    Success: Number of tiled windows.
 *    Failure: 0
 */
WORD WINAPI
TileWindows (HWND hwndParent, UINT wFlags, LPCRECT lpRect,
	     UINT cKids, const HWND *lpKids)
{
    FIXME("(%p,0x%08x,...,%u,...): stub\n", hwndParent, wFlags, cKids);
    return 0;
}

/************************************************************************
 *              "More Windows..." functionality
 */

/*              MDI_MoreWindowsDlgProc
 *
 *    This function will process the messages sent to the "More Windows..."
 *    dialog.
 *    Return values:  0    = cancel pressed
 *                    HWND = ok pressed or double-click in the list...
 *
 */

static INT_PTR WINAPI MDI_MoreWindowsDlgProc (HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
       case WM_INITDIALOG:
       {
#ifdef __REACTOS__
           /* FIXME */
           return FALSE;
#else
           UINT widest       = 0;
           UINT length;
           UINT i;
           MDICLIENTINFO *ci = get_client_info( (HWND)lParam );
           HWND hListBox = GetDlgItem(hDlg, MDI_IDC_LISTBOX);
           HWND *list, *sorted_list;

           if (!(list = WIN_ListChildren( (HWND)lParam ))) return TRUE;
           if (!(sorted_list = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                          sizeof(HWND) * ci->nActiveChildren )))
           {
               HeapFree( GetProcessHeap(), 0, list );
               return FALSE;
           }

           /* Fill the list, sorted by id... */
           for (i = 0; list[i]; i++)
           {
               UINT id = GetWindowLongW( list[i], GWL_ID ) - ci->idFirstChild;
               if (id < ci->nActiveChildren) sorted_list[id] = list[i];
           }
           HeapFree( GetProcessHeap(), 0, list );

           for (i = 0; i < ci->nActiveChildren; i++)
           {
               WCHAR buffer[128];

               if (!GetWindowTextW( sorted_list[i], buffer, sizeof(buffer)/sizeof(WCHAR) ))
                   continue;
               SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer );
               SendMessageW(hListBox, LB_SETITEMDATA, i, (LPARAM)sorted_list[i] );
               length = strlenW(buffer);  /* FIXME: should use GetTextExtentPoint */
               if (length > widest)
                   widest = length;
           }
           /* Make sure the horizontal scrollbar scrolls ok */
           SendMessageW(hListBox, LB_SETHORIZONTALEXTENT, widest * 6, 0);

           /* Set the current selection */
           SendMessageW(hListBox, LB_SETCURSEL, MDI_MOREWINDOWSLIMIT, 0);
           return TRUE;
#endif
       }

       case WM_COMMAND:
           switch (LOWORD(wParam))
           {
                default:
                    if (HIWORD(wParam) != LBN_DBLCLK) break;
                    /* fall through */
                case IDOK:
                {
                    /*  windows are sorted by menu ID, so we must return the
                     *  window associated to the given id
                     */
                    HWND hListBox     = GetDlgItem(hDlg, MDI_IDC_LISTBOX);
                    UINT index        = SendMessageW(hListBox, LB_GETCURSEL, 0, 0);
                    LRESULT res = SendMessageW(hListBox, LB_GETITEMDATA, index, 0);
                    EndDialog(hDlg, res);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return TRUE;
           }
           break;
    }
    return FALSE;
}

/*
 *
 *                      MDI_MoreWindowsDialog
 *
 *     Prompts the user with a listbox containing the opened
 *     documents. The user can then choose a windows and click
 *     on OK to set the current window to the one selected, or
 *     CANCEL to cancel. The function returns a handle to the
 *     selected window.
 */

static HWND MDI_MoreWindowsDialog(HWND hwnd)
{
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl;

    hRes = FindResourceA(GetModuleHandleA("USER32"), "MDI_MOREWINDOWS", (LPSTR)RT_DIALOG);

    if (hRes == 0)
        return 0;

    hDlgTmpl = LoadResource(GetModuleHandleA("USER32"), hRes );

    if (hDlgTmpl == 0)
        return 0;

    template = LockResource( hDlgTmpl );

    if (template == 0)
        return 0;

    return (HWND) DialogBoxIndirectParamA(GetModuleHandleA("USER32"),
                                          (LPDLGTEMPLATE) template,
                                          hwnd, MDI_MoreWindowsDlgProc, (LPARAM) hwnd);
}

/*
 *
 *                      MDI_SwapMenuItems
 *
 *      Will swap the menu IDs for the given 2 positions.
 *      pos1 and pos2 are menu IDs
 *
 *
 */

static void MDI_SwapMenuItems(HWND parent, UINT pos1, UINT pos2)
{
/* FIXME */
#ifndef __REACTOS__
    HWND *list;
    int i;

    if (!(list = WIN_ListChildren( parent ))) return;
    for (i = 0; list[i]; i++)
    {
        UINT id = GetWindowLongW( list[i], GWL_ID );
        if (id == pos1) SetWindowLongW( list[i], GWL_ID, pos2 );
        else if (id == pos2) SetWindowLongW( list[i], GWL_ID, pos1 );
    }
    HeapFree( GetProcessHeap(), 0, list );
#endif
}
