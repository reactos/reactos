#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <user32/win.h>
//#include <user32/debug.h>
#include <user32/resource.h>
#include <user32/sysmetr.h>
#include <user32/menu.h>
#include <user32/nc.h>
#include <stdlib.h>

#include <wchar.h>
#include <stdlib.h>

typedef struct tagMDINEXTMENU 
{
    HMENU  hmenuIn;
    HMENU  hmenuNext;
    HWND   hwndNext;
} MDINEXTMENU, *PMDINEXTMENU, * LPMDINEXTMENU;

//FIXME should be defined in defines.h
#define MFT_RIGHTORDER     0x00002000L

/* Wine extension, I think.  */
#define OBM_RADIOCHECK      32727

  /* Dimension of the menu bitmaps */
static WORD check_bitmap_width = 0, check_bitmap_height = 0;
static WORD arrow_bitmap_width = 0, arrow_bitmap_height = 0;

#define MAX(x,y) x > y ? x : y

DWORD STDCALL GetTextExtent(HDC,LPCWSTR,INT);

static HBITMAP hStdRadioCheck = 0;
static HBITMAP hStdCheck = 0;
static HBITMAP hStdMnArrow = 0;
static HBRUSH hShadeBrush = 0;
HMENU MENU_DefSysPopup = 0;  /* Default system menu popup */

/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */ 

WND* pTopPopupWnd   = 0;
UINT uSubPWndLevel = 0;

  /* Flag set by EndMenu() to force an exit from menu tracking */
WINBOOL fEndMenu = FALSE;


#include<stdio.h>
#define DPRINT printf


/***********************************************************************
 *           debug_print_menuitem
 *
 * Print a menuitem in readable form.
 */
#if 0

#define debug_print_menuitem(pre, mp, post) \
  if(!TRACE_ON(menu)) ; else do_debug_print_menuitem(pre, mp, post)

#define MENUOUT(text) \
  sprintf(menu, "%s%s", (count++ ? "," : ""), (text))

 

#define MENUFLAG(bit,text) \
  do { \
    if (flags & (bit)) { flags &= ~(bit); MENUOUT ((text)); } \
  } while (0)

static void do_debug_print_menuitem(const char *prefix, MENUITEM * mp, 
				    const char *postfix)
{
    //dbg_decl_str(menu, 256);

    char menu[256];
    if (mp) {
	UINT flags = mp->fType;
	int typ = MENU_ITEM_TYPE(flags);
	sprintf(menu, "{ ID=0x%x", mp->wID);
	if (flags & MF_POPUP)
	    sprintf(menu, ", Sub=0x%x",(int) mp->hSubMenu);
	if (flags) {
	    int count = 0;
	    sprintf(menu, ", Typ=");
	    if (typ == MFT_STRING)
		/* Nothing */ ;
	    else if (typ == MFT_SEPARATOR)
		MENUOUT("sep");
	    else if (typ == MFT_OWNERDRAW)
		MENUOUT("own");
	    else if (typ == MFT_BITMAP)
		MENUOUT("bit");
	    else
		MENUOUT("???");
	    flags -= typ;

	    MENUFLAG(MF_POPUP, "pop");
	    MENUFLAG(MFT_MENUBARBREAK, "barbrk");
	    MENUFLAG(MFT_MENUBREAK, "brk");
	    MENUFLAG(MFT_RADIOCHECK, "radio");
	    MENUFLAG(MFT_RIGHTORDER, "rorder");
	    MENUFLAG(MF_SYSMENU, "sys");
	    MENUFLAG(MFT_RIGHTJUSTIFY, "right");

	    if (flags)
		sprintf(menu, "+0x%x", flags);
	}
	flags = mp->fState;
	if (flags) {
	    int count = 0;
	    sprintf(menu, ", State=");
	    MENUFLAG(MFS_GRAYED, "grey");
	    MENUFLAG(MFS_DISABLED, "dis");
	    MENUFLAG(MFS_CHECKED, "check");
	    MENUFLAG(MFS_HILITE, "hi");
	    MENUFLAG(MF_USECHECKBITMAPS, "usebit");
	    MENUFLAG(MF_MOUSESELECT, "mouse");
	    if (flags)
		sprintf(menu, "+0x%x", flags);
	}
	if (mp->hCheckBit)
	    sprintf(menu, ", Chk=0x%x",(int) mp->hCheckBit);
	if (mp->hUnCheckBit)
	    sprintf(menu, ", Unc=0x%x",(int)mp->hUnCheckBit);

	if (typ == MFT_STRING) {
	    if (mp->text)
		sprintf(menu, ", Text=\"%s\"", mp->text);
	    else
		sprintf(menu, ", Text=Null");
	} else if (mp->text == NULL)
	    /* Nothing */ ;
	else
	    sprintf(menu, ", Text=%p", mp->text);
	sprintf(menu, " }");
    } else {
	sprintf(menu, "NULL");
    }

    //DPRINT( "%s %s %s\n", prefix, dbg_str(menu), postfix);
}

#undef MENUOUT
#undef MENUFLAG
#endif
/***********************************************************************
 *           MENU_CopySysPopup
 *
 * Return the default system menu.
 */
HMENU MENU_CopySysPopup(void)
{
    HMENU hMenu = LoadMenuIndirectA(SYSRES_GetResPtr(SYSRES_MENU_SYSMENU));

    if( hMenu ) {
        POPUPMENU* menu = (POPUPMENU *)(hMenu);
        menu->wFlags |= MF_SYSMENU | MF_POPUP;
    }
    else {
	hMenu = 0;
//	DPRINT( "Unable to load default system menu\n" );
    }

//    DPRINT( "returning %x.\n", hMenu );

    return hMenu;
}


/**********************************************************************
 *           MENU_GetSysMenu
 *
 * Create a copy of the system menu. System menu in Windows is
 * a special menu-bar with the single entry - system menu popup.
 * This popup is presented to the outside world as a "system menu". 
 * However, the real system menu handle is sometimes seen in the 
 * WM_MENUSELECT paramemters (and Word 6 likes it this way).
 */
HMENU MENU_GetSysMenu( HWND hWnd, HMENU hPopupMenu )
{
    HMENU hMenu;

    if ((hMenu = CreateMenu()))
    {
	POPUPMENU *menu = (POPUPMENU*) (hMenu);
	menu->wFlags = MF_SYSMENU;
	menu->hWnd = hWnd;

	if (hPopupMenu == (HMENU)(-1))
	    hPopupMenu = MENU_CopySysPopup();
	else if( !hPopupMenu ) hPopupMenu = MENU_DefSysPopup; 

	if (hPopupMenu)
	{
	    InsertMenuA( hMenu, -1, MF_SYSMENU | MF_POPUP | MF_BYPOSITION, (int)hPopupMenu, NULL );

            menu->items[0].fType = MF_SYSMENU | MF_POPUP;
            menu->items[0].fState = 0;
	    menu = (POPUPMENU*) (hPopupMenu);
	    menu->wFlags |= MF_SYSMENU;

//	    DPRINT("GetSysMenu hMenu=%04x (%04x)\n", hMenu, hPopupMenu );
	    return hMenu;
	}
	DestroyMenu( hMenu );
    }
//    ERR(menu, "failed to load system menu!\n");
    return 0;
}


/***********************************************************************
 *           MENU_Init
 *
 * Menus initialisation.
 */
WINBOOL MENU_Init()
{
    HBITMAP hBitmap;
    static unsigned char shade_bits[] = { 0x55, 0, 0xAA, 0,
					    0x55, 0, 0xAA, 0,
					    0x55, 0, 0xAA, 0,
					    0x55, 0, 0xAA, 0 };

    /* Load menu bitmaps */
    hStdCheck = LoadBitmapA(0, MAKEINTRESOURCE(OBM_CHECK));
    hStdRadioCheck = LoadBitmapA(0, MAKEINTRESOURCE(OBM_RADIOCHECK));
    hStdMnArrow = LoadBitmapA(0, MAKEINTRESOURCE(OBM_MNARROW));

    if (hStdCheck)
    {
	BITMAP bm;
	GetObjectA( hStdCheck, sizeof(bm), &bm );
	check_bitmap_width = bm.bmWidth;
	check_bitmap_height = bm.bmHeight;
    } else
	 return FALSE;

    /* Assume that radio checks have the same size as regular check.  */
    if (!hStdRadioCheck)
	 return FALSE;

    if (hStdMnArrow)
	{
	 BITMAP bm;
	    GetObjectA( hStdMnArrow, sizeof(bm), &bm );
	    arrow_bitmap_width = bm.bmWidth;
	    arrow_bitmap_height = bm.bmHeight;
    } else
	 return FALSE;

    if ((hBitmap = CreateBitmap( 8, 8, 1, 1, shade_bits)))
	    {
		if((hShadeBrush = CreatePatternBrush( hBitmap )))
		{
		    DeleteObject( hBitmap );
	      if ((MENU_DefSysPopup = MENU_CopySysPopup()))
		   return TRUE;
	}
    }

    return FALSE;
}

/***********************************************************************
 *           MENU_InitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
 void MENU_InitSysMenuPopup( HMENU hmenu, DWORD style, DWORD clsStyle )
{
    WINBOOL gray;

    gray = !(style & WS_THICKFRAME) || (style & (WS_MAXIMIZE | WS_MINIMIZE));
    EnableMenuItem( hmenu, SC_SIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = ((style & WS_MAXIMIZE) != 0);
    EnableMenuItem( hmenu, SC_MOVE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MINIMIZEBOX) || (style & WS_MINIMIZE);
    EnableMenuItem( hmenu, SC_MINIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MAXIMIZEBOX) || (style & WS_MAXIMIZE);
    EnableMenuItem( hmenu, SC_MAXIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & (WS_MAXIMIZE | WS_MINIMIZE));
    EnableMenuItem( hmenu, SC_RESTORE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = (clsStyle & CS_NOCLOSE) != 0;
    EnableMenuItem( hmenu, SC_CLOSE, (gray ? MF_GRAYED : MF_ENABLED) );
}


/******************************************************************************
 *
 *   UINT  MENU_GetStartOfNextColumn(
 *     HMENU  hMenu )
 *
 *****************************************************************************/

 UINT  MENU_GetStartOfNextColumn(
    HMENU  hMenu )
{
    POPUPMENU  *menu = (POPUPMENU *)(hMenu);
    UINT  i = menu->FocusedItem + 1;

    if(!menu)
	return NO_SELECTED_ITEM;

    if( i == NO_SELECTED_ITEM )
	return i;

    for( ; i < menu->nItems; ++i ) {
	if (menu->items[i].fType & MF_MENUBARBREAK)
	    return i;
    }

    return NO_SELECTED_ITEM;
}


