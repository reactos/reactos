/*
 *  ReactOS Task Manager
 *
 *  performancepage.cpp
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
	
#include "stdafx.h"
#include "TASKMGR.h"
#include "PerformancePage.h"
#include "perfdata.h"
#include "graph.h"

HWND		hPerformancePage;								// Performance Property Page

HWND		hPerformancePageCpuUsageGraph;					// CPU Usage Graph
HWND		hPerformancePageMemUsageGraph;					// MEM Usage Graph
HWND		hPerformancePageMemUsageHistoryGraph;			// Memory Usage History Graph

HWND		hPerformancePageTotalsFrame;					// Totals Frame
HWND		hPerformancePageCommitChargeFrame;				// Commit Charge Frame
HWND		hPerformancePageKernelMemoryFrame;				// Kernel Memory Frame
HWND		hPerformancePagePhysicalMemoryFrame;			// Physical Memory Frame

HWND		hPerformancePageCommitChargeTotalEdit;			// Commit Charge Total Edit Control
HWND		hPerformancePageCommitChargeLimitEdit;			// Commit Charge Limit Edit Control
HWND		hPerformancePageCommitChargePeakEdit;			// Commit Charge Peak Edit Control

HWND		hPerformancePageKernelMemoryTotalEdit;			// Kernel Memory Total Edit Control
HWND		hPerformancePageKernelMemoryPagedEdit;			// Kernel Memory Paged Edit Control
HWND		hPerformancePageKernelMemoryNonPagedEdit;		// Kernel Memory NonPaged Edit Control

HWND		hPerformancePagePhysicalMemoryTotalEdit;		// Physical Memory Total Edit Control
HWND		hPerformancePagePhysicalMemoryAvailableEdit;	// Physical Memory Available Edit Control
HWND		hPerformancePagePhysicalMemorySystemCacheEdit;	// Physical Memory System Cache Edit Control

HWND		hPerformancePageTotalsHandleCountEdit;			// Total Handles Edit Control
HWND		hPerformancePageTotalsProcessCountEdit;			// Total Processes Edit Control
HWND		hPerformancePageTotalsThreadCountEdit;			// Total Threads Edit Control

static int	nPerformancePageWidth;
static int	nPerformancePageHeight;

static HANDLE	hPerformancePageEvent = NULL;	// When this event becomes signaled then we refresh the performance page

void		PerformancePageRefreshThread(void *lpParameter);

LRESULT CALLBACK PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT	rc;
	int		nXDifference;
	int		nYDifference;

	switch (message)
	{
	case WM_INITDIALOG:
		
		// Save the width and height
		GetClientRect(hDlg, &rc);
		nPerformancePageWidth = rc.right;
		nPerformancePageHeight = rc.bottom;

		// Update window position
		SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

		//
		// Get handles to all the controls
		//
		hPerformancePageTotalsFrame = GetDlgItem(hDlg, IDC_TOTALS_FRAME);
		hPerformancePageCommitChargeFrame = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_FRAME);
		hPerformancePageKernelMemoryFrame = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_FRAME);
		hPerformancePagePhysicalMemoryFrame = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_FRAME);
		hPerformancePageCommitChargeTotalEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_TOTAL);
		hPerformancePageCommitChargeLimitEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_LIMIT);
		hPerformancePageCommitChargePeakEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_PEAK);
		hPerformancePageKernelMemoryTotalEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_TOTAL);
		hPerformancePageKernelMemoryPagedEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_PAGED);
		hPerformancePageKernelMemoryNonPagedEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_NONPAGED);
		hPerformancePagePhysicalMemoryTotalEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_TOTAL);
		hPerformancePagePhysicalMemoryAvailableEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_AVAILABLE);
		hPerformancePagePhysicalMemorySystemCacheEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_SYSTEM_CACHE);
		hPerformancePageTotalsHandleCountEdit = GetDlgItem(hDlg, IDC_TOTALS_HANDLE_COUNT);
		hPerformancePageTotalsProcessCountEdit = GetDlgItem(hDlg, IDC_TOTALS_PROCESS_COUNT);
		hPerformancePageTotalsThreadCountEdit = GetDlgItem(hDlg, IDC_TOTALS_THREAD_COUNT);
		hPerformancePageCpuUsageGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_GRAPH);
		hPerformancePageMemUsageGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_GRAPH);
		hPerformancePageMemUsageHistoryGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_HISTORY_GRAPH);
		
		// Start our refresh thread
		_beginthread(PerformancePageRefreshThread, 0, NULL);

		//
		// Subclass graph buttons
		//
		OldGraphWndProc = SetWindowLong(hPerformancePageCpuUsageGraph, GWL_WNDPROC, (LONG)Graph_WndProc);
		SetWindowLong(hPerformancePageMemUsageGraph, GWL_WNDPROC, (LONG)Graph_WndProc);
		SetWindowLong(hPerformancePageMemUsageHistoryGraph, GWL_WNDPROC, (LONG)Graph_WndProc);
		
		return TRUE;

	case WM_COMMAND:
		break;

	case WM_SIZE:
		int		cx, cy;

		if (wParam == SIZE_MINIMIZED)
			return 0;

		cx = LOWORD(lParam);
		cy = HIWORD(lParam);
		nXDifference = cx - nPerformancePageWidth;
		nYDifference = cy - nPerformancePageHeight;
		nPerformancePageWidth = cx;
		nPerformancePageHeight = cy;

		// Reposition the performance page's controls
		/*GetWindowRect(hApplicationPageListCtrl, &rc);
		cx = (rc.right - rc.left) + nXDifference;
		cy = (rc.bottom - rc.top) + nYDifference;
		SetWindowPos(hApplicationPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
		InvalidateRect(hApplicationPageListCtrl, NULL, TRUE);
		
		GetClientRect(hApplicationPageEndTaskButton, &rc);
		MapWindowPoints(hApplicationPageEndTaskButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
   		cx = rc.left + nXDifference;
		cy = rc.top + nYDifference;
		SetWindowPos(hApplicationPageEndTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
		InvalidateRect(hApplicationPageEndTaskButton, NULL, TRUE);
		
		GetClientRect(hApplicationPageSwitchToButton, &rc);
		MapWindowPoints(hApplicationPageSwitchToButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
   		cx = rc.left + nXDifference;
		cy = rc.top + nYDifference;
		SetWindowPos(hApplicationPageSwitchToButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
		InvalidateRect(hApplicationPageSwitchToButton, NULL, TRUE);
		
		GetClientRect(hApplicationPageNewTaskButton, &rc);
		MapWindowPoints(hApplicationPageNewTaskButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
   		cx = rc.left + nXDifference;
		cy = rc.top + nYDifference;
		SetWindowPos(hApplicationPageNewTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
		InvalidateRect(hApplicationPageNewTaskButton, NULL, TRUE);*/

		break;
	}

    return 0;
}

