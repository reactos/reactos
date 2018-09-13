//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       perfpage.cpp
//
//  History:    Nov-10-95   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"

// Size macros for the bitmap we use to draw LED digits

#define DIGITS_STRIP_WIDTH  96
#define DIGITS_STRIP_HEIGHT 11
#define DIGIT_WIDTH         8
#define DIGIT_HEIGHT        (DIGITS_STRIP_HEIGHT)
#define PERCENT_SIGN_INDEX  10  // Index into bitmap strip where % sign lives
#define K_INDEX             11  // Index into bitmap strip where "K" lives
#define BLANK_INDEX         12  // Index into bitmap where blank digit lives

#define GRAPH_BRUSH         BLACK_BRUSH
#define GRAPH_LINE_COLOR    RGB(0, 128, 64)

#define STRIP_HEIGHT        75
#define STRIP_WIDTH         33

LARGE_INTEGER       PreviousCPUIdleTime[MAX_PROCESSOR] = {0 ,0};
LARGE_INTEGER       PreviousCPUTotalTime[MAX_PROCESSOR] = {0 ,0};
LARGE_INTEGER       PreviousCPUKernelTime[MAX_PROCESSOR] = {0 ,0};

LPBYTE              g_pCPUHistory[MAX_PROCESSOR] = { NULL };
LPBYTE              g_pKernelHistory[MAX_PROCESSOR] = { NULL };
LPBYTE              g_pMEMHistory = NULL;

BYTE                g_CPUUsage = 0;
BYTE                g_KernelUsage = 0;
DWORD               g_MEMUsage = 0;
DWORD               g_MEMMax   = 0;

DWORD               g_PageSize;


/*++ CPerfPage::SizePerfPage

Routine Description:

    Sizes its children based on the size of the
    tab control on which it appears.  Welcome to
    sizing hell.

Arguments:

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

static const INT aPerfControls[] =
{
    IDC_STATIC1,
    IDC_STATIC2,
    IDC_STATIC3,
    IDC_STATIC4,
    IDC_STATIC5,
    IDC_STATIC6,
    IDC_STATIC8,
    IDC_STATIC9,
    IDC_STATIC10,
    IDC_STATIC11,
    IDC_STATIC12,
    IDC_STATIC13,
    IDC_STATIC14,
    IDC_STATIC15,
    IDC_STATIC16,
    IDC_STATIC17,
    IDC_TOTAL_PHYSICAL,
    IDC_AVAIL_PHYSICAL,
    IDC_FILE_CACHE,
    IDC_COMMIT_TOTAL,
    IDC_COMMIT_LIMIT,
    IDC_COMMIT_PEAK,
    IDC_KERNEL_TOTAL,
    IDC_KERNEL_PAGED,
    IDC_KERNEL_NONPAGED,
    IDC_TOTAL_HANDLES,
    IDC_TOTAL_THREADS,
    IDC_TOTAL_PROCESSES,
};

// Amount of spacing down from the top of a group box to the
// control it contains

void CPerfPage::SizePerfPage()
{
    // Get the coords of the tab control

    RECT rcParent;

    if (g_Options.m_fNoTitle)
    {
        GetClientRect(g_hMainWnd, &rcParent);
    }
    else
    {
        GetClientRect(m_hwndTabs, &rcParent);
        MapWindowPoints(m_hwndTabs, m_hPage, (LPPOINT) &rcParent, 2);
        TabCtrl_AdjustRect(m_hwndTabs, FALSE, &rcParent);
    }

    HDWP hdwp = BeginDeferWindowPos(10);

    // Calc the deltas in the x and y positions that we need to
    // move each of the child controls

    RECT rcMaster;
    HWND hwndMaster = GetDlgItem(m_hPage, IDC_STATIC5);
    GetWindowRect(hwndMaster, &rcMaster);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcMaster, 2);

    INT dy = ((rcParent.bottom - g_DefSpacing * 2) - rcMaster.bottom);

    // Move each of the child controls by the above delta

    for (int i = 0; i < ARRAYSIZE(aPerfControls); i++)
    {
        HWND hwndCtrl = GetDlgItem(m_hPage, aPerfControls[i]);
        RECT rcCtrl;
        GetWindowRect(hwndCtrl, &rcCtrl);
        MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcCtrl, 2);

        DeferWindowPos(hdwp, hwndCtrl, NULL,
                         rcCtrl.left,
                         rcCtrl.top + dy,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    HWND hwndTopFrame = GetDlgItem(m_hPage, IDC_STATIC13);
    RECT rcTopFrame;
    GetWindowRect(hwndTopFrame, &rcTopFrame);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcTopFrame, 2);
    INT yTop = rcTopFrame.top + dy;

    INT yHist;
    if (g_Options.m_fNoTitle)
    {
        yHist = rcParent.bottom - rcParent.top - g_DefSpacing * 2;
    }
    else
    {
        yHist = (yTop - g_DefSpacing * 3) / 2;
    }

    // Size the CPU history frame

    RECT rcFrame;
    HWND hwndFrame = GetDlgItem(m_hPage, IDC_CPUFRAME);
    GetWindowRect(hwndFrame, &rcFrame);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcFrame, 2);

    DeferWindowPos(hdwp, hwndFrame, NULL, 0, 0,
                     (rcParent.right - rcFrame.left) - g_DefSpacing * 2,
                     yHist,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Size the CPU bar graph frame


    RECT rcCPUFrame;
    HWND hwndCPUFrame = GetDlgItem(m_hPage, IDC_STATIC);
    GetWindowRect(hwndCPUFrame, &rcCPUFrame);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcCPUFrame, 2);

    DeferWindowPos(hdwp, hwndCPUFrame, NULL, 0, 0,
                     (rcCPUFrame.right - rcCPUFrame.left),
                     yHist,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    RECT rcCPUBAR;
    HWND hwndCPUBAR = GetDlgItem(m_hPage, IDC_CPUMETER);
    GetWindowRect(hwndCPUBAR, &rcCPUBAR);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcCPUBAR, 2);

    DeferWindowPos(hdwp, hwndCPUBAR, NULL, rcCPUFrame.left + g_InnerSpacing * 2, rcCPUFrame.top + g_TopSpacing,
                     (rcCPUBAR.right - rcCPUBAR.left),
                     yHist - g_TopSpacing - g_InnerSpacing * 2 ,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    // Size the mem bar graph frame


    RECT rcMEMFrame;
    HWND hwndMEMFrame = GetDlgItem(m_hPage, IDC_MEMBARFRAME);
    GetWindowRect(hwndMEMFrame, &rcMEMFrame);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcMEMFrame, 2);

    DeferWindowPos(hdwp, hwndMEMFrame, NULL, rcMEMFrame.left, yHist + g_DefSpacing * 2,
                     (rcMEMFrame.right - rcMEMFrame.left),
                     yHist,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    RECT rcMEMBAR;
    HWND hwndMEMBAR = GetDlgItem(m_hPage, IDC_MEMMETER);
    GetWindowRect(hwndMEMBAR, &rcMEMBAR);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcMEMBAR, 2);

    DeferWindowPos(hdwp, hwndMEMBAR, NULL, rcMEMBAR.left, yHist + g_DefSpacing * 2 + g_TopSpacing,
                     (rcMEMBAR.right - rcMEMBAR.left),
                     yHist - g_InnerSpacing * 2  - g_TopSpacing,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    // Size the Memory history frame

    RECT rcMemFrame;
    HWND hwndMemFrame = GetDlgItem(m_hPage, IDC_MEMFRAME);
    GetWindowRect(hwndMemFrame, &rcMemFrame);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcMemFrame, 2);

    DeferWindowPos(hdwp, hwndMemFrame, NULL, rcMemFrame.left, yHist + g_DefSpacing * 2,
                     (rcParent.right - rcMemFrame.left) - g_DefSpacing * 2,
                     yHist,
                     SWP_NOZORDER | SWP_NOACTIVATE);


    // We have N panes, where N is 1 or g_cProcessors depending on what mode the
    // cpu meter is currently in

    INT  cPanes = (CM_PANES == g_Options.m_cmHistMode) ? g_cProcessors : 1;

    // Total amount of room available for all of the panes

    INT   Width = (rcParent.right - rcParent.left) - (rcFrame.left - rcParent.left) - g_DefSpacing * 2
                  - g_InnerSpacing * 3;

    // Use this width to size the memory graph

    HWND hwndButton = GetDlgItem(m_hPage, IDC_MEMGRAPH);
    RECT rcButton;
    GetWindowRect(hwndButton, &rcButton);
    MapWindowPoints(NULL, m_hPage, (LPPOINT) &rcButton, 2);

    DeferWindowPos(hdwp, hwndButton, NULL, rcFrame.left + g_InnerSpacing * 2,
                     yHist + g_DefSpacing * 2 + g_TopSpacing,
                     Width - g_InnerSpacing,
                     yHist - g_InnerSpacing * 2  - g_TopSpacing,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    // Total amount of room available for each CPU pane

    Width -= cPanes * g_InnerSpacing;
    Width /= cPanes;
    Width = Width >= 0 ? Width : 0;

    for (i = 0; i < cPanes; i++)
    {
        HWND hwndButton = GetDlgItem(m_hPage, IDC_CPUGRAPH + i);

        INT left = rcFrame.left + g_InnerSpacing * (i + 2) + Width * i;

        DeferWindowPos(hdwp,
                       hwndButton,
                       NULL,
                       left, rcFrame.top + g_TopSpacing,
                       Width,
                       yHist - g_InnerSpacing * 2 -g_TopSpacing,
#if 0
/* the following code doesn't make any sense... Either way, 0 is passed in */
                       SWP_NOZORDER | SWP_NOACTIVATE | i == 0 ? 0 : 0);  // Don't move first button
