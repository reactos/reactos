/*++
Copyright (c) Microsoft. All rights reserved.

Module Name:

    SelfManagedIoStateMachine.cpp

Abstract:

    This module implements the self managed io state machine start / stop logic
    in the framework.

Author:



Environment:
    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "SelfManagedIoStateMachine.tmh"
#endif
}

// * - We can get a restart from the created state if a PDO is newly enumerated
//     but was disabled on a previous enumeration.  Treat restart as an init
//     in this case.
const FxSelfManagedIoTargetState FxSelfManagedIoMachine::m_CreatedStates[] =
{
    { SelfManagedIoEventStart, FxSelfManagedIoInit DEBUGGED_EVENT },
    { SelfManagedIoEventFlush, FxSelfManagedIoCreated DEBUGGED_EVENT },
    { SelfManagedIoEventCleanup, FxSelfManagedIoFinal DEBUGGED_EVENT },
};

const FxSelfManagedIoTargetState FxSelfManagedIoMachine::m_InitFailedStates[] =
{
    { SelfManagedIoEventSuspend, FxSelfManagedIoInitFailed DEBUGGED_EVENT },
    { SelfManagedIoEventFlush, FxSelfManagedIoFlushing DEBUGGED_EVENT },
};

const FxSelfManagedIoTargetState FxSelfManagedIoMachine::m_StartedStates[] =
{
    { SelfManagedIoEventSuspend, FxSelfManagedIoSuspending DEBUGGED_EVENT },
};

const FxSelfManagedIoTargetState FxSelfManagedIoMachine::m_StoppedStates[] =
{
    { SelfManagedIoEventStart, FxSelfManagedIoRestarting DEBUGGED_EVENT },
    { SelfManagedIoEventSuspend, FxSelfManagedIoStopped DEBUGGED_EVENT },
    { SelfManagedIoEventFlush, FxSelfManagedIoFlushing DEBUGGED_EVENT },
};

const FxSelfManagedIoTargetState FxSelfManagedIoMachine::m_FailedStates[] =
{
    { SelfManagedIoEventFlush, FxSelfManagedIoFlushing DEBUGGED_EVENT },
    { SelfManagedIoEventSuspend, FxSelfManagedIoFailed DEBUGGED_EVENT },
};

const FxSelfManagedIoTargetState FxSelfManagedIoMachine::m_FlushedStates[] =
{
    { SelfManagedIoEventStart, FxSelfManagedIoRestarting DEBUGGED_EVENT },
    { SelfManagedIoEventCleanup, FxSelfManagedIoCleanup DEBUGGED_EVENT },
    { SelfManagedIoEventFlush, FxSelfManagedIoFlushed TRAP_ON_EVENT },
};

const FxSelfManagedIoStateTable FxSelfManagedIoMachine::m_StateTable[] =
{
    // FxSelfManagedIoCreated
    {   NULL,
        FxSelfManagedIoMachine::m_CreatedStates,
        ARRAY_SIZE(FxSelfManagedIoMachine::m_CreatedStates),
    },

    // FxSelfManagedIoInit
    {   FxSelfManagedIoMachine::Init,
        NULL,
        0,
    },

    // FxSelfManagedIoInitFailed
    {   NULL,
        FxSelfManagedIoMachine::m_InitFailedStates,
        ARRAY_SIZE(FxSelfManagedIoMachine::m_InitFailedStates),
    },

    // FxSelfManagedIoStarted
    {   NULL,
        FxSelfManagedIoMachine::m_StartedStates,
        ARRAY_SIZE(FxSelfManagedIoMachine::m_StartedStates),
    },

    // FxSelfManagedIoSuspending
    {   FxSelfManagedIoMachine::Suspending,
        NULL,
        0,
    },

    // FxSelfManagedIoStopped
    {   NULL,
        FxSelfManagedIoMachine::m_StoppedStates,
        ARRAY_SIZE(FxSelfManagedIoMachine::m_StoppedStates),
    },

    // FxSelfManagedIoRestarting
    {   FxSelfManagedIoMachine::Restarting,
        NULL,
        0,
    },

    // FxSelfManagedIoFailed
    {   NULL,
        FxSelfManagedIoMachine::m_FailedStates,
        ARRAY_SIZE(FxSelfManagedIoMachine::m_FailedStates),
    },

    // FxSelfManagedIoFlushing
    {   FxSelfManagedIoMachine::Flushing,
        NULL,
        0,
    },

    // FxSelfManagedIoFlushed
    {   NULL,
        FxSelfManagedIoMachine::m_FlushedStates,
        ARRAY_SIZE(FxSelfManagedIoMachine::m_FlushedStates),
    },

    // FxSelfManagedIoCleanup
    {   FxSelfManagedIoMachine::Cleanup,
        NULL,
        0,
    },

    // FxSelfManagedIoFinal
    {   NULL,
        NULL,
        0,
    },
};

FxSelfManagedIoMachine::FxSelfManagedIoMachine(
    __in FxPkgPnp* PkgPnp
    )
{
    m_PkgPnp = PkgPnp;

    m_EventHistoryIndex = 0;
    m_StateHistoryIndex = 0;

    m_CurrentState = FxSelfManagedIoCreated;

    RtlZeroMemory(&m_Events, sizeof(m_Events));
    RtlZeroMemory(&m_States, sizeof(m_States));

    //
    // Make sure we can fit the state into a byte
    //
    ASSERT(FxSelfManagedIoMax <= 0xFF);
}

NTSTATUS
FxSelfManagedIoMachine::_CreateAndInit(
    __deref_out FxSelfManagedIoMachine** SelfManagedIoMachine,
    __in FxPkgPnp* PkgPnp
    )
{
    NTSTATUS status;
    FxSelfManagedIoMachine * selfManagedIoMachine;

    *SelfManagedIoMachine = NULL;

    selfManagedIoMachine = new (PkgPnp->GetDriverGlobals()) FxSelfManagedIoMachine(
        PkgPnp
        );

    if (selfManagedIoMachine == NULL) {
        DoTraceLevelMessage(
            PkgPnp->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Self managed I/O state machine allocation failed for "
            "WDFDEVICE 0x%p",
            PkgPnp->GetDevice()->GetHandle());

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = selfManagedIoMachine->m_StateMachineLock.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            PkgPnp->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Self managed I/O state machine lock initialization failed for "
            "WDFDEVICE 0x%p, %!STATUS!",
            PkgPnp->GetDevice()->GetHandle(),
            status);

        delete selfManagedIoMachine;

        return status;
    }

    *SelfManagedIoMachine = selfManagedIoMachine;

    return status;
}


VOID
FxSelfManagedIoMachine::InitializeMachine(
    __in PWDF_PNPPOWER_EVENT_CALLBACKS Callbacks
    )
/*++

Routine Description:
    Sets all the function event callbacks.

Arguments:
    Callbacks - list of callbacks to set

Return Value:
    None

  --*/
{
    m_DeviceSelfManagedIoCleanup.m_Method = Callbacks->EvtDeviceSelfManagedIoCleanup;
    m_DeviceSelfManagedIoFlush.m_Method = Callbacks->EvtDeviceSelfManagedIoFlush;
    m_DeviceSelfManagedIoInit.m_Method    = Callbacks->EvtDeviceSelfManagedIoInit;
    m_DeviceSelfManagedIoSuspend.m_Method = Callbacks->EvtDeviceSelfManagedIoSuspend;
    m_DeviceSelfManagedIoRestart.m_Method = Callbacks->EvtDeviceSelfManagedIoRestart;
}

