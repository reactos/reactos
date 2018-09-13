/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
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
#define  USECOMM

#include <stdarg.h>
#include <windows.h>
#include <port1632.h>
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "connect.h"

/*---------------------------------------------------------------------------*/
/* mdmConnect() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

/* NOTE: PATCH until WIN COMM DRV is fixed!!! */

#define DEB_MSR_OFFSET           35          /* mbbx 1.10: carrier... */
#define DEB_MSR_RLSD             0x80

BOOL mdmConnect()                            /* mbbx 2.00: network... */
{
   BOOL     bRc,bCarrier = FALSE;
   // -sdj unreferenced local var: LPBYTE	lpMSR;
   DWORD    dwModemStatus;

   if(trmParams.fCarrier)
   {
      switch(trmParams.comDevRef)
      {
      case ITMWINCOM:
        DEBOUT("mdmConnect: %s\n","Calling getmodemstatus to see rlsd!");
        bRc = GetCommModemStatus(sPort,&dwModemStatus);
        DEBOUT("mdmConnect: rc of getmodemstatus = %lx\n",bRc);
        DEBOUT("mdmConnect: dw of getmodemstatus = %lx\n",dwModemStatus);
        if (!bRc)
        {
            DEBOUT("mdmconnect: %s\n","getmodemstatus failed, setting bCar=TRUE");
            bCarrier = TRUE;
	    // BUGBUG! see below break;
        }
        else
        {
            bCarrier = (dwModemStatus & MS_RLSD_ON) ? TRUE : FALSE;
            DEBOUT("mdmconnect: bCarrier is set as:  %lx\n",bCarrier);
        }
	break; // -sdj fix bug#735, break was not invoked in the else path
	       // causing default: stmt to exec all the time, and keeping
	       // bCarrier to be set to TRUE once the user sets it to true.

      default:
         bCarrier = TRUE;
         break;
      }

      if(mdmOnLine != bCarrier)
      {
         if(!(mdmOnLine = bCarrier))
         {
         }

         return(TRUE);
      }
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* modemReset() - Send XON character to the serial port.               [mbb] */
/*---------------------------------------------------------------------------*/

VOID modemReset()                            /* mbbx 2.00: network... */
{
   switch(trmParams.comDevRef)
   {
   case ITMWINCOM:
      switch(trmParams.flowControl)
      {
      case ITMXONFLOW:
         DEBOUT("modemReset: Esccom(SETXON) on comport=%lx\n",sPort);
         EscapeCommFunction(sPort, SETXON);
         break;
      case ITMHARDFLOW:
         DEBOUT("modemReset: Esccom(SETRTS) on comport=%lx\n",sPort);
         EscapeCommFunction(sPort, SETRTS);
         break;
      }
      break;

   }

   sPortErr = FALSE;
}


/*---------------------------------------------------------------------------*/
/* modemBreak() - Send BREAK signal to the serial port.                [mbb] */
/*---------------------------------------------------------------------------*/

/* NOTE: units for modemSendBreak are approx. 1/9 secs ( = 7 ticks)          */
/*---------------------------------------------------------------------------*/
/* VT100 standards for BREAK signals are as follows:                         */
/*                                                                           */
/*       short break:         0.233 sec   =    2 units                       */
/*        long break:         3.500 sec   =   30 units                       */
/*                                                                           */

VOID modemSendBreak(INT   nCount)
{
   DCB      dcb;                             /* slc nova 051 */

   switch(trmParams.comDevRef)
   {
   case ITMWINCOM:
      if(nCount > 2)                         /* slc nova 051 moved... */
      {
         if(GetCommState(sPort, (DCB FAR *)&dcb) == 0)   /* slc nova 051 */
         {
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
            dcb.fDtrControl = DTR_CONTROL_DISABLE;
            EscapeCommFunction(sPort,CLRRTS);
            EscapeCommFunction(sPort,CLRDTR);

            if(!SetCommState(sPort,(DCB FAR *)&dcb))
                {
                }
         }
      }

DEBOUT("modemSendBrk:EscapeCommFunction(sPort, SETBREAK), DelayStart..for port=%lx\n",sPort);
      EscapeCommFunction(sPort, SETBREAK);
      delay(nCount*7, NULL);
DEBOUT("modemSendBrk:DelayOver...EscapeCommFunction(sPort, CLRBREAK)for port=%lx\n",sPort);
      EscapeCommFunction(sPort, CLRBREAK);

      if(nCount > 2)                         /* slc nova 051 moved... */
      {
         if(GetCommState(sPort, (DCB FAR *)&dcb) == 0)   /* slc nova 051 */
         {
            dcb.fRtsControl  = RTS_CONTROL_ENABLE;
            dcb.fDtrControl  = DTR_CONTROL_ENABLE;
            EscapeCommFunction(sPort,SETRTS);
            EscapeCommFunction(sPort,SETDTR);
  DEBOUT("modemSendBreak: set fRtsDtrdisable to false: for port=%lx\n",sPort);
            if(!SetCommState(sPort,(DCB FAR *)&dcb))
                {
  DEBOUT("FAIL: modemSendBreak: set fRtsDtrdisable to false: for port=%lx\n",sPort);
                }
         }
      }
      break;

   case  ITMDLLCONNECT:                      /* rjs bug2 */
      DLL_modemSendBreak(ghCCB, nCount);
      break;

   }/* switch */

   resetSerial(&trmParams, FALSE, FALSE, 0);
}


/*---------------------------------------------------------------------------*/
/* modemBytes() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR WIN_modemBytes()                   /* mbbx 2.00: network... */
{
   // -sdj unreferenced local var: INT	i;
   LPBYTE      ltmp;
   COMSTAT     serInfo;
   DWORD       dwErrors;
   DWORD       dwBytesRead;
   OVERLAPPED  overlap;
   BOOL        bRc;

   overlap.hEvent       = overlapEvent;
   overlap.Internal     = 0;
   overlap.InternalHigh = 0;
   overlap.Offset       = 0;
   overlap.OffsetHigh   = 0;

   ResetEvent(overlapEvent);

   gotCommEvent = FALSE;

   bRc = ReadFile(sPort, serBytes+1, LOCALMODEMBUFSZ-1,
                 (LPDWORD)&serCount, (LPOVERLAPPED)&overlap);

   if(!bRc && ((dwErrors = GetLastError()) != ERROR_IO_PENDING))
   {
      bRc = ClearCommError(sPort, &dwErrors, &serInfo); /* reset after error */

      if(trmParams.flowControl == ITMHARDFLOW)
      {
         if(serInfo.cbInQue < 100)
         {
            modemReset();
         }
      }

      if(serInfo.fXoffSent || serInfo.fCtsHold || serInfo.fDsrHold)
      {
         if(serInfo.cbInQue < 100)
         {
            modemReset();
         }
      }
   }
   else
   {
      if(!bRc && ((dwErrors = GetLastError()) == ERROR_IO_PENDING))
      {
         bRc = ClearCommError(sPort, &dwErrors, &serInfo);
      }

      if(!GetOverlappedResult(sPort, &overlap, &dwBytesRead, FALSE))
         return;

      if(serCount = dwBytesRead)
      {
         ltmp = serBytes+1;

         if(serCount != LOCALMODEMBUFSZ-1)
         {
            bRc = ClearCommError(sPort, &dwErrors, &serInfo); /* reset after error */

            if(trmParams.flowControl == ITMHARDFLOW)     /* rjs bug2 003 */
            {
               if(serInfo.cbInQue < 100)
               {
                  modemReset();
               }
            }

            if(serInfo.fXoffSent || serInfo.fCtsHold || serInfo.fDsrHold)
            {
               if(serInfo.cbInQue < 100)
               {
                  modemReset();
               }
            }
	 }  /* if readfile cameout halfway through */
	 else
	 {
	    gotCommEvent = TRUE;   /* we read a full buffer, try again..*/
	 }

      }
   } /* readfile succeded lets see the bytes read */
}


WORD modemBytes()                            /* mbbx 2.00: network... */
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;	     /* slc nova 031 */
   BYTE  tmp1[TMPNSTR+1];
   BYTE  tmp2[TMPNSTR+1];



   if(serNdx > 0)
      return(serCount - (serNdx-1));

   switch(trmParams.comDevRef)
   {
   case ITMWINCOM:


   //-sdj comments from checkcommevent():
   //-sdj for telnet-quit processing
   //-sdj if this is a telnet connection which was opened before and
   //-sdj the user hits cntl-c/bye/quit etc
   //-sdj the telnet service will stop talking
   //-sdj with us, but terminalapp still keeps
   //-sdj doing io without knowing that the handle
   //-sdj can only be closed now, The way we can
   //-sdj detect this is, to check if getlasterror
   //-sdj is ERROR_NETNAME_DELETED, if this is the
   //-sdj case then we should do exactly same thing
   //-sdj which we do when the user tries to go to
   //-sdj some other comm port, close this one, and
   //-sdj go to the next one.
   //-sdj by setting bPortDisconnected to TRUE,
   //-sdj further modemBytes()[reads] will stop on
   //-sdj this port, and modemBytes will prompt the
   //-sdj user to select some other port, and return.


   if (bPortDisconnected)
      {
      LoadString(hInst, STR_PORTDISCONNECT, (LPSTR) tmp1, TMPNSTR);
      LoadString(hInst, STR_ERRCAPTION, (LPSTR) tmp2, TMPNSTR);
      MessageBox(hItWnd, (LPSTR) tmp1, (LPSTR)tmp2, MB_OK | MB_APPLMODAL);
      serCount = 0;  //so that return dword is 0, no chars to process
      // resetSerial(&trmParams, FALSE,TRUE,0);
      if(!trmParams.fResetDevice)
	 trmParams.newDevRef = trmParams.comDevRef;
      exitSerial();
      doSettings(IDDBCOMM, dbComm);
      break;
      }

       if(gotCommEvent)
      {
         WIN_modemBytes();
      }
      else
         serCount = 0;

      break;

   case ITMDLLCONNECT:                       /* slc nova 012 bjw nova 002 */
      serCount = DLL_ConnectBytes(ghCCB);    /* slc nova 031 */
      if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(ghCCB)) != NULL) /* slc nova 031 */
      {
         if(serCount == CONNECT_READ_ERROR)
            serCount = lpCCB->wReadBufferRead;

         lmovmem((LPSTR)lpCCB->lpReadBuffer, (LPSTR)(serBytes + 1), (WORD)serCount);
         GlobalUnlock(ghCCB);
      }
      break;
   }

   if(serCount > 0)
      serNdx = 1;                            /* indicates chars to process */

   return(serCount);
}


