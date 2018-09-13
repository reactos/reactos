/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "fileopen.h"
#include "task.h"

VOID sndBPre(BYTE  *fname, INT   actionString)
{
   showXferCtrls(IDSTOP | IDFORK | IDSCALE | IDSENDING | IDBERRORS);    /* mbbx 2.00: xfer ctrls... */
   showRXFname(fname, actionString);
}


/*---------------------------------------------------------------------------*/
/* sndBFileErr() -                                               [scf]       */
/*---------------------------------------------------------------------------*/

VOID sndBFileErr(INT    ioErrFlag, STRING *fileName)
{
   memcpy(taskState.string, fileName, *fileName+2);
   TF_ErrProc(ioErrFlag, MB_OK | MB_ICONHAND, 999);      /* taskState.error */
}

#ifdef ORGCODE
#else
VOID swapWords(PVOID pVoid)
{
WORD wTmpHigh,wTmpLow;
PWORD pw;

pw = (PWORD)pVoid;

wTmpHigh = *pw;
wTmpLow  = *(pw+1);
*pw = wTmpLow;
*(pw+1) = wTmpHigh;

}
#endif


/*---------------------------------------------------------------------------*/
/* getSndBFile() -                                             [scf]         */
/*---------------------------------------------------------------------------*/

BOOL getSndBFile(INT  actionString)
{
   BOOL     getSndBFile;
   FSReply  reply;
   INT      refNo;
   DOSTIME  fileDate;
   LONG     secs;
   LONG     tmp;
   WORD     wMode;
   BYTE     fileExt[FILENAMELEN+1];
   OFSTRUCT file;

   getSndBFile = FALSE;
   *reply.vRefNum = *reply.fName = 0;

   wMode = FO_GETFILE | FO_FILEEXIST;     /* rjs - added fileexist */
   if(xferBinType == XFRYMODEM)           /* mbbx 1.01 */
      wMode |= FO_BATCHMODE;
   else if(xferBinType == XFRKERMIT)
   /* jtf 3.17   wMode |= FO_REMOTEFILE */ ;
   getFileDocData(FILE_NDX_DATA, reply.vRefNum, reply.fName, fileExt, NULL);
   *macFileName = 0;

   if(reply.vRefNum[strlen(reply.vRefNum) - 1] != '\\')
      strcat(reply.vRefNum, "\\");

   if(reply.good = FileOpen(reply.vRefNum, reply.fName, macFileName, fileExt, 
                            NULL, FO_DBSNDFILE, NULL, wMode))
   {
      useMacFileName = (*macFileName != 0);
   }

   if(reply.good)
   {
      setFileDocData(FILE_NDX_DATA, reply.vRefNum, reply.fName, NULL, NULL);  /* mbbx 2.00: no forced extents */

      strcpy(xferVRefNum+1, reply.vRefNum);  /* mbbx 0.62: save the path !!! */
      *xferVRefNum = strlen(xferVRefNum+1);

      strcpy(xferFname+1, useMacFileName ? macFileName : reply.fName);
      *xferFname = strlen(xferFname+1);

      xferRefNo = 0;
      if(xferBinType != XFRYMODEM)           /* mbbx 1.01: ymodem */
      {
         if((xferBinType == XFRYTERM) && !answerMode)          /* mbbx: yterm */
            return(FALSE);
#ifdef ORGCODE
         strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
#else

         strcpy(reply.vRefNum+strlen(reply.vRefNum), "\\");
         strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);

         DEBOUT("getSndBFile: opening the file[%s]\n",reply.vRefNum);
         DEBOUT("getSndBFile: with flags      [%lx]\n",O_RDONLY);
#endif

         if((xferRefNo = OpenFile((LPSTR) reply.vRefNum, (LPOFSTRUCT)&file,
                                   OF_READ)) == -1)
         {
            sndBFileErr(STRFERROPEN, reply.fName);
            xferRefNo = 0;
            return FALSE;
         }
         if((xferBytes = fileLength(xferRefNo)) == -1L)
         {
            sndBFileErr(STRFERROPEN, reply.fName);
            if(xferBinType == XFRYTERM)         /* mbbx: yterm */
               answerMode = FALSE;
            _lclose(xferRefNo);
            xferRefNo = 0;
            return FALSE;
         }
         xferParams.ioFlLgLen = xferBytes; 
         swapWords (&xferParams.ioFlLgLen);       /* intel tch tch */
         xferParams.ioFlRLgLen = 0l;
         xferLgLen = xferBytes;

         getFileDate (&fileDate, xferRefNo);

         secs = 0;

         xferParams.ioFlCrDat  =  
         xferParams.ioFlMdDat  = secs;
         xferParams.fdFlags    = 0;

      }
      else
      {
         xferBytes = 1L;
      }

      xferErrors  = 0;
      xferLength  = 0L;
      xferOrig    = xferBytes;
      xferPct     = 0;

      sndBPre(reply.fName, actionString);
      getSndBFile = TRUE;
   }

   return(getSndBFile);
}


