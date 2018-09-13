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
/* sndPre() -                                                                */
/*---------------------------------------------------------------------------*/

VOID sndPre(BYTE  *fileName, INT   actionString)
{
   showXferCtrls(IDSTOP | IDPAUSE | IDSCALE | IDSENDING);   /* mbbx 2.00: xfer ctrls... */
   showRXFname(fileName, actionString);
}


/*---------------------------------------------------------------------------*/
/* sndFileErr() -                                                            */
/*---------------------------------------------------------------------------*/

VOID sndFileErr(BYTE  *fileName, INT   wID)
{
   BYTE tmp1[TMPNSTR+1];

      LoadString(hInst, wID, (LPSTR) tmp1, TMPNSTR);
      testMsg("%s %s", tmp1, fileName);
}


/*---------------------------------------------------------------------------*/
/* getSndTFile() -                                                           */
/*---------------------------------------------------------------------------*/

VOID  APIENTRY FO_SendTextFile(HWND hDlg,WORD  message,WPARAM  wParam,LONG lParam)    /* mbbx 2.00: new FO hook scheme... */
{
   switch(message)
   {
   case WM_INITDIALOG:
      if(xferSndLF != 0)
         CheckDlgButton(hDlg, (xferSndLF > 0) ? FO_IDSNDLF : FO_IDSNDNOLF, TRUE);
      break;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case FO_IDSNDLF:
         if(xferSndLF == -1)
            CheckDlgButton(hDlg, FO_IDSNDNOLF, FALSE);
         xferSndLF = ((xferSndLF == 1) ? 0 : 1);
         CheckDlgButton(hDlg, FO_IDSNDLF, xferSndLF);
         break;
      case FO_IDSNDNOLF:
         if(xferSndLF == 1)
            CheckDlgButton(hDlg, FO_IDSNDLF, FALSE);
         xferSndLF = ((xferSndLF == -1) ? 0 : -1);
         CheckDlgButton(hDlg, FO_IDSNDNOLF, xferSndLF);
         break;
      }
      break;
   }
}