/*---------------------------------------------------------------------------*/
/* getMdmChar() - Get a modem character out of local buffer.           [mbb] */
/*---------------------------------------------------------------------------*/

/* NOTE:  modemBytes() must be called prior to this routine */

BYTE getMdmChar(BOOL  bText)
{
   BYTE nextChar;

   nextChar = serBytes[serNdx++];
   if(serNdx > serCount)
      serNdx = 0;

   if(trmParams.parity != ITMNOPARITY)
      nextChar &= 0x7F;

   if(bText && (trmParams.language > ICS_NONE) && (termState == NULL))  /* mbbx 1.06A: ics new xlate... */
   {
      if(nextChar >= 0x80)                   /* slc swat */
      {
         if(trmParams.setIBMXANSI)
            nextChar = ansiXlateTable[nextChar];   /* IBM extended to ANSI */
      }
      else
      {
         if(trmParams.language > ICS_NONE)
            nextChar = icsXlateTable[nextChar];    /* ISO char to ANSI */
      }
   }

   return(nextChar);
}


/*---------------------------------------------------------------------------*/
/* getRcvChar() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL getRcvChar(BYTE  *theChar, BYTE  charMask)
{
   if(modemBytes())
   {
      *theChar = getMdmChar(FALSE);

      if(charMask != 0)
         *theChar &= charMask;

      return(TRUE);
   }

   *theChar = 0;
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* waitRcvChar() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

BOOL waitRcvChar(BYTE  *theChar, WORD  timeOut, BYTE  charMask, BYTE  charFirst, ...)
{
   va_list  ap;
   DWORD    waitTicks; //-sdj was LONG, getcurrenttime,tickcount returns dword
		       //-sdj this was causing a sign/unsign warning noise
   BYTE     charList;

   waitTicks = tickCount() + (timeOut * 6);
   repeat
   {
      va_start(ap,charFirst);
      updateTimer();

      if(doneFlag)
      {
         xferStopped = TRUE;
         va_end(ap);
         return(FALSE);
      }

      if(xferStopped)
         break;

      gotCommEvent = TRUE;

      if(getRcvChar(theChar, charMask))
      {

         if(charFirst == 0)
         {
            va_end(ap);
            return(TRUE);

          }

         for(charList = charFirst; charList != 0; charList = va_arg(ap,BYTE))
         {
            if(charList == *theChar)
            {
               va_end(ap);
               return(TRUE);

            }
         }

         *theChar = 0;
      }

      //if(PeekMessage((LPMSG) &msg, NULL, 0, 0, PM_NOREMOVE))
      //   mainEventLoop();
      //sdj: now that rcv and snd b file are threads the main
      //sdj: thread will deal with UI while the xfer is going on, so
      //sdj: no need of this hack to peek the msges.
      //sdj: but lets put sleep of 10ms so that we dont hog the CPU
      //sdj: and give chance for the buffer to fill and reduce number
      //sdj: of	calls to ReadFile()

      Sleep(10);

   }
   until(tickCount() >= waitTicks);

   va_end(ap);
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* flushRBuf() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

VOID flushRBuff()
{
   if(modemBytes())
      delay(6, NULL);                        /* mbbx: wtf??? */

   while(modemBytes())
      serNdx = 0;                            /* mbbx: dump serBytes data */
}


