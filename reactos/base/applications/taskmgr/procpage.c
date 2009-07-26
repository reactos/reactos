/*
 *  ReactOS Task Manager
 *
 *  procpage.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *  Copyright (C) 2009         Maxime Vernier <maxime.vernier@gmail.com>
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

#define CMP(x1, x2)\
    (x1 < x2 ? -1 : (x1 > x2 ? 1 : 0))

typedef struct
{
    ULONG Index;
} PROCESS_PAGE_LIST_ITEM, *LPPROCESS_PAGE_LIST_ITEM;

HWND hProcessPage;                        /* Process List Property Page */

HWND hProcessPageListCtrl;                /* Process ListCtrl Window */
HWND hProcessPageHeaderCtrl;            /* Process Header Control */
HWND hProcessPageEndProcessButton;        /* Process End Process button */
HWND hProcessPageShowAllProcessesButton;/* Process Show All Processes checkbox */

static int  nProcessPageWidth;
static int  nProcessPageHeight;
static HANDLE  hProcessPageEvent = NULL;    /* When this event becomes signaled then we refresh the process list */

int CALLBACK    ProcessPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void AddProcess(ULONG Index);
void UpdateProcesses();
void gethmsfromlargeint(LARGE_INTEGER largeint, DWORD *dwHours, DWORD *dwMinutes, DWORD *dwSeconds);
void ProcessPageOnNotify(WPARAM wParam, LPARAM lParam);
void CommaSeparateNumberString(LPWSTR strNumber, int nMaxCount);
void ProcessPageShowContextMenu(DWORD dwProcessId);
BOOL PerfDataGetText(ULONG Index, ULONG ColumnIndex, LPTSTR lpText, int nMaxCount);
DWORD WINAPI ProcessPageRefreshThread(void *lpParameter);

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
            return PerfDataGetProcessId(((LPPROCESS_PAGE_LIST_ITEM)lvitem.lParam)->Index);
    }

    return 0;
}

