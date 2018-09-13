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


/*---------------------------------------------------------------------------*/
/* clipRect () - Sets a rect to the current clip rect                        */
/*              Note: assumes getPort () has been called                     */
/*---------------------------------------------------------------------------*/

VOID clipRect(RECT *clpRect)
{
   HRGN  hClipRgn;

   hClipRgn = CreateRectRgnIndirect ((LPRECT) clpRect);
   SelectClipRgn(thePort, hClipRgn);
   DeleteObject (hClipRgn);
}


/*---------------------------------------------------------------------------*/
/* pos() -                                                             [mbb] */
/*---------------------------------------------------------------------------*/

INT pos(STRING *word, STRING *str)
{
   INT   ndx, count;

   if((count = *str - *word + 1) > 0)
   {
      for(ndx = 1; ndx <= count; ndx += 1)
         if(memcmp(str+ndx, word+1, *word) == 0)
            return(ndx);
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* delay() - Spin wheels & be nice to other windows applications             */
/*---------------------------------------------------------------------------*/

/* NOTE: 1000/60 msec/ticks => 50/3; max delay w/o overflow = 3932 ticks */

VOID delay(WORD units, LONG  *endingCount)
{
   LONG  localEndCount;
   
   localEndCount = GetTickCount() + (long)((units * 50) / 3);
   
   while(GetTickCount() < (localEndCount))
   {
      PeekMessage((LPMSG)&msg, hItWnd, 0, 0, PM_NOREMOVE);
      updateTimer();
   }

   if(endingCount != NULL)
      *endingCount = localEndCount;
}

/*---------------------------------------------------------------------------*/
/* concat() - Concatanate two Pascal-type strings.                      [scf]*/
/*---------------------------------------------------------------------------*/

BYTE *concat (STRING *out, STRING *s1, STRING *s2)
{
   STRING scratch[STR255];

   if ((WORD) (*s1 + *s2) < STR255)
   {                               
      if (out == s2)
      {
         memcpy (scratch, s2, (WORD) *s2 + 1);
         memcpy (&out[1], &s1[1], (WORD) *s1);
         memcpy (&out[*s1 + 1], &scratch[1], (WORD) *scratch);
         *out = *s1 + *scratch;
      }
      else
      {
         memcpy (&out[1], &s1[1], (WORD) *s1);
         memcpy (&out[*s1 + 1], &s2[1], (WORD) *s2);
         *out = *s1 + *s2;
      }
   }
   nullTerminate (out);
   return out;
}


/*---------------------------------------------------------------------------*/
/* stripControl() -                                                    [scf] */
/*---------------------------------------------------------------------------*/

VOID stripControl(STRING *str)
{
   WORD     ndxDst, ndxSrc;
   STRING   newStr[STR255];

   ndxSrc = 1;
   *newStr = 0;
   for (ndxDst = 1; ndxDst <= *str; ndxDst++)
      if (ndxSrc <= *str)
      {
         newStr[0] = (BYTE) ndxDst;
         if((str[ndxSrc] == '^') && (str[ndxSrc+1] != '$'))    /* mbbx 1.04... */
            if (str[ndxSrc+1] == '^' || ndxSrc == *str)
            {
               newStr[ndxDst] = '^';
               ndxSrc += 2;
            }
            else
            {
               newStr[ndxDst] = str[ndxSrc+1] & 0x1f;
               ndxSrc += 2;
            }
         else
         {
            newStr[ndxDst] = str[ndxSrc];
            ndxSrc++;
         }
      }
   strEquals(str, newStr); 
}


/*---------------------------------------------------------------------------*/
/* clearBuffer() - Initialize terminal I/O capture buffer.             [scf] */
/*---------------------------------------------------------------------------*/

VOID clearBuffer()
{
   RECT  clientRect;

   gIdleTimer    = GetCurrentTime();   /* rjs bug2 001 */

   timPointer    = GetCurrentTime();

   escAnsi       = FALSE;
   escCol        = -1;
   escCursor     = FALSE;
   escGraphics   = FALSE;
   grChrMode     = FALSE;
   escLin        = -1;
   escSeq        = FALSE;
   escExtend     = EXNONE;
   escSkip       = 0;
   escVideo      = 0;
   vidGraphics   = GRNONE;
   vidInverse    = FALSE;
   termDirty     = FALSE;
   curLin        = 0;
   curCol        = 0;
   curTopLine    = savTopLine = 0;
   savLeftCol    = 0;
   cursorValid   = FALSE;
   rectCursor(&cursorRect);

   clearTermBuffer(0, maxLines, maxChars + 2);  /* mbbx 2.00.03: lines > 399 ... */
   termSetSelect(0L, 0L);

   chrHeight = stdChrHeight;
   chrWidth  = stdChrWidth;

   GetClientRect (hTermWnd, (LPRECT) &clientRect);
   visScreenLine = hTE.viewRect.bottom / chrHeight - 1;

   nScrollRange.x = maxChars - (clientRect.right / chrWidth);
   nScrollPos.y   = 0;
   nScrollPos.x   = 0;
   updateTermScrollBars(FALSE);

   activSelect = FALSE;
   noSelect = TRUE;     /* rjs bugs 020 */
   clearModes();

   InvalidateRect(hTermWnd, NULL, TRUE);     /* mbbx 2.00.03 ... */
   UpdateWindow(hTermWnd);
}


/*---------------------------------------------------------------------------*/
/* clearModes() - An terminal emulation initialization worker routine. [scf] */
/*---------------------------------------------------------------------------*/

VOID clearModes()
{
   INT ndx;
   INT lin;
   INT col;

   cursorKeyMode = FALSE;
   keyPadAppMode = FALSE;
   originMode    = FALSE;
   chInsMode     = FALSE;
   scrRgnBeg     = 0;
   shiftCharSet  = 0;
   charSet[0]    = 'B';
   charSet[1]    = 'B';
   scrRgnEnd     = maxScreenLine;
   statusLine    = FALSE;
   curAttrib     = 0;
   protectMode   = FALSE;

   for (ndx = 1; ndx <= 131; ndx++)
     if (ndx % 8 == 0)
        tabs[ndx] = 1;
     else
        tabs[ndx] = 0;
   for (lin = 0; lin <= 23; lin++)
      for (col = 0; col <= 132; col++)
         attrib[lin][col] = 0;
   for (ndx = 0; ndx <= maxChars -1; ndx++)
      line25[ndx] = ' ';
}


/*---------------------------------------------------------------------------*/
/* setCommDefaults() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

VOID setCommDefaults()                       /* mbbx 2.01.20 (2.01.17) ... */
{
   BYTE  str[MINRESSTR], portData[32];
   INT   nPort, nSpeed, nData, nStop;

   trmParams.comDevRef = ITMNOCOM;
   trmParams.speed     = 1200;
   trmParams.dataBits  = ITMDATA8;
   trmParams.stopBits  = ITMSTOP1;
   trmParams.parity    = ITMNOPARITY;

   LoadString(hInst, STR_INI_PORT, (LPSTR) str, MINRESSTR);
   GetProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) NULL_STR, (LPSTR) portData, sizeof(portData));

   switch(sscanf(portData, "COM%d:%d,%c,%d,%d", &nPort, &nSpeed, str, &nData, &nStop))
   {
   case 5:
      if(nStop == 2)
         trmParams.stopBits = ITMSTOP2;
                                       /* then fall thru... */
   case 4:
      if((nData >= 4) && (nData < 8))
         trmParams.dataBits = ITMDATA8 - (8 - nData);
                                       /* then fall thru... */
   case 3:
      switch(str[0])
      {
      case 'o':
         trmParams.parity = ITMODDPARITY;
         break;
      case 'e':
         trmParams.parity = ITMEVENPARITY;
         break;
      case 'm':
         trmParams.parity = ITMMARKPARITY;
         break;
      case 's':
         trmParams.parity = ITMSPACEPARITY;
         break;
      }
                                       /* then fall thru... */
   case 2:
      if((nSpeed < 150) || (nSpeed == 192))
         nSpeed *= 100;
      trmParams.speed = nSpeed;
                                       /* then fall thru... */
   case 1:
      if(nPort >= 1)
      {
         trmParams.newDevRef = ITMWINCOM;
         trmParams.comPortRef = ITMCOM1 + (nPort - 1);
         trmParams.fResetDevice = TRUE;
      }
      break;
   }

   DEBOUT("setCommDefaults:   trmParams.comDevRef = %x\n",trmParams.comDevRef);
   DEBOUT("setCommDefaults:   trmParams.speed     = %d\n",trmParams.speed);
   DEBOUT("setCommDefaults:   trmParams.dataBits  = %d\n",trmParams.dataBits);
   DEBOUT("setCommDefaults:   trmParams.stopBits  = %d\n" ,trmParams.stopBits);

   DEBOUT("setCommDefaults:   trmParams.parity    = %d\n",trmParams.parity);
   DEBOUT("setCommDefaults:   trmParams.newDevRef = %d\n",trmParams.newDevRef);

   DEBOUT("setCommDefaults:   trmParams.comPortRef = %d\n",trmParams.comPortRef);
   DEBOUT("setCommDefaults:   trmParams.fResetDevice = %d\n",trmParams.fResetDevice);

}


