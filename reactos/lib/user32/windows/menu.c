/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: menu.c,v 1.14 2003/08/04 10:13:51 weiden Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/menu.c
 * PURPOSE:         Menus
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <string.h>
#include <draw.h>
#include <window.h>

/* TYPES *********************************************************************/

typedef struct _MENUITEM
{
  UINT Type;
  UINT State;
  UINT Id;
  HMENU SubMenu;
  HBITMAP CheckBit;
  HBITMAP UnCheckBit;
  LPWSTR Text;
  ULONG ItemData;
  ULONG TypeData;
  HBITMAP BmpItem;
  RECT Rect;
  UINT XTab;
} MENUITEM, *PMENUITEM;

typedef struct _POPUP_MENU
{
  ULONG Magic;
  MENUITEM* Items;
  ULONG NrItems;
  ULONG Width;
  ULONG Height;
  ULONG FocusedItem;
  ULONG Flags;
  HWND hWnd;
  HWND hWndOwner;
  HBRUSH hbrBack;
  ULONG HelpId;
  ULONG cyMax;
  ULONG MenuData;
  ULONG Style;
} POPUP_MENU, *PPOPUP_MENU;

static HFONT hMenuFont = 0;

#define MENU_TYPE_MASK ((MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))
  
#define MENU_BAR_ITEMS_SPACE (12)
#define SEPARATOR_HEIGHT (5)
#define MENU_TAB_SPACE (8)

static ULONG ArrowBitmapWidth, ArrowBitmapHeight;

static HANDLE hStdMnArrow;

static HANDLE hMenuFontBold;

#define MENU_MAGIC (0x554D)

BOOL EndMenuCalled = FALSE;
HWND hTopPopupWnd = 0;

HANDLE hMenuDefSysPopup = 0;

#define NO_SELECTED_ITEM (0xffff)

#ifndef MF_END
#define MF_END             (0x0080)
#endif

#ifndef MIIM_STRING
#define MIIM_STRING      (0x00000040)
#endif


/* INTERNAL FUNCTIONS ********************************************************/

/* Rip the fun and easy to use and fun WINE unicode string manipulation routines. 
 * Of course I didnt copy the ASM code because we want this to be portable 
 * and it needs to go away.
 */

static inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline WCHAR *strncpyW( WCHAR *str1, const WCHAR *str2, int n )
{
    WCHAR *ret = str1;
    while (n-- > 0) if (!(*str1++ = *str2++)) break;
    while (n-- > 0) *str1++ = 0;
    return ret;
}

static inline WCHAR *strcpyW( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

static inline WCHAR *strcatW( WCHAR *dst, const WCHAR *src )
{
    strcpyW( dst + strlenW(dst), src );
    return dst;
}

NTSTATUS
STATIC HEAP_strdupA2W ( HANDLE hHeap, LPWSTR* ppszW, LPCSTR lpszA, UINT* NewLen )
{
  ULONG len;
  NTSTATUS Status;
  *ppszW = NULL;
  if ( !lpszA )
    return STATUS_SUCCESS;
  len = lstrlenA(lpszA);
  *ppszW = RtlAllocateHeap ( hHeap, 0, (len+1) * sizeof(WCHAR) );
  if ( !*ppszW )
    return STATUS_NO_MEMORY;
  Status = RtlMultiByteToUnicodeN ( *ppszW, len*sizeof(WCHAR), NULL, (PCHAR)lpszA, len ); 
  (*ppszW)[len] = L'\0';
  if(NewLen) (*NewLen) = (UINT)len;
  return Status;
}

#ifndef GET_WORD
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#endif
#ifndef GET_DWORD
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#endif

/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENUEX_ParseResource( LPCSTR res, HMENU hMenu)
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
	resinfo = GET_WORD(res);
        res += sizeof(WORD);
	/* Align the text on a word boundary.  */
	res += (~((int)res - 1)) & 1;
	mii.dwTypeData = (LPWSTR) res;
	res += (1 + strlenW(mii.dwTypeData)) * sizeof(WCHAR);
	/* Align the following fields on a dword boundary.  */
	res += (~((int)res - 1)) & 3;

	if (resinfo & 1) {	/* Pop-up? */
	    /* DWORD helpid = GET_DWORD(res); FIXME: use this.  */
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
	else if(!*mii.dwTypeData && !(mii.fType & MF_SEPARATOR))
	{
	    DbgPrint("WARN: Converting NULL menu item %04x, type %04x to SEPARATOR\n",
		mii.wID, mii.fType);
	    mii.fType |= MF_SEPARATOR;
	}
	InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
    } while (!(resinfo & MF_END));
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
    LPCSTR str;
    BOOL end = FALSE;

    do
    {
        flags = GET_WORD(res);
        
        /* remove MF_END flag before passing it to AppendMenu()! */
        end = (flags & MF_END);
        if(end) flags ^= MF_END;
        
        res += sizeof(WORD);
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        str = res;
        if (!unicode) res += strlen(str) + 1;
        else res += (strlenW((LPCWSTR)str) + 1) * sizeof(WCHAR);
        if (flags & MF_POPUP)
        {
            HMENU hSubMenu = CreatePopupMenu();
            if (!hSubMenu) return NULL;
            if (!(res = MENU_ParseResource( res, hSubMenu, unicode )))
                return NULL;
            if (!unicode) AppendMenuA( hMenu, flags, (UINT)hSubMenu, str );
            else AppendMenuW( hMenu, flags, (UINT)hSubMenu, (LPCWSTR)str );
        }
        else  /* Not a popup */
        {
            if (!unicode) AppendMenuA( hMenu, flags, id, *str ? str : NULL );
            else AppendMenuW( hMenu, flags, id,
                                *(LPCWSTR)str ? (LPCWSTR)str : NULL );
        }
    } while (!end);
    return res;
}


