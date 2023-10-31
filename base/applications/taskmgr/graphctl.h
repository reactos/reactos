/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 2002 Robert Dickenson <robd@reactos.org>
 */

#pragma once

#define NUM_PLOTS    2
#define PLOT_SHIFT   2

typedef struct
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
} TGraphCtrl, *PTGraphCtrl;

typedef struct
{
    COLORREF  clrBack;
    COLORREF  clrGrid;
    COLORREF  clrPlot0;
    COLORREF  clrPlot1;
    INT       GridCellWidth;
    INT       GridCellHeight;
    BOOL      DrawSecondaryPlot;
} TFormat, *PTFormat;

extern WNDPROC OldGraphCtrlWndProc;
INT_PTR CALLBACK GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void GraphCtrl_Create(PTGraphCtrl inst, HWND hWnd, HWND hParentWnd, PTFormat fmt);
void GraphCtrl_Dispose(PTGraphCtrl inst);
void GraphCtrl_AddPoint(PTGraphCtrl inst, BYTE val0, BYTE val1);
void GraphCtrl_RedrawOnHeightChange(PTGraphCtrl inst, INT nh);
void GraphCtrl_RedrawBitmap(PTGraphCtrl inst, INT h);
