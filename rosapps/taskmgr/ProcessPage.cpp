/*
 *  ReactOS Task Manager
 *
 *  processpage.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
	
#include "TaskMgr.h"
#include "ProcessPage.h"
#include "perfdata.h"
#include "column.h"
#include "proclist.h"
#include <ctype.h>

HWND hProcessPage;						// Process List Property Page

HWND hProcessPageListCtrl;				// Process ListCtrl Window
HWND hProcessPageHeaderCtrl;			// Process Header Control
HWND hProcessPageEndProcessButton;		// Process End Process button
HWND hProcessPageShowAllProcessesButton;// Process Show All Processes checkbox

static int	nProcessPageWidth;
static int	nProcessPageHeight;

static HANDLE	hProcessPageEvent = NULL;	// When this event becomes signaled then we refresh the process list

void ProcessPageOnNotify(WPARAM wParam, LPARAM lParam);
void CommaSeparateNumberString(LPTSTR strNumber, int nMaxCount);
void ProcessPageShowContextMenu(DWORD dwProcessId);
void ProcessPageRefreshThread(void *lpParameter);

LRESULT CALLBACK ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT	rc;
	int		nXDifference;
	int		nYDifference;

	switch (message)
	{
	case WM_INITDIALOG:

		//
		// Save the width and height
		//
		GetClientRect(hDlg, &rc);
		nProcessPageWidth = rc.right;
		nProcessPageHeight = rc.bottom;

		// Update window position
		SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

		//
		// Get handles to the controls
		//
		hProcessPageListCtrl = GetDlgItem(hDlg, IDC_PROCESSLIST);
		hProcessPageHeaderCtrl = ListView_GetHeader(hProcessPageListCtrl);
		hProcessPageEndProcessButton = GetDlgItem(hDlg, IDC_ENDPROCESS);
		hProcessPageShowAllProcessesButton = GetDlgItem(hDlg, IDC_SHOWALLPROCESSES);

		//
		// Set the font, title, and extended window styles for the list control
		//
		SendMessage(hProcessPageListCtrl, WM_SETFONT, SendMessage(hProcessPage, WM_GETFONT, 0, 0), TRUE);
		SetWindowText(hProcessPageListCtrl, "Processes");
		ListView_SetExtendedListViewStyle(hProcessPageListCtrl, ListView_GetExtendedListViewStyle(hProcessPageListCtrl) | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

		AddColumns();

		//
		// Subclass the process list control so we can intercept WM_ERASEBKGND
		//
		OldProcessListWndProc = SetWindowLong(hProcessPageListCtrl, GWL_WNDPROC, (LONG)ProcessListWndProc);

		// Start our refresh thread
		_beginthread(ProcessPageRefreshThread, 0, NULL);

		return TRUE;

	case WM_DESTROY:
		// Close the event handle, this will make the
		// refresh thread exit when the wait fails
		CloseHandle(hProcessPageEvent);

		SaveColumnSettings();

		break;

	case WM_COMMAND:
		break;

	case WM_SIZE:
		int		cx, cy;

		if (wParam == SIZE_MINIMIZED)
			return 0;

		cx = LOWORD(lParam);
		cy = HIWORD(lParam);
		nXDifference = cx - nProcessPageWidth;
		nYDifference = cy - nProcessPageHeight;
		nProcessPageWidth = cx;
		nProcessPageHeight = cy;

		// Reposition the application page's controls
		GetWindowRect(hProcessPageListCtrl, &rc);
		cx = (rc.right - rc.left) + nXDifference;
		cy = (rc.bottom - rc.top) + nYDifference;
		SetWindowPos(hProcessPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
		InvalidateRect(hProcessPageListCtrl, NULL, TRUE);
		
		GetClientRect(hProcessPageEndProcessButton, &rc);
		MapWindowPoints(hProcessPageEndProcessButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
   		cx = rc.left + nXDifference;
		cy = rc.top + nYDifference;
		SetWindowPos(hProcessPageEndProcessButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
		InvalidateRect(hProcessPageEndProcessButton, NULL, TRUE);
		
		GetClientRect(hProcessPageShowAllProcessesButton, &rc);
		MapWindowPoints(hProcessPageShowAllProcessesButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
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
	int				idctrl;
	LPNMHDR			pnmh;
	LPNMLISTVIEW	pnmv;
	NMLVDISPINFO*	pnmdi;
	LPNMHEADER		pnmhdr;
	LVITEM			lvitem;
	ULONG			Index;
	ULONG			ColumnIndex;
	IO_COUNTERS		iocounters;
	TIME			time;

	idctrl = (int) wParam;
	pnmh = (LPNMHDR) lParam;
	pnmv = (LPNMLISTVIEW) lParam;
	pnmdi = (NMLVDISPINFO*) lParam;
	pnmhdr = (LPNMHEADER) lParam;

	if (pnmh->hwndFrom == hProcessPageListCtrl)
	{
		switch (pnmh->code)
		{
		/*case LVN_ITEMCHANGED:
			ProcessPageUpdate();
			break;*/
			
		case LVN_GETDISPINFO:

			if (!(pnmdi->item.mask & LVIF_TEXT))
				break;
			
			ColumnIndex = pnmdi->item.iSubItem;
			Index = pnmdi->item.iItem;

			if (ColumnDataHints[ColumnIndex] == COLUMN_IMAGENAME)
				PerfDataGetImageName(Index, pnmdi->item.pszText, pnmdi->item.cchTextMax);
			if (ColumnDataHints[ColumnIndex] == COLUMN_PID)
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetProcessId(Index));
			if (ColumnDataHints[ColumnIndex] == COLUMN_USERNAME)
				PerfDataGetUserName(Index, pnmdi->item.pszText, pnmdi->item.cchTextMax);
			if (ColumnDataHints[ColumnIndex] == COLUMN_SESSIONID)
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetSessionId(Index));
			if (ColumnDataHints[ColumnIndex] == COLUMN_CPUUSAGE)
				wsprintf(pnmdi->item.pszText, _T("%02d"), PerfDataGetCPUUsage(Index));
			if (ColumnDataHints[ColumnIndex] == COLUMN_CPUTIME)
			{
				time = PerfDataGetCPUTime(Index);
#ifdef _MSC_VER
				DWORD dwHours = (DWORD)(time.QuadPart / 36000000000L);
				DWORD dwMinutes = (DWORD)((time.QuadPart % 36000000000L) / 600000000L);
				DWORD dwSeconds = (DWORD)(((time.QuadPart % 36000000000L) % 600000000L) / 10000000L);
#else
				DWORD dwHours = (DWORD)(time.QuadPart / 36000000000LL);
				DWORD dwMinutes = (DWORD)((time.QuadPart % 36000000000LL) / 600000000LL);
				DWORD dwSeconds = (DWORD)(((time.QuadPart % 36000000000LL) % 600000000LL) / 10000000LL);
#endif
				wsprintf(pnmdi->item.pszText, _T("%d:%02d:%02d"), dwHours, dwMinutes, dwSeconds);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGE)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetWorkingSetSizeBytes(Index) / 1024);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
				_tcscat(pnmdi->item.pszText, _T(" K"));
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_PEAKMEMORYUSAGE)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetPeakWorkingSetSizeBytes(Index) / 1024);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
				_tcscat(pnmdi->item.pszText, _T(" K"));
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGEDELTA)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetWorkingSetSizeDelta(Index) / 1024);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
				_tcscat(pnmdi->item.pszText, _T(" K"));
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTS)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetPageFaultCount(Index));
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTSDELTA)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetPageFaultCountDelta(Index));
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_VIRTUALMEMORYSIZE)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetVirtualMemorySizeBytes(Index) / 1024);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
				_tcscat(pnmdi->item.pszText, _T(" K"));
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEDPOOL)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetPagedPoolUsagePages(Index) / 1024);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
				_tcscat(pnmdi->item.pszText, _T(" K"));
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_NONPAGEDPOOL)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetNonPagedPoolUsagePages(Index) / 1024);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
				_tcscat(pnmdi->item.pszText, _T(" K"));
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_BASEPRIORITY)
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetBasePriority(Index));
			if (ColumnDataHints[ColumnIndex] == COLUMN_HANDLECOUNT)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetHandleCount(Index));
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_THREADCOUNT)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetThreadCount(Index));
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_USEROBJECTS)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetUSERObjectCount(Index));
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_GDIOBJECTS)
			{
				wsprintf(pnmdi->item.pszText, _T("%d"), PerfDataGetGDIObjectCount(Index));
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADS)
			{
				PerfDataGetIOCounters(Index, &iocounters);
				//wsprintf(pnmdi->item.pszText, _T("%d"), iocounters.ReadOperationCount);
				_ui64toa(iocounters.ReadOperationCount, pnmdi->item.pszText, 10);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITES)
			{
				PerfDataGetIOCounters(Index, &iocounters);
				//wsprintf(pnmdi->item.pszText, _T("%d"), iocounters.WriteOperationCount);
				_ui64toa(iocounters.WriteOperationCount, pnmdi->item.pszText, 10);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHER)
			{
				PerfDataGetIOCounters(Index, &iocounters);
				//wsprintf(pnmdi->item.pszText, _T("%d"), iocounters.OtherOperationCount);
				_ui64toa(iocounters.OtherOperationCount, pnmdi->item.pszText, 10);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADBYTES)
			{
				PerfDataGetIOCounters(Index, &iocounters);
				//wsprintf(pnmdi->item.pszText, _T("%d"), iocounters.ReadTransferCount);
				_ui64toa(iocounters.ReadTransferCount, pnmdi->item.pszText, 10);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITEBYTES)
			{
				PerfDataGetIOCounters(Index, &iocounters);
				//wsprintf(pnmdi->item.pszText, _T("%d"), iocounters.WriteTransferCount);
				_ui64toa(iocounters.WriteTransferCount, pnmdi->item.pszText, 10);
				CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
			}
			if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHERBYTES)
			{
				PerfDataGetIOCounters(Index, &iocounters);
				//wsprintf(pnmdi->item.pszText, _T("%d"), iocounters.OtherTransferCount);
				_ui64toa(iocounters.OtherTransferCount, pnmdi->item.pszText, 10);
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

				ListView_GetItem(hProcessPageListCtrl, &lvitem);

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

			//
			// FIXME: Fix the column sorting
			//
			//ListView_SortItems(hApplicationPageListCtrl, ApplicationPageCompareFunc, NULL);
			//bSortAscending = !bSortAscending;

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

void CommaSeparateNumberString(LPTSTR strNumber, int nMaxCount)
{
	TCHAR	temp[260];
	UINT	i, j, k;

	for (i=0,j=0; i<(_tcslen(strNumber) % 3); i++, j++)
		temp[j] = strNumber[i];

	for (k=0; i<_tcslen(strNumber); i++,j++,k++)
	{
		if ((k % 3 == 0) && (j > 0))
			temp[j++] = _T(',');

		temp[j] = strNumber[i];
	}

	temp[j] = _T('\0');

	_tcsncpy(strNumber, temp, nMaxCount);
}

void ProcessPageShowContextMenu(DWORD dwProcessId)
{
	HMENU		hMenu;
	HMENU		hSubMenu;
	HMENU		hPriorityMenu;
	POINT		pt;
	SYSTEM_INFO	si;
	HANDLE		hProcess;
	DWORD		dwProcessPriorityClass;
	TCHAR		strDebugger[260];
	DWORD		dwDebuggerSize;
	HKEY		hKey;
	UINT		Idx;

	memset(&si, 0, sizeof(SYSTEM_INFO));

	GetCursorPos(&pt);
	GetSystemInfo(&si);

	hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_PROCESS_PAGE_CONTEXT));
	hSubMenu = GetSubMenu(hMenu, 0);
	hPriorityMenu = GetSubMenu(hSubMenu, 4);

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
	dwProcessPriorityClass = GetPriorityClass(hProcess);
	CloseHandle(hProcess);

	if (si.dwNumberOfProcessors < 2)
		RemoveMenu(hSubMenu, ID_PROCESS_PAGE_SETAFFINITY, MF_BYCOMMAND);

	switch (dwProcessPriorityClass)
	{
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

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		dwDebuggerSize = 260;
		if (RegQueryValueEx(hKey, "Debugger", NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) == ERROR_SUCCESS)
		{
			for (Idx=0; Idx<strlen(strDebugger); Idx++)
				strDebugger[Idx] = toupper(strDebugger[Idx]);

			if (strstr(strDebugger, "DRWTSN32"))
				EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		}
		else
			EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

		RegCloseKey(hKey);
	}
	else
		EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

	TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);

	DestroyMenu(hMenu);
}

