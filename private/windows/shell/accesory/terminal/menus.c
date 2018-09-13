/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
#define  NOVIRTUALKEYCODES TRUE
#define  NOICONS                 TRUE
#define  NOKEYSTATES       TRUE
#define  NOSYSCOMMANDS     TRUE
#define  NOATOM          TRUE
#define  NODRAWTEXT           TRUE
#define  NOMINMAX                TRUE
#define  NOOPENFILE           TRUE
#define  NOSCROLL                TRUE
#define  NOPROFILER           TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31

#include <stdarg.h>
#include <windows.h>
#include <shellapi.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "printfil.h"
#include <commdlg.h>
#include <cderr.h>

/*---------------------------------------------------------------------------*/
/* enableMenuList() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

VOID enableMenuList(HMENU hAppMenu,BOOL bEnable, ...)
{
   va_list ap;
   INT      ndx;
   UINT     miList;
   HMENU    hSubMenu;

   va_start(ap,bEnable);
   for(miList = va_arg(ap,UINT); miList; miList = va_arg(ap,UINT))
   {
      if(miList & 0x8000)
      {
         miList &= 0x000F;
         if(childZoomStatus(0x0001, 0))
            miList += 1;
         hSubMenu = GetSubMenu(hAppMenu, miList);
         for(ndx = GetMenuItemCount(hSubMenu)-1; ndx >= 0; ndx -= 1)
            EnableMenuItem(hSubMenu, ndx, (bEnable ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
      }
      else
         EnableMenuItem(hAppMenu, miList, bEnable ? MF_ENABLED : MF_GRAYED);
   }
   va_end(ap);
}



/*---------------------------------------------------------------------------*/
/* initMenuPopup() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID initMenuPopup(WORD  menuIndex)
{
   HMENU    hAppMenu;
   WORD     systemIndex;

   if(childZoomStatus(0x0001, 0))
      menuIndex -= 1;

   hAppMenu = hMenu;
   systemIndex = menuIndex;
      switch(menuIndex)
      {
      case FILEMENU:
         enableMenuList(hAppMenu, countChildWindows(FALSE), FMCLOSE, FMSAVE, FMSAVEAS, NULL);
         enableMenuList(hAppMenu, TRUE, FMPRINT, FMPRINTSETUP, NULL);
         enableMenuList(hAppMenu, (hPrintFile==NULL), FMPRINTSETUP, FMPRINTSETUP, NULL); /* jtf 3.20 check to see if chanel open */
         break;

      case EDITMENU:
         if(GetTopWindow(hItWnd) == hTermWnd)
         {
            enableMenuList(hAppMenu, TRUE, EMSELECTALL, EMCLEAR, NULL);
            enableMenuList(hAppMenu, (hTE.selStart != hTE.selEnd), EMCOPY, EMCOPYTHENPASTE, EMCOPYSPECIAL, NULL);

            enableMenuList(hAppMenu, IsClipboardFormatAvailable(CF_TEXT), EMPASTE, NULL);

            if (scrapSeq || ((xferFlag != XFRNONE) && (xferFlag != XFRRCV)) ) /* jtf 3.17 disable send if in transfer */
               {
               enableMenuList(hAppMenu, FALSE, EMCOPYTHENPASTE, NULL);
               enableMenuList(hAppMenu, FALSE, EMPASTE, NULL);
               enableMenuList(hAppMenu, FALSE, EMSELECTALL, NULL);
               enableMenuList(hAppMenu, FALSE, EMCOPY, NULL);
               }

            enableMenuList(hAppMenu, (curTopLine >= savTopLine), EMSAVESCREEN, NULL);
         }
         else
            enableMenuList(hAppMenu, FALSE, 0x8000 | systemIndex, NULL);
         break;

      case SETTINGSMENU:
         enableMenuList(hAppMenu, activTerm, SMPHONE, SMEMULATE, SMTERMINAL, SMFUNCTIONKEYS, SMMODEM, NULL);
         enableMenuList(hAppMenu, activTerm && (xferFlag == XFRNONE), SMTEXTXFERS, SMBINXFERS, SMCOMMUNICATIONS, NULL);
         break;

      case PHONEMENU:                        /* mbbx 2.00: network... */
         if((trmParams.comDevRef > ITMNOCOM) && (xferFlag == XFRNONE))  /* mbbx 1.10: carrier... */
         {
            enableMenuList(hAppMenu, TRUE, 0x8000 | systemIndex, NULL);
            enableMenuList(hAppMenu, !mdmOnLine, PMDIAL, NULL);
         }
         else
            enableMenuList(hAppMenu, FALSE, 0x8000 | systemIndex, NULL);
         break;

      case TRANSFERMENU:
                                             /* mbbx 1.10: answerMode... */
         enableMenuList(hAppMenu, (trmParams.comDevRef > ITMNOCOM) && (xferFlag == XFRNONE),    /* mbbx 2.00: network... */
                        TMSENDTEXTFILE, TMRCVTEXTFILE, NULL);
         enableMenuList(hAppMenu, (trmParams.comDevRef > ITMNOCOM) && (xferFlag == XFRNONE) && !answerMode,
                        TMSENDBINFILE, TMRCVBINFILE, NULL);
         enableMenuList(hAppMenu, activTerm && (xferFlag == XFRNONE), TMVIEWTEXTFILE, NULL);
         enableMenuList(hAppMenu, (xferFlag > XFRNONE) && (xferFlag < XFRBSND) && !xferPaused, TMPAUSE, NULL);
         enableMenuList(hAppMenu, (xferFlag > XFRNONE) && (xferFlag < XFRBSND) && xferPaused, TMRESUME, NULL);
         enableMenuList(hAppMenu, (xferFlag > XFRNONE), TMSTOP, NULL);

         if (scrapSeq) /* jtf gold 045 */
         enableMenuList(hAppMenu, FALSE, TMSENDTEXTFILE, TMVIEWTEXTFILE, NULL); /* jtf 3.27 */


         break;

      case HELPMENU:                         /* mbbx 2.00: CUA... */
         break;
      }
}


