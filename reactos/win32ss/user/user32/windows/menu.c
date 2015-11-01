/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/menu.c
 * PURPOSE:         Menus
 *
 * PROGRAMMERS:     Casper S. Hornstrup
 *                  James Tabor
 */

/* INCLUDES ******************************************************************/

#include <user32.h>
#include <wine/debug.h>

BOOL WINAPI GdiValidateHandle(HGDIOBJ hobj);

WINE_DEFAULT_DEBUG_CHANNEL(menu);

/* internal popup menu window messages */

#define MM_SETMENUHANDLE	(WM_USER + 0)
#define MM_GETMENUHANDLE	(WM_USER + 1)

#define MENU_TYPE_MASK (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)

#define MENU_ITEM_TYPE(flags) ((flags) & MENU_TYPE_MASK)

#define MNS_STYLE_MASK (MNS_NOCHECK|MNS_MODELESS|MNS_DRAGDROP|MNS_AUTODISMISS|MNS_NOTIFYBYPOS|MNS_CHECKORBMP)

#define MENUITEMINFO_TYPE_MASK \
                (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
                 MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
                 MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )

#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)

#define STATE_MASK (~TYPE_MASK)

#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))

#define MII_STATE_MASK (MFS_GRAYED|MFS_CHECKED|MFS_HILITE|MFS_DEFAULT)

/* macro to test that flags do not indicate bitmap, ownerdraw or separator */
#define IS_STRING_ITEM(flags) (MF_STRING == MENU_ITEM_TYPE(flags))
#define IS_MAGIC_BITMAP(id) ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define IS_SYSTEM_MENU(MenuInfo)  \
	(0 == ((MenuInfo)->fFlags & MNF_POPUP) && 0 != ((MenuInfo)->fFlags & MNF_SYSMENU))

#define IS_SYSTEM_POPUP(MenuInfo) \
	(0 != ((MenuInfo)->fFlags & MNF_POPUP) && 0 != ((MenuInfo)->fFlags & MNF_SYSMENU))

#define IS_BITMAP_ITEM(flags) (MF_BITMAP == MENU_ITEM_TYPE(flags))

/*********************************************************************
 * PopupMenu class descriptor
 */
const struct builtin_class_descr POPUPMENU_builtin_class =
{
    WC_MENU,                     /* name */
    CS_SAVEBITS | CS_DBLCLKS,                  /* style  */
    NULL,                                      /* FIXME - procA */
    PopupMenuWndProcW,                         /* FIXME - procW */
    sizeof(MENUINFO *),                        /* extra */
    (LPCWSTR) IDC_ARROW,                       /* cursor */
    (HBRUSH)(COLOR_MENU + 1)                   /* brush */
};

#ifndef GET_WORD
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#endif
#ifndef GET_DWORD
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#endif


/***********************************************************************
 *           MENU_GetMenu
 *
 * Validate the given menu handle and returns the menu structure pointer.
 */
FORCEINLINE PMENU MENU_GetMenu(HMENU hMenu)
{
    return ValidateHandleNoErr(hMenu, TYPE_MENU); 
}

/***********************************************************************
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
ITEM *MENU_FindItem( HMENU *hmenu, UINT *nPos, UINT wFlags )
{
    MENU *menu;
    ITEM *fallback = NULL;
    UINT fallback_pos = 0;
    UINT i;
    PITEM pItem;

    if ((*hmenu == (HMENU)0xffff) || (!(menu = MENU_GetMenu(*hmenu)))) return NULL;
    if (wFlags & MF_BYPOSITION)
    {
	if (*nPos >= menu->cItems) return NULL;
	pItem = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
	if (pItem) pItem = &pItem[*nPos];
	return pItem;
    }
    else
    {
        PITEM item = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
	for (i = 0; item, i < menu->cItems; i++, item++)
	{
	    if (item->spSubMenu)
	    {
	        PMENU pSubMenu = DesktopPtrToUser(item->spSubMenu);
		HMENU hsubmenu = UserHMGetHandle(pSubMenu);
		ITEM *subitem = MENU_FindItem( &hsubmenu, nPos, wFlags );
		if (subitem)
		{
		    *hmenu = hsubmenu;
		    return subitem;
		}
		else if (item->wID == *nPos)
		{
		    /* fallback to this item if nothing else found */
		    fallback_pos = i;
		    fallback = item;
		}
	    }
	    else if (item->wID == *nPos)
	    {
		*nPos = i;
		return item;
	    }
	}
    }

    if (fallback)
        *nPos = fallback_pos;

    return fallback;
}

