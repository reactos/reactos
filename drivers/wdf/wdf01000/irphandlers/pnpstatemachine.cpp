#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxmacros.h"


#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_PNP_STATE(_CurrentState, _NewState)     \
        ValidatePnpStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_PNP_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY

VOID
FxPkgPnp::PnpProcessEvent(
    __in FxPnpEvent   Event,
    __in BOOLEAN      ProcessOnDifferentThread
    )
/*++

Routine Description:
    This function implements steps 1-8 of the algorithm described above.

Arguments:
    Event - Current PnP event

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    KIRQL oldIrql;

    //
    // Take the lock, raising to DISPATCH_LEVEL.
    //
    m_PnpMachine.Lock(&oldIrql);

    if (m_PnpMachine.IsFull())
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pnp state %!WDF_DEVICE_PNP_STATE! "
            "dropping event %!FxPnpEvent! because of a full queue",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            m_Device->GetDevicePnpState(),
            Event);

        //
        // The queue is full.  Bail.
        //
        m_PnpMachine.Unlock(oldIrql);

        ASSERT(!"The PnP queue is full.  This shouldn't be able to happen.");
        return;
    }

    if (m_PnpMachine.IsClosedLocked())
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pnp state %!WDF_DEVICE_PNP_STATE! "
            "dropping event %!FxPnpEvent! because of a closed queue",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            m_Device->GetDevicePnpState(),
            Event);

        //
        // The queue is closed.  Bail
        //
        m_PnpMachine.Unlock(oldIrql);

        return;
    }

    //
    // Enqueue the event.  Whether the event goes on the front
    // or the end of the queue depends on which event it is.
    //
    if (Event & PnpPriorityEventsMask)
    {
        //
        // Stick it on the front of the queue, making it the next
        // event that will be processed.
        //
        m_PnpMachine.m_Queue[m_PnpMachine.InsertAtHead()] = Event;
    }
    else
    {
        //
        // Stick it on the end of the queue.
        //
        m_PnpMachine.m_Queue[m_PnpMachine.InsertAtTail()] = Event;
    }

    //
    // Drop the lock.
    //
    m_PnpMachine.Unlock(oldIrql);

    //
    // Now, if we are running at PASSIVE_LEVEL, attempt to run the state
    // machine on this thread.  If we can't do that, then queue a work item.
    //

    if (FALSE == ShouldProcessPnpEventOnDifferentThread(
                    oldIrql,
                    ProcessOnDifferentThread
                    ))
    {

        LONGLONG timeout = 0;

        status = m_PnpMachine.m_StateMachineLock.AcquireLock(GetDriverGlobals(),
                                                             &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status))
        {
            FxPostProcessInfo info;

            //
            // We now hold the state machine lock.  So call the function that
            // dispatches the next state.
            //
            PnpProcessEventInner(&info);

            m_PnpMachine.m_StateMachineLock.ReleaseLock(GetDriverGlobals());

            info.Evaluate(this);
            return;
        }
    }

    //
    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.  If m_PnPWorkItemEnqueuing
    // is non-zero, that means that the work item is already being enqueued
    // on another thread.  This is significant, since it means that we can't do
    // anything with the work item on this thread, but it's okay, since the
    // work item will pick up our work and do it.
    //
    m_PnpMachine.QueueToThread();
}

BOOLEAN
FxPkgPnp::ShouldProcessPnpEventOnDifferentThread(
    __in KIRQL CurrentIrql,
    __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
    )
/*++
Routine Description:

    This function returns whether the PnP state machine should process the 
    current event on the same thread or on a different one.

Arguemnts:

    CurrentIrql - The current IRQL
    
    CallerSpecifiedProcessingOnDifferentThread - Whether or not caller of 
        PnpProcessEvent specified that the event be processed on a different 
        thread.

Returns:
    TRUE if the PnP state machine should process the event on a different
       thread.
       
    FALSE if the PnP state machine should process the event on the same thread

--*/
{
    //
    // For KMDF, we ignore what the caller of PnpProcessEvent specified (which
    // should always be FALSE, BTW) and base our decision on the current IRQL.
    // If we are running at PASSIVE_LEVEL, we process on the same thread else
    // we queue a work item.
    //
    UNREFERENCED_PARAMETER(CallerSpecifiedProcessingOnDifferentThread);

    ASSERT(FALSE == CallerSpecifiedProcessingOnDifferentThread);

    return (CurrentIrql == PASSIVE_LEVEL) ? FALSE : TRUE;
}

