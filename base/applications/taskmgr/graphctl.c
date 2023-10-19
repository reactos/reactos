/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 2002 Robert Dickenson <robd@reactos.org>
 */

#include "precomp.h"

#include <math.h>

WNDPROC OldGraphCtrlWndProc;

static void GraphCtrl_Init(TGraphCtrl* this)
{
    int i;

    this->m_hWnd = 0;
    this->m_hParentWnd = 0;
    this->m_dcGrid = 0;
    this->m_dcPlot = 0;
    this->m_bitmapOldGrid = 0;
    this->m_bitmapOldPlot = 0;
    this->m_bitmapGrid = 0;
    this->m_bitmapPlot = 0;
    this->m_brushBack = 0;

    this->m_penPlot[0] = 0;
    this->m_penPlot[1] = 0;
    this->m_penPlot[2] = 0;
    this->m_penPlot[3] = 0;

    this->m_dPreviousPosition[0] = 0.0;
    this->m_dPreviousPosition[1] = 0.0;
    this->m_dPreviousPosition[2] = 0.0;
    this->m_dPreviousPosition[3] = 0.0;

    this->m_nYDecimals = 3;

    this->m_dLowerLimit = 0.0;
    this->m_dUpperLimit = 100.0;
    this->m_dRange      = this->m_dUpperLimit - this->m_dLowerLimit;

    this->m_nShiftPixels     = 4;
    this->m_nHalfShiftPixels = this->m_nShiftPixels/2;
    this->m_nPlotShiftPixels = this->m_nShiftPixels + this->m_nHalfShiftPixels;

    this->m_crBackColor = RGB(  0,   0,  0);
    this->m_crGridColor = RGB(  0, 128, 64);
    this->m_crPlotColor[0] = RGB(255, 255, 255);
    this->m_crPlotColor[1] = RGB(100, 255, 255);
    this->m_crPlotColor[2] = RGB(255, 100, 255);
    this->m_crPlotColor[3] = RGB(255, 255, 100);

    for (i = 0; i < MAX_PLOTS; i++)
        this->m_penPlot[i] = CreatePen(PS_SOLID, 0, this->m_crPlotColor[i]);
    this->m_brushBack = CreateSolidBrush(this->m_crBackColor);

    strcpy(this->m_strXUnitsString, "Samples");
    strcpy(this->m_strYUnitsString, "Y units");

    this->m_bitmapOldGrid = NULL;
    this->m_bitmapOldPlot = NULL;
}

void GraphCtrl_Dispose(TGraphCtrl* this)
{
    int plot;

    for (plot = 0; plot < MAX_PLOTS; plot++)
        DeleteObject(this->m_penPlot[plot]);

    if (this->m_bitmapOldGrid != NULL) SelectObject(this->m_dcGrid, this->m_bitmapOldGrid);
    if (this->m_bitmapOldPlot != NULL) SelectObject(this->m_dcPlot, this->m_bitmapOldPlot);
    if (this->m_bitmapGrid    != NULL) DeleteObject(this->m_bitmapGrid);
    if (this->m_bitmapPlot    != NULL) DeleteObject(this->m_bitmapPlot);
    if (this->m_dcGrid        != NULL) DeleteDC(this->m_dcGrid);
    if (this->m_dcPlot        != NULL) DeleteDC(this->m_dcPlot);
    if (this->m_brushBack     != NULL) DeleteObject(this->m_brushBack);
}

void GraphCtrl_Create(TGraphCtrl* this, HWND hWnd, HWND hParentWnd, UINT nID)
{
    GraphCtrl_Init(this);
    this->m_hParentWnd = hParentWnd;
    this->m_hWnd = hWnd;

    GraphCtrl_Resize(this);
}

void GraphCtrl_SetRange(TGraphCtrl* this, double dLower, double dUpper, int nDecimalPlaces)
{
    this->m_dLowerLimit     = dLower;
    this->m_dUpperLimit     = dUpper;
    this->m_nYDecimals      = nDecimalPlaces;
    this->m_dRange          = this->m_dUpperLimit - this->m_dLowerLimit;
    this->m_dVerticalFactor = (double)this->m_nPlotHeight / this->m_dRange;
    GraphCtrl_InvalidateCtrl(this, FALSE);
}

void GraphCtrl_SetGridColor(TGraphCtrl* this, COLORREF color)
{
    this->m_crGridColor = color;
    GraphCtrl_InvalidateCtrl(this, FALSE);
}

void GraphCtrl_SetPlotColor(TGraphCtrl* this, int plot, COLORREF color)
{
    this->m_crPlotColor[plot] = color;
    DeleteObject(this->m_penPlot[plot]);
    this->m_penPlot[plot] = CreatePen(PS_SOLID, 0, this->m_crPlotColor[plot]);
    GraphCtrl_InvalidateCtrl(this, FALSE);
}

void GraphCtrl_SetBackgroundColor(TGraphCtrl* this, COLORREF color)
{
    this->m_crBackColor = color;
    DeleteObject(this->m_brushBack);
    this->m_brushBack = CreateSolidBrush(this->m_crBackColor);
    GraphCtrl_InvalidateCtrl(this, FALSE);
}

