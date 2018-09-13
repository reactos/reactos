/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*							*
*	file:	RndRctDP.c				*
*	system: PC Paintbrush for MS-Windows		*
*	descr:	round rect draw proc			*
*	date:	03/16/87 @ 15:15			*
********************************************************/

#include "onlypbr.h"
#undef NOMINMAX
#undef NOWINMESSAGES
#undef NORASTEROPS

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

extern BOOL bExchanged;
extern RECT rDirty;

static int xRoundMax, yRoundMax;

LONG APIENTRY DrawRndRct(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int ovalWid, ovalHgt;
   RECT rcTemp;

   rcTemp = *lprBounds;
   ConstrainRect(&rcTemp, NULL, wParam);

   ovalWid = min(abs(rcTemp.right - rcTemp.left)/ROUNDdiv, xRoundMax);
   ovalHgt = min(abs(rcTemp.bottom - rcTemp.top)/ROUNDdiv, yRoundMax);

   /* are we drawing the actual box or just the outline? */
   if(GetROP2(dstDC) == R2_COPYPEN) {

       if(bExchanged)
	  PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

       rcTemp.left += imageView.left;
       rcTemp.right += imageView.left;
       rcTemp.top += imageView.top;
       rcTemp.bottom += imageView.top;

       RoundRect(hdcWork, rcTemp.left,
			  rcTemp.top,
			  rcTemp.right,
			  rcTemp.bottom,
			  xRoundMax, yRoundMax);

       BitBlt(dstDC, rcTemp.left - imageView.left,
		     rcTemp.top  - imageView.top,
		     rcTemp.right - rcTemp.left,
		     rcTemp.bottom - rcTemp.top,
	      hdcWork, rcTemp.left,
		       rcTemp.top,
	      SRCCOPY);

       UnionWithRect(&rDirty, &rcTemp);

      if (fOLE)
	   SendDocChangeMsg(vpdoc, OLE_CHANGED);

   } else
      RoundRect(dstDC, rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom,
            xRoundMax, yRoundMax);

   return(TRUE);
}

void RndRctDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   HDC dstDC;
   RECT rcReturn;
   HPEN pen, hOldPen;
   HBRUSH brush, hOldBrush;
   int nSavedDC;
   POINT newPt;
   int iDraw;

   LONG2POINT(lParam,newPt);

   if(message == WM_LBUTTONDOWN) {
      GetAspct(-ROUNDmax, &xRoundMax, &yRoundMax);

      rcReturn.left = rcReturn.right = newPt.x;
      rcReturn.top = rcReturn.bottom = newPt.y;
      if(TrackTool(hWnd, DrawRndRct, &rcReturn, &wParam, NULL)
            != WM_RBUTTONDOWN) {
         if(!(dstDC = GetDisplayDC(hWnd)))
            goto Error1;
	 nSavedDC = SaveDC(hdcWork);

	 SetBkMode(hdcWork, OPAQUE);
	 SetROP2(hdcWork, R2_COPYPEN);
	 iDraw=SetROP2(dstDC, R2_COPYPEN);

         if(theTool == RNDRECTFILLtool)
            brush = CreateSolidBrush(rgbColor[theForeg]);
         else
            brush = GetStockObject(NULL_BRUSH);
         if(!brush)
            goto Error2;
	 hOldBrush = SelectObject(hdcWork, brush);

         if(theSize > 0)
            pen = CreatePen(PS_INSIDEFRAME, theSize, 
               rgbColor[(theTool == RNDRECTFRAMEtool) ? theForeg : theBackg]);
         else
            pen = GetStockObject(NULL_PEN);
         if(!pen)
            goto Error3;
	 hOldPen = SelectObject(hdcWork, pen);

	 DrawRndRct(dstDC, &rcReturn, wParam);

         if(hOldPen)
	    SelectObject(hdcWork, hOldPen);
         if(theSize > 0)
            DeleteObject(pen);
Error3:
         if(hOldBrush)
	    SelectObject(hdcWork, hOldBrush);
         if(theTool == RNDRECTFILLtool)
            DeleteObject(brush);
Error2:
	 RestoreDC(hdcWork, nSavedDC);
	 SetROP2(dstDC, iDraw);
	 ReleaseDC(hWnd, dstDC);
Error1:
         ;
      }
   }
}
