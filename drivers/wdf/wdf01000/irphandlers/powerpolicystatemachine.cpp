#include "common/fxpowerpolicystatemachine.h"
#include "common/fxpkgpnp.h"
#include "common/fxpoweridlestatemachine.h"
#include "common/fxdevice.h"
#include "common/fxwatchdog.h"



const POWER_POLICY_STATE_TABLE FxPkgPnp::m_WdfPowerPolicyStates[] =
{
    // TODO: Fill table

    // transition function,
    // { first target state },
    // other target states
    // queue open,

    // WdfDevStatePwrPolObjectCreated
    { NULL,
      { PwrPolStart, WdfDevStatePwrPolStarting DEBUGGED_EVENT },
      0,//FxPkgPnp::m_PowerPolObjectCreatedOtherStates,
      { TRUE,
        PwrPolS0 |  // Sx -> S0 transition on a PDO which was enumerated and
                    // in the disabled state

        PwrPolSx |  // Sx transition right after enumeration
        PwrPolS0IdlePolicyChanged },
    }
};


#if FX_SUPER_DBG
  #define ASSERT_PWR_POL_STATE(_This, _State) \
      ASSERT((_This)->m_Device->GetDevicePowerPolicyState() == (_State))
#else
  #define ASSERT_PWR_POL_STATE(_This, _State) (0)
#endif

#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_PWR_POL_STATE(_CurrentState, _NewState)                      \
        ValidatePwrPolStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_PWR_POL_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY

FxPowerPolicyOwnerSettings::FxPowerPolicyOwnerSettings(
    __in FxPkgPnp* PkgPnp
    ) : m_PoxInterface(PkgPnp)
{
    ULONG i;

    m_UsbIdle = NULL;

    m_PkgPnp = PkgPnp;

    //
    // Default every state to D3 except for system working which is D0 by
    // default.
    //
    m_SystemToDeviceStateMap = 0x0;

    for (i = 0; i < PowerSystemMaximum; i++)
    {
        FxPkgPnp::_SetPowerCapState(i,
                                    i == PowerSystemWorking ? PowerDeviceD0
                                                            : PowerDeviceD3,
                                    &m_SystemToDeviceStateMap);
    }

    m_IdealDxStateForSx = PowerDeviceD3;

    m_RequestedPowerUpIrp = FALSE;
    m_RequestedPowerDownIrp = FALSE;
    m_RequestedWaitWakeIrp = FALSE;
    m_WakeCompletionEventDropped = FALSE;
    m_PowerFailed = FALSE;
    m_CanSaveState = TRUE;

    m_ChildrenCanPowerUp = FALSE;
    m_ChildrenPoweredOnCount = 0;
    m_ChildrenArmedCount = 0;

    m_WaitWakeStatus = STATUS_NOT_SUPPORTED;
    m_SystemWakeSource = FALSE;
    m_WaitWakeCancelCompletionOwnership = CancelOwnershipUnclaimed;

    m_PowerCallbackObject = NULL;
    m_PowerCallbackRegistration = NULL;
}

