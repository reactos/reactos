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


VOID
EXPORT
NdisCancelTimer(
    IN  PNDIS_TIMER Timer,
    OUT PBOOLEAN    TimerCancelled)
{
} 


VOID
EXPORT
NdisGetCurrentSystemTime (
    IN  OUT PLONGLONG   pSystemTime)
{
}


VOID
EXPORT
NdisInitializeTimer(
    IN OUT  PNDIS_TIMER             Timer,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
{
}


VOID
EXPORT
NdisMCancelTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    OUT PBOOLEAN                TimerCancelled)
{
}


VOID
EXPORT
NdisMInitializeTimer(
    IN OUT  PNDIS_MINIPORT_TIMER    Timer,
    IN      NDIS_HANDLE             MiniportAdapterHandle,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
{
}


VOID
EXPORT
NdisMSetPeriodicTimer(
    IN	PNDIS_MINIPORT_TIMER    Timer,
    IN	UINT                    MillisecondsPeriod)
{
}


VOID
EXPORT
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsToDelay)
{
}


VOID
EXPORT
NdisSetTimer(
    IN  PNDIS_TIMER Timer,
    IN  UINT        MillisecondsToDelay)
{
}

/* EOF */