/******************************************************************************
 *
 *   UINT  MENU_GetStartOfPrevColumn(
 *     HMENU  hMenu )
 *
 *****************************************************************************/

 UINT  MENU_GetStartOfPrevColumn(
    HMENU  hMenu )
{
    POPUPMENU const  *menu = (POPUPMENU *)(hMenu);
    UINT  i;

    if( !menu )
	return NO_SELECTED_ITEM;

    if( menu->FocusedItem == 0 || menu->FocusedItem == NO_SELECTED_ITEM )
	return NO_SELECTED_ITEM;

    /* Find the start of the column */

    for(i = menu->FocusedItem; i != 0 &&
	 !(menu->items[i].fType & MF_MENUBARBREAK);
	--i); /* empty */

    if(i == 0)
	return NO_SELECTED_ITEM;

    for(--i; i != 0; --i) {
	if (menu->items[i].fType & MF_MENUBARBREAK)
	    break;
    }

//    DPRINT( "ret %d.\n", i );

    return i;
}



/***********************************************************************
 *           MENU_GetMenuBarHeight
 *
 * Compute the size of the menu bar height. Used by NC_HandleNCCalcSize().
 */
UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                              INT orgX, INT orgY )
{
    HDC hdc;
    RECT rectBar;
    WND *wndPtr;
    LPPOPUPMENU lppop;


    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(lppop = (LPPOPUPMENU)((HMENU)wndPtr->wIDmenu)))
      return 0;
    hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    SetRect(&rectBar, orgX, orgY, orgX+menubarWidth, orgY+SYSMETRICS_CYMENU);
    MENU_MenuBarCalcSize( hdc, &rectBar, lppop, hwnd );
    ReleaseDC( hwnd, hdc );
    return lppop->Height;
}



/***********************************************************************
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
 MENUITEM *MENU_FindItem( HMENU *hmenu, UINT *nPos, UINT wFlags )
{
    POPUPMENU *menu;
    UINT i;

    if (!(menu = (POPUPMENU *)(*hmenu))) return NULL;
    if (wFlags & MF_BYPOSITION)
    {
	if (*nPos >= menu->nItems) return NULL;
	return &menu->items[*nPos];
    }
    else
    {
        MENUITEM *item = menu->items;
	for (i = 0; i < menu->nItems; i++, item++)
	{
	    if (item->wID == *nPos)
	    {
		*nPos = i;
		return item;
	    }
	    else if (item->fType & MF_POPUP)
	    {
		HMENU hsubmenu = item->hSubMenu;
		MENUITEM *subitem = MENU_FindItem( &hsubmenu, nPos, wFlags );
		if (subitem)
		{
		    *hmenu = hsubmenu;
		    return subitem;
		}
	    }
	}
    }
    return NULL;
}

/***********************************************************************
 *           MENU_FreeItemData
 */
void MENU_FreeItemData( MENUITEM* item )
{
    /* delete text */
    if (IS_STRING_ITEM(item->fType) && item->text)
        HeapFree( GetProcessHeap(), 0, item->text );
}

/***********************************************************************
 *           MENU_FindItemByCoords
 *
 * Find the item at the specified coordinates (screen coords). Does 
 * not work for child windows and therefore should not be called for 
 * an arbitrary system menu.
 */
MENUITEM *MENU_FindItemByCoords( POPUPMENU *menu, 
					POINT pt, UINT *pos )
{
    MENUITEM *item;
    WND *wndPtr;
    UINT i;

    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return NULL;
    pt.x -= wndPtr->rectWindow.left;
    pt.y -= wndPtr->rectWindow.top;
    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++)
    {
	if ((pt.x >= item->rect.left) && (pt.x < item->rect.right) &&
	    (pt.y >= item->rect.top) && (pt.y < item->rect.bottom))
	{
	    if (pos) *pos = i;
	    return item;
	}
    }
    return NULL;
}


/***********************************************************************
 *           MENU_FindItemByKey
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
UINT MENU_FindItemByKey( HWND hwndOwner, HMENU hmenu, 
				  UINT key, WINBOOL forceMenuChar )
{
//    DPRINT("\tlooking for '%c' in [%04x]\n", (char)key, (UINT)hmenu );

    if (!IsMenu( hmenu )) 
    {
	WND* w = WIN_FindWndPtr(hwndOwner);
	hmenu = GetSubMenu(w->hSysMenu, 0);
    }

    if (hmenu)
    {
	POPUPMENU *menu = (POPUPMENU *) ( hmenu );
	MENUITEM *item = menu->items;
	LONG menuchar;

	if( !forceMenuChar )
	{
	     UINT i;

	     key = toupper(key);
	     for (i = 0; i < menu->nItems; i++, item++)
	     {
		if (item->text && (IS_STRING_ITEM(item->fType)))
		{
		    WCHAR *p = item->text - 2;
		    do
		    {
		    	p = wcschr (p + 2, '&');
		    }
		    while (p != NULL && p [1] == '&');
		    if (p && (towupper(p[1]) == key)) return i;
		}
	     }
	}
	menuchar = SendMessageW( hwndOwner, WM_MENUCHAR, (WPARAM)MAKEWPARAM( key, menu->wFlags ),(LPARAM) hmenu );
	if (HIWORD(menuchar) == 2) return LOWORD(menuchar);
	if (HIWORD(menuchar) == 1) return (UINT)(-2);
    }
    return (UINT)(-1);
}


/***********************************************************************
 *           MENU_CalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
void MENU_CalcItemSize( HDC hdc, MENUITEM *lpitem, HWND hwndOwner,
			       INT orgX, INT orgY, WINBOOL menuBar )
{
    SIZE szSize;
    WCHAR *p;

//    DPRINT( "HDC 0x%x at (%d,%d)\n",
//                 hdc, orgX, orgY);
//    debug_print_menuitem("MENU_CalcItemSize: menuitem:", lpitem, 
//			 (menuBar ? " (MenuBar)" : ""));

    SetRect( &lpitem->rect, orgX, orgY, orgX, orgY );

    if (lpitem->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT mis;
        mis.CtlType    = ODT_MENU;
        mis.itemID     = lpitem->wID;
        mis.itemData   = (DWORD)lpitem->text;
        mis.itemHeight = 16;
        mis.itemWidth  = 30;
        SendMessageW( hwndOwner, WM_MEASUREITEM, 0, (LPARAM)&mis );
        lpitem->rect.bottom += mis.itemHeight;
        lpitem->rect.right  += mis.itemWidth;
//        DPRINT( "%08x %dx%d\n",
//                     lpitem->wID, mis.itemWidth, mis.itemHeight);
        return;
    } 

    if (lpitem->fType & MF_SEPARATOR)
    {
	lpitem->rect.bottom += SEPARATOR_HEIGHT;
	return;
    }

    if (!menuBar)
    {
	lpitem->rect.right += 2 * check_bitmap_width;
	if (lpitem->fType & MF_POPUP)
	    lpitem->rect.right += arrow_bitmap_width;
    }

    if (lpitem->fType & MF_BITMAP)
    {
	BITMAP bm;
        if (GetObjectA( (HBITMAP)lpitem->text, sizeof(bm), &bm ))
        {
            lpitem->rect.right  += bm.bmWidth;
            lpitem->rect.bottom += bm.bmHeight;
        }
	return;
    }
    
    /* If we get here, then it must be a text item */

    if (IS_STRING_ITEM( lpitem->fType ))
    {
        GetTextExtentPointW( hdc, lpitem->text, wcslen(lpitem->text), &szSize  );
        lpitem->rect.right  += LOWORD(szSize.cx);
	if (TWEAK_WineLook == WIN31_LOOK)
            lpitem->rect.bottom += max( HIWORD(szSize.cy), SYSMETRICS_CYMENU );
        else
            lpitem->rect.bottom += max(HIWORD(szSize.cy), sysMetrics[SM_CYMENU]- 1);
        lpitem->xTab = 0;

        if (menuBar) lpitem->rect.right += MENU_BAR_ITEMS_SPACE;
        else if ((p = wcschr( lpitem->text, '\t' )) != NULL)
        {
            /* Item contains a tab (only meaningful in popup menus) */
            lpitem->xTab = check_bitmap_width + MENU_TAB_SPACE;
 	    GetTextExtentPoint32W( hdc, lpitem->text,(int)(p - lpitem->text) , &szSize);
	    lpitem->xTab += szSize.cx;
            lpitem->rect.right += MENU_TAB_SPACE;
        }
        else
        {
            if (wcschr( lpitem->text, '\b' ))
                lpitem->rect.right += MENU_TAB_SPACE;
            lpitem->xTab = lpitem->rect.right - check_bitmap_width 
                           - arrow_bitmap_width;
        }
    }
}


/***********************************************************************
 *           MENU_PopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
void MENU_PopupMenuCalcSize( LPPOPUPMENU lppop, HWND hwndOwner )
{
    MENUITEM *lpitem;
    HDC hdc;
    int start, i;
    int orgX, orgY, maxX, maxTab, maxTabWidth;

    lppop->Width = lppop->Height = 0;
    if (lppop->nItems == 0) return;
    hdc = GetDC( 0 );
    start = 0;
    maxX = SYSMETRICS_CXBORDER;
    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = maxX;
	orgY = SYSMETRICS_CYBORDER;

	maxTab = maxTabWidth = 0;

	  /* Parse items until column break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	   
	    if (TWEAK_WineLook > WIN31_LOOK)
		++orgY;

	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, FALSE );
            if (lpitem->fType & MF_MENUBARBREAK) orgX++;
	    maxX = max( maxX, lpitem->rect.right );
	    orgY = lpitem->rect.bottom;
	    if (IS_STRING_ITEM(lpitem->fType) && lpitem->xTab)
	    {
		maxTab = max( maxTab, lpitem->xTab );
		maxTabWidth = max(maxTabWidth,lpitem->rect.right-lpitem->xTab);
	    }
	}

	  /* Finish the column (set all items to the largest width found) */
	maxX = max( maxX, maxTab + maxTabWidth );
	for (lpitem = &lppop->items[start]; start < i; start++, lpitem++)
	{
	    lpitem->rect.right = maxX;
	    if (IS_STRING_ITEM(lpitem->fType) && lpitem->xTab)
                lpitem->xTab = maxTab;
	}
	lppop->Height = max( lppop->Height, orgY );
    }

    
    if(TWEAK_WineLook > WIN31_LOOK)
	lppop->Height++;

    lppop->Width  = maxX;
    ReleaseDC( 0, hdc );
}


