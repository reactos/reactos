/*===========================================================================*/
/*                                                                           */
/*  Windows DynaComm version 1.x -- Dynamic Communications for Windows 2.x   */
/*                                                                           */
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*---------------------------------------------------------------------------*/
/* This material is an unpublished work containing trade secrets which are   */
/* the property of Future Soft Engineering, Inc., and is subject to a        */
/* license therefrom.  It may not be disclosed, reproduced, adapted, merged, */
/* translated, or used in any manner without prior written consent from FSE. */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "dcdata.h"
#include "video.h"
#include "printfil.h"


/*---------------------------------------------------------------------------*/
/* mainProcess() - main process loop; called from _INITCODE:WINMAIN    [mbb] */
/*---------------------------------------------------------------------------*/

VOID FAR mainProcess()
{
   DWORD dwThreadID;
   
   doneFlag         = 
   gbThreadDoneFlag = FALSE;
   CommThreadExit   = FALSE;
   dwTimerRes       = 100;

   commThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)checkCommEvent,
					 (LONG)dwTimerRes, CREATE_SUSPENDED | STANDARD_RIGHTS_REQUIRED,
					 (LPDWORD) &dwThreadID);

   if (commThread) 
   {
      SetThreadPriority(commThread, THREAD_PRIORITY_BELOW_NORMAL);
      ResumeThread(commThread);
   }
   
   overlapEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   hMutex       = CreateMutex(NULL, FALSE, NULL);
   
   repeat
   {
      mainEventLoop();
   }
   until(doneFlag);

   // terminate threads here, not just close handles!!!!
   
//   TerminateThread(hwndThread, (DWORD)0);
//   CloseHandle(hwndThread);

//   while(WaitForSingleObject(hMutex, 1000) != 0)
//   {
//	Sleep((DWORD)50);
//	MessageBeep(0);
//   }

   if (WaitForSingleObject(hMutex, 2000) != 0)
      {
      // the comevent thread did not comeout in 2 sec, something wrong
      // Just kill it and indicate by Beep (for the timebeing) that
      // we have this wierd condition
      MessageBeep(0);
      TerminateThread(commThread, (DWORD)0);
      }

   else
      {
       // just in case if that thread as not done ExitThread(), kill it
       // maybe this is a overkill!
       TerminateThread(commThread, (DWORD)0);
       ReleaseMutex(hMutex);
      }


   CloseHandle(commThread);
   CloseHandle(hMutex);
}


/*---------------------------------------------------------------------------*/
/* mainEventLoop() - Main input from the application queue             [mbb] */
/*---------------------------------------------------------------------------*/

VOID FAR mainEventLoop()
{
   idleProcess();

   if(PeekMessage((LPMSG) &msg, NULL, 0, 0, PM_REMOVE))
   {
      if(!IsDialogMessage(hdbmyControls, (LPMSG) &msg) && 
         !IsDialogMessage(hdbXferCtrls, (LPMSG) &msg))
      {
         DispatchMessage((LPMSG) &msg);
      }
   }
}





void    xSndThread (DWORD parentId)
{
    AttachThreadInput (GetCurrentThreadId(), parentId, TRUE);
    xSndBFile();
    xferEnd();
    MessageBeep(10);
    gbXferActive = FALSE;
}


void    xRcvThread (DWORD parentId)
{
    AttachThreadInput (GetCurrentThreadId(), parentId, TRUE);
    xRcvBFile();
    xferEnd();
    MessageBeep(10);
    gbXferActive = FALSE;
}




/*---------------------------------------------------------------------------*/
/* idleProcess() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

DWORD APIENTRY idleProcess(VOID)
{
   // sdj: was unref local - LONG	secs;
   // sdj: was unref local - LONG	finalTicks;
   // sdj: was unref local - HWND	hwnd;

static HANDLE  hThread;
static DWORD   dwId;


      updateTimer();

      if(!gbXferActive)
      {
         if (xferFlag == XFRBSND) 
         {
            gbXferActive = TRUE;
            hThread = CreateThread (NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)xSndThread,
				    (LPVOID)GetCurrentThreadId(),
                                    TRUE,
                                    &dwId);
         }
         else if(xferFlag == XFRBRCV)
         {
            gbXferActive = TRUE;
            hThread = CreateThread (NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)xRcvThread,
				    (LPVOID)GetCurrentThreadId(),
                                    TRUE,
                                    &dwId);
         }
         else
         {
            if (activTerm)
                termSpecial ();

            if(mdmConnect())
            {
               modemReset();
               timerAction(mdmOnLine, mdmOnLine);  /* mbbx 1.03 */
            }

            cursorAdjust();
            if(activTerm)
	       blinkCursor();

         }
      }

      Sleep((DWORD)10);

}