void RefreshPerformancePage(void)
{
	// Signal the event so that our refresh thread
	// will wake up and refresh the performance page
	SetEvent(hPerformancePageEvent);
}

void PerformancePageRefreshThread(void *lpParameter)
{
	ULONG	CommitChargeTotal;
	ULONG	CommitChargeLimit;
	ULONG	CommitChargePeak;

	ULONG	KernelMemoryTotal;
	ULONG	KernelMemoryPaged;
	ULONG	KernelMemoryNonPaged;

	ULONG	PhysicalMemoryTotal;
	ULONG	PhysicalMemoryAvailable;
	ULONG	PhysicalMemorySystemCache;

	ULONG	TotalHandles;
	ULONG	TotalThreads;
	ULONG	TotalProcesses;

	TCHAR	Text[260];

	// Create the event
	hPerformancePageEvent = CreateEvent(NULL, TRUE, TRUE, "Performance Page Event");

	// If we couldn't create the event then exit the thread
	if (!hPerformancePageEvent)
		return;

	while (1)
	{
		DWORD	dwWaitVal;

		// Wait on the event
		dwWaitVal = WaitForSingleObject(hPerformancePageEvent, INFINITE);

		// If the wait failed then the event object must have been
		// closed and the task manager is exiting so exit this thread
		if (dwWaitVal == WAIT_FAILED)
			return;

		if (dwWaitVal == WAIT_OBJECT_0)
		{
			// Reset our event
			ResetEvent(hPerformancePageEvent);

			//
			// Update the commit charge info
			//
			CommitChargeTotal = PerfDataGetCommitChargeTotalK();
			CommitChargeLimit = PerfDataGetCommitChargeLimitK();
			CommitChargePeak = PerfDataGetCommitChargePeakK();
			_ultot(CommitChargeTotal, Text, 10);
			SetWindowText(hPerformancePageCommitChargeTotalEdit, Text);
			_ultot(CommitChargeLimit, Text, 10);
			SetWindowText(hPerformancePageCommitChargeLimitEdit, Text);
			_ultot(CommitChargePeak, Text, 10);
			SetWindowText(hPerformancePageCommitChargePeakEdit, Text);
			wsprintf(Text, _T("Mem Usage: %dK / %dK"), CommitChargeTotal, CommitChargeLimit);
			SendMessage(hStatusWnd, SB_SETTEXT, 2, (LPARAM)Text);

			//
			// Update the kernel memory info
			//
			KernelMemoryTotal = PerfDataGetKernelMemoryTotalK();
			KernelMemoryPaged = PerfDataGetKernelMemoryPagedK();
			KernelMemoryNonPaged = PerfDataGetKernelMemoryNonPagedK();
			_ultot(KernelMemoryTotal, Text, 10);
			SetWindowText(hPerformancePageKernelMemoryTotalEdit, Text);
			_ultot(KernelMemoryPaged, Text, 10);
			SetWindowText(hPerformancePageKernelMemoryPagedEdit, Text);
			_ultot(KernelMemoryNonPaged, Text, 10);
			SetWindowText(hPerformancePageKernelMemoryNonPagedEdit, Text);

			//
			// Update the physical memory info
			//
			PhysicalMemoryTotal = PerfDataGetPhysicalMemoryTotalK();
			PhysicalMemoryAvailable = PerfDataGetPhysicalMemoryAvailableK();
			PhysicalMemorySystemCache = PerfDataGetPhysicalMemorySystemCacheK();
			_ultot(PhysicalMemoryTotal, Text, 10);
			SetWindowText(hPerformancePagePhysicalMemoryTotalEdit, Text);
			_ultot(PhysicalMemoryAvailable, Text, 10);
			SetWindowText(hPerformancePagePhysicalMemoryAvailableEdit, Text);
			_ultot(PhysicalMemorySystemCache, Text, 10);
			SetWindowText(hPerformancePagePhysicalMemorySystemCacheEdit, Text);

			//
			// Update the totals info
			//
			TotalHandles = PerfDataGetSystemHandleCount();
			TotalThreads = PerfDataGetTotalThreadCount();
			TotalProcesses = PerfDataGetProcessCount();
			_ultot(TotalHandles, Text, 10);
			SetWindowText(hPerformancePageTotalsHandleCountEdit, Text);
			_ultot(TotalThreads, Text, 10);
			SetWindowText(hPerformancePageTotalsThreadCountEdit, Text);
			_ultot(TotalProcesses, Text, 10);
			SetWindowText(hPerformancePageTotalsProcessCountEdit, Text);

			//
			// Redraw the graphs
			//
			InvalidateRect(hPerformancePageCpuUsageGraph, NULL, FALSE);
			InvalidateRect(hPerformancePageMemUsageGraph, NULL, FALSE);
			InvalidateRect(hPerformancePageMemUsageHistoryGraph, NULL, FALSE);
		}
	}
}

