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

UINT    ColumnDataHints[25];
TCHAR       szTemp[256];

int                 InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem);
INT_PTR CALLBACK    ColumnsDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void AddColumns(void)
{
    LRESULT        size;

    if (TaskManagerSettings.Column_ImageName) {
        LoadString(hInst, IDS_TAB_IMAGENAME, szTemp, 256);
        InsertColumn(0, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[0], -1);
    }
    if (TaskManagerSettings.Column_PID) {
        LoadString(hInst, IDS_TAB_PID, szTemp, 256);
        InsertColumn(1, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[1], -1);
    }
    if (TaskManagerSettings.Column_UserName) {
        LoadString(hInst, IDS_TAB_USERNAME, szTemp, 256);
        InsertColumn(2, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[2], -1);
    }
    if (TaskManagerSettings.Column_SessionID) {
        LoadString(hInst, IDS_TAB_SESSIONID, szTemp, 256);
        InsertColumn(3, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[3], -1);
    }
    if (TaskManagerSettings.Column_CPUUsage) {
        LoadString(hInst, IDS_TAB_CPU, szTemp, 256);
        InsertColumn(4, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[4], -1);
    }

    if (TaskManagerSettings.Column_CPUTime) {
        LoadString(hInst, IDS_TAB_CPUTIME, szTemp, 256);
        InsertColumn(5, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[5], -1);
    }

    if (TaskManagerSettings.Column_MemoryUsage) {
        LoadString(hInst, IDS_TAB_MEMUSAGE, szTemp, 256);
        InsertColumn(6, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[6], -1);
    }

    if (TaskManagerSettings.Column_PeakMemoryUsage) {
        LoadString(hInst, IDS_TAB_PEAKMEMUSAGE, szTemp, 256);
        InsertColumn(7, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[7], -1);
    }

    if (TaskManagerSettings.Column_MemoryUsageDelta) {
        LoadString(hInst, IDS_TAB_MEMDELTA, szTemp, 256);
        InsertColumn(8, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[8], -1);
    }

    if (TaskManagerSettings.Column_PageFaults) {
        LoadString(hInst, IDS_TAB_PAGEFAULT, szTemp, 256);
        InsertColumn(9, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[9], -1);
    }

    if (TaskManagerSettings.Column_PageFaultsDelta) {
        LoadString(hInst, IDS_TAB_PFDELTA, szTemp, 256);
        InsertColumn(10, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[10], -1);
    }

    if (TaskManagerSettings.Column_VirtualMemorySize) {
        LoadString(hInst, IDS_TAB_VMSIZE, szTemp, 256);
        InsertColumn(11, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[11], -1);
    }

    if (TaskManagerSettings.Column_PagedPool) {
        LoadString(hInst, IDS_TAB_PAGEDPOOL, szTemp, 256);
        InsertColumn(12, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[12], -1);
    }

    if (TaskManagerSettings.Column_NonPagedPool) {
        LoadString(hInst, IDS_TAB_NPPOOL, szTemp, 256);
        InsertColumn(13, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[13], -1);
    }

    if (TaskManagerSettings.Column_BasePriority) {
        LoadString(hInst, IDS_TAB_BASEPRI, szTemp, 256);
        InsertColumn(14, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[14], -1);
    }

    if (TaskManagerSettings.Column_HandleCount) {
        LoadString(hInst, IDS_TAB_HANDLES, szTemp, 256);
        InsertColumn(15, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[15], -1);
    }

    if (TaskManagerSettings.Column_ThreadCount) {
        LoadString(hInst, IDS_TAB_THREADS, szTemp, 256);
        InsertColumn(16, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[16], -1);
    }

    if (TaskManagerSettings.Column_USERObjects) {
        LoadString(hInst, IDS_TAB_USERPBJECTS, szTemp, 256);
        InsertColumn(17, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[17], -1);
    }

    if (TaskManagerSettings.Column_GDIObjects) {
        LoadString(hInst, IDS_TAB_GDIOBJECTS, szTemp, 256);
        InsertColumn(18, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[18], -1);
    }

    if (TaskManagerSettings.Column_IOReads) {
        LoadString(hInst, IDS_TAB_IOREADS, szTemp, 256);
        InsertColumn(19, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[19], -1);
    }

    if (TaskManagerSettings.Column_IOWrites) {
        LoadString(hInst, IDS_TAB_IOWRITES, szTemp, 256);
        InsertColumn(20, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[20], -1);
    }

    if (TaskManagerSettings.Column_IOOther) {
        LoadString(hInst, IDS_TAB_IOOTHER, szTemp, 256);
        InsertColumn(21, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[21], -1);
    }

    if (TaskManagerSettings.Column_IOReadBytes) {
        LoadString(hInst, IDS_TAB_IOREADBYTES, szTemp, 256);
        InsertColumn(22, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[22], -1);
    }

    if (TaskManagerSettings.Column_IOWriteBytes) {
        LoadString(hInst, IDS_TAB_IOWRITESBYTES, szTemp, 256);
        InsertColumn(23, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[23], -1);
    }

    if (TaskManagerSettings.Column_IOOtherBytes) {
        LoadString(hInst, IDS_TAB_IOOTHERBYTES, szTemp, 256);
        InsertColumn(24, szTemp, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[24], -1);
    }

    size = SendMessage(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessage(hProcessPageHeaderCtrl, HDM_SETORDERARRAY, (WPARAM) size, (LPARAM) &TaskManagerSettings.ColumnOrderArray);

    UpdateColumnDataHints();
}

int InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
{
    LVCOLUMN    column;

    column.mask = LVCF_TEXT|LVCF_FMT;
    column.pszText = (LPTSTR)lpszColumnHeading;
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
    HDITEM    hditem;
    int        i;
    TCHAR    text[260];
    LRESULT        size;

    /* Reset column data */
    for (i=0; i<25; i++)
        TaskManagerSettings.ColumnOrderArray[i] = i;

    TaskManagerSettings.Column_ImageName = FALSE;
    TaskManagerSettings.Column_PID = FALSE;
    TaskManagerSettings.Column_CPUUsage = FALSE;
    TaskManagerSettings.Column_CPUTime = FALSE;
    TaskManagerSettings.Column_MemoryUsage = FALSE;
    TaskManagerSettings.Column_MemoryUsageDelta = FALSE;
    TaskManagerSettings.Column_PeakMemoryUsage = FALSE;
    TaskManagerSettings.Column_PageFaults = FALSE;
    TaskManagerSettings.Column_USERObjects = FALSE;
    TaskManagerSettings.Column_IOReads = FALSE;
    TaskManagerSettings.Column_IOReadBytes = FALSE;
    TaskManagerSettings.Column_SessionID = FALSE;
    TaskManagerSettings.Column_UserName = FALSE;
    TaskManagerSettings.Column_PageFaultsDelta = FALSE;
    TaskManagerSettings.Column_VirtualMemorySize = FALSE;
    TaskManagerSettings.Column_PagedPool = FALSE;
    TaskManagerSettings.Column_NonPagedPool = FALSE;
    TaskManagerSettings.Column_BasePriority = FALSE;
    TaskManagerSettings.Column_HandleCount = FALSE;
    TaskManagerSettings.Column_ThreadCount = FALSE;
    TaskManagerSettings.Column_GDIObjects = FALSE;
    TaskManagerSettings.Column_IOWrites = FALSE;
    TaskManagerSettings.Column_IOWriteBytes = FALSE;
    TaskManagerSettings.Column_IOOther = FALSE;
    TaskManagerSettings.Column_IOOtherBytes = FALSE;
    TaskManagerSettings.ColumnSizeArray[0] = 105;
    TaskManagerSettings.ColumnSizeArray[1] = 50;
    TaskManagerSettings.ColumnSizeArray[2] = 107;
    TaskManagerSettings.ColumnSizeArray[3] = 70;
    TaskManagerSettings.ColumnSizeArray[4] = 35;
    TaskManagerSettings.ColumnSizeArray[5] = 70;
    TaskManagerSettings.ColumnSizeArray[6] = 70;
    TaskManagerSettings.ColumnSizeArray[7] = 100;
    TaskManagerSettings.ColumnSizeArray[8] = 70;
    TaskManagerSettings.ColumnSizeArray[9] = 70;
    TaskManagerSettings.ColumnSizeArray[10] = 70;
    TaskManagerSettings.ColumnSizeArray[11] = 70;
    TaskManagerSettings.ColumnSizeArray[12] = 70;
    TaskManagerSettings.ColumnSizeArray[13] = 70;
    TaskManagerSettings.ColumnSizeArray[14] = 60;
    TaskManagerSettings.ColumnSizeArray[15] = 60;
    TaskManagerSettings.ColumnSizeArray[16] = 60;
    TaskManagerSettings.ColumnSizeArray[17] = 60;
    TaskManagerSettings.ColumnSizeArray[18] = 60;
    TaskManagerSettings.ColumnSizeArray[19] = 70;
    TaskManagerSettings.ColumnSizeArray[20] = 70;
    TaskManagerSettings.ColumnSizeArray[21] = 70;
    TaskManagerSettings.ColumnSizeArray[22] = 70;
    TaskManagerSettings.ColumnSizeArray[23] = 70;
    TaskManagerSettings.ColumnSizeArray[24] = 70;

    /* Get header order */
    size = SendMessage(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessage(hProcessPageHeaderCtrl, HDM_GETORDERARRAY, (WPARAM) size, (LPARAM) &TaskManagerSettings.ColumnOrderArray);

    /* Get visible columns */
    for (i=0; i<SendMessage(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); i++) {
        memset(&hditem, 0, sizeof(HDITEM));

        hditem.mask = HDI_TEXT|HDI_WIDTH;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessage(hProcessPageHeaderCtrl, HDM_GETITEM, i, (LPARAM) &hditem);

        LoadString(hInst, IDS_TAB_IMAGENAME, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_ImageName = TRUE;
            TaskManagerSettings.ColumnSizeArray[0] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_PID, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_PID = TRUE;
            TaskManagerSettings.ColumnSizeArray[1] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_USERNAME, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_UserName = TRUE;
            TaskManagerSettings.ColumnSizeArray[2] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_SESSIONID, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_SessionID = TRUE;
            TaskManagerSettings.ColumnSizeArray[3] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_CPU, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_CPUUsage = TRUE;
            TaskManagerSettings.ColumnSizeArray[4] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_CPUTIME, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_CPUTime = TRUE;
            TaskManagerSettings.ColumnSizeArray[5] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_MEMUSAGE, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_MemoryUsage = TRUE;
            TaskManagerSettings.ColumnSizeArray[6] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_PEAKMEMUSAGE, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_PeakMemoryUsage = TRUE;
            TaskManagerSettings.ColumnSizeArray[7] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_MEMDELTA, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_MemoryUsageDelta = TRUE;
            TaskManagerSettings.ColumnSizeArray[8] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_PAGEFAULT, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_PageFaults = TRUE;
            TaskManagerSettings.ColumnSizeArray[9] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_PFDELTA, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_PageFaultsDelta = TRUE;
            TaskManagerSettings.ColumnSizeArray[10] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_VMSIZE, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_VirtualMemorySize = TRUE;
            TaskManagerSettings.ColumnSizeArray[11] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_PAGEDPOOL, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_PagedPool = TRUE;
            TaskManagerSettings.ColumnSizeArray[12] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_NPPOOL, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_NonPagedPool = TRUE;
            TaskManagerSettings.ColumnSizeArray[13] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_BASEPRI, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_BasePriority = TRUE;
            TaskManagerSettings.ColumnSizeArray[14] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_HANDLES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_HandleCount = TRUE;
            TaskManagerSettings.ColumnSizeArray[15] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_THREADS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_ThreadCount = TRUE;
            TaskManagerSettings.ColumnSizeArray[16] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_USERPBJECTS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_USERObjects = TRUE;
            TaskManagerSettings.ColumnSizeArray[17] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_GDIOBJECTS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_GDIObjects = TRUE;
            TaskManagerSettings.ColumnSizeArray[18] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_IOREADS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_IOReads = TRUE;
            TaskManagerSettings.ColumnSizeArray[19] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_IOWRITES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_IOWrites = TRUE;
            TaskManagerSettings.ColumnSizeArray[20] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_IOOTHER, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_IOOther = TRUE;
            TaskManagerSettings.ColumnSizeArray[21] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_IOREADBYTES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_IOReadBytes = TRUE;
            TaskManagerSettings.ColumnSizeArray[22] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_IOWRITESBYTES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_IOWriteBytes = TRUE;
            TaskManagerSettings.ColumnSizeArray[23] = hditem.cxy;
        }

        LoadString(hInst, IDS_TAB_IOOTHERBYTES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
        {
            TaskManagerSettings.Column_IOOtherBytes = TRUE;
            TaskManagerSettings.ColumnSizeArray[24] = hditem.cxy;
        }
    }
}

void ProcessPage_OnViewSelectColumns(void)
{
    int        i;

    if (DialogBox(hInst, MAKEINTRESOURCE(IDD_COLUMNS_DIALOG), hMainWnd, ColumnsDialogWndProc) == IDOK)
    {
        for (i=Header_GetItemCount(hProcessPageHeaderCtrl)-1; i>=0; i--)
        {
            (void)ListView_DeleteColumn(hProcessPageListCtrl, i);
        }

        for (i=0; i<25; i++)
            TaskManagerSettings.ColumnOrderArray[i] = i;

        TaskManagerSettings.ColumnSizeArray[0] = 105;
        TaskManagerSettings.ColumnSizeArray[1] = 50;
        TaskManagerSettings.ColumnSizeArray[2] = 107;
        TaskManagerSettings.ColumnSizeArray[3] = 70;
        TaskManagerSettings.ColumnSizeArray[4] = 35;
        TaskManagerSettings.ColumnSizeArray[5] = 70;
        TaskManagerSettings.ColumnSizeArray[6] = 70;
        TaskManagerSettings.ColumnSizeArray[7] = 100;
        TaskManagerSettings.ColumnSizeArray[8] = 70;
        TaskManagerSettings.ColumnSizeArray[9] = 70;
        TaskManagerSettings.ColumnSizeArray[10] = 70;
        TaskManagerSettings.ColumnSizeArray[11] = 70;
        TaskManagerSettings.ColumnSizeArray[12] = 70;
        TaskManagerSettings.ColumnSizeArray[13] = 70;
        TaskManagerSettings.ColumnSizeArray[14] = 60;
        TaskManagerSettings.ColumnSizeArray[15] = 60;
        TaskManagerSettings.ColumnSizeArray[16] = 60;
        TaskManagerSettings.ColumnSizeArray[17] = 60;
        TaskManagerSettings.ColumnSizeArray[18] = 60;
        TaskManagerSettings.ColumnSizeArray[19] = 70;
        TaskManagerSettings.ColumnSizeArray[20] = 70;
        TaskManagerSettings.ColumnSizeArray[21] = 70;
        TaskManagerSettings.ColumnSizeArray[22] = 70;
        TaskManagerSettings.ColumnSizeArray[23] = 70;
        TaskManagerSettings.ColumnSizeArray[24] = 70;

        AddColumns();
    }
}

INT_PTR CALLBACK
ColumnsDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_INITDIALOG:

        if (TaskManagerSettings.Column_ImageName)
            SendMessage(GetDlgItem(hDlg, IDC_IMAGENAME), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PID)
            SendMessage(GetDlgItem(hDlg, IDC_PID), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_UserName)
            SendMessage(GetDlgItem(hDlg, IDC_USERNAME), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_SessionID)
            SendMessage(GetDlgItem(hDlg, IDC_SESSIONID), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_CPUUsage)
            SendMessage(GetDlgItem(hDlg, IDC_CPUUSAGE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_CPUTime)
            SendMessage(GetDlgItem(hDlg, IDC_CPUTIME), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_MemoryUsage)
            SendMessage(GetDlgItem(hDlg, IDC_MEMORYUSAGE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PeakMemoryUsage)
            SendMessage(GetDlgItem(hDlg, IDC_PEAKMEMORYUSAGE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_MemoryUsageDelta)
            SendMessage(GetDlgItem(hDlg, IDC_MEMORYUSAGEDELTA), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PageFaults)
            SendMessage(GetDlgItem(hDlg, IDC_PAGEFAULTS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PageFaultsDelta)
            SendMessage(GetDlgItem(hDlg, IDC_PAGEFAULTSDELTA), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_VirtualMemorySize)
            SendMessage(GetDlgItem(hDlg, IDC_VIRTUALMEMORYSIZE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PagedPool)
            SendMessage(GetDlgItem(hDlg, IDC_PAGEDPOOL), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_NonPagedPool)
            SendMessage(GetDlgItem(hDlg, IDC_NONPAGEDPOOL), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_BasePriority)
            SendMessage(GetDlgItem(hDlg, IDC_BASEPRIORITY), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_HandleCount)
            SendMessage(GetDlgItem(hDlg, IDC_HANDLECOUNT), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_ThreadCount)
            SendMessage(GetDlgItem(hDlg, IDC_THREADCOUNT), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_USERObjects)
            SendMessage(GetDlgItem(hDlg, IDC_USEROBJECTS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_GDIObjects)
            SendMessage(GetDlgItem(hDlg, IDC_GDIOBJECTS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOReads)
            SendMessage(GetDlgItem(hDlg, IDC_IOREADS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOWrites)
            SendMessage(GetDlgItem(hDlg, IDC_IOWRITES), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOOther)
            SendMessage(GetDlgItem(hDlg, IDC_IOOTHER), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOReadBytes)
            SendMessage(GetDlgItem(hDlg, IDC_IOREADBYTES), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOWriteBytes)
            SendMessage(GetDlgItem(hDlg, IDC_IOWRITEBYTES), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOOtherBytes)
            SendMessage(GetDlgItem(hDlg, IDC_IOOTHERBYTES), BM_SETCHECK, BST_CHECKED, 0);

        return TRUE;

    case WM_COMMAND:

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            TaskManagerSettings.Column_ImageName = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IMAGENAME), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PID = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_PID), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_UserName = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_USERNAME), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_SessionID = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_SESSIONID), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_CPUUsage = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CPUUSAGE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_CPUTime = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CPUTIME), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_MemoryUsage = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_MEMORYUSAGE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PeakMemoryUsage = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_PEAKMEMORYUSAGE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_MemoryUsageDelta = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_MEMORYUSAGEDELTA), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PageFaults = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_PAGEFAULTS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PageFaultsDelta = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_PAGEFAULTSDELTA), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_VirtualMemorySize = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_VIRTUALMEMORYSIZE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PagedPool = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_PAGEDPOOL), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_NonPagedPool = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_NONPAGEDPOOL), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_BasePriority = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_BASEPRIORITY), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_HandleCount = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_HANDLECOUNT), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_ThreadCount = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_THREADCOUNT), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_USERObjects = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_USEROBJECTS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_GDIObjects = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_GDIOBJECTS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOReads = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IOREADS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOWrites = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IOWRITES), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOOther = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IOOTHER), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOReadBytes = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IOREADBYTES), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOWriteBytes = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IOWRITEBYTES), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOOtherBytes = (BOOL) SendMessage(GetDlgItem(hDlg, IDC_IOOTHERBYTES), BM_GETCHECK, 0, 0);

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}

