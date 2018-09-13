/****************************** Module Header ******************************\
* Module Name: menuc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains
*
* History:
* 01-11-93  DavidPe     Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

DWORD CheckMenuItem(
    HMENU hMenu,
    UINT uIDCheckItem,
    UINT uCheck)
{
    PMENU pMenu;
    PITEM pItem;

    pMenu = VALIDATEHMENU(hMenu);
    if (pMenu == NULL) {
        return (DWORD)-1;
    }

    /*
     * Get a pointer the the menu item
     */
    if ((pItem = MNLookUpItem(pMenu, uIDCheckItem, (BOOL) (uCheck & MF_BYPOSITION), NULL)) == NULL)
        return (DWORD)-1;

    /*
     * If the item is already in the state we're
     * trying to set, just return.
     */
    if ((pItem->fState & MFS_CHECKED) == (uCheck & MFS_CHECKED)) {
        return pItem->fState & MF_CHECKED;
    }

    return NtUserCheckMenuItem(hMenu, uIDCheckItem, uCheck);
}

UINT GetMenuDefaultItem(HMENU hMenu, UINT fByPosition, UINT uFlags) {
    PMENU pMenu;

    pMenu = VALIDATEHMENU(hMenu);
    if (pMenu == NULL) {
        return (DWORD)-1;
    }

    return _GetMenuDefaultItem(pMenu, (BOOL)fByPosition, uFlags);
}

/***************************************************************************\
* SetMenuItemInfoStruct
*
* History:
*  07-22-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/
void SetMenuItemInfoStruct(HMENU hMenu, UINT wFlags, UINT_PTR wIDNew, LPWSTR pwszNew, LPMENUITEMINFO pmii)
{
    PMENU pMenu;
    PITEM pItem;

    UserAssert(sizeof(MENUITEMINFOW) == sizeof(MENUITEMINFOA));

    RtlZeroMemory(pmii, sizeof(*pmii));

    pmii->fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;

    /*
     * For compatibility, setting the bitmap drops the string and
     *  viceversa; new apps that want to have sting and bitmaps
     *  must use the MENUITEMINFO APIs
     */
    if (wFlags & MFT_BITMAP) {
        pmii->fMask |= MIIM_BITMAP | MIIM_STRING;
        pmii->hbmpItem = (HBITMAP)pwszNew;
        pmii->dwTypeData  = 0;
    } else if (!(wFlags & MFT_NONSTRING)) {
        pmii->fMask |= MIIM_BITMAP | MIIM_STRING;
        pmii->dwTypeData  = pwszNew;
        pmii->hbmpItem = NULL;
    }

    if (wFlags & MF_POPUP) {
        pmii->fMask |= MIIM_SUBMENU;
        pmii->hSubMenu = (HMENU)wIDNew;
    }

    if (wFlags & MF_OWNERDRAW) {
        pmii->fMask |= MIIM_DATA;
        pmii->dwItemData = (ULONG_PTR) pwszNew;
    }

    pmii->fState = wFlags & MFS_OLDAPI_MASK;
    pmii->fType  = wFlags & MFT_OLDAPI_MASK;
    pMenu = VALIDATEHMENU(hMenu);
    if (pMenu && pMenu->cItems) {
        pItem = &((PITEM)REBASEALWAYS(pMenu, rgItems))[0];
        if (pItem && TestMFT(pItem, MFT_RIGHTORDER)) {
            pmii->fType |= MFT_RIGHTORDER;
        }
    }
    pmii->wID    = (UINT)wIDNew;
}
/***************************************************************************\
* SetMenuItemInfo
*
* History:
*  07-22-96 GerardoB - Added header
\***************************************************************************/
BOOL SetMenuInfo(HMENU hMenu, LPCMENUINFO lpmi)
{
    if (!ValidateMENUINFO(lpmi, MENUAPI_SET)) {
        return FALSE;
    }

    return NtUserThunkedMenuInfo(hMenu, (LPCMENUINFO)lpmi);
}
/***************************************************************************\
* ChangeMenu
*
* Stub routine for compatibility with version < 3.0
*
* History:
\***************************************************************************/

