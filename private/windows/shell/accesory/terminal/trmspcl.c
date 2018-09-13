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

/*---------------------------------------------------------------------------*/
/* getScrCh() -                                                         [scf]*/
/*---------------------------------------------------------------------------*/

BOOL getScrCh ()
{
   BOOL result;

   if (textIndex < textLength)
   {
      rdCH[1] = textPtr[textIndex];
      textIndex++;
      result = TRUE;
   }
   else
      result = FALSE;
   return TRUE;
}


/*---------------------------------------------------------------------------*/
/* stripLeadingSpaces() -                                               [scf]*/
/*---------------------------------------------------------------------------*/

VOID stripLeadingSpaces ()
{
   INT  i;
   BYTE whiteSpace[20];

   sprintf (whiteSpace," %c%c",CHFILL,TAB);
   i = outBuf[0] - 1;
   while (strchr (whiteSpace, outBuf[i]))
   {
      outBuf[i] = CR;
      outBuf[0]--;
      i--;
   }
}


/*---------------------------------------------------------------------------*/
/* getChxmit() - Get xmit char. from clipBoard or file.                 [scf]*/
/*---------------------------------------------------------------------------*/

BOOL getChxmit()
{
   BOOL     result = FALSE;
   INT      err;
   STRING   str[2];
   BYTE     whiteSpace[20];
   DWORD    charsRead;

   if(!trmParams.xWordWrap || (xferFlag == XFRTYP) || (scrapSeq && copiedTable))
   {
      if(scrapSeq)
      {
         if(getScrCh())
            result = TRUE;
      }
      else
      {
         charsRead = _lread((int)xferRefNo, (LPSTR)(&rdCH[1]), ioCnt);
         if(charsRead == (DWORD)ioCnt)
            result = TRUE;
         else if(charsRead != 0)
         {
            if(charsRead != (DWORD)-1)
               charsRead = 0;
            else
               result = TRUE;
         }
         else
            result = FALSE;
      }
      outBufSeq = FALSE;
      return(result);
   }

   if(!outBufSeq)
   {
      repeat
      {
         if(scrapSeq)
         {
            if(getScrCh())
               err = ioCnt;
            else
               err = ioCnt + 1;
         }
         else
         {
            err = _lread(xferRefNo, (LPSTR)(&rdCH[1]), ioCnt);
         }

         if(err == ioCnt)
         {
            str[0] = 0x01; str[1] = rdCH[1];
            concat (outBuf, str, outBuf);
            outBufCol++;
            if (rdCH[1] == CR)
               outBufCol = 0;
            else if (rdCH[1] == LF)
               outBufCol--;
            else if(outBufCol > trmParams.xWrapCol) /* WRN as outBufCol is BOOL*/
            {                                       /* and xWrapCol is INT */
               xferBytes++;
               str[1] = LF;
               concat (outBuf, outBuf, str);
               xferBytes++;
               str[1] = CR;
               concat (outBuf, outBuf, str);
               outBufSeq = TRUE;
               stripLeadingSpaces ();
               outBufCol = (INT) outBuf[0];
            }
            sprintf(whiteSpace," %c%c%c%c",TAB,CHFILL,CR,LF);
            if (strchr(whiteSpace,rdCH[1]))
               outBufSeq = TRUE;
         }
         else
            if (*outBuf == 0)
               return result;
            else
               outBufSeq = TRUE;
      }
      until(outBufSeq);
   }

   if(outBufSeq)
   {
      ioCnt = 1;
      result = TRUE;
      rdCH[1] = outBuf[outBuf[0]];
      if (*outBuf > 1)
         outBuf[0]--;
      else
      {
         outBufSeq = FALSE;
         outBuf[0] = 0;
      }
   }

   return(result);
}


/*---------------------------------------------------------------------------*/
/* xferFile() - Transfer & Receive text file handling.           [scf] [mbb] */
/*---------------------------------------------------------------------------*/

/* NOTE: xferFile no longer masks off the high order bit of the
   chars sent -- I don't see how they could have gotten SET in the first place
   whether they came from a file or the clipboard. [mbbx 1.02] */

