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
/* $Id: timer.c,v 1.11 2003/10/04 22:36:37 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window timers messages
 * FILE:             subsys/win32k/ntuser/timer.c
 * PROGRAMER:        Gunnar
 * REVISION HISTORY:
 *
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/ntuser.h>
#include <internal/ntoskrnl.h>
#include <internal/ps.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/error.h>
#include <messages.h>
#include <napi/win32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

//windows 2000 has room for 32768 handle-less timers
#define NUM_HANDLE_LESS_TIMERS   1024

static FAST_MUTEX     Mutex;
static LIST_ENTRY     TimerListHead;
static LIST_ENTRY     SysTimerListHead;
static KTIMER         Timer;
static RTL_BITMAP     HandleLessTimersBitMap;
static PVOID          HandleLessTimersBitMapBuffer;
static ULONG          HintIndex = 0;
static HANDLE        MsgTimerThreadHandle;
static CLIENT_ID     MsgTimerThreadId;


typedef struct _MSG_TIMER_ENTRY{
   LIST_ENTRY     ListEntry;
   LARGE_INTEGER  Timeout;
   HANDLE          ThreadID;
   UINT           Period;
   MSG            Msg;
} MSG_TIMER_ENTRY, *PMSG_TIMER_ENTRY;


/* FUNCTIONS *****************************************************************/


//return true if the new timer became the first entry
//must hold mutex while calling this
BOOL
FASTCALL
InsertTimerAscendingOrder(PMSG_TIMER_ENTRY NewTimer, BOOL SysTimer)
{
   PLIST_ENTRY EnumEntry, InsertAfter;
   PMSG_TIMER_ENTRY MsgTimer;

   InsertAfter = NULL;

   if(!SysTimer)
   {
     EnumEntry = TimerListHead.Flink;
     while (EnumEntry != &TimerListHead)
     {
        MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
        if (NewTimer->Timeout.QuadPart > MsgTimer->Timeout.QuadPart)
        {
           InsertAfter = EnumEntry;
        }
        EnumEntry = EnumEntry->Flink;
     }

     if (InsertAfter)
     {
        InsertTailList(InsertAfter, &NewTimer->ListEntry);
        return FALSE;
     }
     //insert as first entry
     InsertHeadList(&TimerListHead, &NewTimer->ListEntry);
   }
   else
   {
     EnumEntry = SysTimerListHead.Flink;
     while (EnumEntry != &SysTimerListHead)
     {
        MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
        if (NewTimer->Timeout.QuadPart > MsgTimer->Timeout.QuadPart)
        {
           InsertAfter = EnumEntry;
        }
        EnumEntry = EnumEntry->Flink;
     }

     if (InsertAfter)
     {
        InsertTailList(InsertAfter, &NewTimer->ListEntry);
        return FALSE;
     }
     //insert as first entry
     InsertHeadList(&SysTimerListHead, &NewTimer->ListEntry);
   }

   return TRUE;

}

//must hold mutex while calling this
PMSG_TIMER_ENTRY
FASTCALL
RemoveTimer(HWND hWnd, UINT_PTR IDEvent, HANDLE ThreadID, BOOL SysTimer)
{
   PMSG_TIMER_ENTRY MsgTimer;
   PLIST_ENTRY EnumEntry;

   if(!SysTimer)
   {
     //remove timer if allready in the queue
     EnumEntry = TimerListHead.Flink;
     while (EnumEntry != &TimerListHead)
     {
        MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
        EnumEntry = EnumEntry->Flink;
    
        if (MsgTimer->Msg.hwnd == hWnd && 
            MsgTimer->Msg.wParam == (WPARAM)IDEvent &&
            MsgTimer->ThreadID == ThreadID)
        {
           RemoveEntryList(&MsgTimer->ListEntry);
           return MsgTimer;
        }
     }
   }
   else
   {
     //remove timer if allready in the queue
     EnumEntry = SysTimerListHead.Flink;
     while (EnumEntry != &SysTimerListHead)
     {
        MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
        EnumEntry = EnumEntry->Flink;
    
        if (MsgTimer->Msg.hwnd == hWnd && 
            MsgTimer->Msg.wParam == (WPARAM)IDEvent &&
            MsgTimer->ThreadID == ThreadID)
        {
           RemoveEntryList(&MsgTimer->ListEntry);
           return MsgTimer;
        }
     }
   }

   return NULL;
}


