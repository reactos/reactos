/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxTimer.h

Abstract:

    Mode agnostic definiton of timer

    See MxTimerKm.h and MxTimerUm.h for
    mode specific implementations

Author:


Revision History:



--*/

#pragma once

class MxTimer
{
private:
    //
    // Handle to the timer object
    //
    MdTimer m_Timer;

public:

    __inline
    MxTimer(
        VOID
        );

    __inline
    ~MxTimer(
        VOID
        );

    CHECK_RETURN_IF_USER_MODE
    __inline
    NTSTATUS
    Initialize(
        __in_opt PVOID TimerContext,
        __in MdDeferredRoutine TimerCallback,
        __in LONG Period
        );

    CHECK_RETURN_IF_USER_MODE
    __inline
    NTSTATUS
    InitializeEx(
        __in_opt PVOID TimerContext,
        __in MdExtCallbackType TimerCallback,
        __in LONG Period,
        __in ULONG TolerableDelay,
        __in BOOLEAN UseHighResolutionTimer
        );

    __inline
    VOID
    Start(
        __in LARGE_INTEGER DueTime,
        __in ULONG TolerableDelay = 0
        );

    __inline
    BOOLEAN
    StartWithReturn(
        __in LARGE_INTEGER DueTime,
        __in ULONG TolerableDelay = 0
        );

    _Must_inspect_result_
    __inline
    BOOLEAN
    Stop(
        VOID
        );

    __inline
    VOID
    FlushQueuedDpcs(
        VOID
        );
};