#else
                       0 );                                         // Don't move first button

#endif

    }

    // Create new bitmaps to be used in the history windows

    EndDeferWindowPos(hdwp);

    GetClientRect(hwndButton, &rcButton);
    FreeMemoryBitmaps();        // Free any old ones
    CreateMemoryBitmaps(rcButton.right - rcButton.left, rcButton.bottom - rcButton.top);

}

/*++ CPerfPage::CreatePens

Routine Description:

    Creates 8 different colors pens, saves them in
    the pen array

Arguments:

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

static const COLORREF aColors[] =
{
    RGB(000, 255, 000),
    RGB(255, 000, 000),
    RGB(255, 000, 255),
    RGB(000, 000, 255),
    RGB(000, 255, 255),
    RGB(255, 128, 000),
    RGB(255, 000, 255),
    RGB(000, 128, 255),

    // End of CPU pens

#define MEM_PEN 8

    RGB(255, 255, 0),

};

void CPerfPage::CreatePens()
{
    for (int i = 0; i < ARRAYSIZE(aColors); i++)
    {
        // Create then pen.  If a failure occurs, just substitute
        // the white pen

        m_hPens[i] = CreatePen(PS_SOLID, 1, aColors[i]);
        if (NULL == m_hPens[i])
        {
            m_hPens[i] = (HPEN) GetStockObject(WHITE_PEN);
        }
    }
}

void CPerfPage::ReleasePens()
{
    for (int i = 0; i < NUM_PENS; i++)
    {
        if (m_hPens[i])
        {
            DeleteObject(m_hPens[i]);
        }
    }
}

/*++ CPerfPage::DrawGraphPaper

Routine Description:

    Draws a graph-paper-like grid into a memory bitmap

Arguments:

    hdcGraph    - HDC to draw into
    prcGraph    - RECT describing area to draw
    Width       - Amount, on right side, to actually draw

Revision History:

      Jan-17-95 Davepl  Created

--*/

static int g_Scrollamount = 0;

void DrawGraphPaper(HDC hdcGraph, RECT * prcGraph, int Width)
{
    #define GRAPHPAPERSIZE 12

    int Leftside = prcGraph->right - Width;

    // Only one of the many graphs needs to ask us to scroll

    HPEN hPen = CreatePen(PS_SOLID, 1, GRAPH_LINE_COLOR);

    HGDIOBJ hOld = SelectObject(hdcGraph, hPen);

    for (int i = GRAPHPAPERSIZE - 1; i < prcGraph->bottom - prcGraph->top; i+= GRAPHPAPERSIZE)
    {
        MoveToEx(hdcGraph,
                 Leftside,
                 i + prcGraph->top,
                 (LPPOINT) NULL);

        LineTo(hdcGraph,
               prcGraph->right,
               i + prcGraph->top);
    }

    for (i = prcGraph->right - g_Scrollamount; i > Leftside; i -= GRAPHPAPERSIZE)
    {
        MoveToEx(hdcGraph,
                 i,
                 prcGraph->top,
                 (LPPOINT) NULL);

        LineTo(hdcGraph,
               i,
               prcGraph->bottom);
    }

    if (hOld)
    {
        SelectObject(hdcGraph, hOld);
    }

    DeleteObject(hPen);
}

/*++ CPerfPage::TextToLegend

Routine Description:

    Given the DC, an X, a Y, and a string, writes that string out
    to the legend and returns the new X.

Return Value:

    New X pos in the legend

Revision History:

      Jan-18-95 Davepl  Created

--*/

int CPerfPage::TextToLegend(HDC hDC, int xPos, int yPos, LPCTSTR szCPUName)
{
    int cchText = lstrlen(szCPUName);
    TextOut(hDC, xPos, yPos, szCPUName, cchText);

    SIZE size;
    GetTextExtentPoint32(hDC, szCPUName, cchText, &size);

    return xPos + size.cx;
}