/*---------------------------------------------------------------------------*/
/* checkUserAbort() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/
BOOL checkUserAbort()
{
   BOOL checkUserAbort = FALSE;

   return(FALSE);

   if(PeekMessage((LPMSG) &msg, hdbXferCtrls, 0, 0, PM_REMOVE))
   {
      if((msg.hwnd != xferCtlStop)     ||
         (msg.message < WM_MOUSEFIRST) ||
         (msg.message > WM_MOUSELAST))
      {
         IsDialogMessage(hdbXferCtrls, (LPMSG) &msg);
      }
   }

   while(PeekMessage((LPMSG) &msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
   {
      if((msg.message == WM_KEYDOWN) && (msg.wParam == VK_CANCEL))
         checkUserAbort = TRUE;
   }

   return(checkUserAbort);
}


/*---------------------------------------------------------------------------*/
/* modemWrite() - Send data to comm port (no special processing here)  [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR WIN_modemWrite(LPSTR lpData, INT nSize)
{
   BOOL        modemWrite = TRUE;
   BOOL        bWriteFile;
   INT         nRemain;
   INT         nBytes;
   INT         nSent;
   COMSTAT     serInfo;
   // -sdj unreferenced local var: BYTE	str[80];
   DWORD       dwErrors;
   OVERLAPPED  overlap;
   BOOL        bRc;

   overlap.hEvent       = overlapEvent;
   overlap.Internal     = 0;
   overlap.InternalHigh = 0;
   overlap.Offset       = 0;
   overlap.OffsetHigh   = 0;

   for(nRemain = nSize; nRemain > 0; nRemain -= nSent)
   {
      nBytes = nRemain;

      bWriteFile = WriteFile(sPort,  (LPVOID)  (lpData+(nSize-nRemain)),
                             nBytes, (LPDWORD) &nSent,
                             (LPOVERLAPPED)&overlap);

      dwErrors = GetLastError();

      if ((!bWriteFile) && (dwErrors != ERROR_IO_PENDING))
      {
         bRc = ClearCommError(sPort, &dwErrors, &serInfo);

         if(serInfo.fXoffSent || serInfo.fCtsHold)
         {
            if(serInfo.fCtsHold)
            {
               sPortErr = TRUE;
               return(FALSE);
            }

            rxEventLoop(); /* jtf 3.20 */

            if ( (xferStopped == TRUE) && (xferFlag != XFRNONE) ) /* jtf 3.33 3.30 */
            {
               modemWrite = TRUE;
               return(modemWrite);  /* jtf 3.30 */
            }

            if(checkUserAbort())             /* mbbx: see if CTRL BREAK hit */
            {
               switch(trmParams.flowControl)    /* mbbx 1.10: CUA... */
               {
               case ITMXONFLOW:
                  modemReset();
                  break;

               case ITMHARDFLOW:             /* drastic ... */
                  trmParams.flowControl = ITMNOFLOW;
                  resetSerial(&trmParams, FALSE, FALSE, 0);
                  break;
               }
               modemWrite = FALSE;
               break;
            }/* if checkUserAbort */
         }/* if serInfo.hold */
      }/* if writeComm */
      else
      {
         if(!bWriteFile)
            if(dwErrors != ERROR_IO_PENDING)
            {
               bRc = ClearCommError(sPort, &dwErrors, &serInfo);
               modemWrite = FALSE;
               nSent = 0;
            }
            else
	    {
	       if(WaitForSingleObject(overlapEvent, dwWriteFileTimeout) == 0)
               {
                  GetOverlappedResult(sPort, &overlap, (LPDWORD)&nSent, TRUE);
               }
               else
               {
                  ResetEvent(overlapEvent);
                  bRc = ClearCommError(sPort, &dwErrors, &serInfo);
                  nSent = 0;
               }
            }

#ifdef SLEEP_FOR_CONTEXT_SWITCH

	 Sleep((DWORD)5);
#endif

//         if(!nSent)
//         {
//            modemWrite = FALSE;
//            break;
//         }
      }
   }/* for */

   if(xferBreak)                             /* mbbx 2.00: xfer ctrls... */
   {
      setXferCtrlButton(IDSTOP, STR_STOP);
      xferBreak = FALSE;
   }

   return(modemWrite);
}/* WIN_modemWrite */


