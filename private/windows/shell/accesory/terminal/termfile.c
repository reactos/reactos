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
#define  NOSCROLL	         TRUE
#define  NOHELP            TRUE
#define  NOPROFILER	      TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31
#define  USECOMM

#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "fileopen.h"
#include "task.h"


/*---------------------------------------------------------------------------*/

#define DCS_OLD_FKEYLEVELS       4
#define DCS_OLD_NUMFKEYS         8
#define DCS_OLD_FKEYLEN          64
#define DCS_OLD_FKEYTITLESZ      20
#define DCS_OLD_XTRALEN          44
#define DCS_OLD_MDMCMDLEN        32
#define DCS_OLD_MISCSTRLEN       21


#define TITLEREC                 struct tagTITLEREC

struct tagTITLEREC
{
   BYTE  title[DCS_OLD_FKEYTITLESZ];
   BYTE  xtra[DCS_OLD_XTRALEN];
};


#define oldTrmParams             struct tagOldTrmParams
#define LPOLDSETTINGS            oldTrmParams FAR *

struct tagOldTrmParams
{
   INT         inpRefNum;
   INT         outRefNum;
   INT         emulate;
   INT         dataBits;
   INT         parity;
   INT         speed;
   INT         stopBits;
   BYTE        col80Or132;
   BYTE        flowControl;
   BOOL     localEcho;
   BOOL     lineWrap;
   BOOL     inpCRLF;
   BOOL     outCRLF;
   BOOL     inpLFCR;
   BOOL     fCtrlBits;
   STRING      fKeyText[DCS_OLD_FKEYLEVELS][DCS_OLD_NUMFKEYS][DCS_OLD_FKEYLEN];
   TITLEREC    fKeyTitle[DCS_OLD_FKEYLEVELS][DCS_OLD_NUMFKEYS];
   STRING      phone[DCS_OLD_MDMCMDLEN];
   STRING      binTXPrefix[DCS_OLD_MDMCMDLEN];
   STRING      dialPrefix[DCS_OLD_MDMCMDLEN];
   STRING      binTXSuffix[DCS_OLD_MDMCMDLEN];
   STRING      dialSuffix[DCS_OLD_MDMCMDLEN];
   STRING      binRXPrefix[DCS_OLD_MDMCMDLEN];
   STRING      hangPrefix[DCS_OLD_MDMCMDLEN];
   STRING      binRXSuffix[DCS_OLD_MDMCMDLEN];
   STRING      hangSuffix[DCS_OLD_MDMCMDLEN];
   STRING      fastInq[DCS_OLD_MDMCMDLEN];
   STRING      originate[DCS_OLD_MDMCMDLEN];
   STRING      fastRsp[DCS_OLD_MDMCMDLEN];
   STRING      answer[DCS_OLD_MDMCMDLEN];
   STRING      strXtra8[DCS_OLD_MDMCMDLEN];
   BOOL     flgRetry;
   BYTE        padding2;
   INT         dlyRetry;
   INT         cntRetry;
   BOOL     flgSignal;
   BYTE        version;
   BOOL     carrierFlag;
   BYTE        padding3;
   STRING      xLinStr[DCS_OLD_MISCSTRLEN+1];
   STRING      mdmConnect[DCS_OLD_MISCSTRLEN+1];
   STRING      strXtra2[DCS_OLD_MISCSTRLEN+1];
   STRING      strXtra3[DCS_OLD_MISCSTRLEN+1];
   STRING      strXtra4[DCS_OLD_MISCSTRLEN+1];
   INT         rcvBlSz;
   INT         sendBlSz;
   INT         retryCt;
   BYTE        remote;
   BYTE        padding9;
   INT         xtraInt[4];
   INT         xWrapCol;
   BYTE        termCursor;
   BOOL     cursorBlink;
   BOOL     bsKey;
   BYTE        language;
   STRING      autoExecute[DCS_OLD_FKEYLEN];
   BYTE        xTxtType;
   BYTE        xChrType;
   INT         xChrDelay;
   BYTE        xLinType;
   BYTE        padding10;
   INT         xLinDelay;
   BYTE        xMdmType;
   BOOL     xWordWrap;
   BYTE        xBinType;
   BYTE        xBinFork;
};


