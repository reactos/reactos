/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "video.h"
#include "printfil.h"

extern BOOL bZoomFlag;                       /* itutil2.c - zoomTerm() [mbb] */

VOID NEAR PASCAL FreeList(HANDLE *hList)
{
   GlobalUnlock(*hList);
   GlobalFree(*hList);
}

/*---------------------------------------------------------------------------*/
/* DC_WndProc()                                                        [mbb] */
/*---------------------------------------------------------------------------*/

HWND dlgGetFocus()                           /* mbbx 2.01.85 ... */
{
   HWND  hWnd;

   if(((hWnd = GetActiveWindow()) == NULL) || !IsChild(hItWnd, hWnd))  /* mbbx 2.01.179 (2.01.150) ... */
      hWnd = hItWnd;

   hWnd = GetActiveWindow(); /* jtf gold 044 */
   if (hWnd == NULL) testMsg("BAD WINDOW",NULL,NULL);
   return(hWnd);
}


VOID selectTopWindow()                       /* mbbx 1.03 ... */
{
   HWND  hTopWnd;

   if(((hTopWnd = GetTopWindow(hItWnd)) == NULL) || (hTopWnd == hdbmyControls))
      hTopWnd = hItWnd;
if (hTopWnd != NULL) /* jtfterm */
   SetFocus(hTopWnd);
}



LONG  APIENTRY DC_WndProc(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   LONG           DC_WndProc = 0L;
   // sdj: was unref local - HWND	   hTopWnd;
   PAINTSTRUCT    ps;
   BYTE           work[STR255];              /* jtf 3.15 */
   // sdj: was unref local - RECT	  clipRect;
   // sdj: was unref local - HDC		  tempDC;
   INT		  testFlag;		     /* jtf 3.23 */
   RECT 	  hItWndRectToSave;	     // sdj: added this to save wndsize
   char 	  tmpstr[10];
   BYTE 	  str[MINRESSTR];


   DEBOUT("DC_WndProc: msg[%lx] GOT IT\n",message);
   switch(message)
   {
   case WM_WININICHANGE:  /* jtf 3.21 */
      setDefaultAttrib(TRUE);
      InvalidateRect(hTermWnd, NULL, TRUE);
      InvalidateRect(hdbmyControls, NULL, TRUE);
      UpdateWindow(hTermWnd);
      UpdateWindow(hdbmyControls);
      break;

   /* changed all wParam == hSomething to wParam == (WPARAM) hSomething -sdj*/
   case WM_SETCURSOR: /* jtf 3.20 allow hour glass to stay hour */
      if (scrapSeq && (wParam == (WPARAM)hTermWnd) && (LOWORD(lParam) == HTCLIENT) ) /* jtf 3.30 */
         {
         SetCursor(LoadCursor(NULL, IDC_WAIT));
         return (TRUE);
         }
      if ( ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDMORE) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDTIMER) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK1) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK2) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK3) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK4) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK5) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK6) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK7) ) ||
         ( wParam == (WPARAM)GetDlgItem(hdbmyControls, IDFK8) ) )
            {
            SetCursor(LoadCursor(hInst, (LPSTR) "hand"));
            return(TRUE);
            }
      DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      break;

   case WM_SIZE:
      if(wParam != SIZEICONIC)
         DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      if ((wParam != SIZEICONIC) && (hdbmyControls != NULL) && (!IsIconic(hItWnd)) )/* rjs bugs 015 -> add wParam == SIZEICONIC */
      {
         sizeFkeys(lParam); 
         if ((hTermWnd != NULL) && (!IsIconic(hItWnd)) )
            sizeTerm(0L);
         if(IsWindowVisible(hdbmyControls))
            UpdateWindow(hdbmyControls);
      }
      break;

   case WM_SETFOCUS:
   case WM_KILLFOCUS:                        /* rjs bugs 011 */
      if  (hTermWnd != NULL)     /* jtf 3.30  */
	 UpdateWindow(hTermWnd); /* jtf 3.30 */

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

      if(message == WM_SETFOCUS)             /* rjs bugs 011 */
      {
         showTermCursor();                   /* rjs bugs 011 */
         selectTopWindow();                  /* mbbx 1.03 */
      }
      else                                   /* rjs bugs 011 */
         hideTermCursor();                   /* rjs bugs 011 */
      break;

