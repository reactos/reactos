/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Performance Graph Meters.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"

static int nlastBarsUsed = 0;

static void Graph_DrawCpuUsageGraph(HDC hDC, HWND hWnd);
static void Graph_DrawMemUsageGraph(HDC hDC, HWND hWnd);

INT_PTR CALLBACK
Graph_Draw(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LONG WindowId = GetWindowLongPtrW(hWnd, GWLP_ID);
    HDC  hdc = (HDC)wParam;

    switch (WindowId)
    {
    case IDC_CPU_USAGE_GRAPH:
        Graph_DrawCpuUsageGraph(hdc, hWnd);
        break;
    case IDC_MEM_USAGE_GRAPH:
        Graph_DrawMemUsageGraph(hdc, hWnd);
        break;
    }

    return 0;
}

static void Graph_DrawCpuUsageGraph(HDC hDC, HWND hWnd)
{
    RECT      rcClient;
    RECT      rcBarLeft;
    RECT      rcBarRight;
    RECT      rcText;
    COLORREF  crPrevForeground;
    WCHAR     Text[260];
    HFONT     hOldFont;
    ULONG     CpuUsage;
    ULONG     CpuKernelUsage;
    int       nBars;
    int       nBarsUsed;
/* Bottom bars that are "used", i.e. are bright green, representing used cpu time */
    int       nBarsUsedKernel;
/* Bottom bars that are "used", i.e. are bright green, representing used cpu kernel time */
    int       nBarsFree;
/* Top bars that are "unused", i.e. are dark green, representing free cpu time */
    int       i;

    /* Get the client area rectangle */
    GetClientRect(hWnd, &rcClient);

    /* Fill it with blackness */
    FillSolidRect(hDC, &rcClient, RGB(0, 0, 0));

    /* Get the CPU usage */
    CpuUsage = PerfDataGetProcessorUsage();

    wsprintfW(Text, L"%d%%", (int)CpuUsage);

    /*
     * Draw the font text onto the graph
     */
    rcText = rcClient;
    InflateRect(&rcText, -2, -2);
    crPrevForeground = SetTextColor(hDC, RGB(0, 255, 0));
    hOldFont = SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(hDC, Text, -1, &rcText, DT_BOTTOM | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);
    SelectObject(hDC, hOldFont);
    SetTextColor(hDC, crPrevForeground);

    /*
     * Now we have to draw the graph
     * So first find out how many bars we can fit
     */
    nBars = ((rcClient.bottom - rcClient.top) - 25) / 3;
    nBarsUsed = (nBars * CpuUsage) / 100;
    if ((CpuUsage) && (nBarsUsed == 0))
    {
        nBarsUsed = 1;
    }
    nBarsFree = nBars - (nlastBarsUsed > nBarsUsed ? nlastBarsUsed : nBarsUsed);
    if (TaskManagerSettings.ShowKernelTimes)
    {
        CpuKernelUsage = PerfDataGetProcessorSystemUsage();
        nBarsUsedKernel = (nBars * CpuKernelUsage) / 100;
    }
    else
    {
        nBarsUsedKernel = 0;
    }

    /*
     * Now draw the bar graph
     */
    rcBarLeft.left =  ((rcClient.right - rcClient.left) - 33) / 2;
    rcBarLeft.right =  rcBarLeft.left + 16;
    rcBarRight.left = rcBarLeft.left + 17;
    rcBarRight.right = rcBarLeft.right + 17;
    rcBarLeft.top = rcBarRight.top = 5;
    rcBarLeft.bottom = rcBarRight.bottom = 7;

    if (nBarsUsed < 0)     nBarsUsed = 0;
    if (nBarsUsed > nBars) nBarsUsed = nBars;

    if (nBarsFree < 0)     nBarsFree = 0;
    if (nBarsFree > nBars) nBarsFree = nBars;

    if (nBarsUsedKernel < 0)     nBarsUsedKernel = 0;
    if (nBarsUsedKernel > nBars) nBarsUsedKernel = nBars;

    /*
     * Draw the "free" bars
     */
    for (i=0; i<nBarsFree; i++)
    {
        FillSolidRect(hDC, &rcBarLeft, DARK_GREEN);
        FillSolidRect(hDC, &rcBarRight, DARK_GREEN);

        rcBarLeft.top += 3;
        rcBarLeft.bottom += 3;

        rcBarRight.top += 3;
        rcBarRight.bottom += 3;
    }

    /*
     * Draw the last "used" bars
     */
    if ((nlastBarsUsed - nBarsUsed) > 0)
    {
        for (i=0; i< (nlastBarsUsed - nBarsUsed); i++)
        {
            if (nlastBarsUsed > 5000) nlastBarsUsed = 5000;

            FillSolidRect(hDC, &rcBarLeft, MEDIUM_GREEN);
            FillSolidRect(hDC, &rcBarRight, MEDIUM_GREEN);

            rcBarLeft.top += 3;
            rcBarLeft.bottom += 3;

            rcBarRight.top += 3;
            rcBarRight.bottom += 3;
        }
    }
    nlastBarsUsed = nBarsUsed;
    /*
     * Draw the "used" bars
     */
    for (i=0; i<nBarsUsed; i++)
    {
        if (nBarsUsed > 5000) nBarsUsed = 5000;

        FillSolidRect(hDC, &rcBarLeft, BRIGHT_GREEN);
        FillSolidRect(hDC, &rcBarRight, BRIGHT_GREEN);

        rcBarLeft.top += 3;
        rcBarLeft.bottom += 3;

        rcBarRight.top += 3;
        rcBarRight.bottom += 3;
    }

    /*
     * Draw the "used" kernel bars
     */

    rcBarLeft.top -=3;
    rcBarLeft.bottom -=3;

    rcBarRight.top -=3;
    rcBarRight.bottom -=3;

    for (i=0; i<nBarsUsedKernel; i++)
    {
        FillSolidRect(hDC, &rcBarLeft, RED);
        FillSolidRect(hDC, &rcBarRight, RED);

        rcBarLeft.top -=3;
        rcBarLeft.bottom -=3;

        rcBarRight.top -=3;
        rcBarRight.bottom -=3;
    }

    SelectObject(hDC, hOldFont);
}