/* 
 * NOTE: It doesn't kill timers. It just removes them from the list.
 */
VOID
FASTCALL
RemoveTimersThread(HANDLE ThreadID)
{
   PMSG_TIMER_ENTRY MsgTimer;
   PLIST_ENTRY EnumEntry;

   ExAcquireFastMutex(&Mutex);

   EnumEntry = SysTimerListHead.Flink;
   while (EnumEntry != &SysTimerListHead)
   {
      MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;

      if (MsgTimer->ThreadID == ThreadID)
      {
         RemoveEntryList(&MsgTimer->ListEntry);
         ExFreePool(MsgTimer);
      }
   }

   EnumEntry = TimerListHead.Flink;
   while (EnumEntry != &TimerListHead)
   {
      MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;

      if (MsgTimer->ThreadID == ThreadID)
      {
         if (MsgTimer->Msg.hwnd == NULL)
         {
            RtlClearBits(&HandleLessTimersBitMap, ((UINT_PTR)MsgTimer->Msg.wParam) - 1, 1);   
         }

         RemoveEntryList(&MsgTimer->ListEntry);
         ExFreePool(MsgTimer);
      }
   }

   ExReleaseFastMutex(&Mutex);

}



UINT_PTR
STDCALL
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
)
{
 ULONG Index;
 PMSG_TIMER_ENTRY MsgTimer2, MsgTimer = NULL;
 PMSG_TIMER_ENTRY NewTimer;
 PLIST_ENTRY EnumEntry;
 LARGE_INTEGER CurrentTime;
 PWINDOW_OBJECT WindowObject;
 HANDLE ThreadID;

 ThreadID = PsGetCurrentThreadId();
 KeQuerySystemTime(&CurrentTime);
 ExAcquireFastMutex(&Mutex);

 if(hWnd == NULL)
 {
  /* find a free, handle-less timer id */
  Index = RtlFindClearBitsAndSet(&HandleLessTimersBitMap, 1, HintIndex);

  if(Index == (ULONG) -1)
  {
   ExReleaseFastMutex(&Mutex);
   return 0;
  }

  ++ Index;
  HintIndex = Index;
  ExReleaseFastMutex(&Mutex);
  return Index;
 }
 else
 {
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
   ExReleaseFastMutex(&Mutex);
   SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
   return 0;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
   IntReleaseWindowObject(WindowObject);
   ExReleaseFastMutex(&Mutex);
   SetLastWin32Error(ERROR_ACCESS_DENIED);
   return 0;
  }
  IntReleaseWindowObject(WindowObject);
   
  /* remove timer if already in the queue */
  MsgTimer = RemoveTimer(hWnd, nIDEvent, ThreadID, FALSE); 
 }
 
 #if 1
 
 /* Win NT/2k/XP */
 if(uElapse > 0x7fffffff)
   uElapse = 1;
 
 #else
 
 /* Win Server 2003 */
 if(uElapse > 0x7fffffff)
   uElapse = 0x7fffffff;
 
 #endif
 
 /* Win 2k/XP */
 if(uElapse < 10)
   uElapse = 10;

 if(MsgTimer)
 {
  /* modify existing (removed) timer */
  NewTimer = MsgTimer;

  NewTimer->Period = uElapse;
  NewTimer->Timeout.QuadPart = CurrentTime.QuadPart + (uElapse * 10000);
  NewTimer->Msg.lParam = (LPARAM)lpTimerFunc;
 }
 else
 {
  /* FIXME: use lookaside? */
  NewTimer = ExAllocatePool(PagedPool, sizeof(MSG_TIMER_ENTRY));
  if(!NewTimer)
  {
    ExReleaseFastMutex(&Mutex);
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  NewTimer->Msg.hwnd = hWnd;
  NewTimer->Msg.message = WM_TIMER;
  NewTimer->Msg.wParam = (WPARAM)nIDEvent;
  NewTimer->Msg.lParam = (LPARAM)lpTimerFunc;
  NewTimer->Period = uElapse;
  NewTimer->Timeout.QuadPart = CurrentTime.QuadPart + (uElapse * 10000);
  NewTimer->ThreadID = ThreadID;
 }

 if(InsertTimerAscendingOrder(NewTimer, FALSE))
 {
  EnumEntry = SysTimerListHead.Flink;
  if(EnumEntry != &SysTimerListHead)
  {
    MsgTimer2 = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
    if (NewTimer->Timeout.QuadPart <= MsgTimer2->Timeout.QuadPart)
    {
      /* new timer is first in queue and expires first */
      KeSetTimer(&Timer, NewTimer->Timeout, NULL);
    }
  }
  else
  {  
   /* new timer is first in queue and expires first */
   KeSetTimer(&Timer, NewTimer->Timeout, NULL);
  }
 }

 ExReleaseFastMutex(&Mutex);

 return 1;
}


