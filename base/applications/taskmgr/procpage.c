/*
 *  ReactOS Task Manager
 *
 *  procpage.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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

HWND hProcessPage;                        /* Process List Property Page */

HWND hProcessPageListCtrl;                /* Process ListCtrl Window */
HWND hProcessPageHeaderCtrl;            /* Process Header Control */
HWND hProcessPageEndProcessButton;        /* Process End Process button */
HWND hProcessPageShowAllProcessesButton;/* Process Show All Processes checkbox */

static int  nProcessPageWidth;
static int  nProcessPageHeight;

static HANDLE  hProcessPageEvent = NULL;    /* When this event becomes signaled then we refresh the process list */

void ProcessPageOnNotify(WPARAM wParam, LPARAM lParam);
void CommaSeparateNumberString(LPWSTR strNumber, int nMaxCount);
void ProcessPageShowContextMenu(DWORD dwProcessId);
DWORD WINAPI ProcessPageRefreshThread(void *lpParameter);

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
         * Set the font, title, and extended window styles for the list control
         */
        SendMessageW(hProcessPageListCtrl, WM_SETFONT, SendMessageW(hProcessPage, WM_GETFONT, 0, 0), TRUE);
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
    LVITEM         lvitem;
    ULONG          Index;
    ULONG          ColumnIndex;
    IO_COUNTERS    iocounters;
    LARGE_INTEGER  time;

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

            ColumnIndex = pnmdi->item.iSubItem;
            Index = pnmdi->item.iItem;

            if (ColumnDataHints[ColumnIndex] == COLUMN_IMAGENAME)
                PerfDataGetImageName(Index, pnmdi->item.pszText, pnmdi->item.cchTextMax);
            if (ColumnDataHints[ColumnIndex] == COLUMN_PID)
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetProcessId(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_USERNAME)
                PerfDataGetUserName(Index, pnmdi->item.pszText, pnmdi->item.cchTextMax);
            if (ColumnDataHints[ColumnIndex] == COLUMN_SESSIONID)
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetSessionId(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_CPUUSAGE)
                wsprintfW(pnmdi->item.pszText, L"%02d", PerfDataGetCPUUsage(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_CPUTIME)
            {
                DWORD dwHours;
                DWORD dwMinutes;
                DWORD dwSeconds;

                time = PerfDataGetCPUTime(Index);
#ifdef _MSC_VER
                dwHours = (DWORD)(time.QuadPart / 36000000000L);
                dwMinutes = (DWORD)((time.QuadPart % 36000000000L) / 600000000L);
                dwSeconds = (DWORD)(((time.QuadPart % 36000000000L) % 600000000L) / 10000000L);
#else
                dwHours = (DWORD)(time.QuadPart / 36000000000LL);
                dwMinutes = (DWORD)((time.QuadPart % 36000000000LL) / 600000000LL);
                dwSeconds = (DWORD)(((time.QuadPart % 36000000000LL) % 600000000LL) / 10000000LL);
#endif
                wsprintfW(pnmdi->item.pszText, L"%d:%02d:%02d", dwHours, dwMinutes, dwSeconds);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGE)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetWorkingSetSizeBytes(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, L" K");
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PEAKMEMORYUSAGE)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetPeakWorkingSetSizeBytes(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, L" K");
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGEDELTA)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetWorkingSetSizeDelta(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, L" K");
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTS)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetPageFaultCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTSDELTA)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetPageFaultCountDelta(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_VIRTUALMEMORYSIZE)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetVirtualMemorySizeBytes(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, L" K");
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEDPOOL)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetPagedPoolUsagePages(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, L" K");
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_NONPAGEDPOOL)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetNonPagedPoolUsagePages(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, L" K");
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_BASEPRIORITY)
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetBasePriority(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_HANDLECOUNT)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetHandleCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_THREADCOUNT)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetThreadCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_USEROBJECTS)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetUSERObjectCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_GDIOBJECTS)
            {
                wsprintfW(pnmdi->item.pszText, L"%d", PerfDataGetGDIObjectCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADS)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.ReadOperationCount); */
                _ui64tow(iocounters.ReadOperationCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.WriteOperationCount); */
                _ui64tow(iocounters.WriteOperationCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHER)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.OtherOperationCount); */
                _ui64tow(iocounters.OtherOperationCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADBYTES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.ReadTransferCount); */
                _ui64tow(iocounters.ReadTransferCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITEBYTES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.WriteTransferCount); */
                _ui64tow(iocounters.WriteTransferCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHERBYTES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, L"%d", iocounters.OtherTransferCount); */
                _ui64tow(iocounters.OtherTransferCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }

            break;

        case NM_RCLICK:

            for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
            {
                memset(&lvitem, 0, sizeof(LVITEM));

                lvitem.mask = LVIF_STATE;
                lvitem.stateMask = LVIS_SELECTED;
                lvitem.iItem = Index;

                (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

                if (lvitem.state & LVIS_SELECTED)
                    break;
            }

            if ((ListView_GetSelectedCount(hProcessPageListCtrl) == 1) &&
                (PerfDataGetProcessId(Index) != 0))
            {
                ProcessPageShowContextMenu(PerfDataGetProcessId(Index));
            }

            break;

        }
    }
    else if (pnmh->hwndFrom == hProcessPageHeaderCtrl)
    {
        switch (pnmh->code)
        {
        case HDN_ITEMCLICK:

            /*
             * FIXME: Fix the column sorting
             *
             *ListView_SortItems(hApplicationPageListCtrl, ApplicationPageCompareFunc, NULL);
             *bSortAscending = !bSortAscending;
             */

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

            if ((ULONG)SendMessageW(hProcessPageListCtrl, LVM_GETITEMCOUNT, 0, 0) != PerfDataGetProcessCount())
                SendMessageW(hProcessPageListCtrl, LVM_SETITEMCOUNT, PerfDataGetProcessCount(), /*LVSICF_NOINVALIDATEALL|*/LVSICF_NOSCROLL);

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
