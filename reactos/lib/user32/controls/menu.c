/*
 * Menu functions
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 * Copyright 1997 Morten Welinder
 */

/*
 * Note: the style MF_MOUSESELECT is used to mark popup items that
 * have been selected, i.e. their popup menu is currently displayed.
 * This is probably not the meaning this style has in MS-Windows.
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>


#include <windows.h>
#include <user32/win.h>
#include <user32/debug.h>
#include <user32/resource.h>
#include <user32/sysmetr.h>
#include <user32/menu.h>
#include <user32/syscolor.h>




/**********************************************************************
 *         CreateMenu    (USER32.81)
 */
HMENU STDCALL CreateMenu(void)
{
    HMENU hMenu;
    LPPOPUPMENU menu;
    if (!(menu = HeapAlloc(GetProcessHeap(),0,sizeof(POPUPMENU) ) ) ) 
	return (HMENU)NULL;
    hMenu = (HANDLE)menu;
    menu->wFlags = 0;
    menu->wMagic = MENU_MAGIC;
    menu->hTaskQ = 0;
    menu->Width  = 0;
    menu->Height = 0;
    menu->nItems = 0;
    menu->hWnd   = 0;
    menu->items  = NULL;
    menu->FocusedItem = NO_SELECTED_ITEM;
    DPRINT( "return %04x\n", hMenu );
    return hMenu;
}





/**********************************************************************
 *         DestroyMenu    (USER32.134)
 */
WINBOOL STDCALL DestroyMenu( HMENU hMenu )
{
    DPRINT("menu (%04x)\n", (UINT)hMenu);

    /* Silently ignore attempts to destroy default system popup */

    if (hMenu && hMenu != MENU_DefSysPopup)
    {
	LPPOPUPMENU lppop = (LPPOPUPMENU) (hMenu);

	if( pTopPopupWnd && (hMenu == *(HMENU*)pTopPopupWnd->wExtra) )
	  *(UINT*)pTopPopupWnd->wExtra = 0;

	if (IS_A_MENU( lppop ))
	{
	    lppop->wMagic = 0;  /* Mark it as destroyed */

	    if ((lppop->wFlags & MF_POPUP) && lppop->hWnd &&
	        (!pTopPopupWnd || (lppop->hWnd != pTopPopupWnd->hwndSelf)))
	        DestroyWindow( lppop->hWnd );

	    if (lppop->items)	/* recursively destroy submenus */
	    {
	        int i;
	        MENUITEM *item = lppop->items;
	        for (i = lppop->nItems; i > 0; i--, item++)
	        {
	            if (item->fType & MF_POPUP) DestroyMenu(item->hSubMenu);
		    MENU_FreeItemData( item );
	        }
	        HeapFree( GetProcessHeap(), 0, lppop->items );
	    }
	    HeapFree( GetProcessHeap(),0,hMenu );
	}
	else return FALSE;
    }
    return (hMenu != MENU_DefSysPopup);
}





/**********************************************************************
 *         GetSystemMenu    (USER32.291)
 */
HMENU STDCALL GetSystemMenu( HWND hWnd, WINBOOL bRevert )
{
    WND *wndPtr = WIN_FindWndPtr( hWnd );

    if (wndPtr)
    {
	if( wndPtr->hSysMenu )
	{
	    if( bRevert )
	    {
		DestroyMenu(wndPtr->hSysMenu); 
		wndPtr->hSysMenu = 0;
	    }
	    else
	    {
		POPUPMENU *menu = (POPUPMENU*)(wndPtr->hSysMenu);
		if( menu->items[0].hSubMenu == MENU_DefSysPopup )
		    menu->items[0].hSubMenu = MENU_CopySysPopup();
	    }
	}

	if(!wndPtr->hSysMenu && (wndPtr->dwStyle & WS_SYSMENU) )
	    wndPtr->hSysMenu = MENU_GetSysMenu( hWnd, (HMENU)(-1) );

	if( wndPtr->hSysMenu )
	    return GetSubMenu(wndPtr->hSysMenu, 0);
    }
    return 0;
}





/*******************************************************************
 *         SetSystemMenu    (USER32.508)
 */
WINBOOL STDCALL SetSystemMenu( HWND hwnd, HMENU hMenu )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    if (wndPtr)
    {
	if (wndPtr->hSysMenu) 
		DestroyMenu( wndPtr->hSysMenu );
	wndPtr->hSysMenu = MENU_GetSysMenu( hwnd, hMenu );
	return TRUE;
    }
    return FALSE;
}




/**********************************************************************
 *         GetMenu    (USER32.257)
 */
HMENU STDCALL GetMenu( HWND hWnd ) 
{ 
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD)) 
	return (HMENU)wndPtr->wIDmenu;
    return 0;
}




/**********************************************************************
 *         SetMenu    (USER32.487)
 */