/* ----------------------------------------------------------------------- */


BOOL modemWrite(LPSTR lpData, INT nSize)
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;               /* slc nova 031 */
   BOOL                       bResult = FALSE;     /* slc swat */

   if (nSize == 0)    /* mbbx 2.00.04: check outgoing buffer... */
   {
      switch(trmParams.comDevRef)
      {
      case ITMDLLCONNECT:                       /* slc nova 028 */
         if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(ghCCB)) != NULL)
         {
            lpCCB->wWriteBufferUsed = nSize;    /* slc nova 031 */
            GlobalUnlock(ghCCB);
            DLL_WriteConnector(ghCCB);
         }
         break;
      case ITMWINCOM:
      default:
         break;
      }
      return(TRUE);
   }


   /* We cannot allow recursive calls to the modemWrite() sub,
      cuz we stack overflow!  Design issue: caller should check
      this return value and re-call with same data!
   */

   if(bgOutStandingWrite)                    /* slc swat */
   {
      sysBeep();
      return(FALSE);
   }
   else
      bgOutStandingWrite = TRUE;


   switch(trmParams.comDevRef)
   {
   case ITMWINCOM:
      bResult = WIN_modemWrite(lpData, nSize);
      break;

   case ITMDLLCONNECT:                       /* slc nova 012 bjw nova 002 */
      if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(ghCCB)) != NULL) /* slc nova 031 */
      {
         if(lpCCB->lpWriteBuffer)               /* seh nova 005 */
         {
            lmovmem((LPSTR)lpData, (LPSTR)lpCCB->lpWriteBuffer, (WORD)nSize);  /* seh nova 005 */
            lpCCB->wWriteBufferUsed = nSize;    /* seh nova 005 */

            GlobalUnlock(ghCCB);                /* slc nova 031 */
            bResult = DLL_WriteConnector(ghCCB);
         }
      }
      break;

   }/* switch */

   bgOutStandingWrite = FALSE;               /* slc swat */

   return(bResult);
}


