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

#include <win32k.h>

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

ERESOURCE TimerLock;

#define IntLockWindowlessTimerBitmap() \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&Mutex)

#define IntUnlockWindowlessTimerBitmap() \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&Mutex)

#define TimerEnterExclusive() \
{ \
  KeEnterCriticalRegion(); \
  ExAcquireResourceExclusiveLite(&TimerLock, TRUE); \
}

#define TimerLeave() \
{ \
  ExReleaseResourceLite(&TimerLock); \
  KeLeaveCriticalRegion(); \
}


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
      ExInitializeResourceLite(&TimerLock);
      FirstpTmr = UserCreateObject(gHandleTable, NULL, &Handle, otTimer, sizeof(TIMER));
      if (FirstpTmr)
      {
         FirstpTmr->head.h = Handle;
         InitializeListHead(&FirstpTmr->ptmrList);
      }
      Ret = FirstpTmr;
  }
  else
  {
      Ret = UserCreateObject(gHandleTable, NULL, &Handle, otTimer, sizeof(TIMER));
      if (Ret)
      {
         Ret->head.h = Handle;
         InsertTailList(&FirstpTmr->ptmrList, &Ret->ptmrList);
      }
  }
  return Ret;
}

static
BOOL
FASTCALL
RemoveTimer(PTIMER pTmr)
{
  BOOL Ret = FALSE;
  if (pTmr)
  {
     /* Set the flag, it will be removed when ready */
     RemoveEntryList(&pTmr->ptmrList);
     UserDereferenceObject(pTmr);
     Ret = UserDeleteObject( UserHMGetHandle(pTmr), otTimer);
  }
  if (!Ret) DPRINT1("Warning unable to delete timer\n");

  return Ret;
}

PTIMER
FASTCALL
FindTimer(PWINDOW_OBJECT Window,
          UINT_PTR nID,
          UINT flags,
          BOOL Distroy)
{
  PLIST_ENTRY pLE;
  PTIMER pTmr = FirstpTmr, RetTmr = NULL;
  TimerEnterExclusive();
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
       }
       RetTmr = pTmr;
       break;
    }

    pLE = pTmr->ptmrList.Flink;
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
  } while (pTmr != FirstpTmr);
  TimerLeave();

  return RetTmr;
}

PTIMER
FASTCALL
FindSystemTimer(PMSG pMsg)
{
  PLIST_ENTRY pLE;
  PTIMER pTmr = FirstpTmr;
  TimerEnterExclusive();
  do
  {
    if (!pTmr) break;

    if ( pMsg->lParam == (LPARAM)pTmr->pfn &&
         (pTmr->flags & TMRF_SYSTEM) )
       break;

    pLE = pTmr->ptmrList.Flink;
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);    
  } while (pTmr != FirstpTmr);
  TimerLeave();

  return pTmr;
}

BOOL
FASTCALL
ValidateTimerCallback(PTHREADINFO pti,
                      PWINDOW_OBJECT Window,
                      WPARAM wParam,
                      LPARAM lParam)
{
  PLIST_ENTRY pLE;
  PTIMER pTmr = FirstpTmr;

  if (!pTmr) return FALSE;

  TimerEnterExclusive();
  do
  {
    if ( (lParam == (LPARAM)pTmr->pfn) &&
         (pTmr->flags & (TMRF_SYSTEM|TMRF_RIT)) &&
         (pTmr->pti->ppi == pti->ppi) )
       break;

    pLE = pTmr->ptmrList.Flink;
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
  } while (pTmr != FirstpTmr);
  TimerLeave();

  if (!pTmr) return FALSE;

  return TRUE;
}

