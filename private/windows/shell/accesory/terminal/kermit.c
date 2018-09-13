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
#include "kermit.h"

/*---------------------------------------------------------------------------*/
/* KER_Receive() - state table switcher for receiving files            [rkh] */
/*---------------------------------------------------------------------------*/
BOOL FAR KER_Receive(BOOL  bRemoteServer)
{
   INT  num, len;
   BYTE bySaveState = KER_state;

   KER_Init(KER_RCV);

   if((KERRCVFLAG == KERSCREEN) || answerMode)
   {
      KER_n = 1;
      KER_state = KER_FILE;
      if(bySaveState == KER_TEXT)               /* tge nova kermit ... */
      {
         KER_SndPacket(KER_ACK, KER_n, 0, 0);   /* Return a ACK */
         KER_state = KER_DATA;
      }
   }
   else
   {
      flushRBuff();
   }

   while(TRUE)
   {
      if(xferStopped && (KER_state != KER_ABORT))
      {
         KER_RcvPacket(&len, &num, packet);
         KER_Abort(STR_KER_RCVABORT);
         return(FALSE);
      }
      if((KER_numtry++ > KER_MAXRETRY) && (KER_state != KER_ABORT))
      {
         KER_RcvPacket(&len, &num, packet);
         KER_Abort(STR_KER_RETRYABORT);
         return(FALSE);
      }
      rxEventLoop();

      KER_bSetUp(KER_state);

      switch(KER_state)
      {
         case KER_RCV:                       /* Receive-Init */   
            KER_state = KER_ReceiveInit();
            break;

         case KER_TEXT:                      /* Receive-Text */
         case KER_FILE:                      /* Receive-File */
            KER_state = KER_ReceiveFile();
            break;                           /* Receive-Data */

         case KER_DATA:
            KER_state = KER_ReceiveData();
            break; 

         case KER_CMPLT:                     /* Complete state */
            delay((WORD) 60, NULL);          /* tgex delay 1 sec for last packet */
            return(TRUE);

         case KER_ABORT:                     /* Abort state */
         default:
           delay((WORD) 60, NULL);          /* tgex delay 1 sec for last packet */
           return(FALSE);
      }
   } /* while */
}

/*---------------------------------------------------------------------------*/
/* KER_ReceiveInit() - receive initalization                           [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_ReceiveInit()
{
   INT len, num;                             /* Packet length, number */

   KER_parflg = KER_NOPARITY;                /* tgex : let parity bits through from sender */

   switch(KER_RcvPacket(&len, &num, packet)) /* Get a packet */
   {
      case KER_SEND:                         /* Send-Init */
         if (KERRCVFLAG == KERFILE)          /* set parity off of 'S' packet */
            KER_parflg = KER_AutoPar();
         KER_RcvPacketInit(packet);          /* Get the other side's init data */
         KER_SndPacketInit(packet);          /* Fill up packet with my init info */
         KER_SndPacket(KER_ACK, KER_n, 8, packet); /* ACK with my parameters */
         KER_mask = 0xFF;                    /* set mask so checksum will include 8th bit if parity is used */
                                             /* have to wait until after packet we send back has built
                                                its checksum while ignoring possible parity bits that we
                                                let thru in the first place to be able to check the parity */
         KER_oldtry = KER_numtry;            /* Save old try count */
         KER_numtry = 0;                     /* Start a new counter */
         KER_n = (KER_n+1) % 64;             /* Bump packet number, mod 64 */
         return(KER_FILE);                   /* Enter File-Receive state */

      case KER_ERROR:                        /* Error packet received */
         return(KER_PrintErrPacket(packet)); /* Print it out and */

      case FALSE:                            /* Didn't get packet */
      default: 
         flushRBuff(); 
         showBErrors(++xferErrors);
         KER_SndPacket(KER_NACK, KER_n, 0, 0);  /* Return a NAK */
         return(KER_state);                     /* Keep trying */
   }
}

