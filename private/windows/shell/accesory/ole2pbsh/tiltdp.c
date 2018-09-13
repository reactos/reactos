/****************************Module*Header******************************\
* Module Name: tiltdp.c                                                 *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


static PARAL tiltParal;
extern RECT rDirty;
extern BOOL clearFlag;

LONG APIENTRY
DrawDotParal(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   DotParal(dstDC, &tiltParal);
   tiltParal.botLeft.x = lprBounds->right;
   tiltParal.botRight.x = tiltParal.botLeft.x + pickWid - 1;
   DotParal(dstDC, &tiltParal);

   return TRUE;
}

void
TiltDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
#ifndef WIN32
   FARPROC lpfnTiltBlt;
#endif
   HDC     paintDC;
   BOOL    bCancel;
   RECT    scaleRect;
   HCURSOR oldCsr;
   POINT newPt;
   BOOL b;

   LONG2POINT(lParam,newPt);
   switch (code)
   {
       case WM_HIDECURSOR:
           HideCsr(NULL, hWnd, CROSScsr);
           break;

       /* initiate rubber banding of tilt parallelogram */
       case WM_LBUTTONDOWN:
           scaleRect.left = scaleRect.right = newPt.x;
           scaleRect.top = scaleRect.bottom = newPt.y;

           tiltParal.topLeft.x = tiltParal.botLeft.x = newPt.x;
           tiltParal.topRight.x = tiltParal.botRight.x = newPt.x + pickWid - 1;
           tiltParal.topLeft.y = tiltParal.topRight.y = newPt.y;
           tiltParal.botLeft.y = tiltParal.botRight.y = newPt.y + pickHgt - 1;

           paintDC = GetDisplayDC(pbrushWnd[PAINTid]);
           if (paintDC)
               DotParal(paintDC, &tiltParal);

           /* get scaled rect from user */
           bCancel = (TrackTool(hWnd, DrawDotParal, &scaleRect, &wParam, NULL)
                       == WM_RBUTTONDOWN);

           if (!bCancel)
           {
               oldCsr = SetCursor(LoadCursor(NULL,IDC_WAIT));

               if (paintDC)
                   DotParal(paintDC,&tiltParal);

               /* If clear flag set then clear pick area */
               if (clearFlag)
                   ClearPickArea();

               /* Stretch work buffer onto screen */
               if (paintDC)
               {
                   b=SetWindowOrgEx(hdcWork, -tiltParal.topLeft.x,
                                -tiltParal.topLeft.y,NULL);

#ifndef WIN32
                   lpfnTiltBlt = MakeProcInstance((FARPROC)TiltBlt, hInst);
                   LineDDA(0, 0, tiltParal.botLeft.x - tiltParal.topLeft.x,
                           pickHgt - 1, (LINEDDAPROC)lpfnTiltBlt, (LPARAM) &hdcWork);
                   FreeProcInstance(lpfnTiltBlt);
#else
                   LineDDA(0, 0, tiltParal.botLeft.x - tiltParal.topLeft.x,
                           pickHgt - 1, (LINEDDAPROC)TiltBlt, (LPARAM) &hdcWork);
#endif
                   SetWindowOrgEx(hdcWork, 0, 0,NULL);

                   scaleRect.left = min(tiltParal.topLeft.x,
                                        tiltParal.botLeft.x) + imageView.left;
                   scaleRect.right = scaleRect.left + abs(tiltParal.botLeft.x   - tiltParal.topLeft.x) + pickWid;
                   scaleRect.top = min(tiltParal.topLeft.y,
                                       tiltParal.botLeft.y) + imageView.top;
                   scaleRect.bottom = scaleRect.top + pickHgt;

                   b=BitBlt(paintDC,scaleRect.left - imageView.left,
                                    scaleRect.top - imageView.top,
                                    scaleRect.right - scaleRect.left,
                                    scaleRect.bottom - scaleRect.top,
                            hdcWork,scaleRect.left, scaleRect.top,SRCCOPY);

                   UnionWithRect(&rDirty,&scaleRect);

                   AdviseDataChange();

                   UpdFlag(TRUE);
               }

               SetCursor(oldCsr);
           }
           else if (paintDC)
           {
               DotParal(paintDC, &tiltParal);
           }

           if (paintDC)
               ReleaseDC(pbrushWnd[PAINTid], paintDC);
           break;

       case WM_TERMINATE:
       case WM_PICKTILT:
           EnableOutline(TRUE);
           CheckMenuItem(ghMenuFrame, PICKtilt, MF_UNCHECKED);
           DrawProc = dpArray[theTool];
           if (code == WM_TERMINATE) {
               DOUTR(L"WM_PICKTILT: sending WM_TERMINATE");
               SendMessage(hWnd, WM_TERMINATE, 0, 0L);
           }
           break;

       case WM_PICKSG:
           CheckMenuItem(ghMenuFrame, PICKtilt, MF_UNCHECKED);
           CheckMenuItem(ghMenuFrame, PICKsg, MF_CHECKED);
           ChangeCutCopy(ghMenuFrame, MF_GRAYED);
           DrawProc = ShrGroDP;
           break;
   }
}
