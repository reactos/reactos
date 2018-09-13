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
/* xferPauseResume() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

VOID xferPauseResume(BOOL  bPause, BOOL  bResume)
{
   setXferCtrlButton(IDPAUSE, !(xferPaused = bPause) ? STR_PAUSE : STR_RESUME);

   if(bResume)
   {
      if(xferViewPause > 0)

      xferEndTimer = 0;
      xferWaitEcho = FALSE;
   }
}


/*---------------------------------------------------------------------------*/
/* xferStopBreak() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID xferStopBreak(BOOL  bStop)
{
   if(bStop)
   {
      xferBytes = 0;
      xferEndTimer = 0;
      xferWaitEcho = FALSE;
      xferStopped = TRUE;
   }
   else
      PostMessage(hTermWnd, WM_KEYDOWN, VK_CANCEL, 0L);
}


/*---------------------------------------------------------------------------*/
/* xferEnd() - Termination processing for file transfers.              [scf] */
/*---------------------------------------------------------------------------*/

VOID xferEnd()
{
   BYTE     OEMname[STR255];            /* jtf 3.20 */
   LPSTR lpBuffer;                              /* rjs bugs 016 */

   if (xferFlag == XFRRCV)                         /* rjs bugs 016 */
   {
      lpBuffer = GlobalLock(xferBufferHandle);  /* rjs bugs 016 */
      if (xferBufferCount > 0)                  /* rjs bugs 016 */
         _lwrite(xferRefNo, (LPSTR)lpBuffer, xferBufferCount); /* rjs bugs 016 */
      GlobalUnlock(xferBufferHandle);           /* rjs bugs 016 */
      xferBufferCount = 0;                      /* rjs bugs 016 */
      GlobalFree(xferBufferHandle);             /* rjs bugs 016 */
   }

   if(xferViewPause > 0)
   {
      xferViewPause = 0;
      xferViewLine  = 0;
   }

   xferFlag = XFRNONE;
   xferWaitEcho = FALSE;
   xferStopped = FALSE;
   chrHeight = stdChrHeight;
   chrWidth = stdChrWidth;

   showXferCtrls(0);                      /* mbbx 2.00: xfer ctrls... */
   xferPauseResume(FALSE, FALSE);

   *strRXErrors =
   *strRXBytes =
   *strRXFname =
   *strRXFork = 0;

   if(xferRefNo)
      _lclose(xferRefNo);
   LoadString(hInst, STR_TEMPFILE, (LPSTR) xferVRefNum+(*xferVRefNum)+1, 16);

   // JYF -- replace below two lines with the following if()
   //        to remove the use of AnsiToOem()
   //
   //AnsiToOem((LPSTR) (xferVRefNum+1), (LPSTR) OEMname); /* jtf 3.20 */
   //if(fileExist(OEMname)) /* jtf 3.20 */

   if (fileExist((LPSTR)xferVRefNum+1))
      MDeleteFile(xferVRefNum+1);
   KER_getflag = FALSE;

   flashIcon(TRUE, TRUE);

}


/*---------------------------------------------------------------------------*/
/* rxEventLoop() -                                              [scf]        */
/*---------------------------------------------------------------------------*/

/* mbbx 1.04: moved from RXEVNTLP.C ... */

VOID rxEventLoop()
{
   MSG msg;

//   while(PeekMessage((LPMSG) &msg, NULL, 0, 0, PM_NOREMOVE))
//      mainEventLoop ();

   idleProcess();

#ifdef SLEEP_FOR_CONTEXT_SWTICH
   Sleep((DWORD)5);
#endif

}
