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

   if (!FirstpTmr)
   {
      FirstpTmr = ExAllocatePoolWithTag(PagedPool, sizeof(TIMER), TAG_TIMERBMP);
      if (FirstpTmr) InitializeListHead(&FirstpTmr->ptmrList);
   }

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

   RETURN(IntSetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc, TRUE));

CLEANUP:
   DPRINT("Leave NtUserSetSystemTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


/* EOF */
