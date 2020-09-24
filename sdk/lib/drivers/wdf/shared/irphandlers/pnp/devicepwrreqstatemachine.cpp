/*++
Copyright (c) Microsoft. All rights reserved.

Module Name:

    DevicePwrReq.cpp

Abstract:

    This module implements the device power requirement logic in the framework.

--*/

#include "pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "DevicePwrReqStateMachine.tmh"
#endif
}

const FxDevicePwrRequirementTargetState
FxDevicePwrRequirementMachine::m_UnregisteredStates[] =
{
    {DprEventRegisteredWithPox, DprDevicePowerRequiredD0 DEBUGGED_EVENT}
};

const FxDevicePwrRequirementTargetState
FxDevicePwrRequirementMachine::m_DevicePowerRequiredD0States[] =
{
    {DprEventPoxDoesNotRequirePower, DprDevicePowerNotRequiredD0 DEBUGGED_EVENT},
    {DprEventUnregisteredWithPox, DprUnregistered DEBUGGED_EVENT},
    {DprEventDeviceReturnedToD0, DprDevicePowerRequiredD0 DEBUGGED_EVENT}
};

const FxDevicePwrRequirementTargetState
FxDevicePwrRequirementMachine::m_DevicePowerNotRequiredD0States[] =
{
    {DprEventDeviceGoingToDx, DprDevicePowerNotRequiredDx DEBUGGED_EVENT},
    {DprEventPoxRequiresPower, DprReportingDevicePowerAvailable DEBUGGED_EVENT},
    {DprEventUnregisteredWithPox, DprUnregistered TRAP_ON_EVENT}
};

const FxDevicePwrRequirementTargetState
FxDevicePwrRequirementMachine::m_DevicePowerNotRequiredDxStates[] =
{
    {DprEventDeviceReturnedToD0, DprWaitingForDevicePowerRequiredD0 DEBUGGED_EVENT},
    {DprEventPoxRequiresPower, DprDevicePowerRequiredDx DEBUGGED_EVENT}
};

const FxDevicePwrRequirementTargetState
FxDevicePwrRequirementMachine::m_DevicePowerRequiredDxStates[] =
{
    {DprEventDeviceReturnedToD0, DprReportingDevicePowerAvailable DEBUGGED_EVENT}
};

const FxDevicePwrRequirementTargetState
FxDevicePwrRequirementMachine::m_WaitingForDevicePowerRequiredD0States[] =
{
    {DprEventPoxRequiresPower, DprReportingDevicePowerAvailable DEBUGGED_EVENT},
    {DprEventDeviceReturnedToD0, DprWaitingForDevicePowerRequiredD0 TRAP_ON_EVENT},
    {DprEventUnregisteredWithPox, DprUnregistered DEBUGGED_EVENT},
};

const FxDevicePwrRequirementStateTable
FxDevicePwrRequirementMachine::m_StateTable[] =
{
    // DprUnregistered
    {   NULL,
        FxDevicePwrRequirementMachine::m_UnregisteredStates,
        ARRAY_SIZE(FxDevicePwrRequirementMachine::m_UnregisteredStates),
    },

    // DprDevicePowerRequiredD0
    {   NULL,
        FxDevicePwrRequirementMachine::m_DevicePowerRequiredD0States,
        ARRAY_SIZE(FxDevicePwrRequirementMachine::m_DevicePowerRequiredD0States),
    },

    // DprDevicePowerNotRequiredD0
    {   FxDevicePwrRequirementMachine::PowerNotRequiredD0,
        FxDevicePwrRequirementMachine::m_DevicePowerNotRequiredD0States,
        ARRAY_SIZE(FxDevicePwrRequirementMachine::m_DevicePowerNotRequiredD0States),
    },

    // DprDevicePowerNotRequiredDx
    {   NULL,
        FxDevicePwrRequirementMachine::m_DevicePowerNotRequiredDxStates,
        ARRAY_SIZE(FxDevicePwrRequirementMachine::m_DevicePowerNotRequiredDxStates),
    },

    // DprDevicePowerRequiredDx
    {   FxDevicePwrRequirementMachine::PowerRequiredDx,
        FxDevicePwrRequirementMachine::m_DevicePowerRequiredDxStates,
        ARRAY_SIZE(FxDevicePwrRequirementMachine::m_DevicePowerRequiredDxStates),
    },

    // DprReportingDevicePowerAvailable
    {   FxDevicePwrRequirementMachine::ReportingDevicePowerAvailable,
        NULL,
        0,
    },

    // DprWaitingForDevicePowerRequiredD0
    {   NULL,
        FxDevicePwrRequirementMachine::m_WaitingForDevicePowerRequiredD0States,
        ARRAY_SIZE(FxDevicePwrRequirementMachine::m_WaitingForDevicePowerRequiredD0States),
    },
};

