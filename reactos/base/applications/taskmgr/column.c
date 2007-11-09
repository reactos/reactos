/*
 *  ReactOS Task Manager
 *
 *  column.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

UINT    ColumnDataHints[COLUMN_NMAX];
WCHAR       szTemp[256];

#define DECLARE_COLUMN_PRESET(_name, _size, _state) \
    { IDS_TAB_##_name, IDC_##_name, _size, _state },

const PresetColumnEntry ColumnPresets[COLUMN_NMAX] = {
    DECLARE_COLUMN_PRESET(IMAGENAME,        105,  TRUE)
    DECLARE_COLUMN_PRESET(PID,               50,  TRUE)
    DECLARE_COLUMN_PRESET(USERNAME,         107, FALSE)
    DECLARE_COLUMN_PRESET(SESSIONID,         70, FALSE)
    DECLARE_COLUMN_PRESET(CPUUSAGE,          35,  TRUE)
    DECLARE_COLUMN_PRESET(CPUTIME,           70,  TRUE)
    DECLARE_COLUMN_PRESET(MEMORYUSAGE,       70,  TRUE)
    DECLARE_COLUMN_PRESET(PEAKMEMORYUSAGE,  100, FALSE)
    DECLARE_COLUMN_PRESET(MEMORYUSAGEDELTA,  70, FALSE)
    DECLARE_COLUMN_PRESET(PAGEFAULTS,        70, FALSE)
    DECLARE_COLUMN_PRESET(PAGEFAULTSDELTA,   70, FALSE)
    DECLARE_COLUMN_PRESET(VIRTUALMEMORYSIZE, 70, FALSE)
    DECLARE_COLUMN_PRESET(PAGEDPOOL,         70, FALSE)
    DECLARE_COLUMN_PRESET(NONPAGEDPOOL,      70, FALSE)
    DECLARE_COLUMN_PRESET(BASEPRIORITY,      60, FALSE)
    DECLARE_COLUMN_PRESET(HANDLECOUNT,       60, FALSE)
    DECLARE_COLUMN_PRESET(THREADCOUNT,       60, FALSE)
    DECLARE_COLUMN_PRESET(USEROBJECTS,       60, FALSE)
    DECLARE_COLUMN_PRESET(GDIOBJECTS,        60, FALSE)
    DECLARE_COLUMN_PRESET(IOREADS,           70, FALSE)
    DECLARE_COLUMN_PRESET(IOWRITES,          70, FALSE)
    DECLARE_COLUMN_PRESET(IOOTHER,           70, FALSE)
    DECLARE_COLUMN_PRESET(IOREADBYTES,       70, FALSE)
    DECLARE_COLUMN_PRESET(IOWRITEBYTES,      70, FALSE)
    DECLARE_COLUMN_PRESET(IOOTHERBYTES,      70, FALSE)
};

int                 InsertColumn(int nCol, LPCWSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem);
INT_PTR CALLBACK    ColumnsDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void AddColumns(void)
{
    LRESULT       size;
    WCHAR         szTemp[256];
    unsigned int  n;

    for (n=0; n<COLUMN_NMAX; n++) {
        if (TaskManagerSettings.Columns[n]) {
            LoadStringW(hInst, ColumnPresets[n].dwIdsName, szTemp, sizeof(szTemp)/sizeof(WCHAR));
            InsertColumn(n, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[n], -1);
        }
    }

    size = SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessageW(hProcessPageHeaderCtrl, HDM_SETORDERARRAY, (WPARAM) size, (LPARAM) &TaskManagerSettings.ColumnOrderArray);

    UpdateColumnDataHints();
}

int InsertColumn(int nCol, LPCWSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
{
    LVCOLUMN  column;

    column.mask = LVCF_TEXT|LVCF_FMT;
    column.pszText = (LPWSTR)lpszColumnHeading;
    column.fmt = nFormat;

    if (nWidth != -1)
    {
        column.mask |= LVCF_WIDTH;
        column.cx = nWidth;
    }

    if (nSubItem != -1)
    {
        column.mask |= LVCF_SUBITEM;
        column.iSubItem = nSubItem;
    }

    return ListView_InsertColumn(hProcessPageListCtrl, nCol, &column);
}

void SaveColumnSettings(void)
{
    HDITEM        hditem;
    int           i, n;
    WCHAR         text[260];
    LRESULT       size;

    /* Reset column data */
    for (i=0; i<COLUMN_NMAX; i++) {
        TaskManagerSettings.ColumnOrderArray[i] = i;
        TaskManagerSettings.Columns[i] = FALSE;
        TaskManagerSettings.ColumnSizeArray[i] = ColumnPresets[i].size;
    }

    /* Get header order */
    size = SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessageW(hProcessPageHeaderCtrl, HDM_GETORDERARRAY, (WPARAM) size, (LPARAM) &TaskManagerSettings.ColumnOrderArray);

    /* Get visible columns */
    for (i = 0; i < SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); i++) {
        memset(&hditem, 0, sizeof(HDITEM));

        hditem.mask = HDI_TEXT|HDI_WIDTH;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEM, i, (LPARAM) &hditem);

        for (n = 0; n < COLUMN_NMAX; n++) {
            LoadStringW(hInst, ColumnPresets[n].dwIdsName, szTemp, sizeof(szTemp)/sizeof(WCHAR));
            if (_wcsicmp(text, szTemp) == 0)
            {
                TaskManagerSettings.Columns[n] = TRUE;
                TaskManagerSettings.ColumnSizeArray[n] = hditem.cxy;
            }
        }
    }
}

void ProcessPage_OnViewSelectColumns(void)
{
    int  i;

    if (DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_COLUMNS_DIALOG), hMainWnd, ColumnsDialogWndProc) == IDOK)
    {
        for (i=Header_GetItemCount(hProcessPageHeaderCtrl)-1; i>=0; i--)
        {
            (void)ListView_DeleteColumn(hProcessPageListCtrl, i);
        }

        for (i=0; i<COLUMN_NMAX; i++) {
            TaskManagerSettings.ColumnOrderArray[i] = i;
            TaskManagerSettings.ColumnSizeArray[i] = ColumnPresets[i].size;
        }

        AddColumns();
    }
}

INT_PTR CALLBACK
ColumnsDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    unsigned int  i;

    switch (message)
    {
    case WM_INITDIALOG:

        for (i=0; i<COLUMN_NMAX; i++) {
            if (TaskManagerSettings.Columns[i])
                SendMessageW(GetDlgItem(hDlg, ColumnPresets[i].dwIdcCtrl), BM_SETCHECK, BST_CHECKED, 0);
        }
        return TRUE;

    case WM_COMMAND:

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            for (i=0; i<COLUMN_NMAX; i++)
                TaskManagerSettings.Columns[i] = (BOOL) SendMessageW(GetDlgItem(hDlg, ColumnPresets[i].dwIdcCtrl), BM_GETCHECK, 0, 0);

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}

void UpdateColumnDataHints(void)
{
    HDITEM        hditem;
    WCHAR         text[260];
    ULONG         Index;
    WCHAR         szTemp[256];
    unsigned int  i;

    for (Index=0; Index<(ULONG)SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); Index++)
    {
        memset(&hditem, 0, sizeof(HDITEM));

        hditem.mask = HDI_TEXT;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEM, Index, (LPARAM) &hditem);

        for (i=0; i<COLUMN_NMAX; i++) {
            LoadStringW(hInst, ColumnPresets[i].dwIdsName, szTemp, sizeof(szTemp)/sizeof(WCHAR));
            if (_wcsicmp(text, szTemp) == 0)
                ColumnDataHints[Index] = i;
        }
    }
}
