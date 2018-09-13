/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                        *
*   file:   ParentWP.c                                   *
*   system: PC Paintbrush for MS-Windows                 *
*   descr:  window proc for parent window                *
*   date:   03/18/87 @ 11:00                             *
*                                                        *
********************************************************/

#include <windows.h>
#include <port1632.h>
#include <shellapi.h>

#ifdef DBCS_IME
#include <ime.h>
#endif

#include "oleglue.h"
#include "pbrush.h"


#ifdef DBCS_IME
#include <winnls.h>
#endif


int UpdateCount = 0;

#ifdef DBCS_IME
/* IME can only be used when user selects text function */
BOOL bInitialIMEState;
BOOL bGetIMEState = FALSE; // 02/12/93 raid #3725
#endif

extern HWND pbrushWnd[];
extern int defaultWid, defaultHgt;
extern BOOL drawing;
extern HWND zoomOutWnd;
extern RECT pbrushRct[];
extern BOOL inMagnify, mouseFlag, bZoomedOut;
extern BOOL gfDirty;
extern TCHAR fileName[];
extern TCHAR noFile[];
extern int theTool, theSize, theForeg;
extern BOOL bIsPrinterDefault;
extern TCHAR deviceStr[];
extern HPALETTE hPalette;
extern BOOL bJustActivated;
extern int theBackg;
extern DWORD *rgbColor;
extern int SizeTable[];
extern int YPosTable[];
extern HWND mouseWnd;
extern BOOL TerminateKill;
extern BOOL IsCanceled;
extern WNDPROC lpMouseDlg, lpColorDlg, lpNullWP;
extern BOOL bPrtCreateErr;
extern PRINTDLG PD;

#define SIZE_FUDGE 7

void FileDragOpen(TCHAR szPath[]);

void doDrop(HANDLE wParam, HWND hwnd);

