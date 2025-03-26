/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Page
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"
#include <shlwapi.h>

TM_GRAPH_CONTROL PerformancePageCpuUsageHistoryGraph;
TM_GRAPH_CONTROL PerformancePageMemUsageHistoryGraph;

HWND hPerformancePage;                /* Performance Property Page */
static HWND hCpuUsageGraph;                  /* CPU Usage Graph */
static HWND hMemUsageGraph;                  /* MEM Usage Graph */
HWND hPerformancePageCpuUsageHistoryGraph;           /* CPU Usage History Graph */
HWND hPerformancePageMemUsageHistoryGraph;           /* Memory Usage History Graph */
static HWND hTotalsFrame;                    /* Totals Frame */
static HWND hCommitChargeFrame;              /* Commit Charge Frame */
static HWND hKernelMemoryFrame;              /* Kernel Memory Frame */
static HWND hPhysicalMemoryFrame;            /* Physical Memory Frame */
static HWND hCpuUsageFrame;
static HWND hMemUsageFrame;
static HWND hCpuUsageHistoryFrame;
static HWND hMemUsageHistoryFrame;
static HWND hCommitChargeTotalEdit;          /* Commit Charge Total Edit Control */
static HWND hCommitChargeLimitEdit;          /* Commit Charge Limit Edit Control */
static HWND hCommitChargePeakEdit;           /* Commit Charge Peak Edit Control */
static HWND hKernelMemoryTotalEdit;          /* Kernel Memory Total Edit Control */
static HWND hKernelMemoryPagedEdit;          /* Kernel Memory Paged Edit Control */
static HWND hKernelMemoryNonPagedEdit;       /* Kernel Memory NonPaged Edit Control */
static HWND hPhysicalMemoryTotalEdit;        /* Physical Memory Total Edit Control */
static HWND hPhysicalMemoryAvailableEdit;    /* Physical Memory Available Edit Control */
static HWND hPhysicalMemorySystemCacheEdit;  /* Physical Memory System Cache Edit Control */
static HWND hTotalsHandleCountEdit;          /* Total Handles Edit Control */
static HWND hTotalsProcessCountEdit;         /* Total Processes Edit Control */
static HWND hTotalsThreadCountEdit;          /* Total Threads Edit Control */

#ifdef RUN_PERF_PAGE
static HANDLE hPerformanceThread = NULL;
static DWORD  dwPerformanceThread;
#endif

static int nPerformancePageWidth;
static int nPerformancePageHeight;
static int lastX, lastY;
DWORD WINAPI PerformancePageRefreshThread(PVOID Parameter);