/*---------------------------------------------------------------------------*/
/* doEditMenu() - Edit memu commands happen here !                     [scf] */
/*---------------------------------------------------------------------------*/

VOID doEditMenu(INT  theItem)
{
   DWORD    scrapSize;
   HANDLE   hText;
   HANDLE   tmpScrapHandle;

   // -sdj unreferenced local var HDC	theDC,newDC;
   // -sdj unreferenced local varRECT	  theClient;
   // -sdj unreferenced local varHANDLE   newHdl;

   if(GetTopWindow(hItWnd) == hTermWnd)
   {
      hText = hTE.hText;

      switch(theItem)
      {
      case EMSELECTALL:
         termSetSelect(0l, (LONG) (savTopLine + maxScreenLine + 1) * (maxChars + 2));
         activSelect = TRUE; /* jtf 3.30 */
         noSelect = FALSE;    /* rjs bugs 020 */
         break;

      case EMCOPY:
      case EMPASTE:
      case EMCOPYTABLE:
      case EMCOPYTHENPASTE:
         if((theItem == EMCOPY) || (theItem == EMCOPYTABLE) || (theItem == EMCOPYTHENPASTE))
         {
            copiedTable = FALSE;
            scrapSize = hTE.selEnd - hTE.selStart;
            if (scrapSize == 0)
                  break; /* jtf 3.20 do not copy blank selection */
                                             /* the + 3 for possible <cr>,<lf>, NULL */
            if((tEScrapHandle = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, scrapSize + 3)) != NULL)
            {
               blockMove((LPBYTE) GlobalLock(hText) + hTE.selStart,
                         (LPBYTE) GlobalLock(tEScrapHandle), scrapSize);
               GlobalUnlock(hText);
               GlobalUnlock(tEScrapHandle);

                  stripBlanks(GlobalLock(tEScrapHandle), &scrapSize);
                  GlobalUnlock(tEScrapHandle);

               if((tEScrapHandle = GlobalReAlloc(tEScrapHandle, scrapSize+1, GMEM_MOVEABLE | GMEM_ZEROINIT)) != NULL)
                  if(OpenClipboard(hTermWnd))
                  {
                     EmptyClipboard();
                     SetClipboardData(CF_TEXT, tEScrapHandle);
                     CloseClipboard();
                  }
            }
         }

         if((theItem == EMPASTE) || (theItem == EMCOPYTHENPASTE))
           if (IsClipboardFormatAvailable(CF_TEXT)) /* jtf 3.20 prevent protect violation */
            if(OpenClipboard(hTermWnd))
            {
               if((tmpScrapHandle = GetClipboardData(CF_TEXT)) != NULL)
               {
                  scrapSize = (DWORD) lstrlen(GlobalLock(tmpScrapHandle));
                  GlobalUnlock(tmpScrapHandle);
                  if((tEScrapHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, scrapSize+1)) != NULL)
                  {
                     blockMove(GlobalLock(tmpScrapHandle), GlobalLock(tEScrapHandle), scrapSize);
                     GlobalUnlock(tmpScrapHandle);
                     GlobalUnlock(tEScrapHandle);
                  }
               }
               CloseClipboard();
               useScrap = TRUE;
            }
         break;

      case EMCLEAR:
         cursorAdjust();
         clearBuffer();
         break;

      }
   }

}


