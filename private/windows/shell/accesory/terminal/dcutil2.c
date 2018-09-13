/*===========================================================================*/
/*          Copyright (c) 1985 - 1986, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"


WORD fDisableMoreCtrl;


/*---------------------------------------------------------------------------*/
/* dbmyControls() - Function keys dialog box message processor.        [mbb] */
/*---------------------------------------------------------------------------*/

LONG APIENTRY dbmyControls(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   BOOL  result = FALSE;
   RECT  clipRect;
   WORD  wTemp;

   switch(message)
   {
   case WM_ACTIVATE:
      result = GET_WM_ACTIVATE_STATE(wParam, lParam);
      break;

   case WM_ERASEBKGND:                       /* mbbx 2.00: dlg & ctrl bkgd... */
      GetClipBox((HDC) wParam, (LPRECT) &clipRect);
      FillRect((HDC) wParam, (LPRECT) &clipRect, (HBRUSH) GetStockObject(GRAY_BRUSH));
      return(TRUE);

   case WM_CTLCOLOR:
      return((BOOL) GetStockObject(HOLLOW_BRUSH));

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDSTOP:                           /* mbbx 2.00: xfer ctrls... */
         if(result = updateFKeyButton(wParam, lParam, FKB_UPDATE_BKGD))
            xferStopBreak(!xferBreak);
         break;

      case IDPAUSE:                          /* mbbx 2.00: xfer ctrls... */
         if(result = updateFKeyButton(wParam,lParam, FKB_UPDATE_BKGD))
            xferPauseResume(!xferPaused, xferPaused);
         break;

      case IDFK1:
      case IDFK2:
      case IDFK3:
      case IDFK4:
      case IDFK5:
      case IDFK6:
      case IDFK7:
      case IDFK8:
#ifdef ORGCODE

         if(result = updateFKeyButton(lParam, FKB_UPDATE_BKGD |
                                      ((*trmParams.fKeyText[curLevel-1][wParam-IDFK1] != 0) ? 0 : FKB_DISABLE_CTRL)))
#else

         wTemp = GET_WM_COMMAND_ID(wParam,lParam);
         if(result = updateFKeyButton(wParam, lParam, 
                                      FKB_UPDATE_BKGD |
                                      ((*trmParams.fKeyText[curLevel-1][wTemp-IDFK1] != 0) ? 0 : FKB_DISABLE_CTRL)))
#endif
         {
            selectFKey(wParam);
         }
         break;

      case IDMORE:
      case IDFK9:
#ifdef ORGCODE
         if(result = updateFKeyButton(lParam, FKB_UPDATE_BKGD | fDisableMoreCtrl))  /* mbbx 2.00: fkeys */
#else
         if(result = updateFKeyButton(wParam, lParam, FKB_UPDATE_BKGD | fDisableMoreCtrl))  /* mbbx 2.00: fkeys */
#endif
            setFKeyLevel(-1, FALSE);         /* mbbx 2.00: bReset */
         break;

      case IDTIMER:
      case IDFK10:
#ifdef ORGCODE
         if(result = updateFKeyButton(lParam, FKB_UPDATE_BKGD | FKB_UPDATE_TIMER))  /* mbbx 2.00: fkeys */
#else
         if(result = updateFKeyButton(wParam, lParam, FKB_UPDATE_BKGD | FKB_UPDATE_TIMER))  /* mbbx 2.00: fkeys */
#endif
            timerToggle(FALSE);              /* mbbx 1.03 */
         break;
      }

      if(result && (hDlg == hdbXferCtrls))
         BringWindowToTop(hTermWnd);
      break;
   }

   if(result)
      selectTopWindow();

   return(result);
}


/*---------------------------------------------------------------------------*/
/* updateFKeyButton() -                                               [mbb] */
/*---------------------------------------------------------------------------*/

BOOL fKeyStrBuffer(BYTE *str, WORD len)
{
   if(fKeyNdx > *fKeyStr)
   {
      if(len == 1)
         return(sendKeyInput(*str));

      memcpy(fKeyStr+1, str, *fKeyStr = len);
      fKeyNdx = 1;
      return(TRUE);
   }

   if(fKeyNdx > 1)
   {
      memcpy(fKeyStr+1, fKeyStr+fKeyNdx, (*fKeyStr+1)-fKeyNdx);
      *fKeyStr -= (fKeyNdx-1);
      fKeyNdx = 1;
   }

   if(*fKeyStr+len >= STR255-1)
      return(FALSE);

   memcpy(fKeyStr+(*fKeyStr+1), str, len);
   *fKeyStr += len;
   return(TRUE);
}