FxPowerPolicyOwnerSettings::~FxPowerPolicyOwnerSettings(
    VOID
    )
{
    //
    // There are paths which cleanup this object which  do not go through the
    // pnp state machine, so make sure the power object callback fields are
    // freed.  In these paths, we are guaranteed PASSIVE_LEVEL b/c there will
    // be no dangling references which can cause deletion at a greater IRQL.
    //
    CleanupPowerCallback();

    if (m_UsbIdle != NULL)
    {
        delete m_UsbIdle;
        m_UsbIdle = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPowerPolicyOwnerSettings::Init(
    VOID
    )
/*++

Routine Description:
    Initialize the object.  We will try to register a power state callback so
    that we can be informed of when the machine is changing S states.  We are
    interested in system state changes because we need to know when NOT to write
    to the registry (to save wake settings) so that we don't cause a deadlock
    in a non power pageable device while moving into Sx.

Arguments:
    None

Return Value:
    NTSTATUS

  --*/
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING string;
    NTSTATUS status;

    RtlInitUnicodeString(&string, L"\\Callback\\PowerState");
    InitializeObjectAttributes(
        &oa,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Create a callback object, but we do not want to be the first ones
    // to create it (it should be created way before we load in NTOS
    // anyways)
    //
    status = Mx::CreateCallback(&m_PowerCallbackObject, &oa, FALSE, TRUE);

    if (NT_SUCCESS(status))
    {
        m_PowerCallbackRegistration = Mx::RegisterCallback(
            m_PowerCallbackObject,
            _PowerStateCallback,
            this
            );

        if (m_PowerCallbackRegistration == NULL)
        {
            //
            // Non-critical failure, so we'll free the callback object and keep
            // going
            //
            Mx::MxDereferenceObject(m_PowerCallbackObject);
            m_PowerCallbackObject = NULL;
        }
    }

    //
    // Initialize FxPowerIdleMachine
    //
    status = m_PowerIdleMachine.Init();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return status;
}

VOID
FxPowerPolicyOwnerSettings::CleanupPowerCallback(
    VOID
    )
/*++

Routine Description:
    Cleans up the power state callback registration for this object.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (m_PowerCallbackRegistration != NULL)
    {
        Mx::UnregisterCallback(m_PowerCallbackRegistration);
        m_PowerCallbackRegistration = NULL;
    }

    if (m_PowerCallbackObject != NULL)
    {
        Mx::MxDereferenceObject(m_PowerCallbackObject);
        m_PowerCallbackObject = NULL;
    }
}

VOID
FxPkgPnp::_PowerPolicyProcessEventInner(
    __inout FxPkgPnp* This,
    __inout FxPostProcessInfo* Info,
    __in    PVOID Context
    )
{
    
    UNREFERENCED_PARAMETER(Context);

    //
    // Take the state machine lock.
    //
    This->m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
        This->GetDriverGlobals()
        );

    //
    // Call the function that will actually run the state machine.
    //
    This->PowerPolicyProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the lock
    //
    This->m_PowerPolicyMachine.m_StateMachineLock.ReleaseLock(
        This->GetDriverGlobals()
        );
}

VOID
FxPowerPolicyOwnerSettings::_PowerStateCallback(
    __in     PVOID Context,
    __in_opt PVOID Argument1,
    __in_opt PVOID Argument2
    )
/*++

Routine Description:
    Callback invoked by the power subsystem when the system is changing S states

Arguments:
    Context -  FxPowerPolicyOwnerSettings pointer (the "this" pointer)

    Argument1 - Reason why the callback was invoked, we only care about
                PO_CB_SYSTEM_STATE_LOCK

    Argument2 - State transition that is occurring

Return Value:
    None

  --*/
{
    FxPowerPolicyOwnerSettings* pThis;

    pThis = (FxPowerPolicyOwnerSettings*) Context;

    if (Argument1 != (PVOID) PO_CB_SYSTEM_STATE_LOCK)
    {
        return;
    }

    pThis->m_PkgPnp->m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
        pThis->m_PkgPnp->GetDriverGlobals(),
        NULL
        );

    if (Argument2 == (PVOID) 0)
    {
        //
        // Write out the state if necessary before we turn off the paging path.
        //
        pThis->m_PkgPnp->SaveState(TRUE);

        //
        // Exiting S0
        //
        pThis->m_CanSaveState = FALSE;
    }
    else if (Argument2 == (PVOID) 1)
    {
        //
        // We have reentered S0
        //
        pThis->m_CanSaveState = TRUE;

        //
        // Write out the state if necessary now that the paging path is back
        //
        pThis->m_PkgPnp->SaveState(TRUE);
    }

    pThis->m_PkgPnp->m_PowerPolicyMachine.m_StateMachineLock.ReleaseLock(
        pThis->m_PkgPnp->GetDriverGlobals()
        );
}

VOID
FxPkgPnp::PowerPolicyProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
{
    WDF_DEVICE_POWER_POLICY_STATE newState;
    FxPowerPolicyEvent event;
    ULONG i;
    KIRQL irql;

    if (IsPowerPolicyOwner())
    {
        CPPOWER_POLICY_STATE_TABLE entry;

        //
        // Process as many events as we can.
        //
        for ( ; ; )
        {
            entry = GetPowerPolicyTableEntry(m_Device->GetDevicePowerPolicyState());

            //
            // Get an event from the queue.
            //
            m_PowerPolicyMachine.Lock(&irql);

            if (m_PowerPolicyMachine.IsEmpty())
            {
                m_PowerPolicyMachine.GetFinishedState(Info);

                //
                // The queue is empty.
                //
                m_PowerPolicyMachine.Unlock(irql);
                return;
            }

            event = m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.GetHead()];

            //
            // At this point, we need to determine whether we can process this
            // event.
            //
            if (event & PwrPolPriorityEventsMask)
            {
                //
                // These are always possible to handle.
                //
                DO_NOTHING();
            }
            else
            {
                //
                // Check to see if this state can handle new events, ie if this
                // is a green dot (queue open) or red dot (queue *not* open) state.
                //
                if (entry->StateInfo.Bits.QueueOpen == FALSE)
                {
                    //
                    // This state can't handle new events.
                    //
                    m_PowerPolicyMachine.Unlock(irql);
                    return;
                }
            }

            //
            // If the event obtained from the queue was a singular event, then
            // clear the flag to allow other similar events to be put into this
            // queue for processing.
            //
            if (m_PowerPolicyMachine.m_SingularEventsPresent & event)
            {
               m_PowerPolicyMachine.m_SingularEventsPresent &= ~event;
            }

            m_PowerPolicyMachine.IncrementHead();
            m_PowerPolicyMachine.Unlock(irql);

            //
            // Find the entry in the power policy state table that corresponds
            // to this event.
            //
            newState = WdfDevStatePwrPolNull;

            if (entry->FirstTargetState.PowerPolicyEvent == event)
            {
                newState = entry->FirstTargetState.TargetState;

                DO_EVENT_TRAP(&entry->FirstTargetState);
            }
            else if (entry->OtherTargetStates != NULL)
            {
                for (i = 0;
                     entry->OtherTargetStates[i].PowerPolicyEvent != PwrPolNull;
                     i++)
                {
                    if (entry->OtherTargetStates[i].PowerPolicyEvent == event)
                    {
                        newState = entry->OtherTargetStates[i].TargetState;
                        DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                        break;
                    }
                }
            }

            if (newState == WdfDevStatePwrPolNull)
            {
                //
                // This state doesn't respond to the event.  Just throw the event
                // away.
                //
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
                    "%!WDF_DEVICE_POWER_POLICY_STATE! dropping event "
                    "%!FxPowerPolicyEvent!", m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    m_Device->GetDevicePowerPolicyState(), event);

                if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0)
                {
                    COVERAGE_TRAP();

                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "WDFDEVICE 0x%p !devobj 0x%p current state "
                        "%!WDF_DEVICE_POWER_POLICY_STATE!, policy event "
                        "%!FxPowerPolicyEvent! is not a known dropped "
                        "event, known dropped events are %!FxPowerPolicyEvent!",
                        m_Device->GetHandle(), m_Device->GetDeviceObject(),
                        m_Device->GetDevicePowerPolicyState(),
                        event, entry->StateInfo.Bits.KnownDroppedEvents);
                }

                //
                // Failsafes for events which have required processing in them.
                //
                switch (event) {
                case PwrPolSx:
                    //
                    // The Sx handling code expects that the state machine
                    // complete the Sx irp.  (S0 irps are never pended).  Since
                    // we don't have a state to transition to that will complete
                    // the request, do so now.
                    //
                    // (This can legitimately happen if a PDO is disabled and
                    //  the machines moves into an Sx state.)
                    //
                    PowerPolicyCompleteSystemPowerIrp();
                    break;

                case PwrPolUsbSelectiveSuspendCompleted:
                    //
                    // This state did not handle the event and event got 
                    // dropped. However some state is definitely going to wait
                    // for this event. That's why we need m_EventDropped flag. 
                    // If we didn't have this flag there will be no way to know 
                    // if the event got dropped and some state will end up 
                    // waiting for it indefinitely.
                    //

                    // TODO: Uncomment when implement USB
                    //m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_EventDropped = TRUE;
                    __debugbreak();
                    break;

                case PwrPolUsbSelectiveSuspendCallback:
                    // TODO: Uncomment when implement USB
                    //m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
                    __debugbreak();
                    break;

                case PwrPolWakeSuccess:
                case PwrPolWakeFailed:
                    //
                    // This state did not handle the event and event got 
                    // dropped. However some state is definitely going to wait
                    // for this event. That's why we need 
                    // m_WakeCompletionEventDropped flag. If we didn't have this
                    // flag there will be no way to know if the event got 
                    // dropped and some state will end up waiting for it 
                    // indefinitely.
                    //
                    m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped = TRUE;
                    break;

                default:
                    DO_NOTHING();
                    break;
                }
            }
            else
            {
                //
                // Now enter the new state.
                //
                PowerPolicyEnterNewState(newState);
            }
        }
    }
    else
    {
        //
        // Process as many events as we can.
        //
        for ( ; ; )
        {
            CPNOT_POWER_POLICY_OWNER_STATE_TABLE entry;

#pragma prefast(suppress:__WARNING_DEREF_NULL_PTR, "The current power policy state will always be in the table so entry will never be NULL")
            entry = GetNotPowerPolicyOwnerTableEntry(
                m_Device->GetDevicePowerPolicyState()
                );

            //
            // Get an event from the queue.
            //
            m_PowerPolicyMachine.Lock(&irql);

            if (m_PowerPolicyMachine.IsEmpty())
            {
                //
                // The queue is empty.
                //
                m_PowerPolicyMachine.Unlock(irql);
                return;
            }

            event = m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.GetHead()];

            //
            // At this point, we need to determine whether we can process this
            // event.
            //
            if (event & PwrPolNotOwnerPriorityEventsMask)
            {
                //
                // These are always possible to handle.
                //
                DO_NOTHING();
            }
            else
            {
                //
                // Check to see if this state can handle new events, ie if this
                // is a green dot (queue open) or red dot (queue *not* open) state.
                //
                if (entry->QueueOpen == FALSE)
                {
                    //
                    // This state can't handle new events.
                    //
                    m_PowerPolicyMachine.Unlock(irql);
                    return;
                }
            }

            //
            // If the event obtained from the queue was a singular event, then
            // clear the flag to allow other similar events to be put into this
            // queue for processing.
            //
            if (m_PowerPolicyMachine.m_SingularEventsPresent & event)
            {
               m_PowerPolicyMachine.m_SingularEventsPresent &= ~event;
            }

            m_PowerPolicyMachine.IncrementHead();
            m_PowerPolicyMachine.Unlock(irql);

            if (entry != NULL && entry->TargetStatesCount > 0)
            {
                for (i = 0; i < entry->TargetStatesCount; i++)
                {
                    if (event == entry->TargetStates[i].PowerPolicyEvent)
                    {
                        DO_EVENT_TRAP(&entry->TargetStates[i]);

                        //
                        // Now enter the new state.
                        //
                        NotPowerPolicyOwnerEnterNewState(
                            entry->TargetStates[i].TargetState);
                        break;
                    }
                }
            }
        }
    }
}