void AdjustFrameSize(HWND hCntrl, HWND hDlg, int nXDifference, int nYDifference, int pos)
{
    RECT  rc;
    int   cx, cy, sx, sy;

    GetClientRect(hCntrl, &rc);
    MapWindowPoints(hCntrl, hDlg, (LPPOINT)(PRECT)(&rc), sizeof(RECT)/sizeof(POINT));
    if (pos) {
        cx = rc.left;
        cy = rc.top;
        sx = rc.right - rc.left;
        switch (pos) {
        case 1:
            break;
        case 2:
            cy += nYDifference / 2;
            break;
        case 3:
            sx += nXDifference;
            break;
        case 4:
            cy += nYDifference / 2;
            sx += nXDifference;
            break;
        }
        sy = rc.bottom - rc.top + nYDifference / 2;
        SetWindowPos(hCntrl, NULL, cx, cy, sx, sy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    } else {
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hCntrl, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
    }
    InvalidateRect(hCntrl, NULL, TRUE);
}

static inline
void AdjustControlPosition(HWND hCntrl, HWND hDlg, int nXDifference, int nYDifference)
{
    AdjustFrameSize(hCntrl, hDlg, nXDifference, nYDifference, 0);
}

static inline
void AdjustCntrlPos(int ctrl_id, HWND hDlg, int nXDifference, int nYDifference)
{
    AdjustFrameSize(GetDlgItem(hDlg, ctrl_id), hDlg, nXDifference, nYDifference, 0);
}

INT_PTR CALLBACK
PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;

    switch (message)
    {
        case WM_DESTROY:
            GraphCtrl_Dispose(&PerformancePageCpuUsageHistoryGraph);
            GraphCtrl_Dispose(&PerformancePageMemUsageHistoryGraph);
#ifdef RUN_PERF_PAGE
            EndLocalThread(&hPerformanceThread, dwPerformanceThread);
#endif
            break;

        case WM_INITDIALOG:
        {
            BOOL bGraph;
            TM_FORMAT fmt;

            /* Save the width and height */
            GetClientRect(hDlg, &rc);
            nPerformancePageWidth = rc.right;
            nPerformancePageHeight = rc.bottom;

            /* Update window position */
            SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

            /*
             * Get handles to all the controls
             */
            hTotalsFrame = GetDlgItem(hDlg, IDC_TOTALS_FRAME);
            hCommitChargeFrame = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_FRAME);
            hKernelMemoryFrame = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_FRAME);
            hPhysicalMemoryFrame = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_FRAME);

            hCpuUsageFrame = GetDlgItem(hDlg, IDC_CPU_USAGE_FRAME);
            hMemUsageFrame = GetDlgItem(hDlg, IDC_MEM_USAGE_FRAME);
            hCpuUsageHistoryFrame = GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_FRAME);
            hMemUsageHistoryFrame = GetDlgItem(hDlg, IDC_MEMORY_USAGE_HISTORY_FRAME);

            hCommitChargeTotalEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_TOTAL);
            hCommitChargeLimitEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_LIMIT);
            hCommitChargePeakEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_PEAK);
            hKernelMemoryTotalEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_TOTAL);
            hKernelMemoryPagedEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_PAGED);
            hKernelMemoryNonPagedEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_NONPAGED);
            hPhysicalMemoryTotalEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_TOTAL);
            hPhysicalMemoryAvailableEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_AVAILABLE);
            hPhysicalMemorySystemCacheEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_SYSTEM_CACHE);
            hTotalsHandleCountEdit = GetDlgItem(hDlg, IDC_TOTALS_HANDLE_COUNT);
            hTotalsProcessCountEdit = GetDlgItem(hDlg, IDC_TOTALS_PROCESS_COUNT);
            hTotalsThreadCountEdit = GetDlgItem(hDlg, IDC_TOTALS_THREAD_COUNT);

            hCpuUsageGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_GRAPH);
            hMemUsageGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_GRAPH);
            hPerformancePageMemUsageHistoryGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_HISTORY_GRAPH);
            hPerformancePageCpuUsageHistoryGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_GRAPH);

            /* Create the controls */
            fmt.clrBack = RGB(0, 0, 0);
            fmt.clrGrid = RGB(0, 128, 64);
            fmt.clrPlot0 = RGB(0, 255, 0);
            fmt.clrPlot1 = RGB(255, 0, 0);
            fmt.GridCellWidth = fmt.GridCellHeight = 12;
            fmt.DrawSecondaryPlot = TaskManagerSettings.ShowKernelTimes;
            bGraph = GraphCtrl_Create(&PerformancePageCpuUsageHistoryGraph, hPerformancePageCpuUsageHistoryGraph, hDlg, &fmt);
            if (!bGraph)
            {
                EndDialog(hDlg, 0);
                return FALSE;
            }

            fmt.clrPlot0 = RGB(255, 255, 0);
            fmt.clrPlot1 = RGB(100, 255, 255);
            fmt.DrawSecondaryPlot = TRUE;
            bGraph = GraphCtrl_Create(&PerformancePageMemUsageHistoryGraph, hPerformancePageMemUsageHistoryGraph, hDlg, &fmt);
            if (!bGraph)
            {
                EndDialog(hDlg, 0);
                return FALSE;
            }

            /* Start our refresh thread */
#ifdef RUN_PERF_PAGE
            hPerformanceThread = CreateThread(NULL, 0, PerformancePageRefreshThread, NULL, 0, &dwPerformanceThread);
