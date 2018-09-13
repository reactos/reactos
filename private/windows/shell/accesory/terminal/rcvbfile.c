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


/*---------------------------------------------------------------------------*/
/* rcvBPre() -                                                         [mbb] */
/*---------------------------------------------------------------------------*/

VOID rcvBPre(BYTE *fileName)
{
   WORD  fScale = ((xferBinType != XFRYMODEM) && (xferBinFork == XFRBOTH)) ? IDSCALE : 0;

   showXferCtrls(IDSTOP | IDFORK | fScale | IDSENDING | IDBERRORS);
   showRXFname(fileName, STR_RECEIVING);
}


/*---------------------------------------------------------------------------*/
/* rcvPutBFile() -                                              [scf]        */
/*---------------------------------------------------------------------------*/

BOOL rcvPutBFile()
{
   BOOL     rcvPutBFile = FALSE;
   FSReply  reply;
   WORD     wMode;
   BYTE     fileExt[FILENAMELEN+1];
   OFSTRUCT file;


      wMode = FO_PUTFILE; /* jtf 3.31 | ((xferBinType == XFRYMODEM) ? FO_BATCHMODE : FO_NONDOSFILE); */
      if(xferBinType == XFRKERMIT)
     /* jtf 3.17    wMode |= FO_REMOTEFILE */ ;
      getFileDocData(FILE_NDX_DATA, reply.vRefNum, reply.fName, fileExt, NULL);
      *macFileName = 0;
                                             /* mbbx 1.10: CUA... */
      if(reply.vRefNum[strlen(reply.vRefNum) - 1] != '\\')
         strcat(reply.vRefNum, "\\");

      if(reply.good = FileOpen(reply.vRefNum, reply.fName, macFileName, fileExt, 
                               NULL, FO_DBRCVFILE, NULL, wMode))
      {
         useMacFileName = (*macFileName != 0);
      }

   if(reply.good)
   {
      setFileDocData(FILE_NDX_DATA, reply.vRefNum, reply.fName, NULL, NULL);  /* mbbx 2.00: no forced extents */

      strcpy(xferVRefNum+1, reply.vRefNum);  /* mbbx 0.62: save the path !!! */
      *xferVRefNum = strlen(xferVRefNum+1);

      xferRefNo = 0;
      if(xferBinType != XFRYMODEM)           /* mbbx 1.01: ymodem */
      {
         strcpy(xferFname+1, useMacFileName ? macFileName : reply.fName);
         *xferFname = strlen(xferFname+1);

         if((xferBinType == XFRYTERM) && !answerMode)       /* mbbx: yterm */
            return(FALSE);
#ifdef ORGCODE
         strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
#else

         strcpy(reply.vRefNum+strlen(reply.vRefNum), "\\");
         strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
         DEBOUT("rcvPutBfile: opening %s\n",reply.vRefNum);
         DEBOUT("rcvPutBfile: %s","using O_CREAT|O_TRUNC, S_IWRITE args\n");
#endif
         /* jtf 3.20 */
         if((xferRefNo = OpenFile((LPSTR) reply.vRefNum, (LPOFSTRUCT)&file,
                                  OF_WRITE | OF_CREATE)) == -1)
         {
            xferRefNo = 0;                   /* mbbx 2.00: remove XTalk... */
            rcvErr(reply.fName);
            return(FALSE);
         }
      }

      xferErrors  = 0;
      xferLength  = 0L;
      xferPct     = 0;
      xferOrig    = -1;

      rcvBPre(reply.fName);
      rcvPutBFile = TRUE;
   }

   return(rcvPutBFile);
}


/*---------------------------------------------------------------------------*/
/* rcvBFile() -                                                  [scf]       */
/*---------------------------------------------------------------------------*/

VOID rcvBFile()
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
         KERRCVFLAG = KERFILE;
         xferBinType = XFRKERMIT;
         break;
      }
   }

      xferBinFork = XFRDATA;


   if(rcvPutBFile())
   {
      xferFlag = XFRBRCV;
   }
}



/*---------------------------------------------------------------------------*/
/* rcvTerminate() -                                             [scf]        */
/*---------------------------------------------------------------------------*/

VOID rcvTerminate()
{
   BYTE        filePath[PATHLEN];
   OFSTRUCT    dummy;

   if(xferRefNo != 0)                        /* mbbx 1.01: ymodem */
   {
      _lclose(xferRefNo);
      strcpy(filePath, xferVRefNum+1);
      strcpy(filePath+strlen(filePath), xferFname+1);
      MOpenFile((LPSTR) filePath, (LPOFSTRUCT) &dummy, OF_DELETE);
   }
}


/*---------------------------------------------------------------------------*/
/* rcvAbort() -                                                  [scf]       */
/*---------------------------------------------------------------------------*/

VOID rcvAbort()
{
   BYTE tmp1[TMPNSTR+1];

      LoadString(hInst, STR_ABORTRCV, (LPSTR) tmp1, TMPNSTR);
      testBox(NULL, -(MB_ICONEXCLAMATION | MB_OK), STR_APPNAME, tmp1, &xferFname[1]);
   rcvTerminate();
}


/*---------------------------------------------------------------------------*/
/* rcvFileErr() -                                               [scf]        */
/*---------------------------------------------------------------------------*/

VOID rcvFileErr()
{
   memcpy(taskState.string, xferFname, *xferFname+2);
   TF_ErrProc(STREWRERR, MB_OK | MB_ICONHAND, 999);


   rcvTerminate();
}



/*---------------------------------------------------------------------------*/
/* xRcvBFile() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

VOID xRcvBFile()
{
   BYTE  saveDataBits, saveParity, saveFlowCtrl;

   termSendCmd(trmParams.binRXPrefix, strlen(trmParams.binRXPrefix), 0x42 | TRUE);   /* mbbx 2.01.19 ... */

   saveDataBits = trmParams.dataBits;        /* mbbx 2.00: auto adjust settings... */
   saveParity   = trmParams.parity;
   saveFlowCtrl = trmParams.flowControl;     /* mbbx 2.00.05: eliminate flowSerial()... */

   trmParams.dataBits = ITMDATA8;
   trmParams.parity = ITMNOPARITY;
   trmParams.flowControl = ITMNOFLOW;
   resetSerial(&trmParams, FALSE, FALSE, 0);  /* mbbx 2.00.05: auto reset... */

   switch(xferBinType)
   {
   case XFRXMODEM:
      XM_RcvFile(0x0800);                    /* XM_CRC */
      break;

   case XFRKERMIT:
      KER_Receive(FALSE);                    /* rkhx 2.00 */
      break;
   }

   if((trmParams.dataBits != saveDataBits) || (trmParams.parity != saveParity) || (trmParams.flowControl != saveFlowCtrl))
   {
      trmParams.dataBits = saveDataBits;
      trmParams.parity = saveParity;
      trmParams.flowControl = saveFlowCtrl;
      resetSerial(&trmParams, FALSE, FALSE, 0); /* slc swat */
   }

   termSendCmd(trmParams.binRXSuffix, strlen(trmParams.binRXSuffix), 0x42 | TRUE);   /* mbbx 2.01.19 ... */
}
