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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window timers messages
 * FILE:             subsys/win32k/ntuser/timer.c
 * PROGRAMER:        Gunnar
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY: 10/04/2003 Implemented System Timers
 *
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/*

Windowless timers:
------------------

In Windows, windowless timers are "system" global. There is a "system" wide
bitmap keeping track of what ids are free and this bitmap is 32768 bits large
(w2k sp4). There is two problems with this:
-You can kill _any_ windowless timer regardless of thread/process.
-You can easily exhaust the "system" wide bitmap and freeze the "system".

In Wine otoh, windowless timers are bound to threads, just like windowed timers.

----------------

The Windows behaviour sounds really wrong, but the Wine behaviour may
be too restrictive. As a comprimise, i choose to bind windowless timers to the 
process. Lets hope no app rely on killing windowless timers from a different
process:-P

----------------

The meaning of the word "system" above:
system = WinSta or Session. Im not certain of which, but my guess is Session.

-Gunnar

*/

#define NUM_WINDOW_LESS_TIMERS   32768 /* 32768 bits = 4096 bytes = 1 page */

static RTL_BITMAP     WindowLessTimersBitMap;
static PVOID          WindowLessTimersBitMapBuffer;
static ULONG          HintIndex = 0;
static LIST_ENTRY     gPendingTimersList;
//static LIST_ENTRY     gExpiredTimersList;
static KTIMER         gTimer;
static HANDLE        MsgTimerThreadHandle;
static CLIENT_ID     MsgTimerThreadId;
static PAGED_LOOKASIDE_LIST TimerLookasideList;


STATIC VOID STDCALL
TimerThreadMain(PVOID StartContext);

/* FUNCTIONS *****************************************************************/

inline VOID FASTCALL UserFreeTimer(PTIMER_ENTRY Timer)
{
   ExFreeToPagedLookasideList(&TimerLookasideList, Timer);
}

VOID FASTCALL
UserRemoveTimersWindow(PWINDOW_OBJECT Wnd)
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry, OriginalFirstEntry;
   
   ASSERT(Wnd);

   OriginalFirstEntry = gPendingTimersList.Flink;

   /* remove pending timers */
   LIST_FOR_EACH_SAFE(EnumEntry, &gPendingTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Timer->Wnd == Wnd)
      {
         DPRINT("Removing timer %p because its window is going away\n", Timer);
         RemoveEntryList(&Timer->ListEntry);
         
         UserFreeTimer(Timer);
      }
   }

   /* did we remove the first pending entry? */
   if (gPendingTimersList.Flink != OriginalFirstEntry)
      UserSetNextPendingTimer();


   /* remove expired timers */
   LIST_FOR_EACH_SAFE(EnumEntry, &Wnd->WThread->WProcess->ExpiredTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Timer->Wnd == Wnd)
      {
         DPRINT("Removing timer %p because its window is going away\n", Timer);

         RemoveEntryList(&Timer->ListEntry);
         
         //FIXME: MsqIncTimerCount() ?
         Timer->WThread->Queue->TimerCount--;
         if (Timer->WThread->Queue->TimerCount == 0)
            MsqClearQueueBits(Timer->WThread->Queue, QS_TIMER);
         
         UserFreeTimer(Timer);
      }
   }
   
}

VOID FASTCALL
UserRemoveTimersThread(PW32THREAD WThread)
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry, OriginalFirstEntry;

   ASSERT(WThread);

   OriginalFirstEntry = gPendingTimersList.Flink;

   /* remove pending timers */
   LIST_FOR_EACH_SAFE(EnumEntry, &gPendingTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Timer->WThread == WThread)
      {
         DPRINT("Removing timer %p because its w32thread is going away\n", Timer);
         RemoveEntryList(&Timer->ListEntry);
         
         UserFreeTimer(Timer);
      }
   }

   /* did we remove the first pending entry? */
   if (OriginalFirstEntry != gPendingTimersList.Flink)
      UserSetNextPendingTimer();


   /* remove expired timers */
   LIST_FOR_EACH_SAFE(EnumEntry, &WThread->WProcess->ExpiredTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Timer->WThread == WThread)
      {
         DPRINT("Removing timer %p because its w32thread is going away\n", Timer);
         RemoveEntryList(&Timer->ListEntry);
         
         //FIXME: MsqDecTimerCount() ?
         Timer->WThread->Queue->TimerCount--;
        
         UserFreeTimer(Timer);
      }
   }

   ASSERT(WThread->Queue->TimerCount == 0);
   MsqClearQueueBits(WThread->Queue, QS_TIMER);
}