/*++ CPerfPage::DrawCPUGraph

Routine Description:

    Draws the CPU graph (which is an ownerdraw control)

Arguments:

    lpdi    - LPDRAWITEMSTRUCT describing area we need to paint
    iPane   - Pane number to be drawn (ie: which CPU)

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

void CPerfPage::DrawCPUGraph(LPDRAWITEMSTRUCT lpdi, UINT iPane)
{
    #define THISCPU 0

    if (NULL == m_hdcGraph)
    {
        return;
    }

    FillRect(m_hdcGraph, &m_rcGraph, (HBRUSH) GetStockObject(GRAPH_BRUSH));

    int Width = lpdi->rcItem.right - lpdi->rcItem.left;
    int Scale = (Width - 1) / HIST_SIZE;
    if (0 == Scale)
    {
        Scale = 2;
    }

    // Draw the CPU history graph

    DrawGraphPaper(m_hdcGraph, &m_rcGraph, Width);

    int GraphHeight = m_rcGraph.bottom - m_rcGraph.top - 1;
    int GraphWidth  = m_rcGraph.right  - m_rcGraph.left + 1;


    if (g_Options.m_cmHistMode == CM_PANES)
    {
        // Draw the kernel times

        if (g_Options.m_fKernelTimes)
        {
            HGDIOBJ hOld = SelectObject(m_hdcGraph, m_hPens[1]);

            MoveToEx(m_hdcGraph,
                     m_rcGraph.right,
                     m_rcGraph.bottom - (g_pKernelHistory[iPane][0] * GraphHeight) / 100,
                     (LPPOINT) NULL);

            for (int i = 0; i < HIST_SIZE && i * Scale < Width; i++)
            {
                LineTo(m_hdcGraph,
                       m_rcGraph.right - Scale * i,
                       m_rcGraph.bottom - (g_pKernelHistory[iPane][i] * GraphHeight) / 100);
            }

            if (hOld)
                SelectObject(m_hdcGraph, hOld);
        }

        // Draw a particular CPU in its pane

        HGDIOBJ hOld = SelectObject(m_hdcGraph, m_hPens[0]);

        MoveToEx(m_hdcGraph,
                 m_rcGraph.right,
                 m_rcGraph.bottom - (g_pCPUHistory[iPane][0] * GraphHeight) / 100,
                 (LPPOINT) NULL);

        for (int i = 0; i < HIST_SIZE && i * Scale < Width; i++)
        {
            LineTo(m_hdcGraph,
                   m_rcGraph.right - Scale * i,
                   m_rcGraph.bottom - (g_pCPUHistory[iPane][i] * GraphHeight) / 100);
        }

        if (hOld)
            SelectObject(m_hdcGraph, hOld);

    }
    else
    {
        ASSERT(iPane == 0);

        // Draw the kernel times

        if (g_Options.m_fKernelTimes)
        {
            HGDIOBJ hOld = SelectObject(m_hdcGraph, m_hPens[1]);

            DWORD dwSum = 0;

            for (int iCPU = 0; iCPU < g_cProcessors; iCPU++)
            {
                dwSum += g_pKernelHistory[iCPU][0];
            }
            dwSum /= g_cProcessors;

            MoveToEx(m_hdcGraph,
                     m_rcGraph.right,
                     m_rcGraph.bottom - (dwSum * GraphHeight) / 100,
                     (LPPOINT) NULL);

            for (int i = 0; i < HIST_SIZE && i * Scale < Width; i++)
            {
                dwSum = 0;

                for (iCPU = 0; iCPU < g_cProcessors; iCPU++)
                {
                    dwSum += g_pKernelHistory[iCPU][i];
                }

                dwSum /= g_cProcessors;

                LineTo(m_hdcGraph,
                       m_rcGraph.right - Scale * i,
                       m_rcGraph.bottom - (dwSum * GraphHeight) / 100);
            }

            if (hOld)
                SelectObject(m_hdcGraph, hOld);
        }

        // Draw History as a sum of all CPUs

        HGDIOBJ hOld = SelectObject(m_hdcGraph, m_hPens[0]);

        DWORD dwSum = 0;

        for (int iCPU = 0; iCPU < g_cProcessors; iCPU++)
        {
            dwSum += g_pCPUHistory[iCPU][0];
        }
        dwSum /= g_cProcessors;

        MoveToEx(m_hdcGraph,
                 m_rcGraph.right,
                 m_rcGraph.bottom - (dwSum * GraphHeight) / 100,
                 (LPPOINT) NULL);

        for (int i = 0; i < HIST_SIZE && i * Scale < Width; i++)
        {
            dwSum = 0;

            for (iCPU = 0; iCPU < g_cProcessors; iCPU++)
            {
                dwSum += g_pCPUHistory[iCPU][i];
            }

            dwSum /= g_cProcessors;

            LineTo(m_hdcGraph,
                   m_rcGraph.right - Scale * i,
                   m_rcGraph.bottom - (dwSum * GraphHeight) / 100);
        }

        if (hOld)
            SelectObject(m_hdcGraph, hOld);

    }

    // Memory bitmap could be wider than the target control, so find a delta

    INT xDiff = (m_rcGraph.right - m_rcGraph.left) - (lpdi->rcItem.right - lpdi->rcItem.left);

    BitBlt( lpdi->hDC,
            lpdi->rcItem.left,
            lpdi->rcItem.top,
            lpdi->rcItem.right - lpdi->rcItem.left,
            lpdi->rcItem.bottom - lpdi->rcItem.top,
            m_hdcGraph,
            xDiff,
            0,
            SRCCOPY);
}

/*++ CPerfPage::DrawMEMGraph

Routine Description:

    Draws the Memory history graph (which is an ownerdraw control)

Arguments:

    lpdi - LPDRAWITEMSTRUCT describing area we need to paint

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

void CPerfPage::DrawMEMGraph(LPDRAWITEMSTRUCT lpdi)
{
    #define THISCPU 0

    if (NULL == m_hdcGraph)
    {
        return;
    }

    FillRect(m_hdcGraph, &m_rcGraph, (HBRUSH) GetStockObject(GRAPH_BRUSH));

    int Width = lpdi->rcItem.right - lpdi->rcItem.left;

    DrawGraphPaper(m_hdcGraph, &m_rcGraph, Width);

    int Scale = (Width - 1) / HIST_SIZE;
    if (0 == Scale)
    {
        Scale = 2;
    }

    int GraphHeight = m_rcGraph.bottom - m_rcGraph.top - 1;

    HGDIOBJ hOld = SelectObject(m_hdcGraph, m_hPens[MEM_PEN]);

    MoveToEx(m_hdcGraph,
             m_rcGraph.right,
             m_rcGraph.bottom - (g_pMEMHistory[0] * GraphHeight) / 100,
             (LPPOINT) NULL);

    for (int i = 0; i < HIST_SIZE && i * Scale < Width - 1; i++)
    {
        if (0 == g_pMEMHistory[i])
        {
            break;  // End of Data
        }

        LineTo(m_hdcGraph,
               m_rcGraph.right - Scale * i,
               m_rcGraph.bottom - (g_pMEMHistory[i] * GraphHeight) / 100);
    }

    BitBlt( lpdi->hDC,
            lpdi->rcItem.left,
            lpdi->rcItem.top,
            lpdi->rcItem.right - lpdi->rcItem.left,
            lpdi->rcItem.bottom - lpdi->rcItem.top,
            m_hdcGraph,
            0,
            0,
            SRCCOPY);

    if (hOld)
        SelectObject(m_hdcGraph, hOld);

}

/*++ CPerfPage::UpdateGraphs

Routine Description:

    Adds and removed CPU panes as required

Arguments:

    none

Return Value:

    none

Revision History:

    Dec-16-96   Davepl  Create

***/