BOOL
STDCALL
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
 PMSG_TIMER_ENTRY MsgTimer;
 PWINDOW_OBJECT WindowObject;
 
 ExAcquireFastMutex(&Mutex);

 /* handle-less timer? */
 if(hWnd == NULL)
 {
  if(!RtlAreBitsSet(&HandleLessTimersBitMap, uIDEvent - 1, 1))
  {
   /* bit was not set */
   /* FIXME: set the last error */
   ExReleaseFastMutex(&Mutex); 
   return FALSE;
  }

  RtlClearBits(&HandleLessTimersBitMap, uIDEvent - 1, 1);
 }
 else
 {
   WindowObject = IntGetWindowObject(hWnd);
   if(!WindowObject)
   {
     ExReleaseFastMutex(&Mutex); 
     SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
     return FALSE;
   }
   if(WindowObject->OwnerThread != PsGetCurrentThread())
   {
     IntReleaseWindowObject(WindowObject);
     ExReleaseFastMutex(&Mutex); 
     SetLastWin32Error(ERROR_ACCESS_DENIED);
     return FALSE;
   }
   IntReleaseWindowObject(WindowObject);
 }

 MsgTimer = RemoveTimer(hWnd, uIDEvent, PsGetCurrentThreadId(), FALSE);

 ExReleaseFastMutex(&Mutex);

 if(MsgTimer == NULL)
 {
  /* didn't find timer */
  /* FIXME: set the last error */
  return FALSE;
 }

 /* FIXME: use lookaside? */
 ExFreePool(MsgTimer);
 
 return TRUE;
}

