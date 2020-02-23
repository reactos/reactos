#include "common/fxpowerstatemachine.h"
#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxwatchdog.h"




#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_POWER_STATE(_CurrentState, _NewState)                        \
        ValidatePowerStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_POWER_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY


const POWER_STATE_TABLE FxPkgPnp::m_WdfPowerStates[] =
{
    NULL
    // TODO: Fill this array
};


_Must_inspect_result_
NTSTATUS
FxPowerMachine::Init(
    __inout FxPkgPnp* Pnp,
    __in PFN_PNP_EVENT_WORKER WorkerRoutine
    )
{
    NTSTATUS status;
    
    status = FxThreadedEventQueue::Init(Pnp, WorkerRoutine);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
FxPkgPnp::PowerProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
/*++

Routine Description:
    This routine runs the state machine.  It implements steps 10-15 of the
    algorithm described above.

--*/
{
    WDF_DEVICE_POWER_STATE currentPowerState, newState;
    CPPOWER_STATE_TABLE entry;
    FxPowerEvent event;
    KIRQL       oldIrql;

    //
    // Process as many events as we can.
    //
    for (;;)
    {
        newState = WdfDevStatePowerNull;
        currentPowerState = m_Device->GetDevicePowerState();
        entry = GetPowerTableEntry(currentPowerState);

        //
        // Get an event from the queue.
        //
        m_PowerMachine.Lock(&oldIrql);

        if (m_PowerMachine.IsEmpty())
        {
            m_PowerMachine.GetFinishedState(Info);

            //
            // The queue is empty.
            //
            m_PowerMachine.Unlock(oldIrql);
            return;
        }

        event = (FxPowerEvent) m_PowerMachine.m_Queue.Events[m_PowerMachine.GetHead()];

        //
        // At this point, we need to determine whether we can process this
        // event.
        //
        if (event & PowerPriorityEventsMask)
        {
            //
            // These are always possible to handle.
            //
            DO_NOTHING();
        }
        else
        {
            //
            // Check to see if this state can handle new events.
            //
            if (entry->StateInfo.Bits.QueueOpen == FALSE)
            {
                //
                // This state can't handle new events.
                //
                m_PowerMachine.Unlock(oldIrql);
                return;
            }
        }

        //
        // If the event obtained from the queue was a singular event, then
        // clear the flag to allow other similar events to be put into this
        // queue for processing.
        //
        if (m_PowerMachine.m_SingularEventsPresent & event)
        {
           m_PowerMachine.m_SingularEventsPresent &= ~event;
        }

        m_PowerMachine.IncrementHead();
        m_PowerMachine.Unlock(oldIrql);

        //
        // Find the entry in the power state table that corresponds to this event
        //
        if (entry->FirstTargetState.PowerEvent == event)
        {
            newState = entry->FirstTargetState.TargetState;

            DO_EVENT_TRAP(&entry->FirstTargetState);
        }
        else if (entry->OtherTargetStates != NULL)
        {
            ULONG i = 0;

            for (i = 0;
                 entry->OtherTargetStates[i].PowerEvent != PowerEventMaximum;
                 i++)
            {
                if (entry->OtherTargetStates[i].PowerEvent == event)
                {
                    newState = entry->OtherTargetStates[i].TargetState;
                    DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                    break;
                }
            }
        }

        if (newState == WdfDevStatePowerNull)
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current power state "
                "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent!",
                m_Device->GetHandle(), 
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePowerState(), event);

            //
            // This state doesn't respond to the Event.  Potentially throw
            // the event away.
            //
            if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0)
            {
                COVERAGE_TRAP();

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                    "WDFDEVICE %p !devobj 0x%p current state "
                    "%!WDF_DEVICE_POWER_STATE! event %!FxPowerEvent! is not a "
                    "known dropped event, known dropped events are "
                    "%!FxPowerEvent!", m_Device->GetHandle(),
                    m_Device->GetDeviceObject(), 
                    m_Device->GetDevicePowerState(),
                    event, entry->StateInfo.Bits.KnownDroppedEvents);
            }

            switch (event) {
            case PowerWakeSucceeded:
            case PowerWakeFailed:
            case PowerWakeCanceled:
                //
                // There are states where we don't care if the wake completed.
                // Since the completion/cancellation of the wake request posts
                // an event which it assumes will complete the request, we must
                // catch these events here and complete the request.
                //
                PowerCompletePendedWakeIrp();
                break;

            case PowerD0:
            case PowerDx:
                //
                // There are some (non WDF) power policy owner implementations
                // which send Dx to Dx or D0 to D0 transitions to the stack.
                //
                // We don't explicitly handle them in the state machine.
                //
                // Instead, we complete the pended irp if are about to drop it
                // on the floor.
                //
                PowerReleasePendingDeviceIrp();
                break;
            }
        }
        else
        {
            //
            // Now enter the new state.
            //
            PowerEnterNewState(newState);
        }
    }
}

