/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/time.c
 * PURPOSE:     Time related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   Vizzini 08-Oct-2003  Formatting, commenting, and ASSERTs
 *
 * NOTES:
 *     - Although the standard kernel-mode M.O. is to trust the caller
 *       to not provide bad arguments, we have added lots of argument
 *       validation to assist in the effort to get third-party binaries
 *       working.  It is easiest to track bugs when things break quickly
 *       and badly.
 */

#include "ndissys.h"

/*
 * @implemented
 */
VOID
EXPORT
NdisCancelTimer(
    IN  PNDIS_TIMER Timer,
    OUT PBOOLEAN    TimerCancelled)
/*
 * FUNCTION: Cancels a scheduled NDIS timer
 * ARGUMENTS:
 *     Timer: pointer to an NDIS_TIMER object to cancel
 *     TimerCancelled: boolean that returns cancellation status
 * NOTES:
 *     - call at IRQL <= DISPATCH_LEVEL
 */
{
  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(Timer);

  *TimerCancelled = KeCancelTimer (&Timer->Timer);
}

/*
 * @implemented
 */
#undef NdisGetCurrentSystemTime
VOID
EXPORT
NdisGetCurrentSystemTime (
    IN  OUT PLARGE_INTEGER   pSystemTime)
/*
 * FUNCTION: Retrieve the current system time
 * ARGUMENTS:
 *     pSystemTime: pointer to the returned system time
 * NOTES:
 *     - call at any IRQL
 */
{
  ASSERT(pSystemTime);

  KeQuerySystemTime (pSystemTime);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeTimer(
    IN OUT  PNDIS_TIMER             Timer,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
/*
 * FUNCTION: Set up an NDIS_TIMER for later use
 * ARGUMENTS:
 *     Timer: pointer to caller-allocated storage to receive an NDIS_TIMER
 *     TimerFunction: function pointer to routine to run when timer expires
 *     FunctionContext: context (param 2) to be passed to the timer function when it runs
 * NOTES:
 *     - TimerFunction will be called at DISPATCH_LEVEL
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 */
{
  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(Timer);

  KeInitializeTimer (&Timer->Timer);

  KeInitializeDpc (&Timer->Dpc, (PKDEFERRED_ROUTINE)TimerFunction, FunctionContext);
}

BOOLEAN DequeueMiniportTimer(PNDIS_MINIPORT_TIMER Timer)
{
  PNDIS_MINIPORT_TIMER CurrentTimer;

  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

  if (!Timer->Miniport->TimerQueue)
      return FALSE;

  if (Timer->Miniport->TimerQueue == Timer)
  {
      Timer->Miniport->TimerQueue = Timer->NextDeferredTimer;
      Timer->NextDeferredTimer = NULL;
      return TRUE;
  }
  else
  {
      CurrentTimer = Timer->Miniport->TimerQueue;
      while (CurrentTimer->NextDeferredTimer)
      {
          if (CurrentTimer->NextDeferredTimer == Timer)
          {
              CurrentTimer->NextDeferredTimer = Timer->NextDeferredTimer;
              Timer->NextDeferredTimer = NULL;
              return TRUE;
          }
          CurrentTimer = CurrentTimer->NextDeferredTimer;
      }
      return FALSE;
  }
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMCancelTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    OUT PBOOLEAN                TimerCancelled)
/*
 * FUNCTION: cancel a scheduled NDIS_MINIPORT_TIMER
 * ARGUMENTS:
 *     Timer: timer object to cancel
 *     TimerCancelled: status of cancel operation
 * NOTES:
 *     - call at IRQL <= DISPATCH_LEVEL
 */
{
  //KIRQL OldIrql;

  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(TimerCancelled);
  ASSERT(Timer);

  *TimerCancelled = KeCancelTimer (&Timer->Timer);

#if 0
  if (*TimerCancelled)
  {
      KeAcquireSpinLock(&Timer->Miniport->Lock, &OldIrql);
      /* If it's somebody already dequeued it, something is wrong (maybe a double-cancel?) */
      if (!DequeueMiniportTimer(Timer)) ASSERT(FALSE);
      KeReleaseSpinLock(&Timer->Miniport->Lock, OldIrql);
  }
#endif
}

VOID NTAPI
MiniTimerDpcFunction(PKDPC Dpc,
                     PVOID DeferredContext,
                     PVOID SystemArgument1,
                     PVOID SystemArgument2)
{
  PNDIS_MINIPORT_TIMER Timer = DeferredContext;

#if 0
  /* Only dequeue if the timer has a period of 0 */
  if (!Timer->Timer.Period)
  {
      KeAcquireSpinLockAtDpcLevel(&Timer->Miniport->Lock);
      /* If someone already dequeued it, something is wrong (borked timer implementation?) */
      if (!DequeueMiniportTimer(Timer)) ASSERT(FALSE);
      KeReleaseSpinLockFromDpcLevel(&Timer->Miniport->Lock);
  }
#endif

  Timer->MiniportTimerFunction(Dpc,
                               Timer->MiniportTimerContext,
                               SystemArgument1,
                               SystemArgument2);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMInitializeTimer(
    IN OUT  PNDIS_MINIPORT_TIMER    Timer,
    IN      NDIS_HANDLE             MiniportAdapterHandle,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
/*
 * FUNCTION: Initialize an NDIS_MINIPORT_TIMER
 * ARGUMENTS:
 *     Timer: Timer object to initialize
 *     MiniportAdapterHandle: Handle to the miniport, passed in to MiniportInitialize
 *     TimerFunction: function to be executed when the timer expires
 *     FunctionContext: argument passed to TimerFunction when it is called
 * NOTES:
 *     - TimerFunction is called at IRQL = DISPATCH_LEVEL
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 */
{
  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(Timer);

  KeInitializeTimer (&Timer->Timer);
  KeInitializeDpc (&Timer->Dpc, MiniTimerDpcFunction, Timer);

  Timer->MiniportTimerFunction = TimerFunction;
  Timer->MiniportTimerContext = FunctionContext;
  Timer->Miniport = &((PLOGICAL_ADAPTER)MiniportAdapterHandle)->NdisMiniportBlock;
  Timer->NextDeferredTimer = NULL;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMSetPeriodicTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsPeriod)
/*
 * FUNCTION: Set a timer to go off periodically
 * ARGUMENTS:
 *     Timer: pointer to the timer object to set
 *     MillisecondsPeriod: period of the timer
 * NOTES:
 *     - Minimum predictable interval is ~10ms
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 */
{
  LARGE_INTEGER Timeout;
  //KIRQL OldIrql;

  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(Timer);

  /* relative delays are negative, absolute are positive; resolution is 100ns */
  Timeout.QuadPart = Int32x32To64(MillisecondsPeriod, -10000);

#if 0
  /* Lock the miniport block */
  KeAcquireSpinLock(&Timer->Miniport->Lock, &OldIrql);

  /* Attempt to dequeue the timer */
  DequeueMiniportTimer(Timer);

  /* Add the timer at the head of the timer queue */
  Timer->NextDeferredTimer = Timer->Miniport->TimerQueue;
  Timer->Miniport->TimerQueue = Timer;

  /* Unlock the miniport block */
  KeReleaseSpinLock(&Timer->Miniport->Lock, OldIrql);
#endif

  KeSetTimerEx(&Timer->Timer, Timeout, MillisecondsPeriod, &Timer->Dpc);
}

/*
 * @implemented
 */
#undef NdisMSetTimer
VOID
EXPORT
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsToDelay)
/*
 * FUNCTION: Set a NDIS_MINIPORT_TIMER so that it goes off
 * ARGUMENTS:
 *     Timer: timer object to set
 *     MillisecondsToDelay: time to wait for the timer to expire
 * NOTES:
 *     - Minimum predictable interval is ~10ms
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 */
{
  LARGE_INTEGER Timeout;
  //KIRQL OldIrql;

  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(Timer);

  /* relative delays are negative, absolute are positive; resolution is 100ns */
  Timeout.QuadPart = Int32x32To64(MillisecondsToDelay, -10000);

#if 0
  /* Lock the miniport block */
  KeAcquireSpinLock(&Timer->Miniport->Lock, &OldIrql);

  /* Attempt to dequeue the timer */
  DequeueMiniportTimer(Timer);

  /* Add the timer at the head of the timer queue */
  Timer->NextDeferredTimer = Timer->Miniport->TimerQueue;
  Timer->Miniport->TimerQueue = Timer;

  /* Unlock the miniport block */
  KeReleaseSpinLock(&Timer->Miniport->Lock, OldIrql);
#endif

  KeSetTimer(&Timer->Timer, Timeout, &Timer->Dpc);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisSetTimer(
    IN  PNDIS_TIMER Timer,
    IN  UINT        MillisecondsToDelay)
/*
 * FUNCTION: Set an NDIS_TIMER so that it goes off
 * ARGUMENTS:
 *     Timer: timer object to set
 *     MillisecondsToDelay: time to wait for the timer to expire
 * NOTES:
 *     - Minimum predictable interval is ~10ms
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 */
{
  LARGE_INTEGER Timeout;

  ASSERT_IRQL(DISPATCH_LEVEL);
  ASSERT(Timer);

  NDIS_DbgPrint(MAX_TRACE, ("Called. Timer is: 0x%x, Timeout is: %ld\n", Timer, MillisecondsToDelay));

  /* relative delays are negative, absolute are positive; resolution is 100ns */
  Timeout.QuadPart = Int32x32To64(MillisecondsToDelay, -10000);

  KeSetTimer (&Timer->Timer, Timeout, &Timer->Dpc);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisSetTimerEx(
    IN PNDIS_TIMER  Timer,
    IN UINT  MillisecondsToDelay,
    IN PVOID  FunctionContext)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called. Timer is: 0x%x, Timeout is: %ld, FunctionContext is: 0x%x\n",
                               Timer, MillisecondsToDelay, FunctionContext));

    Timer->Dpc.DeferredContext = FunctionContext;

    NdisSetTimer(Timer, MillisecondsToDelay);
}

/* EOF */