/*---------------------------------------------------------------------------*/
/* KER_ReceiveFile() - receive file header                             [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_ReceiveFile()
{
   INT      num, len;   
   BYTE     filnam1[50];
   OFSTRUCT dummy;

   switch(KER_RcvPacket(&len, &num, packet)) /* Get a packet */
   {
      case KER_SEND:                         /* Send-Init, maybe our ACK lost */
         if(KER_oldtry++ > KER_MAXRETRY)           /* If too many tries abort */
            return(KER_Abort(STR_KER_RETRYABORT));

         if(num==((KER_n==0) ? 63 : KER_n-1))/* Previous packet, mod 64? */
         {                                   /* Yes, ACK it again with  */
            KER_SndPacketInit(packet);       /* our Send-Init parameters */
            KER_SndPacket(KER_ACK, num, 8, packet);
            showBErrors(++xferErrors);
            KER_numtry = 0;                  /* Reset try counter */
            return(KER_state);               /* Stay in this state */
         }
         else                                /* Not previous packet, abort */
            return(KER_Abort(STR_KER_BADPACKNUM));

      case KER_EOF:                          /* End-Of-File */
         if(KER_oldtry++ > KER_MAXRETRY)          /* If too many tries abort */
            return(KER_Abort(STR_KER_RETRYABORT));

         if(num == ((KER_n==0) ? 63 : KER_n-1)) /* Previous packet, mod 64? */
         {                                      /* Yes, ACK it again. */
            KER_SndPacket(KER_ACK, num, 0, 0);
            KER_numtry = 0;
            showBErrors(++xferErrors);
            return(KER_state);               /* Stay in this state */
         }
         else                                /* Not previous packet, abort */
            return(KER_Abort(STR_KER_BADPACKNUM));

      case KER_TEXT:
      case KER_FILE:                         /* File Header (just what we want) */
         if(num != KER_n)                    /* The packet number must be right */
            return(KER_Abort(STR_KER_BADPACKNUM));

         if(!KER_firstfile)
            return(KER_Abort(STR_KER_CREATEFILE));

         KER_SndPacket(KER_ACK, KER_n, 0, 0);/* Acknowledge the file header */
         KER_oldtry = KER_numtry;            /* Reset try counters */
         KER_numtry = 0;
         KER_n = (KER_n+1) % 64;             /* Bump packet number, mod 64 */
         KER_bytes = 0l;                     /* don't have any bytes from file yet */
         return(KER_DATA);                   /* Switch to Data state */

      case KER_BREAK:                        /* Break transmission (EOT) */
          if(num != KER_n)                   /* Need right packet number here */
             return(KER_Abort(STR_KER_BADPACKNUM));

          KER_SndPacket(KER_ACK, KER_n, 0, 0);  /* Say OK */
          return(KER_CMPLT);                    /* Go to complete state */

      case KER_ERROR:                           /* Error packet received */
         return(KER_PrintErrPacket(packet)); /* Print it out and */

      case FALSE:                               /* Didn't get packet */
      default: 
         flushRBuff(); 
         KER_SndPacket(KER_NACK, KER_n, 0, 0);  /* Return a NAK */
         showBErrors(++xferErrors);
         return(KER_state);                     /* Keep trying */
      }
}

/*---------------------------------------------------------------------------*/
/* KER_ReceiveData() - receive data packets                            [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_ReceiveData()
{
   INT num, len;                             /* Packet number, length */

   switch(KER_RcvPacket(&len, &num, packet)) /* Get packet */
   {
   case KER_DATA:                            /* Got Data packet */
      if(num != KER_n)                       /* Right packet? */
      {                                      /* No */
         if(KER_oldtry++ > KER_MAXRETRY)           /* If too many tries abort */
            return(KER_Abort(STR_KER_RETRYABORT));

         if(num == ((KER_n==0) ? 63 : KER_n-1)) /* Else check packet number */
         {                                      /* Previous packet again? */
            flushRBuff(); 
            KER_SndPacket(KER_ACK, num, 6, packet);   /* Yes, re-ACK it */
            KER_numtry = 0;                           /* Reset try counter */
            showBErrors(++xferErrors);
            return(KER_state);               /* Don't write out data! */
         }
         else                                /* sorry, wrong number */
            return(KER_Abort(STR_KER_BADPACKNUM));
      }
                                                /* Got data with right packet number */
      KER_BufferEmpty(packet, len, KERRCVFLAG); /* Write the data to the file */
      if(KERRCVFLAG != KERSCREEN)
         showBBytes(KER_bytes, FALSE);       /* mbbx 2.00: xfer ctrl */
      KER_SndPacket(KER_ACK, KER_n, 0, 0);   /* Acknowledge the packet */
      KER_oldtry = KER_numtry;               /* Reset the try counters */
      KER_numtry = 0;        
      KER_n = (KER_n+1) % 64;                /* Bump packet number, mod 64 */
      return(KER_DATA);                      /* Remain in data state */

   case KER_FILE:                            /* Got a File Header */
      if(KER_oldtry++ > KER_MAXRETRY)              /* If too many tries abort */
         return(KER_Abort(STR_KER_RETRYABORT));

      if(num == ((KER_n==0) ? 63:KER_n-1))  /* Else check packet number */
      {                                      /* It was the previous one */
         flushRBuff(); 
         KER_SndPacket(KER_ACK, num, 0, 0);  /* ACK it again */
         KER_numtry = 0;                     /* Reset try counter */
         showBErrors(++xferErrors);
         return(KER_state);                  /* Stay in Data state */
      }
      else                                   /* Not previous packet, abort */
         return(KER_Abort(STR_KER_BADPACKNUM));

   case KER_EOF:                             /* End-Of-File */
      if(num != KER_n)                       /* Must have right packet number */
         return(KER_Abort(STR_KER_BADPACKNUM));

      KER_SndPacket(KER_ACK, KER_n, 0, 0);   /* OK, ACK it. */
      KER_firstfile = FALSE;                 /* we have got at least one complete file*/
      _lclose(xferRefNo);                    /* Close the file */
      KER_n = (KER_n+1)%64;                  /* Bump packet number */
      return(KER_FILE);                      /* Go back to Receive File state */

   case KER_ERROR:                           /* Error packet received */
      return(KER_PrintErrPacket(packet));    /* Print it out and */

   case FALSE:                               /* Didn't get packet */
   default: 
      flushRBuff(); 
      KER_SndPacket(KER_NACK, KER_n, 0, 0);  /* Return a NAK */
      showBErrors(++xferErrors);
      return(KER_state);                     /* Keep trying */
   }
}