UINT FASTCALL
IntGetMenuDefaultItem(PMENU Menu, BOOL fByPos, UINT gmdiFlags, DWORD *gismc)
{
   UINT i = 0;
   PITEM Item = Menu->rgItems ? DesktopPtrToUser(Menu->rgItems) : NULL;

   /* empty menu */
   if (!Item) return -1;

   while ( !( Item->fState & MFS_DEFAULT ) )
   {
      i++; Item++;
      if  (i >= Menu->cItems ) return -1;
   }

   /* default: don't return disabled items */
   if ( (!(GMDI_USEDISABLED & gmdiFlags)) && (Item->fState & MFS_DISABLED )) return -1;

   /* search rekursiv when needed */
   if ( (Item->fType & MF_POPUP) &&  (gmdiFlags & GMDI_GOINTOPOPUPS) && Item->spSubMenu)
   {
      UINT ret;
      (*gismc)++;
      ret = IntGetMenuDefaultItem( DesktopPtrToUser(Item->spSubMenu), fByPos, gmdiFlags, gismc );
      (*gismc)--;
      if ( -1 != ret ) return ret;

      /* when item not found in submenu, return the popup item */
   }
   return ( fByPos ) ? i : Item->wID;
}

static BOOL GetMenuItemInfo_common ( HMENU hmenu,
                                     UINT item,
                                     BOOL bypos,
                                     LPMENUITEMINFOW lpmii,
                                     BOOL unicode)
{
    ITEM *pItem = MENU_FindItem (&hmenu, &item, bypos ? MF_BYPOSITION : 0);

    //debug_print_menuitem("GetMenuItemInfo_common: ", pItem, "");

    if (!pItem)
    {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return FALSE;
    }

    if( lpmii->fMask & MIIM_TYPE)
    {
        if( lpmii->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP))
        {
            ERR("invalid combination of fMask bits used\n");
            /* this does not happen on Win9x/ME */
            SetLastError( ERROR_INVALID_PARAMETER);
            return FALSE;
        }
	lpmii->fType = pItem->fType & MENUITEMINFO_TYPE_MASK;
        if( pItem->hbmp) lpmii->fType |= MFT_BITMAP;
	lpmii->hbmpItem = pItem->hbmp; /* not on Win9x/ME */
        if( lpmii->fType & MFT_BITMAP)
        {
	    lpmii->dwTypeData = (LPWSTR) pItem->hbmp;
	    lpmii->cch = 0;
        }
        else if( lpmii->fType & (MFT_OWNERDRAW | MFT_SEPARATOR))
        {
            /* this does not happen on Win9x/ME */
	    lpmii->dwTypeData = 0;
	    lpmii->cch = 0;
        }
    }

    /* copy the text string */
    if ((lpmii->fMask & (MIIM_TYPE|MIIM_STRING)))
    {
         if( !pItem->Xlpstr )
         {                                         // Very strange this fixes a wine test with a crash.
                if(lpmii->dwTypeData && lpmii->cch && !(GdiValidateHandle((HGDIOBJ)lpmii->dwTypeData)) )
                {
                    if( unicode)
                        *((WCHAR *)lpmii->dwTypeData) = 0;
                    else
                        *((CHAR *)lpmii->dwTypeData) = 0;
                }
                lpmii->cch = 0;
         }
         else
         {
            int len;
            LPWSTR text = DesktopPtrToUser(pItem->Xlpstr);
            if (unicode)
            {
                len = strlenW(text);
                if(lpmii->dwTypeData && lpmii->cch)
                    lstrcpynW(lpmii->dwTypeData, text, lpmii->cch);
            }
            else
            {
                len = WideCharToMultiByte( CP_ACP, 0, text, -1, NULL, 0, NULL, NULL ) - 1;
                if(lpmii->dwTypeData && lpmii->cch)
                    if (!WideCharToMultiByte( CP_ACP, 0, text, -1,
                            (LPSTR)lpmii->dwTypeData, lpmii->cch, NULL, NULL ))
                        ((LPSTR)lpmii->dwTypeData)[lpmii->cch - 1] = 0;
            }
            /* if we've copied a substring we return its length */
            if(lpmii->dwTypeData && lpmii->cch)
                if (lpmii->cch <= len + 1)
                    lpmii->cch--;
                else
                    lpmii->cch = len;
            else
            {
                /* return length of string */
                /* not on Win9x/ME if fType & MFT_BITMAP */
                lpmii->cch = len;
            }
        }
    }

    if (lpmii->fMask & MIIM_FTYPE)
	lpmii->fType = pItem->fType & MENUITEMINFO_TYPE_MASK;

    if (lpmii->fMask & MIIM_BITMAP)
	lpmii->hbmpItem = pItem->hbmp;

    if (lpmii->fMask & MIIM_STATE)
	lpmii->fState = pItem->fState & MENUITEMINFO_STATE_MASK;

    if (lpmii->fMask & MIIM_ID)
	lpmii->wID = pItem->wID;

    if (lpmii->fMask & MIIM_SUBMENU && pItem->spSubMenu )
    {
        PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
        HMENU hSubMenu = UserHMGetHandle(pSubMenu);
	lpmii->hSubMenu = hSubMenu;
    }
    else
    {
        /* hSubMenu is always cleared 
         * (not on Win9x/ME ) */
        lpmii->hSubMenu = 0;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS)
    {
	lpmii->hbmpChecked = pItem->hbmpChecked;
	lpmii->hbmpUnchecked = pItem->hbmpUnchecked;
    }
    if (lpmii->fMask & MIIM_DATA)
	lpmii->dwItemData = pItem->dwItemData;

  return TRUE;
}


