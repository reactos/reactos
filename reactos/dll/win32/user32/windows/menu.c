/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            user32/windows/menu.c
 * PURPOSE:         Menus
 *
 * PROGRAMMERS:     Casper S. Hornstrup
 *                  James Tabor
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

LRESULT DefWndNCPaint(HWND hWnd, HRGN hRgn, BOOL Active);

/* internal popup menu window messages */
#define MM_SETMENUHANDLE (WM_USER + 0)
#define MM_GETMENUHANDLE (WM_USER + 1)

/* Internal MenuTrackMenu() flags */
#define TPM_INTERNAL		0xF0000000
#define TPM_ENTERIDLEEX	 	0x80000000		/* set owner window for WM_ENTERIDLE */
#define TPM_BUTTONDOWN		0x40000000		/* menu was clicked before tracking */
#define TPM_POPUPMENU           0x20000000              /* menu is a popup menu */

/* TYPES *********************************************************************/

#define MENU_TYPE_MASK (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)

#define MENU_ITEM_TYPE(flags) ((flags) & MENU_TYPE_MASK)
#define IS_STRING_ITEM(flags) (MF_STRING == MENU_ITEM_TYPE(flags))
#define IS_BITMAP_ITEM(flags) (MF_BITMAP == MENU_ITEM_TYPE(flags))

#define IS_SYSTEM_MENU(MenuInfo)  \
	(0 == ((MenuInfo)->Flags & MF_POPUP) && 0 != ((MenuInfo)->Flags & MF_SYSMENU))

#define IS_SYSTEM_POPUP(MenuInfo) \
	(0 != ((MenuInfo)->Flags & MF_POPUP) && 0 != ((MenuInfo)->Flags & MF_SYSMENU))

#define IS_MAGIC_BITMAP(id) ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define MENU_ITEM_HBMP_SPACE (5)
#define MENU_BAR_ITEMS_SPACE (12)
#define SEPARATOR_HEIGHT (5)
#define MENU_TAB_SPACE (8)

#define ITEM_PREV		-1
#define ITEM_NEXT		 1

#define MAKEINTATOMA(atom)  ((LPCSTR)((ULONG_PTR)((WORD)(atom))))
#define MAKEINTATOMW(atom)  ((LPCWSTR)((ULONG_PTR)((WORD)(atom))))
#define POPUPMENU_CLASS_ATOMA   MAKEINTATOMA(32768)  /* PopupMenu */
#define POPUPMENU_CLASS_ATOMW   MAKEINTATOMW(32768)  /* PopupMenu */

/* internal flags for menu tracking */

#define TF_ENDMENU              0x0001
#define TF_SUSPENDPOPUP         0x0002
#define TF_SKIPREMOVE		0x0004

typedef struct
{
  UINT  TrackFlags;
  HMENU CurrentMenu; /* current submenu (can be equal to hTopMenu)*/
  HMENU TopMenu;     /* initial menu */
  HWND  OwnerWnd;    /* where notifications are sent */
  POINT Pt;
} MTRACKER;

static LRESULT WINAPI PopupMenuWndProcW(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*********************************************************************
 * PopupMenu class descriptor
 */
const struct builtin_class_descr POPUPMENU_builtin_class =
{
    POPUPMENU_CLASS_ATOMW,                     /* name */
    CS_SAVEBITS | CS_DBLCLKS,                  /* style  */
    (WNDPROC) NULL,                            /* FIXME - procA */
    (WNDPROC) PopupMenuWndProcW,               /* FIXME - procW */
    sizeof(MENUINFO *),                        /* extra */
    (LPCWSTR) IDC_ARROW,                       /* cursor */
    (HBRUSH)(COLOR_MENU + 1)                   /* brush */
};


/* INTERNAL FUNCTIONS ********************************************************/

/* Rip the fun and easy to use and fun WINE unicode string manipulation routines.
 * Of course I didnt copy the ASM code because we want this to be portable
 * and it needs to go away.
 */

#ifndef GET_WORD
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#endif
#ifndef GET_DWORD
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#endif

HFONT hMenuFont = NULL;
HFONT hMenuFontBold = NULL;

/* Flag set by EndMenu() to force an exit from menu tracking */
static BOOL fEndMenu = FALSE;

/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */
static HWND TopPopup;

/* Dimension of the menu bitmaps */
static HBITMAP BmpSysMenu = NULL;

static SIZE MenuCharSize;

/***********************************************************************
 *           MenuGetRosMenuInfo
 *
 * Get full information about menu
 */
static BOOL FASTCALL
MenuGetRosMenuInfo(PROSMENUINFO MenuInfo, HMENU Menu)
{
  MenuInfo->cbSize = sizeof(ROSMENUINFO);
  MenuInfo->fMask = MIM_BACKGROUND | MIM_HELPID | MIM_MAXHEIGHT | MIM_MENUDATA | MIM_STYLE;

  return NtUserMenuInfo(Menu, MenuInfo, FALSE);
}

/***********************************************************************
 *           MenuSetRosMenuInfo
 *
 * Set full information about menu
 */
static BOOL FASTCALL
MenuSetRosMenuInfo(PROSMENUINFO MenuInfo)
{
  MenuInfo->cbSize = sizeof(ROSMENUINFO);
  MenuInfo->fMask = MIM_BACKGROUND | MIM_HELPID | MIM_MAXHEIGHT | MIM_MENUDATA | MIM_STYLE;

  return NtUserMenuInfo(MenuInfo->Self, MenuInfo, TRUE);
}

/***********************************************************************
 *           MenuInitRosMenuItemInfo
 *
 * Initialize a buffer for use with MenuGet/SetRosMenuItemInfo
 */
static VOID FASTCALL
MenuInitRosMenuItemInfo(PROSMENUITEMINFO ItemInfo)
{
  ZeroMemory(ItemInfo, sizeof(ROSMENUITEMINFO));
  ItemInfo->cbSize = sizeof(ROSMENUITEMINFO);
}

/***********************************************************************
 *           MenuGetRosMenuItemInfo
 *
 * Get full information about a menu item
 */
static BOOL FASTCALL
MenuGetRosMenuItemInfo(HMENU Menu, UINT Index, PROSMENUITEMINFO ItemInfo)
{
  UINT Save_Mask = ItemInfo->fMask; /* Save the org mask bits. */

  if (ItemInfo->dwTypeData != NULL)
    {
      HeapFree(GetProcessHeap(), 0, ItemInfo->dwTypeData);
    }


  ItemInfo->fMask = MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA | MIIM_FTYPE
             | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU | MIIM_TYPE;
  ItemInfo->dwTypeData = NULL;

  if (! NtUserMenuItemInfo(Menu, Index, TRUE, ItemInfo, FALSE))
    {
      ItemInfo->fType = 0;
      return FALSE;
    }

  if (MENU_ITEM_TYPE(ItemInfo->fType) == MF_STRING)
    {
      ItemInfo->cch++;
      ItemInfo->dwTypeData = HeapAlloc(GetProcessHeap(), 0,
                                       ItemInfo->cch * sizeof(WCHAR));
      if (NULL == ItemInfo->dwTypeData)
        {
          return FALSE;
        }

      if (! NtUserMenuItemInfo(Menu, Index, TRUE, ItemInfo, FALSE))
        {
          ItemInfo->fType = 0;
          return FALSE;
        }
      ItemInfo->dwTypeData[ItemInfo->cch - 1] = UNICODE_NULL;
    }
  ItemInfo->fMask =  Save_Mask;
  return TRUE;
}

/***********************************************************************
 *           MenuSetRosMenuItemInfo
 *
 * Set selected information about a menu item, need to set the mask bits.
 */
static BOOL FASTCALL
MenuSetRosMenuItemInfo(HMENU Menu, UINT Index, PROSMENUITEMINFO ItemInfo)
{
  BOOL Ret;

  if (MENU_ITEM_TYPE(ItemInfo->fType) == MF_STRING &&
      ItemInfo->dwTypeData != NULL)
  {
    ItemInfo->cch = strlenW(ItemInfo->dwTypeData);
  }
  Ret = NtUserMenuItemInfo(Menu, Index, TRUE, ItemInfo, TRUE);

  return Ret;
}

/***********************************************************************
 *           MenuCleanupRosMenuItemInfo
 *
 * Cleanup after use of MenuGet/SetRosMenuItemInfo
 */
static VOID FASTCALL
MenuCleanupRosMenuItemInfo(PROSMENUITEMINFO ItemInfo)
{
  if (ItemInfo->dwTypeData != NULL)
    {
      HeapFree(GetProcessHeap(), 0, ItemInfo->dwTypeData);
      ItemInfo->dwTypeData = NULL;
    }
}

/***********************************************************************
 *           MenuGetAllRosMenuItemInfo
 *
 * Get full information about all menu items
 */
static INT FASTCALL
MenuGetAllRosMenuItemInfo(HMENU Menu, PROSMENUITEMINFO *ItemInfo)
{
  DWORD BufSize;

  BufSize = NtUserBuildMenuItemList(Menu, (VOID *) 1, 0, 0);
  if (BufSize == (DWORD) -1 || BufSize == 0)
    {
      return -1;
    }
  *ItemInfo = HeapAlloc(GetProcessHeap(), 0, BufSize);
  if (NULL == *ItemInfo)
    {
      return -1;
    }

  return NtUserBuildMenuItemList(Menu, *ItemInfo, BufSize, 0);
}

/***********************************************************************
 *           MenuCleanupAllRosMenuItemInfo
 *
 * Cleanup after use of MenuGetAllRosMenuItemInfo
 */
static VOID FASTCALL
MenuCleanupAllRosMenuItemInfo(PROSMENUITEMINFO ItemInfo)
{
  HeapFree(GetProcessHeap(), 0, ItemInfo);
}


/***********************************************************************
 *           MenuLoadBitmaps
 *
 * Load the arrow bitmap. We can't do this from MenuInit since user32
 * can also be used (and thus initialized) from text-mode.
 */
static void FASTCALL
MenuLoadBitmaps(VOID)
{
  /* Load system buttons bitmaps */
  if (NULL == BmpSysMenu)
    {
      BmpSysMenu = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
    }
}

/***********************************************************************
 *           MenuGetBitmapItemSize
 *
 * Get the size of a bitmap item.
 */
static void FASTCALL
MenuGetBitmapItemSize(PROSMENUITEMINFO lpitem, SIZE *Size, HWND WndOwner)
{
  BITMAP Bm;
  HBITMAP Bmp = lpitem->hbmpItem;

  Size->cx = Size->cy = 0;

  /* check if there is a magic menu item associated with this item */
  if (IS_MAGIC_BITMAP(Bmp))
    {
      switch((INT_PTR) Bmp)
        {
            case (INT_PTR)HBMMENU_CALLBACK:
            {
                MEASUREITEMSTRUCT measItem;
                measItem.CtlType = ODT_MENU;
                measItem.CtlID = 0;
                measItem.itemID = lpitem->wID;
                measItem.itemWidth = lpitem->Rect.right - lpitem->Rect.left;
                measItem.itemHeight = lpitem->Rect.bottom - lpitem->Rect.top;
                measItem.itemData = lpitem->dwItemData;
                SendMessageW( WndOwner, WM_MEASUREITEM, lpitem->wID, (LPARAM)&measItem);
                Size->cx = measItem.itemWidth;
                Size->cy = measItem.itemHeight;
                return;
            }
            break;

          case (INT_PTR) HBMMENU_SYSTEM:
            if (0 != lpitem->dwItemData)
              {
                Bmp = (HBITMAP) lpitem->dwItemData;
                break;
              }
            /* fall through */
          case (INT_PTR) HBMMENU_MBAR_RESTORE:
          case (INT_PTR) HBMMENU_MBAR_MINIMIZE:
          case (INT_PTR) HBMMENU_MBAR_CLOSE:
          case (INT_PTR) HBMMENU_MBAR_MINIMIZE_D:
          case (INT_PTR) HBMMENU_MBAR_CLOSE_D:
          case (INT_PTR) HBMMENU_POPUP_CLOSE:
          case (INT_PTR) HBMMENU_POPUP_RESTORE:
          case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
          case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
            /* FIXME: Why we need to subtract these magic values? */
            /* to make them smaller than the menu bar? */
            Size->cx = GetSystemMetrics(SM_CXSIZE) - 2;
            Size->cy = GetSystemMetrics(SM_CYSIZE) - 4;
            return;
        }
    }

  if (GetObjectW(Bmp, sizeof(BITMAP), &Bm))
    {
      Size->cx = Bm.bmWidth;
      Size->cy = Bm.bmHeight;
    }
}

/***********************************************************************
 *           MenuDrawPopupGlyph
 *
 * Draws popup magic glyphs (can be found in system menu).
 */
static void FASTCALL
MenuDrawPopupGlyph(HDC dc, LPRECT r, INT_PTR popupMagic, BOOL inactive, BOOL hilite)
{
  LOGFONTW lf;
  HFONT hFont, hOldFont;
  COLORREF clrsave;
  INT bkmode;
  TCHAR symbol;
  switch (popupMagic)
  {
  case (INT_PTR) HBMMENU_POPUP_RESTORE:
    symbol = '2';
    break;
  case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
    symbol = '0';
    break;
  case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
    symbol = '1';
    break;
  case (INT_PTR) HBMMENU_POPUP_CLOSE:
    symbol = 'r';
    break;
  default:
    ERR("Invalid popup magic bitmap %d\n", (int)popupMagic);
    return;
  }
  ZeroMemory(&lf, sizeof(LOGFONTW));
  InflateRect(r, -2, -2);
  lf.lfHeight = r->bottom - r->top;
  lf.lfWidth = 0;
  lf.lfWeight = FW_NORMAL;
  lf.lfCharSet = DEFAULT_CHARSET;
  lstrcpy(lf.lfFaceName, TEXT("Marlett"));
  hFont = CreateFontIndirect(&lf);
  /* save font and text color */
  hOldFont = SelectObject(dc, hFont);
  clrsave = GetTextColor(dc);
  bkmode = GetBkMode(dc);
  /* set color and drawing mode */
  SetBkMode(dc, TRANSPARENT);
  if (inactive)
  {
    /* draw shadow */
    if (!hilite)
    {
      SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
      TextOut(dc, r->left + 1, r->top + 1, &symbol, 1);
    }
  }
  SetTextColor(dc, GetSysColor(inactive ? COLOR_GRAYTEXT : (hilite ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)));
  /* draw selected symbol */
  TextOut(dc, r->left, r->top, &symbol, 1);
  /* restore previous settings */
  SetTextColor(dc, clrsave);
  SelectObject(dc, hOldFont);
  SetBkMode(dc, bkmode);
  DeleteObject(hFont);
}

/***********************************************************************
 *           MenuDrawBitmapItem
 *
 * Draw a bitmap item.
 */
static void FASTCALL
MenuDrawBitmapItem(HDC Dc, PROSMENUITEMINFO Item, const RECT *Rect,
                    HMENU hmenu, HWND WndOwner, UINT odaction, BOOL MenuBar)
{
  BITMAP Bm;
  DWORD Rop;
  HDC DcMem;
  HBITMAP Bmp;
  int w = Rect->right - Rect->left;
  int h = Rect->bottom - Rect->top;
  int BmpXoffset = 0;
  int Left, Top;
  HBITMAP hbmpToDraw = (HBITMAP) Item->hbmpItem;
  Bmp = hbmpToDraw;

  /* Check if there is a magic menu item associated with this item */
  if (IS_MAGIC_BITMAP(hbmpToDraw))
    {
      UINT Flags = 0;
      RECT r;

      r = *Rect;
      switch ((INT_PTR)hbmpToDraw)
        {
          case (INT_PTR) HBMMENU_SYSTEM:
            if (NULL != Item->dwTypeData)
              {
                Bmp = (HBITMAP)Item->dwTypeData;
                if (! GetObjectW(Bmp, sizeof(BITMAP), &Bm))
                  {
                    return;
                  }
              }
            else
              {
                if (!BmpSysMenu) BmpSysMenu = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
                Bmp = BmpSysMenu;
                if (! GetObjectW(Bmp, sizeof(BITMAP), &Bm))
                  {
                    return;
                  }
                /* only use right half of the bitmap */
                BmpXoffset = Bm.bmWidth / 2;
                Bm.bmWidth -= BmpXoffset;
              }
            goto got_bitmap;
          case (INT_PTR) HBMMENU_MBAR_RESTORE:
            Flags = DFCS_CAPTIONRESTORE;
            break;
          case (INT_PTR) HBMMENU_MBAR_MINIMIZE:
            r.right += 1;
            Flags = DFCS_CAPTIONMIN;
            break;
          case (INT_PTR) HBMMENU_MBAR_MINIMIZE_D:
            r.right += 1;
            Flags = DFCS_CAPTIONMIN | DFCS_INACTIVE;
            break;
          case (INT_PTR) HBMMENU_MBAR_CLOSE:
            Flags = DFCS_CAPTIONCLOSE;
            break;
          case (INT_PTR) HBMMENU_MBAR_CLOSE_D:
            Flags = DFCS_CAPTIONCLOSE | DFCS_INACTIVE;
            break;
          case (INT_PTR) HBMMENU_CALLBACK:
            {
                DRAWITEMSTRUCT drawItem;
                POINT origorg;
                drawItem.CtlType = ODT_MENU;
                drawItem.CtlID = 0;
                drawItem.itemID = Item->wID;
                drawItem.itemAction = odaction;
                drawItem.itemState = (Item->fState & MF_CHECKED)?ODS_CHECKED:0;
                drawItem.itemState |= (Item->fState & MF_DEFAULT)?ODS_DEFAULT:0;
                drawItem.itemState |= (Item->fState & MF_DISABLED)?ODS_DISABLED:0;
                drawItem.itemState |= (Item->fState & MF_GRAYED)?ODS_GRAYED|ODS_DISABLED:0;
                drawItem.itemState |= (Item->fState & MF_HILITE)?ODS_SELECTED:0;
                drawItem.hwndItem = (HWND)hmenu;
                drawItem.hDC = Dc;
                drawItem.rcItem = *Rect;
                drawItem.itemData = Item->dwItemData;
                /* some applications make this assumption on the DC's origin */
                SetViewportOrgEx( Dc, Item->Rect.left, Item->Rect.top, &origorg);
                OffsetRect( &drawItem.rcItem, - Item->Rect.left, - Item->Rect.top);
                SendMessageW( WndOwner, WM_DRAWITEM, 0, (LPARAM)&drawItem);
                SetViewportOrgEx( Dc, origorg.x, origorg.y, NULL);
                return;
            }
            break;

          case (INT_PTR) HBMMENU_POPUP_CLOSE:
          case (INT_PTR) HBMMENU_POPUP_RESTORE:
          case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
          case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
            MenuDrawPopupGlyph(Dc, &r, (INT_PTR)hbmpToDraw, Item->fState & MF_GRAYED, Item->fState & MF_HILITE);
            return;
        }
      InflateRect(&r, -1, -1);
      if (0 != (Item->fState & MF_HILITE))
        {
          Flags |= DFCS_PUSHED;
        }
      DrawFrameControl(Dc, &r, DFC_CAPTION, Flags);
      return;
    }

  if (NULL == Bmp || ! GetObjectW(Bmp, sizeof(BITMAP), &Bm))
    {
      return;
    }

got_bitmap:
  DcMem = CreateCompatibleDC(Dc);
  SelectObject(DcMem, Bmp);

  /* handle fontsize > bitmap_height */
  Top = (Bm.bmHeight < h) ? Rect->top + (h - Bm.bmHeight) / 2 : Rect->top;
  Left = Rect->left;
  Rop= ((Item->fState & MF_HILITE) && !IS_MAGIC_BITMAP(hbmpToDraw)) ? NOTSRCCOPY : SRCCOPY;
  if ((Item->fState & MF_HILITE) && Item->hbmpItem)
    {
      SetBkColor(Dc, GetSysColor(COLOR_HIGHLIGHT));
    }
  BitBlt(Dc, Left, Top, w, h, DcMem, BmpXoffset, 0, Rop);
  DeleteDC(DcMem);
}

/***********************************************************************
 *           MenuDrawMenuItem
 *
 * Draw a single menu item.
 */
static void FASTCALL
MenuDrawMenuItem(HWND hWnd, PROSMENUINFO MenuInfo, HWND WndOwner, HDC Dc,
                 PROSMENUITEMINFO Item, UINT Height, BOOL MenuBar, UINT Action)
{
  RECT Rect;
  PWCHAR Text;
  BOOL flat_menu = FALSE;
  int bkgnd;
  PWINDOW Wnd = ValidateHwnd(hWnd);

  if (!Wnd)
      return;

  if (0 != (Item->fType & MF_SYSMENU))
    {
      if ( (Wnd->Style & WS_MINIMIZE))
        {
          UserGetInsideRectNC(Wnd, &Rect);
          UserDrawSysMenuButton(hWnd, Dc, &Rect,
                                Item->fState & (MF_HILITE | MF_MOUSESELECT));
	}
      return;
    }

    SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);
    bkgnd = (MenuBar && flat_menu) ? COLOR_MENUBAR : COLOR_MENU;

  /* Setup colors */

  if (0 != (Item->fState & MF_HILITE))
    {
      if (MenuBar && !flat_menu)
        {
          SetTextColor(Dc, GetSysColor(COLOR_MENUTEXT));
          SetBkColor(Dc, GetSysColor(COLOR_MENU));
        }
      else
        {
          if (0 != (Item->fState & MF_GRAYED))
            {
              SetTextColor(Dc, GetSysColor(COLOR_GRAYTEXT));
            }
          else
            {
              SetTextColor(Dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            }
          SetBkColor(Dc, GetSysColor(COLOR_HIGHLIGHT));
        }
    }
  else
    {
      if (0 != (Item->fState & MF_GRAYED))
        {
          SetTextColor(Dc, GetSysColor(COLOR_GRAYTEXT));
        }
      else
        {
          SetTextColor(Dc, GetSysColor(COLOR_MENUTEXT));
        }
      SetBkColor(Dc, GetSysColor(bkgnd));
    }

  Rect = Item->Rect;

  if (Item->fType & MF_OWNERDRAW)
    {
      /*
      ** Experimentation under Windows reveals that an owner-drawn
      ** menu is given the rectangle which includes the space it requested
      ** in its response to WM_MEASUREITEM _plus_ width for a checkmark
      ** and a popup-menu arrow.  This is the value of lpitem->rect.
      ** Windows will leave all drawing to the application except for
      ** the popup-menu arrow.  Windows always draws that itself, after
      ** the menu owner has finished drawing.
      */
      DRAWITEMSTRUCT dis;

      dis.CtlType   = ODT_MENU;
      dis.CtlID     = 0;
      dis.itemID    = Item->wID;
      dis.itemData  = (DWORD)Item->dwItemData;
      dis.itemState = 0;
      if (0 != (Item->fState & MF_CHECKED))
        {
          dis.itemState |= ODS_CHECKED;
        }
      if (0 != (Item->fState & MF_GRAYED))
        {
          dis.itemState |= ODS_GRAYED | ODS_DISABLED;
        }
      if (0 != (Item->fState & MF_HILITE))
        {
          dis.itemState |= ODS_SELECTED;
        }
      dis.itemAction = Action; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
      dis.hwndItem   = (HWND) MenuInfo->Self;
      dis.hDC        = Dc;
      dis.rcItem     = Rect;
      TRACE("Ownerdraw: owner=%p itemID=%d, itemState=%d, itemAction=%d, "
	      "hwndItem=%p, hdc=%p, rcItem={%ld,%ld,%ld,%ld}\n", hWnd,
	      dis.itemID, dis.itemState, dis.itemAction, dis.hwndItem,
	      dis.hDC, dis.rcItem.left, dis.rcItem.top, dis.rcItem.right,
	      dis.rcItem.bottom);
      SendMessageW(WndOwner, WM_DRAWITEM, 0, (LPARAM) &dis);
      /* Draw the popup-menu arrow */
      if (0 != (Item->fType & MF_POPUP))
      {
           RECT rectTemp;
           CopyRect(&rectTemp, &Rect);
           rectTemp.left = rectTemp.right - GetSystemMetrics(SM_CXMENUCHECK);
           DrawFrameControl(Dc, &rectTemp, DFC_MENU, DFCS_MENUARROW);
      }
      return;
    }

  TRACE("rect={%ld,%ld,%ld,%ld}\n", Item->Rect.left, Item->Rect.top,
                                     Item->Rect.right, Item->Rect.bottom);

  if (MenuBar && 0 != (Item->fType & MF_SEPARATOR))
  {
     return;
  }

  if (Item->fState & MF_HILITE)
  {
    if (flat_menu)
    {
       InflateRect (&Rect, -1, -1);
       FillRect(Dc, &Rect, GetSysColorBrush(COLOR_MENUHILIGHT));
       InflateRect (&Rect, 1, 1);
       FrameRect(Dc, &Rect, GetSysColorBrush(COLOR_HIGHLIGHT));
    }
    else
    {
       if (MenuBar)
       {
          DrawEdge(Dc, &Rect, BDR_SUNKENOUTER, BF_RECT);
       }
       else
       {
          FillRect(Dc, &Rect, GetSysColorBrush(COLOR_HIGHLIGHT));
       }
    }
  }
  else
  {
     FillRect(Dc, &Rect, GetSysColorBrush(bkgnd));
  }

  SetBkMode(Dc, TRANSPARENT);

  /* vertical separator */
  if (! MenuBar && 0 != (Item->fType & MF_MENUBARBREAK))
  {
    HPEN oldPen;
    RECT rc = Rect;
    rc.left -= 3;
    rc.top = 3;
    rc.bottom = Height - 3;
    if (flat_menu)
    {
       oldPen = SelectObject( Dc, GetStockObject(DC_PEN) );
       SetDCPenColor(Dc, GetSysColor(COLOR_BTNSHADOW));
       MoveToEx( Dc, rc.left, rc.top, NULL );
       LineTo( Dc, rc.left, rc.bottom );
       SelectObject( Dc, oldPen );
    }
    else
       DrawEdge(Dc, &rc, EDGE_ETCHED, BF_LEFT);
  }

  /* horizontal separator */
  if (0 != (Item->fType & MF_SEPARATOR))
  {
    HPEN oldPen;
    RECT rc = Rect;
    rc.left++;
    rc.right--;
    rc.top += SEPARATOR_HEIGHT / 2;
    if (flat_menu)
    {
       oldPen = SelectObject( Dc, GetStockObject(DC_PEN) );
       SetDCPenColor(Dc, GetSysColor(COLOR_BTNSHADOW));
       MoveToEx( Dc, rc.left, rc.top, NULL );
       LineTo( Dc, rc.right, rc.top );
       SelectObject( Dc, oldPen );
    }
    else
       DrawEdge(Dc, &rc, EDGE_ETCHED, BF_TOP);
    return;
  }

#if 0
  /* helper lines for debugging */
  /* This is a very good test tool when hacking menus! (JT) 07/16/2006 */
  FrameRect(Dc, &Rect, GetStockObject(BLACK_BRUSH));
  SelectObject(Dc, GetStockObject(DC_PEN));
  SetDCPenColor(Dc, GetSysColor(COLOR_WINDOWFRAME));
  MoveToEx(Dc, Rect.left, (Rect.top + Rect.bottom) / 2, NULL);
  LineTo(Dc, Rect.right, (Rect.top + Rect.bottom) / 2);
#endif

  if (! MenuBar)
  {
     INT y = Rect.top + Rect.bottom;
     RECT Rc = Rect;
     UINT CheckBitmapWidth = GetSystemMetrics(SM_CXMENUCHECK);
     UINT CheckBitmapHeight = GetSystemMetrics(SM_CYMENUCHECK);
     int checked = FALSE;
     /* Draw the check mark
      *
      * FIXME:
      * Custom checkmark bitmaps are monochrome but not always 1bpp.
      */
     if( !(MenuInfo->dwStyle & MNS_NOCHECK))
     {
        HBITMAP bm = 0 != (Item->fState & MF_CHECKED) ? Item->hbmpChecked : Item->hbmpUnchecked;
        if (NULL != bm)  /* we have a custom bitmap */
        {
           HDC DcMem = CreateCompatibleDC(Dc);
           SelectObject(DcMem, bm);
           BitBlt(Dc, Rc.left, (y - CheckBitmapHeight) / 2,
                  CheckBitmapWidth, CheckBitmapHeight,
                  DcMem, 0, 0, SRCCOPY);
           DeleteDC(DcMem);
           checked = TRUE;
        }
        else if (0 != (Item->fState & MF_CHECKED))  /* standard bitmaps */
        {
           RECT rectTemp;
           CopyRect(&rectTemp, &Rect);
           rectTemp.right = rectTemp.left + GetSystemMetrics(SM_CXMENUCHECK);
           DrawFrameControl(Dc, &rectTemp, DFC_MENU,
                            0 != (Item->fType & MFT_RADIOCHECK) ?
                                 DFCS_MENUBULLET : DFCS_MENUCHECK);
           checked = TRUE;
        }
     }
     if (Item->hbmpItem)
     {
       RECT bmpRect;
       CopyRect(&bmpRect, &Rect);
       if (!(MenuInfo->dwStyle & MNS_CHECKORBMP) && !(MenuInfo->dwStyle & MNS_NOCHECK))
         bmpRect.left += CheckBitmapWidth + 2;
       if (!(checked && (MenuInfo->dwStyle & MNS_CHECKORBMP)))
       {
         bmpRect.right = bmpRect.left + MenuInfo->maxBmpSize.cx;
         MenuDrawBitmapItem(Dc, Item, &bmpRect, MenuInfo->Self, WndOwner, Action, MenuBar);
       }
     }
     /* Draw the popup-menu arrow */
     if (0 != (Item->fType & MF_POPUP))
     {
           RECT rectTemp;
           CopyRect(&rectTemp, &Rect);
           rectTemp.left = rectTemp.right - GetSystemMetrics(SM_CXMENUCHECK);
           DrawFrameControl(Dc, &rectTemp, DFC_MENU, DFCS_MENUARROW);
     }
     Rect.left += 4;
     if( !(MenuInfo->dwStyle & MNS_NOCHECK))
        Rect.left += CheckBitmapWidth;
     Rect.right -= CheckBitmapWidth;
  }
  else if (Item->hbmpItem) /* Draw the bitmap */
  {
     MenuDrawBitmapItem(Dc, Item, &Rect, MenuInfo->Self, WndOwner, Action, MenuBar);
  }

  /* No bitmap - process text if present */
  if (Item->Text)
  {
      register int i = 0;
      HFONT FontOld = NULL;

      UINT uFormat = MenuBar ? DT_CENTER | DT_VCENTER | DT_SINGLELINE
                     : DT_LEFT | DT_VCENTER | DT_SINGLELINE;

      if(MenuInfo->dwStyle & MNS_CHECKORBMP)
             Rect.left += max(0, MenuInfo->maxBmpSize.cx - GetSystemMetrics(SM_CXMENUCHECK));
      else
             Rect.left += MenuInfo->maxBmpSize.cx;

      if (0 != (Item->fState & MFS_DEFAULT))
        {
          FontOld = SelectObject(Dc, hMenuFontBold);
        }

      if (MenuBar)
        {
          Rect.left += MENU_BAR_ITEMS_SPACE / 2;
          Rect.right -= MENU_BAR_ITEMS_SPACE / 2;
        }

      Text = (PWCHAR) Item->dwTypeData;
      if(Text)
      {
        for (i = 0; L'\0' != Text[i]; i++)
        {
          if (L'\t' == Text[i] || L'\b' == Text[i])
            {
              break;
            }
        }
      }

      if (0 != (Item->fState & MF_GRAYED))
	{
          if (0 == (Item->fState & MF_HILITE))
	    {
              ++Rect.left; ++Rect.top; ++Rect.right; ++Rect.bottom;
              SetTextColor(Dc, RGB(0xff, 0xff, 0xff));
              DrawTextW(Dc, Text, i, &Rect, uFormat);
              --Rect.left; --Rect.top; --Rect.right; --Rect.bottom;
	    }
          SetTextColor(Dc, RGB(0x80, 0x80, 0x80));
        }

      DrawTextW(Dc, Text, i, &Rect, uFormat);

      /* paint the shortcut text */
      if (! MenuBar && L'\0' != Text[i])  /* There's a tab or flush-right char */
        {
          if (L'\t' == Text[i])
            {
              Rect.left = Item->XTab;
              uFormat = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
            }
          else
            {
              Rect.right = Item->XTab;
              uFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;
            }

          if (0 != (Item->fState & MF_GRAYED))
            {
              if (0 == (Item->fState & MF_HILITE))
                {
                  ++Rect.left; ++Rect.top; ++Rect.right; ++Rect.bottom;
                  SetTextColor(Dc, RGB(0xff, 0xff, 0xff));
                  DrawTextW(Dc, Text + i + 1, -1, &Rect, uFormat);
                  --Rect.left; --Rect.top; --Rect.right; --Rect.bottom;
                }
              SetTextColor(Dc, RGB(0x80, 0x80, 0x80));
	    }
          DrawTextW(Dc, Text + i + 1, -1, &Rect, uFormat);
        }

      if (NULL != FontOld)
        {
          SelectObject(Dc, FontOld);
        }
  }
}

/***********************************************************************
 *           MenuDrawPopupMenu
 *
 * Paint a popup menu.
 */
static void FASTCALL
MenuDrawPopupMenu(HWND Wnd, HDC Dc, HMENU Menu)
{
  HBRUSH PrevBrush = NULL;
  HPEN PrevPen;
  RECT Rect;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  UINT u;

  TRACE("wnd=%x dc=%x menu=%x\n", Wnd, Dc, Menu);

  GetClientRect(Wnd, &Rect);

  if (NULL != (PrevBrush = SelectObject(Dc, GetSysColorBrush(COLOR_MENU)))
      && NULL != SelectObject(Dc, hMenuFont))
    {
      Rectangle(Dc, Rect.left, Rect.top, Rect.right, Rect.bottom);

      PrevPen = SelectObject(Dc, GetStockObject(NULL_PEN));
      if (NULL != PrevPen)
        {
          BOOL flat_menu = FALSE;

          SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);
          if (flat_menu)
             FrameRect(Dc, &Rect, GetSysColorBrush(COLOR_BTNSHADOW));
          else
             DrawEdge(Dc, &Rect, EDGE_RAISED, BF_RECT);

          /* draw menu items */

          if (MenuGetRosMenuInfo(&MenuInfo, Menu) && 0 != MenuInfo.MenuItemCount)
            {
              MenuInitRosMenuItemInfo(&ItemInfo);

              for (u = 0; u < MenuInfo.MenuItemCount; u++)
                {
                  if (MenuGetRosMenuItemInfo(MenuInfo.Self, u, &ItemInfo))
                    {
                      MenuDrawMenuItem(Wnd, &MenuInfo, MenuInfo.WndOwner, Dc, &ItemInfo,
				       MenuInfo.Height, FALSE, ODA_DRAWENTIRE);
                    }
                }

	      MenuCleanupRosMenuItemInfo(&ItemInfo);
	    }
	}
      else
        {
          SelectObject(Dc, PrevBrush);
	}
    }
}