/*---------------------------------------------------------------------------*/
/* setDefaultFonts() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

VOID setDefaultFonts()                       /* rkhx 2.00 ... */
{
   BYTE szFont[MINRESSTR], szDefFontFace[LF_FACESIZE], szFontFace[LF_FACESIZE];
   INT  ndx;
   WORD fontHeight = 8;                      /* mbbx 2.00: was 0 */

   LoadString(hInst, STR_INI_FONT, (LPSTR) szFont, MINRESSTR);
   LoadString(hInst, STR_INI_FONTFACE, (LPSTR) szDefFontFace, LF_FACESIZE);
   GetProfileString((LPSTR) szAppName_private, (LPSTR) szFont, (LPSTR) szDefFontFace, 
                    (LPSTR) szFontFace, LF_FACESIZE);

   for(ndx = 0; szFontFace[ndx] != 0; ndx += 1)
   {
      if(szFontFace[ndx] == ',')
      {
         sscanf(szFontFace+(ndx+1), "%d", &fontHeight);
         szFontFace[ndx] = 0;
         break;
      }
   }

   strcpy(trmParams.fontFace, szFontFace);   /* mbbx 2.00: font selection... */
   trmParams.fontSize = fontHeight;
   buildTermFont();
}


/*---------------------------------------------------------------------------*/
/* getDefCountry() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

/* MS-DOS 3.0 COUNTRY command: */