/*---------------------------------------------------------------------------*/
/* sndBFile() - Send a binary file.                              [scf]       */
/*---------------------------------------------------------------------------*/

VOID sndBFile()
{
   if(xferFlag != XFRNONE)                   /* mbbx 1.10: answerMode... */
   {
      return;
   }

   xferFast  = FALSE;

   if(!answerMode)
   {
      switch(trmParams.xBinType)
      {
      case ITMXMODEM:
         xferBinType = XFRXMODEM;
         break;

      case ITMKERMIT:
         xferBinType = XFRKERMIT;
         break;
      }
      xferBinFork = XFRDATA;
   }

   if(getSndBFile(STR_SENDING))
      xferFlag = XFRBSND;
}


/*---------------------------------------------------------------------------*/
/* sndAbort() -                                                  [scf]       */
/*---------------------------------------------------------------------------*/

VOID sndAbort  ()
{
   BYTE tmp1[TMPNSTR+1];

      LoadString(hInst, STR_ABORTSND, (LPSTR) tmp1, TMPNSTR);
      testMsg(tmp1, &xferFname[1],NULL);
}


/*---------------------------------------------------------------------------*/
/* xSndBFile() -                                                             */
/*---------------------------------------------------------------------------*/

VOID xSndBFile()
{
   BYTE  saveDataBits, saveParity, saveFlowCtrl;

   termSendCmd(trmParams.binTXPrefix, strlen(trmParams.binTXPrefix), 0x0042 | TRUE);   /* mbbx 2.01.19 ... */

   saveDataBits = trmParams.dataBits;        /* mbbx 2.00: auto adjust settings... */
   saveParity   = trmParams.parity;
   saveFlowCtrl = trmParams.flowControl;     /* mbbx 2.00.05: eliminate flowSerial()... */

   if((xferBinType != XFRKERMIT) && (xferBinType != XFRYTERM))
   {
      trmParams.dataBits = ITMDATA8;
      trmParams.parity = ITMNOPARITY;
      trmParams.flowControl = ITMNOFLOW;
      resetSerial(&trmParams, FALSE, FALSE, 0); /* slc swat */
   }

   switch(xferBinType)
   {
   case XFRXMODEM:
      XM_SndFile(0x0800);                    /* XM_CRC */
      break;

   case XFRKERMIT:
      KER_Send();                            /* rkhx 2.00 */
      break;

   }

   if((trmParams.dataBits != saveDataBits) || (trmParams.parity != saveParity) || (trmParams.flowControl != saveFlowCtrl))
   {
      trmParams.dataBits = saveDataBits;
      trmParams.parity = saveParity;
      trmParams.flowControl = saveFlowCtrl;
      resetSerial(&trmParams, FALSE, FALSE, 0); /* slc swat */
   }

   termSendCmd(trmParams.binTXSuffix, strlen(trmParams.binTXSuffix), 0x0042 | TRUE);   /* mbbx 2.01.19 ... */
}
