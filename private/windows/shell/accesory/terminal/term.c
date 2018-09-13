/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"


/*---------------------------------------------------------------------------*/

BOOL  insertionPoint;
LONG  anchorNdx, lastNdx;


/*---------------------------------------------------------------------------*/
/* pointToLong() - Convert coordinates (pt.x, pt.y) relative to the          */
/*                 current display to an absolute buffer index.              */
/*---------------------------------------------------------------------------*/

VOID pointToLong(POINT	pt, LONG *l)
{
   INT row, col;

   if((col = ((pt.x + (chrWidth / 2) - 1) / chrWidth) + curLeftCol) < 0)
      col = 0;
   else if(col > maxChars)
      col = maxChars;

   if((row = (pt.y / chrHeight) + curTopLine) < 0)
      row = 0;
   else if(row > maxLines)
   {
      row = maxLines;
      col = 0;
   }

   *l = (row * (maxChars + 2)) + col;
}


/*---------------------------------------------------------------------------*/
/* rcToPoint () - Convert absolute rc.row, rc.col to coordinates (GDI)       */
/*                relative to the currently displayed portion of the buffer. */
/*---------------------------------------------------------------------------*/

VOID rcToPoint (ROWCOL rc, POINT *pt, INT bottom)
{
   pt->y = hTE.viewRect.top  + (rc.row - curTopLine) * chrHeight + bottom;
   pt->x = hTE.viewRect.left + (rc.col - curLeftCol) * chrWidth;
}   


/*---------------------------------------------------------------------------*/
/* rcToPointS () - Convert absolute rc.row, rc.col to coordinates (GDI)       */
/*                relative to the currently displayed portion of the buffer. */
/*---------------------------------------------------------------------------*/

#ifdef NOMORE
VOID rcToPointS (ROWCOL rc, POINT *pt, INT bottom)
{
   pt->y = hTE.viewRect.top  + (rc.row - curTopLine) * chrHeight + bottom;
   pt->x = hTE.viewRect.left + (rc.col - curLeftCol) * chrWidth;
}   
#endif


/*---------------------------------------------------------------------------*/
/* longToPoint () - Convert selection to coordinates relative                */
/*                  to the currently displayed portion of the buffer.        */
/*---------------------------------------------------------------------------*/

VOID longToPoint(LONG sel, POINT *pt)
{
   ROWCOL   rc;

   rc.row = sel / (maxChars + 2);
   if((rc.col = sel % (maxChars + 2)) > maxChars)
      rc.col = maxChars;
   rcToPoint(rc, pt, 0);
}


/*---------------------------------------------------------------------------*/
/* hiliteSelect() -                                                          */
/*---------------------------------------------------------------------------*/

VOID NEAR invertSelectRect(NPRECT   pRect)
{
   INT   viewRectEnd, offset;

   viewRectEnd = hTE.viewRect.bottom;
   if((offset = (curTopLine + visScreenLine) - (savTopLine + maxScreenLine)) > 0)
      viewRectEnd -= (offset * chrHeight);

   if(pRect->top > viewRectEnd)
      pRect->top = viewRectEnd;
   if(pRect->bottom > viewRectEnd)
      pRect->bottom = viewRectEnd;

   InvertRect(thePort, (LPRECT) pRect);
}


VOID hiliteSelect(LONG  lSelStart, LONG  lSelEnd)
{
   ROWCOL   selStart, selEnd;
   RECT     hiliteRect;

   getPort();

   selStart.row = lSelStart / (maxChars + 2);
   selStart.col = lSelStart % (maxChars + 2);
   selEnd.row   = lSelEnd   / (maxChars + 2);
   selEnd.col   = lSelEnd   % (maxChars + 2);

   if(lSelEnd > lSelStart)
   {
      if(selStart.row == selEnd.row)
      {
         rcToPoint(selStart, (PPOINT) &hiliteRect.left, 0);
         rcToPoint(selEnd, (PPOINT) &hiliteRect.right, chrHeight);
         invertSelectRect(&hiliteRect);
      }
      else
      {
         rcToPoint(selStart, (PPOINT) &hiliteRect.left, 0);
         selStart.col = maxChars;
         rcToPoint(selStart, (PPOINT) &hiliteRect.right, chrHeight);
         invertSelectRect(&hiliteRect);
         selStart.row += 1;
         selStart.col = 0;

         if(selEnd.row > selStart.row)
         {
            rcToPoint(selStart, (PPOINT) &hiliteRect.left, 0);
            selStart.row = selEnd.row - 1;
            selStart.col = maxChars;
            rcToPoint(selStart, (PPOINT) &hiliteRect.right, chrHeight);
            invertSelectRect(&hiliteRect);
            selStart.col = 0;
         }

         if(selStart.col != selEnd.col)
         {
            selStart.row = selEnd.row;
            rcToPoint(selStart, (PPOINT) &hiliteRect.left, 0);
            rcToPoint(selEnd, (PPOINT) &hiliteRect.right, chrHeight);
            invertSelectRect(&hiliteRect);
         }
      }
   }
   else if((lSelEnd == lSelStart) && insertionPoint)
   {
      rcToPoint(selStart, (PPOINT) &hiliteRect.left, 0);
      rcToPoint(selEnd, (PPOINT) &hiliteRect.right, chrHeight);
      hiliteRect.right += GetSystemMetrics(SM_CXBORDER);
      invertSelectRect(&hiliteRect);
   }

   releasePort();
}