void UpdateColumnDataHints(void)
{
    HDITEM            hditem;
    TCHAR            text[260];
    ULONG            Index;

    for (Index=0; Index<(ULONG)SendMessage(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); Index++)
    {
        memset(&hditem, 0, sizeof(HDITEM));

        hditem.mask = HDI_TEXT;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessage(hProcessPageHeaderCtrl, HDM_GETITEM, Index, (LPARAM) &hditem);

        LoadString(hInst, IDS_TAB_IMAGENAME, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IMAGENAME;

        LoadString(hInst, IDS_TAB_PID, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_PID;

        LoadString(hInst, IDS_TAB_USERNAME, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_USERNAME;

        LoadString(hInst, IDS_TAB_SESSIONID, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_SESSIONID;

        LoadString(hInst, IDS_TAB_CPU, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_CPUUSAGE;

        LoadString(hInst, IDS_TAB_CPUTIME, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_CPUTIME;

        LoadString(hInst, IDS_TAB_MEMUSAGE, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_MEMORYUSAGE;

        LoadString(hInst, IDS_TAB_PEAKMEMUSAGE, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_PEAKMEMORYUSAGE;

        LoadString(hInst, IDS_TAB_MEMDELTA, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_MEMORYUSAGEDELTA;

        LoadString(hInst, IDS_TAB_PAGEFAULT, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_PAGEFAULTS;

        LoadString(hInst, IDS_TAB_PFDELTA, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_PAGEFAULTSDELTA;

        LoadString(hInst, IDS_TAB_VMSIZE, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_VIRTUALMEMORYSIZE;

        LoadString(hInst, IDS_TAB_PAGEDPOOL, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_PAGEDPOOL;

        LoadString(hInst, IDS_TAB_NPPOOL, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_NONPAGEDPOOL;

        LoadString(hInst, IDS_TAB_BASEPRI, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_BASEPRIORITY;

        LoadString(hInst, IDS_TAB_HANDLES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_HANDLECOUNT;

        LoadString(hInst, IDS_TAB_THREADS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_THREADCOUNT;

        LoadString(hInst, IDS_TAB_USERPBJECTS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_USEROBJECTS;

        LoadString(hInst, IDS_TAB_GDIOBJECTS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_GDIOBJECTS;

        LoadString(hInst, IDS_TAB_IOREADS, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IOREADS;

        LoadString(hInst, IDS_TAB_IOWRITES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IOWRITES;

        LoadString(hInst, IDS_TAB_IOOTHER, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IOOTHER;

        LoadString(hInst, IDS_TAB_IOREADBYTES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IOREADBYTES;

        LoadString(hInst, IDS_TAB_IOWRITESBYTES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IOWRITEBYTES;

        LoadString(hInst, IDS_TAB_IOOTHERBYTES, szTemp, 256);
        if (_tcsicmp(text, szTemp) == 0)
            ColumnDataHints[Index] = COLUMN_IOOTHERBYTES;
    }
}
