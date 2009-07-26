/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window timers messages
 * FILE:             subsystems/win32/win32k/ntuser/timer.c
 * PROGRAMER:        Gunnar
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY: 10/04/2003 Implemented System Timers
 *
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static PTIMER FirstpTmr = NULL;
static LONG TimeLast = 0;

#define MAX_ELAPSE_TIME 0x7FFFFFFF

/* Windows 2000 has room for 32768 window-less timers */
#define NUM_WINDOW_LESS_TIMERS   1024

static FAST_MUTEX     Mutex;
static RTL_BITMAP     WindowLessTimersBitMap;
static PVOID          WindowLessTimersBitMapBuffer;
static ULONG          HintIndex = 0;


#define IntLockWindowlessTimerBitmap() \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&Mutex)

#define IntUnlockWindowlessTimerBitmap() \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&Mutex)

/* FUNCTIONS *****************************************************************/
static
PTIMER
FASTCALL
CreateTimer(VOID)
{
  HANDLE Handle;
  PTIMER Ret = NULL;

  if (!FirstpTmr)
  {
      FirstpTmr = UserCreateObject(gHandleTable, &Handle, otTimer, sizeof(TIMER));
      if (FirstpTmr) InitializeListHead(&FirstpTmr->ptmrList);
      Ret = FirstpTmr;
  }
  else
  {
      Ret = UserCreateObject(gHandleTable, &Handle, otTimer, sizeof(TIMER));
      if (Ret) InsertTailList(&FirstpTmr->ptmrList, &Ret->ptmrList);
  } 
  return Ret;
}

static
BOOL
FASTCALL
RemoveTimer(PTIMER pTmr)
{
  if (pTmr)
  {
     RemoveEntryList(&pTmr->ptmrList);
     UserDeleteObject( USER_BODY_TO_HEADER(pTmr)->hSelf, otTimer);
     return TRUE;
  }
  return FALSE;
}

PTIMER
FASTCALL
FindTimer(PWINDOW_OBJECT Window,
          UINT_PTR nID,
          UINT flags,
          BOOL Distroy)
{
  PTIMER pTmr = FirstpTmr;
  KeEnterCriticalRegion();
  do
  {
    if (!pTmr) break;

    if ( pTmr->nID == nID &&
         pTmr->pWnd == Window &&
        (pTmr->flags & (TMRF_SYSTEM|TMRF_RIT)) == (flags & (TMRF_SYSTEM|TMRF_RIT)))
    {
       if (Distroy)
       {
          RemoveTimer(pTmr);
          pTmr = (PTIMER)1; // We are here to remove the timer.
       }
       break;
    }

    pTmr = (PTIMER)pTmr->ptmrList.Flink;
  } while (pTmr != FirstpTmr);
  KeLeaveCriticalRegion();

  return pTmr;
}

PTIMER
FASTCALL
FindSystemTimer(PMSG pMsg)
{
  PTIMER pTmr = FirstpTmr;
  KeEnterCriticalRegion();
  do
  {
    if (!pTmr) break;

    if ( pMsg->lParam == (LPARAM)pTmr->pfn &&
         (pTmr->flags & TMRF_SYSTEM) )
       break;

    pTmr = (PTIMER)pTmr->ptmrList.Flink;
  } while (pTmr != FirstpTmr);
  KeLeaveCriticalRegion();

  return pTmr;
}

BOOL
FASTCALL
ValidateTimerCallback(PTHREADINFO pti,
                      PWINDOW_OBJECT Window,
                      WPARAM wParam,
                      LPARAM lParam)
{
  PTIMER pTmr = FirstpTmr;

  if (!pTmr) return FALSE;

  KeEnterCriticalRegion();
  do
  {
    if ( (lParam == (LPARAM)pTmr->pfn) &&
         (pTmr->flags & (TMRF_SYSTEM|TMRF_RIT)) &&
         (pTmr->pti->ThreadInfo->ppi == pti->ThreadInfo->ppi) )
       break;

    pTmr = (PTIMER)pTmr->ptmrList.Flink;
  } while (pTmr != FirstpTmr);
  KeLeaveCriticalRegion();

  if (!pTmr) return FALSE;

  return TRUE;
}

