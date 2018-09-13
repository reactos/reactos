/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   PrintDlg.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  print dialog proc                           *
*   date:   03/24/87 @ 10:15                            *
*                                                       *
********************************************************/

#ifdef DEBUG
#define SAVEHEADFOOT
#endif

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NOCTLMGR
#undef NOKERNEL
#undef NOLSTRING
#undef NOMB
#undef NOMEMMGR
#undef NOMENUS
#undef NOMINMAX

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "dlgs.h"
#include "pbrush.h"
#include "fixedpt.h"

extern BOOL FirstPrint;
extern HWND pbrushWnd[], dlgWnd;
extern DPPROC DrawProc;
extern int imageWid, imageHgt;
extern LPTSTR DrawCursor;
extern RECT imageRect;
extern TCHAR winIniAppName[];
extern TCHAR pgmTitle[];
extern int iMeasure;        /* 0 => metric, 1=> English system of measurement */
#ifdef JAPAN            //  added  by Hiraisi (BUG#1389)
static BOOL fEnglish;
#endif


int hSizePrt = 2032, vSizePrt = 2540;    /* HORZSIZE, VERTSIZE */
int hResPrt = 2400, vResPrt = 3000;      /* HORZRES, VERTRES */
int xPelsPrt = 300, yPelsPrt = 300;    /* LOGPIXELSX, LOGPIXELSY */
NUM nmLeft = ToNND(1,2), nmTop = ToNND(1,2);
NUM nmRight = ToNND(1,2), nmBottom = ToNND(1,2);
static int wquality = IDPROOF;   /* set for getprintparms */
static int wamount, wcopies;
BOOL fStretch = TRUE;     /* do we stretch bitmap to printer? */

TCHAR szHeader[80] = TEXT(""), szFooter[80] = TEXT("");

static BOOL PRIVATE ValidateMargins(NUM nmLeft, NUM nmTop,
      NUM nmRight, NUM nmBottom)
{
   NUM nmWidth, nmHeight;

   if(NLTI(nmLeft, 0) ||
         NLTI(nmTop, 0) ||
         NLTI(nmRight, 0) ||
         NLTI(nmBottom, 0)) {
      PbrushOkError(IDSInvalidNumb, MB_ICONHAND);
      return FALSE;
   }

#ifdef JAPAN            //  added  by Hiraisi (BUG#1389)
   if (fEnglish == 0)   /* if Metric, width in mm  */
#else
   if (iMeasure == 0)   /* if Metric, width in mm  */
#endif
   {
       nmWidth =  hSizePrt*10; /* convert to CMs, multiplied by 100 */
       nmHeight = vSizePrt*10;
   }
   else
   {
       nmWidth = NDivI(ToN(hResPrt), xPelsPrt);
       nmHeight = NDivI(ToN(vResPrt), yPelsPrt);
   }

   if(NLEN(nmWidth, NAddN(nmLeft, nmRight)) ||
         NLEN(nmHeight, NAddN(nmTop, nmBottom))) {
      PbrushOkError(IDSInvalidMargins, MB_ICONHAND);
      return FALSE;
   }

   return TRUE;
}

void PUBLIC ComputePrintRect(NPRECT npImageRect, NPRECT npPrintRect,
      NPRECT npHeaderRect, NPRECT npFooterRect)
{

   /* start by computing size of printed page in pixels */
   npPrintRect->left = npPrintRect->top = 0;
   npPrintRect->right = hResPrt;
   npPrintRect->bottom = vResPrt;

   /* adjust fields to account for margins */
   npPrintRect->left   += FLOOR(NMulI(nmLeft  , xPelsPrt));
   npPrintRect->top    += FLOOR(NMulI(nmTop   , yPelsPrt));
   npPrintRect->right  -= FLOOR(NMulI(nmRight , xPelsPrt));
   npPrintRect->bottom -= FLOOR(NMulI(nmBottom, yPelsPrt));

   /* compute header and footer rects */
   *npHeaderRect = *npFooterRect = *npPrintRect;
   npHeaderRect->bottom = npHeaderRect->top;
   npHeaderRect->top = 0;
   npFooterRect->top = npFooterRect->bottom;
   npFooterRect->bottom = vResPrt;
}