#endif

            /*
             * Subclass graph buttons
             */
            OldGraphWndProc = (WNDPROC)SetWindowLongPtrW(hCpuUsageGraph, GWLP_WNDPROC, (LONG_PTR)Graph_WndProc);
            SetWindowLongPtrW(hMemUsageGraph, GWLP_WNDPROC, (LONG_PTR)Graph_WndProc);
            OldGraphCtrlWndProc = (WNDPROC)SetWindowLongPtrW(hPerformancePageMemUsageHistoryGraph, GWLP_WNDPROC, (LONG_PTR)GraphCtrl_WndProc);
            SetWindowLongPtrW(hPerformancePageCpuUsageHistoryGraph, GWLP_WNDPROC, (LONG_PTR)GraphCtrl_WndProc);
            return TRUE;
        }

        case WM_COMMAND:
            break;

        case WM_SIZE:
        {
            int  cx, cy;
            int  nXDifference;
            int  nYDifference;

            if (wParam == SIZE_MINIMIZED)
                return 0;

            cx = LOWORD(lParam);
            cy = HIWORD(lParam);
            nXDifference = cx - nPerformancePageWidth;
            nYDifference = cy - nPerformancePageHeight;
            nPerformancePageWidth = cx;
            nPerformancePageHeight = cy;

            /* Reposition the performance page's controls */
            AdjustFrameSize(hTotalsFrame, hDlg, 0, nYDifference, 0);
            AdjustFrameSize(hCommitChargeFrame, hDlg, 0, nYDifference, 0);
            AdjustFrameSize(hKernelMemoryFrame, hDlg, 0, nYDifference, 0);
            AdjustFrameSize(hPhysicalMemoryFrame, hDlg, 0, nYDifference, 0);
            AdjustCntrlPos(IDS_COMMIT_CHARGE_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_COMMIT_CHARGE_LIMIT, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_COMMIT_CHARGE_PEAK, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_KERNEL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_KERNEL_MEMORY_PAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_KERNEL_MEMORY_NONPAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_PHYSICAL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_PHYSICAL_MEMORY_AVAILABLE, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_PHYSICAL_MEMORY_SYSTEM_CACHE, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_TOTALS_HANDLE_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_TOTALS_PROCESS_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDS_TOTALS_THREAD_COUNT, hDlg, 0, nYDifference);

            AdjustControlPosition(hCommitChargeTotalEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hCommitChargeLimitEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hCommitChargePeakEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hKernelMemoryTotalEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hKernelMemoryPagedEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hKernelMemoryNonPagedEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hPhysicalMemoryTotalEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hPhysicalMemoryAvailableEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hPhysicalMemorySystemCacheEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hTotalsHandleCountEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hTotalsProcessCountEdit, hDlg, 0, nYDifference);
            AdjustControlPosition(hTotalsThreadCountEdit, hDlg, 0, nYDifference);

            nXDifference += lastX;
            nYDifference += lastY;
            lastX = lastY = 0;
            if (nXDifference % 2)
            {
                if (nXDifference > 0)
                {
                    nXDifference--;
                    lastX++;
                }
                else
                {
                    nXDifference++;
                    lastX--;
                }
            }
            if (nYDifference % 2)
            {
                if (nYDifference > 0)
                {
                    nYDifference--;
                    lastY++;
                }
                else
                {
                    nYDifference++;
                    lastY--;
                }
            }
            AdjustFrameSize(hCpuUsageFrame, hDlg, nXDifference, nYDifference, 1);
            AdjustFrameSize(hMemUsageFrame, hDlg, nXDifference, nYDifference, 2);
            AdjustFrameSize(hCpuUsageHistoryFrame, hDlg, nXDifference, nYDifference, 3);
            AdjustFrameSize(hMemUsageHistoryFrame, hDlg, nXDifference, nYDifference, 4);
            AdjustFrameSize(hCpuUsageGraph, hDlg, nXDifference, nYDifference, 1);
            AdjustFrameSize(hMemUsageGraph, hDlg, nXDifference, nYDifference, 2);
            AdjustFrameSize(hPerformancePageCpuUsageHistoryGraph, hDlg, nXDifference, nYDifference, 3);
            AdjustFrameSize(hPerformancePageMemUsageHistoryGraph, hDlg, nXDifference, nYDifference, 4);
            break;
        }
    }
    return 0;
}

void RefreshPerformancePage(void)
{
#ifdef RUN_PERF_PAGE
    /* Signal the event so that our refresh thread
     * will wake up and refresh the performance page */
    PostThreadMessage(dwPerformanceThread, WM_TIMER, 0, 0);
#endif
}

