/*
 *  ReactOS Task Manager
 *
 *  GraphCtrl.cpp
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

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include "math.h"
#include "graphctl.h"
#include "taskmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


LONG OldGraphCtrlWndProc;


TGraphCtrl::TGraphCtrl() :   
  m_hWnd(0), 
  m_hParentWnd(0), 
  m_dcGrid(0), 
  m_dcPlot(0), 
  m_bitmapOldGrid(0), 
  m_bitmapOldPlot(0), 
  m_bitmapGrid(0), 
  m_bitmapPlot(0), 
  m_brushBack(0)
{
  //RECT   m_rectClient;
  //RECT   m_rectPlot;
  m_penPlot[0] = 0;
  m_penPlot[1] = 0;
  m_penPlot[2] = 0;
  m_penPlot[3] = 0;

  // since plotting is based on a LineTo for each new point
  // we need a starting point (i.e. a "previous" point)
  // use 0.0 as the default first point.
  // these are public member variables, and can be changed outside
  // (after construction).  Therefore m_perviousPosition could be set to
  // a more appropriate value prior to the first call to SetPosition.
  m_dPreviousPosition[0] = 0.0;
  m_dPreviousPosition[1] = 0.0;
  m_dPreviousPosition[2] = 0.0;
  m_dPreviousPosition[3] = 0.0;

  // public variable for the number of decimal places on the y axis
  m_nYDecimals = 3;

  // set some initial values for the scaling until "SetRange" is called.
  // these are protected varaibles and must be set with SetRange
  // in order to ensure that m_dRange is updated accordingly
//  m_dLowerLimit = -10.0;
//  m_dUpperLimit =  10.0;
  m_dLowerLimit = 0.0;
  m_dUpperLimit = 100.0;
  m_dRange      =  m_dUpperLimit - m_dLowerLimit;   // protected member variable

  // m_nShiftPixels determines how much the plot shifts (in terms of pixels) 
  // with the addition of a new data point
  m_nShiftPixels     = 4;
  m_nHalfShiftPixels = m_nShiftPixels/2;                     // protected
  m_nPlotShiftPixels = m_nShiftPixels + m_nHalfShiftPixels;  // protected

  // background, grid and data colors
  // these are public variables and can be set directly
  m_crBackColor = RGB(  0,   0,   0);  // see also SetBackgroundColor
  m_crGridColor = RGB(  0, 255, 255);  // see also SetGridColor
  m_crPlotColor[0] = RGB(255, 255, 255);  // see also SetPlotColor
  m_crPlotColor[1] = RGB(100, 255, 255);  // see also SetPlotColor
  m_crPlotColor[2] = RGB(255, 100, 255);  // see also SetPlotColor
  m_crPlotColor[3] = RGB(255, 255, 100);  // see also SetPlotColor

  // protected variables
  int i;
  for (i = 0; i < MAX_PLOTS; i++) {
    m_penPlot[i] = CreatePen(PS_SOLID, 0, m_crPlotColor[i]);
  }
  m_brushBack = CreateSolidBrush(m_crBackColor);

  // public member variables, can be set directly 
  strcpy(m_strXUnitsString, "Samples");  // can also be set with SetXUnits
  strcpy(m_strYUnitsString, "Y units");  // can also be set with SetYUnits

  // protected bitmaps to restore the memory DC's
  m_bitmapOldGrid = NULL;
  m_bitmapOldPlot = NULL;
#if 0
  for (i = 0; i < MAX_CTRLS; i++) {
    if (pCtrlArray[i] == 0) {
      pCtrlArray[i] = this;
    }
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
TGraphCtrl::~TGraphCtrl()
{
  // just to be picky restore the bitmaps for the two memory dc's
  // (these dc's are being destroyed so there shouldn't be any leaks)
  if (m_bitmapOldGrid != NULL) SelectObject(m_dcGrid, m_bitmapOldGrid);  
  if (m_bitmapOldPlot != NULL) SelectObject(m_dcPlot, m_bitmapOldPlot);  
  if (m_bitmapGrid    != NULL) DeleteObject(m_bitmapGrid);
  if (m_bitmapPlot    != NULL) DeleteObject(m_bitmapPlot);
  if (m_dcGrid        != NULL) DeleteDC(m_dcGrid);
  if (m_dcPlot        != NULL) DeleteDC(m_dcPlot);
  if (m_brushBack     != NULL) DeleteObject(m_brushBack);
#if 0
  for (int i = 0; i < MAX_CTRLS; i++) {
    if (pCtrlArray[i] == this) {
      pCtrlArray[i] = 0;
    }
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
BOOL TGraphCtrl::Create(HWND hWnd, HWND hParentWnd, UINT nID) 
{
  BOOL result = 0;

  m_hParentWnd = hParentWnd;
  m_hWnd = hWnd;
  Resize();
  if (result != 0)
    InvalidateCtrl();
  return result;
}

/*
BOOL TGraphCtrl::Create(DWORD dwStyle, const RECT& rect, 
                         HWND hParentWnd, UINT nID) 
{
  BOOL result = 0;

  m_hParentWnd = hParentWnd;
//  GetClientRect(m_hParentWnd, &m_rectClient);

  // set some member variables to avoid multiple function calls
  m_nClientHeight = rect.bottom - rect.top;//rect.Height();
  m_nClientWidth  = rect.right - rect.left;//rect.Width();
//  m_nClientHeight = cx;
//  m_nClientWidth  = cy;

  // the "left" coordinate and "width" will be modified in 
  // InvalidateCtrl to be based on the width of the y axis scaling
#if 0
  m_rectPlot.left   = 20;  
  m_rectPlot.top    = 10;
  m_rectPlot.right  = rect.right-10;
  m_rectPlot.bottom = rect.bottom-25;
#else
  m_rectPlot.left   = -1;  
  m_rectPlot.top    = -1;
  m_rectPlot.right  = rect.right-0;
  m_rectPlot.bottom = rect.bottom-0;
#endif
  // set some member variables to avoid multiple function calls
  m_nPlotHeight = m_rectPlot.bottom - m_rectPlot.top;//m_rectPlot.Height();
  m_nPlotWidth  = m_rectPlot.right - m_rectPlot.left;//m_rectPlot.Width();

  // set the scaling factor for now, this can be adjusted 
  // in the SetRange functions
  m_dVerticalFactor = (double)m_nPlotHeight / m_dRange; 

  if (result != 0)
    InvalidateCtrl();
  return result;
}
 */
