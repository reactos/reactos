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
STDCALL
NdisCancelTimer(
    IN  PNDIS_TIMER Timer,
    OUT PBOOLEAN    TimerCancelled)
{
} 

#undef NdisGetCurrentSystemTime

VOID
STDCALL
NdisGetCurrentSystemTime (
    IN  OUT PLONGLONG   pSystemTime)
{
}


VOID
STDCALL
NdisInitializeTimer(
    IN OUT  PNDIS_TIMER             Timer,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
{
}


VOID
STDCALL
NdisMCancelTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    OUT PBOOLEAN                TimerCancelled)
{
}


VOID
STDCALL
NdisMInitializeTimer(
    IN OUT  PNDIS_MINIPORT_TIMER    Timer,
    IN      NDIS_HANDLE             MiniportAdapterHandle,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
{
}


VOID
STDCALL
NdisMSetPeriodicTimer(
    IN	PNDIS_MINIPORT_TIMER    Timer,
    IN	UINT                    MillisecondsPeriod)
{
}


VOID
STDCALL
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsToDelay)
{
}


VOID
STDCALL
NdisSetTimer(
    IN  PNDIS_TIMER Timer,
    IN  UINT        MillisecondsToDelay)
{
}

/* EOF */