VOID FASTCALL
UserInsertPendingTimer(PTIMER_ENTRY NewTimer)
{
   InsertAscendingListFIFO
   (
      &gPendingTimersList,
      TIMER_ENTRY,
      ListEntry,
      NewTimer,
      ExpiryTime.QuadPart
   );
   
   /* did we become the first pending entry? */
   if (gPendingTimersList.Flink == &NewTimer->ListEntry)
      UserSetNextPendingTimer();
}




VOID FASTCALL UserSetNextPendingTimer()
{
   if (!IsListEmpty(&gPendingTimersList))
   {
      PTIMER_ENTRY Timer = CONTAINING_RECORD( gPendingTimersList.Flink, TIMER_ENTRY, ListEntry);
      KeSetTimer(&gTimer, Timer->ExpiryTime, NULL);
   }
   else
   {
      KeCancelTimer(&gTimer);
   }
}
 
 
 
 
 
 
/* restart an expired timer */
VOID FASTCALL
UserRestartTimer(PTIMER_ENTRY Timer )
{
   LARGE_INTEGER CurrentTime;

   RemoveEntryList(&Timer->ListEntry);
   
   Timer->WThread->Queue->TimerCount--;
   if (Timer->WThread->Queue->TimerCount == 0)
      MsqSetQueueBits(Timer->WThread->Queue, QS_TIMER);
   
   KeQuerySystemTime(&CurrentTime);
   
   /* adjust timeout to a value forth (or current) in time */
   while (Timer->ExpiryTime.QuadPart < CurrentTime.QuadPart)
   {
      Timer->ExpiryTime.QuadPart += (ULONGLONG) Timer->Period * (ULONGLONG) 10000;
   }

   UserInsertPendingTimer(Timer);
}


 
 
PTIMER_ENTRY FASTCALL
UserFindExpiredTimer(   
   PW32THREAD WThread,
   PWINDOW_OBJECT Wnd OPTIONAL, 
   UINT MsgFilterMin, 
   UINT MsgFilterMax,
   BOOL Remove
   )
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry;
   
   LIST_FOR_EACH_SAFE(EnumEntry, &WThread->WProcess->ExpiredTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Wnd && Timer->Wnd != Wnd) continue;

      if (Timer->WThread != WThread) continue;
      
      if (UserMessageFilter(Timer->Message, MsgFilterMin, MsgFilterMax))
      {
         if (Remove)
            UserRestartTimer(Timer);
         
         return Timer;
      }
   }

   return NULL;
}


 
 
 
/*
Get and remove a timer
*/
PTIMER_ENTRY FASTCALL
UserRemoveTimer(
   PWINDOW_OBJECT Wnd OPTIONAL, 
   UINT_PTR IDEvent, 
   UINT Message
   )
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry, OriginalFirstEntry;

   OriginalFirstEntry = gPendingTimersList.Flink;
   
   /* remove timer if in the pending queue */
   LIST_FOR_EACH_SAFE(EnumEntry, &gPendingTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Timer->Wnd == Wnd && 
          Timer->IDEvent == IDEvent &&
          Timer->Message == Message)
      {
         RemoveEntryList(&Timer->ListEntry);
         
         /* did we remove the first pending entry? */
         if (OriginalFirstEntry != gPendingTimersList.Flink)
            UserSetNextPendingTimer();

         return Timer;
      }
   }

   /* remove timer if in the expired queue */
   LIST_FOR_EACH_SAFE(EnumEntry, &PsGetWin32Process()->ExpiredTimersList, Timer, TIMER_ENTRY, ListEntry)
   {
      if (Timer->Wnd == Wnd && 
          Timer->IDEvent == IDEvent &&
          Timer->Message == Message)
      {
         RemoveEntryList(&Timer->ListEntry);
         
         Timer->WThread->Queue->TimerCount--;
         if (Timer->WThread->Queue->TimerCount == 0)
            MsqClearQueueBits(Timer->WThread->Queue, QS_TIMER);

         return Timer;
      }
   }
    
   return NULL;   
}