/*---------------------------------------------------------------------------*/

#define MSSETTINGS               struct tagMSSETTINGS
#define LPMSSETTINGS             MSSETTINGS FAR *

struct tagMSSETTINGS
{
   INT     iCRC;               /* The CRC checksum for the settings */
   INT     iVersion;           /* The version number */
   BOOL    fModem;             /* True if port is connected to a modem */
   BOOL    fNewLine;           /* True if new-line button is on */
   BOOL    fLocalEcho;         /* True if local-echo button is on */
   BOOL    fAutoWrap;          /* True if auto-wrap button is on */
   BOOL    fVT52;              /* True if term is VT52, false if ANSI */
   BOOL    fLarge;             /* True for large text size */
   BOOL    fTone;              /* True if dial type is tone */
   BOOL    fFast;              /* True if dial pulses are short */
   INT     clnBuf;             /* Count of lines in buffer */
   INT     iBaud;              /* The baud rate */
   INT     iPort;              /* The port number */
   INT     iByteSize;          /* The number of bits per char */
   INT     iStopBits;          /* The number of stop bits */
   INT     iParity;            /* The parity method */
   INT     iHandshake;         /* The handshake method */
   INT     iAnswer;            /* The number of seconds to wait for answer */
   INT     iToneWait;          /* Number of seconds to wait for tone */
   CHAR    szPhNum[30];        /* The phone number */
   CHAR    szFNCapture[128];   /* The capture file name */
   INT     iCountry;           /* The character translation country code */
};


/*---------------------------------------------------------------------------*/

BOOL termInitSetup(HANDLE);                  /* ==> termfile.h */
VOID termCloseEnable(BOOL);
BOOL termCloseFile(VOID);
BOOL termCloseAll(VOID);
BOOL termSaveFile(BOOL);
BOOL NEAR termReadFile(BYTE *);
VOID NEAR termCloseWindow(VOID);
BOOL NEAR termCloseFileInternal(VOID);
INT TF_ErrProc(WORD, WORD, WORD);

BOOL NEAR readTermSettings(INT, WORD);       /* mbbx 2.00: was LONG */
VOID NEAR convertMSToIBM(LPMSSETTINGS, recTrmParams *);
BOOL NEAR writeTermSettings(INT);
VOID NEAR convertInternalToVirtual(LPSETTINGS, LPSETTINGS); /* rkhx 2.00 */


/*---------------------------------------------------------------------------*/
/* termInitSetup() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

BOOL termInitSetup(HANDLE   hPrevInstance)
{
   // sdj: was unref local- WNDCLASS	termClass;

   activTerm = FALSE;

   strcpy(termData.filePath, fileDocData[FILE_NDX_SETTINGS].filePath);
   *termData.fileName = 0;
   strcpy(termData.fileExt, fileDocData[FILE_NDX_SETTINGS].fileExt);
   LoadString(hInst, STR_TERMINAL, (LPSTR) termData.title, MINRESSTR);
   termData.flags = TF_DEFTITLE;

   resetEmul();                              /* must have init settings! */

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* termFile() -                                                        [mbb] */
/*---------------------------------------------------------------------------*/

