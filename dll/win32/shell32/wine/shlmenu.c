/*
 * see www.geocities.com/SiliconValley/4942/filemenu.html
 *
 * Copyright 1999, 2000 Juergen Schmied
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <undocshell.h>
#include <shlwapi.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#include "pidl.h"
#include "shell32_main.h"

#ifdef FM_SEPARATOR
#undef FM_SEPARATOR
#endif
#define FM_SEPARATOR (LPCWSTR)1

static BOOL FileMenu_AppendItemW(HMENU hMenu, LPCWSTR lpText, UINT uID, int icon,
                                 HMENU hMenuPopup, int nItemHeight);

typedef struct
{
	BOOL		bInitialized;
	BOOL		bFixedItems;
	/* create */
	COLORREF	crBorderColor;
	int		nBorderWidth;
	HBITMAP		hBorderBmp;

	/* insert using pidl */
	LPITEMIDLIST	pidl;
	UINT		uID;
	UINT		uFlags;
	UINT		uEnumFlags;
	LPFNFMCALLBACK lpfnCallback;
} FMINFO, *LPFMINFO;

typedef struct
{	int	cchItemText;
	int	iIconIndex;
	HMENU	hMenu;
	WCHAR	szItemText[1];
} FMITEM, * LPFMITEM;

static BOOL bAbortInit;

#define	CCH_MAXITEMTEXT 256

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static LPFMINFO FM_GetMenuInfo(HMENU hmenu)
{
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hmenu, &MenuInfo))
	  return NULL;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;

	if ((menudata == 0) || (MenuInfo.cbSize != sizeof(MENUINFO)))
	{
	  ERR("menudata corrupt: %p %u\n", menudata, MenuInfo.cbSize);
	  return 0;
	}

	return menudata;

}
/*************************************************************************
 * FM_SetMenuParameter				[internal]
 *
 */
static LPFMINFO FM_SetMenuParameter(
	HMENU hmenu,
	UINT uID,
	LPCITEMIDLIST pidl,
	UINT uFlags,
	UINT uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	LPFMINFO	menudata;

	TRACE("\n");

	menudata = FM_GetMenuInfo(hmenu);

	SHFree(menudata->pidl);

	menudata->uID = uID;
	menudata->pidl = ILClone(pidl);
	menudata->uFlags = uFlags;
	menudata->uEnumFlags = uEnumFlags;
	menudata->lpfnCallback = lpfnCallback;

	return menudata;
}

/*************************************************************************
 * FM_InitMenuPopup				[internal]
 *
 */
