/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Graph Meters.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2022 Hermès Bélusca-Maïto
 */

#include "precomp.h"

/* Snapshot of instantaneous values */
ULONGLONG Meter_CommitChargeTotal = 0;
ULONGLONG Meter_CommitChargeLimit = 0;

INT_PTR CALLBACK
Graph_DrawUsageGraph(HWND hWnd, PTM_GAUGE_CONTROL gauge, WPARAM wParam, LPARAM lParam)
{
    RECT      rcClient;
    RECT      rcBarLeft;
    RECT      rcBarRight;
    RECT      rcText;
    COLORREF  crPrevForeground;
    HFONT     hOldFont;
    WCHAR     Text[260];

/* Total number of bars */
    int nBars;
/* Bottom bars that are "used", i.e. are bright green, representing used CPU time / used memory */
    int nBarsUsed = 0;
/* Bottom bars that are "used", i.e. are bright green, representing used CPU kernel time */
    int nBarsUsedKernel;
/* Top bars that are "unused", i.e. are dark green, representing free CPU time / free memory */
    int nBarsFree;

    int i;

    HDC hDC = (HDC)wParam;

    /* Get the client area rectangle */
    GetClientRect(hWnd, &rcClient);

    /* Fill it with blackness */
    FillSolidRect(hDC, &rcClient, RGB(0, 0, 0));

    if (gauge->bIsCPU)
    {
        /* Show the CPU usage */
        wsprintfW(Text, L"%lu%%", gauge->Current1);
    }
    else
    {
        /* Show the memory usage */
        if (Meter_CommitChargeTotal > 1024)
            wsprintfW(Text, L"%lu MB", (ULONG)(Meter_CommitChargeTotal / 1024));
        else
            wsprintfW(Text, L"%lu K", (ULONG)Meter_CommitChargeTotal);
    }

    /*
     * Draw the text onto the graph
     */
    rcText = rcClient;
    InflateRect(&rcText, -2, -2);
    crPrevForeground = SetTextColor(hDC, BRIGHT_GREEN);
    hOldFont = SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(hDC, Text, -1, &rcText, DT_BOTTOM | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);
    SelectObject(hDC, hOldFont);
    SetTextColor(hDC, crPrevForeground);

    /*
     * Now we have to draw the graph.
     * So first find out how many bars we can fit.
     */
    nBars = ((rcClient.bottom - rcClient.top) - 25) / 3;
    nBarsUsed = (nBars * gauge->Current1) / 100;
    if ((gauge->Current1 > 0) && (nBarsUsed == 0))
        nBarsUsed = 1;

    if (gauge->bIsCPU)
    {
        nBarsFree = nBars - max(nBarsUsed, gauge->nlastBarsUsed);

        if (TaskManagerSettings.ShowKernelTimes)
            nBarsUsedKernel = (nBars * gauge->Current2) / 100;
        else
            nBarsUsedKernel = 0;

        nBarsUsedKernel = min(max(nBarsUsedKernel, 0), nBars);
    }
    else
    {
        nBarsFree = nBars - nBarsUsed;
    }

    nBarsUsed = min(max(nBarsUsed, 0), nBars);
    nBarsFree = min(max(nBarsFree, 0), nBars);

    /*
     * Draw the bar graph
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
    for (i = 0; i < nBarsFree; i++)
    {
        FillSolidRect(hDC, &rcBarLeft, DARK_GREEN);
        FillSolidRect(hDC, &rcBarRight, DARK_GREEN);

        rcBarLeft.top += 3;
        rcBarLeft.bottom += 3;

        rcBarRight.top += 3;
        rcBarRight.bottom += 3;
    }

#if 1 // TEST

    if (gauge->bIsCPU)
    {
        /*
         * Draw the last "used" bars
         */
        if ((gauge->nlastBarsUsed - nBarsUsed) > 0)
        {
            for (i = 0; i < (gauge->nlastBarsUsed - nBarsUsed); i++)
            {
                if (gauge->nlastBarsUsed > 5000)
                    gauge->nlastBarsUsed = 5000;

                FillSolidRect(hDC, &rcBarLeft, MEDIUM_GREEN);
                FillSolidRect(hDC, &rcBarRight, MEDIUM_GREEN);

                rcBarLeft.top += 3;
                rcBarLeft.bottom += 3;

                rcBarRight.top += 3;
                rcBarRight.bottom += 3;
            }
        }
        gauge->nlastBarsUsed = nBarsUsed;
    }

    /*
     * Draw the "used" bars
     */
    for (i = 0; i < nBarsUsed; i++)
    {
        if (gauge->bIsCPU)
        {
            if (nBarsUsed > 5000)
                nBarsUsed = 5000;
        }

        FillSolidRect(hDC, &rcBarLeft, BRIGHT_GREEN);
        FillSolidRect(hDC, &rcBarRight, BRIGHT_GREEN);

        rcBarLeft.top += 3;
        rcBarLeft.bottom += 3;

        rcBarRight.top += 3;
        rcBarRight.bottom += 3;
    }

    if (gauge->bIsCPU)
    {
        /*
         * Draw the "used" kernel bars
         */

        rcBarLeft.top -=3;
        rcBarLeft.bottom -=3;

        rcBarRight.top -=3;
        rcBarRight.bottom -=3;

        for (i = 0; i < nBarsUsedKernel; i++)
        {
            FillSolidRect(hDC, &rcBarLeft, RED);
            FillSolidRect(hDC, &rcBarRight, RED);

            rcBarLeft.top -=3;
            rcBarLeft.bottom -=3;

            rcBarRight.top -=3;
            rcBarRight.bottom -=3;
        }
    }

#endif

    return 0;
}