VOID
FxPkgPnp::_PnpProcessEventInner(
    __inout FxPkgPnp* This,
    __inout FxPostProcessInfo* Info,
    __in PVOID WorkerContext
    )
{
    UNREFERENCED_PARAMETER(WorkerContext);

    //
    // Take the state machine lock.
    //
    This->m_PnpMachine.m_StateMachineLock.AcquireLock(
        This->GetDriverGlobals()
        );

    //
    // Call the function that will actually run the state machine.
    //
    This->PnpProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the lock
    //
    This->m_PnpMachine.m_StateMachineLock.ReleaseLock(
        This->GetDriverGlobals()
        );
}

VOID
FxPkgPnp::PnpProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
/*++

Routine Description:
    This routine runs the state machine.  It implements steps 10-15 of the
    algorithm described above.

--*/
{
    WDF_DEVICE_PNP_STATE newState;
    CPPNP_STATE_TABLE    entry;
    FxPnpEvent           event;
    KIRQL                oldIrql;

    //
    // Process as many events as we can.
    //
    for ( ; ; )
    {
        entry = GetPnpTableEntry(m_Device->GetDevicePnpState());

        //
        // Get an event from the queue.
        //
        m_PnpMachine.Lock(&oldIrql);

        if (m_PnpMachine.IsEmpty())
        {
            m_PnpMachine.GetFinishedState(Info);

            if (m_PnpMachine.m_FireAndForget)
            {
                m_PnpMachine.m_FireAndForget = FALSE;
                Info->m_FireAndForgetIrp = ClearPendingPnpIrp();

                ASSERT(Info->m_FireAndForgetIrp != NULL);
            }

            Info->m_SetRemovedEvent = m_SetDeviceRemoveProcessed;
            m_SetDeviceRemoveProcessed = FALSE;

            //
            // The queue is empty.
            //
            m_PnpMachine.Unlock(oldIrql);

            return;
        }

        event = m_PnpMachine.m_Queue[m_PnpMachine.GetHead()];

        //
        // At this point, we need to determine whether we can process this
        // event.
        //
        if (event & PnpPriorityEventsMask)
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
                m_PnpMachine.Unlock(oldIrql);
                return;
            }
        }

        m_PnpMachine.IncrementHead();
        m_PnpMachine.Unlock(oldIrql);

        //
        // Find the entry in the PnP state table that corresponds
        // to this event.
        //
        newState = WdfDevStatePnpNull;

        if (entry->FirstTargetState.PnpEvent == event)
        {
            newState = entry->FirstTargetState.TargetState;

            DO_EVENT_TRAP(&entry->FirstTargetState);
        }
        else if (entry->OtherTargetStates != NULL)
        {
            ULONG i = 0;

            for (i = 0;
                 entry->OtherTargetStates[i].PnpEvent != PnpEventNull;
                 i++)
            {
                if (entry->OtherTargetStates[i].PnpEvent == event)
                {
                    newState = entry->OtherTargetStates[i].TargetState;
                    DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                    break;
                }
            }
        }

        if (newState == WdfDevStatePnpNull)
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current pnp state "
                "%!WDF_DEVICE_PNP_STATE! dropping event %!FxPnpEvent!",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePnpState(),
                event);

            if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0)
            {
                COVERAGE_TRAP();

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                    "WDFDEVICE 0x%p !devobj %p current state "
                    "%!WDF_DEVICE_PNP_STATE!, policy event %!FxPnpEvent! is not"
                    " a known dropped event, known dropped events are "
                    "%!FxPnpEvent!",
                    m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    m_Device->GetDevicePnpState(),
                    event,
                    entry->StateInfo.Bits.KnownDroppedEvents);

                // DIAG:  add diag code here
            }

            //
            // This state doesn't respond to the Event.  Make sure we do not
            // drop an event which pends a pnp irp on the floor though.
            //
            if (event & PnpEventPending)
            {
                //
                // In the case of a previous power up/down failure, the following
                // can happen
                // 1  invalidate device relations
                // 2  failure event is posted to pnp state machine
                // 3  process power failure event first, but while processing,
                //    query device state is completed, and failed /removed is reported
                // 4  surprise remove comes, the irp is queued, the event is queued
                // 5  processing of power failure event continues, gets to
                //    Failed and completes the s.r irp
                // 6  the surprise remove event is processed (the current value
                //    of the local var event), but since we are already in the
                //    Failed state, we ignore this event and end up
                //    here.
                //
                // This means that if we are processing surprise remove, we cannot
                // 100% expect that an irp has been pended.
                //
                PnpFinishProcessingIrp(
                    (event == PnpEventSurpriseRemove) ? FALSE : TRUE);
            }
            else
            {
                DO_NOTHING();
            }
        }
        else
        {
            //
            // Now enter the new state.
            //
            PnpEnterNewState(newState);
        }
    }
}