UINT_PTR FASTCALL
UserSetTimer(
   PWINDOW_OBJECT Wnd OPTIONAL,
   UINT_PTR IDEvent /* IGNORED if Wnd is NULL */,
   UINT Elapse,
   TIMERPROC TimerFunc OPTIONAL,
   BOOL SystemTimer
)
{
   LARGE_INTEGER CurrentTime;
   PTIMER_ENTRY MsgTimer = NULL;
   PW32THREAD WThread;

   DPRINT("IntSetTimer wnd %x id %p elapse %u timerproc %p systemtimer %s\n",
          Wnd, IDEvent, Elapse, TimerFunc, SystemTimer ? "TRUE" : "FALSE");

   if (!Wnd)
   {
      DPRINT("Window-less timer\n");
      
      /* find a free, window-less timer id.
       * note: RtlFindClearBitsAndSet return zero-based value
       */
      IDEvent = RtlFindClearBitsAndSet(&WindowLessTimersBitMap, 1, HintIndex);

      if (IDEvent == (UINT_PTR) -1)
      {
         //FIXME: loop to find a free id for this process
         DPRINT1("Unable to find a free window-less timer id\n");
         SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
         return 0;
      }

      HintIndex = ++IDEvent;

      WThread = PsGetWin32Thread();
   }
   else
   {
      /*
      NOTE: MSDN says window timers must have a NONZERO id. But tons of appz use zero as id
      and Windows allow this (obviously). So we must allow this also. Gunnar
      */
      
      /* docs: window must be owned by current thread. but wine (and ros) check that it belongs to
      owner process.... docs are wrong? should test this...
      */
      if (Wnd->WThread->WProcess != PsGetWin32Process())
      {
         DPRINT1("Trying to set timer for window in another process (shatter attack?)\n");
         /* Wrong! Shatter attack has nothing to do with SetTimer. Shatter attach is posting/sending
         WM_TIMER messages to another process' queue. Gunnar */

         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return 0;
      }
      
      
      MsgTimer = UserRemoveTimer(Wnd, IDEvent, SystemTimer?WM_SYSTIMER:WM_TIMER);
      
      WThread = Wnd->WThread;
   }

#if 1

   /* Win NT/2k/XP */
   if (Elapse > 0x7fffffff)
   {
      DPRINT("Adjusting uElapse\n");
      Elapse = 1;
   }

#else

   /* Win Server 2003 */
   if (Elapse > 0x7fffffff)
   {
      DPRINT("Adjusting uElapse\n");
      Elapse = 0x7fffffff;
   }

#endif

   /* Win 2k/XP */
   if (Elapse < 10)
   {
      DPRINT("Adjusting uElapse\n");
      Elapse = 10;
   }

   if(!MsgTimer)
   {
      MsgTimer = ExAllocateFromPagedLookasideList(&TimerLookasideList);
      if (!MsgTimer)
      {
         //FIXME: last error
         return 0;
      }
   }

   KeQuerySystemTime(&CurrentTime);

   MsgTimer->Wnd = Wnd;
   MsgTimer->Message = SystemTimer ? WM_SYSTIMER : WM_TIMER;
   MsgTimer->Period = Elapse;
   MsgTimer->ExpiryTime.QuadPart = CurrentTime.QuadPart + (Elapse * 10000);
   MsgTimer->WThread = WThread;
   MsgTimer->TimerFunc = TimerFunc;
   MsgTimer->IDEvent = IDEvent;

   UserInsertPendingTimer(MsgTimer);
   
   return IDEvent;
}






BOOL FASTCALL
UserKillTimer(
   PWINDOW_OBJECT Wnd OPTIONAL,
   UINT_PTR IDEvent,
   BOOL SystemTimer
)
{
   DPRINT("IntKillTimer wnd %x id %p systemtimer %s\n",
          Wnd, IDEvent, SystemTimer ? "TRUE" : "FALSE");

   /*
   NOTE: msdn docs says id 0 is invalid, but many appz use it and windows
   allows this. so we must allow this as well.
   */
   
   //FIXME: check process for timers

   if (!Wnd)
   {
      BOOL killedOne;
      
      killedOne = UserRemoveTimer(NULL, IDEvent, SystemTimer ? WM_SYSTIMER : WM_TIMER) != NULL;  
      
      if (killedOne)
      {
         ASSERT(RtlAreBitsSet(&WindowLessTimersBitMap, IDEvent - 1, 1));
         RtlClearBits(&WindowLessTimersBitMap, IDEvent - 1, 1);
      }
      
      return killedOne;
   }
   else
   {
      if (Wnd->WThread->WProcess != PsGetWin32Process())
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return FALSE;
      }

      return UserRemoveTimer(Wnd, IDEvent, SystemTimer ? WM_SYSTIMER : WM_TIMER) != NULL;   
   }

      /* 
      docs says KillTimer doesnt remove already expired/posted timers, but Wine and we do.
      It probably doesnt hurt thou.
      */
   
   

}




