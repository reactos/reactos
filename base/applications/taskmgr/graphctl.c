/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Graph plotting controls
 * COPYRIGHT:   Copyright 2002 Robert Dickenson <robd@reactos.org>
 *              Copyright 2021 Wu Haotian <rigoligo03@gmail.com>
 */

#include "precomp.h"

#include <math.h>

WNDPROC OldGraphCtrlWndProc;

BOOL
GraphCtrl_Create(PTM_GRAPH_CONTROL inst, HWND hWnd, HWND hParentWnd, PTM_FORMAT fmt)
{
    HDC     hdc, hdcg;
    HBITMAP hbmOld;
    UINT    Size;
    INT     p;
    RECT    rc;

    inst->hParentWnd = hParentWnd;
    inst->hWnd = hWnd;

    Size = GetSystemMetrics(SM_CXSCREEN);
    inst->BitmapWidth = Size;
    Size /= PLOT_SHIFT;
    inst->PointBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size * NUM_PLOTS);
    if (!inst->PointBuffer)
    {
        goto fail;
    }

    inst->NumberOfPoints = Size;
    inst->CurrIndex = 0;

    /* Styling */
    inst->hPenGrid = CreatePen(PS_SOLID, 0, fmt->clrGrid);
    inst->hPen0 = CreatePen(PS_SOLID, 0, fmt->clrPlot0);
    inst->hPen1 = CreatePen(PS_SOLID, 0, fmt->clrPlot1);
    inst->hBrushBack = CreateSolidBrush(fmt->clrBack);

    if (!inst->hPenGrid ||
        !inst->hPen0 ||
        !inst->hPen1 ||
        !inst->hBrushBack)
    {
        goto fail;
    }

    if (fmt->GridCellWidth >= PLOT_SHIFT << 2)
        inst->GridCellWidth = fmt->GridCellWidth;
    else
        inst->GridCellWidth = PLOT_SHIFT << 2;
    if (fmt->GridCellHeight >= PLOT_SHIFT << 2)
        inst->GridCellHeight = fmt->GridCellHeight;
    else
        inst->GridCellHeight = PLOT_SHIFT << 2;

    inst->DrawSecondaryPlot = fmt->DrawSecondaryPlot;

    GetClientRect(hWnd, &rc);
    inst->BitmapHeight = rc.bottom;
    inst->ftPixelsPerPercent = (FLOAT)(inst->BitmapHeight) / 100.00f;

    hdc = GetDC(hParentWnd);
    hdcg = CreateCompatibleDC(hdc);
    inst->hdcGraph = hdcg;
    inst->hbmGraph = CreateCompatibleBitmap(hdc, inst->BitmapWidth, inst->BitmapHeight);

    if (!hdc ||
        !hdcg ||
        !inst->hbmGraph)
    {
        goto fail;
    }

    ReleaseDC(hParentWnd, hdc);
    hbmOld = (HBITMAP)SelectObject(hdcg, inst->hbmGraph);
    DeleteObject(hbmOld);

    SetBkColor(hdcg, fmt->clrBack);
    rc.right = inst->BitmapWidth;
    FillRect(hdcg, &rc, inst->hBrushBack);

    inst->CurrShift = 0;
    SelectObject(hdcg, inst->hPenGrid);
    for (p = inst->GridCellHeight - 1;
         p < inst->BitmapHeight;
         p += inst->GridCellHeight)
    {
        MoveToEx(hdcg, 0, p, NULL);
        LineTo(hdcg, inst->BitmapWidth, p);
    }
    for (p = inst->BitmapWidth - 1;
         p > 0;
         p -= inst->GridCellWidth)
    {
        MoveToEx(hdcg, p, 0, NULL);
        LineTo(hdcg, p, inst->BitmapHeight);
    }
    SelectObject(hdcg, inst->hPen0);

    return TRUE;

fail:
    GraphCtrl_Dispose(inst);
    return FALSE;
}

void
GraphCtrl_Dispose(PTM_GRAPH_CONTROL inst)
{
    if (inst->PointBuffer)
        HeapFree(GetProcessHeap(), 0, inst->PointBuffer);

    if (inst->hPenGrid)
        DeleteObject(inst->hPenGrid);

    if (inst->hPen0)
        DeleteObject(inst->hPen0);

    if (inst->hPen1)
        DeleteObject(inst->hPen1);

    if (inst->hBrushBack)
        DeleteObject(inst->hBrushBack);

    if (inst->hbmGraph)
        DeleteObject(inst->hbmGraph);

    if (inst->hdcGraph)
        DeleteObject(inst->hdcGraph);
}

