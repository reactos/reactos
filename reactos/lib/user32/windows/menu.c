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
/* $Id: menu.c,v 1.5 2002/09/08 10:23:12 chorns Exp $
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
  DWORD ItemData;
  DWORD TypeData;
  HBITMAP BmpItem;
  RECT Rect;
  UINT XTab;
} MENUITEM, *PMENUITEM;

typedef struct _POPUP_MENU 
{
  MENUITEM* Items;
  WORD NrItems;
} POPUP_MENU, *PPOPUP_MENU;

/* FUNCTIONS *****************************************************************/

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

  if ((*hMenu) == 0xFFFF || (Menu = MenuGetMenu(*hMenu)) == NULL)
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

WINBOOL
STDCALL
CheckMenuRadioItem(
  HMENU hmenu,
  UINT idFirst,
  UINT idLast,
  UINT idCheck,
  UINT uFlags)
{
  return FALSE;
}
HMENU
STDCALL
CreateMenu(VOID)
{
  return (HMENU)0;
}

HMENU
STDCALL
CreatePopupMenu(VOID)
{
  return (HMENU)0;
}
WINBOOL
STDCALL
DeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  return FALSE;
}
WINBOOL
STDCALL
DestroyMenu(
  HMENU hMenu)
{
  return FALSE;
}
WINBOOL
STDCALL
DrawMenuBar(
  HWND hWnd)
{
  return FALSE;
}
WINBOOL
STDCALL
EnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
  return FALSE;
}
WINBOOL
STDCALL
EndMenu(VOID)
{
  return FALSE;
}
HMENU
STDCALL
GetMenu(
  HWND hWnd)
{
  return (HMENU)0;
}

WINBOOL
STDCALL
GetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi)
{
  return FALSE;
}

LONG
STDCALL
GetMenuCheckMarkDimensions(VOID)
{
  return 0;
}

UINT
STDCALL
GetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags)
{
  return 0;
}

WINBOOL
STDCALL
GetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  return FALSE;
}

int
STDCALL
GetMenuItemCount(
  HMENU hMenu)
{
  return 0;
}

UINT
STDCALL
GetMenuItemID(
  HMENU hMenu,
  int nPos)
{
  return 0;
}

WINBOOL
STDCALL
GetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
GetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
GetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
  return FALSE;
}

UINT
STDCALL
GetMenuState(
  HMENU hMenu,
  UINT uId,
  UINT uFlags)
{
  return 0;
}

int
STDCALL
GetMenuStringA(
  HMENU hMenu,
  UINT uIDItem,
  LPSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  return 0;
}

int
STDCALL
GetMenuStringW(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  return 0;
}
HMENU
STDCALL
GetSubMenu(
  HMENU hMenu,
  int nPos)
{
  return (HMENU)0;
}
WINBOOL
STDCALL
HiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  return FALSE;
}
WINBOOL
STDCALL
InsertMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuItemA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuItemW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  return FALSE;
}
WINBOOL
STDCALL
IsMenu(
  HMENU hMenu)
{
  return FALSE;
}
HMENU
STDCALL
LoadMenuA(
  HINSTANCE hInstance,
  LPCSTR lpMenuName)
{
  return (HMENU)0;
}

HMENU
STDCALL
LoadMenuIndirectA(
  CONST MENUTEMPLATE *lpMenuTemplate)
{
  return (HMENU)0;
}

HMENU
STDCALL
LoadMenuIndirectW(
  CONST MENUTEMPLATE *lpMenuTemplate)
{
  return (HMENU)0;
}

HMENU
STDCALL
LoadMenuW(
  HINSTANCE hInstance,
  LPCWSTR lpMenuName)
{
  return (HMENU)0;
}
int
STDCALL
MenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  POINT ptScreen)
{
  return 0;
}
WINBOOL
STDCALL
ModifyMenuA(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  return FALSE;
}

WINBOOL
STDCALL
ModifyMenuW(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  return FALSE;
}
WINBOOL
STDCALL
RemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  return FALSE;
}
WINBOOL
STDCALL
SetMenu(
  HWND hWnd,
  HMENU hMenu)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuItemBitmaps(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  HBITMAP hBitmapUnchecked,
  HBITMAP hBitmapChecked)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

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
  return FALSE;
}

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
  return FALSE;
}
