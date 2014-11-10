/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/listview.c
 * PURPOSE:         ListView functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

HWND hListView;
BOOL bAscending = TRUE;

PVOID
ListViewGetlParam(INT Index)
{
    INT ItemIndex;
    LVITEM Item;

    if (Index == -1)
    {
        ItemIndex = (INT) SendMessage(hListView, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
        if (ItemIndex == -1)
            return NULL;
    }
    else
    {
        ItemIndex = Index;
    }

    ZeroMemory(&Item, sizeof(LVITEM));

    Item.mask = LVIF_PARAM;
    Item.iItem = ItemIndex;
    if (!ListView_GetItem(hListView, &Item))
        return NULL;

    return (PVOID)Item.lParam;
}

BOOL
ListViewAddColumn(INT Index, LPWSTR lpText, INT Width, INT Format)
{
    LV_COLUMN Column;

    ZeroMemory(&Column, sizeof(LV_COLUMN));

    Column.mask     = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.iSubItem = Index;
    Column.pszText  = (LPTSTR)lpText;
    Column.cx       = Width;
    Column.fmt      = Format;

    return (ListView_InsertColumn(hListView, Index, &Column) == -1) ? FALSE : TRUE;
}

INT
ListViewAddItem(INT ItemIndex, INT IconIndex, LPWSTR lpText, LPARAM lParam)
{
    LV_ITEMW Item;

    ZeroMemory(&Item, sizeof(LV_ITEM));

    Item.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    Item.pszText    = lpText;
    Item.lParam     = lParam;
    Item.iItem      = ItemIndex;
    Item.iImage     = IconIndex;

    return ListView_InsertItem(hListView, &Item);
}

INT
CALLBACK
ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    WCHAR Item1[MAX_STR_LEN], Item2[MAX_STR_LEN];
    LVFINDINFO IndexInfo;
    INT Index;

    IndexInfo.flags = LVFI_PARAM;

    IndexInfo.lParam = lParam1;
    Index = ListView_FindItem(hListView, -1, &IndexInfo);
    ListView_GetItemText(hListView, Index, (INT)lParamSort, Item1, sizeof(Item1) / sizeof(WCHAR));

    IndexInfo.lParam = lParam2;
    Index = ListView_FindItem(hListView, -1, &IndexInfo);
    ListView_GetItemText(hListView, Index, (INT)lParamSort, Item2, sizeof(Item2) / sizeof(WCHAR));

    if (bAscending)
        return wcscmp(Item2, Item1);
    else
        return wcscmp(Item1, Item2);

    return 0;
}

BOOL
CreateListView(HWND hwnd)
{
    hListView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                WC_LISTVIEWW,
                                L"",
                                WS_CHILD | WS_VISIBLE | LVS_SORTASCENDING | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                205, 28, 465, 250,
                                hwnd,
                                GetSubMenu(LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATIONMENU)), 0),
                                hInst,
                                NULL);

    if (!hListView)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    (VOID) ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

    return TRUE;
}