/***********************************************************************
 *           MENU_MenuBarCalcSize
 *
 * FIXME: Word 6 implements its own MDI and its own 'close window' bitmap
 * height is off by 1 pixel which causes lengthy window relocations when
 * active document window is maximized/restored.
 *
 * Calculate the size of the menu bar.
 */
static void MENU_MenuBarCalcSize( HDC hdc, LPRECT lprect,
                                  LPPOPUPMENU lppop, HWND hwndOwner )
{
    MENUITEM *lpitem;
    int start, i, orgX, orgY, maxY, helpPos;

    if ((lprect == NULL) || (lppop == NULL)) return;
    if (lppop->nItems == 0) return;
    DPRINT("left=%d top=%d right=%d bottom=%d\n", 
                 lprect->left, lprect->top, lprect->right, lprect->bottom);
    lppop->Width  = lprect->right - lprect->left;
    lppop->Height = 0;
    maxY = lprect->top;
    start = 0;
    helpPos = -1;
    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = lprect->left;
	orgY = maxY;

	  /* Parse items until line break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((helpPos == -1) && (lpitem->fType & MF_HELP)) helpPos = i;
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	    DPRINT( "calling MENU_CalcItemSize org=(%d, %d)\n", orgX, orgY );
//	    debug_print_menuitem ("  item: ", lpitem, "");
	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, TRUE );
	    if (lpitem->rect.right > lprect->right)
	    {
		if (i != start) break;
		else lpitem->rect.right = lprect->right;
	    }
	    maxY = MAX( maxY, lpitem->rect.bottom );
	    orgX = lpitem->rect.right;
	}

	  /* Finish the line (set all items to the largest height found) */
	while (start < i) lppop->items[start++].rect.bottom = maxY;
    }

    lprect->bottom = maxY;
    lppop->Height = lprect->bottom - lprect->top;

      /* Flush right all items between the MF_HELP and the last item */
      /* (if several lines, only move the last line) */
    if (helpPos != -1)
    {
	lpitem = &lppop->items[lppop->nItems-1];
	orgY = lpitem->rect.top;
	orgX = lprect->right;
	for (i = lppop->nItems - 1; i >= helpPos; i--, lpitem--)
	{
	    if (lpitem->rect.top != orgY) break;    /* Other line */
	    if (lpitem->rect.right >= orgX) break;  /* Too far right already */
	    lpitem->rect.left += orgX - lpitem->rect.right;
	    lpitem->rect.right = orgX;
	    orgX = lpitem->rect.left;
	}
    }
}

/***********************************************************************
 *           MENU_DrawMenuItem
 *
 * Draw a single menu item.
 */
 void MENU_DrawMenuItem( HWND hwnd, HDC hdc, MENUITEM *lpitem,
			       UINT height, WINBOOL menuBar, UINT odaction )
{
    RECT rect;

    //debug_print_menuitem("MENU_DrawMenuItem: ", lpitem, "");

    if (lpitem->fType & MF_SYSMENU)
    {
	if( !IsIconic(hwnd) ) {
	    if (TWEAK_WineLook > WIN31_LOOK)
		NC_DrawSysButton95( hwnd, hdc,
				    lpitem->fState &
				    (MF_HILITE | MF_MOUSESELECT) );
	    else
		NC_DrawSysButton( hwnd, hdc, 
				  lpitem->fState &
				  (MF_HILITE | MF_MOUSESELECT) );
	}

	return;
    }

    if (lpitem->fType & MF_OWNERDRAW)
    {
        DRAWITEMSTRUCT dis;

        dis.CtlType   = ODT_MENU;
        dis.itemID    = lpitem->wID;
        dis.itemData  = (DWORD)lpitem->text;
        dis.itemState = 0;
        if (lpitem->fState & MF_CHECKED) dis.itemState |= ODS_CHECKED;
        if (lpitem->fState & MF_GRAYED)  dis.itemState |= ODS_GRAYED;
        if (lpitem->fState & MF_HILITE)  dis.itemState |= ODS_SELECTED;
        dis.itemAction = odaction; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
        dis.hwndItem   = hwnd;
        dis.hDC        = hdc;
        dis.rcItem     = lpitem->rect;
        //DPRINT( "Ownerdraw: itemID=%d, itemState=%d, itemAction=%d, "
	//      "hwndItem=%04x, hdc=%04x, rcItem={%d,%d,%d,%d}\n",dis.itemID,
	//      dis.itemState, dis.itemAction, dis.hwndItem, dis.hDC,
	//      dis.rcItem.left, dis.rcItem.top, dis.rcItem.right,
	//      dis.rcItem.bottom );
        SendMessageW( GetWindow(hwnd,GW_OWNER), WM_DRAWITEM, 0, (LPARAM)&dis );
        return;
    }

    if (menuBar && (lpitem->fType & MF_SEPARATOR)) return;
    rect = lpitem->rect;

    /* Draw the background */
    if (TWEAK_WineLook > WIN31_LOOK) {
	rect.left += 2;
	rect.right -= 2;

        /*
	if(menuBar) {
	    --rect.left;
	    ++rect.bottom;
	    --rect.top;
	}
	InflateRect( &rect, -1, -1 );
	*/
    }

    if (lpitem->fState & MF_HILITE)
	FillRect( hdc, &rect, GetSysColorBrush(COLOR_HIGHLIGHT) );
    else
	FillRect( hdc, &rect, GetSysColorBrush(COLOR_MENU) );

    SetBkMode( hdc, TRANSPARENT );

      /* Draw the separator bar (if any) */

    if (!menuBar && (lpitem->fType & MF_MENUBARBREAK))
    {
	/* vertical separator */
	if (TWEAK_WineLook > WIN31_LOOK) {
	    RECT rc = rect;
	    rc.top = 3;
	    rc.bottom = height - 3;
	    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_LEFT);
	}
	else {
	    SelectObject( hdc,(HGDIOBJ) GetSysColorPen(COLOR_WINDOWFRAME) );
	    MoveToEx( hdc, rect.left, 0,NULL );
	    LineTo( hdc, rect.left, height );
	}
    }
    if (lpitem->fType & MF_SEPARATOR)
    {
	/* horizontal separator */
	if (TWEAK_WineLook > WIN31_LOOK) {
	    RECT rc = rect;
	    rc.left++;
	    rc.right--;
	    rc.top += SEPARATOR_HEIGHT / 2;
	    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_TOP);
	}
	else {
	    SelectObject( hdc,(HGDIOBJ) GetSysColorPen(COLOR_WINDOWFRAME) );
	    MoveToEx( hdc, rect.left, rect.top + SEPARATOR_HEIGHT/2 ,NULL);
	    LineTo( hdc, rect.right, rect.top + SEPARATOR_HEIGHT/2 );
	}

	return;
    }

      /* Setup colors */

    if (lpitem->fState & MF_HILITE)
    {
	if (lpitem->fState & MF_GRAYED)
	    SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
	else
	    SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
	SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
    }
    else
    {
	if (lpitem->fState & MF_GRAYED)
	    SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
	else
	    SetTextColor( hdc, GetSysColor( COLOR_MENUTEXT ) );
	SetBkColor( hdc, GetSysColor( COLOR_MENU ) );
    }

    if (!menuBar)
    {
	INT	y = rect.top + rect.bottom;

	  /* Draw the check mark
	   *
	   * FIXME:
	   * Custom checkmark bitmaps are monochrome but not always 1bpp. 
	   */

	if (lpitem->fState & MF_CHECKED)
	{
	    HBITMAP bm =
		 lpitem->hCheckBit ? lpitem->hCheckBit :
		 ((lpitem->fType & MFT_RADIOCHECK)
		  ? hStdRadioCheck : hStdCheck);
	    HDC hdcMem = CreateCompatibleDC( hdc );

	    SelectObject( hdcMem, bm );
            BitBlt( hdc, rect.left, (y - check_bitmap_height) / 2,
		      check_bitmap_width, check_bitmap_height,
		      hdcMem, 0, 0, SRCCOPY );
	    DeleteDC( hdcMem );
        } else if (lpitem->hUnCheckBit) {
	    HDC hdcMem = CreateCompatibleDC( hdc );

	    SelectObject( hdcMem, lpitem->hUnCheckBit );
            BitBlt( hdc, rect.left, (y - check_bitmap_height) / 2,
		      check_bitmap_width, check_bitmap_height,
		      hdcMem, 0, 0, SRCCOPY );
	    DeleteDC( hdcMem );
	}

	  /* Draw the popup-menu arrow */

	if (lpitem->fType & MF_POPUP)
	{
	    HDC hdcMem = CreateCompatibleDC( hdc );

	    SelectObject( hdcMem, hStdMnArrow );
            BitBlt( hdc, rect.right - arrow_bitmap_width - 1,
		      (y - arrow_bitmap_height) / 2,
		      arrow_bitmap_width, arrow_bitmap_height,
		      hdcMem, 0, 0, SRCCOPY );
	    DeleteDC( hdcMem );
	}

	rect.left += check_bitmap_width;
	rect.right -= arrow_bitmap_width;
    }

      /* Draw the item text or bitmap */

    if (lpitem->fType & MF_BITMAP)
    {
        HDC hdcMem = CreateCompatibleDC( hdc );

	SelectObject( hdcMem, (HBITMAP)lpitem->text );
	BitBlt( hdc, rect.left, rect.top, rect.right - rect.left,
		  rect.bottom - rect.top, hdcMem, 0, 0, SRCCOPY );
	DeleteDC( hdcMem );
	return;
    }
    /* No bitmap - process text if present */
    else if (IS_STRING_ITEM(lpitem->fType))
    {
	register int i;

	if (menuBar)
	{
	    rect.left += MENU_BAR_ITEMS_SPACE / 2;
	    rect.right -= MENU_BAR_ITEMS_SPACE / 2;
	    i = wcslen( lpitem->text );
	}
	else
	{
	    for (i = 0; lpitem->text[i]; i++)
                if ((lpitem->text[i] == '\t') || (lpitem->text[i] == '\b'))
                    break;
	}

	if((TWEAK_WineLook == WIN31_LOOK) || !(lpitem->fState & MF_GRAYED)) {
	    DrawTextW( hdc, lpitem->text, i, &rect,
			 DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}

	else {
	    if (!(lpitem->fState & MF_HILITE))
            {
		++rect.left;
		++rect.top;
		++rect.right;
		++rect.bottom;
		SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
		DrawTextW( hdc, lpitem->text, i, &rect,
			     DT_LEFT | DT_VCENTER | DT_SINGLELINE );
		--rect.left;
		--rect.top;
		--rect.right;
		--rect.bottom;
	    }
	    SetTextColor(hdc, RGB(0x80, 0x80, 0x80));
	    DrawTextW( hdc, lpitem->text, i, &rect,
			 DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}

	if (lpitem->text[i])  /* There's a tab or flush-right char */
	{
	    if (lpitem->text[i] == '\t')
	    {
		rect.left = lpitem->xTab;
		DrawTextW( hdc, lpitem->text + i + 1, -1, &rect,
                             DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	    }
	    else DrawTextW( hdc, lpitem->text + i + 1, -1, &rect,
                              DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
	}
    }
}

/***********************************************************************
 *           MENU_DrawPopupMenu
 *
 * Paint a popup menu.
 */
void MENU_DrawPopupMenu( HWND hwnd, HDC hdc, HMENU hmenu )
{
    HBRUSH hPrevBrush = 0;
    RECT rect;

    GetClientRect( hwnd, &rect );


    rect.bottom -= POPUP_YSHADE * SYSMETRICS_CYBORDER;
    rect.right -= POPUP_XSHADE * SYSMETRICS_CXBORDER;


    if((hPrevBrush = SelectObject( hdc, GetSysColorBrush(COLOR_MENU) )))
    {
	HPEN hPrevPen;
	
	Rectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );

	hPrevPen = SelectObject( hdc, GetStockObject( NULL_PEN ) );
	if( hPrevPen )
	{
	    POPUPMENU *menu;

	    INT ropPrev, i;
	   

	    /* draw 3-d shade */
	    if(TWEAK_WineLook == WIN31_LOOK) {
		SelectObject( hdc, hShadeBrush );
		SetBkMode( hdc, TRANSPARENT );
		ropPrev = SetROP2( hdc, R2_MASKPEN );

		i = rect.right;		/* why SetBrushOrg() doesn't? */
		PatBlt( hdc, i & 0xfffffffe,
			  rect.top + POPUP_YSHADE*SYSMETRICS_CYBORDER, 
			  i%2 + POPUP_XSHADE*SYSMETRICS_CXBORDER,
			  rect.bottom - rect.top, 0x00a000c9 );
		i = rect.bottom;
		PatBlt( hdc, rect.left + POPUP_XSHADE*SYSMETRICS_CXBORDER,
			  i & 0xfffffffe,rect.right - rect.left,
			  i%2 + POPUP_YSHADE*SYSMETRICS_CYBORDER, 0x00a000c9 );
		SelectObject( hdc, hPrevPen );
		SelectObject( hdc, hPrevBrush );
		SetROP2( hdc, ropPrev );
	    }
	    else
		DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT);

	    /* draw menu items */

	    menu = (POPUPMENU *) ( hmenu );
	    if (menu && menu->nItems)
	    {
		MENUITEM *item;
		UINT u;

		for (u = menu->nItems, item = menu->items; u > 0; u--, item++)
		    MENU_DrawMenuItem( hwnd, hdc, item, menu->Height, FALSE,
				       ODA_DRAWENTIRE );

	    }
	} else SelectObject( hdc, hPrevBrush );
    }
}


/***********************************************************************
 *           MENU_DrawMenuBar
 *
 * Paint a menu bar. Returns the height of the menu bar.
 */
UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect, HWND hwnd,
                         WINBOOL suppress_draw)
{
    LPPOPUPMENU lppop;
    UINT i;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    lppop = (LPPOPUPMENU) ( (HMENU)wndPtr->wIDmenu );
    if (lppop == NULL || lprect == NULL) return SYSMETRICS_CYMENU;
//    DPRINT("(%04x, %p, %p); !\n",  hDC, lprect, lppop);
    if (lppop->Height == 0) MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);
    lprect->bottom = lprect->top + lppop->Height;
    if (suppress_draw) return lppop->Height;
    
    FillRect(hDC, lprect, GetSysColorBrush(COLOR_MENU) );

    if (TWEAK_WineLook == WIN31_LOOK) {
	SelectObject( hDC,(HGDIOBJ)GetSysColorPen(COLOR_WINDOWFRAME) );
	MoveToEx( hDC, lprect->left, lprect->bottom,NULL );
	LineTo( hDC, lprect->right, lprect->bottom );
    }
    else {
	SelectObject( hDC,(HGDIOBJ) GetSysColorPen(COLOR_3DFACE));
	MoveToEx( hDC, lprect->left, lprect->bottom,NULL );
	LineTo( hDC, lprect->right, lprect->bottom );
    }

    if (lppop->nItems == 0) return SYSMETRICS_CYMENU;
    for (i = 0; i < lppop->nItems; i++)
    {
	MENU_DrawMenuItem( hwnd, hDC, &lppop->items[i], lppop->Height, TRUE,
			   ODA_DRAWENTIRE );
    }
    return lppop->Height;
} 


