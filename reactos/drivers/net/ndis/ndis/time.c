/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/time.c
 * PURPOSE:     Time related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


VOID STDCALL
MiniportTimerDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2)
{
    PNDIS_MINIPORT_TIMER Timer;

    Timer = (PNDIS_MINIPORT_TIMER)DeferredContext;

    Timer->MiniportTimerFunction (NULL, Timer->MiniportTimerContext, NULL, NULL);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisCancelTimer(
    IN  PNDIS_TIMER Timer,
    OUT PBOOLEAN    TimerCancelled)
{
    *TimerCancelled = KeCancelTimer (&Timer->Timer);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisGetCurrentSystemTime (
    IN  OUT PLONGLONG   pSystemTime)
{
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
{
    KeInitializeTimer (&Timer->Timer);

    KeInitializeDpc (&Timer->Dpc, TimerFunction, FunctionContext);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMCancelTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    OUT PBOOLEAN                TimerCancelled)
{
    *TimerCancelled = KeCancelTimer (&Timer->Timer);
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
{
    KeInitializeTimer (&Timer->Timer);

    KeInitializeDpc (&Timer->Dpc, MiniportTimerDpc, (PVOID) Timer);

    Timer->MiniportTimerFunction = TimerFunction;
    Timer->MiniportTimerContext = FunctionContext;
    Timer->Miniport = MiniportAdapterHandle;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetPeriodicTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsPeriod)
{
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = MillisecondsPeriod * -10000;

    KeSetTimerEx (&Timer->Timer, Timeout, MillisecondsPeriod, &Timer->Dpc);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsToDelay)
{
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = MillisecondsToDelay * -10000;

    KeSetTimer (&Timer->Timer, Timeout, &Timer->Dpc);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisSetTimer(
    IN  PNDIS_TIMER Timer,
    IN  UINT        MillisecondsToDelay)
{
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = MillisecondsToDelay * -10000;

    KeSetTimer (&Timer->Timer, Timeout, &Timer->Dpc);
}

/* EOF */
