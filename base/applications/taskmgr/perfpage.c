/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Page
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"
#include <shlwapi.h>

#define MAX_CPU_PER_LINE    16 // TODO: Make this selectable in submenu.

// typedef struct _CPU_GRAPH CPU_GRAPH, *PCPU_GRAPH;
static ULONG CpuTotalPanes = 0;
static PTM_GRAPH_CONTROL CpuUsageHistoryGraphs = NULL; /* CPU Usage History Graphs Array */

static HWND hCpuUsageHistoryGraph; /* CPU Usage History Graph */
static TM_GRAPH_CONTROL CpuUsageHistoryGraph; /* CPU Usage History Graph template control */
static RECT rcCpuGraphArea; /* Rectangle area for CPU graphs */

static HWND hMemUsageHistoryGraph; /* Memory Usage History Graph */
static TM_GRAPH_CONTROL MemUsageHistoryGraph;

static HWND hCpuUsageGraph; /* CPU Usage Graph */
static HWND hMemUsageGraph; /* MEM Usage Graph */
static TM_GAUGE_CONTROL CpuUsageGraph = {0};
static TM_GAUGE_CONTROL MemUsageGraph = {0};

HWND hPerformancePage; /* Performance Property Page */
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

    GetWindowRect(hCntrl, &rc);
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
            CpuUsageGraph.bIsCPU = TRUE;
            // CpuUsageGraph.hWnd = hCpuUsageGraph;
            MemUsageGraph.bIsCPU = FALSE;
            // MemUsageGraph.hWnd = hMemUsageGraph;

            fmt.clrBack = RGB(0, 0, 0);
            fmt.clrGrid = RGB(0, 128, 64);

            fmt.clrPlot0 = RGB(0, 255, 0);
            fmt.clrPlot1 = RGB(255, 0, 0);
            fmt.GridCellWidth = fmt.GridCellHeight = 12;
            fmt.DrawSecondaryPlot = TaskManagerSettings.ShowKernelTimes;

            /* Retrieve the size of the single original CPU graph control,
             * that will serve as our overall CPU graph area where the
             * per-CPU graph panels will reside. */
            GetWindowRect(hCpuUsageHistoryGraph, &rcCpuGraphArea);
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rcCpuGraphArea, sizeof(RECT) / sizeof(POINT));

            /* Initialize the number of total CPU history graph panes to the number of CPUs on the system */
            CpuTotalPanes = PerfDataGetProcessorCount();

            /* Initialize the CPU history graph panes */
            if (CpuTotalPanes > 1)
            {
                /*
                 * Inherit the characteristics of the new per-CPU graph panes
                 * from the main original one, and create their corresponding panes.
                 */
                LPCWSTR lpClassAtom = (LPCWSTR)GetClassLongPtrW(hCpuUsageHistoryGraph, GCW_ATOM);
                DWORD dwExStyle = GetWindowLongPtrW(hCpuUsageHistoryGraph, GWL_EXSTYLE);
                DWORD dwStyle = GetWindowLongPtrW(hCpuUsageHistoryGraph, GWL_STYLE);
                HWND hwndParent = GetParent(hCpuUsageHistoryGraph);

                HWND hwndCPU;
                ULONG i;

                CpuUsageHistoryGraphs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                  sizeof(TM_GRAPH_CONTROL) * CpuTotalPanes);
                if (!CpuUsageHistoryGraphs)
                    goto oneCPUGraph; /* Fall back to one graph for all CPUs */

                /* Initialize the first entry for CPU #0 */
                bGraph = GraphCtrl_Create(&CpuUsageHistoryGraphs[0], hCpuUsageHistoryGraph, hDlg, &fmt);
                if (!bGraph)
                {
                    /* Ignore graph creation failure (may happen under low memory resources) */
                    // CpuUsageHistoryGraphs[0].hWnd = hCpuUsageHistoryGraph;
                    // NOTHING;
                }

                /* Start the loop at 1 for the other CPUs */
                ASSERT(CpuTotalPanes <= MAXWORD);
                for (i = 1; i < CpuTotalPanes; ++i)
                {
                    /* Allocate a new window, inheriting its class and style
                     * from the single original CPU graph control. Its actual
                     * position will be determined at the first WM_SIZE event
                     * after the property sheet page gets created. */
                    hwndCPU = CreateWindowExW(dwExStyle, lpClassAtom, L"CPU Graph", dwStyle,
                                              rcCpuGraphArea.left, rcCpuGraphArea.top,
                                              0, 0,
                                              hwndParent,
                                              /* Specifies child ID */
                                              (HMENU)ULongToHandle(MAKELONG(IDC_CPU_USAGE_HISTORY_GRAPH, (WORD)i)),
                                              hInst, NULL);
                    if (!hwndCPU)
                        continue;

                    bGraph = GraphCtrl_Create(&CpuUsageHistoryGraphs[i], hwndCPU, hDlg, &fmt);
                    if (!bGraph)
                    {
                        /* Ignore graph creation failure (may happen under low memory resources) */
                        // CpuUsageHistoryGraphs[i].hWnd = hwndCPU;
                        // NOTHING;
                    }

                    ShowWindow(hwndCPU, TaskManagerSettings.CPUHistory_OneGraphPerCPU ? SW_SHOW : SW_HIDE);
                }
            }
            else
            {
                HMENU hCPUHistoryMenu;
oneCPUGraph:
                /* Fall back to one graph for all CPUs */
                CpuTotalPanes = 1;
                CpuUsageHistoryGraphs = &CpuUsageHistoryGraph;
                bGraph = GraphCtrl_Create(&CpuUsageHistoryGraphs[0], hCpuUsageHistoryGraph, hDlg, &fmt);
                if (!bGraph)
                {
                    /* Ignore graph creation failure (may happen under low memory resources) */
                    // NOTHING;
                }

                /* Select one graph for all CPUs and disable the per-CPU graph menu item */
                PerformancePage_OnViewCPUHistoryGraph(TRUE);
                hCPUHistoryMenu = GetSubMenu(GetSubMenu(GetMenu(hMainWnd), 2), 3);
                EnableMenuItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND | MF_DISABLED);
            }

            fmt.clrPlot0 = RGB(255, 255, 0);
            fmt.clrPlot1 = RGB(100, 255, 255);
            fmt.DrawSecondaryPlot = TRUE;
            bGraph = GraphCtrl_Create(&MemUsageHistoryGraph, hMemUsageHistoryGraph, hDlg, &fmt);
            if (!bGraph)
            {
                /* Ignore graph creation failure (may happen under low memory resources) */
                // NOTHING;
            }

            return TRUE;
        }

        case WM_DESTROY:
        {
            if (CpuUsageHistoryGraphs && (CpuUsageHistoryGraphs != &CpuUsageHistoryGraph))
            {
                ULONG i;
                for (i = 0; i < CpuTotalPanes; ++i)
                {
                    HWND hwnd = CpuUsageHistoryGraphs[i].hWnd;
                    GraphCtrl_Dispose(&CpuUsageHistoryGraphs[i]);
                    DestroyWindow(hwnd);
                }
                HeapFree(GetProcessHeap(), 0, CpuUsageHistoryGraphs);
                CpuUsageHistoryGraphs = NULL;
            }
            GraphCtrl_Dispose(&CpuUsageHistoryGraph);
            GraphCtrl_Dispose(&MemUsageHistoryGraph);
            break;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            if (LOWORD(drawItem->CtlID) == IDC_CPU_USAGE_HISTORY_GRAPH)
            {
                ULONG i = HIWORD(drawItem->CtlID);
                if ((i < CpuTotalPanes) &&
                    (drawItem->hwndItem == CpuUsageHistoryGraphs[i].hWnd))
                {
                    GraphCtrl_OnDraw(drawItem->hwndItem,
                                     &CpuUsageHistoryGraphs[i],
                                     (WPARAM)drawItem->hDC, 0);
                }
            }
            else if (drawItem->CtlID == IDC_MEM_USAGE_HISTORY_GRAPH)
            {
                ASSERT(drawItem->hwndItem == MemUsageHistoryGraph.hWnd);
                GraphCtrl_OnDraw(drawItem->hwndItem,
                                 &MemUsageHistoryGraph,
                                 (WPARAM)drawItem->hDC, 0);
            }
            else if (drawItem->CtlID == IDC_CPU_USAGE_GRAPH)
            {
                Graph_DrawUsageGraph(drawItem->hwndItem,
                                     &CpuUsageGraph,
                                     (WPARAM)drawItem->hDC, 0);
            }
            else if (drawItem->CtlID == IDC_MEM_USAGE_GRAPH)
            {
                Graph_DrawUsageGraph(drawItem->hwndItem,
                                     &MemUsageGraph,
                                     (WPARAM)drawItem->hDC, 0);
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

            ULONG CPUPanes;
            ULONG i;

            if (wParam == SIZE_MINIMIZED)
                return 0;

            cx = LOWORD(lParam);
            cy = HIWORD(lParam);
            nXDifference = cx - nPerformancePageWidth;
            nYDifference = cy - nPerformancePageHeight;
            nPerformancePageWidth = cx;
            nPerformancePageHeight = cy;

            CPUPanes = (TaskManagerSettings.CPUHistory_OneGraphPerCPU ? CpuTotalPanes : 1);

            hdwp = BeginDeferWindowPos(16 + 12 + 6 + CPUPanes + 1);

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

            /* Lay out the CPU graphs */
            // if (CPUPanes > 1)
            {
                int nWidth, nHeight;

                /* Lay out the several CPU graphs in a grid-like manner */
                rcCpuGraphArea.right += nXDifference;
                rcCpuGraphArea.bottom += nYDifference / 2;
                nWidth = (rcCpuGraphArea.right - rcCpuGraphArea.left) / min(CPUPanes, MAX_CPU_PER_LINE); // - GetSystemMetrics(SM_CXBORDER);
                nHeight = (rcCpuGraphArea.bottom - rcCpuGraphArea.top) / (1 + (CPUPanes-1) / MAX_CPU_PER_LINE); // - GetSystemMetrics(SM_CYBORDER);

                for (i = 0; i < CPUPanes; ++i)
                {
                    hdwp = DeferWindowPos(hdwp,
                                          CpuUsageHistoryGraphs[i].hWnd,
                                          NULL,
                                          rcCpuGraphArea.left + (i % MAX_CPU_PER_LINE) * nWidth,
                                          rcCpuGraphArea.top + (i / MAX_CPU_PER_LINE) * nHeight,
                                          nWidth,
                                          nHeight,
                                          SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

                    InvalidateRect(CpuUsageHistoryGraphs[i].hWnd, NULL, TRUE);
                }
            }
#if 0
            else
            {
                /* The single CPU graph takes the whole CPU graph area */
                AdjustFrameSize(&hdwp, CpuUsageHistoryGraphs[0].hWnd, hDlg, nXDifference, nYDifference, 3);
            }
#endif

            AdjustFrameSize(&hdwp, hMemUsageHistoryGraph, hDlg, nXDifference, nYDifference, 4);

            if (hdwp)
                EndDeferWindowPos(hdwp);

            /* Resize the graphs */
            for (i = 0; i < CPUPanes; ++i)
            {
                GetClientRect(CpuUsageHistoryGraphs[i].hWnd, &rcClient);
                GraphCtrl_OnSize(CpuUsageHistoryGraphs[i].hWnd,
                                 &CpuUsageHistoryGraphs[i],
                                 wParam,
                                 MAKELPARAM(rcClient.right - rcClient.left,
                                            rcClient.bottom - rcClient.top));
            }

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
    extern BOOL bTrackMenu; // From taskmgr.c

    static WCHAR szProcesses[256] = L"";
    static WCHAR szCpuUsage[256]  = L"";
    static WCHAR szMemUsage[256]  = L"";

    WCHAR szChargeTotalFormat[256];
    WCHAR szChargeLimitFormat[256];
    WCHAR Text[260];

    /* Do nothing if the status bar is used to show menu hints */
    if (bTrackMenu)
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

    ULONG i;
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
    if (CpuUsageHistoryGraphs)
    {
        PerfDataAcquireLock();

        for (i = 0; i < CpuTotalPanes; ++i)
        {
            GraphCtrl_AddPoint(&CpuUsageHistoryGraphs[i],
                               PerfDataGetProcessorUsagePerCPU(i),
                               PerfDataGetProcessorSystemUsagePerCPU(i));
            InvalidateRect(CpuUsageHistoryGraphs[i].hWnd, NULL, FALSE);
        }

        PerfDataReleaseLock();
    }
    else
    {
        GraphCtrl_AddPoint(&CpuUsageHistoryGraph, CpuUsage, CpuKernelUsage);
        InvalidateRect(CpuUsageHistoryGraph.hWnd, NULL, FALSE);
    }

    /*
     * Update the graphs
     */
    nBarsUsed1 = CommitChargeLimit ? ((CommitChargeTotal * 100) / CommitChargeLimit) : 0;
    nBarsUsed2 = PhysicalMemoryTotal ? ((PhysicalMemoryAvailable * 100) / PhysicalMemoryTotal) : 0;
    // nBarsUsed2 = PhysicalMemoryTotal ? (((PhysicalMemoryTotal - PhysicalMemoryAvailable) * 100) / PhysicalMemoryTotal) : 0;
    GraphCtrl_AddPoint(&MemUsageHistoryGraph, nBarsUsed1, nBarsUsed2);

    // CpuUsageGraph.Maximum  = 100;
    CpuUsageGraph.Current1 = CpuUsage;
    CpuUsageGraph.Current2 = CpuKernelUsage;

    //
    // TODO: The memory gauge may show the commit charge (Win2000/XP/2003),
    // or show the **physical** memory amount (Windows Vista+). Have something
    // to set the preference...
    //
    Meter_CommitChargeTotal = (PhysicalMemoryTotal - PhysicalMemoryAvailable); // CommitChargeTotal;
    Meter_CommitChargeLimit = PhysicalMemoryTotal; // CommitChargeLimit;

    // MemUsageGraph.Maximum = Meter_CommitChargeLimit;
    if (Meter_CommitChargeLimit)
        MemUsageGraph.Current1 = (ULONG)((Meter_CommitChargeTotal * 100) / Meter_CommitChargeLimit);
    else
        MemUsageGraph.Current1 = 0;

    /* Update the status bar */
    UpdatePerfStatusBar(TotalProcesses, CpuUsage, CommitChargeTotal, CommitChargeLimit);

    /** Down below, that's what we do IIF we are actually active and need to repaint stuff **/

    /*
     * Redraw the graphs
     */
    InvalidateRect(hCpuUsageGraph, NULL, FALSE);
    InvalidateRect(hMemUsageGraph, NULL, FALSE);

    // InvalidateRect(CpuUsageHistoryGraph.hwndGraph, NULL, FALSE);
#if 0
    for (i = 0; i < (TaskManagerSettings.CPUHistory_OneGraphPerCPU ? CpuTotalPanes : 1); ++i)
    {
        InvalidateRect(CpuUsageHistoryGraphs[i].hWnd, NULL, FALSE);
    }
#endif
    InvalidateRect(hMemUsageHistoryGraph, NULL, FALSE);
}

void PerformancePage_OnViewShowKernelTimes(void)
{
    HMENU hViewMenu;
    ULONG i;

    hViewMenu = GetSubMenu(GetMenu(hMainWnd), 2);

    /* Check or uncheck the show kernel times menu item */
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

    for (i = 0; i < CpuTotalPanes; ++i)
    {
        CpuUsageHistoryGraphs[i].DrawSecondaryPlot = TaskManagerSettings.ShowKernelTimes;
        GraphCtrl_RedrawBitmap(&CpuUsageHistoryGraphs[i], CpuUsageHistoryGraphs[i].BitmapHeight);
    }

    RefreshPerformancePage();
}

VOID
PerformancePage_OnViewCPUHistoryGraph(
    _In_ BOOL bShowAll)
{
    HMENU hCPUHistoryMenu;
    ULONG i;

    hCPUHistoryMenu = GetSubMenu(GetSubMenu(GetMenu(hMainWnd), 2), 3);

    TaskManagerSettings.CPUHistory_OneGraphPerCPU = !bShowAll;
    CheckMenuRadioItem(hCPUHistoryMenu,
                       ID_VIEW_CPUHISTORY_ONEGRAPHALL,
                       ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU,
                       bShowAll ? ID_VIEW_CPUHISTORY_ONEGRAPHALL
                                : ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU,
                       MF_BYCOMMAND);

    /* Start the loop at 1 for the other CPUs; always keep the first CPU pane around */
    // ShowWindow(CpuUsageHistoryGraphs[0].hwndGraph, SW_SHOW);
    for (i = 1; i < CpuTotalPanes; ++i)
    {
        ShowWindow(CpuUsageHistoryGraphs[i].hWnd, TaskManagerSettings.CPUHistory_OneGraphPerCPU ? SW_SHOW : SW_HIDE);
    }

    // RefreshPerformancePage();
}
