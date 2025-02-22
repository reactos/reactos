/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window timers messages
 * FILE:             win32ss/user/ntuser/timer.c
 * PROGRAMER:        Gunnar
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                   Michael Martin (michael.martin@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserTimer);

/* GLOBALS *******************************************************************/

static LIST_ENTRY TimersListHead;
static LONG TimeLast = 0;

/* Windows 2000 has room for 32768 window-less timers */
/* These values give timer IDs [256,32767], same as on Windows */
#define MAX_WINDOW_LESS_TIMER_ID  (32768 - 1)
#define NUM_WINDOW_LESS_TIMERS    (32768 - 256)

#define HINTINDEX_BEGIN_VALUE   0

static PFAST_MUTEX    Mutex;
static RTL_BITMAP     WindowLessTimersBitMap;
static PVOID          WindowLessTimersBitMapBuffer;
static ULONG          HintIndex = HINTINDEX_BEGIN_VALUE;

ERESOURCE TimerLock;

#define IntLockWindowlessTimerBitmap() \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(Mutex)

#define IntUnlockWindowlessTimerBitmap() \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(Mutex)

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

  Ret = UserCreateObject(gHandleTable, NULL, NULL, &Handle, TYPE_TIMER, sizeof(TIMER));
  if (Ret)
  {
     UserHMSetHandle(Ret, Handle);
     InsertTailList(&TimersListHead, &Ret->ptmrList);
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
     if ((pTmr->pWnd == NULL) && (!(pTmr->flags & TMRF_SYSTEM))) // System timers are reusable.
     {
        ULONG ulBitmapIndex;

        ASSERT(pTmr->nID <= MAX_WINDOW_LESS_TIMER_ID);
        ulBitmapIndex = (ULONG)(MAX_WINDOW_LESS_TIMER_ID - pTmr->nID);
        IntLockWindowlessTimerBitmap();
        RtlClearBit(&WindowLessTimersBitMap, ulBitmapIndex);
        IntUnlockWindowlessTimerBitmap();
     }
     UserDereferenceObject(pTmr);
     Ret = UserDeleteObject( UserHMGetHandle(pTmr), TYPE_TIMER);
  }
  if (!Ret) ERR("Warning: Unable to delete timer\n");

  return Ret;
}

PTIMER
FASTCALL
FindTimer(PWND Window,
          UINT_PTR nID,
          UINT flags)
{
  PLIST_ENTRY pLE;
  PTIMER pTmr, RetTmr = NULL;

  TimerEnterExclusive();
  pLE = TimersListHead.Flink;
  while (pLE != &TimersListHead)
  {
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);

    if ( pTmr->nID == nID &&
         pTmr->pWnd == Window &&
        (pTmr->flags & (TMRF_SYSTEM|TMRF_RIT)) == (flags & (TMRF_SYSTEM|TMRF_RIT)))
    {
       RetTmr = pTmr;
       break;
    }

    pLE = pLE->Flink;
  }
  TimerLeave();

  return RetTmr;
}

PTIMER
FASTCALL
FindSystemTimer(PMSG pMsg)
{
  PLIST_ENTRY pLE;
  PTIMER pTmr = NULL;

  TimerEnterExclusive();
  pLE = TimersListHead.Flink;
  while (pLE != &TimersListHead)
  {
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);

    if ( pMsg->lParam == (LPARAM)pTmr->pfn &&
         (pTmr->flags & TMRF_SYSTEM) )
       break;

    pLE = pLE->Flink;
  }
  TimerLeave();

  return pTmr;
}

BOOL
FASTCALL
ValidateTimerCallback(PTHREADINFO pti,
                      LPARAM lParam)
{
  PLIST_ENTRY pLE;
  BOOL Ret = FALSE;
  PTIMER pTmr;

  TimerEnterExclusive();
  pLE = TimersListHead.Flink;
  while (pLE != &TimersListHead)
  {
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
    if ( (lParam == (LPARAM)pTmr->pfn) &&
        !(pTmr->flags & (TMRF_SYSTEM|TMRF_RIT)) &&
         (pTmr->pti->ppi == pti->ppi) )
    {
       Ret = TRUE;
       break;
    }
    pLE = pLE->Flink;
  }
  TimerLeave();

  return Ret;
}

