/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   FullWP.c                                    *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  window proc for full (show) screen          *
*   date:   03/19/87 @ 16:55                            *
*                                                       *
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NOSYSMETRICS
#undef NORASTEROPS
#undef NOKERNEL

#include <windows.h>
#include "port1632.h"

#include "pbrush.h"

LONG FAR PASCAL FullWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int x, y, wid, hgt;
   PAINTSTRUCT ps;
   HCURSOR oldcsr;
   HPALETTE hOldPalette = NULL;

   switch(message) {
   case WM_PAINT:
      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
      x = y = 0;
      BeginPaint(hWnd,&ps);
      if(hPalette) {
         hOldPalette = SelectPalette(ps.hdc, hPalette, 0);
         RealizePalette(ps.hdc);
      }
      wid = GetSystemMetrics(SM_CXSCREEN);
      hgt = GetSystemMetrics(SM_CYSCREEN);
      if(imageWid < wid)
         x = (wid - imageWid) >> 1;
      if(imageHgt < hgt)
         y = (hgt - imageHgt) >> 1;
      BitBlt(ps.hdc, x, y, imageWid, imageHgt, hdcWork, 0, 0, SRCCOPY);
      if(hOldPalette)
         SelectPalette(ps.hdc, hOldPalette, 0);
      EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
      SetCursor(oldcsr);
      break;
   
   case WM_MOUSEMOVE:
      SetCursor(LoadCursor(NULL, IDC_ARROW));
      break;

   case WM_LBUTTONDOWN:
   case WM_RBUTTONDOWN:
   case WM_CHAR:
   case WM_KEYDOWN:
      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
      DestroyWindow(hWnd);
      SetFocus(pbrushWnd[PAINTid]);
      SetCursor(oldcsr);
      break;

   default:
      return(DefWindowProc(hWnd, message, wParam, lParam));
      break;
   }

   return(FALSE);
}
