/*
 *  ReactOS Task Manager
 *
 *  procpage.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *  Copyright (C) 2009         Maxime Vernier <maxime.vernier@gmail.com>
 *  Copyright (C) 2022         Thamatip Chitpong <tangaming123456@outlook.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

#include "proclist.h"

#include <strsafe.h>

#include <ndk/psfuncs.h>

#define CMP(x1, x2)\
    (x1 < x2 ? -1 : (x1 > x2 ? 1 : 0))

typedef struct
{
    ULONG ProcessId;
} PROCESS_PAGE_LIST_ITEM, *LPPROCESS_PAGE_LIST_ITEM;

HWND hProcessPage;                      /* Process List Property Page */

HWND hProcessPageListCtrl;              /* Process ListCtrl Window */
HWND hProcessPageHeaderCtrl;            /* Process Header Control */
HWND hProcessPageEndProcessButton;      /* Process End Process button */
HWND hProcessPageShowAllProcessesButton;/* Process Show All Processes checkbox */
BOOL bProcessPageSelectionMade = FALSE; /* Is item in ListCtrl selected */

static int  nProcessPageWidth;
static int  nProcessPageHeight;
#ifdef RUN_PROC_PAGE
static HANDLE   hProcessThread = NULL;
static DWORD    dwProcessThread;
#endif

int CALLBACK    ProcessPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void AddProcess(ULONG Index);
void UpdateProcesses();
void gethmsfromlargeint(LARGE_INTEGER largeint, DWORD *dwHours, DWORD *dwMinutes, DWORD *dwSeconds);
void ProcessPageOnNotify(WPARAM wParam, LPARAM lParam);
UINT SH_FormatInteger(LONGLONG Num, LPWSTR pwszResult, UINT cchResultMax);
void ProcessPageShowContextMenu(DWORD dwProcessId);
BOOL PerfDataGetText(ULONG Index, ULONG ColumnIndex, LPTSTR lpText, ULONG nMaxCount);
DWORD WINAPI ProcessPageRefreshThread(void *lpParameter);
int ProcessRunning(ULONG ProcessId);

void Cleanup(void)
{
    int i;
    LV_ITEM item;
    LPPROCESS_PAGE_LIST_ITEM pData;
    for (i = 0; i < ListView_GetItemCount(hProcessPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hProcessPageListCtrl, &item);
        pData = (LPPROCESS_PAGE_LIST_ITEM)item.lParam;
        HeapFree(GetProcessHeap(), 0, pData);
    }
}

int ProcGetIndexByProcessId(DWORD dwProcessId)
{
    int     i;
    LVITEM  item;
    LPPROCESS_PAGE_LIST_ITEM pData;

    for (i=0; i<ListView_GetItemCount(hProcessPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hProcessPageListCtrl, &item);
        pData = (LPPROCESS_PAGE_LIST_ITEM)item.lParam;
        if (pData->ProcessId == dwProcessId)
        {
            return i;
        }
    }
    return 0;
}

DWORD GetSelectedProcessId(void)
{
    int     Index;
    LVITEM  lvitem;

    if(ListView_GetSelectedCount(hProcessPageListCtrl) == 1)
    {
        Index = ListView_GetSelectionMark(hProcessPageListCtrl);

        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_PARAM;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.lParam)
            return ((LPPROCESS_PAGE_LIST_ITEM)lvitem.lParam)->ProcessId;
    }

    return 0;
}

void ProcessPageUpdate(void)
{
    /* Enable or disable the "End Process" button */
    if (ListView_GetSelectedCount(hProcessPageListCtrl))
        EnableWindow(hProcessPageEndProcessButton, TRUE);
    else
        EnableWindow(hProcessPageEndProcessButton, FALSE);
}