/*---------------------------------------------------------------------------*/
/* termActivate() -                                                          */
/*---------------------------------------------------------------------------*/

VOID termActivate (tEHandle *hTE)
{
   if (!hTE->active)
   {
      hiliteSelect (hTE->selStart, hTE->selEnd);
      hTE->active = TRUE;
   }
}


/*---------------------------------------------------------------------------*/
/* termDeactivate() -                                                        */
/*---------------------------------------------------------------------------*/

VOID termDeactivate (tEHandle *hTE)
{
   if (hTE->active)
   {
      hiliteSelect (hTE->selStart, hTE->selEnd);
      hTE->active = FALSE;
   }
}


/*---------------------------------------------------------------------------*/
/* termSetSelect() -                                                         */
/*---------------------------------------------------------------------------*/

VOID termSetSelect (LONG   selStart, LONG   selEnd)
{
   LONG  eob;

   eob = maxLines * (maxChars + 2);
   if (selStart > eob)
      selStart = eob;
   if (selEnd > eob)
      selEnd = eob;
   if (hTE.active)
   {
      hiliteSelect (hTE.selStart, hTE.selEnd);
      hiliteSelect (selStart, selEnd);
   }
   hTE.selStart = selStart;
   hTE.selEnd   = selEnd;
}


/*---------------------------------------------------------------------------*/
/* extendSelect () -                                                         */
/*---------------------------------------------------------------------------*/

