/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"


/*---------------------------------------------------------------------------*/
/* updateTermScrollBars() -                                            [mbb] */
/*---------------------------------------------------------------------------*/

VOID updateTermScrollBars(BOOL  bScroll)
{
   INT   saveScrollRange, offset;

   saveScrollRange = nScrollRange.y;
   if((offset = maxScreenLine - visScreenLine) < 0)
      offset = 0;
   if((nScrollRange.y = savTopLine + offset) < 0)
      nScrollRange.y = 0;

   if(((nScrollPos.y > 0) && (nScrollPos.y == saveScrollRange)) || (nScrollPos.y > nScrollRange.y)) 
      nScrollPos.y = nScrollRange.y;

   if(!trmParams.fHideTermVSB)
   {
      SetScrollRange(hTermWnd, SB_VERT, 0, (nScrollRange.y > 0) ? nScrollRange.y : 1, FALSE);
      if (GetScrollPos(hTermWnd, SB_VERT) != nScrollPos.y)    /* jtf 3.14 */
         SetScrollPos(hTermWnd, SB_VERT, nScrollPos.y, TRUE);
   }
   else
      SetScrollRange(hTermWnd, SB_VERT, 0, 0, TRUE);

   if(!trmParams.fHideTermHSB)
   {
      SetScrollRange(hTermWnd, SB_HORZ, 0, (nScrollRange.x > 0) ? nScrollRange.x : 1, FALSE);
      if (GetScrollPos(hTermWnd, SB_HORZ) != nScrollPos.x)    /* jtf 3.14 */
         SetScrollPos(hTermWnd, SB_HORZ, nScrollPos.x, TRUE);
   }
   else
      SetScrollRange(hTermWnd, SB_HORZ, 0, 0, TRUE);

   if(bScroll)
      scrollBits();
}


/*---------------------------------------------------------------------------*/
/* scrollTermWindow() -                                                [mbb] */
/*---------------------------------------------------------------------------*/

proc scrollTermWindow(INT  dh, INT  dv)
{
   ScrollWindow(hTermWnd, -dh*chrWidth, -dv*chrHeight, (LPRECT) &hTE.viewRect,
                (LPRECT) &hTE.viewRect);
}


/*---------------------------------------------------------------------------*/
/* scrollTermLine() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

proc scrollTermLine(INT  newLine, INT  dh, INT  dv)
{
   RECT  r;

   getPort();
   hTE.active = 0;

   if(dv > 0)
      r.top = hTE.viewRect.bottom - stdChrHeight;
   else
      r.top = 0;
   r.left = hTE.viewRect.left;
   r.bottom = r.top + stdChrHeight;
   r.right = hTE.viewRect.right;

   scrollTermWindow(dh, dv);
   ValidateRect(hTermWnd, (LPRECT) &r);

   reDrawTermLine(newLine, 0, maxChars);
   clipRect(&r);
   termActivate(&hTE);
   releasePort();
}


/*---------------------------------------------------------------------------*/
/* scrollBits() - Move text on term. screen + some IT house keeping.   [mbb] */
/*---------------------------------------------------------------------------*/

VOID scrollBits()
{
   INT   oldTopLine;
   INT   oldLeftCol;
   INT   dh, dv;
   INT   offset;
   INT   lin;

   oldLeftCol = curLeftCol;
   oldTopLine = curTopLine;

   curLeftCol = nScrollPos.x;
   curTopLine = nScrollPos.y;
   dh = curLeftCol - oldLeftCol;
   dv = curTopLine - oldTopLine;

   hideTermCursor();
   if(dh != 0)
   {
      scrollTermWindow(dh, dv);
      UpdateWindow(hTermWnd);
   }
   else if(dv == 1)
      scrollTermLine(curTopLine-savTopLine+visScreenLine, dh, dv);
   else if(dv == -1)
      scrollTermLine(curTopLine-savTopLine, dh, dv);
   else if(dv != 0)
   {
      termDeactivate(&hTE); 
      reDrawTermScreen(0, visScreenLine+1, curTopLine - savTopLine);    /* mbbx 2.00: jtf disp2... */
      termActivate(&hTE); 
   }
   showTermCursor();

   if((curTopLine + visScreenLine) > (savTopLine + maxScreenLine))   /* mbbx 2.00.06: jtf disp2... */
      cleanRect(maxScreenLine+1, (visScreenLine + curTopLine) - (savTopLine + maxScreenLine));
}


