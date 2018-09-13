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
#define  USECOMM

#include <windows.h>
#include <port1632.h>
#include "dcrc.h"
#include "dynacomm.h"
#include "connect.h"

/*---------------------------------------------------------------------------*/
/* exitSerial() -                                                  [mbb/rkh] */
/*---------------------------------------------------------------------------*/

VOID NEAR WIN_exitSerial()                   /* mbbx 2.00: network... */
{
   DCB   dcb;

   if(GetCommState(sPort, (DCB FAR *) &dcb) == 0)  /* mbbx 1.04: RTS/DTR disable... */
   {
      dcb.fRtsControl = RTS_CONTROL_DISABLE;
      dcb.fDtrControl = DTR_CONTROL_DISABLE;
      EscapeCommFunction(sPort,CLRRTS);
      EscapeCommFunction(sPort,CLRDTR);

      DEBOUT("SetCommState from Win_exitSerial for comport=%lx\n",sPort);
      if(!SetCommState(sPort,(DCB FAR *) &dcb))
      {
         DEBOUT("FAIL: SetCommState from Win_exitSerial for comport=%lx\n",sPort);
      }
   }


#ifdef ORGCODE
   FlushComm(sPort, 1);
   FlushComm(sPort, 0);
#else
   DEBOUT("FlushFileBuffers from Win_exitSerial: comport %lx\n",sPort);
#ifndef BUGBYPASS
   DEBOUT("FlushFileBuffers from Win_exitSerial: BYPASSING FLUSH DUE TO BUG %lx\n",sPort);
#else
   if (!FlushFileBuffers(sPort))
    {
    DEBOUT("FAIL: FlushFileBuffers comport %lx\n",sPort);
    }
#endif

   DEBOUT("PurgeComm from Win_exitSerial:comport %lx\n",sPort);
   if (!PurgeComm(sPort,0))
    {
    DEBOUT("FAIL: PurgeComm comport %lx\n",sPort);
    }
#endif

   SetCommMask(sPort, EV_RXCHAR);
   //WaitForSingleObject(hMutex, 500);
   bPortIsGood = FALSE;



    /**********
    {
    // Make suer sPort gets closed, bug#9671
    // Actually a bug in serial driver, The fix is a
    // quick hack, should be removed once the driver
    // is fixed.

    int cnt=20;

    while (cnt-- && CloseHandle(sPort)) Sleep (200);
    }
    **********/

    //
    //  We can't close the handle twice now.
    //  So, just close it and wait a little.
    //

    Sleep (200);
    CloseHandle (sPort);
    Sleep (200);



   sPort = NULL;
   //ReleaseMutex(hMutex);
}


VOID exitSerial()                            /* mbbx 2.00: network... */
{
   switch(trmParams.comDevRef)
   {
   case ITMWINCOM:
      WIN_exitSerial();
      break;

   case ITMDLLCONNECT:                       /* slc nova 012 bjw nova 02 */
      DLL_ExitConnector(ghCCB, &trmParams);  /* slc nova 031 */
      break;
   }

   trmParams.comDevRef = ITMNOCOM;
}


/*---------------------------------------------------------------------------*/
/* resetSerial() -                                                 [mbb/rkh] */
/*---------------------------------------------------------------------------*/