VOID xferFile()
{
   BOOL  breakXfer;
   int   ndx;

   breakXfer = (xferPaused || xferStopped);

   if(xferEndTimer > 0)
      if(xferEndTimer > tickCount())  /* WRN xferEndTimer is LONG and */
         breakXfer = TRUE;            /* and GetCurrentTime returns DWORD*/
      else                            /* but was same in 3.0 too -sdj */
         xferEndTimer = 0;

   if(xferWaitEcho)
      breakXfer = TRUE;


   while(!breakXfer)
   {
      rxEventLoop();

      ioCnt = 1;

      if((xferViewPause == 0) &&             /* mbbx: auto line count */
         (((xferFlag == XFRTYP) ) || 
          ((xferFlag == XFRSND) && !trmParams.xWordWrap &&     /* mbbx 1.03: lineWrap -> xWordWrap */
           (xferTxtType == XFRNORMAL))))
      {
         if((ioCnt = YIELDCHARS) > xferBytes)
            ioCnt = xferBytes;
      }

      if(xferBytes > 0)
      {
         if(getChxmit())
         {
            for(ndx = 1; ndx <= ioCnt; ndx++)
            {
                                             /* mbbx 1.10: set rdCh[0] to LF if appending LF's... */
               if(((theChar = rdCH[ndx]) != CR) || (rdCH[0] != CR))  /* mbbx 1.02: CR CR LF -> CR */
               {
                  rdCH[0] = theChar;         /* mbbx 1.02: save char... */

                  if((theChar != LF) || (xferSndLF != -1))  /* mbbx: strip LF's */
                  {
                     if(xferFlag == XFRTYP)
                        modemInp(theChar, FALSE);
                     else
                        modemWr(theChar);
                  }

                  if (xferStopped) 
                     break; /* jtf 3.30 */

                  if(((theChar == CR) && (xferSndLF != 0)) || ((theChar == LF) && (xferSndLF == 0)))
                  {
                     if(xferSndLF == 1)                    /* mbbx: append LF's */
                     {
                        if(xferFlag == XFRTYP)
                           modemInp(LF, FALSE);
                        else
                           modemWr(LF);
                        rdCH[0] = LF;        /* mbbx 1.10: last char sent = LF */
                     }

                     if(xferViewPause > 0)
                     {
                        if(++xferViewLine >= xferViewPause)
                        {
                           xferViewLine = 0;
                           termCleanUp();
                           xferPauseResume(TRUE, FALSE);    /* mbbx 2.00: xfer ctrls... */
                        }
                     }
                  }
               }
               else
                  theChar = 0;               /* mbbx 1.10: avoid 2 line waits on CR CR LF */

               xferBytes -= 1;
               if(!scrapSeq)
                  updateProgress(FALSE);
            }
         }
         else
            xferBytes = 0;
      }

      if((theChar == CR) || (xferBytes == 0) || (ioCnt > 1) || xferPaused ||
         (xferTxtType == XFRCHAR) || scrapSeq)
         break;                              /* mbbx: DO NOT set breakXfer !!! */
   }

   if(xferBytes == 0)
   {
      if(scrapSeq)
      {
         scrapSeq = FALSE;
         GlobalUnlock(tEScrapHandle);
         tEScrapHandle = GlobalFree(tEScrapHandle);
         if(!IsIconic(hItWnd))
            SetCursor(LoadCursor(NULL, IDC_ARROW));
         xferBytes = 1L; /* Need so pasting during a Rcv. Text doesn't terminate */
      }
      else
      {
         termCleanUp();                      /* mbbx: finish term update */
         xferEnd();
      }
   }

   if(((xferFlag == XFRSND) || scrapSeq) && !breakXfer)
   {
      if(xferTxtType == XFRCHAR)
      {
         if(xferChrType == XFRCHRDELAY)
            xferEndTimer = tickCount() + (xferChrDelay * 6);
         else
         {
            xferWaitEcho = TRUE;
            xferCharEcho = theChar; /* jtf 3.30 allow ansi text & 0x7f; */
         }
      }
      else if((xferTxtType == XFRLINE) && (theChar == CR))
      {
         if (xferLinType == XFRLINDELAY)
            xferEndTimer = tickCount() + (xferLinDelay * 6);
         else
            xferWaitEcho = TRUE;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* termSpecial() -                                                      [scf]*/
/*---------------------------------------------------------------------------*/

VOID termSpecial()
{
   RECT     buttonRect;
   HDC      hButtonDC;
   BOOL     breakTerm;
   BOOL     breakKey;  /* jtf 3.20 */
                                             /* mbbx: these WERE static vars */
   STRING   execStr[STR255];
   INT      execNdx;
   INT      brkLen;
   LONG     finalTicks;

   breakKey  = FALSE;  /* jtf 3.20 */
   breakTerm = FALSE;

   if(useScrap)
   {
      if((textPtr = GlobalLock (tEScrapHandle)) != NULL)
         xferBytes = textLength = (LONG) lstrlen(textPtr);
      else
         xferBytes = textLength = 0;

      textIndex  = 0;
      useScrap   = FALSE;
      scrapSeq   = TRUE;
      xferSndLF  = (trmParams.outCRLF ? 0 : -1);   /* mbbx 1.10: CUA */
      *outBuf    = 0;

      if(trmParams.xWordWrap && !copiedTable)
      {
         outBufCol   = 0;
         outBufSeq   = FALSE;
         xferBlkSize = 1;
      }

      xferTxtType  = (TTXTTYPE) (trmParams.xTxtType - ITMSTD);
      xferChrType  = (TCHRTYPE) (trmParams.xChrType - ITMCHRDELAY);
      xferLinType  = (TLINTYPE) (trmParams.xLinType - ITMLINDELAY);
      xferChrDelay = trmParams.xChrDelay;
      xferLinDelay = trmParams.xLinDelay;
      if (!IsIconic (hItWnd))
         SetCursor (LoadCursor (NULL, IDC_WAIT));
   }
  /* WRN fKeyNdx is INT and *fKeyStr is BYTE -sdj */
   if(fKeyNdx <= *fKeyStr)                   /* mbbx 2.00: dc terminal... */
   {
      if((theChar = fKeyStr[fKeyNdx++]) == '^')
      {
         if((fKeyNdx <= *fKeyStr) && ((theChar = fKeyStr[fKeyNdx++]) == '$'))
            theChar = 0xFF;
         else if(theChar != '^')
         {
            if(theChar == '~')
               theChar = '^';                /* NOTE: must use ^~ to send 0x1E */

            if(((theChar >= 'A') && (theChar <= '_')) || ((theChar >= 'a') && (theChar <= 'z')))
               theChar &= 0x1F;
            else
               theChar = fKeyStr[(--fKeyNdx)-1];
         }
      }

      if(theChar != 0xFF)
      {
         sendKeyInput(theChar);              /* mbbx: per slc request */

      }
      else                                   /* special ctrl strings... */
      {
         if(((theChar = fKeyStr[fKeyNdx++]) >= 'a') && (theChar <= 'z'))
            theChar -= 0x20;

         while((fKeyNdx <= *fKeyStr) && (fKeyStr[fKeyNdx] == ' '))
            fKeyNdx++;

         switch(theChar)
         {
         case 'L':                           /* ^$L - level */
            setFKeyLevel(fKeyStr[fKeyNdx++] - '0', FALSE);  /* mbbx 2.00: bReset */
            break;

         case 'C':                           /* mbbx 2.00: ^$C - call... */
            dialPhone();
            break;
         case 'H':                           /* mbbx 2.00: ^$H - hangup... */
            hangUpPhone();
            break;

         case 'B':                           /* ^$B - break */
            breakKey = TRUE;   /* jtf 3.20 */
         case 'D':                           /* mbbx 1.04: ^$D - delay */
            brkLen = 2;
            if(((theChar = fKeyStr[fKeyNdx]) >= '0') && (theChar <= '9'))
            {
               brkLen = theChar - '0';
               if(((theChar = fKeyStr[++fKeyNdx]) >= '0') && (theChar <= '9'))
               {
                  fKeyNdx += 1;
                  brkLen = (brkLen * 10) + (theChar - '0');
               }
            }

            if(breakKey)   /* jtf 3.20 */
               modemSendBreak(brkLen);
            else
               delay(brkLen*60, &finalTicks);   /* mbbx 1.04: ^$D <secs> */
            break;
         }
      }
   }

   if(!breakTerm)
   {
      switch(xferFlag)
      {
      case XFRSND:
      case XFRTYP:
         if(!gbXferActive)
         {
            gbXferActive = TRUE;
            xferFile();
            gbXferActive = FALSE;
         }
         break;
      case XFRRCV:
         if(!gbXferActive)
         {
            gbXferActive = TRUE;
            if(xferBytes == 0)                  /* stop button was clicked */
               xferEnd();
            gbXferActive = FALSE;
         }
         break;
      default:
         if(!gbXferActive)
         {
            gbXferActive = TRUE;
            if(scrapSeq)
               xferFile();
            gbXferActive = FALSE;
         }
         break;
      }
   }

   if(!breakTerm)
      rdModem(FALSE);
}
