/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Performance Page.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"
#include <shlwapi.h>

extern BOOL bInMenuLoop;    /* Tells us if we are in the menu loop - from taskmgr.c */

static TM_GRAPH_CONTROL CpuUsageHistoryGraph;
static TM_GRAPH_CONTROL MemUsageHistoryGraph;

HWND hPerformancePage;              /* Performance Property Page */
static HWND hCpuUsageGraph;         /* CPU Usage Graph */
static HWND hMemUsageGraph;         /* MEM Usage Graph */
static HWND hCpuUsageHistoryGraph;  /* CPU Usage History Graph */
static HWND hMemUsageHistoryGraph;  /* Memory Usage History Graph */

static int nPerformancePageWidth;
static int nPerformancePageHeight;
static int lastX, lastY;

static void
AdjustFrameSize(HDWP* phdwp, HWND hCntrl, HWND hDlg, int nXDifference, int nYDifference, int posFlag)
{
    RECT rc;
    int  cx, cy, sx, sy;

    if (!phdwp || !*phdwp)
        return;

    // GetClientRect(hCntrl, &rc);
    GetWindowRect(hCntrl, &rc);
    // MapWindowPoints(hCntrl, hDlg, (LPPOINT)&rc, sizeof(RECT)/sizeof(POINT));
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, sizeof(RECT) / sizeof(POINT));
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
        *phdwp = DeferWindowPos(*phdwp,
                                hCntrl, NULL,
                                cx, cy, sx, sy,
                                SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    }
    else
    {
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        *phdwp = DeferWindowPos(*phdwp,
                                hCntrl, NULL,
                                cx, cy, 0, 0,
                                SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
    }
    InvalidateRect(hCntrl, NULL, TRUE);
}

// AdjustControlPosition
static inline
void AdjustCntrlPos(HDWP* phdwp, int ctrl_id, HWND hDlg, int nXDifference, int nYDifference)
{
    AdjustFrameSize(phdwp, GetDlgItem(hDlg, ctrl_id), hDlg, nXDifference, nYDifference, 0);
}