VOID NEAR WIN_resetSerial(recTrmParams *trmParams, BOOL bLoad, NEARPROC errProc)
{
   INT            attempts;
   // sdj: this is replaced by global szCurrentPortName ;BYTE	   tmp1[TMPNSTR+1];
   BYTE 	  tmp1[TMPNSTR+1],tmp2[TMPNSTR+1];
   DCB		  dcb;
   DCB		  PrevDcb;
   COMMTIMEOUTS   CommTimeOuts;
   BOOL 	  bRc;
   DWORD	  dwError;
   COMMPROP	  CommProp;  // -sdj sep92 on low mem rx buffer can be < 1024

   modemReset();                             /* mbbx 0.72: avoid hang if XOFF-ed */

   if(bLoad)
   {
      for(attempts = 0; trmParams->comDevRef == ITMNOCOM; attempts += 1)
      {
	 if(trmParams->comPortRef > MaxComPortNumberInMenu)

	    strcpy(szCurrentPortName, "\\\\.\\TELNET");

	 else

         {
	   // LoadString(hInst, STR_COM, (LPSTR) tmp2, MINRESSTR);
	   //	 sprintf(szCurrentPortName, tmp2, trmParams->comPortRef);
	   strcpy(szCurrentPortName,arComNumAndName[trmParams->comPortRef].PortName);
         }


	 if( (sPort != NULL) && (sPort != (HANDLE)-1) )
	   {
	    SetCommMask(sPort, 0);   // so that waitcommevent comes out; -sdj
	    sPort = NULL;
	   }

	 sPort = CreateFile(szCurrentPortName, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

         if (sPort == (HANDLE)-1)
	 {
	    dwError	= GetLastError();
	    bPortIsGood = FALSE;
            if(!(*errProc)(trmParams, attempts))
               return;
         }
         else
	    {
	        SetCommMask(sPort, EV_RXCHAR); // -sdj 27apr92  telnet deadlock
	        dwWriteFileTimeout = 5000;	   // -sdj 28apr92  telnet debug

            trmParams->comDevRef                     = ITMWINCOM;
            CommTimeOuts.ReadIntervalTimeout         = 0xFFFFFFFF;
            CommTimeOuts.ReadTotalTimeoutMultiplier  = 0;
            CommTimeOuts.ReadTotalTimeoutConstant    = 0;
            CommTimeOuts.WriteTotalTimeoutMultiplier = 0;

            if (trmParams->flowControl == ITMHARDFLOW)
            {
               CommTimeOuts.WriteTotalTimeoutConstant   = 5000;   // 5msecs
            }
            else
            {
               CommTimeOuts.WriteTotalTimeoutConstant   = 10000;  //10secs
            }

            if (!(bRc = SetCommTimeouts(sPort,&CommTimeOuts) ) )
            {
               if(!(*errProc)(trmParams, attempts))
                  return;
	        }

	        //  There may be data already waiting to be read - i.e. data
	        //	that arrived before we set comm mask.  This data will not
	        //	be uncovered by wait mask, so we must try to read it.
	    
	        gotCommEvent = TRUE;

         }
      } // endof for
   }    // endof if bload

   /* This would work on both win30 and win32 -sdj*/
/*   if(GetCommState(sPort, (DCB FAR *) &dcb) == 0) -sdj*/

DEBOUT("HACK : %s\n","rc of GetCommState: not checked for now");
   {
      GetCommState(sPort, (DCB FAR *) &dcb);
      GetCommState(sPort, (DCB FAR *) &PrevDcb);

      // -sdj sep92 on low mem rx buffer can be < 1024
      // -sdj set rx buffer to nice 4096 bytes size
      // -sdj driver will do its best and set the Rx buffer to this size
      // -sdj if it fails then dwCurrentRxQueue will be the one we have
      // -sdj so do getcommprop again to fetch this value, which can
      // -sdj be used to set xoff and xon lims

      GetCommProperties(sPort,&CommProp);

      SetupComm(sPort,4096,4096);

      CommProp.dwCurrentRxQueue = 0; // -sdj dirty it so that we
				     // -sdj can use this only if !=0
      GetCommProperties(sPort,&CommProp);

      // sdj: added this code to take care of extra baud rates support

      if (trmParams->speed <= 57600)
	{
	 dcb.BaudRate = trmParams->speed;	/* mbbx 2.00: allow any baud... */
	}
      else
	{
	if (trmParams->speed == 57601)
	    {
	    // sdj: this means 115.2K baud rate which cannot fit into BYTE!
	    dcb.BaudRate = 115200;
	    }
	else{
	     if (trmParams->speed == 57602)
		{
		dcb.BaudRate = 128000;
		}
	     else
		{
		// sdj: something wrong! default to 1200
		dcb.BaudRate = 1200;
		}
	    }
	}

      dcb.ByteSize = 8 + (trmParams->dataBits - ITMDATA8);
      dcb.Parity = NOPARITY + (trmParams->parity - ITMNOPARITY);
      dcb.StopBits = ONESTOPBIT + (trmParams->stopBits - ITMSTOP1);

//      dcb.RlsTimeout   = 0;
//      dcb.CtsTimeout   = (trmParams->flowControl == ITMHARDFLOW) ? 5 : 0;  /* mbbx 1.10: CUA */
//      dcb.DsrTimeout   = 0;

      dcb.fBinary      = TRUE;
      dcb.fRtsControl  = RTS_CONTROL_ENABLE;
      dcb.fParity      = trmParams->fParity;    /* mbbx 1.10: CUA */
      dcb.fOutxCtsFlow = (trmParams->flowControl == ITMHARDFLOW);    /* mbbx 1.10: CUA... */
      dcb.fOutxDsrFlow = FALSE;
      dcb.fDtrControl  = DTR_CONTROL_ENABLE;

      dcb.fOutX        =
      dcb.fInX         = (trmParams->flowControl == ITMXONFLOW);
      dcb.fErrorChar   = trmParams->fParity;    /* mbbx 1.10: CUA */
      dcb.fNull        = FALSE;
//      dcb.fChEvt       = FALSE;
//      dcb.fDtrFlow     = FALSE;

if (trmParams->flowControl == ITMHARDFLOW)
    {
      dcb.fRtsControl     = RTS_CONTROL_HANDSHAKE;    /* mbbx 1.10: CUA... */
    }

      dcb.XonChar      = XON;
      dcb.XoffChar     = XOFF;

      /* -sdj sep92, this is 1k/4=256, 9M system can have RXQ=256
	 -sdj in that case SetCommState can fail due to invalid
	 -sdj parameters of xoff xon limits
      dcb.XonLim       = NINQUEUE / 4;
      dcb.XoffLim      = NINQUEUE / 4;
      */

      // -sdj if for some wierd reason dwCurrentRxQueue is not
      // -sdj filled in by the driver, then let xon xoff lims
      // -sdj be the default which the driver has.
      // -sdj (dwCurrentRxQueue was set to 0 before calling Get again)

      if (CommProp.dwCurrentRxQueue != 0)
	 {
	  dcb.XonLim	= (WORD)(CommProp.dwCurrentRxQueue / 4);
	  dcb.XoffLim	= (WORD)(CommProp.dwCurrentRxQueue / 4);
	 }

      dcb.ErrorChar    = '?';
      dcb.EofChar      = CNTRLZ;
      dcb.EvtChar      = 0;
      dcb.wReserved    = 0;

#ifdef ORGCODE
      if(SetCommState((DCB FAR *) &dcb) == 0)
      {
#else
      if(SetCommState(sPort, (DCB FAR *) &dcb) == 0)
      {

DEBOUT("FAIL: SetCommState for comport=%lx\n",sPort);
#endif
         mdmOnLine = FALSE;                  /* mbbx 1.10: carrier... */
	 mdmConnect();
	 LoadString(hInst, STR_SETCOMFAIL, (LPSTR) tmp1, TMPNSTR);
	 LoadString(hInst, STR_ERRCAPTION, (LPSTR) tmp2, TMPNSTR);
	 MessageBox(hItWnd, (LPSTR) tmp1, (LPSTR)tmp2, MB_OK | MB_APPLMODAL);
	 SetCommState(sPort,&PrevDcb);
         return;
      }

#ifdef ORGCODE
#else
    /*DWORD ReadIntervalTimeout;           Maximum time between read chars. */
    /*DWORD ReadTotalTimeoutMultiplier;    Multiplier of characters.        */
    /*DWORD ReadTotalTimeoutConstant;      Constant in milliseconds.        */
    /*DWORD WriteTotalTimeoutMultiplier;   Multiplier of characters.        */
    /*DWORD WriteTotalTimeoutConstant;     Constant in milliseconds.        */

               DEBOUT("Win_resetSerial: %s\n","Setting Comm timeouts");
               CommTimeOuts.ReadIntervalTimeout          = 0xFFFFFFFF;
               CommTimeOuts.ReadTotalTimeoutMultiplier   = 0;
               CommTimeOuts.ReadTotalTimeoutConstant     = 0;
               CommTimeOuts.WriteTotalTimeoutMultiplier  = 0;

               if (trmParams->flowControl == ITMHARDFLOW)
               {
                  CommTimeOuts.WriteTotalTimeoutConstant   = 5000;  //5 msecs
               }
               else
               {
                  CommTimeOuts.WriteTotalTimeoutConstant   = 10000; //10secs
               }

               if (!(bRc = SetCommTimeouts(sPort,&CommTimeOuts) ) )
               {
                  DEBOUT("FAIL: SetCommTimeouts failed rc: %lx\n",bRc);
                  mdmOnLine = FALSE;                  /* mbbx 1.10: carrier... */
                  mdmConnect();
                  return;
               }
#endif
   }

bPortIsGood = TRUE;	   /* now the port is properly initialized -sdj 05/21/92*/
			  //-sdj for telnet-quit processing
bPortDisconnected = FALSE; /* there is no problem of port_was_opened_but_not_working */

}


VOID resetSerial(recTrmParams *trmParams, BOOL bLoad,BOOL  bInit,BYTE byFlowFlag) /* slc swat */
{
   /* LPCONNECTOR_CONTROL_BLOCK	lpCCB; -sdj no unref variables please ; slc nova 031 */

   if(bLoad)
   {
      if(!trmParams->fResetDevice)
         trmParams->newDevRef = trmParams->comDevRef;

      exitSerial();
   }

   switch(bLoad ? trmParams->newDevRef : trmParams->comDevRef)
   {

   case ITMNOCOM:

    break;


   default:      // case ITMWINCOM:
      WIN_resetSerial(trmParams, bLoad, bInit ? (NEARPROC)resetSerialError0 : (NEARPROC)resetSerialError1 /*, byFlowFlag*/); /* slc swat */
      break;

#ifdef OLDCODE

   case ITMWINCOM:
      WIN_resetSerial(trmParams, bLoad, bInit ? (NEARPROC)resetSerialError0 : (NEARPROC)resetSerialError1 /*, byFlowFlag*/); /* slc swat */
      break;

   case ITMDLLCONNECT:                       /* slc nova 012 bjw nova 002 */
      if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(ghCCB)) != NULL)   /* slc nova 031 */
      {
         if(lpCCB->hConnectorInst == NULL)   /* first time? */
            if(!loadConnector(NULL, ghCCB, (LPSTR)trmParams->szConnectorName, FALSE))
               return;
         GlobalUnlock(ghCCB);

         trmParams->comDevRef = ITMDLLCONNECT;
         DLL_SetupConnector(ghCCB, FALSE);   /* slc nova 031 */
      }
      break;

#endif

   }

   trmParams->fResetDevice = FALSE;
}


