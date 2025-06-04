/*
 * PROJECT:     ReactOS Keyboard Layout Switcher
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IME menu handling
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "kbswitch.h"
#include "imemenu.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(internat);

PIMEMENUNODE g_pMenuList = NULL;
INT g_nNextMenuID = 0;

static BOOL MakeImeMenu(_In_ HMENU hMenu, _In_ const IMEMENUNODE *pMenu);

static VOID
AddImeMenuNode(_In_ PIMEMENUNODE pMenu)
{
    if (!g_pMenuList)
    {
        g_pMenuList = pMenu;
        return;
    }

    pMenu->m_pNext = g_pMenuList;
    g_pMenuList = pMenu;
}

static PIMEMENUNODE
AllocateImeMenu(_In_ DWORD itemCount)
{
    SIZE_T cbMenu = sizeof(IMEMENUNODE) + (itemCount - 1) * sizeof(IMEMENUITEM);
    PIMEMENUNODE pMenu = LocalAlloc(LPTR, cbMenu);
    if (!pMenu)
        return NULL;
    pMenu->m_nItems = itemCount;
    AddImeMenuNode(pMenu);
    return pMenu;
}

static VOID
GetImeMenuItem(
    _In_ HIMC hIMC,
    _Out_ PIMEMENUITEMINFO lpImeParentMenu,
    _In_ BOOL bRightMenu,
    _Out_ PIMEMENUITEM pItem)
{
    ZeroMemory(pItem, sizeof(IMEMENUITEM));
    pItem->m_Info = *lpImeParentMenu;

    if (lpImeParentMenu->fType & IMFT_SUBMENU)
        pItem->m_pSubMenu = CreateImeMenu(hIMC, lpImeParentMenu, bRightMenu);

    pItem->m_nRealID = pItem->m_Info.wID;
    pItem->m_Info.wID = ID_STARTIMEMENU + g_nNextMenuID++;
}

PIMEMENUNODE
CreateImeMenu(
    _In_ HIMC hIMC,
    _Inout_opt_ PIMEMENUITEMINFO lpImeParentMenu,
    _In_ BOOL bRightMenu)
{
    const DWORD dwFlags = (bRightMenu ? IGIMIF_RIGHTMENU : 0);
    const DWORD dwTypes = IGIMII_CMODE     |
                          IGIMII_SMODE     |
                          IGIMII_CONFIGURE |
                          IGIMII_TOOLS     |
                          IGIMII_HELP      |
                          IGIMII_OTHER;
    DWORD itemCount = ImmGetImeMenuItems(hIMC, dwFlags, dwTypes, lpImeParentMenu, NULL, 0);
    if (!itemCount)
        return NULL;

    PIMEMENUNODE pMenu = AllocateImeMenu(itemCount);
    if (!pMenu)
        return NULL;

    DWORD cbItems = sizeof(IMEMENUITEMINFO) * itemCount;
    PIMEMENUITEMINFO pImeMenuItems = LocalAlloc(LPTR, cbItems);
    if (!pImeMenuItems)
    {
        LocalFree(pMenu);
        return NULL;
    }

    itemCount = ImmGetImeMenuItems(hIMC, dwFlags, dwTypes, lpImeParentMenu, pImeMenuItems, cbItems);
    if (!itemCount)
    {
        LocalFree(pImeMenuItems);
        LocalFree(pMenu);
        return NULL;
    }

    PIMEMENUITEM pItems = pMenu->m_Items;
    for (DWORD iItem = 0; iItem < itemCount; ++iItem)
    {
        GetImeMenuItem(hIMC, &pImeMenuItems[iItem], bRightMenu, &pItems[iItem]);
    }

    LocalFree(pImeMenuItems);
    return pMenu;
}

static BOOL
FillImeMenuItem(_Out_ LPMENUITEMINFO pItemInfo, _In_ const IMEMENUITEM *pItem)
{
    ZeroMemory(pItemInfo, sizeof(MENUITEMINFO));
    pItemInfo->cbSize = sizeof(MENUITEMINFO);
    pItemInfo->fMask = MIIM_ID | MIIM_STATE | MIIM_DATA;
    pItemInfo->wID = pItem->m_Info.wID;
    pItemInfo->fState = pItem->m_Info.fState;
    pItemInfo->dwItemData = pItem->m_Info.dwItemData;

    if (pItem->m_Info.fType)
    {
        pItemInfo->fMask |= MIIM_FTYPE;
        pItemInfo->fType = 0;
        if (pItem->m_Info.fType & IMFT_RADIOCHECK)
            pItemInfo->fType |= MFT_RADIOCHECK;
        if (pItem->m_Info.fType & IMFT_SEPARATOR)
            pItemInfo->fType |= MFT_SEPARATOR;
    }

    if (pItem->m_Info.fType & IMFT_SUBMENU)
    {
        pItemInfo->fMask |= MIIM_SUBMENU;
        pItemInfo->hSubMenu = CreatePopupMenu();
        if (!MakeImeMenu(pItemInfo->hSubMenu, pItem->m_pSubMenu))
        {
            DestroyMenu(pItemInfo->hSubMenu);
            pItemInfo->hSubMenu = NULL;
            return FALSE;
        }
    }

    if (pItem->m_Info.hbmpChecked && pItem->m_Info.hbmpUnchecked)
    {
        pItemInfo->fMask |= MIIM_CHECKMARKS;
        pItemInfo->hbmpChecked = pItem->m_Info.hbmpChecked;
        pItemInfo->hbmpUnchecked = pItem->m_Info.hbmpUnchecked;
    }

    if (pItem->m_Info.hbmpItem)
    {
        pItemInfo->fMask |= MIIM_BITMAP;
        pItemInfo->hbmpItem = pItem->m_Info.hbmpItem;
    }

    PCTSTR szString = pItem->m_Info.szString;
    if (szString && szString[0])
    {
        pItemInfo->fMask |= MIIM_STRING;
        pItemInfo->dwTypeData = (PTSTR)szString;
        pItemInfo->cch = lstrlen(szString);
    }

    return TRUE;
}

static BOOL
MakeImeMenu(_In_ HMENU hMenu, _In_ const IMEMENUNODE *pMenu)
{
    if (!pMenu || !pMenu->m_nItems)
        return FALSE;

    for (INT iItem = 0; iItem < pMenu->m_nItems; ++iItem)
    {
        MENUITEMINFO mi = { sizeof(mi) };
        if (!FillImeMenuItem(&mi, &pMenu->m_Items[iItem]))
        {
            ERR("FillImeMenuItem failed\n");
            return FALSE;
        }
        if (!InsertMenuItem(hMenu, iItem, TRUE, &mi))
        {
            ERR("InsertMenuItem failed\n");
            return FALSE;
        }
    }

    return TRUE;
}

HMENU MenuFromImeMenu(_In_ const IMEMENUNODE *pMenu)
{
    HMENU hMenu = CreatePopupMenu();
    if (!pMenu)
        return hMenu;
    if (!MakeImeMenu(hMenu, pMenu))
    {
        DestroyMenu(hMenu);
        return NULL;
    }
    return hMenu;
}

INT
GetRealImeMenuID(_In_ const IMEMENUNODE *pMenu, _In_ INT nFakeID)
{
    if (!pMenu || !pMenu->m_nItems || nFakeID < ID_STARTIMEMENU)
        return 0;

    for (INT iItem = 0; iItem < pMenu->m_nItems; ++iItem)
    {
        const IMEMENUITEM *pItem = &pMenu->m_Items[iItem];
        if (pItem->m_Info.wID == nFakeID)
            return pItem->m_nRealID;

        if (pItem->m_pSubMenu)
        {
            INT nRealID = GetRealImeMenuID(pItem->m_pSubMenu, nFakeID);
            if (nRealID)
                return nRealID;
        }
    }

    return 0;
}

static BOOL
FreeMenuNode(_In_ PIMEMENUNODE pMenuNode)
{
    if (!pMenuNode)
        return FALSE;

    for (INT iItem = 0; iItem < pMenuNode->m_nItems; ++iItem)
    {
        PIMEMENUITEM pItem = &pMenuNode->m_Items[iItem];
        if (pItem->m_Info.hbmpChecked)
            DeleteObject(pItem->m_Info.hbmpChecked);
        if (pItem->m_Info.hbmpUnchecked)
            DeleteObject(pItem->m_Info.hbmpUnchecked);
        if (pItem->m_Info.hbmpItem)
            DeleteObject(pItem->m_Info.hbmpItem);
    }

    LocalFree(pMenuNode);
    return TRUE;
}

VOID
CleanupImeMenus(VOID)
{
    if (!g_pMenuList)
        return;

    PIMEMENUNODE pNext;
    for (PIMEMENUNODE pNode = g_pMenuList; pNode; pNode = pNext)
    {
        pNext = pNode->m_pNext;
        FreeMenuNode(pNode);
    }

    g_pMenuList = NULL;
    g_nNextMenuID = 0;
}
