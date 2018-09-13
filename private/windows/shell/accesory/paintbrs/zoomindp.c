/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   ZoomInDP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  zoom-in draw proc                           *
*   date:   03/20/87 @ 15:00                            *
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NOKERNEL
#undef NOKEYSTATES
#undef NOMENUS
#undef NOMINMAX
#undef NORASTEROPS
#undef NOSCROLL
#undef NOSYSMETRICS

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

extern BOOL imageFlag;
extern BOOL drawing;
extern BOOL inMagnify, mouseFlag;
extern BOOL updateFlag;
extern DWORD *rgbColor;
extern HWND pbrushWnd[], mouseWnd;
extern DPPROC DrawProc;
extern int cursTool;
extern int paintWid, paintHgt, zoomWid, zoomHgt, zoomAmount;
extern int theTool, theForeg, theBackg;
extern LPTSTR DrawCursor;
extern POINT csrPt;
extern POINT viewOrg, viewExt;
extern DPPROC dpArray[];
extern RECT imageView, zoomView;
extern RECT pbrushRct[];
extern struct   csstat CursorStat;
extern int imageWid, imageHgt;

extern BOOL bExchanged;
extern RECT rDirty;

#ifdef DEBUG
extern WORD wGridLines;
#endif


static UINT ZoAcMe1[] = {EDITpasteFrom, MISCzoomIn, FILEshow, WINDOWtool,
      WINDOWpalette, MISCmousePos, (UINT)-1};
static UINT ZoAcMe2[] = {0, 3, 5,  (UINT)-1};

