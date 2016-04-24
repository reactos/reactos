/*
 * PROJECT:         ReactOS TDI driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tdi/cte/timer.c
 * PURPOSE:         CTE timer support
 * PROGRAMMERS:     Oleg Baikalow (obaikalow@gmail.com)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

/* FIXME: Move to a common header! */
struct _CTE_DELAYED_EVENT;
typedef void (*CTE_WORKER_ROUTINE)(struct _CTE_DELAYED_EVENT *, void *Context);

typedef struct _CTE_TIMER
{
    BOOLEAN Queued;
    KSPIN_LOCK Lock;
    CTE_WORKER_ROUTINE Callback;
    PVOID Context;
    KDPC Dpc;
    KTIMER Timer;
} CTE_TIMER, *PCTE_TIMER;

LONG CteTimeIncrement;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
InternalDpcRoutine(PKDPC Dpc,
                   PVOID Context,
                   PVOID SystemArgument1,
                   PVOID SystemArgument2)
{
    PCTE_TIMER Timer = (PCTE_TIMER)Context;

    /* Call our registered callback */
    Timer->Callback((struct _CTE_DELAYED_EVENT *)Timer, Timer->Context);
}

/*
 * @implemented
 */
VOID
NTAPI
CTEInitTimer(PCTE_TIMER Timer)
{
    /* Zero all fields */
    RtlZeroMemory(Timer, sizeof(CTE_TIMER));

    /* Create a DPC and a timer */
    KeInitializeDpc(&Timer->Dpc, InternalDpcRoutine, Timer);
    KeInitializeTimer(&Timer->Timer);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
CTEStartTimer(PCTE_TIMER Timer,
              ULONG DueTimeShort,
              CTE_WORKER_ROUTINE Callback,
              PVOID Context)
{
    LARGE_INTEGER DueTime;

    /* Make sure a callback was provided */
    ASSERT(Callback);

    /* We need to convert due time, because DueTimeShort is in ms,
       but NT timer expects 100s of ns, negative one */
    DueTime.QuadPart = -Int32x32To64(DueTimeShort, 10000);

    /* Set other timer members */
    Timer->Callback = Callback;
    Timer->Context = Context;

    /* Set the timer */
    KeSetTimer(&Timer->Timer, DueTime, &Timer->Dpc);

    return TRUE;
}


/*
 * @implemented
 */
ULONG
NTAPI
CTESystemUpTime(VOID)
{
    LARGE_INTEGER Ticks;

    /* Get the tick count */
    KeQueryTickCount(&Ticks);

    /* Convert to 100s of ns and then to ms*/
    Ticks.QuadPart = (Ticks.QuadPart * CteTimeIncrement) / 10000ULL;

    return Ticks.LowPart;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
CTEInitialize(VOID)
{
    /* Just return success */
    return TRUE;
}

/* EOF */
