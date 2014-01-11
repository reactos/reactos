/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/treeview.c
 * PURPOSE:         TreeView functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

HWND hTreeView;

HTREEITEM
TreeViewAddItem(HTREEITEM hParent, LPWSTR lpText, INT Image, INT SelectedImage, LPARAM lParam)
{
    TV_INSERTSTRUCTW Insert;

    ZeroMemory(&Insert, sizeof(TV_INSERTSTRUCT));

    Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = hParent;
    Insert.item.iSelectedImage = SelectedImage;
    Insert.item.iImage = Image;
    Insert.item.lParam = lParam;
    Insert.item.pszText = lpText;

    return TreeView_InsertItem(hTreeView, &Insert);
}

BOOL
CreateTreeView(HWND hwnd)
{
    hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                WC_TREEVIEWW,
                                L"",
                                WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_SHOWSELALWAYS,
                                0, 28, 200, 350,
                                hwnd,
                                NULL,
                                hInst,
                                NULL);

    if (!hListView)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    SetFocus(hTreeView);

    return TRUE;
}