FxDevicePwrRequirementMachine::FxDevicePwrRequirementMachine(
    __in FxPoxInterface * PoxInterface
    ) : FxThreadedEventQueue(FxDevicePwrRequirementEventQueueDepth)
{
    //
    // Make sure we can fit the state into a byte
    //
    C_ASSERT(DprMax <= 0xFF);

    m_CurrentState = DprUnregistered;

    RtlZeroMemory(&m_Queue, sizeof(m_Queue));
    RtlZeroMemory(&m_States, sizeof(m_States));

    //
    // Store the initial state in the state history array
    //
    m_States.History[IncrementHistoryIndex()] = m_CurrentState;
    m_PoxInterface = PoxInterface;
}

VOID
FxDevicePwrRequirementMachine::ProcessEvent(
    __in FxDevicePwrRequirementEvents Event
    )
{
    NTSTATUS status;
    KIRQL irql;
    LONGLONG timeout = 0;

    //
    // Acquire state machine *queue* lock, raising to DISPATCH_LEVEL
    //
    Lock(&irql);

    if (IsFull()) {
        //
        // The queue is full. This should never happen.
        //
        Unlock(irql);

        ASSERTMSG("The device power requirement state machine queue is full\n",
                  FALSE);
        return;
    }

    if (IsClosedLocked()) {
        //
        // The queue is closed. This should never happen.
        //
        DoTraceLevelMessage(
          m_PkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
          "WDFDEVICE 0x%p !devobj 0x%p current device power requirement state"
          " %!FxDevicePwrRequirementStates! dropping event "
          "%!FxDevicePwrRequirementEvents! because of a closed queue",
          m_PoxInterface->m_PkgPnp->GetDevice()->GetHandle(),
          m_PoxInterface->m_PkgPnp->GetDevice()->GetDeviceObject(),
          m_CurrentState,
          Event);

        Unlock(irql);

        ASSERTMSG(
            "The device power requirement state machine queue is closed\n",
            FALSE
            );
        return;
    }

    //
    // Enqueue the event
    //
    m_Queue[InsertAtTail()] = Event;

    //
    // Drop the state machine *queue* lock
    //
    Unlock(irql);

    //
    // Now, if we are running at PASSIVE_LEVEL, attempt to run the state machine
    // on this thread. If we can't do that, then queue a work item.
    //
    if (irql == PASSIVE_LEVEL) {
        //
        // Try to acquire the state machine lock
        //
        status = m_StateMachineLock.AcquireLock(
                    m_PoxInterface->m_PkgPnp->GetDriverGlobals(),
                    &timeout
                    );
        if (FxWaitLockInternal::IsLockAcquired(status)) {
            FxPostProcessInfo info;

            //
            // We now hold the state machine lock.  So call the function that
            // dispatches the next state.
            //
            ProcessEventInner(&info);

            //
            // The pnp state machine should be the only one deleting the object
            //
            ASSERT(info.m_DeleteObject == FALSE);

            //
            // Release the state machine lock
            //
            m_StateMachineLock.ReleaseLock(
                m_PoxInterface->m_PkgPnp->GetDriverGlobals()
                );

            info.Evaluate(m_PkgPnp);

            return;
        }
    }

    //
    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.
    //
    QueueToThread();
    return;
}

VOID
FxDevicePwrRequirementMachine::_ProcessEventInner(
    __inout FxPkgPnp* PkgPnp,
    __inout FxPostProcessInfo* Info,
    __in PVOID WorkerContext
    )
{
    FxDevicePwrRequirementMachine * pThis = NULL;

    UNREFERENCED_PARAMETER(WorkerContext);

    pThis = PkgPnp->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.m_DevicePowerRequirementMachine;

    //
    // Take the state machine lock.
    //
    pThis->m_StateMachineLock.AcquireLock(
                pThis->m_PoxInterface->m_PkgPnp->GetDriverGlobals()
                );

    //
    // Call the function that will actually run the state machine.
    //
    pThis->ProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the state machine lock
    //
    pThis->m_StateMachineLock.ReleaseLock(
                pThis->m_PoxInterface->m_PkgPnp->GetDriverGlobals()
                );

    return;
}