// WARNING: CPU pane IDs must be contiguous in resource.h

#define ID_FIRSTPANE IDC_CPUGRAPH
#define ID_LASTPANE  IDC_CPUGRAPH32
#define NUMPANES     32

void CPerfPage::UpdateGraphs()
{
    // WARNING: CPU pane IDs must be contiguous in resource.h

    ASSERT(ID_LASTPANE - ID_FIRSTPANE == (NUMPANES - 1));

    // Turn the extra windows on or off as required

    for (UINT i = 1; i < g_cProcessors && i < NUMPANES; i++)
    {
        ShowWindow(GetDlgItem(m_hPage, ID_FIRSTPANE + i),
                   CM_PANES == g_Options.m_cmHistMode ? SW_SHOW : SW_HIDE);
    }

    // Hide/show everything but the CPU meters when we're in notitle/title mode

    for (i = 0; i < ARRAYSIZE(aPerfControls); i++)
    {
        ShowWindow(GetDlgItem(m_hPage, aPerfControls[i]), g_Options.m_fNoTitle ? SW_HIDE : SW_SHOW);
    }

    ShowWindow(GetDlgItem(m_hPage, IDC_MEMGRAPH), g_Options.m_fNoTitle ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(m_hPage, IDC_MEMFRAME), g_Options.m_fNoTitle ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(m_hPage, IDC_MEMBARFRAME), g_Options.m_fNoTitle ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(m_hPage, IDC_MEMMETER), g_Options.m_fNoTitle ? SW_HIDE : SW_SHOW);

    SizePerfPage();
}

