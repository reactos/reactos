/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/listview.c
 * PURPOSE:     List-View helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "listview.h"

/////////////  ListView Sorting  /////////////

typedef struct __tagSort
{
    HWND hList;
    int nClickedColumn;
    BOOL bSortAsc;
} Sort, *PSort;

int CALLBACK
SortListView(LPARAM lItemParam1,
             LPARAM lItemParam2,
             LPARAM lPSort_S)
{
    PSort pSort = (PSort)lPSort_S;

    int iItem1 = (int)lItemParam1;
    int iItem2 = (int)lItemParam2;

    WCHAR strItem1[MAX_VALUE_NAME];
    WCHAR strItem2[MAX_VALUE_NAME];

    ListView_GetItemText(pSort->hList, iItem1, pSort->nClickedColumn, strItem1, MAX_VALUE_NAME);
    ListView_GetItemText(pSort->hList, iItem2, pSort->nClickedColumn, strItem2, MAX_VALUE_NAME);

    // StrCmpLogicalW helps in comparing numbers intelligently, 10 is greater that 2, other
    // wise string comparison will always return 2 is greater that 10...
    return ( pSort->bSortAsc ? StrCmpLogicalW(strItem1, strItem2) : StrCmpLogicalW(strItem2, strItem1) );
}

BOOL
ListView_SortEx(HWND hListView,
                int iSortingColumn,
                int iSortedColumn)
{
    HWND hHeader;
    HDITEM hColumn;
    BOOL bSortAsc;
    Sort sort;

    if (GetWindowLongPtr(hListView, GWL_STYLE) & LVS_NOSORTHEADER)
        return TRUE;

    hHeader = ListView_GetHeader(hListView);
    SecureZeroMemory(&hColumn, sizeof(hColumn));

    if ( (iSortedColumn != -1) && (iSortedColumn != iSortingColumn) )
    {
        hColumn.mask = HDI_FORMAT | HDI_LPARAM;
        Header_GetItem(hHeader, iSortedColumn, &hColumn);
        hColumn.fmt &= ~HDF_SORTUP & ~HDF_SORTDOWN;
        hColumn.lParam = 0; // 0: deactivated, 1: false, 2: true.
        Header_SetItem(hHeader, iSortedColumn, &hColumn);
    }

    hColumn.mask = HDI_FORMAT | HDI_LPARAM;
    Header_GetItem(hHeader, iSortingColumn, &hColumn);

    bSortAsc = !(hColumn.lParam == 2); // 0: deactivated, 1: false, 2: true.

    hColumn.fmt &= (bSortAsc ? ~HDF_SORTDOWN : ~HDF_SORTUP );
    hColumn.fmt |= (bSortAsc ?  HDF_SORTUP   : HDF_SORTDOWN);
    hColumn.lParam = (LPARAM)(bSortAsc ? 2 : 1);
    Header_SetItem(hHeader, iSortingColumn, &hColumn);

    /* Sort the list */
    sort.bSortAsc = bSortAsc;
    sort.hList    = hListView;
    sort.nClickedColumn = iSortingColumn;
    return ListView_SortItemsEx(hListView, SortListView, (LPARAM)&sort);
}

//////////////////////////////////////////////

/* EOF */