VOID
FxDevicePwrRequirementMachine::ProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
{
    KIRQL irql;
    FxDevicePwrRequirementEvents event;
    const FxDevicePwrRequirementStateTable* entry;
    FxDevicePwrRequirementStates newState;

    //
    // Process as many events as we can
    //
    for ( ; ; ) {
        //
        // Acquire state machine *queue* lock
        //
        Lock(&irql);

        if (IsEmpty()) {
            //
            // The queue is empty.
            //
            GetFinishedState(Info);
            Unlock(irql);
            return;
        }

        //
        // Get the event from the queue
        //
        event = m_Queue[GetHead()];
        IncrementHead();

        //
        // Drop the state machine *queue* lock
        //
        Unlock(irql);

        //
        // Get the state table entry for the current state
        //
        // NOTE: Prefast complains about buffer overflow if (m_CurrentState ==
        // DprMax), but that should never happen because DprMax is not a real
        // state. We just use it to represent the maximum value in the enum that
        // defines the states.
        //
        __analysis_assume(m_CurrentState < DprMax);
        entry = &m_StateTable[m_CurrentState - DprUnregistered];

        //
        // Based on the event received, figure out the next state
        //
        newState = DprMax;
        for (ULONG i = 0; i < entry->TargetStatesCount; i++) {
            if (entry->TargetStates[i].DprEvent == event) {
                DO_EVENT_TRAP(&entry->TargetStates[i]);
                newState = entry->TargetStates[i].DprState;
                break;
            }
        }

        if (newState == DprMax) {
            //
            // Unexpected event for this state
            //
            DoTraceLevelMessage(
                m_PoxInterface->PkgPnp()->GetDriverGlobals(),
                TRACE_LEVEL_INFORMATION,
                TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p device power requirement state "
                "%!FxDevicePwrRequirementStates! dropping event "
                "%!FxDevicePwrRequirementEvents!",
                m_PoxInterface->PkgPnp()->GetDevice()->GetHandle(),
                m_PoxInterface->PkgPnp()->GetDevice()->GetDeviceObject(),
                m_CurrentState,
                event
                );

            COVERAGE_TRAP();
        }

        while (newState != DprMax) {
            DoTraceLevelMessage(
                m_PoxInterface->PkgPnp()->GetDriverGlobals(),
                TRACE_LEVEL_INFORMATION,
                TRACINGPNPPOWERSTATES,
                "WDFDEVICE 0x%p !devobj 0x%p entering device power requirement "
                "state %!FxDevicePwrRequirementStates! from "
                "%!FxDevicePwrRequirementStates!",
                m_PoxInterface->PkgPnp()->GetDevice()->GetHandle(),
                m_PoxInterface->PkgPnp()->GetDevice()->GetDeviceObject(),
                newState,
                m_CurrentState
                );

            //
            // Update the state history array
            //
            m_States.History[IncrementHistoryIndex()] = (UCHAR) newState;

            //
            // Move to the new state
            //
            m_CurrentState = (BYTE) newState;
            entry = &m_StateTable[m_CurrentState-DprUnregistered];

            //
            // Invoke the state entry function (if present) for the new state
            //
            if (entry->StateFunc != NULL) {
                newState = entry->StateFunc(this);
            }
            else {
                newState = DprMax;
            }
        }
    }

    return;
}

FxDevicePwrRequirementStates
FxDevicePwrRequirementMachine::PowerNotRequiredD0(
    __in FxDevicePwrRequirementMachine* This
    )
{
    This->m_PoxInterface->PkgPnp()->PowerPolicyProcessEvent(
                                        PwrPolDevicePowerNotRequired
                                        );
    return DprMax;
}

FxDevicePwrRequirementStates
FxDevicePwrRequirementMachine::PowerRequiredDx(
    __in FxDevicePwrRequirementMachine* This
    )
{
    This->m_PoxInterface->PkgPnp()->PowerPolicyProcessEvent(
                                        PwrPolDevicePowerRequired
                                        );
    return DprMax;
}

FxDevicePwrRequirementStates
FxDevicePwrRequirementMachine::ReportingDevicePowerAvailable(
    __in FxDevicePwrRequirementMachine* This
    )
{
    This->m_PoxInterface->PoxReportDevicePoweredOn();
    return DprDevicePowerRequiredD0;
}