BOOL termFile(BYTE *filePath, BYTE *fileName,BYTE *fileExt,BYTE *title,WORD flags)
{
   BYTE  work[PATHLEN];
   RECT  termRect;
   BYTE  OEMname[STR255];            /* jtf 3.20 */

   if(strlen(fileName) > 0)                  /* mbbx 1.03 ... */
   {
#ifdef ORGCODE
      strcpy(work, filePath);                /* mbbx 2.00: no forced extents... */
      strcpy(work+strlen(work), fileName);      /* jtf 3.20 */
#else
      strcpy(work, filePath);                /* mbbx 2.00: no forced extents... */
      strcpy(work+strlen(work), "\\");      /* jtf 3.20 */
      strcpy(work+strlen(work), fileName);      /* jtf 3.20 */
#endif


      // JYF -- replace below two lines with following if()
      //        to remove the use of AnsiToOem()
      //
      //AnsiToOem((LPSTR) work, (LPSTR) OEMname); /* jtf 3.20 */
      //if(!fileExist(OEMname))

      if (!fileExist(work))
         return(FALSE);
   }

   if(termCloseFileInternal())
   {
      if((flags & TF_DEFTITLE) && (strlen(fileName) > 0))
         strcpy(termData.title, fileName);
      else
         strcpy(termData.title, title);
      SetWindowText(hTermWnd, (LPSTR) termData.title);
      setAppTitle();

      SetCursor(LoadCursor(NULL, IDC_WAIT));

      strcpy(termData.filePath, filePath);
      strcpy(termData.fileName, fileName);
      strcpy(termData.fileExt, fileExt);
      termData.flags = flags | (termData.flags & (TF_DIM | TF_HIDE));

      setDefaults();                         /* mbbx 2.00: do this first! */
      if(strlen(fileName) > 0)
      {
         if(!termReadFile(work))
            doFileNew(); /* jtf 3.31 */
      }

      activTerm = TRUE;

      setFKeyLevel(1, TRUE);       /* mbbx 2.00 ... */
      showHidedbmyControls(trmParams.environmentFlags & DCS_EVF_FKEYSSHOW, TRUE);   /* jtf 3.20 */
      resetSerial(&trmParams, TRUE, FALSE, 0);
      resetEmul();
      initChildSize(&termRect);              /* mbbx 2.00: reset window pos... */
      if(IsZoomed(hTermWnd))
         SetFocus(NULL);

      if(!(termData.flags & TF_HIDE))
         showTerminal(TRUE, TRUE);

      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }

   gotCommEvent = TRUE;
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* termCloseFile() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

BOOL termCloseFile(VOID)
{
   BOOL  termCloseFile = TRUE;

   if(activTerm)
   {
      if(termCloseFile = termCloseFileInternal())
         termCloseWindow();
   }

   return(termCloseFile);
}


/*---------------------------------------------------------------------------*/
/* termCloseAll() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

BOOL termCloseAll(VOID)
{
   return(termCloseFile());
}


/*---------------------------------------------------------------------------*/
/* termSaveFile() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

BOOL termSaveFile(BOOL  bGetName)
{
   BOOL        termSaveFile = TRUE;
   BYTE        titleText[MINRESSTR];
   BYTE        filePath[PATHLEN+1];
   INT         hFile;
   // sdj: was unref local - OFSTRUCT	ofDummy;
   BYTE        fileName[PATHLEN];

   SetCursor(LoadCursor(NULL, IDC_WAIT));

   if(activTerm)
   {
      if(bGetName || (strlen(termData.fileName) == 0))
      {
         /* rjs swat - default to *.trm if no default term name */
         if(strlen(termData.fileName))
            strcpy(fileName, termData.fileName);
         else
            strcpy(fileName, "*.TRM");
         /* end rjs swat */

                                             /* mbbx 1.10: CUA... */
         LoadString(hInst, !bGetName ? STR_SAVE : STR_SAVEAS, (LPSTR) titleText, MINRESSTR);

         if(termData.filePath[strlen(termData.filePath) - 1] != '\\')
            strcat(termData.filePath, "\\");

         termSaveFile = FileOpen(termData.filePath, fileName, NULL, termData.fileExt,
                                 titleText, 0, NULL, FO_PUTFILE | FO_FORCEEXTENT);

         if(strrchr(fileName, '\\'))
            strcpy(termData.fileName, strrchr(fileName, '\\') + 1);
         else
            strcpy(termData.fileName, fileName);
      }

      if(termSaveFile)
      {
         setFileDocData(FILE_NDX_SETTINGS, termData.filePath, termData.fileName, NULL, NULL);   /* mbbx 2.00: no forced extents... */

         if(termData.flags & TF_DEFTITLE)
         {
            strcpy(termData.title, termData.fileName);
            SetWindowText(hTermWnd, (LPSTR) termData.fileName);
            setAppTitle();                   /* mbbx 1.00 */
         }

#ifdef ORGCODE
         strcpy(filePath, termData.filePath);
         strcpy(filePath+strlen(filePath), termData.fileName);
#else

         /* jtf 3.20 */

         strcpy(filePath, termData.filePath);
         strcpy(filePath+strlen(filePath), "\\");
         strcpy(filePath+strlen(filePath), termData.fileName);

      DEBOUT("termSaveFile: opening the file[%s]\n",filePath);
      DEBOUT("termSaveFile: with flags      [%lx]\n",O_WRONLY|O_TRUNC|O_CREAT|S_IWRITE);
#endif
         if((hFile = _open((LPSTR) filePath, O_WRONLY | O_TRUNC | O_CREAT, S_IWRITE)) != -1)
         {
            if(termSaveFile = writeTermSettings(hFile))
               termData.flags &= ~TF_CHANGED;

            _close(hFile);
         }
         else
            termSaveFile = FALSE;

         if(!termSaveFile)
         {
            TF_ErrProc(STREWRERR, MB_OK | MB_ICONHAND, 999);
            strcpy(termData.fileName, "");      /* rjs swat */
         }
      }
      else
         strcpy(termData.fileName, "");      /* rjs swat */
   }

   SetCursor(LoadCursor(NULL, IDC_ARROW));

   return(termSaveFile);
}