/*---------------------------------------------------------------------------*/
/* KER_Send() - state table switcher for sending files                 [rkh] */
/*---------------------------------------------------------------------------*/
BOOL FAR KER_Send()
{
   LONG  dummy;

   KER_Init(KER_SEND);

   if(answerMode)
      delay(100, &dummy);                    /* Sleep to give the guy a chance */

   flushRBuff();                             /* Flush pending input */

   while(TRUE)                               /* Do this as long as necessary */
   {
      if(xferStopped && (KER_state != KER_ABORT))
      {
         KER_Abort(STR_KER_SNDABORT);
         return(FALSE);
      }
      if((KER_numtry++ > KER_MAXRETRY) && (KER_state != KER_ABORT))  /* If too many tries, give up */
      {
         KER_Abort(STR_KER_RETRYABORT);
         return(FALSE);
      }
      rxEventLoop();

      KER_bSetUp(KER_state);


      switch(KER_state)
      {
         case KER_SEND:
            KER_state = KER_SendInit();
            break;
         case KER_FILE:
            KER_state = KER_SendFile();
            break;
         case KER_DATA:
            KER_state = KER_SendData();
            break;
         case KER_EOF:
         case KER_BREAK:
            KER_state = KER_SendGeneric(KER_state);
            break;
         case KER_CMPLT:
            delay((WORD) 60, NULL);          /* tgex delay 1 sec for last packet */
            return(TRUE);
         default:
            KER_bSetUp('?');
            /* fall through */
         case KER_ABORT:
            delay((WORD) 60, NULL);          /* tgex delay 1 sec for last packet */
            return(FALSE);
      }
   }
}

/*---------------------------------------------------------------------------*/
/* KER_SendInit() - send parameters to other side                      [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_SendInit()
{
   INT num, len;                             /* Packet number, length */
   LONG dummy;

   KER_SndPacketInit(packet);                /* Fill up init info packet */
   KER_SndPacket(KER_SEND, KER_n, 8, packet);/* Send an S packet */

   switch(KER_RcvPacket(&len, &num, recpkt)) /* What was the reply? */
   {
      case KER_RCV:
         return(KER_state);                  /* remote get */
      case KER_NACK:  
         flushRBuff(); 
         showBErrors(++xferErrors);
         return(KER_state);                  /* NAK, try it again */
      case KER_ACK:                          /* ACK */
         if(KER_n != num)                    /* If wrong ACK, stay in S state */
         {
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state);               /* and try again */
         }
         KER_RcvPacketInit(recpkt);                   /* Get other side's init info */
         KER_eol = (KER_eol == 0) ? '\n' : KER_eol;   /* Check and set defaults */
         KER_quote = (KER_quote == 0) ? '#' : KER_quote; 
         KER_numtry = 0;                     /* Reset try counter */
         KER_n = (KER_n+1) % 64;             /* Bump packet count */
         return(KER_FILE);                   /* OK, switch state to F */
      case KER_ERROR:                        /* Error packet received */
         return(KER_PrintErrPacket(recpkt)); /* Print it out and */
      case FALSE:
      default: 
         flushRBuff(); 
         showBErrors(++xferErrors);
/* debug:rkh 60 ticks ??? */
         delay(60,&dummy);
         return(KER_state);                  /* Receive failure, try again */
   }
}

/*---------------------------------------------------------------------------*/
/* KER_SendFile() -send the file header                                [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_SendFile()
{
   INT num, len;                             /* Packet number, length */
   BYTE filnam1[50],                         /* Converted file name */
   *newfilnam,                               /* Pointer to file name to send */
   *cp;                                      /* BYTE pointer */

   KER_SndPacket(KER_FILE, KER_n, (INT)*xferFname, xferFname+1);   /* Send an F packet */

   switch(KER_RcvPacket(&len, &num, recpkt))    /* What was the reply? */
   {       
      case KER_NACK:                            /* NAK, just stay in this state, */
         num = ((--num < 0) ? 63 : num);        /* unless it's NAK for next packet */
         if(KER_n != num)                       /* which is just like an ACK for */ 
         {                                      /* this packet so fall thru to... */
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state); 
         }
         /* fall through if nacking next packet
            ... we did not get last ACK, but they sent it */
      case KER_ACK:                             /* ACK */
         if(KER_n != num)                       /* If wrong ACK, stay in F state */
         {                      
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state); 
         }
         KER_numtry = 0;                        /* Reset try counter */
         KER_n = (KER_n+1) % 64;                /* Bump packet count */
         KER_size = KER_BufferFill(packet);     /* Get first data from file */
         return(KER_DATA);                      /* Switch state to D */
      case KER_ERROR:                           /* Error packet received */
         return(KER_PrintErrPacket(recpkt));    /* Print it out and */
      case FALSE:                               /* Receive failure, stay in F state */
      default:                                  /* Something else, just abort */
         flushRBuff(); 
         showBErrors(++xferErrors);
         return(KER_state); 
   }
}

