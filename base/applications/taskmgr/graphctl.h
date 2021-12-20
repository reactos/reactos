/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Graph Plotting controls.
 * COPYRIGHT:   Copyright 2002 Robert Dickenson <robd@reactos.org>
 *              Copyright 2021 Wu Haotian <rigoligo03@gmail.com>
 *              Copyright 2021 Valerij Zaporogeci <vlrzprgts@gmail.com>
 */

#pragma once

#define NUM_PLOTS    2
#define PLOT_SHIFT   2

typedef struct _TM_GRAPH_CONTROL
{
    HWND     hParentWnd;
    HWND     hWnd;
    HDC      hdcGraph;
    HBITMAP  hbmGraph;
    HPEN     hPenGrid;
    HPEN     hPen0;
    HPEN     hPen1;
    HBRUSH   hBrushBack;

    INT      BitmapWidth;
    INT      BitmapHeight;
    INT      GridCellWidth;
    INT      GridCellHeight;
    INT      CurrShift;

    PBYTE    PointBuffer;
    UINT32   NumberOfPoints;
    UINT32   CurrIndex;

    FLOAT    ftPixelsPerPercent;
    BOOL     DrawSecondaryPlot;
}
TM_GRAPH_CONTROL, *PTM_GRAPH_CONTROL;

typedef struct _TM_FORMAT
{
    COLORREF  clrBack;
    COLORREF  clrGrid;
    COLORREF  clrPlot0;
    COLORREF  clrPlot1;
    INT       GridCellWidth;
    INT       GridCellHeight;
    BOOL      DrawSecondaryPlot;
}
TM_FORMAT, *PTM_FORMAT;

INT_PTR CALLBACK GraphCtrl_OnSize(HWND hWnd, PTM_GRAPH_CONTROL graph, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GraphCtrl_OnDraw(HWND hWnd, PTM_GRAPH_CONTROL graph, WPARAM wParam, LPARAM lParam);

BOOL GraphCtrl_Create(PTM_GRAPH_CONTROL inst, HWND hWnd, HWND hParentWnd, PTM_FORMAT fmt);
void GraphCtrl_Dispose(PTM_GRAPH_CONTROL inst);
void GraphCtrl_AddPoint(PTM_GRAPH_CONTROL inst, BYTE val0, BYTE val1);
void GraphCtrl_RedrawOnHeightChange(PTM_GRAPH_CONTROL inst, INT nh);
void GraphCtrl_RedrawBitmap(PTM_GRAPH_CONTROL inst, INT h);