#define CC_USA                   1
#define CC_UK                    44
#define CC_DENMARK               45
#define CC_NORWAY                47
#define CC_FINLAND               358
#define CC_FRANCE                33
                                             /* NOTE: CANADA not defined */
#define CC_GERMANY               49
#define CC_ITALY                 39
#define CC_SPAIN                 34
#define CC_SWEDEN                46
#define CC_SWITZERLAND           41

WORD getDefCountry()                         /* mbbx 1.04: ics... */
{
   ICS_TYPE    icsType;
   BYTE        szIntl[MINRESSTR];
   BYTE        szCountry[MINRESSTR];

   LoadString(hInst, STR_INI_INTL, (LPSTR) szIntl, MINRESSTR);
   LoadString(hInst, STR_INI_ICOUNTRY, (LPSTR) szCountry, MINRESSTR);
   switch(GetProfileInt((LPSTR) szIntl, (LPSTR) szCountry, 0))
   {
   case CC_UK:
      icsType = ICS_BRITISH;
      break;
   case CC_DENMARK:
   case CC_NORWAY:
      icsType = ICS_DANISH;
      break;
   case CC_FINLAND:
      icsType = ICS_FINISH;
      break;
   case CC_FRANCE:
      icsType = ICS_FRENCH;
      break;
   case CC_GERMANY:
      icsType = ICS_GERMAN;
      break;
   case CC_ITALY:
      icsType = ICS_ITALIAN;
      break;
   case CC_SPAIN:
      icsType = ICS_SPANISH;
      break;
   case CC_SWEDEN:
      icsType = ICS_SWEDISH;
      break;
   case CC_SWITZERLAND:
      icsType = ICS_SWISS;
      break;
   default:
      icsType = ICS_NONE;
      break;
   }

   return((WORD) icsType);
}


