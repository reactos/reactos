/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    imemenu.c

Abstract:

    This module implements the dll for handling the shell hooks for the
    multilingual language indicator.

Revision History:

--*/



//
//  Include Files.
//

#include "indicdll.h"
#include <immp.h>




//
//  Constant Declarations.
//

//
//  Max and Min number of MENUITEM in the shared heap.
//
#define MAX_IMITEMS 500
#define MIN_IMITEMS 200




//
//  Typedef Definitions.
//

typedef struct s_MENULIST
{
    struct s_MENULIST *pPrev;
    struct s_MENULIST *pNext;
    DWORD dwNum;
} MENULIST, *PMENULIST;

typedef struct s_MENUITEM
{
    IMEMENUITEMINFO imii;
    int nMenuID;
    PMENULIST pmlSubMenu;
} MENUITEM, *PMENUITEM;




//
//  On NT, unlike Win95/98, the heap cannot be used to share data among
//  processes.  Instead let IMM32 handle the inter-process GetImeMenu.
//
HANDLE g_hHeap;                 // shared heap
PMENULIST g_pMenuHdr;           // header of pMenuList
int g_nMenuList;                // number of pMenuList items
int g_nMenuCnt;                 // sequence number for IME menu items




//
//  Macro Definitions.
//

#define GETMENUITEM(pMenu) ((PMENUITEM)((LPBYTE)pMenu + sizeof(MENULIST)))




//
//  Function Prototypes.
//

PMENULIST
IndicDll_CreateIMEMenu(
    HWND hWnd,
    HIMC hIMC,
    LPIMEMENUITEMINFO lpImeParentMenu,
    BOOL fRight);

BOOL
IndicDll_BuildIMEMenuItems(
    HMENU hMenu,
    PMENULIST pMenu,
    BOOL fRight);