static int FM_InitMenuPopup(HMENU hmenu, LPCITEMIDLIST pAlternatePidl)
{	IShellFolder	*lpsf, *lpsf2;
	ULONG		ulItemAttr = SFGAO_FOLDER;
	UINT		uID, uEnumFlags;
	LPFNFMCALLBACK	lpfnCallback;
	LPCITEMIDLIST	pidl;
	WCHAR		sTemp[MAX_PATH];
	int		NumberOfItems = 0, iIcon;
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	TRACE("%p %p\n", hmenu, pAlternatePidl);

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hmenu, &MenuInfo))
	  return FALSE;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;

	if ((menudata == 0) || (MenuInfo.cbSize != sizeof(MENUINFO)))
	{
	  ERR("menudata corrupt: %p %u\n", menudata, MenuInfo.cbSize);
	  return 0;
	}

	if (menudata->bInitialized)
	  return 0;

	pidl = (pAlternatePidl? pAlternatePidl: menudata->pidl);
	if (!pidl)
	  return 0;

	uID = menudata->uID;
	uEnumFlags = menudata->uEnumFlags;
	lpfnCallback = menudata->lpfnCallback;
	menudata->bInitialized = FALSE;

	SetMenuInfo(hmenu, &MenuInfo);

	if (SUCCEEDED (SHGetDesktopFolder(&lpsf)))
	{
	  if (SUCCEEDED(IShellFolder_BindToObject(lpsf, pidl,0,&IID_IShellFolder,(LPVOID *)&lpsf2)))
	  {
	    IEnumIDList	*lpe = NULL;

	    if (SUCCEEDED (IShellFolder_EnumObjects(lpsf2, 0, uEnumFlags, &lpe )))
	    {

	      LPITEMIDLIST pidlTemp = NULL;
	      ULONG ulFetched;

	      while ((!bAbortInit) && (S_OK == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched)))
	      {
		if (SUCCEEDED (IShellFolder_GetAttributesOf(lpsf, 1, (LPCITEMIDLIST*)&pidlTemp, &ulItemAttr)))
		{
		  ILGetDisplayNameExW(NULL, pidlTemp, sTemp, ILGDN_FORPARSING);
		  if (! (PidlToSicIndex(lpsf, pidlTemp, FALSE, 0, &iIcon)))
		    iIcon = FM_BLANK_ICON;
		  if ( SFGAO_FOLDER & ulItemAttr)
		  {
		    LPFMINFO lpFmMi;
		    MENUINFO MenuInfo;
		    HMENU hMenuPopup = CreatePopupMenu();

		    lpFmMi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FMINFO));

		    lpFmMi->pidl = ILCombine(pidl, pidlTemp);
		    lpFmMi->uEnumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

		    MenuInfo.cbSize = sizeof(MENUINFO);
		    MenuInfo.fMask = MIM_MENUDATA;
		    MenuInfo.dwMenuData = (ULONG_PTR) lpFmMi;
		    SetMenuInfo (hMenuPopup, &MenuInfo);

		    FileMenu_AppendItemW (hmenu, sTemp, uID, iIcon, hMenuPopup, FM_DEFAULT_HEIGHT);
		  }
		  else
		  {
		    LPWSTR pExt = PathFindExtensionW(sTemp);
		    if (pExt)
		      *pExt = 0;
		    FileMenu_AppendItemW (hmenu, sTemp, uID, iIcon, 0, FM_DEFAULT_HEIGHT);
		  }
		}

		if (lpfnCallback)
		{
		  TRACE("enter callback\n");
		  lpfnCallback ( pidl, pidlTemp);
		  TRACE("leave callback\n");
		}

		NumberOfItems++;
	      }
	      IEnumIDList_Release (lpe);
	    }
	    IShellFolder_Release(lpsf2);
	  }
	  IShellFolder_Release(lpsf);
	}

	if ( GetMenuItemCount (hmenu) == 0 )
	{
	  FileMenu_AppendItemW (hmenu, L"(empty)", uID, FM_BLANK_ICON, 0, FM_DEFAULT_HEIGHT);
	  NumberOfItems++;
	}

	menudata->bInitialized = TRUE;
	SetMenuInfo(hmenu, &MenuInfo);

	return NumberOfItems;
}
/*************************************************************************
 * FileMenu_Create				[SHELL32.114]
 *
 * NOTES
 *  for non-root menus values are
 *  (ffffffff,00000000,00000000,00000000,00000000)
 */
HMENU WINAPI FileMenu_Create (
	COLORREF crBorderColor,
	int nBorderWidth,
	HBITMAP hBorderBmp,
	int nSelHeight,
	UINT uFlags)
{
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	HMENU hMenu = CreatePopupMenu();

	TRACE("0x%08x 0x%08x %p 0x%08x 0x%08x  hMenu=%p\n",
	crBorderColor, nBorderWidth, hBorderBmp, nSelHeight, uFlags, hMenu);

	menudata = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FMINFO));
	menudata->crBorderColor = crBorderColor;
	menudata->nBorderWidth = nBorderWidth;
	menudata->hBorderBmp = hBorderBmp;

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;
	MenuInfo.dwMenuData = (ULONG_PTR) menudata;
	SetMenuInfo (hMenu, &MenuInfo);

	return hMenu;
}