/*---------------------------------------------------------------------------*/
/* icsResetTable() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

#define ICS_RESBYTES             12

VOID icsResetTable(WORD icsType)
{
   WORD  ndx;
   BYTE  work1[ICS_RESBYTES+1], work2[ICS_RESBYTES+1];

   trmParams.language = (BYTE) ICS_NONE;
   for(ndx = 0; ndx < 256; ndx += 1)
      icsXlateTable[ndx] = ndx;

   if((icsType > ICS_NONE) && (icsType < ICS_MAXTYPE))
   {
      if((LoadString(hInst, STR_ICS_DATA, (LPSTR) work1, ICS_RESBYTES+1) == ICS_RESBYTES) && 
         (LoadString(hInst, STR_ICS_DATA + icsType, (LPSTR) work2, ICS_RESBYTES+1) == ICS_RESBYTES))
      {
         for(ndx = 0; ndx < ICS_RESBYTES; ndx += 1)
         {
            icsXlateTable[work1[ndx]] = work2[ndx];
            icsXlateTable[work2[ndx]] = work1[ndx];
         }

         trmParams.language = icsType;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* setDefaults() -                                                     [scf] */
/*---------------------------------------------------------------------------*/

#define DEFBUFFERLINES           100

VOID setDefaults()
{
   BYTE  str[80];
   INT   ndx;

   memset(&trmParams, 0, sizeof(recTrmParams));    /* mbbx 1.00: default to NULL */

   trmParams.fileID = DCS_FILE_ID;           /* mbbx 2.00 ... */
   LoadString(hInst, STR_VERSION, (LPSTR) trmParams.version, DCS_VERSIONSZ);

   trmParams.controlZ = CNTRLZ;
   trmParams.fileSize = sizeof(recTrmParams);

   setCommDefaults();                        /* mbbx 2.01.17 ... */
   trmParams.fParity     = FALSE;            /* mbbx 1.10: CUA */
   trmParams.flowControl = ITMXONFLOW;
   trmParams.fCarrier    = FALSE;            /* mbbx 1.10: CUA */


/* Terminal: */
   trmParams.emulate     = ITMVT100;         /* mbbx 1.04: was ITMTTY; */
   trmParams.fCtrlBits   = FALSE;            /* mbbx 1.10: VT220 8BIT = TRUE */
   LoadString(hInst, STR_ANSWERBACK, (LPSTR) trmParams.answerBack, DCS_ANSWERBACKSZ);
   trmParams.lineWrap    = TRUE;             /* mbbx 1.10: CUA... */
   trmParams.localEcho   = FALSE;
   trmParams.sound       = TRUE;             /* mbbx 1.04: synch */
   trmParams.inpCRLF     = FALSE;
   trmParams.outCRLF     = FALSE;
   trmParams.columns     = ITM80COL;
   trmParams.termCursor  = ITMBLKCURSOR;
   trmParams.cursorBlink = TRUE;
   LoadString(hInst, STR_INI_BUFFER, (LPSTR) str, MINRESSTR);
   trmParams.bufferLines = GetProfileInt((LPSTR) szAppName_private, (LPSTR) str, DEFBUFFERLINES);
   maxChars = 0; /* mbbx 2.00.04 */

   DEBOUT("Calling: %s\n","resetTermBuffer()");
   resetTermBuffer();
   DEBOUT("Outof: %s\n","resetTermBuffer()");

   DEBOUT("Calling: %s\n","setDefaultFonts()");
   setDefaultFonts();
   DEBOUT("Outof: %s\n","setDefaultFonts()");

   DEBOUT("Calling: %s\n","icsResetTable()");
   icsResetTable(getDefCountry());           /* mbbx 1.04: ics */
   DEBOUT("Outof: %s\n","icsResetTable()");

   trmParams.fHideTermVSB = FALSE;
   trmParams.fHideTermHSB = FALSE;

/*   trmParams.useWinCtrl   = TRUE;	      rjs msoft */

// -sdj 08 may 92: I am not sure why cntl-c is not xmited
// and used instead for copy-paste. When terminal is used
// as a debug machine, or to connect to mainframe, it is important
// most of the times that the control-c should go out to the other end
// user can turn this other way if he needs to..
// changing default from TRUE to FALSE

     trmParams.useWinCtrl   = FALSE;

/* Binary Transfers: */
   trmParams.xBinType    = ITMXMODEM;
   trmParams.rcvBlSz     = 2000;
   trmParams.sendBlSz    = 2000;
   trmParams.retryCt     = 20;

/* Text Transfers: */
   trmParams.xTxtType    = ITMSTD;
   trmParams.xChrType    = ITMCHRDELAY;
   trmParams.xChrDelay   = 1;
   trmParams.xLinType    = ITMLINDELAY;
   trmParams.xLinDelay   = 1;
   LoadString(hInst, STR_XFERLINESTR, (LPSTR) trmParams.xLinStr, DCS_XLINSTRSZ);
   trmParams.xWordWrap   = FALSE;
   trmParams.xWrapCol    = 79;               /* mbbx 1.04: revert from 65 */

/* Phone: */
   trmParams.dlyRetry     = 30;              /* mbbx 1.10: CUA */
   trmParams.cntRetry     = 0;
   trmParams.flgRetry     = FALSE;
   trmParams.flgSignal    = FALSE;

/* Modem: */
   trmParams.xMdmType = ITMHAYES;
   LoadString(hInst, STR_DIALPREFIX, (LPSTR) trmParams.dialPrefix, DCS_MODEMCMDSZ);
   LoadString(hInst, STR_DIALSUFFIX, (LPSTR) trmParams.dialSuffix, DCS_MODEMCMDSZ);
   LoadString(hInst, STR_HANGPREFIX, (LPSTR) trmParams.hangPrefix, DCS_MODEMCMDSZ);
   LoadString(hInst, STR_HANGSUFFIX, (LPSTR) trmParams.hangSuffix, DCS_MODEMCMDSZ);
   LoadString(hInst, STR_ANSWER, (LPSTR) trmParams.answer, DCS_MODEMCMDSZ);
   LoadString(hInst, STR_ORIGINATE, (LPSTR) trmParams.originate, DCS_MODEMCMDSZ);

/* Environment: */
   if(fKeysShown)                            /* mbbx 2.00: show fkeys... */
      trmParams.environmentFlags |= DCS_EVF_FKEYSSHOW;
   else
      trmParams.environmentFlags &= ~DCS_EVF_FKEYSSHOW;
   trmParams.environmentFlags |= DCS_EVF_FKEYSARRANGE;

/* Parent: */

   for(ndx = 1; ndx <= 131; ndx++)
      if(ndx % 8 == 0)
         tabs[ndx] = 1;
      else
         tabs[ndx] = 0;

   answerMode    = FALSE;
   keyPadAppMode = FALSE;
   cursorKeyMode = FALSE;
}


