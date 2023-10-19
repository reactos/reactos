/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 2002 Robert Dickenson <robd@reactos.org>
 */

#pragma once

#define MAX_PLOTS 4
#define MAX_CTRLS 4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  int m_nShiftPixels;
  int m_nYDecimals;

  char m_strXUnitsString[50];
  char m_strYUnitsString[50];

  COLORREF m_crBackColor;
  COLORREF m_crGridColor;
  COLORREF m_crPlotColor[MAX_PLOTS];

  double m_dCurrentPosition[MAX_PLOTS];
  double m_dPreviousPosition[MAX_PLOTS];

  int m_nHalfShiftPixels;
  int m_nPlotShiftPixels;
  int m_nClientHeight;
  int m_nClientWidth;
  int m_nPlotHeight;
  int m_nPlotWidth;

  double m_dLowerLimit;
  double m_dUpperLimit;
  double m_dRange;
  double m_dVerticalFactor;

  HWND     m_hWnd;
  HWND     m_hParentWnd;
  HDC      m_dcGrid;
  HDC      m_dcPlot;
  HBITMAP  m_bitmapOldGrid;
  HBITMAP  m_bitmapOldPlot;
  HBITMAP  m_bitmapGrid;
  HBITMAP  m_bitmapPlot;
  HBRUSH   m_brushBack;
  HPEN     m_penPlot[MAX_PLOTS];
  RECT     m_rectClient;
  RECT     m_rectPlot;
} TGraphCtrl;

extern WNDPROC OldGraphCtrlWndProc;
double GraphCtrl_AppendPoint(TGraphCtrl* this,
    double dNewPoint0, double dNewPoint1,
    double dNewPoint2, double dNewPoint3);
void GraphCtrl_Create(TGraphCtrl* this, HWND hWnd, HWND hParentWnd, UINT nID);
void GraphCtrl_Dispose(TGraphCtrl* this);
void GraphCtrl_DrawPoint(TGraphCtrl* this);
void GraphCtrl_InvalidateCtrl(TGraphCtrl* this, BOOL bResize);
void GraphCtrl_Paint(TGraphCtrl* this, HWND hWnd, HDC dc);
void GraphCtrl_Reset(TGraphCtrl* this);
void GraphCtrl_Resize(TGraphCtrl* this);
void GraphCtrl_SetBackgroundColor(TGraphCtrl* this, COLORREF color);
void GraphCtrl_SetGridColor(TGraphCtrl* this, COLORREF color);
void GraphCtrl_SetPlotColor(TGraphCtrl* this, int plot, COLORREF color);
void GraphCtrl_SetRange(TGraphCtrl* this, double dLower, double dUpper, int nDecimalPlaces);

INT_PTR CALLBACK GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif
