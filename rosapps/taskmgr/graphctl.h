/*
 *  ReactOS Task Manager
 *
 *  GraphCtrl.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GRAPH_CTRL_H__
#define __GRAPH_CTRL_H__

#define MAX_PLOTS 4
#define MAX_CTRLS 4


#ifdef __cplusplus


class TGraphCtrl
{
// Attributes
public:
  double AppendPoint(double dNewPoint0, double dNewPoint1 = 0.0,
                     double dNewPoint2 = 0.0, double dNewPoint3 = 0.0);
  void SetRange(double dLower, double dUpper, int nDecimalPlaces=1);
  void SetXUnits(const char* string);
  void SetYUnits(const char* string);
  void SetGridColor(COLORREF color);
  void SetPlotColor(int plot, COLORREF color);
  void SetBackgroundColor(COLORREF color);
  void InvalidateCtrl();
  void DrawPoint();
  void Reset();

  // Operations
public:
  BOOL Create(DWORD dwStyle, const RECT& rect, HWND hParentWnd, UINT nID=NULL);
  BOOL Create(HWND hWnd, HWND hParentWnd, UINT nID=NULL);
  void Paint(HWND hWnd, HDC dc);
  void Resize(void); 

#if 0
  static TGraphCtrl* LookupGraphCtrl(HWND hWnd);
  static TGraphCtrl* pCtrlArray[MAX_CTRLS];
  static int CtrlCount;
#endif

// Implementation
public:
  int m_nShiftPixels;          // amount to shift with each new point 
  int m_nYDecimals;

  char m_strXUnitsString[50];
  char m_strYUnitsString[50];

  COLORREF m_crBackColor;                 // background color
  COLORREF m_crGridColor;                 // grid color
  COLORREF m_crPlotColor[MAX_PLOTS];      // data color  
  
  double m_dCurrentPosition[MAX_PLOTS];   // current position
  double m_dPreviousPosition[MAX_PLOTS];  // previous position

// Construction
public:
  TGraphCtrl();
  virtual ~TGraphCtrl();

protected:
  int m_nHalfShiftPixels;
  int m_nPlotShiftPixels;
  int m_nClientHeight;
  int m_nClientWidth;
  int m_nPlotHeight;
  int m_nPlotWidth;

  double m_dLowerLimit;        // lower bounds
  double m_dUpperLimit;        // upper bounds
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
};

extern "C" {
#endif

extern LONG OldGraphCtrlWndProc;

LRESULT CALLBACK GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
};
#endif


#endif /* __GRAPH_CTRL_H__ */