/* FUNCTIONS *****************************************************************/

static BOOL
MenuIsStringItem(ULONG TypeData)
{
  return((TypeData & MENU_TYPE_MASK) == MF_STRING);
}

BOOL
MenuInit(VOID)
{
  NONCLIENTMETRICSW ncm;
  BITMAP bm;

  hStdMnArrow = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_MNARROW));
  if (hStdMnArrow == NULL)
    {
      return(FALSE);
    }
  GetObjectA(hStdMnArrow, sizeof(bm), &bm);
  ArrowBitmapWidth = bm.bmWidth;
  ArrowBitmapHeight = bm.bmHeight;

  if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
      return(FALSE);
    }

  hMenuFont = CreateFontIndirect(&ncm.lfMenuFont);
  if (hMenuFont == NULL)
    {
      return(FALSE);
    }

  ncm.lfMenuFont.lfWeight = max(ncm.lfMenuFont.lfWeight + 300, 1000);
  
  hMenuFontBold = CreateFontIndirect(&ncm.lfMenuFont);
  if (hMenuFontBold == NULL)
    {
      return(FALSE);
    }

  return(TRUE);
}

/*static BOOL
MenuIsMenu(PPOPUP_MENU Menu)
{
  return(Menu->Magic == MENU_MAGIC);
}*/

static PPOPUP_MENU
MenuGetMenu(HMENU hMenu)
{
  PPOPUP_MENU Menu;
  Menu = (PPOPUP_MENU)hMenu;
  return(Menu);
}

static MENUITEM*
MenuFindItem(HMENU* hMenu, UINT* nPos, UINT wFlags)
{
  POPUP_MENU* Menu;
  ULONG i;

  if ((ULONG)(*hMenu) == 0xFFFF || (Menu = MenuGetMenu(*hMenu)) == NULL)
    {
      return(NULL);
    }
  if (wFlags & MF_BYPOSITION)
    {
      if ((*nPos) >= Menu->NrItems)
	{
	  return(NULL);
	}
      return(&Menu->Items[*nPos]);
    }
  else
    {
      MENUITEM* Item = Menu->Items;
      for (i = 0; i < Menu->NrItems; i++)
	{
	  if (Item->Id == (*nPos))
	    {
	      *nPos = i;
	      return(Item);
	    }
	  else if (Item->Type & MF_POPUP)
	    {
	      HMENU SubMenu = Item->SubMenu;
	      MENUITEM* SubItem = MenuFindItem(&SubMenu, nPos, wFlags);
	      if (SubItem)
		{
		  *hMenu = SubMenu;
		  return(SubItem);
		}
	    }
	}
    }
  return(NULL);
}


/*
 * @implemented
 */
WINBOOL STDCALL
AppendMenuA(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCSTR lpNewItem)
{
  DPRINT("AppendMenuA(hMenu 0x%X, uFlags 0x%X, uIDNewItem %d, "
         "lpNewItem %s\n", hMenu, uFlags, uIDNewItem, lpNewItem);
  return(InsertMenuA(hMenu, -1, uFlags | MF_BYPOSITION, uIDNewItem, 
		     lpNewItem));
}


/*
 * @implemented
 */
WINBOOL STDCALL
AppendMenuW(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCWSTR lpNewItem)
{
  DPRINT("AppendMenuW(hMenu 0x%X, uFlags 0x%X, uIDNewItem %d, "
         "lpNewItem %S\n", hMenu, uFlags, uIDNewItem, lpNewItem);
  return(InsertMenuW(hMenu, -1, uFlags | MF_BYPOSITION, uIDNewItem, 
		     lpNewItem));
}


/*
 * @implemented
 */
