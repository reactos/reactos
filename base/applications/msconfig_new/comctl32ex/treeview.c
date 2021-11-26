/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig/treeview.c
 * PURPOSE:     Tree-View helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

// For TVIF_EXPANDEDIMAGE and TVIF_STATEEX (are they really useful ?)
#if !defined(_WIN32_IE) || (_WIN32_IE < 0x0600)
    #define _WIN32_IE 0x0600
#endif

// Fake _WIN32_WINNT to 0x0600 in order to get Vista+ style flags
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include "precomp.h"
#include "treeview.h"

#include <wingdi.h> // For RGB macro


void TreeView_Set3StateCheck(HWND hTree)
{
    LONG_PTR lStyle;
    DWORD dwExStyle;

    DWORD Major, Minor, Build;
    GetComCtl32Version(&Major, &Minor, &Build);

    /*
     * Choose the best way to handle 3-state TreeView checkboxes
     * according to the version of comctl32.dll we are running against.
     *
     * Starting version comctl32 version 6.10 (Vista+, via SxS)
     * we have native 3-state checkboxes available.
     * Only when comctl32 version 5.82 (no SxS) is available,
     * use its build number to know whether we should use 2k3-style
     * or Vista+ style check-boxes.
     */
    if (Major > 6 || (Major == 6 && Minor >= 10))
    {
        /*
         * NOTE: As explained in the following link:
         * http://stackoverflow.com/questions/31488233/treeview-setextendedstyle-does-not-apply-certain-styles-what-am-i-doing-wrong
         * the TreeView control should have the extended check-box style set
         * *BEFORE* actually setting the check-box window style, because it is
         * only at that step that the TreeView control builds its image list
         * containing the three check-box states. Indeed, if the extended
         * check-box style was applied after setting the window style, then
         * the image list would be already built with the default two states
         * and would not be updated.
         *
         * The MSDN documentation is not very clear on that point.
         *
         * Let me also take this opportunity to document what those
         * extended check-box state styles look like on Windows Vista+ :
         *
         * - TVS_EX_DIMMEDCHECKBOXES creates a grey tone version of the normal checked box state.
         * - TVS_EX_EXCLUSIONCHECKBOXES creates a red 'X'-style cross check-box state.
         * - TVS_EX_PARTIALCHECKBOXES creates a filled box.
         */
        dwExStyle = TreeView_GetExtendedStyle(hTree);
        TreeView_SetExtendedStyle(hTree, dwExStyle | TVS_EX_PARTIALCHECKBOXES, 0);

        lStyle = GetWindowLongPtr(hTree, GWL_STYLE);
        SetWindowLongPtr(hTree, GWL_STYLE, lStyle | TVS_CHECKBOXES);
    }
    else
    {
        lStyle = GetWindowLongPtr(hTree, GWL_STYLE);
        SetWindowLongPtr(hTree, GWL_STYLE, lStyle | TVS_CHECKBOXES);

        // TODO: Implement this function which should build at runtime
        // the image list with either two or three check-box states
        // (as it is done by the real common control TreeView), instead
        // of storing resource bitmaps.
        //
        // hCheckImageList = CreateCheckBoxImagelist(NULL, TRUE, TRUE, FALSE);
        TreeView_SetImageList(hTree,
                              ImageList_LoadBitmap(hInst, (Build >= 6000 ? MAKEINTRESOURCEW(IDB_V7CHECK) : MAKEINTRESOURCEW(IDB_2K3CHECK)), 16, 4, RGB(255, 255, 255)),
                              TVSIL_STATE);
    }
}

void TreeView_Cleanup(HWND hTree)
{
    // FIXME: Should we do it always, or only when the custom image list was set?
    ImageList_Destroy(TreeView_GetImageList(hTree, TVSIL_STATE));
}


HTREEITEM
InsertItem(HWND hTree,
           LPCWSTR szName,
           HTREEITEM hParent,
           HTREEITEM hInsertAfter)
{
    TVINSERTSTRUCTW tvis;
    SecureZeroMemory(&tvis, sizeof(tvis));

    tvis.hParent        = hParent;
    tvis.hInsertAfter   = hInsertAfter;
    tvis.itemex.mask    = TVIF_TEXT;
    tvis.itemex.pszText = (LPWSTR)szName;

    return (tvis.itemex.hItem = TreeView_InsertItem(hTree, &tvis));
}

UINT TreeView_GetRealSubtreeState(HWND hTree, HTREEITEM htiSubtreeItem)
{
#define OP(a, b) ((a) == (b) ? (a) : 2)

    HTREEITEM htiIterator       = TreeView_GetChild(hTree, htiSubtreeItem);
    UINT      uRealSubtreeState = TreeView_GetCheckState(hTree, htiIterator);
    /*
    while (htiIterator)
    {
        UINT temp = TreeView_GetCheckState(hTree, htiIterator);
        uRealSubtreeState = OP(uRealSubtreeState, temp);

        htiIterator = TreeView_GetNextSibling(hTree, htiIterator);
    }
    */
    while ( htiIterator && ( (htiIterator = TreeView_GetNextSibling(hTree, htiIterator)) != NULL ) )
    {
        UINT temp = TreeView_GetCheckState(hTree, htiIterator);
        uRealSubtreeState = OP(uRealSubtreeState, temp);
    }

    return uRealSubtreeState;
}