extern int nSizeNum, nSizeDen;
int copies, quality;

#define BUFSIZE 128

BOOL FAR PASCAL PrintFileDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   static int amount;
//   RECT printRect;
//   RECT headerRect, footerRect;
   TCHAR szMsg[BUFSIZE], buf[BUFSIZE];
   HDC hDC;
   POINT sFactor;
   long xLogPrint, yLogPrint, xLogImage, yLogImage;
   int nSizeMin, nSizeMax;

   BOOL success;
   HCURSOR oldcsr;
   extern HDC printDC;

   switch(message) {
   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam,lParam)) {
      case IDOK:
         oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
         copies = GetDlgItemInt(hDlg, IDCOPIES, &success, FALSE);
         if((!success) || (copies < 0)) {
            PbrushOkError(IDSInvalidCopy, MB_ICONHAND);
            break;
         }
         fStretch = !IsDlgButtonChecked(hDlg, IDUSEPRINTER);

         nSizeNum = GetDlgItemInt(hDlg, 209, &success, FALSE);

         if(!printDC ||
               Escape(printDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&sFactor) <= 0)
            sFactor.x = sFactor.y = 0;
         sFactor.x = 1 << sFactor.x;
         sFactor.y = 1 << sFactor.y;

         hDC = GetDC(NULL);
         if(fStretch) {
            xLogPrint = (long)xPelsPrt;
            yLogPrint = (long)yPelsPrt;
            xLogImage = (long)nSizeDen * GetDeviceCaps(hDC, LOGPIXELSX);
            yLogImage = (long)nSizeDen * GetDeviceCaps(hDC, LOGPIXELSY);
         } else {
            xLogPrint = sFactor.x;
            yLogPrint = sFactor.y;
            xLogImage = nSizeDen;
            yLogImage = nSizeDen;
         }
         ReleaseDC(NULL, hDC);

/* Actually, I want to round up in the next line, but +1 will do */
         nSizeMin = (int)max(max(hResPrt*xLogImage/(xLogPrint*0x7fffL) + 1,
               vResPrt*yLogImage/(yLogPrint*0x7fffL) + 1), 1);
         nSizeMax = (int)min(min(0x7fffL*xLogImage/(imageWid*xLogPrint),
               0x7fffL*yLogImage/(imageHgt*yLogPrint)), 10000);

         if(!success || nSizeNum<nSizeMin || nSizeNum>nSizeMax) {
            nSizeNum = 100;
            LoadString(hInst, IDSInvalidScale, buf, CharSizeOf(buf));
            wsprintf(szMsg, buf, nSizeMin, nSizeMax);
            MessageBox(hDlg, szMsg, pgmTitle, MB_OK | MB_ICONHAND);
            break;
         }
         nSizeDen = 100;

         wcopies = copies;
         wquality = quality;
         wamount = amount;

         /* The result of the dialog will indicate if IDPARTIAL was selected
          * or not
          */
         EndDialog(hDlg, amount);
         if(amount == IDPARTIAL) {
            DrawProc = PrintDP;
            DrawCursor = IDC_ARROW;
            SendMessage(pbrushWnd[PARENTid], WM_COMMAND, MISCzoomOut, 0L);
         } else {
/**         DrawProc = NULL;  **/ /* signal that IDPARTIAL wasn't selected */
            imageRect.left = imageRect.top = 0;
            imageRect.right = imageWid;
            imageRect.bottom = imageHgt;
            PrintImg(&imageRect, quality == IDDRAFT, copies);
         }
         SetCursor(oldcsr);
         break;

               /* fall through */
/*
      case IDPRINT:
         oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
         ComputePrintRect(&imageRect, &printRect, &headerRect, &footerRect);
         PrintImg(&imageRect, &printRect, &headerRect, &footerRect,
               quality == IDDRAFT, copies);
         SetCursor(oldcsr);
         break;
*/

      case IDCANCEL:
/* Must not reset the DrawProc value to NULL */
/**      DrawProc = NULL;  **/  /* signal that IDPARTIAL wasn't selected */
         EndDialog(hDlg, 0); /* 0 => The dialog was cancelled */
         break;

      case IDDRAFT:
      case IDPROOF:
         CheckRadioButton(hDlg, IDDRAFT, IDPROOF, quality = GET_WM_COMMAND_ID(wParam,lParam));
         break;

      case IDWHOLE:
      case IDPARTIAL:
         CheckRadioButton(hDlg, IDWHOLE, IDPARTIAL,
                amount = GET_WM_COMMAND_ID(wParam,lParam));
         break;
      }
      break;

   case WM_INITDIALOG:
      CenterWindow(hDlg);
      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
      GetPrintParms(printDC);
      if(FirstPrint) {
         wcopies = 1;
         wamount = IDWHOLE;
         wquality = IDPROOF;
      }

      copies = wcopies;
      amount = wamount;
      quality = wquality;

      SetDlgItemInt(hDlg, IDCOPIES, copies, FALSE);
      SetDlgItemInt(hDlg, 209, nSizeNum, FALSE);
