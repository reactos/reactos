/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   RollerDP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  flood fill draw proc                        *
*   date:   01/28/87 @ 15:35                            *
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NOKERNEL
#undef NOMB

#include <windows.h>
#include "port1632.h"

//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

extern BOOL bExchanged;
extern RECT rDirty;

void RollerDP(HWND hWnd, UINT code, WPARAM mouseKeys, LONG lParam)
{
   POINT newPt;
   HCURSOR oldcsr;
   RECT rcWind;
   DWORD rgb;
   WORD errMsg = 0;
   HBRUSH oldbrush, brush;

   LONG2POINT(lParam,newPt);
   switch(code) {
/* flood fill an area */
   case WM_LBUTTONDOWN:
       if(bExchanged)
	  PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

      GetWindowRect(hWnd, &rcWind);
      if(inMagnify) {
	 newPt.x = imageView.left + zoomView.left + newPt.x/zoomAmount;
	 newPt.y = imageView.top + zoomView.top + newPt.y/zoomAmount;
      }
      else {
	 newPt.x += imageView.left;
	 newPt.y += imageView.top;

      }

      if(!(brush = CreateSolidBrush(rgbColor[theForeg]))) {
         errMsg = IDSNoBrush;
         goto Error2;
      }
      oldbrush = SelectObject(hdcWork, brush);

      rgb = (code == WM_LBUTTONDOWN) ? GetPixel(hdcWork, newPt.x, newPt.y)
                                     : rgbColor[theBackg];
      {
      int fResult = TRUE;


      fResult = ExtFloodFill(hdcWork, newPt.x, newPt.y, rgb,
	     (UINT)((code == WM_LBUTTONDOWN) ? FLOODFILLSURFACE : FLOODFILLBORDER));

      rDirty.left = 0;
      rDirty.top  = 0;
      rDirty.right = nNewImageWidth;
      rDirty.bottom = nNewImageHeight;

      InvalidateRect(hWnd,NULL,FALSE);
      UpdateWindow(hWnd);
      if (fOLE)
	   SendDocChangeMsg(vpdoc, OLE_CHANGED);

      }

      if(oldbrush)
	 SelectObject(hdcWork, oldbrush);
      DeleteObject(brush);
Error2:
      if(errMsg)
         SimpleMessage(errMsg, NULL, MB_OK | MB_ICONEXCLAMATION);

      SetCursor(oldcsr);
      break;

   default:
      break;
   }
}