UINT_PTR
STDCALL
NtUserSetSystemTimer(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
)
{
 /* As opposed to SetTimer() this one seems not to allow window-less timers! */
 PMSG_TIMER_ENTRY MsgTimer2, MsgTimer = NULL;
 PMSG_TIMER_ENTRY NewTimer;
 PLIST_ENTRY EnumEntry;
 LARGE_INTEGER CurrentTime;
 PWINDOW_OBJECT WindowObject;
 HANDLE ThreadID;

 ThreadID = PsGetCurrentThreadId();
 KeQuerySystemTime(&CurrentTime);
 ExAcquireFastMutex(&Mutex);

 if(hWnd == NULL)
 {
   ExReleaseFastMutex(&Mutex);
   SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
   return 0;
 }
 else
 {
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
   ExReleaseFastMutex(&Mutex);
   SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
   return 0;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
   IntReleaseWindowObject(WindowObject);
   ExReleaseFastMutex(&Mutex);
   SetLastWin32Error(ERROR_ACCESS_DENIED);
   return 0;
  }
  IntReleaseWindowObject(WindowObject);
   
  /* remove timer if already in the queue */
  MsgTimer = RemoveTimer(hWnd, nIDEvent, ThreadID, TRUE); 
 }
 
 #if 1
 
 /* Win NT/2k/XP */
 if(uElapse > 0x7fffffff)
   uElapse = 1;
 
 #else
 
 /* Win Server 2003 */
 if(uElapse > 0x7fffffff)
   uElapse = 0x7fffffff;
 
 #endif
 
 /* Win 2k/XP */
 if(uElapse < 10)
   uElapse = 10;

 if(MsgTimer)
 {
  /* modify existing (removed) timer */
  NewTimer = MsgTimer;

  NewTimer->Period = uElapse;
  NewTimer->Timeout.QuadPart = CurrentTime.QuadPart + (uElapse * 10000);
  NewTimer->Msg.lParam = (LPARAM)lpTimerFunc;
 }
 else
 {
  /* FIXME: use lookaside? */
  NewTimer = ExAllocatePool(PagedPool, sizeof(MSG_TIMER_ENTRY));
  if(!NewTimer)
  {
    ExReleaseFastMutex(&Mutex);
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  NewTimer->Msg.hwnd = hWnd;
  NewTimer->Msg.message = WM_SYSTIMER;
  NewTimer->Msg.wParam = (WPARAM)nIDEvent;
  NewTimer->Msg.lParam = (LPARAM)lpTimerFunc;
  NewTimer->Period = uElapse;
  NewTimer->Timeout.QuadPart = CurrentTime.QuadPart + (uElapse * 10000);
  NewTimer->ThreadID = ThreadID;
 }

 if(InsertTimerAscendingOrder(NewTimer, TRUE))
 {
  EnumEntry = TimerListHead.Flink;
  if(EnumEntry != &TimerListHead)
  {
    MsgTimer2 = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
    if (NewTimer->Timeout.QuadPart <= MsgTimer2->Timeout.QuadPart)
    {
      /* new timer is first in queue and expires first */
      KeSetTimer(&Timer, NewTimer->Timeout, NULL);
    }
  }
  else
  {  
   /* new timer is first in queue and expires first */
   KeSetTimer(&Timer, NewTimer->Timeout, NULL);
  }
 }

 ExReleaseFastMutex(&Mutex);

 return 1;
}

BOOL
STDCALL
NtUserKillSystemTimer(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
 /* As opposed to KillTimer() this one seems not to allow window-less timers! */
 PMSG_TIMER_ENTRY MsgTimer;
 PWINDOW_OBJECT WindowObject;
 
 WindowObject = IntGetWindowObject(hWnd);

 /* handle-less timer? Not allowed for SystemTimers! */
 if(!WindowObject)
 {
   SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
   return FALSE;
 }
 if(WindowObject->OwnerThread != PsGetCurrentThread())
 {
   IntReleaseWindowObject(WindowObject);
   SetLastWin32Error(ERROR_ACCESS_DENIED);
   return FALSE;
 }
 IntReleaseWindowObject(WindowObject);
 
 ExAcquireFastMutex(&Mutex);

 MsgTimer = RemoveTimer(hWnd, uIDEvent, PsGetCurrentThreadId(), TRUE);

 ExReleaseFastMutex(&Mutex);

 if(MsgTimer == NULL)
 {
  /* didn't find timer */
  /* FIXME: set the last error */
  return FALSE;
 }

 /* FIXME: use lookaside? */
 ExFreePool(MsgTimer);

 return TRUE;
}