DWORD STDCALL
CheckMenuItem(HMENU hmenu,
	      UINT uIDCheckItem,
	      UINT uCheck)
{
  MENUITEM* Item;
  DWORD Ret;

  DPRINT("CheckMenuItem(hmenu 0x%X, uIDCheckItem %d, uCheck %d",
	 hmenu, uIDCheckItem, uCheck);
  if ((Item = MenuFindItem(&hmenu, &uIDCheckItem, uCheck)) == NULL)
    {
      return(-1);
    }
  Ret = Item->State & MF_CHECKED;
  if (uCheck & MF_CHECKED)
    {
      Item->State |= MF_CHECKED;
    }
  else
    {
      Item->State &= ~MF_CHECKED;
    }
  return(Ret);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
CheckMenuRadioItem(HMENU hmenu,
		   UINT idFirst,
		   UINT idLast,
		   UINT idCheck,
		   UINT uFlags)
{
  PMENUITEM First, Last, Check;
  HMENU FirstMenu = hmenu;
  HMENU LastMenu = hmenu;
  HMENU CheckMenu = hmenu;

  DPRINT("CheckMenuRadioItem(hmenu %X, idFirst %d, idLast %d, idCheck %d "
	 "uFlags %d", hmenu, idFirst, idLast, idCheck, uFlags);
  
  First = MenuFindItem(&FirstMenu, &idFirst, uFlags);
  Last = MenuFindItem(&LastMenu, &idLast, uFlags);
  Check = MenuFindItem(&CheckMenu, &idCheck, uFlags);

  if (First == NULL || Last == NULL || Check == NULL ||
      First > Last || FirstMenu != LastMenu || FirstMenu != CheckMenu ||
      Check > Last || Check < Last)
    {
      return(FALSE);
    }
  while (First < Last)
    {
      if (First == Check)
	{
	  First->TypeData |= MFT_RADIOCHECK;
	  First->State |= MFS_CHECKED;
	}
      else
	{
	  First->TypeData &= ~MFT_RADIOCHECK;
	  First->State &= ~MFS_CHECKED;
	}
      First++;
    }
  return(TRUE);
}


/*
 * @implemented
 */
HMENU STDCALL
CreateMenu(VOID)
{
    return NtUserCreateMenu();
}


/*
 * @implemented
 */
HMENU STDCALL
CreatePopupMenu(VOID)
{
  HMENU hMenu;
  PPOPUP_MENU Menu;

  hMenu = CreateMenu();
  Menu = MenuGetMenu(hMenu);
  Menu->Flags |= MF_POPUP;

  return (HMENU)Menu;
}


/*
 * @implemented
 */
WINBOOL STDCALL
DeleteMenu(HMENU hMenu,
	   UINT uPosition,
	   UINT uFlags)
{
  PMENUITEM Item;

  Item = MenuFindItem(&hMenu, &uPosition, uFlags);
  if (Item == NULL)
    {
      return(FALSE);
    }
  if (Item->TypeData & MF_POPUP)
    {
      DestroyMenu(Item->SubMenu);
    }
  RemoveMenu(hMenu, uPosition, uFlags | MF_BYPOSITION);
  return(TRUE);
}

VOID
MenuFreeItemData(PMENUITEM Item)
{
  if (MenuIsStringItem(Item->TypeData) && Item->Text != NULL)
    {
      HeapFree(GetProcessHeap(), 0, Item->Text);
    }
}

/*
 * @implemented
 */
WINBOOL STDCALL
DestroyMenu(HMENU hMenu)
{
    return NtUserDestroyMenu(hMenu);
}


/*
 * @implemented
 */
WINBOOL STDCALL
DrawMenuBar(HWND hWnd)
{
  ULONG Style;
  ULONG MenuId;
  PPOPUP_MENU Menu;

  Style = GetWindowLong(hWnd, GWL_STYLE);
  MenuId = GetWindowLong(hWnd, GWL_ID);
  
  if (!(Style & WS_CHILD) && MenuId != 0)
    {
      Menu = MenuGetMenu((HMENU)MenuId);

      Menu->Height = 0;
      Menu->hWndOwner = hWnd;
      SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
		   SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
      return(TRUE);
    }
  return(FALSE);
}


/*
 * @implemented
 */
UINT STDCALL
EnableMenuItem(HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable)
{
  PPOPUP_MENU Menu;
  PMENUITEM Item;
  UINT OldFlags;

  Menu = MenuGetMenu(hMenu);
  if (Menu == NULL)
    {
      return(-1);
    }
  Item = MenuFindItem(&hMenu, &uIDEnableItem, uEnable);
  if (Item == NULL)
    {
      return(-1);
    }
  OldFlags = Item->State & (MF_GRAYED | MF_DISABLED);
  Item->State ^= (OldFlags ^ uEnable) & (MF_GRAYED | MF_DISABLED);
  return(OldFlags);
}

BOOL STATIC
MenuIsMenuActive(VOID)
{
  return(hTopPopupWnd != NULL && IsWindowVisible(hTopPopupWnd));
}

/*
 * @implemented
 */
WINBOOL STDCALL
EndMenu(VOID)
{
  if (!EndMenuCalled && MenuIsMenuActive())
    {
      EndMenuCalled = TRUE;

      PostMessageA(hTopPopupWnd, WM_CANCELMODE, 0, 0);
    }
  return(TRUE);
}


/*
 * @implemented
 */
HMENU STDCALL
GetMenu(HWND hWnd)
{
  return((HMENU)GetWindowLong(hWnd, GWL_ID));
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuBarInfo(HWND hwnd,
	       LONG idObject,
	       LONG idItem,
	       PMENUBARINFO pmbi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
LONG STDCALL
GetMenuCheckMarkDimensions(VOID)
{
  return(MAKELONG(GetSystemMetrics(SM_CXMENUCHECK), 
		  GetSystemMetrics(SM_CYMENUCHECK)));
}


/*
 * @implemented
 */
UINT STDCALL
GetMenuDefaultItem(HMENU hMenu,
		   UINT fByPos,
		   UINT gmdiFlags)
{
  PPOPUP_MENU Menu;
  PMENUITEM Item;
  ULONG i;

  Menu = MenuGetMenu(hMenu);
  
  if (Menu->Items == NULL)
    {
      return(-1);
    }

  Item = Menu->Items;
  while (!(Item->State & MFS_DEFAULT))
    {
      i++;
      Item++;
      if (i >= Menu->NrItems)
	{
	  return(-1);
	}
    }

  if (!(gmdiFlags & GMDI_USEDISABLED) && (Item->State & MFS_DISABLED))
    {
      return(-1);
    }

  if ((Item->TypeData & MF_POPUP) && (gmdiFlags & GMDI_GOINTOPOPUPS))
    {
      UINT Ret;
      Ret = GetMenuDefaultItem(Item->SubMenu, fByPos, gmdiFlags);
      if (Ret != -1)
	{
	  return(Ret);
	}
    }
  return(fByPos ? i : Item->Id);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuInfo(HMENU hmenu,
	    LPMENUINFO lpcmi)
{
  PPOPUP_MENU Menu;

  Menu = MenuGetMenu(hmenu);

  if (lpcmi->fMask & MIM_BACKGROUND)
    {
      lpcmi->hbrBack = Menu->hbrBack;
    }
  if (lpcmi->fMask & MIM_HELPID)
    {
      lpcmi->dwContextHelpID = Menu->HelpId;
    }
  if (lpcmi->fMask & MIM_MAXHEIGHT)
    {
      lpcmi->cyMax = Menu->cyMax;
    }
  if (lpcmi->fMask & MIM_MENUDATA)
    {
      lpcmi->dwMenuData = Menu->MenuData;
    }
  if (lpcmi->fMask & MIM_STYLE)
    {
      lpcmi->dwStyle = Menu->Style;
    }
  return(TRUE);
}


/*
 * @implemented
 */
int STDCALL
GetMenuItemCount(HMENU hMenu)
{
  #if 0
  return NtUserBuildMenuItemList(hMenu, NULL, 0, 0);
  #else
  HANDLE hHeap = RtlGetProcessHeap();
  MENUITEMINFOW *mi;
  int i = 0;
  DWORD cnt = NtUserBuildMenuItemList(hMenu, NULL, 0, 0);
  DbgPrint("NtUserBuildMenuItemList() count = %d\n", cnt);
  mi = RtlAllocateHeap(hHeap, 0, cnt * sizeof(MENUITEMINFOW));
  while(cnt > i)
  {
    DbgPrint("  %d-> (NULL)\n", i);
    i++;
  }
  RtlFreeHeap (hHeap, 0, mi);
  return cnt;
  #endif
}


/*
 * @implemented
 */
UINT STDCALL
GetMenuItemID(HMENU hMenu,
	      int nPos)
{
  PMENUITEM Item;

  Item = MenuFindItem(&hMenu, &nPos, MF_BYPOSITION);
  if (Item == NULL)
    {
      return(0);
    }
  if (Item->TypeData & MF_POPUP)
    {
      return(-1);
    }
  return(Item->Id);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuItemRect(HWND hWnd,
		HMENU hMenu,
		UINT uItem,
		LPRECT lprcItem)
{
  UNIMPLEMENTED;
  return(FALSE);
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetMenuState(
  HMENU hMenu,
  UINT uId,
  UINT uFlags)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetMenuStringA(
  HMENU hMenu,
  UINT uIDItem,
  LPSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetMenuStringW(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HMENU
STDCALL
GetSubMenu(
  HMENU hMenu,
  int nPos)
{
  UNIMPLEMENTED;
  return (HMENU)0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
HiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InsertMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InsertMenuItemA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  MENUITEMINFOW mi;
  WINBOOL res = FALSE;
  BOOL CleanHeap = FALSE;
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();

  if((lpmii->cbSize == sizeof(MENUITEMINFO)) || 
     (lpmii->cbSize == sizeof(MENUITEMINFO) - sizeof(HBITMAP)))
  {
    memcpy(&mi, lpmii, lpmii->cbSize);
    
    /* copy the text string */
    if((mi.fMask & (MIIM_TYPE | MIIM_STRING)) && 
      (MENU_ITEM_TYPE(mi.fType) == MF_STRING) && mi.dwTypeData)
    {
      Status = HEAP_strdupA2W (hHeap, &mi.dwTypeData, (LPCSTR)mi.dwTypeData, &mi.cch);
      if (!NT_SUCCESS (Status))
      {
        SetLastError (RtlNtStatusToDosError(Status));
        return FALSE;
      }
      CleanHeap = TRUE;
      DbgPrint("InsertMenuItemA() Text = %ws\n", (PWSTR)mi.dwTypeData);
    }
    else
    {
      DbgPrint("InsertMenuItemA() No Text\n");
    }
    
    res = NtUserInsertMenuItem(hMenu, uItem, fByPosition, &mi);
    
    if(CleanHeap) RtlFreeHeap (hHeap, 0, mi.dwTypeData);
  }
  return res;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InsertMenuItemW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  MENUITEMINFOW mi;
  WINBOOL res = FALSE;
  BOOL CleanHeap = FALSE;
  ULONG len = 0;
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();

  if((lpmii->cbSize == sizeof(MENUITEMINFO)) || 
     (lpmii->cbSize == sizeof(MENUITEMINFO) - sizeof(HBITMAP)))
  {
    memcpy(&mi, lpmii, lpmii->cbSize);
    
    /* copy the text string */
    if((mi.fMask & (MIIM_TYPE | MIIM_STRING)) && 
      (MENU_ITEM_TYPE(mi.fType) == MF_STRING) && mi.dwTypeData)
    {
      len = lstrlenW(lpmii->dwTypeData);
      DbgPrint("InsertMenuItemW() len = %d\n", len);
      mi.dwTypeData = RtlAllocateHeap(hHeap, 0, (len + 1) * sizeof(WCHAR));
      if(!mi.dwTypeData)
      {
        SetLastError (RtlNtStatusToDosError(Status));
        return FALSE;
      }
      memcpy(&mi.dwTypeData, &lpmii->dwTypeData, len);
      CleanHeap = TRUE;
      mi.cch = len;
      DbgPrint("InsertMenuItemW() Text = %ws\n", (PWSTR)mi.dwTypeData);
    }
    else
    {
      DbgPrint("InsertMenuItemW() No Text\n");
    }
    
    res = NtUserInsertMenuItem(hMenu, uItem, fByPosition, &mi);
    
    if(CleanHeap) RtlFreeHeap (hHeap, 0, mi.dwTypeData);
  }
  return res;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InsertMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsMenu(
  HMENU hMenu)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HMENU STDCALL
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
HMENU STDCALL
LoadMenuIndirectA(CONST MENUTEMPLATE *lpMenuTemplate)
{
  return(LoadMenuIndirectW(lpMenuTemplate));
}


/*
 * @implemented
 */
HMENU STDCALL
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
	if (!MENU_ParseResource( p, hMenu, TRUE ))
	  {
	    DestroyMenu( hMenu );
	    return 0;
	  }
	return hMenu;
      case 1: /* extended format is version of 1 */
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu())) return 0;
	if (!MENUEX_ParseResource( p, hMenu))
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
HMENU STDCALL
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
 * @unimplemented
 */
int
STDCALL
MenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  POINT ptScreen)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ModifyMenuA(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ModifyMenuW(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
RemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetMenu(HWND hWnd,
	HMENU hMenu)
{
  ULONG Style = GetWindowLong(hWnd, GWL_STYLE);

  if (Style & WS_CHILD)
    {
      return(FALSE);
    }
  if (GetCapture() == hWnd)
    {
      ReleaseCapture();
    }
  
  SetWindowLong(hWnd, GWL_ID, (LONG)hMenu);
      
  if (hMenu != 0)
    {
      PPOPUP_MENU Menu;
      
      Menu = MenuGetMenu(hMenu);
      
      Menu->hWnd = hWnd;
      Menu->Height = 0;
    }
  if (IsWindowVisible(hWnd))
    {
      SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
		   SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
  return(TRUE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuItemBitmaps(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  HBITMAP hBitmapUnchecked,
  HBITMAP hBitmapChecked)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
TrackPopupMenu(
  HMENU hMenu,
  UINT uFlags,
  int x,
  int y,
  int nReserved,
  HWND hWnd,
  CONST RECT *prcRect)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
TrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuContextHelpId(HMENU hmenu,
          DWORD dwContextHelpId)
{
  return NtUserSetMenuContextHelpId(hmenu, dwContextHelpId);
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetMenuContextHelpId(HMENU hmenu)
{
  UNIMPLEMENTED;
  return(0);
}


static BOOL
MenuIsBitmapItem(ULONG TypeData)
{
  return((TypeData & MENU_TYPE_MASK) == MF_BITMAP);
}

static BOOL
MenuIsMagicItem(LPWSTR Text)
{
  return(LOWORD((ULONG)Text) < 12);
}

static HBITMAP
MenuLoadMagicItem(ULONG Id, ULONG Hilite, ULONG ItemData)
{
  UNIMPLEMENTED;
  return(NULL);
}

static VOID
MenuCalcItemSize(HDC hDC, PMENUITEM Item, HWND hWndOwner, LONG orgX, 
		 LONG orgY, BOOL MenuBar)
{
  ULONG CheckBitmapWidth = GetSystemMetrics(SM_CXMENUCHECK);

  SetRect(&Item->Rect, orgX, orgY, orgX, orgY);

  if (Item->TypeData & MF_OWNERDRAW)
    {
      MEASUREITEMSTRUCT mis;

      mis.CtlType = ODT_MENU;
      mis.CtlID = 0;
      mis.itemID = Item->Id;
      mis.itemData = Item->ItemData;
      mis.itemHeight = 0;
      mis.itemWidth = 0;
      SendMessageA(hWndOwner, WM_MEASUREITEM, 0, (LPARAM)&mis);
      Item->Rect.right += mis.itemWidth;

      if (MenuBar)
	{
	  Item->Rect.right += MENU_BAR_ITEMS_SPACE;

	  Item->Rect.bottom += GetSystemMetrics(SM_CYMENU);
	}
      else
	{
	  Item->Rect.bottom += mis.itemHeight;
	}
    }

  if (Item->ItemData & MF_SEPARATOR)
    {
      Item->Rect.bottom += SEPARATOR_HEIGHT;
      return;
    }

  if (!MenuBar)
    {
      Item->Rect.right += 2 * CheckBitmapWidth;
      if (Item->TypeData & MF_POPUP)
	{
	  Item->Rect.right += ArrowBitmapWidth;
	}
    }

  if (Item->TypeData & MF_OWNERDRAW)
    {
      return;
    }

  if (MenuIsBitmapItem(Item->TypeData))
    {
      HBITMAP Bmp;
      BITMAP Bm;

      if (MenuIsMagicItem(Item->Text))
	{
	  Bmp = MenuLoadMagicItem((LONG)Item->Text, 
				  Item->TypeData & MF_HILITE,
				  Item->ItemData);
	}
      else
	{
	  Bmp = (HBITMAP)Item->Text;
	}

      if (GetObjectA(Bmp, sizeof(Bm), &Bm))
	{
	  Item->Rect.right += Bm.bmWidth;
	  Item->Rect.bottom += Bm.bmHeight;
	}
    }

  if (MenuIsStringItem(Item->TypeData))
    {
      SIZE size;
      LPWSTR p;

      GetTextExtentPoint32W(hDC, Item->Text, wcslen(Item->Text), &size);

      Item->Rect.right += size.cx;
      Item->Rect.bottom += max(size.cy, GetSystemMetrics(SM_CYMENU));
      Item->XTab = 0;

      if (MenuBar)
	{
	  Item->Rect.right += MENU_BAR_ITEMS_SPACE;
	}
      else if ((p = wcschr(Item->Text, '\t')) != NULL)
	{
	  GetTextExtentPoint32W(hDC, Item->Text, (LONG)(p - Item->Text),
				&size);
	  Item->XTab = CheckBitmapWidth + MENU_TAB_SPACE + size.cx;
	  Item->Rect.right += MENU_TAB_SPACE;
	}
      else 
	{
	  if (wcschr(Item->Text, '\b') != NULL)
	    {
	      Item->Rect.right += MENU_TAB_SPACE;
	    }
	  Item->XTab = Item->Rect.right - CheckBitmapWidth - ArrowBitmapWidth;
	}
    }
}

static VOID
MenuMenuBarCalcSize(HDC hDC, LPRECT Rect, PPOPUP_MENU Menu, HWND hWndOwner)
{
  LONG maxY, start, helpPos, orgX, orgY, i;
  PMENUITEM Item;

  if (Rect == NULL || Menu == NULL)
    {
      return;
    }
  if (Menu->NrItems == 0)
    {
      return;
    }
  Menu->Width = Rect->right - Rect->left;
  Menu->Height = 0;
  maxY = Rect->top + 1;
  start = 0;
  helpPos = -1;
  while (start < Menu->NrItems)
    {
      Item = Menu->Items + start;

      orgX = Rect->left;
      orgY = maxY;

      for (i = start; i < Menu->NrItems; i++, Item++)
	{
	  if (helpPos == -1 && Item->TypeData & MF_RIGHTJUSTIFY)
	    {
	      helpPos = i;
	    }
	  if (i != start && Item->TypeData & (MF_MENUBREAK | MF_MENUBARBREAK))
	    {
	      break;
	    }
	  MenuCalcItemSize(hDC, Item, hWndOwner, orgX, orgY, TRUE);
	  if (Item->Rect.right > Rect->right)
	    {
	      if (i != start)
		{
		  break;
		}
	      else
		{
		  Item->Rect.right = Rect->right;
		}
	    }
	  maxY = max(maxY, Item->Rect.bottom);
	  orgX = Item->Rect.right;
	}

      for (;start < i; start++)
	{
	  Menu->Items[start].Rect.bottom = maxY;
	}
    }

  Rect->bottom = maxY;
  Menu->Height = Rect->bottom - Rect->top;

  Item = Menu->Items + Menu->NrItems - 1;
  orgY = Item->Rect.top;
  orgX = Rect->right;
  for (i = Menu->NrItems; i >= helpPos; i--, Item--)
    {
      if (helpPos == -1 || helpPos > i)
	{
	  break;
	}
      if (Item->Rect.top != orgY)
	{
	  break;
	}
      if (Item->Rect.right >= orgX)
	{
	  break;
	}
      Item->Rect.left += orgX - Item->Rect.right;
      Item->Rect.right = orgX;
      orgX = Item->Rect.left;
    }
}

VOID STATIC
MenuDrawMenuItem(HWND hWnd, HMENU hMenu, HWND hWndOwner, HDC hDC,
		 PMENUITEM Item, ULONG Height, BOOL MenuBar, ULONG OdAction)
{
  RECT Rect;

  if (Item->ItemData & MF_SYSMENU)
    {
      if (!IsIconic(hWnd))
	{
	  UserDrawSysMenuButton(hWnd, hDC, 
				Item->State & (MF_HILITE | MF_MOUSESELECT));
	}
      return;
    }

  if (Item->ItemData & MF_OWNERDRAW)
    {
      DRAWITEMSTRUCT dis;

      dis.CtlType = ODT_MENU;
      dis.CtlID = 0;
      dis.itemID = Item->Id;
      dis.itemData = (ULONG)Item->ItemData;
      dis.itemState = 0;
      if (Item->State & MF_CHECKED)
	{
	  dis.itemState |= ODS_CHECKED;
	}
      if (Item->State & MF_GRAYED)
	{
	  dis.itemState |= ODS_GRAYED;
	}
      if (Item->State & MF_HILITE)
	{
	  dis.itemState |= ODS_SELECTED;
	}
      dis.itemAction = OdAction;
      dis.hDC = hDC;
      dis.rcItem = Item->Rect;
      SendMessageA(hWndOwner, WM_DRAWITEM, 0, (LPARAM)&dis);
    }

  if (MenuBar && (Item->TypeData & MF_SEPARATOR))
    {
      return;
    }

  Rect = Item->Rect;

  if (!(Item->TypeData & MF_OWNERDRAW))
    {
      if (Item->State & MF_HILITE)
	{
	  if (!MenuIsBitmapItem(Item->TypeData))
	    {
	      FillRect(hDC, &Rect, GetSysColorBrush(COLOR_HIGHLIGHT));
	    }
	}
      else
	{
	  FillRect(hDC, &Rect, GetSysColorBrush(COLOR_MENU));
	}
    }

  SetBkMode(hDC, TRANSPARENT);

  if (!(Item->TypeData & MF_OWNERDRAW))
    {
      if (!MenuBar && (Item->TypeData & MF_MENUBARBREAK))
	{
	  SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
	  MoveToEx(hDC, Rect.left, 0, NULL);
	  LineTo(hDC, Rect.left, Height);
	}
      if (Item->TypeData & MF_SEPARATOR)
	{
	  SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
	  MoveToEx(hDC, Rect.left, Rect.right + (SEPARATOR_HEIGHT / 2), NULL);
	  LineTo(hDC, Rect.right, Rect.top + (SEPARATOR_HEIGHT / 2));
	  return;
	}
    }

  if (Item->State & MF_HILITE)
    {
      SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
      if (!MenuIsBitmapItem(Item->Type))
	{
	  SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
	}
    }
  else
    {
      if (Item->State & MF_GRAYED)
	{
	  SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
	}
      else
	{
	  SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
	}
      SetBkColor(hDC, GetSysColor(COLOR_MENU));
    }

  if (!MenuBar)
    {
      LONG y = Rect.top + Rect.bottom;
      ULONG CheckBitmapWidth = GetSystemMetrics(SM_CXMENUCHECK);
      ULONG CheckBitmapHeight = GetSystemMetrics(SM_CYMENUCHECK);
      
      if (!(Item->TypeData & MF_OWNERDRAW))
	{
	  HBITMAP bm;
	  if (Item->TypeData & MF_CHECKED)
	    {
	      bm = Item->CheckBit;
	    }
	  else
	    {
	      bm = Item->UnCheckBit;
	    }
	  if (bm)
	    {
	      HDC hDCMem = CreateCompatibleDC(hDC);
	      SelectObject(hDCMem, bm);
	      BitBlt(hDC, 
		     Rect.left, 
		     (y - CheckBitmapHeight) / 2,
		     CheckBitmapWidth,
		     CheckBitmapHeight,
		     hDCMem,
		     0,
		     0,
		     SRCCOPY);
	      DeleteDC(hDCMem);
	    }
	  else if (Item->TypeData & MF_CHECKED)
	    {
	      RECT r;
	      ULONG Type;
	      HBITMAP bm = CreateBitmap(CheckBitmapWidth, CheckBitmapHeight,
					1, 1, NULL);
	      HDC hDCMem = CreateCompatibleDC(hDC);
	      SelectObject(hDCMem, bm);
	      SetRect(&r, 0, 0, CheckBitmapWidth, CheckBitmapHeight);
	      if (Item->TypeData & MFT_RADIOCHECK)
		{
		  Type = DFCS_MENUBULLET;
		}
	      else
		{
		  Type = DFCS_MENUCHECK;
		}
	      DrawFrameControl(hDCMem, &r, DFC_MENU, Type);
	      BitBlt(hDC, Rect.left, (y - r.bottom) / 2, r.right, r.bottom,
		     hDCMem, 0, 0, SRCCOPY);
	      DeleteDC(hDCMem);
	      DeleteObject(bm);
	    }
	}

      if (Item->TypeData & MF_POPUP)
	{
	  HDC hDCMem = CreateCompatibleDC(hDC);

	  SelectObject(hDCMem, hStdMnArrow);
	  BitBlt(hDC, 
		 Rect.right - ArrowBitmapWidth - 1,
		 (y - ArrowBitmapHeight) / 2,
		 ArrowBitmapWidth,
		 ArrowBitmapHeight,
		 hDCMem,
		 0,
		 0,
		 SRCCOPY);
	  DeleteDC(hDCMem);
	}

      Rect.left += CheckBitmapWidth;
      Rect.right -= ArrowBitmapWidth;
    }

  if (Item->TypeData & MF_OWNERDRAW)
    {
      return;
    }

  if (MenuIsBitmapItem(Item->TypeData))
    {
      LONG left, top, w, h;
      ULONG Rop;
      HBITMAP ResBmp = 0;
      HDC hDCMem = CreateCompatibleDC(hDC);

      if (MenuIsMagicItem(Item->Text))
	{
	  ResBmp = MenuLoadMagicItem((LONG)Item->Text, Item->State & MF_HILITE,
				     Item->ItemData);
	}
      else
	{
	  ResBmp = (HBITMAP)Item->Text;
	}

      if (ResBmp)
	{
	  BITMAP Bm;

	  GetObjectA(ResBmp, sizeof(Bm), &Bm);
	  SelectObject(hDCMem, ResBmp);
	  h = Rect.bottom - Rect.top;
	  if (h > Bm.bmHeight) 
	    {
	      top = Rect.top + (h - Bm.bmHeight) / 2;
	    }
	  else
	    {
	      top = Rect.top;
	    }
	  w = Rect.right - Rect.left;
	  left = Rect.left;
	  left++;
	  w -= 2;
	  if ((Item->State & MF_HILITE) && !MenuIsMagicItem(Item->Text) &&
	      !MenuBar)
	    {
	      Rop = MERGEPAINT;
	    }
	  else
	    {
	      Rop = SRCCOPY;
	    }
	  BitBlt(hDC, left, top, w, h, hDCMem, 0, 0, Rop);
	}
      DeleteDC(hDCMem);
      return;
    }
  else if (MenuIsStringItem(Item->Type))
    {
      LONG i;
      HFONT hFontOld = 0;
      UINT Format = (MenuBar) ? DT_CENTER | DT_VCENTER | DT_SINGLELINE :
	DT_LEFT | DT_VCENTER | DT_SINGLELINE;

      if (Item->State & MFS_DEFAULT)
	{
	  hFontOld = SelectObject(hDC, hMenuFontBold);
	}

      if (MenuBar)
	{
	  Rect.left += MENU_BAR_ITEMS_SPACE / 2;
	  Rect.right += MENU_BAR_ITEMS_SPACE / 2;
	  i = wcslen(Item->Text);
	}
      else
	{
	  for (i = 0; i < wcslen(Item->Text); i++)
	    {
	      if (Item->Text[i] == '\t' || Item->Text[i] == '\b')
		{
		  break;
		}
	    }
	}

      DrawTextW(hDC, Item->Text, i, &Rect, Format);

      if (Item->Text[i] != '\0')
	{
	  if (Item->Text[i] == '\t')
	    {
	      Rect.left = Item->XTab;
	      Format = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	    }
	  else
	    {
	      Format = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;
	    }
	  DrawTextW(hDC, Item->Text + i + 1, -1, &Rect, Format);
	}

      if (hFontOld != 0)
	{
	  SelectObject(hDC, hFontOld);
	}
    }
}

UINT
MenuDrawMenuBar(HDC hDC, LPRECT Rect, HWND hWnd, BOOL Draw)
{
  ULONG MenuID;
  PPOPUP_MENU Menu;
  HFONT hFontOld;
  ULONG i;

  MenuID = GetWindowLong(hWnd, GWL_ID);
  Menu = MenuGetMenu((HMENU)MenuID);

  if (Menu == NULL || Rect == NULL)
    {
      return(GetSystemMetrics(SM_CYMENU));
    }

  hFontOld = SelectObject(hDC, hMenuFont);

  if (Menu->Height == 0)
    {
      MenuMenuBarCalcSize(hDC, Rect, Menu, hWnd);
    }

  Rect->bottom = Rect->top + Menu->Height;

  if (!Draw)
    {
      SelectObject(hDC, hFontOld);
      return(Menu->Height);
    }

  FillRect(hDC, Rect, GetSysColorBrush(COLOR_MENU));

  SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
  MoveToEx(hDC, Rect->left, Rect->bottom, NULL);
  LineTo(hDC, Rect->right, Rect->bottom);

  if (Menu->NrItems == 0)
    {
      SelectObject(hDC, hFontOld);
      return(GetSystemMetrics(SM_CYMENU));
    }

  for (i = 0; i < Menu->NrItems; i++)
    {
      MenuDrawMenuItem(hWnd, (HMENU)MenuID, hWnd, hDC,
		       Menu->Items + i, Menu->Height, TRUE, ODA_DRAWENTIRE);
    }

  SelectObject(hDC, hFontOld);
  return(Menu->Height);
}

ULONG
MenuGetMenuBarHeight(HWND hWnd, ULONG MenuBarWidth, LONG OrgX, LONG OrgY)
{
  ULONG MenuId;
  PPOPUP_MENU Menu;
  RECT Rect;
  HDC hDC;

  MenuId = GetWindowLong(hWnd, GWL_ID);
  Menu = MenuGetMenu((HMENU)MenuId);
  if (Menu == NULL)
    {
      return(0);
    }
  hDC = GetDCEx(hWnd, 0, DCX_CACHE | DCX_WINDOW);
  SelectObject(hDC, hMenuFont);
  SetRect(&Rect, OrgX, OrgY, OrgX + MenuBarWidth, 
	  OrgY + GetSystemMetrics(SM_CYMENU));
  MenuMenuBarCalcSize(hDC, &Rect, Menu, hWnd);
  ReleaseDC(hWnd, hDC);
  return(Menu->Height);
}

VOID
MenuTrackMouseMenuBar(HWND hWnd, ULONG Ht, POINT Pt)
{
}

VOID
MenuTrackKbdMenuBar(HWND hWnd, ULONG wParam, ULONG Key)
{
}