/*++ CPerfPage::DrawCPUDigits

Routine Description:

    Draws the CPU meter and digits

Arguments:

    lpdi - LPDRAWITEMSTRUCT describing area we need to paint

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

void CPerfPage::DrawCPUDigits(LPDRAWITEMSTRUCT lpdi)
{
    int cDigits = 3;
    HBRUSH hBlack = (HBRUSH) GetStockObject(BLACK_BRUSH);
    HGDIOBJ hOld = SelectObject(lpdi->hDC, hBlack);
    Rectangle(lpdi->hDC, lpdi->rcItem.left, lpdi->rcItem.top, lpdi->rcItem.right, lpdi->rcItem.bottom);

    // Draw the digits into the ownder draw control

    INT xOffset = ((lpdi->rcItem.right - lpdi->rcItem.left) - 4 * DIGIT_WIDTH) / 2 - 2;
    INT yOffset = (lpdi->rcItem.bottom - DIGIT_HEIGHT - g_DefSpacing);
    INT xBarOffset = ((lpdi->rcItem.right - lpdi->rcItem.left) - STRIP_WIDTH) / 2;

    RECT rcBar;
    GetWindowRect(GetDlgItem(m_hPage, IDC_MEMMETER), &rcBar);
    INT cBarHeight = lpdi->rcItem.bottom - lpdi->rcItem.top - (DIGIT_HEIGHT + g_DefSpacing * 3);
    if (cBarHeight <= 0)
    {
        return;
    }

    INT ctmpBarLitPixels = (g_CPUUsage * cBarHeight) / 100;
    INT ctmpBarRedPixels = g_Options.m_fKernelTimes ? ctmpBarRedPixels = (g_KernelUsage * cBarHeight) / 100 : 0;

    INT cBarUnLitPixels = cBarHeight - ctmpBarLitPixels;
        cBarUnLitPixels = (cBarUnLitPixels / 3) * 3;

    INT cBarLitPixels = cBarHeight - cBarUnLitPixels;
    INT cBarRedPixels = ctmpBarRedPixels;

    HDC hdcMem = CreateCompatibleDC(lpdi->hDC);
    if (hdcMem)
    {
        HBITMAP hOldbmp = (HBITMAP) SelectObject(hdcMem, m_hDigits);
        if (hOldbmp)
        {
            int Place = 100;
            int Value = g_CPUUsage;
            BOOL fDrawnYet = FALSE;

            for (int i = 0; i < 3; i++)
            {
                // Don't zero-pad

                if (Value / Place == 0 && fDrawnYet == FALSE && Place != 1)
                {
                    BitBlt(lpdi->hDC, xOffset + DIGIT_WIDTH * i, yOffset,
                                      DIGIT_WIDTH, DIGIT_HEIGHT,
                                      hdcMem,
                                      BLANK_INDEX * DIGIT_WIDTH, 0,
                                      SRCCOPY);
                }
                else
                {
                    BitBlt(lpdi->hDC, xOffset + DIGIT_WIDTH * i, yOffset,
                                      DIGIT_WIDTH, DIGIT_HEIGHT,
                                      hdcMem,
                                      (Value / Place) * DIGIT_WIDTH, 0,
                                      SRCCOPY);
                    if (Value / Place)
                    {
                        fDrawnYet = TRUE;
                    }
                }
                Value %= Place;
                Place /= 10;

            };

            // Percent sign

            BitBlt(lpdi->hDC, xOffset + cDigits * DIGIT_WIDTH, yOffset,
                              DIGIT_WIDTH, DIGIT_HEIGHT,
                              hdcMem,
                              PERCENT_SIGN_INDEX * DIGIT_WIDTH, 0, SRCCOPY);

            // Draw the CPU meter

            // Draw unlit portion

            if (cBarHeight != cBarLitPixels)
            {
                INT cUnlit = cBarHeight - cBarLitPixels;
                INT cOffset = 0;
                SelectObject(hdcMem, m_hStripUnlit);

                while (cUnlit > 0)
                {
                    BitBlt(lpdi->hDC, xBarOffset, g_DefSpacing + cOffset,
                                      STRIP_WIDTH, min(cUnlit, STRIP_HEIGHT),
                                      hdcMem,
                                      0, 0, SRCCOPY);
                    cOffset += min(cUnlit, STRIP_HEIGHT);
                    cUnlit -= min(cUnlit, STRIP_HEIGHT);
                }
            };

            // Draw lit portion

            if (0 != cBarLitPixels)
            {
                SelectObject(hdcMem, m_hStripLit);
                INT cOffset = 0;
                INT cLit = cBarLitPixels - cBarRedPixels;

                while (cLit > 0)
                {
                    BitBlt(lpdi->hDC, xBarOffset, g_DefSpacing + (cBarHeight - cBarLitPixels) + cOffset,
                                      STRIP_WIDTH, min(STRIP_HEIGHT, cLit),
                                      hdcMem,
                                      0, 0, SRCCOPY);
                    cOffset += min(cLit, STRIP_HEIGHT);
                    cLit -= min(cLit, STRIP_HEIGHT);
                }

            }

            if (0 != cBarRedPixels)
            {
                SelectObject(hdcMem, m_hStripLitRed);
                INT cOffset = 0;
                INT cRed = cBarRedPixels;

                while (cRed > 0)
                {
                    BitBlt(lpdi->hDC, xBarOffset, g_DefSpacing + (cBarHeight - cBarRedPixels) + cOffset,
                                      STRIP_WIDTH, min(cRed, STRIP_HEIGHT),
                                      hdcMem,
                                      0, 0, SRCCOPY);
                    cOffset += min(cRed, STRIP_HEIGHT);
                    cRed -= min(cRed, STRIP_HEIGHT);
                }
            }

            SelectObject(hdcMem, hOldbmp);
        }
        DeleteDC(hdcMem);
    }
    SelectObject(lpdi->hDC, hOld);
}

// CPerfPage::DrawMEMMeter
//
// Draws the memory meter

void CPerfPage::DrawMEMMeter(LPDRAWITEMSTRUCT lpdi)
{
    int cDigits = 6;
	int Place = 100000;

    BOOL fNoSymbol = FALSE;

    // For systems with >1G of memory, we add another digit and
    // skip the K symbol to make room for it.

	if (g_MEMMax > 1000000)
    {
        cDigits = 7;
        Place = 1000000;
        fNoSymbol = TRUE;
    }

    HBRUSH hBlack = (HBRUSH) GetStockObject(BLACK_BRUSH);
    HGDIOBJ hOld = SelectObject(lpdi->hDC, hBlack);
    Rectangle(lpdi->hDC, lpdi->rcItem.left, lpdi->rcItem.top, lpdi->rcItem.right, lpdi->rcItem.bottom);

    // Draw the digits into the ownder draw control

    INT xOffset = ((lpdi->rcItem.right - lpdi->rcItem.left) -
                  ((fNoSymbol ? (cDigits) : (cDigits + 1)) * DIGIT_WIDTH)) / 2 ;
    INT yOffset = (lpdi->rcItem.bottom - DIGIT_HEIGHT - g_DefSpacing);
    INT xBarOffset = ((lpdi->rcItem.right - lpdi->rcItem.left) - STRIP_WIDTH) / 2;

    HDC hdcMem = CreateCompatibleDC(lpdi->hDC);
    if (hdcMem)
    {
        HBITMAP hOldbmp = (HBITMAP) SelectObject(hdcMem, m_hDigits);
        if (hOldbmp)
        {
            int Value = g_MEMUsage;
            BOOL fDrawnYet = FALSE;

            for (int i = 0; i < cDigits; i++)
            {
                // Don't zero-pad

                if (Value / Place == 0 && fDrawnYet == FALSE)
                {
                    BitBlt(lpdi->hDC, xOffset + DIGIT_WIDTH * i, yOffset,
                                      DIGIT_WIDTH, DIGIT_HEIGHT,
                                      hdcMem,
                                      BLANK_INDEX * DIGIT_WIDTH, 0,
                                      SRCCOPY);
                }
                else
                {
                    BitBlt(lpdi->hDC, xOffset + DIGIT_WIDTH * i, yOffset,
                                      DIGIT_WIDTH, DIGIT_HEIGHT,
                                      hdcMem,
                                      (Value / Place) * DIGIT_WIDTH, 0,
                                      SRCCOPY);
                    if (Value / Place)
                    {
                        fDrawnYet = TRUE;
                    }
                }
                Value %= Place;
                Place /= 10;

            };

            if (FALSE == fNoSymbol)
            {
                // K

                BitBlt(lpdi->hDC, xOffset + cDigits * DIGIT_WIDTH, yOffset,
                                  DIGIT_WIDTH, DIGIT_HEIGHT,
                                  hdcMem,
                                  K_INDEX * DIGIT_WIDTH, 0, SRCCOPY);
            }

            // Draw the CPU meter

            // Draw unlit portion

            INT cBarHeight = lpdi->rcItem.bottom - lpdi->rcItem.top - (DIGIT_HEIGHT + g_DefSpacing * 3);

            if (cBarHeight > 0)
            {
                INT cBarLitPixels = (g_MEMUsage * cBarHeight) / g_MEMMax;
                    cBarLitPixels = (cBarLitPixels / 3) * 3;
                INT cBarUnlitPixels = cBarHeight - cBarLitPixels;

                if (cBarHeight != cBarLitPixels)
                {
                    SelectObject(hdcMem, m_hStripUnlit);
                    INT cUnlit = cBarHeight - cBarLitPixels;
                    INT cOffset = 0;

                    while (cUnlit > 0)
                    {
                        BitBlt(lpdi->hDC, xBarOffset, g_DefSpacing + cOffset,
                                          STRIP_WIDTH, min(cUnlit, STRIP_HEIGHT),
                                          hdcMem,
                                          0, 0, SRCCOPY);
                        cOffset += min(cUnlit, STRIP_HEIGHT);
                        cUnlit  -= min(cUnlit, STRIP_HEIGHT);
                    }
                };

                // Draw lit portion

                if (0 != cBarLitPixels)
                {
                    SelectObject(hdcMem, m_hStripLit);
                    INT cOffset = 0;
                    INT cLit    = cBarLitPixels;

                    while (cLit > 0)
                    {
                        BitBlt(lpdi->hDC, xBarOffset, g_DefSpacing + (cBarHeight - cBarLitPixels) + cOffset,
                                          STRIP_WIDTH, min(STRIP_HEIGHT, cLit),
                                          hdcMem,
                                          0, 0, SRCCOPY);
                        cOffset += min(cLit, STRIP_HEIGHT);
                        cLit    -= min(cLit, STRIP_HEIGHT);
                    }
                }
            }

            SelectObject(hdcMem, hOldbmp);
        }
        DeleteDC(hdcMem);
    }
    SelectObject(lpdi->hDC, hOld);
}

/*++ CPerfPage::TimerEvent

Routine Description:

    Called by main app when the update time fires

Arguments:

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

void CPerfPage::TimerEvent()
{
    CalcCpuTime(TRUE);

    g_Scrollamount+=2;
    g_Scrollamount %= GRAPHPAPERSIZE;

    // Force the displays to update

	if (FALSE == IsIconic(g_hMainWnd))
	{
		InvalidateRect(GetDlgItem(m_hPage, IDC_CPUMETER), NULL, FALSE);
		UpdateWindow(GetDlgItem(m_hPage, IDC_CPUMETER));
		InvalidateRect(GetDlgItem(m_hPage, IDC_MEMMETER), NULL, FALSE);
		UpdateWindow(GetDlgItem(m_hPage, IDC_MEMMETER));
		
                for (UINT i = 0; i < g_cProcessors; i++)
                {
                    InvalidateRect(GetDlgItem(m_hPage, ID_FIRSTPANE + i), NULL, FALSE);
		    UpdateWindow(GetDlgItem(m_hPage, ID_FIRSTPANE + i));
                    if (CM_PANES != g_Options.m_cmHistMode)
                    {
                        // If we're not in multipane mode, we can stop after the first graph
                        break;
                    }
                }
                		
                InvalidateRect(GetDlgItem(m_hPage, IDC_MEMGRAPH), NULL, FALSE);
		UpdateWindow(GetDlgItem(m_hPage, IDC_MEMGRAPH));
	}
}

/*++ PerfPageProc

Routine Description:

    Dialogproc for the performance page.

Arguments:

    hwnd   	- handle to dialog box
    uMsg	- message
    wParam	- first message parameter
    lParam 	- second message parameter

Return Value:

    For WM_INITDIALOG, TRUE == success
    For others, TRUE == this proc handles the message

Revision History:

      Nov-12-95 Davepl  Created

--*/