INT   selectFKey(WORD wIDFKey)
//WORD  wIDFKey;
{
   fKeyStrBuffer(trmParams.fKeyText[curLevel-1][wIDFKey-IDFK1], 
                 strlen(trmParams.fKeyText[curLevel-1][wIDFKey-IDFK1]));
}


/*---------------------------------------------------------------------------*/
/* setFKeyTitles() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID setFKeyTitles()
{
   INT   ndx;
   BYTE  str[STR255];
   CHAR  szBuffer[16];
   DEBOUT("setKKeyTitles: curLevel = %d\n",curLevel);
   DEBOUT("setKKeyTitles:%s\n","SetWindowText BUG? check this out, HACK return");

   for(ndx = 0; ndx < DCS_NUMFKEYS; ndx += 1)
   {
      DEBOUT("setFKeyTitles: fKH[ndx]=%lx\n",fKeyHandles[ndx]);
      DEBOUT("setFKeyTitles: fKT[curlevel-1][ndx]=%s\n",(LPSTR) trmParams.fKeyTitle[curLevel-1][ndx]);
      SetWindowText(fKeyHandles[ndx], (LPSTR) trmParams.fKeyTitle[curLevel-1][ndx]);
   }

      if(*trmParams.fKeyNext == 0)
   {
      LoadString(hInst, STR_LEVEL, szBuffer, 15);
      sprintf(str, szBuffer, curLevel);
   }
   else
      strcpy(str, trmParams.fKeyNext);

   SetWindowText(GetDlgItem(hdbmyControls, IDMORE), (LPSTR) str);
}

/*---------------------------------------------------------------------------*/
/* testFkeyLevel() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/
BOOL NEAR testFKeyLevel(INT level)
//INT   level;
{
   INT   ndx;

   for(ndx = 0; ndx < DCS_NUMFKEYS; ndx += 1)
      if((*trmParams.fKeyTitle[level][ndx] != 0) || (*trmParams.fKeyText[level][ndx] != 0))
         return(TRUE);

   return(FALSE);
}



/*---------------------------------------------------------------------------*/
/* nextFkeyLevel() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/
INT NEAR nextFKeyLevel(INT level)
{
   INT   ndx;

   for(ndx = 0; ndx < DCS_FKEYLEVELS; ndx += 1)
   {
      if(level >= DCS_FKEYLEVELS)
         level = 0;
      if(testFKeyLevel(level++))
         return(level);
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* setFKeyLevel() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

VOID setFKeyLevel(INT newLevel, BOOL bReset)
{
   if(newLevel == -1)
      newLevel = nextFKeyLevel(curLevel);

   if((newLevel < 1) || (newLevel > DCS_FKEYLEVELS))
      newLevel = bReset ? 1 : curLevel;

   if(bReset || (newLevel != curLevel))
   {
      fDisableMoreCtrl = ((newLevel = nextFKeyLevel(curLevel = newLevel)) && 
                         (newLevel != curLevel)) ? 0 : FKB_DISABLE_CTRL;
      setFKeyTitles();
   }
}


/*---------------------------------------------------------------------------*/
/* timerAction() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID timerAction(BOOL bTimer, BOOL bReset)
{
   DWORD tickCount;
   
   if(bTimer != timerActiv)
   {
      CheckMenuItem(hMenu, FMTIMER, timerActiv = bTimer ? MF_CHECKED : MF_UNCHECKED);
      PostMessage(hdbmyControls, WM_COMMAND, GET_WM_COMMAND_MPS(IDTIMER, GetDlgItem(hdbmyControls, IDTIMER), BN_PAINT));
   }

   if(bReset)
      readDateTime(startTimer);
}

/*---------------------------------------------------------------------------*/
/* timerToggle() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID timerToggle(BOOL bReset)
{
   timerAction(!timerActiv, bReset);
}

/*---------------------------------------------------------------------------*/
/* sizeFkeys() - Resize fkeys dialog                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID sizeFkeys(LONG clientSize)
{
   RECT  fKeysRect, fCtrlRect;
   GetWindowRect(hdbmyControls, (LPRECT) &fKeysRect);
/* -------------------------------------------------------------- 
if ((HIWORD(clientSize) - fKeysRect.bottom) < 0)   jtf 3.15 
         clientSize = (LOWORD(clientSize),fKeysRect.bottom);
 -------------------------------------------------------------- */

   fKeysRect.top += (HIWORD(clientSize) - fKeysRect.bottom);

   MoveWindow(hdbmyControls, 0, fKeysRect.top, 
              LOWORD(clientSize), HIWORD(clientSize) - fKeysRect.top, TRUE);

   GetWindowRect(fKeyHandles[1], (LPRECT) &fKeysRect);
   GetWindowRect(fKeyHandles[2], (LPRECT) &fCtrlRect);
   fKeysRect.left = fCtrlRect.left - fKeysRect.right;
   fKeysRect.top -= fCtrlRect.bottom;

   fCtrlRect.left = 0;
   fCtrlRect.bottom -= fCtrlRect.top;
   fCtrlRect.right = (LOWORD(clientSize) / ((DCS_NUMFKEYS/2)+1));
   for(fCtrlRect.top = 0; fCtrlRect.top < DCS_NUMFKEYS; fCtrlRect.top += 2)
   {
      MoveWindow(fKeyHandles[fCtrlRect.top], fCtrlRect.left, 0, 
                 fCtrlRect.right, fCtrlRect.bottom, TRUE);
      MoveWindow(fKeyHandles[fCtrlRect.top+1], fCtrlRect.left, fCtrlRect.bottom + fKeysRect.top, 
                 fCtrlRect.right, fCtrlRect.bottom, TRUE);
      fCtrlRect.left += (fCtrlRect.right + fKeysRect.left);
   }

   fCtrlRect.right = LOWORD(clientSize) - fCtrlRect.left;
   MoveWindow(GetDlgItem(hdbmyControls, IDMORE), fCtrlRect.left, 0, 
              fCtrlRect.right, fCtrlRect.bottom, TRUE);
   MoveWindow(GetDlgItem(hdbmyControls, IDTIMER), fCtrlRect.left, fCtrlRect.bottom + fKeysRect.top, 
              fCtrlRect.right, fCtrlRect.bottom, TRUE);
}