BOOL getSndTFile(INT   actionString)
{
   BYTE        fileExt[FILENAMELEN+1];
   FSReply     reply;
   BYTE        szTitle[MINRESSTR];
   OFSTRUCT    file;

   DEBOUT("getSndTFile: %s\n","Got IN");
   getFileDocData(FILE_NDX_MEMO, reply.vRefNum, reply.fName, fileExt, NULL);  /* mbbx 1.03 */


   if(actionString == STR_VIEWING)
   {
      xferSndLF = 0;                      /* mbbx: WAS xferSndLF = TRUE; */
      LoadString(hInst, STR_VIEWTEXTFILE, (LPSTR) szTitle, MINRESSTR);
   }
   else
   {
      xferSndLF = -1; /* jtf 3.22 */
      LoadString(hInst, STR_SENDTEXTFILE, (LPSTR) szTitle, MINRESSTR);
   }

                                          /* mbbx 2.00: new FO hook scheme... */
   DEBOUT("getSndTFile: %s\n","Calling FileOpen");
   if(reply.vRefNum[strlen(reply.vRefNum) - 1] != '\\')
      strcat(reply.vRefNum, "\\");

   reply.good = FileOpen(reply.vRefNum, reply.fName, NULL, fileExt, szTitle,
                         FO_DBSNDTEXT, (FARPROC)FO_SendTextFile, FO_FILEEXIST);

   if(reply.good)
   {
      DEBOUT("getSndTFile: %s\n","FileOpen reply was good");
      setFileDocData(FILE_NDX_MEMO, reply.vRefNum, reply.fName, fileExt, NULL);  /* mbbx 2.00: no forced extents */

      strcpy(xferVRefNum+1, reply.vRefNum);  /* mbbx 0.62: save the path !!! */
      *xferVRefNum = strlen(xferVRefNum+1);

      strcpy(xferFname+1, reply.fName);
      *xferFname = strlen(xferFname+1);
#ifdef ORGCODE
      strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
#else
      strcpy(reply.vRefNum+strlen(reply.vRefNum), "\\");
      strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
      DEBOUT("getSndTFile: opening the file[%s]\n",reply.vRefNum);
      DEBOUT("getSndTFile: with flags      [%lx]\n",O_RDONLY);

#endif
      /* jtf 3.20 */

      if((xferRefNo = OpenFile((LPSTR) reply.vRefNum, (LPOFSTRUCT)&file,
                               OF_READ)) == -1)
      {
	 //sndFileErr (STRFERROPEN, reply.fName); BUG:12588 args are swaped!
	 sndFileErr (reply.fName,STRFERROPEN);
         DEBOUT("getSndTFile: %s\n","Got OUT with FAIL(open)");
         return FALSE;
      }

      if ((xferBytes = fileLength (xferRefNo)) == -1l)
      {
	 //sndFileErr (STRFERRFILELENGTH, reply.fName); BUG:12588 args are swapped!
	 sndFileErr (reply.fName,STRFERRFILELENGTH);
         DEBOUT("getSndTFile: %s\n","Got OUT with FAIL(filelength)");
         return FALSE;
      }

      DEBOUT("getSndTFile: size of file put in xferBytes=%lx\n",xferBytes);
      xferOrig = xferBytes;
      xferPct = 0;
      sndPre(reply.fName, actionString);
      DEBOUT("getSndTFile: %s\n","Got OUT with success");
      return TRUE;
   }

   DEBOUT("getSndTFile: %s\n","Got OUT with FAIL(!reply.good)");
   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* sndTfile() -                                                              */
/*---------------------------------------------------------------------------*/

VOID sndTFile ()
{
   xferFlag     = XFRNONE;
   xferPaused   = FALSE;
   xferTxtType  = (TTXTTYPE) (trmParams.xTxtType - ITMSTD);
   xferChrType  = (TCHRTYPE) (trmParams.xChrType - ITMCHRDELAY);
   xferLinType  = (TLINTYPE) (trmParams.xLinType - ITMLINDELAY);
   xferChrDelay = trmParams.xChrDelay;
   xferLinDelay = trmParams.xLinDelay;
   strcpy(xferLinStr+1, trmParams.xLinStr);
   xferLinStr[0] = strlen(trmParams.xLinStr);
   stripControl(xferLinStr);

   if(getSndTFile(STR_SENDING))
   {
      xferFlag  = XFRSND;
      *outBuf   = 0;                         /* (mbbx) clear out residuals */
      if(trmParams.xWordWrap)
      {
         outBufCol = 0;
         outBufSeq = FALSE;
         xferBlkSize = 1;                    /* (xmbb) */
      }
   }
}


/*---------------------------------------------------------------------------*/
/* typTFile() -                                                              */
/*---------------------------------------------------------------------------*/

VOID typTFile()
{
   xferFlag = XFRNONE;
   xferPaused = FALSE;
   xferStopped = FALSE;

   if(getSndTFile(STR_VIEWING))
   {
      xferFlag = XFRTYP;
      *outBuf  = 0;                          /* (mbbx) */
      if(trmParams.xWordWrap && (xferSndLF >= 0))     /* mbb?: wth??? */
      {
         outBufCol = 0;
         outBufSeq = FALSE;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* rcvErr() -                                                                */
/*---------------------------------------------------------------------------*/

VOID rcvErr(BYTE  *fileName)
{
   strcpy(taskState.string+1, fileName);
   *taskState.string = strlen(fileName);
   TF_ErrProc(STREWRERR, MB_OK | MB_ICONHAND, 999);

}


/*---------------------------------------------------------------------------*/
/* rcvPutFile() -                                                            */
/*---------------------------------------------------------------------------*/

VOID  APIENTRY FO_RcvTextFile(HWND hDlg,WORD  message,WPARAM wParam,LONG lParam)  /* mbbx 2.00: new FO hook scheme... */
//HWND  hDlg;
//WORD  message;
//WPARAM wParam;
//LONG  lParam;
{
   switch(message)
   {
   case WM_INITDIALOG:
      xferAppend = FALSE;
      CheckDlgButton(hDlg, FO_IDCTRL, xferSaveCtlChr);
      CheckDlgButton(hDlg, FO_IDTABLE, xferTableSave);
      break;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case FO_IDAPPEND:
         if(xferAppend = !xferAppend)
            pFOData->wMode |= FO_FILEEXIST;
         else
            pFOData->wMode &= ~FO_FILEEXIST;
         CheckDlgButton(hDlg, FO_IDAPPEND, xferAppend);
         break;
      case FO_IDCTRL:
         CheckDlgButton(hDlg, FO_IDCTRL, xferSaveCtlChr = !xferSaveCtlChr);
         break;
      case FO_IDTABLE:
         CheckDlgButton(hDlg, FO_IDTABLE, xferTableSave = !xferTableSave);
         break;
      }
      break;
   }
}


VOID rcvPutFile(FSReply *reply, BYTE *fileExt)
{
   *reply->fName = 0;                        /* mbbx 2.00: CUA */

                                             /* mbbx 2.00: new FO hook scheme... */
   if(reply->vRefNum[strlen(reply->vRefNum) - 1] != '\\')
      strcat(reply->vRefNum, "\\");

   reply->good = FileOpen(reply->vRefNum, reply->fName, NULL, fileExt, NULL, 
                          FO_DBRCVTEXT, (FARPROC)FO_RcvTextFile, FO_PUTFILE);
}


/*---------------------------------------------------------------------------*/
/* rcvPre() - Show transfer control & set up for receiving text.[scf]        */
/*---------------------------------------------------------------------------*/

VOID rcvPre(BYTE *fileName, INT  actionString)
{
   xferFlag = XFRRCV;
   xferBytes = -1;                           /* Special flag to enable stop  */
   xferPaused = FALSE;                       /* button                       */
   xferLength = 0L;                          /* mbbx 2.00: mac */
   tblPos = TBLBEGINLINE;

   showXferCtrls(IDSTOP | IDPAUSE | IDSENDING);    /* mbbx 2.00: xfer ctrls... */
   showRXFname(fileName, actionString);

}


/*---------------------------------------------------------------------------*/
/* rcvTFile() -                                                              */
/*---------------------------------------------------------------------------*/

VOID rcvTFile()
{
   FSReply     reply;
   BYTE        fileExt[FILENAMELEN+1];
   OFSTRUCT    file;
   BYTE     OEMname[STR255];            /* jtf 3.20 */

   getFileDocData(FILE_NDX_MEMO, reply.vRefNum, NULL, fileExt, NULL);  /* mbbx 1.03 */

   xferSaveCtlChr = FALSE;
   xferTableSave  = FALSE;
   rcvPutFile(&reply, fileExt);

   if(reply.good)
   {
      setFileDocData(FILE_NDX_MEMO, reply.vRefNum, reply.fName, fileExt, NULL);  /* mbbx 2.00: no forced extents */

      strcpy(xferVRefNum+1, reply.vRefNum);  /* mbbx 0.62: save the path !!! */
      *xferVRefNum = strlen(xferVRefNum+1);

      strcpy(xferFname+1, reply.fName);
      *xferFname = strlen(xferFname+1);
#ifdef ORGCODE
      strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
#else
      strcpy(reply.vRefNum+strlen(reply.vRefNum), "\\");
      strcpy(reply.vRefNum+strlen(reply.vRefNum), reply.fName);
#endif

      // JYF -- replace below two lines with the following if ()
      //        to remove the use of AnsiToOem()
      //
      //AnsiToOem((LPSTR) reply.vRefNum, (LPSTR) OEMname); /* jtf 3.20 */
      //if(xferAppend && fileExist(OEMname))

      if (xferAppend && fileExist((LPSTR)reply.vRefNum))
      {
         /* jtf 3.20 */
         DEBOUT("rcvTFile: doing open(%s)\n",reply.vRefNum);
         DEBOUT("rcvTFile: with flag [%lx]\n",O_WRONLY);

         if((xferRefNo = OpenFile((LPSTR) reply.vRefNum, 
                                  (LPOFSTRUCT)&file, 
                                  OF_WRITE | OF_CANCEL)) == -1)
         {
            rcvErr(reply.fName);
            return;
         }
         _lseek(xferRefNo, 0L, 2);
      }
      else
      {
         /* jtf 3.20 */
         DEBOUT("rcvTFile: doing open(%s)\n",reply.vRefNum);
         DEBOUT("rcvTFile: with flag [%lx]\n",O_WRONLY|O_CREAT|O_TRUNC|S_IWRITE);

         if((xferRefNo = OpenFile((LPSTR) reply.vRefNum, 
                                  (LPOFSTRUCT)&file, 
                                  OF_WRITE | OF_CREATE)) == -1)
         {
            rcvErr(reply.fName);
            return;
         }
      }

      xferBufferCount = 0;                                           /* rjs bugs 016 */
      xferBufferHandle = GlobalAlloc(GMEM_MOVEABLE, (DWORD) 1024);   /* rjs bugs 016 */
      rcvPre(reply.fName, STR_RECEIVING);
   }
}