DWORD WINAPI PerformancePageRefreshThread(PVOID Parameter)
{
    ULONGLONG CommitChargeTotal;
    ULONGLONG CommitChargeLimit;
    ULONGLONG CommitChargePeak;

    ULONG CpuUsage;
    ULONG CpuKernelUsage;

    ULONGLONG KernelMemoryTotal;
    ULONGLONG KernelMemoryPaged;
    ULONGLONG KernelMemoryNonPaged;

    ULONGLONG PhysicalMemoryTotal;
    ULONGLONG PhysicalMemoryAvailable;
    ULONGLONG PhysicalMemorySystemCache;

    ULONG TotalHandles;
    ULONG TotalThreads;
    ULONG TotalProcesses;

    MSG msg;

    WCHAR Text[260];
    WCHAR szMemUsage[256], szCpuUsage[256], szProcesses[256];

    LoadStringW(hInst, IDS_STATUS_CPUUSAGE, szCpuUsage, _countof(szCpuUsage));
    LoadStringW(hInst, IDS_STATUS_MEMUSAGE, szMemUsage, _countof(szMemUsage));
    LoadStringW(hInst, IDS_STATUS_PROCESSES, szProcesses, _countof(szProcesses));

    while (1)
    {
        extern BOOL bTrackMenu; // From taskmgr.c

        int nBarsUsed1;
        int nBarsUsed2;

        WCHAR szChargeTotalFormat[256];
        WCHAR szChargeLimitFormat[256];

        /* Wait for an the event or application close */
        if (GetMessage(&msg, NULL, 0, 0) <= 0)
            return 0;

        if (msg.message == WM_TIMER)
        {
            /*
             * Update the commit charge info
             */
            CommitChargeTotal = PerfDataGetCommitChargeTotalK();
            CommitChargeLimit = PerfDataGetCommitChargeLimitK();
            CommitChargePeak  = PerfDataGetCommitChargePeakK();
            _ultow(CommitChargeTotal, Text, 10);
            SetWindowTextW(hCommitChargeTotalEdit, Text);
            _ultow(CommitChargeLimit, Text, 10);
            SetWindowTextW(hCommitChargeLimitEdit, Text);
            _ultow(CommitChargePeak, Text, 10);
            SetWindowTextW(hCommitChargePeakEdit, Text);

            StrFormatByteSizeW(CommitChargeTotal * 1024,
                               szChargeTotalFormat,
                               _countof(szChargeTotalFormat));

            StrFormatByteSizeW(CommitChargeLimit * 1024,
                               szChargeLimitFormat,
                               _countof(szChargeLimitFormat));

            if (!bTrackMenu)
            {
                wsprintfW(Text, szMemUsage, szChargeTotalFormat, szChargeLimitFormat,
                    (CommitChargeLimit ? ((CommitChargeTotal * 100) / CommitChargeLimit) : 0));
                SendMessageW(hStatusWnd, SB_SETTEXT, 2, (LPARAM)Text);
            }

            /*
             * Update the kernel memory info
             */
            KernelMemoryTotal = PerfDataGetKernelMemoryTotalK();
            KernelMemoryPaged = PerfDataGetKernelMemoryPagedK();
            KernelMemoryNonPaged = PerfDataGetKernelMemoryNonPagedK();
            _ultow(KernelMemoryTotal, Text, 10);
            SetWindowTextW(hKernelMemoryTotalEdit, Text);
            _ultow(KernelMemoryPaged, Text, 10);
            SetWindowTextW(hKernelMemoryPagedEdit, Text);
            _ultow(KernelMemoryNonPaged, Text, 10);
            SetWindowTextW(hKernelMemoryNonPagedEdit, Text);

            /*
             * Update the physical memory info
             */
            PhysicalMemoryTotal = PerfDataGetPhysicalMemoryTotalK();
            PhysicalMemoryAvailable = PerfDataGetPhysicalMemoryAvailableK();
            PhysicalMemorySystemCache = PerfDataGetPhysicalMemorySystemCacheK();
            _ultow(PhysicalMemoryTotal, Text, 10);
            SetWindowTextW(hPhysicalMemoryTotalEdit, Text);
            _ultow(PhysicalMemoryAvailable, Text, 10);
            SetWindowTextW(hPhysicalMemoryAvailableEdit, Text);
            _ultow(PhysicalMemorySystemCache, Text, 10);
            SetWindowTextW(hPhysicalMemorySystemCacheEdit, Text);

            /*
             * Update the totals info
             */
            TotalHandles = PerfDataGetSystemHandleCount();
            TotalThreads = PerfDataGetTotalThreadCount();
            TotalProcesses = PerfDataGetProcessCount();
            _ultow(TotalHandles, Text, 10);
            SetWindowTextW(hTotalsHandleCountEdit, Text);
            _ultow(TotalThreads, Text, 10);
            SetWindowTextW(hTotalsThreadCountEdit, Text);
            _ultow(TotalProcesses, Text, 10);
            SetWindowTextW(hTotalsProcessCountEdit, Text);
            if (!bTrackMenu)
            {
                wsprintfW(Text, szProcesses, TotalProcesses);
                SendMessageW(hStatusWnd, SB_SETTEXT, 0, (LPARAM)Text);
            }

            /*
             * Redraw the graphs
             */
            InvalidateRect(hCpuUsageGraph, NULL, FALSE);
            InvalidateRect(hMemUsageGraph, NULL, FALSE);

            /*
             * Get the CPU usage
             */
            CpuUsage = PerfDataGetProcessorUsage();
            CpuKernelUsage = PerfDataGetProcessorSystemUsage();

            if (!bTrackMenu)
            {
                wsprintfW(Text, szCpuUsage, CpuUsage);
                SendMessageW(hStatusWnd, SB_SETTEXT, 1, (LPARAM)Text);
            }

            /*
             * Get the memory usage
             */
            CommitChargeTotal = PerfDataGetCommitChargeTotalK();
            CommitChargeLimit = PerfDataGetCommitChargeLimitK();
            nBarsUsed1 = CommitChargeLimit ? ((CommitChargeTotal * 100) / CommitChargeLimit) : 0;

            PhysicalMemoryTotal = PerfDataGetPhysicalMemoryTotalK();
            PhysicalMemoryAvailable = PerfDataGetPhysicalMemoryAvailableK();
            nBarsUsed2 = PhysicalMemoryTotal ? ((PhysicalMemoryAvailable * 100) / PhysicalMemoryTotal) : 0;

            GraphCtrl_AddPoint(&PerformancePageCpuUsageHistoryGraph, CpuUsage, CpuKernelUsage);
            GraphCtrl_AddPoint(&PerformancePageMemUsageHistoryGraph, nBarsUsed1, nBarsUsed2);
            InvalidateRect(hPerformancePageMemUsageHistoryGraph, NULL, FALSE);
            InvalidateRect(hPerformancePageCpuUsageHistoryGraph, NULL, FALSE);
        }
    }
    return 0;
}

void PerformancePage_OnViewShowKernelTimes(void)
{
    HMENU hMenu;
    HMENU hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    /* Check or uncheck the show 16-bit tasks menu item */
    if (GetMenuState(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND) & MF_CHECKED)
    {
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);
        TaskManagerSettings.ShowKernelTimes = FALSE;
        PerformancePageCpuUsageHistoryGraph.DrawSecondaryPlot = FALSE;
    }
    else
    {
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
        TaskManagerSettings.ShowKernelTimes = TRUE;
        PerformancePageCpuUsageHistoryGraph.DrawSecondaryPlot = TRUE;
    }

    GraphCtrl_RedrawBitmap(&PerformancePageCpuUsageHistoryGraph, PerformancePageCpuUsageHistoryGraph.BitmapHeight);
    RefreshPerformancePage();
}

void PerformancePage_OnViewCPUHistoryOneGraphAll(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hCPUHistoryMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hCPUHistoryMenu = GetSubMenu(hViewMenu, 3);

    TaskManagerSettings.CPUHistory_OneGraphPerCPU = FALSE;
    CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);
}

void PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hCPUHistoryMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hCPUHistoryMenu = GetSubMenu(hViewMenu, 3);

    TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
    CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
}