/*---------------------------------------------------------------------------*/
/* KER_SendData() -send the data packets                               [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_SendData()
{
   INT num, len;                             /* Packet number, length */

   KER_SndPacket(KER_DATA, KER_n, KER_size, packet); /* Send a D packet */

   switch(KER_RcvPacket(&len, &num, recpkt)) /* What was the reply? */
   {        
      case KER_NACK:                         /* NAK, just stay in this state, */
         num = ((--num < 0) ? 63 : num);     /* unless it's NAK for next packet */
         if(KER_n != num)                    /* which is just like an ACK for */
         {                                   /* this packet so fall thru to... */
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state); 
         } 
         /* fall through if nacking next packet
            ... we did not get last ACK, but they sent it */
      case KER_ACK:                          /* ACK */
         if(KER_n != num)                    /* If wrong ACK, fail */
         {
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state); 
         }
         updateProgress(FALSE);
         updateTimer();
         KER_numtry = 0;                     /* Reset try counter */
         KER_n = (KER_n+1) % 64;             /* Bump packet count */
         if((KER_size = KER_BufferFill(packet)) == EOF)     /* Get data from file */
            return(KER_EOF);                 /* If EOF set state to that */
         return(KER_DATA);                   /* Got data, stay in state D */
      case KER_ERROR:                        /* Error packet received */
         return(KER_PrintErrPacket(recpkt)); /* Print it out and */
      case FALSE:                            /* Receive failure, stay in D */
      default:
         flushRBuff(); 
         showBErrors(++xferErrors);
         return(KER_state); 
      }
}

/*---------------------------------------------------------------------------*/
/* KER_SendGeneric() -                                                 [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_SendGeneric(BYTE lKER_state)
{
   INT num, len;

   KER_SndPacket(lKER_state, KER_n, 0, packet);        /* Send a KER_EOF packet */

   switch(KER_RcvPacket(&len, &num, recpkt)) /* What was the reply? */
   {
      case KER_NACK:                         /* NAK, just stay in this state, */
         num = ((--num < 0) ? 63 : num);     /* unless NAK for previous packet, */

         if(KER_n != num)                    /* which is just like an ACK for */
         {                                   /* this packet so fall thru to... */
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state); 
         }
         /* fall through if nacking next packet
            ... we did not get last ACK, but they sent it */
      case KER_ACK:                          /* ACK */
         if(KER_n != num)                    /* If wrong ACK, hold out */
         {
            flushRBuff(); 
            showBErrors(++xferErrors);
            return(KER_state); 
         }
         KER_numtry = 0;                     /* Reset try counter */
         KER_n = (KER_n+1) % 64;             /* and bump packet count */

         switch(lKER_state)
         {
            case KER_EOF:
               _lclose (xferRefNo);          /* Close the input file */
               xferRefNo = 0;                /* Set flag indicating no file open */ 
               if(!KER_GetNextFile())        /* No more files go? */
                  return(KER_BREAK);         /* if not, break, EOT, all done */
               return(KER_FILE);             /* More files, switch state to F */
            case KER_BREAK:
               return(KER_CMPLT);            /* Switch state to Complete */
         }
      case KER_ERROR:                        /* Error packet received */
         return(KER_PrintErrPacket(recpkt)); /* Print it out and */
      case FALSE:                            /* Receive failure, stay in Z */
      default:
         flushRBuff(); 
         showBErrors(++xferErrors);
         return(KER_state); 
   }
}