/*---------------------------------------------------------------------------*/
/* modemWr() - Send character to the windows serial port driver.       [mbb] */
/*---------------------------------------------------------------------------*/

VOID modemWr(BYTE  theByte)
{
   BYTE  saveByte = theByte;
   BYTE  ISOByte;

   if((theByte >= 0x80) && (xferFlag < XFRBSND))   /* mbbx 1.10: VT220 8BIT... */
   {
      if(trmParams.language > ICS_NONE)
         ISOByte = icsXlateTable[theByte];
      else
         ISOByte = theByte;

      if(trmParams.setIBMXANSI)
      {
         if(ISOByte >= 0x80)  /* was not ISO */
            theByte = ansiXlateTable[theByte & 0x7F]; /* ANSI to IBM extended */
      }
      else
         theByte = ISOByte;
   }
   else if((theByte == CR) && (trmParams.emulate == ITMDELTA))
      theByte = XOFF;

   if(!modemWrite((LPSTR) &theByte, 1))
      return;

   if(trmParams.localEcho && (xferFlag < XFRTYP))
   {
      modemInp(saveByte, FALSE);
      if((theByte == CR) && (xferFlag == XFRSND))
         modemInp(LF, FALSE);
   }

   if(trmParams.outCRLF && (theByte == CR) && (xferFlag == XFRNONE))    /* mbbx 2.00: heed outCRLF... */
      modemWr(LF);                           /* yikes!  it's recursive!!! */
}