long FAR PASCAL
ParentWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   static int Thing;
   static int RepeatCount = 0;
   static long lastMsgTime = 0;
   static WORD lastMsgWParam = 0;
   static BOOL processCntrlI = TRUE;
   static BOOL printerChanged = FALSE;
   static int CurrentWindow = PAINTid;

   HWND theWnd;
   WORD shiftStates;
   long lResult;
   int i, command, answer;
   HDC hdcPrint, parentDC;
   HCURSOR oldcsr;
   RECT trect, trect1;
   POINT tpoint, cursDiff;
   int x, y;
   DWORD dwMsgPos;
   HPALETTE hOldPalette = NULL;
   BOOL bButtonDown;
   WORD scrollAmount;

   switch (message)
   {
   case WM_ERRORMSG:
      SimpleMessage((WORD)wParam, (LPTSTR)lParam, MB_OK | MB_ICONEXCLAMATION);
      break;

   /* create and initialize the image and file buffers */
   case WM_ACTIVATE:
      if((GET_WM_ACTIVATE_STATE(wParam, lParam) == 0) && TerminateKill)
      {
         DB_OUTF((acDbgBfr,TEXT("wParam = %lx, lParam = %ld WM_ACTIVATE in parentwp\n"),
            wParam,lParam));

         SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
      }
      else if(GET_WM_ACTIVATE_STATE(wParam,lParam) == 2 && !IsIconic(hWnd))
      {
         GetWindowRect(pbrushWnd[PAINTid], &trect);
         dwMsgPos = GetMessagePos();
         LONG2POINT(dwMsgPos, tpoint);
         bJustActivated = PtInRect(&trect, tpoint);
      }
      lResult = DefWindowProc(hWnd, message, wParam, lParam);
      if(printerChanged)
      {
         PostMessage(pbrushWnd[PARENTid], WM_ERRORMSG, IDSPrinterChange, 0L);
         printerChanged = FALSE;
      }
      return(lResult);
      break;

   case WM_CREATE:
      defaultWid = GetSystemMetrics(SM_CXSCREEN);
      defaultHgt = GetSystemMetrics(SM_CYSCREEN);
      break;

#ifdef DBCS_IME
   // control IME status when focus messages are received
   case WM_SETFOCUS:
      bInitialIMEState = WINNLSEnableIME(NULL,FALSE);
      // KKBUGFIX                           //02/12/93 raid #3725
      bGetIMEState = TRUE;
      break;

   case WM_KILLFOCUS:
      // KKBUGFIX                           //02/12/93 raid #3725
      if(bGetIMEState)
      {
          WINNLSEnableIME(NULL,bInitialIMEState);
          bGetIMEState = FALSE;
      }
      break;
#endif

   case WM_SYSCOMMAND:
      if(drawing)
         break;

      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

      command = wParam & 0xFFF0;
      if(!inMagnify && command != SC_MOUSEMENU && command != SC_KEYMENU)
      {
         SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
         UpdatImg();
      }
      else if(inMagnify && (command == SC_ICON || command == SC_CLOSE))
      {
         SendMessage(pbrushWnd[PAINTid], WM_ZOOMACCEPT, 0, 0L);
         SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
         UpdatImg();
      }
      else if(bZoomedOut
                || (inMagnify && command != SC_MOUSEMENU && command != SC_KEYMENU))
      {
         SendMessage(pbrushWnd[PAINTid], WM_SCROLLINIT, 0, 0L);
      }
      else if(command == SC_MOUSEMENU || command == SC_KEYMENU)
      {
         SendMessage(pbrushWnd[PAINTid], WM_HIDECURSOR, 0, 0L);
      }

      SetCursor(oldcsr);

      return(DefWindowProc(hWnd,message,wParam,lParam));
      break;

   /* enable paste item if bitmap in clipboard */
   case WM_INITMENU:
      {
          CLIPFORMAT cf;
          SendMessage(pbrushWnd[PAINTid], WM_HIDECURSOR, 0, 0l);
          EnableMenuItem(ghMenuFrame,
                        EDITpaste,
                        (!inMagnify && OleClipboardContainsAcceptableFormats(&cf)) ? MF_ENABLED : MF_GRAYED);
          if(bPrtCreateErr)
          {
             bPrtCreateErr = FALSE;
             GetPrintParms(NULL);
          }
          if(!gfInPlace)
              EnableMenuItem(ghMenuFrame, FILEprint, bPrtCreateErr? MF_GRAYED : MF_ENABLED);

          break;
      }

   case WM_COMMAND:
      if(pbrushWnd[PARENTid]
          && (GET_WM_COMMAND_ID(wParam, lParam) != STYLEitalic || processCntrlI))
      {
         MenuCmd(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
      }
      break;

   case WM_DESTROY:
#ifdef DBCS_IME
      /* backup IME status before we gone */
      // KKBUGFIX                           //02/12/93 raid #3725
      if(bGetIMEState)
      {
          WINNLSEnableIME(NULL, bInitialIMEState);
          bGetIMEState = FALSE;
      }
#endif
      /* delete allocated image and file buffer memory */
      FreeImg();
//      TerminateShapeLibrary();

      Help(hWnd, HELP_QUIT, 0L);

      if (lpMouseDlg)
         FreeProcInstance(lpMouseDlg);
      if (lpColorDlg)
         FreeProcInstance(lpColorDlg);
      if (lpNullWP)
         FreeProcInstance(lpNullWP);

      if(hPalette)
         DeleteObject(hPalette);
      hPalette = NULL;

      pbrushWnd[PARENTid] = NULL;   /* signal parent window invalid */
      PostQuitMessage(0);
      break;

   case WM_MOVE:
      /* reposition the mouse window */
      if(!gfInPlace && mouseFlag)
      {
         GetWindowRect(mouseWnd, &trect);
         GetWindowRect(pbrushWnd[PARENTid], &trect1);
         MoveWindow(mouseWnd,
               trect1.right - 2 * GetSystemMetrics(SM_CXSIZE)
                            - GetSystemMetrics(SM_CXFRAME)
                            - (trect.right - trect.left) - 8,
               trect1.top + GetSystemMetrics(SM_CYFRAME)
                          - GetSystemMetrics(SM_CYBORDER),
               trect.right - trect.left,
               trect.bottom - trect.top,
               TRUE);
      }
      return(DefWindowProc(hWnd,message,wParam,lParam));
      break;

   case WM_SIZE:
      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
      if(!gfInPlace && wParam != SIZEICONIC)
      {
         pbrushRct[PARENTid].right = LOWORD(lParam);
         pbrushRct[PARENTid].bottom = HIWORD(lParam);
         if(pbrushWnd[PAINTid] && IsWindow(pbrushWnd[PAINTid]))
         {
            CalcWnds(NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW);
            for(i = 1; i < MAXwnds; ++i)
            {
               MoveWindow(pbrushWnd[i],
                     pbrushRct[i].left,
                     pbrushRct[i].top,
                     pbrushRct[i].right - pbrushRct[i].left,
                     pbrushRct[i].bottom - pbrushRct[i].top,
                     TRUE);
            }
            if(bZoomedOut)
            {
               MoveWindow(zoomOutWnd,
                     pbrushRct[PAINTid].left,
                     pbrushRct[PAINTid].top,
                     pbrushRct[PAINTid].right - pbrushRct[PAINTid].left,
                     pbrushRct[PAINTid].bottom - pbrushRct[PAINTid].top,
                     TRUE);
            }
         }
      }
      SetCursor(oldcsr);
      break;

      case WM_CLOSE:
      case WM_QUERYENDSESSION:
            oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
            SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
            UpdatImg();
            if (!(answer = SaveAsNeeded()))
                break;
            if(message == WM_CLOSE)
            {
                gfUserClose = TRUE;
                TerminateServer();
            }
            else
            {
                SetCursor(oldcsr);
                return TRUE;
            }
            SetCursor(oldcsr);
            break;

     case WM_ENDSESSION:
     /* remove any temporary files */
       if(wParam)
           FreeImg();
       break;

   case WM_ACTIVATEAPP:

       if(!gfStandalone)
            break;

      i = ShowCursor(wParam);

      DB_OUTF((acDbgBfr,TEXT("wParam = %lx, SC %d, WM_ACTIVATEAPP in parentwp\n"),
          wParam,i));

      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
      if(pbrushWnd[PAINTid])
      {
         if(wParam == 0)
         {
            if(!inMagnify && !bZoomedOut)
            {
               if(TerminateKill)
               {
                  SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
                  UpdatImg();
               }
            } else
               SendMessage(pbrushWnd[PAINTid], WM_SCROLLINIT, 0, 0L);
         }
         else
         {
            BringWindowToTop(pbrushWnd[PAINTid]);
            InvalidateRect(pbrushWnd[PAINTid], NULL, FALSE);
            UpdateWindow(pbrushWnd[PAINTid]);
         }
      }
      SetCursor(oldcsr);
      break;

   case WM_KEYUP:
      RepeatCount = 0;
      if(bZoomedOut && CurrentWindow == PAINTid)
         theWnd = zoomOutWnd;
      else
         theWnd = pbrushWnd[CurrentWindow];

      switch(wParam)
      {
      case VK_INSERT:
         message = WM_LBUTTONUP;
         break;

      case VK_DELETE:
         message = WM_RBUTTONUP;
         break;

      default:
         return(0L);
         break;
      }

      wParam = (GetKeyState(VK_CONTROL)&0x8000 ? MK_CONTROL : 0)
             | (GetKeyState(VK_MBUTTON)&0x8000 ? MK_MBUTTON : 0)
             | (GetKeyState(VK_LBUTTON)&0x8000 ? MK_LBUTTON : 0)
             | (GetKeyState(VK_RBUTTON)&0x8000 ? MK_RBUTTON : 0)
             | (GetKeyState(VK_SHIFT  )&0x8000 ? MK_SHIFT   : 0);

      GetCursorPos(&tpoint);
      ScreenToClient(theWnd, &tpoint);
      lParam = MAKELONG(tpoint.x, tpoint.y);

      PostMessage(theWnd, message, wParam, lParam);
      break;

   case WM_KEYDOWN:
      processCntrlI = TRUE;
      shiftStates = (WORD)((GetKeyState(VK_CONTROL)&0x8000 ? MK_CONTROL : 0)
             | (GetKeyState(VK_MBUTTON)&0x8000 ? MK_MBUTTON : 0)
             | (GetKeyState(VK_LBUTTON)&0x8000 ? MK_LBUTTON : 0)
             | (GetKeyState(VK_RBUTTON)&0x8000 ? MK_RBUTTON : 0)
             | (GetKeyState(VK_SHIFT  )&0x8000 ? MK_SHIFT   : 0));

      bButtonDown = (GetKeyState(VK_INSERT)&0x8000)
            || (GetKeyState(VK_DELETE)&0x8000)
            || (GetKeyState(VK_LBUTTON)&0x8000)
            || (GetKeyState(VK_RBUTTON)&0x8000);

      GetCursorPos(&tpoint);
      CurrentWindow = PAINTid;       /* in case cursor not in any window */
      for(i = 1; i < MAXwnds; ++i)
      {
         GetWindowRect(pbrushWnd[i], &trect);
         if(PtInRect(&trect, tpoint))
         {
            CurrentWindow = i;
            break;
         }
      }
      if(bZoomedOut && CurrentWindow == PAINTid)
         theWnd = zoomOutWnd;
      else
         theWnd = pbrushWnd[CurrentWindow];

      GetWindowRect(theWnd, &trect1);
      GetClientRect(theWnd, &trect);

      switch(wParam)
      {
      case VK_ESCAPE:
         if(!bButtonDown && bZoomedOut)
            SendMessage(zoomOutWnd, message, wParam, lParam);
         break;

      case VK_INSERT:
         if((lParam&(1L<<30)) || (GetKeyState(VK_LBUTTON)&0x8000))
            break;

         message = WM_LBUTTONDOWN;

         wParam = shiftStates;

         ScreenToClient(theWnd, &tpoint);
         lParam = MAKELONG(tpoint.x, tpoint.y);

         PostMessage(theWnd, message, wParam, lParam);

         if(GetKeyState(VK_F9) & 0x8000)
         {
            PostMessage(theWnd, WM_LBUTTONUP, wParam, lParam);
            PostMessage(theWnd, WM_LBUTTONDBLCLK, wParam, lParam);
         }
         break;

      case VK_DELETE:
         if((lParam&(1L<<30)) || (GetKeyState(VK_RBUTTON)&0x8000))
            break;

         message = WM_RBUTTONDOWN;

         wParam = shiftStates;

         ScreenToClient(theWnd, &tpoint);
         lParam = MAKELONG(tpoint.x, tpoint.y);

         PostMessage(theWnd, message, wParam, lParam);

         if(GetKeyState(VK_F9) & 0x8000)
         {
            PostMessage(theWnd, WM_RBUTTONUP, wParam, lParam);
            PostMessage(theWnd, WM_RBUTTONDBLCLK, wParam, lParam);
         }
         break;

      case VK_HOME:
         scrollAmount = SB_TOP;
         goto KeyScroll;

      case VK_END:
         scrollAmount = SB_BOTTOM;
         goto KeyScroll;

      case VK_NEXT:
         scrollAmount = SB_PAGEDOWN;
         goto KeyScroll;

      case VK_PRIOR:
         scrollAmount = SB_PAGEUP;
         goto KeyScroll;

KeyScroll:
         if(!bButtonDown)
         {
            PostMessage(pbrushWnd[PAINTid],
                  (WORD)((GetKeyState(VK_SHIFT) & 0x8000) ? WM_HSCROLL : WM_VSCROLL),
                  (WPARAM)scrollAmount,
                  0l);
         }
         break;

      case VK_RIGHT:
         RepeatCount++;
         if(!bButtonDown && (GetKeyState(VK_SHIFT)&0x8000))
         {
            PostMessage(pbrushWnd[PAINTid], WM_HSCROLL, SB_LINEDOWN, 0l);
         }
         else
         {
            switch (CurrentWindow)
            {
            case TOOLid:
               if(Thing < MAXtools-1)
                  ++Thing;
               goto MoveToolCurs;

            case PAINTid:
               cursDiff.x = RepeatCount;
               cursDiff.y = 0;
               goto MovePaintCurs;

            case SIZEid:
               goto MoveSizeCurs;

            case COLORid:
               if(Thing < MAXcolors - 2)
                  Thing += 2;
               goto MoveColorCurs;
            }
         }
         break;

      case VK_LEFT:
         RepeatCount++;
         if(!bButtonDown && (GetKeyState(VK_SHIFT) & 0x8000))
         {
            PostMessage(pbrushWnd[PAINTid], WM_HSCROLL, SB_LINEUP, 0l);
         }
         else
         {
            switch (CurrentWindow)
            {
            case TOOLid:
               if(Thing > 0)
                  --Thing;
               goto MoveToolCurs;

            case PAINTid:
               cursDiff.x = -RepeatCount;
               cursDiff.y = 0;
               goto MovePaintCurs;

            case SIZEid:
               goto MoveSizeCurs;

            case COLORid:
               if(Thing > 1)
                  Thing -= 2;
               goto MoveColorCurs;
            }
         }
         break;

      case VK_UP:
         RepeatCount++;
         if(!bButtonDown && (GetKeyState(VK_SHIFT) & 0x8000))
         {
            PostMessage(pbrushWnd[PAINTid], WM_VSCROLL, SB_LINEUP, 0l);
         }
         else
         {
            switch (CurrentWindow)
            {
            case TOOLid:
               if(Thing > 1)
                  Thing -= 2;
               goto MoveToolCurs;

            case PAINTid:
               cursDiff.x = 0;
               cursDiff.y = -RepeatCount;
               goto MovePaintCurs;

            case SIZEid:
               if(Thing > 0)
                  --Thing;
               goto MoveSizeCurs;

            case COLORid:
               if(Thing > 0)
                  --Thing;
               goto MoveColorCurs;
            }
         }
         break;

      case VK_DOWN:
         RepeatCount++;
         if(!bButtonDown && (GetKeyState(VK_SHIFT) & 0x8000))
         {
            PostMessage(pbrushWnd[PAINTid], WM_VSCROLL, SB_LINEDOWN, 0l);
         }
         else
         {
            switch (CurrentWindow)
            {
            case TOOLid:
               if(Thing < MAXtools-2)
                  Thing += 2;
               goto MoveToolCurs;

            case PAINTid:
               cursDiff.x = 0;
               cursDiff.y = RepeatCount;
               goto MovePaintCurs;

            case SIZEid:
               if(Thing < NUM_SIZES-1)
                  ++Thing;
               goto MoveSizeCurs;

            case COLORid:
               if(Thing < MAXcolors-1)
                  ++Thing;
               goto MoveColorCurs;
            }
         }
         break;

      case VK_TAB:
         processCntrlI = FALSE;
         dwMsgPos = GetMessagePos();
         LONG2POINT(dwMsgPos,tpoint);
         SendMessage(theWnd, WM_LBUTTONUP, shiftStates,
                     MAKELONG((tpoint.x - trect1.left) -
                     GetSystemMetrics(SM_CXBORDER),
                     (tpoint.y - trect1.top) -
                     GetSystemMetrics(SM_CYBORDER)));

         do
         {
            if(!(GetKeyState(VK_SHIFT) & 0x8000))
               CurrentWindow = CurrentWindow % 4 + 1;
            else
               CurrentWindow = (CurrentWindow + 2) % 4 + 1;
            if(bZoomedOut && CurrentWindow == PAINTid)
               theWnd = zoomOutWnd;
            else
               theWnd = pbrushWnd[CurrentWindow];
         } while (!IsWindowVisible(theWnd));

         GetWindowRect(theWnd, &trect1);
         GetClientRect(theWnd, &trect);
         switch (CurrentWindow)
         {
         case TOOLid:
            Thing = theTool;
            goto MoveToolCurs;

         case PAINTid:
            cursDiff.x = cursDiff.y = 0;
            goto MovePaintCurs;

         case SIZEid:
            for(Thing=0; SizeTable[Thing]<theSize; ++Thing) ;
            goto MoveSizeCurs;

         case COLORid:
            Thing = theForeg;
            goto MoveColorCurs;
         }
         break;

      MoveToolCurs:
         x = (trect.right  * ((Thing % 2) * 2 + 1)) / 4;
         y = (trect.bottom * ((Thing >> 1) * 2 + 1)) / MAXtools;
         goto MoveMiscCurs;

      MoveSizeCurs:
         x = trect.right / 2;
         y = (int)((long)(YPosTable[Thing] + SizeTable[Thing] / 2)
               * (trect.bottom - trect.top) / SIZE_EXTY);
         goto MoveMiscCurs;

      MoveColorCurs:
         x = ((((Thing >> 1) << 1) + 5) * (trect.right/ COLORdiv)) / 2;
         y = (((Thing % 2) * 2 + 1) * trect.bottom) / 4 ;
         goto MoveMiscCurs;

      MoveMiscCurs:
         SetCursorPos(trect1.left +
               GetSystemMetrics(SM_CXBORDER) + x,
               trect1.top + GetSystemMetrics(SM_CYBORDER) + y);
         break;

      MovePaintCurs:
         ClientToScreen(theWnd, ((LPPOINT)&trect));
         ClientToScreen(theWnd, ((LPPOINT)&trect) + 1);
         GetCursorPos(&tpoint);
         tpoint.x += cursDiff.x;
         tpoint.y += cursDiff.y;
         tpoint.x = min(max(tpoint.x, trect.left), trect.right  - 1);
         tpoint.y = min(max(tpoint.y, trect.top ), trect.bottom - 1);
         SetCursorPos(tpoint.x, tpoint.y);
         break;

      default:
            break;
      }
      break;

#ifdef KOREA
   case WM_INTERIM:
#endif
   case WM_CHAR:
      SendMessage(pbrushWnd[PAINTid], message, wParam, lParam);
      break;

#ifdef JAPAN        //  added by Hiraisi
   case WM_IME_REPORT:
      return( SendMessage(pbrushWnd[PAINTid], message, wParam, lParam) );
      break;
#endif

   case WM_WININICHANGE:
      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

      if(!bIsPrinterDefault)
      {
         if(!(hdcPrint = GetPrtDC()))
         {
            bIsPrinterDefault = TRUE;
            printerChanged = TRUE;

            hWnd = GetActiveWindow();
            for(i = 1; i < MAXwnds; ++i)
            {
               if(pbrushWnd[i] == hWnd)
                  printerChanged = FALSE;
            }
            if(!printerChanged)
            {
               PostMessage(pbrushWnd[PARENTid], WM_ERRORMSG,
                     IDSPrinterChange, 0L);
            }
         }
         else
         {
            DeleteDC(hdcPrint);
         }
      }

      /* Let Commdlg retrieve default printer so we can modify printer
       * settings with hDevMode.
       */
      if(bIsPrinterDefault)
         GetDefaultPort();

      GetPrintParms(NULL);

      InitDecimal((LPTSTR)lParam);

      if (mouseFlag)
      {
           TCHAR szSep[3];
extern TCHAR sList[];
extern TCHAR szIntl[];

           GetProfileString(szIntl, sList, TEXT(","), szSep, CharSizeOf(szSep));
           SetDlgItemText(mouseWnd, IDMOUSESEP, szSep);
      }

      SetCursor(oldcsr);
      break;

   case WM_DROPFILES: /*case added 03/26/91 for file drag/drop support*/
        doDrop((HANDLE)wParam,hWnd);
        break;

   /* Palette manager stuff... */
   case WM_QUERYNEWPALETTE:
      /* we are about to receive input focus.  Return TRUE if we realize
         palette, FALSE otherwise. */
      if(!hPalette)
         break;
      parentDC = GetDC(hWnd);
      hOldPalette = SelectPalette(parentDC, hPalette, 0);
      x = RealizePalette(parentDC);
      if (hOldPalette)
         SelectPalette(parentDC, hOldPalette, 0);
      ReleaseDC(hWnd, parentDC);
      if(x)
      {
         InvalidateRect(hWnd, (LPRECT) NULL, 1);
         return TRUE;
      }
      else
      {
         return FALSE;
      }
      break;

   case WM_PALETTECHANGED:
      if(!hPalette || (HWND)wParam == hWnd)
         break;

      parentDC = GetDC(hWnd);
      hOldPalette = SelectPalette(parentDC, hPalette, 0);
      x = RealizePalette(parentDC);

      InvalidateRect(hWnd, NULL, FALSE);

      if (hOldPalette)
         SelectPalette(parentDC, hOldPalette, 0);
      ReleaseDC(hWnd, parentDC);
      break;


   default:
      return(DefWindowProc(hWnd,message,wParam,lParam));
      break;
   }

   return(0L);
}