static VOID STDCALL_FUNC
TimerThreadMain(
   PVOID StartContext
   )
{
   NTSTATUS Status;
   LARGE_INTEGER CurrentTime;
   PLIST_ENTRY EnumEntry;
   PMSG_TIMER_ENTRY MsgTimer, MsgTimer2;
   PETHREAD Thread;

   for (;;)
   {

      Status = KeWaitForSingleObject(   &Timer,
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

      ExAcquireFastMutex(&Mutex);
      
      KeQuerySystemTime(&CurrentTime);
      
      EnumEntry = SysTimerListHead.Flink;
      while (EnumEntry != &SysTimerListHead)
      {
         MsgTimer2 = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
         EnumEntry = EnumEntry->Flink;

         if (CurrentTime.QuadPart >= MsgTimer2->Timeout.QuadPart)
         {
            RemoveEntryList(&MsgTimer2->ListEntry);

            /* 
             * FIXME: 1) Find a faster way of getting the thread message queue? (lookup by id is slow)
             */

            if (!NT_SUCCESS(PsLookupThreadByThreadId(MsgTimer2->ThreadID, &Thread)))
            {
               ExFreePool(MsgTimer2);
               continue;
            }
            
            MsqPostMessage(((PW32THREAD)Thread->Win32Thread)->MessageQueue, MsqCreateMessage(&MsgTimer2->Msg));

            ObDereferenceObject(Thread);

            //set up next periodic timeout
            MsgTimer2->Timeout.QuadPart += (MsgTimer2->Period * 10000); 
            InsertTimerAscendingOrder(MsgTimer2, TRUE);

         }                          
         else                       
         {
            break;
         }
      }

      EnumEntry = TimerListHead.Flink;
      while (EnumEntry != &TimerListHead)
      {
         MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
         EnumEntry = EnumEntry->Flink;

         if (CurrentTime.QuadPart >= MsgTimer->Timeout.QuadPart)
         {
            RemoveEntryList(&MsgTimer->ListEntry);

            /* 
             * FIXME: 1) Find a faster way of getting the thread message queue? (lookup by id is slow)
             */

            if (!NT_SUCCESS(PsLookupThreadByThreadId(MsgTimer->ThreadID, &Thread)))
            {
               ExFreePool(MsgTimer);
               continue;
            }
            
            MsqPostMessage(((PW32THREAD)Thread->Win32Thread)->MessageQueue, MsqCreateMessage(&MsgTimer->Msg));

            ObDereferenceObject(Thread);

            //set up next periodic timeout
            MsgTimer->Timeout.QuadPart += (MsgTimer->Period * 10000); 
            InsertTimerAscendingOrder(MsgTimer, FALSE);

         }                          
         else                       
         {
            break;
         }
      }
   
      //set up next timeout from first entry (if any)
      if (!IsListEmpty(&TimerListHead))
      {
         MsgTimer = CONTAINING_RECORD( TimerListHead.Flink, MSG_TIMER_ENTRY, ListEntry);
         if(!IsListEmpty(&SysTimerListHead))
         {         
           MsgTimer2 = CONTAINING_RECORD( SysTimerListHead.Flink, MSG_TIMER_ENTRY, ListEntry);
           if(MsgTimer->Timeout.QuadPart >= MsgTimer2->Timeout.QuadPart)
             KeSetTimer(&Timer, MsgTimer->Timeout, NULL);
           else
             KeSetTimer(&Timer, MsgTimer2->Timeout, NULL);
         }
         else
         {
           KeSetTimer(&Timer, MsgTimer->Timeout, NULL);
         }
      }
      else
      {
         if(!IsListEmpty(&SysTimerListHead))
         {
           MsgTimer2 = CONTAINING_RECORD( SysTimerListHead.Flink, MSG_TIMER_ENTRY, ListEntry);
           KeSetTimer(&Timer, MsgTimer2->Timeout, NULL);
         }
         else
         {
           /* Reinitialize the timer, this reset the state of the timer event on which we wait */
           KeInitializeTimer(&Timer);
         }
      }

      ExReleaseFastMutex(&Mutex);

   }

}



NTSTATUS FASTCALL
InitTimerImpl(VOID)
{
   NTSTATUS Status;
   ULONG BitmapBytes;

   BitmapBytes = ROUND_UP(NUM_HANDLE_LESS_TIMERS, sizeof(ULONG) * 8) / 8;

   InitializeListHead(&TimerListHead);
   InitializeListHead(&SysTimerListHead);
   KeInitializeTimer(&Timer);
   ExInitializeFastMutex(&Mutex);

   HandleLessTimersBitMapBuffer = ExAllocatePool(PagedPool, BitmapBytes);
   RtlInitializeBitMap(&HandleLessTimersBitMap,
                       HandleLessTimersBitMapBuffer,
                       BitmapBytes * 8);

   //yes we need this, since ExAllocatePool isn't supposed to zero out allocated memory
   RtlClearAllBits(&HandleLessTimersBitMap); 

   Status = PsCreateSystemThread(&MsgTimerThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &MsgTimerThreadId,
                                 TimerThreadMain,
                                 NULL);
   return Status;
}

/* EOF */
