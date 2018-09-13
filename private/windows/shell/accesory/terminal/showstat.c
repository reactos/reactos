/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "video.h"


/*---------------------------------------------------------------------------*/
/* showXferCtrls() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID setXferCtrlButton(WORD wCtrlID, WORD wResID)      /* mbbx 2.00: xfer ctrls... */
//WORD  wCtrlID;
//WORD  wResID;
{
   BYTE  work[MINRESSTR];

   LoadString(hInst, wResID, (LPSTR) work, MINRESSTR);
   SetWindowText(GetDlgItem(hdbXferCtrls, wCtrlID), (LPSTR) work);
}


INT NEAR placeXferCtrl(HWND  hCtrl, INT   fCtrlLeft)
{
   RECT  fCtrlRect;

   GetWindowRect(hCtrl, (LPRECT) &fCtrlRect);
   fCtrlRect.right -= fCtrlRect.left;
   MoveWindow(hCtrl, fCtrlLeft, 0, fCtrlRect.right, fCtrlRect.bottom - fCtrlRect.top, FALSE);

   return(fCtrlLeft + fCtrlRect.right);
}


VOID showXferCtrls(WORD fShowCtrls)               /* mbbx 2.00: xfer ctrls... */
//WORD  fShowCtrls;
{
   INT	 fCtrlLeft;

      //sdj: the status line does not get updates when you come back
      //sdj: to terminal focus.

      if (hdbmyControls != NULL)
	{
	 if(IsWindowVisible(hdbmyControls))
	    {
            InvalidateRect(hdbmyControls, NULL, FALSE);
            UpdateWindow(hdbmyControls);
            }
	}


   if(fShowCtrls)
   {
      fCtrlLeft = placeXferCtrl(xferCtlStop, 0);

      if(fShowCtrls & IDPAUSE)
      {
         fCtrlLeft = placeXferCtrl(xferCtlPause, fCtrlLeft);
      }

      if(fShowCtrls & IDFORK)
      {
         fCtrlLeft = placeXferCtrl(GetDlgItem(hdbXferCtrls, IDFORK), fCtrlLeft);
      }

      fCtrlLeft = placeXferCtrl(xferCtlScale, fCtrlLeft);

      fCtrlLeft = placeXferCtrl(GetDlgItem(hdbXferCtrls, IDSENDING), fCtrlLeft);

      if(fShowCtrls & IDBERRORS)
      {
         fCtrlLeft = placeXferCtrl(GetDlgItem(hdbXferCtrls, IDBERRORS), fCtrlLeft);
      }

      ShowWindow(hdbXferCtrls, SW_SHOWNOACTIVATE);

      if(fShowCtrls & IDPAUSE)
      {
         ShowWindow(xferCtlPause, SW_SHOWNOACTIVATE);
      }

      if(fShowCtrls & IDFORK)
      {
#ifdef ORGCODE
         bSetUp(SPACE_STR+1);
#else
         bSetup(SPACE_STR+1);
#endif
      }

      if(fShowCtrls & IDSCALE)
         showScale();
      else
         showBBytes(0L, TRUE);

      if(fShowCtrls & IDBERRORS)
      {
         showBErrors(0);
      }

   }
   else
   {
      ShowWindow(hdbXferCtrls, SW_HIDE);

      ShowWindow(xferCtlPause, SW_HIDE);
      ShowWindow(GetDlgItem(hdbXferCtrls, IDFORK), SW_HIDE);
      ShowWindow(GetDlgItem(hdbXferCtrls, IDBERRORS), SW_HIDE);
   }
}


/*---------------------------------------------------------------------------*/
/* setItemText() - Set the text of a dialog item                             */
/*---------------------------------------------------------------------------*/