/*---------------------------------------------------------------------------*/
/* updateTimer() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID FAR updateTimer()                       /* mbbx 2.00: fkeys... */
{

#ifdef WIN32
WPARAM	wParam;
#endif

/* rjs bug2 001 */
   if(GetCurrentTime() >= gIdleTimer)        
   {
      gIdleTimer = GetCurrentTime() + 900;

      if((hItWnd != NULL) && IsWindowVisible(hdbmyControls))
#ifdef ORGCODE
         updateFKeyButton(MAKELONG(GetDlgItem(hdbmyControls, IDTIMER), BN_PAINT), FKB_UPDATE_TIMER);
#else
// sdj: 10/8/91 for bug#3277 changed
//                 wParam = MAKELONG(BN_PAINT, NULL); /* set HIWORD to cmd, LOWORD not used*/
// to:
                   wParam = MAKELONG(NULL,BN_PAINT); /* set HIWORD to cmd, LOWORD not used*/

         updateFKeyButton(wParam,(LONG)GetDlgItem(hdbmyControls, IDTIMER), FKB_UPDATE_TIMER);
#endif
   }
}

/*---------------------------------------------------------------------------*/
/* updateFKeyButton() -                                                [mbb] */
/*---------------------------------------------------------------------------*/


/* *****NOTE NOTE ******  this function will not work under win16, as */
/* I removed the textExtent variable and replaced with iHeigth and iWidth */
/* and didn't ifdef ORGCODE around the old code  (JAP)*/