/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::SetRange(double dLower, double dUpper, int nDecimalPlaces)
{
  //ASSERT(dUpper > dLower);
  m_dLowerLimit     = dLower;
  m_dUpperLimit     = dUpper;
  m_nYDecimals      = nDecimalPlaces;
  m_dRange          = m_dUpperLimit - m_dLowerLimit;
  m_dVerticalFactor = (double)m_nPlotHeight / m_dRange; 
  // clear out the existing garbage, re-start with a clean plot
  InvalidateCtrl();
}


/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::SetXUnits(const char* string)
{
  strncpy(m_strXUnitsString, string, sizeof(m_strXUnitsString) - 1);
  // clear out the existing garbage, re-start with a clean plot
  InvalidateCtrl();
}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::SetYUnits(const char* string)
{
  strncpy(m_strYUnitsString, string, sizeof(m_strYUnitsString) - 1);
  // clear out the existing garbage, re-start with a clean plot
  InvalidateCtrl();
}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::SetGridColor(COLORREF color)
{
  m_crGridColor = color;
  // clear out the existing garbage, re-start with a clean plot
  InvalidateCtrl();
}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::SetPlotColor(int plot, COLORREF color)
{
  m_crPlotColor[plot] = color;
  DeleteObject(m_penPlot[plot]);
  m_penPlot[plot] = CreatePen(PS_SOLID, 0, m_crPlotColor[plot]);
  // clear out the existing garbage, re-start with a clean plot
  InvalidateCtrl();
}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::SetBackgroundColor(COLORREF color)
{
  m_crBackColor = color;
  DeleteObject(m_brushBack);
  m_brushBack = CreateSolidBrush(m_crBackColor);
  // clear out the existing garbage, re-start with a clean plot
  InvalidateCtrl();

}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::InvalidateCtrl()
{
  // There is a lot of drawing going on here - particularly in terms of 
  // drawing the grid.  Don't panic, this is all being drawn (only once)
  // to a bitmap.  The result is then BitBlt'd to the control whenever needed.
  int i, j;
  int nCharacters;
  int nTopGridPix, nMidGridPix, nBottomGridPix;

  HPEN oldPen;
  HPEN solidPen = CreatePen(PS_SOLID, 0, m_crGridColor);
  //HFONT axisFont, yUnitFont, oldFont;
  //char strTemp[50];

  // in case we haven't established the memory dc's
  //CClientDC dc(this);  
  HDC dc = GetDC(m_hParentWnd);

  // if we don't have one yet, set up a memory dc for the grid
  if (m_dcGrid == NULL) {
    m_dcGrid = CreateCompatibleDC(dc);
    m_bitmapGrid = CreateCompatibleBitmap(dc, m_nClientWidth, m_nClientHeight);
    m_bitmapOldGrid = (HBITMAP)SelectObject(m_dcGrid, m_bitmapGrid);
  }
  
  SetBkColor(m_dcGrid, m_crBackColor);

  // fill the grid background
  FillRect(m_dcGrid, &m_rectClient, m_brushBack);

  // draw the plot rectangle:
  // determine how wide the y axis scaling values are
  nCharacters = abs((int)log10(fabs(m_dUpperLimit)));
  nCharacters = max(nCharacters, abs((int)log10(fabs(m_dLowerLimit))));

  // add the units digit, decimal point and a minus sign, and an extra space
  // as well as the number of decimal places to display
  nCharacters = nCharacters + 4 + m_nYDecimals;  

  // adjust the plot rectangle dimensions
  // assume 6 pixels per character (this may need to be adjusted)
//  m_rectPlot.left = m_rectClient.left + 6*(nCharacters);
  m_rectPlot.left = m_rectClient.left;
  m_nPlotWidth    = m_rectPlot.right - m_rectPlot.left;//m_rectPlot.Width();

  // draw the plot rectangle
  oldPen = (HPEN)SelectObject(m_dcGrid, solidPen); 
  MoveToEx(m_dcGrid, m_rectPlot.left, m_rectPlot.top, NULL);
  LineTo(m_dcGrid, m_rectPlot.right+1, m_rectPlot.top);
  LineTo(m_dcGrid, m_rectPlot.right+1, m_rectPlot.bottom+1);
  LineTo(m_dcGrid, m_rectPlot.left, m_rectPlot.bottom+1);
//  LineTo(m_dcGrid, m_rectPlot.left, m_rectPlot.top);
  SelectObject(m_dcGrid, oldPen); 
  DeleteObject(solidPen);

  // draw the dotted lines, 
  // use SetPixel instead of a dotted pen - this allows for a 
  // finer dotted line and a more "technical" look
  nMidGridPix    = (m_rectPlot.top + m_rectPlot.bottom)/2;
  nTopGridPix    = nMidGridPix - m_nPlotHeight/4;
  nBottomGridPix = nMidGridPix + m_nPlotHeight/4;

  for (i=m_rectPlot.left; i<m_rectPlot.right; i+=2) {
    SetPixel(m_dcGrid, i, nTopGridPix,    m_crGridColor);
    SetPixel(m_dcGrid, i, nMidGridPix,    m_crGridColor);
    SetPixel(m_dcGrid, i, nBottomGridPix, m_crGridColor);
  }

  for (i=m_rectPlot.left; i<m_rectPlot.right; i+=10) {
    for (j=m_rectPlot.top; j<m_rectPlot.bottom; j+=2) {
      SetPixel(m_dcGrid, i, j, m_crGridColor);
//      SetPixel(m_dcGrid, i, j, m_crGridColor);
//      SetPixel(m_dcGrid, i, j, m_crGridColor);
    }
  }

#if 0
  // create some fonts (horizontal and vertical)
  // use a height of 14 pixels and 300 weight 
  // (these may need to be adjusted depending on the display)
  axisFont = CreateFont (14, 0, 0, 0, 300,
                       FALSE, FALSE, 0, ANSI_CHARSET,
                       OUT_DEFAULT_PRECIS, 
                       CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, 
                       DEFAULT_PITCH|FF_SWISS, "Arial");
  yUnitFont = CreateFont (14, 0, 900, 0, 300,
                       FALSE, FALSE, 0, ANSI_CHARSET,
                       OUT_DEFAULT_PRECIS, 
                       CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, 
                       DEFAULT_PITCH|FF_SWISS, "Arial");
  
  // grab the horizontal font
  oldFont = (HFONT)SelectObject(m_dcGrid, axisFont);
  
  // y max
  SetTextColor(m_dcGrid, m_crGridColor);
  SetTextAlign(m_dcGrid, TA_RIGHT|TA_TOP);
  sprintf(strTemp, "%.*lf", m_nYDecimals, m_dUpperLimit);
  TextOut(m_dcGrid, m_rectPlot.left-4, m_rectPlot.top, strTemp, _tcslen(strTemp));

  // y min
  SetTextAlign(m_dcGrid, TA_RIGHT|TA_BASELINE);
  sprintf(strTemp, "%.*lf", m_nYDecimals, m_dLowerLimit);
  TextOut(m_dcGrid, m_rectPlot.left-4, m_rectPlot.bottom, strTemp, _tcslen(strTemp));

  // x min
  SetTextAlign(m_dcGrid, TA_LEFT|TA_TOP);
  TextOut(m_dcGrid, m_rectPlot.left, m_rectPlot.bottom+4, "0", 1);

  // x max
  SetTextAlign(m_dcGrid, TA_RIGHT|TA_TOP);
  sprintf(strTemp, "%d", m_nPlotWidth/m_nShiftPixels); 
  TextOut(m_dcGrid, m_rectPlot.right, m_rectPlot.bottom+4, strTemp, _tcslen(strTemp));

  // x units
  SetTextAlign(m_dcGrid, TA_CENTER|TA_TOP);
  TextOut(m_dcGrid, (m_rectPlot.left+m_rectPlot.right)/2, 
                     m_rectPlot.bottom+4, m_strXUnitsString, _tcslen(m_strXUnitsString));

  // restore the font
  SelectObject(m_dcGrid, oldFont);

  // y units
  oldFont = (HFONT)SelectObject(m_dcGrid, yUnitFont);
  SetTextAlign(m_dcGrid, TA_CENTER|TA_BASELINE);
  TextOut(m_dcGrid, (m_rectClient.left+m_rectPlot.left)/2, 
                    (m_rectPlot.bottom+m_rectPlot.top)/2, m_strYUnitsString, _tcslen(m_strYUnitsString));
  SelectObject(m_dcGrid, oldFont);
#endif
  // at this point we are done filling the the grid bitmap, 
  // no more drawing to this bitmap is needed until the setting are changed
  
  // if we don't have one yet, set up a memory dc for the plot
  if (m_dcPlot == NULL) {
    m_dcPlot = CreateCompatibleDC(dc);
    m_bitmapPlot = CreateCompatibleBitmap(dc, m_nClientWidth, m_nClientHeight);
    m_bitmapOldPlot = (HBITMAP)SelectObject(m_dcPlot, m_bitmapPlot);
  }

  // make sure the plot bitmap is cleared
  SetBkColor(m_dcPlot, m_crBackColor);
  FillRect(m_dcPlot, &m_rectClient, m_brushBack);

  // finally, force the plot area to redraw
  InvalidateRect(m_hParentWnd, &m_rectClient, TRUE);
  ReleaseDC(m_hParentWnd, dc);

}