#ifdef NEXTVERSION
      {
         HDC hPrtDC;
         long lHolder;

         lHolder = DRAFTMODE;
         if((hPrtDC = GetPrtDC())) {
            if(Escape(hPrtDC, QUERYESCSUPPORT, sizeof(int), (LPSTR) &lHolder, NULL))
               CheckRadioButton(hDlg, IDDRAFT, IDPROOF, quality);
            else {
               EnableWindow(GetDlgItem(hDlg, IDDRAFT), FALSE);
               CheckRadioButton(hDlg, IDDRAFT, IDPROOF, IDPROOF);
            }
            DeleteDC(hPrtDC);
         } else
            CheckRadioButton(hDlg, IDDRAFT, IDPROOF, quality);
      }
#else
      CheckRadioButton(hDlg, IDDRAFT, IDPROOF, quality);
#endif
      CheckRadioButton(hDlg, IDWHOLE, IDPARTIAL, amount);
      /* Don't allow partial print if command line printing
       */
      if (!IsWindowVisible(pbrushWnd[PARENTid]))
          EnableWindow(GetDlgItem(hDlg, IDPARTIAL), FALSE);

      CheckDlgButton(hDlg, IDUSEPRINTER, (WORD)(!fStretch));
      SetCursor(oldcsr);
      break;

   default:
      return FALSE;
   }

   return TRUE;
}

/* When metric system is used, show margins in cms.
 * When English system is used show margins in inches.
 * The final value entered by the user is always stored in nmLeft, nmRight, nmTop, nmBottom
 * in Inches.
 */
BOOL FAR PASCAL PageSetDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   HCURSOR oldcsr;
   NUM tnmLeft, tnmTop, tnmRight, tnmBottom;
   NUM Num;
#ifdef JAPAN // Change caption of Page settings layout by intl
   TCHAR szSpaceText[32];