BOOL updateFKeyButton(WPARAM wParam ,LONG lParam,WORD  status)        /* mbbx 2.01.163 ... */
{
   BOOL     updateFKeyButton = FALSE;
   HDC      hDC;
   RECT     timerRect, saveRect;
   DOSTIME  time;
   LONG     delta;
   BYTE     text[80];
   HBRUSH   hBrush, hOldBrush;
#ifdef ORGCODE
   DWORD    textExtent;
#else
int			iHeight;
int			iWidth;
#endif
   LONG     secs;       /* rjs bugs 004 */



   if(IsWindowVisible((HWND)GET_WM_COMMAND_HWND(wParam, lParam)))
   {
      hDC = GetWindowDC((HWND) GET_WM_COMMAND_HWND(wParam, lParam));

      GetClientRect(GET_WM_COMMAND_HWND(wParam,lParam), (LPRECT)&timerRect);
      CopyRect( &saveRect, &timerRect);
      InflateRect(&timerRect, -1, -1);

      switch(GET_WM_COMMAND_CMD(wParam, lParam))
      {
      case BN_CLICKED:
         if(updateFKeyButton = !(status & FKB_DISABLE_CTRL))
         {
            InvertRect(hDC, (LPRECT) &timerRect);
            delay(6, NULL);
            InvertRect(hDC, (LPRECT) &timerRect);
         }
         else
            sysBeep();
         break;

      case BN_PAINT:
         if(status & FKB_UPDATE_TIMER)
         {
            readDateTime(&time);

            if(timerActiv)
            {
               date2secs(&time, &delta);    

               date2secs(startTimer, &secs);

               delta -= secs;               

               if(delta < 0)                    /* rjs - msoft 4061 */
               {
                  delta = 0;
                  readDateTime(&startTimer);
               }

               time.hour = delta / 3600;
               delta -= (LONG) time.hour * 3600;
               time.minute = delta / 60;
               time.second = delta - time.minute * 60;
               sprintf(text, "%2.2d:%2.2d:%2.2d", time.hour, time.minute, time.second);
            }
            else
               getTimeString(text, &time);

            if(time.hour != lastTime.hour)
               status |= FKB_UPDATE_BKGD;
         }
         else
            SendMessage((HWND)GET_WM_COMMAND_HWND(wParam, lParam), WM_GETTEXT, 80, (LONG) ((LPSTR) text));

         if(status & FKB_UPDATE_BKGD)
         {
            hBrush = CreateSolidBrush(RGB(vidAttr[ANORMAL & AMASK].bkgd[VID_RED], 
                                          vidAttr[ANORMAL & AMASK].bkgd[VID_GREEN], 
                                          vidAttr[ANORMAL & AMASK].bkgd[VID_BLUE]));
            hOldBrush = (HBRUSH) SelectObject(hDC, hBrush);

            RoundRect(hDC, timerRect.left, timerRect.top, 
                      timerRect.right, timerRect.bottom, 10, 10);

            SelectObject(hDC, hOldBrush);
            DeleteObject(hBrush);
         }

         if((status & FKB_UPDATE_BKGD) || (time.second != lastTime.second))
         {
	         InflateRect(&timerRect, -4, -1);
            MGetTextExtent(hDC, (LPSTR) text, strlen(text), &iHeight, &iWidth);
            SetBkColor(hDC, RGB(vidAttr[ANORMAL & AMASK].bkgd[VID_RED], 
                                vidAttr[ANORMAL & AMASK].bkgd[VID_GREEN], 
                                vidAttr[ANORMAL & AMASK].bkgd[VID_BLUE]));

            if(!(status & FKB_DISABLE_CTRL))
            {
               SetTextColor(hDC, RGB(vidAttr[ANORMAL & AMASK].text[VID_RED], 
                                     vidAttr[ANORMAL & AMASK].text[VID_GREEN], 
                                     vidAttr[ANORMAL & AMASK].text[VID_BLUE]));
            }
            else
            {
               SetTextColor(hDC, (COLORREF)GetSysColor(COLOR_GRAYTEXT));
            }

            DrawText(hDC, (LPSTR) text, strlen(text), (LPRECT) &timerRect, 
                     (iWidth < (timerRect.right - timerRect.left)) ? 
                     DT_CENTER | DT_SINGLELINE | DT_VCENTER : DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            if(status & FKB_UPDATE_TIMER)
               lastTime = time;
         }
         break;
      }
      ReleaseDC((HWND)GET_WM_COMMAND_HWND(wParam, lParam), hDC);
   }

   return(updateFKeyButton);
}


/*---------------------------------------------------------------------------*/
/* getTimeString() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID getTimeString(BYTE     *str, DOSTIME  *time)
{
   BYTE  work[16];
   BYTE     str1[MINRESSTR], str2[MINRESSTR];

   work[0] = 0;


   //sdj: the bug is about terminal not updating the timeformat
   //sdj: when control panel changes, it only updates while coming
   //sdj: up, when it inits the intlData elements. To fix this
   //sdj: update the iTime structure element before generating the
   //sdj: string, so that it reflects the current state saved by
   //sdj: the control panel applet. It is expensive to do this
   //sdj: because each time the paint msg comes, getprofile is called
   //sdj: but the other way is too complecated where you can wait
   //sdj: for the change in this value in the registry and signal
   //sdj: the change and call profile apis only when this flag is set
   //sdj: saving grace is that, this performance hit is only present
   //sdj: when bn_paint is received AND the timer mode is not on!

   LoadString(hInst, STR_INI_INTL, (LPSTR) str1, MINRESSTR);
   LoadString(hInst, STR_INI_IDATE, (LPSTR) str2, MINRESSTR);
   intlData.iDate = GetProfileInt((LPSTR) str1, (LPSTR) str2, 0);
   LoadString(hInst, STR_INI_SDATE, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str1, (LPSTR) str2, (LPSTR) "/", (LPSTR) intlData.sDate, 2);
   LoadString(hInst, STR_INI_ITIME, (LPSTR) str2, MINRESSTR);
   intlData.iTime = GetProfileInt((LPSTR) str1, (LPSTR) str2, 0);
   LoadString(hInst, STR_INI_STIME, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str1, (LPSTR) str2, (LPSTR) ":", (LPSTR) intlData.sTime, 2);
   LoadString(hInst, STR_INI_S1159, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str1, (LPSTR) str2, (LPSTR) "AM", (LPSTR) intlData.s1159, 4);
   LoadString(hInst, STR_INI_S2359, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str1, (LPSTR) str2, (LPSTR) "PM", (LPSTR) intlData.s2359, 4);



   if(!intlData.iTime)                       /* 12 Hour Clock */
   {
      if(time->hour < 12)
      {
         if(intlData.s1159[0] != 0)
            sprintf(work, " %s", intlData.s1159);
      }
      else
      {
         time->hour -= 12;

         if(intlData.s2359[0] != 0)
            sprintf(work, " %s", intlData.s2359);
      }

      if(time->hour == 0)
         time->hour = 12;
   }

   sprintf(str, "%02d%s%02d%s%02d%s", time->hour, intlData.sTime, time->minute, intlData.sTime, time->second, work);
}


