/*++
Copyright (c) Microsoft. All rights reserved.

Module Name:

    WakeInterrupt.cpp

Abstract:

    This module implements the wake interrupt logic in the framework.

--*/

#include "pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "WakeInterruptStateMachine.tmh"
#endif
}

const FxWakeInterruptTargetState
FxWakeInterruptMachine::m_FailedStates[] =
{
    {WakeInterruptEventIsr, WakeInterruptFailed DEBUGGED_EVENT}
};

const FxWakeInterruptTargetState
FxWakeInterruptMachine::m_D0States[] =
{
    {WakeInterruptEventIsr, WakeInterruptInvokingEvtIsrInD0 DEBUGGED_EVENT},
    {WakeInterruptEventLeavingD0, WakeInterruptDx DEBUGGED_EVENT},
    {WakeInterruptEventLeavingD0NotArmedForWake, WakeInterruptDxNotArmedForWake DEBUGGED_EVENT}
};

const FxWakeInterruptTargetState
FxWakeInterruptMachine::m_DxStates[] =
{
    {WakeInterruptEventEnteringD0, WakeInterruptCompletingD0 DEBUGGED_EVENT},
    {WakeInterruptEventIsr, WakeInterruptWaking DEBUGGED_EVENT},
    {WakeInterruptEventD0EntryFailed, WakeInterruptFailed DEBUGGED_EVENT}
};

const FxWakeInterruptTargetState
FxWakeInterruptMachine::m_DxNotArmedForWakeStates[] =
{
    { WakeInterruptEventEnteringD0, WakeInterruptCompletingD0 DEBUGGED_EVENT },
    { WakeInterruptEventIsr, WakeInterruptInvokingEvtIsrInDxNotArmedForWake DEBUGGED_EVENT },
    { WakeInterruptEventD0EntryFailed, WakeInterruptFailed DEBUGGED_EVENT }
};

const FxWakeInterruptTargetState
FxWakeInterruptMachine::m_WakingStates[] =
{
    {WakeInterruptEventEnteringD0, WakeInterruptInvokingEvtIsrPostWake DEBUGGED_EVENT},
    {WakeInterruptEventD0EntryFailed, WakeInterruptFailed DEBUGGED_EVENT}
};


const FxWakeInterruptStateTable
FxWakeInterruptMachine::m_StateTable[] =
{
    // WakeInterruptFailed
    {   FxWakeInterruptMachine::Failed,
        FxWakeInterruptMachine::m_FailedStates,
        ARRAY_SIZE(FxWakeInterruptMachine::m_FailedStates),
    },

    // WakeInterruptD0
    {   NULL,
        FxWakeInterruptMachine::m_D0States,
        ARRAY_SIZE(FxWakeInterruptMachine::m_D0States),
    },

    // WakeInterruptDx
    {   FxWakeInterruptMachine::Dx,
        FxWakeInterruptMachine::m_DxStates,
        ARRAY_SIZE(FxWakeInterruptMachine::m_DxStates),
    },

    // WakeInterruptWaking
    {   FxWakeInterruptMachine::Waking,
        FxWakeInterruptMachine::m_WakingStates,
        ARRAY_SIZE(FxWakeInterruptMachine::m_WakingStates),
    },

    // WakeInterruptInvokingEvtIsrPostWakeStates
    {   FxWakeInterruptMachine::InvokingEvtIsrPostWake,
        NULL,
        0,
    },

    // WakeInterruptCompletingD0States
    {   FxWakeInterruptMachine::CompletingD0,
        NULL,
        0,
    },

    // WakeInterruptInvokingEvtIsrInD0
    {   FxWakeInterruptMachine::InvokingEvtIsrInD0,
        NULL,
        0,
    },

    // WakeInterruptDxNotArmedForWake
    {   FxWakeInterruptMachine::DxNotArmedForWake,
        FxWakeInterruptMachine::m_DxNotArmedForWakeStates,
        ARRAY_SIZE(FxWakeInterruptMachine::m_DxNotArmedForWakeStates),
    },

    // WakeInterruptInvokingEvtIsrInDxNotArmedForWake
    { FxWakeInterruptMachine::InvokingEvtIsrInDxNotArmedForWake,
      NULL,
      0,
    },
};