INT_PTR CALLBACK
ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT    rc;
    int     nXDifference;
    int     nYDifference;
    int     cx, cy;

    switch (message) {
    case WM_INITDIALOG:
        /*
         * Save the width and height
         */
        GetClientRect(hDlg, &rc);
        nProcessPageWidth = rc.right;
        nProcessPageHeight = rc.bottom;

        /* Update window position */
        SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

        /*
         * Get handles to the controls
         */
        hProcessPageListCtrl = GetDlgItem(hDlg, IDC_PROCESSLIST);
        hProcessPageHeaderCtrl = ListView_GetHeader(hProcessPageListCtrl);
        hProcessPageEndProcessButton = GetDlgItem(hDlg, IDC_ENDPROCESS);
        hProcessPageShowAllProcessesButton = GetDlgItem(hDlg, IDC_SHOWALLPROCESSES);

        /*
         * Set the title, and extended window styles for the list control
         */
        SetWindowTextW(hProcessPageListCtrl, L"Processes");
        (void)ListView_SetExtendedListViewStyle(hProcessPageListCtrl, ListView_GetExtendedListViewStyle(hProcessPageListCtrl) | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

        AddColumns();

        /*
         * Subclass the process list control so we can intercept WM_ERASEBKGND
         */
        OldProcessListWndProc = (WNDPROC)SetWindowLongPtrW(hProcessPageListCtrl, GWLP_WNDPROC, (LONG_PTR)ProcessListWndProc);

#ifdef RUN_PROC_PAGE
        /* Start our refresh thread */
        hProcessThread = CreateThread(NULL, 0, ProcessPageRefreshThread, NULL, 0, &dwProcessThread);
#endif

        /* Refresh page */
        ProcessPageUpdate();

        return TRUE;

    case WM_DESTROY:
        /* Close the event handle, this will make the */
        /* refresh thread exit when the wait fails */
#ifdef RUN_PROC_PAGE
        EndLocalThread(&hProcessThread, dwProcessThread);
#endif
        SaveColumnSettings();
        Cleanup();
        break;

    case WM_COMMAND:
        /* Handle the button clicks */
        switch (LOWORD(wParam))
        {
                case IDC_ENDPROCESS:
                        ProcessPage_OnEndProcess();
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;

        cx = LOWORD(lParam);
        cy = HIWORD(lParam);
        nXDifference = cx - nProcessPageWidth;
        nYDifference = cy - nProcessPageHeight;
        nProcessPageWidth = cx;
        nProcessPageHeight = cy;

        /* Reposition the application page's controls */
        GetWindowRect(hProcessPageListCtrl, &rc);
        cx = (rc.right - rc.left) + nXDifference;
        cy = (rc.bottom - rc.top) + nYDifference;
        SetWindowPos(hProcessPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
        InvalidateRect(hProcessPageListCtrl, NULL, TRUE);

        GetClientRect(hProcessPageEndProcessButton, &rc);
        MapWindowPoints(hProcessPageEndProcessButton, hDlg, (LPPOINT)(PRECT)(&rc), sizeof(RECT)/sizeof(POINT));
           cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hProcessPageEndProcessButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hProcessPageEndProcessButton, NULL, TRUE);

        GetClientRect(hProcessPageShowAllProcessesButton, &rc);
        MapWindowPoints(hProcessPageShowAllProcessesButton, hDlg, (LPPOINT)(PRECT)(&rc), sizeof(RECT)/sizeof(POINT));
           cx = rc.left;
        cy = rc.top + nYDifference;
        SetWindowPos(hProcessPageShowAllProcessesButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hProcessPageShowAllProcessesButton, NULL, TRUE);
        break;

    case WM_NOTIFY:
        ProcessPageOnNotify(wParam, lParam);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_DELETE)
            ProcessPage_OnEndProcess();
        break;
    }

    return 0;
}

void ProcessPageOnNotify(WPARAM wParam, LPARAM lParam)
{
    LPNMHDR        pnmh;
    NMLVDISPINFO*  pnmdi;
    LPNMHEADER     pnmhdr;
    ULONG          Index;
    ULONG          ColumnIndex;
    LPPROCESS_PAGE_LIST_ITEM  pData;

    pnmh = (LPNMHDR) lParam;
    pnmdi = (NMLVDISPINFO*) lParam;
    pnmhdr = (LPNMHEADER) lParam;

    if (pnmh->hwndFrom == hProcessPageListCtrl)
    {
        switch (pnmh->code)
        {
        case LVN_ITEMCHANGED:
            ProcessPageUpdate();
            break;

        case LVN_GETDISPINFO:

            if (!(pnmdi->item.mask & LVIF_TEXT))
                break;

            pData = (LPPROCESS_PAGE_LIST_ITEM)pnmdi->item.lParam;
            Index = PerfDataGetProcessIndex(pData->ProcessId);
            ColumnIndex = pnmdi->item.iSubItem;

            PerfDataGetText(Index, ColumnIndex, pnmdi->item.pszText, (ULONG)pnmdi->item.cchTextMax);

            break;

        case NM_RCLICK:

            ProcessPageShowContextMenu(GetSelectedProcessId());
            break;

        case LVN_KEYDOWN:

            if (((LPNMLVKEYDOWN)lParam)->wVKey == VK_DELETE)
                ProcessPage_OnEndProcess();
            break;

        }
    }
    else if (pnmh->hwndFrom == hProcessPageHeaderCtrl)
    {
        switch (pnmh->code)
        {
        case HDN_ITEMCLICK:

            TaskManagerSettings.SortColumn = ColumnDataHints[pnmhdr->iItem];
            TaskManagerSettings.SortAscending = !TaskManagerSettings.SortAscending;
            (void)ListView_SortItems(hProcessPageListCtrl, ProcessPageCompareFunc, NULL);

            break;

        case HDN_ITEMCHANGED:

            UpdateColumnDataHints();

            break;

        case HDN_ENDDRAG:

            UpdateColumnDataHints();

            break;

        }
    }
}