/* jtfterm   case WM_ERASEBKGND:                     
      GetClipBox((HDC) wParam, (LPRECT) &clipRect);
      eraseColorRect((HDC) wParam, (LPRECT) &clipRect, ANORMAL);
      DC_WndProc = 1L;
      break; */

   case WM_ERASEBKGND:                     /* jtf 3.17 */
      if(IsIconic(hWnd))
         return DefWindowProc(hWnd,WM_ICONERASEBKGND,wParam,lParam);
      else
      {
         DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
         break;
      }
   
   case WM_QUERYDRAGICON: /* jtf 3.17 */
      return (LONG)icon.hIcon;

   case WM_PAINT:
      if(IsIconic(hWnd))
      {
         BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
         myDrawIcon(ps.hdc, !ps.fErase);
         EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
         break;
      }

      DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      break;

   case WM_CLOSE:
   case WM_QUERYENDSESSION:
         testFlag = 0;
	 /* Added 02/22/91 for win  3.1 common print dialog w-dougw */
	 if(hDevNames)
	   FreeList(&hDevNames);
	 if(hDevMode)
	   FreeList(&hDevMode);

         if (mdmOnLine)
         {
            LoadString(hInst, STRERRHANGUP, (LPSTR) work, STR255-1); /* jtf 3.15 */
            testFlag = MessageBox(GetActiveWindow(), (LPSTR) work, (LPSTR) szAppName, MB_YESNOCANCEL);
         }
         if (testFlag==IDCANCEL)
            break;
         if (testFlag==IDYES)
            hangUpPhone();

         WinHelp(hTermWnd,(LPSTR) work, (WORD) HELP_QUIT,0L);
         if (prtFlag)  
	    PrintFileComm(!prtFlag); /* jtf 3.15 */

	 //sdj: lets get the current window x,y,width,height and save it into
	 //sdj: the registry so that next time we load the settings user wants
	 //sdj: only save the values into registry if the api succeeds


	 if(GetWindowRect(hWnd,&hItWndRectToSave))
	    {

	    WindowXPosition = (int)hItWndRectToSave.left;
	    WindowYPosition = (int)hItWndRectToSave.top;
	    WindowWidth     = ((int)hItWndRectToSave.right - WindowXPosition);
	    WindowHeight    = ((int)hItWndRectToSave.bottom - WindowYPosition);

	    sprintf(tmpstr,"%d",WindowXPosition);
	    LoadString(hInst, STR_INI_XPOSITION, (LPSTR) str, MINRESSTR);
	    WriteProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) tmpstr);

	    sprintf(tmpstr,"%d",WindowYPosition);
	    LoadString(hInst, STR_INI_YPOSITION, (LPSTR) str, MINRESSTR);
	    WriteProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) tmpstr);

	    sprintf(tmpstr,"%d",WindowWidth);
	    LoadString(hInst, STR_INI_WIDTH, (LPSTR) str, MINRESSTR);
	    WriteProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) tmpstr);

	    sprintf(tmpstr,"%d",WindowHeight);
	    LoadString(hInst, STR_INI_HEIGHT, (LPSTR) str, MINRESSTR);
	    WriteProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) tmpstr);

	    }



         if(doneFlag = termCloseAll())    /* mbbx 1.01... */
         {
         return(TRUE); /* jtf 3.33 */
/*         DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);  jtf 3.33, 3.28 and 3.26 */
         }
      break;

   case WM_QUERYOPEN:
      flashIcon(FALSE, FALSE);
      DC_WndProc = 1L;
      break;

   case WM_ENDSESSION:
      doneFlag = wParam;           
      break;

   case WM_NCLBUTTONDBLCLK:
      if((wParam == HTCAPTION) && childZoomStatus(0x0001, 0))
      {
         ShowWindow(GetTopWindow(hItWnd), SW_RESTORE);
         break;
      }
      DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      break;

   case WM_KEYDOWN:                       /* mbbx 1.04: keymap ... */
   case WM_KEYUP:
   case WM_SYSKEYDOWN:
   case WM_SYSKEYUP:
      if (wParam == VK_F10)                     /* jtf 3.21 */
      {
         DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
         break;
      }
      termKeyProc(hWnd, message, wParam, lParam);
      break;

   case WM_COMMAND:
      doCommand(hWnd, wParam, lParam);
      break;

   case WM_INITMENUPOPUP:
      initMenuPopup(LOWORD(lParam));
      break;

   default:
      DC_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      // idleProcess();  -sdj CAUSES RECURSION AND GDI/USER DEATH
      break;
   }