INT_PTR CALLBACK
PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
            GraphCtrl_Dispose(&CpuUsageHistoryGraph);
            GraphCtrl_Dispose(&MemUsageHistoryGraph);
            break;

        case WM_INITDIALOG:
        {
            RECT rc;
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
            hCpuUsageHistoryGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_GRAPH);
            hMemUsageHistoryGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_HISTORY_GRAPH);

            /* Create the graph controls */
            fmt.clrBack = RGB(0, 0, 0);
            fmt.clrGrid = RGB(0, 128, 64);
            fmt.clrPlot0 = RGB(0, 255, 0);
            fmt.clrPlot1 = RGB(255, 0, 0);
            fmt.GridCellWidth = fmt.GridCellHeight = 12;
            fmt.DrawSecondaryPlot = TaskManagerSettings.ShowKernelTimes;
            bGraph = GraphCtrl_Create(&CpuUsageHistoryGraph, hCpuUsageHistoryGraph, hDlg, &fmt);
            if (!bGraph)
            {
                EndDialog(hDlg, 0);
                return FALSE;
            }

            fmt.clrPlot0 = RGB(255, 255, 0);
            fmt.clrPlot1 = RGB(100, 255, 255);
            fmt.DrawSecondaryPlot = TRUE;
            bGraph = GraphCtrl_Create(&MemUsageHistoryGraph, hMemUsageHistoryGraph, hDlg, &fmt);
            if (!bGraph)
            {
                EndDialog(hDlg, 0);
                return FALSE;
            }

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            // TODO: Support multiple CPU graphs.
            if ((drawItem->CtlID == IDC_CPU_USAGE_HISTORY_GRAPH) ||
                (drawItem->CtlID == IDC_MEM_USAGE_HISTORY_GRAPH))
            {
                PTM_GRAPH_CONTROL graph;

                if (drawItem->CtlID == IDC_CPU_USAGE_HISTORY_GRAPH)
                    graph = &CpuUsageHistoryGraph;
                else if (drawItem->CtlID == IDC_MEM_USAGE_HISTORY_GRAPH)
                    graph = &MemUsageHistoryGraph;

                GraphCtrl_OnDraw(drawItem->hwndItem,
                                 graph,
                                 (WPARAM)drawItem->hDC, 0);
            }
            else
            if ((drawItem->CtlID == IDC_CPU_USAGE_GRAPH) ||
                (drawItem->CtlID == IDC_MEM_USAGE_GRAPH))
            {
                Graph_Draw(drawItem->hwndItem, (WPARAM)drawItem->hDC, 0);
            }

            break;
            // return TRUE;
        }

        case WM_SIZE:
        {
            int cx, cy;
            int nXDifference;
            int nYDifference;
            HDWP hdwp;
            RECT rcClient;

            if (wParam == SIZE_MINIMIZED)
                return 0;

            cx = LOWORD(lParam);
            cy = HIWORD(lParam);
            nXDifference = cx - nPerformancePageWidth;
            nYDifference = cy - nPerformancePageHeight;
            nPerformancePageWidth = cx;
            nPerformancePageHeight = cy;

            hdwp = BeginDeferWindowPos(16 + 12 + 8);

            /* Reposition the performance page's controls */
            AdjustCntrlPos(&hdwp, IDC_TOTALS_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_COMMIT_CHARGE_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_KERNEL_MEMORY_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_PHYSICAL_MEMORY_FRAME, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_COMMIT_CHARGE_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_COMMIT_CHARGE_LIMIT, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_COMMIT_CHARGE_PEAK, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_KERNEL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_KERNEL_MEMORY_PAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_KERNEL_MEMORY_NONPAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_PHYSICAL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_PHYSICAL_MEMORY_AVAILABLE, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_PHYSICAL_MEMORY_SYSTEM_CACHE, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_TOTALS_HANDLE_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_TOTALS_PROCESS_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDS_TOTALS_THREAD_COUNT, hDlg, 0, nYDifference);

            AdjustCntrlPos(&hdwp, IDC_COMMIT_CHARGE_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_COMMIT_CHARGE_LIMIT, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_COMMIT_CHARGE_PEAK, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_KERNEL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_KERNEL_MEMORY_PAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_KERNEL_MEMORY_NONPAGED, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_PHYSICAL_MEMORY_TOTAL, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_PHYSICAL_MEMORY_AVAILABLE, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_PHYSICAL_MEMORY_SYSTEM_CACHE, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_TOTALS_HANDLE_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_TOTALS_THREAD_COUNT, hDlg, 0, nYDifference);
            AdjustCntrlPos(&hdwp, IDC_TOTALS_PROCESS_COUNT, hDlg, 0, nYDifference);

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

            AdjustFrameSize(&hdwp, GetDlgItem(hDlg, IDC_CPU_USAGE_FRAME), hDlg, nXDifference, nYDifference, 1);
            AdjustFrameSize(&hdwp, GetDlgItem(hDlg, IDC_MEM_USAGE_FRAME), hDlg, nXDifference, nYDifference, 2);
            AdjustFrameSize(&hdwp, GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_FRAME), hDlg, nXDifference, nYDifference, 3);
            AdjustFrameSize(&hdwp, GetDlgItem(hDlg, IDC_MEMORY_USAGE_HISTORY_FRAME), hDlg, nXDifference, nYDifference, 4);
            AdjustFrameSize(&hdwp, hCpuUsageGraph, hDlg, nXDifference, nYDifference, 1);
            AdjustFrameSize(&hdwp, hMemUsageGraph, hDlg, nXDifference, nYDifference, 2);
            AdjustFrameSize(&hdwp, hCpuUsageHistoryGraph, hDlg, nXDifference, nYDifference, 3);
            AdjustFrameSize(&hdwp, hMemUsageHistoryGraph, hDlg, nXDifference, nYDifference, 4);

            if (hdwp)
                EndDeferWindowPos(hdwp);

            /* Resize the graphs */
            GetClientRect(hCpuUsageHistoryGraph, &rcClient);
            GraphCtrl_OnSize(hCpuUsageHistoryGraph,
                             &CpuUsageHistoryGraph,
                             wParam,
                             MAKELPARAM(rcClient.right - rcClient.left,
                                        rcClient.bottom - rcClient.top));

            GetClientRect(hMemUsageHistoryGraph, &rcClient);
            GraphCtrl_OnSize(hMemUsageHistoryGraph,
                             &MemUsageHistoryGraph,
                             wParam,
                             MAKELPARAM(rcClient.right - rcClient.left,
                                        rcClient.bottom - rcClient.top));

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
    GraphCtrl_AddPoint(&CpuUsageHistoryGraph, CpuUsage, CpuKernelUsage);
    GraphCtrl_AddPoint(&MemUsageHistoryGraph, nBarsUsed1, nBarsUsed2);

    /* Update the status bar */
    UpdatePerfStatusBar(TotalProcesses, CpuUsage, CommitChargeTotal, CommitChargeLimit);

    /** Down below, that's what we do IIF we are actually active and need to repaint stuff **/

    /*
     * Redraw the graphs
     */
    InvalidateRect(hCpuUsageGraph, NULL, FALSE);
    InvalidateRect(hMemUsageGraph, NULL, FALSE);

    InvalidateRect(hCpuUsageHistoryGraph, NULL, FALSE);
    InvalidateRect(hMemUsageHistoryGraph, NULL, FALSE);
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
        CpuUsageHistoryGraph.DrawSecondaryPlot = FALSE;
    }
    else
    {
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
        TaskManagerSettings.ShowKernelTimes = TRUE;
        CpuUsageHistoryGraph.DrawSecondaryPlot = TRUE;
    }

    GraphCtrl_RedrawBitmap(&CpuUsageHistoryGraph, CpuUsageHistoryGraph.BitmapHeight);
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