void
GraphCtrl_AddPoint(PTM_GRAPH_CONTROL inst, BYTE val0, BYTE val1)
{
    HDC    hdcg;
    PBYTE  t;
    RECT   rcDirt;

    UINT   Prev0, Prev1, RetainingWidth;
    INT    PrevY, CurrY, p, v;

    hdcg = inst->hdcGraph;
    RetainingWidth = inst->BitmapWidth - PLOT_SHIFT;
    t = inst->PointBuffer;
    Prev0 = *(t + inst->CurrIndex);
    Prev1 = *(t + inst->CurrIndex + inst->NumberOfPoints);
    if (inst->CurrIndex < inst->NumberOfPoints)
    {
        inst->CurrIndex++;
    }
    else
    {
        inst->CurrIndex = 0;
    }
    *(t + inst->CurrIndex) = val0;
    *(t + inst->CurrIndex + inst->NumberOfPoints) = val1;

    /* Drawing points, first shifting the plot left */
    BitBlt(hdcg, 0, 0, RetainingWidth, inst->BitmapHeight, hdcg, PLOT_SHIFT, 0, SRCCOPY);

    rcDirt.left = RetainingWidth;
    rcDirt.top = 0;
    rcDirt.right = inst->BitmapWidth;
    rcDirt.bottom = inst->BitmapHeight;
    FillRect(hdcg, &rcDirt, inst->hBrushBack);

    SelectObject(hdcg, inst->hPenGrid);
    for (p = inst->GridCellHeight - 1;
         p < inst->BitmapHeight;
         p += inst->GridCellHeight)
    {
        MoveToEx(hdcg, RetainingWidth, p, NULL);
        LineTo(hdcg, inst->BitmapWidth, p);
    }
    v = inst->CurrShift + PLOT_SHIFT;
    if (v >= inst->GridCellWidth)
    {
        v -= inst->GridCellWidth;
        p = inst->BitmapWidth - v - 1;
        MoveToEx(hdcg, p, 0, NULL);
        LineTo(hdcg, p, inst->BitmapHeight);
    }
    inst->CurrShift = v;

    if (inst->DrawSecondaryPlot)
    {
        SelectObject(inst->hdcGraph, inst->hPen1);

        PrevY = inst->BitmapHeight - Prev1 * inst->ftPixelsPerPercent;
        MoveToEx(inst->hdcGraph, RetainingWidth - 1, PrevY, NULL);
        CurrY = inst->BitmapHeight - val1 * inst->ftPixelsPerPercent;
        LineTo(inst->hdcGraph, inst->BitmapWidth - 1, CurrY);
    }

    SelectObject(inst->hdcGraph, inst->hPen0);
    PrevY = inst->BitmapHeight - Prev0 * inst->ftPixelsPerPercent;
    MoveToEx(inst->hdcGraph, RetainingWidth - 1, PrevY, NULL);
    CurrY = inst->BitmapHeight - val0 * inst->ftPixelsPerPercent;
    LineTo(inst->hdcGraph, inst->BitmapWidth - 1, CurrY);
}

inline void
GraphCtrl_RedrawBitmap(PTM_GRAPH_CONTROL inst, INT h)
{
    HDC    hdcg;
    PBYTE  t;
    RECT   rc;
    INT    i, j, y, x, p;
    FLOAT  coef;

    hdcg = inst->hdcGraph;
    rc.left = 0; rc.top = 0;
    rc.right = inst->BitmapWidth; rc.bottom = h;
    FillRect(hdcg, &rc, inst->hBrushBack);

    SelectObject(hdcg, inst->hPenGrid);

    for (p = inst->GridCellHeight - 1;
         p < inst->BitmapHeight;
         p += inst->GridCellHeight)
    {
        MoveToEx(hdcg, 0, p, NULL);
        LineTo(hdcg, inst->BitmapWidth, p);
    }

    for (p = inst->BitmapWidth - inst->CurrShift - 1;
         p > 0;
         p -= inst->GridCellWidth)
    {
        MoveToEx(hdcg, p, 0, NULL);
        LineTo(hdcg, p, inst->BitmapHeight);
    }

    coef = inst->ftPixelsPerPercent;

    if (inst->DrawSecondaryPlot)
    {
        SelectObject(hdcg, inst->hPen1);
        t = inst->PointBuffer + inst->NumberOfPoints;
        x = inst->BitmapWidth - 1;
        j = inst->CurrIndex;
        y = h - *(t + j) * coef;
        MoveToEx(hdcg, x, y, NULL);
        for (i = 0; i < inst->NumberOfPoints; i++)
        {
            j = (j ? j : inst->NumberOfPoints) - 1;
            y = h - *(t + j) * coef;
            x -= PLOT_SHIFT;
            LineTo(hdcg, x, y);
        }
    }

    SelectObject(hdcg, inst->hPen0);
    t = inst->PointBuffer;
    x = inst->BitmapWidth - 1;
    j = inst->CurrIndex;
    y = h - *(t + j) * coef;
    MoveToEx(hdcg, x, y, NULL);

    for (i = 0; i < inst->NumberOfPoints; i++)
    {
        j = (j ? j : inst->NumberOfPoints) - 1;
        y = h - *(t + j) * coef;
        x -= PLOT_SHIFT;
        LineTo(hdcg, x, y);
    }
}