INT_PTR CALLBACK PerfPageProc(
                HWND        hwnd,   	        // handle to dialog box
                UINT        uMsg,	            // message
                WPARAM      wParam,	            // first message parameter
                LPARAM      lParam 	            // second message parameter
                )
{
    CPerfPage * thispage = (CPerfPage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // See if the parent wants this message

    if (TRUE == CheckParentDeferrals(uMsg, wParam, lParam))
    {
        return TRUE;
    }

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {

            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            CPerfPage * thispage = (CPerfPage *) lParam;

            DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
            dwStyle |= WS_CLIPCHILDREN;
            SetWindowLong(hwnd, GWL_STYLE, dwStyle);

            if (IS_WINDOW_RTL_MIRRORED(hwnd))
            {
                HWND hItem;
                LONG lExtStyle;

                hItem = GetDlgItem(hwnd,IDC_CPUMETER);
                lExtStyle = GetWindowLong(hItem,GWL_EXSTYLE);
                SetWindowLong(hItem,GWL_EXSTYLE, lExtStyle & ~(RTL_MIRRORED_WINDOW | RTL_NOINHERITLAYOUT));
                hItem = GetDlgItem(hwnd,IDC_MEMMETER);
                lExtStyle = GetWindowLong(hItem,GWL_EXSTYLE);
                SetWindowLong(hItem,GWL_EXSTYLE, lExtStyle & ~(dwExStyleRTLMirrorWnd | dwExStyleNoInheritLayout));
            }

            return TRUE;
        }


        // We need to fake client mouse clicks in this child to appear as nonclient
        // (caption) clicks in the parent so that the user can drag the entire app
        // when the title bar is hidden by dragging the client area of this child

        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        {
            if (g_Options.m_fNoTitle)
            {
                SendMessage(g_hMainWnd,
                            uMsg == WM_LBUTTONUP ? WM_NCLBUTTONUP : WM_NCLBUTTONDOWN,
                            HTCAPTION,
                            lParam);
            }
            break;

        }

        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            SendMessage(g_hMainWnd, uMsg, wParam, lParam);
            break;
        }

        case WM_CTLCOLORBTN:
        {
            // When windows tries to redraw our owner draw controls, tell
            // it to do so with a black background

            if (lParam >= ID_FIRSTPANE && lParam <= ID_LASTPANE)
            {
                // Trivial case: its a CPU graph

                return (INT_PTR) GetStockObject(GRAPH_BRUSH);
            }
            else 
            {
                // Not a CPU graph, maybe its one of the other graphs 

                const static int rgGraphs[] =
                {
                    IDC_MEMGRAPH,
                    IDC_MEMMETER,
                    IDC_CPUMETER
                };

                for (int i = 0; i < ARRAYSIZE(rgGraphs); i++)
                {
                    if (lParam == (LPARAM) GetDlgItem(hwnd, rgGraphs[i]))
                    {
                        return (INT_PTR) GetStockObject(GRAPH_BRUSH);
                    }
                }
            }
            break;
        }

        // Size our kids

        case WM_SIZE:
            thispage->SizePerfPage();
            return FALSE;

        // Draw one of our owner draw controls

        case WM_DRAWITEM:
        {
            if (wParam >= ID_FIRSTPANE && wParam <= ID_LASTPANE)
            {
                thispage->DrawCPUGraph( (LPDRAWITEMSTRUCT) lParam, (UINT)wParam - ID_FIRSTPANE);
                return TRUE;
            }
            else if (IDC_CPUMETER == wParam)
            {
                thispage->DrawCPUDigits( (LPDRAWITEMSTRUCT) lParam);
                return TRUE;
            }
            else if (IDC_MEMMETER == wParam)
            {
                thispage->DrawMEMMeter( (LPDRAWITEMSTRUCT) lParam);
                return TRUE;
            }
            else if (IDC_MEMGRAPH == wParam)
            {
                thispage->DrawMEMGraph( (LPDRAWITEMSTRUCT) lParam);
                return TRUE;
            }
            break;
        }

        default:
            return FALSE;
    }
    return FALSE;
}

/*++ CPerfPage::GetTitle

Routine Description:

    Copies the title of this page to the caller-supplied buffer

Arguments:

    pszText     - the buffer to copy to
    bufsize     - size of buffer, in characters

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

void CPerfPage::GetTitle(LPTSTR pszText, size_t bufsize)
{
    LoadString(g_hInstance, IDS_PERFPAGETITLE, pszText, bufsize);
}

/*++ CPerfPage::Activate

Routine Description:

    Brings this page to the front, sets its initial position,
    and shows it

Arguments:

Return Value:

    HRESULT (S_OK on success)

Revision History:

      Nov-12-95 Davepl  Created

--*/

HRESULT CPerfPage::Activate()
{
    // Adjust the size and position of our dialog relative
    // to the tab control which "owns" us

    RECT rcParent;
    GetClientRect(m_hwndTabs, &rcParent);
    MapWindowPoints(m_hwndTabs, g_hMainWnd, (LPPOINT) &rcParent, 2);
    TabCtrl_AdjustRect(m_hwndTabs, FALSE, &rcParent);

    SetWindowPos(m_hPage,
                 HWND_TOP,
                 rcParent.left, rcParent.top,
                 rcParent.right - rcParent.left, rcParent.bottom - rcParent.top,
                 0);

    // Make this page visible

    ShowWindow(m_hPage, SW_SHOW);

    // Make the CPU graphs visible or invisible depending on its current mode

    UpdateGraphs();

    // Newly created dialogs seem to steal the focus, so give it back to the
    // tab control (which must have had it if we got here in the first place)

    SetFocus(m_hwndTabs);

    // Change the menu bar to be the menu for this page

    HMENU hMenuOld = GetMenu(g_hMainWnd);
    HMENU hMenuNew = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU_PERF));

    if (hMenuNew && SHRestricted(REST_NORUN))
    {
        DeleteMenu(hMenuNew, IDM_RUN, MF_BYCOMMAND);
    }

    g_hMenu = hMenuNew;
    if (g_Options.m_fNoTitle == FALSE)
    {
        SetMenu(g_hMainWnd, hMenuNew);
    }

    if (hMenuOld)
    {
        DestroyMenu(hMenuOld);
    }

    return S_OK;
}