static LRESULT WINAPI
PopupMenuWndProcW(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  TRACE("hwnd=%x msg=0x%04x wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  switch(Message)
    {
    case WM_CREATE:
      {
        CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
        SetWindowLongPtrW(Wnd, 0, (LONG) cs->lpCreateParams);
        return 0;
      }

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
      return MA_NOACTIVATE;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        BeginPaint(Wnd, &ps);
        MenuDrawPopupMenu(Wnd, ps.hdc, (HMENU)GetWindowLongPtrW(Wnd, 0));
        EndPaint(Wnd, &ps);
        return 0;
      }

    case WM_ERASEBKGND:
      return 1;

    case WM_DESTROY:
      /* zero out global pointer in case resident popup window was destroyed. */
      if (Wnd == TopPopup)
        {
          TopPopup = NULL;
        }
      break;

    case WM_SHOWWINDOW:
      if (0 != wParam)
        {
          if (0 == GetWindowLongPtrW(Wnd, 0))
            {
              OutputDebugStringA("no menu to display\n");
            }
        }
      else
        {
          SetWindowLongPtrW(Wnd, 0, 0);
        }
      break;

    case MM_SETMENUHANDLE:
      SetWindowLongPtrW(Wnd, 0, wParam);
      break;

    case MM_GETMENUHANDLE:
      return GetWindowLongPtrW(Wnd, 0);

    default:
      return DefWindowProcW(Wnd, Message, wParam, lParam);
    }

  return 0;
}

/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * FIXME - should we be passing an LPCSTR to a predominantly UNICODE function?
 */
static LPCSTR MENUEX_ParseResource( LPCSTR res, HMENU hMenu)
{
  WORD resinfo;

  do
    {
      MENUITEMINFOW mii;

      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;
      mii.fType = GET_DWORD(res);
      res += sizeof(DWORD);
      mii.fState = GET_DWORD(res);
      res += sizeof(DWORD);
      mii.wID = GET_DWORD(res);
      res += sizeof(DWORD);
      resinfo = GET_WORD(res);
      res += sizeof(WORD);
      /* Align the text on a word boundary.  */
      res += (~((int)res - 1)) & 1;
      mii.dwTypeData = (LPWSTR) res;
      res += (1 + strlenW(mii.dwTypeData)) * sizeof(WCHAR);
      /* Align the following fields on a dword boundary.  */
      res += (~((int)res - 1)) & 3;

      if (resinfo & 1) /* Pop-up? */
	{
	  /* DWORD helpid = GET_DWORD(res); FIXME: use this.  */
	  res += sizeof(DWORD);
	  mii.hSubMenu = CreatePopupMenu();
	  if (!mii.hSubMenu)
	      return NULL;
	  if (!(res = MENUEX_ParseResource(res, mii.hSubMenu)))
	  {
	      DestroyMenu(mii.hSubMenu);
	      return NULL;
	  }
	  mii.fMask |= MIIM_SUBMENU;
	  mii.fType |= MF_POPUP;
	  mii.wID = (UINT) mii.hSubMenu;
	}
      else if(!*mii.dwTypeData && !(mii.fType & MF_SEPARATOR))
	{
	  mii.fType |= MF_SEPARATOR;
	}
    InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
  }
  while (!(resinfo & MF_END));
  return res;
}


/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * NOTE: flags is equivalent to the mtOption field
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu, BOOL unicode )
{
  WORD flags, id = 0;
  HMENU hSubMenu;
  LPCSTR str;
  BOOL end = FALSE;

  do
  {
    flags = GET_WORD(res);

    /* remove MF_END flag before passing it to AppendMenu()! */
    end = (flags & MF_END);
    if(end) flags ^= MF_END;

    res += sizeof(WORD);
    if(!(flags & MF_POPUP))
    {
      id = GET_WORD(res);
      res += sizeof(WORD);
    }
    str = res;
    if(!unicode)
      res += strlen(str) + 1;
    else
      res += (strlenW((LPCWSTR)str) + 1) * sizeof(WCHAR);
    if (flags & MF_POPUP)
    {
      hSubMenu = CreatePopupMenu();
      if(!hSubMenu) return NULL;
      if(!(res = MENU_ParseResource(res, hSubMenu, unicode)))
        return NULL;
      if(!unicode)
        AppendMenuA(hMenu, flags, (UINT)hSubMenu, str);
      else
        AppendMenuW(hMenu, flags, (UINT)hSubMenu, (LPCWSTR)str);
    }
    else  /* Not a popup */
    {
      if (*str == 0)
        flags = MF_SEPARATOR;

      if (flags & MF_SEPARATOR)
      {
        if (!(flags & (MF_GRAYED | MF_DISABLED)))
          flags |= MF_GRAYED | MF_DISABLED;
      }

      if(!unicode)
        AppendMenuA(hMenu, flags, id, *str ? str : NULL);
      else
        AppendMenuW(hMenu, flags, id,
                    *(LPCWSTR)str ? (LPCWSTR)str : NULL);
    }
  } while(!end);

  return res;
}


NTSTATUS WINAPI
User32LoadSysMenuTemplateForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  HMENU hmenu = LoadMenuW(User32Instance, L"SYSMENU");
  LRESULT Result = (LRESULT)hmenu;

  // removing space for checkboxes from menu
  MENUINFO menuinfo = {0};
  menuinfo.cbSize = sizeof(menuinfo);
  menuinfo.fMask = MIM_STYLE;
  GetMenuInfo(hmenu, &menuinfo);
  menuinfo.dwStyle |= MNS_NOCHECK;
  SetMenuInfo(hmenu, &menuinfo);

  // adding bitmaps to menu items
  MENUITEMINFOW info = {0};
  info.cbSize = sizeof(info);
  info.fMask |= MIIM_BITMAP;
  info.hbmpItem = HBMMENU_POPUP_MINIMIZE;
  SetMenuItemInfoW(hmenu, SC_MINIMIZE, FALSE, &info);
  info.hbmpItem = HBMMENU_POPUP_RESTORE;
  SetMenuItemInfoW(hmenu, SC_RESTORE, FALSE, &info);
  info.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
  SetMenuItemInfoW(hmenu, SC_MAXIMIZE, FALSE, &info);
  info.hbmpItem = HBMMENU_POPUP_CLOSE;
  SetMenuItemInfoW(hmenu, SC_CLOSE, FALSE, &info);

  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