/*---------------------------------------------------------------------------*/
/*
 * KERMIT utilities.
 */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* KER_Init() initializes kermit variables                             [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_Init(BYTE state)
{
   KER_eol = ((trmParams.emulate == ITMDELTA) && trmParams.localEcho) ? XOFF : CR;
                                             /* mbbx 1.10: CUA */
                                             /* EOL for outgoing packets (half duplex) */
                                             /* EOL for outgoing packets */
   KER_escchr = DEFESC;                      /* Default escape character */
   KER_filnamcnv = DEFFNC;                   /* Default filename case conversion */
   KER_firstfile = TRUE;                     /* First file transfer */
   KER_image = DEFIM;                        /* Default image mode (TRUE) */
   KER_lecho = KER_GetLocalEcho();           /* Get local echo info */
   KER_n = 0;                                /* Initialize message number */
   KER_numtry = 0;                           /* Say no tries yet */
   KER_pad = 0;                              /* No padding */
   KER_padchar = KER_PADCHAR;                /* Use null if any padding wanted */
   KER_pktdeb = FALSE;                       /* No packet file debugging */
   KER_quote = '#';                          /* Standard control-quote char */
   KER_state = state;                        /* Receive-Init is the start state */
   KER_timeout = FALSE;                      /* No timeout has occurred yet */
   KER_timint = KER_SNDTIMEOUT;              /* Start with default time outs. */
   KER_turn = KER_GetTurnAroundTime();       /* Get turnaround info */
   KER_8flag   = FALSE;  /* start with 8th bit quote flag off */ 

   KER_parMask = 0;       /* tgex used for auto parity */
   KER_mask = 0xFF;       /* used for checksumming with autoparity, to screen parity */
	KER_initState = 0;     /* tgex */
   KER_select8 = '\0';    /* tgex set to illegal value, set later */

   xferErrors = 0;
   xferStopped = FALSE;

   KER_parflg = KER_GetParity();             /* Get parity info -- before changed */
                                             /* if we are sending, we need it. */ 
}

/*---------------------------------------------------------------------------*/
/* KER_GetParity() - get parity setting                                [rkh] */
/*---------------------------------------------------------------------------*/
INT NEAR KER_GetParity()
{
   switch(trmParams.parity)
   {
      case ITMODDPARITY:
         return(KER_ODDPARITY);
      case ITMEVENPARITY:
         return(KER_EVENPARITY);
      case ITMMARKPARITY:
         return(KER_MARKPARITY);
      case ITMSPACEPARITY:
         return(KER_SPACEPARITY);
      case ITMNOPARITY:
      default:
         return(KER_NOPARITY);
   }
}

/*---------------------------------------------------------------------------*/
/* KER_GetTurnAroundTime() - get turnaround parameter                  [rkh] */
/*---------------------------------------------------------------------------*/
INT NEAR KER_GetTurnAroundTime()
{
   return(FALSE);

   switch(trmParams.flowControl)
   {
      case ITMXONFLOW:
         return (TRUE);
      case ITMNOFLOW:
      case ITMHARDFLOW:
      default:
         return (FALSE);
   }
}

/*---------------------------------------------------------------------------*/
/* KER_GetLocalEcho() - get local echo parameter                       [rkh] */
/*---------------------------------------------------------------------------*/
INT NEAR KER_GetLocalEcho()
{
   return(trmParams.localEcho);              /* mbbx 1.10: CUA... */
}

/*---------------------------------------------------------------------------*/
/* KER_Abort() - Kermit abort transfer routine                         [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_Abort(WORD msgID)
{
   LONG  dummy;

   LoadString(hInst, msgID, (LPSTR)taskState.string, 80);  /* mbbx 1.04: REZ... */
   KER_SndPacket(KER_ERROR, KER_n, strlen(taskState.string), taskState.string);

   if(xferFlag == XFRBSND)
      sndAbort();
   else
      rcvAbort();

   delay(180, &dummy);
   flushRBuff();

   return(KER_ABORT);
}

/*---------------------------------------------------------------------------*/
/* KER_DoParity()- do parity on character ch                           [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_DoParity(BYTE ch)
{
   INT a;

   if (!KER_parflg)                       /* tgex: was masking out bit 8 */
      return (ch);

   ch &= 0177;

   switch(KER_parflg)
   {
      case KER_ODDPARITY:
         ch |= 0x80;
      case KER_EVENPARITY:
         a = (ch & 15) ^ ((ch >> 4) & 15);
         a = (a & 3) ^ ((a >> 2) & 3);
         a = (a & 1) ^ ((a >> 1) & 1);
         return((ch & 0177) | (a << 7));
      case KER_MARKPARITY:             /* tgex ITMMARKPARITY: */
         return(ch | 0x80);
      case KER_SPACEPARITY:            /* tgex ITMSPACEPARITY: */
         return(ch & 0x7F);
      default:
         return(ch);
   }
}

/*---------------------------------------------------------------------------*/
/* KER_SndPacket()- send a kermit packet                               [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_SndPacket(BYTE  type, int   num, int len, BYTE  *data)
{
   INT i;                                    /* Character loop counter */
   BYTE chksum, buffer[200];                 /* Checksum, packet buffer */
   register BYTE *bufp;                      /* Buffer pointer */

   bufp = buffer;                            /* Set up buffer pointer */

   for(i=1; i <= KER_pad; i++)
      modemWr(KER_padchar);                  /* Issue any padding */

   *bufp++ = KER_DoParity(CHSOH);            /* Packet marker, ASCII 1 (SOH) */

   *bufp++ = KER_DoParity(tochar(len+3));    /* Send the character count */
   chksum  = tochar(len+3) & KER_mask;       /* Initialize the checksum */

   *bufp++ = KER_DoParity(tochar(num));      /* Packet number */
   chksum += tochar(num) & KER_mask;         /* Update checksum */

   *bufp++ = KER_DoParity(type);             /* Packet type */
   chksum += type & KER_mask;                /* Update checksum */

   for(i=0; i<len; i++)                      /* Loop for all data characters */
   {
      *bufp++ = KER_DoParity(data[i]);       /* Get a character */
      chksum += data[i] & KER_mask;          /* Update checksum */
   }

   chksum = (((chksum & 0300) >> 6) + chksum) & 077;  /* Compute final checksum */

   *bufp++ = KER_DoParity(tochar(chksum));      /* Put it in the packet */
   *bufp++ = KER_DoParity(KER_eol);             /* Extra-packet line terminator */

   modemWrite((LPSTR)buffer, (INT) (bufp - buffer));    /* mbbx 1.03: isolate comm... */
}