BOOL FAR
SaveAsNeeded(void)
{
    WORD wResult = IDYES;
    iExitWithSaving = IDNO;

    if (gfDirty)
    {
        if(gfStandalone)
        {
            wResult = SimpleMessage((WORD)IDSSaveTo,
                              *fileName ? fileName : noFile,
                              MB_YESNOCANCEL | MB_ICONEXCLAMATION);
        }
        switch (wResult)
        {
            case IDCANCEL:
                return FALSE;
                break;

            case IDYES:
                SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEsave, 0L);
                // IsCanceled is set to TRUE if during FILEsave, the SaveAs
                // dlg is cancelled or file write fails
                if (IsCanceled)
                    return FALSE;
                break;

            default:
                return IDNO;
                break;
        }
    }
    return IDYES;
}


/* Proccess file drop/drag options. */
void
doDrop(HANDLE hDrop, HWND hwnd)
{
    TCHAR szPath[MAX_PATH];

     if (DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0)) /* # of files dropped */
     {
           /* If user dragged/dropped a file regardless of keys pressed
            * at the time, open the first selected file from file
            * manager.
            */
           DragQueryFile(hDrop,0,szPath,MAX_PATH);
           SetActiveWindow(hwnd);
           FileDragOpen(szPath);
     }
     DragFinish(hDrop);      /* Delete structure alocated for WM_DROPFILES*/
}

void
FileDragOpen(TCHAR szPath[])
{
    if (!SaveAsNeeded())
        return;
    SetupFileVars(szPath);
    SendMessage(pbrushWnd[PARENTid], WM_COMMAND, GET_WM_COMMAND_MPS(FILEload, NULL, 0));
}