/////////////////////////////////////////////////////////////////////////////
double TGraphCtrl::AppendPoint(double dNewPoint0, double dNewPoint1,
                                double dNewPoint2, double dNewPoint3)
{
  // append a data point to the plot & return the previous point
  double dPrevious;
  
  dPrevious = m_dCurrentPosition[0];
  m_dCurrentPosition[0] = dNewPoint0;
  m_dCurrentPosition[1] = dNewPoint1;
  m_dCurrentPosition[2] = dNewPoint2;
  m_dCurrentPosition[3] = dNewPoint3;
  DrawPoint();
  //Invalidate();
  return dPrevious;

}
 
////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::Paint(HWND hWnd, HDC dc) 
{
  HDC memDC;
  HBITMAP memBitmap;
  HBITMAP oldBitmap; // bitmap originally found in CMemDC

//  RECT rcClient;
//  GetClientRect(hWnd, &rcClient);
//  FillSolidRect(dc, &rcClient, RGB(255, 0, 255));
//  m_nClientWidth = rcClient.right - rcClient.left;
//  m_nClientHeight = rcClient.bottom - rcClient.top;

  // no real plotting work is performed here, 
  // just putting the existing bitmaps on the client

  // to avoid flicker, establish a memory dc, draw to it 
  // and then BitBlt it to the client
  memDC = CreateCompatibleDC(dc);
  memBitmap = (HBITMAP)CreateCompatibleBitmap(dc, m_nClientWidth, m_nClientHeight);
  oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

  if (memDC != NULL) {
    // first drop the grid on the memory dc
    BitBlt(memDC, 0, 0, m_nClientWidth, m_nClientHeight, m_dcGrid, 0, 0, SRCCOPY);
    // now add the plot on top as a "pattern" via SRCPAINT.
    // works well with dark background and a light plot
    BitBlt(memDC, 0, 0, m_nClientWidth, m_nClientHeight, m_dcPlot, 0, 0, SRCPAINT);  //SRCPAINT
    // finally send the result to the display
    BitBlt(dc, 0, 0, m_nClientWidth, m_nClientHeight, memDC, 0, 0, SRCCOPY);
  }
  SelectObject(memDC, oldBitmap);
  DeleteObject(memBitmap);
  DeleteDC(memDC);
}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::DrawPoint()
{
  // this does the work of "scrolling" the plot to the left
  // and appending a new data point all of the plotting is 
  // directed to the memory based bitmap associated with m_dcPlot
  // the will subsequently be BitBlt'd to the client in Paint
  
  int currX, prevX, currY, prevY;
  HPEN oldPen;
  RECT rectCleanUp;

  if (m_dcPlot != NULL) {
    // shift the plot by BitBlt'ing it to itself 
    // note: the m_dcPlot covers the entire client
    //       but we only shift bitmap that is the size 
    //       of the plot rectangle
    // grab the right side of the plot (exluding m_nShiftPixels on the left)
    // move this grabbed bitmap to the left by m_nShiftPixels

    BitBlt(m_dcPlot, m_rectPlot.left, m_rectPlot.top+1, 
                    m_nPlotWidth, m_nPlotHeight, m_dcPlot, 
                    m_rectPlot.left+m_nShiftPixels, m_rectPlot.top+1, 
                    SRCCOPY);

    // establish a rectangle over the right side of plot
    // which now needs to be cleaned up proir to adding the new point
    rectCleanUp = m_rectPlot;
    rectCleanUp.left  = rectCleanUp.right - m_nShiftPixels;

    // fill the cleanup area with the background
    FillRect(m_dcPlot, &rectCleanUp, m_brushBack);

    // draw the next line segement
    for (int i = 0; i < MAX_PLOTS; i++) {

        // grab the plotting pen
        oldPen = (HPEN)SelectObject(m_dcPlot, m_penPlot[i]);

        // move to the previous point
        prevX = m_rectPlot.right-m_nPlotShiftPixels;
        prevY = m_rectPlot.bottom - 
                (long)((m_dPreviousPosition[i] - m_dLowerLimit) * m_dVerticalFactor);
        MoveToEx(m_dcPlot, prevX, prevY, NULL);

        // draw to the current point
        currX = m_rectPlot.right-m_nHalfShiftPixels;
        currY = m_rectPlot.bottom -
                (long)((m_dCurrentPosition[i] - m_dLowerLimit) * m_dVerticalFactor);
        LineTo(m_dcPlot, currX, currY);

        // restore the pen 
        SelectObject(m_dcPlot, oldPen);

        // if the data leaks over the upper or lower plot boundaries
        // fill the upper and lower leakage with the background
        // this will facilitate clipping on an as needed basis
        // as opposed to always calling IntersectClipRect

        if ((prevY <= m_rectPlot.top) || (currY <= m_rectPlot.top)) {
            RECT rc;
            rc.bottom = m_rectPlot.top+1;
            rc.left = prevX;
            rc.right = currX+1;
            rc.top = m_rectClient.top;
            FillRect(m_dcPlot, &rc, m_brushBack);
        }
        if ((prevY >= m_rectPlot.bottom) || (currY >= m_rectPlot.bottom)) {
            RECT rc;
            rc.bottom = m_rectClient.bottom+1;
            rc.left = prevX;
            rc.right = currX+1;
            rc.top = m_rectPlot.bottom+1;
            //RECT rc(prevX, m_rectPlot.bottom+1, currX+1, m_rectClient.bottom+1);
            FillRect(m_dcPlot, &rc, m_brushBack);
        }

        // store the current point for connection to the next point
        m_dPreviousPosition[i] = m_dCurrentPosition[i];
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::Resize(void) 
{
  // NOTE: Resize automatically gets called during the setup of the control
  GetClientRect(m_hWnd, &m_rectClient);

  // set some member variables to avoid multiple function calls
  m_nClientHeight = m_rectClient.bottom - m_rectClient.top;//m_rectClient.Height();
  m_nClientWidth  = m_rectClient.right - m_rectClient.left;//m_rectClient.Width();

  // the "left" coordinate and "width" will be modified in 
  // InvalidateCtrl to be based on the width of the y axis scaling
#if 0
  m_rectPlot.left   = 20;  
  m_rectPlot.top    = 10;
  m_rectPlot.right  = m_rectClient.right-10;
  m_rectPlot.bottom = m_rectClient.bottom-25;
#else
  m_rectPlot.left   = 0;  
  m_rectPlot.top    = -1;
  m_rectPlot.right  = m_rectClient.right-0;
  m_rectPlot.bottom = m_rectClient.bottom-0;
#endif

  // set some member variables to avoid multiple function calls
  m_nPlotHeight = m_rectPlot.bottom - m_rectPlot.top;//m_rectPlot.Height();
  m_nPlotWidth  = m_rectPlot.right - m_rectPlot.left;//m_rectPlot.Width();

  // set the scaling factor for now, this can be adjusted 
  // in the SetRange functions
  m_dVerticalFactor = (double)m_nPlotHeight / m_dRange; 
}


/////////////////////////////////////////////////////////////////////////////
void TGraphCtrl::Reset()
{
  // to clear the existing data (in the form of a bitmap)
  // simply invalidate the entire control
  InvalidateCtrl();
}


extern TGraphCtrl PerformancePageCpuUsageHistoryGraph;
extern TGraphCtrl PerformancePageMemUsageHistoryGraph;
extern HWND hPerformancePageCpuUsageHistoryGraph;
extern HWND hPerformancePageMemUsageHistoryGraph;

LRESULT CALLBACK GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT		rcClient;
	HDC			hdc;
	PAINTSTRUCT	ps;
	//LONG        WindowId;
    //TGraphCtrl* pGraphCtrl;
	
	switch (message) {
	case WM_ERASEBKGND:
		return TRUE;
	//
	// Filter out mouse  & keyboard messages
	//
	//case WM_APPCOMMAND:
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
	//case WM_MOUSEWHEEL:
	case WM_NCHITTEST:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	//case WM_NCMOUSEHOVER:
	//case WM_NCMOUSELEAVE:
	case WM_NCMOUSEMOVE:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	//case WM_NCXBUTTONDBLCLK:
	//case WM_NCXBUTTONDOWN:
	//case WM_NCXBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	//case WM_XBUTTONDBLCLK:
	//case WM_XBUTTONDOWN:
	//case WM_XBUTTONUP:
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
//        pGraphCtrl = TGraphCtrl::LookupGraphCtrl(hWnd);
//        if (pGraphCtrl) pGraphCtrl->Resize(wParam, HIWORD(lParam), LOWORD(lParam));
        if (hWnd == hPerformancePageMemUsageHistoryGraph) {
            PerformancePageMemUsageHistoryGraph.Resize();
            PerformancePageMemUsageHistoryGraph.InvalidateCtrl();
        }
        if (hWnd == hPerformancePageCpuUsageHistoryGraph) {
            PerformancePageCpuUsageHistoryGraph.Resize();
            PerformancePageCpuUsageHistoryGraph.InvalidateCtrl();
        }
        return 0;
        break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
//        pGraphCtrl = TGraphCtrl::LookupGraphCtrl(hWnd);
//        if (pGraphCtrl) pGraphCtrl->Paint(hdc);
        GetClientRect(hWnd, &rcClient);
        if (hWnd == hPerformancePageMemUsageHistoryGraph) {
            PerformancePageMemUsageHistoryGraph.Paint(hWnd, hdc);
        }
        if (hWnd == hPerformancePageCpuUsageHistoryGraph) {
            PerformancePageCpuUsageHistoryGraph.Paint(hWnd, hdc);
        }
		EndPaint(hWnd, &ps);
		return 0;
	}
	
	//
	// We pass on all non-handled messages
	//
	return CallWindowProc((WNDPROC)OldGraphCtrlWndProc, hWnd, message, wParam, lParam);
}


#if 0

#include "GraphCtrl.h"

TGraphCtrl* TGraphCtrl::pCtrlArray[] = { 0, 0, 0, 0 };
int TGraphCtrl::CtrlCount = 0;

TGraphCtrl* TGraphCtrl::LookupGraphCtrl(HWND hWnd)
{
    for (int i = 0; i < MAX_CTRLS; i++) {
        if (pCtrlArray[i] != 0) {
            if (pCtrlArray[i]->m_hParentWnd == hWnd) {
                return pCtrlArray[i];
            }
        }
    }
    return NULL;
}

#endif