/***********************************************************************
 *	     MENU_PatchResidentPopup
 */
WINBOOL MENU_PatchResidentPopup( HQUEUE checkQueue, WND* checkWnd )
{
 
    return FALSE;
}


/***********************************************************************
 *           MENU_ShowPopup
 *
 * Display a popup menu.
 */
 WINBOOL MENU_ShowPopup( HWND hwndOwner, HMENU hmenu, UINT id,
                              INT x, INT y, INT xanchor, INT yanchor )
{
    POPUPMENU 	*menu;
    WND 	*wndOwner = NULL;

    if (!(menu = (POPUPMENU *) ( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
	menu->items[menu->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
	menu->FocusedItem = NO_SELECTED_ITEM;
    }

    if( (wndOwner = WIN_FindWndPtr( hwndOwner )) )
    {
	UINT	width, height;

	MENU_PopupMenuCalcSize( menu, hwndOwner );

	/* adjust popup menu pos so that it fits within the desktop */

	width = menu->Width + SYSMETRICS_CXBORDER;
	height = menu->Height + SYSMETRICS_CYBORDER; 

	if( x + width > SYSMETRICS_CXSCREEN )
	{
	    if( xanchor )
            	x -= width - xanchor;
            if( x + width > SYSMETRICS_CXSCREEN)
	    	x = SYSMETRICS_CXSCREEN - width;
	}
	if( x < 0 ) x = 0;

	if( y + height > SYSMETRICS_CYSCREEN )
	{ 
	    if( yanchor )
	    	y -= height + yanchor;
	    if( y + height > SYSMETRICS_CYSCREEN )
	    	y = SYSMETRICS_CYSCREEN - height;
	}
	if( y < 0 ) y = 0;

	width += POPUP_XSHADE * SYSMETRICS_CXBORDER;	/* add space for shading */
	height += POPUP_YSHADE * SYSMETRICS_CYBORDER;

	/* NOTE: In Windows, top menu popup is not owned. */
	if (!pTopPopupWnd)	/* create top level popup menu window */
	{
//	    assert( uSubPWndLevel == 0 );

	    pTopPopupWnd = WIN_FindWndPtr(CreateWindowA( POPUPMENU_CLASS_ATOM, NULL,
					  WS_POPUP, x, y, width, height,
					  hwndOwner, 0, wndOwner->hInstance,
					  (LPVOID)hmenu ));
	    if (!pTopPopupWnd) return FALSE;
	    menu->hWnd = pTopPopupWnd->hwndSelf;
	} 
	else
	    if( uSubPWndLevel )
	    {
		/* create a new window for the submenu */

		menu->hWnd = CreateWindowA( POPUPMENU_CLASS_ATOM, NULL,
					  WS_POPUP, x, y, width, height,
					  menu->hWnd, 0, wndOwner->hInstance,
					  (LPVOID)hmenu );
		if( !menu->hWnd ) return FALSE;
	    }
	    else /* top level popup menu window already exists */
	    {
		menu->hWnd = pTopPopupWnd->hwndSelf;
		MENU_PatchResidentPopup( 0, wndOwner );
		SendMessageW( pTopPopupWnd->hwndSelf, MM_SETMENUHANDLE, (WPARAM)hmenu, 0L);	

		/* adjust its size */

	        SetWindowPos( menu->hWnd, 0, x, y, width, height,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
	    }

	uSubPWndLevel++;	/* menu level counter */

      /* Display the window */

	SetWindowPos( menu->hWnd, HWND_TOP, 0, 0, 0, 0,
			SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
	UpdateWindow( menu->hWnd );
	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           MENU_SelectItem
 */
 void MENU_SelectItem( HWND hwndOwner, HMENU hmenu, UINT wIndex,
                             WINBOOL sendMenuSelect )
{
    LPPOPUPMENU lppop;
    HDC hdc;

    lppop = (POPUPMENU *) ( hmenu );
    if (!lppop->nItems) return;

    if ((wIndex != NO_SELECTED_ITEM) && 
	(lppop->items[wIndex].fType & MF_SEPARATOR))
	wIndex = NO_SELECTED_ITEM;

    if (lppop->FocusedItem == wIndex) return;
    if (lppop->wFlags & MF_POPUP) hdc = GetDC( lppop->hWnd );
    else hdc = GetDCEx( lppop->hWnd, 0, DCX_CACHE | DCX_WINDOW);

      /* Clear previous highlighted item */
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	lppop->items[lppop->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
	MENU_DrawMenuItem(lppop->hWnd,hdc,&lppop->items[lppop->FocusedItem],
                          lppop->Height, !(lppop->wFlags & MF_POPUP),
			  ODA_SELECT );
    }

      /* Highlight new item (if any) */
    lppop->FocusedItem = wIndex;
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	lppop->items[lppop->FocusedItem].fState |= MF_HILITE;
	MENU_DrawMenuItem( lppop->hWnd, hdc, &lppop->items[lppop->FocusedItem],
                           lppop->Height, !(lppop->wFlags & MF_POPUP),
			   ODA_SELECT );
        if (sendMenuSelect)
        {
            MENUITEM *ip = &lppop->items[lppop->FocusedItem];
	    SendMessageW( hwndOwner, WM_MENUSELECT, ip->wID,
                           MAKELONG(ip->fType | (ip->fState | MF_MOUSESELECT),
                                    hmenu) );
        }
    }
    else if (sendMenuSelect) {
        SendMessageW( hwndOwner, WM_MENUSELECT, (WPARAM)hmenu,
                       (LPARAM)MAKELONG( lppop->wFlags | MF_MOUSESELECT, hmenu ) ); 
    }
    ReleaseDC( lppop->hWnd, hdc );
}


/***********************************************************************
 *           MENU_MoveSelection
 *
 * Moves currently selected item according to the offset parameter.
 * If there is no selection then it should select the last item if
 * offset is ITEM_PREV or the first item if offset is ITEM_NEXT.
 */
 void MENU_MoveSelection( HWND hwndOwner, HMENU hmenu, INT offset )
{
    INT i;
    POPUPMENU *menu;

    menu = (POPUPMENU *) ( hmenu );
    if (!menu->items) return;

    if ( menu->FocusedItem != NO_SELECTED_ITEM )
    {
	if( menu->nItems == 1 ) return; else
	for (i = menu->FocusedItem + offset ; i >= 0 && i < menu->nItems 
					    ; i += offset)
	    if (!(menu->items[i].fType & MF_SEPARATOR))
	    {
		MENU_SelectItem( hwndOwner, hmenu, i, TRUE );
		return;
	    }
    }

    for ( i = (offset > 0) ? 0 : menu->nItems - 1; 
		  i >= 0 && i < menu->nItems ; i += offset)
	if (!(menu->items[i].fType & MF_SEPARATOR))
	{
	    MENU_SelectItem( hwndOwner, hmenu, i, TRUE );
	    return;
	}
}


/**********************************************************************
 *         MENU_SetItemData
 *
 * Set an item flags, id and text ptr. Called by InsertMenu() and
 * ModifyMenu().
 */
 WINBOOL MENU_SetItemData( MENUITEM *item, UINT flags, UINT id,
                                LPCWSTR str )
{
    LPWSTR prevText = IS_STRING_ITEM(item->fType) ? item->text : NULL;

//    debug_print_menuitem("MENU_SetItemData from: ", item, "");

    if (IS_STRING_ITEM(flags))
    {
        if (!str || !*str)
        {
            flags |= MF_SEPARATOR;
            item->text = NULL;
        }
        else
        {
            LPWSTR text;
            /* Item beginning with a backspace is a help item */
            if (*str == '\b')
            {
                flags |= MF_HELP;
                str++;
            }
	    
            if ( ! (text = HEAP_strdupW(GetProcessHeap,0, str ) )) 
		return FALSE;
            item->text = text;
        }
    }
    else if (flags & MF_BITMAP) item->text = (LPWSTR)(HBITMAP)LOWORD(str);
    else item->text = NULL;

    if (flags & MF_OWNERDRAW) 
        item->dwItemData = (DWORD)str;
    else
        item->dwItemData = 0;

    if ((item->fType & MF_POPUP) && (flags & MF_POPUP) && (item->hSubMenu != id) )
	DestroyMenu( item->hSubMenu );   /* ModifyMenu() spec */

    if (flags & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *)((UINT)id);
        if (IS_A_MENU(menu)) menu->wFlags |= MF_POPUP;
	else
        {
            item->wID = 0;
            item->hSubMenu = 0;
            item->fType = 0;
            item->fState = 0;
	    return FALSE;
        }
    } 

    item->wID = id;
    if (flags & MF_POPUP)
      item->hSubMenu = id;

    if ((item->fType & MF_POPUP) && !(flags & MF_POPUP) )
      flags |= MF_POPUP; /* keep popup */

    item->fType = flags & TYPE_MASK;
    item->fState = (flags & STATE_MASK) &
        ~(MF_HILITE | MF_MOUSESELECT | MF_BYPOSITION);


    /* Don't call SetRectEmpty here! */


    if (prevText) HeapFree( GetProcessHeap(), 0, prevText );

//    debug_print_menuitem("MENU_SetItemData to  : ", item, "");
    return TRUE;
}


/**********************************************************************
 *         MENU_InsertItem
 *
 * Insert a new item into a menu.
 */
 MENUITEM *MENU_InsertItem( HMENU hMenu, UINT pos, UINT flags )
{
    MENUITEM *newItems;
    POPUPMENU *menu;

    if (!(menu = (POPUPMENU *)(hMenu))) 
    {
        DPRINT( "%04x not a menu handle\n",(UINT)hMenu );
        return NULL;
    }

    /* Find where to insert new item */

    if ((flags & MF_BYPOSITION) &&
        ((pos == (UINT)-1) || (pos == menu->nItems)))
    {
        /* Special case: append to menu */
        /* Some programs specify the menu length to do that */
        pos = menu->nItems;
    }
    else
    {
        if (!MENU_FindItem( &hMenu, &pos, flags )) 
        {
            DPRINT( "item %x not found\n",pos );
            return NULL;
        }
        if (!(menu = (LPPOPUPMENU) (hMenu)))
        {
            DPRINT("%04x not a menu handle\n",(UINT)hMenu);
            return NULL;
        }
    }

    /* Create new items array */

    newItems = HeapAlloc( GetProcessHeap(), 0, sizeof(MENUITEM) * (menu->nItems+1) );
    if (!newItems)
    {
        DPRINT( "allocation failed\n" );
        return NULL;
    }
    if (menu->nItems > 0)
    {
	  /* Copy the old array into the new */
	if (pos > 0) memcpy( newItems, menu->items, pos * sizeof(MENUITEM) );
	if (pos < menu->nItems) memcpy( &newItems[pos+1], &menu->items[pos],
					(menu->nItems-pos)*sizeof(MENUITEM) );
        HeapFree( GetProcessHeap(), 0, menu->items );
    }
    menu->items = newItems;
    menu->nItems++;
    memset( &newItems[pos], 0, sizeof(*newItems) );
    return &newItems[pos];
}


/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
LPCWSTR MENU_ParseResource( LPCWSTR res, HMENU hMenu)
{
    WORD flags, id = 0;
    LPWSTR str;

    do
    {
        flags = GET_WORD(res);
        res += sizeof(WORD);
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        if (!IS_STRING_ITEM(flags)) {
          //  DPRINT( "not a string item %04x\n", flags );
	}
        str = (LPWSTR)res;
        res += (lstrlenW((LPCWSTR)str) + 1) * sizeof(WCHAR);
        if (flags & MF_POPUP)
        {
            HMENU hSubMenu = CreatePopupMenu();
            if (!hSubMenu) return NULL;
            if (!(res = MENU_ParseResource( res, hSubMenu)))
                return NULL;
            AppendMenuW( hMenu, flags, (UINT)hSubMenu, (LPCWSTR)str );
        }
        else  /* Not a popup */
        {
           AppendMenuW( hMenu, flags, id,
                                *(LPCWSTR)str ? (LPCWSTR)str : NULL );
        }
    } while (!(flags & MF_END));
    return res;
}


/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
LPCWSTR MENUEX_ParseResource( LPCWSTR res, HMENU hMenu)
{
    WORD resinfo;
    do {
	MENUITEMINFO mii;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
	mii.fType = GET_DWORD(res);
        res += sizeof(DWORD);
	mii.fState = GET_DWORD(res);
        res += sizeof(DWORD);
	mii.wID = GET_DWORD(res);
        res += sizeof(DWORD);
	resinfo = GET_WORD(res); /* FIXME: for -bit apps this is a byte.  */
        res += sizeof(WORD);
	/* Align the text on a word boundary.  */
	res += (~((int)res - 1)) & 1;
	mii.dwTypeData = (LPWSTR) res;
	res += (1 + lstrlenW((LPWSTR)mii.dwTypeData)) * sizeof(WCHAR);
	/* Align the following fields on a dword boundary.  */
	res += (~((int)res - 1)) & 3;

        /* FIXME: This is inefficient and cannot be optimised away by gcc.  */
/*
	{
	    LPSTR newstr = HEAP_strdupWtoA(GetProcessHeap(),
					   0, mii.dwTypeData);
	    DPRINT( "Menu item: [%08x,%08x,%04x,%04x,%s]\n",
			 mii.fType, mii.fState, mii.wID, resinfo, newstr);
	    HeapFree( GetProcessHeap(), 0, newstr );
	}
*/
	if (resinfo & 1) {	/* Pop-up? */
	   // DWORD helpid = GET_DWORD(res); /* FIXME: use this.  */
	    res += sizeof(DWORD);
	    mii.hSubMenu = CreatePopupMenu();
	    if (!mii.hSubMenu)
		return NULL;
	    if (!(res = MENUEX_ParseResource(res, mii.hSubMenu))) {
		DestroyMenu(mii.hSubMenu);
                return NULL;
        }
	    mii.fMask |= MIIM_SUBMENU;
	    mii.fType |= MF_POPUP;
        }
	InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
    } while (!(resinfo & MF_END));
    return res;
}


/***********************************************************************
 *           MENU_GetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
 HMENU MENU_GetSubPopup( HMENU hmenu )
{
    POPUPMENU *menu;
    MENUITEM *item;

    menu = (POPUPMENU *) ( hmenu );

    if (menu->FocusedItem == NO_SELECTED_ITEM) return 0;

    item = &menu->items[menu->FocusedItem];
    if ((item->fType & MF_POPUP) && (item->fState & MF_MOUSESELECT))
        return item->hSubMenu;
    return 0;
}


/***********************************************************************
 *           MENU_HideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
 void MENU_HideSubPopups( HWND hwndOwner, HMENU hmenu,
                                WINBOOL sendMenuSelect )
{
    POPUPMENU *menu = (POPUPMENU*) ( hmenu );;

    if (menu && uSubPWndLevel)
    {
	HMENU hsubmenu;
	POPUPMENU *submenu;
	MENUITEM *item;

	if (menu->FocusedItem != NO_SELECTED_ITEM)
	{
	    item = &menu->items[menu->FocusedItem];
	    if (!(item->fType & MF_POPUP) ||
		!(item->fState & MF_MOUSESELECT)) return;
	    item->fState &= ~MF_MOUSESELECT;
	    hsubmenu = item->hSubMenu;
	} else return;

	submenu = (POPUPMENU *) ( hsubmenu );
	MENU_HideSubPopups( hwndOwner, hsubmenu, FALSE );
	MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, sendMenuSelect );

	if (submenu->hWnd == pTopPopupWnd->hwndSelf ) 
	{
	    ShowWindow( submenu->hWnd, SW_HIDE );
	    uSubPWndLevel = 0;
	}
	else
	{
	    DestroyWindow( submenu->hWnd );
	    submenu->hWnd = 0;
	}
    }
}


/***********************************************************************
 *           MENU_ShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or hmenu if no submenu to display.
 */
 HMENU MENU_ShowSubPopup( HWND hwndOwner, HMENU hmenu,
                                  WINBOOL selectFirst )
{
    RECT rect;
    POPUPMENU *menu;
    MENUITEM *item;
    WND *wndPtr;
    HDC hdc;

    if (!(menu = (POPUPMENU *) ( hmenu ))) return hmenu;

    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd )) ||
         (menu->FocusedItem == NO_SELECTED_ITEM)) return hmenu;

    item = &menu->items[menu->FocusedItem];
    if (!(item->fType & MF_POPUP) ||
         (item->fState & (MF_GRAYED | MF_DISABLED))) return hmenu;

    /* message must be send before using item,
       because nearly everything may by changed by the application ! */

    SendMessageW( hwndOwner, WM_INITMENUPOPUP, (WPARAM)item->hSubMenu,
		   MAKELONG( menu->FocusedItem, IS_SYSTEM_MENU(menu) ));

    item = &menu->items[menu->FocusedItem];
    rect = item->rect;

    /* correct item if modified as a reaction to WM_INITMENUPOPUP-message */
    if (!(item->fState & MF_HILITE)) 
    {
        if (menu->wFlags & MF_POPUP) hdc = GetDC( menu->hWnd );
        else hdc = GetDCEx( menu->hWnd, 0, DCX_CACHE | DCX_WINDOW);
        item->fState |= MF_HILITE;
        MENU_DrawMenuItem( menu->hWnd, hdc, item, menu->Height, !(menu->wFlags & MF_POPUP), ODA_DRAWENTIRE ); 
	ReleaseDC( menu->hWnd, hdc );
    }
    if (!item->rect.top && !item->rect.left && !item->rect.bottom && !item->rect.right)
      item->rect = rect;

    item->fState |= MF_MOUSESELECT;

    if (IS_SYSTEM_MENU(menu))
    {
	MENU_InitSysMenuPopup(item->hSubMenu, wndPtr->dwStyle, wndPtr->class->style);

	NC_GetSysPopupPos( wndPtr, &rect );
	rect.top = rect.bottom;
	rect.right = SYSMETRICS_CXSIZE;
        rect.bottom = SYSMETRICS_CYSIZE;
    }
    else
    {
	if (menu->wFlags & MF_POPUP)
	{
	    rect.left = wndPtr->rectWindow.left + item->rect.right-arrow_bitmap_width;
	    rect.top = wndPtr->rectWindow.top + item->rect.top;
	    rect.right = item->rect.left - item->rect.right + 2*arrow_bitmap_width;
	    rect.bottom = item->rect.top - item->rect.bottom;
	}
	else
	{
	    rect.left = wndPtr->rectWindow.left + item->rect.left;
	    rect.top = wndPtr->rectWindow.top + item->rect.bottom;
	    rect.right = item->rect.right - item->rect.left;
	    rect.bottom = item->rect.bottom - item->rect.top;
	}
    }

    MENU_ShowPopup( hwndOwner, item->hSubMenu, menu->FocusedItem,
		    rect.left, rect.top, rect.right, rect.bottom );
    if (selectFirst)
        MENU_MoveSelection( hwndOwner, item->hSubMenu, ITEM_NEXT );
    return item->hSubMenu;
}

/***********************************************************************
 *           MENU_PtMenu
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
 HMENU MENU_PtMenu( HMENU hMenu, POINT pt )
{
   POPUPMENU *menu = (POPUPMENU *) ( hMenu );
   register UINT ht = menu->FocusedItem;

   /* try subpopup first (if any) */
    ht = (ht != NO_SELECTED_ITEM &&
          (menu->items[ht].fType & MF_POPUP) &&
          (menu->items[ht].fState & MF_MOUSESELECT))
        ? (UINT) MENU_PtMenu(menu->items[ht].hSubMenu, pt) : 0;

   if( !ht )	/* check the current window (avoiding WM_HITTEST) */
   {
	ht = (UINT)NC_HandleNCHitTest( menu->hWnd, pt );
	if( menu->wFlags & MF_POPUP )
	    ht =  (ht != (UINT)HTNOWHERE && 
		   ht != (UINT)HTERROR) ? (UINT)hMenu : 0;
	else
	{
	    WND* wndPtr = WIN_FindWndPtr(menu->hWnd);

	    ht = ( ht == HTSYSMENU ) ? (UINT)(wndPtr->hSysMenu)
		 : ( ht == HTMENU ) ? (UINT)(wndPtr->wIDmenu) : 0;
	}
   }
   return (HMENU)ht;
}

/***********************************************************************
 *           MENU_ExecFocusedItem
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return TRUE if we can go on with menu tracking.
 */
 WINBOOL MENU_ExecFocusedItem( MTRACKER* pmt, HMENU hMenu )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) ( hMenu );
    if (!menu || !menu->nItems || 
	(menu->FocusedItem == NO_SELECTED_ITEM)) return TRUE;

    item = &menu->items[menu->FocusedItem];