void ZoomInDP(HWND hWnd, UINT code, WPARAM mouseKeys, LONG lParam)
{
   int i;
   HDC paintDC;
   LPPAINTSTRUCT lpps;
   HMENU hmenu;
   RECT rWind;
   POINT newPt;

   LONG2POINT(lParam,newPt);

   switch(code) {
   case WM_PAINT:
      lpps = (LPPAINTSTRUCT )lParam;
      XorCsr(lpps->hdc,csrPt,ZOOMINcsr);
      break;

   case WM_ZOOMACCEPT:
   case WM_ZOOMUNDO:
   case WM_RBUTTONDOWN:
   case WM_TERMINATE:
      DrawProc = dpArray[theTool];
      DrawCursor = szPbCursor(theTool);
      cursTool = theTool;
      PbSetCursor(DrawCursor);
      drawing = inMagnify = FALSE;
      CursorStat.noted = FALSE;

/* Reset the paint window */
      GetClientRect(pbrushWnd[PARENTid], &rWind);
      SendMessage(pbrushWnd[PARENTid], WM_SIZE,
            0, MAKELONG(rWind.right, rWind.bottom));
      GetClientRect(hWnd, &rWind);
      SendMessage(hWnd, WM_SIZE, 0, MAKELONG(rWind.right, rWind.bottom));
      InvalidateRect(hWnd, NULL, FALSE);
      SetScrollPos(hWnd, SB_HORZ, imageView.left, TRUE);
      SetScrollPos(hWnd, SB_VERT, imageView.top, TRUE);

      hmenu = GetMenu(pbrushWnd[PARENTid]);
      EnableMenuItem(hmenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItems(hmenu, ZoAcMe1, MF_ENABLED | MF_BYCOMMAND);
      EnableMenuItems(hmenu, ZoAcMe2, MF_ENABLED | MF_BYPOSITION);
      DrawMenuBar(pbrushWnd[PARENTid]);

      EnableWindow(pbrushWnd[TOOLid], TRUE);

/* FALL THROUGH */
   case WM_HIDECURSOR:
      HideCsr(NULL, hWnd, ZOOMINcsr);
      SetCursorOn();
      break;

         /* initiate zoom in */
   case WM_LBUTTONUP:
      EnableWindow(pbrushWnd[TOOLid], TRUE);
      if(!(paintDC = GetDisplayDC(hWnd)))
         goto Error1;
      HideCsr(paintDC, hWnd, ZOOMINcsr);
      ReleaseDC(hWnd, paintDC);
Error1:
      if(CursorStat.captured) {
         ReleaseCapture();
         CursorStat.captured = FALSE;
      }
      inMagnify = TRUE;
      CalcWnds(NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW);
      zoomView.left = newPt.x;
      zoomView.right = zoomView.left + zoomWid;
      zoomView.top = newPt.y;
      zoomView.bottom = zoomView.top + zoomHgt;
      SetCursorOn();
      if(theTool == BRUSHtool)
         PbSetCursor(IDC_ARROW);
      else {
/* theTool == ROLLERtool*/
         DrawCursor = szPbCursor(theTool);
         cursTool = theTool;
         PbSetCursor(DrawCursor = szPbCursor(theTool));
      }
      i = PAINTid;
      MoveWindow(pbrushWnd[i],pbrushRct[i].left, pbrushRct[i].top,
            pbrushRct[i].right - pbrushRct[i].left,
            pbrushRct[i].bottom - pbrushRct[i].top, TRUE);
      SendMessage(hWnd, WM_SIZE, 0, MAKELONG(paintWid, paintHgt));
      InvalidateRect(hWnd, NULL, FALSE);
      UpdateWindow(hWnd);
#ifdef JAPAN //KKBUGFIX added by Hiraisi 11 Nov. 1992 (BUG#457/WIN31 in Japan)
       // The mouse window sometimes overlaps on the paint window.
      if( mouseFlag )
          SendMessage(mouseWnd, WM_MOUSEWINDOW, 0, 0L);
#endif
      break;

   case WM_SHOWCURSOR:
      csrPt.x = csrPt.y = -1;
      break;

            /* position cursor and possibly set magnified pixel */
   case WM_MOUSEMOVE:
      if(!(paintDC = GetDisplayDC(hWnd)))
         goto Error2;
      if((-1 == csrPt.x) && (-1 == csrPt.y))
         SetCursorOff();
      else
         XorCsr(paintDC, csrPt, ZOOMINcsr);
      csrPt = newPt;
      csrPt.x = min(csrPt.x, paintWid - zoomWid);
      csrPt.y = min(csrPt.y, paintHgt - zoomHgt);
      XorCsr(paintDC, csrPt, ZOOMINcsr);
      ReleaseDC(hWnd, paintDC);
Error2:
      break;
   }
}

#define oldPt (*((LPPOINT)lprBounds  ))
#define newPt (*((LPPOINT)lprBounds+1))

static int dir1;

LONG APIENTRY DrawZoomedIn(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int dx, dy;
   static int cntr = 0;

   if(01&cntr++)
      return(FALSE);

   if(wParam & MK_SHIFT) {
      if(!dir1) {
         dx = abs(newPt.x - oldPt.x);
         dy = abs(newPt.y - oldPt.y);

         if(dx > dy)
            dir1 = HORIZdir;
         else if(dy > dx)
            dir1 = VERTdir;
      }
      if(dir1 == HORIZdir)
         newPt.y = oldPt.y;
      else if(dir1 == VERTdir)
         newPt.x = oldPt.x;
   }

   PatBlt(hdcWork, imageView.left + zoomView.left + (newPt.x / zoomAmount),
                   imageView.top + zoomView.top + (newPt.y / zoomAmount),
                   1, 1, PATCOPY);

   BitBlt(dstDC, viewOrg.x + (newPt.x / zoomAmount),
                 viewOrg.y + (newPt.y / zoomAmount), 1, 1,
          hdcWork, imageView.left + zoomView.left + (newPt.x / zoomAmount),
                   imageView.top + zoomView.top + (newPt.y / zoomAmount),
          SRCCOPY);

   if(newPt.x>=viewExt.x || newPt.y>=viewExt.y) {
          DWORD pixel = GetPixel(hdcWork,
                   imageView.left + zoomView.left + (newPt.x / zoomAmount),
                   imageView.top + zoomView.top + (newPt.y / zoomAmount));
//          wsprintf(acDbgBfr,"pixel = %lx, ivtop = %d, zvtop = %d\n",
//                  pixel,imageView.top,zoomView.top);
//          OutputDebugString(acDbgBfr);

          StretchBlt(dstDC, newPt.x - newPt.x % zoomAmount,
                newPt.y - newPt.y % zoomAmount, zoomAmount-1, zoomAmount-1,
                hdcWork, imageView.left + zoomView.left + (newPt.x / zoomAmount),
                         imageView.top + zoomView.top + (newPt.y / zoomAmount), 1, 1, SRCCOPY);
//          StretchBlt(dstDC, newPt.x - newPt.x % zoomAmount,
//                newPt.y - newPt.y % zoomAmount, zoomAmount-1, zoomAmount-1,
//                dstDC, viewOrg.x + (newPt.x / zoomAmount),
//                       viewOrg.y + (newPt.y / zoomAmount), 1, 1, SRCCOPY);

   }

   if(mouseFlag)
      SendMessage(mouseWnd,WM_MOUSEPOS,0,
            MAKELONG(zoomView.left + newPt.x / zoomAmount,
            zoomView.top + newPt.y / zoomAmount));

   return(TRUE);
}


void ZoomedInDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   int oldForeg, reason;
   HDC paintDC;
   HBITMAP brushBM;
   HBRUSH brush, oldBrush;
   RECT rcReturn, rcTemp;
   POINT thePt;

   LONG2POINT(lParam,thePt);
   switch (code) {
   case WM_HIDECURSOR:
      break;

/* set magnified pixel to foreground or background color */
   case WM_RBUTTONDOWN:
      oldForeg = theForeg;
      theForeg = theBackg;
   case WM_LBUTTONDOWN:
      updateFlag = TRUE;

      //  added by Hiraisi (BUG#2867/WIN31)
      if( !(code == WM_RBUTTONDOWN && theTool == ROLLERtool) )
          imageFlag = TRUE;

      if(bExchanged)
        PasteDownRect(rDirty.left, rDirty.top,
               rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

      if(theTool == ROLLERtool) {
         if(code == WM_LBUTTONDOWN) {
            RollerDP(hWnd, WM_LBUTTONDOWN, 0, lParam);
            InvalidateRect(hWnd, NULL, FALSE);
         }
      } else {
         dir1 = 0;
         rcReturn.left = rcReturn.right = thePt.x;
         rcReturn.top = rcReturn.bottom = thePt.y;

         if(!(paintDC = GetDisplayDC(hWnd)))
            goto Error1;
/* These next few lines create a pattern brush for hdcWork */
         if(!(brushBM = CreatePatternBM(hdcWork, rgbColor[theForeg])))
            goto Error2;
         if(!(brush = CreatePatternBrush(brushBM)))
            goto Error3;

         oldBrush = SelectObject(hdcWork, brush);

         reason = TrackTool(hWnd, DrawZoomedIn, &rcReturn, &wParam, paintDC);

//           CopyToWork(0, 0, 0, 0);

         rcTemp.left = imageView.left + zoomView.left;
         rcTemp.top = imageView.left + zoomView.top;
         rcTemp.right = rcTemp.left + zoomWid;
         rcTemp.bottom = rcTemp.top + zoomHgt;
         UnionWithRect(&rDirty,&rcTemp);

         if (fOLE)
            SendDocChangeMsg(vpdoc, OLE_CHANGED);
         if (oldBrush)
            SelectObject(hdcWork, oldBrush);
         DeleteObject(brush);
Error3:
         DeleteObject(brushBM);
Error2:
         ReleaseDC(hWnd, paintDC);
Error1:
         ;
      }

      if(code == WM_RBUTTONDOWN)
         theForeg = oldForeg;
      break;

            /* magnify portion of screen */
   case WM_SCROLLDONE:
      InvalidateRect(hWnd,NULL,FALSE);
      UpdateWindow(hWnd);
      break;

            /* scroll view only */
   case WM_SCROLLVIEW:
      if(!(paintDC = GetDisplayDC(hWnd)))
         return;
      BitBlt(paintDC, viewOrg.x, viewOrg.y, zoomWid, zoomHgt,
            hdcWork, imageView.left + zoomView.left,
                     imageView.top + zoomView.top, SRCCOPY);
      ReleaseDC(hWnd,paintDC);
      break;

   case WM_SHOWCURSOR:
      csrPt.x = csrPt.y = -1;
      break;

   case WM_ZOOMACCEPT:
      PasteDownRect(0, 0, 0, 0);
      goto ZoomOut;

   case WM_ZOOMUNDO:
      UndoRect(0, 0, 0, 0);
ZoomOut:
      ZoomInDP(hWnd, code, wParam, lParam);
      break;

   default:
      break;
   }
}