void TreeView_PropagateStateOfItemToParent(HWND hTree, HTREEITEM htiItem)
{
    HTREEITEM htiParent;
    UINT uGlobalSiblingsCheckState;

    if (!hTree || !htiItem /* || htiItem == TVI_ROOT */)
        return;

    htiParent = TreeView_GetParent(hTree, htiItem);
    if (!htiParent)
        return;

    uGlobalSiblingsCheckState = TreeView_GetRealSubtreeState(hTree, htiParent);
    TreeView_SetItemState(hTree, htiParent, INDEXTOSTATEIMAGEMASK(uGlobalSiblingsCheckState + 1), TVIS_STATEIMAGEMASK);
    TreeView_PropagateStateOfItemToParent(hTree, htiParent);

    return;
}

HTREEITEM Tree_Item_Copy(HWND hTree, HTREEITEM hSourceItem, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    HTREEITEM htiIterator;
    TVINSERTSTRUCTW tvis;
    WCHAR label[MAX_VALUE_NAME] = L"";

    if (!hTree || !hSourceItem || !hInsertAfter)
        return NULL;

    // 1- Retrieve properties.
    SecureZeroMemory(&tvis, sizeof(tvis));

    tvis.itemex.hItem = hSourceItem; // Handle of the item to be retrieved.
    tvis.itemex.mask  = TVIF_HANDLE | TVIF_TEXT | TVIF_STATE |
                        TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_EXPANDEDIMAGE |
                        TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATEEX;
    tvis.itemex.pszText    = label;
    tvis.itemex.cchTextMax = MAX_VALUE_NAME;
    TreeView_GetItem(hTree, &tvis.itemex);

    // 2- Now, copy to destination.
    tvis.hParent          = hParent;
    tvis.hInsertAfter     = hInsertAfter;
    tvis.itemex.stateMask = tvis.itemex.state;
    tvis.itemex.hItem     = TreeView_InsertItem(hTree, &tvis);

    for (htiIterator = TreeView_GetChild(hTree, hSourceItem) ; htiIterator ; htiIterator = TreeView_GetNextSibling(hTree, htiIterator))
        Tree_Item_Copy(hTree, htiIterator, tvis.itemex.hItem, TVI_LAST);

    return tvis.itemex.hItem;
}

void TreeView_DownItem(HWND hTree, HTREEITEM htiItemToDown)
{
    HTREEITEM htiNextItem, htiNewItem;

    if (!hTree || !htiItemToDown)
        return;

    htiNextItem = TreeView_GetNextSibling(hTree, htiItemToDown);
    if (!htiNextItem)
        htiNextItem = TVI_LAST;

    htiNewItem = Tree_Item_Copy(hTree, htiItemToDown, TreeView_GetParent(hTree, htiItemToDown), htiNextItem);
    TreeView_DeleteItem(hTree, htiItemToDown); // Delete the item and ALL its children.
    TreeView_SelectItem(hTree, htiNewItem);

    return;
}

void TreeView_UpItem(HWND hTree, HTREEITEM htiItemToUp)
{
    HTREEITEM htiPrevItem, htiPrevPrevItem, htiNewItem;

    if (!hTree || !htiItemToUp)
        return;

    htiPrevItem     = TreeView_GetPrevSibling(hTree, htiItemToUp);
    htiPrevPrevItem = TreeView_GetPrevSibling(hTree, htiPrevItem);
    if (!htiPrevPrevItem)
        htiPrevPrevItem = TVI_FIRST;
    // if htiPrevItem == NULL , htiPrevPrevItem == NULL.

    htiNewItem = Tree_Item_Copy(hTree, htiItemToUp, TreeView_GetParent(hTree, htiItemToUp), htiPrevPrevItem);
    TreeView_DeleteItem(hTree, htiItemToUp); // Delete the item and ALL its children.
    TreeView_SelectItem(hTree, htiNewItem);

    return;
}

HTREEITEM TreeView_GetFirst(HWND hTree)
{
    return TreeView_GetRoot(hTree);
}

HTREEITEM TreeView_GetLastFromItem(HWND hTree, HTREEITEM hItem)
{
    HTREEITEM htiRet = NULL;
    HTREEITEM htiIterator;

    for (htiIterator = hItem ; htiIterator ; htiIterator = TreeView_GetNextSibling(hTree, htiIterator))
        htiRet = htiIterator;

    return htiRet;
}

HTREEITEM TreeView_GetLast(HWND hTree)
{
    return TreeView_GetLastFromItem(hTree, TreeView_GetRoot(hTree));
}

HTREEITEM TreeView_GetPrev(HWND hTree, HTREEITEM hItem)
{
    HTREEITEM hPrev, hTmp;

    if (!hTree)
        return NULL;

    hPrev = TreeView_GetPrevSibling(hTree, hItem);
    if (!hPrev)
        return TreeView_GetParent(hTree, hItem);

    hTmp = TreeView_GetChild(hTree, hPrev);
    if (hTmp)
        return TreeView_GetLastFromItem(hTree, hTmp);
    else
        return hPrev;
}

HTREEITEM TreeView_GetNext(HWND hTree, HTREEITEM hItem)
{
    HTREEITEM hNext;

    if (!hTree)
        return NULL;

    hNext = TreeView_GetChild(hTree, hItem);
    if (hNext)
        return hNext;

    hNext = TreeView_GetNextSibling(hTree, hItem);
    if (hNext)
        return hNext;
    else
        return TreeView_GetNextSibling(hTree, TreeView_GetParent(hTree, hItem));
}

/* EOF */