//    DPRINT( "%08x %08x %08x\n",
//                 hMenu, item->wID, item->hSubMenu);

    if (!(item->fType & MF_POPUP))
    {
	if (!(item->fState & (MF_GRAYED | MF_DISABLED)))
	{
	    if( menu->wFlags & MF_SYSMENU )
	    {
		PostMessageA( pmt->hOwnerWnd, WM_SYSCOMMAND, item->wID,
			       MAKELPARAM((INT)pmt->pt.x, (INT)pmt->pt.y) );
	    }
	    else
		PostMessageA( pmt->hOwnerWnd, WM_COMMAND, item->wID, 0 );
	    return FALSE;
	}
	else return TRUE;
    }
    else
    {
	pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hMenu, TRUE );
	return TRUE;
    }
}


/***********************************************************************
 *           MENU_SwitchTracking
 *
 * Helper function for menu navigation routines.
 */
 void MENU_SwitchTracking( MTRACKER* pmt, HMENU hPtMenu, UINT id )
{
    POPUPMENU *ptmenu = (POPUPMENU *) ( hPtMenu );
    POPUPMENU *topmenu = (POPUPMENU *) ( pmt->hTopMenu );

    if( pmt->hTopMenu != hPtMenu &&
	!((ptmenu->wFlags | topmenu->wFlags) & MF_POPUP) )
    {
	/* both are top level menus (system and menu-bar) */

	MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE );
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE );
        pmt->hTopMenu = hPtMenu;
    }
    else MENU_HideSubPopups( pmt->hOwnerWnd, hPtMenu, FALSE );
    MENU_SelectItem( pmt->hOwnerWnd, hPtMenu, id, TRUE );
}