BOOL
MenuInit(VOID)
{
  NONCLIENTMETRICSW ncm;

  /* get the menu font */
  if(!hMenuFont || !hMenuFontBold)
  {
    ncm.cbSize = sizeof(ncm);
    if(!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
      DbgPrint("MenuInit(): SystemParametersInfoW(SPI_GETNONCLIENTMETRICS) failed!\n");
      return FALSE;
    }

    hMenuFont = CreateFontIndirectW(&ncm.lfMenuFont);
    if(hMenuFont == NULL)
    {
      DbgPrint("MenuInit(): CreateFontIndirectW(hMenuFont) failed!\n");
      return FALSE;
    }

    ncm.lfMenuFont.lfWeight = max(ncm.lfMenuFont.lfWeight + 300, 1000);
    hMenuFontBold = CreateFontIndirectW(&ncm.lfMenuFont);
    if(hMenuFontBold == NULL)
    {
      DbgPrint("MenuInit(): CreateFontIndirectW(hMenuFontBold) failed!\n");
      DeleteObject(hMenuFont);
      hMenuFont = NULL;
      return FALSE;
    }
  }

  return TRUE;
}


VOID
MenuCleanup(VOID)
{
  if (hMenuFont)
  {
    DeleteObject(hMenuFont);
    hMenuFont = NULL;
  }

  if (hMenuFontBold)
  {
    DeleteObject(hMenuFontBold);
    hMenuFontBold = NULL;
  }
}



/***********************************************************************
 *           MenuCalcItemSize
 *
 * Calculate the size of the menu item and store it in ItemInfo->rect.
 */
static void FASTCALL
MenuCalcItemSize(HDC Dc, PROSMENUITEMINFO ItemInfo, PROSMENUINFO MenuInfo, HWND WndOwner,
                 INT OrgX, INT OrgY, BOOL MenuBar)
{
  PWCHAR p;
  INT itemheight = 0;
  UINT CheckBitmapWidth = GetSystemMetrics(SM_CXMENUCHECK);

  TRACE("dc=%x owner=%x (%d,%d)\n", Dc, WndOwner, OrgX, OrgY);

  MenuCharSize.cx = GdiGetCharDimensions( Dc, NULL, &MenuCharSize.cy );

  SetRect(&ItemInfo->Rect, OrgX, OrgY, OrgX, OrgY);

  if (0 != (ItemInfo->fType & MF_OWNERDRAW))
    {
      /*
       ** Experimentation under Windows reveals that an owner-drawn
       ** menu is expected to return the size of the content part of
       ** the menu item, not including the checkmark nor the submenu
       ** arrow.  Windows adds those values itself and returns the
       ** enlarged rectangle on subsequent WM_DRAWITEM messages.
       */
      MEASUREITEMSTRUCT mis;
      mis.CtlType    = ODT_MENU;
      mis.CtlID      = 0;
      mis.itemID     = ItemInfo->wID;
      mis.itemData   = (DWORD)ItemInfo->dwItemData;
      mis.itemHeight = HIWORD( GetDialogBaseUnits());
      mis.itemWidth  = 0;
      SendMessageW(WndOwner, WM_MEASUREITEM, 0, (LPARAM) &mis);
   /* Tests reveal that Windows ( Win95 thru WinXP) adds twice the average
    * width of a menufont character to the width of an owner-drawn menu.
    */
      ItemInfo->Rect.right += mis.itemWidth + 2 *  MenuCharSize.cx;

      if (MenuBar)
        {
          /* under at least win95 you seem to be given a standard
             height for the menu and the height value is ignored */
            ItemInfo->Rect.bottom += GetSystemMetrics(SM_CYMENUSIZE);
        }
      else
        {
          ItemInfo->Rect.bottom += mis.itemHeight;
        }

      TRACE("id=%04x size=%dx%d\n", ItemInfo->wID, mis.itemWidth, mis.itemHeight);
      return;
    }

  if (0 != (ItemInfo->fType & MF_SEPARATOR))
    {
      ItemInfo->Rect.bottom += SEPARATOR_HEIGHT;
      if( !MenuBar)
            ItemInfo->Rect.right += CheckBitmapWidth +  MenuCharSize.cx;
      return;
    }

  ItemInfo->XTab = 0;

  if (ItemInfo->hbmpItem)
  {
      SIZE Size;

      if (!MenuBar)  /* hbmpItem */
      {
         MenuGetBitmapItemSize(ItemInfo, &Size, WndOwner );
      /* Keep the size of the bitmap in callback mode to be able
       * to draw it correctly */
         ItemInfo->Rect.right = ItemInfo->Rect.left + Size.cx;
         if (MenuInfo->maxBmpSize.cx < abs(Size.cx) +  MENU_ITEM_HBMP_SPACE ||
             MenuInfo->maxBmpSize.cy < abs(Size.cy))
         {
             MenuInfo->maxBmpSize.cx = abs(Size.cx) + MENU_ITEM_HBMP_SPACE;
             MenuInfo->maxBmpSize.cy = abs(Size.cy);
         }
         MenuSetRosMenuInfo(MenuInfo);
         itemheight = Size.cy + 2;

         if( !(MenuInfo->dwStyle & MNS_NOCHECK))
           ItemInfo->Rect.right += 2 * CheckBitmapWidth;
         ItemInfo->Rect.right += 4 + MenuCharSize.cx;
         ItemInfo->XTab = ItemInfo->Rect.right;
         ItemInfo->Rect.right += CheckBitmapWidth;
      }
      else /* hbmpItem & MenuBar */
      {
        MenuGetBitmapItemSize(ItemInfo, &Size, WndOwner );
        ItemInfo->Rect.right  += Size.cx;
          if( ItemInfo->Text) ItemInfo->Rect.right  += 2;
          itemheight = Size.cy;

      /* Special case: Minimize button doesn't have a space behind it. */
        if (ItemInfo->hbmpItem == (HBITMAP)HBMMENU_MBAR_MINIMIZE ||
            ItemInfo->hbmpItem == (HBITMAP)HBMMENU_MBAR_MINIMIZE_D)
        ItemInfo->Rect.right -= 1;
      }
  }
  else if (!MenuBar)
  {
      if( !(MenuInfo->dwStyle & MNS_NOCHECK))
           ItemInfo->Rect.right += CheckBitmapWidth;
      ItemInfo->Rect.right += 4 + MenuCharSize.cx;
      ItemInfo->XTab = ItemInfo->Rect.right;
      ItemInfo->Rect.right += CheckBitmapWidth;
  }

  /* it must be a text item - unless it's the system menu */
  if (0 == (ItemInfo->fType & MF_SYSMENU) && ItemInfo->Text)
  {
     HFONT hfontOld = NULL;
     RECT rc = ItemInfo->Rect;
     LONG txtheight, txtwidth;

     if ( ItemInfo->fState & MFS_DEFAULT )
     {
        hfontOld = SelectObject( Dc, hMenuFontBold );
     }
     if (MenuBar)
     {
        txtheight = DrawTextW( Dc, ItemInfo->dwTypeData, -1, &rc,
                                                  DT_SINGLELINE|DT_CALCRECT);
        ItemInfo->Rect.right  += rc.right - rc.left;
        itemheight = max( max( itemheight, txtheight),
                                           GetSystemMetrics( SM_CYMENU) - 1);
        ItemInfo->Rect.right +=  2 * MenuCharSize.cx;
     }
     else
     {
        if ((p = strchrW( ItemInfo->dwTypeData, '\t' )) != NULL)
        {
          RECT tmprc = rc;
          LONG tmpheight;
          int n = (int)( p - ItemInfo->dwTypeData);
         /* Item contains a tab (only meaningful in popup menus) */
         /* get text size before the tab */
          txtheight = DrawTextW( Dc, ItemInfo->dwTypeData, n, &rc,
                                                  DT_SINGLELINE|DT_CALCRECT);
          txtwidth = rc.right - rc.left;
          p += 1; /* advance past the Tab */
         /* get text size after the tab */
          tmpheight = DrawTextW( Dc, p, -1, &tmprc, DT_SINGLELINE|DT_CALCRECT);
          ItemInfo->XTab += txtwidth;
          txtheight = max( txtheight, tmpheight);
          txtwidth += MenuCharSize.cx + /* space for the tab */
          tmprc.right - tmprc.left; /* space for the short cut */
        }
        else
        {
          txtheight = DrawTextW( Dc, ItemInfo->dwTypeData, -1, &rc,
                                                   DT_SINGLELINE|DT_CALCRECT);
          txtwidth = rc.right - rc.left;
          ItemInfo->XTab += txtwidth;
        }
        ItemInfo->Rect.right  += 2 + txtwidth;
        itemheight = max( itemheight, max( txtheight + 2, MenuCharSize.cy + 4));
     }
     if (hfontOld) SelectObject (Dc, hfontOld);
  }
  else if( MenuBar)
  {
     itemheight = max( itemheight, GetSystemMetrics(SM_CYMENU)-1);
  }
  ItemInfo->Rect.bottom += itemheight;
  TRACE("(%ld,%ld)-(%ld,%ld)\n", ItemInfo->Rect.left, ItemInfo->Rect.top, ItemInfo->Rect.right, ItemInfo->Rect.bottom);
}

/***********************************************************************
 *           MenuPopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
static void FASTCALL
MenuPopupMenuCalcSize(PROSMENUINFO MenuInfo, HWND WndOwner)
{
  ROSMENUITEMINFO ItemInfo;
  HDC Dc;
  int Start, i;
  int OrgX, OrgY, MaxX, MaxTab, MaxTabWidth;

  MenuInfo->Width = MenuInfo->Height = 0;
  if (0 == MenuInfo->MenuItemCount)
    {
      MenuSetRosMenuInfo(MenuInfo);
      return;
    }

  Dc = GetDC(NULL);
  SelectObject(Dc, hMenuFont);

  Start = 0;
  MaxX = 2 + 1;

  MenuInfo->maxBmpSize.cx = 0;
  MenuInfo->maxBmpSize.cy = 0;

  MenuInitRosMenuItemInfo(&ItemInfo);
  while (Start < MenuInfo->MenuItemCount)
    {
      OrgX = MaxX;
      OrgY = 2;

      MaxTab = MaxTabWidth = 0;

      /* Parse items until column break or end of menu */
      for (i = Start; i < MenuInfo->MenuItemCount; i++)
	{
          if (! MenuGetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo))
            {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              MenuSetRosMenuInfo(MenuInfo);
              return;
            }
          if (i != Start &&
              0 != (ItemInfo.fType & (MF_MENUBREAK | MF_MENUBARBREAK)))
            {
              break;
            }
          MenuCalcItemSize(Dc, &ItemInfo, MenuInfo, WndOwner, OrgX, OrgY, FALSE);
          if (! MenuSetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo))
            {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              MenuSetRosMenuInfo(MenuInfo);
              return;
            }
// Not sure here,, The patch from wine removes this.
//          if (0 != (ItemInfo.fType & MF_MENUBARBREAK))
//            {
//              OrgX++;
//            }
          MaxX = max(MaxX, ItemInfo.Rect.right);
          OrgY = ItemInfo.Rect.bottom;
          if ((ItemInfo.Text) && 0 != ItemInfo.XTab)
	    {
              MaxTab = max(MaxTab, ItemInfo.XTab);
              MaxTabWidth = max(MaxTabWidth, ItemInfo.Rect.right - ItemInfo.XTab);
            }
        }

      /* Finish the column (set all items to the largest width found) */
      MaxX = max(MaxX, MaxTab + MaxTabWidth);
      while (Start < i)
        {
          if (MenuGetRosMenuItemInfo(MenuInfo->Self, Start, &ItemInfo))
            {
              ItemInfo.Rect.right = MaxX;
              if ((ItemInfo.Text) && 0 != ItemInfo.XTab)
                {
                  ItemInfo.XTab = MaxTab;
                }
              MenuSetRosMenuItemInfo(MenuInfo->Self, Start, &ItemInfo);
            }
          Start++;
	}
      MenuInfo->Height = max(MenuInfo->Height, OrgY);
    }

  MenuInfo->Width = MaxX;

  /* space for 3d border */
  MenuInfo->Height += 2;
  MenuInfo->Width += 2;

  ReleaseDC(NULL, Dc);
  MenuCleanupRosMenuItemInfo(&ItemInfo);
  MenuSetRosMenuInfo(MenuInfo);
}

/***********************************************************************
 *           MenuMenuBarCalcSize
 *
 * FIXME: Word 6 implements its own MDI and its own 'close window' bitmap
 * height is off by 1 pixel which causes lengthy window relocations when
 * active document window is maximized/restored.
 *
 * Calculate the size of the menu bar.
 */
static void FASTCALL
MenuMenuBarCalcSize(HDC Dc, LPRECT Rect, PROSMENUINFO MenuInfo, HWND WndOwner)
{
  ROSMENUITEMINFO ItemInfo;
  int Start, i, OrgX, OrgY, MaxY, HelpPos;

  if (NULL == Rect || NULL == MenuInfo)
    {
      return;
    }
  if (0 == MenuInfo->MenuItemCount)
    {
      return;
    }

  TRACE("left=%ld top=%ld right=%ld bottom=%ld\n",
         Rect->left, Rect->top, Rect->right, Rect->bottom);
  MenuInfo->Width = Rect->right - Rect->left;
  MenuInfo->Height = 0;
  MaxY = Rect->top + 1;
  Start = 0;
  HelpPos = -1;

  MenuInfo->maxBmpSize.cx = 0;
  MenuInfo->maxBmpSize.cy = 0;

  MenuInitRosMenuItemInfo(&ItemInfo);
  while (Start < MenuInfo->MenuItemCount)
    {
      if (! MenuGetRosMenuItemInfo(MenuInfo->Self, Start, &ItemInfo))
        {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
        }
      OrgX = Rect->left;
      OrgY = MaxY;

      /* Parse items until line break or end of menu */
      for (i = Start; i < MenuInfo->MenuItemCount; i++)
	{
          if (-1 == HelpPos && 0 != (ItemInfo.fType & MF_RIGHTJUSTIFY))
            {
              HelpPos = i;
            }
          if (i != Start &&
              0 != (ItemInfo.fType & (MF_MENUBREAK | MF_MENUBARBREAK)))
            {
              break;
            }

          TRACE("calling MENU_CalcItemSize org=(%d, %d)\n", OrgX, OrgY);
          MenuCalcItemSize(Dc, &ItemInfo, MenuInfo, WndOwner, OrgX, OrgY, TRUE);
          if (! MenuSetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo))
            {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              return;
            }

          if (ItemInfo.Rect.right > Rect->right)
            {
              if (i != Start)
                {
                  break;
                }
              else
                {
                  ItemInfo.Rect.right = Rect->right;
                }
            }
          MaxY = max(MaxY, ItemInfo.Rect.bottom );
          OrgX = ItemInfo.Rect.right;
          if (i + 1 < MenuInfo->MenuItemCount)
            {
              if (! MenuGetRosMenuItemInfo(MenuInfo->Self, i + 1, &ItemInfo))
                {
                  MenuCleanupRosMenuItemInfo(&ItemInfo);
                  return;
                }
            }
	}

/* FIXME: Is this really needed? */ /*NO! it is not needed, why make the
HBMMENU_MBAR_CLOSE, MINIMIZE & RESTORE, look the same size as the menu bar! */
#if 0
      /* Finish the line (set all items to the largest height found) */
      while (Start < i)
        {
          if (MenuGetRosMenuItemInfo(MenuInfo->Self, Start, &ItemInfo))
            {
              ItemInfo.Rect.bottom = MaxY;
              MenuSetRosMenuItemInfo(MenuInfo->Self, Start, &ItemInfo);
            }
          Start++;
        }
#else
     Start = i; /* This works! */
#endif
    }

  Rect->bottom = MaxY;
  MenuInfo->Height = Rect->bottom - Rect->top;
  MenuSetRosMenuInfo(MenuInfo);

  if (-1 != HelpPos)
    {
      /* Flush right all items between the MF_RIGHTJUSTIFY and */
      /* the last item (if several lines, only move the last line) */
      if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->MenuItemCount - 1, &ItemInfo))
        {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
        }
      OrgY = ItemInfo.Rect.top;
      OrgX = Rect->right;
      for (i = MenuInfo->MenuItemCount - 1; HelpPos <= i; i--)
        {
          if (i < HelpPos)
            {
              break;				/* done */
            }
          if (ItemInfo.Rect.top != OrgY)
            {
              break;				/* Other line */
            }
          if (OrgX <= ItemInfo.Rect.right)
            {
              break;				/* Too far right already */
            }
          ItemInfo.Rect.left += OrgX - ItemInfo.Rect.right;
          ItemInfo.Rect.right = OrgX;
          OrgX = ItemInfo.Rect.left;
          MenuSetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo);
          if (HelpPos + 1 <= i &&
              ! MenuGetRosMenuItemInfo(MenuInfo->Self, i - 1, &ItemInfo))
            {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              return;
            }
        }
    }

  MenuCleanupRosMenuItemInfo(&ItemInfo);
}

/***********************************************************************
 *           DrawMenuBarTemp   (USER32.@)
 *
 * UNDOCUMENTED !!
 *
 * called by W98SE desk.cpl Control Panel Applet
 *
 * Not 100% sure about the param names, but close.
 *
 * @implemented
 */
DWORD WINAPI
DrawMenuBarTemp(HWND Wnd, HDC DC, LPRECT Rect, HMENU Menu, HFONT Font)
{
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  UINT i;
  HFONT FontOld = NULL;
  BOOL flat_menu = FALSE;

  SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);

  if (NULL == Menu)
    {
      Menu = GetMenu(Wnd);
    }

  if (NULL == Font)
    {
      Font = hMenuFont;
    }

  if (NULL == Rect || ! MenuGetRosMenuInfo(&MenuInfo, Menu))
    {
      return GetSystemMetrics(SM_CYMENU);
    }

  TRACE("(%x, %x, %p, %x, %x)\n", Wnd, DC, Rect, Menu, Font);

  FontOld = SelectObject(DC, Font);

  if (0 == MenuInfo.Height)
    {
      MenuMenuBarCalcSize(DC, Rect, &MenuInfo, Wnd);
    }

  Rect->bottom = Rect->top + MenuInfo.Height;

  FillRect(DC, Rect, GetSysColorBrush(flat_menu ? COLOR_MENUBAR : COLOR_MENU));

  SelectObject(DC, GetStockObject(DC_PEN));
  SetDCPenColor(DC, GetSysColor(COLOR_3DFACE));
  MoveToEx(DC, Rect->left, Rect->bottom, NULL);
  LineTo(DC, Rect->right, Rect->bottom);

  if (0 == MenuInfo.MenuItemCount)
    {
      SelectObject(DC, FontOld);
      return GetSystemMetrics(SM_CYMENU);
    }

  MenuInitRosMenuItemInfo(&ItemInfo);
  for (i = 0; i < MenuInfo.MenuItemCount; i++)
    {
      if (MenuGetRosMenuItemInfo(MenuInfo.Self, i, &ItemInfo))
        {
          MenuDrawMenuItem(Wnd, &MenuInfo, Wnd, DC, &ItemInfo,
                           MenuInfo.Height, TRUE, ODA_DRAWENTIRE);
        }
    }
  MenuCleanupRosMenuItemInfo(&ItemInfo);

  SelectObject(DC, FontOld);

  return MenuInfo.Height;
}


/***********************************************************************
 *           MenuDrawMenuBar
 *
 * Paint a menu bar. Returns the height of the menu bar.
 * called from [windows/nonclient.c]
 */
UINT MenuDrawMenuBar(HDC DC, LPRECT Rect, HWND Wnd, BOOL SuppressDraw)
{
  ROSMENUINFO MenuInfo;
  HFONT FontOld = NULL;
  HMENU Menu = GetMenu(Wnd);

  if (NULL == Rect || ! MenuGetRosMenuInfo(&MenuInfo, Menu))
    {
      return GetSystemMetrics(SM_CYMENU);
    }

  if (SuppressDraw)
    {
      FontOld = SelectObject(DC, hMenuFont);

      MenuMenuBarCalcSize(DC, Rect, &MenuInfo, Wnd);

      Rect->bottom = Rect->top + MenuInfo.Height;

      if (NULL != FontOld)
        {
          SelectObject(DC, FontOld);
        }
      return MenuInfo.Height;
    }
  else
    {
      return DrawMenuBarTemp(Wnd, DC, Rect, Menu, NULL);
    }
}

/***********************************************************************
 *           MenuInitTracking
 */