/*---------------------------------------------------------------------------*/
/* termReadFile() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR termReadFile(BYTE  *filePath)
{
   BOOL        termReadFile = FALSE;
   INT         hFile;
   // sdj: was unref local - OFSTRUCT	ofDummy;
   // sdj: was unref local - LOGFONT	logFont;
   BYTE        *junk;

   /* jtf 3.20 */
   DEBOUT("termReadFile: opening the file[%s]\n",filePath);
   DEBOUT("termReadFile: with flags      [%lx]\n",O_RDONLY);

   junk = strrchr(filePath, '.');
   AnsiUpper((LPSTR)junk);

   if((hFile = _open((LPSTR) filePath, O_RDONLY)) != -1)
   {
      if(termReadFile = readTermSettings(hFile, _filelength(hFile)))
      {
         resetTermBuffer();               /* mbbx 1.03 */
         icsResetTable((WORD) trmParams.language);    /* mbbx 1.04: ics */
         buildTermFont();                    /* mbbx 2.00: font selection... */
         trmParams.fResetDevice = TRUE;      /* mbbx 2.00: network... */
         trmParams.newDevRef = trmParams.comDevRef;
         trmParams.comDevRef = ITMNOCOM;
      }

      _close(hFile);
   }
   else
      TF_ErrProc(STRFERROPEN, MB_OK | MB_ICONHAND, 999);

   return(termReadFile);
}


/*---------------------------------------------------------------------------*/
/* termCloseWindow() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR termCloseWindow(VOID)
{
   if(IsWindowVisible(hTermWnd))
   {
      if(childZoomStatus(0x0001, 0) && (countChildWindows(FALSE) == 1))
      {
         childZoomStatus(0, 0x4000);
         ShowWindow(hTermWnd, SW_RESTORE);
      }
      /* added typecast to param 2 as HWND -sdj*/
      SetWindowPos(hTermWnd,(HWND) 1, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW);
      selectTopWindow();                     /* mbbx 1.03 ... */
   }
}