INT_PTR CALLBACK
ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT    rc;
    int     nXDifference;
    int     nYDifference;
    int     cx, cy;
    HANDLE  hRefreshThread = NULL;

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
        OldProcessListWndProc = (WNDPROC)(LONG_PTR) SetWindowLongPtrW(hProcessPageListCtrl, GWL_WNDPROC, (LONG_PTR)ProcessListWndProc);

        /* Start our refresh thread */
        hRefreshThread = CreateThread(NULL, 0, ProcessPageRefreshThread, NULL, 0, NULL);

        return TRUE;

    case WM_DESTROY:
        /* Close the event handle, this will make the */
        /* refresh thread exit when the wait fails */
        CloseHandle(hProcessPageEvent);
        CloseHandle(hRefreshThread);

        SaveColumnSettings();

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
        MapWindowPoints(hProcessPageEndProcessButton, hDlg, (LPPOINT)(PRECT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
           cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hProcessPageEndProcessButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hProcessPageEndProcessButton, NULL, TRUE);

        GetClientRect(hProcessPageShowAllProcessesButton, &rc);
        MapWindowPoints(hProcessPageShowAllProcessesButton, hDlg, (LPPOINT)(PRECT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
           cx = rc.left;
        cy = rc.top + nYDifference;
        SetWindowPos(hProcessPageShowAllProcessesButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hProcessPageShowAllProcessesButton, NULL, TRUE);

        break;

    case WM_NOTIFY:

        ProcessPageOnNotify(wParam, lParam);
        break;
    }

    return 0;
}

void ProcessPageOnNotify(WPARAM wParam, LPARAM lParam)
{
    int            idctrl;
    LPNMHDR        pnmh;
    LPNMLISTVIEW   pnmv;
    NMLVDISPINFO*  pnmdi;
    LPNMHEADER     pnmhdr;
    ULONG          Index;
    ULONG          ColumnIndex;
    LPPROCESS_PAGE_LIST_ITEM  pData;

    idctrl = (int) wParam;
    pnmh = (LPNMHDR) lParam;
    pnmv = (LPNMLISTVIEW) lParam;
    pnmdi = (NMLVDISPINFO*) lParam;
    pnmhdr = (LPNMHEADER) lParam;

    if (pnmh->hwndFrom == hProcessPageListCtrl)
    {
        switch (pnmh->code)
        {
        #if 0
        case LVN_ITEMCHANGED:
            ProcessPageUpdate();
            break;
        #endif

        case LVN_GETDISPINFO:

            if (!(pnmdi->item.mask & LVIF_TEXT))
                break;

            pData = (LPPROCESS_PAGE_LIST_ITEM)pnmdi->item.lParam;
            Index = pData->Index;
            ColumnIndex = pnmdi->item.iSubItem;

            PerfDataGetText(Index, ColumnIndex, pnmdi->item.pszText, pnmdi->item.cchTextMax);

            break;

        case NM_RCLICK:

            ProcessPageShowContextMenu(GetSelectedProcessId());
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

void CommaSeparateNumberString(LPWSTR strNumber, int nMaxCount)
{
    WCHAR  temp[260];
    UINT   i, j, k;

    for (i=0,j=0; i<(wcslen(strNumber) % 3); i++, j++)
        temp[j] = strNumber[i];
    for (k=0; i<wcslen(strNumber); i++,j++,k++) {
        if ((k % 3 == 0) && (j > 0))
            temp[j++] = L',';
        temp[j] = strNumber[i];
    }
    temp[j] = L'\0';
    wcsncpy(strNumber, temp, nMaxCount);
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

    if (!DebugChannelsAreSupported())
        RemoveMenu(hSubMenu, ID_PROCESS_PAGE_DEBUGCHANNELS, MF_BYCOMMAND);

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
        dwDebuggerSize = 260;
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
    /* Signal the event so that our refresh thread */
    /* will wake up and refresh the process page */
    SetEvent(hProcessPageEvent);
}

DWORD WINAPI ProcessPageRefreshThread(void *lpParameter)
{
    ULONG    OldProcessorUsage = 0;
    ULONG    OldProcessCount = 0;
    WCHAR    szCpuUsage[256], szProcesses[256];

    /* Create the event */
    hProcessPageEvent = CreateEventW(NULL, TRUE, TRUE, NULL);

    /* If we couldn't create the event then exit the thread */
    if (!hProcessPageEvent)
        return 0;

    LoadStringW(hInst, IDS_STATUS_CPUUSAGE, szCpuUsage, 256);
    LoadStringW(hInst, IDS_STATUS_PROCESSES, szProcesses, 256);

    while (1) {
        DWORD    dwWaitVal;

        /* Wait on the event */
        dwWaitVal = WaitForSingleObject(hProcessPageEvent, INFINITE);

        /* If the wait failed then the event object must have been */
        /* closed and the task manager is exiting so exit this thread */
        if (dwWaitVal == WAIT_FAILED)
            return 0;

        if (dwWaitVal == WAIT_OBJECT_0) {
            WCHAR    text[260];

            /* Reset our event */
            ResetEvent(hProcessPageEvent);

            UpdateProcesses();

            if (IsWindowVisible(hProcessPage))
                InvalidateRect(hProcessPageListCtrl, NULL, FALSE);

            if (OldProcessorUsage != PerfDataGetProcessorUsage()) {
                OldProcessorUsage = PerfDataGetProcessorUsage();
                wsprintfW(text, szCpuUsage, OldProcessorUsage);
                SendMessageW(hStatusWnd, SB_SETTEXT, 1, (LPARAM)text);
            }
            if (OldProcessCount != PerfDataGetProcessCount()) {
                OldProcessCount = PerfDataGetProcessCount();
                wsprintfW(text, szProcesses, OldProcessCount);
                SendMessageW(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text);
            }
        }
    }
    return 0;
}

void UpdateProcesses()
{
    int i;
    BOOL found = FALSE;
    ULONG l;
    ULONG pid;
    LV_ITEM    item;
    LPPROCESS_PAGE_LIST_ITEM pData;
    PPERFDATA  pPerfData;

    /* Remove old processes */
    for (i = 0; i < ListView_GetItemCount(hProcessPageListCtrl); i++)
    {
        memset(&item, 0, sizeof (LV_ITEM));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hProcessPageListCtrl, &item);
        pData = (LPPROCESS_PAGE_LIST_ITEM)item.lParam;
        (void)PerfDataGet(pData->Index, &pPerfData);
        pid = PerfDataGetProcessId(pData->Index);
        for (l = 0; l < PerfDataGetProcessCount(); l++)
        {
            if (PerfDataGetProcessId(l) == pid)
            {
                found = TRUE;
                break;
            }
        }
        if (!found)
        {
            (void)ListView_DeleteItem(hApplicationPageListCtrl, i);
            HeapFree(GetProcessHeap(), 0, pData);
        }
    }
    for (l = 0; l < PerfDataGetProcessCount(); l++)
    {
        AddProcess(l);
    }
    if (TaskManagerSettings.SortColumn != -1)
    {
        (void)ListView_SortItems(hProcessPageListCtrl, ProcessPageCompareFunc, NULL);
    }
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
        if (PerfDataGetProcessId(pData->Index) == pid)
        {
            bAlreadyInList = TRUE;
            break;
        }
    }
    if (!bAlreadyInList)  /* Add */
    {
        pData = (LPPROCESS_PAGE_LIST_ITEM)HeapAlloc(GetProcessHeap(), 0, sizeof(PROCESS_PAGE_LIST_ITEM));
        pData->Index = Index;

        /* Add the item to the list */
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_TEXT|LVIF_PARAM;
        item.pszText = LPSTR_TEXTCALLBACK;
        item.iItem = ListView_GetItemCount(hProcessPageListCtrl);
        item.lParam = (LPARAM)pData;
        (void)ListView_InsertItem(hProcessPageListCtrl, &item);
    }
}

BOOL PerfDataGetText(ULONG Index, ULONG ColumnIndex, LPTSTR lpText, int nMaxCount)
{
    IO_COUNTERS    iocounters;
    LARGE_INTEGER  time;

    if (ColumnDataHints[ColumnIndex] == COLUMN_IMAGENAME)
        PerfDataGetImageName(Index, lpText, nMaxCount);
    if (ColumnDataHints[ColumnIndex] == COLUMN_PID)
        wsprintfW(lpText, L"%d", PerfDataGetProcessId(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_USERNAME)
        PerfDataGetUserName(Index, lpText, nMaxCount);
    if (ColumnDataHints[ColumnIndex] == COLUMN_SESSIONID)
        wsprintfW(lpText, L"%d", PerfDataGetSessionId(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_CPUUSAGE)
        wsprintfW(lpText, L"%02d", PerfDataGetCPUUsage(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_CPUTIME)
    {
        DWORD dwHours;
        DWORD dwMinutes;
        DWORD dwSeconds;

        time = PerfDataGetCPUTime(Index);
        gethmsfromlargeint(time, &dwHours, &dwMinutes, &dwSeconds);
        wsprintfW(lpText, L"%d:%02d:%02d", dwHours, dwMinutes, dwSeconds);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGE)
    {
        wsprintfW(lpText, L"%d", PerfDataGetWorkingSetSizeBytes(Index) / 1024);
        CommaSeparateNumberString(lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PEAKMEMORYUSAGE)
    {
        wsprintfW(lpText, L"%d", PerfDataGetPeakWorkingSetSizeBytes(Index) / 1024);
        CommaSeparateNumberString(lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGEDELTA)
    {
        wsprintfW(lpText, L"%d", PerfDataGetWorkingSetSizeDelta(Index) / 1024);
        CommaSeparateNumberString(lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTS)
    {
        wsprintfW(lpText, L"%d", PerfDataGetPageFaultCount(Index));
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTSDELTA)
    {
        wsprintfW(lpText, L"%d", PerfDataGetPageFaultCountDelta(Index));
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_VIRTUALMEMORYSIZE)
    {
        wsprintfW(lpText, L"%d", PerfDataGetVirtualMemorySizeBytes(Index) / 1024);
        CommaSeparateNumberString(lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEDPOOL)
    {
        wsprintfW(lpText, L"%d", PerfDataGetPagedPoolUsagePages(Index) / 1024);
        CommaSeparateNumberString(lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_NONPAGEDPOOL)
    {
        wsprintfW(lpText, L"%d", PerfDataGetNonPagedPoolUsagePages(Index) / 1024);
        CommaSeparateNumberString(lpText, nMaxCount);
        wcscat(lpText, L" K");
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_BASEPRIORITY)
        wsprintfW(lpText, L"%d", PerfDataGetBasePriority(Index));
    if (ColumnDataHints[ColumnIndex] == COLUMN_HANDLECOUNT)
    {
        wsprintfW(lpText, L"%d", PerfDataGetHandleCount(Index));
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_THREADCOUNT)
    {
        wsprintfW(lpText, L"%d", PerfDataGetThreadCount(Index));
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_USEROBJECTS)
    {
        wsprintfW(lpText, L"%d", PerfDataGetUSERObjectCount(Index));
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_GDIOBJECTS)
    {
        wsprintfW(lpText, L"%d", PerfDataGetGDIObjectCount(Index));
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADS)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.ReadOperationCount); */
        _ui64tow(iocounters.ReadOperationCount, lpText, 10);
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.WriteOperationCount); */
        _ui64tow(iocounters.WriteOperationCount, lpText, 10);
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHER)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.OtherOperationCount); */
        _ui64tow(iocounters.OtherOperationCount, lpText, 10);
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADBYTES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.ReadTransferCount); */
        _ui64tow(iocounters.ReadTransferCount, lpText, 10);
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITEBYTES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.WriteTransferCount); */
        _ui64tow(iocounters.WriteTransferCount, lpText, 10);
        CommaSeparateNumberString(lpText, nMaxCount);
    }
    if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHERBYTES)
    {
        PerfDataGetIOCounters(Index, &iocounters);
        /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.OtherTransferCount); */
        _ui64tow(iocounters.OtherTransferCount, lpText, 10);
        CommaSeparateNumberString(lpText, nMaxCount);
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

    if (TaskManagerSettings.SortColumn == COLUMN_IMAGENAME)
    {
        PerfDataGetImageName(Param1->Index, text1, sizeof (text1) / sizeof (*text1));
        PerfDataGetImageName(Param2->Index, text2, sizeof (text2) / sizeof (*text2));
        ret = _wcsicmp(text1, text2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PID)
    {
        l1 = PerfDataGetProcessId(Param1->Index);
        l2 = PerfDataGetProcessId(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_USERNAME)
    {
        PerfDataGetUserName(Param1->Index, text1, sizeof (text1) / sizeof (*text1));
        PerfDataGetUserName(Param2->Index, text2, sizeof (text2) / sizeof (*text2));
        ret = _wcsicmp(text1, text2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_SESSIONID)
    {
        l1 = PerfDataGetSessionId(Param1->Index);
        l2 = PerfDataGetSessionId(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_CPUUSAGE)
    {
        l1 = PerfDataGetCPUUsage(Param1->Index);
        l2 = PerfDataGetCPUUsage(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_CPUTIME)
    {
        time1 = PerfDataGetCPUTime(Param1->Index);
        time2 = PerfDataGetCPUTime(Param2->Index);
        ret = largeintcmp(time1, time2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_MEMORYUSAGE)
    {
        l1 = PerfDataGetWorkingSetSizeBytes(Param1->Index);
        l2 = PerfDataGetWorkingSetSizeBytes(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PEAKMEMORYUSAGE)
    {
        l1 = PerfDataGetPeakWorkingSetSizeBytes(Param1->Index);
        l2 = PerfDataGetPeakWorkingSetSizeBytes(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_MEMORYUSAGEDELTA)
    {
        l1 = PerfDataGetWorkingSetSizeDelta(Param1->Index);
        l2 = PerfDataGetWorkingSetSizeDelta(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PAGEFAULTS)
    {
        l1 = PerfDataGetPageFaultCount(Param1->Index);
        l2 = PerfDataGetPageFaultCount(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PAGEFAULTSDELTA)
    {
        l1 = PerfDataGetPageFaultCountDelta(Param1->Index);
        l2 = PerfDataGetPageFaultCountDelta(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_VIRTUALMEMORYSIZE)
    {
        l1 = PerfDataGetVirtualMemorySizeBytes(Param1->Index);
        l2 = PerfDataGetVirtualMemorySizeBytes(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_PAGEDPOOL)
    {
        l1 = PerfDataGetPagedPoolUsagePages(Param1->Index);
        l2 = PerfDataGetPagedPoolUsagePages(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_NONPAGEDPOOL)
    {
        l1 = PerfDataGetNonPagedPoolUsagePages(Param1->Index);
        l2 = PerfDataGetNonPagedPoolUsagePages(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_BASEPRIORITY)
    {
        l1 = PerfDataGetBasePriority(Param1->Index);
        l2 = PerfDataGetBasePriority(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_HANDLECOUNT)
    {
        l1 = PerfDataGetHandleCount(Param1->Index);
        l2 = PerfDataGetHandleCount(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_THREADCOUNT)
    {
        l1 = PerfDataGetThreadCount(Param1->Index);
        l2 = PerfDataGetThreadCount(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_USEROBJECTS)
    {
        l1 = PerfDataGetUSERObjectCount(Param1->Index);
        l2 = PerfDataGetUSERObjectCount(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_GDIOBJECTS)
    {
        l1 = PerfDataGetGDIObjectCount(Param1->Index);
        l2 = PerfDataGetGDIObjectCount(Param2->Index);
        ret = CMP(l1, l2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOREADS)
    {
        PerfDataGetIOCounters(Param1->Index, &iocounters1);
        PerfDataGetIOCounters(Param2->Index, &iocounters2);
        ull1 = iocounters1.ReadOperationCount;
        ull2 = iocounters2.ReadOperationCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOWRITES)
    {
        PerfDataGetIOCounters(Param1->Index, &iocounters1);
        PerfDataGetIOCounters(Param2->Index, &iocounters2);
        ull1 = iocounters1.WriteOperationCount;
        ull2 = iocounters2.WriteOperationCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOOTHER)
    {
        PerfDataGetIOCounters(Param1->Index, &iocounters1);
        PerfDataGetIOCounters(Param2->Index, &iocounters2);
        ull1 = iocounters1.OtherOperationCount;
        ull2 = iocounters2.OtherOperationCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOREADBYTES)
    {
        PerfDataGetIOCounters(Param1->Index, &iocounters1);
        PerfDataGetIOCounters(Param2->Index, &iocounters2);
        ull1 = iocounters1.ReadTransferCount;
        ull2 = iocounters2.ReadTransferCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOWRITEBYTES)
    {
        PerfDataGetIOCounters(Param1->Index, &iocounters1);
        PerfDataGetIOCounters(Param2->Index, &iocounters2);
        ull1 = iocounters1.WriteTransferCount;
        ull2 = iocounters2.WriteTransferCount;
        ret = CMP(ull1, ull2);
    }
    else if (TaskManagerSettings.SortColumn == COLUMN_IOOTHERBYTES)
    {
        PerfDataGetIOCounters(Param1->Index, &iocounters1);
        PerfDataGetIOCounters(Param2->Index, &iocounters2);
        ull1 = iocounters1.OtherTransferCount;
        ull2 = iocounters2.OtherTransferCount;
        ret = CMP(ull1, ull2);
    }
    return ret;
}