/*---------------------------------------------------------------------------*/
/* scrollUp() -                                                              */
/*---------------------------------------------------------------------------*/

VOID scrollUp(INT   nBar, INT   partCode, INT   deltaLines)
{
   LONG   *pScrollPos;

   if(partCode == SB_LINEUP)
   {
      pScrollPos = ((nBar == SB_VERT) ? &nScrollPos.y : &nScrollPos.x);

      if(*pScrollPos > 0)
      {
         *pScrollPos -= deltaLines;
         updateTermScrollBars(TRUE);
      }
   }
}


/*---------------------------------------------------------------------------*/
/* scrollDown() -                                                            */
/*---------------------------------------------------------------------------*/

VOID scrollDown(INT   nBar, INT   partCode, INT   deltaLines)
{
   LONG  *pScrollRange, *pScrollPos;

   if(partCode == SB_LINEDOWN)
   {
      if(nBar == SB_VERT)
      {
         pScrollRange = &nScrollRange.y;
         pScrollPos   = &nScrollPos.y;
      }
      else
      {
         pScrollRange = &nScrollRange.x;
         pScrollPos   = &nScrollPos.x;
      }

      if(*pScrollPos < *pScrollRange)
      {
         *pScrollPos += deltaLines;
         updateTermScrollBars(TRUE);
      }
   }
}


/*---------------------------------------------------------------------------*/
/* pageScroll() -                                                            */
/*---------------------------------------------------------------------------*/

VOID pageScroll(INT   which)
{
   if(which == SB_PAGEUP)
   {
      if((nScrollPos.y -= (visScreenLine + 1)) < 0)
         nScrollPos.y = 0;
   }
   else
   {
      if((nScrollPos.y += (visScreenLine + 1)) > nScrollRange.y)
         nScrollPos.y = nScrollRange.y;
   }

   updateTermScrollBars(TRUE);
   termCleanUp();
}


/*---------------------------------------------------------------------------*/
/* hPageScroll() -                                                           */
/*---------------------------------------------------------------------------*/

VOID hPageScroll(INT   which)
{
   nScrollPos.x = ((which == SB_PAGEUP) ? 0 : nScrollRange.x);
   updateTermScrollBars(TRUE);
   termCleanUp();                            /* mbbx 1.04: per jtfx */
}


/*---------------------------------------------------------------------------*/
/* trackScroll() -                                                     [scf] */
/*---------------------------------------------------------------------------*/

VOID trackScroll(INT   nBar, INT   partCode)
{
   LONG  *pScrollRange, *pScrollPos;
   INT   amount;

   if(nBar == SB_VERT)
   {
      pScrollRange = &nScrollRange.y;
      pScrollPos   = &nScrollPos.y;
   }
   else
   {
      pScrollRange = &nScrollRange.x;
      pScrollPos   = &nScrollPos.x;
   }

   amount = (((partCode == SB_LINEUP) || (partCode == SB_PAGEUP)) ? -1 : 1);

   if(((amount == -1) && (*pScrollPos > 0)) || ((amount == 1) && (*pScrollPos < *pScrollRange)))
   {
      if((partCode == SB_PAGEUP) || (partCode == SB_PAGEDOWN))
      {
         if(nBar == SB_VERT)
         {
            if(amount == -1)
            {
               if((*pScrollPos -= ((hTE.viewRect.bottom - hTE.viewRect.top) / chrHeight)) < 0)
                  *pScrollPos = 0;
            }
            else
            {
               if((*pScrollPos += ((hTE.viewRect.bottom - hTE.viewRect.top) / chrHeight)) > *pScrollRange)
                  *pScrollPos = *pScrollRange;
            }
         }
         else
            *pScrollPos += ((amount == -1) ? 0 : *pScrollRange);
      }
      else
         *pScrollPos += amount;

      updateTermScrollBars(FALSE);
   }
}