NTSTATUS FASTCALL
InitTimerImpl(VOID)
{
   ULONG BitmapBytes;

   KeInitializeTimer(&gTimer);
   InitializeListHead(&gPendingTimersList);
//   InitializeListHead(&gExpiredTimersList);
   
   BitmapBytes = ROUND_UP(NUM_WINDOW_LESS_TIMERS, sizeof(ULONG) * 8) / 8;
   WindowLessTimersBitMapBuffer = ExAllocatePoolWithTag(PagedPool, BitmapBytes, TAG_TIMERBMP);
   RtlInitializeBitMap(&WindowLessTimersBitMap,
                       WindowLessTimersBitMapBuffer,
                       BitmapBytes * 8);

   /* FIXME: is it really necesary to call RtlClearAllBits? Doesnt RtlInitializeBitMap do that for us??
    The docs are not clear on this point...
   */
   /* yes we need this, since ExAllocatePool isn't supposed to zero out allocated memory */
   RtlClearAllBits(&WindowLessTimersBitMap);
   ExInitializePagedLookasideList(&TimerLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(TIMER_ENTRY),
                                  0,
                                  64);

   //FIXME!
   /*Status =*/ PsCreateSystemThread(&MsgTimerThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &MsgTimerThreadId,
                                 TimerThreadMain,
                                 NULL);
   return STATUS_SUCCESS;
}

//FIXME: cleanup impl.

UINT_PTR
STDCALL
NtUserSetTimer
(
   HWND hWnd OPTIONAL,
   UINT_PTR nIDEvent,
   UINT uElapse,
   TIMERPROC lpTimerFunc
)
{
   PWINDOW_OBJECT Wnd=NULL;
   DECLARE_RETURN(UINT_PTR);

   DPRINT("Enter NtUserSetTimer\n");
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Wnd = IntGetWindowObject(hWnd)))
      {
         //SetLast
         RETURN( 0);
      }
   }

   RETURN( UserSetTimer(Wnd, nIDEvent, uElapse, lpTimerFunc, FALSE));

CLEANUP:
   DPRINT("Leave NtUserSetTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
STDCALL
NtUserKillTimer
(
   HWND hWnd OPTIONAL,
   UINT_PTR uIDEvent
)
{
   PWINDOW_OBJECT Wnd=NULL;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserKillTimer\n");
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Wnd = IntGetWindowObject(hWnd)))
      {
         //SetLast
         RETURN( FALSE);
      }
   }

   RETURN( UserKillTimer(Wnd, uIDEvent, FALSE));

CLEANUP:
   DPRINT("Leave NtUserKillTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


UINT_PTR
STDCALL
NtUserSetSystemTimer(
   HWND hWnd OPTIONAL, /* FIXME: hwnd is optional for SetTimer, but is this true for systimers also?*/
   UINT_PTR nIDEvent,
   UINT uElapse,
   TIMERPROC lpTimerFunc
)
{
   PWINDOW_OBJECT Wnd=NULL;
   DECLARE_RETURN(UINT_PTR);

   DPRINT("Enter NtUserSetSystemTimer\n");
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Wnd = IntGetWindowObject(hWnd)))
      {
         //SetLast
         RETURN( 0);
      }
   }

   RETURN( UserSetTimer(Wnd, nIDEvent, uElapse, lpTimerFunc, TRUE));

CLEANUP:
   DPRINT("Leave NtUserSetSystemTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
STDCALL
NtUserKillSystemTimer(
   HWND hWnd OPTIONAL,
   UINT_PTR uIDEvent
)
{
   PWINDOW_OBJECT Wnd=NULL;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserKillSystemTimer\n");
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Wnd = IntGetWindowObject(hWnd)))
      {
         //SetLast
         RETURN( FALSE);
      }
   }

   RETURN( UserKillTimer(Wnd, uIDEvent, TRUE));

CLEANUP:
   DPRINT("Leave NtUserKillSystemTimer, ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}



STATIC VOID STDCALL
TimerThreadMain(PVOID StartContext)
{
   NTSTATUS Status;
   LARGE_INTEGER CurrentTime;
   PLIST_ENTRY EnumEntry;
   PTIMER_ENTRY Timer;
   
   for (;;)
   {

      Status = KeWaitForSingleObject(   &gTimer,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL
                                    );

      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Error waiting in TimerThreadMain\n");
         KEBUGCHECK(0);
      }

      UserEnterExclusive();

      KeQuerySystemTime(&CurrentTime);

      LIST_FOR_EACH_SAFE(EnumEntry, &gPendingTimersList, Timer, TIMER_ENTRY, ListEntry)
      {
         if (CurrentTime.QuadPart >= Timer->ExpiryTime.QuadPart)
         {
            
            DPRINT1("Timer expired: id=%i, elapse=%i, wnd=0x%x, wthread=0x%x\n",Timer->IDEvent, Timer->Period, 
               Timer->Wnd, Timer->WThread);
            
            RemoveEntryList(&Timer->ListEntry);
            
            InsertTailList(&Timer->WThread->WProcess->ExpiredTimersList, &Timer->ListEntry);
            
            Timer->WThread->Queue->TimerCount++;
            MsqSetQueueBits(Timer->WThread->Queue, QS_TIMER);

            continue;
         }

         break;
      }

      UserSetNextPendingTimer();

      UserLeave();
   }

}
/* EOF */