WINBOOL STDCALL SetMenu( HWND hWnd, HMENU hMenu )
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);

    DPRINT("menu (%04x, %04x);\n", (UINT)hWnd, (UINT)hMenu);

    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD))
    {
	if (GetCapture() == hWnd) ReleaseCapture();

	wndPtr->wIDmenu = (UINT)hMenu;
	if (hMenu != 0)
	{
	    LPPOPUPMENU lpmenu;

            if (!(lpmenu = (LPPOPUPMENU)(hMenu))) 
		return FALSE;
            lpmenu->hWnd = hWnd;
            lpmenu->wFlags &= ~MF_POPUP;  /* Can't be a popup */
            lpmenu->Height = 0;  /* Make sure we recalculate the size */
	}
	if (IsWindowVisible(hWnd))
            SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
	return TRUE;
    }
    return FALSE;
}



/**********************************************************************
 *         GetSubMenu    (USER32.288)
 */
HMENU STDCALL GetSubMenu( HMENU hMenu, INT nPos )
{
    LPPOPUPMENU lppop;

    if (!(lppop = (LPPOPUPMENU)(hMenu))) return 0;
    if ((UINT)nPos >= lppop->nItems) return 0;
    if (!(lppop->items[nPos].fType & MF_POPUP)) return 0;
    return lppop->items[nPos].hSubMenu;
}



/**********************************************************************
 *         DrawMenuBar    (USER32.1)
 */
WINBOOL STDCALL DrawMenuBar( HWND hWnd )
{
    LPPOPUPMENU lppop;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD) && wndPtr->wIDmenu)
    {
        lppop = (LPPOPUPMENU)((HMENU)wndPtr->wIDmenu);
        if (lppop == NULL) return FALSE;

        lppop->Height = 0; /* Make sure we call MENU_MenuBarCalcSize */
        SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
        return TRUE;
    }
    return FALSE;
}







/*****************************************************************
 *        LoadMenuA   (USER32.370)
 */