HDC NEAR beginXferCtrlUpdate(HWND  hCtrl, RECT  *ctrlRect, BOOL  bRepaint)
{
   HDC      hDC;
   HBRUSH   hBrush;

   hDC = GetDC(hCtrl);
   GetClientRect(hCtrl, (LPRECT) ctrlRect);

   if(bRepaint)
      FillRect(hDC, (LPRECT) ctrlRect, (HBRUSH) GetStockObject(GRAY_BRUSH));

   InflateRect((LPRECT) ctrlRect, -1, -1);

   if(bRepaint)
   {
      hBrush = CreateSolidBrush(RGB(vidAttr[ANORMAL & AMASK].bkgd[VID_RED], 
                                    vidAttr[ANORMAL & AMASK].bkgd[VID_GREEN], 
                                    vidAttr[ANORMAL & AMASK].bkgd[VID_BLUE]));
      FillRect(hDC, (LPRECT) ctrlRect, hBrush);
      DeleteObject(hBrush);

      FrameRect(hDC, (LPRECT) ctrlRect, (HBRUSH) GetStockObject(BLACK_BRUSH));
   }

   return(hDC);
}


VOID NEAR setItemText(INT   item, BYTE  *str, BOOL  bRepaint)
{
   HWND     hItem;
   HDC      hDC;
   RECT     rect;

   hItem = GetDlgItem(hdbXferCtrls, item);

   if(bRepaint)
   {
      UpdateWindow(hdbXferCtrls);
      if(!IsWindowVisible(hItem))
         ShowWindow(hItem, SW_SHOWNOACTIVATE);
   }

   hDC = beginXferCtrlUpdate(hItem, &rect, bRepaint);
   InflateRect((LPRECT) &rect, -4, -1);
   SetBkColor(hDC, RGB(vidAttr[ANORMAL & AMASK].bkgd[VID_RED], 
                       vidAttr[ANORMAL & AMASK].bkgd[VID_GREEN], 
                       vidAttr[ANORMAL & AMASK].bkgd[VID_BLUE]));
   SetTextColor(hDC, RGB(vidAttr[ANORMAL & AMASK].text[VID_RED], 
                         vidAttr[ANORMAL & AMASK].text[VID_GREEN], 
                         vidAttr[ANORMAL & AMASK].text[VID_BLUE]));
   DrawText(hDC, (LPSTR) str, strlen(str), (LPRECT) &rect, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
   ReleaseDC(hItem, hDC);
}

/*---------------------------------------------------------------------------*/
/* bSetUp() -                                                          [mbb] */
/*---------------------------------------------------------------------------*/

VOID bSetup(BYTE  *str)            /* mbbx 2.00: moved from XFERUTIL.C ... */
{
   setItemText(IDFORK, str, TRUE);
   strcpy(strRXFork, str);
}


/*---------------------------------------------------------------------------*/
/* showScale() - Draw the % done scale for file transfers.        [scf]      */
/*---------------------------------------------------------------------------*/

#define SECTIONS        10
#define TICKWIDTH       2

VOID showScale()                             /* mbbx 2.00: xfer ctrls... */
{
   HDC      hDC;
   RECT     rBase, rect;
   INT      increment, error, tickHeight, ndx, offset;
   HBRUSH   hBrush;

   *strRXBytes = 255;                        /* IDSCALE is scale, not bytes count */

   UpdateWindow(hdbXferCtrls);
   hDC = beginXferCtrlUpdate(xferCtlScale, &rBase, TRUE);   /* mbbx 2.00: xfer ctrls... */

   InflateRect((LPRECT) &rBase, -1, -1);
   increment = rBase.right / SECTIONS;
   error = rBase.right - (increment * SECTIONS);
   tickHeight = rBase.bottom / 2;

   hBrush = CreateSolidBrush(RGB(vidAttr[ANORMAL & AMASK].text[VID_RED], 
                                 vidAttr[ANORMAL & AMASK].text[VID_GREEN], 
                                 vidAttr[ANORMAL & AMASK].text[VID_BLUE]));
   for(ndx = 1; ndx < SECTIONS; ndx += 1)
   {
      offset = (ndx * increment) + ((ndx * error) / SECTIONS);
      SetRect((LPRECT) &rect, (rBase.left + offset) - TICKWIDTH, 
              (ndx != SECTIONS / 2) ? tickHeight : tickHeight - (tickHeight / 2), 
              (rBase.left + offset) + TICKWIDTH, rBase.bottom);
      FillRect(hDC, (LPRECT) &rect, hBrush);
   }
   DeleteObject(hBrush);

   ReleaseDC(xferCtlScale, hDC);
}

/*---------------------------------------------------------------------------*/
/* updateProgress() Update the scale (thermometer) for xfers                 */
/*---------------------------------------------------------------------------*/

VOID updateProgress(BOOL  redraw)
{
   HDC   hDC;
   RECT  ctrlRect;
   INT   left, right;

   if(redraw)
   {
      xferPct = 0;
      icon.last = 0;

      if(IsIconic(hItWnd))
      {
         myDrawIcon(getPort(), TRUE);
         releasePort();
      }
      else
         showScale();
   }

   progress = 256 - (xferBytes * 256) / (xferOrig ? xferOrig : 1);   /* avoid div by zero */
   if(IsIconic(hItWnd))
      updateIcon();
   else
   {   
      if(progress > xferPct)
      {
         hDC = GetDC(xferCtlScale);
         GetClientRect(xferCtlScale, (LPRECT) &ctrlRect);
         InflateRect((LPRECT) &ctrlRect, -2, -2);  /* mbbx 2.00: xfer ctrls... */
         ctrlRect.right -= ctrlRect.left;
         left = (INT) (((LONG) xferPct * (LONG) ctrlRect.right) / 256);
         right = (INT) (((LONG) progress * (LONG) ctrlRect.right) / 256);
         ctrlRect.right = ctrlRect.left + (INT) right;
         ctrlRect.left += (INT) left;
         InvertRect(hDC, (LPRECT) &ctrlRect);
         ReleaseDC(xferCtlScale, hDC);
      }
   }

   if(progress > xferPct)
      xferPct = progress;
}


/*---------------------------------------------------------------------------*/
/* showBBytes() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

VOID showBBytes(LONG  cnt, BOOL  bRepaint)
{
   BYTE tmp1[TMPNSTR+1];
   BYTE tmp2[TMPNSTR+1];

   LoadString(hInst, STR_BYTECOUNT, (LPSTR) tmp1, TMPNSTR);
   sprintf(tmp2, tmp1, cnt);
   setItemText(IDSCALE, tmp2, bRepaint);
   strcpy(strRXBytes, tmp2);
}


/*---------------------------------------------------------------------------*/
/* showRXFname() -                                                           */
/*---------------------------------------------------------------------------*/

VOID showRXFname(BYTE  *fileName, INT   actionString)
{
   BYTE  tmp1[TMPNSTR+1];

   LoadString(hInst, actionString, (LPSTR) tmp1, TMPNSTR);
   strcpy(tmp1+strlen(tmp1), fileName);
   setItemText(IDSENDING, tmp1, TRUE);       /* mbbx 2.00: xfer ctrls */
   strcpy(strRXFname, tmp1);
}


/*---------------------------------------------------------------------------*/
/* showBErrors() -                                               [scf]       */
/*---------------------------------------------------------------------------*/

VOID showBErrors(INT  cnt)
{
   BYTE tmp1[TMPNSTR+1];
   BYTE tmp2[TMPNSTR+1];
   
   LoadString(hInst, STR_RETRIES, (LPSTR) tmp1, TMPNSTR);   /* mbbx 1.00 ... */
   sprintf(tmp2, tmp1, cnt);
   setItemText(IDBERRORS, tmp2, TRUE);       /* mbbx 2.00: xfer ctrls */
   strcpy(strRXErrors, tmp2);
}


/*---------------------------------------------------------------------------*/
/* updateIndicators() -                                                      */
/*---------------------------------------------------------------------------*/

VOID updateIndicators()                      /* mbbx 2.00: moved from DCUTIL2.C ... */
{
   if(xferFlag != XFRNONE)
   {
      UpdateWindow(hdbXferCtrls);            /* mbbx 1.04: fkeys... */

      if(*strRXFork != 0)
         setItemText(IDFORK, strRXFork, TRUE);  /* mbbx 2.00: xfer ctrls... */

      if(*strRXBytes == 255)
         updateProgress(TRUE);
      else if(*strRXBytes != 0)
         setItemText(IDSCALE, strRXBytes, TRUE);

      if(*strRXFname != 0)
         setItemText(IDSENDING, strRXFname, TRUE);

      if(*strRXErrors != 0)
         setItemText(IDBERRORS, strRXErrors, TRUE);
   }
}