/*---------------------------------------------------------------------------*/
/* termCloseFileInternal() -                                           [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR termCloseFileInternal(VOID)
{
   BOOL  termCloseFileInternal = TRUE;
   if(activTerm)
   {
      if(termData.flags & TF_NOCLOSE)        /* mbbx 1.01 */
      {
         termCloseFileInternal = FALSE;
      }
      else if(xferFlag != XFRNONE)
      {
         if(termCloseFileInternal = (TF_ErrProc(STR_STOPXFER, MB_YESNO | MB_ICONEXCLAMATION, 0) == IDYES))
         {
            xferBytes = 0;
            xferEndTimer = 0;
            xferWaitEcho = FALSE;
            xferStopped = TRUE;
            xferEnd();

         }
      }

      if(termCloseFileInternal)
      {
         if(termData.flags & TF_CHANGED)
         {
            switch(TF_ErrProc(STR_SAVECHANGES, MB_YESNOCANCEL | MB_ICONEXCLAMATION, 0))
            {
            case IDYES:
               termCloseFileInternal = termSaveFile(FALSE);    /* mbbx 2.00 */
               break;
            case IDCANCEL:
               termCloseFileInternal = FALSE;
               break;
            }
         }

         if(termCloseFileInternal)
	 {
            exitSerial();
            activTerm = FALSE;
         }
      }
   }

   return(termCloseFileInternal);
}


/*---------------------------------------------------------------------------*/
/* TF_ErrProc() -                                                            */
/*---------------------------------------------------------------------------*/

INT TF_ErrProc(WORD errMess, WORD errType,WORD  errCode)
{
   // sdj: was unref local - INT	result;
   //BYTE  temp1[PATHLEN]; /* rjs bugs 008 -> from 80 to 255 sdj:see below..*/
   BYTE  temp1[256];	 /* sdj: rjs just said it should be 255 but left it 80!*/
			   /* sdj: with PATHLEN as 80, LoadString was corrupting the */
			   /* sdj: stack with a err popup of invalid rcbfile */




   BYTE  temp2[80];


   GetWindowText(hItWnd, (LPSTR) temp2, 80);
   if(!strchr(temp2, '-'))
   {
      sprintf(temp1, " - %s", termData.title);
      strcpy(temp2+strlen(temp2), temp1);
   }

/* NOTE: some messages use taskState.result to pass fileName to error handler... */
   LoadString(hInst, errMess, (LPSTR) temp1, 255); /* rjs bugs 008 -> from 80 to 255 */

   MessageBeep(0);
   /* jtf 3.20 this was GetFocus() changed to GetActiveWindow() to stop rip */
   return(MessageBox(GetActiveWindow(), (LPSTR) temp1, (LPSTR) temp2, errType));
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/* SETTINGS FILE I/O ROUTINES                                                */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* readTermSettings() -                                                [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR readTermSettings(INT hFile, WORD fileSize)  /* mbbx 2.00: support NEW & OLD DCS and MS settings... */
//INT   hFile;
//WORD  fileSize;                              /* mbbx 2.00: was LONG */
{
   BOOL     readTermSettings = FALSE;
   HANDLE   hData;
   LPSTR    lpData;
   BOOL     flag; /* jtf 3.31 */

   flag = TRUE;   /* jtf 3.31 */

   if((hData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) fileSize)) != NULL)
   {
      if((lpData = GlobalLock(hData)) != NULL)
      {

  /******** this was to debug the 4K trm file/struct compact bug
	    now we can avoid this noise on the debug machine, so
	    that we get the satisfaction of resolving one more bug(5399)!

  DbgPrint("sizeof (recTrm)                 = %lx\n",sizeof(trmParams));
  DbgPrint("Address of trmParams.fileID     = %lx\n",&(trmParams.fileID));
  DbgPrint("Address of trmParams.newDevRef  = %lx\n",&(trmParams.newDevRef));
  DbgPrint("Address of trmParams.speed      = %lx\n",&(trmParams.speed));
  DbgPrint("Address of trmParams.answerBack = %lx\n",&(trmParams.answerBack));
  DbgPrint("Address of trmParams.bufferLines= %lx\n",&(trmParams.bufferLines));
  DbgPrint("Address of trmParams.retryCt    = %lx\n",&(trmParams.retryCt));
  DbgPrint("Address of trmParams.xWrapCol   = %lx\n",&(trmParams.xWrapCol));
  DbgPrint("Address of trmParams.xLinStr    = %lx\n",&(trmParams.xLinStr));
  DbgPrint("Address of trmParams.phone      = %lx\n",&(trmParams.phone));
  DbgPrint("Address of trmParams.dialSuffix = %lx\n",&(trmParams.dialSuffix));


  **************/


         if(_read(hFile, lpData, fileSize) == fileSize)
         {
            readTermSettings = TRUE;         /* mbbx 2.00: read OLD DCS... */


            switch(fileSize)
            {
            case sizeof(recTrmParams):
               lmovmem(lpData, (LPBYTE) &trmParams, fileSize);
               break;
            case sizeof(MSSETTINGS):
               convertMSToIBM((LPMSSETTINGS) lpData, &trmParams);
               break;
            default:
               TF_ErrProc(STRFERRFILELENGTH, MB_OK | MB_ICONHAND, 999);
               flag = FALSE; /* jtf 3.31 */
               readTermSettings = FALSE;
               break;
            }
         }

         GlobalUnlock(hData);
      }

      GlobalFree(hData);
   }

   if ((!readTermSettings) && (flag))
      TF_ErrProc(STRFERRREAD, MB_OK | MB_ICONHAND, 999);

   return(readTermSettings);
}

