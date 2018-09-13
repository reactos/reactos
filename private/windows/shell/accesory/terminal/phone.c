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
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "connect.h"


/*--------------------- >>> Local Global Declarations <<< -------------------*/

static INT retryCnt;
static INT timeOutSec;                       /* mbbx 2.00: display time remaining */


/*---------------------------------------------------------------------------*/
/* dbDialing() - Dialing dialog box message proccessing.                     */
/*---------------------------------------------------------------------------*/

#define MAX_DIAL_STRLEN             21       /* mbbx 2.00: CUA */

LONG APIENTRY dbDialing(HWND hDlg,UINT message,WPARAM wParam,LONG lParam)
{
   BYTE  tmp1[TMPNSTR], tmp2[TMPNSTR];
   COMSTAT  serInfo;

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);                      /* mbbx 2.00: CUA... */

      strcpy(tmp1, trmParams.phone);
      if(strlen(tmp1) > MAX_DIAL_STRLEN)
         strcpy(tmp1+(MAX_DIAL_STRLEN-3), "...");
      SetDlgItemText(hDlg, IDDIALING, (LPSTR) tmp1);

      retryCnt = 0;
      itemHit = -1;

      if(!SetTimer(hDlg, -1, 128, lpdbDialing))    /* rjs bugs 006 */
      {
         LoadString(hInst, STRERRNOTIMERS, (LPSTR) tmp1, TMPNSTR);
         MessageBox(hDlg, (LPSTR)tmp1, NULL, MB_OK | MB_ICONEXCLAMATION);
         break;
      }

      return(TRUE);

   case WM_TIMER:
      if(itemHit == -1)
      {
         sprintf(tmp1, "%u", timeOutSec = trmParams.dlyRetry);    /* mbbx 2.00: time remaining... */
         SetDlgItemText(hDlg, IDDIALTIME, (LPSTR) tmp1);

         if(!sPortErr)    /* rjs bug2 */
         {
            termSendCmd(trmParams.originate, strlen(trmParams.originate), TRUE);    /* mbbx 2.01.19 ... */
            getMdmResult();

            termSendCmd(trmParams.dialPrefix, strlen(trmParams.dialPrefix), FALSE);

            strcpy(tmp1, trmParams.phone);      /* mbbx 2.01.18 ... */
            strcpy(tmp1+strlen(tmp1), trmParams.phone2);
            termSendCmd(tmp1, strlen(tmp1), 0x0080 | FALSE);

            termSendCmd(trmParams.dialSuffix, strlen(trmParams.dialSuffix), TRUE);

         }
         else
            modemReset();

         dialStart = tickCount();
         itemHit = 0;
      }
      else if(itemHit == 0)
      {
         updateTimer();

         if(trmParams.fCarrier)               /* mbbx 1.10: CUA... */
            mdmConnect();
         else if(modemBytes())
         {
            getMdmResult();
            mdmOnLine = (mdmValid && (mdmResult[2] == 'C'));
//            mdmOnLine = TRUE;
         }

         if(mdmOnLine)
         {
            getMdmResult();
            if(trmParams.flgSignal)
            {
               sysBeep();
               sysBeep();                    /* mbbx 2.00: more noise for signal... */
               sysBeep();
            }
            timerAction(TRUE, TRUE);         /* mbbx 1.03 */
            break;
         }

         if((tickCount() - dialStart) > ((trmParams.dlyRetry > 30) ? (trmParams.dlyRetry * 60) : (30 * 60)))
         {
            if((trmParams.flgRetry) && ((retryCnt < trmParams.cntRetry) || (trmParams.cntRetry == 255)))
            {
               LoadString(hInst, STR_RETRYCOUNT, (LPSTR) tmp1, TMPNSTR);
               sprintf(tmp2, tmp1, ++retryCnt);
               SetDlgItemText(hDlg, IDDIALRETRY, (LPSTR) tmp2);

               if(trmParams.flgSignal)       /* mbbx 2.00: redial beep optional... */
                  sysBeep();

               itemHit = -1;
            }
            else
            {
               if(!sPortErr)    /* rjs bug2 */
                  modemWr(CR);
               break;
            }
         }
         else if((trmParams.dlyRetry - ((tickCount() - dialStart) / 60)) != timeOutSec)
         {
            sprintf(tmp1, "%u", timeOutSec = trmParams.dlyRetry - ((tickCount() - dialStart) / 60));
            SetDlgItemText(hDlg, IDDIALTIME, (LPSTR) tmp1);
         }
      }
      return(TRUE);

   case WM_COMMAND:                          /* mbbx 2.00: only one control... */
      modemWr(CR);
      break;

   default:
      return(FALSE);
   }

   KillTimer(hDlg, -1);                      /* mbbx 2.00: eliminate statics... */
   EndDialog(hDlg, TRUE);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* dialPhone() -                                                       [scf] */