/*---------------------------------------------------------------------------*/
/* termStr() - Send PASCAL character string to the modem.              [mbb] */
/*---------------------------------------------------------------------------*/

VOID termStr(STRING *tStr, INT nDelay, BOOL crFlag)
{
   WORD  ndx;

   for(ndx = 1; ndx <= *tStr; ndx++)
   {
      modemWr(tStr[ndx]);
      if(nDelay > 0)
         delay(nDelay, NULL);
      if(!dialing)
         idleProcess();
   }

   if(crFlag)
      modemWr(CR);
}



VOID  checkCommEvent()
{
   DWORD    eventMask;
   HANDLE   hEvent;
   OVERLAPPED OverLapped;
   DWORD      dwGetLastError;

//   eventMask = EV_RXCHAR | EV_ERR | EV_BREAK | EV_CTS | EV_DSR;
   eventMask = EV_RXCHAR;
   SetCommMask(sPort, eventMask);

   hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   OverLapped.hEvent = hEvent;

   while(TRUE)
   {
      if(bPortIsGood && (sPort != NULL) && (sPort != (HANDLE)-1) && (!CommThreadExit) )
      {
         if(WaitForSingleObject(hMutex, 50) == 0)
	 {
	    eventMask = EV_RXCHAR;
	   // SetCommMask(sPort, eventMask);

            if(WaitCommEvent(sPort, (LPDWORD)&eventMask, &OverLapped))
            {
               gotCommEvent = TRUE;
	    }
	    else
	    {
		dwGetLastError = GetLastError();
		if (dwGetLastError == ERROR_IO_PENDING)
		     {

                    DWORD Trash;
                    GetOverlappedResult(
                        sPort,
                        &OverLapped,
                        &Trash,
                        TRUE
                        );
		    gotCommEvent = TRUE;

		      }
		else  {

		      //-sdj for telnet-quit processing
		      //-sdj if this is a telnet connection and
		      //-sdj the user hits cntl-c/bye/quit etc
		      //-sdj the telnet service will stop talking
		      //-sdj with us, but terminalapp still keeps
		      //-sdj doing io without knowing that the handle
		      //-sdj can only be close now, The way we can
		      //-sdj detect this is, to check if getlasterror
		      //-sdj is ERROR_NETNAME_DELETED, if this is the
		      //-sdj case then we should do exactly same thing
		      //-sdj which we do when the user tries to go to
		      //-sdj some other comm port, close this one, and
		      //-sdj go to the next one.
		      //-sdj by setting bPortDisconnected to TRUE,
		      //-sdj further modemBytes()[reads] will stop on
		      //-sdj this port, and modemBytes will prompt the
		      //-sdj user to select some other port, and return.

		      CloseHandle(sPort);  // only valid operation in this state
		      sPort = NULL;	   // this will prevent checkcommevent to
					   // attempt unnecessary waits untill sPort
					   // becomes valid, and bPortDisconnected
					   // is set back to FALSE by modembytes/resetserial

		      bPortDisconnected = TRUE;


		      }


            }

	    if(CommThreadExit)	  // was ,doneFlag but doneFlag
				  // does not get set for sometime
				  // even after the comm port closes
				  // so exit the thread when you know
				  // that the port is going to get closed
				  // This flag is init to false and set
				  // to true just before calling exitserial
				  // in termfile.c

            {
               gbThreadDoneFlag = TRUE;
               ReleaseMutex(hMutex);
               ResetEvent(hEvent);
               ExitThread((DWORD)0);
            }

            ReleaseMutex(hMutex);

            eventMask = EV_RXCHAR;
         }  // wait on mutex

#ifdef SLEEP_FOR_CONTEXT_SWITCH

	 Sleep((DWORD)3);
#endif


         ResetEvent(hEvent);
      }  // good sPort
      else
      {
	 if(CommThreadExit)	    // was doneFlag : see above for comments
         {
            gbThreadDoneFlag = TRUE;
            ReleaseMutex(hMutex);
            ResetEvent(hEvent);
            ExitThread((DWORD)0);
	 }

	 Sleep((DWORD)3);   // -sdj either the comm port handle is changing
			  // or it is invalid, so instead of doing
			  // a tight while-true, sleep each time you
			  // come here, so that others get a chance

      }
   }
}