UINT
SH_FormatInteger(LONGLONG Num, LPWSTR pwszResult, UINT cchResultMax)
{
    NUMBERFMTW nf;
    INT i;
    INT cchGrouping, cchResult;
    WCHAR wszNumber[24];
    WCHAR wszDecimalSep[8], wszThousandSep[8];
    WCHAR wszGrouping[12];

    /* Print the number in uniform mode */
    swprintf(wszNumber, L"%I64u", Num);

    // Get system strings for decimal and thousand separators.
    WCHAR wszDecimalSep[8], wszThousandSep[8];
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, wszDecimalSep, _countof(wszDecimalSep));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, wszThousandSep, _countof(wszThousandSep));

    /* Initialize format for printing the number in bytes */
    ZeroMemory(&nf, sizeof(nf));
    nf.lpDecimalSep = wszDecimalSep;
    nf.lpThousandSep = wszThousandSep;

    /* Get system string for groups separator */
    cchGrouping = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                                 LOCALE_SGROUPING,
                                 wszGrouping,
                                 _countof(wszGrouping));

    /* Convert grouping specs from string to integer */
    for (i = 0; i < cchGrouping; i++)
    {
        WCHAR wch = wszGrouping[i];

        if (wch >= L'0' && wch <= L'9')
            nf.Grouping = nf.Grouping * 10 + (wch - L'0');
        else if (wch != L';')
            break;
    }

    if ((nf.Grouping % 10) == 0)
        nf.Grouping /= 10;
    else
        nf.Grouping *= 10;

    // Format the number
    INT cchResult = GetNumberFormatW(LOCALE_USER_DEFAULT,
                                    0,
                                    wszNumber,
                                    &nf,
                                    pwszResult,
                                    cchResultMax);

    if (!cchResult)
        return 0;

    // GetNumberFormatW returns number of characters including UNICODE_NULL
    return cchResult - 1;
}

void ProcessPageShowContextMenu(DWORD dwProcessId)
{
    HMENU        hMenu;
    HMENU        hSubMenu;
    HMENU        hPriorityMenu;
    POINT        pt;
    SYSTEM_INFO  si;
    HANDLE       hProcess;
    DWORD        dwProcessPriorityClass;
    WCHAR        strDebugger[260];
    DWORD        dwDebuggerSize;
    HKEY         hKey;

    memset(&si, 0, sizeof(SYSTEM_INFO));

    GetCursorPos(&pt);
    GetSystemInfo(&si);

    hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_PROCESS_PAGE_CONTEXT));
    hSubMenu = GetSubMenu(hMenu, 0);
    hPriorityMenu = GetSubMenu(hSubMenu, 4);

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    dwProcessPriorityClass = GetPriorityClass(hProcess);
    CloseHandle(hProcess);

    if (si.dwNumberOfProcessors < 2)
        RemoveMenu(hSubMenu, ID_PROCESS_PAGE_SETAFFINITY, MF_BYCOMMAND);

    switch (dwProcessPriorityClass)    {
    case REALTIME_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, MF_BYCOMMAND);
        break;
    case HIGH_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_HIGH, MF_BYCOMMAND);
        break;
    case ABOVE_NORMAL_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_ABOVENORMAL, MF_BYCOMMAND);
        break;
    case NORMAL_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_NORMAL, MF_BYCOMMAND);
        break;
    case BELOW_NORMAL_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_BELOWNORMAL, MF_BYCOMMAND);
        break;
    case IDLE_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_LOW, MF_BYCOMMAND);
        break;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwDebuggerSize = sizeof(strDebugger);
        if (RegQueryValueExW(hKey, L"Debugger", NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) == ERROR_SUCCESS)
        {
            CharUpper(strDebugger);
            if (wcsstr(strDebugger, L"DRWTSN32"))
                EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
        else
            EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

        RegCloseKey(hKey);
    } else {
        EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    }
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);
    DestroyMenu(hMenu);
}

void RefreshProcessPage(void)
{
#ifdef RUN_PROC_PAGE
    /* Signal the event so that our refresh thread */
    /* will wake up and refresh the process page */
    PostThreadMessage(dwProcessThread, WM_TIMER, 0, 0);
#endif
}

DWORD WINAPI ProcessPageRefreshThread(void *lpParameter)
{
    MSG      msg;

    while (1) {
        /*  Wait for an the event or application close */
        if (GetMessage(&msg, NULL, 0, 0) <= 0)
            return 0;

        if (msg.message == WM_TIMER) {

            UpdateProcesses();

            if (IsWindowVisible(hProcessPage))
                InvalidateRect(hProcessPageListCtrl, NULL, FALSE);

            ProcessPageUpdate();
        }
    }
    return 0;
}