/***********************************************************************
 *           MENU_ButtonDown
 *
 * Return TRUE if we can go on with menu tracking.
 */
 WINBOOL MENU_ButtonDown( MTRACKER* pmt, HMENU hPtMenu )
{
    if (hPtMenu)
    {
	UINT id = 0;
	POPUPMENU *ptmenu = (POPUPMENU *) ( hPtMenu );
	MENUITEM *item;

	if( IS_SYSTEM_MENU(ptmenu) )
	    item = ptmenu->items;
	else
	    item = MENU_FindItemByCoords( ptmenu, pmt->pt, &id );

	if( item )
	{
	    if( ptmenu->FocusedItem == id )
	    {
		/* nothing to do with already selected non-popup */
		if( !(item->fType & MF_POPUP) ) return TRUE;

	        if( item->fState & MF_MOUSESELECT )
		{
		    if( ptmenu->wFlags & MF_POPUP )
		    {
			/* hide selected subpopup */

			MENU_HideSubPopups( pmt->hOwnerWnd, hPtMenu, TRUE );
			pmt->hCurrentMenu = hPtMenu;
			return TRUE;
		    }
		    return FALSE; /* shouldn't get here */
		} 
	    }
	    else MENU_SwitchTracking( pmt, hPtMenu, id );

	    /* try to display a subpopup */

	    pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hPtMenu, FALSE );
	    return TRUE;
	} 
	else DPRINT("\tunable to find clicked item!\n");
    }
    return FALSE;
}