/*---------------------------------------------------------------------------*/
/* KER_RcvPacket()- read a kermit packet                               [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_RcvPacket(INT   *len, INT *num, BYTE  *data)
{
   INT i, tries, done;                       /* Data character number, loop exit */
   BYTE t,                                   /* Current input character */
        type,                                /* Packet type */
        rchksum;                             /* Checksum received from other host */
   BOOL resync;

   tries = 0;

   while(unpar(t = KER_InChar()) != CHSOH)              /* Wait for packet header */
   {
      if(KER_timeout || xferStopped)
      {
         KER_timeout = FALSE;
         return(FALSE);
      }
   }

   done = FALSE;                             /* Got SOH, init loop */

   while(!done)                              /* Loop to get a packet */
   {
      KER_parMask = t & 0x80;         /* Get SOH parity bit */
      KER_cchksum = 0;                       /* reset checksum */
      if((t = unpar(KER_CInChar())) == CHSOH)
         continue;                           /* Resynchronize if SOH */
      *len = unchar(t)-3;             /* Character count */

      if((t = unpar(KER_CInChar())) == CHSOH)
         continue;                           /* Resynchronize if SOH */
      *num = unchar(t);               /* Packet number */

      if(unpar(t = KER_CInChar()) == CHSOH)
         continue;                           /* Resynchronize if SOH */
      type = unpar(t);                       /* Packet type */
      KER_parMask |= (t & 0x80) >> 1;        /* get packet type parity into bit 6 of KER_parMask */

   /* Put len characters from the packet into the data buffer */
       
      resync = FALSE;
      for (i=0; i < *len; i++)
      {
         if((data[i] = KER_mask & (t = KER_CInChar())) == CHSOH) /* Resynch if SOH */
                     /* tgex ^ use KER_mask to get rid of parity bits in data of initial packet sent */
         {
            resync = TRUE;
            break;
         }
         if(KER_timeout || xferStopped)
         {
            KER_timeout = FALSE;
            return(FALSE);
         }
      }
      if(resync)
         continue;

      data[*len] = 0;                        /* Mark the end of the data */

      if((t = unpar(KER_InChar())) == CHSOH)
         continue;                           /* Resynchronize if SOH */
      rchksum = unchar(t);                   /* Convert to numeric */
      done = TRUE;                           /* Got checksum, done */
   }
                                             /* Fold in bits 7,8 to compute */
   KER_cchksum = (((KER_cchksum & 0300) >> 6) + KER_cchksum) & 077; /* final checksum */

   if(KER_timeout || xferStopped)
   {
      KER_timeout = FALSE;
      return(FALSE);
   }

   if(KER_cchksum != rchksum)
   {
      return(FALSE);
   }

   return(type);                             /* All OK, return packet type */
}

/*---------------------------------------------------------------------------*/
/* KER_CInChar() - get parity adjusted char and update checksum        [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_CInChar()
{
    BYTE ch;

    ch = KER_InChar();                       /* Get a character */
    KER_cchksum += ch & KER_mask;            /* Add to the checksum, drop parity bit if first packet */
    return(ch);
}

/*---------------------------------------------------------------------------*/
/* KER_InChar()- get parity adjusted character                         [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_InChar()
{
   BYTE  ch;

   ch = KER_ModemWait();

   return((KER_parflg || KER_parMask) ? ch & 0x7F : ch);
}

/*---------------------------------------------------------------------------*/
/* KER_ModemWait() -                                                   [rkh] */
/*---------------------------------------------------------------------------*/
INT NEAR KER_ModemWait()                            /* mbbx 0.81 */
{
   BYTE  theChar;

   if(!waitRcvChar(&theChar, (KER_timint * 10), 0, 0))
      KER_timeout = TRUE;                             /* rjs */

   return((INT) theChar);
}

