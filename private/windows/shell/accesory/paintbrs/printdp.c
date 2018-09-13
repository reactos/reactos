/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   PrintDP.c                                   *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  print partial image draw proc               *
*   date:   03/24/87 @ 12:15                            *
*                                                       *
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"
#include "fixedpt.h"

BOOL IsConstrained;
RECT zoomRect;

LONG APIENTRY DrawPrint(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   RECT rcTemp, rcZoom;

   if(!IsConstrained) {
      GetClientRect(zoomOutWnd, &rcTemp);
      ComputeZoomRect(&rcTemp, &zoomRect);
      rcZoom = zoomRect;

      ClipPointToRect((LPPOINT)lprBounds  , &rcZoom);
      ClipPointToRect((LPPOINT)lprBounds+1, &rcZoom);

      ClientToScreen(zoomOutWnd, (LPPOINT) &rcZoom);
      ClientToScreen(zoomOutWnd, ((LPPOINT) &rcZoom) + 1);
      ClipCursor(&rcZoom);

      IsConstrained = TRUE;
   }

   rcTemp = *lprBounds;
   ConstrainRect(&rcTemp, &zoomRect, wParam);

   /* are we drawing the actual box or just the outline? */
   Rectangle(dstDC, rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom);

   return(TRUE);
}

void PrintDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   RECT rcReturn;
   POINT thePt;

   LONG2POINT(lParam,thePt);
   switch(code) {
   case WM_LBUTTONDOWN:
      IsConstrained = FALSE;
      rcReturn.left = rcReturn.right = thePt.x;
      rcReturn.top = rcReturn.bottom = thePt.y;

      if(TrackTool(hWnd, DrawPrint, &rcReturn, &wParam, NULL)
            == WM_RBUTTONDOWN) {
         SetRectEmpty(&imageRect);
      } else {
         ConstrainRect(&rcReturn, &zoomRect, wParam);

         if(rcReturn.left < rcReturn.right) {
            imageRect.left = rcReturn.left;
            imageRect.right = rcReturn.right;
         } else {
            imageRect.left = rcReturn.right;
            imageRect.right = rcReturn.left;
         }
         if(rcReturn.top < rcReturn.bottom) {
            imageRect.top = rcReturn.top;
            imageRect.bottom = rcReturn.bottom;
         } else {
            imageRect.top = rcReturn.bottom;
            imageRect.bottom = rcReturn.top;
         }
      }

      break;
   }
}