UINT_PTR FASTCALL
IntSetTimer( PWND Window,
                  UINT_PTR IDEvent,
                  UINT Elapse,
                  TIMERPROC TimerFunc,
                  INT Type)
{
  PTIMER pTmr;
  UINT_PTR Ret = IDEvent;
  ULONG ulBitmapIndex;
  LARGE_INTEGER DueTime;
  DueTime.QuadPart = (LONGLONG)(-97656); // 1024hz .9765625 ms set to 10.0 ms

#if 0
  /* Windows NT/2k/XP behaviour */
  if (Elapse > USER_TIMER_MAXIMUM)
  {
     TRACE("Adjusting uElapse\n");
     Elapse = 1;
  }
#else
  /* Windows XP SP2 and Windows Server 2003 behaviour */
  if (Elapse > USER_TIMER_MAXIMUM)
  {
     TRACE("Adjusting uElapse\n");
     Elapse = USER_TIMER_MAXIMUM;
  }
#endif

  /* Windows 2k/XP and Windows Server 2003 SP1 behaviour */
  if (Elapse < USER_TIMER_MINIMUM)
  {
     TRACE("Adjusting uElapse\n");
     Elapse = USER_TIMER_MINIMUM; // 1024hz .9765625 ms, set to 10.0 ms (+/-)1 ms
  }

  /* Passing an IDEvent of 0 and the SetTimer returns 1.
     It will create the timer with an ID of 0 */
  if ((Window) && (IDEvent == 0))
     Ret = 1;

  pTmr = FindTimer(Window, IDEvent, Type);

  if ((!pTmr) && (Window == NULL) && (!(Type & TMRF_SYSTEM)))
  {
      IntLockWindowlessTimerBitmap();

      ulBitmapIndex = RtlFindClearBitsAndSet(&WindowLessTimersBitMap, 1, HintIndex);
      HintIndex = (ulBitmapIndex + 1) % NUM_WINDOW_LESS_TIMERS;
      if (ulBitmapIndex == ULONG_MAX)
      {
         IntUnlockWindowlessTimerBitmap();
         ERR("Unable to find a free window-less timer id\n");
         EngSetLastError(ERROR_NO_SYSTEM_RESOURCES);
         return 0;
      }

      ASSERT(ulBitmapIndex < NUM_WINDOW_LESS_TIMERS);
      IDEvent = MAX_WINDOW_LESS_TIMER_ID - ulBitmapIndex;
      Ret = IDEvent;

      IntUnlockWindowlessTimerBitmap();
  }

  if (!pTmr)
  {
     pTmr = CreateTimer();
     if (!pTmr) return 0;

     if (Window && (Type & TMRF_TIFROMWND))
        pTmr->pti = Window->head.pti->pEThread->Tcb.Win32Thread;
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
     pTmr->flags   = Type|TMRF_INIT;
  }
  else
  {
     pTmr->cmsCountdown = Elapse;
     pTmr->cmsRate = Elapse;
  }

  ASSERT(MasterTimer != NULL);
  // Start the timer thread!
  if (TimersListHead.Flink == TimersListHead.Blink) // There is only one timer
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
  PDESKTOP pDesk;
  PWND pWnd = NULL;

  if (hwnd)
  {
     pWnd = UserGetWindowObject(hwnd);
     if (!pWnd)
     {
        ERR("System Timer Proc has invalid window handle! %p Id: %u\n", hwnd, idEvent);
        return;
     }
  }
  else
  {
     TRACE( "Windowless Timer Running!\n" );
     return;
  }

  switch (idEvent)
  {
/*
   Used in NtUserTrackMouseEvent.
 */
     case ID_EVENT_SYSTIMER_MOUSEHOVER:
       {
          POINT Point;
          UINT Msg;
          WPARAM wParam;

          pDesk = pWnd->head.rpdesk;
          if ( pDesk->dwDTFlags & DF_TME_HOVER &&
               pWnd == pDesk->spwndTrack )
          {
             Point = gpsi->ptCursor;
             if ( RECTL_bPointInRect(&pDesk->rcMouseHover, Point.x, Point.y) )
             {
                if (pDesk->htEx == HTCLIENT) // In a client area.
                {
                   wParam = MsqGetDownKeyState(pWnd->head.pti->MessageQueue);
                   Msg = WM_MOUSEHOVER;

                   if (pWnd->ExStyle & WS_EX_LAYOUTRTL)
                   {
                      Point.x = pWnd->rcClient.right - Point.x - 1;
                   }
                   else
                      Point.x -= pWnd->rcClient.left;
                   Point.y -= pWnd->rcClient.top;
                }
                else
                {
                   wParam = pDesk->htEx; // Need to support all HTXYZ hits.
                   Msg = WM_NCMOUSEHOVER;
                }
                TRACE("Generating WM_NCMOUSEHOVER\n");
                UserPostMessage(hwnd, Msg, wParam, MAKELPARAM(Point.x, Point.y));
                pDesk->dwDTFlags &= ~DF_TME_HOVER;
                break; // Kill this timer.
             }
          }
       }
       return; // Not this window so just return.

     case ID_EVENT_SYSTIMER_FLASHWIN:
       {
          FLASHWINFO fwi =
            {sizeof(FLASHWINFO),
             UserHMGetHandle(pWnd),
             FLASHW_SYSTIMER,0,0};

          IntFlashWindowEx(pWnd, &fwi);
       }
       return;

     default:
       ERR("System Timer Proc invalid id %u!\n", idEvent);
       break;
  }
  IntKillTimer(pWnd, idEvent, TRUE);
}

