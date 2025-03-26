/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Graph Meters.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"

int      nlastBarsUsed = 0;

WNDPROC  OldGraphWndProc;

void     Graph_DrawCpuUsageGraph(HDC hDC, HWND hWnd);
void     Graph_DrawMemUsageGraph(HDC hDC, HWND hWnd);
void     Graph_DrawMemUsageHistoryGraph(HDC hDC, HWND hWnd);

INT_PTR CALLBACK
Graph_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;
    LONG         WindowId;

    switch (message)
    {
    case WM_ERASEBKGND:
        return TRUE;

    /*
     * Filter out mouse & keyboard messages
     */
    /* case WM_APPCOMMAND: */
    case WM_CAPTURECHANGED:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEHOVER:
    case WM_MOUSELEAVE:
    case WM_MOUSEMOVE:
    /* case WM_MOUSEWHEEL: */
    case WM_NCHITTEST:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    /* case WM_NCMOUSEHOVER: */
    /* case WM_NCMOUSELEAVE: */
    case WM_NCMOUSEMOVE:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    /* case WM_NCXBUTTONDBLCLK: */
    /* case WM_NCXBUTTONDOWN: */
    /* case WM_NCXBUTTONUP: */
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    /* case WM_XBUTTONDBLCLK: */
    /* case WM_XBUTTONDOWN: */
    /* case WM_XBUTTONUP: */
    case WM_ACTIVATE:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_GETHOTKEY:
    case WM_HOTKEY:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    case WM_SETHOTKEY:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:

    case WM_NCCALCSIZE:
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        WindowId = GetWindowLongPtrW(hWnd, GWLP_ID);

        switch (WindowId)
        {
        case IDC_CPU_USAGE_GRAPH:
            Graph_DrawCpuUsageGraph(hdc, hWnd);
            break;
        case IDC_MEM_USAGE_GRAPH:
            Graph_DrawMemUsageGraph(hdc, hWnd);
            break;
        case IDC_MEM_USAGE_HISTORY_GRAPH:
            Graph_DrawMemUsageHistoryGraph(hdc, hWnd);
            break;
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    /*
     * Pass on all non-handled messages
     */
    return CallWindowProcW(OldGraphWndProc, hWnd, message, wParam, lParam);
}

void Graph_DrawCpuUsageGraph(HDC hDC, HWND hWnd)
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

    /*
     * Get the client area rectangle
     */
    GetClientRect(hWnd, &rcClient);

    /*
     * Fill it with blackness
     */
    FillSolidRect(hDC, &rcClient, RGB(0, 0, 0));

    /*
     * Get the CPU usage
     */
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
     * Draw the graph. So first find out how many bars we can fit
     */
    nBars = ((rcClient.bottom - rcClient.top) - 25) / 3;
    nBarsUsed = (nBars * CpuUsage) / 100;
    if ((CpuUsage) && (nBarsUsed == 0))
    {
        nBarsUsed = 1;
    }
    nBarsFree = nBars - (nlastBarsUsed>nBarsUsed ? nlastBarsUsed : nBarsUsed);

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
     * Draw the bar graph
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
    if ((nlastBarsUsed - nBarsUsed) > 0) {
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

void Graph_DrawMemUsageGraph(HDC hDC, HWND hWnd)
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

    /*
     * Get the client area rectangle
     */
    GetClientRect(hWnd, &rcClient);

    /*
     * Fill it with blackness
     */
    FillSolidRect(hDC, &rcClient, RGB(0, 0, 0));

    /*
     * Get the memory usage
     */
    CommitChargeTotal = (ULONGLONG)PerfDataGetCommitChargeTotalK();
    CommitChargeLimit = (ULONGLONG)PerfDataGetCommitChargeLimitK();

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
     * Draw the graph. So first find out how many bars we can fit
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
     * Draw the bar graph
     */
    rcBarLeft.left =  ((rcClient.right - rcClient.left) - 33) / 2;
    rcBarLeft.right =  rcBarLeft.left + 16;
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

void Graph_DrawMemUsageHistoryGraph(HDC hDC, HWND hWnd)
{
    RECT        rcClient;
    //ULONGLONG   CommitChargeLimit;
    int         i;
    static int  offset = 0;

    if (offset++ >= 10)
        offset = 0;

    /*
     * Get the client area rectangle
     */
    GetClientRect(hWnd, &rcClient);

    /*
     * Fill it with blackness
     */
    FillSolidRect(hDC, &rcClient, RGB(0, 0, 0));

    /*
     * Get the memory usage
     */
    //CommitChargeLimit = (ULONGLONG)PerfDataGetCommitChargeLimitK();

    /*
     * Draw the graph background and horizontal bars
     */
    for (i=0; i<rcClient.bottom; i++)
    {
        if ((i % 11) == 0)
        {
            //FillSolidRect2(hDC, 0, i, rcClient.right, 1, DARK_GREEN);
        }
    }

    /*
     * Draw the vertical bars
     */
    for (i=11; i<rcClient.right + offset; i++)
    {
        if ((i % 11) == 0)
        {
            //FillSolidRect2(hDC, i - offset, 0, 1, rcClient.bottom, DARK_GREEN);
        }
    }

    /*
     * Draw the memory usage
     */
    for (i=rcClient.right; i>=0; i--)
    {
    }
}