void PerformancePage_OnViewShowKernelTimes(void)
{
	HMENU	hMenu;
	HMENU	hViewMenu;

	hMenu = GetMenu(hMainWnd);
	hViewMenu = GetSubMenu(hMenu, 2);

	// Check or uncheck the show 16-bit tasks menu item
	if (GetMenuState(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND) & MF_CHECKED)
	{
		CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);
		TaskManagerSettings.ShowKernelTimes = FALSE;
	}
	else
	{
		CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
		TaskManagerSettings.ShowKernelTimes = TRUE;
	}

	RefreshPerformancePage();
}

void PerformancePage_OnViewCPUHistoryOneGraphAll(void)
{
	HMENU	hMenu;
	HMENU	hViewMenu;
	HMENU	hCPUHistoryMenu;

	hMenu = GetMenu(hMainWnd);
	hViewMenu = GetSubMenu(hMenu, 2);
	hCPUHistoryMenu = GetSubMenu(hViewMenu, 3);

	TaskManagerSettings.CPUHistory_OneGraphPerCPU = FALSE;
	CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);
}

void PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void)
{
	HMENU	hMenu;
	HMENU	hViewMenu;
	HMENU	hCPUHistoryMenu;

	hMenu = GetMenu(hMainWnd);
	hViewMenu = GetSubMenu(hMenu, 2);
	hCPUHistoryMenu = GetSubMenu(hViewMenu, 3);

	TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
	CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
}