VOID
FASTCALL
StartTheTimers(VOID)
{
  // Need to start gdi syncro timers then start timer with Hang App proc
  // that calles Idle process so the screen savers will know to run......
  IntSetTimer(NULL, 0, 1000, HungAppSysTimerProc, TMRF_RIT);
// Test Timers
//  IntSetTimer(NULL, 0, 1000, SystemTimerProc, TMRF_RIT);
}

UINT_PTR
FASTCALL
SystemTimerSet( PWND Window,
                UINT_PTR nIDEvent,
                UINT uElapse,
                TIMERPROC lpTimerFunc)
{
  if (Window && Window->head.pti->pEThread->ThreadsProcess != PsGetCurrentProcess())
  {
     EngSetLastError(ERROR_ACCESS_DENIED);
     TRACE("SysemTimerSet: Access Denied!\n");
     return 0;
  }
  return IntSetTimer( Window, nIDEvent, uElapse, lpTimerFunc, TMRF_SYSTEM);
}

BOOL
FASTCALL
PostTimerMessages(PWND Window)
{
  PLIST_ENTRY pLE;
  MSG Msg;
  PTHREADINFO pti;
  BOOL Hit = FALSE;
  PTIMER pTmr;

  pti = PsGetCurrentThreadWin32Thread();

  TimerEnterExclusive();
  pLE = TimersListHead.Flink;
  while(pLE != &TimersListHead)
  {
     pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
     if ( (pTmr->flags & TMRF_READY) &&
          (pTmr->pti == pti) &&
          ((pTmr->pWnd == Window) || (Window == NULL)) )
        {
           Msg.hwnd    = (pTmr->pWnd ? UserHMGetHandle(pTmr->pWnd) : NULL);
           Msg.message = (pTmr->flags & TMRF_SYSTEM) ? WM_SYSTIMER : WM_TIMER;
           Msg.wParam  = (WPARAM) pTmr->nID;
           Msg.lParam  = (LPARAM) pTmr->pfn;
           Msg.time    = EngGetTickCount32();
           // Fix all wine win:test_GetMessagePos WM_TIMER tests. See CORE-10867.
           Msg.pt      = gpsi->ptCursor;

           MsqPostMessage(pti, &Msg, FALSE, (QS_POSTMESSAGE|QS_ALLPOSTMESSAGE), 0, 0);
           pTmr->flags &= ~TMRF_READY;
           ClearMsgBitsMask(pti, QS_TIMER);
           Hit = TRUE;
           // Now move this entry to the end of the list so it will not be
           // called again in the next msg loop.
           if (pLE != &TimersListHead)
           {
              RemoveEntryList(&pTmr->ptmrList);
              InsertTailList(&TimersListHead, &pTmr->ptmrList);
           }
           break;
        }

     pLE = pLE->Flink;
  }

  TimerLeave();

  return Hit;
}

VOID
FASTCALL
ProcessTimers(VOID)
{
  LARGE_INTEGER DueTime;
  LONG Time;
  PLIST_ENTRY pLE;
  PTIMER pTmr;
  LONG TimerCount = 0;

  TimerEnterExclusive();
  pLE = TimersListHead.Flink;
  Time = EngGetTickCount32();

  DueTime.QuadPart = (LONGLONG)(-97656); // 1024hz .9765625 ms set to 10.0 ms

  while(pLE != &TimersListHead)
  {
    pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
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
                if (pTmr->pti)
                {  // Wakeup thread
                   pTmr->pti->cTimersReady++;
                   ASSERT(pTmr->pti->pEventQueueServer != NULL);
                   MsqWakeQueue(pTmr->pti, QS_TIMER, TRUE);
                }
             }
          }
          pTmr->cmsCountdown = pTmr->cmsRate;
       }
       else
          pTmr->cmsCountdown -= Time - TimeLast;
    }

    pLE = pLE->Flink;
  }

  // Restart the timer thread!
  ASSERT(MasterTimer != NULL);
  KeSetTimer(MasterTimer, DueTime, NULL);

  TimeLast = Time;

  TimerLeave();
  TRACE("TimerCount = %d\n", TimerCount);
}