//
// User side Menu Class Proc.
//
LRESULT WINAPI
PopupMenuWndProcW(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  LRESULT lResult;
  PWND pWnd;

  TRACE("PMWPW : hwnd=%x msg=0x%04x wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  pWnd = ValidateHwnd(Wnd);
  if (pWnd)
  {
     if (!pWnd->fnid)
     {
        if (Message != WM_NCCREATE)
        {
           return DefWindowProcW(Wnd, Message, wParam, lParam);
        }
     }
     else
     {
        if (pWnd->fnid != FNID_MENU)
        {
           ERR("Wrong window class for Menu!\n");
           return 0;
        }
     }
  }

  switch(Message)
    {
    case WM_DESTROY:
    case WM_NCDESTROY:
    case WM_NCCREATE:
    case WM_CREATE:
    case MM_SETMENUHANDLE:
    case MM_GETMENUHANDLE:
    case MN_SETHMENU:
    case MN_GETHMENU:
    case WM_PAINT:
    case WM_PRINTCLIENT:
      {
        TRACE("Menu Class ProcW\n");
        NtUserMessageCall( Wnd, Message, wParam, lParam, (ULONG_PTR)&lResult, FNID_MENU, FALSE);
        return lResult;
      }
    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
      return MA_NOACTIVATE;

    case WM_ERASEBKGND:
      return 1;

    case WM_SHOWWINDOW: // Not sure what this does....
      if (0 != wParam)
        {
          if (0 == GetWindowLongPtrW(Wnd, 0))
            {
              OutputDebugStringA("no menu to display\n");
            }
        }
      else
        {
          //SetWindowLongPtrW(Wnd, 0, 0);
        }
      break;

    default:
      return DefWindowProcW(Wnd, Message, wParam, lParam);
    }

  return 0;
}

LRESULT WINAPI PopupMenuWndProcA(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  PWND pWnd;

  pWnd = ValidateHwnd(Wnd);
  if (pWnd && !pWnd->fnid && Message != WM_NCCREATE)
  {
     return DefWindowProcA(Wnd, Message, wParam, lParam);
  }    
  TRACE("YES! hwnd=%x msg=0x%04x wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  switch(Message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    case WM_MOUSEACTIVATE:
    case WM_PAINT:
    case WM_PRINTCLIENT:
    case WM_ERASEBKGND:
    case WM_DESTROY:
    case WM_NCDESTROY:
    case WM_SHOWWINDOW:
    case MM_SETMENUHANDLE:
    case MM_GETMENUHANDLE:
    case MN_SETHMENU:
    case MN_GETHMENU: 
      return PopupMenuWndProcW(Wnd, Message, wParam, lParam);

    default:
      return DefWindowProcA(Wnd, Message, wParam, lParam);
    }
  return 0;
}

/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * NOTE: flags is equivalent to the mtOption field
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu)
{
    WORD flags, id = 0;
    HMENU hSubMenu;
    LPCWSTR str;
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
        str = (LPCWSTR)res;
        res += (strlenW(str) + 1) * sizeof(WCHAR);

        if (flags & MF_POPUP)
        {
            hSubMenu = CreatePopupMenu();
            if(!hSubMenu) return NULL;
            if(!(res = MENU_ParseResource(res, hSubMenu))) return NULL;
            AppendMenuW(hMenu, flags, (UINT_PTR)hSubMenu, (LPCWSTR)str);
        }
        else  /* Not a popup */
        {
            AppendMenuW(hMenu, flags, id, *(LPCWSTR)str ? (LPCWSTR)str : NULL);
        }
    } while(!end);
    return res;
}