/*---------------------------------------------------------------------------*/
/* resetSerialError0 - called during initialization;                   [mbb] */
/*                     auto attempt other COM port, then fail                */
/*---------------------------------------------------------------------------*/

BOOL PASCAL NEAR resetSerialError0(recTrmParams *trmParams, WORD count)
{
   BYTE  tmp1[TMPNSTR+1];
   BYTE  tmp2[TMPNSTR+1];

   //sdj: if this is a telnet port then advice the user to go to
   //sdj: the control panel and see if telnet service is started
   //sdj: else stick with the original msg of selected com port not
   //sdj: available, select other port.

   if (!strcmp(szCurrentPortName,"\\\\.\\TELNET"))
    {
     LoadString(hInst, STR_TELNETFAIL, (LPSTR) tmp1, TMPNSTR);
    }
   else
    {
     LoadString(hInst, STR_OTHERCOM, (LPSTR) tmp1, TMPNSTR);
    }
   LoadString(hInst, STR_ERRCAPTION, (LPSTR) tmp2, TMPNSTR);

   MessageBox(hItWnd, (LPSTR) tmp1, (LPSTR)tmp2, MB_OK | MB_APPLMODAL);
   doSettings(IDDBCOMM, dbComm);

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* resetSerialError1 - default case (e.g., after loading settings)     [mbb] */
/*                     prompt to attempt other COM port, then fail           */
/*---------------------------------------------------------------------------*/

BOOL PASCAL NEAR resetSerialError1(recTrmParams *trmParams, WORD count)
{
   BYTE  tmp1[TMPNSTR+1];
   BYTE  tmp2[TMPNSTR+1];

   if(count > 0)
   {
      LoadString(hInst, STR_NOCOMMPORTS, (LPSTR) tmp1, TMPNSTR);    /* mbbx 1.00 */
      testMsg(tmp1,NULL,NULL);
   }
   else
   {
   //sdj: if this is a telnet port then advice the user to go to
   //sdj: the control panel and see if telnet service is started
   //sdj: else stick with the original msg of selected com port not
   //sdj: available, select other port.

   if (!strcmp(szCurrentPortName,"\\\\.\\TELNET"))
    {
     LoadString(hInst, STR_TELNETFAIL, (LPSTR) tmp1, TMPNSTR);
    }
   else
    {
      LoadString(hInst, STR_OTHERCOM, (LPSTR) tmp1, TMPNSTR);
    }
      LoadString(hInst, STR_ERRCAPTION, (LPSTR) tmp2, TMPNSTR);
      MessageBox(hItWnd, (LPSTR) tmp1, (LPSTR)tmp2, MB_OK | MB_APPLMODAL);

      trmParams->comPortRef = ITMNOCOM;       /* mbbx 1.10: CUA */
   }
   return(FALSE);
}