/*************************************************************************
 * FileMenu_Destroy				[SHELL32.118]
 *
 * NOTES
 *  exported by name
 */
void WINAPI FileMenu_Destroy (HMENU hmenu)
{
	LPFMINFO	menudata;

	TRACE("%p\n", hmenu);

	FileMenu_DeleteAllItems (hmenu);

	menudata = FM_GetMenuInfo(hmenu);

	SHFree( menudata->pidl);
	HeapFree(GetProcessHeap(), 0, menudata);

	DestroyMenu (hmenu);
}

/*************************************************************************
 * FileMenu_AppendItem			[SHELL32.115]
 *
 */
static BOOL FileMenu_AppendItemW(
	HMENU hMenu,
	LPCWSTR lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	MENUITEMINFOW	mii;
	LPFMITEM	myItem;
	LPFMINFO	menudata;
	MENUINFO        MenuInfo;


	TRACE("%p %s 0x%08x 0x%08x %p 0x%08x\n",
	  hMenu, (lpText!=FM_SEPARATOR) ? debugstr_w(lpText) : NULL,
	  uID, icon, hMenuPopup, nItemHeight);

	ZeroMemory (&mii, sizeof(MENUITEMINFOW));

	mii.cbSize = sizeof(MENUITEMINFOW);

	if (lpText != FM_SEPARATOR)
	{
	  int len = strlenW (lpText);
          myItem = SHAlloc(sizeof(FMITEM) + len*sizeof(WCHAR));
	  strcpyW (myItem->szItemText, lpText);
	  myItem->cchItemText = len;
	  myItem->iIconIndex = icon;
	  myItem->hMenu = hMenu;
	  mii.fMask = MIIM_DATA;
	  mii.dwItemData = (ULONG_PTR) myItem;
	}

	if ( hMenuPopup )
	{ /* sub menu */
	  mii.fMask |= MIIM_TYPE | MIIM_SUBMENU;
	  mii.fType = MFT_OWNERDRAW;
	  mii.hSubMenu = hMenuPopup;
	}
	else if (lpText == FM_SEPARATOR )
	{ mii.fMask |= MIIM_ID | MIIM_TYPE;
	  mii.fType = MFT_SEPARATOR;
	}
	else
	{ /* normal item */
	  mii.fMask |= MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.fState = MFS_ENABLED | MFS_DEFAULT;
	  mii.fType = MFT_OWNERDRAW;
	}
	mii.wID = uID;

	InsertMenuItemW (hMenu, (UINT)-1, TRUE, &mii);

	/* set bFixedItems to true */
	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hMenu, &MenuInfo))
	  return FALSE;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;
	if ((menudata == 0) || (MenuInfo.cbSize != sizeof(MENUINFO)))
	{
	  ERR("menudata corrupt: %p %u\n", menudata, MenuInfo.cbSize);
	  return FALSE;
	}

	menudata->bFixedItems = TRUE;
	SetMenuInfo(hMenu, &MenuInfo);

	return TRUE;

}

/**********************************************************************/

BOOL WINAPI FileMenu_AppendItemAW(
	HMENU hMenu,
	LPCVOID lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	BOOL ret;

        if (!lpText) return FALSE;

	if (SHELL_OsIsUnicode() || lpText == FM_SEPARATOR)
	  ret = FileMenu_AppendItemW(hMenu, lpText, uID, icon, hMenuPopup, nItemHeight);
        else
	{
	  DWORD len = MultiByteToWideChar( CP_ACP, 0, lpText, -1, NULL, 0 );
	  LPWSTR lpszText = HeapAlloc ( GetProcessHeap(), 0, len*sizeof(WCHAR) );
	  if (!lpszText) return FALSE;
	  MultiByteToWideChar( CP_ACP, 0, lpText, -1, lpszText, len );
	  ret = FileMenu_AppendItemW(hMenu, lpszText, uID, icon, hMenuPopup, nItemHeight);
	  HeapFree( GetProcessHeap(), 0, lpszText );
	}

	return ret;
}