/*---------------------------------------------------------------------------*/
/* KER_BufferFill() - fill up a buffer from file that is being sent    [rkh] */
/*---------------------------------------------------------------------------*/
INT NEAR KER_BufferFill(BYTE *buffer)
{
   INT   i = 0;                              /* Loop index */
   INT   bytes = 0;
   INT   t;                                  /* Char read from file */
   BYTE  t7;                                 /* 7-bit version of above */
   BYTE  t8;                                 /* 8-bit flag */

   /* myh swat: if all chars in the buffer have been transfered */
   if (KER_bytetran >= KER_buffsiz) 
   {
      /* read in the next buffer from the file */
      if ((KER_buffsiz = _lread(xferRefNo, KER_buffer, BUFFSIZE)) <= 0)
         return(EOF);
      /* reset the number of bytes transfered for next read in buffer */
      else
         KER_bytetran = 0;
   }   /* myh swat */

   /* myh swat: continue to fill the packet until the full packet size */
   /*           or the end of the read buffer                          */
   while ((i < KER_spsiz - 9) && (KER_bytetran < KER_buffsiz))
   {
      xferBytes--;
      t = KER_buffer[KER_bytetran++];   /* myh swat: check one char at a time */
      t7 = t & 0x7F;                    /* Get low order 7 bits */
      t8 = t & 0x80;

      if(KER_8flag && t8)                    /* do 8th bit quoting, this char */
      {
         buffer[i++] = KER_select8;
         t = t7;
      }

      if((t7 < SP) || (t7 == DEL))
      {
         buffer[i++] = KER_quote;
         t = ctl(t);
      }
      else if(t7 == KER_quote)
         buffer[i++] = KER_quote;
      else if(KER_8flag && (t7 == KER_select8)) 
         buffer[i++] = KER_quote;

      buffer[i++] = t;
   }

   buffer[i] = '\0';
   return(i);
}

/*---------------------------------------------------------------------------*/
/* KER_BufferEmpty() - decode an incoming packet                       [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_BufferEmpty(BYTE  *buffer, int len, BYTE  flag)
{
   INT   i;                                 /* Counter*/
   INT   KER_bufflen = 0;                   /* buffer length */
   BYTE  b8, t7, t;                         /* Character holder */
   BYTE  WriteBuffer[KER_MAXPACKSIZE];      /* myh swat: create a buffer so later can write */
                                            /*     the whole buffer instead of char by char */

   for(i=0; i<len; i++)                     /* Loop thru the data field */
   {
      t = buffer[i];                        /* Get character */
      b8 = 0;                               /* 8th bit flag */

      if(KER_8flag && (t == KER_select8))
      {
         b8 = 128;
         t = buffer[++i];
      }

      if(t == KER_QUOTE)                     /* Control quote? */
      {                                     /* Yes */
         t = buffer[++i];                   /* Get the quoted character */
         t7 = t & 127;
         if((t7 > 62) && (t7 < 96))
            t = ctl(t);
      }
      t |= b8;

      if((t == CR) && !KER_image)               /* Don't pass CR if in image mode */
         continue;
      switch (flag)
      {
      case KERFILE:
         WriteBuffer[KER_bufflen++] = t;   /* myh swat: store char to a buffer */         
         KER_bytes++;
         break;
      case KERSCREEN:
         modemInp(t, TRUE);
         KER_bytes++;
         break;
      case KERBUFF:
         KER_buff[KER_bufflen++] = t;
         break;
      }
   }

   /* myh swat: write the buffer to a file */
   if (flag == KERFILE)
         _lwrite(xferRefNo, WriteBuffer, KER_bufflen);   

   KER_buff[KER_bufflen] = 0;
}

/*---------------------------------------------------------------------------*/
/* KER_GetNextFile() - get next file in a file group                   [rkh] */
/*---------------------------------------------------------------------------*/
BOOL NEAR KER_GetNextFile()
{
   return FALSE;
}

/*---------------------------------------------------------------------------*/
/* KER_PrintErrPacket() - print an error packet                        [rkh] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_PrintErrPacket(BYTE  *msg)
{
   if(!answerMode)
   {
      LoadString(hInst, STR_KER_XFERABORT, (LPSTR)taskState.string, 80);  /* mbbx 1.04: REZ... */
      sprintf(outstr, taskState.string, msg);
      KER_PutScreenStr(outstr);
   }

   xferStopped = TRUE;
   return(KER_ABORT);                     /* abort */
}

/*---------------------------------------------------------------------------*/
/* KER_RcvPacketInit() - get the initalization stuff from other side   [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_RcvPacketInit(BYTE *data)
{
   BYTE tmp;

/* rjs swat 011 -> unpar()'d each of the characters in data[] per Altitude fix */
   KER_spsiz = unchar(unpar(data[0]));              /* Maximum send packet size */
   KER_timint = unchar(unpar(data[1]));             /* When I should time out */

   if(KER_timint == 0) 
      KER_timint = 32000;                    /* 32000 is close enough to infinite */
   else if((KER_timint > KER_MAXTIMEOUT) || (KER_timint < KER_MINTIMEOUT))
      KER_timint = (xferFlag == XFRBSND) ? KER_SNDTIMEOUT : KER_RCVTIMEOUT;

   KER_pad = unchar(unpar(data[2]));                /* Number of pads to send */
   KER_padchar = ctl(unpar(data[3]));               /* Padding character to send */

   if(trmParams.emulate != ITMDELTA)         /* (mbbx) for DELTA, send XOFF */
      KER_eol = unchar(unpar(data[4]));             /* EOL character I must send */

   KER_quote = unpar(data[5]);                      /* Incoming data quote character */
  
