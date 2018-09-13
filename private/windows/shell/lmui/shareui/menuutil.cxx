//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menuutil.cxx
//
//  Contents:   Context menu utilities, stolen from the shell. This is
//              basically CDefFolderMenu_MergeMenu and its support.
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "menuutil.hxx"

// Cannonical command names stolen from the shell
TCHAR const c_szDelete[]     = TEXT("delete");
TCHAR const c_szCut[]        = TEXT("cut");
TCHAR const c_szCopy[]       = TEXT("copy");
TCHAR const c_szLink[]       = TEXT("link");
TCHAR const c_szProperties[] = TEXT("properties");
TCHAR const c_szPaste[]      = TEXT("paste");
TCHAR const c_szPasteLink[]  = TEXT("pastelink");
TCHAR const c_szRename[]     = TEXT("rename");

HMENU
LoadPopupMenu(
    HINSTANCE hinst,
    UINT id
    )
{
    HMENU hmParent = LoadMenu(hinst, MAKEINTRESOURCE(id));
    if (NULL == hmParent)
    {
        return NULL;
    }

    HMENU hmPopup = GetSubMenu(hmParent, 0);
    RemoveMenu(hmParent, 0, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return hmPopup;
}

HMENU
MyGetMenuFromID(
    HMENU hmMain,
    UINT uID
    )
{
    MENUITEMINFO mii;

    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_SUBMENU;
    mii.cch    = 0;     // just in case

    if (!GetMenuItemInfo(hmMain, uID, FALSE, &mii))
    {
        return NULL;
    }

    return mii.hSubMenu;
}

int
MyMergePopupMenus(
    HMENU hmMain,
    HMENU hmMerge,
    int idCmdFirst,
    int idCmdLast)
{
    int i;
    int idTemp;
    int idMax = idCmdFirst;

    for (i = GetMenuItemCount(hmMerge) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_ID | MIIM_SUBMENU;
        mii.cch    = 0;     // just in case

        if (!GetMenuItemInfo(hmMerge, i, TRUE, &mii))
        {
            continue;
        }

        idTemp = Shell_MergeMenus(
                    MyGetMenuFromID(hmMain, mii.wID),
                    mii.hSubMenu,
                    0,
                    idCmdFirst,
                    idCmdLast,
                    MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
        if (idMax < idTemp)
        {
            idMax = idTemp;
        }
    }

    return idMax;
}


VOID
MyMergeMenu(
    HINSTANCE hinst,
    UINT idMainMerge,
    UINT idPopupMerge,
    LPQCMINFO pqcm)
{
    HMENU hmMerge;
    UINT idMax = pqcm->idCmdFirst;
    UINT idTemp;

    if (idMainMerge
        && (hmMerge = LoadPopupMenu(hinst, idMainMerge)) != NULL)
    {
        idMax = Shell_MergeMenus(
                        pqcm->hmenu,
                        hmMerge,
                        pqcm->indexMenu,
                        pqcm->idCmdFirst,
                        pqcm->idCmdLast,
                        MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
        DestroyMenu(hmMerge);
    }

    if (idPopupMerge
        && (hmMerge = LoadMenu(hinst, MAKEINTRESOURCE(idPopupMerge))) != NULL)
    {
        idTemp = MyMergePopupMenus(
                        pqcm->hmenu,
                        hmMerge,
                        pqcm->idCmdFirst,
                        pqcm->idCmdLast);
        if (idMax < idTemp)
        {
            idMax = idTemp;
        }
        DestroyMenu(hmMerge);
    }

    pqcm->idCmdFirst = idMax;
}


VOID
MyInsertMenu(
    HINSTANCE hinst,
    UINT idInsert,
    LPQCMINFO pqcm)
{
    HMENU hmInsert;
    UINT idMax = pqcm->idCmdFirst;

    if (idInsert
        && (hmInsert = LoadPopupMenu(hinst, idInsert)) != NULL)
    {
        for (int i = GetMenuItemCount(hmInsert) - 1; i >= 0; --i)
        {
            MENUITEMINFO mii;

            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_ID | MIIM_SUBMENU;
            mii.cch    = 0;     // just in case

            if (!GetMenuItemInfo(hmInsert, i, TRUE, &mii))
            {
                continue;
            }

            if (!InsertMenuItem(pqcm->hmenu, idMax, TRUE, &mii))
            {
                ++idMax;
            }
        }
        DestroyMenu(hmInsert);
    }

    pqcm->idCmdFirst = idMax;
}