BOOL ChangeMenuW(
    HMENU hMenu,
    UINT cmd,
    LPCWSTR lpNewItem,
    UINT IdItem,
    UINT flags)
{
    /*
     * These next two statements take care of sleazyness needed for
     * compatability with old changemenu.
     */
    if ((flags & MF_SEPARATOR) && cmd == MFMWFP_OFFMENU && !(flags & MF_CHANGE))
        flags |= MF_APPEND;

    if (lpNewItem == NULL)
        flags |= MF_SEPARATOR;



    /*
     * MUST be MF_BYPOSITION for Win2.x compatability.
     */
    if (flags & MF_REMOVE)
        return(NtUserRemoveMenu(hMenu, cmd,
                (DWORD)((flags & ~MF_REMOVE) | MF_BYPOSITION)));

    if (flags & MF_DELETE)
        return(NtUserDeleteMenu(hMenu, cmd, (DWORD)(flags & ~MF_DELETE)));

    if (flags & MF_CHANGE)
        return(ModifyMenuW(hMenu, cmd, (DWORD)((flags & ~MF_CHANGE) &
                (0x07F | MF_HELP | MF_BYPOSITION | MF_BYCOMMAND |
                MF_SEPARATOR)), IdItem, lpNewItem));

    if (flags & MF_APPEND)
        return(AppendMenuW(hMenu, (UINT)(flags & ~MF_APPEND),
            IdItem, lpNewItem));

    /*
     * Default is insert
     */
    return(InsertMenuW(hMenu, cmd, (DWORD)(flags & ~MF_INSERT),
            IdItem, lpNewItem));
}

BOOL ChangeMenuA(
    HMENU hMenu,
    UINT cmd,
    LPCSTR lpNewItem,
    UINT IdItem,
    UINT flags)
{
    /*
     * These next two statements take care of sleazyness needed for
     * compatability with old changemenu.
     */
    if ((flags & MF_SEPARATOR) && cmd == MFMWFP_OFFMENU && !(flags & MF_CHANGE))
        flags |= MF_APPEND;

    if (lpNewItem == NULL)
        flags |= MF_SEPARATOR;



    /*
     * MUST be MF_BYPOSITION for Win2.x compatability.
     */
    if (flags & MF_REMOVE)
        return(NtUserRemoveMenu(hMenu, cmd,
                (DWORD)((flags & ~MF_REMOVE) | MF_BYPOSITION)));

    if (flags & MF_DELETE)
        return(NtUserDeleteMenu(hMenu, cmd, (DWORD)(flags & ~MF_DELETE)));

    if (flags & MF_CHANGE)
        return(ModifyMenuA(hMenu, cmd, (DWORD)((flags & ~MF_CHANGE) &
                (0x07F | MF_HELP | MF_BYPOSITION | MF_BYCOMMAND |
                MF_SEPARATOR)), IdItem, lpNewItem));

    if (flags & MF_APPEND)
        return(AppendMenuA(hMenu, (UINT)(flags & ~MF_APPEND),
            IdItem, lpNewItem));

    /*
     * Default is insert
     */
    return(InsertMenuA(hMenu, cmd, (DWORD)(flags & ~MF_INSERT),
            IdItem, lpNewItem));
}

LONG GetMenuCheckMarkDimensions()
{
    return((DWORD)MAKELONG(SYSMET(CXMENUCHECK), SYSMET(CYMENUCHECK)));
}

/***************************************************************************\
* GetMenuContextHelpId
*
* Return the help id of a menu.
*
\***************************************************************************/

WINUSERAPI DWORD WINAPI GetMenuContextHelpId(
    HMENU hMenu)
{
    PMENU pMenu;

    pMenu = VALIDATEHMENU(hMenu);

    if (pMenu == NULL)
        return 0;

    return pMenu->dwContextHelpId;
}

BOOL TrackPopupMenu(
    HMENU hMenu,
    UINT fuFlags,
    int x,
    int y,
    int nReserved,
    HWND hwnd,
    CONST RECT *prcRect)
{
    UNREFERENCED_PARAMETER(nReserved);
    UNREFERENCED_PARAMETER(prcRect);

    return NtUserTrackPopupMenuEx(hMenu, fuFlags, x, y, hwnd, NULL);
}

/***************************************************************************\
* GetSysMenuHandle
*
* Returns a handle to the system menu of the given window. NULL if
* the window doesn't have a system menu.
*
* History:
\***************************************************************************/

PMENU xxxGetSysMenuHandle(
    PWND pwnd)
{
    PMENU pMenu;

    if (TestWF(pwnd, WFSYSMENU)) {
        pMenu = pwnd->spmenuSys;

        /*
         * If the window doesn't have a System Menu, use the default one.
         */
        if (pMenu == NULL) {

            /*
             * Change owner so this app can access this menu.
             */
            pMenu = (PMENU)NtUserCallHwndLock(HWq(pwnd), SFI_XXXGETSYSMENUHANDLE);
        }
    } else {
        pMenu = NULL;
    }

    return pMenu;
}

