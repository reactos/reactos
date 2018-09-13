/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
#define  NOVIRTUALKEYCODES TRUE
#define  NOICONS	         TRUE
#define  NOKEYSTATES       TRUE
#define  NOSYSCOMMANDS     TRUE
#define  NOATOM	         TRUE
#define  NOCLIPBOARD       TRUE
#define  NODRAWTEXT	      TRUE
#define  NOMINMAX	         TRUE
#define  NOOPENFILE	      TRUE
#define  NOSCROLL	         TRUE
#define  NOHELP            TRUE
#define  NOPROFILER	      TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31

#include <windows.h>
#include <port1632.h>
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"                            /* mbbx 1.04: using taskState.string */
#include "connect.h"
#include <ctype.h>    /* adding for prototype of isalpha() -sdj*/

#define SAVEPARAMS                  struct saveParams    /* mbbx 1.04 ... */
#define PSAVEPARAMS                 SAVEPARAMS *

struct saveParams
{
   HANDLE            hData;
   recTrmParams FAR  *lpData;
   INT               saveLevel;
};


/*---------------------------------------------------------------------------*/
/* doSettings() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL doSettings(INT      dbResID, FARPROC  dbProc)
{
   BOOL     doSettings = FALSE;
   FARPROC  lpProc;
   BYTE     work[STR255];

   lpProc = MakeProcInstance((FARPROC) dbProc,  hInst);
   doSettings = DialogBox(hInst, MAKEINTRESOURCE(dbResID), hItWnd, (DLGPROC)lpProc);
   FreeProcInstance(lpProc);

   if(doSettings == -1)
   {
      LoadString(hInst, STR_OUTOFMEMORY, (LPSTR) work, STR255-1);
      testMsg(work,NULL,NULL);
   }

   return(doSettings);
}


/*---------------------------------------------------------------------------*/
/* initDlgPos() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

VOID initDlgPos(HWND hDlg)
{
   RECT  dialogRect, windowRect;


   GetWindowRect(hDlg, (LPRECT) &dialogRect);
   dialogRect.right -= dialogRect.left;
   dialogRect.bottom -= dialogRect.top;

   GetWindowRect(hItWnd, (LPRECT) &windowRect);
   windowRect.left += (((windowRect.right - windowRect.left) - dialogRect.right) / 2);
   windowRect.top += (((windowRect.bottom - windowRect.top) - dialogRect.bottom) / 2);

   if(windowRect.left < 10)
      windowRect.left = 10;
   else if((windowRect.left + dialogRect.right) > (GetSystemMetrics(SM_CXSCREEN) - 10))
      windowRect.left = (GetSystemMetrics(SM_CXSCREEN) - 10) - dialogRect.right;

   if(windowRect.top < 10)
      windowRect.top = 10;
   else if((windowRect.top + dialogRect.bottom) > (GetSystemMetrics(SM_CYSCREEN) - 10))
      windowRect.top = (GetSystemMetrics(SM_CYSCREEN) - 10) - dialogRect.bottom;

   MoveWindow(hDlg, windowRect.left, windowRect.top, dialogRect.right, dialogRect.bottom, FALSE);
}


/*---------------------------------------------------------------------------*/
/* loadListData() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR loadListData(HWND  hDlg, INT   nItem, INT   nList)
{
   INT   ndx;
   BYTE  work[STR255];

   for(ndx = 0; LoadString(hInst, nList + ndx, (LPSTR) work, STR255-1); ndx += 1)
      SendDlgItemMessage(hDlg, nItem, LB_INSERTSTRING, -1, (LONG) ((LPSTR) work));
}


/*---------------------------------------------------------------------------*/
/* dbPhon() -                                                                */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbPhon(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   BYTE szPhone[((DCS_MODEMCMDSZ-1)*2)+1];
   BYTE work[255]; /* jtf 3.20 */
   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      SendDlgItemMessage(hDlg, ITMPHONENUMBER, EM_LIMITTEXT, sizeof(szPhone)-1, 0L);  /* mbbx 2.00 */
      strcpy(szPhone,trmParams.phone);
      strcpy(szPhone+strlen(szPhone),trmParams.phone2);
      SetDlgItemText(hDlg, ITMPHONENUMBER, (LPSTR) szPhone);

      SetDlgItemInt(hDlg, ITMRETRYTIME, trmParams.dlyRetry, FALSE);
      CheckDlgButton(hDlg, ITMRETRY, trmParams.flgRetry);
      CheckDlgButton(hDlg, ITMSIGNAL, trmParams.flgSignal);
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         GetDlgItemText(hDlg, ITMPHONENUMBER, (LPSTR) szPhone, sizeof(szPhone));
         strncpy(trmParams.phone, szPhone, DCS_MODEMCMDSZ);
         trmParams.phone[DCS_MODEMCMDSZ-1] = 0;
         strcpy(trmParams.phone2, szPhone+strlen(trmParams.phone));

         if((trmParams.dlyRetry = GetDlgItemInt(hDlg, ITMRETRYTIME, NULL, FALSE)) < 30)
            { /* jtf 3.20 */
            LoadString(hInst, STR_MINTIME, (LPSTR) work, STR255-1);
            testMsg(work,NULL,NULL);
            trmParams.dlyRetry = 30;
            }
         trmParams.flgRetry = IsDlgButtonChecked(hDlg, ITMRETRY);
         trmParams.flgSignal = IsDlgButtonChecked(hDlg, ITMSIGNAL);
         termData.flags |= TF_CHANGED;
         break;

      case IDCANCEL:
         break;

      case ITMRETRY:
      case ITMSIGNAL:
         CheckDlgButton(hDlg, GET_WM_COMMAND_ID(wParam, lParam), !IsDlgButtonChecked(hDlg, GET_WM_COMMAND_ID(wParam, lParam)));
         return(TRUE);

      default:
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/*  dbEmul() -                                                               */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbEmul(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      chkGrpButton(hDlg, ITMTERMFIRST, ITMTERMLAST, (trmParams.emulate != ITMDELTA) ? trmParams.emulate : ITMTTY);
      initDlgPos(hDlg);
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         if((wParam = whichGrpButton(hDlg, ITMTERMFIRST, ITMTERMLAST)) != trmParams.emulate)
         {
            trmParams.emulate = wParam;
            resetEmul();
         }
         termData.flags |= TF_CHANGED;
         break;

      case IDCANCEL:
         break;

      default:
         chkGrpButton(hDlg, ITMTERMFIRST, ITMTERMLAST, GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* dbTerm() - Terminal Settings dialog control.                        [mbb] */
/*---------------------------------------------------------------------------*/

/* mbbx 2.00: all new font enumeration code... */

int APIENTRY nextFontName(LPLOGFONT lpLogFont, LPTEXTMETRIC lpTextMetrics, 
                          DWORD          FontType, PHANDLE          lpData)

{
   BYTE testStr[40];
   BYTE testStr1[40];

   if (lpLogFont->lfPitchAndFamily & FIXED_PITCH)  /* jtf 3.21 */
#ifdef ORGCODE
      SendDlgItemMessage((HWND) LOWORD((LONG) lpData), ITMFONTFACE, LB_ADDSTRING, 0, (LONG) lpLogFont->lfFaceName);
#else
      SendDlgItemMessage((HWND)*lpData, ITMFONTFACE, LB_ADDSTRING, 0, (LONG) lpLogFont->lfFaceName);
#endif

   lstrcpy( (LPSTR) testStr, (LPSTR) lpLogFont->lfFaceName); /* jtf 3.21 */
   LoadString(hInst, STR_INI_FONTFACE, (LPSTR) testStr1, LF_FACESIZE);
   /* changed from strcmpi -sdj */
   if (lstrcmpi(testStr,testStr1) == 0)
#ifdef ORGCODE
      SendDlgItemMessage((HWND) LOWORD((LONG) lpData), ITMFONTFACE, LB_ADDSTRING, 0, (LONG) lpLogFont->lfFaceName);
#else
      SendDlgItemMessage((HWND) *lpData, ITMFONTFACE, LB_ADDSTRING, 0, (LONG) lpLogFont->lfFaceName);
#endif
   return(TRUE);
}


VOID NEAR loadFontNames(HWND  hDlg)
{
   FARPROC  lpFontProc;

   getPort();
   lpFontProc = MakeProcInstance((FARPROC) nextFontName, hInst);
#ifdef ORGCODE
   EnumFonts(thePort, NULL, lpFontProc, (LPSTR) ((LONG) hDlg));
#else
   EnumFonts(thePort, NULL, lpFontProc, (LPARAM) &hDlg);
#endif
   FreeProcInstance(lpFontProc);
   releasePort();

   SendDlgItemMessage(hDlg, ITMFONTFACE, LB_SELECTSTRING, -1, (LONG) ((LPSTR) trmParams.fontFace));
}


VOID NEAR loadFontSizes(HWND  hDlg, BYTE  *faceName, INT   fontSize)
{
   BYTE  sizeList[64], work[16];
   INT   ndx, ndx1;

   listFontSizes(faceName, sizeList, 64-1);

   ndx = -1;
   for(ndx1 = 7; ndx1 < 64; ndx1 += 1)
      if(sizeList[ndx1] != 0)
      {
         sprintf(work, "%d", ndx1);
         SendDlgItemMessage(hDlg, ITMFONTSIZE, LB_INSERTSTRING, -1, (LONG) ((LPSTR) work));

         if((ndx1 <= fontSize) || (ndx == -1))
            ndx += 1;
      }

   SendDlgItemMessage(hDlg, ITMFONTSIZE, LB_SETCURSEL, ndx, 0L);
}


INT NEAR getFontSize(HWND hDlg)                   /* mbbx 2.00: font selection... */
{
   INT   ndx;
   BYTE  work[16];

   ndx = SendDlgItemMessage(hDlg, ITMFONTSIZE, LB_GETCURSEL, 0, 0L);
   SendDlgItemMessage(hDlg, ITMFONTSIZE, LB_GETTEXT, ndx, (LONG) ((LPSTR) work));
   sscanf(work, "%d", &ndx);

   return(ndx);
}

BOOL  APIENTRY dbTerm(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   WORD  tempWord;
   BOOL  tempBool;

   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      if((((PSAVEPARAMS) taskState.string)->hData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(recTrmParams))) == NULL)
         break;
      ((PSAVEPARAMS) taskState.string)->lpData = (recTrmParams FAR *) GlobalLock(((PSAVEPARAMS) taskState.string)->hData);
      *(((PSAVEPARAMS) taskState.string)->lpData) = trmParams;

      CheckDlgButton(hDlg,   ITMLINEWRAP,    trmParams.lineWrap);
      CheckDlgButton(hDlg,   ITMLOCALECHO,   trmParams.localEcho);
      CheckDlgButton(hDlg,   ITMSOUND,       trmParams.sound);
      CheckDlgButton(hDlg,   ITMINCRLF,      trmParams.inpCRLF);
      CheckDlgButton(hDlg,   ITMOUTCRLF,     trmParams.outCRLF);
      CheckRadioButton(hDlg, ITM80COL,       ITM132COL, trmParams.columns);
      CheckRadioButton(hDlg, ITMBLKCURSOR,   ITMUNDCURSOR, trmParams.termCursor);
      CheckDlgButton(hDlg,   ITMBLINKCURSOR, trmParams.cursorBlink);

      loadFontNames(hDlg);                   /* mbbx 2.00: font selection... */
      loadFontSizes(hDlg, trmParams.fontFace, trmParams.fontSize);

      ((PSAVEPARAMS) taskState.string)->saveLevel = -1;

      loadListData(hDlg, ITMTRANSLATE, STR_ICS_NAME);
      SendDlgItemMessage(hDlg, ITMTRANSLATE, LB_SETCURSEL, trmParams.language, 0L);

      SetDlgItemInt(hDlg, ITMBUFFERLINES, trmParams.bufferLines = maxLines, FALSE); /* mbbx 2.00.03 ... */

      CheckDlgButton(hDlg, ITMSCROLLBARS, !trmParams.fHideTermVSB);
      CheckDlgButton(hDlg, ITMIBMXANSI, trmParams.setIBMXANSI); /* rjs swat */
      CheckDlgButton(hDlg, ITMWINCTRL,  trmParams.useWinCtrl);  /* rjs swat */

      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
                                             /* mbbx 2.00.03 ... */
/* rjs bug2 002 */
         tempWord = GetDlgItemInt(hDlg, ITMBUFFERLINES, (BOOL FAR *)&tempBool, FALSE);

         if(tempBool)
         {
            if(tempWord > 399)
            {
               MessageBeep(0);
               lParam = 0xFFFF0000;
               SendDlgItemMessage(hDlg, ITMBUFFERLINES, EM_SETSEL, GET_EM_SETSEL_MPS( 0, lParam));
               return(TRUE);
            }
         }
         else
         {
               MessageBeep(0);
               lParam = 0xFFFF0000;
               SendDlgItemMessage(hDlg, ITMBUFFERLINES, EM_SETSEL, GET_EM_SETSEL_MPS(0, lParam));
               return(TRUE);
         }
/* end rjs bug2 002 */

         if(tempWord != trmParams.bufferLines)
            trmParams.bufferLines = tempWord;

         resetTermBuffer();

         icsResetTable((WORD) SendDlgItemMessage(hDlg, ITMTRANSLATE, LB_GETCURSEL, 0, 0L));

         if(((PSAVEPARAMS) taskState.string)->saveLevel != -1)    /* mbbx 2.00: font selection... */
            buildTermFont();

         updateTermScrollBars(FALSE);

         if(!IsIconic(hItWnd))/* rjs bugs 015 */
            sizeTerm(0L);

         termData.flags |= TF_CHANGED;

         // clears the hTermWnd make sure there's no garbage left
         // when font is changed.
         InvalidateRect (hTermWnd, NULL, TRUE);
         UpdateWindow (hTermWnd);
         break;

      case IDCANCEL:
         trmParams = *(((PSAVEPARAMS) taskState.string)->lpData);
         break;

      case ITMIBMXANSI:
         CheckDlgButton(hDlg, ITMIBMXANSI, trmParams.setIBMXANSI = !trmParams.setIBMXANSI);
         return(TRUE);
      case ITMLINEWRAP:
         CheckDlgButton(hDlg, ITMLINEWRAP, trmParams.lineWrap = !trmParams.lineWrap);
         return(TRUE);
      case ITMLOCALECHO:
         CheckDlgButton(hDlg, ITMLOCALECHO, trmParams.localEcho = !trmParams.localEcho);
         return(TRUE);
      case ITMSOUND:
         CheckDlgButton(hDlg, ITMSOUND, trmParams.sound = !trmParams.sound);
         return(TRUE);

      case ITMINCRLF:
         CheckDlgButton(hDlg, ITMINCRLF, trmParams.inpCRLF = !trmParams.inpCRLF);
         return(TRUE);
      case ITMOUTCRLF:
         CheckDlgButton(hDlg, ITMOUTCRLF, trmParams.outCRLF = !trmParams.outCRLF);
         return(TRUE);

      case ITM80COL:
      case ITM132COL:
         CheckRadioButton(hDlg, ITM80COL, ITM132COL, trmParams.columns = wParam);
         return(TRUE);

      case ITMBLKCURSOR:
      case ITMUNDCURSOR:
         CheckRadioButton(hDlg, ITMBLKCURSOR, ITMUNDCURSOR, trmParams.termCursor = wParam);
         return(TRUE);
      case ITMBLINKCURSOR:
         CheckDlgButton(hDlg, ITMBLINKCURSOR, trmParams.cursorBlink = !trmParams.cursorBlink);
         return(TRUE);

      case ITMFONTFACE:
// -sdj 10/09 LBN_ HAS CHANGED IN WIN32, have to take care of this in spec.
#ifdef ORGCODE
         if(HIWORD(lParam) == LBN_SELCHANGE)    /* mbbx 2.00: font selection... */
#else
         if(HIWORD(wParam) == LBN_SELCHANGE)    /* mbbx 2.00: font selection... */
#endif
         {
            SendDlgItemMessage(hDlg, ITMFONTFACE, LB_GETTEXT, SendDlgItemMessage(hDlg, ITMFONTFACE, LB_GETCURSEL, 0, 0L), 
                               (LONG) ((LPSTR) trmParams.fontFace));

            SendDlgItemMessage(hDlg, ITMFONTSIZE, LB_RESETCONTENT, 0, 0L);
            loadFontSizes(hDlg, trmParams.fontFace, trmParams.fontSize);
         }
      case ITMFONTSIZE:

// -sdj 10/09 LBN_ HAS CHANGED IN WIN32, have to take care of this in spec.
#ifdef ORGCODE
         if(HIWORD(lParam) == LBN_SELCHANGE)    /* mbbx 2.00: font selection... */
#else
         if(HIWORD(wParam) == LBN_SELCHANGE)    /* mbbx 2.00: font selection... */
#endif
         {
            trmParams.fontSize = getFontSize(hDlg);
            ((PSAVEPARAMS) taskState.string)->saveLevel = 0;
         }
         return(TRUE);

      case ITMTRANSLATE:
      case ITMBUFFERLINES:
         return(TRUE);

      case ITMSCROLLBARS:
         CheckDlgButton(hDlg, ITMSCROLLBARS, !(trmParams.fHideTermVSB = !trmParams.fHideTermVSB));
         trmParams.fHideTermHSB = trmParams.fHideTermVSB;
         return(TRUE);

      case ITMWINCTRL:
         CheckDlgButton(hDlg, ITMWINCTRL, trmParams.useWinCtrl = !trmParams.useWinCtrl);
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   if(((PSAVEPARAMS) taskState.string)->hData != NULL)
   {
      GlobalUnlock(((PSAVEPARAMS) taskState.string)->hData);
      GlobalFree(((PSAVEPARAMS) taskState.string)->hData);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* dbFkey() - Dialog box control for function keys.                          */
/*---------------------------------------------------------------------------*/

VOID NEAR setDlgFkeys(HWND  hDlg, INT   level)
{
   INT   key;

   for(key = 0; key < DCS_NUMFKEYS; key += 1)
   {
      SetDlgItemText(hDlg, ITMFKEYTITLE+(key+1), (LPSTR) trmParams.fKeyTitle[level-1][key]); /* rkhx 2.00 */
      SetDlgItemText(hDlg, ITMFKEYTEXT+(key+1), (LPSTR) trmParams.fKeyText[level-1][key]);
   }
}


VOID NEAR getDlgFkeys(HWND  hDlg, INT   level)
{
   INT   key;

   for(key = 0; key < DCS_NUMFKEYS; key += 1)
   {
      GetDlgItemText(hDlg, ITMFKEYTITLE+(key+1), (LPSTR) trmParams.fKeyTitle[level-1][key], DCS_FKEYTITLESZ-2); /* rkhx 2.00 */
      GetDlgItemText(hDlg, ITMFKEYTEXT+(key+1), (LPSTR) trmParams.fKeyText[level-1][key], DCS_FKEYTEXTSZ-2); /* jtf 3.12 */
   }
}


BOOL  APIENTRY dbFkey(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      if((((PSAVEPARAMS) taskState.string)->hData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(recTrmParams))) == NULL)
         break;
      ((PSAVEPARAMS) taskState.string)->lpData = (recTrmParams FAR *) GlobalLock(((PSAVEPARAMS) taskState.string)->hData);
      *(((PSAVEPARAMS) taskState.string)->lpData) = trmParams;
      ((PSAVEPARAMS) taskState.string)->saveLevel = curLevel;


      for(wParam = 1; wParam <= DCS_NUMFKEYS; wParam += 1)
      {
         SendDlgItemMessage(hDlg, ITMFKEYTITLE + wParam, EM_LIMITTEXT, DCS_FKEYTITLESZ-2, 0L);
         SendDlgItemMessage(hDlg, ITMFKEYTEXT + wParam, EM_LIMITTEXT, DCS_FKEYTEXTSZ-3, 0L); /* jtf 3.12 */
      }
      setDlgFkeys(hDlg, curLevel);

      CheckRadioButton(hDlg, ITMLEVEL1, ITMLEVEL4, ITMLEVEL1+(curLevel-1));

      CheckDlgButton(hDlg, ITMSHOWFKEYS, trmParams.environmentFlags & DCS_EVF_FKEYSSHOW);    /* mbbx 2.00: show fkeys... */
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         getDlgFkeys(hDlg, curLevel);

         setFKeyLevel(curLevel, TRUE);       /* mbbx 2.00 */

         if(IsDlgButtonChecked(hDlg, ITMSHOWFKEYS))   /* mbbx 2.00: show fkeys... */
         {
            if(!(trmParams.environmentFlags & DCS_EVF_FKEYSSHOW))
            {
               showHidedbmyControls(TRUE, TRUE);
               trmParams.environmentFlags |= DCS_EVF_FKEYSSHOW;
            }
         }
         else
	    trmParams.environmentFlags &= ~DCS_EVF_FKEYSSHOW;

// -sdj: 02jun92 when keys visible is ticked and ok pressed
//	 IsDlgButtonChecked(hDlg, AUTOARRANGE ) is called, but
//	 AUTOARRANGE is not present in the rc file, NT doesnt like
//	 this any more! If this flag is important then we need to
//	 define this some place to avoid Unknown control Id error
//	 from USER.

#ifdef BUGBYPASS
#else
         if(IsDlgButtonChecked(hDlg, ITMAUTOARRANGE))
            trmParams.environmentFlags |= DCS_EVF_FKEYSARRANGE;
         else
	    trmParams.environmentFlags &= ~DCS_EVF_FKEYSARRANGE;
#endif

         termData.flags |= TF_CHANGED;
         break;

      case IDCANCEL:
         trmParams = *(((PSAVEPARAMS) taskState.string)->lpData);
         curLevel = ((PSAVEPARAMS) taskState.string)->saveLevel;
         break;

      case ITMLEVEL1:
      case ITMLEVEL2:
      case ITMLEVEL3:
      case ITMLEVEL4:
         CheckRadioButton(hDlg, ITMLEVEL1, ITMLEVEL4, GET_WM_COMMAND_ID(wParam, lParam));
         getDlgFkeys(hDlg, curLevel);
         setDlgFkeys(hDlg, curLevel = wParam-(ITMLEVEL1-1));
         return(TRUE);

      case ITMSHOWFKEYS:                     /* mbbx 2.00 ... */
         CheckDlgButton(hDlg, GET_WM_COMMAND_ID(wParam, lParam), !IsDlgButtonChecked(hDlg, GET_WM_COMMAND_ID(wParam, lParam)));
         return(TRUE);

      default:
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   if(((PSAVEPARAMS) taskState.string)->hData != NULL)
   {
      GlobalUnlock(((PSAVEPARAMS) taskState.string)->hData);
      GlobalFree(((PSAVEPARAMS) taskState.string)->hData);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* dbTxtX() - Control for Text transfer dialogue box.                        */
/*---------------------------------------------------------------------------*/

VOID NEAR enableStdItems(HWND  hDlg, BOOL  bEnable)
{
   ShowWindow(GetDlgItem(hDlg, ITMSTDGRP), bEnable ? SW_SHOWNOACTIVATE : SW_HIDE);
   ShowWindow(GetDlgItem(hDlg, ITMSTDXON), bEnable && (trmParams.flowControl == ITMXONFLOW) ? SW_SHOWNOACTIVATE : SW_HIDE);
   ShowWindow(GetDlgItem(hDlg, ITMSTDHARD), bEnable && (trmParams.flowControl == ITMHARDFLOW) ? SW_SHOWNOACTIVATE : SW_HIDE);
   ShowWindow(GetDlgItem(hDlg, ITMSTDNONE), bEnable && (trmParams.flowControl == ITMNOFLOW) ? SW_SHOWNOACTIVATE : SW_HIDE);
   UpdateWindow(hDlg);     /* rjs bugs 001 */
}


VOID NEAR enableChrItems(HWND  hDlg, BOOL  bEnable)
{
   INT   nCmdShow, nResID;

   nCmdShow = bEnable ? SW_SHOWNOACTIVATE : SW_HIDE;
   for(nResID = ITMCHRGRP; nResID <= ITMCHRUNITS; nResID += 1)
      ShowWindow(GetDlgItem(hDlg, nResID), nCmdShow);

   if(bEnable)
      EnableWindow(GetDlgItem(hDlg, ITMCHRTIME), trmParams.xChrType == ITMCHRDELAY);

   UpdateWindow(hDlg);     /* rjs bugs 001 */
}


VOID NEAR enableLinItems(HWND  hDlg, BOOL  bEnable)
{
   INT   nCmdShow, nResID;

   nCmdShow = bEnable ? SW_SHOWNOACTIVATE : SW_HIDE;
   for(nResID = ITMLINGRP; nResID <= ITMLINSTR; nResID += 1)
      ShowWindow(GetDlgItem(hDlg, nResID), nCmdShow);

   if(bEnable)
   {
      EnableWindow(GetDlgItem(hDlg, ITMLINTIME), trmParams.xLinType == ITMLINDELAY);
      EnableWindow(GetDlgItem(hDlg, ITMLINSTR), trmParams.xLinType == ITMLINWAIT);
   }

   UpdateWindow(hDlg);     /* rjs bugs 001 */
}


BOOL  APIENTRY dbTxtX(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   DWORD  temp;
   BYTE  tempStr[16];

   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      if((((PSAVEPARAMS) taskState.string)->hData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(recTrmParams))) == NULL)
         break;
      ((PSAVEPARAMS) taskState.string)->lpData = (recTrmParams FAR *) GlobalLock(((PSAVEPARAMS) taskState.string)->hData);
      *(((PSAVEPARAMS) taskState.string)->lpData) = trmParams;

      CheckRadioButton(hDlg, ITMSTD, ITMLIN, trmParams.xTxtType);
      enableStdItems(hDlg, trmParams.xTxtType == ITMSTD);
      enableChrItems(hDlg, trmParams.xTxtType == ITMCHR);
      enableLinItems(hDlg, trmParams.xTxtType == ITMLIN);

      CheckRadioButton(hDlg, ITMCHRDELAY, ITMCHRWAIT, trmParams.xChrType);
      SetDlgItemInt(hDlg, ITMCHRTIME, trmParams.xChrDelay, FALSE);

      CheckRadioButton(hDlg, ITMLINDELAY, ITMLINWAIT, trmParams.xLinType);
      SetDlgItemInt(hDlg, ITMLINTIME, trmParams.xLinDelay, FALSE);
      SendDlgItemMessage(hDlg, ITMLINSTR, EM_LIMITTEXT, DCS_XLINSTRSZ-1, 0L); /* rkhx 2.00 */
      SetDlgItemText(hDlg, ITMLINSTR, (LPSTR) trmParams.xLinStr);

      CheckDlgButton(hDlg, ITMWORDWRAP, trmParams.xWordWrap);
      EnableWindow(GetDlgItem(hDlg, ITMWRAPCOL), trmParams.xWordWrap);
      SetDlgItemInt(hDlg, ITMWRAPCOL, trmParams.xWrapCol, FALSE);
      initDlgPos(hDlg);
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         trmParams.xTxtType = whichGrpButton(hDlg, ITMSTD, ITMLIN);
#ifdef OLDCODE       /* rjs bugs 007 */
         trmParams.xChrDelay = GetDlgItemInt(hDlg, ITMCHRTIME, NULL, FALSE);
         trmParams.xLinDelay = GetDlgItemInt(hDlg, ITMLINTIME, NULL, FALSE);
         trmParams.xWrapCol  = GetDlgItemInt(hDlg, ITMWRAPCOL, NULL, FALSE);
#endif
         GetDlgItemText(hDlg, ITMLINSTR, (LPSTR) trmParams.xLinStr, DCS_XLINSTRSZ-1); /* rkhx 2.00 */

 /* rjs bugs 007 */
         GetDlgItemText(hDlg, ITMCHRTIME, (LPSTR) tempStr, DCS_XLINSTRSZ-1); /* rjs bugs 007 */
         temp = 0;
         while(tempStr[temp])
            if(isalpha(tempStr[temp++]))
            {
               MessageBeep(0);
               return(TRUE);
            }
         sscanf(tempStr, "%u", &temp);
         if(temp > 255)
         {
            MessageBeep(0);
            return(TRUE);
         }
         trmParams.xChrDelay = temp;

         GetDlgItemText(hDlg, ITMLINTIME, (LPSTR) tempStr, DCS_XLINSTRSZ-1); /* rjs bugs 007 */
         temp = 0;
         while(tempStr[temp])
            if(isalpha(tempStr[temp++]))
            {
               MessageBeep(0);
               return(TRUE);
            }
         sscanf(tempStr, "%u", &temp);
         if(temp > 255)
         {
            MessageBeep(0);
            return(TRUE);
         }
         trmParams.xLinDelay = temp;

         GetDlgItemText(hDlg, ITMWRAPCOL, (LPSTR) tempStr, DCS_XLINSTRSZ-1); /* rjs bugs 007 */
         temp = 0;
         while(tempStr[temp])
            if(isalpha(tempStr[temp++]))
            {
               MessageBeep(0);
               return(TRUE);
            }
         sscanf(tempStr, "%u", &temp);
         if(temp > 255)
         {
            MessageBeep(0);
            return(TRUE);
         }

         trmParams.xWrapCol = temp;
         termData.flags |= TF_CHANGED;
         break;

      case IDCANCEL:
         trmParams = *(((PSAVEPARAMS) taskState.string)->lpData);
         break;

      case ITMSTD:
      case ITMCHR:
      case ITMLIN:
            CheckRadioButton(hDlg, ITMSTD, ITMLIN, trmParams.xTxtType = GET_WM_COMMAND_ID(wParam, lParaqm));
            enableLinItems(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == ITMLIN);
            enableChrItems(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == ITMCHR);
            enableStdItems(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == ITMSTD);
            return(TRUE);

      case ITMCHRDELAY:
      case ITMCHRWAIT:
         if(trmParams.xTxtType == ITMCHR)
         {
            CheckRadioButton(hDlg, ITMCHRDELAY, ITMCHRWAIT, trmParams.xChrType = GET_WM_COMMAND_ID(wParam, lParam));
            enableChrItems(hDlg, TRUE);
         }
         return(TRUE);

      case ITMLINDELAY:
      case ITMLINWAIT:
         if(trmParams.xTxtType == ITMLIN)
         {
            CheckRadioButton(hDlg, ITMLINDELAY, ITMLINWAIT, trmParams.xLinType = GET_WM_COMMAND_ID(wParam, lParam));
            enableLinItems(hDlg, TRUE);
         }
         return(TRUE);

      case ITMWORDWRAP:
         CheckDlgButton(hDlg, ITMWORDWRAP, trmParams.xWordWrap = !trmParams.xWordWrap);
         EnableWindow(GetDlgItem(hDlg, ITMWRAPCOL), trmParams.xWordWrap);
         if(trmParams.xWordWrap)
         {
            SetFocus(GetDlgItem(hDlg, ITMWRAPCOL));
            SendDlgItemMessage(hDlg, ITMWRAPCOL, EM_SETSEL, GET_EM_SETSEL_MPS(0, MAKELONG(0, 0x7FFF)));
         }
         return(TRUE);

      default:
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   if(((PSAVEPARAMS) taskState.string)->hData != NULL)
   {
      GlobalUnlock(((PSAVEPARAMS) taskState.string)->hData);
      GlobalFree(((PSAVEPARAMS) taskState.string)->hData);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* dbBinX() - Dialogue box control for Binary File Transfers.                */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbBinX(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      chkGrpButton(hDlg, ITMKERMIT, ITMXMODEM, trmParams.xBinType);
      initDlgPos(hDlg);
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
//	  trmParams.xBinType = whichGrpButton(hDlg, ITMDYNACOMM, ITMYTERM);
// change this to kermit,xmodem, to avoid controlid unknown error from user
// -sdj 02jun92
	  trmParams.xBinType = whichGrpButton(hDlg, ITMKERMIT, ITMXMODEM);
         termData.flags |= TF_CHANGED;
         break;

      case IDCANCEL:
         break;

      default:
// change this to kermit,xmodem, to avoid control id unknown error from user
// -sdj 02jun92
//	  chkGrpButton(hDlg, ITMDYNACOMM, ITMYTERM, GET_WM_COMMAND_ID(wParam, lParam));
	  chkGrpButton(hDlg, ITMKERMIT, ITMXMODEM, GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}

/*---------------------------------------------------------------------------*/
/* dbComm() -                                                          [mbb] */
/*---------------------------------------------------------------------------*/

#ifdef ORGCODE
BYTE  comDevList[5];
#else
BYTE  comDevList[20];
#endif


VOID initComDevSelect(HWND hDlg,WORD  wListID,BOOL bInit)  /* mbbx 2.01.09 ... */
{
   BYTE  str1[8], str2[8], str3[16];
   INT   ndx;

   loadListData(hDlg, wListID, STR_COM_CONNECT);

   LoadString(hInst, STR_COM_CONNECT+2, (LPSTR) str1, sizeof(str1));
   comDevList[0] = 0;
   for(ndx = 1; ndx <= MaxComPortNumberInMenu; ndx += 1)
   {
      //sprintf(str2, str1, ndx);
      //if(GetProfileString((LPSTR) "ports", (LPSTR) str2, (LPSTR) NULL_STR, (LPSTR) str3, sizeof(str3)))

      if (TRUE)
      {
	 SendDlgItemMessage(hDlg, wListID, LB_INSERTSTRING, -1, (LONG) ((LPSTR) arComNumAndName[ndx].PortName));
         comDevList[++comDevList[0]] = ndx;
      }
   }

#ifdef ORGCODE
#else

   comDevList[(comDevList[0])+1] = ndx;

   //MaxComPortNumberInMenu	 = ndx-1;

   SendDlgItemMessage(hDlg, wListID, LB_INSERTSTRING, -1, (LONG)((LPSTR)"TELNET"));

#endif



   if(bInit)
   {
      ndx = 0;
   }
   else
   {
      loadListData(hDlg, wListID, STR_COM_CONNECT+4);

      if(trmParams.comDevRef < ITMNETCOM)
      {
         for(ndx = comDevList[0]; ndx > 0; ndx -= 1)
            if(comDevList[ndx] == trmParams.comPortRef)
               break;
      }
      else
         ndx = trmParams.comDevRef + (comDevList[0] - 1);
   }

   SendDlgItemMessage(hDlg, wListID, LB_SETCURSEL, ndx, 0L);
}


BYTE getComDevSelect(HWND hDlg, WORD wListID, BYTE *newDevRef)  /* mbbx 2.01.09 ... */
{
   BYTE  comPortRef = 0;

   if((*newDevRef = SendDlgItemMessage(hDlg, wListID, LB_GETCURSEL, 0, 0L)) != (BYTE) -1)
   {
#ifdef ORGCODE
      if(*newDevRef <= comDevList[0])
#else
      if(*newDevRef <= (comDevList[0])+1)
#endif
      {
         if(*newDevRef > ITMNOCOM)
         {
            comPortRef = comDevList[*newDevRef];
            *newDevRef = ITMWINCOM;
         }
      }
      else
         *newDevRef -= (comDevList[0] - 1);
   }
   else
      *newDevRef = ITMNOCOM;

   return(comPortRef);
}


/*---------------------------------------------------------------------------*/

VOID NEAR enableGrpItems(HWND  hDlg, INT   itemFirst, INT   itemLast, INT   itemSelect)
{
   INT   itemNext;

   CheckRadioButton(hDlg, itemFirst, itemLast, itemSelect);
   for(itemNext = itemFirst; itemNext <= itemLast; itemNext += 1)
      EnableWindow(GetDlgItem(hDlg, itemNext), itemSelect);
}


VOID NEAR enableCommItems(HWND  hDlg, BYTE  comDevRef)
{
   WORD  wBaudID;

   switch(trmParams.speed)
   {
   case 110:
      wBaudID = ITMBD110;
      break;
   case 300:
      wBaudID = ITMBD300;
      break;
   case 600:
      wBaudID = ITMBD600;
      break;
   case 2400:
      wBaudID = ITMBD240;
      break;
   case 4800:
      wBaudID = ITMBD480;
      break;
   case 9600:
      wBaudID = ITMBD960;
      break;
   case 14400:
      wBaudID = ITMBD144;
      break;
   case 19200:
      wBaudID = ITMBD192;
      break;

   case 38400:
      wBaudID = ITMBD384;
      break;
   case 57600:
      wBaudID = ITMBD576;
      break;
   case 57601:
      wBaudID = ITMBD1152;
      break;
   //case 57602:
   //	wBaudID = ITMBD1280;
   //	break;


   default:
      wBaudID = ITMBD120;
      trmParams.speed = 1200;
      break;
   }

   switch(comDevRef)
   {
   case ITMWINCOM:
      enableGrpItems(hDlg, ITMBD110, ITMBD1152, wBaudID);
      enableGrpItems(hDlg, ITMDATA5, ITMDATA8, trmParams.dataBits); /* jtf 3.24 */
      enableGrpItems(hDlg, ITMSTOP1, ITMSTOP2, trmParams.stopBits);
      enableGrpItems(hDlg, ITMNOPARITY, ITMSPACEPARITY, trmParams.parity);
      enableGrpItems(hDlg, ITMXONFLOW, ITMNOFLOW, trmParams.flowControl);
      if(trmParams.dataBits == ITMDATA5) /* jtf 3.24 */
        {
        EnableWindow(GetDlgItem(hDlg, ITMDATA6), FALSE);
        EnableWindow(GetDlgItem(hDlg, ITMDATA7), FALSE);
        EnableWindow(GetDlgItem(hDlg, ITMDATA8), FALSE);
        }
      else
        EnableWindow(GetDlgItem(hDlg, ITMDATA5), FALSE);

      break;

   default:
      enableGrpItems(hDlg, ITMBD110, ITMBD1152, 0);
      enableGrpItems(hDlg, ITMDATA5, ITMDATA8, 0);  /* jtf 3.24 */
      enableGrpItems(hDlg, ITMSTOP1, ITMSTOP2, 0);
      enableGrpItems(hDlg, ITMNOPARITY, ITMSPACEPARITY, 0);
      enableGrpItems(hDlg, ITMXONFLOW, ITMNOFLOW, 0);
      break;
   }

   CheckDlgButton(hDlg, ITMPARITY, (comDevRef == ITMWINCOM) ? trmParams.fParity : FALSE);
   EnableWindow(GetDlgItem(hDlg, ITMPARITY), (comDevRef == ITMWINCOM) ? TRUE : FALSE);
   CheckDlgButton(hDlg, ITMCARRIER, (comDevRef == ITMWINCOM) ? trmParams.fCarrier : FALSE);
   EnableWindow(GetDlgItem(hDlg, ITMCARRIER), (comDevRef == ITMWINCOM) ? TRUE : FALSE);
}


BOOL  APIENTRY dbComm(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   WORD  wCurSel;                            /* seh nova 005 */
   LPCONNECTOR_CONTROL_BLOCK  lpWorkCCB;     /* slc nova 047 */
   WORD  lastItem;                           /* slc nova 117 */
   WORD  lastPort;			     /* slc nova 117 */
   BOOL  prevcarrier;

   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      if((((PSAVEPARAMS) taskState.string)->hData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(recTrmParams))) == NULL)
         break;
      ((PSAVEPARAMS) taskState.string)->lpData = (recTrmParams FAR *) GlobalLock(((PSAVEPARAMS) taskState.string)->hData);
      *(((PSAVEPARAMS) taskState.string)->lpData) = trmParams;

      trmParams.newDevRef = -1;              /* mbbx 2.00: network... */
      enableCommItems(hDlg, trmParams.comDevRef);

      initComDevSelect(hDlg, ITMCONNECTOR, FALSE);    /* mbbx 2.01.09 ... */

/* rjs bug2 - add tcp/ip connector for microsoft */
      EnableWindow(GetDlgItem(hDlg, ITMSETUP), FALSE);   /* slc nova 031, seh nova 005 */
      ShowWindow(GetDlgItem(hDlg, ITMSETUP), SW_HIDE);   /* seh nova 005 */
      addConnectorList(hDlg, ITMCONNECTOR);  /* slc nova 012 bjw gold 027 */
      ghWorkConnector = NULL;                /* slc nova 031 */
      if(trmParams.comDevRef == ITMDLLCONNECT)
      {
         SendDlgItemMessage(hDlg, ITMCONNECTOR, LB_SETCURSEL, trmParams.comExtRef - 1, 0L);
         ghWorkConnector = ghCCB;            /* slc nova 031 */
      }
/* end of rjs bug2 */
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:


         if(!trmParams.fResetDevice)
         {
            if(trmParams.newDevRef != (BYTE) -1)   /* mbbx 2.00.04: force reload... */
               trmParams.fResetDevice = TRUE;
         }

         if(trmParams.newDevRef == ITMDLLCONNECT)  /* slc nova 031 */
         {
            if((ghWorkConnector != NULL) &&
               (ghWorkConnector != ghCCB))
            {
               GlobalFree(ghCCB);
               ghCCB = ghWorkConnector;
               ghWorkConnector = NULL;
            }
         }

         resetSerial(&trmParams, (trmParams.newDevRef != (BYTE) -1), FALSE, 0);  /* slc swat */

         if(sPort == (HANDLE)-1)
	    break;	 //return(TRUE);

         if((trmParams.newDevRef != (BYTE) -1) && (trmParams.comDevRef != trmParams.newDevRef))
            return(TRUE);

         termData.flags |= TF_CHANGED;
         break;

      case IDCANCEL:
         trmParams = *(((PSAVEPARAMS) taskState.string)->lpData);
         break;

/* rjs bug2 - begin */
      case ITMSETUP:
         if(ghWorkConnector != NULL)
            DLL_SetupConnector(ghWorkConnector, TRUE);
         SetFocus(GetDlgItem(hDlg, IDOK));
         return(TRUE);
/* rjs bug2 - end */

      case ITMBD110:
      case ITMBD300:
      case ITMBD600:                         /* mbbx 2.00: support 600 baud... */
      case ITMBD120:
      case ITMBD240:
      case ITMBD480:
      case ITMBD960:
      case ITMBD144:
      case ITMBD192:
      case ITMBD384:
      case ITMBD576:
      case ITMBD1152:
         switch(GET_WM_COMMAND_ID(wParam, lParam))
         {
         case ITMBD110:
            trmParams.speed = 110;
            break;
         case ITMBD300:
            trmParams.speed = 300;
            break;
         case ITMBD600:
            trmParams.speed = 600;
            break;
         case ITMBD120:
            trmParams.speed = 1200;
            break;
         case ITMBD240:
            trmParams.speed = 2400;
            break;
         case ITMBD480:
            trmParams.speed = 4800;
            break;
         case ITMBD960:
	    trmParams.speed = 9600;
	    break;
	 case ITMBD144:
	    trmParams.speed = 14400;
            break;
         case ITMBD192:
	    trmParams.speed = 19200;
	    break;
	 case ITMBD384:
	    trmParams.speed = 38400;
	    break;
	 case ITMBD576:
	    trmParams.speed = 57600;
	    break;
	 case ITMBD1152:
	    trmParams.speed = 57601;
	    break;
	 //case ITMBD1280:
	 //   trmParams.speed = 57602;
	 //   break;


         }

	 CheckRadioButton(hDlg, ITMBD110, ITMBD1152, GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);

      case ITMDATA8:                         /* mbbx 2.00 ... */
         if(trmParams.parity != ITMNOPARITY)
            CheckRadioButton(hDlg, ITMNOPARITY, ITMSPACEPARITY, trmParams.parity = ITMNOPARITY);
                                             /* then fall thru... */
      case ITMDATA5:
      case ITMDATA6:
      case ITMDATA7:
         CheckRadioButton(hDlg, ITMDATA5, ITMDATA8, trmParams.dataBits = GET_WM_COMMAND_ID(wParam, lParam)); /* jtf 3.24 */
         return(TRUE);

      case ITMSTOP5:                      /* jtf 3.24 */
         CheckRadioButton(hDlg, ITMSTOP1, ITMSTOP2, trmParams.stopBits = GET_WM_COMMAND_ID(wParam, lParam));
         CheckRadioButton(hDlg, ITMDATA5, ITMDATA8, trmParams.dataBits = ITMDATA5); /* jtf 3.24 */
         EnableWindow(GetDlgItem(hDlg, ITMDATA5), TRUE);
         EnableWindow(GetDlgItem(hDlg, ITMDATA6), FALSE);
         EnableWindow(GetDlgItem(hDlg, ITMDATA7), FALSE);
         EnableWindow(GetDlgItem(hDlg, ITMDATA8), FALSE);
         return(TRUE);
        
      case ITMSTOP1:
      case ITMSTOP2:
         EnableWindow(GetDlgItem(hDlg, ITMDATA5), FALSE);
         EnableWindow(GetDlgItem(hDlg, ITMDATA6), TRUE);
         EnableWindow(GetDlgItem(hDlg, ITMDATA7), TRUE);
         EnableWindow(GetDlgItem(hDlg, ITMDATA8), TRUE);
         if (trmParams.dataBits = ITMDATA5)
            CheckRadioButton(hDlg, ITMDATA5, ITMDATA8, trmParams.dataBits = ITMDATA8); /* jtf 3.24 */
         CheckRadioButton(hDlg, ITMSTOP1, ITMSTOP2, trmParams.stopBits = GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);

      case ITMODDPARITY:                     /* mbbx 2.00 ... */
      case ITMEVENPARITY:
      case ITMMARKPARITY:
      case ITMSPACEPARITY:
         if(trmParams.dataBits == ITMDATA8)
            CheckRadioButton(hDlg, ITMDATA5, ITMDATA8, trmParams.dataBits = ITMDATA7); /* jtf 3.24 */
                                             /* then fall thru... */
      case ITMNOPARITY:
         CheckRadioButton(hDlg, ITMNOPARITY, ITMSPACEPARITY, trmParams.parity = GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);

      case ITMXONFLOW:
      case ITMHARDFLOW:
      case ITMNOFLOW:
         CheckRadioButton(hDlg, ITMXONFLOW, ITMNOFLOW, trmParams.flowControl = GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);

      case ITMCONNECTOR:
         switch(GET_WM_COMMAND_CMD(wParam, lParam))              /* mbbx 2.00: network... */
         {
	 case LBN_SELCHANGE:		     /* mbbx 2.01.09 ... */
            lastItem = trmParams.newDevRef;  /* slc nova 117 */
            lastPort = trmParams.comPortRef; /* slc nova 117 */
            trmParams.comPortRef = getComDevSelect(hDlg, ITMCONNECTOR, &trmParams.newDevRef);

            if(trmParams.comPortRef > ITMCOM4)
            {
               wCurSel = (WORD) SendDlgItemMessage(hDlg, ITMCONNECTOR, LB_GETCURSEL, 0, 0L);  /* seh nova 005 */
               SendDlgItemMessage(hDlg, ITMCONNECTOR, LB_GETTEXT, wCurSel,
                                 (LONG) (LPSTR)gszWork); /* bjw gold 027, seh nova 005 */

               if((ghWorkConnector == NULL) ||   /* slc nova 031 */
                  (ghWorkConnector == ghCCB))
               {
                  ghWorkConnector = GlobalAlloc(GHND | GMEM_ZEROINIT, (DWORD)sizeof(CONNECTOR_CONTROL_BLOCK));
               }

               EnableWindow(GetDlgItem(hDlg, ITMSETUP), FALSE);   /* slc nova 031 */
               ShowWindow(GetDlgItem(hDlg, ITMSETUP), SW_HIDE);
               if(loadConnector(hDlg, ghWorkConnector, (LPSTR)gszWork, FALSE)) /* slc nova 031 */
               {
                  trmParams.comExtRef = (BYTE)(wCurSel + 1);
                  trmParams.newDevRef = ITMDLLCONNECT;
                  lstrcpy((LPSTR)trmParams.szConnectorName, (LPSTR)gszWork);

                  if(DLL_HasSetupBox(ghWorkConnector))   /* slc nova 031 */
                  {
                     EnableWindow(GetDlgItem(hDlg, ITMSETUP), TRUE);
                     ShowWindow(GetDlgItem(hDlg, ITMSETUP), SW_SHOW);
                  }
               }
            }

            enableCommItems(hDlg, trmParams.newDevRef);
	    trmParams.fResetDevice = FALSE;
            break;
	 }
	 return(TRUE);



      case ITMPARITY:
         CheckDlgButton(hDlg, ITMPARITY, trmParams.fParity = !trmParams.fParity);
         return(TRUE);

      case ITMCARRIER:
	 prevcarrier = trmParams.fCarrier;
	 CheckDlgButton(hDlg, ITMCARRIER, trmParams.fCarrier = !trmParams.fCarrier);
	 //-sdj if fcarrier is disabled by the user, reset the mdmOnLine flag
	 //-sdj so that, the Dial menu item gets enabled. bug# 735
	 if (prevcarrier && !(trmParams.fCarrier) ) mdmOnLine = FALSE;
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   if(((PSAVEPARAMS) taskState.string)->hData != NULL)
   {
      GlobalUnlock(((PSAVEPARAMS) taskState.string)->hData);
      GlobalFree(((PSAVEPARAMS) taskState.string)->hData);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}

/*---------------------------------------------------------------------------*/
/* dbModem() -                                                         [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR loadModemData(HWND  hDlg, WORD  wMdmType)
{
   WORD  nResID;
   INT   ndx;
   BYTE  work[STR255];

   switch(wMdmType)
   {
   case ITMHAYES:
      nResID = STR_MDM_HAYES;
      break;
   case ITMMULTITECH:
      nResID = STR_MDM_MNP;
      break;
   case ITMTRAILBLAZER:
      nResID = STR_MDM_TELEBIT;
      break;
   default:                                  /* ITMNOMODEM */
      nResID = STR_MDM_NONE;
      break;
   }

   for(ndx = 0; ndx < 10; ndx ++)      /* rjs swat - changed to 10 from 11 */
   {
      LoadString(hInst, nResID + ndx, (LPSTR) work, STR255-1);
      SetDlgItemText(hDlg, ITMDIAL + ndx, (LPSTR) work);
   }

   ndx++;

   LoadString(hInst, nResID + ndx, (LPSTR) work, STR255-1);
   SetDlgItemText(hDlg, ITMDIAL + ndx, (LPSTR) work);
}


BOOL  APIENTRY dbModem(HWND  hDlg, UINT  message, WPARAM wParam, LONG  lParam)
{
   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      if((((PSAVEPARAMS) taskState.string)->hData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(recTrmParams))) == NULL)
         break;
      ((PSAVEPARAMS) taskState.string)->lpData = (recTrmParams FAR *) GlobalLock(((PSAVEPARAMS) taskState.string)->hData);
      *(((PSAVEPARAMS) taskState.string)->lpData) = trmParams;

      SetDlgItemText(hDlg, ITMDIAL, (LPSTR) trmParams.dialPrefix);
      SetDlgItemText(hDlg, ITMDIAL+1, (LPSTR) trmParams.dialSuffix);
      SetDlgItemText(hDlg, ITMHANGUP, (LPSTR) trmParams.hangPrefix);
      SetDlgItemText(hDlg, ITMHANGUP+1, (LPSTR) trmParams.hangSuffix);
      SetDlgItemText(hDlg, ITMBINTX, (LPSTR) trmParams.binTXPrefix);
      SetDlgItemText(hDlg, ITMBINTX+1, (LPSTR) trmParams.binTXSuffix);
      SetDlgItemText(hDlg, ITMBINRX, (LPSTR) trmParams.binRXPrefix);
      SetDlgItemText(hDlg, ITMBINRX+1, (LPSTR) trmParams.binRXSuffix);
      SetDlgItemText(hDlg, ITMFASTQRY, (LPSTR) trmParams.fastInq);
      SetDlgItemText(hDlg, ITMFASTQRY+1, (LPSTR) trmParams.fastRsp);
      SetDlgItemText(hDlg, ITMANSWER, (LPSTR) trmParams.answer);
      SetDlgItemText(hDlg, ITMORIGIN, (LPSTR) trmParams.originate);

      CheckRadioButton(hDlg, ITMMODEMFIRST, ITMMODEMLAST, trmParams.xMdmType);

      ((PSAVEPARAMS) taskState.string)->saveLevel = -1;
      
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         GetDlgItemText(hDlg, ITMDIAL, (LPSTR) trmParams.dialPrefix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMDIAL+1, (LPSTR) trmParams.dialSuffix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMHANGUP, (LPSTR) trmParams.hangPrefix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMHANGUP+1, (LPSTR) trmParams.hangSuffix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMBINTX, (LPSTR) trmParams.binTXPrefix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMBINTX+1, (LPSTR) trmParams.binTXSuffix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMBINRX, (LPSTR) trmParams.binRXPrefix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMBINRX+1, (LPSTR) trmParams.binRXSuffix, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMFASTQRY, (LPSTR) trmParams.fastInq, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMFASTQRY+1, (LPSTR) trmParams.fastRsp, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMANSWER, (LPSTR) trmParams.answer, DCS_MODEMCMDSZ-1);
         GetDlgItemText(hDlg, ITMORIGIN, (LPSTR) trmParams.originate, DCS_MODEMCMDSZ-1);

         trmParams.xMdmType = whichGrpButton(hDlg, ITMMODEMFIRST, ITMMODEMLAST);

         termData.flags |= TF_CHANGED;

         break;

      case IDCANCEL:
         trmParams = *(((PSAVEPARAMS) taskState.string)->lpData);
         break;

      case ITMHAYES:
      case ITMMULTITECH:
      case ITMTRAILBLAZER:
      case ITMNOMODEM:
         CheckRadioButton(hDlg, ITMMODEMFIRST, ITMMODEMLAST, GET_WM_COMMAND_ID(wParam, lParam));
         loadModemData(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
         return(TRUE);

      default:
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   EndDialog(hDlg, TRUE);
   return(TRUE);
}




/*---------------------------------------------------------------------------*/
/* chkGrpButton() -  Toggle on radio buttons.                                */
/*---------------------------------------------------------------------------*/

VOID chkGrpButton(HWND hDlg, INT iFirst, INT iLast, INT item)
{
   if((item >= iFirst) && (item <= iLast))   /* mbbx 1.04 ... */
      CheckRadioButton(hDlg, iFirst, iLast, item);
}


/*---------------------------------------------------------------------------*/
/* whichGrpButton() - Return which button in a group was pressed.            */
/*---------------------------------------------------------------------------*/

BYTE whichGrpButton(HWND  hDlg, INT   iFirst, INT iLast)
{
   INT   which;

   for(which = iFirst; which <= iLast; which += 1)
      if(IsDlgButtonChecked(hDlg, which))
         return((BYTE) which);

   return((BYTE) iFirst);
}