inline void
GraphCtrl_RedrawOnHeightChange(PTM_GRAPH_CONTROL inst, INT nh)
{
    HDC     hdc;
    HBITMAP hbmOld;

    inst->BitmapHeight = nh;
    inst->ftPixelsPerPercent = (FLOAT)nh / 100.00f;

    hdc = GetDC(inst->hParentWnd);
    hbmOld = inst->hbmGraph;
    inst->hbmGraph = CreateCompatibleBitmap(hdc, inst->BitmapWidth, nh);
    SelectObject(inst->hdcGraph, inst->hbmGraph);
    DeleteObject(hbmOld);
    ReleaseDC(inst->hParentWnd, hdc);

    GraphCtrl_RedrawBitmap(inst, nh);
}

extern TM_GRAPH_CONTROL PerformancePageCpuUsageHistoryGraph;
extern TM_GRAPH_CONTROL PerformancePageMemUsageHistoryGraph;
extern HWND hPerformancePageCpuUsageHistoryGraph;
extern HWND hPerformancePageMemUsageHistoryGraph;

INT_PTR CALLBACK
GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PTM_GRAPH_CONTROL graph;

    switch (message)
    {
        case WM_ERASEBKGND:
            return TRUE;
        /*
         *  Filter out mouse  & keyboard messages
         */
        // case WM_APPCOMMAND:
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
        // case WM_MOUSEWHEEL:
        case WM_NCHITTEST:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        // case WM_NCMOUSEHOVER:
        // case WM_NCMOUSELEAVE:
        case WM_NCMOUSEMOVE:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        // case WM_NCXBUTTONDBLCLK:
        // case WM_NCXBUTTONDOWN:
        // case WM_NCXBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        // case WM_XBUTTONDBLCLK:
        // case WM_XBUTTONDOWN:
        // case WM_XBUTTONUP:
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
            return 0;

        case WM_NCCALCSIZE:
            return 0;

        case WM_SIZE:
        {
            if (hWnd == hPerformancePageCpuUsageHistoryGraph)
                graph = &PerformancePageCpuUsageHistoryGraph;
            else if (hWnd == hPerformancePageMemUsageHistoryGraph)
                graph = &PerformancePageMemUsageHistoryGraph;
            else
                return 0;

            if (HIWORD(lParam) != graph->BitmapHeight)
            {
                GraphCtrl_RedrawOnHeightChange(graph, HIWORD(lParam));
            }
            InvalidateRect(hWnd, NULL, FALSE);

            return 0;
        }

        case WM_PAINT:
        {
            RECT        rcClient;
            HDC         hdc;
            PAINTSTRUCT ps;

            if (hWnd == hPerformancePageCpuUsageHistoryGraph)
                graph = &PerformancePageCpuUsageHistoryGraph;
            else if (hWnd == hPerformancePageMemUsageHistoryGraph)
                graph = &PerformancePageMemUsageHistoryGraph;
            else
                return 0;

            hdc = BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &rcClient);
            BitBlt(hdc, 0, 0,
                   rcClient.right,
                   rcClient.bottom,
                   graph->hdcGraph,
                   graph->BitmapWidth - rcClient.right,
                   0,
                   SRCCOPY);
            EndPaint(hWnd, &ps);
            return 0;
        }
    }

    /*
     *  We pass on all non-handled messages
     */
    return CallWindowProcW(OldGraphCtrlWndProc, hWnd, message, wParam, lParam);
}