// Rename it to IntSetTimer after move.
UINT_PTR FASTCALL
InternalSetTimer( PWINDOW_OBJECT Window,
                  UINT_PTR IDEvent,
                  UINT Elapse,
                  TIMERPROC TimerFunc,
                  INT Type)
{
  PTIMER pTmr;
  LARGE_INTEGER DueTime;
  DueTime.QuadPart = (LONGLONG)(-10000000);

#if 0
  /* Windows NT/2k/XP behaviour */
  if (Elapse > MAX_ELAPSE_TIME)
  {
     DPRINT("Adjusting uElapse\n");
     Elapse = 1;
  }
#else
  /* Windows XP SP2 and Windows Server 2003 behaviour */
  if (Elapse > MAX_ELAPSE_TIME)
  {
     DPRINT("Adjusting uElapse\n");
     Elapse = MAX_ELAPSE_TIME;
  }
#endif

  /* Windows 2k/XP and Windows Server 2003 SP1 behaviour */
  if (Elapse < 10)
  {
     DPRINT("Adjusting uElapse\n");
     Elapse = 10;
  }

  pTmr = FindTimer(Window, IDEvent, Type, FALSE);
  if (!pTmr)
  {
     pTmr = CreateTimer();
     if (!pTmr) return 0;

    if (Window && (Type & TMRF_TIFROMWND))
       pTmr->pti = Window->OwnerThread->Tcb.Win32Thread;
    else
    {
       if (Type & TMRF_RIT)
          pTmr->pti = ptiRawInput;
       else
          pTmr->pti = PsGetCurrentThreadWin32Thread();
     }
     pTmr->pWnd    = Window;
     pTmr->cmsCountdown = Elapse;
     pTmr->cmsRate = Elapse;
     pTmr->flags   = Type|TMRF_INIT; // Set timer to Init mode.
     pTmr->pfn     = TimerFunc;
     pTmr->nID     = IDEvent;

     InsertTailList(&FirstpTmr->ptmrList, &pTmr->ptmrList);
  }

  // Start the timer thread!
  KeSetTimer(MasterTimer, DueTime, NULL);

  if (!pTmr->nID) return 1;
  return pTmr->nID;
}

//
// Process win32k system timers.
//
VOID
CALLBACK
SystemTimerProc(HWND hwnd,
                UINT uMsg,
         UINT_PTR idEvent,
             DWORD dwTime)
{
  DPRINT( "Timer Running!\n" );
}

VOID
FASTCALL
StartTheTimers(VOID)
{
  // Need to start gdi syncro timers then start timer with Hang App proc
  // that calles Idle process so the screen savers will know to run......    
  InternalSetTimer(NULL, 0, 1000, SystemTimerProc, TMRF_RIT);
}

UINT_PTR
FASTCALL
SetSystemTimer( PWINDOW_OBJECT Window,
                UINT_PTR nIDEvent,
                UINT uElapse,
                TIMERPROC lpTimerFunc) 
{
  if (Window && Window->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
  {
     SetLastWin32Error(ERROR_ACCESS_DENIED);
     return 0;
  }
  return InternalSetTimer( Window, nIDEvent, uElapse, lpTimerFunc, TMRF_SYSTEM);
}

BOOL
FASTCALL
PostTimerMessages(PWINDOW_OBJECT Window)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  MSG Msg;
  PTHREADINFO pti;
  BOOL Hit = FALSE;
  PTIMER pTmr = FirstpTmr;

  if (!pTmr) return FALSE;

  if (Window && (int)Window != 1)
  {
     if (!Window->Wnd) return FALSE;
  }

  pti = PsGetCurrentThreadWin32Thread();
  ThreadQueue = pti->MessageQueue;

  KeEnterCriticalRegion();
  do
  {
     if ( (pTmr->flags & TMRF_READY) &&
          (pTmr->pti == pti) &&
          (pTmr->pWnd == Window))
        {
           Msg.hwnd    = Window->hSelf;
           Msg.message = (pTmr->flags & TMRF_SYSTEM) ? WM_SYSTIMER : WM_TIMER;
           Msg.wParam  = (WPARAM) pTmr->nID;
           Msg.lParam  = (LPARAM) pTmr->pfn;
           MsqPostMessage(ThreadQueue, &Msg, FALSE, QS_POSTMESSAGE);

           pTmr->flags &= ~TMRF_READY;
           ThreadQueue->WakeMask = ~QS_TIMER;
           Hit = TRUE;
        }

     pTmr = (PTIMER)pTmr->ptmrList.Flink;
  } while (pTmr != FirstpTmr);
  KeLeaveCriticalRegion();

  return Hit;
}