/*---------------------------------------------------------------------------*/

VOID dialPhone()
{
   BYTE tmp1[TMPNSTR];
   DCB  dcb;

   if(trmParams.comDevRef == ITMDLLCONNECT)  /* slc nova 031 */
   {
      DLL_ConnectConnector(ghCCB, TRUE);
      return;
   }

   if(*trmParams.phone == 0)                 /* Don't dial if no phone no.   */
      doSettings(IDDBPHON, dbPhon);

   if(*trmParams.phone == 0)                 /* Don't dial if no phone no.   */
      return;

   while(modemBytes())
      rdModem(FALSE);

   offCursor();
   if(mdmOnLine)                             /* Tell user to hang up         */
   {
      LoadString(hInst, STR_HANGUP, (LPSTR) tmp1, TMPNSTR);
         itemHit = testBox(NULL, -(MB_OK | MB_ICONHAND), STR_ERRCAPTION, tmp1);
      onCursor();
      return;
   }

   if(trmParams.flgRetry )
      trmParams.cntRetry = 255;

   dialing = TRUE;
   DialogBox(hInst, MAKEINTRESOURCE(IDDBDIALING), GetFocus(), lpdbDialing);
   dialing = FALSE;

   onCursor();
   trmParams.cntRetry = 0;
}


/*---------------------------------------------------------------------------*/
/* hangUpPhone() -                                                   [scf]   */
/*---------------------------------------------------------------------------*/

VOID hangUpPhone()
{
   if(trmParams.comDevRef == ITMDLLCONNECT)  /* slc nova 031 */
   {
      DLL_DisconnectConnector(ghCCB);
      return;
   }

   SetCursor(LoadCursor(NULL, IDC_WAIT));    /* mbbx 1.10: ala jtf */

   delay(33, NULL);                          /* mbbx 0.62: formerly 66 ticks */

   termSendCmd(trmParams.hangSuffix, strlen(trmParams.hangSuffix), 0x0040 | TRUE);  /* mbbx 2.01.19 ... */
   getMdmResult();

   if(trmParams.fCarrier)                     /* mbbx 1.10: carrier... */
      mdmConnect();
   else
      mdmOnLine = FALSE;

   modemReset();                             /* mbbx: isolate COM */
   timerAction(FALSE, FALSE);                /* mbbx 1.03 */

   SetCursor(LoadCursor(NULL, IDC_ARROW));   /* mbbx 1.10: ala jtf */
}


/*---------------------------------------------------------------------------*/
/* getMdmResult() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

VOID getMdmResult()
{
   LONG  begTime;

   begTime = tickCount();
   *mdmResult = 0;
   mdmValid = FALSE;

   repeat
   {
      rdModem(TRUE);
      if(!dialing)
         idleProcess();
   }
   until(mdmValid || ((tickCount() - begTime) > 90));

   if(!mdmValid && (*mdmResult >= 2))
      mdmValid = TRUE;                       /* mbbx: override timeout !!! */
}


/*---------------------------------------------------------------------------*/
/* termSendCmd() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

/* convert ==>       ABCDEFGHIJKLMNOPQRSTUVWXYZ */
#define PHONE_CHARS "2223334445556667-77888999-"

BOOL termSendCmd(BYTE *   str, int      nBytes, WORD     wFlags)
{
   INT   ndx;

   if(wFlags & 0x0040)                       /* get modem's attention */
   {
      if(nBytes == 0)
         return(TRUE);

      termSendCmd(trmParams.hangPrefix, strlen(trmParams.hangPrefix), (3 << 8) | FALSE);
      delay(66, NULL);
      getMdmResult();
   }

   for(ndx = 0; ndx < nBytes; ndx += 1)
   {
      if(wFlags & 0x0080)                    /* xlate phone string */
      {
         if((str[ndx] >= 'A') && (str[ndx] <= 'Z'))
            str[ndx] = PHONE_CHARS[str[ndx] - 'A'];
         else if((str[ndx] >= 'a') && (str[ndx] <= 'z'))
            str[ndx] = PHONE_CHARS[str[ndx] - 'a'];
      }

      modemWr(str[ndx]);

      if((wFlags & 0xFF00) > 0)
         delay(((wFlags >> 8) & 0x00FF), NULL);

      if(!dialing)                           /* mbbx 2.01.19: why is this needed ??? */
         idleProcess();
   }

   if(wFlags & 0x0001)                       /* send CR */
      modemWr(CR);

   if(wFlags & 0x0002)                       /* clear modem response */
      getMdmResult();

   return(TRUE);
}
