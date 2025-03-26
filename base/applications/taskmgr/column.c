/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Processes List Columns.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

UINT    ColumnDataHints[COLUMN_NMAX];

#define DECLARE_COLUMN_PRESET(_name, _size, _state, _align) \
    { IDS_TAB_##_name, IDC_##_name, _size, _state, _align },

const PresetColumnEntry ColumnPresets[COLUMN_NMAX] = {
    DECLARE_COLUMN_PRESET(IMAGENAME,        105,  TRUE, LVCFMT_LEFT)
    DECLARE_COLUMN_PRESET(PID,               50,  TRUE, LVCFMT_LEFT)
    DECLARE_COLUMN_PRESET(USERNAME,         107, FALSE, LVCFMT_LEFT)
    DECLARE_COLUMN_PRESET(SESSIONID,         70, FALSE, LVCFMT_LEFT)
    DECLARE_COLUMN_PRESET(CPUUSAGE,          35,  TRUE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(CPUTIME,           70,  TRUE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(MEMORYUSAGE,       70,  TRUE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(PEAKMEMORYUSAGE,  100, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(MEMORYUSAGEDELTA,  70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(PAGEFAULTS,        70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(PAGEFAULTSDELTA,   70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(VIRTUALMEMORYSIZE, 70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(PAGEDPOOL,         70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(NONPAGEDPOOL,      70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(BASEPRIORITY,      60, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(HANDLECOUNT,       60, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(THREADCOUNT,       60, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(USEROBJECTS,       60, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(GDIOBJECTS,        60, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(IOREADS,           70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(IOWRITES,          70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(IOOTHER,           70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(IOREADBYTES,       70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(IOWRITEBYTES,      70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(IOOTHERBYTES,      70, FALSE, LVCFMT_RIGHT)
    DECLARE_COLUMN_PRESET(COMMANDLINE,      450, FALSE, LVCFMT_LEFT)
};

static int       InsertColumn(int nCol, LPCWSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem);
INT_PTR CALLBACK ColumnsDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void AddColumns(void)
{
    LRESULT       size;
    WCHAR         szTemp[256];
    unsigned int  n;

    for (n=0; n<COLUMN_NMAX; n++) {
        if (TaskManagerSettings.Columns[n]) {
            LoadStringW(hInst, ColumnPresets[n].dwIdsName, szTemp, _countof(szTemp));
            InsertColumn(n, szTemp, ColumnPresets[n].dwAlign, TaskManagerSettings.ColumnSizeArray[n], -1);
        }
    }

    size = SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessageW(hProcessPageHeaderCtrl, HDM_SETORDERARRAY, (WPARAM)size, (LPARAM)&TaskManagerSettings.ColumnOrderArray);

    UpdateColumnDataHints();
}

static int InsertColumn(int nCol, LPCWSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
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
    WCHAR         szTemp[256];
    LRESULT       size;

    /* Reset column data */
    for (i=0; i<COLUMN_NMAX; i++) {
        TaskManagerSettings.ColumnOrderArray[i] = i;
        TaskManagerSettings.Columns[i] = FALSE;
        TaskManagerSettings.ColumnSizeArray[i] = ColumnPresets[i].size;
    }

    /* Get header order */
    size = SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessageW(hProcessPageHeaderCtrl, HDM_GETORDERARRAY, (WPARAM)size, (LPARAM)&TaskManagerSettings.ColumnOrderArray);

    /* Get visible columns */
    for (i = 0; i < SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); i++) {
        memset(&hditem, 0, sizeof(HDITEM));

        hditem.mask = HDI_TEXT|HDI_WIDTH;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEM, i, (LPARAM) &hditem);

        for (n = 0; n < COLUMN_NMAX; n++) {
            LoadStringW(hInst, ColumnPresets[n].dwIdsName, szTemp, _countof(szTemp));
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
                CheckDlgButton(hDlg, ColumnPresets[i].dwIdcCtrl, BST_CHECKED);
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
                TaskManagerSettings.Columns[i] = (BOOL)IsDlgButtonChecked(hDlg, ColumnPresets[i].dwIdcCtrl);

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
    ULONG         uItems;
    WCHAR         szTemp[256];
    UINT          i;

    uItems = min(SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0), COLUMN_NMAX);

    for (Index=0; Index<uItems; Index++)
    {
        memset(&hditem, 0, sizeof(HDITEM));

        hditem.mask = HDI_TEXT;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEM, Index, (LPARAM) &hditem);

        for (i=0; i<COLUMN_NMAX; i++) {
            LoadStringW(hInst, ColumnPresets[i].dwIdsName, szTemp, _countof(szTemp));
            if (_wcsicmp(text, szTemp) == 0)
                ColumnDataHints[Index] = i;
        }
    }
}