FxWakeInterruptMachine::FxWakeInterruptMachine(
    __in FxInterrupt * Interrupt
    ) : FxThreadedEventQueue(FxWakeInterruptEventQueueDepth)
{
    //
    // Make sure we can fit the state into a byte
    //
    C_ASSERT(WakeInterruptMax <= 0xFF);

    m_CurrentState = WakeInterruptD0;

    RtlZeroMemory(&m_Queue, sizeof(m_Queue));
    RtlZeroMemory(&m_States, sizeof(m_States));

    //
    // Store the initial state in the state history array
    //
    m_States.History[IncrementHistoryIndex()] = m_CurrentState;
    m_Interrupt = Interrupt;
}

VOID
FxWakeInterruptMachine::ProcessEvent(
    __in FxWakeInterruptEvents Event
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

        ASSERTMSG("The wake interrupt state machine queue is full\n",
                  FALSE);
        return;
    }

    if (IsClosedLocked()) {
        //
        // The queue is closed. This should never happen.
        //
        DoTraceLevelMessage(
          m_PkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
          "WDFDEVICE 0x%p !devobj 0x%p current wake interrupt state"
          " %!FxWakeInterruptStates! dropping event "
          "%!FxWakeInterruptEvents! because of a closed queue",
          m_PkgPnp->GetDevice()->GetHandle(),
          m_PkgPnp->GetDevice()->GetDeviceObject(),
          m_CurrentState,
          Event);

        Unlock(irql);

        ASSERTMSG(
            "The wake interrupt state machine queue is closed\n",
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
                    m_PkgPnp->GetDriverGlobals(),
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
                m_PkgPnp->GetDriverGlobals()
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
FxWakeInterruptMachine::_ProcessEventInner(
    __inout FxPkgPnp* PkgPnp,
    __inout FxPostProcessInfo* Info,
    __in PVOID WorkerContext
    )
{

    UNREFERENCED_PARAMETER(PkgPnp);

    FxWakeInterruptMachine * pThis = (FxWakeInterruptMachine *) WorkerContext;

    //
    // Take the state machine lock.
    //
    pThis->m_StateMachineLock.AcquireLock(
                pThis->m_PkgPnp->GetDriverGlobals()
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
                pThis->m_PkgPnp->GetDriverGlobals()
                );

    return;
}

VOID
FxWakeInterruptMachine::ProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
{
    KIRQL irql;
    FxWakeInterruptEvents event;
    const FxWakeInterruptStateTable* entry;
    FxWakeInterruptStates newState;

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
        // WakeInterruptMax), but that should never happen because WakeInterruptMax is not a real
        // state. We just use it to represent the maximum value in the enum that
        // defines the states.
        //
        __analysis_assume(m_CurrentState < WakeInterruptMax);
        entry = &m_StateTable[m_CurrentState - WakeInterruptFailed];

        //
        // Based on the event received, figure out the next state
        //
        newState = WakeInterruptMax;
        for (ULONG i = 0; i < entry->TargetStatesCount; i++) {
            if (entry->TargetStates[i].WakeInterruptEvent == event) {
                DO_EVENT_TRAP(&entry->TargetStates[i]);
                newState = entry->TargetStates[i].WakeInterruptState;
                break;
            }
        }

        if (newState == WakeInterruptMax) {
            //
            // Unexpected event for this state
            //
            DoTraceLevelMessage(
                m_PkgPnp->GetDriverGlobals(),
                TRACE_LEVEL_INFORMATION,
                TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p wake interrupt state "
                "%!FxWakeInterruptStates! dropping event "
                "%!FxWakeInterruptEvents!",
                m_PkgPnp->GetDevice()->GetHandle(),
                m_PkgPnp->GetDevice()->GetDeviceObject(),
                m_CurrentState,
                event
                );

            COVERAGE_TRAP();
        }

        while (newState != WakeInterruptMax) {
            DoTraceLevelMessage(
                m_PkgPnp->GetDriverGlobals(),
                TRACE_LEVEL_INFORMATION,
                TRACINGPNPPOWERSTATES,
                "WDFDEVICE 0x%p !devobj 0x%p entering wake interrupt "
                "state %!FxWakeInterruptStates! from "
                "%!FxWakeInterruptStates!",
                m_PkgPnp->GetDevice()->GetHandle(),
                m_PkgPnp->GetDevice()->GetDeviceObject(),
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
            entry = &m_StateTable[m_CurrentState-WakeInterruptFailed];

            //
            // Invoke the state entry function (if present) for the new state
            //
            if (entry->StateFunc != NULL) {
                newState = entry->StateFunc(this);
            }
            else {
                newState = WakeInterruptMax;
            }
        }
    }

    return;
}


FxWakeInterruptStates
FxWakeInterruptMachine::Waking(
    __in FxWakeInterruptMachine* This
    )
{
    This->m_PkgPnp->PowerPolicyProcessEvent(PwrPolWakeInterruptFired);

    return WakeInterruptMax;
}

FxWakeInterruptStates
FxWakeInterruptMachine::Dx(
    __in FxWakeInterruptMachine* This
    )
{
    //
    // Flush queued callbacks so that we know that nobody is still trying to
    // synchronize against this interrupt. For KMDF this will flush DPCs and
    // for UMDF this will send a message to reflector to flush queued DPCs.
    //
    This->m_Interrupt->FlushQueuedDpcs();

#if FX_IS_KERNEL_MODE
    //
    // Rundown the workitem if present (passive-level interrupt support or KMDF).
    // Not needed for UMDF since reflector doesn't use workitem for isr.
    //
    This->m_Interrupt->FlushQueuedWorkitem();

#endif

    This->m_PkgPnp->AckPendingWakeInterruptOperation(FALSE);

    return WakeInterruptMax;
}

FxWakeInterruptStates
FxWakeInterruptMachine::DxNotArmedForWake(
    __in FxWakeInterruptMachine* This
    )
{
    //
    // Ask power state machine to process the acknowledgement event
    // on a different thread as we could be running the state machine's
    // engine in the context of a wake ISR, and the power
    // state machine will attempt to disconnect this interrupt when
    // it processes the acknowledgement event.
    //
    This->m_PkgPnp->AckPendingWakeInterruptOperation(TRUE);

    return WakeInterruptMax;
}


FxWakeInterruptStates
FxWakeInterruptMachine::InvokingEvtIsrInDxNotArmedForWake(
    __in FxWakeInterruptMachine* This
    )
{
    This->m_Interrupt->InvokeWakeInterruptEvtIsr();

    This->m_IsrEvent.Set();

    return WakeInterruptDxNotArmedForWake;
}

FxWakeInterruptStates
FxWakeInterruptMachine::InvokingEvtIsrPostWake(
    __in FxWakeInterruptMachine* This
    )
{
    This->m_Interrupt->InvokeWakeInterruptEvtIsr();

    This->m_IsrEvent.Set();

    return WakeInterruptCompletingD0;
}

FxWakeInterruptStates
FxWakeInterruptMachine::CompletingD0(
    __in FxWakeInterruptMachine* This
    )
{
    This->m_PkgPnp->AckPendingWakeInterruptOperation(FALSE);

    return WakeInterruptD0;
}

FxWakeInterruptStates
FxWakeInterruptMachine::InvokingEvtIsrInD0(
    __in FxWakeInterruptMachine* This
    )
{
    This->m_Interrupt->InvokeWakeInterruptEvtIsr();

    This->m_IsrEvent.Set();

    return WakeInterruptD0;
}

FxWakeInterruptStates
FxWakeInterruptMachine::Failed(
    __in FxWakeInterruptMachine* This
    )
{
    //
    // Device failed to power up and we are not invoking the
    // client driver's callback. So we cannot claim the
    // interrupt
    //
    This->m_Claimed = FALSE;

    This->m_IsrEvent.Set();

    return WakeInterruptMax;
}