VOID
FxPkgPnp::PnpFinishProcessingIrp(
    __in BOOLEAN IrpMustBePresent
    )
/*++

Routine Description:
    Finishes handling a pended pnp irp

Arguments:
    None

Return Value:
    None

  --*/
{
    FxIrp irp;

    UNREFERENCED_PARAMETER(IrpMustBePresent);
    ASSERT(IrpMustBePresent == FALSE || IsPresentPendingPnpIrp());

    //
    // Start device is the only request we handle on the way back up the stack.
    // Also, if we fail any pnp irps that we are allowed to fail, we just
    // complete them.
    //
    if (IsPresentPendingPnpIrp())
    {
        irp.SetIrp(GetPendingPnpIrp());
        if (irp.GetMinorFunction() == IRP_MN_START_DEVICE
            ||
            !NT_SUCCESS(irp.GetStatus()))
        {
            irp.SetIrp(ClearPendingPnpIrp());
            CompletePnpRequest(&irp, irp.GetStatus());
        }
        else
        {
            m_PnpMachine.m_FireAndForget = TRUE;
        }
    }
}

VOID
FxPkgPnp::PnpEnterNewState(
    __in WDF_DEVICE_PNP_STATE State
    )
/*++

Routine Description:
    This function looks up the handler for a state and
    then calls it.

Arguments:
    Event - Current PnP event

Return Value:
    None.

--*/
{
    CPPNP_STATE_TABLE    entry;
    WDF_DEVICE_PNP_STATE currentState, newState;
    WDF_DEVICE_PNP_NOTIFICATION_DATA data;

    currentState = m_Device->GetDevicePnpState();
    newState = State;

    while (newState != WdfDevStatePnpNull)
    {
        DoTraceLevelMessage(
             GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
             "WDFDEVICE 0x%p !devobj 0x%p entering PnP State "
             "%!WDF_DEVICE_PNP_STATE! from %!WDF_DEVICE_PNP_STATE!",
             m_Device->GetHandle(),
             m_Device->GetDeviceObject(),
             newState,
             currentState);

        if (m_PnpStateCallbacks != NULL)
        {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PnpStateCallbacks->Invoke(currentState,
                                        StateNotificationLeaveState,
                                        m_Device->GetHandle(),
                                        &data);
        }

        m_PnpMachine.m_States.History[m_PnpMachine.IncrementHistoryIndex()] =
            (USHORT) newState;

        if (m_PnpStateCallbacks != NULL)
        {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PnpStateCallbacks->Invoke(newState,
                                        StateNotificationEnterState,
                                        m_Device->GetHandle(),
                                        &data);
        }

        m_Device->SetDevicePnpState(newState);
        currentState = newState;

        entry = GetPnpTableEntry(currentState);

        //
        // Call the state handler if one is present and record our new state
        //
        if (entry->StateFunc != NULL)
        {
            newState = entry->StateFunc(this);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_PNP_STATE(currentState, newState);
        }
        else
        {
            newState = WdfDevStatePnpNull;
        }

        if (m_PnpStateCallbacks != NULL)
        {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PnpStateCallbacks->Invoke(currentState,
                                        StateNotificationPostProcessState,
                                        m_Device->GetHandle(),
                                        &data);
        }
    }
}