////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_AddMenuList
//
//  Inserts the new menu after the last one in the list.
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_AddMenuList(
    PMENULIST pMenu)
{
    PMENULIST pMenuPrev, pMenuNext;

    //
    //  Create the header if it hasn't already been created.
    //
    if (!g_pMenuHdr)
    {
        g_pMenuHdr = (PMENULIST)HeapAlloc( g_hHeap,
                                           HEAP_ZERO_MEMORY,
                                           sizeof(MENULIST) );
        if (!g_pMenuHdr)
        {
            return (FALSE);
        }
        g_pMenuHdr->pPrev = g_pMenuHdr;
        g_pMenuHdr->pNext = g_pMenuHdr;
    }

    //
    //  Add the new item to the end of the list.
    //
    pMenuPrev = g_pMenuHdr->pPrev;
    pMenuNext = pMenuPrev->pNext;
    pMenu->pNext = pMenuPrev->pNext;
    pMenu->pPrev = pMenuNext->pPrev;
    pMenuPrev->pNext = pMenu;
    pMenuNext->pPrev = pMenu;

    //
    //  Increment the count of items.
    //
    g_nMenuList++;

    //
    //  See if we're at the max number of items.
    //
    if (g_nMenuList > MAX_IMITEMS)
    {
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_DeleteMenuList
//
//  Deletes the given menu from the list.
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_DeleteMenuList(
    PMENULIST pMenu)
{
    PMENULIST pMenuPrev, pMenuNext;

    //
    //  Make sure we're not deleting the header.
    //
    if (pMenu == g_pMenuHdr)
    {
        return;
    }

    //
    //  NT only:
    //  Since IMM32 has created a copy of the menu bitmaps, Internat is
    //  responsible for deleting the GDI objects here.
    //
    if (pMenu->dwNum)
    {
        DWORD dwI;
        PMENUITEM pMenuItem;

        pMenuItem = GETMENUITEM(pMenu);

        for (dwI = 0; dwI < pMenu->dwNum; dwI++)
        {
            if (pMenuItem[dwI].imii.hbmpChecked)
            {
                DeleteObject(pMenuItem[dwI].imii.hbmpChecked);
            }
            if (pMenuItem[dwI].imii.hbmpUnchecked)
            {
                DeleteObject(pMenuItem[dwI].imii.hbmpUnchecked);
            }
            if (pMenuItem[dwI].imii.hbmpItem)
            {
                DeleteObject(pMenuItem[dwI].imii.hbmpItem);
            }
        }
    }

    //
    //  Delete the item from the list.
    //
    pMenuPrev = pMenu->pPrev;
    pMenuNext = pMenu->pNext;
    pMenuPrev->pNext = pMenu->pNext;
    pMenuNext->pPrev = pMenu->pPrev;

    //
    //  Decrement the count of items.
    //
    g_nMenuList--;

    if (g_nMenuList < 0)
    {
        g_nMenuList = 0;
    }

    //
    //  Free the item from the heap.
    //
    HeapFree(g_hHeap, 0, pMenu);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_DeleteAllMenuList
//
//  Deletes all menu list items and the frees them from the heap.
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_DeleteAllMenuList()
{
    PMENULIST pMenu, pMenuNext;

    //
    //  Make sure the header exists and that there is something in the list.
    //
    if ((g_pMenuHdr) &&
        (pMenu = g_pMenuHdr->pNext) &&
        (pMenu != g_pMenuHdr))
    {
        //
        //  Delete each item from the list.
        //
        while (pMenu != g_pMenuHdr)
        {
            pMenuNext = pMenu->pNext;
            IndicDll_DeleteMenuList(pMenu);
            pMenu = pMenuNext;
        }

        //
        //  Make sure the number of items in the list is now set to zero.
        //  If not, set it to zero.    (this shouldn't happen)
        //
        if (g_nMenuList > 0)
        {
            g_nMenuList = 0;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_AllocMenuList
//
////////////////////////////////////////////////////////////////////////////

PMENULIST IndicDll_AllocMenuList(
    DWORD dwNum)
{
    PMENULIST pMenu;

    pMenu = (PMENULIST)HeapAlloc( g_hHeap,
                                  HEAP_ZERO_MEMORY,
                                  sizeof(MENULIST) + sizeof(MENUITEM) * dwNum );
    if (pMenu)
    {
        IndicDll_AddMenuList(pMenu);
    }

    pMenu->dwNum = dwNum;

    return (pMenu);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_SetMenuItem
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_SetMenuItem(
    HWND hWnd,
    HIMC hIMC,
    LPIMEMENUITEMINFO lpIme,
    BOOL fRight,
    PMENUITEM pMenuItem)
{
    FillMemory((PVOID)pMenuItem, sizeof(MENUITEM), 0);

    pMenuItem->imii = *lpIme;

    if (lpIme->fType & IMFT_SUBMENU)
    {
        //
        //  If lpIme has a SubMenu, we need to create another MENULIST.
        //
        pMenuItem->pmlSubMenu = IndicDll_CreateIMEMenu( hWnd,
                                                        hIMC,
                                                        lpIme,
                                                        fRight );
    }

    pMenuItem->nMenuID = IDM_IME_MENUSTART + g_nMenuCnt;
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_CreateIMEMenu
//
////////////////////////////////////////////////////////////////////////////

PMENULIST IndicDll_CreateIMEMenu(
    HWND hWnd,
    HIMC hIMC,
    LPIMEMENUITEMINFO lpImeParentMenu,
    BOOL fRight)
{
    DWORD dwSize, dwNum, dwI;
    LPIMEMENUITEMINFO lpImeMenu;
    PMENULIST pMenu;
    PMENUITEM pMenuItem;

    dwNum = ImmGetImeMenuItems( hIMC,
                                fRight ? IGIMIF_RIGHTMENU : 0,
                                IGIMII_CMODE |
                                  IGIMII_SMODE |
                                  IGIMII_CONFIGURE |
                                  IGIMII_TOOLS |
                                  IGIMII_HELP |
                                  IGIMII_OTHER,
                                lpImeParentMenu,
                                NULL,
                                0 );
    if (!dwNum)
    {
        return (0);
    }

    pMenu = IndicDll_AllocMenuList(dwNum);
    if (!pMenu)
    {
        return (0);
    }

    pMenuItem = GETMENUITEM(pMenu);

    dwSize = dwNum * sizeof(IMEMENUITEMINFO);

    lpImeMenu = (LPIMEMENUITEMINFO)GlobalAlloc(GPTR, dwSize);
    if (!lpImeMenu)
    {
        return (0);
    }

    dwNum = ImmGetImeMenuItems( hIMC,
                                fRight ? IGIMIF_RIGHTMENU : 0,
                                IGIMII_CMODE |
                                  IGIMII_SMODE |
                                  IGIMII_CONFIGURE |
                                  IGIMII_TOOLS |
                                  IGIMII_HELP |
                                  IGIMII_OTHER,
                                lpImeParentMenu,
                                lpImeMenu,
                                dwSize );

    //
    //  Set up this MENULIST.
    //
    for (dwI = 0; dwI < dwNum; dwI++)
    {
        IndicDll_SetMenuItem( hWnd,
                              hIMC,
                              lpImeMenu + dwI,
                              fRight,
                              pMenuItem + dwI );
        g_nMenuCnt++;
    }

    GlobalFree((HANDLE)lpImeMenu);

    return (pMenu);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetIMEMenu
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_GetIMEMenu(
    HWND hWnd,
    BOOL fRight)
{
    HIMC hIMC;
    HWND hwndDefIme;

    //
    //  Create shared heap space.
    //
    if (!g_hHeap)
    {
        g_hHeap = HeapCreate( 0x04000000,
                              MIN_IMITEMS * sizeof(MENUITEM),
                              MAX_IMITEMS * sizeof(MENUITEM) );
    }

    if (!g_hHeap)
    {
        return (FALSE);
    }

    //
    //  Since we're in a different process, hIMC cannot be retrieved
    //  directly from hWnd.
    //
    hwndDefIme = ImmGetDefaultIMEWnd(hWnd);
    if (!IsWindow(hwndDefIme))
    {
        return (FALSE);
    }

    hIMC = (HIMC)SendMessage( hwndDefIme,
                              WM_IME_SYSTEM,
                              IMS_GETCONTEXT,
                              (LPARAM)hWnd );
    if (hIMC != NULL)
    {
        //
        //  Init sequence number.
        //
        g_nMenuCnt = 0;

        IndicDll_CreateIMEMenu(hWnd, hIMC, NULL, fRight);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_FillMenuItemInfo
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_FillMenuItemInfo(
    LPMENUITEMINFO lpmii,
    PMENUITEM pMenuItem,
    BOOL fRight)
{
    FillMemory((PVOID)lpmii, sizeof(MENUITEMINFO), 0);

    lpmii->cbSize = sizeof(MENUITEMINFO);
    lpmii->fMask = 0;

    if (pMenuItem->imii.fType)
    {
        lpmii->fMask |= MIIM_FTYPE;
        lpmii->fType = 0;

        if (pMenuItem->imii.fType & IMFT_RADIOCHECK)
        {
            lpmii->fType |= MFT_RADIOCHECK;
        }

        if (pMenuItem->imii.fType & IMFT_SEPARATOR)
        {
            lpmii->fType |= MFT_SEPARATOR;
        }
    }

    lpmii->fMask |= MIIM_ID;
    lpmii->wID = pMenuItem->nMenuID;

    if (pMenuItem->imii.fType & IMFT_SUBMENU)
    {
        //
        //  If lpIme has a SubMenu, we need to create another Popup Menu.
        //
        lpmii->fMask |= MIIM_SUBMENU;
        lpmii->hSubMenu = CreatePopupMenu();
        IndicDll_BuildIMEMenuItems( lpmii->hSubMenu,
                                    pMenuItem->pmlSubMenu,
                                    fRight );
    }

    lpmii->fMask |= MIIM_STATE;
    lpmii->fState = pMenuItem->imii.fState;

    if ((pMenuItem->imii.hbmpChecked) && (pMenuItem->imii.hbmpUnchecked))
    {
       lpmii->fMask |= MIIM_CHECKMARKS;
       lpmii->hbmpChecked = pMenuItem->imii.hbmpChecked;
       lpmii->hbmpUnchecked = pMenuItem->imii.hbmpUnchecked;
    }

    lpmii->fMask |= MIIM_DATA;
    lpmii->dwItemData = pMenuItem->imii.dwItemData;

    if (pMenuItem->imii.hbmpItem)
    {
       lpmii->fMask |= MIIM_BITMAP;
       lpmii->hbmpItem = pMenuItem->imii.hbmpItem;
    }

    if (lstrlen(pMenuItem->imii.szString))
    {
        lpmii->fMask |= MIIM_STRING;
        lpmii->dwTypeData = pMenuItem->imii.szString;
        lpmii->cch = lstrlen(pMenuItem->imii.szString);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//    IndicDll_GetDefaultImeMenuItem
//
////////////////////////////////////////////////////////////////////////////

UINT IndicDll_GetDefaultImeMenuItem(void)
{
    PMENULIST pMenu;
    DWORD dwI;
    MENUITEMINFO mii;
    PMENUITEM pMenuItem;


    if (!g_pMenuHdr)
        return 0;

    pMenu = g_pMenuHdr->pNext;

    if (pMenu == g_pMenuHdr)
        return 0;

    if (!pMenu->dwNum)
        return 0;

    pMenuItem = GETMENUITEM(pMenu);

    for (dwI = 0 ; dwI < pMenu->dwNum; dwI++)
    {
        if (pMenuItem->imii.fState & IMFS_DEFAULT)
            return pMenuItem->imii.wID;

        pMenuItem++;
    }
    return 0;

}

////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_BuildIMEMenuItems
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_BuildIMEMenuItems(
    HMENU hMenu,
    PMENULIST pMenu,
    BOOL fRight)
{
    DWORD dwI;
    MENUITEMINFO mii;
    PMENUITEM pMenuItem;

    if (!pMenu->dwNum)
    {
        return (FALSE);
    }

    pMenuItem = GETMENUITEM(pMenu);

    for (dwI = 0; dwI < pMenu->dwNum; dwI++)
    {
        IndicDll_FillMenuItemInfo(&mii, pMenuItem + dwI, fRight);
        VERIFY(InsertMenuItem(hMenu, dwI, TRUE, &mii));
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_BuildIMEMenu
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_BuildIMEMenu(
    HMENU hMenu,
    BOOL fRight)
{
    PMENULIST pMenu;

    if (g_pMenuHdr == NULL) {
        return FALSE;
    }

    pMenu = g_pMenuHdr->pNext;

    if (pMenu == g_pMenuHdr) {
        return FALSE;
    }

    return IndicDll_BuildIMEMenuItems(hMenu, pMenu, fRight);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetIMEMenuItemID
//
////////////////////////////////////////////////////////////////////////////

UINT IndicDll_GetIMEMenuItemID(
    int nMenuID)
{
    DWORD dwI;
    PMENULIST pMenu, pMenuNext;
    PMENUITEM pMenuItem;
    UINT uRet = 0;

    if ((g_pMenuHdr) &&
        (pMenu = g_pMenuHdr->pNext) &&
        (pMenu != g_pMenuHdr))
    {
        while (pMenu != g_pMenuHdr)
        {
            pMenuNext = pMenu->pNext;
            pMenuItem = GETMENUITEM(pMenu);

            for (dwI = 0; dwI < pMenu->dwNum; dwI++)
            {
                if (pMenuItem->nMenuID == nMenuID)
                {
                    uRet = pMenuItem->imii.wID;
                    return (uRet);
                }
                pMenuItem++;
            }
            pMenu = pMenuNext;
        }
    }

    return (uRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_SearchMenuItemData
//
////////////////////////////////////////////////////////////////////////////

DWORD IndicDll_SearchMenuItemData(
    UINT uID)
{
    DWORD dwI;
    PMENULIST pMenu, pMenuNext;
    PMENUITEM pMenuItem;
    DWORD dwRet = 0;

    if ((g_pMenuHdr) &&
        (pMenu = g_pMenuHdr->pNext) &&
        (pMenu != g_pMenuHdr))
    {
        while (pMenu != g_pMenuHdr)
        {
            pMenuNext = pMenu->pNext;
            pMenuItem = GETMENUITEM(pMenu);

            for (dwI = 0; dwI < pMenu->dwNum; dwI ++)
            {
                if (pMenuItem->imii.wID == uID)
                {
                    dwRet = pMenuItem->imii.dwItemData;
                    return (dwRet);
                }
                pMenuItem++;
            }
            pMenu = pMenuNext;
        }
    }

    ASSERT(FALSE);
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetIMEMenuItemData
//
//  Called from user32 of target application.
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_GetIMEMenuItemData(
    UINT* puID,
    DWORD* pdwMenuItemData)
{
    ASSERT(puID);
    ASSERT(pdwMenuItemData);
    ASSERT(lpvSHDataHead);

    *puID = lpvSHDataHead->uMenuID;
    *pdwMenuItemData = lpvSHDataHead->dwMenuItemData;
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_SetIMEMenuItemData
//
//  Called from user32 of target application.
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_SetIMEMenuItemData(
    UINT uID)
{
    ASSERT(lpvSHDataHead);

    lpvSHDataHead->uMenuID = uID;
    lpvSHDataHead->dwMenuItemData = IndicDll_SearchMenuItemData(uID);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_DestroyIMEMenu
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_DestroyIMEMenu()
{
    IndicDll_DeleteAllMenuList();
}