UINT_PTR FASTCALL
IntSetTimer( PWINDOW_OBJECT Window,
                  UINT_PTR IDEvent,
                  UINT Elapse,
                  TIMERPROC TimerFunc,
                  INT Type)
{
  PTIMER pTmr;
  UINT Ret= IDEvent;
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

  if ((Window == NULL) && (!(Type & TMRF_SYSTEM)))
  {
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
  }

  if ((Window) && (IDEvent == 0))
     IDEvent = 1;

  pTmr = FindTimer(Window, IDEvent, Type, FALSE);
  if (!pTmr)
  {
     pTmr = CreateTimer();
     if (!pTmr) return 0;

     if (Window && (Type & TMRF_TIFROMWND))
        pTmr->pti = Window->pti->pEThread->Tcb.Win32Thread;
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
     pTmr->pfn     = TimerFunc;
     pTmr->nID     = IDEvent;
     pTmr->flags   = Type|TMRF_INIT; // Set timer to Init mode.
  }
  else
  {
     pTmr->cmsCountdown = Elapse;
     pTmr->cmsRate = Elapse;
  }

  ASSERT(MasterTimer != NULL);
  // Start the timer thread!
  if (pTmr == FirstpTmr)
     KeSetTimer(MasterTimer, DueTime, NULL);

  return Ret;
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
  IntSetTimer(NULL, 0, 1000, SystemTimerProc, TMRF_RIT);
}

UINT_PTR
FASTCALL
SystemTimerSet( PWINDOW_OBJECT Window,
                UINT_PTR nIDEvent,
                UINT uElapse,
                TIMERPROC lpTimerFunc)
{
  if (Window && Window->pti->pEThread->ThreadsProcess != PsGetCurrentProcess())
  {
     SetLastWin32Error(ERROR_ACCESS_DENIED);
     return 0;
  }
  return IntSetTimer( Window, nIDEvent, uElapse, lpTimerFunc, TMRF_SYSTEM);
}

BOOL
FASTCALL
PostTimerMessages(PWINDOW_OBJECT Window)
{
  PLIST_ENTRY pLE;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  MSG Msg;
  PTHREADINFO pti;
  BOOL Hit = FALSE;
  PTIMER pTmr = FirstpTmr;

  if (!pTmr) return FALSE;

  pti = PsGetCurrentThreadWin32Thread();
  ThreadQueue = pti->MessageQueue;

  TimerEnterExclusive();

  do
  {
     if ( (pTmr->flags & TMRF_READY) &&
          (pTmr->pti == pti) &&
          ((pTmr->pWnd == Window) || (Window == NULL) ) )
        {
           Msg.hwnd    = (pTmr->pWnd) ? pTmr->pWnd->hSelf : 0;
           Msg.message = (pTmr->flags & TMRF_SYSTEM) ? WM_SYSTIMER : WM_TIMER;
           Msg.wParam  = (WPARAM) pTmr->nID;
           Msg.lParam  = (LPARAM) pTmr->pfn;

           MsqPostMessage(ThreadQueue, &Msg, FALSE, QS_TIMER);
           pTmr->flags &= ~TMRF_READY;
           ThreadQueue->WakeMask = ~QS_TIMER;
           Hit = TRUE;
        }

     pLE = pTmr->ptmrList.Flink;
     pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
  } while (pTmr != FirstpTmr);

  TimerLeave();

  return Hit;
}

VOID
FASTCALL
ProcessTimers(VOID)
{
  LARGE_INTEGER TickCount, DueTime;
  LONG Time;
  PLIST_ENTRY pLE;
  PTIMER pTmr = FirstpTmr;
  LONG TimerCount = 0;

  if (!pTmr) return;

  TimerEnterExclusive();

  KeQueryTickCount(&TickCount);
  Time = MsqCalculateMessageTime(&TickCount);

  DueTime.QuadPart = (LONGLONG)(-1000000);

  do
  {
    TimerCount++;
    if (pTmr->flags & TMRF_WAITING)
    {
       pLE = pTmr->ptmrList.Flink;
       pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
       continue;
    }

    if (pTmr->flags & TMRF_INIT)
    {
       pTmr->flags &= ~TMRF_INIT; // Skip this run.
    }
    else
    {
       if (pTmr->cmsCountdown < 0)
       {
          ASSERT(pTmr->pti);
          if ((!(pTmr->flags & TMRF_READY)) && (!(pTmr->pti->TIF_flags & TIF_INCLEANUP)))
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
                   ASSERT(pTmr->pti->MessageQueue->NewMessages != NULL);
                   KeSetEvent(pTmr->pti->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
                }
             }
          }
          pTmr->cmsCountdown = pTmr->cmsRate;
       }
       else
          pTmr->cmsCountdown -= Time - TimeLast;
    }

    pLE = pTmr->ptmrList.Flink;
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
  } while (pTmr != FirstpTmr);

  // Restart the timer thread!
  ASSERT(MasterTimer != NULL);
  KeSetTimer(MasterTimer, DueTime, NULL);

  TimeLast = Time;

  TimerLeave();
  DPRINT("TimerCount = %d\n", TimerCount);
}