/***********************************************************************
 *           MENU_TrackMenu
 *
 * Menu tracking code.
 */
 WINBOOL MENU_TrackMenu( HMENU hmenu, UINT wFlags, INT x, INT y,
                              HWND hwnd, const RECT *lprect )
{
    MSG msg;
    POPUPMENU *menu;
    WINBOOL fRemove;
    MTRACKER mt = { 0, hmenu, hmenu, hwnd, {x, y} };	/* control struct */

    fEndMenu = FALSE;
    if (!(menu = (POPUPMENU *) ( hmenu ))) return FALSE;

    if (wFlags & TPM_BUTTONDOWN) MENU_ButtonDown( &mt, hmenu );

    EVENT_Capture( mt.hOwnerWnd, HTMENU );

    while (!fEndMenu)
    {
	menu = (POPUPMENU *) ( mt.hCurrentMenu );
	msg.hwnd = (wFlags & TPM_ENTERIDLEEX && menu->wFlags & MF_POPUP) ? menu->hWnd : 0;

	/* we have to keep the message in the queue until it's
	 * clear that menu loop is not over yet. */

	if (!MSG_InternalGetMessage( &msg, msg.hwnd, mt.hOwnerWnd,
				     MSGF_MENU, PM_NOREMOVE, TRUE )) break;

        TranslateMessage( &msg );
        memcpy( &msg.pt, &mt.pt ,sizeof(POINT));

        fRemove = FALSE;
	if ((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
	{
	    /* Find a menu for this mouse event */

	    hmenu = MENU_PtMenu( mt.hTopMenu, msg.pt );

	    switch(msg.message)
	    {
		/* no WM_NC... messages in captured state */

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		    if (!(wFlags & TPM_RIGHTBUTTON)) break;
		    /* fall through */
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		    fEndMenu |= !MENU_ButtonDown( &mt, hmenu );
		    break;
		
		case WM_RBUTTONUP:
		    if (!(wFlags & TPM_RIGHTBUTTON)) break;
		    /* fall through */
		case WM_LBUTTONUP:
		    /* If outside all menus but inside lprect, ignore it */
		    if (hmenu || !lprect || !PtInRect(lprect, mt.pt))
		    {
			fEndMenu |= !MENU_ButtonUp( &mt, hmenu );
			fRemove = TRUE; 
		    }
		    break;
		
		case WM_MOUSEMOVE:
		    if ((msg.wParam & MK_LBUTTON) || ((wFlags & TPM_RIGHTBUTTON) 
						  && (msg.wParam & MK_RBUTTON)))
		    {
			fEndMenu |= !MENU_MouseMove( &mt, hmenu );
		    }
	    } /* switch(msg.message) - mouse */
	}
	else if ((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST))
	{
            fRemove = TRUE;  /* Keyboard messages are always removed */
	    switch(msg.message)
	    {
	    case WM_KEYDOWN:
		switch(msg.wParam)
		{
		case VK_HOME:
		case VK_END:
		    MENU_SelectItem( mt.hOwnerWnd, mt.hCurrentMenu, 
				     NO_SELECTED_ITEM, FALSE );
		/* fall through */
		case VK_UP:
		    MENU_MoveSelection( mt.hOwnerWnd, mt.hCurrentMenu, 
				       (msg.wParam == VK_HOME)? ITEM_NEXT : ITEM_PREV );
		    break;

		case VK_DOWN: /* If on menu bar, pull-down the menu */

		    menu = (POPUPMENU *) ( mt.hCurrentMenu );
		    if (!(menu->wFlags & MF_POPUP))
			mt.hCurrentMenu = MENU_ShowSubPopup( mt.hOwnerWnd, mt.hTopMenu, TRUE );
		    else      /* otherwise try to move selection */
			MENU_MoveSelection( mt.hOwnerWnd, mt.hCurrentMenu, ITEM_NEXT );
		    break;

		case VK_LEFT:
		    MENU_KeyLeft( &mt );
		    break;
		    
		case VK_RIGHT:
		    MENU_KeyRight( &mt );
		    break;
		    
		case VK_ESCAPE:
		    fEndMenu = TRUE;
		    break;

		default:
		    break;
		}
		break;  /* WM_KEYDOWN */

	    case WM_SYSKEYDOWN:
		switch(msg.wParam)
		{
		case VK_MENU:
		    fEndMenu = TRUE;
		    break;
		    
		}
		break;  /* WM_SYSKEYDOWN */

	    case WM_CHAR:
		{
		    UINT	pos;

		    if (msg.wParam == '\r' || msg.wParam == ' ')
		    {
			fEndMenu |= !MENU_ExecFocusedItem( &mt, mt.hCurrentMenu );
			break;
		    }

		      /* Hack to avoid control chars. */
		      /* We will find a better way real soon... */
		    if ((msg.wParam <= 32) || (msg.wParam >= 127)) break;

		    pos = MENU_FindItemByKey( mt.hOwnerWnd, mt.hCurrentMenu, 
							msg.wParam, FALSE );
		    if (pos == (UINT)-2) fEndMenu = TRUE;
		    else if (pos == (UINT)-1) MessageBeep(0);
		    else
		    {
			MENU_SelectItem( mt.hOwnerWnd, mt.hCurrentMenu, pos, TRUE );
			fEndMenu |= !MENU_ExecFocusedItem( &mt, mt.hCurrentMenu );
		    }
		}		    
		break;
	    }  /* switch(msg.message) - kbd */
	}
	else
	{
	    DispatchMessageA( &msg );
	}

	if (!fEndMenu) fRemove = TRUE;

	/* finally remove message from the queue */

        if (fRemove && !(mt.trackFlags & TF_SKIPREMOVE) )
	    PeekMessageA( &msg, 0, msg.message, msg.message, PM_REMOVE );
	else mt.trackFlags &= ~TF_SKIPREMOVE;
    }

    ReleaseCapture();
    if( IsWindow( mt.hOwnerWnd ) )
    {
	MENU_HideSubPopups( mt.hOwnerWnd, mt.hTopMenu, FALSE );

	menu = (POPUPMENU *) ( mt.hTopMenu );
	if (menu && menu->wFlags & MF_POPUP) 
	{
	    ShowWindow( menu->hWnd, SW_HIDE );
	    uSubPWndLevel = 0;
	}
	MENU_SelectItem( mt.hOwnerWnd, mt.hTopMenu, NO_SELECTED_ITEM, FALSE );
	SendMessageW( mt.hOwnerWnd, WM_MENUSELECT, 0, MAKELONG( 0xffff, 0 ) );
    }
    fEndMenu = FALSE;
    return TRUE;
}





/***********************************************************************
 *           MENU_ButtonUp
 *
 * Return TRUE if we can go on with menu tracking.
 */
 WINBOOL MENU_ButtonUp( MTRACKER* pmt, HMENU hPtMenu )
{
    if (hPtMenu)
    {
	UINT id = 0;
	POPUPMENU *ptmenu = (POPUPMENU *) ( hPtMenu );
	MENUITEM *item;

        if( IS_SYSTEM_MENU(ptmenu) )
            item = ptmenu->items;
        else
            item = MENU_FindItemByCoords( ptmenu, pmt->pt, &id );

	if( item && (ptmenu->FocusedItem == id ))
	{
	    if( !(item->fType & MF_POPUP) )
		return MENU_ExecFocusedItem( pmt, hPtMenu );
	    hPtMenu = item->hSubMenu;
	    if( hPtMenu == pmt->hCurrentMenu )
	    {
	        /* Select first item of sub-popup */    

	        MENU_SelectItem( pmt->hOwnerWnd, hPtMenu, NO_SELECTED_ITEM, FALSE );
	        MENU_MoveSelection( pmt->hOwnerWnd, hPtMenu, ITEM_NEXT );
	    }
	    return TRUE;
	}
    }
    return FALSE;
}


/***********************************************************************
 *           MENU_MouseMove
 *
 * Return TRUE if we can go on with menu tracking.
 */
 WINBOOL MENU_MouseMove( MTRACKER* pmt, HMENU hPtMenu )
{
    UINT id = NO_SELECTED_ITEM;
    POPUPMENU *ptmenu = NULL;

    if( hPtMenu )
    {
	ptmenu = (POPUPMENU *) ( hPtMenu );
        if( IS_SYSTEM_MENU(ptmenu) )
	    id = 0;
        else
            MENU_FindItemByCoords( ptmenu, pmt->pt, &id );
    } 

    if( id == NO_SELECTED_ITEM )
    {
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu, 
			 NO_SELECTED_ITEM, TRUE );
    }
    else if( ptmenu->FocusedItem != id )
    {
	    MENU_SwitchTracking( pmt, hPtMenu, id );
	    pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hPtMenu, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_DoNextMenu
 *
 * NOTE: WM_NEXTMENU documented in Win is a bit different.
 */
 LRESULT MENU_DoNextMenu( MTRACKER* pmt, UINT vk )
{
    POPUPMENU *menu = (POPUPMENU *) ( pmt->hTopMenu );

    if( (vk == VK_LEFT &&  menu->FocusedItem == 0 ) ||
        (vk == VK_RIGHT && menu->FocusedItem == menu->nItems - 1))
    {
	WND*    wndPtr;
	HMENU hNewMenu;
	HWND  hNewWnd;
	UINT  id = 0;
	MDINEXTMENU MdiNextMenu;
	LRESULT l;
	
	MdiNextMenu.hmenuIn = menu;
	if (IS_SYSTEM_MENU(menu)) 
		MdiNextMenu.hmenuNext = GetSubMenu(pmt->hTopMenu,0);
	else 
		MdiNextMenu.hmenuNext = pmt->hTopMenu;

	MdiNextMenu.hwndNext =  pmt->hOwnerWnd;

	l = SendMessageW( pmt->hOwnerWnd, WM_NEXTMENU, (WPARAM)vk, &MdiNextMenu);

	if( l == 0 )
	{
	    wndPtr = WIN_FindWndPtr(pmt->hOwnerWnd);

	    hNewWnd = pmt->hOwnerWnd;
	    if( IS_SYSTEM_MENU(menu) )
	    {
		/* switch to the menu bar */

		if( ((wndPtr->dwStyle & WS_CHILD) == WS_CHILD) || !wndPtr->wIDmenu ) 
		    return FALSE;

	        hNewMenu = (HMENU)wndPtr->wIDmenu;
	        if( vk == VK_LEFT )
	        {
		    menu = (POPUPMENU *) ( hNewMenu );
		    id = menu->nItems - 1;
	        }
	    }
	    else if( (( wndPtr->dwStyle & WS_SYSMENU) == WS_SYSMENU ) ) 
	    {
		/* switch to the system menu */
	        hNewMenu = wndPtr->hSysMenu; 
	    }
	    else return FALSE;
	}
	else    /* application returned a new menu to switch to */
	{
	    hNewMenu = MdiNextMenu.hmenuNext; 
	    hNewWnd = MdiNextMenu.hwndNext;

	    if( IsMenu(hNewMenu) && IsWindow(hNewWnd) )
	    {
		wndPtr = WIN_FindWndPtr(hNewWnd);

		if( ( (wndPtr->dwStyle & WS_SYSMENU) == WS_SYSMENU ) &&
		    GetSubMenu(wndPtr->hSysMenu, 0) == hNewMenu )
		{
	            /* get the real system menu */
		    hNewMenu =  wndPtr->hSysMenu;
		}
	        else if( ((wndPtr->dwStyle & WS_CHILD) == WS_CHILD) || (HANDLE)wndPtr->wIDmenu != hNewMenu )
		{
		    /* FIXME: Not sure what to do here, perhaps,
		     * try to track hNewMenu as a popup? */

		    //DPRINT(" -- got confused.\n");
		    return FALSE;
		}
	    }
	    else return FALSE;
	}

	if( hNewMenu != pmt->hTopMenu )
	{
	    MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE );
	    if( pmt->hCurrentMenu != pmt->hTopMenu ) 
		MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE );
	}

	if( hNewWnd != pmt->hOwnerWnd )
	{
	    ReleaseCapture(); 
	    pmt->hOwnerWnd = hNewWnd;
	    EVENT_Capture( pmt->hOwnerWnd, HTMENU );
	}

	pmt->hTopMenu = pmt->hCurrentMenu = hNewMenu; /* all subpopups are hidden */
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, id, TRUE ); 

	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           MENU_SuspendPopup
 *
 * The idea is not to show the popup if the next input message is
 * going to hide it anyway.
 */
WINBOOL MENU_SuspendPopup( MTRACKER* pmt, UINT uMsg )
{
    MSG msg;

    msg.hwnd = pmt->hOwnerWnd;

    PeekMessageA( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
    pmt->trackFlags |= TF_SKIPREMOVE;

    switch( uMsg )
    {
	case WM_KEYDOWN:
	     PeekMessageA( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
	     if( msg.message == WM_KEYUP || msg.message == WM_PAINT )
	     {
		 PeekMessageA( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
	         PeekMessageA( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
	         if( msg.message == WM_KEYDOWN &&
		    (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
	         {
		     pmt->trackFlags |= TF_SUSPENDPOPUP;
		     return TRUE;
	         }
	     }
	     break;
    }

    /* failures go through this */
    pmt->trackFlags &= ~TF_SUSPENDPOPUP;
    return FALSE;
}

/***********************************************************************
 *           MENU_KeyLeft
 *
 * Handle a VK_LEFT key event in a menu. 
 */
void MENU_KeyLeft( MTRACKER* pmt )
{
    POPUPMENU *menu;
    HMENU hmenutmp, hmenuprev;
    UINT  prevcol;

    hmenuprev = hmenutmp = pmt->hTopMenu;
    menu = (POPUPMENU *) ( hmenutmp );

    /* Try to move 1 column left (if possible) */
    if( (prevcol = MENU_GetStartOfPrevColumn( pmt->hCurrentMenu )) != 
	NO_SELECTED_ITEM ) {
	
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 prevcol, TRUE );
	return;
    }

    /* close topmost popup */
    while (hmenutmp != pmt->hCurrentMenu)
    {
	hmenuprev = hmenutmp;
	hmenutmp = MENU_GetSubPopup( hmenuprev );
    }

    MENU_HideSubPopups( pmt->hOwnerWnd, hmenuprev, TRUE );
    pmt->hCurrentMenu = hmenuprev; 

    if ( (hmenuprev == pmt->hTopMenu) && !(menu->wFlags & MF_POPUP) )
    {
	/* move menu bar selection if no more popups are left */

	if( !MENU_DoNextMenu( pmt, VK_LEFT) )
	     MENU_MoveSelection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_PREV );

	if ( hmenuprev != hmenutmp || pmt->trackFlags & TF_SUSPENDPOPUP )
	{
	   /* A sublevel menu was displayed - display the next one
	    * unless there is another displacement coming up */

	    if( !MENU_SuspendPopup( pmt, WM_KEYDOWN ) )
		pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd,
						pmt->hTopMenu, TRUE );
	}
    }
}


/***********************************************************************
 *           MENU_KeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 */
void MENU_KeyRight( MTRACKER* pmt )
{
    HMENU hmenutmp;
    POPUPMENU *menu = (POPUPMENU *) ( pmt->hTopMenu );
    UINT  nextcol;

//    DPRINT( "MENU_KeyRight called, cur %x (%s), top %x (%s).\n",
//		  pmt->hCurrentMenu,
//		  ((POPUPMENU *)(pmt->hCurrentMenu))->
//		     items[0].text,
//		  pmt->hTopMenu, menu->items[0].text );

    if ( (menu->wFlags & MF_POPUP) || (pmt->hCurrentMenu != pmt->hTopMenu))
    {
	/* If already displaying a popup, try to display sub-popup */

	hmenutmp = pmt->hCurrentMenu;
	pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hmenutmp, TRUE );

	/* if subpopup was displayed then we are done */
	if (hmenutmp != pmt->hCurrentMenu) return;
    }

    /* Check to see if there's another column */
    if( (nextcol = MENU_GetStartOfNextColumn( pmt->hCurrentMenu )) != 
	NO_SELECTED_ITEM ) {
//	DPRINT( "Going to %d.\n", nextcol );
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 nextcol, TRUE );
	return;
    }

    if (!(menu->wFlags & MF_POPUP))	/* menu bar tracking */
    {
	if( pmt->hCurrentMenu != pmt->hTopMenu )
	{
	    MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE );
	    hmenutmp = pmt->hCurrentMenu = pmt->hTopMenu;
	} else hmenutmp = 0;

	/* try to move to the next item */
	if( !MENU_DoNextMenu( pmt, VK_RIGHT) )
	     MENU_MoveSelection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_NEXT );

	if( hmenutmp || pmt->trackFlags & TF_SUSPENDPOPUP )
	    if( !MENU_SuspendPopup(pmt, WM_KEYDOWN) )
		pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, 
						       pmt->hTopMenu, TRUE );
    }
}