/*---------------------------------------------------------------------------*/
/* initChildSize() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID initChildSize(RECT *pRect)
{
   if(IsIconic(hItWnd))
   {
      SetRectEmpty((LPRECT) pRect);
      pRect->right  = GetSystemMetrics(SM_CXSCREEN);
      pRect->bottom = GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYCAPTION) - 
                      GetSystemMetrics(SM_CYMENU);
   }
   else
   {
      GetClientRect(hItWnd, (LPRECT) pRect);
      if(IsWindowVisible(hdbmyControls))     /* mbbx 1.04: fkeys... */
         pRect->bottom -= fKeysHeight;
   }
}


/*---------------------------------------------------------------------------*/
/* childZoomSize() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

BOOL bZoomFlag = FALSE;                      /* prevents recursive calls */


/*---------------------------------------------------------------------------*/
/* childWindowZoom() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

#define ZC_NOTZOOMED                0x0000
#define ZC_ZOOMED                   0x0001
#define ZC_ZOOMNEXT                 0x4000
#define ZC_NORESTORE                0x8000

WORD fZoomChild = ZC_NOTZOOMED;

WORD childZoomStatus(WORD wTest, WORD wSet)
{
   WORD childZoomStatus = (fZoomChild & wTest);

   fZoomChild |= wSet;

   return(childZoomStatus);
}


VOID setAppTitle()
{
   BYTE  work[STR255];

   strcpy(work, szMessage);

      strcpy(work+strlen(work), " - ");
      GetWindowText(hTermWnd, (LPSTR) work+strlen(work), 80);

   SetWindowText(hItWnd, (LPSTR) work);
}



/*---------------------------------------------------------------------------*/
/* sizeTerm() - Resize terminal window                                 [mbb] */
/*---------------------------------------------------------------------------*/

VOID sizeTerm(LONG termSize)
{
   RECT     termRect, ctrlRect;
   
   GetClientRect(hItWnd, (LPRECT) &termRect);
   if(IsWindowVisible(hdbmyControls))
         termRect.bottom -= fKeysHeight;

   MoveWindow(hTermWnd, 0, 0, termRect.right,termRect.bottom,TRUE); 

   GetClientRect(hTermWnd, (LPRECT) &termRect);

   CopyRect((LPRECT) &statusRect, (LPRECT) &termRect);
   statusRect.top = termRect.bottom - (chrHeight + STATUSRECTBORDER);

   CopyRect((LPRECT) &ctrlRect, (LPRECT) &statusRect);
   if((ctrlRect.bottom - ctrlRect.top) < ctrlsHeight)
      ctrlRect.top = termRect.bottom - ctrlsHeight;
   MoveWindow(hdbXferCtrls, 0, ctrlRect.top, ctrlRect.right - ctrlRect.left, 
              ctrlRect.bottom - ctrlRect.top, FALSE);
   updateIndicators();

   if(chrHeight != 0)
      visScreenLine = (ctrlRect.top / chrHeight) - 1;
   CopyRect((LPRECT) &hTE.viewRect, (LPRECT) &termRect);
   hTE.viewRect.bottom = (visScreenLine + 1) * chrHeight;

   curTopLine = min(curTopLine, savTopLine + maxScreenLine - visScreenLine);

   if (curTopLine < 0) 
      curTopLine = 0;

   if((nScrollRange.x = maxChars - (termRect.right / chrWidth)) < 0)
      nScrollRange.x = 0;
   if(nScrollPos.x > nScrollRange.x)
      nScrollPos.x = nScrollRange.x;

   updateTermScrollBars(TRUE);
   InvalidateRect(hTermWnd, NULL, FALSE); /* rjs swat - was TRUE */
   UpdateWindow(hTermWnd);

}