DEBOUT("DC_WndProc: msg[%lx] PRCESSED\n",message);
   return(DC_WndProc);
}


/*---------------------------------------------------------------------------*/
/* TF_WndProc()                                                        [mbb] */
/*---------------------------------------------------------------------------*/

VOID termKeyProc(HWND hWnd, WORD message, WPARAM wParam,LONG lParam)    /* mbbx 1.04: keymap ... */
{
   STRING   termStr[STR255];
   POINT   point;    /* porting macro  change (JAP) */

   if(keyMapTranslate(&wParam, &lParam, termStr))
   {
      if(!(lParam & (1L << 31)))
         fKeyStrBuffer(termStr+1, *termStr);    /* termStr -> fKeyStr */
      return;
   }

   if(keyMapSysKey(hWnd, message, &wParam, lParam)) // sdj: AltGr
      return;

   if((hWnd == hTermWnd) && !(lParam & (1L << 31)))
   {
      switch(classifyKey(wParam))
      {
      case LONGBREAK:
         modemSendBreak(30);
         break;

      case SHORTBREAK:
         modemSendBreak(2);
         break;

      case TERMINALFKEY:
         if(keyPadSequence() && (*keyPadString > 0))     /* mbbx 1.04 ... */
            fKeyStrBuffer(keyPadString+1, *keyPadString);   /* keyPadString -> fKeyStr */
         else
            sysBeep();
         break;

      case SCROLLKEY:
         switch(wParam)
         {
         case VK_HOME:
         case VK_END:
/* rjs bugs 009 - add if statement from vk_prior case */
            if(keyMapState & VKS_CTRL)
            {
               nScrollPos.y = ((wParam == VK_HOME) ? 0 : nScrollRange.y);
               updateTermScrollBars(TRUE);
            }
            else
            {
               nScrollPos.x = ((wParam == VK_HOME) ? 0 : nScrollRange.x);
               updateTermScrollBars(TRUE);
            }
            break;

         case VK_PRIOR:
         case VK_NEXT:
            pageScroll((wParam == VK_PRIOR) ? SB_PAGEUP : SB_PAGEDOWN);
            break;

         case VK_UP:
         case VK_DOWN:
         case VK_RIGHT:
         case VK_LEFT:
            offCursor();
            longToPoint(hTE.selStart, &point);
            termClick(point, (keyMapState & VKS_SHIFT) ? TRUE : FALSE, wParam);
            onCursor();
            activSelect = TRUE;
            noSelect = FALSE;    /* rjs bugs 020 */
            break;
         }
         break;

      default:
         switch(wParam)
         {
         case VK_INSERT:
            switch(keyMapState & (VKS_SHIFT | VKS_CTRL))
            {
            case VKS_SHIFT:
               if (!scrapSeq && ((xferFlag == XFRNONE) || (xferFlag == XFRRCV)) ) /* jtf 3.17 disable send if in transfer */
                  doEditMenu(EMPASTE);
               break;
            case VKS_CTRL:
               if (!scrapSeq && ((xferFlag == XFRNONE) || (xferFlag == XFRRCV)) ) /* jtf 3.Final disable copy if in transfer */
                  doEditMenu(EMCOPY);
               break;
            case VKS_SHIFT | VKS_CTRL:
               if (!scrapSeq && ((xferFlag == XFRNONE) || (xferFlag == XFRRCV)) ) /* jtf 3.17 disable send if in transfer */
               doEditMenu(EMCOPYTHENPASTE);
               break;
            }
            break;

         case VK_BACK:
            if(keyMapState & VKS_CTRL)   /* rjs bugs 014 */
            {
               fKeyStrBuffer("\177", 1); /* rjs bugs 014 */
            }
            else                         /* rjs bugs 014 */
            {
               wParam = VK_DELETE;
               fKeyStrBuffer("\010\040\010", 3); /* jtf 3.33 */
            }
            break;

         case 'C':
         case 'c':
            if(!trmParams.useWinCtrl)
               break;

            switch(keyMapState & (VKS_SHIFT | VKS_CTRL | VKS_ALT))
            {
            case VKS_CTRL:
               if (!scrapSeq && ((xferFlag == XFRNONE) || (xferFlag == XFRRCV)) ) /* jtf 3.Final disable copy if in transfer */
                  doEditMenu(EMCOPY);
               return;
            default:
               break;
            }
            break;

         case 'V':
         case 'v':
            if(!trmParams.useWinCtrl)
               break;

            switch(keyMapState & (VKS_SHIFT | VKS_CTRL | VKS_ALT))
            {
            case VKS_CTRL:
               if (!scrapSeq && ((xferFlag == XFRNONE) || (xferFlag == XFRRCV)) ) /* jtf 3.Final disable copy if in transfer */
                  doEditMenu(EMPASTE);
               return;
            default:
               break;
            }
            break;
         }
	 keyMapKeyProc(hWnd, message, wParam, lParam);	// sdj: AltGr
         break;
      }
   }
}