static BOOL FASTCALL
MenuInitTracking(HWND Wnd, HMENU Menu, BOOL Popup, UINT Flags)
{
  TRACE("Wnd=%p Menu=%p\n", Wnd, Menu);

  HideCaret(0);

  /* Send WM_ENTERMENULOOP and WM_INITMENU message only if TPM_NONOTIFY flag is not specified */
  if (0 == (Flags & TPM_NONOTIFY))
    {
      SendMessageW(Wnd, WM_ENTERMENULOOP, Popup, 0);
    }

  SendMessageW(Wnd, WM_SETCURSOR, (WPARAM) Wnd, HTCAPTION);

  if (0 == (Flags & TPM_NONOTIFY))
    {
      ROSMENUINFO MenuInfo;

      SendMessageW(Wnd, WM_INITMENU, (WPARAM)Menu, 0);

      MenuGetRosMenuInfo(&MenuInfo, Menu);

      if (0 == MenuInfo.Height)
        {
          /* app changed/recreated menu bar entries in WM_INITMENU
             Recalculate menu sizes else clicks will not work */
          SetWindowPos(Wnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                       SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );

        }
    /* This makes the menus of applications built with Delphi work.
     * It also enables menus to be displayed in more than one window,
     * but there are some bugs left that need to be fixed in this case.
     */
      if(MenuInfo.Self == Menu)
      {
         MenuInfo.Wnd = Wnd;
         MenuSetRosMenuInfo(&MenuInfo);
      }
    }

  return TRUE;
}


/***********************************************************************
 *           MenuShowPopup
 *
 * Display a popup menu.
 */
static BOOL FASTCALL
MenuShowPopup(HWND WndOwner, HMENU Menu, UINT Id,
              INT X, INT Y, INT XAnchor, INT YAnchor )
{
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  UINT Width, Height;

  TRACE("owner=%x hmenu=%x id=0x%04x x=0x%04x y=0x%04x xa=0x%04x ya=0x%04x\n",
         WndOwner, Menu, Id, X, Y, XAnchor, YAnchor);

  if (! MenuGetRosMenuInfo(&MenuInfo, Menu))
    {
      return FALSE;
    }

  if (NO_SELECTED_ITEM != MenuInfo.FocusedItem)
    {
      MenuInitRosMenuItemInfo(&ItemInfo);
      if (MenuGetRosMenuItemInfo(MenuInfo.Self, MenuInfo.FocusedItem, &ItemInfo))
        {
          ItemInfo.fMask |= MIIM_STATE;
          ItemInfo.fState &= ~(MF_HILITE|MF_MOUSESELECT);
          MenuSetRosMenuItemInfo(MenuInfo.Self, MenuInfo.FocusedItem, &ItemInfo);
        }
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      MenuInfo.FocusedItem = NO_SELECTED_ITEM;
    }

  /* store the owner for DrawItem */
  MenuInfo.WndOwner = WndOwner;
  MenuSetRosMenuInfo(&MenuInfo);

  MenuPopupMenuCalcSize(&MenuInfo, WndOwner);

  /* adjust popup menu pos so that it fits within the desktop */

  Width = MenuInfo.Width + GetSystemMetrics(SM_CXBORDER);
  Height = MenuInfo.Height + GetSystemMetrics(SM_CYBORDER);

  if (GetSystemMetrics(SM_CXSCREEN ) < X + Width)
    {
      if (0 != XAnchor && X >= Width - XAnchor)
        {
          X -= Width - XAnchor;
        }
      if (GetSystemMetrics(SM_CXSCREEN) < X + Width)
        {
          X = GetSystemMetrics(SM_CXSCREEN) - Width;
        }
    }
  if (X < 0 )
    {
      X = 0;
    }

  if (GetSystemMetrics(SM_CYSCREEN) < Y + Height)
    {
      if (0 != YAnchor && Y >= Height + YAnchor)
        {
          Y -= Height + YAnchor;
        }
      if (GetSystemMetrics(SM_CYSCREEN) < Y + Height)
        {
          Y = GetSystemMetrics(SM_CYSCREEN) - Height;
        }
    }
  if (Y < 0 )
    {
      Y = 0;
    }


  /* NOTE: In Windows, top menu popup is not owned. */
  MenuInfo.Wnd = CreateWindowExW(0, POPUPMENU_CLASS_ATOMW, NULL,
                                 WS_POPUP, X, Y, Width, Height,
                                 WndOwner, 0, (HINSTANCE) GetWindowLongPtrW(WndOwner, GWLP_HINSTANCE),
                                 (LPVOID) MenuInfo.Self);
  if (NULL == MenuInfo.Wnd || ! MenuSetRosMenuInfo(&MenuInfo))
    {
      return FALSE;
    }
  if (NULL == TopPopup)
    {
      TopPopup = MenuInfo.Wnd;
    }

  /* Display the window */
  SetWindowPos(MenuInfo.Wnd, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
  UpdateWindow(MenuInfo.Wnd);

  return TRUE;
}

/***********************************************************************
 *           MenuFindSubMenu
 *
 * Find a Sub menu. Return the position of the submenu, and modifies
 * *hmenu in case it is found in another sub-menu.
 * If the submenu cannot be found, NO_SELECTED_ITEM is returned.
 */
static UINT FASTCALL
MenuFindSubMenu(HMENU *Menu, HMENU SubTarget)
{
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  UINT i;
  HMENU SubMenu;
  UINT Pos;

  if ((HMENU) 0xffff == *Menu
      || ! MenuGetRosMenuInfo(&MenuInfo, *Menu))
    {
      return NO_SELECTED_ITEM;
    }

  MenuInitRosMenuItemInfo(&ItemInfo);
  for (i = 0; i < MenuInfo.MenuItemCount; i++)
    {
      if (! MenuGetRosMenuItemInfo(MenuInfo.Self, i, &ItemInfo))
        {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return NO_SELECTED_ITEM;
        }
      if (0 == (ItemInfo.fType & MF_POPUP))
        {
          continue;
        }
      if (ItemInfo.hSubMenu == SubTarget)
        {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return i;
        }
      SubMenu = ItemInfo.hSubMenu;
      Pos = MenuFindSubMenu(&SubMenu, SubTarget);
      if (NO_SELECTED_ITEM != Pos)
        {
          *Menu = SubMenu;
          return Pos;
        }
    }
  MenuCleanupRosMenuItemInfo(&ItemInfo);

  return NO_SELECTED_ITEM;
}

/***********************************************************************
 *           MenuSelectItem
 */
static void FASTCALL
MenuSelectItem(HWND WndOwner, PROSMENUINFO MenuInfo, UINT Index,
               BOOL SendMenuSelect, HMENU TopMenu)
{
  HDC Dc;
  ROSMENUITEMINFO ItemInfo;
  ROSMENUINFO TopMenuInfo;
  int Pos;

  TRACE("owner=%x menu=%p index=0x%04x select=0x%04x\n", WndOwner, MenuInfo, Index, SendMenuSelect);

  if (NULL == MenuInfo || 0 == MenuInfo->MenuItemCount || NULL == MenuInfo->Wnd)
    {
      return;
    }

  if (MenuInfo->FocusedItem == Index)
    {
      return;
    }

  if (0 != (MenuInfo->Flags & MF_POPUP))
    {
      Dc = GetDC(MenuInfo->Wnd);
    }
  else
    {
      Dc = GetDCEx(MenuInfo->Wnd, 0, DCX_CACHE | DCX_WINDOW);
    }

  if (NULL == TopPopup)
    {
      TopPopup = MenuInfo->Wnd;
    }

  SelectObject(Dc, hMenuFont);
  MenuInitRosMenuItemInfo(&ItemInfo);
  /* Clear previous highlighted item */
  if (NO_SELECTED_ITEM != MenuInfo->FocusedItem)
    {
      if (MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo))
        {
          ItemInfo.fMask |= MIIM_STATE;
          ItemInfo.fState &= ~(MF_HILITE|MF_MOUSESELECT);
          MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo);
        }
      MenuDrawMenuItem(MenuInfo->Wnd, MenuInfo, WndOwner, Dc, &ItemInfo,
                       MenuInfo->Height, ! (MenuInfo->Flags & MF_POPUP),
                       ODA_SELECT);
    }

  /* Highlight new item (if any) */
  MenuInfo->FocusedItem = Index;
  MenuSetRosMenuInfo(MenuInfo);
  if (NO_SELECTED_ITEM != MenuInfo->FocusedItem)
    {
      if (MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo))
        {
          if (0 == (ItemInfo.fType & MF_SEPARATOR))
            {
              ItemInfo.fMask |= MIIM_STATE;
              ItemInfo.fState |= MF_HILITE;
              MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo);
              MenuDrawMenuItem(MenuInfo->Wnd, MenuInfo, WndOwner, Dc,
                               &ItemInfo, MenuInfo->Height, ! (MenuInfo->Flags & MF_POPUP),
                               ODA_SELECT);
            }
          if (SendMenuSelect)
            {
              SendMessageW(WndOwner, WM_MENUSELECT,
                           MAKELONG(ItemInfo.fType & MF_POPUP ? Index : ItemInfo.wID,
                                    ItemInfo.fType | ItemInfo.fState | MF_MOUSESELECT |
                                    (MenuInfo->Flags & MF_SYSMENU)), (LPARAM) MenuInfo->Self);
            }
        }
    }
  else if (SendMenuSelect)
    {
      if (NULL != TopMenu)
        {
          Pos = MenuFindSubMenu(&TopMenu, MenuInfo->Self);
          if (NO_SELECTED_ITEM != Pos)
            {
              if (MenuGetRosMenuInfo(&TopMenuInfo, TopMenu)
                  && MenuGetRosMenuItemInfo(TopMenu, Pos, &ItemInfo))
                {
                  SendMessageW(WndOwner, WM_MENUSELECT,
                               MAKELONG(Pos, ItemInfo.fType | ItemInfo.fState
                                             | MF_MOUSESELECT
                                             | (TopMenuInfo.Flags & MF_SYSMENU)),
                               (LPARAM) TopMenu);
                }
            }
        }
    }
  MenuCleanupRosMenuItemInfo(&ItemInfo);
  ReleaseDC(MenuInfo->Wnd, Dc);
}

/***********************************************************************
 *           MenuMoveSelection
 *
 * Moves currently selected item according to the Offset parameter.
 * If there is no selection then it should select the last item if
 * Offset is ITEM_PREV or the first item if Offset is ITEM_NEXT.
 */
static void FASTCALL
MenuMoveSelection(HWND WndOwner, PROSMENUINFO MenuInfo, INT Offset)
{
  INT i;
  ROSMENUITEMINFO ItemInfo;
  INT OrigPos;

  TRACE("hwnd=%x menu=%x off=0x%04x\n", WndOwner, MenuInfo, Offset);

  /* Prevent looping */
  if (0 == MenuInfo->MenuItemCount || 0 == Offset)
    return;
  else if (Offset < -1)
    Offset = -1;
  else if (Offset > 1)
    Offset = 1;

  MenuInitRosMenuItemInfo(&ItemInfo);

  OrigPos = MenuInfo->FocusedItem;
  if (OrigPos == NO_SELECTED_ITEM) /* NO_SELECTED_ITEM is not -1 ! */
    {
       OrigPos = 0;
       i = -1;
    }
  else
    {
      i = MenuInfo->FocusedItem;
    }

  do
    {
      /* Step */
      i += Offset;
      /* Clip and wrap around */
      if (i < 0)
        {
          i = MenuInfo->MenuItemCount - 1;
        }
      else if (i >= MenuInfo->MenuItemCount)
        {
          i = 0;
        }
      /* If this is a good candidate; */
      if (MenuGetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo) &&
          0 == (ItemInfo.fType & MF_SEPARATOR))
        {
          MenuSelectItem(WndOwner, MenuInfo, i, TRUE, NULL);
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
        }
    } while (i != OrigPos);

  /* Not found */
  MenuCleanupRosMenuItemInfo(&ItemInfo);
}

/***********************************************************************
 *           MenuInitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
void FASTCALL
MenuInitSysMenuPopup(HMENU Menu, DWORD Style, DWORD ClsStyle, LONG HitTest )
{
  BOOL Gray;
  UINT DefItem;
  #if 0
  MENUITEMINFOW mii;
  #endif

  Gray = 0 == (Style & WS_THICKFRAME) || 0 != (Style & (WS_MAXIMIZE | WS_MINIMIZE));
  EnableMenuItem(Menu, SC_SIZE, (Gray ? MF_GRAYED : MF_ENABLED));
  Gray = 0 != (Style & WS_MAXIMIZE);
  EnableMenuItem(Menu, SC_MOVE, (Gray ? MF_GRAYED : MF_ENABLED));
  Gray = 0 == (Style & WS_MINIMIZEBOX) || 0 != (Style & WS_MINIMIZE);
  EnableMenuItem(Menu, SC_MINIMIZE, (Gray ? MF_GRAYED : MF_ENABLED));
  Gray = 0 == (Style & WS_MAXIMIZEBOX) || 0 != (Style & WS_MAXIMIZE);
  EnableMenuItem(Menu, SC_MAXIMIZE, (Gray ? MF_GRAYED : MF_ENABLED));
  Gray = 0 == (Style & (WS_MAXIMIZE | WS_MINIMIZE));
  EnableMenuItem(Menu, SC_RESTORE, (Gray ? MF_GRAYED : MF_ENABLED));
  Gray = 0 != (ClsStyle & CS_NOCLOSE);

  /* The menu item must keep its state if it's disabled */
  if (Gray)
    {
      EnableMenuItem(Menu, SC_CLOSE, MF_GRAYED);
    }

  /* Set default menu item */
  if(Style & WS_MINIMIZE)
  {
    DefItem = SC_RESTORE;
  }
  else
  {
    if(HitTest == HTCAPTION)
    {
      DefItem = ((Style & (WS_MAXIMIZE | WS_MINIMIZE)) ? SC_RESTORE : SC_MAXIMIZE);
    }
    else
    {
      DefItem = SC_CLOSE;
    }
  }
  #if 0
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask |= MIIM_STATE;
  if((DefItem != SC_CLOSE) && GetMenuItemInfoW(Menu, DefItem, FALSE, &mii) &&
     (mii.fState & (MFS_GRAYED | MFS_DISABLED)))
  {
    DefItem = SC_CLOSE;
  }
  #endif
  SetMenuDefaultItem(Menu, DefItem, MF_BYCOMMAND);
}

/***********************************************************************
 *           MenuShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or menu if no submenu to display.
 */
static HMENU FASTCALL
MenuShowSubPopup(HWND WndOwner, PROSMENUINFO MenuInfo, BOOL SelectFirst, UINT Flags)
{
  extern void FASTCALL NcGetSysPopupPos(HWND Wnd, RECT *Rect);
  RECT Rect;
  ROSMENUITEMINFO ItemInfo;
  ROSMENUINFO SubMenuInfo;
  HDC Dc;
  HMENU Ret;

  TRACE("owner=%x menu=%p 0x%04x\n", WndOwner, MenuInfo, SelectFirst);

  if (NO_SELECTED_ITEM == MenuInfo->FocusedItem)
    {
      return MenuInfo->Self;
    }

  MenuInitRosMenuItemInfo(&ItemInfo);
  if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo))
    {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return MenuInfo->Self;
    }
  if (0 == (ItemInfo.fType & MF_POPUP) || 0 != (ItemInfo.fState & (MF_GRAYED | MF_DISABLED)))
    {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return MenuInfo->Self;
    }

  /* message must be sent before using item,
     because nearly everything may be changed by the application ! */

  /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
  if (0 == (Flags & TPM_NONOTIFY))
    {
      SendMessageW(WndOwner, WM_INITMENUPOPUP, (WPARAM) ItemInfo.hSubMenu,
                   MAKELONG(MenuInfo->FocusedItem, IS_SYSTEM_MENU(MenuInfo)));
    }

  if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo))
    {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return MenuInfo->Self;
    }
  Rect = ItemInfo.Rect;

  /* correct item if modified as a reaction to WM_INITMENUPOPUP message */
  if (0 == (ItemInfo.fState & MF_HILITE))
    {
      if (0 != (MenuInfo->Flags & MF_POPUP))
        {
          Dc = GetDC(MenuInfo->Wnd);
        }
      else
        {
          Dc = GetDCEx(MenuInfo->Wnd, 0, DCX_CACHE | DCX_WINDOW);
        }

      SelectObject(Dc, hMenuFont);
      ItemInfo.fMask |= MIIM_STATE;
      ItemInfo.fState |= MF_HILITE;
      MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo);
      MenuDrawMenuItem(MenuInfo->Wnd, MenuInfo, WndOwner, Dc, &ItemInfo, MenuInfo->Height,
                       ! (MenuInfo->Flags & MF_POPUP), ODA_DRAWENTIRE);
      ReleaseDC(MenuInfo->Wnd, Dc);
    }

  if (0 == ItemInfo.Rect.top && 0 == ItemInfo.Rect.left
      && 0 == ItemInfo.Rect.bottom && 0 == ItemInfo.Rect.right)
    {
      ItemInfo.Rect = Rect;
    }

  ItemInfo.fMask |= MIIM_STATE;
  ItemInfo.fState |= MF_MOUSESELECT;
  MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo);

  if (IS_SYSTEM_MENU(MenuInfo))
    {
      MenuInitSysMenuPopup(ItemInfo.hSubMenu, GetWindowLongPtrW(MenuInfo->Wnd, GWL_STYLE),
                           GetClassLongPtrW(MenuInfo->Wnd, GCL_STYLE), HTSYSMENU);

      NcGetSysPopupPos(MenuInfo->Wnd, &Rect);
      Rect.top = Rect.bottom;
      Rect.right = GetSystemMetrics(SM_CXSIZE);
      Rect.bottom = GetSystemMetrics(SM_CYSIZE);
    }
  else
    {
      GetWindowRect(MenuInfo->Wnd, &Rect);
      if (0 != (MenuInfo->Flags & MF_POPUP))
	{
          Rect.left += ItemInfo.Rect.right - GetSystemMetrics(SM_CXBORDER);
          Rect.top += ItemInfo.Rect.top - 3;
          Rect.right = ItemInfo.Rect.left - ItemInfo.Rect.right + GetSystemMetrics(SM_CXBORDER);
          Rect.bottom = ItemInfo.Rect.top - ItemInfo.Rect.bottom - 3 - 2
                                                    - GetSystemMetrics(SM_CYBORDER);
        }
      else
        {
          Rect.left += ItemInfo.Rect.left;
          Rect.top += ItemInfo.Rect.bottom;
          Rect.right = ItemInfo.Rect.right - ItemInfo.Rect.left;
          Rect.bottom = ItemInfo.Rect.bottom - ItemInfo.Rect.top;
        }
    }

  MenuShowPopup(WndOwner, ItemInfo.hSubMenu, MenuInfo->FocusedItem,
                Rect.left, Rect.top, Rect.right, Rect.bottom );
  if (SelectFirst && MenuGetRosMenuInfo(&SubMenuInfo, ItemInfo.hSubMenu))
    {
      MenuMoveSelection(WndOwner, &SubMenuInfo, ITEM_NEXT);
    }

  Ret = ItemInfo.hSubMenu;
  MenuCleanupRosMenuItemInfo(&ItemInfo);

  return Ret;
}

/***********************************************************************
 *           MenuHideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void FASTCALL
MenuHideSubPopups(HWND WndOwner, PROSMENUINFO MenuInfo, BOOL SendMenuSelect)
{
  ROSMENUINFO SubMenuInfo;
  ROSMENUITEMINFO ItemInfo;

  TRACE("owner=%x menu=%x 0x%04x\n", WndOwner, MenuInfo, SendMenuSelect);

  if (NULL != MenuInfo && NULL != TopPopup && NO_SELECTED_ITEM != MenuInfo->FocusedItem)
    {
      MenuInitRosMenuItemInfo(&ItemInfo);
      ItemInfo.fMask |= MIIM_FTYPE | MIIM_STATE;
      if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo)
          || 0 == (ItemInfo.fType & MF_POPUP)
          || 0 == (ItemInfo.fState & MF_MOUSESELECT))
        {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
        }
      ItemInfo.fState &= ~MF_MOUSESELECT;
      ItemInfo.fMask |= MIIM_STATE;
      MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo);
      if (MenuGetRosMenuInfo(&SubMenuInfo, ItemInfo.hSubMenu))
        {
          MenuHideSubPopups(WndOwner, &SubMenuInfo, FALSE);
          MenuSelectItem(WndOwner, &SubMenuInfo, NO_SELECTED_ITEM, SendMenuSelect, NULL);
          DestroyWindow(SubMenuInfo.Wnd);
          SubMenuInfo.Wnd = NULL;
          MenuSetRosMenuInfo(&SubMenuInfo);
        }
    }
}

/***********************************************************************
 *           MenuSwitchTracking
 *
 * Helper function for menu navigation routines.
 */