/*************************************************************************
 * FileMenu_InsertUsingPidl			[SHELL32.110]
 *
 * NOTES
 *	uEnumFlags	any SHCONTF flag
 */
int WINAPI FileMenu_InsertUsingPidl (
	HMENU hmenu,
	UINT uID,
	LPCITEMIDLIST pidl,
	UINT uFlags,
	UINT uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	TRACE("%p 0x%08x %p 0x%08x 0x%08x %p\n",
	hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	pdump (pidl);

	bAbortInit = FALSE;

	FM_SetMenuParameter(hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	return FM_InitMenuPopup(hmenu, NULL);
}

/*************************************************************************
 * FileMenu_ReplaceUsingPidl			[SHELL32.113]
 *
 * FIXME: the static items are deleted but won't be refreshed
 */
int WINAPI FileMenu_ReplaceUsingPidl(
	HMENU	hmenu,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	TRACE("%p 0x%08x %p 0x%08x %p\n",
	hmenu, uID, pidl, uEnumFlags, lpfnCallback);

	FileMenu_DeleteAllItems (hmenu);

	FM_SetMenuParameter(hmenu, uID, pidl, 0, uEnumFlags, lpfnCallback);

	return FM_InitMenuPopup(hmenu, NULL);
}

/*************************************************************************
 * FileMenu_Invalidate			[SHELL32.111]
 */
void WINAPI FileMenu_Invalidate (HMENU hMenu)
{
	FIXME("%p\n",hMenu);
}

/*************************************************************************
 * FileMenu_FindSubMenuByPidl			[SHELL32.106]
 */
HMENU WINAPI FileMenu_FindSubMenuByPidl(
	HMENU	hMenu,
	LPCITEMIDLIST	pidl)
{
	FIXME("%p %p\n",hMenu, pidl);
	return 0;
}

/*************************************************************************
 * FileMenu_AppendFilesForPidl			[SHELL32.124]
 */
int WINAPI FileMenu_AppendFilesForPidl(
	HMENU	hmenu,
	LPCITEMIDLIST	pidl,
	BOOL	bAddSeparator)
{
	LPFMINFO	menudata;

	menudata = FM_GetMenuInfo(hmenu);

	menudata->bInitialized = FALSE;

	FM_InitMenuPopup(hmenu, pidl);

	if (bAddSeparator)
	  FileMenu_AppendItemW (hmenu, FM_SEPARATOR, 0, 0, 0, FM_DEFAULT_HEIGHT);

	TRACE("%p %p 0x%08x\n",hmenu, pidl,bAddSeparator);

	return 0;
}
/*************************************************************************
 * FileMenu_AddFilesForPidl			[SHELL32.125]
 *
 * NOTES
 *	uEnumFlags	any SHCONTF flag
 */
int WINAPI FileMenu_AddFilesForPidl (
	HMENU	hmenu,
	UINT	uReserved,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uFlags,
	UINT	uEnumFlags,
	LPFNFMCALLBACK	lpfnCallback)
{
	TRACE("%p 0x%08x 0x%08x %p 0x%08x 0x%08x %p\n",
	hmenu, uReserved, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	return FileMenu_InsertUsingPidl ( hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

}


/*************************************************************************
 * FileMenu_TrackPopupMenuEx			[SHELL32.116]
 */
BOOL WINAPI FileMenu_TrackPopupMenuEx (
	HMENU hMenu,
	UINT uFlags,
	int x,
	int y,
	HWND hWnd,
	LPTPMPARAMS lptpm)
{
	TRACE("%p 0x%08x 0x%x 0x%x %p %p\n",
	hMenu, uFlags, x, y, hWnd, lptpm);
	return TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, lptpm);
}

/*************************************************************************
 * FileMenu_GetLastSelectedItemPidls		[SHELL32.107]
 */
BOOL WINAPI FileMenu_GetLastSelectedItemPidls(
	UINT	uReserved,
	LPCITEMIDLIST	*ppidlFolder,
	LPCITEMIDLIST	*ppidlItem)
{
	FIXME("0x%08x %p %p\n",uReserved, ppidlFolder, ppidlItem);
	return FALSE;
}

#define FM_ICON_SIZE	16
#define FM_Y_SPACE	4
#define FM_SPACE1	4
#define FM_SPACE2	2
#define FM_LEFTBORDER	2
#define FM_RIGHTBORDER	8
/*************************************************************************
 * FileMenu_MeasureItem				[SHELL32.112]
 */
LRESULT WINAPI FileMenu_MeasureItem(
	HWND	hWnd,
	LPMEASUREITEMSTRUCT	lpmis)
{
	LPFMITEM pMyItem = (LPFMITEM)(lpmis->itemData);
	HDC hdc = GetDC(hWnd);
	SIZE size;
	LPFMINFO menuinfo;

	TRACE("%p %p %s\n", hWnd, lpmis, debugstr_w(pMyItem->szItemText));

	GetTextExtentPoint32W(hdc, pMyItem->szItemText, pMyItem->cchItemText, &size);

	lpmis->itemWidth = size.cx + FM_LEFTBORDER + FM_ICON_SIZE + FM_SPACE1 + FM_SPACE2 + FM_RIGHTBORDER;
	lpmis->itemHeight = (size.cy > (FM_ICON_SIZE + FM_Y_SPACE)) ? size.cy : (FM_ICON_SIZE + FM_Y_SPACE);

	/* add the menubitmap */
	menuinfo = FM_GetMenuInfo(pMyItem->hMenu);
	if (menuinfo->nBorderWidth)
	  lpmis->itemWidth += menuinfo->nBorderWidth;

	TRACE("-- 0x%04x 0x%04x\n", lpmis->itemWidth, lpmis->itemHeight);
	ReleaseDC (hWnd, hdc);
	return 0;
}
/*************************************************************************
 * FileMenu_DrawItem				[SHELL32.105]
 */
LRESULT WINAPI FileMenu_DrawItem(
	HWND			hWnd,
	LPDRAWITEMSTRUCT	lpdis)
{
	LPFMITEM pMyItem = (LPFMITEM)(lpdis->itemData);
	COLORREF clrPrevText, clrPrevBkgnd;
	int xi,yi,xt,yt;
	HIMAGELIST hImageList;
	RECT TextRect;
	LPFMINFO menuinfo;

	TRACE("%p %p %s\n", hWnd, lpdis, debugstr_w(pMyItem->szItemText));

	if (lpdis->itemState & ODS_SELECTED)
	{
	  clrPrevText = SetTextColor(lpdis->hDC, GetSysColor (COLOR_HIGHLIGHTTEXT));
	  clrPrevBkgnd = SetBkColor(lpdis->hDC, GetSysColor (COLOR_HIGHLIGHT));
	}
	else
	{
	  clrPrevText = SetTextColor(lpdis->hDC, GetSysColor (COLOR_MENUTEXT));
	  clrPrevBkgnd = SetBkColor(lpdis->hDC, GetSysColor (COLOR_MENU));
	}

        TextRect = lpdis->rcItem;

	/* add the menubitmap */
	menuinfo = FM_GetMenuInfo(pMyItem->hMenu);
	if (menuinfo->nBorderWidth)
	  TextRect.left += menuinfo->nBorderWidth;

	TextRect.left += FM_LEFTBORDER;
	xi = TextRect.left + FM_SPACE1;
	yi = TextRect.top + FM_Y_SPACE/2;
	TextRect.bottom -= FM_Y_SPACE/2;

	xt = xi + FM_ICON_SIZE + FM_SPACE2;
	yt = yi;

	ExtTextOutW (lpdis->hDC, xt , yt, ETO_OPAQUE, &TextRect, pMyItem->szItemText, pMyItem->cchItemText, NULL);

	Shell_GetImageLists(0, &hImageList);
	ImageList_Draw(hImageList, pMyItem->iIconIndex, lpdis->hDC, xi, yi, ILD_NORMAL);

        TRACE("-- %s\n", wine_dbgstr_rect(&TextRect));

	SetTextColor(lpdis->hDC, clrPrevText);
	SetBkColor(lpdis->hDC, clrPrevBkgnd);

	return TRUE;
}

/*************************************************************************
 * FileMenu_InitMenuPopup			[SHELL32.109]
 *
 * NOTES
 *  The filemenu is an ownerdrawn menu. Call this function responding to
 *  WM_INITPOPUPMENU
 *
 */
BOOL WINAPI FileMenu_InitMenuPopup (HMENU hmenu)
{
	FM_InitMenuPopup(hmenu, NULL);
	return TRUE;
}

/*************************************************************************
 * FileMenu_HandleMenuChar			[SHELL32.108]
 */
LRESULT WINAPI FileMenu_HandleMenuChar(
	HMENU	hMenu,
	WPARAM	wParam)
{
	FIXME("%p 0x%08lx\n",hMenu,wParam);
	return 0;
}

/*************************************************************************
 * FileMenu_DeleteAllItems			[SHELL32.104]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI FileMenu_DeleteAllItems (HMENU hmenu)
{
	MENUITEMINFOW	mii;
	LPFMINFO	menudata;

	int i;

	TRACE("%p\n", hmenu);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOW));
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_SUBMENU|MIIM_DATA;

	for (i = 0; i < GetMenuItemCount( hmenu ); i++)
	{ GetMenuItemInfoW(hmenu, i, TRUE, &mii );

	  SHFree((LPFMINFO)mii.dwItemData);

	  if (mii.hSubMenu)
	    FileMenu_Destroy(mii.hSubMenu);
	}

	while (DeleteMenu (hmenu, 0, MF_BYPOSITION)){};

	menudata = FM_GetMenuInfo(hmenu);

	menudata->bInitialized = FALSE;

	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByCmd 			[SHELL32.117]
 *
 */
BOOL WINAPI FileMenu_DeleteItemByCmd (HMENU hMenu, UINT uID)
{
	MENUITEMINFOW mii;

	TRACE("%p 0x%08x\n", hMenu, uID);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOW));
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_SUBMENU;

	GetMenuItemInfoW(hMenu, uID, FALSE, &mii );
	if ( mii.hSubMenu )
	{
	  /* FIXME: Do what? */
	}

	DeleteMenu(hMenu, MF_BYCOMMAND, uID);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByIndex			[SHELL32.140]
 */
BOOL WINAPI FileMenu_DeleteItemByIndex ( HMENU hMenu, UINT uPos)
{
	MENUITEMINFOW mii;

	TRACE("%p 0x%08x\n", hMenu, uPos);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOW));
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_SUBMENU;

	GetMenuItemInfoW(hMenu, uPos, TRUE, &mii );
	if ( mii.hSubMenu )
	{
	  /* FIXME: Do what? */
	}

	DeleteMenu(hMenu, MF_BYPOSITION, uPos);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByFirstID			[SHELL32.141]
 */
BOOL WINAPI FileMenu_DeleteItemByFirstID(
	HMENU	hMenu,
	UINT	uID)
{
	TRACE("%p 0x%08x\n", hMenu, uID);
	return FALSE;
}

/*************************************************************************
 * FileMenu_DeleteSeparator			[SHELL32.142]
 */
BOOL WINAPI FileMenu_DeleteSeparator(HMENU hMenu)
{
	TRACE("%p\n", hMenu);
	return FALSE;
}

/*************************************************************************
 * FileMenu_EnableItemByCmd			[SHELL32.143]
 */
BOOL WINAPI FileMenu_EnableItemByCmd(
	HMENU	hMenu,
	UINT	uID,
	BOOL	bEnable)
{
	TRACE("%p 0x%08x 0x%08x\n", hMenu, uID,bEnable);
	return FALSE;
}

/*************************************************************************
 * FileMenu_GetItemExtent			[SHELL32.144]
 *
 * NOTES
 *  if the menu is too big, entries are getting cut away!!
 */
DWORD WINAPI FileMenu_GetItemExtent (HMENU hMenu, UINT uPos)
{	RECT rect;

	FIXME("%p 0x%08x\n", hMenu, uPos);

	if (GetMenuItemRect(0, hMenu, uPos, &rect))
        {
          FIXME("%s\n", wine_dbgstr_rect(&rect));
	  return ((rect.right-rect.left)<<16) + (rect.top-rect.bottom);
	}
	return 0x00100010; /*FIXME*/
}

/*************************************************************************
 * FileMenu_AbortInitMenu 			[SHELL32.120]
 *
 */
void WINAPI FileMenu_AbortInitMenu (void)
{	TRACE("\n");
	bAbortInit = TRUE;
}

/*************************************************************************
 * SHFind_InitMenuPopup				[SHELL32.149]
 *
 * Get the IContextMenu instance for the submenu of options displayed
 * for the Search entry in the Classic style Start menu.
 *
 * PARAMETERS
 *  hMenu		[in] handle of menu previously created
 *  hWndParent	[in] parent window
 *  w			[in] no pointer (0x209 over here) perhaps menu IDs ???
 *  x			[in] no pointer (0x226 over here)
 *
 * RETURNS
 *  LPXXXXX			 pointer to struct containing a func addr at offset 8
 *					 or NULL at failure.
 */
IContextMenu * WINAPI SHFind_InitMenuPopup (HMENU hMenu, HWND hWndParent, UINT w, UINT x)
{
	FIXME("hmenu=%p hwnd=%p 0x%08x 0x%08x stub\n",
		hMenu,hWndParent,w,x);
	return NULL; /* this is supposed to be a pointer */
}

/*************************************************************************
 * _SHIsMenuSeparator   (internal)
 */
static BOOL _SHIsMenuSeparator(HMENU hm, int i)
{
	MENUITEMINFOW mii;

	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_TYPE;
	mii.cch = 0;    /* WARNING: We MUST initialize it to 0*/
	if (!GetMenuItemInfoW(hm, i, TRUE, &mii))
	{
	  return(FALSE);
	}

	if (mii.fType & MFT_SEPARATOR)
	{
	  return(TRUE);
	}

	return(FALSE);
}

/*************************************************************************
 * Shell_MergeMenus				[SHELL32.67]
 */
UINT WINAPI Shell_MergeMenus (HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags)
{	int		nItem;
	HMENU		hmSubMenu;
	BOOL		bAlreadySeparated;
	MENUITEMINFOW	miiSrc;
	WCHAR		szName[256];
	UINT		uTemp, uIDMax = uIDAdjust;

	TRACE("hmenu1=%p hmenu2=%p 0x%04x 0x%04x 0x%04x  0x%04x\n",
		 hmDst, hmSrc, uInsert, uIDAdjust, uIDAdjustMax, uFlags);

	if (!hmDst || !hmSrc)
	  return uIDMax;

	nItem = GetMenuItemCount(hmDst);
        if (nItem == -1)
	  return uIDMax;

	if (uInsert >= (UINT)nItem)	/* insert position inside menu? */
	{
	  uInsert = (UINT)nItem;	/* append on the end */
	  bAlreadySeparated = TRUE;
	}
	else
	{
	  bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);
	}

	if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	{
	  /* Add a separator between the menus */
	  InsertMenuA(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	  bAlreadySeparated = TRUE;
	}


	/* Go through the menu items and clone them*/
	for (nItem = GetMenuItemCount(hmSrc) - 1; nItem >= 0; nItem--)
	{
	  miiSrc.cbSize = sizeof(MENUITEMINFOW);
	  miiSrc.fMask =  MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;

	  /* We need to reset this every time through the loop in case menus DON'T have IDs*/
	  miiSrc.fType = MFT_STRING;
	  miiSrc.dwTypeData = szName;
	  miiSrc.dwItemData = 0;
	  miiSrc.cch = sizeof(szName)/sizeof(WCHAR);

	  if (!GetMenuItemInfoW(hmSrc, nItem, TRUE, &miiSrc))
	  {
	    continue;
	  }

/*	  TRACE("found menu=0x%04x %s id=0x%04x mask=0x%08x smenu=0x%04x\n", hmSrc, debugstr_a(miiSrc.dwTypeData), miiSrc.wID, miiSrc.fMask,  miiSrc.hSubMenu);
*/
	  if (miiSrc.fType & MFT_SEPARATOR)
	  {
	    /* This is a separator; don't put two of them in a row */
	    if (bAlreadySeparated)
	      continue;

	    bAlreadySeparated = TRUE;
	  }
	  else if (miiSrc.hSubMenu)
	  {
	    if (uFlags & MM_SUBMENUSHAVEIDS)
	    {
	      miiSrc.wID += uIDAdjust;			/* add uIDAdjust to the ID */

	      if (miiSrc.wID > uIDAdjustMax)		/* skip IDs higher than uIDAdjustMax */
	        continue;

	      if (uIDMax <= miiSrc.wID)			/* remember the highest ID */
	        uIDMax = miiSrc.wID + 1;
	    }
	    else
	    {
	      miiSrc.fMask &= ~MIIM_ID;			/* Don't set IDs for submenus that didn't have them already */
	    }
	    hmSubMenu = miiSrc.hSubMenu;

	    miiSrc.hSubMenu = CreatePopupMenu();

	    if (!miiSrc.hSubMenu) return(uIDMax);

	    uTemp = Shell_MergeMenus(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust, uIDAdjustMax, uFlags & MM_SUBMENUSHAVEIDS);

	    if (uIDMax <= uTemp)
	      uIDMax = uTemp;

	    bAlreadySeparated = FALSE;
	  }
	  else						/* normal menu item */
	  {
	    miiSrc.wID += uIDAdjust;			/* add uIDAdjust to the ID */

	    if (miiSrc.wID > uIDAdjustMax)		/* skip IDs higher than uIDAdjustMax */
	      continue;

	    if (uIDMax <= miiSrc.wID)			/* remember the highest ID */
	      uIDMax = miiSrc.wID + 1;

	    bAlreadySeparated = FALSE;
	  }

/*	  TRACE("inserting menu=0x%04x %s id=0x%04x mask=0x%08x smenu=0x%04x\n", hmDst, debugstr_a(miiSrc.dwTypeData), miiSrc.wID, miiSrc.fMask, miiSrc.hSubMenu);
*/
	  if (!InsertMenuItemW(hmDst, uInsert, TRUE, &miiSrc))
	  {
	    return(uIDMax);
	  }
	}

	/* Ensure the correct number of separators at the beginning of the
	inserted menu items*/
	if (uInsert == 0)
	{
	  if (bAlreadySeparated)
	  {
	    DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	  }
	}
	else
	{
	  if (_SHIsMenuSeparator(hmDst, uInsert-1))
	  {
	    if (bAlreadySeparated)
	    {
	      DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	    }
	  }
	  else
	  {
	    if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	    {
	      /* Add a separator between the menus*/
	      InsertMenuW(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	    }
	  }
	}
	return(uIDMax);
}