/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENUEX_ParseResource(LPCSTR res, HMENU hMenu)
{
    WORD resinfo;
    do
    {
        MENUITEMINFOW mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
        mii.fType = GET_DWORD(res);
        res += sizeof(DWORD);
        mii.fState = GET_DWORD(res);
        res += sizeof(DWORD);
        mii.wID = GET_DWORD(res);
        res += sizeof(DWORD);
        resinfo = GET_WORD(res);
        res += sizeof(WORD);
        /* Align the text on a word boundary.  */
        res += (~((UINT_PTR)res - 1)) & 1;
        mii.dwTypeData = (LPWSTR)res;
        mii.cch = strlenW(mii.dwTypeData);
        res += (1 + strlenW(mii.dwTypeData)) * sizeof(WCHAR);
        /* Align the following fields on a dword boundary.  */
        res += (~((UINT_PTR)res - 1)) & 3;

        TRACE("Menu item: [%08x,%08x,%04x,%04x,%S]\n",
              mii.fType, mii.fState, mii.wID, resinfo, mii.dwTypeData);

        if (resinfo & 1) /* Pop-up? */
        {
            /* DWORD helpid = GET_DWORD(res); FIXME: use this. */
            res += sizeof(DWORD);
            mii.hSubMenu = CreatePopupMenu();
            if (!mii.hSubMenu)
            {
                ERR("CreatePopupMenu failed\n");
                return NULL;
            }

            if (!(res = MENUEX_ParseResource(res, mii.hSubMenu)))
            {
                ERR("MENUEX_ParseResource failed\n");
                DestroyMenu(mii.hSubMenu);
                return NULL;
            }
            mii.fMask |= MIIM_SUBMENU;
        }
        else if (!mii.dwTypeData[0] && !(mii.fType & MF_SEPARATOR))
        {
            WARN("Converting NULL menu item %04x, type %04x to SEPARATOR\n",
                mii.wID, mii.fType);
            mii.fType |= MF_SEPARATOR;
        }
        InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
    } while (!(resinfo & MF_END));
    return res;
}


/**********************************************************************
 *         MENU_mnu2mnuii
 *
 * Uses flags, id and text ptr, passed by InsertMenu() and
 * ModifyMenu() to setup a MenuItemInfo structure.
 */
static void MENU_mnu2mnuii( UINT flags, UINT_PTR id, LPCWSTR str, LPMENUITEMINFOW pmii, BOOL Unicode)
{
    RtlZeroMemory( pmii, sizeof( MENUITEMINFOW));
    pmii->cbSize = sizeof( MENUITEMINFOW);
    pmii->fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;
    /* setting bitmap clears text and vice versa */
    if( IS_STRING_ITEM(flags)) {
        pmii->fMask |= MIIM_STRING | MIIM_BITMAP;
        if( !str)
            flags |= MF_SEPARATOR;
        /* Item beginning with a backspace is a help item */
        /* FIXME: wrong place, this is only true in win16 */
        else
        {
            if (Unicode)
           {
              if (*str == '\b')
              {
                flags |= MF_HELP;
                str++;
              }
           }
           else
           {
              LPCSTR NewItemA = (LPCSTR) str;
              if (*NewItemA == '\b')
              {
                 flags |= MF_HELP;
                 NewItemA++;
                 str = (LPCWSTR) NewItemA;
              }
              TRACE("A cch %d\n",strlen(NewItemA));
           }
        }
        pmii->dwTypeData = (LPWSTR)str;
    } else if( flags & MFT_BITMAP){
        pmii->fMask |= MIIM_BITMAP | MIIM_STRING;
        pmii->hbmpItem = (HBITMAP)str;
    }
    if( flags & MF_OWNERDRAW){
        pmii->fMask |= MIIM_DATA;
        pmii->dwItemData = (ULONG_PTR) str;
    }
    if( flags & MF_POPUP && MENU_GetMenu((HMENU)id)) {
        pmii->fMask |= MIIM_SUBMENU;
        pmii->hSubMenu = (HMENU)id;
    }
    if( flags & MF_SEPARATOR) flags |= MF_GRAYED | MF_DISABLED;
    pmii->fState = flags & MENUITEMINFO_STATE_MASK & ~MFS_DEFAULT;
    pmii->fType = flags & MENUITEMINFO_TYPE_MASK;
    pmii->wID = (UINT)id;
}

/**********************************************************************
 *		MENU_NormalizeMenuItemInfoStruct
 *
 * Helper for SetMenuItemInfo and InsertMenuItemInfo:
 * check, copy and extend the MENUITEMINFO struct from the version that the application
 * supplied to the version used by wine source. */