/*---------------------------------------------------------------------------*/
/* convertMSToIBM() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

#define MS_HSSOFTWARE      0
#define MS_HSHARDWARE      1
#define MS_HSNONE          2

#define MS_IDS_NONE        0
#define MS_IDS_BRITISH     1
#define MS_IDS_DANISH      2
#define MS_IDS_FINNISH     3
#define MS_IDS_FRENCH      4
#define MS_IDS_CANADIAN    5
#define MS_IDS_GERMAN      6
#define MS_IDS_ITALIAN     7
#define MS_IDS_SPANISH     8
#define MS_IDS_SWEDISH     9
#define MS_IDS_SWISS       10

VOID NEAR convertMSToIBM(LPMSSETTINGS   lpMSSettings, recTrmParams   *ptrTrmParams)
{

   ptrTrmParams->outCRLF   = (BYTE) lpMSSettings->fNewLine;
   ptrTrmParams->localEcho = (BYTE) lpMSSettings->fLocalEcho;
   ptrTrmParams->lineWrap  = (BYTE) lpMSSettings->fAutoWrap;
   ptrTrmParams->bufferLines = lpMSSettings->clnBuf;   /* jtf term */

   ptrTrmParams->emulate = lpMSSettings->fVT52 ? ITMVT52 : ITMVT100;
   ptrTrmParams->speed = lpMSSettings->iBaud; /* jtf term */
   switch(lpMSSettings->iPort)               /* mbbx 2.00: network... */
   {
   case 0: /* jtf term */
   case 1:
      ptrTrmParams->comDevRef = ITMWINCOM;
      ptrTrmParams->comPortRef = ITMCOM1 + (lpMSSettings->iPort); /* jtf term */
      break;
   default:
      ptrTrmParams->comDevRef = ITMNOCOM;
      break;
   }

   switch(lpMSSettings->iByteSize)
   {
   case 4:
      ptrTrmParams->dataBits = ITMDATA4;
      break;
   case 5:
      ptrTrmParams->dataBits = ITMDATA5;
      break;
   case 6:
      ptrTrmParams->dataBits = ITMDATA6;
      break;
   case 7:
      ptrTrmParams->dataBits = ITMDATA7;
      break;
   default:
      ptrTrmParams->dataBits = ITMDATA8;
      break;
   }

   switch(lpMSSettings->iStopBits)
   {
   case ONE5STOPBITS:
      ptrTrmParams->stopBits = ITMSTOP5;
      break;
   case TWOSTOPBITS:
      ptrTrmParams->stopBits = ITMSTOP2;
      break;
   default:
      ptrTrmParams->stopBits = ITMSTOP1;
      break;
   }

   switch(lpMSSettings->iParity)
   {
   case EVENPARITY:
      ptrTrmParams->parity = ITMEVENPARITY;
      break;
   case ODDPARITY:
      ptrTrmParams->parity = ITMODDPARITY;
      break;
   case MARKPARITY:
      ptrTrmParams->parity = ITMMARKPARITY;
      break;
   case SPACEPARITY:
      ptrTrmParams->parity = ITMSPACEPARITY;
      break;
   default:
      ptrTrmParams->parity = ITMNOPARITY;
      break;
   }

   switch(lpMSSettings->iHandshake)
   {
   case MS_HSHARDWARE:
      ptrTrmParams->flowControl = ITMHARDFLOW;
      break;
   case MS_HSNONE:
      ptrTrmParams->flowControl = ITMNOFLOW;
      break;
   default:
      ptrTrmParams->flowControl = ITMXONFLOW;
      break;
   }

   ptrTrmParams->dlyRetry = (lpMSSettings->iAnswer + lpMSSettings->iToneWait);
   lstrcpy(ptrTrmParams->phone, lpMSSettings->szPhNum);

   switch(lpMSSettings->iCountry)
   {
   case MS_IDS_BRITISH:
      ptrTrmParams->language = ICS_BRITISH;
      break;
   case MS_IDS_GERMAN:
      ptrTrmParams->language = ICS_GERMAN;
      break;
   case MS_IDS_FRENCH:
      ptrTrmParams->language = ICS_FRENCH;
      break;
   case MS_IDS_SWEDISH:
      ptrTrmParams->language = ICS_SWEDISH;
      break;
   case MS_IDS_ITALIAN:
      ptrTrmParams->language = ICS_ITALIAN;
      break;
   case MS_IDS_SPANISH:
      ptrTrmParams->language = ICS_SPANISH;
      break;
   case MS_IDS_DANISH:
      ptrTrmParams->language = ICS_DANISH;
      break;
   case MS_IDS_FINNISH:
      ptrTrmParams->language = ICS_FINISH;
      break;
   case MS_IDS_CANADIAN:
      ptrTrmParams->language = ICS_CANADIAN;
      break;
   case MS_IDS_SWISS:
      ptrTrmParams->language = ICS_SWISS;
      break;
   default:
      ptrTrmParams->language = ICS_NONE;
      break;
   }
}


/*---------------------------------------------------------------------------*/
/* writeTermSettings() -                                               [scf] */
/*---------------------------------------------------------------------------*/

BOOL NEAR writeTermSettings(INT  fileHandle)
{
   BOOL        writeTermSettings = FALSE;
   HANDLE      hSettings;
   LPSETTINGS  lpSettings;

   if((hSettings = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(recTrmParams))) != NULL)
   {
      if(lpSettings = (LPSETTINGS) GlobalLock(hSettings))
      {
         lmovmem((LPBYTE) &trmParams, (LPBYTE) lpSettings, sizeof(recTrmParams));

         /* perform conversions here, if necessary... */

         if(_write(fileHandle, (LPSTR) lpSettings, sizeof(recTrmParams)) == sizeof(recTrmParams)) /* jtf 3.15 */
            writeTermSettings = TRUE;

         GlobalUnlock(hSettings);
      }

      GlobalFree(hSettings);
   }

   return(writeTermSettings);
}