/* mbbx 1.04: split DC & TF... */

LONG  APIENTRY TF_WndProc(HWND  hWnd, UINT  message, WPARAM wParam, LONG  lParam)
{
   LONG           TF_WndProc = 0L;
   INT            tmpPortLocks;
   HDC            tmpThePort;
   PAINTSTRUCT    ps;
   RECT           clipRect;                  /* mbbx 1.04: erase bkgd */
   POINT          point;

DEBOUT("TF_WndProc: msg[%lx] GOT IT\n",message);
   switch(message)
   {
   case WM_SIZE:
      break;

   case WM_SETFOCUS: 
   case WM_KILLFOCUS:
      if  (hTermWnd != NULL)     /* jtf 3.30  */
         UpdateWindow(hTermWnd); /* jtf 3.30 */
      if(message == WM_SETFOCUS)
      {
         BringWindowToTop(hWnd);
         if(childZoomStatus(0x4001, 0) && !IsZoomed(hWnd))
            ShowWindow(hWnd, SW_MAXIMIZE);
         showTermCursor(); /* rjs bugs 011 */
      }
      else
      {
         if(childZoomStatus(0x8000, 0) && IsZoomed(hWnd))
            ShowWindow(hWnd, SW_RESTORE);
         hideTermCursor(); /* rjs bugs 011 */
      }
      FlashWindow(hWnd, TRUE);
      TF_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      break;

   case WM_PAINT:
      offCursor();
      tmpThePort = thePort;
      tmpPortLocks = portLocks;


      BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
      thePort = ps.hdc;

      if (xferFlag <= XFRNONE)
          FrameRect(thePort, (LPRECT) &statusRect, blackBrush);     // This works, used to be after reDrawTermLine

      portLocks = 1;
      hTE.active = FALSE;
      SelectObject(thePort, hTE.hFont);


      /* rjs swat - was below next if block */
      termDeactivate(&hTE);   /* rjs swat - added this line */
      reDrawTermScreen(0, visScreenLine+1, curTopLine - savTopLine); /* jtf 3.20 this was up one line this should repaint faster */
      termActivate(&hTE);

      if(xferFlag > XFRNONE)                 /* mbbx 1.04: fkeys... */
         updateIndicators();
      else
         reDrawTermLine(maxScreenLine+1, 0, maxChars);

      EndPaint(hWnd, (LPPAINTSTRUCT) &ps);


      portLocks = tmpPortLocks;
      thePort = tmpThePort;
      onCursor();
      break;

   case WM_ERASEBKGND:                       /* mbbx 1.04: fkeys... */
      GetClipBox((HDC) wParam, (LPRECT) &clipRect);
      eraseColorRect((HDC) wParam, (LPRECT) &clipRect, ANORMAL);
      TF_WndProc = 1L;
      break;

   case WM_NCLBUTTONDOWN:
      BringWindowToTop(hWnd);
      SetFocus(hWnd);
      TF_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      break;

   case WM_KEYDOWN:                          /* mbbx 1.04: keymap ... */
   case WM_KEYUP:
   case WM_SYSKEYDOWN:
   case WM_SYSKEYUP:
      if (wParam==VK_F10)                     /* jtf 3.21 */
         {
         TF_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
         break;
         }
      termKeyProc(hWnd, message, wParam, lParam);
      break;

   case WM_CHAR:                             /* mbbx 1.04: keymap... */
      if((xferFlag == XFRNONE) || (xferFlag == XFRRCV) || xferPaused)
         if(!fKeyStrBuffer((BYTE *) &wParam, 1))
            sysBeep();

/* rjs bugs 017 -> this entire if statement */
      if(nScrollPos.y != nScrollRange.y)
         if(maxScreenLine < visScreenLine)
         {
            nScrollPos.y = nScrollRange.y;
            updateTermScrollBars(TRUE);
         }
         else
            if((maxScreenLine - visScreenLine) < curLin)
            {
               nScrollPos.y = nScrollRange.y;
               updateTermScrollBars(TRUE);
            }
/* rjs bugs 017 -> the end of the fix */

      break;

   case WM_SETCURSOR:  /* block resize box make it arrow jtfterm */
      if ( (LOWORD(lParam) == HTBOTTOMRIGHT) ||
           (LOWORD(lParam) == HTBOTTOM) )
         lParam = MAKELONG( HTNOWHERE, HIWORD(lParam) );
      TF_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      break;

   case WM_SYSCOMMAND:
      switch(wParam & 0xFFF0)
      {
      case SC_SIZE: /*  block resize box jtfterm */
         break; 
      case SC_CLOSE:
         termCloseFile();
         break;
      default:
         TF_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
         break;
      }
      break;

   case WM_HSCROLL:
      switch(GET_WM_HSCROLL_CODE(wParam, lParam))
      {
      case SB_LINEUP:
         scrollUp(SB_HORZ, GET_WM_HSCROLL_CODE(wParam, lParam), 1);
         break;
      case SB_LINEDOWN:
         scrollDown(SB_HORZ, GET_WM_HSCROLL_CODE(wParam, lParam), 1);
         break;
      case SB_PAGEUP:
      case SB_PAGEDOWN:
#ifdef ORGCODE
         hpageScroll(GET_WM_HSCROLL_CODE(wParam, lParam));
#else
/* it is coded a hPageScroll in scroll.c, no such routine as hpageScroll-sdj*/
         hPageScroll(GET_WM_HSCROLL_CODE(wParam, lParam));
#endif
         break;
      case SB_THUMBPOSITION:
         nScrollPos.x = GET_WM_HSCROLL_POS(wParam, lParam);
         updateTermScrollBars(TRUE);
         break;
      }
      break;

   case WM_VSCROLL:
      switch(GET_WM_VSCROLL_CODE(wParam, lParam))
      {
      case SB_LINEUP:
         scrollUp(SB_VERT, GET_WM_VSCROLL_CODE(wParam, lParam), 1);
         break;
      case SB_LINEDOWN:
         scrollDown(SB_VERT, GET_WM_VSCROLL_CODE(wParam, lParam), 1);
         break;
      case SB_PAGEUP:
      case SB_PAGEDOWN:
         pageScroll(GET_WM_VSCROLL_CODE(wParam, lParam));
         break;
      case SB_THUMBPOSITION:
         nScrollPos.y = GET_WM_VSCROLL_POS(wParam, lParam);
         updateTermScrollBars(TRUE);
         break;
      }
      break;

   case WM_LBUTTONDOWN:
      if(GetFocus() != hWnd)                 /* mbbx 1.04 ... */
      {
         BringWindowToTop(hWnd);
         SetFocus(hWnd);
         break;
      }

      offCursor();
//      termClick(MAKEMPOINT(lParam), (wParam & MK_SHIFT), FALSE);
      point.x = (LONG)LOWORD(lParam);
      point.y = (LONG)HIWORD(lParam);
      termClick(point, (wParam & MK_SHIFT), FALSE);
      onCursor();
      activSelect = TRUE;
      noSelect = FALSE;    /* rjs bugs 020 */
      break;

   default:
      TF_WndProc = DefWindowProc(hWnd, message, wParam, lParam);
      // idleProcess();   -sdj CAUSES RECURSION AND GDI/USER DEATH
      break;
   }

DEBOUT("TF_WndProc: msg[%lx] PRCESSED\n",message);
   return(TF_WndProc);
}
