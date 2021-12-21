/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Performance Page.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"
#include <shlwapi.h>

extern BOOL bInMenuLoop;        /* Tells us if we are in the menu loop - from taskmgr.c */

TM_GRAPH_CONTROL PerformancePageCpuUsageHistoryGraph;
TM_GRAPH_CONTROL PerformancePageMemUsageHistoryGraph;

HWND hPerformancePage;                /* Performance Property Page */
static HWND hCpuUsageGraph;                  /* CPU Usage Graph */
static HWND hMemUsageGraph;                  /* MEM Usage Graph */
HWND hPerformancePageCpuUsageHistoryGraph;           /* CPU Usage History Graph */
HWND hPerformancePageMemUsageHistoryGraph;           /* Memory Usage History Graph */

static int nPerformancePageWidth;
static int nPerformancePageHeight;
static int lastX, lastY;

static void
AdjustFrameSize(HWND hCntrl, HWND hDlg, int nXDifference, int nYDifference, int posFlag)
{
    RECT rc;
    int  cx, cy, sx, sy;

    GetClientRect(hCntrl, &rc);
    MapWindowPoints(hCntrl, hDlg, (LPPOINT)&rc, sizeof(RECT)/sizeof(POINT));
    if (posFlag)
    {
        cx = rc.left;
        cy = rc.top;
        sx = rc.right - rc.left;
        switch (posFlag)
        {
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
    }
    else
    {
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hCntrl, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
    }
    InvalidateRect(hCntrl, NULL, TRUE);
}

// AdjustControlPosition
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

            /* Get handles to the graph controls */
            hCpuUsageGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_GRAPH);
            hMemUsageGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_GRAPH);
            hPerformancePageMemUsageHistoryGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_HISTORY_GRAPH);
            hPerformancePageCpuUsageHistoryGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_GRAPH);

            /* Create the graph controls */
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
            AdjustCntrlPos(IDC_TOTALS_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_COMMIT_CHARGE_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_KERNEL_MEMORY_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_PHYSICAL_MEMORY_FRAME, hDlg, 0, nYDifference);
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

            AdjustCntrlPos(IDC_COMMIT_CHARGE_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_COMMIT_CHARGE_LIMIT, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_COMMIT_CHARGE_PEAK, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_KERNEL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_KERNEL_MEMORY_PAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_KERNEL_MEMORY_NONPAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_PHYSICAL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_PHYSICAL_MEMORY_AVAILABLE, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_PHYSICAL_MEMORY_SYSTEM_CACHE, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_TOTALS_HANDLE_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_TOTALS_THREAD_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(IDC_TOTALS_PROCESS_COUNT, hDlg, 0, nYDifference);

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

            AdjustFrameSize(GetDlgItem(hDlg, IDC_CPU_USAGE_FRAME), hDlg, nXDifference, nYDifference, 1);
            AdjustFrameSize(GetDlgItem(hDlg, IDC_MEM_USAGE_FRAME), hDlg, nXDifference, nYDifference, 2);
            AdjustFrameSize(GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_FRAME), hDlg, nXDifference, nYDifference, 3);
            AdjustFrameSize(GetDlgItem(hDlg, IDC_MEMORY_USAGE_HISTORY_FRAME), hDlg, nXDifference, nYDifference, 4);
            AdjustFrameSize(hCpuUsageGraph, hDlg, nXDifference, nYDifference, 1);
            AdjustFrameSize(hMemUsageGraph, hDlg, nXDifference, nYDifference, 2);
            AdjustFrameSize(hPerformancePageCpuUsageHistoryGraph, hDlg, nXDifference, nYDifference, 3);
            AdjustFrameSize(hPerformancePageMemUsageHistoryGraph, hDlg, nXDifference, nYDifference, 4);

            break;
        }
    }
    return 0;
}

static void
UpdatePerfStatusBar(
    _In_ ULONG TotalProcesses,
    _In_ ULONG CpuUsage,
    _In_ ULONGLONG CommitChargeTotal,
    _In_ ULONGLONG CommitChargeLimit)
{
    static WCHAR szProcesses[256] = L"";
    static WCHAR szCpuUsage[256]  = L"";
    static WCHAR szMemUsage[256]  = L"";

    WCHAR szChargeTotalFormat[256];
    WCHAR szChargeLimitFormat[256];
    WCHAR Text[260];

    /* Do nothing if we are in the menu loop */
    if (bInMenuLoop)
        return;

    if (!*szProcesses)
        LoadStringW(hInst, IDS_STATUS_PROCESSES, szProcesses, ARRAYSIZE(szProcesses));
    if (!*szCpuUsage)
        LoadStringW(hInst, IDS_STATUS_CPUUSAGE, szCpuUsage, ARRAYSIZE(szCpuUsage));
    if (!*szMemUsage)
        LoadStringW(hInst, IDS_STATUS_MEMUSAGE, szMemUsage, ARRAYSIZE(szMemUsage));

    wsprintfW(Text, szProcesses, TotalProcesses);
    SendMessageW(hStatusWnd, SB_SETTEXT, 0, (LPARAM)Text);

    wsprintfW(Text, szCpuUsage, CpuUsage);
    SendMessageW(hStatusWnd, SB_SETTEXT, 1, (LPARAM)Text);

    StrFormatByteSizeW(CommitChargeTotal * 1024,
                       szChargeTotalFormat,
                       ARRAYSIZE(szChargeTotalFormat));

    StrFormatByteSizeW(CommitChargeLimit * 1024,
                       szChargeLimitFormat,
                       ARRAYSIZE(szChargeLimitFormat));

    wsprintfW(Text, szMemUsage, szChargeTotalFormat, szChargeLimitFormat,
              (CommitChargeLimit ? ((CommitChargeTotal * 100) / CommitChargeLimit) : 0));
    SendMessageW(hStatusWnd, SB_SETTEXT, 2, (LPARAM)Text);
}