static BOOL MENU_NormalizeMenuItemInfoStruct( const MENUITEMINFOW *pmii_in,
                                              MENUITEMINFOW *pmii_out )
{
    /* do we recognize the size? */
    if( !pmii_in || (pmii_in->cbSize != sizeof( MENUITEMINFOW) &&
            pmii_in->cbSize != sizeof( MENUITEMINFOW) - sizeof( pmii_in->hbmpItem)) ) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* copy the fields that we have */
    memcpy( pmii_out, pmii_in, pmii_in->cbSize);
    /* if the hbmpItem member is missing then extend */
    if( pmii_in->cbSize != sizeof( MENUITEMINFOW)) {
        pmii_out->cbSize = sizeof( MENUITEMINFOW);
        pmii_out->hbmpItem = NULL;
    }
    /* test for invalid bit combinations */
    if( (pmii_out->fMask & MIIM_TYPE &&
         pmii_out->fMask & (MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP)) ||
        (pmii_out->fMask & MIIM_FTYPE && pmii_out->fType & MFT_BITMAP)) {
        ERR("invalid combination of fMask bits used\n");
        /* this does not happen on Win9x/ME */
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* convert old style (MIIM_TYPE) to the new and keep the old one too */
    if( pmii_out->fMask & MIIM_TYPE){
        pmii_out->fMask |= MIIM_FTYPE;
        if( IS_STRING_ITEM(pmii_out->fType)){
            pmii_out->fMask |= MIIM_STRING;
        } else if( (pmii_out->fType) & MFT_BITMAP){
            pmii_out->fMask |= MIIM_BITMAP;
            pmii_out->hbmpItem = UlongToHandle(LOWORD(pmii_out->dwTypeData));
        }
    }
    if (pmii_out->fMask & MIIM_FTYPE )
    {
        pmii_out->fType &= ~MENUITEMINFO_TYPE_MASK;
        pmii_out->fType |= pmii_in->fType & MENUITEMINFO_TYPE_MASK;
    }
    if (pmii_out->fMask & MIIM_STATE)
    /* Other menu items having MFS_DEFAULT are not converted
       to normal items */
       pmii_out->fState = pmii_in->fState & MENUITEMINFO_STATE_MASK;

    if (pmii_out->fMask & MIIM_SUBMENU)
    {
       if ((pmii_out->hSubMenu != NULL) && !IsMenu(pmii_out->hSubMenu))
          return FALSE;
    }

    return TRUE;
}

BOOL
MenuInit(VOID)
{
  return TRUE;
}

VOID
MenuCleanup(VOID)
{
}


NTSTATUS WINAPI
User32LoadSysMenuTemplateForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  LRESULT Result = 0;

  // Use this for Menu Ole!!

  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}

