/*****************************************************************************/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*****************************************************************************/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"


/*---------------------------------------------------------------------------*/

#define XM_ABORT                    0x8000
#define XM_COMPLETE                 0x4000
#define XM_BLKREPEAT                0x2000
#define XM_CRC                      0x0800
#define YM_1KBLK                    0x0400
#define YM_GOPTION                  0x0200
#define XM_RETRYMASK                0x001F   /* mbbx 1.04: relax 15 -> 31 */

#define XM_RETRIES                  0x0014   /* mbbx 1.04: relax 10 -> 20 */
#define XM_RETRYINITCRC             4
#define XM_RETRYINITCKS             10

#define XM_WAITRCVINIT              50       /* mbbx 1.04: relax... */
#define XM_WAITNEXTBLK              100
#define XM_WAITNEXTCHAR 	    50	     //sdj: was 20 to get rid of xmodem
					     //sdj: retries when moused moved..move to 50
#define XM_WAITSNDINIT              600


BOOL YM_RcvBatch(WORD);
BOOL NEAR YM_RcvFileInfo(WORD *, WORD *);

BOOL XM_RcvFile(WORD);
BOOL NEAR XM_RcvInit(WORD *, WORD *);
BOOL NEAR XM_RcvData(WORD *, WORD *);
BOOL NEAR XM_RcvBlockHeader(WORD *, WORD *);
BOOL NEAR XM_RcvBlockData(WORD *blockNumber, WORD blockSize,WORD *rcvStatus);
VOID NEAR XM_RcvBlockAbort(WORD *);
BOOL NEAR XM_RcvEnd();
VOID NEAR XM_RcvAbort();

BOOL YM_SndBatch(WORD);
BOOL NEAR YM_SndFileInfo(WORD *, BOOL);

BOOL XM_SndFile(WORD sndStatus);
BOOL NEAR XM_SndInit(WORD *);
BOOL NEAR XM_SndData(WORD *);
BOOL NEAR XM_SndBlockData(WORD *, WORD *, WORD *);
BOOL NEAR XM_SndEnd();
VOID NEAR XM_SndAbort();

BYTE XM_CheckSum(BYTE *dataBlock, WORD blockSize);              /* mbbx 2.00: NEAR -> FAR */
WORD XM_CalcCRC(BYTE *, INT);                /* mbbx 2.00: NEAR -> FAR */


/*---------------------------------------------------------------------------*/
/* UTILITIES --> RCVBFILE.C */

BOOL initXfrBuffer(WORD wBufSize);
VOID fillXfrBuffer(BYTE *, WORD);
WORD readXfrBuffer(BYTE *dataBlock,WORD  blockSize,BOOL bBlkRepeat);
BOOL writeXfrBuffer(BYTE *dataBlock, WORD blockSize,BOOL bBlkRepeat);
VOID grabXfrBuffer(BYTE *, WORD);
BOOL clearXfrBuffer();
VOID freeXfrBuffer();