void UpdateProcesses()
{
    int i;
    ULONG l;
    LV_ITEM item;
    LPPROCESS_PAGE_LIST_ITEM pData;

    SendMessage(hProcessPageListCtrl, WM_SETREDRAW, FALSE, 0);

    /* Remove old processes */
    for (i = 0; i < ListView_GetItemCount(hProcessPageListCtrl); i++)
    {
        memset(&item, 0, sizeof (LV_ITEM));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hProcessPageListCtrl, &item);
        pData = (LPPROCESS_PAGE_LIST_ITEM)item.lParam;
        if (!ProcessRunning(pData->ProcessId))
        {
            (void)ListView_DeleteItem(hProcessPageListCtrl, i);
            HeapFree(GetProcessHeap(), 0, pData);
        }
    }

    /* Check for difference in listview process and performance process counts */
    if (ListView_GetItemCount(hProcessPageListCtrl) != PerfDataGetProcessCount())
    {
        /* Add new processes by checking against the current items */
        for (l = 0; l < PerfDataGetProcessCount(); l++)
        {
            AddProcess(l);
        }
    }

    if (TaskManagerSettings.SortColumn != -1)
    {
        (void)ListView_SortItems(hProcessPageListCtrl, ProcessPageCompareFunc, NULL);
    }

    SendMessage(hProcessPageListCtrl, WM_SETREDRAW, TRUE, 0);

    /* Select first item if any */
    if ((ListView_GetNextItem(hProcessPageListCtrl, -1, LVNI_FOCUSED | LVNI_SELECTED) == -1) &&
        (ListView_GetItemCount(hProcessPageListCtrl) > 0) && !bProcessPageSelectionMade)
    {
        ListView_SetItemState(hProcessPageListCtrl, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        bProcessPageSelectionMade = TRUE;
    }
    /*
    else
    {
        bProcessPageSelectionMade = FALSE;
    }
    */
}

BOOL ProcessRunning(ULONG ProcessId)
{
    HANDLE hProcess;
    DWORD exitCode;

    if (ProcessId == 0) {
        return TRUE;
    }

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
    if (hProcess == NULL) {
        return FALSE;
    }

    if (GetExitCodeProcess(hProcess, &exitCode)) {
        CloseHandle(hProcess);
        return (exitCode == STILL_ACTIVE);
    }

    CloseHandle(hProcess);
    return FALSE;
}

void AddProcess(ULONG Index)
{
    LPPROCESS_PAGE_LIST_ITEM pData;
    int         i;
    LV_ITEM     item;
    BOOL    bAlreadyInList = FALSE;
    ULONG pid;

    pid = PerfDataGetProcessId(Index);

    /* Check to see if it's already in our list */
    for (i=0; i<ListView_GetItemCount(hProcessPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hProcessPageListCtrl, &item);
        pData = (LPPROCESS_PAGE_LIST_ITEM)item.lParam;
        if (pData->ProcessId == pid)
        {
            bAlreadyInList = TRUE;
            break;
        }
    }
    if (!bAlreadyInList)  /* Add */
    {
        pData = (LPPROCESS_PAGE_LIST_ITEM)HeapAlloc(GetProcessHeap(), 0, sizeof(PROCESS_PAGE_LIST_ITEM));
        pData->ProcessId = pid;

        /* Add the item to the list */
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_TEXT|LVIF_PARAM;
        item.pszText = LPSTR_TEXTCALLBACK;
        item.iItem = ListView_GetItemCount(hProcessPageListCtrl);
        item.lParam = (LPARAM)pData;
        (void)ListView_InsertItem(hProcessPageListCtrl, &item);
    }
}