#endif

   switch(message) {
   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam,lParam)) {
      case IDDEFAULT:
#ifdef JAPAN            //  added  by Hiraisi (BUG#1389)
         Num = (fEnglish == 0)?  ToNND(127,100): ToNND(1,2);
#else
         Num = (iMeasure == 0)?  ToNND(127,100): ToNND(1,2);
#endif
         SetDlgItemNum(hDlg, IDLEFT  , Num, TRUE);
         SetDlgItemNum(hDlg, IDTOP   , Num, TRUE);
         SetDlgItemNum(hDlg, IDRIGHT , Num, TRUE);
         SetDlgItemNum(hDlg, IDBOTTOM, Num, TRUE);
         break;

      case IDOK:
         /* read header and footer */
         GetDlgItemText(hDlg, IDHEADER, szHeader, 79);
         GetDlgItemText(hDlg, IDFOOTER, szFooter, 79);

#ifdef SAVEHEADFOOT
         /* write out the profile strings */
         WriteProfileString(winIniAppName, TEXT("header"), szHeader);
         WriteProfileString(winIniAppName, TEXT("footer"), szFooter);
#endif

         /* get margins */
         if(!GetDlgItemNum(hDlg, IDLEFT  , &tnmLeft) ||
               !GetDlgItemNum(hDlg, IDTOP    , &tnmTop) ||
               !GetDlgItemNum(hDlg, IDRIGHT  , &tnmRight) ||
               !GetDlgItemNum(hDlg, IDBOTTOM , &tnmBottom)) {
            PbrushOkError(IDSInvalidNumb, MB_ICONHAND);
            break;
         }
         if(!ValidateMargins(tnmLeft, tnmTop, tnmRight, tnmBottom))
            break;
#ifdef JAPAN            //  added  by Hiraisi (BUG#1389)
         if (fEnglish == 0) /* if Metric(i.e. in CMs), convert to inches */
#else
         if (iMeasure == 0) /* if Metric(i.e. in CMs), convert to inches */
#endif
         {
             tnmLeft = CMToInches(tnmLeft);
             tnmRight = CMToInches(tnmRight);
             tnmTop = CMToInches(tnmTop);
             tnmBottom = CMToInches(tnmBottom);
         }
         nmLeft = tnmLeft;
         nmTop = tnmTop;
         nmRight = tnmRight;
         nmBottom = tnmBottom;
         EndDialog(hDlg, TRUE);
         break;

      case IDCANCEL:
         EndDialog(hDlg, FALSE);
         break;
      }
      break;

   case WM_INITDIALOG:
#ifdef JAPAN            //  added  by Hiraisi (BUG#1389)
      fEnglish = iMeasure;
#endif
      CenterWindow(hDlg);
      oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
      GetPrintParms(NULL);
      if(FirstPrint) {
         wcopies = 1;
         wamount = IDWHOLE;
         wquality = IDPROOF;
         SendMessage(hDlg, WM_COMMAND, IDDEFAULT, 0L);
         FirstPrint = FALSE;
      } else {
         SetDlgItemNum(hDlg, IDLEFT  , (iMeasure == 0)? InchesToCM(nmLeft): nmLeft, TRUE);
         SetDlgItemNum(hDlg, IDTOP   , (iMeasure == 0)? InchesToCM(nmTop): nmTop, TRUE);
         SetDlgItemNum(hDlg, IDRIGHT , (iMeasure == 0)? InchesToCM(nmRight): nmRight, TRUE);
         SetDlgItemNum(hDlg, IDBOTTOM, (iMeasure == 0)? InchesToCM(nmBottom): nmBottom, TRUE);
      }

#ifdef JAPAN   // added by Hiraisi(BUG#4327)
      SendDlgItemMessage(hDlg, IDHEADER, EM_LIMITTEXT, 78, 0L);
      SendDlgItemMessage(hDlg, IDFOOTER, EM_LIMITTEXT, 78, 0L);
#endif
      SetDlgItemText(hDlg, IDHEADER, szHeader);
      SetDlgItemText(hDlg, IDFOOTER, szFooter);

#ifdef JAPAN // Change caption of Page settings layout by intl
      LoadString(hInst, (iMeasure ? IDS_SPACEISINCH : IDS_SPACEISCENTI),
                 szSpaceText, CharSizeOf(szSpaceText));
      SetDlgItemText(hDlg, IDSPACE, szSpaceText);
#endif

      SetCursor(oldcsr);
      break;
   default:
      return FALSE;
   }

   return TRUE;
}