/*---------------------------------------------------------------------------*/
/* XM_RcvFile() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL XM_RcvFile(WORD rcvStatus)
{
   WORD  blockSize;

   if(XM_RcvInit(&blockSize, &rcvStatus))
      if(XM_RcvData(&blockSize, &rcvStatus))
         if(XM_RcvEnd())
            return(TRUE);

   XM_RcvAbort();
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_RcvInit() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_RcvInit(WORD  *blockSize, WORD  *rcvStatus)
{
   BYTE  work[3];
   WORD  retry;

   LoadString(hInst, STR_RI, (LPSTR) work, 4);  /* mbbx 1.04: REZ... */
   bSetup(work);

   *blockSize = 128;
   *rcvStatus |= XM_RETRIES;

   if(*rcvStatus & (YM_1KBLK | YM_GOPTION))
      *rcvStatus |= XM_CRC;

   while(*rcvStatus & XM_CRC)
   {
      for(retry = XM_RETRYINITCRC; retry > 0; retry -= 1)
      {
         modemWr('C');

         if(xferPSChar)                      /* mbbx 1.02: packet switching */
            modemWr(xferPSChar);

         if(waitRcvChar(work, XM_WAITRCVINIT, 0, CHSTX, CHSOH, CHEOT, CHCAN, NULL))
         {
            switch(work[0])
            {
            case CHSTX:
               *blockSize = 1024;
                                             /* then fall thru... */
            case CHSOH:
               return(TRUE);

            case CHEOT:
               *rcvStatus |= XM_COMPLETE;
               return(TRUE);

            case CHCAN:
               return(FALSE);
            }
         }

         if(xferStopped)
            return(FALSE);
      }

      *rcvStatus &= ((*rcvStatus & YM_GOPTION) ? ~YM_GOPTION : ~XM_CRC);
   }

   for(retry = XM_RETRYINITCKS; retry > 0; retry -= 1)   /* mbbx 1.04: relax */
   {
      modemWr(CHNAK);
      if(xferPSChar)                         /* mbbx 1.02: packet switching */
         modemWr(xferPSChar);

      if(waitRcvChar(work, XM_WAITRCVINIT, 0, CHSTX, CHSOH, CHEOT, CHCAN, NULL))
      {
         switch(work[0])
         {
         case CHSTX:
            *blockSize = 1024;
                                             /* then fall thru... */
         case CHSOH:
            return(TRUE);

         case CHEOT:
            *rcvStatus |= XM_COMPLETE;
            return(TRUE);

         case CHCAN:
            return(FALSE);
         }
      }

      if(xferStopped)
         break;

      showBErrors(++xferErrors);
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_RcvData() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_RcvData(WORD  *blockSize, WORD  *rcvStatus)
{
   BYTE  work[3];
   WORD  blockNumber = 1;

   if(*rcvStatus & XM_COMPLETE)
      return(TRUE);

   LoadString(hInst, STR_DF, (LPSTR) work, 4);  /* mbbx 1.04: REZ... */
   bSetup(work);

   if(initXfrBuffer(12 * 1024))
   {
      XM_RcvBlockData(&blockNumber, *blockSize, rcvStatus);

      while(!(*rcvStatus & (XM_COMPLETE | XM_ABORT)))
      {
         if(XM_RcvBlockHeader(blockSize, rcvStatus))
            XM_RcvBlockData(&blockNumber, *blockSize, rcvStatus);
      }

      if(!(*rcvStatus & XM_ABORT))
         if(!clearXfrBuffer())
            *rcvStatus |= XM_ABORT;

      freeXfrBuffer();
   }

   return(!(*rcvStatus & XM_ABORT));
}


/*---------------------------------------------------------------------------*/
/* XM_RcvBlockHeader() -                                               [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_RcvBlockHeader(WORD  *blockSize, WORD  *rcvStatus)
{
   BYTE  work[1];

   if(waitRcvChar(work, XM_WAITNEXTBLK, 0, CHSTX, CHSOH, CHEOT, CHCAN, NULL))
   {
      switch(work[0])
      {
      case CHSTX:
         *blockSize = 1024;
         return(TRUE);

      case CHSOH:
         *blockSize = 128;
         return(TRUE);

      case CHEOT:
         *rcvStatus |= XM_COMPLETE;
         return(FALSE);

      case CHCAN:
         xferStopped = TRUE;
         break;
      }
   }

   XM_RcvBlockAbort(rcvStatus);
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_RcvBlockData() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_RcvBlockData(WORD *blockNumber, WORD blockSize,WORD *rcvStatus)
{
   BYTE  work[2];
   BOOL  bBlkRepeat;
   WORD  ndx;
   BYTE  dataBlock[1024];
   signed char i,j;

   while(waitRcvChar(work, XM_WAITNEXTCHAR, 0, 0) &&
         waitRcvChar(work+1, XM_WAITNEXTCHAR, 0, 0) &&
	 ( (j=(signed char)work[0]) == ~(i = (signed char)work[1])	) )

   //sdj: on mips xmodem rcv was broken due to (BYTE)~work[1]

   {
      if(bBlkRepeat = (work[0] != (BYTE) *blockNumber))
         if(work[0] != (BYTE) (*blockNumber-1))
            break;

      for(ndx = 0; ndx < blockSize; ndx += 1)
         if(!waitRcvChar(dataBlock+ndx, XM_WAITNEXTCHAR, 0, 0))
            break;
      if(ndx < blockSize)
         break;

      if(!waitRcvChar(work, XM_WAITNEXTCHAR, 0, 0))
         break;
      if(!(*rcvStatus & XM_CRC))
      {
         if(XM_CheckSum(dataBlock, blockSize) != work[0])
            break;
      }
      else
      {
         if(!waitRcvChar(work+1, XM_WAITNEXTCHAR, 0, 0) || (XM_CalcCRC(dataBlock, blockSize) != ((work[0] << 8) | work[1])))
            break;
      }

      if(!writeXfrBuffer(dataBlock, blockSize, bBlkRepeat))
      {
         xferStopped = TRUE;
         break;
      }

      if(!bBlkRepeat)                        /* mbb: reset retry counter */
      {
         *blockNumber += 1;
         *rcvStatus = (*rcvStatus & ~XM_RETRYMASK) | XM_RETRIES;

         if(*blockNumber > 1)                /* mbb: skip block 0 */
         {
            if(xferOrig > 0)
            {
               xferBytes -= blockSize;
               updateProgress(FALSE);
            }
            else
               showBBytes(xferLength += blockSize, FALSE);  /* mbbx 2.00: xfer ctrls */
         }
      }

      if(!(*rcvStatus & YM_GOPTION))
      {
         modemWr(CHACK);
         if(xferPSChar)                      /* mbbx 1.02: packet switching */
            modemWr(xferPSChar);
      }

      return(TRUE);
   }

   XM_RcvBlockAbort(rcvStatus);
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_RcvBlockAbort() -                                                [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR XM_RcvBlockAbort(WORD  *rcvStatus)
{
   BYTE  work[1];

   if(xferStopped || (((*rcvStatus -= 1) & XM_RETRYMASK) == 0))
      *rcvStatus |= XM_ABORT;
   else
   {
      while(waitRcvChar(work, XM_WAITNEXTCHAR, 0, 0));
      modemWr(CHNAK);
      if(xferPSChar)                         /* mbbx 1.02: packet switching */
         modemWr(xferPSChar);
   }

   if(!xferStopped)
      showBErrors(++xferErrors);
}


/*---------------------------------------------------------------------------*/
/* XM_RcvEnd() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_RcvEnd()
{
   BYTE  work[3];
   WORD  retry;

   LoadString(hInst, STR_RE, (LPSTR) work, 4);  /* mbbx 1.04: REZ... */
   bSetup(work);

   modemWr(CHACK);
   if(xferPSChar)                            /* mbbx 1.02: packet switching */
      modemWr(xferPSChar);

   for(retry = XM_RETRIES; retry > 0; retry -= 1)     /* mbbx 1.04: relax */
   {
      if(waitRcvChar(work, XM_WAITNEXTBLK / 2, 0, CHEOT, CHCAN, NULL))
      {
         switch(work[0])
         {
         case CHEOT:
            modemWr(CHACK);
            if(xferPSChar)                   /* mbbx 1.02: packet switching */
               modemWr(xferPSChar);
            showBErrors(++xferErrors);
            continue;

         case CHCAN:
            xferStopped = TRUE;
            break;
         }
      }

      if(xferStopped)
         break;

      return(TRUE);
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_RcvAbort() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR XM_RcvAbort()
{
   BYTE  work[1];

   rcvAbort();

   while(waitRcvChar(work, XM_WAITNEXTCHAR, 0, 0));
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(BS);
   modemWr(BS);
   modemWr(BS);
   modemWr(BS);
   modemWr(BS);
   if(xferPSChar)                            /* mbbx 1.02: packet switching */
      modemWr(xferPSChar);
}


/*---------------------------------------------------------------------------*/
/* XM_SndFile() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL XM_SndFile(WORD sndStatus)
{
   if(XM_SndInit(&sndStatus))
      if(XM_SndData(&sndStatus))
         if(XM_SndEnd())
            return(TRUE);

   XM_SndAbort();
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_SndInit() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_SndInit(WORD  *sndStatus)
{
   BYTE  work[3];

   LoadString(hInst, STR_SI, (LPSTR) work, 4);  /* mbbx 1.04: REZ... */
   bSetup(work);

   *sndStatus |= XM_RETRIES;

   if(waitRcvChar(work, XM_WAITSNDINIT, 0, 'C', CHNAK, CHCAN, 0))
   {
      switch(work[0])
      {
      case 'C':
         *sndStatus |= XM_CRC;
         if(!(*sndStatus & YM_1KBLK) && waitRcvChar(work, XM_WAITNEXTCHAR / 2, 0, 'K', 0))
            *sndStatus |= YM_1KBLK;
         return(TRUE);

      case CHNAK:
         *sndStatus &= ~(XM_CRC | YM_1KBLK);
         return(TRUE);

      case CHCAN:
         break;
      }
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_SndData() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_SndData(WORD  *sndStatus)
{
   BYTE  work[3];
   WORD  blockNumber = 1;
   WORD  blockSize;

   LoadString(hInst, STR_DF, (LPSTR) work, 4);  /* mbbx 1.04: REZ... */
   bSetup(work);

   if(initXfrBuffer(12 * 1024))
   {
      blockSize = (!(*sndStatus & YM_1KBLK) ? 128 : 1024);

      while(!(*sndStatus & (XM_COMPLETE | XM_ABORT)))
         XM_SndBlockData(&blockNumber, &blockSize, sndStatus);

      freeXfrBuffer();
   }

   return(!(*sndStatus & XM_ABORT));
}


/*---------------------------------------------------------------------------*/
/* XM_SndBlockData() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_SndBlockData(WORD *blockNumber, WORD *blockSize, WORD *sndStatus)
{
   BYTE  dataBlock[1024];
   WORD  dataBytes;
   WORD  wCRC;
   BYTE  work[1];
   BOOL  writeGood;

   switch(dataBytes = readXfrBuffer(dataBlock, *blockSize, (*sndStatus & XM_BLKREPEAT)))
   {
   case (WORD)-1:
      xferStopped = TRUE;
      break;

   case 0:
      *sndStatus |= XM_COMPLETE;
      return(TRUE);

   default:
      if((*blockSize == 1024) && (dataBytes <= (5 * 128)) && !(*sndStatus & XM_BLKREPEAT))
         readXfrBuffer(dataBlock, *blockSize = 128, TRUE);

      modemWr((*blockSize == 128) ? CHSOH : CHSTX);
      modemWr((BYTE) *blockNumber);
      modemWr((BYTE) ~*blockNumber);

      writeGood = modemWrite((LPSTR) dataBlock, (INT)(*blockSize));

      if(!writeGood)
      {
         wCRC = 0;
         break;
      }

      if(!(*sndStatus & XM_CRC))
         modemWr(XM_CheckSum(dataBlock, *blockSize));
      else
      {
         wCRC = XM_CalcCRC(dataBlock, *blockSize);
         modemWr(HIBYTE(wCRC));
         modemWr(LOBYTE(wCRC));
      }
      if(xferPSChar)                         /* mbbx 1.02: packet switching */
         modemWr(xferPSChar);

      if(!waitRcvChar(work, XM_WAITSNDINIT, 0, CHACK, CHNAK, CHCAN, (*blockNumber <= 1) ? 'C' : 0, 0))
      {
         xferStopped = TRUE;
         break;
      }

      switch(work[0])
      {
      case CHACK:
         *blockNumber += 1;
         *sndStatus = (*sndStatus & ~(XM_BLKREPEAT | XM_RETRYMASK)) | XM_RETRIES;

         if(*blockNumber > 1)
         {
            xferBytes -= *blockSize;
            updateProgress(FALSE);
         }
         return(TRUE);

      case CHNAK:
         break;

      case CHCAN:
         xferStopped = TRUE;
         break;
      }
      break;
   }

   if(xferStopped || (((*sndStatus -= 1) & XM_RETRYMASK) == 0))
      *sndStatus |= XM_ABORT;
   else
      *sndStatus |= XM_BLKREPEAT;

   if(!xferStopped)
      showBErrors(++xferErrors);

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_SndEnd() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR XM_SndEnd()
{
   BYTE  work[3];
   WORD  retry;

   LoadString(hInst, STR_SE, (LPSTR) work, 4);  /* mbbx 1.04: REZ... */
   bSetup(work);

   for(retry = XM_RETRIES; retry > 0; retry -= 1)     /* mbbx 1.04: relax */
   {
      modemWr(CHEOT);
      if(xferPSChar)                         /* mbbx 1.02: packet switching... */
         modemWr(xferPSChar);

      if(waitRcvChar(work, XM_WAITNEXTBLK, 0, CHACK, CHCAN, 0))   /* mbbx 1.04: relax 15 -> 60 */
      {
         switch(work[0])
         {
         case CHACK:
            return(TRUE);

         case CHCAN:
            xferStopped = TRUE;
            break;
         }
      }

      if(xferStopped)
         break;

      showBErrors(++xferErrors);
   }

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* XM_SndAbort() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR XM_SndAbort()
{
   BYTE  work[1];

   sndAbort();

   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(CHCAN);
   modemWr(BS);
   modemWr(BS);
   modemWr(BS);
   modemWr(BS);
   modemWr(BS);
   if(xferPSChar)                            /* mbbx 1.02: packet switching */
      modemWr(xferPSChar);
}


/*---------------------------------------------------------------------------*/
/* XM_CheckSum() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

BYTE XM_CheckSum(BYTE *dataBlock, WORD blockSize)       /* mbbx 2.00: NEAR -> FAR */
{
   BYTE  XM_CheckSum = 0;

   while(blockSize > 0)
      XM_CheckSum += dataBlock[--blockSize];

   return(XM_CheckSum);
}


/*---------------------------------------------------------------------------*/
/* XM_CalcCRC() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

WORD XM_CalcCRC(BYTE  *dataBlock, INT   blockSize)
{
   WORD  XM_CalcCRC = 0;
   INT   ndx;

   while(--blockSize >= 0)
   {
      XM_CalcCRC = XM_CalcCRC ^ (((WORD) *dataBlock++) << 8);
      for(ndx = 0; ndx < 8; ndx += 1)
      {
         if(XM_CalcCRC & 0x8000)
            XM_CalcCRC = (XM_CalcCRC << 1) ^ 0x1021;
         else
            XM_CalcCRC = (XM_CalcCRC << 1);
      }
   }

   return(XM_CalcCRC & 0xFFFF);
}


/*---------------------------------------------------------------------------*/
/* UTILITIES --> file buffering to be used by all RCV protocols !!!    [mbb] */
/*---------------------------------------------------------------------------*/

HANDLE   hXfrBuf;
LPSTR    lpXfrBuf;
WORD     wXfrBufSize;
WORD     wXfrBufBytes;
WORD     wXfrBufIndex;
WORD     wXfrBufExtend;


BOOL initXfrBuffer(WORD wBufSize)
{
   wXfrBufSize = wBufSize;
   if((hXfrBuf = GlobalAlloc(GMEM_MOVEABLE, (DWORD) wXfrBufSize)) != NULL)
   {
#ifdef ORGCODE
      if((lpXfrBuf = GlobalWire(hXfrBuf)) != NULL)
#else
      if((lpXfrBuf = GlobalLock(hXfrBuf)) != NULL)
#endif

      {
         wXfrBufBytes  = 0;
         wXfrBufIndex  = 0;
         wXfrBufExtend = 0;
         return(TRUE);
      }

      GlobalFree(hXfrBuf);
   }

   rcvFileErr();
   return(FALSE);
}



WORD readXfrBuffer(BYTE *dataBlock, WORD blockSize, BOOL bBlkRepeat)
{
   if(!bBlkRepeat)
      wXfrBufIndex += wXfrBufExtend;

   if((wXfrBufIndex+blockSize) > wXfrBufBytes)
   {
      if((wXfrBufBytes -= wXfrBufIndex) > 0)
         lmovmem(lpXfrBuf+wXfrBufIndex, lpXfrBuf, wXfrBufBytes);

      if((wXfrBufIndex = _lread(xferRefNo, lpXfrBuf, wXfrBufSize-wXfrBufBytes)) == -1)
      {
         return((WORD)-1);
      }

      wXfrBufBytes += wXfrBufIndex;
      wXfrBufIndex  = 0;
   }

   if((wXfrBufExtend = (wXfrBufBytes-wXfrBufIndex)) > 0)
   {
      if(wXfrBufExtend > blockSize)
         wXfrBufExtend = blockSize;
      lmovmem(lpXfrBuf+wXfrBufIndex, (LPSTR) dataBlock, wXfrBufExtend);
      if(wXfrBufExtend < blockSize)
         memset(dataBlock+wXfrBufExtend, CNTRLZ, blockSize-wXfrBufExtend);
   }

   return(wXfrBufExtend);
}


BOOL writeXfrBuffer(BYTE *dataBlock, WORD blockSize,BOOL bBlkRepeat)
{
   if(!bBlkRepeat)
      wXfrBufBytes += wXfrBufExtend;

   if((wXfrBufBytes+blockSize) > wXfrBufSize)
   {
      if(_lwrite(xferRefNo, lpXfrBuf, wXfrBufBytes) != wXfrBufBytes)
      {
         rcvFileErr();
         return(FALSE);
      }
      wXfrBufBytes = 0;
   }

   lmovmem((LPSTR) dataBlock, lpXfrBuf+wXfrBufBytes, wXfrBufExtend = blockSize);
   return(TRUE);
}


BOOL clearXfrBuffer()
{
   if(wXfrBufExtend > 0)
   {
      while(wXfrBufExtend > 0)               /* mbbx 1.04 ... */
      {
         if(*(lpXfrBuf+(wXfrBufBytes+wXfrBufExtend-1)) != CNTRLZ)
            break;
         wXfrBufExtend -= 1;
      }
      wXfrBufBytes += wXfrBufExtend;
      wXfrBufExtend = 0;
   }

   if(wXfrBufBytes > 0)
   {
      if(_lwrite(xferRefNo, lpXfrBuf, wXfrBufBytes) != wXfrBufBytes)
      {
         rcvFileErr();
         return(FALSE);
      }
      wXfrBufBytes = 0;
   }

   return(TRUE);
}


VOID freeXfrBuffer()
{
#ifdef ORGCODE
   GlobalUnWire(hXfrBuf);
#else
   GlobalUnlock(hXfrBuf);
#endif

   GlobalFree(hXfrBuf);
}