static void FASTCALL
MenuSwitchTracking(MTRACKER* Mt, PROSMENUINFO PtMenuInfo, UINT Index)
{
  ROSMENUINFO TopMenuInfo;

  TRACE("%x menu=%x 0x%04x\n", Mt, PtMenuInfo->Self, Index);

  if (MenuGetRosMenuInfo(&TopMenuInfo, Mt->TopMenu) &&
      Mt->TopMenu != PtMenuInfo->Self &&
      0 == ((PtMenuInfo->Flags | TopMenuInfo.Flags) & MF_POPUP))
    {
      /* both are top level menus (system and menu-bar) */
      MenuHideSubPopups(Mt->OwnerWnd, &TopMenuInfo, FALSE);
      MenuSelectItem(Mt->OwnerWnd, &TopMenuInfo, NO_SELECTED_ITEM, FALSE, NULL);
      Mt->TopMenu = PtMenuInfo->Self;
    }
  else
    {
      MenuHideSubPopups(Mt->OwnerWnd, PtMenuInfo, FALSE);
    }

  MenuSelectItem(Mt->OwnerWnd, PtMenuInfo, Index, TRUE, NULL);
}

/***********************************************************************
 *           MenuExecFocusedItem
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return the wID of the executed item. Otherwise, -1 indicating
 * that no menu item was executed, -2 if a popup is shown;
 * Have to receive the flags for the TrackPopupMenu options to avoid
 * sending unwanted message.
 *
 */
static INT FASTCALL
MenuExecFocusedItem(MTRACKER *Mt, PROSMENUINFO MenuInfo, UINT Flags)
{
  ROSMENUITEMINFO ItemInfo;
  UINT wID;

  TRACE("%p menu=%p\n", Mt, MenuInfo);

  if (0 == MenuInfo->MenuItemCount || NO_SELECTED_ITEM == MenuInfo->FocusedItem)
    {
      return -1;
    }

  MenuInitRosMenuItemInfo(&ItemInfo);
  if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->FocusedItem, &ItemInfo))
    {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return -1;
    }

  TRACE("%p %08x %p\n", MenuInfo, ItemInfo.wID, ItemInfo.hSubMenu);

  if (0 == (ItemInfo.fType & MF_POPUP))
    {
      if (0 == (ItemInfo.fState & (MF_GRAYED | MF_DISABLED))
          && 0 == (ItemInfo.fType & MF_SEPARATOR))
        {
          /* If TPM_RETURNCMD is set you return the id, but
            do not send a message to the owner */
          if (0 == (Flags & TPM_RETURNCMD))
	    {
              if (0 != (MenuInfo->Flags & MF_SYSMENU))
                {
                  PostMessageW(Mt->OwnerWnd, WM_SYSCOMMAND, ItemInfo.wID,
                               MAKELPARAM((SHORT) Mt->Pt.x, (SHORT) Mt->Pt.y));
                }
              else
                {
                  if (MenuInfo->dwStyle & MNS_NOTIFYBYPOS)
                      PostMessageW(Mt->OwnerWnd, WM_MENUCOMMAND,
                                                 MenuInfo->FocusedItem,
                                                       (LPARAM)MenuInfo->Self);
                  else
                    PostMessageW(Mt->OwnerWnd, WM_COMMAND, ItemInfo.wID, 0);
                }
            }
          wID = ItemInfo.wID;
          MenuCleanupRosMenuItemInfo(&ItemInfo);
	  return wID;
        }
    }
  else
    {
      Mt->CurrentMenu = MenuShowSubPopup(Mt->OwnerWnd, MenuInfo, TRUE, Flags);
      return -2;
    }

  return -1;
}

/***********************************************************************
 *           MenuButtonDown
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL FASTCALL
MenuButtonDown(MTRACKER* Mt, HMENU PtMenu, UINT Flags)
{
  int Index;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO Item;

  TRACE("%x PtMenu=%p\n", Mt, PtMenu);

  if (NULL != PtMenu)
    {
      if (! MenuGetRosMenuInfo(&MenuInfo, PtMenu))
        {
          return FALSE;
        }
      if (IS_SYSTEM_MENU(&MenuInfo))
        {
          Index = 0;
        }
      else
        {
          Index = NtUserMenuItemFromPoint(Mt->OwnerWnd, PtMenu, Mt->Pt.x, Mt->Pt.y);
        }
      MenuInitRosMenuItemInfo(&Item);
      if (NO_SELECTED_ITEM == Index || ! MenuGetRosMenuItemInfo(PtMenu, Index, &Item))
        {
          MenuCleanupRosMenuItemInfo(&Item);
          return FALSE;
        }

      if (!(Item.fType & MF_SEPARATOR) &&
          !(Item.fState & (MFS_DISABLED | MFS_GRAYED)) )
	{
          if (MenuInfo.FocusedItem != Index)
            {
              MenuSwitchTracking(Mt, &MenuInfo, Index);
            }

          /* If the popup menu is not already "popped" */
          if (0 == (Item.fState & MF_MOUSESELECT))
            {
              Mt->CurrentMenu = MenuShowSubPopup(Mt->OwnerWnd, &MenuInfo, FALSE, Flags);
            }
        }

      MenuCleanupRosMenuItemInfo(&Item);

      return TRUE;
    }

  /* else the click was on the menu bar, finish the tracking */

  return FALSE;
}

/***********************************************************************
 *           MenuButtonUp
 *
 * Return the value of MenuExecFocusedItem if
 * the selected item was not a popup. Else open the popup.
 * A -1 return value indicates that we go on with menu tracking.
 *
 */
static INT FASTCALL
MenuButtonUp(MTRACKER *Mt, HMENU PtMenu, UINT Flags)
{
  UINT Id;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;

  TRACE("%p hmenu=%x\n", Mt, PtMenu);

  if (NULL != PtMenu)
    {
      Id = 0;
      if (! MenuGetRosMenuInfo(&MenuInfo, PtMenu))
        {
          return -1;
        }

      if (! IS_SYSTEM_MENU(&MenuInfo))
        {
          Id = NtUserMenuItemFromPoint(Mt->OwnerWnd, MenuInfo.Self, Mt->Pt.x, Mt->Pt.y);
        }
      MenuInitRosMenuItemInfo(&ItemInfo);
      if (0 <= Id && MenuGetRosMenuItemInfo(MenuInfo.Self, Id, &ItemInfo) &&
          MenuInfo.FocusedItem == Id)
        {
          if (0 == (ItemInfo.fType & MF_POPUP))
            {
              INT ExecutedMenuId = MenuExecFocusedItem(Mt, &MenuInfo, Flags);
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              return (ExecutedMenuId < 0) ? -1 : ExecutedMenuId;
            }
          MenuCleanupRosMenuItemInfo(&ItemInfo);

          /* If we are dealing with the top-level menu            */
          /* and this is a click on an already "popped" item:     */
          /* Stop the menu tracking and close the opened submenus */
          if (Mt->TopMenu == MenuInfo.Self && MenuInfo.TimeToHide)
            {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              return 0;
            }
        }
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      MenuInfo.TimeToHide = TRUE;
      MenuSetRosMenuInfo(&MenuInfo);
    }

  return -1;
}

/***********************************************************************
 *           MenuPtMenu
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
static HMENU FASTCALL
MenuPtMenu(HMENU Menu, POINT Pt)
{
  extern LRESULT DefWndNCHitTest(HWND hWnd, POINT Point);
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  HMENU Ret = NULL;
  INT Ht;

  if (! MenuGetRosMenuInfo(&MenuInfo, Menu))
    {
      return NULL;
    }

  /* try subpopup first (if any) */
  if (NO_SELECTED_ITEM != MenuInfo.FocusedItem)
    {
      MenuInitRosMenuItemInfo(&ItemInfo);
      if (MenuGetRosMenuItemInfo(MenuInfo.Self, MenuInfo.FocusedItem, &ItemInfo) &&
          0 != (ItemInfo.fType & MF_POPUP) &&
          0 != (ItemInfo.fState & MF_MOUSESELECT))
        {
          Ret = MenuPtMenu(ItemInfo.hSubMenu, Pt);
          if (NULL != Ret)
            {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              return Ret;
            }
        }
      MenuCleanupRosMenuItemInfo(&ItemInfo);
    }

  /* check the current window (avoiding WM_HITTEST) */
  Ht = DefWndNCHitTest(MenuInfo.Wnd, Pt);
  if (0 != (MenuInfo.Flags & MF_POPUP))
    {
      if (HTNOWHERE != Ht && HTERROR != Ht)
        {
          Ret = Menu;
        }
    }
  else if (HTSYSMENU == Ht)
    {
      Ret = NtUserGetSystemMenu(MenuInfo.Wnd, FALSE);
    }
  else if (HTMENU == Ht)
    {
      Ret = GetMenu(MenuInfo.Wnd);
    }

  return Ret;
}

/***********************************************************************
 *           MenuMouseMove
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL FASTCALL
MenuMouseMove(MTRACKER *Mt, HMENU PtMenu, UINT Flags)
{
  UINT Index;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;

  if (NULL != PtMenu)
    {
      if (! MenuGetRosMenuInfo(&MenuInfo, PtMenu))
        {
          return TRUE;
        }
      if (IS_SYSTEM_MENU(&MenuInfo))
        {
          Index = 0;
        }
      else
        {
          Index = NtUserMenuItemFromPoint(Mt->OwnerWnd, PtMenu, Mt->Pt.x, Mt->Pt.y);
        }
    }
  else
    {
      Index = NO_SELECTED_ITEM;
    }

  if (NO_SELECTED_ITEM == Index)
    {
      if (Mt->CurrentMenu == MenuInfo.Self ||
          MenuGetRosMenuInfo(&MenuInfo, Mt->CurrentMenu))
        {
          MenuSelectItem(Mt->OwnerWnd, &MenuInfo, NO_SELECTED_ITEM,
                         TRUE, Mt->TopMenu);
        }
    }
  else if (MenuInfo.FocusedItem != Index)
    {
	MenuInitRosMenuItemInfo(&ItemInfo);
	if (MenuGetRosMenuItemInfo(MenuInfo.Self, Index, &ItemInfo) &&
           !(ItemInfo.fType & MF_SEPARATOR))
	{
	    MenuSwitchTracking(Mt, &MenuInfo, Index);
        if (!(ItemInfo.fState & (MFS_DISABLED | MFS_GRAYED)))
	        Mt->CurrentMenu = MenuShowSubPopup(Mt->OwnerWnd, &MenuInfo, FALSE, Flags);
	}
	MenuCleanupRosMenuItemInfo(&ItemInfo);
    }

  return TRUE;
}

/******************************************************************************
 *
 *   UINT MenuGetStartOfNextColumn(PROSMENUINFO MenuInfo)
 */
static UINT MenuGetStartOfNextColumn(PROSMENUINFO MenuInfo)
{
  UINT i;
  PROSMENUITEMINFO MenuItems;

  i = MenuInfo->FocusedItem;
  if (NO_SELECTED_ITEM == i)
    {
      return i;
    }

  if (MenuGetAllRosMenuItemInfo(MenuInfo->Self, &MenuItems) <= 0)
    {
      return NO_SELECTED_ITEM;
    }

  for (i++ ; i < MenuInfo->MenuItemCount; i++)
    {
      if (0 != (MenuItems[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK)))
        {
          return i;
        }
    }

  return NO_SELECTED_ITEM;
}

/******************************************************************************
 *
 *   UINT MenuGetStartOfPrevColumn(PROSMENUINFO MenuInfo)
 */