BOOL WINAPI SetMenuItemBitmaps
(
    HMENU hMenu,
    UINT nPosition,
    UINT uFlags,
    HBITMAP hbmpUnchecked,
    HBITMAP hbmpChecked
)
{
    MENUITEMINFO    mii;
    mii.cbSize          = sizeof(MENUITEMINFO);
    mii.fMask           = MIIM_CHECKMARKS;
    mii.hbmpChecked     = hbmpChecked;
    mii.hbmpUnchecked   = hbmpUnchecked;

    return(SetMenuItemInfo(hMenu, nPosition, (BOOL) (uFlags & MF_BYPOSITION), &mii));
}

int WINAPI DrawMenuBarTemp(
    HWND hwnd,
    HDC hdc,
    LPCRECT lprc,
    HMENU hMenu,
    HFONT hFont)
{
    HDC hdcr;

    if (IsMetaFile(hdc))
        return -1;

    hdcr = GdiConvertAndCheckDC(hdc);
    if (hdcr == (HDC)0)
        return -1;

    if (!hMenu)
        return -1;

    return NtUserDrawMenuBarTemp(
            hwnd,
            hdc,
            lprc,
            hMenu,
            hFont);
}

/***************************************************************************\
*
*  CheckMenuRadioItem() -
*
*  Checks one menu item in a range, unchecking the others.  This can be
*  done either MF_BYCOMMAND or MF_BYPOSITION.  It works similarly to
*  CheckRadioButton().
*
*  The return value is TRUE if the given item was checked, FALSE if not.
*
* History
* 04/04/97 GerardoB     Moved to the client side
\***************************************************************************/

BOOL CheckMenuRadioItem(HMENU hMenu, UINT wIDFirst, UINT wIDLast,
        UINT wIDCheck, UINT flags)
{
    BOOL    fByPosition = (BOOL) (flags & MF_BYPOSITION);
    PMENU   pMenu, pMenuItemIsOn;
    PITEM   pItem;
    UINT    wIDCur;
    BOOL    fChecked = FALSE;
    BOOL    fFirst  = TRUE;
    MENUITEMINFO mii;

    pMenu = VALIDATEHMENU(hMenu);
    if (pMenu == NULL) {
        return FALSE;
    }

    mii.cbSize = sizeof(mii);
    /*
     * Make sure we won't loop for ever
     */
    wIDLast = min(wIDLast, (UINT)0xFFFFFFFE);

    for (wIDCur = wIDFirst; wIDCur <= wIDLast; wIDCur++) {
        pItem = MNLookUpItem(pMenu, wIDCur, fByPosition, &pMenuItemIsOn);
        /*
         * Continue searching if it didn't find the item or it's a separator
         */
        if ((pItem == NULL) || TestMFT(pItem, MFT_SEPARATOR)) {
            continue;
        }
        /*
         * If this is the first one, rememeber what menu it's on because
         *  all items are supposed to be in the same menu.
         */
        if (fFirst) {
            pMenu = pMenuItemIsOn;
            hMenu = PtoHq(pMenu);
            fFirst = FALSE;
        }
        /*
         * If this item is on a different menu, don't touch it
         */
        if (pMenu != pMenuItemIsOn) {
            continue;
        }
        /*
         * Set the new check state. Avoid the trip to the kernel if possible
         */
        if (wIDCur == wIDCheck) {
            /*
             * Check it.
             */
            if (!TestMFT(pItem, MFT_RADIOCHECK) || !TestMFS(pItem, MFS_CHECKED)) {
                mii.fMask = MIIM_FTYPE | MIIM_STATE;
                mii.fType = (pItem->fType & MFT_MASK) | MFT_RADIOCHECK;
                mii.fState = (pItem->fState & MFS_MASK) | MFS_CHECKED;
                NtUserThunkedMenuItemInfo(hMenu, wIDCheck, fByPosition, FALSE, &mii, NULL);
            }
            fChecked = TRUE;
        } else {
            /*
             * Uncheck it
             * NOTE:  don't remove MFT_RADIOCHECK type
             */
            if (TestMFS(pItem, MFS_CHECKED)) {
                mii.fMask = MIIM_STATE;
                mii.fState = (pItem->fState & MFS_MASK) & ~MFS_CHECKED;
                NtUserThunkedMenuItemInfo(hMenu, wIDCur, fByPosition, FALSE, &mii, NULL);
            }
        }
    } /* for */

    if (fFirst) {
        /*
         * No item was ever found.
         */
        RIPERR0(ERROR_MENU_ITEM_NOT_FOUND, RIP_VERBOSE, "CheckMenuRadioItem, no items found\n");

    }
    return(fChecked);
}