/*---------------------------------------------------------------------------*/
/* resetEmul() - Load the Emulation table from resource.               [scf] */
/*---------------------------------------------------------------------------*/

VOID resetEmul()                             /* mbbx per slc */
{
#ifdef ORGCODE
   HANDLE      hResInfo, hFile;
   TEXTMETRIC  fontMetrics;
   INT         ndx, ndx2;

   if((hResInfo = FindResource(hInst, getResId(1000 + (trmParams.emulate - ITMTERMFIRST)), (LPSTR) DC_RES_CCTL)) != NULL) /* mbbx 1.04: REZ */
   {
      if((hFile = AccessResource(hInst, hResInfo)) != -1)
      {
         if(_read(hFile, emulInfo, 128) == 128)
		   {
            if(_read(hFile, GlobalLock(hemulKeyInfo), SIZEOFEMULKEYINFO) == SIZEOFEMULKEYINFO)
            {
               GlobalUnlock(hemulKeyInfo);

               if(_read(hFile, vidGraphChars, 128) != 128)   /* mbbx 1.10 ... */
                  trmParams.emulate = -1;

               if((trmParams.emulate == ITMVT100) || (trmParams.emulate == ITMVT220))
                  ansi = TRUE;
               else
                  ansi = FALSE;
            }
            else   /* read for emulKeyInfo failed so unlock it */
            {
               GlobalUnlock (hemulKeyInfo);
               trmParams.emulate = -1;
            }
         }
         else
            trmParams.emulate = -1;

         _close(hFile);
      }
      else
         trmParams.emulate = -1;
   }
   else
      trmParams.emulate = -1;

#else
   HANDLE      hFoundRes,hResInfo;
   LPSTR       lpResData,lpEmulKey;
   TEXTMETRIC  fontMetrics;
   INT         ndx, ndx2;

   if((hFoundRes = FindResource(hInst, getResId(1000 + (trmParams.emulate - ITMTERMFIRST)), (LPSTR) DC_RES_CCTL)) != NULL) /* mbbx 1.04: REZ */
   {
      DEBOUT("resetEmul: findresource returns %lx\n",hFoundRes);
      /*accessresource no longer in win32, so gotta LoadResource, then lock it, */
      /*so I get a pointer to the resource data itself.(JAP)*/
      if( (hResInfo = LoadResource(hInst,hFoundRes)) != NULL)
      {
         DEBOUT("resetEmul: LoadResource returns %lx\n",hResInfo);
         if((lpResData = LockResource(hResInfo)) )
         {
            DEBOUT("resetEmul: LockResource returns %lx\n",lpResData);
            memcpy(emulInfo, lpResData, 128);
            if ( (lpEmulKey = GlobalLock(hemulKeyInfo)) != NULL )
            {
               memcpy(lpEmulKey,lpResData+128,SIZEOFEMULKEYINFO);
               memcpy(vidGraphChars, lpResData + 128 + SIZEOFEMULKEYINFO, 128);
               if((trmParams.emulate == ITMVT100) || (trmParams.emulate == ITMVT220))
               {
                  DEBOUT("resetEmul:%s\n","emulate = VT100|VT52, ansi=true");
         	      ansi = TRUE;
		         }
               else
		         {
                  DEBOUT("resetEmul:%s\n","emulate not VT100|VT52, ansi=false");
         	      ansi = FALSE;
		         }
               GlobalUnlock(hemulKeyInfo);
               UnlockResource(hResInfo);
            }
            else  /* Globallock failed, so put -1 in .emulate */
            {
               DEBOUT("resetEmul: %s\n","GlobalLock FAILED");
               trmParams.emulate = -1;
               UnlockResource(hResInfo);
            }
         }
         else     /* LockResource failed, so put -1 in .emulate */
         {
            DEBOUT("resetEmul: %s\n","LockResource FAILED");
            trmParams.emulate = -1;
         }
      }
      else        /* LoadResource failed, so put -1 in .emulate */
      {
         DEBOUT("resetEmul: %s\n","LoadResource FAILED");
         trmParams.emulate = -1;
      }
   }
   else
   /* FindResource failed, so put -1 in .emulate  */
   {
      DEBOUT("resetEmul: %s\n","FindResource FAILED");
      trmParams.emulate = -1;
   }

#endif

   /*********** now check if .emulate is -1 and do the defaults ***********/
   if(trmParams.emulate == (BYTE) -1)
   {
      LoadString(hInst, STR_LOADEMUL, (LPSTR) taskState.string, 80);    /* mbbx 1.04: REZ... */
      testMsg(taskState.string,NULL,NULL);
      trmParams.emulate = ITMTTY;
      escHandler = pNullState;
   }
   else
   {
      escHandler = pEscSequence;
   
      getPort();
      GetTextMetrics(thePort, (TEXTMETRIC FAR *) &fontMetrics);
      releasePort();
      if(fontMetrics.tmCharSet == ANSI_CHARSET)
      {
         for(ndx = 0; ndx < 64; ndx += 1)
         {
            switch(vidGraphChars[ndx].buffer)
            {
            case 0x9C:
               vidGraphChars[ndx].buffer = 0xA3;
               break;
            case 0xF1:
               vidGraphChars[ndx].buffer = 0xB1;
               break;
            case 0xF8:
               vidGraphChars[ndx].buffer = 0xB0;
               break;
            case 0xFA:
               vidGraphChars[ndx].buffer = 0xB7;
               break;
            default:
               if(vidGraphChars[ndx].buffer > 0x80)
                  vidGraphChars[ndx].buffer = 0x20;
               break;
            }
         }
      }
   }

   clearModes();

   for(ndx2 = 0; ndx2 <= 127; ndx2 += 1)
   {
      ndx = emulInfo[ndx2];
      if(ndx > 128)
         ndx = ESCSKIPNDX;
      pEscTable[ndx2] = pProcTable[ndx];
      if(ansi)
         aEscTable[ndx2] = aProcTable[ndx];
   }

   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* initTermBuffer() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

/* mbbx 2.00.03: buffer lines > 399 ... */

/* NOTE: the following routines contain code which assumes the term buffer   */
/*       will be limited to 64K...  This seems like a reasonable limit       */
/*       since too large a buffer would slow down the emulation!             */

/* #define MINBUFFERLINES           25
#define MAXBUFFERSIZE            0xFFFF                          jtf 3.12 */
#define MINBUFFERLINES           (maxScreenLine+2)
#define MAXBUFFERSIZE            0x7FFF                         /* jtf 3.12 */
#define TERMBUFFERFUDGE          1

BOOL clearTermBuffer(WORD prevLines, WORD bufLines, WORD lineWidth)
{
   LPSTR    lpBufData;
   WORD     wPrevSize;

   if((lpBufData = GlobalLock(hTE.hText)) == NULL)
      return(FALSE);

   wPrevSize = prevLines * lineWidth;
   lsetmem(lpBufData += wPrevSize, 0x20, (bufLines * lineWidth) - wPrevSize);

   while(prevLines < bufLines)
   {
      lpBufData += lineWidth;
      *(((WORD FAR *) lpBufData)-1) = 0x0A0D;
      prevLines += 1;
   }
   *lpBufData = 0;                           /* NULL terminated, of course */

#ifndef BUGBYPASS
   DEBOUT("initTermBuffer: %s\n","GlobalUnlock BUG??? CHECK THIS OUT");
   return (TRUE);
#else
   GlobalUnlock(hTE.hText);
   return (TRUE);
#endif
}


BOOL initTermBuffer(WORD bufLines, WORD lineWidth, BOOL bReset)
{
   LONG     lBufSize;
   LPSTR    lpBufData;
   HANDLE   hNewBuf;

   if(bReset && (hTE.hText != NULL))
      hTE.hText = GlobalFree(hTE.hText);

   if(bufLines < MINBUFFERLINES)
      bufLines = MINBUFFERLINES;
   if((lBufSize = ((LONG) bufLines * lineWidth) + TERMBUFFERFUDGE) > MAXBUFFERSIZE)
   {
      bufLines = (MAXBUFFERSIZE - TERMBUFFERFUDGE) / lineWidth;
      lBufSize = (bufLines * lineWidth) + TERMBUFFERFUDGE;
   }

   if(hTE.hText == NULL)
   {
      GlobalCompact(lBufSize);

      if((hTE.hText = GlobalAlloc(GMEM_MOVEABLE, (DWORD) (MINBUFFERLINES * lineWidth) + 
                                  TERMBUFFERFUDGE)) == NULL)
      {
         testResMsg(STR_OUTOFMEMORY);
         SendMessage(hItWnd, WM_CLOSE, 0, 0L);
         return(FALSE);
      }

      maxLines = 0;
   }
   else if(bufLines < savTopLine + (maxScreenLine + 2))
   {
      lpBufData = GlobalLock(hTE.hText);
      lmovmem(lpBufData + ((savTopLine + (maxScreenLine + 2) - bufLines) * lineWidth), lpBufData, lBufSize);
      GlobalUnlock(hTE.hText);

      if(curTopLine > (savTopLine = bufLines - (maxScreenLine + 2)))
         curTopLine = savTopLine;
      if(!IsIconic(hItWnd))      /* rjs bugs 015 */
         sizeTerm(0L);                          /* reset scrollbars */
   }

   while(bufLines > MINBUFFERLINES)
   {
      if((hNewBuf = GlobalReAlloc(hTE.hText, lBufSize, GMEM_MOVEABLE)) != NULL)
      {
         hTE.hText = hNewBuf;
         break;
      }

      bufLines -= 1;
      lBufSize -= lineWidth;
   }

   if(bufLines > maxLines)
      clearTermBuffer(maxLines, bufLines, lineWidth);

   maxLines = bufLines;

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* resetTermBuffer() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

VOID resetTermBuffer()
{
   WORD  lineWidth;
   BOOL  bNewWidth;

   lineWidth = (trmParams.columns == ITM80COL) ? 80 : 132;
   if((bNewWidth = (lineWidth != maxChars)) || (trmParams.bufferLines != maxLines))
   {
      maxChars = lineWidth;
      initTermBuffer(trmParams.bufferLines, lineWidth + 2, bNewWidth);

      if(bNewWidth)
         clearBuffer();
   }
}