static UINT FASTCALL
MenuGetStartOfPrevColumn(PROSMENUINFO MenuInfo)
{
  UINT i;
  PROSMENUITEMINFO MenuItems;

  if (0 == MenuInfo->FocusedItem || NO_SELECTED_ITEM == MenuInfo->FocusedItem)
    {
      return NO_SELECTED_ITEM;
    }

  if (MenuGetAllRosMenuItemInfo(MenuInfo->Self, &MenuItems) <= 0)
    {
      return NO_SELECTED_ITEM;
    }

  /* Find the start of the column */

  for (i = MenuInfo->FocusedItem;
       0 != i && 0 == (MenuItems[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK));
       --i)
    {
      ; /* empty */
    }

  if (0 == i)
    {
      MenuCleanupAllRosMenuItemInfo(MenuItems);
      return NO_SELECTED_ITEM;
    }

  for (--i; 0 != i; --i)
    {
      if (MenuItems[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
        {
          break;
        }
    }

  MenuCleanupAllRosMenuItemInfo(MenuItems);
  TRACE("ret %d.\n", i );

  return i;
}

/***********************************************************************
 *           MenuGetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static HMENU FASTCALL
MenuGetSubPopup(HMENU Menu)
{
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;

  if (! MenuGetRosMenuInfo(&MenuInfo, Menu)
      || NO_SELECTED_ITEM == MenuInfo.FocusedItem)
    {
      return NULL;
    }

  MenuInitRosMenuItemInfo(&ItemInfo);
  if (! MenuGetRosMenuItemInfo(MenuInfo.Self, MenuInfo.FocusedItem, &ItemInfo))
    {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return NULL;
    }
  if (0 != (ItemInfo.fType & MF_POPUP) && 0 != (ItemInfo.fState & MF_MOUSESELECT))
    {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return ItemInfo.hSubMenu;
    }

  MenuCleanupRosMenuItemInfo(&ItemInfo);
  return NULL;
}

/***********************************************************************
 *           MenuDoNextMenu
 *
 * NOTE: WM_NEXTMENU documented in Win32 is a bit different.
 */
static LRESULT FASTCALL
MenuDoNextMenu(MTRACKER* Mt, UINT Vk)
{
  ROSMENUINFO TopMenuInfo;
  ROSMENUINFO MenuInfo;

  if (! MenuGetRosMenuInfo(&TopMenuInfo, Mt->TopMenu))
    {
      return (LRESULT) FALSE;
    }

  if ((VK_LEFT == Vk && 0 == TopMenuInfo.FocusedItem)
      || (VK_RIGHT == Vk && TopMenuInfo.FocusedItem == TopMenuInfo.MenuItemCount - 1))
    {
      MDINEXTMENU NextMenu;
      HMENU NewMenu;
      HWND NewWnd;
      UINT Id = 0;

      NextMenu.hmenuIn = (IS_SYSTEM_MENU(&TopMenuInfo)) ? GetSubMenu(Mt->TopMenu, 0) : Mt->TopMenu;
      NextMenu.hmenuNext = NULL;
      NextMenu.hwndNext = NULL;
      SendMessageW(Mt->OwnerWnd, WM_NEXTMENU, Vk, (LPARAM) &NextMenu);

      TRACE("%p [%p] -> %p [%p]\n",
             Mt->CurrentMenu, Mt->OwnerWnd, NextMenu.hmenuNext, NextMenu.hwndNext );

      if (NULL == NextMenu.hmenuNext || NULL == NextMenu.hwndNext)
        {
          DWORD Style = GetWindowLongPtrW(Mt->OwnerWnd, GWL_STYLE);
          NewWnd = Mt->OwnerWnd;
          if (IS_SYSTEM_MENU(&TopMenuInfo))
            {
              /* switch to the menu bar */

              if (0 != (Style & WS_CHILD)
                  || NULL == (NewMenu = GetMenu(NewWnd)))
                {
                  return FALSE;
                }

              if (VK_LEFT == Vk)
                {
                  if (! MenuGetRosMenuInfo(&MenuInfo, NewMenu))
                    {
                      return FALSE;
                    }
                  Id = MenuInfo.MenuItemCount - 1;
                }
            }
          else if (0 != (Style & WS_SYSMENU))
            {
              /* switch to the system menu */
              NewMenu = NtUserGetSystemMenu(NewWnd, FALSE);
            }
          else
            {
              return FALSE;
            }
        }
      else    /* application returned a new menu to switch to */
        {
          NewMenu = NextMenu.hmenuNext;
          NewWnd = NextMenu.hwndNext;

          if (IsMenu(NewMenu) && IsWindow(NewWnd))
            {
              DWORD Style = GetWindowLongPtrW(NewWnd, GWL_STYLE);

              if (0 != (Style & WS_SYSMENU)
                  && GetSystemMenu(NewWnd, FALSE) == NewMenu)
                {
                  /* get the real system menu */
                  NewMenu = NtUserGetSystemMenu(NewWnd, FALSE);
                }
              else if (0 != (Style & WS_CHILD) || GetMenu(NewWnd) != NewMenu)
                {
                  /* FIXME: Not sure what to do here;
                   * perhaps try to track NewMenu as a popup? */

                  WARN(" -- got confused.\n");
                  return FALSE;
                }
            }
          else
            {
              return FALSE;
            }
        }

      if (NewMenu != Mt->TopMenu)
        {
          MenuSelectItem(Mt->OwnerWnd, &TopMenuInfo, NO_SELECTED_ITEM,
                         FALSE, 0 );
          if (Mt->CurrentMenu != Mt->TopMenu)
            {
              MenuHideSubPopups(Mt->OwnerWnd, &TopMenuInfo, FALSE);
            }
        }

      if (NewWnd != Mt->OwnerWnd)
        {
          Mt->OwnerWnd = NewWnd;
          SetCapture(Mt->OwnerWnd);
          (void)NtUserSetGUIThreadHandle(MSQ_STATE_MENUOWNER, Mt->OwnerWnd);
        }

      Mt->TopMenu = Mt->CurrentMenu = NewMenu; /* all subpopups are hidden */
      if (MenuGetRosMenuInfo(&TopMenuInfo, Mt->TopMenu))
        {
          MenuSelectItem(Mt->OwnerWnd, &TopMenuInfo, Id, TRUE, 0);
        }

      return TRUE;
    }

  return FALSE;
}

/***********************************************************************
 *           MenuSuspendPopup
 *
 * The idea is not to show the popup if the next input message is
 * going to hide it anyway.
 */
static BOOL FASTCALL
MenuSuspendPopup(MTRACKER* Mt, UINT Message)
{
  MSG Msg;

  Msg.hwnd = Mt->OwnerWnd;

  PeekMessageW(&Msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
  Mt->TrackFlags |= TF_SKIPREMOVE;

  switch (Message)
    {
      case WM_KEYDOWN:
        PeekMessageW(&Msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
        if (WM_KEYUP == Msg.message || WM_PAINT == Msg.message)
          {
            PeekMessageW(&Msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
            PeekMessageW(&Msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
            if (WM_KEYDOWN == Msg.message
                && (VK_LEFT == Msg.wParam || VK_RIGHT == Msg.wParam))
              {
                Mt->TrackFlags |= TF_SUSPENDPOPUP;
                return TRUE;
              }
          }
        break;
    }

  /* failures go through this */
  Mt->TrackFlags &= ~TF_SUSPENDPOPUP;

  return FALSE;
}

/***********************************************************************
 *           MenuKeyEscape
 *
 * Handle a VK_ESCAPE key event in a menu.
 */
static BOOL FASTCALL
MenuKeyEscape(MTRACKER *Mt, UINT Flags)
{
  BOOL EndMenu = TRUE;
  ROSMENUINFO MenuInfo;
  HMENU MenuTmp, MenuPrev;

  if (Mt->CurrentMenu != Mt->TopMenu)
    {
      if (MenuGetRosMenuInfo(&MenuInfo, Mt->CurrentMenu)
          && 0 != (MenuInfo.Flags & MF_POPUP))
        {
          MenuPrev = MenuTmp = Mt->TopMenu;

          /* close topmost popup */
          while (MenuTmp != Mt->CurrentMenu)
            {
              MenuPrev = MenuTmp;
              MenuTmp = MenuGetSubPopup(MenuPrev);
            }

          if (MenuGetRosMenuInfo(&MenuInfo, MenuPrev))
            {
              MenuHideSubPopups(Mt->OwnerWnd, &MenuInfo, TRUE);
            }
          Mt->CurrentMenu = MenuPrev;
          EndMenu = FALSE;
        }
    }

  return EndMenu;
}

/***********************************************************************
 *           MenuKeyLeft
 *
 * Handle a VK_LEFT key event in a menu.
 */
static void FASTCALL
MenuKeyLeft(MTRACKER* Mt, UINT Flags)
{
  ROSMENUINFO MenuInfo;
  ROSMENUINFO TopMenuInfo;
  ROSMENUINFO PrevMenuInfo;
  HMENU MenuTmp, MenuPrev;
  UINT PrevCol;

  MenuPrev = MenuTmp = Mt->TopMenu;

  if (! MenuGetRosMenuInfo(&MenuInfo, Mt->CurrentMenu))
    {
      return;
    }

  /* Try to move 1 column left (if possible) */
  if (NO_SELECTED_ITEM != (PrevCol = MenuGetStartOfPrevColumn(&MenuInfo)))
    {
      if (MenuGetRosMenuInfo(&MenuInfo, Mt->CurrentMenu))
        {
          MenuSelectItem(Mt->OwnerWnd, &MenuInfo, PrevCol, TRUE, 0);
        }
      return;
    }

  /* close topmost popup */
  while (MenuTmp != Mt->CurrentMenu)
    {
      MenuPrev = MenuTmp;
      MenuTmp = MenuGetSubPopup(MenuPrev);
    }

  if (! MenuGetRosMenuInfo(&PrevMenuInfo, MenuPrev))
    {
      return;
    }
  MenuHideSubPopups(Mt->OwnerWnd, &PrevMenuInfo, TRUE);
  Mt->CurrentMenu = MenuPrev;

  if (! MenuGetRosMenuInfo(&TopMenuInfo, Mt->TopMenu))
    {
      return;
    }
  if ((MenuPrev == Mt->TopMenu) && 0 == (TopMenuInfo.Flags & MF_POPUP))
    {
      /* move menu bar selection if no more popups are left */

      if (! MenuDoNextMenu(Mt, VK_LEFT))
        {
          MenuMoveSelection(Mt->OwnerWnd, &TopMenuInfo, ITEM_PREV);
        }

      if (MenuPrev != MenuTmp || 0 != (Mt->TrackFlags & TF_SUSPENDPOPUP))
        {
          /* A sublevel menu was displayed - display the next one
           * unless there is another displacement coming up */

          if (! MenuSuspendPopup(Mt, WM_KEYDOWN)
              && MenuGetRosMenuInfo(&TopMenuInfo, Mt->TopMenu))
            {
              Mt->CurrentMenu = MenuShowSubPopup(Mt->OwnerWnd, &TopMenuInfo,
                                                 TRUE, Flags);
            }
        }
    }
}

/***********************************************************************
 *           MenuKeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 */
static void FASTCALL
MenuKeyRight(MTRACKER *Mt, UINT Flags)
{
  HMENU MenuTmp;
  ROSMENUINFO MenuInfo;
  ROSMENUINFO CurrentMenuInfo;
  UINT NextCol;

  TRACE("MenuKeyRight called, cur %p, top %p.\n",
         Mt->CurrentMenu, Mt->TopMenu);

  if (! MenuGetRosMenuInfo(&MenuInfo, Mt->TopMenu))
    {
      return;
    }
  if (0 != (MenuInfo.Flags & MF_POPUP) || (Mt->CurrentMenu != Mt->TopMenu))
    {
      /* If already displaying a popup, try to display sub-popup */

      MenuTmp = Mt->CurrentMenu;
      if (MenuGetRosMenuInfo(&CurrentMenuInfo, Mt->CurrentMenu))
        {
          Mt->CurrentMenu = MenuShowSubPopup(Mt->OwnerWnd, &CurrentMenuInfo, TRUE, Flags);
        }

      /* if subpopup was displayed then we are done */
      if (MenuTmp != Mt->CurrentMenu)
        {
          return;
        }
    }

  if (! MenuGetRosMenuInfo(&CurrentMenuInfo, Mt->CurrentMenu))
    {
      return;
    }

  /* Check to see if there's another column */
  if (NO_SELECTED_ITEM != (NextCol = MenuGetStartOfNextColumn(&CurrentMenuInfo)))
    {
      TRACE("Going to %d.\n", NextCol);
      if (MenuGetRosMenuInfo(&MenuInfo, Mt->CurrentMenu))
        {
          MenuSelectItem(Mt->OwnerWnd, &MenuInfo, NextCol, TRUE, 0);
        }
      return;
    }

  if (0 == (MenuInfo.Flags & MF_POPUP))	/* menu bar tracking */
    {
      if (Mt->CurrentMenu != Mt->TopMenu)
        {
          MenuHideSubPopups(Mt->OwnerWnd, &MenuInfo, FALSE );
          MenuTmp = Mt->CurrentMenu = Mt->TopMenu;
        }
      else
        {
          MenuTmp = NULL;
        }

      /* try to move to the next item */
      if (! MenuDoNextMenu(Mt, VK_RIGHT))
        {
          MenuMoveSelection(Mt->OwnerWnd, &MenuInfo, ITEM_NEXT);
        }

      if (NULL != MenuTmp || 0 != (Mt->TrackFlags & TF_SUSPENDPOPUP))
        {
          if (! MenuSuspendPopup(Mt, WM_KEYDOWN)
              && MenuGetRosMenuInfo(&MenuInfo, Mt->TopMenu))
            {
              Mt->CurrentMenu = MenuShowSubPopup(Mt->OwnerWnd, &MenuInfo,
                                                 TRUE, Flags);
            }
        }
    }
}

/***********************************************************************
 *           MenuFindItemByKey
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
static UINT FASTCALL
MenuFindItemByKey(HWND WndOwner, PROSMENUINFO MenuInfo,
                  WCHAR Key, BOOL ForceMenuChar)
{
  ROSMENUINFO SysMenuInfo;
  PROSMENUITEMINFO Items, ItemInfo;
  LRESULT MenuChar;
  UINT i;

  TRACE("\tlooking for '%c' (0x%02x) in [%p]\n", (char) Key, Key, MenuInfo);

  if (NULL == MenuInfo || ! IsMenu(MenuInfo->Self))
    {
      if (MenuGetRosMenuInfo(&SysMenuInfo, GetSystemMenu(WndOwner, FALSE)))
        {
          MenuInfo = &SysMenuInfo;
        }
      else
        {
          MenuInfo = NULL;
        }
    }

  if (NULL != MenuInfo)
    {
      if (MenuGetAllRosMenuItemInfo(MenuInfo->Self, &Items) <= 0)
        {
          return -1;
        }
      if (! ForceMenuChar)
        {
          Key = toupperW(Key);
          ItemInfo = Items;
          for (i = 0; i < MenuInfo->MenuItemCount; i++, ItemInfo++)
            {
              if ((ItemInfo->Text) && NULL != ItemInfo->dwTypeData)
                {
                  WCHAR *p = (WCHAR *) ItemInfo->dwTypeData - 2;
                  do
                    {
                      p = strchrW(p + 2, '&');
		    }
                  while (NULL != p && L'&' == p[1]);
                  if (NULL != p && (toupperW(p[1]) == Key))
                    {
                      return i;
                    }
                }
            }
        }

      MenuChar = SendMessageW(WndOwner, WM_MENUCHAR,
                              MAKEWPARAM(Key, MenuInfo->Flags), (LPARAM) MenuInfo->Self);
      if (2 == HIWORD(MenuChar))
        {
          return LOWORD(MenuChar);
        }
      if (1 == HIWORD(MenuChar))
        {
          return (UINT) (-2);
        }
    }

  return (UINT)(-1);
}

/***********************************************************************
 *           MenuTrackMenu
 *
 * Menu tracking code.
 */
static INT FASTCALL
MenuTrackMenu(HMENU Menu, UINT Flags, INT x, INT y,
              HWND Wnd, const RECT *Rect )
{
  MSG Msg;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  BOOL fRemove;
  INT ExecutedMenuId = -1;
  MTRACKER Mt;
  BOOL EnterIdleSent = FALSE;

  Mt.TrackFlags = 0;
  Mt.CurrentMenu = Menu;
  Mt.TopMenu = Menu;
  Mt.OwnerWnd = Wnd;
  Mt.Pt.x = x;
  Mt.Pt.y = y;

  TRACE("Menu=%x Flags=0x%08x (%d,%d) Wnd=%x (%ld,%ld)-(%ld,%ld)\n",
         Menu, Flags, x, y, Wnd, Rect ? Rect->left : 0, Rect ? Rect->top : 0,
         Rect ? Rect->right : 0, Rect ? Rect->bottom : 0);

  fEndMenu = FALSE;
  if (! MenuGetRosMenuInfo(&MenuInfo, Menu))
    {
      return FALSE;
    }

  if (0 != (Flags & TPM_BUTTONDOWN))
    {
      /* Get the result in order to start the tracking or not */
      fRemove = MenuButtonDown(&Mt, Menu, Flags);
      fEndMenu = ! fRemove;
    }

  SetCapture(Mt.OwnerWnd);
  (void)NtUserSetGUIThreadHandle(MSQ_STATE_MENUOWNER, Mt.OwnerWnd);

  while (! fEndMenu)
    {
      /* we have to keep the message in the queue until it's
       * clear that menu loop is not over yet. */

      for (;;)
        {
          if (PeekMessageW(&Msg, 0, 0, 0, PM_NOREMOVE))
            {
              if (! CallMsgFilterW(&Msg, MSGF_MENU))
                {
                  break;
                }
              /* remove the message from the queue */
              PeekMessageW(&Msg, 0, Msg.message, Msg.message, PM_REMOVE );
            }
          else
            {
              if (! EnterIdleSent)
                {
                  HWND Win = (0 != (Flags & TPM_ENTERIDLEEX)
                              && 0 != (MenuInfo.Flags & MF_POPUP)) ? MenuInfo.Wnd : NULL;
                  EnterIdleSent = TRUE;
                  SendMessageW(Mt.OwnerWnd, WM_ENTERIDLE, MSGF_MENU, (LPARAM) Win);
                }
              WaitMessage();
            }
        }

      /* check if EndMenu() tried to cancel us, by posting this message */
      if (WM_CANCELMODE == Msg.message)
        {
          /* we are now out of the loop */
          fEndMenu = TRUE;

          /* remove the message from the queue */
          PeekMessageW(&Msg, 0, Msg.message, Msg.message, PM_REMOVE);

          /* break out of internal loop, ala ESCAPE */
          break;
        }

      TranslateMessage(&Msg);
      Mt.Pt = Msg.pt;

      if (Msg.hwnd == MenuInfo.Wnd || WM_TIMER != Msg.message)
        {
          EnterIdleSent = FALSE;
        }

      fRemove = FALSE;
      if (WM_MOUSEFIRST <= Msg.message && Msg.message <= WM_MOUSELAST)
        {
          /*
           * Use the mouse coordinates in lParam instead of those in the MSG
           * struct to properly handle synthetic messages. They are already
           * in screen coordinates.
           */
          Mt.Pt.x = (short) LOWORD(Msg.lParam);
          Mt.Pt.y = (short) HIWORD(Msg.lParam);

          /* Find a menu for this mouse event */
          Menu = MenuPtMenu(Mt.TopMenu, Mt.Pt);

          switch(Msg.message)
            {
              /* no WM_NC... messages in captured state */

              case WM_RBUTTONDBLCLK:
              case WM_RBUTTONDOWN:
                if (0 == (Flags & TPM_RIGHTBUTTON))
                  {
                    break;
                  }
                /* fall through */
              case WM_LBUTTONDBLCLK:
              case WM_LBUTTONDOWN:
                /* If the message belongs to the menu, removes it from the queue */
                /* Else, end menu tracking */
                fRemove = MenuButtonDown(&Mt, Menu, Flags);
                fEndMenu = ! fRemove;
                break;

              case WM_RBUTTONUP:
                if (0 == (Flags & TPM_RIGHTBUTTON))
                  {
                    break;
                  }
                /* fall through */
              case WM_LBUTTONUP:
                /* Check if a menu was selected by the mouse */
                if (NULL != Menu)
                  {
                    ExecutedMenuId = MenuButtonUp(&Mt, Menu, Flags);

                    /* End the loop if ExecutedMenuId is an item ID */
                    /* or if the job was done (ExecutedMenuId = 0). */
                    fEndMenu = fRemove = (-1 != ExecutedMenuId);
                  }
                else
                  {
                    /* No menu was selected by the mouse */
                    /* if the function was called by TrackPopupMenu, continue
                       with the menu tracking. If not, stop it */
                    fEndMenu = (0 != (Flags & TPM_POPUPMENU) ? FALSE : TRUE);
                  }
                break;

              case WM_MOUSEMOVE:
                if (Menu)
                  {
                    fEndMenu |= ! MenuMouseMove(&Mt, Menu, Flags);
                  }
                break;

	    } /* switch(Msg.message) - mouse */
	}
      else if (WM_KEYFIRST <= Msg.message && Msg.message <= WM_KEYLAST)
	{
          fRemove = TRUE;  /* Keyboard messages are always removed */
          switch(Msg.message)
            {
              case WM_SYSKEYDOWN:
              case WM_KEYDOWN:
                switch(Msg.wParam)
                  {
                    case VK_MENU:
                      fEndMenu = TRUE;
                      break;
                    case VK_HOME:
                    case VK_END:
                      if (MenuGetRosMenuInfo(&MenuInfo, Mt.CurrentMenu))
                        {
                          MenuSelectItem(Mt.OwnerWnd, &MenuInfo, NO_SELECTED_ITEM,
                                         FALSE, 0 );
                        }
                      /* fall through */

                    case VK_UP:
                      if (MenuGetRosMenuInfo(&MenuInfo, Mt.CurrentMenu))
                        {
                          MenuMoveSelection(Mt.OwnerWnd, &MenuInfo,
                                            VK_HOME == Msg.wParam ? ITEM_NEXT : ITEM_PREV);
                        }
                      break;

                    case VK_DOWN: /* If on menu bar, pull-down the menu */
                      if (MenuGetRosMenuInfo(&MenuInfo, Mt.CurrentMenu))
                        {
                          if (0 == (MenuInfo.Flags & MF_POPUP))
                            {
                              if (MenuGetRosMenuInfo(&MenuInfo, Mt.TopMenu))
                                {
                                  Mt.CurrentMenu = MenuShowSubPopup(Mt.OwnerWnd, &MenuInfo,
                                                                    TRUE, Flags);
                                }
                            }
                          else      /* otherwise try to move selection */
                            {
                              MenuMoveSelection(Mt.OwnerWnd, &MenuInfo, ITEM_NEXT);
                            }
                        }
                      break;

                    case VK_LEFT:
                      MenuKeyLeft(&Mt, Flags);
                      break;

                    case VK_RIGHT:
                      MenuKeyRight(&Mt, Flags);
                      break;

                    case VK_ESCAPE:
                      fEndMenu = MenuKeyEscape(&Mt, Flags);
                      break;

                    case VK_F1:
                      {
                        HELPINFO hi;
                        hi.cbSize = sizeof(HELPINFO);
                        hi.iContextType = HELPINFO_MENUITEM;
                        if (MenuGetRosMenuInfo(&MenuInfo, Mt.CurrentMenu))
                          {
                            if (NO_SELECTED_ITEM == MenuInfo.FocusedItem)
                              {
                                hi.iCtrlId = 0;
                              }
                            else
                              {
                                MenuInitRosMenuItemInfo(&ItemInfo);
                                if (MenuGetRosMenuItemInfo(MenuInfo.Self,
                                                           MenuInfo.FocusedItem,
                                                           &ItemInfo))
                                  {
                                    hi.iCtrlId = ItemInfo.wID;
                                  }
                                else
                                  {
                                    hi.iCtrlId = 0;
                                  }
                                MenuCleanupRosMenuItemInfo(&ItemInfo);
                              }
                          }
                        hi.hItemHandle = Menu;
			hi.dwContextId = MenuInfo.dwContextHelpID;
			hi.MousePos = Msg.pt;
			SendMessageW(Wnd, WM_HELP, 0, (LPARAM) &hi);
                        break;
                      }

                    default:
                      break;
                  }
                break;  /* WM_KEYDOWN */

              case WM_CHAR:
              case WM_SYSCHAR:
                {
                  UINT Pos;

                  if (! MenuGetRosMenuInfo(&MenuInfo, Mt.CurrentMenu))
                    {
                      break;
                    }
                  if (L'\r' == Msg.wParam || L' ' == Msg.wParam)
                    {
                      ExecutedMenuId = MenuExecFocusedItem(&Mt, &MenuInfo, Flags);
                      fEndMenu = (ExecutedMenuId != -2);
                      break;
                    }

                  /* Hack to avoid control chars. */
                  /* We will find a better way real soon... */
                  if (Msg.wParam < 32)
                    {
                      break;
                    }

                  Pos = MenuFindItemByKey(Mt.OwnerWnd, &MenuInfo,
                                          LOWORD(Msg.wParam), FALSE);
                  if ((UINT) -2 == Pos)
                    {
                      fEndMenu = TRUE;
                    }
                  else if ((UINT) -1 == Pos)
                    {
                      MessageBeep(0);
                    }
                  else
                    {
                      MenuSelectItem(Mt.OwnerWnd, &MenuInfo, Pos, TRUE, 0);
                      ExecutedMenuId = MenuExecFocusedItem(&Mt, &MenuInfo, Flags);
                      fEndMenu = (-2 != ExecutedMenuId);
                    }
		}
              break;
            }  /* switch(msg.message) - kbd */
        }
      else
        {
          PeekMessageW( &Msg, 0, Msg.message, Msg.message, PM_REMOVE );
          DispatchMessageW(&Msg);
          continue;
        }

      if (! fEndMenu)
        {
          fRemove = TRUE;
        }

      /* finally remove message from the queue */

      if (fRemove && 0 == (Mt.TrackFlags & TF_SKIPREMOVE))
        {
          PeekMessageW(&Msg, 0, Msg.message, Msg.message, PM_REMOVE);
        }
      else
        {
          Mt.TrackFlags &= ~TF_SKIPREMOVE;
        }
    }

  (void)NtUserSetGUIThreadHandle(MSQ_STATE_MENUOWNER, NULL);
  SetCapture(NULL);  /* release the capture */

  /* If dropdown is still painted and the close box is clicked on
     then the menu will be destroyed as part of the DispatchMessage above.
     This will then invalidate the menu handle in Mt.hTopMenu. We should
     check for this first.  */
  if (IsMenu(Mt.TopMenu))
    {
      if (IsWindow(Mt.OwnerWnd))
        {
          if (MenuGetRosMenuInfo(&MenuInfo, Mt.TopMenu))
            {
              MenuHideSubPopups(Mt.OwnerWnd, &MenuInfo, FALSE);

              if (0 != (MenuInfo.Flags & MF_POPUP))
                {
                  DestroyWindow(MenuInfo.Wnd);
                  MenuInfo.Wnd = NULL;
                }
              MenuSelectItem(Mt.OwnerWnd, &MenuInfo, NO_SELECTED_ITEM, FALSE, NULL);
            }

          SendMessageW(Mt.OwnerWnd, WM_MENUSELECT, MAKELONG(0, 0xffff), 0);
        }

      if (MenuGetRosMenuInfo(&MenuInfo, Mt.TopMenu))
        {
          /* Reset the variable for hiding menu */
          MenuInfo.TimeToHide = FALSE;
          MenuSetRosMenuInfo(&MenuInfo);
        }
    }

  /* The return value is only used by TrackPopupMenu */
  if (!(Flags & TPM_RETURNCMD)) return TRUE;
  if (ExecutedMenuId < 0) ExecutedMenuId = 0;
  return ExecutedMenuId;
}

/***********************************************************************
 *           MenuExitTracking
 */
static BOOL FASTCALL
MenuExitTracking(HWND Wnd)
{
  TRACE("hwnd=%p\n", Wnd);

  SendMessageW(Wnd, WM_EXITMENULOOP, 0, 0);
  ShowCaret(0);
  return TRUE;
}


VOID
MenuTrackMouseMenuBar(HWND Wnd, ULONG Ht, POINT Pt)
{
  HMENU Menu = (HTSYSMENU == Ht) ? NtUserGetSystemMenu(Wnd, FALSE) : GetMenu(Wnd);
  UINT Flags = TPM_ENTERIDLEEX | TPM_BUTTONDOWN | TPM_LEFTALIGN | TPM_LEFTBUTTON;

  TRACE("wnd=%p ht=0x%04x (%ld,%ld)\n", Wnd, Ht, Pt.x, Pt.y);

  if (IsMenu(Menu))
    {
      /* map point to parent client coordinates */
      HWND Parent = GetAncestor(Wnd, GA_PARENT );
      if (Parent != GetDesktopWindow())
        {
          ScreenToClient(Parent, &Pt);
        }

      MenuInitTracking(Wnd, Menu, FALSE, Flags);
      MenuTrackMenu(Menu, Flags, Pt.x, Pt.y, Wnd, NULL);
      MenuExitTracking(Wnd);
    }
}


VOID
MenuTrackKbdMenuBar(HWND hWnd, UINT wParam, WCHAR wChar)
{
    UINT uItem = NO_SELECTED_ITEM;
    HMENU hTrackMenu;
    ROSMENUINFO MenuInfo;
    UINT wFlags = TPM_ENTERIDLEEX | TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE("hwnd %p wParam 0x%04x wChar 0x%04x\n", hWnd, wParam, wChar);

    /* find window that has a menu */

    while (!((GetWindowLongPtrW( hWnd, GWL_STYLE ) &
                                         (WS_CHILD | WS_POPUP)) != WS_CHILD))
        if (!(hWnd = GetAncestor( hWnd, GA_PARENT ))) return;

    /* check if we have to track a system menu */

    hTrackMenu = GetMenu( hWnd );
    if (!hTrackMenu || IsIconic(hWnd) || wChar == ' ' )
    {
        if (!(GetWindowLongPtrW( hWnd, GWL_STYLE ) & WS_SYSMENU)) return;
        hTrackMenu = NtUserGetSystemMenu(hWnd, FALSE);
        uItem = 0;
        wParam |= HTSYSMENU; /* prevent item lookup */
    }

    if (!IsMenu( hTrackMenu )) return;

    MenuInitTracking( hWnd, hTrackMenu, FALSE, wFlags );

    if (! MenuGetRosMenuInfo(&MenuInfo, hTrackMenu))
    {
      goto track_menu;
    }

    if( wChar && wChar != ' ' )
    {
        uItem = MenuFindItemByKey( hWnd, &MenuInfo, wChar, (wParam & HTSYSMENU) );
        if ( uItem >= (UINT)(-2) )
        {
            if( uItem == (UINT)(-1) ) MessageBeep(0);
            /* schedule end of menu tracking */
            wFlags |= TF_ENDMENU;
            goto track_menu;
        }
    }

    MenuSelectItem( hWnd, &MenuInfo, uItem, TRUE, 0 );

    if (wParam & HTSYSMENU)
    {
        /* prevent sysmenu activation for managed windows on Alt down/up */
//        if (GetPropA( hwnd, "__wine_x11_managed" ))
            wFlags |= TF_ENDMENU; /* schedule end of menu tracking */
    }
    else
    {
        if( uItem == NO_SELECTED_ITEM )
            MenuMoveSelection( hWnd, &MenuInfo, ITEM_NEXT );
        else
            PostMessageW( hWnd, WM_KEYDOWN, VK_DOWN, 0L );
    }

track_menu:
    MenuTrackMenu( hTrackMenu, wFlags, 0, 0, hWnd, NULL );
    MenuExitTracking( hWnd );

}

/*
 *  From MSDN:
 *  The MFT_BITMAP, MFT_SEPARATOR, and MFT_STRING values cannot be combined
 *  with one another. Also MFT_OWNERDRAW. Set fMask to MIIM_TYPE to use fType.
 *
 *  Windows 2K/XP: fType is used only if fMask has a value of MIIM_FTYPE.
 *
 *  MIIM_TYPE: Retrieves or sets the fType and dwTypeData members. Windows
 *  2K/XP: MIIM_TYPE is replaced by  MIIM_BITMAP, MIIM_FTYPE, and MIIM_STRING.
 *  MFT_STRING is replaced by MIIM_STRING.
 *  (So, I guess we should use MIIM_STRING only for strings?)
 *
 *  MIIM_FTYPE: Windows 2K/Windows XP: Retrieves or sets the fType member.
 *
 *  Based on wine, SetMenuItemInfo_common:
 *  1) set MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP any one with MIIM_TYPE,
 *     it will result in a error.
 *  2) set menu mask to MIIM_FTYPE and MFT_BITMAP ftype it will result in a error.
 *     These conditions are addressed in Win32k IntSetMenuItemInfo.
 *
 */
static
BOOL
FASTCALL
MenuSetItemData(
  LPMENUITEMINFOW mii,
  UINT Flags,
  UINT_PTR IDNewItem,
  LPCWSTR NewItem,
  BOOL Unicode)
{
/*
 * Let us assume MIIM_FTYPE is set and building a new menu item structure.
 */
  if(Flags & MF_BITMAP)
  {
     mii->fMask |= MIIM_BITMAP;   /* Use the new way of seting hbmpItem.*/
     mii->hbmpItem = (HBITMAP) NewItem;

     if (Flags & MF_HELP)
     {
         /* increase ident */
         mii->fType |= MF_HELP;
     }
  }
  else if(Flags & MF_OWNERDRAW)
  {
    mii->fType |= MFT_OWNERDRAW;
    mii->fMask |= MIIM_DATA;
    mii->dwItemData = (DWORD) NewItem;
  }
  else if (Flags & MF_SEPARATOR)
  {
    mii->fType |= MFT_SEPARATOR;
    if (!(Flags & (MF_GRAYED|MF_DISABLED)))
      Flags |= MF_GRAYED|MF_DISABLED;
  }
  else /* Default action MF_STRING. */
  {
    /* Item beginning with a backspace is a help item */
    if (NewItem != NULL)
    {
       if (Unicode)
       {
          if (*NewItem == '\b')
          {
             mii->fType |= MF_HELP;
             NewItem++;
          }
       }
       else
       {
          LPCSTR NewItemA = (LPCSTR) NewItem;
          if (*NewItemA == '\b')
          {
             mii->fType |= MF_HELP;
             NewItemA++;
             NewItem = (LPCWSTR) NewItemA;
          }
       }

       if (Flags & MF_HELP)
         mii->fType |= MF_HELP;
       mii->fMask |= MIIM_STRING;
       mii->fType |= MFT_STRING; /* Zero */
       mii->dwTypeData = (LPWSTR)NewItem;
       if (Unicode)
         mii->cch = (NULL == NewItem ? 0 : strlenW(NewItem));
       else
         mii->cch = (NULL == NewItem ? 0 : strlen((LPCSTR)NewItem));
    }
    else
    {
      mii->fType |= MFT_SEPARATOR;
      if (!(Flags & (MF_GRAYED|MF_DISABLED)))
        Flags |= MF_GRAYED|MF_DISABLED;
    }
  }

  if(Flags & MF_RIGHTJUSTIFY) /* Same as MF_HELP */
  {
    mii->fType |= MFT_RIGHTJUSTIFY;
  }

  if(Flags & MF_MENUBREAK)
  {
    mii->fType |= MFT_MENUBREAK;
  }
  else if(Flags & MF_MENUBARBREAK)
  {
    mii->fType |= MFT_MENUBARBREAK;
  }

  if(Flags & MF_GRAYED || Flags & MF_DISABLED)
  {
    if (Flags & MF_GRAYED)
      mii->fState |= MF_GRAYED;

    if (Flags & MF_DISABLED)
      mii->fState |= MF_DISABLED;

    mii->fMask |= MIIM_STATE;
  }
  else if (Flags & MF_HILITE)
  {
    mii->fState |= MF_HILITE;
    mii->fMask |= MIIM_STATE;
  }
  else /* default state */
  {
    mii->fState |= MFS_ENABLED;
    mii->fMask |= MIIM_STATE;
  }

  if(Flags & MF_POPUP)
  {
    mii->fType |= MF_POPUP;
    mii->fMask |= MIIM_SUBMENU;
    mii->hSubMenu = (HMENU)IDNewItem;
  }
  else
  {
    mii->fMask |= MIIM_ID;
    mii->wID = (UINT)IDNewItem;
  }
  return TRUE;
}


/* FUNCTIONS *****************************************************************/

/*static BOOL
MenuIsStringItem(ULONG TypeData)
{
  return(MF_STRING == MENU_ITEM_TYPE(ItemInfo->fType));
}*/


/*
 * @implemented
 */
BOOL WINAPI
AppendMenuA(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCSTR lpNewItem)
{
  return(InsertMenuA(hMenu, -1, uFlags | MF_BYPOSITION, uIDNewItem,
		     lpNewItem));
}


/*
 * @implemented
 */
BOOL WINAPI
AppendMenuW(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCWSTR lpNewItem)
{
  return(InsertMenuW(hMenu, -1, uFlags | MF_BYPOSITION, uIDNewItem,
		     lpNewItem));
}


/*
 * @implemented
 */
DWORD WINAPI
CheckMenuItem(HMENU hmenu,
	      UINT uIDCheckItem,
	      UINT uCheck)
{
  return NtUserCheckMenuItem(hmenu, uIDCheckItem, uCheck);
}

static
BOOL
MenuCheckMenuRadioItem(HMENU hMenu, UINT idFirst, UINT idLast, UINT idCheck, UINT uFlags, BOOL bCheck, PUINT pChecked, PUINT pUnchecked, PUINT pMenuChanged)
{
  UINT ItemCount, i;
  PROSMENUITEMINFO Items = NULL;
  UINT cChecked, cUnchecked;
  BOOL bRet = TRUE;
  //ROSMENUINFO mi;

  if(idFirst > idLast)
      return FALSE;

  ItemCount = GetMenuItemCount(hMenu);

  //mi.cbSize = sizeof(ROSMENUINFO);
  //if(!NtUserMenuInfo(hmenu, &mi, FALSE)) return ret;


  if(MenuGetAllRosMenuItemInfo(hMenu, &Items) <= 0)
  {
    ERR("MenuGetAllRosMenuItemInfo failed\n");
    return FALSE;
  }

  cChecked = cUnchecked = 0;

  for (i = 0 ; i < ItemCount; i++)
  {
    BOOL check = FALSE;
    if (0 != (Items[i].fType & MF_MENUBARBREAK)) continue;
    if (0 != (Items[i].fType & MF_SEPARATOR)) continue;

    if ((Items[i].fType & MF_POPUP) && (uFlags == MF_BYCOMMAND))
    {
      MenuCheckMenuRadioItem(Items[i].hSubMenu, idFirst, idLast, idCheck, uFlags, bCheck, pChecked, pUnchecked, pMenuChanged);
      continue;
    }
    if (uFlags & MF_BYPOSITION)
    {
      if (i < idFirst || i > idLast)
        continue;

      if (i == idCheck)
      {
        cChecked++;
        check = TRUE;
      }
      else
      {
        cUnchecked++;
      }
    }
      else
      {
        if (Items[i].wID < idFirst || Items[i].wID > idLast)
          continue;

        if (Items[i].wID == idCheck)
        {
          cChecked++;
          check = TRUE;
        }
        else
        {
          cUnchecked++;
        }
      }

      if (!bCheck)
        continue;

      Items[i].fMask = MIIM_STATE | MIIM_FTYPE;
      if (check)
      {
        Items[i].fType |= MFT_RADIOCHECK;
        Items[i].fState |= MFS_CHECKED;
      }
      else
      {
        Items[i].fState &= ~MFS_CHECKED;
      }

      if(!MenuSetRosMenuItemInfo(hMenu, i ,&Items[i]))
      {
        ERR("MenuSetRosMenuItemInfo failed\n");
        bRet = FALSE;
        break;
      }
  }
  HeapFree(GetProcessHeap(), 0, Items);

  *pChecked += cChecked;
  *pUnchecked += cUnchecked;

  if (cChecked || cUnchecked)
    (*pMenuChanged)++;

  return bRet;
}

/*
 * @implemented
 */
BOOL WINAPI
CheckMenuRadioItem(HMENU hmenu,
		   UINT idFirst,
		   UINT idLast,
		   UINT idCheck,
		   UINT uFlags)
{
  UINT cChecked = 0;
  UINT cUnchecked = 0;
  UINT cMenuChanged = 0;

  if (!MenuCheckMenuRadioItem(hmenu, idFirst, idLast, idCheck, uFlags, FALSE, &cChecked, &cUnchecked, &cMenuChanged))
    return FALSE;

  if (cMenuChanged > 1)
    return FALSE;

  cMenuChanged = 0;
  cChecked = 0;
  cUnchecked = 0;

  if (!MenuCheckMenuRadioItem(hmenu, idFirst, idLast, idCheck, uFlags, TRUE, &cChecked, &cUnchecked, &cMenuChanged))
    return FALSE;

  return (cChecked != 0);
}


/*
 * @implemented
 */
HMENU WINAPI
CreateMenu(VOID)
{
  MenuLoadBitmaps();
  return (HMENU)NtUserCallNoParam(NOPARAM_ROUTINE_CREATEMENU);
}


/*
 * @implemented
 */
HMENU WINAPI
CreatePopupMenu(VOID)
{
  MenuLoadBitmaps();
  return (HMENU)NtUserCallNoParam(NOPARAM_ROUTINE_CREATEMENUPOPUP);
}


/*
 * @implemented
 */
BOOL WINAPI
DrawMenuBar(HWND hWnd)
{
//  return (BOOL)NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_DRAWMENUBAR);
  ROSMENUINFO MenuInfo;
  HMENU hMenu;
  hMenu = GetMenu(hWnd);
  if (!hMenu)
     return FALSE;
  MenuGetRosMenuInfo(&MenuInfo, hMenu);
  MenuInfo.Height = 0; // make sure to recalc size
  MenuSetRosMenuInfo(&MenuInfo);
  /* The wine method doesn't work and I suspect it's more effort
     then hackfix solution
  SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                  SWP_NOZORDER | SWP_FRAMECHANGED );
  return TRUE;*/
  // FIXME: hackfix
  DefWndNCPaint(hWnd,(HRGN)-1,-1);
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
EnableMenuItem(HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable)
{
  return NtUserEnableMenuItem(hMenu, uIDEnableItem, uEnable);
}

/*
 * @implemented
 */
BOOL WINAPI
EndMenu(VOID)
{
  GUITHREADINFO guii;
  guii.cbSize = sizeof(GUITHREADINFO);
  if(GetGUIThreadInfo(GetCurrentThreadId(), &guii) && guii.hwndMenuOwner)
  {
    PostMessageW(guii.hwndMenuOwner, WM_CANCELMODE, 0, 0);
  }
  return TRUE;
}


/*
 * @implemented
 */
HMENU WINAPI
GetMenu(HWND hWnd)
{
  return NtUserGetMenu(hWnd);
}


/*
 * @implemented
 */
LONG WINAPI
GetMenuCheckMarkDimensions(VOID)
{
  return(MAKELONG(GetSystemMetrics(SM_CXMENUCHECK),
		  GetSystemMetrics(SM_CYMENUCHECK)));
}


/*
 * @implemented
 */
UINT WINAPI
GetMenuDefaultItem(HMENU hMenu,
		   UINT fByPos,
		   UINT gmdiFlags)
{
  return NtUserGetMenuDefaultItem(hMenu, fByPos, gmdiFlags);
}


/*
 * @implemented
 */
BOOL WINAPI
GetMenuInfo(HMENU hmenu,
	    LPMENUINFO lpcmi)
{
  ROSMENUINFO mi;
  BOOL res = FALSE;

  if(!lpcmi || (lpcmi->cbSize != sizeof(MENUINFO)))
    return FALSE;

  RtlZeroMemory(&mi, sizeof(MENUINFO));
  mi.cbSize = sizeof(MENUINFO);
  mi.fMask = lpcmi->fMask;

  res = NtUserMenuInfo(hmenu, &mi, FALSE);

  memcpy(lpcmi, &mi, sizeof(MENUINFO));
  return res;
}


/*
 * @implemented
 */
int WINAPI
GetMenuItemCount(HMENU Menu)
{
  ROSMENUINFO MenuInfo;

  return MenuGetRosMenuInfo(&MenuInfo, Menu) ? MenuInfo.MenuItemCount : 0;
}


/*
 * @implemented
 */
UINT WINAPI
GetMenuItemID(HMENU hMenu,
	      int nPos)
{
  ROSMENUITEMINFO mii;

  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_ID | MIIM_SUBMENU;

  if (! NtUserMenuItemInfo(hMenu, nPos, MF_BYPOSITION, &mii, FALSE))
    {
      return -1;
    }

  if (NULL != mii.hSubMenu)
    {
      return -1;
    }
  if (0 == mii.wID)
    {
      return -1;
    }

  return mii.wID;
}


/*
 * @implemented
 */
BOOL WINAPI
GetMenuItemInfoA(
   HMENU Menu,
   UINT Item,
   BOOL ByPosition,
   LPMENUITEMINFOA mii)
{
   MENUITEMINFOW miiW;
   LPSTR AnsiBuffer;
   INT Count;

   if (mii->cbSize != sizeof(MENUITEMINFOA) &&
       mii->cbSize != sizeof(MENUITEMINFOA) - sizeof(HBITMAP))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if(!(mii->fMask & (MIIM_TYPE | MIIM_STRING)))
   {
      /* No text requested, just pass on */
      return NtUserMenuItemInfo(Menu, Item, ByPosition, (PROSMENUITEMINFO) mii, FALSE);
   }

   AnsiBuffer = mii->dwTypeData;
   Count = miiW.cch = mii->cch;
   RtlCopyMemory(&miiW, mii, mii->cbSize);
   miiW.dwTypeData = 0;

   if (AnsiBuffer)
   {
      miiW.dwTypeData = RtlAllocateHeap(GetProcessHeap(), 0,
                                        miiW.cch * sizeof(WCHAR));
      if (miiW.dwTypeData == NULL) return FALSE;
      miiW.dwTypeData[0] = 0;
   }

   if (!NtUserMenuItemInfo(Menu, Item, ByPosition, (PROSMENUITEMINFO)&miiW, FALSE))
   {
      if (miiW.dwTypeData) RtlFreeHeap(GetProcessHeap(), 0, miiW.dwTypeData);
      return FALSE;
   }

   RtlCopyMemory(mii, &miiW, miiW.cbSize);

   if (!AnsiBuffer || !Count)
   {
      if (miiW.dwTypeData) RtlFreeHeap(GetProcessHeap(), 0, miiW.dwTypeData);
      mii->dwTypeData = AnsiBuffer;
      mii->cch = miiW.cch;
      return TRUE;
   }

   if ((miiW.fMask & MIIM_STRING) || (IS_STRING_ITEM(miiW.fType)))
   {
      if (miiW.cch)
      {
         if (!WideCharToMultiByte(CP_ACP, 0, miiW.dwTypeData, miiW.cch, AnsiBuffer, mii->cch, NULL, NULL))
         {
            AnsiBuffer[0] = 0;
         }
         if (Count > miiW.cch)
         {
            AnsiBuffer[miiW.cch] = 0;
         }
         mii->cch = mii->cch;
      }
   }
   else
   {
      AnsiBuffer[0] = 0;
   }

   RtlFreeHeap(GetProcessHeap(), 0, miiW.dwTypeData);
   mii->dwTypeData = AnsiBuffer;

   return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
GetMenuItemInfoW(
   HMENU Menu,
   UINT Item,
   BOOL ByPosition,
   LPMENUITEMINFOW mii)
{
   MENUITEMINFOW miiW;
   LPWSTR String;
   INT Count;

   if (mii->cbSize != sizeof(MENUITEMINFOW) &&
       mii->cbSize != sizeof(MENUITEMINFOW) - sizeof(HBITMAP))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if(!(mii->fMask & (MIIM_TYPE | MIIM_STRING)))
   {
      /* No text requested, just pass on */
      return NtUserMenuItemInfo(Menu, Item, ByPosition, (PROSMENUITEMINFO) mii, FALSE);
   }

   String = mii->dwTypeData;
   Count = mii->cch;
   RtlCopyMemory(&miiW, mii, mii->cbSize);
   miiW.dwTypeData = 0;

   if (String)
   {
      miiW.dwTypeData = RtlAllocateHeap(GetProcessHeap(), 0,
                                        miiW.cch * sizeof(WCHAR));
      if (miiW.dwTypeData == NULL) return FALSE;
      miiW.dwTypeData[0] = 0;
   }

   if (!NtUserMenuItemInfo(Menu, Item, ByPosition, (PROSMENUITEMINFO) &miiW, FALSE))
   {
      if (miiW.dwTypeData) RtlFreeHeap(GetProcessHeap(), 0, miiW.dwTypeData);
      return FALSE;
   }

   RtlCopyMemory(mii, &miiW, miiW.cbSize); // Okay to over write user data.

   if (!String || !Count)
   {
      if (miiW.dwTypeData) RtlFreeHeap(GetProcessHeap(), 0, miiW.dwTypeData);
      mii->dwTypeData = String; // may not be zero.
      mii->cch = miiW.cch;
      return TRUE;
   }

   if ((miiW.fMask & MIIM_STRING) || (IS_STRING_ITEM(miiW.fType)))
   {
      lstrcpynW( String, miiW.dwTypeData, Count );
   }

   RtlFreeHeap(GetProcessHeap(), 0, miiW.dwTypeData);
   mii->dwTypeData = String;
   mii->cch = strlenW(String);
   return TRUE;
}


/*
 * @implemented
 */
UINT
WINAPI
GetMenuState(
  HMENU hMenu,
  UINT uId,
  UINT uFlags)
{
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO mii;
  memset( &mii, 0, sizeof(mii) );
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_STATE | MIIM_FTYPE | MIIM_SUBMENU;

  SetLastError(0);
  if(NtUserMenuItemInfo(hMenu, uId, uFlags, &mii, FALSE))
    {
      UINT nSubItems = 0;
      if(mii.hSubMenu)
        {
          if (! MenuGetRosMenuInfo(&MenuInfo, mii.hSubMenu))
            {
              return (UINT) -1;
            }
          nSubItems = MenuInfo.MenuItemCount;

          /* FIXME - ported from wine, does that work (0xff)? */
          if(GetLastError() != ERROR_INVALID_MENU_HANDLE)
            return (nSubItems << 8) | ((mii.fState | mii.fType) & 0xff);

          return (UINT)-1; /* Invalid submenu */
        }

      /* FIXME - ported from wine, does that work? */
      return (mii.fType | mii.fState);
    }

  return (UINT)-1;
}


/*
 * @implemented
 */
int
WINAPI
GetMenuStringA(
  HMENU hMenu,
  UINT uIDItem,
  LPSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  MENUITEMINFOA mii;
  memset( &mii, 0, sizeof(mii) );
  mii.dwTypeData = lpString;
  mii.fMask = MIIM_STRING | MIIM_FTYPE;
  mii.fType = MFT_STRING;
  mii.cbSize = sizeof(MENUITEMINFOA);
  mii.cch = nMaxCount;

  if(!(GetMenuItemInfoA( hMenu, uIDItem, (BOOL)(MF_BYPOSITION & uFlag),&mii)))
     return 0;
  else
     return mii.cch;
}


/*
 * @implemented
 */
int
WINAPI
GetMenuStringW(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  MENUITEMINFOW miiW;
  memset( &miiW, 0, sizeof(miiW) );
  miiW.dwTypeData = lpString;
  miiW.fMask = MIIM_STRING | MIIM_FTYPE;
  miiW.fType = MFT_STRING;
  miiW.cbSize = sizeof(MENUITEMINFOW);
  miiW.cch = nMaxCount;

  if(!(GetMenuItemInfoW( hMenu, uIDItem, (BOOL)(MF_BYPOSITION & uFlag),&miiW)))
     return 0;
  else
     return miiW.cch;
}


/*
 * @implemented
 */
HMENU
WINAPI
GetSubMenu(
  HMENU hMenu,
  int nPos)
{
  ROSMENUITEMINFO mi;

  mi.cbSize = sizeof(MENUITEMINFOW);
  mi.fMask = MIIM_SUBMENU;

  if (NtUserMenuItemInfo(hMenu, (UINT)nPos, MF_BYPOSITION, &mi, FALSE))
    {
      return IsMenu(mi.hSubMenu) ? mi.hSubMenu : NULL;
    }

  return NULL;
}

/*
 * @implemented
 */
HMENU
WINAPI
GetSystemMenu(
  HWND hWnd,
  BOOL bRevert)
{
  HMENU TopMenu;

  TopMenu = NtUserGetSystemMenu(hWnd, bRevert);

  return NULL == TopMenu ? NULL : GetSubMenu(TopMenu, 0);
}


/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  MENUITEMINFOA mii;
  memset( &mii, 0, sizeof(mii) );
  mii.cbSize = sizeof(MENUITEMINFOA);
  mii.fMask = MIIM_FTYPE;

  MenuSetItemData((LPMENUITEMINFOW) &mii,
  		   uFlags,
  		   uIDNewItem,
  		  (LPCWSTR) lpNewItem,
  		   FALSE);

  return InsertMenuItemA(hMenu, uPosition, (BOOL)((MF_BYPOSITION & uFlags) > 0), &mii);
}



/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuItemA(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOA lpmii)
{
  MENUITEMINFOW mi;
  UNICODE_STRING MenuText;
  BOOL res = FALSE;
  BOOL CleanHeap = FALSE;
  NTSTATUS Status;

  if((lpmii->cbSize == sizeof(MENUITEMINFOA)) ||
     (lpmii->cbSize == sizeof(MENUITEMINFOA) - sizeof(HBITMAP)))
  {
    RtlCopyMemory ( &mi, lpmii, lpmii->cbSize );

    if( lpmii->cbSize != sizeof( MENUITEMINFOW))
    {
       mi.cbSize = sizeof( MENUITEMINFOW);
       mi.hbmpItem = NULL;
    }
    /* copy the text string */
    if (((mi.fMask & MIIM_STRING) ||
        ((mi.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mi.fType) == MF_STRING)))
          && mi.dwTypeData != NULL)
    {
      Status = RtlCreateUnicodeStringFromAsciiz(&MenuText, (LPSTR)mi.dwTypeData);
      if (!NT_SUCCESS (Status))
      {
        SetLastError (RtlNtStatusToDosError(Status));
        return FALSE;
      }
      mi.dwTypeData = MenuText.Buffer;
      mi.cch = MenuText.Length / sizeof(WCHAR);
      CleanHeap = TRUE;
    }
    res = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, TRUE, &mi, NULL);

    if ( CleanHeap ) RtlFreeUnicodeString ( &MenuText );
  }
  return res;
}


/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuItemW(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii)
{
  MENUITEMINFOW mi;
  UNICODE_STRING MenuText;
  BOOL res = FALSE;

  /* while we could just pass 'lpmii' to win32k, we make a copy so that
     if a bad user passes bad data, we crash his process instead of the
     entire kernel */

  if((lpmii->cbSize == sizeof(MENUITEMINFOW)) ||
     (lpmii->cbSize == sizeof(MENUITEMINFOW) - sizeof(HBITMAP)))
  {
    RtlCopyMemory(&mi, lpmii, lpmii->cbSize);

    if( lpmii->cbSize != sizeof( MENUITEMINFOW))
    {
       mi.cbSize = sizeof( MENUITEMINFOW);
       mi.hbmpItem = NULL;
    }
    /* copy the text string */
    if (((mi.fMask & MIIM_STRING) ||
        ((mi.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mi.fType) == MF_STRING)))
          && mi.dwTypeData != NULL)
    {
      RtlInitUnicodeString(&MenuText, (PWSTR)lpmii->dwTypeData);
      mi.dwTypeData = MenuText.Buffer;
      mi.cch = MenuText.Length / sizeof(WCHAR);
    }
    res = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, TRUE, &mi, NULL);
  }
  return res;
}


/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  MENUITEMINFOW mii;
  memset( &mii, 0, sizeof(mii) );
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_FTYPE;

  MenuSetItemData( &mii,
  		   uFlags,
  		   uIDNewItem,
  		   lpNewItem,
  		   TRUE);

  return InsertMenuItemW(hMenu, uPosition, (BOOL)((MF_BYPOSITION & uFlags) > 0), &mii);
}


/*
 * @implemented
 */
BOOL
WINAPI
IsMenu(
  HMENU Menu)
{
  ROSMENUINFO MenuInfo;

  return MenuGetRosMenuInfo(&MenuInfo, Menu);
}


/*
 * @implemented
 */
HMENU WINAPI
LoadMenuA(HINSTANCE hInstance,
	  LPCSTR lpMenuName)
{
  HANDLE Resource = FindResourceA(hInstance, lpMenuName, MAKEINTRESOURCEA(4));
  if (Resource == NULL)
    {
      return(NULL);
    }
  return(LoadMenuIndirectA((PVOID)LoadResource(hInstance, Resource)));
}


/*
 * @implemented
 */
HMENU WINAPI
LoadMenuIndirectA(CONST MENUTEMPLATE *lpMenuTemplate)
{
  return(LoadMenuIndirectW(lpMenuTemplate));
}


/*
 * @implemented
 */
HMENU WINAPI
LoadMenuIndirectW(CONST MENUTEMPLATE *lpMenuTemplate)
{
  HMENU hMenu;
  WORD version, offset;
  LPCSTR p = (LPCSTR)lpMenuTemplate;

  version = GET_WORD(p);
  p += sizeof(WORD);

  switch (version)
  {
    case 0: /* standard format is version of 0 */
      offset = GET_WORD(p);
      p += sizeof(WORD) + offset;
      if (!(hMenu = CreateMenu())) return 0;
      if (!MENU_ParseResource(p, hMenu, TRUE))
      {
        DestroyMenu(hMenu);
        return 0;
      }
      return hMenu;
    case 1: /* extended format is version of 1 */
      offset = GET_WORD(p);
      p += sizeof(WORD) + offset;
      if (!(hMenu = CreateMenu())) return 0;
      if (!MENUEX_ParseResource(p, hMenu))
      {
        DestroyMenu( hMenu );
        return 0;
      }
      return hMenu;
    default:
      DbgPrint("LoadMenuIndirectW(): version %d not supported.\n", version);
      return 0;
  }
}


/*
 * @implemented
 */
HMENU WINAPI
LoadMenuW(HINSTANCE hInstance,
	  LPCWSTR lpMenuName)
{
  HANDLE Resource = FindResourceW(hInstance, lpMenuName, RT_MENU);
  if (Resource == NULL)
    {
      return(NULL);
    }
  return(LoadMenuIndirectW((PVOID)LoadResource(hInstance, Resource)));
}


/*
 * @implemented
 */
int
WINAPI
MenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  POINT ptScreen)
{
  return NtUserMenuItemFromPoint(hWnd, hMenu, ptScreen.x, ptScreen.y);
}


/*
 * @implemented
 */
BOOL
WINAPI
ModifyMenuA(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  ROSMENUINFO mi;
  ROSMENUITEMINFO rmii;
  MENUITEMINFOA mii;
  memset( &mii, 0, sizeof(mii) );
  mii.cbSize = sizeof(MENUITEMINFOA);
  mii.fMask = MIIM_FTYPE;

  if (!MenuGetRosMenuInfo( &mi, hMnu )) return FALSE;

  mi.Height = 0;

  if (!MenuSetRosMenuInfo( &mi )) return FALSE;

  MenuInitRosMenuItemInfo( &rmii );

  if(!MenuGetRosMenuItemInfo( hMnu, uPosition, &rmii)) return FALSE;

  if ((rmii.fType & MF_POPUP) && (uFlags & MF_POPUP) && (rmii.hSubMenu != (HMENU)uIDNewItem))
    NtUserDestroyMenu( rmii.hSubMenu );   /* ModifyMenu() spec */

  MenuCleanupRosMenuItemInfo( &rmii );

  MenuSetItemData((LPMENUITEMINFOW) &mii,
  		   uFlags,
  		   uIDNewItem,
  		  (LPCWSTR) lpNewItem,
  		   FALSE);

  return SetMenuItemInfoA( hMnu,
                           uPosition,
                          (BOOL)(MF_BYPOSITION & uFlags),
                           &mii);
}


/*
 * @implemented
 */
BOOL
WINAPI
ModifyMenuW(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  ROSMENUINFO mi;
  ROSMENUITEMINFO rmii;
  MENUITEMINFOW mii;
  memset ( &mii, 0, sizeof(mii) );
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_FTYPE;

  if (!MenuGetRosMenuInfo( &mi, hMnu )) return FALSE;

  mi.Height = 0; // Force size recalculation.

  if (!MenuSetRosMenuInfo( &mi )) return FALSE;

  MenuInitRosMenuItemInfo( &rmii );

  if(!MenuGetRosMenuItemInfo( hMnu, uPosition, &rmii)) return FALSE;

  if ((rmii.fType & MF_POPUP) && (uFlags & MF_POPUP) && (rmii.hSubMenu != (HMENU)uIDNewItem))
    NtUserDestroyMenu( rmii.hSubMenu );   /* ModifyMenu() spec */

  MenuCleanupRosMenuItemInfo( &rmii );

  /* Init new data for this menu item */
  MenuSetItemData( &mii,
  		   uFlags,
  		   uIDNewItem,
  		   lpNewItem,
  		   TRUE);

  /* Now, make Win32k IntSetMenuItemInfo handle the changes to this menu item. */
  return SetMenuItemInfoW( hMnu,
                           uPosition,
                           (BOOL)(MF_BYPOSITION & uFlags),
                           &mii);
}


/*
 * @implemented
 */
BOOL WINAPI
SetMenu(HWND hWnd,
	HMENU hMenu)
{
  return NtUserSetMenu(hWnd, hMenu, TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  ROSMENUINFO mi;
  BOOL res = FALSE;
  if(lpcmi->cbSize != sizeof(MENUINFO))
    return res;

  memcpy(&mi, lpcmi, sizeof(MENUINFO));
  return NtUserMenuInfo(hmenu, &mi, TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemBitmaps(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  HBITMAP hBitmapUnchecked,
  HBITMAP hBitmapChecked)
{
  ROSMENUITEMINFO uItem;
  memset ( &uItem, 0, sizeof(uItem) );
  uItem.fMask = MIIM_STATE | MIIM_BITMAP;

  if(!(NtUserMenuItemInfo(hMenu, uPosition,
                 (BOOL)(MF_BYPOSITION & uFlags), &uItem, FALSE))) return FALSE;

  if (!hBitmapChecked && !hBitmapUnchecked)
  {
    uItem.fState &= ~MF_USECHECKBITMAPS;
  }
  else  /* Install new bitmaps */
  {
    uItem.hbmpChecked = hBitmapChecked;
    uItem.hbmpUnchecked = hBitmapUnchecked;
    uItem.fState |= MF_USECHECKBITMAPS;
  }
 return NtUserMenuItemInfo(hMenu, uPosition,
                                 (BOOL)(MF_BYPOSITION & uFlags), &uItem, TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOA lpmii)
{
  MENUITEMINFOW MenuItemInfoW;
  UNICODE_STRING UnicodeString;
  NTSTATUS Status;
  ULONG Result = FALSE;

  RtlCopyMemory(&MenuItemInfoW, lpmii, min(lpmii->cbSize, sizeof(MENUITEMINFOW)));

  if( lpmii->cbSize != sizeof( MENUITEMINFOW))
  {
     MenuItemInfoW.cbSize = sizeof( MENUITEMINFOW);
     MenuItemInfoW.hbmpItem = NULL;
  }
/*
 *  MIIM_STRING              == good
 *  MIIM_TYPE & MFT_STRING   == good
 *  MIIM_STRING & MFT_STRING == good
 *  MIIM_STRING & MFT_OWNERSRAW == good
 */
  if (((MenuItemInfoW.fMask & MIIM_STRING) ||
      ((MenuItemInfoW.fMask & MIIM_TYPE) &&
                           (MENU_ITEM_TYPE(MenuItemInfoW.fType) == MF_STRING)))
        && MenuItemInfoW.dwTypeData != NULL)
  {
/* cch is ignored when the content of a menu item is set by calling SetMenuItemInfo. */
    Status = RtlCreateUnicodeStringFromAsciiz(&UnicodeString,
                                     (LPSTR)MenuItemInfoW.dwTypeData);
    if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return FALSE;
    }
    MenuItemInfoW.dwTypeData = UnicodeString.Buffer;
    MenuItemInfoW.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
    UnicodeString.Buffer = NULL;
  }

  Result = NtUserMenuItemInfo(hMenu, uItem, fByPosition,
                              (PROSMENUITEMINFO)&MenuItemInfoW, TRUE);

  if (UnicodeString.Buffer != NULL)
  {
    RtlFreeUnicodeString(&UnicodeString);
  }

  return Result;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii)
{
  MENUITEMINFOW MenuItemInfoW;
  ULONG Result;

  RtlCopyMemory(&MenuItemInfoW, lpmii, min(lpmii->cbSize, sizeof(MENUITEMINFOW)));

  if( lpmii->cbSize != sizeof( MENUITEMINFOW))
  {
     MenuItemInfoW.cbSize = sizeof( MENUITEMINFOW);
     MenuItemInfoW.hbmpItem = NULL;
  }

  if (((MenuItemInfoW.fMask & MIIM_STRING) ||
      ((MenuItemInfoW.fMask & MIIM_TYPE) &&
                           (MENU_ITEM_TYPE(MenuItemInfoW.fType) == MF_STRING)))
        && MenuItemInfoW.dwTypeData != NULL)
  {
     MenuItemInfoW.cch = strlenW(MenuItemInfoW.dwTypeData);
  }
  Result = NtUserMenuItemInfo(hMenu, uItem, fByPosition,
                            (PROSMENUITEMINFO)&MenuItemInfoW, TRUE);

  return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetSystemMenu (
  HWND hwnd,
  HMENU hMenu)
{
  if(!hwnd)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  if(!hMenu)
  {
    SetLastError(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }
  return NtUserSetSystemMenu(hwnd, hMenu);
}


/*
 * @implemented
 */
BOOL
WINAPI
TrackPopupMenu(
  HMENU Menu,
  UINT Flags,
  int x,
  int y,
  int Reserved,
  HWND Wnd,
  CONST RECT *Rect)
{
  BOOL ret = FALSE;

  MenuInitTracking(Wnd, Menu, TRUE, Flags);

  /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
  if (0 == (Flags & TPM_NONOTIFY))
    {
      SendMessageW(Wnd, WM_INITMENUPOPUP, (WPARAM) Menu, 0);
    }

  if (MenuShowPopup(Wnd, Menu, 0, x, y, 0, 0 ))
    {
      ret = MenuTrackMenu(Menu, Flags | TPM_POPUPMENU, 0, 0, Wnd, Rect);
    }
  MenuExitTracking(Wnd);

  return ret;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
TrackPopupMenuEx(
  HMENU Menu,
  UINT Flags,
  int x,
  int y,
  HWND Wnd,
  LPTPMPARAMS Tpm)
{
  /* Not fully implemented */
  return TrackPopupMenu(Menu, Flags, x, y, 0, Wnd,
                        NULL != Tpm ? &Tpm->rcExclude : NULL);
}

//
// Example for the Win32/User32 rewrite.
// Def = TrackPopupMenuEx@24=NtUserTrackPopupMenuEx@24
//
//
BOOL
WINAPI
NEWTrackPopupMenu(
  HMENU Menu,
  UINT Flags,
  int x,
  int y,
  int Reserved,
  HWND Wnd,
  CONST RECT *Rect)
{
  return NtUserTrackPopupMenuEx( Menu,
                                Flags,
                                    x,
                                    y,
                                  Wnd,
                                 NULL); // LPTPMPARAMS is null
}


/*
 * @implemented
 */
DWORD
WINAPI
GetMenuContextHelpId(HMENU hmenu)
{
  ROSMENUINFO mi;
  mi.cbSize = sizeof(ROSMENUINFO);
  mi.fMask = MIM_HELPID;

  if(NtUserMenuInfo(hmenu, &mi, FALSE))
  {
    return mi.dwContextHelpID;
  }
  return 0;
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
MenuWindowProcA(
		HWND   hWnd,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
MenuWindowProcW(
		HWND   hWnd,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ChangeMenuW(
    HMENU hMenu,
    UINT cmd,
    LPCWSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags)
{
    /*
        FIXME: Word passes the item id in 'cmd' and 0 or 0xffff as cmdInsert
        for MF_DELETE. We should check the parameters for all others
        MF_* actions also (anybody got a doc on ChangeMenu?).
    */

    switch(flags & (MF_APPEND | MF_DELETE | MF_CHANGE | MF_REMOVE | MF_INSERT))
    {
        case MF_APPEND :
            return AppendMenuW(hMenu, flags &~ MF_APPEND, cmdInsert, lpszNewItem);

        case MF_DELETE :
            return DeleteMenu(hMenu, cmd, flags &~ MF_DELETE);

        case MF_CHANGE :
            return ModifyMenuW(hMenu, cmd, flags &~ MF_CHANGE, cmdInsert, lpszNewItem);

        case MF_REMOVE :
            return RemoveMenu(hMenu, flags & MF_BYPOSITION ? cmd : cmdInsert,
                                flags &~ MF_REMOVE);

        default :   /* MF_INSERT */
            return InsertMenuW(hMenu, cmd, flags, cmdInsert, lpszNewItem);
    };
}

/*
 * @implemented
 */
BOOL
WINAPI
ChangeMenuA(
    HMENU hMenu,
    UINT cmd,
    LPCSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags)
{
    /*
        FIXME: Word passes the item id in 'cmd' and 0 or 0xffff as cmdInsert
        for MF_DELETE. We should check the parameters for all others
        MF_* actions also (anybody got a doc on ChangeMenu?).
    */

    switch(flags & (MF_APPEND | MF_DELETE | MF_CHANGE | MF_REMOVE | MF_INSERT))
    {
        case MF_APPEND :
            return AppendMenuA(hMenu, flags &~ MF_APPEND, cmdInsert, lpszNewItem);

        case MF_DELETE :
            return DeleteMenu(hMenu, cmd, flags &~ MF_DELETE);

        case MF_CHANGE :
            return ModifyMenuA(hMenu, cmd, flags &~ MF_CHANGE, cmdInsert, lpszNewItem);

        case MF_REMOVE :
            return RemoveMenu(hMenu, flags & MF_BYPOSITION ? cmd : cmdInsert,
                                flags &~ MF_REMOVE);

        default :   /* MF_INSERT */
            return InsertMenuA(hMenu, cmd, flags, cmdInsert, lpszNewItem);
    };
}