static void Graph_DrawMemUsageGraph(HDC hDC, HWND hWnd)
{
    RECT       rcClient;
    RECT       rcBarLeft;
    RECT       rcBarRight;
    RECT       rcText;
    COLORREF   crPrevForeground;
    WCHAR      Text[260];
    HFONT      hOldFont;
    ULONGLONG  CommitChargeTotal;
    ULONGLONG  CommitChargeLimit;
    int        nBars;
    int        nBarsUsed = 0;
/* Bottom bars that are "used", i.e. are bright green, representing used memory */
    int        nBarsFree;
/* Top bars that are "unused", i.e. are dark green, representing free memory */
    int        i;

    /* Get the client area rectangle */
    GetClientRect(hWnd, &rcClient);

    /* Fill it with blackness */
    FillSolidRect(hDC, &rcClient, RGB(0, 0, 0));

    /* Get the memory usage */
    PerfDataGetCommitChargeK(&CommitChargeTotal,
                             &CommitChargeLimit,
                             NULL);

    if (CommitChargeTotal > 1024)
        wsprintfW(Text, L"%d MB", (int)(CommitChargeTotal / 1024));
    else
        wsprintfW(Text, L"%d K", (int)CommitChargeTotal);

    /*
     * Draw the font text onto the graph
     */
    rcText = rcClient;
    InflateRect(&rcText, -2, -2);
    crPrevForeground = SetTextColor(hDC, RGB(0, 255, 0));
    hOldFont = SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(hDC, Text, -1, &rcText, DT_BOTTOM | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);
    SelectObject(hDC, hOldFont);
    SetTextColor(hDC, crPrevForeground);

    /*
     * Now we have to draw the graph
     * So first find out how many bars we can fit
     */
    nBars = ((rcClient.bottom - rcClient.top) - 25) / 3;
    if (CommitChargeLimit)
        nBarsUsed = (nBars * (int)((CommitChargeTotal * 100) / CommitChargeLimit)) / 100;
    nBarsFree = nBars - nBarsUsed;

    if (nBarsUsed < 0)     nBarsUsed = 0;
    if (nBarsUsed > nBars) nBarsUsed = nBars;

    if (nBarsFree < 0)     nBarsFree = 0;
    if (nBarsFree > nBars) nBarsFree = nBars;

    /*
     * Now draw the bar graph
     */
    rcBarLeft.left = ((rcClient.right - rcClient.left) - 33) / 2;
    rcBarLeft.right = rcBarLeft.left + 16;
    rcBarRight.left = rcBarLeft.left + 17;
    rcBarRight.right = rcBarLeft.right + 17;
    rcBarLeft.top = rcBarRight.top = 5;
    rcBarLeft.bottom = rcBarRight.bottom = 7;

    /*
     * Draw the "free" bars
     */
    for (i=0; i<nBarsFree; i++)
    {
        FillSolidRect(hDC, &rcBarLeft, DARK_GREEN);
        FillSolidRect(hDC, &rcBarRight, DARK_GREEN);

        rcBarLeft.top += 3;
        rcBarLeft.bottom += 3;

        rcBarRight.top += 3;
        rcBarRight.bottom += 3;
    }

    /*
     * Draw the "used" bars
     */
    for (i=0; i<nBarsUsed; i++)
    {
        FillSolidRect(hDC, &rcBarLeft, BRIGHT_GREEN);
        FillSolidRect(hDC, &rcBarRight, BRIGHT_GREEN);

        rcBarLeft.top += 3;
        rcBarLeft.bottom += 3;

        rcBarRight.top += 3;
        rcBarRight.bottom += 3;
    }

    SelectObject(hDC, hOldFont);
}