void GraphCtrl_InvalidateCtrl(TGraphCtrl* this, BOOL bResize)
{
    int i;
    int nCharacters;
    HPEN oldPen;
    HPEN solidPen = CreatePen(PS_SOLID, 0, this->m_crGridColor);
    HDC dc = GetDC(this->m_hParentWnd);

    if (this->m_dcGrid == NULL)
    {
        this->m_dcGrid = CreateCompatibleDC(dc);
        this->m_bitmapGrid = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
        this->m_bitmapOldGrid = (HBITMAP)SelectObject(this->m_dcGrid, this->m_bitmapGrid);
    }
    else if(bResize)
    {
        if(this->m_bitmapGrid != NULL)
        {
            this->m_bitmapGrid = (HBITMAP)SelectObject(this->m_dcGrid, this->m_bitmapOldGrid);
            DeleteObject(this->m_bitmapGrid);
            this->m_bitmapGrid = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
            SelectObject(this->m_dcGrid, this->m_bitmapGrid);
        }
    }

    SetBkColor(this->m_dcGrid, this->m_crBackColor);

    FillRect(this->m_dcGrid, &this->m_rectClient, this->m_brushBack);

    nCharacters = abs((int)log10(fabs(this->m_dUpperLimit)));
    nCharacters = max(nCharacters, abs((int)log10(fabs(this->m_dLowerLimit))));
    nCharacters = nCharacters + 4 + this->m_nYDecimals;

    this->m_rectPlot.left = this->m_rectClient.left;
    this->m_nPlotWidth    = this->m_rectPlot.right - this->m_rectPlot.left;

    oldPen = (HPEN)SelectObject(this->m_dcGrid, solidPen);
    MoveToEx(this->m_dcGrid, this->m_rectPlot.left, this->m_rectPlot.top, NULL);
    LineTo(this->m_dcGrid, this->m_rectPlot.right+1, this->m_rectPlot.top);
    LineTo(this->m_dcGrid, this->m_rectPlot.right+1, this->m_rectPlot.bottom+1);
    LineTo(this->m_dcGrid, this->m_rectPlot.left, this->m_rectPlot.bottom+1);

    for (i = this->m_rectPlot.top; i < this->m_rectPlot.bottom; i += 12)
    {
        MoveToEx(this->m_dcGrid, this->m_rectPlot.left, this->m_rectPlot.top + i, NULL);
        LineTo(this->m_dcGrid, this->m_rectPlot.right, this->m_rectPlot.top + i);
    }

    for (i = this->m_rectPlot.left; i < this->m_rectPlot.right; i += 12)
    {
        MoveToEx(this->m_dcGrid, this->m_rectPlot.left + i, this->m_rectPlot.bottom, NULL);
        LineTo(this->m_dcGrid, this->m_rectPlot.left + i, this->m_rectPlot.top);
    }

    SelectObject(this->m_dcGrid, oldPen);
    DeleteObject(solidPen);

    if (this->m_dcPlot == NULL)
    {
        this->m_dcPlot = CreateCompatibleDC(dc);
        this->m_bitmapPlot = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
        this->m_bitmapOldPlot = (HBITMAP)SelectObject(this->m_dcPlot, this->m_bitmapPlot);
    }
    else if(bResize)
    {
        if(this->m_bitmapPlot != NULL)
        {
            this->m_bitmapPlot = (HBITMAP)SelectObject(this->m_dcPlot, this->m_bitmapOldPlot);
            DeleteObject(this->m_bitmapPlot);
            this->m_bitmapPlot = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
            SelectObject(this->m_dcPlot, this->m_bitmapPlot);
        }
    }

    SetBkColor(this->m_dcPlot, this->m_crBackColor);
    FillRect(this->m_dcPlot, &this->m_rectClient, this->m_brushBack);

    InvalidateRect(this->m_hParentWnd, &this->m_rectClient, TRUE);
    ReleaseDC(this->m_hParentWnd, dc);
}

double GraphCtrl_AppendPoint(TGraphCtrl* this,
    double dNewPoint0, double dNewPoint1,
    double dNewPoint2, double dNewPoint3)
{
    double dPrevious;

    dPrevious = this->m_dCurrentPosition[0];
    this->m_dCurrentPosition[0] = dNewPoint0;
    this->m_dCurrentPosition[1] = dNewPoint1;
    this->m_dCurrentPosition[2] = dNewPoint2;
    this->m_dCurrentPosition[3] = dNewPoint3;
    GraphCtrl_DrawPoint(this);
    return dPrevious;
}