/*---------------------------------------------------------------------------*/
/* countChildWindows() -                                               [mbb] */
/*---------------------------------------------------------------------------*/

INT countChildWindows(BOOL bUnzoom)
{
   INT   nWndCount = 0;
   HWND  hNextWnd;

   hNextWnd = hdbmyControls;                 /* mbb?: reasonable assumption? */
   while((hNextWnd = GetNextWindow(hNextWnd, GW_HWNDPREV)) != NULL)
      if(IsWindowVisible(hNextWnd))
         nWndCount++;

   if(bUnzoom && IsZoomed(hNextWnd = GetTopWindow(hItWnd)))   /* AFTER count!!! */
      ShowWindow(hNextWnd, SW_RESTORE);

   return(nWndCount);
}

/*---------------------------------------------------------------------------*/
/* showTerminal() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

VOID showTerminal(BOOL bShow, BOOL bReset)
{
   if((bShow != (!(termData.flags & TF_HIDE) ? TRUE : FALSE)) || bReset)
   {
      if(bShow)
      {
         termData.flags &= ~TF_HIDE;

         if(activTerm)
         {
            SetWindowPos(hTermWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
            SetFocus(hTermWnd);
         }
      }
      else
      {
         termData.flags |= TF_HIDE;

         if(activTerm)
         {
            if(childZoomStatus(ZC_ZOOMED, 0) && (countChildWindows(FALSE) == 1))
            {
               childZoomStatus(0, ZC_ZOOMNEXT);
               ShowWindow(hTermWnd, SW_RESTORE);
            }
            /* typecasted the 2nd param to HWND -sdj*/
            SetWindowPos(hTermWnd,(HWND) 1, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW);
            selectTopWindow();                  /* mbbx 1.03 ... */
         }
      }
   }
}


/*---------------------------------------------------------------------------*/
/* showHidedbmyControls() -                                                  */
/*---------------------------------------------------------------------------*/

VOID showHidedbmyControls(BOOL bShow, BOOL bArrange)
{
   BYTE  strShowHide[TMPNSTR+1];

   if(bShow != fKeysShown)
   {
      if(fKeysShown = bShow)
      {
         ShowWindow(hdbmyControls, SW_SHOW);
         LoadString(hInst, STR_HIDEFKEYS, (LPSTR) strShowHide, TMPNSTR);
         ChangeMenu(hMenu, WMFKEYS, (LPSTR) strShowHide, WMFKEYS, MF_CHANGE);
      }
      else
      {
         ShowWindow(hdbmyControls, SW_HIDE);
         LoadString(hInst, STR_SHOWFKEYS, (LPSTR) strShowHide, TMPNSTR);
         ChangeMenu(hMenu, WMFKEYS, (LPSTR) strShowHide, WMFKEYS, MF_CHANGE);
      }
      if(!IsIconic(hItWnd))   /* rjs bugs 015 */
         sizeTerm(0L); /* jtf 3.21 */


      // make sure hTermWnd gets cleaned so there's no
      // garbage left on hTermWnd.
      InvalidateRect (hTermWnd, NULL, TRUE);
      UpdateWindow (hTermWnd);
   }
}



/*---------------------------------------------------------------------------*/
/* makeActiveNext() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

VOID makeActiveNext(BOOL bPrevWnd)
{
   HWND  hNextWnd, hTopWnd;

   if((hNextWnd = GetNextWindow(hdbmyControls, GW_HWNDPREV)) != (hTopWnd = GetTopWindow(hItWnd)))
   {
      if(childZoomStatus(ZC_ZOOMED, 0))
      {
         childZoomStatus(0, ZC_NORESTORE);
         ShowWindow(hTopWnd, SW_RESTORE);
      }

      if(bPrevWnd)
         BringWindowToTop(hNextWnd);
      else
      {
         SetWindowPos(hTopWnd, hNextWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
         hNextWnd = GetTopWindow(hItWnd);
      }

      SetFocus(hNextWnd);
   }
}