HMENU STDCALL LoadMenuA( HINSTANCE instance, LPCSTR name )
{

    HRSRC hrsrc = FindResourceA(  GetModuleHandle(NULL), name, (LPCSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirectA( (LPCVOID)LoadResource( GetModuleHandle(NULL), hrsrc ));
}


/*****************************************************************
 *        LoadMenuW   (USER32.373)
 */
HMENU STDCALL LoadMenuW( HINSTANCE hInstance, LPCWSTR name )
{

    HRSRC hrsrc = FindResourceW(  GetModuleHandle(NULL), name, (LPCSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirectW( (LPCVOID)LoadResource( GetModuleHandle(NULL), hrsrc ));
}
/*

A menu template consists of a MENUITEMTEMPLATEHEADER structure 
followed by one or more contiguous MENUITEMTEMPLATE structures. 
In Windows 95, an extended menu template consists of a 
MENUEX_TEMPLATE_HEADER structure followed by one or more 
contiguous MENUEX_TEMPLATE_ITEM structures.  


typedef struct {
  WORD  wVersion; 
  WORD  wOffset; 
  DWORD dwHelpId; 
} MENUEX_TEMPLATE_HEADER;

typedef struct { 
  DWORD  dwType; 
  DWORD  dwState; 
  UINT   uId; 
  BYTE   bResInfo; 
  WCHAR  szText[1]; 
  DWORD dwHelpId; 
} MENUEX_TEMPLATE_ITEM; 

typedef struct tagMENUITEMINFO {
  UINT    cbSize; 
  UINT    fMask; 
  UINT    fType; 
  UINT    fState; 
  UINT    wID; 
  HMENU   hSubMenu; 
  HBITMAP hbmpChecked; 
  HBITMAP hbmpUnchecked; 
  DWORD   dwItemData; 
  LPTSTR  dwTypeData; 
  UINT    cch; 
} MENUITEMINFO, *LPMENUITEMINFO; 
typedef MENUITEMINFO CONST *LPCMENUITEMINFO;
 
typedef struct {   
  WORD mtOption;      
  WORD mtID;          
  WCHAR mtString[1];  
} MENUITEMTEMPLATE; 
 
typedef struct {      
  WORD versionNumber; 
  WORD offset;        
} MENUITEMTEMPLATEHEADER; 
typedef VOID MENUTEMPLATE, *LPMENUTEMPLATE;

*/

/**********************************************************************
 *	    LoadMenuIndirectA    (USER32.371)
 */
HMENU
STDCALL
LoadMenuIndirectA(
    CONST MENUTEMPLATE *lpMenuTemplate)
{
	return NULL;
}


/**********************************************************************
 *	    LoadMenuIndirectW    (USER32.372)
 */
HMENU
STDCALL
LoadMenuIndirectW(
    CONST MENUTEMPLATE *lpMenuTemplate)
{
	return NULL;
}



/**********************************************************************
 *		IsMenu    (USER32.346)
 */
WINBOOL STDCALL IsMenu(HMENU hMenu)
{
    POPUPMENU *pMenu = hMenu;
    return ((pMenu) && (pMenu)->wMagic == MENU_MAGIC);
}


/**********************************************************************
 *		GetMenuItemInfoA    (USER32.264)
 */
WINBOOL STDCALL GetMenuItemInfoA( HMENU hmenu, UINT item, WINBOOL bypos,
                                  LPMENUITEMINFO lpmii)
{
     int len;
     MENUITEM *menu = MENU_FindItem (&hmenu, &item, bypos? MF_BYPOSITION : 0);
  //  debug_print_menuitem("GetMenuItemInfo_common: ", menu, "");
    if (!menu)
	return FALSE;

    if (lpmii->fMask & MIIM_TYPE) {
	lpmii->fType = menu->fType;
	switch (MENU_ITEM_TYPE(menu->fType)) {
	case MF_STRING:
	    	if (menu->text && lpmii->dwTypeData && lpmii->cch) {
	 	    len = min( lpmii->cch, wcslen(menu->text));
		    lstrcpynWtoA((LPSTR) lpmii->dwTypeData,menu->text,len);
		    lpmii->cch = len;
		}
	    break;
	case MF_OWNERDRAW:
	case MF_BITMAP:
	    lpmii->dwTypeData = (DWORD)menu->text;
	    break;
	default:
	    break;
    }
    }
    if (lpmii->fMask & MIIM_STATE)
	lpmii->fState = menu->fState;

    if (lpmii->fMask & MIIM_ID)
	lpmii->wID = menu->wID;

    if (lpmii->fMask & MIIM_SUBMENU)
	lpmii->hSubMenu = menu->hSubMenu;

    if (lpmii->fMask & MIIM_CHECKMARKS) {
	lpmii->hbmpChecked = menu->hCheckBit;
	lpmii->hbmpUnchecked = menu->hUnCheckBit;
    }
    if (lpmii->fMask & MIIM_DATA)
	lpmii->dwItemData = menu->dwItemData;

    return TRUE;
    
}

/**********************************************************************
 *		GetMenuItemInfoW    (USER32.265)
 */
WINBOOL STDCALL GetMenuItemInfoW( HMENU hmenu, UINT item, WINBOOL bypos,
                                  LPMENUITEMINFO lpmii)
{
     int len;
     MENUITEM *menu = MENU_FindItem (&hmenu, &item, bypos? MF_BYPOSITION : 0);
  //  debug_print_menuitem("GetMenuItemInfo_common: ", menu, "");
    if (!menu)
	return FALSE;

    if (lpmii->fMask & MIIM_TYPE) {
	lpmii->fType = menu->fType;
	switch (MENU_ITEM_TYPE(menu->fType)) {
	case MF_STRING:
	    	if (menu->text && lpmii->dwTypeData && lpmii->cch) {
	 	    len = min( lpmii->cch, wcslen(menu->text));
		    lstrcpynW((LPWSTR) lpmii->dwTypeData,menu->text,len);
		    lpmii->cch = len;
		}
	    break;
	case MF_OWNERDRAW:
	case MF_BITMAP:
	    lpmii->dwTypeData = (DWORD)menu->text;
	    break;
	default:
	    break;
    }
    }
    if (lpmii->fMask & MIIM_STATE)
	lpmii->fState = menu->fState;

    if (lpmii->fMask & MIIM_ID)
	lpmii->wID = menu->wID;

    if (lpmii->fMask & MIIM_SUBMENU)
	lpmii->hSubMenu = menu->hSubMenu;

    if (lpmii->fMask & MIIM_CHECKMARKS) {
	lpmii->hbmpChecked = menu->hCheckBit;
	lpmii->hbmpUnchecked = menu->hUnCheckBit;
    }
    if (lpmii->fMask & MIIM_DATA)
	lpmii->dwItemData = menu->dwItemData;

    return TRUE;
}



/**********************************************************************
 *		SetMenuItemInfoA    (USER32.491)
 */
WINBOOL STDCALL SetMenuItemInfoA(HMENU hmenu, UINT item, WINBOOL bypos,
                                 const MENUITEMINFO *lpmiiBuf) 
{
    LPMENUITEMINFO lpmii = (LPMENUITEMINFO)lpmiiBuf;
    WCHAR MenuTextW[MAX_PATH];
    char *MenuTextA;
    WINBOOL bRet;
    int i = 0;
    MenuTextA = lpmii->dwTypeData;

    lpmii->dwTypeData = MenuTextW;
    while ( MenuTextA[i] != 0 && i < MAX_PATH ) 
    {
	MenuTextW[i] = MenuTextA[i];
	i++;
    }
    MenuTextW[i] = 0;
    lpmii->dwTypeData = MenuTextW;
    bRet =  SetMenuItemInfoW(hmenu, item, bypos, lpmii);
    lpmii->dwTypeData = MenuTextA;
    
    return bRet;
}

/**********************************************************************
 *		SetMenuItemInfoW    (USER32.492)
 */
WINBOOL STDCALL SetMenuItemInfoW(HMENU hmenu, UINT item, WINBOOL bypos,
                                 const MENUITEMINFO *lpmii)
{
     MENUITEM *menu = MENU_FindItem (&hmenu, &item, bypos? MF_BYPOSITION : 0);
    if (!menu) return FALSE;

    if (lpmii->fMask & MIIM_TYPE) {
	/* Get rid of old string.  */
	if (IS_STRING_ITEM(menu->fType) && menu->text)
	    HeapFree(GetProcessHeap(), 0, menu->text);

	menu->fType = lpmii->fType;
	menu->text = lpmii->dwTypeData;
	if (IS_STRING_ITEM(menu->fType) && menu->text) {
		    menu->text = HeapAlloc(GetProcessHeap(), 0,(lpmii->cch + 1)*sizeof(WCHAR));
		    lstrcpynW(menu->text,(LPWSTR) lpmii->dwTypeData,lpmii->cch);
		}
	
	
    }
    if (lpmii->fMask & MIIM_STATE)
	menu->fState = lpmii->fState;

    if (lpmii->fMask & MIIM_ID)
	menu->wID = lpmii->wID;

    if (lpmii->fMask & MIIM_SUBMENU) {
	menu->hSubMenu = lpmii->hSubMenu;
	if (menu->hSubMenu) {
	    POPUPMENU *subMenu = (POPUPMENU *)((UINT)menu->hSubMenu);
	    if (IS_A_MENU(subMenu)) {
		subMenu->wFlags |= MF_POPUP;
		menu->fType |= MF_POPUP;
	    }
	    else
		/* FIXME: Return an error ? */
		menu->fType &= ~MF_POPUP;
	}
	else
	    menu->fType &= ~MF_POPUP;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS)
    {
	menu->hCheckBit = lpmii->hbmpChecked;
	menu->hUnCheckBit = lpmii->hbmpUnchecked;
    }
    if (lpmii->fMask & MIIM_DATA)
	menu->dwItemData = lpmii->dwItemData;

    //debug_print_menuitem("SetMenuItemInfo_common: ", menu, "");
    return TRUE;
}

/**********************************************************************
 *		SetMenuDefaultItem    (USER32.489)
 */
WINBOOL STDCALL SetMenuDefaultItem(HMENU hmenu, UINT item, UINT bypos)
{
    MENUITEM *menuitem = MENU_FindItem(&hmenu, &item, bypos);
    POPUPMENU *menu;

    if (!menuitem) return FALSE;
    if (!(menu = (POPUPMENU *)(hmenu))) return FALSE;

    menu->defitem = item; /* position */

  //  debug_print_menuitem("SetMenuDefaultItem: ", menuitem, "");
  //  FIXME(menu, "(0x%x,%d,%d), empty stub!\n",
//		  hmenu, item, bypos);
    return TRUE;
}

/**********************************************************************
 *		GetMenuDefaultItem    (USER32.260)
 */
UINT STDCALL GetMenuDefaultItem(HMENU hmenu, UINT bypos, UINT flags)
{
    POPUPMENU *menu;

    if (!(menu = (POPUPMENU *)(hmenu)))
	return -1;

    //FIXME(menu, "(0x%x,%d,%d), stub!\n", hmenu, bypos, flags);

// bypos should be specified TRUE to return default item by position
// flags are ignored

    if (bypos & MF_BYPOSITION)
    	return menu->defitem;
    else {
	//FIXME (menu, "default item 0x%x\n", menu->defitem);
	if ((menu->defitem > 0) && (menu->defitem < menu->nItems))
	    return menu->items[menu->defitem].wID;
    }
    return -1;
}



/**********************************************************************
 *		InsertMenuItemA    
 */
WINBOOL STDCALL InsertMenuItemA(HMENU hMenu, UINT uItem, WINBOOL bypos,
                                const MENUITEMINFO *lpmiiBuf)
{

    LPMENUITEMINFO lpmii = lpmiiBuf;
    WCHAR MenuTextW[MAX_PATH];
    char *MenuTextA;
    WINBOOL bRet;
    int i=0;
    MenuTextA = lpmii->dwTypeData;
    lpmii->dwTypeData = MenuTextW;
    while ( MenuTextA[i] != 0 && i < MAX_PATH ) 
    {
	MenuTextW[i] = MenuTextA[i];
	i++;
    }
    MenuTextW[i] = 0;
    lpmii->dwTypeData = (LPWSTR)MenuTextW;
    bRet =  InsertMenuItemW(hMenu, uItem, bypos, lpmii);
    lpmii->dwTypeData = (LPWSTR)MenuTextA;    
    return bRet;


}


/**********************************************************************
 *		InsertMenuItemW    
 */
WINBOOL STDCALL InsertMenuItemW(HMENU hMenu, UINT uItem, WINBOOL bypos,
                                const MENUITEMINFO *lpmii)
{
    return SetMenuItemInfoW(hMenu,uItem,bypos,lpmii);
}

/**********************************************************************
 *		CheckMenuRadioItem    
 */

WINBOOL STDCALL CheckMenuRadioItem(HMENU hMenu,
				   UINT first, UINT last, UINT check,
				   UINT bypos)
{
     MENUITEM *mifirst, *milast, *micheck;
     HMENU mfirst = hMenu, mlast = hMenu, mcheck = hMenu;

  //   TRACE(menu, "ox%x: %d-%d, check %d, bypos=%d\n",
//		  hMenu, first, last, check, bypos);

     mifirst = MENU_FindItem (&mfirst, &first, bypos);
     milast = MENU_FindItem (&mlast, &last, bypos);
     micheck = MENU_FindItem (&mcheck, &check, bypos);

     if (mifirst == NULL || milast == NULL || micheck == NULL ||
	 mifirst > milast || mfirst != mlast || mfirst != mcheck ||
	 micheck > milast || micheck < mifirst)
	  return FALSE;

     while (mifirst <= milast)
     {
	  if (mifirst == micheck)
	  {
	       mifirst->fType |= MFT_RADIOCHECK;
	       mifirst->fState |= MFS_CHECKED;
	  } else {
	       mifirst->fType &= ~MFT_RADIOCHECK;
	       mifirst->fState &= ~MFS_CHECKED;
	  }
	  mifirst++;
     }

     return TRUE;
}



/**********************************************************************
 *		GetMenuItemRect    
 */

WINBOOL STDCALL GetMenuItemRect (HWND hwnd, HMENU hMenu, UINT uItem,
				 LPRECT rect)
{
     RECT saverect, clientrect;
     WINBOOL barp;
     HDC hdc;
     WND *wndPtr;
     MENUITEM *item;
     HMENU orghMenu = hMenu;

  //   TRACE(menu, "(0x%x,0x%x,%d,%p)\n",
//		  hwnd, hMenu, uItem, rect);

     item = MENU_FindItem (&hMenu, &uItem, MF_BYPOSITION);
     wndPtr = WIN_FindWndPtr (hwnd);
     if (!rect || !item || !wndPtr) return FALSE;

     GetClientRect( hwnd, &clientrect );
     hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
     barp = (hMenu == orghMenu);

     saverect = item->rect;
     MENU_CalcItemSize (hdc, item, hwnd,
			clientrect.left, clientrect.top, barp);
     *rect = item->rect;
     item->rect = saverect;

     ReleaseDC( hwnd, hdc );
     return TRUE;
}



/**********************************************************************
 *         SetMenuContextHelpId    
 */
WINBOOL STDCALL SetMenuContextHelpId( HMENU hMenu, DWORD dwContextHelpId)
{
     POPUPMENU *pMenu;

    if (!(pMenu = (POPUPMENU *)(hMenu)))
	return FALSE;
    pMenu->dwContextHelpId = dwContextHelpId;
    return TRUE;
}


 
/**********************************************************************
 *         GetMenuContextHelpId    
 */
DWORD STDCALL GetMenuContextHelpId( HMENU hMenu )
{
    POPUPMENU *pMenu;

    if (!(pMenu = (POPUPMENU *)(hMenu)))
	return -1;
    return pMenu->dwContextHelpId;
}
 




/**********************************************************************
 *         CreatePopupMenu    (USER32.82)
 */
HMENU STDCALL CreatePopupMenu(void)
{
    HMENU hmenu;
    POPUPMENU *menu;

    if (!(hmenu = CreateMenu())) return 0;
    menu = (POPUPMENU *)( hmenu );
    menu->wFlags |= MF_POPUP;
    return hmenu;
}

/**********************************************************************
 *           TrackPopupMenu   (USER32.549)
 */
WINBOOL STDCALL TrackPopupMenu( HMENU hMenu, UINT wFlags, INT x, INT y,
                           INT nReserved, HWND hWnd, const RECT *lpRect )
{
    WINBOOL ret = FALSE;

    HideCaret(0);
    SendMessageA( hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, 0);
    if (MENU_ShowPopup( hWnd, hMenu, 0, x, y, 0, 0 )) 
	ret = MENU_TrackMenu( hMenu, wFlags & ~TPM_INTERNAL, 0, 0, hWnd, lpRect );
    ShowCaret(0);
    return ret;
}

/**********************************************************************
 *           TrackPopupMenuEx   (USER32.550)
 */
WINBOOL STDCALL TrackPopupMenuEx( HMENU hMenu, UINT wFlags, INT x, INT y,
                                HWND hWnd, LPTPMPARAMS lpTpm )
{
//    FIXME(menu, "not fully implemented\n" );
    return TrackPopupMenu( hMenu, wFlags, x, y, 0, hWnd,
                             lpTpm ? &lpTpm->rcExclude : NULL );
}

/***********************************************************************
 *           PopupMenuWndProc
 *
 * NOTE: Windows has totally different (and undocumented) popup wndproc.
 */
LRESULT STDCALL PopupMenuWndProcA( HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam )
{    
    WND* wndPtr = WIN_FindWndPtr(hwnd);

    switch(message)
    {
    case WM_CREATE:
	{
	    CREATESTRUCT *cs = (CREATESTRUCT*)lParam;
	    SetWindowLong( hwnd, 0, (LONG)cs->lpCreateParams );
	    return 0;
	}

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
	return MA_NOACTIVATE;

    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    BeginPaint( hwnd, &ps );
	    MENU_DrawPopupMenu( hwnd, ps.hdc, (HMENU)GetWindowLong( hwnd, 0 ) );
	    EndPaint( hwnd, &ps );
	    return 0;
	}
    case WM_ERASEBKGND:
	return 1;

    case WM_DESTROY:

	/* zero out global pointer in case resident popup window
	 * was somehow destroyed. */

	if( pTopPopupWnd )
	{
	    if( hwnd == pTopPopupWnd->hwndSelf )
	    {
		DPRINT("menu resident popup destroyed!\n");

		pTopPopupWnd = NULL;
		uSubPWndLevel = 0;
	    }
	    else
		uSubPWndLevel--;
	}
	break;

    case WM_SHOWWINDOW:

	if( wParam )
	{
	    if( !(*(HMENU*)wndPtr->wExtra) ) {}
		 DPRINT("menu no menu to display\n");
	}
	else
	    (LRESULT)*(HMENU*)wndPtr->wExtra = 0;
	break;

    case MM_SETMENUHANDLE:

	(LRESULT)*(HMENU*)wndPtr->wExtra = (HMENU)wParam;
        break;

    case MM_GETMENUHANDLE:

	return (LRESULT)*(HMENU*)wndPtr->wExtra;

    default:
	return (LRESULT)DefWindowProcA( hwnd, message, wParam, lParam );
    }
    return (LRESULT)0;
}

/***********************************************************************
 *           PopupMenuWndProc
 *
 * NOTE: Windows has totally different (and undocumented) popup wndproc.
 */
LRESULT STDCALL PopupMenuWndProcW( HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam )
{    
    WND* wndPtr = WIN_FindWndPtr(hwnd);

    switch(message)
    {
    case WM_CREATE:
	{
	    CREATESTRUCT *cs = (CREATESTRUCT*)lParam;
	    SetWindowLong( hwnd, 0, (LONG)cs->lpCreateParams );
	    return 0;
	}

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
	return MA_NOACTIVATE;

    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    BeginPaint( hwnd, &ps );
	    MENU_DrawPopupMenu( hwnd, ps.hdc, (HMENU)GetWindowLong( hwnd, 0 ) );
	    EndPaint( hwnd, &ps );
	    return 0;
	}
    case WM_ERASEBKGND:
	return 1;

    case WM_DESTROY:

	/* zero out global pointer in case resident popup window
	 * was somehow destroyed. */

	if( pTopPopupWnd )
	{
	    if( hwnd == pTopPopupWnd->hwndSelf )
	    {
		DPRINT("menu resident popup destroyed!\n");

		pTopPopupWnd = NULL;
		uSubPWndLevel = 0;
	    }
	    else
		uSubPWndLevel--;
	}
	break;

    case WM_SHOWWINDOW:

	if( wParam )
	{
	    if( !(*(HMENU*)wndPtr->wExtra) )
		DPRINT("menu no menu to display\n");
	}
	else
	    (LRESULT)*(HMENU*)wndPtr->wExtra = 0;
	break;

    case MM_SETMENUHANDLE:

	(LRESULT)*(HMENU*)wndPtr->wExtra = (HMENU)wParam;
        break;

    case MM_GETMENUHANDLE:

	return (LRESULT)*(HMENU*)wndPtr->wExtra;

    default:
	return (LRESULT)DefWindowProcW( hwnd, message, wParam, lParam );
    }
    return (LRESULT)0;
}




/*******************************************************************
 *         ChangeMenuA    (USER32.23)
 */
WINBOOL STDCALL ChangeMenuA( HMENU hMenu, UINT pos, LPCSTR data,
                             UINT id, UINT flags )
{
    //DPRINT("menu menu=%08x pos=%d data=%08lx id=%08x flags=%08x\n",
    //              hMenu, pos, (DWORD)data, id, flags );
    if (flags & MF_APPEND) return AppendMenuA( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return DeleteMenu(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenuA(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenuA( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         ChangeMenuW    (USER32.24)
 */
WINBOOL STDCALL ChangeMenuW( HMENU hMenu, UINT pos, LPCWSTR data,
                             UINT id, UINT flags )
{
    //DPRINT("menu menu=%08x pos=%d data=%08lx id=%08x flags=%08x\n",
    //              hMenu, pos, (DWORD)data, id, flags );
    if (flags & MF_APPEND) return AppendMenuW( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return DeleteMenu(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenuW(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenuW( hMenu, pos, flags, id, data );
}

/*******************************************************************
 *         CheckMenuItem    (USER32.46)
 */
DWORD STDCALL CheckMenuItem( HMENU hMenu, UINT id, UINT flags )
{
    MENUITEM *item;
    DWORD ret;

    DPRINT("menu %04x %04x %04x\n", (UINT)hMenu, id, flags );

    if (!(item = MENU_FindItem( &hMenu, &id, flags ))) return -1;
    ret = item->fState & MF_CHECKED;
    if (flags & MF_CHECKED) item->fState |= MF_CHECKED;
    else item->fState &= ~MF_CHECKED;
    return ret;
}


/**********************************************************************
 *         EnableMenuItem    (USER32.170)
 */
WINBOOL
STDCALL
EnableMenuItem(
	       HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable)
{
    UINT    oldflags;
    MENUITEM *item;

    DPRINT("menu (%04x, %04X, %04X) !\n", (UINT) hMenu, uIDEnableItem, uEnable);

    if (!(item = MENU_FindItem( &hMenu, &uIDEnableItem, uEnable )))
	return (UINT)-1;

    oldflags = item->fState & (MF_GRAYED | MF_DISABLED);
    item->fState ^= (oldflags ^ uEnable) & (MF_GRAYED | MF_DISABLED);
    return oldflags;
}

/*******************************************************************
 *         GetMenuStringA    (USER32.268)
 */
INT STDCALL GetMenuStringA( HMENU hMenu, UINT wItemID,
                               LPSTR str, INT nMaxSiz, UINT wFlags )
{
    MENUITEM *item;
    int i;
    int len;
    DPRINT( "menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",  hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz)
	 return 0;
    str[0] = '\0';
    
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) 
	return 0;
    if (!IS_STRING_ITEM(item->fType)) return 0;

    len = min(lstrlenW(item->text),nMaxSiz-1);
    for(i=0;i<len+1;i++)
	str[i] = item->text[i];

    return len;
}


/*******************************************************************
 *         GetMenuStringW    (USER32.269)
 */
INT STDCALL GetMenuStringW( HMENU hMenu, UINT wItemID,
                               LPWSTR str, INT nMaxSiz, UINT wFlags )
{
    MENUITEM *item;

    DPRINT( "menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",   hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz) 
	return 0;
    str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) 
	return 0;
    lstrcpynW( str, item->text, nMaxSiz );
    	return lstrlenW(str);
}

/**********************************************************************
 *         HiliteMenuItem    (USER32.318)
 */
WINBOOL STDCALL HiliteMenuItem( HWND hWnd, HMENU hMenu, UINT wItemID,
                                UINT wHilite )
{
    LPPOPUPMENU menu;
    DPRINT("menu (%04x, %04x, %04x, %04x);\n", hWnd, hMenu, wItemID, wHilite);

    if (!MENU_FindItem( &hMenu, &wItemID, wHilite )) return FALSE;
    if (!(menu = (LPPOPUPMENU) (hMenu))) return FALSE;
    if (menu->FocusedItem == wItemID) return TRUE;
    MENU_HideSubPopups( hWnd, hMenu, FALSE );
    MENU_SelectItem( hWnd, hMenu, wItemID, TRUE );
    return TRUE;
}

/**********************************************************************
 *         GetMenuState    (USER32.267)
 */
UINT STDCALL GetMenuState( HMENU hMenu, UINT wItemID, UINT wFlags )
{
    MENUITEM *item;
    DPRINT("menu (%04x, %04x, %04x);\n", (UINT)hMenu, wItemID, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return -1;
    //debug_print_menuitem ("  item: ", item, "");
    if (item->fType & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *)( item->hSubMenu );
	if (!menu) return -1;
	else return (menu->nItems << 8) | ((item->fState|item->fType) & 0xff);
    }
    else
    {
	/* We used to (from way back then) mask the result to 0xff.  */
	/* I don't know why and it seems wrong as the documented */
	/* return flag MF_SEPARATOR is outside that mask.  */
	return (item->fType | item->fState);
    }
}

/**********************************************************************
 *         GetMenuItemCount    (USER32.262)
 */
INT STDCALL GetMenuItemCount( HMENU hMenu )
{
    LPPOPUPMENU	menu = (LPPOPUPMENU)(hMenu);
    if (!IS_A_MENU(menu)) return -1;
    DPRINT("menu (%04x) returning %d\n", (UINT)hMenu, menu->nItems );
    return menu->nItems;
}

/**********************************************************************
 *         GetMenuItemID    (USER32.263)
 */
UINT STDCALL GetMenuItemID( HMENU hMenu, INT nPos )
{
    LPPOPUPMENU	menu;

    if (!(menu = (LPPOPUPMENU)(hMenu))) return -1;
    if ((nPos < 0) || (nPos >= menu->nItems)) return -1;
    if (menu->items[nPos].fType & MF_POPUP) return -1; 
    return menu->items[nPos].wID;
}




/*******************************************************************
 *         InsertMenuA    (USER32.2)
 */
WINBOOL STDCALL InsertMenuA( HMENU hMenu, UINT pos, UINT flags,
                             UINT id, LPCSTR str )
{
	int i;
        WCHAR MenuTextW[MAX_PATH];
    	i = 0;
   	while ((*str)!=0 && i < MAX_PATH)
     	{
		MenuTextW[i] = *str;
		str++;
		i++;
     	}
   	MenuTextW[i] = 0;
	return InsertMenuW(hMenu,pos,flags,id,MenuTextW);


}


/*******************************************************************
 *         InsertMenuW    (USER32.5)
 */
WINBOOL STDCALL InsertMenuW( HMENU hMenu, UINT pos, UINT flags,
                             UINT id, LPCWSTR str )
{
     MENUITEM *item;
 

    if (!(item = MENU_InsertItem( hMenu, pos, flags ))) return FALSE;

    

    if (!(MENU_SetItemData( item, flags, id, str )))
    {
        RemoveMenu( hMenu, pos, flags );
        return FALSE;
    }

    if (flags & MF_POPUP)  /* Set the MF_POPUP flag on the popup-menu */
	((POPUPMENU *)((HMENU)id))->wFlags |= MF_POPUP;

    item->hCheckBit = item->hUnCheckBit = 0;
    return TRUE;
}




/*******************************************************************
 *         AppendMenuA    (USER32.5)
 */
WINBOOL STDCALL AppendMenuA( HMENU hMenu, UINT flags,
                             UINT id, LPCSTR data )
{
    return InsertMenuA( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenuW    (USER32.6)
 */
WINBOOL STDCALL AppendMenuW( HMENU hMenu, UINT flags,
                             UINT id, LPCWSTR data )
{
    return InsertMenuW( hMenu, -1, flags | MF_BYPOSITION, id, data );
}




/**********************************************************************
 *         RemoveMenu    (USER32.441)
 */
WINBOOL STDCALL RemoveMenu( HMENU hMenu, UINT nPos, UINT wFlags )
{
    LPPOPUPMENU	menu;
    MENUITEM *item;

    DPRINT("menu (%04x, %04x, %04x)\n",(UINT)hMenu, nPos, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;
    if (!(menu = (LPPOPUPMENU) (hMenu))) return FALSE;
    
      /* Remove item */

    MENU_FreeItemData( item );

    if (--menu->nItems == 0)
    {
        HeapFree( GetProcessHeap(), 0, menu->items );
        menu->items = NULL;
    }
    else
    {
	while(nPos < menu->nItems)
	{
	    *item = *(item+1);
	    item++;
	    nPos++;
	}
        menu->items = HeapReAlloc( GetProcessHeap(), 0, menu->items,
                                   menu->nItems * sizeof(MENUITEM) );
    }
    return TRUE;
}


/**********************************************************************
 *         DeleteMenu    (USER32.129)
 */
WINBOOL STDCALL DeleteMenu( HMENU hMenu, UINT nPos, UINT wFlags )
{
    MENUITEM *item = MENU_FindItem( &hMenu, &nPos, wFlags );
    if (!item) 
	return FALSE;
    if (item->fType & MF_POPUP) 
	DestroyMenu( item->hSubMenu );
      /* nPos is now the position of the item */
    RemoveMenu( hMenu, nPos, wFlags | MF_BYPOSITION );
    return TRUE;
}




/*******************************************************************
 *         ModifyMenuA    (USER32.397)
 */
WINBOOL STDCALL ModifyMenuA( HMENU hMenu, UINT pos, UINT flags,
                             UINT id, LPCSTR str )
{
	int i;
        WCHAR MenuTextW[MAX_PATH];
    	i = 0;
   	while ((*str)!=0 && i < MAX_PATH)
     	{
		MenuTextW[i] = *str;
		str++;
		i++;
     	}
   	MenuTextW[i] = 0;
	return ModifyMenuW(hMenu,pos,flags,id,MenuTextW);
}


/*******************************************************************
 *         ModifyMenuW    (USER32.398)
 */
WINBOOL
STDCALL
ModifyMenuW(HMENU hMnu,UINT uPosition,
    UINT uFlags, UINT uIDNewItem,
    LPCWSTR lpNewItem )
{
    MENUITEM *mnItem;

 
    if (!(mnItem = MENU_FindItem( &hMnu, &uPosition, uFlags ))) 
	return FALSE;
    return MENU_SetItemData( mnItem, uFlags, uIDNewItem, lpNewItem );
}







/**********************************************************************
 *         GetMenuCheckMarkDimensions    (USER32.258)
 */
LONG
STDCALL
GetMenuCheckMarkDimensions(VOID)
{

    return MAKELONG(GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK) );
}





/**********************************************************************
 *         SetMenuItemBitmaps    (USER32.490)
 */
WINBOOL STDCALL SetMenuItemBitmaps( HMENU hMenu, UINT nPos, UINT wFlags,
                                    HBITMAP hNewUnCheck, HBITMAP hNewCheck)
{
    MENUITEM *item;
    DPRINT("menu (%04x, %04x, %04x, %04x, %04x)\n",
                 (UINT)hMenu, nPos, wFlags, (UINT)hNewCheck, (UINT)hNewUnCheck);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;

    if (!hNewCheck && !hNewUnCheck)
    {
	item->fState &= ~MF_USECHECKBITMAPS;
    }
    else  /* Install new bitmaps */
    {
	item->hCheckBit = hNewCheck;
	item->hUnCheckBit = hNewUnCheck;
	item->fState |= MF_USECHECKBITMAPS;
    }
    return TRUE;
}