/* tge comment: we need to check the packet length before the next section
                to see if they even have the 8bit quote slot present */

   tmp = unpar(data[6]);
   if (KER_initState <= 0)         /* tgex */
   {  /* we are the receiver */
      if(tmp == 'Y')
      {
         if (KER_parflg)
         {
            KER_select8 = '&';
            KER_8flag = TRUE;
         }
         else
            KER_select8 = 'N';               /* KER_8flag stays FALSE */
      }
      else if(tmp == 'N')
        KER_select8 = 'N';                  /* KER_8flag stays FALSE */
      else if(((tmp > ' ') && (tmp < '?')) || 
               ((tmp > '_') && (tmp < DEL)))   /* jtf 3.20 this was > DEL */
      {
         KER_select8 = tmp;
         KER_8flag = TRUE;
      }
   }
   else
   {                                      /* we are the sender */
      if (KER_select8 == '&')
      {
         if ((tmp == '&') || (tmp == 'Y'))
            KER_8flag = TRUE;
      }
      else                                /* KER_select8 == 'Y' */
       if(((tmp > ' ') && (tmp < '?')) || 
            ((tmp > '_') && (tmp < DEL)))   /* jtf 3.20 this was > DEL */
         {
            KER_select8 = tmp;
            KER_8flag = TRUE;
         }
   }


  KER_initState = -1;

}

/*---------------------------------------------------------------------------*/
/* KER_SndPacketInit() - send the initalization stuff to other side     [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_SndPacketInit(BYTE *data)
{
   data[KER_INIT_MAXL] = tochar(KER_MAXPACKSIZE);     /* Biggest packet I can receive */

   data[KER_INIT_TIME] =                              /* When I want to be timed out */
      (xferFlag == XFRBSND) ? tochar(KER_SNDTIMEOUT) : tochar(KER_RCVTIMEOUT);

   data[KER_INIT_NPAD] = tochar(KER_NPAD);            /* How much padding I need */
   data[KER_INIT_PADC] = ctl(KER_PADCHAR);            /* Padding character I want */
   data[KER_INIT_EOL]  = tochar(KER_EOL);             /* End-Of-Line character I want */
   data[KER_INIT_QCTL] = KER_QUOTE;                   /* Control-Quote character I send */

   if (KER_initState >= 0)
   {                                            /* we are the sender */
      KER_select8 = (KER_parflg) ? '&' : 'Y';
   }
   else                                         /* we are the receiver */
   {
      if (KER_select8 == 'Y')                   /* they sent us yes */
      {
         if (KER_parflg)
         {
            KER_select8 = '&';
            KER_8flag = TRUE;
         }
         else
            KER_select8 = 'N';               /* KER_8flag stays FALSE */
      }
      else                                   /* they told us to quote */
         if(((KER_select8 > ' ') && (KER_select8 < '?')) || 
            ((KER_select8 > '_') && (KER_select8 < DEL)))
         {
            KER_select8 = data[6];
            KER_8flag = TRUE;
         }
   }

   data[KER_INIT_QBIN] = KER_select8;                 /* 8th bit quote char */
   data[KER_INIT_CHKT] = KER_BLOCKCHK1;
   data[KER_INIT_CHKT+1] = '\0';

   KER_initState = 1;
}


/*---------------------------------------------------------------------------*/
/* KER_PutScreenStr() - put data onto the screen                       [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_PutScreenStr(BYTE *data) 
{
   INT i;
#ifdef ORGCODE
   for(i=0; i<strlen(data); i++)
#else
   for(i=0; i< lstrlen(data); i++)
#endif
      modemInp(data[i], FALSE);

   modemInp(CR, FALSE);
   modemInp(LF, FALSE);
}

/*---------------------------------------------------------------------------*/
/* KER_bSetUp()                                                        [rkh] */
/*---------------------------------------------------------------------------*/
VOID NEAR KER_bSetUp(BYTE state)
{
   BYTE stateStr[2];

   *stateStr = state;
   stateStr[1] = 0;
}

/*---------------------------------------------------------------------------*/
/* KER_AutoPar()                                                       [tge] */
/*---------------------------------------------------------------------------*/
BYTE NEAR KER_AutoPar()
{
   switch(KER_parMask)
   {
      case 0x80:
         return(KER_EVENPARITY);
      case 0xC0:
         return(KER_MARKPARITY);
      case 0x40:
         return(KER_ODDPARITY);
      case 0x00:
         return(KER_NOPARITY);
      default:
         return(-1);
   }
}
