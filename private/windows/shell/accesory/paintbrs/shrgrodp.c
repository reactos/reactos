/****************************Module*Header******************************\
* Module Name: shrgrodp.c                                               *
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
#include "port1632.h"

#include "pbrush.h"
#include "fixedpt.h"
#include "pbserver.h"

extern BOOL clearFlag;
extern HDC pickDC, monoDC;
extern int pickWid, pickHgt;
extern HWND pbrushWnd[];
extern DPPROC DrawProc;
extern DPPROC dpArray[];
extern int theTool;

extern RECT rDirty;

void ShrGroDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   HDC     paintDC;
   int     scaleWid, scaleHgt;
   RECT    scaleRect;
   HMENU   hMenu;
   HCURSOR oldCsr;
   BOOL    bCancel;
   POINT  newPt;

   LONG2POINT(lParam,newPt);
   switch (code) {
       case WM_HIDECURSOR:
           HideCsr(NULL, hWnd, CROSScsr);
           break;

       /* initiate rubber banding of scale rect */
       case WM_LBUTTONDOWN:
           scaleRect.left = scaleRect.right = newPt.x;
           scaleRect.top = scaleRect.bottom = newPt.y;

           /* get scaled rect from user */
           bCancel = (TrackTool(hWnd, DrawShrGroRect, &scaleRect, &wParam,
                                NULL) == WM_RBUTTONDOWN);

           paintDC = GetDisplayDC(hWnd);
           if (paintDC) {
               DrawShrGroRect(paintDC, &scaleRect, wParam);
               NormalizeRect(&scaleRect);
           } else
               bCancel = TRUE;

           /* if bounding rect is empty then just cancel operation */
           if (IsRectEmpty(&scaleRect))
               bCancel = TRUE;

           if (!bCancel) {
               scaleWid = scaleRect.right - scaleRect.left;
               scaleHgt = scaleRect.bottom - scaleRect.top;

               oldCsr = SetCursor(LoadCursor(NULL,IDC_WAIT));

               /* If clear flag set then clear pick area */
               if (clearFlag)
                  ClearPickArea();

               /* draw what we want to stretch in work buffer */
               scaleRect.left += imageView.left;
               scaleRect.right += imageView.left;
               scaleRect.top += imageView.top;
               scaleRect.bottom += imageView.top;

               /* Stretch work buffer onto screen */
               BitBlt(hdcWork, scaleRect.left, scaleRect.top,
                               scaleWid, scaleHgt,
                      hdcWork, scaleRect.left, scaleRect.top,
                      SRCCOPY);

               MaskStretchBlt(hdcWork, scaleRect.left, scaleRect.top,
                                       scaleWid, scaleHgt,
                              pickDC, monoDC, 0, 0, pickWid, pickHgt);


               BitBlt(paintDC, scaleRect.left - imageView.left,
                               scaleRect.top - imageView.top,
                               scaleWid, scaleHgt,
                      hdcWork, scaleRect.left, scaleRect.top,
                      SRCCOPY);

               UnionWithRect(&rDirty,&scaleRect);

               if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);

               UpdFlag(TRUE);

               SetCursor(oldCsr);
           }

           if (paintDC)
               ReleaseDC(hWnd, paintDC);
           break;

       case WM_TERMINATE:
       case WM_PICKSG:
           EnableOutline(TRUE);
           hMenu = GetMenu(pbrushWnd[PARENTid]);
           CheckMenuItem(hMenu, PICKsg, MF_UNCHECKED);
           ChangeCutCopy(hMenu, MF_ENABLED);
           DrawProc = dpArray[theTool];
           if (code == WM_TERMINATE)
               SendMessage(hWnd, WM_TERMINATE, 0, 0L);
           break;

       case WM_PICKTILT:
           hMenu = GetMenu(pbrushWnd[PARENTid]);
           CheckMenuItem(hMenu, PICKsg, MF_UNCHECKED);
           CheckMenuItem(hMenu, PICKtilt, MF_CHECKED);
           ChangeCutCopy(hMenu, MF_ENABLED);
           DrawProc = TiltDP;
           break;
   }
}