/*++ CPerfPage::Initialize

Routine Description:

    Loads the resources we need for this page, creates the inmemory DCs
    and bitmaps for the charts, and creates the actual window (a dialog)
    that represents this page

Arguments:

    hwndParent  - Parent on which to base sizing on: not used for creation,
                  since the main app window is always used as the parent in
                  order to keep tab order correct

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

HRESULT CPerfPage::Initialize(HWND hwndParent)
{
    // Our pseudo-parent is the tab contrl, and is what we base our
    // sizing on.  However, in order to keep tab order right among
    // the controls, we actually create ourselves with the main
    // window as the parent

    m_hwndTabs = hwndParent;

    // Create the color pens

    CreatePens();

    //
    // Load the LED digits bitmap, etc.
    //

    m_hDigits = (HBITMAP) LoadImage(g_hInstance, MAKEINTRESOURCE(LED_NUMBERS),
                                     IMAGE_BITMAP,
                                     0, 0,
                                     LR_DEFAULTCOLOR);

    m_hStripLit = (HBITMAP) LoadImage(g_hInstance, MAKEINTRESOURCE(LED_STRIP_LIT),
                                     IMAGE_BITMAP,
                                     0, 0,
                                     LR_DEFAULTCOLOR);

    m_hStripLitRed = (HBITMAP) LoadImage(g_hInstance, MAKEINTRESOURCE(LED_STRIP_LIT_RED),
                                     IMAGE_BITMAP,
                                     0, 0,
                                     LR_DEFAULTCOLOR);

    m_hStripUnlit = (HBITMAP) LoadImage(g_hInstance, MAKEINTRESOURCE(LED_STRIP_UNLIT),
                                     IMAGE_BITMAP,
                                     0, 0,
                                     LR_DEFAULTCOLOR);

    if (NULL == m_hDigits)
    {
        return GetLastHRESULT();
    }

    //
    // Create the dialog which represents the body of this page
    //

    m_hPage = CreateDialogParam(
                    g_hInstance,	                // handle to application instance
                    MAKEINTRESOURCE(IDD_PERFPAGE),	// identifies dialog box template name
                    g_hMainWnd,	                    // handle to owner window
                    PerfPageProc,        	// pointer to dialog box procedure
                    (LPARAM) this );                // User data (our this pointer)

    if (NULL == m_hPage)
    {
        return GetLastHRESULT();
    }

    return S_OK;
}

/*++ CPerfPage::CreateMemoryBitmaps

Routine Description:

    Creates the inmemory bitmaps used to draw the history graphics

Arguments:

    x, y    - size of bitmap to create

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

static HGDIOBJ hOld;

HRESULT CPerfPage::CreateMemoryBitmaps(int x, int y)
{
    //
    // Create the inmemory bitmaps and DCs that we will use
    //

    HDC hdcPage = GetDC(m_hPage);
    m_hdcGraph = CreateCompatibleDC(hdcPage);

    if (NULL == m_hdcGraph)
    {
        ReleaseDC(m_hPage, hdcPage);
        return GetLastHRESULT();
    }

    m_rcGraph.left   = 0;
    m_rcGraph.top    = 0;
    m_rcGraph.right  = x;
    m_rcGraph.bottom = y;

    m_hbmpGraph = CreateCompatibleBitmap(hdcPage, x, y);
    ReleaseDC(m_hPage, hdcPage);
    if (NULL == m_hbmpGraph)
    {
        HRESULT hr = GetLastHRESULT();
        DeleteDC(m_hdcGraph);
        m_hdcGraph = NULL;
        return hr;
    }

    // Select the bitmap into the DC

    hOld = SelectObject(m_hdcGraph, m_hbmpGraph);

    return S_OK;
}

/*++ CPerfPage::FreeMemoryBitmaps

Routine Description:

    Frees the inmemory bitmaps used to drag the history graphs

Arguments:

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

void CPerfPage::FreeMemoryBitmaps()
{

    if (m_hdcGraph)
    {
        if (hOld)
           SelectObject(m_hdcGraph, m_hbmpGraph);
        DeleteDC(m_hdcGraph);
    }

    if (m_hbmpGraph)
    {
        DeleteObject(m_hbmpGraph);
    }

}

/*++ CPerfPage::Deactivate

Routine Description:

    Called when this page is losing its place up front

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CPerfPage::Deactivate()
{
    if (m_hPage)
    {
        ShowWindow(m_hPage, SW_HIDE);
    }
}

/*++ CPerfPage::Destroy

Routine Description:

    Frees whatever has been allocated by the Initialize call

Arguments:

Return Value:

Revision History:

      Nov-12-95 Davepl  Created

--*/

HRESULT CPerfPage::Destroy()
{
    //
    // When we are being destroyed, kill off our dialog
    //

    ReleasePens();

    if (m_hPage)
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
    }

    if (m_hDigits)
    {
        DeleteObject(m_hDigits);
        m_hDigits = NULL;
    }

    if (m_hStripLit)
    {
        DeleteObject(m_hStripLit);
        m_hStripLit = NULL;
    }

    if (m_hStripUnlit)
    {
        DeleteObject(m_hStripUnlit);
        m_hStripUnlit = NULL;
    }

    if (m_hStripLitRed)
    {
        DeleteObject(m_hStripLitRed);
        m_hStripLitRed = NULL;
    }

    if (m_hbmpGraph)
    {
        DeleteObject(m_hbmpGraph);
        m_hbmpGraph = NULL;
    }

    if (m_hdcGraph)
    {
        DeleteDC(m_hdcGraph);
        m_hdcGraph = NULL;
    }

    return S_OK;
}


/*++

Routine Description:

    Initialize data for perf measurements

Arguments:

    None

Return Value:

    Number of system processors (0 if error)

Revision History:

      10-13-95  Modified from WPERF

--*/

BYTE InitPerfInfo()
{
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfo[MAX_PROCESSOR];
    int                                         i;

    NTSTATUS Status = NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    g_PageSize = BasicInfo.PageSize;
    g_cProcessors = BasicInfo.NumberOfProcessors;

    if (g_cProcessors > MAX_PROCESSOR) {
        return(0);
    }

    for (i = 0; i < g_cProcessors; i++)
    {
        g_pCPUHistory[i] = (LPBYTE) LocalAlloc(LPTR, HIST_SIZE * sizeof(LPBYTE));
        if (NULL == g_pCPUHistory[i])
        {
            return 0;
        }
        g_pKernelHistory[i] = (LPBYTE) LocalAlloc(LPTR, HIST_SIZE * sizeof(LPBYTE));
        if (NULL == g_pKernelHistory[i])
        {
            return 0;
        }

    }

    g_pMEMHistory = (LPBYTE) LocalAlloc(LPTR, HIST_SIZE * sizeof(LPBYTE));
    if (NULL == g_pMEMHistory)
    {
        return 0;
    }

    Status = NtQuerySystemInformation(
       SystemProcessorPerformanceInformation,
       ProcessorInfo,
       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
       NULL
    );

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    PPerfInfo = ProcessorInfo;


    for (i=0; i < g_cProcessors; i++)
    {
        PreviousCPUIdleTime[i]           =  PPerfInfo->IdleTime;
        PreviousCPUTotalTime[i].QuadPart =  PPerfInfo->UserTime.QuadPart +
                                            PPerfInfo->KernelTime.QuadPart;
        PreviousCPUKernelTime[i].QuadPart =  PPerfInfo->KernelTime.QuadPart +
                                             PPerfInfo->IdleTime.QuadPart;

                                            // PPerfInfo->IdleTime.QuadPart;
        PPerfInfo++;
    }

    //
    // Get the maximum commit limit
    //

    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL);

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    g_MEMMax = PerfInfo.CommitLimit * ( g_PageSize / 1024 );

    return(g_cProcessors);
}