/***********************************************************************
 *           MENU_InitTracking
 */
WINBOOL MENU_InitTracking(HWND hWnd, HMENU hMenu)
{
    HideCaret(0);
    SendMessageW( hWnd, WM_ENTERMENULOOP, (WPARAM)0, (LPARAM)0 );
    SendMessageW( hWnd, WM_SETCURSOR, (WPARAM)hWnd, (LPARAM)HTCAPTION );
    SendMessageW( hWnd, WM_INITMENU, (WPARAM)hMenu, (LPARAM)0 );
    return TRUE;
}

/***********************************************************************
 *           MENU_TrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( WND* wndPtr, INT ht, POINT pt )
{
    HWND  hWnd = (HWND)(wndPtr->hwndSelf);
    HMENU hMenu = ((ht == (HTSYSMENU)) ? (HMENU)wndPtr->hSysMenu : (HMENU)wndPtr->wIDmenu);

    if (IsMenu(hMenu))
    {
	MENU_InitTracking( hWnd, hMenu );
	MENU_TrackMenu( hMenu, TPM_ENTERIDLEEX | TPM_BUTTONDOWN | 
		        TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, hWnd, NULL );

	SendMessageW( hWnd, WM_EXITMENULOOP,(WPARAM)0,(LPARAM)0 );
	ShowCaret(0);
    }
}




/***********************************************************************
 *           MENU_TrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
void MENU_TrackKbdMenuBar( WND* wndPtr, UINT wParam, INT vkey)
{
   UINT uItem = NO_SELECTED_ITEM;
   HMENU hTrackMenu; 

    /* find window that has a menu */
 
    while( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwStyle & WS_SYSMENU) )
	if( !(wndPtr = wndPtr->parent) ) return;

    /* check if we have to track a system menu */
          
    if( (wndPtr->dwStyle & (WS_CHILD | WS_MINIMIZE)) || 
	!wndPtr->wIDmenu || vkey == VK_SPACE )
    {
	if( !(wndPtr->dwStyle & WS_SYSMENU) ) return;
	hTrackMenu = wndPtr->hSysMenu;
	uItem = 0;
	wParam |= HTSYSMENU;	/* prevent item lookup */
    }
    else
	hTrackMenu = (HANDLE)wndPtr->wIDmenu;

    if (IsMenu( hTrackMenu ))
    {
	MENU_InitTracking( wndPtr->hwndSelf, hTrackMenu );

        if( vkey && vkey != VK_SPACE )
        {
            uItem = MENU_FindItemByKey( wndPtr->hwndSelf, hTrackMenu, 
					vkey, (wParam & HTSYSMENU) );
	    if( uItem >= (UINT)(-2) )
	    {
	        if( uItem == (UINT)(-1) ) MessageBeep(0);
		hTrackMenu = 0;
	    }
        }

	if( hTrackMenu )
	{
 	    MENU_SelectItem( wndPtr->hwndSelf, hTrackMenu, uItem, TRUE );

	    if( uItem == NO_SELECTED_ITEM )
		MENU_MoveSelection( wndPtr->hwndSelf, hTrackMenu, ITEM_NEXT );
	    else if( vkey )
		PostMessage( wndPtr->hwndSelf, WM_KEYDOWN, VK_DOWN, 0L );

	    MENU_TrackMenu( hTrackMenu, TPM_ENTERIDLEEX | TPM_LEFTALIGN | TPM_LEFTBUTTON,
			    0, 0, wndPtr->hwndSelf, NULL );
	}
        SendMessage( wndPtr->hwndSelf, WM_EXITMENULOOP, 0, 0 );
        ShowCaret(0);
    }
}

#if 0


/***********************************************************************
 *           MENU_TrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( WND* wndPtr, INT ht, POINT pt )
{
   UINT uItem = NO_SELECTED_ITEM;
   HMENU hTrackMenu; 

    /* find window that has a menu */
 
    while( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwStyle & WS_SYSMENU) )
	if( !(wndPtr = wndPtr->parent) ) return;

    /* check if we have to track a system menu */
          
    if( (wndPtr->dwStyle & (WS_CHILD | WS_MINIMIZE)) || 
	!wndPtr->wIDmenu || vkey == VK_SPACE )
    {
	if( !(wndPtr->dwStyle & WS_SYSMENU) ) return;
	hTrackMenu = wndPtr->hSysMenu;
	uItem = 0;
	wParam |= HTSYSMENU;	/* prevent item lookup */
    }
    else
	hTrackMenu = (HANDLE)wndPtr->wIDmenu;

    if (IsMenu( hTrackMenu ))
    {
	MENU_InitTracking( wndPtr->hwndSelf, hTrackMenu );

        if( vkey && vkey != VK_SPACE )
        {
            uItem = MENU_FindItemByKey( wndPtr->hwndSelf, hTrackMenu, 
					vkey, (wParam & HTSYSMENU) );
	    if( uItem >= (UINT)(-2) )
	    {
	        if( uItem == (UINT)(-1) ) MessageBeep(0);
		hTrackMenu = 0;
	    }
        }

	if( hTrackMenu )
	{
 	    MENU_SelectItem( wndPtr->hwndSelf, hTrackMenu, uItem, TRUE );

	    if( uItem == NO_SELECTED_ITEM )
		MENU_MoveSelection( wndPtr->hwndSelf, hTrackMenu, ITEM_NEXT );
	    else if( vkey )
		PostMessageA( wndPtr->hwndSelf, WM_KEYDOWN, VK_DOWN, 0L );

	    MENU_TrackMenu( hTrackMenu, TPM_ENTERIDLEEX | TPM_LEFTALIGN | TPM_LEFTBUTTON,
			    0, 0, wndPtr->hwndSelf, NULL );
	}
        SendMessageW( wndPtr->hwndSelf, WM_EXITMENULOOP, 0, 0 );
        ShowCaret(0);
    }
}

#endif