BOOL FASTCALL
DestroyTimersForWindow(PTHREADINFO pti, PWND Window)
{
   PLIST_ENTRY pLE;
   PTIMER pTmr;
   BOOL TimersRemoved = FALSE;

   if (Window == NULL)
      return FALSE;

   TimerEnterExclusive();
   pLE = TimersListHead.Flink;
   while(pLE != &TimersListHead)
   {
      pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
      pLE = pLE->Flink; /* get next timer list entry before current timer is removed */
      if ((pTmr) && (pTmr->pti == pti) && (pTmr->pWnd == Window))
      {
         TimersRemoved = RemoveTimer(pTmr);
      }
   }

   TimerLeave();

   return TimersRemoved;
}

BOOL FASTCALL
DestroyTimersForThread(PTHREADINFO pti)
{
   PLIST_ENTRY pLE = TimersListHead.Flink;
   PTIMER pTmr;
   BOOL TimersRemoved = FALSE;

   TimerEnterExclusive();

   while(pLE != &TimersListHead)
   {
      pTmr = CONTAINING_RECORD(pLE, TIMER, ptmrList);
      pLE = pLE->Flink; /* get next timer list entry before current timer is removed */
      if ((pTmr) && (pTmr->pti == pti))
      {
         TimersRemoved = RemoveTimer(pTmr);
      }
   }

   TimerLeave();

   return TimersRemoved;
}

BOOL FASTCALL
IntKillTimer(PWND Window, UINT_PTR IDEvent, BOOL SystemTimer)
{
   PTIMER pTmr = NULL;
   TRACE("IntKillTimer Window %p id %uI systemtimer %s\n",
         Window, IDEvent, SystemTimer ? "TRUE" : "FALSE");

   TimerEnterExclusive();
   pTmr = FindTimer(Window, IDEvent, SystemTimer ? TMRF_SYSTEM : 0);

   if (pTmr)
   {
      RemoveTimer(pTmr);
   }
   TimerLeave();

   return pTmr ? TRUE :  FALSE;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitTimerImpl(VOID)
{
   ULONG BitmapBytes;

   /* Allocate FAST_MUTEX from non paged pool */
   Mutex = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), TAG_INTERNAL_SYNC);
   if (!Mutex)
   {
       return STATUS_INSUFFICIENT_RESOURCES;
   }

   ExInitializeFastMutex(Mutex);

   BitmapBytes = ALIGN_UP_BY(NUM_WINDOW_LESS_TIMERS, sizeof(ULONG) * 8) / 8;
   WindowLessTimersBitMapBuffer = ExAllocatePoolWithTag(NonPagedPool, BitmapBytes, TAG_TIMERBMP);
   if (WindowLessTimersBitMapBuffer == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   RtlInitializeBitMap(&WindowLessTimersBitMap,
                       WindowLessTimersBitMapBuffer,
                       NUM_WINDOW_LESS_TIMERS);

   /* Yes we need this, since ExAllocatePoolWithTag isn't supposed to zero out allocated memory */
   RtlClearAllBits(&WindowLessTimersBitMap);

   ExInitializeResourceLite(&TimerLock);
   InitializeListHead(&TimersListHead);

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
   PWND Window = NULL;
   UINT_PTR ret;

   TRACE("Enter NtUserSetTimer\n");
   UserEnterExclusive();
   if (hWnd) Window = UserGetWindowObject(hWnd);

   ret = IntSetTimer(Window, nIDEvent, uElapse, lpTimerFunc, TMRF_TIFROMWND);

   UserLeave();
   TRACE("Leave NtUserSetTimer, ret=%u\n", ret);

   return ret;
}


BOOL
APIENTRY
NtUserKillTimer
(
   HWND hWnd,
   UINT_PTR uIDEvent
)
{
   PWND Window = NULL;
   BOOL ret;

   TRACE("Enter NtUserKillTimer\n");
   UserEnterExclusive();
   if (hWnd) Window = UserGetWindowObject(hWnd);

   ret = IntKillTimer(Window, uIDEvent, FALSE);

   UserLeave();

   TRACE("Leave NtUserKillTimer, ret=%i\n", ret);
   return ret;
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
    UINT_PTR ret;

    UserEnterExclusive();
    TRACE("Enter NtUserSetSystemTimer\n");

    ret = IntSetTimer(UserGetWindowObject(hWnd), nIDEvent, uElapse, NULL, TMRF_SYSTEM);

    UserLeave();

    TRACE("Leave NtUserSetSystemTimer, ret=%u\n", ret);
    return ret;
}

BOOL
APIENTRY
NtUserValidateTimerCallback(
    LPARAM lParam)
{
  BOOL Ret = FALSE;

  UserEnterShared();

  Ret = ValidateTimerCallback(PsGetCurrentThreadWin32Thread(), lParam);

  UserLeave();
  return Ret;
}

/* EOF */