VOID
FASTCALL
ProcessTimers(VOID)
{
  LARGE_INTEGER TickCount, DueTime;
  LONG Time;
  PTIMER pTmr = FirstpTmr;

  if (!pTmr) return;

  UserEnterExclusive();

  KeQueryTickCount(&TickCount);
  Time = MsqCalculateMessageTime(&TickCount);

  DueTime.QuadPart = (LONGLONG)(-10000000);

  do
  {
    if (pTmr->flags & TMRF_WAITING)
    {
       pTmr = (PTIMER)pTmr->ptmrList.Flink;
       continue;
    }

    if (pTmr->flags & TMRF_INIT)  
       pTmr->flags &= ~TMRF_INIT; // Skip this run.
    else
    {
       if (pTmr->cmsCountdown < 0)
       {
          if (!(pTmr->flags & TMRF_READY))
          {
             if (pTmr->flags & TMRF_ONESHOT)
                pTmr->flags |= TMRF_WAITING;

             if (pTmr->flags & TMRF_RIT)
             {
                // Hard coded call here, inside raw input thread.
                pTmr->pfn(NULL, WM_SYSTIMER, pTmr->nID, (LPARAM)pTmr);
             }
             else
             {
                pTmr->flags |= TMRF_READY; // Set timer ready to be ran.
                // Set thread message queue for this timer.
                if (pTmr->pti->MessageQueue)
                {  // Wakeup thread
                   pTmr->pti->MessageQueue->WakeMask |= QS_TIMER;
                   KeSetEvent(pTmr->pti->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
                }
             }
          }
          pTmr->cmsCountdown = pTmr->cmsRate;
       }
       else
          pTmr->cmsCountdown -= Time - TimeLast;
    }
    pTmr = (PTIMER)pTmr->ptmrList.Flink;
  } while (pTmr != FirstpTmr);

  // Restart the timer thread!
  KeSetTimer(MasterTimer, DueTime, NULL);

  TimeLast = Time;

  UserLeave();
}

//
//
// Old Timer Queueing
//
//
UINT_PTR FASTCALL
IntSetTimer(HWND Wnd, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, BOOL SystemTimer)
{
   PWINDOW_OBJECT Window;
   UINT_PTR Ret = 0;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   DPRINT("IntSetTimer wnd %x id %p elapse %u timerproc %p systemtimer %s\n",
          Wnd, IDEvent, Elapse, TimerFunc, SystemTimer ? "TRUE" : "FALSE");

   if ((Wnd == NULL) && ! SystemTimer)
   {
      DPRINT("Window-less timer\n");
      /* find a free, window-less timer id */
      IntLockWindowlessTimerBitmap();
      IDEvent = RtlFindClearBitsAndSet(&WindowLessTimersBitMap, 1, HintIndex);

      if (IDEvent == (UINT_PTR) -1)
      {
         IntUnlockWindowlessTimerBitmap();
         DPRINT1("Unable to find a free window-less timer id\n");
         SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
         return 0;
      }

      HintIndex = ++IDEvent;
      IntUnlockWindowlessTimerBitmap();
      Ret = IDEvent;
      pti = PsGetCurrentThreadWin32Thread();
      MessageQueue = pti->MessageQueue;
   }
   else
   {
      if (!(Window = UserGetWindowObject(Wnd)))
      {
         DPRINT1("Invalid window handle\n");
         return 0;
      }

      if (Window->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
      {
         DPRINT1("Trying to set timer for window in another process (shatter attack?)\n");
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return 0;
      }

      Ret = IDEvent;
      MessageQueue = Window->MessageQueue;
   }

#if 0

   /* Windows NT/2k/XP behaviour */
   if (Elapse > 0x7fffffff)
   {
      DPRINT("Adjusting uElapse\n");
      Elapse = 1;
   }

#else

   /* Windows XP SP2 and Windows Server 2003 behaviour */
   if (Elapse > 0x7fffffff)
   {
      DPRINT("Adjusting uElapse\n");
      Elapse = 0x7fffffff;
   }

#endif

   /* Windows 2k/XP and Windows Server 2003 SP1 behaviour */
   if (Elapse < 10)
   {
      DPRINT("Adjusting uElapse\n");
      Elapse = 10;
   }

   if (! MsqSetTimer(MessageQueue, Wnd,
                     IDEvent, Elapse, TimerFunc,
                     SystemTimer ? WM_SYSTIMER : WM_TIMER))
   {
      DPRINT1("Failed to set timer in message queue\n");
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
   }


   return Ret;
}


BOOL FASTCALL
IntKillTimer(HWND Wnd, UINT_PTR IDEvent, BOOL SystemTimer)
{
   PTHREADINFO pti;
   PWINDOW_OBJECT Window = NULL;
   
   DPRINT("IntKillTimer wnd %x id %p systemtimer %s\n",
          Wnd, IDEvent, SystemTimer ? "TRUE" : "FALSE");

   pti = PsGetCurrentThreadWin32Thread();
   if (Wnd)
   {
      Window = UserGetWindowObject(Wnd);
   
      if (! MsqKillTimer(pti->MessageQueue, Wnd,
                                IDEvent, SystemTimer ? WM_SYSTIMER : WM_TIMER))
      {
         // Give it another chance to find the timer.
         if (Window && !( MsqKillTimer(Window->MessageQueue, Wnd,
                            IDEvent, SystemTimer ? WM_SYSTIMER : WM_TIMER)))
         {
            DPRINT1("Unable to locate timer in message queue for Window.\n");
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return FALSE;
         }
      }
   }

   /* window-less timer? */
   if ((Wnd == NULL) && ! SystemTimer)
   {
      if (! MsqKillTimer(pti->MessageQueue, Wnd,
                                IDEvent, SystemTimer ? WM_SYSTIMER : WM_TIMER))
      {
         DPRINT1("Unable to locate timer in message queue for Window-less timer.\n");
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return FALSE;
      }

      /* Release the id */
      IntLockWindowlessTimerBitmap();

      ASSERT(RtlAreBitsSet(&WindowLessTimersBitMap, IDEvent - 1, 1));
      RtlClearBits(&WindowLessTimersBitMap, IDEvent - 1, 1);

      IntUnlockWindowlessTimerBitmap();
   }

   return TRUE;
}

NTSTATUS FASTCALL
InitTimerImpl(VOID)
{
   ULONG BitmapBytes;

   ExInitializeFastMutex(&Mutex);

   BitmapBytes = ROUND_UP(NUM_WINDOW_LESS_TIMERS, sizeof(ULONG) * 8) / 8;
   WindowLessTimersBitMapBuffer = ExAllocatePoolWithTag(PagedPool, BitmapBytes, TAG_TIMERBMP);
   if (WindowLessTimersBitMapBuffer == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   RtlInitializeBitMap(&WindowLessTimersBitMap,
                       WindowLessTimersBitMapBuffer,
                       BitmapBytes * 8);

   /* yes we need this, since ExAllocatePool isn't supposed to zero out allocated memory */
   RtlClearAllBits(&WindowLessTimersBitMap);

   return STATUS_SUCCESS;
}

UINT_PTR
APIENTRY
NtUserSetTimer
(
   HWND hWnd,
   UINT_PTR nIDEvent,
   UINT uElapse,
   TIMERPROC lpTimerFunc
)
{
   DECLARE_RETURN(UINT_PTR);

   DPRINT("Enter NtUserSetTimer\n");
   UserEnterExclusive();

   RETURN(IntSetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc, FALSE));

CLEANUP:
   DPRINT("Leave NtUserSetTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
APIENTRY
NtUserKillTimer
(
   HWND hWnd,
   UINT_PTR uIDEvent
)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserKillTimer\n");
   UserEnterExclusive();

   RETURN(IntKillTimer(hWnd, uIDEvent, FALSE));

CLEANUP:
   DPRINT("Leave NtUserKillTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


UINT_PTR
APIENTRY
NtUserSetSystemTimer(
   HWND hWnd,
   UINT_PTR nIDEvent,
   UINT uElapse,
   TIMERPROC lpTimerFunc
)
{
   DECLARE_RETURN(UINT_PTR);

   DPRINT("Enter NtUserSetSystemTimer\n");
   UserEnterExclusive();

   // This is wrong, lpTimerFunc is NULL!
   RETURN(IntSetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc, TRUE));

CLEANUP:
   DPRINT("Leave NtUserSetSystemTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


/* EOF */