VOID
FxPkgPnp::PowerEnterNewState(
    __in WDF_DEVICE_POWER_STATE State
    )
/*++

Routine Description:
    This function looks up the handler for a state and
    then calls it.

Arguments:
    Event - Current PnP event

Return Value:

    NTSTATUS

--*/
{
    CPPOWER_STATE_TABLE    entry;
    WDF_DEVICE_POWER_STATE currentState, newState;
    WDF_DEVICE_POWER_NOTIFICATION_DATA data;
    FxWatchdog watchdog(this);

    currentState = m_Device->GetDevicePowerState();
    newState = State;

    while (newState != WdfDevStatePowerNull)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering Power State "
            "%!WDF_DEVICE_POWER_STATE! from %!WDF_DEVICE_POWER_STATE!",
            m_Device->GetHandle(), 
            m_Device->GetDeviceObject(),
            newState, currentState);

        if (m_PowerStateCallbacks != NULL)
        {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PowerStateCallbacks->Invoke(currentState,
                                          StateNotificationLeaveState,
                                          m_Device->GetHandle(),
                                          &data);
        }

        m_PowerMachine.m_States.History[m_PowerMachine.IncrementHistoryIndex()] =
            (USHORT) newState;

        if (m_PowerStateCallbacks != NULL)
        {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PowerStateCallbacks->Invoke(newState,
                                          StateNotificationEnterState,
                                          m_Device->GetHandle(),
                                          &data);
        }

        m_Device->SetDevicePowerState(newState);
        currentState = newState;

        entry = GetPowerTableEntry(currentState);

        //
        // And call the state handler, if there is one.
        //
        if (entry->StateFunc != NULL)
        {
            watchdog.StartTimer(currentState);
            newState = entry->StateFunc(this);
            watchdog.CancelTimer(currentState);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_POWER_STATE(currentState, newState);
        }
        else
        {
            newState = WdfDevStatePowerNull;
        }

        if (m_PowerStateCallbacks != NULL)
        {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PowerStateCallbacks->Invoke(currentState,
                                          StateNotificationPostProcessState,
                                          m_Device->GetHandle(),
                                          &data);
        }
    }
}

VOID
FxPkgPnp::PowerCompletePendedWakeIrp(
    VOID
    )
/*++

Routine Description:
    Completes the wait wake request that was pended by the power state machine.
    It is valid if there is no request to complete (the only time there will be
    a request to complete is when this power state machine is the wait wake owner

Arguments:
    None

Return Value:
    None

  --*/
{
    PLIST_ENTRY ple;
    KIRQL irql;

    if (m_SharedPower.m_WaitWakeOwner == FALSE)
    {
        COVERAGE_TRAP();
        return;
    }

    //
    // Pop an irp off of the list
    //
    m_PowerMachine.m_WaitWakeLock.Acquire(&irql);
    ASSERT(IsListEmpty(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList) == FALSE);
    ple = RemoveHeadList(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList);
    m_PowerMachine.m_WaitWakeLock.Release(irql);

    InitializeListHead(ple);
    
    FxIrp irp(FxIrp::GetIrpFromListEntry(ple));

    CompletePowerRequest(&irp, irp.GetStatus());
}