VOID extendSelect (LONG anchorRc, LONG lastRc)
{
   if (lastRc < hTE.selStart)
   {
      if (anchorRc == hTE.selStart)
         termSetSelect (lastRc, anchorRc);
      else
      {
         hiliteSelect (lastRc, hTE.selStart);
         hTE.selStart = lastRc; 
      }
   }
   else if (lastRc > hTE.selEnd)
   {
      if (anchorRc == hTE.selEnd)
         termSetSelect (anchorRc, lastRc);
      else
      {
         hiliteSelect (hTE.selEnd, lastRc);
         hTE.selEnd = lastRc;
      }
   }
   else
   {
      if (anchorRc == hTE.selStart)
      {
         hiliteSelect (lastRc, hTE.selEnd);
         hTE.selEnd = lastRc;
      }
      else
      {
         hiliteSelect (hTE.selStart, lastRc);
         hTE.selStart = lastRc;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* keyBoardToMouse() -                                                       */
/*---------------------------------------------------------------------------*/
VOID keyBoardToMouse(INT partCode)               /* mbbx 2.01.185 ... */
{
   DEBOUT("keyBoardToMouse(): %s\n","TAKE CARE OF LastPoint MPOIN/POINT");

   switch(partCode)
   {
   case VK_LEFT:
      if((lastPoint.x > 0) || (curLeftCol > 0))
      {
         lastPoint.x -= chrWidth;
         break;
      }
                                             /* else fall thru... */
   case VK_UP:
      if((lastPoint.y > 0) || (curTopLine > 0))
      {
         lastPoint.y -= chrHeight;
         if(partCode == VK_LEFT)
            lastPoint.x = (maxChars * chrWidth);
      }
      else
         MessageBeep(0);
      break;

   case VK_RIGHT:
   case VK_DOWN:
      if(lastPoint.y <= (((savTopLine + maxScreenLine) - curTopLine) * chrHeight))
      {
         if((partCode == VK_RIGHT) && 
            (lastPoint.x < ((maxChars - curLeftCol) * chrWidth)))
         {
            lastPoint.x += chrWidth;
         }
         else if(((lastPoint.y += chrHeight) > (((savTopLine + maxScreenLine) - 
            curTopLine) * chrHeight)) || (partCode == VK_RIGHT))
         {
            lastPoint.x = -((curLeftCol + 1) * chrWidth);
         }
      }
      else
         MessageBeep(0);
      break;
   }
}


/*---------------------------------------------------------------------------*/
/* termClick() - Handles mouse & keyboard for selection ranges.              */
/*---------------------------------------------------------------------------*/

VOID hideInsertionPoint()
{
   if(hTE.selStart == hTE.selEnd)
      hiliteSelect(hTE.selStart, hTE.selEnd);
   insertionPoint = FALSE;
}


/*---------------------------------------------------------------------------*/

BOOL stillDown(BOOL keyboard)
{
   MSG  msg;
   BOOL result;

   result = TRUE;

   if (!keyboard)
   {
      if (PeekMessage((LPMSG) &msg, hTermWnd, WM_MOUSEFIRST, WM_MOUSELAST, TRUE))
      {
         switch (msg.message)
         {
            case WM_LBUTTONUP:
               result = FALSE;
               break;
            case WM_MOUSEMOVE:
//               lastPoint = MAKEMPOINT(msg.lParam);
               lastPoint.x = (LONG)LOWORD(msg.lParam);
               lastPoint.y = (LONG)HIWORD(msg.lParam);
               break;
         }
      }
   }
   else 
      result = FALSE;

   return result;
}


/*---------------------------------------------------------------------------*/

VOID showInsertionPoint()
{
   insertionPoint = TRUE;
   if(hTE.selStart == hTE.selEnd)
      hiliteSelect(hTE.selStart, hTE.selEnd);
}


/*---------------------------------------------------------------------------*/

VOID termClick(POINT anchorPt, BOOL extend, INT partCode)
{
   BOOL  keyboard;

   if (scrapSeq || ((xferFlag != XFRNONE) && (xferFlag != XFRRCV)) ) 
         return; 

   hideInsertionPoint();

   if(keyboard = (partCode != 0))
   {
      if((hTE.selStart == hTE.selEnd) && !extend)
      {
         if(anchorPt.y < 0)
            anchorPt.y = 0;
         else if(anchorPt.y > ((visScreenLine + 1) * chrHeight))
            anchorPt.y = ((visScreenLine + 1) * chrHeight);

         lastPoint = anchorPt;
         pointToLong(anchorPt, &anchorNdx);
         termSetSelect(anchorNdx, anchorNdx);
      }
      else
      {
         longToPoint(lastNdx, &lastPoint);
         if(lastNdx == hTE.selStart)
            anchorNdx = hTE.selEnd;
         else
            anchorNdx = hTE.selStart;
      }

      keyBoardToMouse(partCode);
   }
   else
   {
      SetCapture(hTermWnd);

      lastPoint = anchorPt;
      pointToLong(anchorPt, &anchorNdx);

      if(extend)
      {
         if(anchorNdx >= hTE.selStart)
            anchorNdx = hTE.selStart;
         else
            anchorNdx = hTE.selEnd;
      }
      else
         termSetSelect(anchorNdx, anchorNdx);
   }

   repeat
   {
      pointToLong(lastPoint, &lastNdx);
      pasClikLoop();

      if(keyboard && !extend)
      {
         anchorNdx = lastNdx;
         termSetSelect(lastNdx, lastNdx);
      }
      else
         extendSelect(anchorNdx, lastNdx);
   }
   until(!stillDown(keyboard));

   if(!keyboard)
      ReleaseCapture();

   showInsertionPoint();
}


/*---------------------------------------------------------------------------*/
/* getPort() -                                                               */
/*---------------------------------------------------------------------------*/

HDC getPort ()
{
   if (thePort == 0)
   { 
      thePort = GetDC (hTermWnd);
      SelectObject (thePort, hTE.hFont);
      portLocks = 1;
   }
   else
      portLocks++;
   return thePort;
}


/*---------------------------------------------------------------------------*/
/* releasePort() -                                                           */
/*---------------------------------------------------------------------------*/

VOID releasePort ()
{
   if (--portLocks <= 0)
   {
      ReleaseDC (hTermWnd,thePort);
      thePort = 0;
   }
}


/*---------------------------------------------------------------------------*/
/* pasClickLoop() -                                                          */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY pasClikLoop()                /* mbbx 2.01.185 ... */
{
   INT   ndx;
   POINT PointL;

   if (lastPoint.y < hTE.viewRect.top - chrHeight) 
         lastPoint.y = hTE.viewRect.top - chrHeight; /* jtf 3.21 */
   if (lastPoint.y > hTE.viewRect.bottom) 
         lastPoint.y = hTE.viewRect.bottom; /* jtf 3.21 */

/* changed &hTE.viewRect  to &(hTE.viewRect) to see if error goes -sdj */
/* Actual error was due to lastPoint being MPOINT ie POINTS, changed type -sdj */
/* changed lastPoint back to MPOINT and taking care of conversion here -sdj*/

   if(!PtInRect((LPRECT) &(hTE.viewRect), lastPoint))
   {
      for(ndx = lastPoint.y; ndx < hTE.viewRect.top; ndx += chrHeight)
         trackScroll(SB_VERT, SB_LINEUP);
      while(ndx >= hTE.viewRect.bottom)
      {
         trackScroll(SB_VERT, SB_LINEDOWN);
         ndx -= chrHeight;
      }

      for(ndx = lastPoint.x; ndx < hTE.viewRect.left; ndx += chrWidth)
         trackScroll(SB_HORZ, SB_LINEUP);
      while(ndx >= hTE.viewRect.right)
      {
         trackScroll(SB_HORZ, SB_LINEDOWN);
         ndx -= chrWidth;
      }

      scrollBits();
   }

   return(TRUE);
}