WDFDEVICE
FxSelfManagedIoMachine::GetDeviceHandle(
    VOID
    )
{
    return m_PkgPnp->GetDevice()->GetHandle();
}

_Must_inspect_result_
NTSTATUS
FxSelfManagedIoMachine::ProcessEvent(
    __in FxSelfManagedIoEvents Event
    )
/*++

Routine Description:
    Processes an event and runs it through the state machine.  Unlike other
    state machines in the framework, this one acquires lock in the event
    processing function rather then relying on the caller to do so.

Arguments:
    Event - The event to feed into the state machine.

Return Value:
    result of the event

  --*/
{
    const FxSelfManagedIoStateTable* entry;
    FxSelfManagedIoStates newState;
    NTSTATUS status;

    m_StateMachineLock.AcquireLock(m_PkgPnp->GetDriverGlobals());

    m_Events.History[m_EventHistoryIndex] = (UCHAR) Event;
    m_EventHistoryIndex = (m_EventHistoryIndex + 1) %
                          (sizeof(m_Events.History)/sizeof(m_Events.History[0]));

    entry = &m_StateTable[m_CurrentState-FxSelfManagedIoCreated];
    newState = FxSelfManagedIoMax;

    for (ULONG i = 0; i < entry->TargetStatesCount; i++) {
        if (entry->TargetStates[i].SelfManagedIoEvent == Event) {
            DO_EVENT_TRAP(&entry->TargetStates[i]);
            newState = entry->TargetStates[i].SelfManagedIoState;
            break;
        }
    }

    if (newState == FxSelfManagedIoMax) {
        //
        // We always can handle io increment/decrement from any state, but we
        // should not be dropping any other events from this state.
        //

        COVERAGE_TRAP();
    }

    status = STATUS_SUCCESS;

    while (newState != FxSelfManagedIoMax) {
        DoTraceLevelMessage(
            m_PkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p entering self managed io state "
            "%!FxSelfManagedIoStates! from %!FxSelfManagedIoStates!",
            m_PkgPnp->GetDevice()->GetHandle(),
            m_PkgPnp->GetDevice()->GetDeviceObject(),
            newState, m_CurrentState);

        m_States.History[m_StateHistoryIndex] = (UCHAR) newState;
        m_StateHistoryIndex = (m_StateHistoryIndex + 1) %
                              (sizeof(m_States.History)/sizeof(m_States.History[0]));

        m_CurrentState = (BYTE) newState;
        entry = &m_StateTable[m_CurrentState-FxSelfManagedIoCreated];

        if (entry->StateFunc != NULL) {
            newState = entry->StateFunc(this, &status);
        }
        else {
            newState = FxSelfManagedIoMax;
        }
    }

    m_StateMachineLock.ReleaseLock(m_PkgPnp->GetDriverGlobals());

    return status;
}

