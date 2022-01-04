/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxTimerKm.h

Abstract:

    Kernel mode implementation of timer defined in
    MxTimer.h

Author:


Revision History:



--*/

#pragma once

#define TolerableDelayUnlimited ((ULONG)-1)

typedef
BOOLEAN
(STDCALL *PFN_KE_SET_COALESCABLE_TIMER) (
    __inout PKTIMER Timer,
    __in LARGE_INTEGER DueTime,
    __in ULONG Period,
    __in ULONG TolerableDelay,
    __in_opt PKDPC Dpc
    );

typedef struct _MdTimer {


    //
    // The timer period
    //
    LONG m_Period;

    //
    // Tracks whether the ex timer is being used
    //
    BOOLEAN m_IsExtTimer;

// #pragma warning(push)
// #pragma warning( disable: 4201 ) // nonstandard extension used : nameless struct/union

    union {
        struct {
            //
            // Callback function to be invoked upon timer expiration
            //
            MdDeferredRoutine m_TimerCallback;
            KTIMER KernelTimer;
            KDPC TimerDpc;
        };

        struct {
            //
            // Callback function to be invoked upon timer expiration
            //
            MdExtCallback m_ExTimerCallback;
            PEX_TIMER m_KernelExTimer;
        };
    };

// #pragma warning(pop)

    //
    // Context to be passed in to the callback function
    //
    PVOID m_TimerContext;

} MdTimer;

#include "mxtimer.h"

MxTimer::MxTimer(
    VOID
    )
{
    m_Timer.m_TimerContext = NULL;
    m_Timer.m_TimerCallback = NULL;
    m_Timer.m_Period = 0;
    m_Timer.m_KernelExTimer = NULL;
}

MxTimer::~MxTimer(
    VOID
    )
{
    // __REACTOS__ Ex timers are not supported
    // BOOLEAN wasCancelled;

    // if (m_Timer.m_IsExtTimer &&
    //     m_Timer.m_KernelExTimer) {
    //     wasCancelled = ExDeleteTimer(m_Timer.m_KernelExTimer,
    //                                  TRUE, // Cancel if pending. We don't expect
    //                                        // it to be pending though
    //                                  FALSE,// Wait
    //                                  NULL);
    //     //
    //     // Timer should not have been pending
    //     //
    //     ASSERT(wasCancelled == FALSE);
    //     m_Timer.m_KernelExTimer = NULL;
    // }
}

NTSTATUS
#ifdef _MSC_VER
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "_Must_inspect_result_ not needed in kernel mode as the function always succeeds");
#endif
MxTimer::Initialize(
    __in_opt PVOID TimerContext,
    __in MdDeferredRoutine TimerCallback,
    __in LONG Period
    )
{
    m_Timer.m_TimerContext = TimerContext;
    m_Timer.m_TimerCallback = TimerCallback;
    m_Timer.m_Period = Period;

    KeInitializeTimerEx(&(m_Timer.KernelTimer), NotificationTimer);
    KeInitializeDpc(&(m_Timer.TimerDpc), // Timer DPC
                    m_Timer.m_TimerCallback, // DeferredRoutine
                    m_Timer.m_TimerContext); // DeferredContext

    m_Timer.m_IsExtTimer = FALSE;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
MxTimer::InitializeEx(
    __in_opt PVOID TimerContext,
    __in MdExtCallback TimerCallback,
    __in LONG Period,
    __in ULONG TolerableDelay,
    __in BOOLEAN UseHighResolutionTimer
    )
/*++

Routine Description:

    Initializes an Ex timer. By invoking this routine instead of
    Initialize, the client is implicitly declaring that it wants
    to use the new timers

Arguments:

    TimerContext -          Context to be passed back with the cllback
    TimerCallback -         Callback to be invoked when the timer fires
    Period -                Period in ms
    TolerableDelay -        Tolerable delay in ms
    UseHighResolutionTimer- Indicates whether to use the high
                            resolution timers

Returns:

    Status

--*/

{
    // NTSTATUS status;
    // ULONG attributes = 0;

    // m_Timer.m_TimerContext = TimerContext;
    // m_Timer.m_ExTimerCallback = TimerCallback;
    // m_Timer.m_Period = Period;

    // if (TolerableDelay != 0) {

    //     attributes |= EX_TIMER_NO_WAKE;

    // } else if (UseHighResolutionTimer) {

    //     attributes |= EX_TIMER_HIGH_RESOLUTION;
    // }

    // m_Timer.m_KernelExTimer = ExAllocateTimer(m_Timer.m_ExTimerCallback,
    //                                           TimerContext,
    //                                           attributes);
    // if (m_Timer.m_KernelExTimer) {

    //     status = STATUS_SUCCESS;

    // } else {

    //     status = STATUS_INSUFFICIENT_RESOURCES;
    // }

    // m_Timer.m_IsExtTimer = TRUE;

    // return status;
    return STATUS_NOT_IMPLEMENTED; // __REACTOS__ Ex timers are not supported
}


__inline
BOOLEAN
MxTimer::StartWithReturn(
    __in LARGE_INTEGER DueTime,
    __in ULONG TolerableDelay
    )
{
    if (m_Timer.m_IsExtTimer) {
        // __REACTOS__ Ex timers are not supported
        // EXT_SET_PARAMETERS parameters;

        // ExInitializeSetTimerParameters(&parameters);

        // //
        // // We get the delay in ms but the underlying API needs it in 100 ns
        // // units. Convert tolerable delay from ms to 100 ns. However,
        // // MAXULONG (TolerableDelayUnlimited) has a special meaning that the
        // // system should never be woken up, so we assign the corresponding
        // // special value for Ex timers
        // //
        // if (TolerableDelay == TolerableDelayUnlimited) {
        //     parameters.NoWakeTolerance = EX_TIMER_UNLIMITED_TOLERANCE;
        // } else {
        //     parameters.NoWakeTolerance = ((LONGLONG) TolerableDelay) * 10 * 1000;
        // }

        // return ExSetTimer(m_Timer.m_KernelExTimer,
        //                   DueTime.QuadPart,
        //                   (((LONGLONG) m_Timer.m_Period) * 10 * 1000),
        //                   &parameters);
        return FALSE;
    } else {

        return KeSetCoalescableTimer(&(m_Timer.KernelTimer),
                                     DueTime,
                                     m_Timer.m_Period,
                                     TolerableDelay,
                                     &(m_Timer.TimerDpc));
    }

}


VOID
MxTimer::Start(
    __in LARGE_INTEGER DueTime,
    __in ULONG TolerableDelay
    )
{
    if (m_Timer.m_IsExtTimer) {

        StartWithReturn(DueTime,TolerableDelay);

    } else {
        KeSetCoalescableTimer(&(m_Timer.KernelTimer),
                              DueTime,
                              m_Timer.m_Period,
                              TolerableDelay,
                              &(m_Timer.TimerDpc));
    }

    return;
}

_Must_inspect_result_
BOOLEAN
MxTimer::Stop(
    VOID
    )
{
    BOOLEAN bRetVal;

    if (m_Timer.m_IsExtTimer) {
        bRetVal = FALSE;
        // bRetVal = ExCancelTimer(m_Timer.m_KernelExTimer, NULL); // __REACTOS__ Ex timers are not supported

    } else {
        bRetVal = KeCancelTimer(&(m_Timer.KernelTimer));
    }

    return bRetVal;
}

VOID
MxTimer::FlushQueuedDpcs(
    VOID
    )
{
    Mx::MxFlushQueuedDpcs();
}