/*++ ReleasePerfInfo

Routine Description:

   Frees the history buffers

Arguments:

Return Value:

Revision History:

      Nov-13-95 DavePl  Created

--*/

void ReleasePerfInfo()
{
    for (int i = 0; i < g_cProcessors; i++)
    {
        if (g_pCPUHistory[i])
        {
            LocalFree(g_pCPUHistory[i]);
            g_pCPUHistory[i] = NULL;
        }
        if (g_pKernelHistory[i])
        {
            LocalFree(g_pKernelHistory[i]);
            g_pKernelHistory[i] = NULL;
        }

    }

    if (g_pMEMHistory)
    {
        LocalFree(g_pMEMHistory);
    }
}

/*++ CalcCpuTime

Routine Description:

   calculate and return %cpu time and time periods

Arguments:

   None

Notes:

Revision History:

      Nov-13-95 DavePl  Created

--*/

void CalcCpuTime(BOOL fUpdateHistory)
{
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorInfo[MAX_PROCESSOR];
    LARGE_INTEGER                            CPUIdleTime[MAX_PROCESSOR];
    LARGE_INTEGER                            CPUTotalTime[MAX_PROCESSOR];
    LARGE_INTEGER                            CPUKernelTime[MAX_PROCESSOR];

    LARGE_INTEGER                            SumIdleTime   = { 0 ,0 };
    LARGE_INTEGER                            SumTotalTime  = { 0, 0 };
    LARGE_INTEGER                            SumKernelTime = { 0, 0 };

    NTSTATUS Status;

    Status = NtQuerySystemInformation(
       SystemProcessorPerformanceInformation,
       ProcessorInfo,
       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
       NULL
    );

    if (!NT_SUCCESS(Status))
    {
        return;
    }

    //
    // Walk through the info for each CPU, and compile
    //
    //  - Amount of time each CPU has spent idle (since last check)
    //  - Amount of time each CPU has spent entirely (since last check)
    //
    // In addition to keeping per-CPU stats, compile a sum for
    //
    //  - Amount of time system has spent idle (since last check)
    //  - Amount of time that has elapsed, in total (since last check)
    //

    for (int ListIndex = 0; ListIndex < g_cProcessors; ListIndex++)
    {
        LARGE_INTEGER DeltaCPUIdleTime;
        LARGE_INTEGER DeltaCPUTotalTime;
        LARGE_INTEGER DeltaCPUKernelTime;

        CPUIdleTime[ListIndex].QuadPart  = ProcessorInfo[ListIndex].IdleTime.QuadPart;
        CPUKernelTime[ListIndex].QuadPart= ProcessorInfo[ListIndex].KernelTime.QuadPart-
                                           ProcessorInfo[ListIndex].IdleTime.QuadPart;
        CPUTotalTime[ListIndex].QuadPart = ProcessorInfo[ListIndex].KernelTime.QuadPart +
                                           ProcessorInfo[ListIndex].UserTime.QuadPart;// +
                                           //ProcessorInfo[ListIndex].IdleTime.QuadPart;

        DeltaCPUIdleTime.QuadPart        = CPUIdleTime[ListIndex].QuadPart -
                                           PreviousCPUIdleTime[ListIndex].QuadPart;
        DeltaCPUKernelTime.QuadPart      = CPUKernelTime[ListIndex].QuadPart -
                                           PreviousCPUKernelTime[ListIndex].QuadPart;
        DeltaCPUTotalTime.QuadPart       = CPUTotalTime[ListIndex].QuadPart -
                                           PreviousCPUTotalTime[ListIndex].QuadPart;

        SumIdleTime.QuadPart            += DeltaCPUIdleTime.QuadPart;
        SumTotalTime.QuadPart           += DeltaCPUTotalTime.QuadPart;
        SumKernelTime.QuadPart          += DeltaCPUKernelTime.QuadPart;

        // Calc CPU Usage % for this processor, scroll the history buffer, and store
        // the newly calced value at the head of the history buffer

        BYTE ThisCPU =  DeltaCPUTotalTime.QuadPart ?
                        (BYTE) (100 - ((DeltaCPUIdleTime.QuadPart * 100) / DeltaCPUTotalTime.QuadPart))
                        : 0;

        BYTE * pbHistory = g_pCPUHistory[ListIndex];
        MoveMemory((LPVOID) (pbHistory + 1),
                   (LPVOID) (pbHistory),
                   sizeof(BYTE) * (HIST_SIZE - 1) );
        pbHistory[0] = ThisCPU;

        BYTE ThisKernel = DeltaCPUTotalTime.QuadPart ?
                          (BYTE) (((DeltaCPUKernelTime.QuadPart * 100) / DeltaCPUTotalTime.QuadPart))
                          : 0;

        pbHistory = g_pKernelHistory[ListIndex];
        MoveMemory((LPVOID) (pbHistory + 1),
                   (LPVOID) (pbHistory),
                   sizeof(BYTE) * (HIST_SIZE - 1) );
        pbHistory[0] = ThisKernel;


        PreviousCPUTotalTime[ListIndex].QuadPart = CPUTotalTime[ListIndex].QuadPart;
        PreviousCPUIdleTime[ListIndex].QuadPart  = CPUIdleTime[ListIndex].QuadPart;
        PreviousCPUKernelTime[ListIndex].QuadPart = CPUKernelTime[ListIndex].QuadPart;
    }

    g_CPUUsage = SumTotalTime.QuadPart ?
                 (BYTE) (100 - ((SumIdleTime.QuadPart * 100) / SumTotalTime.QuadPart))
                 : 0;

    if (fUpdateHistory)
    {
        g_KernelUsage = SumTotalTime.QuadPart ?
                        (BYTE) ((SumKernelTime.QuadPart * 100) / SumTotalTime.QuadPart)
                        : 0;

        //
        // Get the commit size
        //

        SYSTEM_PERFORMANCE_INFORMATION PerfInfo;

        Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &PerfInfo,
                    sizeof(PerfInfo),
                    NULL);

        if (!NT_SUCCESS(Status))
        {
            return;
        }

        g_MEMUsage = PerfInfo.CommittedPages * (g_PageSize / 1024);
        MoveMemory((LPVOID) (g_pMEMHistory + 1),
                   (LPVOID) (g_pMEMHistory),
                   sizeof(BYTE) * (HIST_SIZE - 1) );

        g_pMEMHistory[0] = (BYTE) ((g_MEMUsage * 100) / g_MEMMax);
    }
}
