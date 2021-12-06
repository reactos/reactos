/*
 *  ReactOS Task Manager
 *
 *  graphctl.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#define NUM_PLOTS    2
#define PLOT_SHIFT   2

#ifdef __cplusplus
extern "C" {
#endif



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

extern WNDPROC OldGraphCtrlWndProc;
INT_PTR CALLBACK GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL GraphCtrl_Create(PTM_GRAPH_CONTROL inst, HWND hWnd, HWND hParentWnd, PTM_FORMAT fmt);
void GraphCtrl_Dispose(PTM_GRAPH_CONTROL inst);
void GraphCtrl_AddPoint(PTM_GRAPH_CONTROL inst, BYTE val0, BYTE val1);
void GraphCtrl_RedrawOnHeightChange(PTM_GRAPH_CONTROL inst, INT nh);
void GraphCtrl_RedrawBitmap(PTM_GRAPH_CONTROL inst, INT h);

#ifdef __cplusplus
}
#endif
