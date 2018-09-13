/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*							*
*	file:	RectDP.c				*
*	system: PC Paintbrush for MS-Windows		*
*	descr:	rect draw proc				*
*	date:	03/16/87 @ 15:15			*
********************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


#define MAXCVPTS 4

extern POINT polyPts[];
extern int theSize, theForeg;
extern DWORD *rgbColor;

extern BOOL bExchanged;
extern RECT rDirty;

int Curve(HDC dc, POINT *curvePt, UINT numPts)
{
  REGISTER int i, segs;
  int count = MAXCVPTS;

  segs = max(5,(abs(curvePt[0].x - curvePt[1].x) + abs(curvePt[0].y -
	 curvePt[1].y) + abs(curvePt[1].x - curvePt[2].x) + abs(curvePt[1].y -
	 curvePt[2].y)) / 20);

   curvePt[count++] = curvePt[0];

   if(numPts == 3) {
      for(i=1; i<segs; ++i, ++count) {
           curvePt[count].x = (int)(((long)curvePt[0].x*(segs-i)*(segs-i)
                     + 2*(long)curvePt[1].x*(segs-i)*i
                     + (long)curvePt[2].x*i*i)/((long)segs*segs));

           curvePt[count].y = (int)(((long)curvePt[0].y*(segs-i)*(segs-i)
                     + 2*(long)curvePt[1].y*(segs-i)*i
                     + (long)curvePt[2].y*i*i)/((long)segs*segs));
      }
   } else if(numPts == 4) {
      for(i=1; i<segs; ++i, ++count) {
           curvePt[count].x = (int)(((long)curvePt[0].x*(segs-i)*(segs-i)*(segs-i)
                     + 3*(long)curvePt[1].x*(segs-i)*(segs-i)*i
                     + 3*(long)curvePt[2].x*(segs-i)*i*i
                     + (long)curvePt[3].x*i*i*i)/((long)segs*segs*segs));

           curvePt[count].y = (int)(((long)curvePt[0].y*(segs-i)*(segs-i)*(segs-i)
                     + 3*(long)curvePt[1].y*(segs-i)*(segs-i)*i
                     + 3*(long)curvePt[2].y*(segs-i)*i*i
                     + (long)curvePt[3].y*i*i*i)/((long)segs*segs*segs));
      }
   } else
      return(FALSE);

   curvePt[count] = curvePt[numPts-1];

   /* are we drawing the actual box or just the outline? */
   if(GetROP2(dc) == R2_COPYPEN) {
        if(bExchanged) {
          PasteDownRect(rDirty.left, rDirty.top,
               rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);
        }

    	{
        	RECT rcRect;
        	int i;

                for (i=0;i<=segs;i++) {
        	    curvePt[MAXCVPTS+i].x += imageView.left;
        	    curvePt[MAXCVPTS+i].y += imageView.top;
        	}
        	/* get bounding rect of Polyline (ignoring pen width) */
        	PolyRect(curvePt+MAXCVPTS, segs+1, &rcRect);

        	/* Inflate bounding box to surround the thick pen */
        	CompensateForPen(hdcWork, &rcRect);

        	Polyline(hdcWork, curvePt+MAXCVPTS, segs+1);

        	BitBlt(dc, rcRect.left - imageView.left,
        		   rcRect.top - imageView.top,
        		   rcRect.right - rcRect.left,
        		   rcRect.bottom - rcRect.top,
        	       hdcWork, rcRect.left,rcRect.top,
        	       SRCCOPY);

        	UnionWithRect(&rDirty, &rcRect);

            AdviseDataChange();
    	}
   } else
      Polyline(dc, curvePt+MAXCVPTS, segs+1);

   return(TRUE);
}

static int num1Pts;

LONG APIENTRY DrawCurve(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   if(num1Pts == 2) {
      polyPts[1].x = lprBounds->right;
      polyPts[1].y = lprBounds->bottom;

      MMoveTo(dstDC, polyPts[0].x, polyPts[0].y);
      return(LineTo(dstDC, polyPts[1].x, polyPts[1].y));
   } else {
      polyPts[num1Pts-2].x = lprBounds->right;
      polyPts[num1Pts-2].y = lprBounds->bottom;

      return(Curve(dstDC, polyPts, num1Pts));
   }
}

void CurveDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   POINT newPt;
   static BOOL drawing = FALSE;
   static HDC dstDC = NULL;
   static int nSavedDC,iBk;
   static RECT rcReturn;

   HPEN pen, hOldPen;

   LONG2POINT(lParam,newPt);
   switch(message) {
   case WM_LBUTTONDOWN:
      if(!drawing) {
         if(!(dstDC = GetDisplayDC(hWnd)))
            return;
         nSavedDC = SaveDC(dstDC);

         drawing = TRUE;

         SelectObject(dstDC, GetStockObject(WHITE_PEN));
         SetROP2(dstDC, R2_XORPEN);

    	 polyPts[0] = newPt;
    	 num1Pts = 1;
      } else {
         DrawCurve(dstDC, &rcReturn, wParam);
      }

      polyPts[num1Pts] = polyPts[num1Pts-1];
      polyPts[num1Pts-1] = newPt;
      ++num1Pts;

      rcReturn.left = rcReturn.right = newPt.x;
      rcReturn.top = rcReturn.bottom = newPt.y;
      if(TrackTool(hWnd, DrawCurve, &rcReturn, &wParam, dstDC) == WM_RBUTTONDOWN) {
         drawing = FALSE;
    	 CurveDP(hWnd, WM_TERMINATE, wParam, lParam);
         return;
      }

      if(num1Pts >= MAXCVPTS) {
         drawing = FALSE;

    	 iBk=SetBkMode(hdcWork, OPAQUE);
         SetROP2(dstDC, R2_COPYPEN);

         if(!(pen = CreatePen(PS_INSIDEFRAME, theSize, rgbColor[theForeg])))
            goto Error1;

         hOldPen = SelectObject(hdcWork, pen);
         DrawCurve(dstDC, &rcReturn, wParam);

         if (hOldPen)
    	    SelectObject(hdcWork, hOldPen);
         DeleteObject(pen);
Error1:
    	 CurveDP(hWnd, WM_TERMINATE, wParam, lParam);
      } else {
         DrawCurve(dstDC, &rcReturn, wParam);
      }
      break;

   case WM_RBUTTONDOWN:
   case WM_TERMINATE:
      if(drawing) {
         DrawCurve(dstDC, &rcReturn, wParam) != 0;
         drawing = FALSE;
      }

      if(dstDC) {
    	 SetBkMode(hdcWork, iBk);
	     RestoreDC(dstDC, nSavedDC);
         ReleaseDC(hWnd, dstDC);
         dstDC = NULL;
      }
      break;

   default:
      break;
   }
}