NTSTATUS WINAPI
User32CallLoadMenuFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PLOADMENU_CALLBACK_ARGUMENTS Common;
  LRESULT Result;

  Common = (PLOADMENU_CALLBACK_ARGUMENTS) Arguments;

  Result = (LRESULT)LoadMenuW( Common->hModule, Common->InterSource ? MAKEINTRESOURCE(Common->InterSource) : (LPCWSTR)&Common->MenuName);

  return ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS);
}


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL WINAPI
AppendMenuA(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  RtlInitUnicodeString(&UnicodeString, 0);

  MENU_mnu2mnuii( uFlags, uIDNewItem, (LPCWSTR)lpNewItem, &mii, FALSE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("AMA Handle bitmaps\n");
  }
  ////// Answer a question, why a -1? To hunt for the end of the item list. Get it, to Append?
  res = NtUserThunkedMenuItemInfo(hMenu, -1, TRUE, TRUE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
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
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res;

  RtlInitUnicodeString(&MenuText, 0);    

  MENU_mnu2mnuii( uFlags, uIDNewItem, lpNewItem, &mii, TRUE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)mii.dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  res = NtUserThunkedMenuItemInfo(hMenu, -1, TRUE, TRUE, &mii, &MenuText);
  return res;
}

/*
 * @implemented
 */
DWORD WINAPI
CheckMenuItem(HMENU hmenu,
	      UINT uIDCheckItem,
	      UINT uCheck)
{
  PITEM item;
  DWORD Ret;
  UINT uID = uIDCheckItem;

  if (!ValidateHandle(hmenu, TYPE_MENU))
     return -1;

  if (!(item = MENU_FindItem( &hmenu, &uID, uCheck ))) return -1;

  Ret = item->fState & MFS_CHECKED;
  if ( Ret == (uCheck & MFS_CHECKED)) return Ret; // Already Checked...

  return NtUserCheckMenuItem(hmenu, uIDCheckItem, uCheck);
}

/*
 * @implemented
 */
BOOL WINAPI
CheckMenuRadioItem(HMENU hMenu,
                   UINT first,
                   UINT last,
                   UINT check,
                   UINT bypos)
{
    BOOL done = FALSE;
    UINT i;
    PITEM mi_first = NULL, mi_check;
    HMENU m_first, m_check;
    MENUITEMINFOW mii;
    mii.cbSize = sizeof( mii);

    for (i = first; i <= last; i++)
    {
        UINT pos = i;

        if (!mi_first)
        {
            m_first = hMenu;
            mi_first = MENU_FindItem(&m_first, &pos, bypos);
            if (!mi_first) continue;
            mi_check = mi_first;
            m_check = m_first;
        }
        else
        {
            m_check = hMenu;
            mi_check = MENU_FindItem(&m_check, &pos, bypos);
            if (!mi_check) continue;
        }

        if (m_first != m_check) continue;
        if (mi_check->fType == MFT_SEPARATOR) continue;

        if (i == check)
        {
            if (!(mi_check->fType & MFT_RADIOCHECK) || !(mi_check->fState & MFS_CHECKED))
            {
               mii.fMask = MIIM_FTYPE | MIIM_STATE;
               mii.fType = (mi_check->fType & MENUITEMINFO_TYPE_MASK) | MFT_RADIOCHECK;
               mii.fState = (mi_check->fState & MII_STATE_MASK) | MFS_CHECKED;
               NtUserThunkedMenuItemInfo(m_check, i, bypos, FALSE, &mii, NULL);
            }
            done = TRUE;
        }
        else
        {
            /* MSDN is wrong, Windows does not remove MFT_RADIOCHECK */
            if (mi_check->fState & MFS_CHECKED)
            {
               mii.fMask = MIIM_STATE;
               mii.fState = (mi_check->fState & MII_STATE_MASK) & ~MFS_CHECKED;
               NtUserThunkedMenuItemInfo(m_check, i, bypos, FALSE, &mii, NULL);
            }
        }
    }
    return done;
}

/*
 * @implemented
 */
HMENU WINAPI
CreateMenu(VOID)
{
  return NtUserxCreateMenu();
}

/*
 * @implemented
 */
HMENU WINAPI
CreatePopupMenu(VOID)
{
  return NtUserxCreatePopupMenu();
}

/*
 * @implemented
 */
BOOL WINAPI
DrawMenuBar(HWND hWnd)
{
  return NtUserxDrawMenuBar(hWnd);
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
HMENU WINAPI
GetMenu(HWND hWnd)
{
       PWND Wnd = ValidateHwnd(hWnd);

       if (!Wnd)
               return NULL;

       return UlongToHandle(Wnd->IDMenu);
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
DWORD
WINAPI
GetMenuContextHelpId(HMENU hmenu)
{
  PMENU pMenu;
  if ((pMenu = ValidateHandle(hmenu, TYPE_MENU)))
     return pMenu->dwContextHelpId;
  return 0;
}

/*
 * @implemented
 */
UINT WINAPI
GetMenuDefaultItem(HMENU hMenu,
		   UINT fByPos,
		   UINT gmdiFlags)
{
  PMENU pMenu;
  DWORD gismc = 0;
  if (!(pMenu = ValidateHandle(hMenu, TYPE_MENU)))
       return (UINT)-1;

  return IntGetMenuDefaultItem( pMenu, (BOOL)fByPos, gmdiFlags, &gismc);
}

/*
 * @implemented
 */
BOOL WINAPI
GetMenuInfo(HMENU hmenu,
	    LPMENUINFO lpcmi)
{
  PMENU pMenu;

  if (!lpcmi || (lpcmi->cbSize != sizeof(MENUINFO)))
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return FALSE;
  }

  if (!(pMenu = ValidateHandle(hmenu, TYPE_MENU)))
     return FALSE;

  if (lpcmi->fMask & MIM_BACKGROUND)
      lpcmi->hbrBack = pMenu->hbrBack;

  if (lpcmi->fMask & MIM_HELPID)
      lpcmi->dwContextHelpID = pMenu->dwContextHelpId;

  if (lpcmi->fMask & MIM_MAXHEIGHT)
      lpcmi->cyMax = pMenu->cyMax;

  if (lpcmi->fMask & MIM_MENUDATA)
      lpcmi->dwMenuData = pMenu->dwMenuData;

  if (lpcmi->fMask & MIM_STYLE)
      lpcmi->dwStyle = pMenu->fFlags & MNS_STYLE_MASK;

  return TRUE;
}

/*
 * @implemented
 */
int WINAPI
GetMenuItemCount(HMENU hmenu)
{
  PMENU pMenu;
  if ((pMenu = ValidateHandle(hmenu, TYPE_MENU)))
     return pMenu->cItems;
  return -1;
}

/*
 * @implemented
 */
UINT WINAPI
GetMenuItemID(HMENU hMenu,
	      int nPos)
{
  ITEM * lpmi;
  if (!(lpmi = MENU_FindItem(&hMenu,(UINT*)&nPos,MF_BYPOSITION))) return -1;
  if (lpmi->spSubMenu) return -1;
  return lpmi->wID;
}

/*
 * @implemented
 */
BOOL WINAPI
GetMenuItemInfoA(
   HMENU hmenu,
   UINT item,
   BOOL bypos,
   LPMENUITEMINFOA lpmii)
{
    BOOL ret;
    MENUITEMINFOA mii;

    if( lpmii->cbSize != sizeof( mii) &&
        lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy( &mii, lpmii, lpmii->cbSize);
    mii.cbSize = sizeof( mii);
    ret = GetMenuItemInfo_common (hmenu,
                                  item,
                                  bypos,
                                  (LPMENUITEMINFOW)&mii,
                                  FALSE);
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize);
    return ret;
}

/*
 * @implemented
 */
BOOL WINAPI
GetMenuItemInfoW(
   HMENU hMenu,
   UINT Item,
   BOOL bypos,
   LPMENUITEMINFOW lpmii)
{
   BOOL ret;
   MENUITEMINFOW mii;
   if( lpmii->cbSize != sizeof( mii) && lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem))
   {
      SetLastError( ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   memcpy( &mii, lpmii, lpmii->cbSize);
   mii.cbSize = sizeof( mii);
   ret = GetMenuItemInfo_common (hMenu, Item, bypos, &mii, TRUE);
   mii.cbSize = lpmii->cbSize;
   memcpy( lpmii, &mii, mii.cbSize);
   return ret;
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
  PITEM pItem;
  UINT Type = 0;
  TRACE("(menu=%p, id=%04x, flags=%04x);\n", hMenu, uId, uFlags);
  if (!(pItem = MENU_FindItem( &hMenu, &uId, uFlags ))) return -1;

  if (!pItem->Xlpstr && pItem->hbmp) Type = MFT_BITMAP;

  if (pItem->spSubMenu)
  {
     PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
     HMENU hsubmenu = UserHMGetHandle(pSubMenu);
     if (!IsMenu(hsubmenu)) return (UINT)-1;
     else return (pSubMenu->cItems << 8) | ((pItem->fState|pItem->fType|Type) & 0xff);
  }
  else
     return (pItem->fType | pItem->fState | Type);  
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
  ITEM *item;
  LPWSTR text;
  ////// wine Code, seems to be faster.
  TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, uIDItem, lpString, nMaxCount, uFlag );

  if (lpString && nMaxCount) lpString[0] = '\0';

  if (!(item = MENU_FindItem( &hMenu, &uIDItem, uFlag )))
  {
      SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
      return 0;
  }

  text = item->Xlpstr ? DesktopPtrToUser(item->Xlpstr) : NULL;

  if (!text) return 0;
  if (!lpString || !nMaxCount) return WideCharToMultiByte( CP_ACP, 0, text, -1, NULL, 0, NULL, NULL );
  if (!WideCharToMultiByte( CP_ACP, 0, text, -1, lpString, nMaxCount, NULL, NULL ))
      lpString[nMaxCount-1] = 0;
  TRACE("A returning %s\n", lpString);
  return strlen(lpString);
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
  ITEM *item;
  LPWSTR text;

  TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, uIDItem, lpString, nMaxCount, uFlag );

  if (lpString && nMaxCount) lpString[0] = '\0';

  if (!(item = MENU_FindItem( &hMenu, &uIDItem, uFlag )))
  {
      SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
      return 0;
  }

  text = item->Xlpstr ? DesktopPtrToUser(item->Xlpstr) : NULL;

  if (!lpString || !nMaxCount) return text ? strlenW(text) : 0;
  if( !(text))
  {
      lpString[0] = 0;
      return 0;
  }
  lstrcpynW( lpString, text, nMaxCount );
  TRACE("W returning %S\n", lpString);
  return strlenW(lpString);
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
  PITEM pItem;
  if (!(pItem = MENU_FindItem( &hMenu, (UINT*)&nPos, MF_BYPOSITION ))) return NULL;

  if (pItem->spSubMenu)
  {
     PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
     HMENU hsubmenu = UserHMGetHandle(pSubMenu);
     if (IsMenu(hsubmenu)) return hsubmenu;
  }
  return NULL;
}

/*
 * @implemented
 */
HMENU
WINAPI
GetSystemMenu(HWND hWnd, BOOL bRevert)
{
    return NtUserGetSystemMenu(hWnd, bRevert);
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
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  RtlInitUnicodeString(&UnicodeString, 0);

  MENU_mnu2mnuii( uFlags, uIDNewItem, (LPCWSTR)lpNewItem, &mii, FALSE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), TRUE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
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
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, fByPosition, lpmii);

  RtlInitUnicodeString(&UnicodeString, 0);

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

  /* copy the text string */
  if (((mii.fMask & MIIM_STRING) ||
      ((mii.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mii.fType) == MF_STRING)))
        && mii.dwTypeData && !(GdiValidateHandle((HGDIOBJ)mii.dwTypeData)) )
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, TRUE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
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
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res = FALSE;

  /* while we could just pass 'lpmii' to win32k, we make a copy so that
     if a bad user passes bad data, we crash his process instead of the
     entire kernel */

  TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, fByPosition, lpmii);

  RtlInitUnicodeString(&MenuText, 0);    

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

  /* copy the text string */
  if (((mii.fMask & MIIM_STRING) ||
      ((mii.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mii.fType) == MF_STRING)))
        && mii.dwTypeData && !(GdiValidateHandle((HGDIOBJ)mii.dwTypeData)) )
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)lpmii->dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, TRUE, &mii, &MenuText);
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
  UNICODE_STRING MenuText;
  BOOL res;

  RtlInitUnicodeString(&MenuText, 0);    

  MENU_mnu2mnuii( uFlags, uIDNewItem, lpNewItem, &mii, TRUE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)mii.dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), TRUE, &mii, &MenuText);
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsMenu(
  HMENU Menu)
{
  if (ValidateHandle(Menu, TYPE_MENU)) return TRUE;
  return FALSE;
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
      if (!MENU_ParseResource(p, hMenu))
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
      ERR("Menu template version %d not supported.\n", version);
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
BOOL
WINAPI
ModifyMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  RtlInitUnicodeString(&UnicodeString, 0);

  MENU_mnu2mnuii( uFlags, uIDNewItem, (LPCWSTR)lpNewItem, &mii, FALSE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), FALSE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
ModifyMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res;

  RtlInitUnicodeString(&MenuText, 0);    

  MENU_mnu2mnuii( uFlags, uIDNewItem, lpNewItem, &mii, TRUE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)mii.dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), FALSE, &mii, &MenuText);
  return res;
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
  MENUINFO mi;
  BOOL res = FALSE;

  if (!lpcmi || (lpcmi->cbSize != sizeof(MENUINFO)))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return res;
  }

  memcpy(&mi, lpcmi, sizeof(MENUINFO));
  return NtUserThunkedMenuInfo(hmenu, (LPCMENUINFO)&mi);
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
  MENUITEMINFOW uItem;
  memset ( &uItem, 0, sizeof(uItem) );
  uItem.cbSize = sizeof(MENUITEMINFOW);
  uItem.fMask = MIIM_CHECKMARKS;
  uItem.hbmpUnchecked = hBitmapUnchecked;
  uItem.hbmpChecked = hBitmapChecked;
  return SetMenuItemInfoW(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), &uItem);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemInfoA(
  HMENU hmenu,
  UINT item,
  BOOL bypos,
  LPCMENUITEMINFOA lpmii)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL Ret;

  TRACE("hmenu %p, item %u, by pos %d, info %p\n", hmenu, item, bypos, lpmii);

  RtlInitUnicodeString(&UnicodeString, 0);

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;
/*
 *  MIIM_STRING              == good
 *  MIIM_TYPE & MFT_STRING   == good
 *  MIIM_STRING & MFT_STRING == good
 *  MIIM_STRING & MFT_OWNERDRAW == good
 */
  if (((mii.fMask & MIIM_STRING) ||
      ((mii.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mii.fType) == MF_STRING)))
        && mii.dwTypeData && !(GdiValidateHandle((HGDIOBJ)mii.dwTypeData)) )
  {
    /* cch is ignored when the content of a menu item is set by calling SetMenuItemInfo. */
     if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
     {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
     }
     mii.dwTypeData = UnicodeString.Buffer;
     mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
     UnicodeString.Buffer = NULL;
  }
  Ret = NtUserThunkedMenuItemInfo(hmenu, item, bypos, FALSE, &mii, &UnicodeString);
  if (UnicodeString.Buffer != NULL) RtlFreeUnicodeString(&UnicodeString);
  return Ret;
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
  UNICODE_STRING UnicodeString;
  BOOL Ret;

  TRACE("hmenu %p, item %u, by pos %d, info %p\n", hMenu, uItem, fByPosition, lpmii);

  RtlInitUnicodeString(&UnicodeString, 0);

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &MenuItemInfoW )) return FALSE;

  if (((MenuItemInfoW.fMask & MIIM_STRING) ||
      ((MenuItemInfoW.fMask & MIIM_TYPE) &&
                           (MENU_ITEM_TYPE(MenuItemInfoW.fType) == MF_STRING)))
        && MenuItemInfoW.dwTypeData && !(GdiValidateHandle((HGDIOBJ)MenuItemInfoW.dwTypeData)) )
  {
      RtlInitUnicodeString(&UnicodeString, (PCWSTR)MenuItemInfoW.dwTypeData);
      MenuItemInfoW.cch = strlenW(MenuItemInfoW.dwTypeData);
  }
  Ret = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, FALSE, &MenuItemInfoW, &UnicodeString);

  return Ret;
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
  return NtUserTrackPopupMenuEx( Menu,
                                Flags,
                                    x,
                                    y,
                                  Wnd,
                                 NULL); // LPTPMPARAMS is null
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
MenuWindowProcA(
		HWND   hWnd,
		ULONG_PTR Result,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  if ( Msg < WM_USER)
  {
     return PopupMenuWndProcA(hWnd, Msg, wParam, lParam );
  }
  return NtUserMessageCall(hWnd, Msg, wParam, lParam, Result, FNID_MENU, TRUE);
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
MenuWindowProcW(
		HWND   hWnd,
		ULONG_PTR Result,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  if ( Msg < WM_USER)
  {
     return PopupMenuWndProcW(hWnd, Msg, wParam, lParam );
  }
  return NtUserMessageCall(hWnd, Msg, wParam, lParam, Result, FNID_MENU, FALSE);
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