/*---------------------------------------------------------------------------*/
/* cursorAdjust() - Catch terminal paint up with received characters.  [scf] */
/*---------------------------------------------------------------------------*/

VOID FAR cursorAdjust ()
{
   if(termDirty || (nScroll != 0))           /* mbbx: termLine -> termDirty */
      termCleanUp();
}


/*---------------------------------------------------------------------------*/
/* blinkCursor() -                                                      [scf]*/
/*---------------------------------------------------------------------------*/

VOID NEAR blinkCursor()
{
   RECT  rect;

   if(activTerm && (cursorOn > 0) && (GetCurrentTime() >= cursorTick))  /* mbbx 1.10: use msec... */
   {
      rectCursor(&rect);
      if(cursBlinkOn && memcmp(&cursorRect, &rect, sizeof(RECT)))
         toggleCursor(&cursorRect);

      cursorRect = rect;
      if(!cursBlinkOn || trmParams.cursorBlink)    /* mbbx 1.10: CUA */
         toggleCursor(&cursorRect);
      cursorTick = GetCurrentTime() + GetCaretBlinkTime();  /* mbbx 1.10: lilly... */
   }
}


/*---------------------------------------------------------------------------*/
/* checkInputBuffer() - support keystroke buffering (type-ahead)       [mbb] */
/*---------------------------------------------------------------------------*/
#ifdef OLDCODE
#define MAX_INPUT_BUFFER         64          /* allows 32 keystrokes */

WORD  inputBufferItems = 0;
MSG   inputBuffer[MAX_INPUT_BUFFER];

BOOL NEAR checkInputBuffer(MSG   *msg)
{
   WORD  ndx;

   if((kbdLock == KBD_BUFFER) || ((kbdLock == KBD_WAIT) ) || 
      ((inputBufferItems > 0) )) 
   {
      while(PeekMessage((LPMSG) msg, NULL, WM_KEYFIRST, WM_KEYLAST, TRUE))
      {
         if((msg->message == WM_KEYUP) && (inputBufferItems == 0))
            return(TRUE);
         else if(((msg->message == WM_KEYDOWN) || (msg->message == WM_KEYUP)) && 
                 (((msg->wParam >= '0') && (msg->wParam <= 'Z')) || (msg->wParam >= 0x80)))
         {
                                             /* mbbx 2.00: new xlate scheme... */
            if(!TranslateMessage((LPMSG) msg) || !PeekMessage((LPMSG) msg, NULL, WM_CHAR, WM_CHAR, TRUE))
               continue;
         }

         if(inputBufferItems < MAX_INPUT_BUFFER)
            inputBuffer[inputBufferItems++] = *msg;
         else
            sysBeep();
      }
   }
   else if(PeekMessage((LPMSG) msg, NULL, WM_CHAR, WM_CHAR, TRUE))
   {
      return(TRUE);
   }
   else if(inputBufferItems > 0)
   {
      *msg = inputBuffer[0];
      msg->hwnd = GetFocus();

      inputBufferItems -= 1;
      for(ndx = 0; ndx < inputBufferItems; ndx++)
         inputBuffer[ndx] = inputBuffer[ndx+1];

      return(TRUE);
   }

   return(FALSE);
}

#endif