PolicySettings::~PolicySettings()
{
}

VOID
FxPkgPnp::PowerPolicyCompleteSystemPowerIrp(
    VOID
    )
{
    FxIrp irp(m_PendingSystemPowerIrp);
    NTSTATUS status;

    ASSERT(m_PendingSystemPowerIrp != NULL);

    status = STATUS_SUCCESS;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Completing system power irp %p (S%d), %!STATUS!",
        m_PendingSystemPowerIrp,
        irp.GetParameterPowerStateSystemState()-1,
        status);

    m_PendingSystemPowerIrp = NULL;

    CompletePowerRequest(&irp, STATUS_SUCCESS);
}

VOID
FxPkgPnp::PowerPolicyEnterNewState(
    __in WDF_DEVICE_POWER_POLICY_STATE NewState
    )
/*++

Routine Description:
    This function looks up the handler for a state and then calls it.

Arguments:
    Event - Current power plicy event

Return Value:

    NTSTATUS

--*/
{
    CPPOWER_POLICY_STATE_TABLE entry;
    WDF_DEVICE_POWER_POLICY_STATE currentState, newState;
    WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA data;
    FxWatchdog watchdog(this);

    currentState = m_Device->GetDevicePowerPolicyState();
    newState = NewState;

    while (newState != WdfDevStatePwrPolNull)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering power policy state "
            "%!WDF_DEVICE_POWER_POLICY_STATE! from "
            "%!WDF_DEVICE_POWER_POLICY_STATE!", m_Device->GetHandle(),
            m_Device->GetDeviceObject(), newState, currentState);

        if (m_PowerPolicyStateCallbacks != NULL)
        {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PowerPolicyStateCallbacks->Invoke(currentState,
                                                StateNotificationLeaveState,
                                                m_Device->GetHandle(),
                                                &data);
        }

        m_PowerPolicyMachine.m_States.History[
            m_PowerPolicyMachine.IncrementHistoryIndex()] = (USHORT) newState;

        if (m_PowerPolicyStateCallbacks != NULL)
        {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PowerPolicyStateCallbacks->Invoke(newState,
                                                StateNotificationEnterState,
                                                m_Device->GetHandle(),
                                                &data);
        }

        m_Device->SetDevicePowerPolicyState(newState);
        currentState = newState;

        entry = GetPowerPolicyTableEntry(currentState);

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
            VALIDATE_PWR_POL_STATE(currentState, newState);

        }
        else
        {
            newState = WdfDevStatePwrPolNull;
        }

        if (m_PowerPolicyStateCallbacks != NULL)
        {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PowerPolicyStateCallbacks->Invoke(currentState,
                                                StateNotificationPostProcessState,
                                                m_Device->GetHandle(),
                                                &data);
        }
    }
}

FxPowerPolicyMachine::FxPowerPolicyMachine(
    VOID
    ) : FxThreadedEventQueue(FxPowerPolicyEventQueueDepth)
{
    m_Owner = NULL;

    RtlZeroMemory(&m_Queue[0], sizeof(m_Queue));
    RtlZeroMemory(&m_States, sizeof(m_States));

    m_States.History[IncrementHistoryIndex()] = WdfDevStatePwrPolObjectCreated;

    m_SingularEventsPresent = 0x0;
}

FxPowerPolicyMachine::~FxPowerPolicyMachine(
    VOID
    )
{
    if (m_Owner != NULL)
    {
        delete m_Owner;
        m_Owner = NULL;
    }
}