void GraphCtrl_Paint(TGraphCtrl* this, HWND hWnd, HDC dc)
{
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;

    memDC = CreateCompatibleDC(dc);
    memBitmap = (HBITMAP)CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
    oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    if (memDC != NULL)
    {
        BitBlt(memDC, 0, 0, this->m_nClientWidth, this->m_nClientHeight, this->m_dcGrid, 0, 0, SRCCOPY);
        BitBlt(memDC, 0, 0, this->m_nClientWidth, this->m_nClientHeight, this->m_dcPlot, 0, 0, SRCPAINT);
        BitBlt(dc, 0, 0, this->m_nClientWidth, this->m_nClientHeight, memDC, 0, 0, SRCCOPY);
    }
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void GraphCtrl_DrawPoint(TGraphCtrl* this)
{
    int currX, prevX, currY, prevY;
    HPEN oldPen;
    RECT rectCleanUp;
    int i;

    if (this->m_dcPlot != NULL)
    {
        BitBlt(this->m_dcPlot, this->m_rectPlot.left, this->m_rectPlot.top+1,
               this->m_nPlotWidth, this->m_nPlotHeight, this->m_dcPlot,
               this->m_rectPlot.left+this->m_nShiftPixels, this->m_rectPlot.top+1,
               SRCCOPY);

        rectCleanUp = this->m_rectPlot;
        rectCleanUp.left  = rectCleanUp.right - this->m_nShiftPixels;

        FillRect(this->m_dcPlot, &rectCleanUp, this->m_brushBack);

        for (i = 0; i < MAX_PLOTS; i++)
        {
            oldPen = (HPEN)SelectObject(this->m_dcPlot, this->m_penPlot[i]);

            prevX = this->m_rectPlot.right-this->m_nPlotShiftPixels;
            prevY = this->m_rectPlot.bottom -
                (long)((this->m_dPreviousPosition[i] - this->m_dLowerLimit) * this->m_dVerticalFactor);
            MoveToEx(this->m_dcPlot, prevX, prevY, NULL);

            currX = this->m_rectPlot.right-this->m_nHalfShiftPixels;
            currY = this->m_rectPlot.bottom -
                (long)((this->m_dCurrentPosition[i] - this->m_dLowerLimit) * this->m_dVerticalFactor);
            LineTo(this->m_dcPlot, currX, currY);

            SelectObject(this->m_dcPlot, oldPen);

            if ((prevY <= this->m_rectPlot.top) || (currY <= this->m_rectPlot.top))
            {
                RECT rc;
                rc.bottom = this->m_rectPlot.top+1;
                rc.left = prevX;
                rc.right = currX+1;
                rc.top = this->m_rectClient.top;
                FillRect(this->m_dcPlot, &rc, this->m_brushBack);
            }
            if ((prevY >= this->m_rectPlot.bottom) || (currY >= this->m_rectPlot.bottom))
            {
                RECT rc;
                rc.bottom = this->m_rectClient.bottom+1;
                rc.left = prevX;
                rc.right = currX+1;
                rc.top = this->m_rectPlot.bottom+1;
                FillRect(this->m_dcPlot, &rc, this->m_brushBack);
            }

            this->m_dPreviousPosition[i] = this->m_dCurrentPosition[i];
        }
    }
}

void GraphCtrl_Resize(TGraphCtrl* this)
{
    GetClientRect(this->m_hWnd, &this->m_rectClient);

    this->m_nClientHeight = this->m_rectClient.bottom - this->m_rectClient.top;
    this->m_nClientWidth  = this->m_rectClient.right - this->m_rectClient.left;

    this->m_rectPlot.left   = 0;
    this->m_rectPlot.top    = -1;
    this->m_rectPlot.right  = this->m_rectClient.right;
    this->m_rectPlot.bottom = this->m_rectClient.bottom;

    this->m_nPlotHeight = this->m_rectPlot.bottom - this->m_rectPlot.top;
    this->m_nPlotWidth  = this->m_rectPlot.right - this->m_rectPlot.left;

    this->m_dVerticalFactor = (double)this->m_nPlotHeight / this->m_dRange;
}

extern TGraphCtrl PerformancePageCpuUsageHistoryGraph;
extern TGraphCtrl PerformancePageMemUsageHistoryGraph;
extern HWND hPerformancePageCpuUsageHistoryGraph;
extern HWND hPerformancePageMemUsageHistoryGraph;

INT_PTR CALLBACK
GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT        rcClient;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_ERASEBKGND:
        return TRUE;
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
    case WM_NCHITTEST:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCMOUSEMOVE:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
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
    case WM_SIZE:
        if (hWnd == hPerformancePageMemUsageHistoryGraph)
        {
            GraphCtrl_Resize(&PerformancePageMemUsageHistoryGraph);
            GraphCtrl_InvalidateCtrl(&PerformancePageMemUsageHistoryGraph, TRUE);
        }
        if (hWnd == hPerformancePageCpuUsageHistoryGraph)
        {
            GraphCtrl_Resize(&PerformancePageCpuUsageHistoryGraph);
            GraphCtrl_InvalidateCtrl(&PerformancePageCpuUsageHistoryGraph, TRUE);
        }
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rcClient);
        if (hWnd == hPerformancePageMemUsageHistoryGraph)
            GraphCtrl_Paint(&PerformancePageMemUsageHistoryGraph, hWnd, hdc);
        if (hWnd == hPerformancePageCpuUsageHistoryGraph)
            GraphCtrl_Paint(&PerformancePageCpuUsageHistoryGraph, hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    return CallWindowProcW(OldGraphCtrlWndProc, hWnd, message, wParam, lParam);
}