const PNP_STATE_TABLE FxPkgPnp::m_WdfPnpStates[] = {
    // State function
    // First transition event & state
    // Other transition events & states
    // state info

    //      WdfDevStatePnpObjectCreated
    {   NULL,
        { PnpEventAddDevice, WdfDevStatePnpInit DEBUGGED_EVENT },
        NULL,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpCheckForDevicePresence
    {   NULL,//FxPkgPnp::PnpEventCheckForDevicePresence,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpEjectFailed
    {   NULL,
        { PnpEventStartDevice, WdfDevStatePnpPdoRestart DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpEjectFailedOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpEjectHardware
    {   NULL,//FxPkgPnp::PnpEventEjectHardware,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpEjectedWaitingForRemove
    {   NULL,
        { PnpEventRemove, WdfDevStatePnpPdoRemoved DEBUGGED_EVENT },
        NULL,
        { TRUE,
          PnpEventSurpriseRemove },  // can receive this if parent is surprise
                                     // removed while the ejected pdo is waiting
                                     // for remove.
    },

    //      WdfDevStatePnpInit
    {   NULL,
        { PnpEventStartDevice, WdfDevStatePnpInitStarting DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpInitOtherStates,
        { TRUE,
          PnpEventStartDevice },
    },

    //      WdfDevStatePnpInitStarting
    {   NULL,//FxPkgPnp::PnpEventInitStarting,
        { PnpEventStartDeviceComplete, WdfDevStatePnpHardwareAvailable DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpInitStartingOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpInitSurpriseRemoved
    {   NULL,//FxPkgPnp::PnpEventInitSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpHardwareAvailable
    {   NULL,//FxPkgPnp::PnpEventHardwareAvailable,
        { PnpEventPwrPolStarted, WdfDevStatePnpEnableInterfaces DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpHardwareAvailableOtherStates,
        { FALSE,
          PnpEventPowerUpFailed
        },
    },

    //      WdfDevStatePnpEnableInterfaces
    {   NULL,//FxPkgPnp::PnpEventEnableInterfaces,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpHardwareAvailablePowerPolicyFailed
    {   NULL,//FxPkgPnp::PnpEventHardwareAvailablePowerPolicyFailed,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryRemoveAskDriver
    {    NULL,//FxPkgPnp::PnpEventQueryRemoveAskDriver,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryRemovePending
    {   NULL,//FxPkgPnp::PnpEventQueryRemovePending,
        { PnpEventRemove, WdfDevStatePnpQueriedRemoving DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpQueryRemovePendingOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpQueryRemoveStaticCheck
    {   NULL,//FxPkgPnp::PnpEventQueryRemoveStaticCheck,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueriedRemoving,
    {   NULL,//FxPkgPnp::PnpEventQueriedRemoving,
        { PnpEventPwrPolStopped, WdfDevStatePnpRemovingDisableInterfaces DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpQueriedRemovingOtherStates,
        { FALSE,
          PnpEventPowerDownFailed | // We ignore these power failed events because
          PnpEventPowerUpFailed     // they will be translated into failed power
                                    // policy events.
        },
    },

    //      WdfDevStatePnpQueryStopAskDriver
    {   NULL,//FxPkgPnp::PnpEventQueryStopAskDriver,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryStopPending
    {   NULL,//FxPkgPnp::PnpEventQueryStopPending,
        { PnpEventStop, WdfDevStatePnpStartedStopping DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpQueryStopPendingOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpQueryStopStaticCheck
    {   NULL,//FxPkgPnp::PnpEventQueryStopStaticCheck,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryCanceled,
    {   NULL,//FxPkgPnp::PnpEventQueryCanceled,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemoved
    {   NULL,//FxPkgPnp::PnpEventRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpPdoRemoved
    {   NULL,//FxPkgPnp::PnpEventPdoRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemovedPdoWait
    {   NULL,//FxPkgPnp::PnpEventRemovedPdoWait,
        { PnpEventEject, WdfDevStatePnpEjectHardware DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpRemovedPdoWaitOtherStates,
        { TRUE,
          PnpEventCancelRemove | // Amazingly enough, you can get a cancel q.r.
                                 // on a PDO without seeing the query remove if
                                 // the stack is partially built
          PnpEventQueryRemove  | // Can get a query remove from the removed state
                                 // when installing a PDO that is disabled
          PnpEventPowerDownFailed // We may get this for a PDO if implicit power
                                  // down callbacks were failed. The failed power
                                  // policy stop event took care of rundown.
        },
    },

    //      WdfDevStatePnpRemovedPdoSurpriseRemoved
    {   NULL,//FxPkgPnp::PnpEventRemovedPdoSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemovingDisableInterfaces
    {   NULL,//FxPkgPnp::PnpEventRemovingDisableInterfaces,
        { PnpEventPwrPolRemoved, WdfDevStatePnpRemoved DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRestarting
    {   NULL,//FxPkgPnp::PnpEventRestarting,
        { PnpEventPwrPolStarted, WdfDevStatePnpStarted DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpRestartingOtherStates,
        { FALSE,
          PnpEventPowerUpFailed
        },
    },

    //      WdfDevStatePnpStarted
    {   NULL,//FxPkgPnp::PnpEventStarted,
        { PnpEventQueryRemove, WdfDevStatePnpQueryRemoveStaticCheck DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpStartedOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpStartedCancelStop
    {   NULL,//FxPkgPnp::PnpEventStartedCancelStop,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpStartedCancelRemove
    {   NULL,//FxPkgPnp::PnpEventStartedCancelRemove,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpStartedRemoving
    {   NULL,//FxPkgPnp::PnpEventStartedRemoving,
        { PnpEventPwrPolStopped, WdfDevStatePnpRemovingDisableInterfaces DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpStartedRemovingOtherStates,
        { TRUE,
          PnpEventPowerUpFailed | // device was idled out and in Dx when we got removed
                                  // and this event is due to the power up that occured
                                  // to move it into D0 so it could be disarmed
          PnpEventPowerDownFailed
        },
    },

    //      WdfDevStatePnpStartingFromStopped
    {   NULL,//FxPkgPnp::PnpEventStartingFromStopped,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpStopped
    {   NULL,//FxPkgPnp::PnpEventStopped,
        { PnpEventStartDevice, WdfDevStatePnpStoppedWaitForStartCompletion DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpStoppedOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpStoppedWaitForStartCompletion
    {   NULL,//FxPkgPnp::PnpEventStoppedWaitForStartCompletion,
        { PnpEventStartDeviceComplete, WdfDevStatePnpStartingFromStopped DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpStoppedWaitForStartCompletionOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpStartedStopping
    {   NULL,//FxPkgPnp::PnpEventStartedStopping,
        { PnpEventPwrPolStopped, WdfDevStatePnpStopped DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpStartedStoppingOtherStates,
        { TRUE,
          PnpEventPowerUpFailed | // device was idled out and in Dx when we got stopped
                                  // and this event is due to the power up that occured
                                  // to move it into D0 so it could be disarmed
          PnpEventPowerDownFailed
        },
    },

    // The function is named PnpEventSurpriseRemoved with a 'd' because
    // PnpEventSurpriseRemove (no 'd') is an event name

    //      WdfDevStatePnpSurpriseRemove
    {   NULL,//FxPkgPnp::PnpEventSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpInitQueryRemove
    {   NULL,//FxPkgPnp::PnpEventInitQueryRemove,
        { PnpEventRemove, WdfDevStatePnpRemoved DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpInitQueryRemoveOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpInitQueryRemoveCanceled
    {   NULL,//FxPkgPnp::PnpEventInitQueryRemoveCanceled,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpFdoRemoved
    {   NULL,//FxPkgPnp::PnpEventFdoRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemovedWaitForChildren
    {   NULL,
        { PnpEventChildrenRemovalComplete, WdfDevStatePnpRemovedChildrenRemoved DEBUGGED_EVENT },
        NULL,
        { TRUE,
          PnpEventPowerDownFailed  // device power down even from processing remove
        },
    },

    //      WdfDevStatePnpQueriedSurpriseRemove
    {   NULL,//FxPkgPnp::PnpEventQueriedSurpriseRemove,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpSurpriseRemoveIoStarted
    {   NULL,//FxPkgPnp::PnpEventSurpriseRemoveIoStarted,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFailedPowerDown
    {   NULL,//FxPkgPnp::PnpEventFailedPowerDown,
        { PnpEventPwrPolStopped, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpFailedPowerDownOtherStates,
        { FALSE,
          PnpEventPowerDownFailed ,
        },
    },

    //      WdfDevStatePnpFailedIoStarting
    {   NULL,//FxPkgPnp::PnpEventFailedIoStarting,
        { PnpEventPwrPolStopped, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpFailedIoStartingOtherStates,
        { FALSE,
          PnpEventPowerDownFailed |

          PnpEventPowerUpFailed   // if the device idled out and then failed
                                  // d0 entry, the power up failed can be passed
                                  // up by the IoInvalidateDeviceRelations and
                                  // subsequence surprise remove event.
        },
    },

    //      WdfDevStatePnpFailedOwnHardware
    {   NULL,//FxPkgPnp::PnpEventFailedOwnHardware,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFailed
    {   NULL,//FxPkgPnp::PnpEventFailed,
        //
        // NOTE: WdfDevStatePnpFailedPowerPolicyRemoved not in WDF 1.9
        //
        //{ PnpEventPwrPolRemoved, WdfDevStatePnpFailedPowerPolicyRemoved DEBUGGED_EVENT },
        { PnpEventPwrPolRemoved, WdfDevStatePnpNull DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0,
        },
    },

    //      WdfDevStatePnpFailedSurpriseRemoved
    {   NULL,//FxPkgPnp::PnpEventFailedSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0, },
    },

    //      WdfDevStatePnpFailedStarted
    {   NULL,//FxPkgPnp::PnpEventFailedStarted,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0, },
    },

    //      WdfDevStatePnpFailedWaitForRemove,
    {   NULL,
        { PnpEventRemove, WdfDevStatePnpRemoved DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpFailedWaitForRemoveOtherStates,
        { TRUE,
          PnpEventPowerUpFailed | // initial power up failed, power policy start
                                  // failed event moved the state machine to the
                                  // failed state first
          PnpEventPowerDownFailed | // implicitD3 power down failed
          PnpEventQueryRemove |  // start succeeded, but we still get a query in
                                 // the removed case
          PnpEventCancelStop |   // power down failure while processing query stop
                                 // and q.s. irp completed with error
          PnpEventCancelRemove   // power down failure while processing query remove
                                 // and q.r. irp completed with error
        },
    },

    //      WdfDevStatePnpFailedInit
    {   NULL,//FxPkgPnp::PnpEventFailedInit,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpPdoInitFailed
    {   NULL,//FxPkgPnp::PnpEventPdoInitFailed,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRestart
    {   NULL,//FxPkgPnp::PnpEventRestart,
        { PnpEventPwrPolStopped, WdfDevStatePnpRestartReleaseHardware DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpRestartOtherStates,
        { FALSE,
          PnpEventPowerUpFailed | // when stopping power policy, device was in
                                  // Dx and bringing it to D0 succeeded or failed
          PnpEventPowerDownFailed // same as power up
        },
    },

    //      WdfDevStatePnpRestartReleaseHardware
    {   NULL,//FxPkgPnp::PnpEventRestartReleaseHardware,
        { PnpEventStartDeviceComplete, WdfDevStatePnpRestartHardwareAvailable DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpRestartReleaseHardware,
        { TRUE,
          PnpEventPowerDownFailed // the previous pwr policy stop
                                  // in WdfDevStaePnpRestart will
                                  // cause these events to show up here
        },
    },

    //      WdfDevStatePnpRestartHardwareAvailable
    {   NULL,//FxPkgPnp::PnpEventRestartHardwareAvailable,
        { PnpEventPwrPolStarted, WdfDevStatePnpStarted DEBUGGED_EVENT },
        NULL,//FxPkgPnp::m_PnpRestartHardwareAvailableOtherStates,
        { TRUE,
          PnpEventPowerUpFailed
        },
    },

    //      WdfDevStatePnpPdoRestart
    {   NULL,//FxPkgPnp::PnpEventPdoRestart,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFinal
    {   NULL,//FxPkgPnp::PnpEventFinal,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { TRUE,
          PnpEventPowerDownFailed, // on the final implicit power down, a
                                   // callback returned !NT_SUCCESS
        },
    },

    //      WdfDevStatePnpRemovedChildrenRemoved
    {   NULL,//FxPkgPnp::PnpEventRemovedChildrenRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { TRUE,
          0 } ,
    },

    //      WdfDevStatePnpQueryRemoveEnsureDeviceAwake
    {   NULL,//FxPkgPnp::PnpEventQueryRemoveEnsureDeviceAwake,
        { PnpEventDeviceInD0, WdfDevStatePnpQueryRemovePending DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryStopEnsureDeviceAwake
    {   NULL,//FxPkgPnp::PnpEventQueryStopEnsureDeviceAwake,
        { PnpEventDeviceInD0, WdfDevStatePnpQueryStopPending DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFailedPowerPolicyRemoved
    {   NULL,//FxPkgPnp::PnpEventFailedPowerPolicyRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 } ,
    },
};

VOID
FxPkgPnp::_PowerProcessEventInner(
    __in FxPkgPnp* This,
    __in FxPostProcessInfo* Info,
    __in PVOID WorkerContext
    )
{
    
    UNREFERENCED_PARAMETER(WorkerContext);

    //
    // Take the state machine lock.
    //
    This->m_PowerMachine.m_StateMachineLock.AcquireLock(
        This->GetDriverGlobals()
        );

    //
    // Call the function that will actually run the state machine.
    //
    This->PowerProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the lock
    //
    This->m_PowerMachine.m_StateMachineLock.ReleaseLock(
        This->GetDriverGlobals()
        );
}