BOOL PerfDataGetText(ULONG Index, ULONG ColumnIndex, LPTSTR lpText, ULONG nMaxCount)
{
    IO_COUNTERS    iocounters;
    LARGE_INTEGER  time;

    if (ColumnDataHints[ColumnIndex] == COLUMN_IMAGENAME)
        PerfDataGetImageName(Index, lpText, nMaxCount);
    if (ColumnDataHints[ColumnIndex] == COLUMN_PID)
        wsprintfW(lpText, L"%lu", PerfDataGetProcessId(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_USERNAME)
        PerfDataGetUserName(Index, lpText, nMaxCount);
    if (ColumnDataHints[ColumnIndex] == COLUMN_COMMANDLINE)
        PerfDataGetCommandLine(Index, lpText, nMaxCount);
    if (ColumnDataHints[ColumnIndex] == COLUMN_SESSIONID)
        wsprintfW(lpText, L"%lu", PerfDataGetSessionId(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_CPUUSAGE)
        wsprintfW(lpText, L"%02lu", PerfDataGetCPUUsage(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_CPUTIME)
    {
        DWORD dwHours;
        DWORD dwMinutes;
        DWORD dwSeconds;

        time = PerfDataGetCPUTime(Index);
        gethmsfromlargeint(time, &dwHours, &dwMinutes, &dwSeconds);
        wsprintfW(lpText, L"%lu:%02lu:%02lu", dwHours, dwMinutes, dwSeconds);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGE)
    {
        SH_FormatInteger(PerfDataGetWorkingSetSizeBytes(Index) / 1024, lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PEAKMEMORYUSAGE)
    {
        SH_FormatInteger(PerfDataGetPeakWorkingSetSizeBytes(Index) / 1024, lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGEDELTA)
    {
        SH_FormatInteger(PerfDataGetWorkingSetSizeDelta(Index) / 1024, lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTS)
    {
        SH_FormatInteger(PerfDataGetPageFaultCount(Index), lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTSDELTA)
    {
        SH_FormatInteger(PerfDataGetPageFaultCountDelta(Index), lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_VIRTUALMEMORYSIZE)
    {
        SH_FormatInteger(PerfDataGetVirtualMemorySizeBytes(Index) / 1024, lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEDPOOL)
    {
        SH_FormatInteger(PerfDataGetPagedPoolUsagePages(Index) / 1024, lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_NONPAGEDPOOL)
    {
        SH_FormatInteger(PerfDataGetNonPagedPoolUsagePages(Index) / 1024, lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_BASEPRIORITY)
        wsprintfW(lpText, L"%lu", PerfDataGetBasePriority(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_HANDLECOUNT)
    {
        SH_FormatInteger(PerfDataGetHandleCount(Index), lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_THREADCOUNT)
    {
        SH_FormatInteger(PerfDataGetThreadCount(Index), lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_USEROBJECTS)
    {
        SH_FormatInteger(PerfDataGetUSERObjectCount(Index), lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_GDIOBJECTS)
    {
        SH_FormatInteger(PerfDataGetGDIObjectCount(Index), lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADS)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.ReadOperationCount); */
        SH_FormatInteger(iocounters.ReadOperationCount, lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.WriteOperationCount); */
        SH_FormatInteger(iocounters.WriteOperationCount, lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHER)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.OtherOperationCount); */
        SH_FormatInteger(iocounters.OtherOperationCount, lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADBYTES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.ReadTransferCount); */
        SH_FormatInteger(iocounters.ReadTransferCount, lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITEBYTES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        SH_FormatInteger(iocounters.WriteTransferCount, lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHERBYTES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        SH_FormatInteger(iocounters.OtherTransferCount, lpText, nMaxCount);
    }

    return FALSE;
}


void gethmsfromlargeint(LARGE_INTEGER largeint, DWORD *dwHours, DWORD *dwMinutes, DWORD *dwSeconds)
{
#ifdef _MSC_VER
    *dwHours = (DWORD)(largeint.QuadPart / 36000000000L);
    *dwMinutes = (DWORD)((largeint.QuadPart % 36000000000L) / 600000000L);
    *dwSeconds = (DWORD)(((largeint.QuadPart % 36000000000L) % 600000000L) / 10000000L);
#else
    *dwHours = (DWORD)(largeint.QuadPart / 36000000000LL);
    *dwMinutes = (DWORD)((largeint.QuadPart % 36000000000LL) / 600000000LL);
    *dwSeconds = (DWORD)(((largeint.QuadPart % 36000000000LL) % 600000000LL) / 10000000LL);
#endif
}

int largeintcmp(LARGE_INTEGER l1, LARGE_INTEGER l2)
{
    int ret = 0;
    DWORD dwHours1;
    DWORD dwMinutes1;
    DWORD dwSeconds1;
    DWORD dwHours2;
    DWORD dwMinutes2;
    DWORD dwSeconds2;

    gethmsfromlargeint(l1, &dwHours1, &dwMinutes1, &dwSeconds1);
    gethmsfromlargeint(l2, &dwHours2, &dwMinutes2, &dwSeconds2);
    ret = CMP(dwHours1, dwHours2);
    if (ret == 0)
    {
        ret = CMP(dwMinutes1, dwMinutes2);
        if (ret == 0)
        {
            ret = CMP(dwSeconds1, dwSeconds2);
        }
    }
    return ret;
}

int CALLBACK ProcessPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int ret = 0;
    LPPROCESS_PAGE_LIST_ITEM Param1;
    LPPROCESS_PAGE_LIST_ITEM Param2;
    ULONG IndexParam1;
    ULONG IndexParam2;
    WCHAR text1[260];
    WCHAR text2[260];
    ULONG l1;
    ULONG l2;
    LARGE_INTEGER  time1;
    LARGE_INTEGER  time2;
    IO_COUNTERS    iocounters1;
    IO_COUNTERS    iocounters2;
    ULONGLONG      ull1;
    ULONGLONG      ull2;

    if (TaskManagerSettings.SortAscending) {
        Param1 = (LPPROCESS_PAGE_LIST_ITEM)lParam1;
        Param2 = (LPPROCESS_PAGE_LIST_ITEM)lParam2;
    } else {
        Param1 = (LPPROCESS_PAGE_LIST_ITEM)lParam2;
        Param2 = (LPPROCESS_PAGE_LIST_ITEM)lParam1;
    }
    IndexParam1 = PerfDataGetProcessIndex(Param1->ProcessId);
    IndexParam2 = PerfDataGetProcessIndex(Param2->ProcessId);

    if (TaskManagerSettings.SortColumn == COLUMN_IMAGENAME)
    {
        PerfDataGetImageName(IndexParam1, text1, sizeof (text1) / sizeof (*text1));
        PerfDataGetImageName(IndexParam2, text2, sizeof (text2) / sizeof (*text2));
        ret = _wcsicmp(text1, text2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PID)
    {
        l1 = Param1->ProcessId;
        l2 = Param2->ProcessId;
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_USERNAME)
    {
        PerfDataGetUserName(IndexParam1, text1, sizeof (text1) / sizeof (*text1));
        PerfDataGetUserName(IndexParam2, text2, sizeof (text2) / sizeof (*text2));
        ret = _wcsicmp(text1, text2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_COMMANDLINE)
    {
        PerfDataGetCommandLine(IndexParam1, text1, sizeof (text1) / sizeof (*text1));
        PerfDataGetCommandLine(IndexParam2, text2, sizeof (text2) / sizeof (*text2));
        ret = _wcsicmp(text1, text2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_SESSIONID)
    {
        l1 = PerfDataGetSessionId(IndexParam1);
        l2 = PerfDataGetSessionId(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_CPUUSAGE)
    {
        l1 = PerfDataGetCPUUsage(IndexParam1);
        l2 = PerfDataGetCPUUsage(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_CPUTIME)
    {
        time1 = PerfDataGetCPUTime(IndexParam1);
        time2 = PerfDataGetCPUTime(IndexParam2);
        ret = largeintcmp(time1, time2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_MEMORYUSAGE)
    {
        l1 = PerfDataGetWorkingSetSizeBytes(IndexParam1);
        l2 = PerfDataGetWorkingSetSizeBytes(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PEAKMEMORYUSAGE)
    {
        l1 = PerfDataGetPeakWorkingSetSizeBytes(IndexParam1);
        l2 = PerfDataGetPeakWorkingSetSizeBytes(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_MEMORYUSAGEDELTA)
    {
        l1 = PerfDataGetWorkingSetSizeDelta(IndexParam1);
        l2 = PerfDataGetWorkingSetSizeDelta(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PAGEFAULTS)
    {
        l1 = PerfDataGetPageFaultCount(IndexParam1);
        l2 = PerfDataGetPageFaultCount(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PAGEFAULTSDELTA)
    {
        l1 = PerfDataGetPageFaultCountDelta(IndexParam1);
        l2 = PerfDataGetPageFaultCountDelta(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_VIRTUALMEMORYSIZE)
    {
        l1 = PerfDataGetVirtualMemorySizeBytes(IndexParam1);
        l2 = PerfDataGetVirtualMemorySizeBytes(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PAGEDPOOL)
    {
        l1 = PerfDataGetPagedPoolUsagePages(IndexParam1);
        l2 = PerfDataGetPagedPoolUsagePages(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_NONPAGEDPOOL)
    {
        l1 = PerfDataGetNonPagedPoolUsagePages(IndexParam1);
        l2 = PerfDataGetNonPagedPoolUsagePages(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_BASEPRIORITY)
    {
        l1 = PerfDataGetBasePriority(IndexParam1);
        l2 = PerfDataGetBasePriority(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_HANDLECOUNT)
    {
        l1 = PerfDataGetHandleCount(IndexParam1);
        l2 = PerfDataGetHandleCount(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_THREADCOUNT)
    {
        l1 = PerfDataGetThreadCount(IndexParam1);
        l2 = PerfDataGetThreadCount(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_USEROBJECTS)
    {
        l1 = PerfDataGetUSERObjectCount(IndexParam1);
        l2 = PerfDataGetUSERObjectCount(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_GDIOBJECTS)
    {
        l1 = PerfDataGetGDIObjectCount(IndexParam1);
        l2 = PerfDataGetGDIObjectCount(IndexParam2);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOREADS)
    {
        PerfDataGetIOCounters(IndexParam1, &iocounters1);
        PerfDataGetIOCounters(IndexParam2, &iocounters2);
        ull1 = iocounters1.ReadOperationCount;
        ull2 = iocounters2.ReadOperationCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOWRITES)
    {
        PerfDataGetIOCounters(IndexParam1, &iocounters1);
        PerfDataGetIOCounters(IndexParam2, &iocounters2);
        ull1 = iocounters1.WriteOperationCount;
        ull2 = iocounters2.WriteOperationCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOOTHER)
    {
        PerfDataGetIOCounters(IndexParam1, &iocounters1);
        PerfDataGetIOCounters(IndexParam2, &iocounters2);
        ull1 = iocounters1.OtherOperationCount;
        ull2 = iocounters2.OtherOperationCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOREADBYTES)
    {
        PerfDataGetIOCounters(IndexParam1, &iocounters1);
        PerfDataGetIOCounters(IndexParam2, &iocounters2);
        ull1 = iocounters1.ReadTransferCount;
        ull2 = iocounters2.ReadTransferCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOWRITEBYTES)
    {
        PerfDataGetIOCounters(IndexParam1, &iocounters1);
        PerfDataGetIOCounters(IndexParam2, &iocounters2);
        ull1 = iocounters1.WriteTransferCount;
        ull2 = iocounters2.WriteTransferCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOOTHERBYTES)
    {
        PerfDataGetIOCounters(IndexParam1, &iocounters1);
        PerfDataGetIOCounters(IndexParam2, &iocounters2);
        ull1 = iocounters1.OtherTransferCount;
        ull2 = iocounters2.OtherTransferCount;
        ret = CMP(ull1, ull2);
    }
    return ret;
}

/**
 * @brief
 * Maps an NT "\Device\..." path to its Win32 "DOS" equivalent.
 *
 * @param[in]   lpDevicePath
 * The NT device path to convert.
 *
 * @param[out]  lpDosPath
 * Receives the converted Win32 path.
 *
 * @param[in]   dwLength
 * Size of the lpDosPath buffer in characters.
 *
 * @return
 * The number of characters required (if lpDosPath == NULL or dwLength == 0),
 * or actually written in the lpDosPath buffer, including the NULL terminator.
 * Returns 0 in case of failure.
 **/
static DWORD
DevicePathToDosPath(
    _In_ LPCWSTR lpDevicePath,
    _Out_writes_to_opt_(dwLength, return)
         LPWSTR lpDosPath,
    _In_opt_ DWORD dwLength)
{
    DWORD dwRet = 0;
    WCHAR szDrive[3] = L"?:";
    WCHAR szDeviceName[MAX_PATH];

    /* Check if lpDevicePath is a device path */
    if (_wcsnicmp(lpDevicePath, L"\\Device\\", _countof(L"\\Device\\")-1) != 0)
    {
        return 0;
    }

    for (szDrive[0] = L'A'; szDrive[0] <= L'`'; szDrive[0]++)
    {
        if (QueryDosDeviceW(szDrive, szDeviceName, _countof(szDeviceName)) != 0)
        {
            size_t len = wcslen(szDeviceName);

            if (_wcsnicmp(lpDevicePath, szDeviceName, len) == 0)
            {
                /* Get the required length, including the NULL terminator */
                dwRet = _countof(szDrive) + wcslen(lpDevicePath + len);

                if (lpDosPath && (dwLength >= dwRet))
                {
                    StringCchPrintfW(lpDosPath, dwLength, L"%s%s",
                                     szDrive, lpDevicePath + len);
                }

                break;
            }
        }
    }

    return dwRet;
}

/**
 * @brief
 * Retrieves the Win32 path of an executable image, by handle.
 *
 * @param[in]   hProcess
 * Handle to the executable image; it should be opened with
 * PROCESS_QUERY_INFORMATION access rights.
 *
 * @param[out]  lpExePath
 * Receives the Win32 image path.
 *
 * @param[in]   dwLength
 * Size of the lpExePath buffer in characters.
 *
 * @return
 * The number of characters required (if lpExePath == NULL or dwLength == 0),
 * or actually written in the lpExePath buffer, including the NULL terminator.
 * Returns 0 in case of failure.
 **/
static DWORD
GetProcessExecutablePath(
    _In_ HANDLE hProcess,
    _Out_writes_to_opt_(dwLength, return)
         LPWSTR lpExePath,
    _In_opt_ DWORD dwLength)
{
    DWORD dwRet = 0;
    NTSTATUS Status;
    BYTE StaticBuffer[sizeof(UNICODE_STRING) + (MAX_PATH * sizeof(WCHAR))];
    PVOID DynamicBuffer = NULL;
    PUNICODE_STRING ExePath;
    ULONG SizeNeeded;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       StaticBuffer,
                                       /* Reserve a NULL terminator */
                                       sizeof(StaticBuffer) - sizeof(WCHAR),
                                       &SizeNeeded);
    if (NT_SUCCESS(Status))
    {
        ExePath = (PUNICODE_STRING)StaticBuffer;
    }
    else if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        /* Allocate the buffer, reserving space for a NULL terminator */
        DynamicBuffer = HeapAlloc(GetProcessHeap(), 0, SizeNeeded + sizeof(WCHAR));
        if (!DynamicBuffer)
            return 0;

        Status = NtQueryInformationProcess(hProcess,
                                           ProcessImageFileName,
                                           DynamicBuffer,
                                           SizeNeeded,
                                           &SizeNeeded);
        if (!NT_SUCCESS(Status))
            goto Cleanup;

        ExePath = DynamicBuffer;
    }
    else
    {
        return 0;
    }

    /* Manually NULL-terminate */
    ExePath->Buffer[ExePath->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* HACK: Convert device path format into Win32 path format.
     * Use ProcessImageFileNameWin32 instead if the kernel supports it. */
    dwRet = DevicePathToDosPath(ExePath->Buffer, lpExePath, dwLength);

Cleanup:
    HeapFree(GetProcessHeap(), 0, DynamicBuffer);

    return dwRet;
}

/**
 * @brief
 * Retrieves the Win32 path of an executable image, by identifier.
 *
 * @param[in]   dwProcessId
 * Identifier of the running executable image.
 *
 * @param[out]  lpExePath
 * Receives the Win32 image path.
 *
 * @param[in]   dwLength
 * Size of the lpExePath buffer in characters.
 *
 * @return
 * The number of characters required (if lpExePath == NULL or dwLength == 0),
 * or actually written in the lpExePath buffer, including the NULL terminator.
 * Returns 0 in case of failure.
 **/
static DWORD
GetProcessExecutablePathById(
    _In_ DWORD dwProcessId,
    _Out_writes_to_opt_(dwLength, return)
         LPWSTR lpExePath,
    _In_opt_ DWORD dwLength)
{
    DWORD dwRet = 0;

    if (dwProcessId == 0)
        return 0;

    /* PID = 4 ("System") */
    if (dwProcessId == 4)
    {
        static const WCHAR szKernelExe[] = L"\\ntoskrnl.exe";
        LPWSTR pszSystemDir;
        UINT uLength;

        uLength = GetSystemDirectoryW(NULL, 0);
        if (uLength == 0)
            return 0;

        pszSystemDir = HeapAlloc(GetProcessHeap(), 0, uLength * sizeof(WCHAR));
        if (!pszSystemDir)
            return 0;

        if (GetSystemDirectoryW(pszSystemDir, uLength) != 0)
        {
            /* Get the required length, including the NULL terminator */
            dwRet = uLength + _countof(szKernelExe) - 1;

            if (lpExePath && (dwLength >= dwRet))
            {
                StringCchPrintfW(lpExePath, dwLength, L"%s%s",
                                 pszSystemDir, szKernelExe);
            }
        }

        HeapFree(GetProcessHeap(), 0, pszSystemDir);
    }
    else
    {
        HANDLE hProcess;

        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
        if (hProcess)
        {
            dwRet = GetProcessExecutablePath(hProcess, lpExePath, dwLength);
            CloseHandle(hProcess);
        }
    }

    return dwRet;
}

void ProcessPage_OnProperties(void)
{
    DWORD dwProcessId;
    DWORD dwLength;
    LPWSTR pszExePath;
    SHELLEXECUTEINFOW info = { 0 };

    dwProcessId = GetSelectedProcessId();

    /* Retrieve the image path length */
    dwLength = GetProcessExecutablePathById(dwProcessId, NULL, 0);
    if (dwLength == 0)
        return;

    /* Allocate and retrieve the image path */
    pszExePath = HeapAlloc(GetProcessHeap(), 0, dwLength * sizeof(WCHAR));
    if (!pszExePath)
        return;

    if (GetProcessExecutablePathById(dwProcessId, pszExePath, dwLength) == 0)
        goto Cleanup;

    /* Call the shell to display the file properties */
    info.cbSize = sizeof(SHELLEXECUTEINFOW);
    info.fMask = SEE_MASK_INVOKEIDLIST;
    info.hwnd = NULL;
    info.lpVerb = L"properties";
    info.lpFile = pszExePath;
    info.lpParameters = L"";
    info.lpDirectory = NULL;
    info.nShow = SW_SHOW;
    info.hInstApp = NULL;

    ShellExecuteExW(&info);

Cleanup:
    HeapFree(GetProcessHeap(), 0, pszExePath);
}

void ProcessPage_OnOpenFileLocation(void)
{
    DWORD dwProcessId;
    DWORD dwLength;
    LPWSTR pszExePath;
    LPWSTR pszCmdLine = NULL;

    dwProcessId = GetSelectedProcessId();

    /* Retrieve the image path length */
    dwLength = GetProcessExecutablePathById(dwProcessId, NULL, 0);
    if (dwLength == 0)
        return;

    /* Allocate and retrieve the image path */
    pszExePath = HeapAlloc(GetProcessHeap(), 0, dwLength * sizeof(WCHAR));
    if (!pszExePath)
        return;

    if (GetProcessExecutablePathById(dwProcessId, pszExePath, dwLength) == 0)
        goto Cleanup;

    /* Build the shell command line */
    pszCmdLine = HeapAlloc(GetProcessHeap(), 0, (dwLength + 10) * sizeof(WCHAR));
    if (!pszCmdLine)
        goto Cleanup;

    StringCchPrintfW(pszCmdLine, dwLength + 10, L"/select,\"%s\"", pszExePath);

    /* Call the shell to open the file location and select it */
    ShellExecuteW(NULL, L"open", L"explorer.exe", pszCmdLine, NULL, SW_SHOWNORMAL);

Cleanup:
    HeapFree(GetProcessHeap(), 0, pszCmdLine);
    HeapFree(GetProcessHeap(), 0, pszExePath);
}