void RefreshProcessPage(void)
{
	// Signal the event so that our refresh thread
	// will wake up and refresh the process page
	SetEvent(hProcessPageEvent);
}

void ProcessPageRefreshThread(void *lpParameter)
{
	ULONG	OldProcessorUsage = 0;
	ULONG	OldProcessCount = 0;

	// Create the event
	hProcessPageEvent = CreateEvent(NULL, TRUE, TRUE, "Process Page Event");

	// If we couldn't create the event then exit the thread
	if (!hProcessPageEvent)
		return;

	while (1)
	{
		DWORD	dwWaitVal;

		// Wait on the event
		dwWaitVal = WaitForSingleObject(hProcessPageEvent, INFINITE);

		// If the wait failed then the event object must have been
		// closed and the task manager is exiting so exit this thread
		if (dwWaitVal == WAIT_FAILED)
			return;

		if (dwWaitVal == WAIT_OBJECT_0)
		{
			// Reset our event
			ResetEvent(hProcessPageEvent);

			if ((ULONG)SendMessage(hProcessPageListCtrl, LVM_GETITEMCOUNT, 0, 0) != PerfDataGetProcessCount())
				SendMessage(hProcessPageListCtrl, LVM_SETITEMCOUNT, PerfDataGetProcessCount(), /*LVSICF_NOINVALIDATEALL|*/LVSICF_NOSCROLL);

			if (IsWindowVisible(hProcessPage))
				InvalidateRect(hProcessPageListCtrl, NULL, FALSE);

			TCHAR	text[260];

			if (OldProcessorUsage != PerfDataGetProcessorUsage())
			{
				OldProcessorUsage = PerfDataGetProcessorUsage();
				wsprintf(text, _T("CPU Usage: %3d%%"), OldProcessorUsage);
				SendMessage(hStatusWnd, SB_SETTEXT, 1, (LPARAM)text);
			}
			if (OldProcessCount != PerfDataGetProcessCount())
			{
				OldProcessCount = PerfDataGetProcessCount();
				wsprintf(text, _T("Processes: %d"), OldProcessCount);
				SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text);
			}
		}
	}
}