FxSelfManagedIoStates
FxSelfManagedIoMachine::Init(
    __in  FxSelfManagedIoMachine* This,
    __out PNTSTATUS Status
    )
/*++

Routine Description:
    Calls the event callback for initializing self managed io.

Arguments:
    This - instance of the state machine

    Status - result of the event callback into the driver

Return Value:
    new machine state

  --*/
{
    *Status = This->m_DeviceSelfManagedIoInit.Invoke(This->GetDeviceHandle());

    if (NT_SUCCESS(*Status))  {
        return FxSelfManagedIoStarted;
    }
    else {
        return FxSelfManagedIoInitFailed;
    }
}

FxSelfManagedIoStates
FxSelfManagedIoMachine::Suspending(
    __in  FxSelfManagedIoMachine* This,
    __out PNTSTATUS Status
    )
/*++

Routine Description:
    Invokes the self managed io suspend event callback.  Upon failure goes to
    the failed state and awaits teardown of the stack.

Arguments:
    This - instance of the state machine

    Status - result of the event callback into the driver

Return Value:
    new machine state

  --*/
{
    *Status = This->m_DeviceSelfManagedIoSuspend.Invoke(This->GetDeviceHandle());

    if (NT_SUCCESS(*Status)) {
        return FxSelfManagedIoStopped;
    }
    else {
        return FxSelfManagedIoFailed;
    }
}

FxSelfManagedIoStates
FxSelfManagedIoMachine::Restarting(
    __in  FxSelfManagedIoMachine* This,
    __out PNTSTATUS Status
    )
/*++

Routine Description:
    Invokes the self managed io event callback for restarting self managed io
    from the stopped state.

Arguments:
    This - instance of the state machine

    Status - result of the event callback into the driver

Return Value:
    new machine state

  --*/
{
    *Status = This->m_DeviceSelfManagedIoRestart.Invoke(This->GetDeviceHandle());

    if (NT_SUCCESS(*Status)) {
        return FxSelfManagedIoStarted;
    }
    else {
        return FxSelfManagedIoFailed;
    }
}

FxSelfManagedIoStates
FxSelfManagedIoMachine::Flushing(
    __in  FxSelfManagedIoMachine* This,
    __out PNTSTATUS Status
    )
/*++

Routine Description:
    Calls the self managed io flush routine.

Arguments:
    This - instance of the state machine

    Status - result of the event callback into the driver

Return Value:
    FxSelfManagedIoFlushed

  --*/
{
    UNREFERENCED_PARAMETER(Status);
    This->m_DeviceSelfManagedIoFlush.Invoke(This->GetDeviceHandle());
    return FxSelfManagedIoFlushed;
}

FxSelfManagedIoStates
FxSelfManagedIoMachine::Cleanup(
    __in  FxSelfManagedIoMachine* This,
    __out PNTSTATUS Status
    )
/*++

Routine Description:
    Calls the self managed io cleanup routine.

Arguments:
    This - instance of the state machine

    Status - result of the event callback into the driver

Return Value:
    FxSelfManagedIoFinal

  --*/
{
    UNREFERENCED_PARAMETER(Status);

    This->m_DeviceSelfManagedIoCleanup.Invoke(This->GetDeviceHandle());
    return FxSelfManagedIoFinal;
}