void RefreshPerformancePage(void)
{
    ULONGLONG CommitChargeTotal;
    ULONGLONG CommitChargeLimit;
    ULONGLONG CommitChargePeak;

    ULONGLONG KernelMemoryTotal;
    ULONGLONG KernelMemoryPaged;
    ULONGLONG KernelMemoryNonPaged;

    ULONGLONG PhysicalMemoryTotal;
    ULONGLONG PhysicalMemoryAvailable;
    ULONGLONG PhysicalMemorySystemCache;

    ULONG TotalHandles;
    ULONG TotalThreads;
    ULONG TotalProcesses;

    ULONG CpuUsage;
    ULONG CpuKernelUsage;

    WCHAR Text[260];

    int nBarsUsed1;
    int nBarsUsed2;

    /*
     * Update the commit charge info
     */
    PerfDataGetCommitChargeK(&CommitChargeTotal,
                             &CommitChargeLimit,
                             &CommitChargePeak);

    _ui64tow(CommitChargeTotal, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_COMMIT_CHARGE_TOTAL, Text);
    _ui64tow(CommitChargeLimit, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_COMMIT_CHARGE_LIMIT, Text);
    _ui64tow(CommitChargePeak, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_COMMIT_CHARGE_PEAK, Text);

    /*
     * Update the kernel memory info
     */
    PerfDataGetKernelMemoryK(&KernelMemoryTotal,
                             &KernelMemoryPaged,
                             &KernelMemoryNonPaged);

    _ui64tow(KernelMemoryTotal, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_KERNEL_MEMORY_TOTAL, Text);
    _ui64tow(KernelMemoryPaged, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_KERNEL_MEMORY_PAGED, Text);
    _ui64tow(KernelMemoryNonPaged, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_KERNEL_MEMORY_NONPAGED, Text);

    /*
     * Update the physical memory info
     */
    PerfDataGetPhysicalMemoryK(&PhysicalMemoryTotal,
                               &PhysicalMemoryAvailable,
                               &PhysicalMemorySystemCache);

    _ui64tow(PhysicalMemoryTotal, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_PHYSICAL_MEMORY_TOTAL, Text);
    _ui64tow(PhysicalMemoryAvailable, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_PHYSICAL_MEMORY_AVAILABLE, Text);
    _ui64tow(PhysicalMemorySystemCache, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_PHYSICAL_MEMORY_SYSTEM_CACHE, Text);

    /*
     * Update the totals info
     */
    TotalHandles = PerfDataGetSystemHandleCount();
    TotalThreads = PerfDataGetTotalThreadCount();
    TotalProcesses = PerfDataGetProcessCount();

    _ultow(TotalHandles, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_TOTALS_HANDLE_COUNT, Text);
    _ultow(TotalThreads, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_TOTALS_THREAD_COUNT, Text);
    _ultow(TotalProcesses, Text, 10);
    SetDlgItemTextW(hPerformancePage, IDC_TOTALS_PROCESS_COUNT, Text);

    /*
     * Get the CPU usage
     */
    CpuUsage = PerfDataGetProcessorUsage();
    CpuKernelUsage = PerfDataGetProcessorSystemUsage();

    /*
     * Update the graphs
     */
    nBarsUsed1 = CommitChargeLimit ? ((CommitChargeTotal * 100) / CommitChargeLimit) : 0;
    nBarsUsed2 = PhysicalMemoryTotal ? ((PhysicalMemoryAvailable * 100) / PhysicalMemoryTotal) : 0;
    GraphCtrl_AddPoint(&PerformancePageCpuUsageHistoryGraph, CpuUsage, CpuKernelUsage);
    GraphCtrl_AddPoint(&PerformancePageMemUsageHistoryGraph, nBarsUsed1, nBarsUsed2);

    /* Update the status bar */
    UpdatePerfStatusBar(TotalProcesses, CpuUsage, CommitChargeTotal, CommitChargeLimit);

    /** Down below, that's what we do IIF we are actually active and need to repaint stuff **/

    /*
     * Redraw the graphs
     */
    InvalidateRect(hCpuUsageGraph, NULL, FALSE);
    InvalidateRect(hMemUsageGraph, NULL, FALSE);

    InvalidateRect(hPerformancePageCpuUsageHistoryGraph, NULL, FALSE);
    InvalidateRect(hPerformancePageMemUsageHistoryGraph, NULL, FALSE);
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