//
//
// Old Timer Queueing
//
//
UINT_PTR FASTCALL
InternalSetTimer(HWND Wnd, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, BOOL SystemTimer)
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

      if (Window->pti->pEThread->ThreadsProcess != PsGetCurrentProcess())
      {
         DPRINT1("Trying to set timer for window in another process (shatter attack?)\n");
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return 0;
      }

      Ret = IDEvent;
      MessageQueue = Window->pti->MessageQueue;
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

if (Ret == 0) ASSERT(FALSE);
   return Ret;
}

BOOL FASTCALL
DestroyTimersForWindow(PTHREADINFO pti, PWINDOW_OBJECT Window)
{
   PLIST_ENTRY pLE;
   PTIMER pTmr = FirstpTmr;
   BOOL TimersRemoved = FALSE;

   if ((FirstpTmr == NULL) || (Window == NULL))
      return FALSE;

   TimerEnterExclusive();

   do
   {
      if ((pTmr) && (pTmr->pti == pti) && (pTmr->pWnd == Window))
      {
         TimersRemoved = RemoveTimer(pTmr);
      }
      pLE = pTmr->ptmrList.Flink;
      pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
   } while (pTmr != FirstpTmr);

   TimerLeave();

   return TimersRemoved;
}

BOOL FASTCALL
DestroyTimersForThread(PTHREADINFO pti)
{
   PLIST_ENTRY pLE;
   PTIMER pTmr = FirstpTmr;
   BOOL TimersRemoved = FALSE;

   if (FirstpTmr == NULL)
      return FALSE;

   TimerEnterExclusive();

   do
   {
      if ((pTmr) && (pTmr->pti == pti))
      {
         TimersRemoved = RemoveTimer(pTmr);
      }
      pLE = pTmr->ptmrList.Flink;
      pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
   } while (pTmr != FirstpTmr);

   TimerLeave();

   return TimersRemoved;
}

BOOL FASTCALL
IntKillTimer(PWINDOW_OBJECT Window, UINT_PTR IDEvent, BOOL SystemTimer)
{
   PTIMER pTmr = NULL;
   DPRINT("IntKillTimer Window %x id %p systemtimer %s\n",
          Window, IDEvent, SystemTimer ? "TRUE" : "FALSE");

   if ((Window) && (IDEvent == 0))
      IDEvent = 1;

   pTmr = FindTimer(Window, IDEvent, SystemTimer ? TMRF_SYSTEM : 0, TRUE);
   return pTmr ? TRUE :  FALSE;
}

//
//
// Old Kill Timer
//
//
BOOL FASTCALL
InternalKillTimer(HWND Wnd, UINT_PTR IDEvent, BOOL SystemTimer)
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
         if (Window && !( MsqKillTimer(Window->pti->MessageQueue, Wnd,
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

      HintIndex = IDEvent - 1;

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
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(UINT_PTR);

   DPRINT("Enter NtUserSetTimer\n");
   UserEnterExclusive();
   Window = UserGetWindowObject(hWnd);
   UserLeave();

   RETURN(IntSetTimer(Window, nIDEvent, uElapse, lpTimerFunc, TMRF_TIFROMWND));

CLEANUP:
   DPRINT("Leave NtUserSetTimer, ret=%i\n", _ret_);

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
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserKillTimer\n");
   UserEnterExclusive();
   Window = UserGetWindowObject(hWnd);
   UserLeave();

   RETURN(IntKillTimer(Window, uIDEvent, FALSE));

CLEANUP:
   DPRINT("Leave NtUserKillTimer, ret=%i\n", _ret_);
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

   // This is wrong, lpTimerFunc is NULL!
   RETURN(IntSetTimer(UserGetWindowObject(hWnd), nIDEvent, uElapse, lpTimerFunc, TMRF_SYSTEM));

CLEANUP:
   DPRINT("Leave NtUserSetSystemTimer, ret=%i\n", _ret_);
   END_CLEANUP;
}


/* EOF */