/*---------------------------------------------------------------------------*/
/* doCommand() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

VOID doCommand(HWND     hWnd, WPARAM   wParam, LONG     lParam)
{

/* this is called from within WM_COMMAND, so that wParam and lParam change as per*/
/* win32.  This is also called from another place with lParm set to zero */
/* I'll fix the wParam references, and debug to see if any other fixes needed*/

   BYTE     work[STR255];
   BYTE     extrainfo[STR255];	// -sdj another buff to hold EXTRAINFO string of aboutbox;
   BOOL     result;
   // -sdj unreferenced local var LPSTR	lpEOS;
   INT      rc;
   PRINTDLG PRN;

   offCursor();

   switch(GET_WM_COMMAND_ID(wParam, lParam))
   {
   case FMNEW:
      doFileNew();
      break;

   case FMOPEN:
      doFileOpen();
      break;

   case FMCLOSE:
      doFileClose();
      break;

   case FMSAVE:
      doFileSave();
      break;

   case FMSAVEAS:
      doFileSaveAs();
      break;

   case FMPRINTSETUP:
      /* initialize PRINTDLG structure */
      PRN.lStructSize = sizeof(PRINTDLG);
      PRN.hDevMode    = hDevMode;
      PRN.hDevNames   = hDevNames;
      PRN.Flags       = PD_PRINTSETUP;
      PRN.hwndOwner   = hWnd;

      PrintDlg(&PRN);
      rc = CommDlgExtendedError();
      /* Probable cause is low memory. */
      if(rc == CDERR_DIALOGFAILURE  || rc == CDERR_INITIALIZATION ||
           rc == CDERR_LOADSTRFAILURE || rc == CDERR_LOADRESFAILURE ||
           rc == PDERR_LOADDRVFAILURE || rc == PDERR_GETDEVMODEFAIL)
      {
        testBox(hWnd,-(MB_ICONHAND|MB_SYSTEMMODAL|MB_OK),STR_ERRCAPTION,NoMemStr);
      }
      else
      {
        hDevMode = PRN.hDevMode;
        hDevNames= PRN.hDevNames;
      }
      break;

   case FMPRINTER:
      PrintFileComm(!prtFlag);         /* mbbx 1.03: prAction(!prtFlag); */
      break;

   case FMTIMER:
      timerToggle(TRUE);                     /* mbbx 1.03 */
      break;

   case FMEXIT:                              /* mbbx 1.10: CUA... */
      CommThreadExit = TRUE;  // we are exiting, so let the thread
                              // exit on its own by ExitThread.

      PostMessage(hItWnd, WM_CLOSE, 0, 0L);
      break;

   case EMUNDO:
   case EMSELECTALL:
   case EMCUT:
   case EMCOPY:
   case EMPASTE:
   case EMCLEAR:
   case EMCOPYTHENPASTE:
      doEditMenu(GET_WM_COMMAND_ID(wParam, lParam));
      break;

   case SMPHONE:                             /* mbbx 1.04 ... */
      doSettings(IDDBPHON, dbPhon);
      break;

   case SMEMULATE:
      doSettings(IDDBEMUL, dbEmul);
      break;

   case SMTERMINAL:
      doSettings(IDDBTERM, dbTerm);
      break;

   case SMFUNCTIONKEYS:
      doSettings(IDDBFKEY, dbFkey);
      break;

   case SMTEXTXFERS:
      doSettings(IDDBTXTX, dbTxtX);
      break;

   case SMBINXFERS:
      doSettings(IDDBBINX, dbBinX);
      break;

   case SMCOMMUNICATIONS:
      doSettings(IDDBCOMM, dbComm);
      break;

   case SMMODEM:                             /* mbbx 1.10: CUA... */
      doSettings(IDDBMODEM, dbModem);
      break;

   case PMDIAL:
      dialPhone();
      break;

   case PMHANGUP:
      hangUpPhone();
      break;

   case TMSENDTEXTFILE:
      DEBOUT("doCommand: Got %s menu click\n","SENDTEXTFILE");
      *taskState.string = 0;                 /* mbbx 1.01... */
      sndTFile();
      break;

   case TMRCVTEXTFILE:
      DEBOUT("doCommand: Got %s menu click\n","RCVTEXTFILE");
      *taskState.string = 0;                 /* mbbx 1.01... */
      rcvTFile();
      break;

   case TMVIEWTEXTFILE:
      DEBOUT("doCommand: Got %s menu click\n","VIEWTEXTFILE");
      *taskState.string = 0;                 /* mbbx 1.01... */
      typTFile();
      break;

   case TMSENDBINFILE:
      DEBOUT("doCommand: Got %s menu click\n","SENDBINFILE");
      *taskState.string = 0;                 /* mbbx 1.01... */
      sndBFile();
      break;

   case TMRCVBINFILE:
      DEBOUT("doCommand: Got %s menu click\n","RCVBINFILE");
      *taskState.string = 0;                 /* mbbx 1.01... */
      rcvBFile();
      break;

   case TMPAUSE:                             /* mbbx 2.00: xfer ctrls... */
      xferPauseResume(TRUE, FALSE);
      break;

   case TMRESUME:
      xferPauseResume(FALSE, TRUE);
      break;

   case TMSTOP:
      xferStopBreak(TRUE);
      break;

   case WMFKEYS:
      showHidedbmyControls(!fKeysShown, TRUE);  /* mbbx 2.00: bArrange... */
      break;

   case HMCOMMANDS:
   case HMPROCEDURES:
   case HMKEYBOARD:
      LoadString(hInst, STR_HELPFILE, (LPSTR) work, STR255-1);
      result = WinHelp(hTermWnd, (LPSTR) work, (WORD) HELP_CONTEXT, (DWORD) wParam);
      if(!result)
         testBox(hTermWnd,-(MB_ICONHAND|MB_SYSTEMMODAL|MB_OK),STR_ERRCAPTION,NoMemStr);
      break;

   case HMINDEX:                             /* mbbx 2.00: CUA... */
      LoadString(hInst, STR_HELPFILE, (LPSTR) work, STR255-1);
      result = WinHelp(hTermWnd,(LPSTR) work, (WORD) HELP_INDEX,0L);
      if(!result)
         testBox(hTermWnd,-(MB_ICONHAND|MB_SYSTEMMODAL|MB_OK),STR_ERRCAPTION,NoMemStr);
      break;

   case HMHELP:
      result = WinHelp(hTermWnd,(LPSTR) NULL,(WORD) HELP_HELPONHELP,0L);
           if(!result)
                testBox(hTermWnd,-(MB_ICONHAND|MB_SYSTEMMODAL|MB_OK),STR_ERRCAPTION,NoMemStr);
      break;

   case HMSEARCH:
      LoadString(hInst, STR_HELPFILE, (LPSTR) work, STR255-1);
      if(!WinHelp(hTermWnd, (LPSTR) work , (WORD) HELP_PARTIALKEY, (DWORD)""))
	  testBox(hTermWnd,-(MB_ICONHAND|MB_SYSTEMMODAL|MB_OK),STR_ERRCAPTION,NoMemStr);
      break;

//   case HMABOUT:
//		 DialogBox(hInst, MAKEINTRESOURCE(IDDBABOUT), hItWnd, (DLGPROC)dbAbout);
//	break;
// lets try to close one bugreport, by using shellabout again so that
// the about box is consistent with the other shell applets and win31
// -sdj 12/15/92

   case HMABOUT:
      LoadString(hInst, STR_APPNAME, (LPSTR) work,	 STR255-1);
      LoadString(hInst, STR_EXTRAINFO, (LPSTR) extrainfo, STR255-1);

		ShellAbout(hTermWnd,(LPSTR) work, (LPSTR) extrainfo,
		       LoadIcon(hInst, MAKEINTRESOURCE(ICO_DYNACOMM)));
            break;




   default:
      DEBOUT("doCommand: %s\n","Got INTO default option");
      switch(GET_WM_COMMAND_ID(wParam, lParam) & 0xF000)
      {
      case 0xF000:
                 /* NOTE under lParam may be zero and screw up under win32, since lParam*/
                 /* is passed in sometimes == zero by another function*/

         DEBOUT("doCommand: %s\n","Got into default option: doing SendMsg");
         SendMessage(GetTopWindow(hItWnd), WM_SYSCOMMAND, wParam, lParam);
         DEBOUT("doCommand: %s\n","Got into default option: done SendMsg");
         break;
      }
      DEBOUT("doCommand: %s\n","Got OUT of default option");
      break;
   }

   DEBOUT("doCommand: %s\n","Doing UpdateWindow(hItWnd)");
   UpdateWindow(hItWnd);
   DEBOUT("doCommand: %s\n","Done  UpdateWindow(hItWnd)");
   onCursor();
}

long CALLBACK dbAbout(HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
   switch (message)
   {
   case WM_INITDIALOG:
      return TRUE;

   case WM_COMMAND:
      if (wParam == IDOK)
         EndDialog(hDlg, wParam);
      break;
   }

   return FALSE;
}

