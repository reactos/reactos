#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxirp.h"
#include "common/fxmacros.h"


VOID
FxPkgPnp::CleanupDeviceFromFailedCreate(
    __in MxEvent * WaitEvent
    )
/*++

Routine Description:
    The device failed creation in some stage.  It is assumed that the device has
    enough state that it can survive a transition through the pnp state machine
    (which means that pointers like m_PkgIo are valid and != NULL).  When this
    function returns, it will have deleted the owning FxDevice.

Arguments:
    WaitEvent - Event on which RemoveProcessed wait will be performed

                We can't initialize this event on stack as the initialization
                can fail in user-mode. We can't have Initialize method
                preinitailize this event either as this function may get called
                before initialize (or in case of initialization failure).

                Hence the caller preallocates the event and passes to this
                function.

                Caller must initialize this event as SynchronizationEvent
                and it must be unsignalled.
Return Value:
    None

  --*/
{    
    Mx::MxAssert(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Caller must initialize the event as Synchronization event and it should
    // be passed as non-signalled. But we Clear it just to be sure.
    //
    WaitEvent->Clear();

    ADDREF(WaitEvent);

    ASSERT(m_DeviceRemoveProcessed == NULL);
    m_DeviceRemoveProcessed = WaitEvent;

    //
    // Simulate a remove event coming to the device.  After this call returns
    // m_Device is still valid and must be deleted.
    //
    PnpProcessEvent(PnpEventRemove);

    //
    // No need to wait in a critical region because we are in the context of a
    // pnp request which is in the system context.
    //
    WaitEvent->WaitFor(Executive, KernelMode, FALSE, NULL);
    m_DeviceRemoveProcessed = NULL;

    RELEASE(WaitEvent);    
}

NTSTATUS
FxPkgPnp::CompletePnpRequest(
    __inout FxIrp* Irp,
    __in    NTSTATUS Status
    )
{
    MdIrp pIrp = Irp->GetIrp();

    Irp->SetStatus(Status);
    Irp->CompleteRequest(IO_NO_INCREMENT);

    Mx::MxReleaseRemoveLock(m_Device->GetRemoveLock(),
                            pIrp);

    return Status;
}

VOID
FxPkgPnp::ProcessDelayedDeletion(
    VOID
    )
{
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "WDFDEVICE %p, !devobj %p processing delayed deletion from pnp state "
        "machine", m_Device->GetHandle(), m_Device->GetDeviceObject());

    CleanupStateMachines(FALSE);
    DeleteDevice();
}

VOID
FxPkgPnp::DeleteDevice(
    VOID
    )
/*++

Routine Description:
    This routine will detach and delete the device object and free the memory
    for the device if there are no other references to it.  Before calling this
    routine, the state machines should have been cleaned up and the power thread
    released.

--*/
{
    //
    // This will detach and delete the device object
    //
    m_Device->Destroy();

    //
    // If this is the last reference, this will free the memory for the device
    //
    m_Device->DeleteObject();
}

VOID
FxPkgPnp::CleanupStateMachines(
    __in BOOLEAN CleanupPnp
    )
{
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    FxCREvent * event = m_CleanupEventUm.GetSelfPointer();
#else
    FxCREvent eventOnStack;
    eventOnStack.Initialize();
    FxCREvent * event = eventOnStack.GetSelfPointer();
#endif

    //
    // Order of shutdown is important here.
    // o Pnp initiates events to power policy.
    // o Power policy initiates events to power and device-power-requirement
    // o Power does not initiate any events
    // o Device-power-requirement does not initiate any events
    //
    // By shutting them down in the order in which they send events, we can
    // guarantee that no new events will be posted into the subsidiary state
    // machines.
    //

    //
    // This will shut off the pnp state machine and synchronize any outstanding
    // threads of execution.
    //
    if (CleanupPnp && m_PnpMachine.SetFinished(
            event
            ) == FALSE)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p, !devobj %p waiting for pnp state machine to finish",
            m_Device->GetHandle(), m_Device->GetDeviceObject());

        //
        // Process the event *before* completing the irp so that this event is in
        // the queue before the device remove event which will be processed
        // right after the start irp has been completed.
        //
        event->EnterCRAndWaitAndLeave();
    }

    //
    // Even though event is a SynchronizationEvent, so we need to reset it for
    // the next wait because SetFinished will set it if even if the transition
    // to the finished state is immediate
    //
    event->Clear();

    if (m_PowerPolicyMachine.SetFinished(event) == FALSE)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p, !devobj %p waiting for pwr pol state machine to finish",
            m_Device->GetHandle(), m_Device->GetDeviceObject());

        event->EnterCRAndWaitAndLeave();
    }

    //
    // See previous comment about why we Clear()
    //
    event->Clear();

    if (m_PowerMachine.SetFinished(event) == FALSE)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p, !devobj %p waiting for pwr state machine to finish",
            m_Device->GetHandle(), m_Device->GetDeviceObject());

        event->EnterCRAndWaitAndLeave();
    }

    if (IsPowerPolicyOwner())
    {
        //
        // See previous comment about why we Clear()
        //
        event->Clear();

        if (NULL != m_PowerPolicyMachine.m_Owner->m_PoxInterface.
                                        m_DevicePowerRequirementMachine)
        {
            if (FALSE == m_PowerPolicyMachine.m_Owner->m_PoxInterface.
                          m_DevicePowerRequirementMachine->SetFinished(event))
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                    "WDFDEVICE %p, !devobj %p waiting for device power "
                    "requirement state machine to finish",
                    m_Device->GetHandle(),
                    m_Device->GetDeviceObject());

                event->EnterCRAndWaitAndLeave();
            }
        }

        m_PowerPolicyMachine.m_Owner->CleanupPowerCallback();
    }

    //
    // Release the power thread if we have one either through creation or query.
    // Since the power policy state machine is off, we should no longer need
    // a dedicated thread.
    //
    // *** NOTE ***
    // The power thread must be released *BEFORE* sending the irp down the stack
    // because this can happen
    // 1)  this driver is not the power thread owner, but the last client
    // 2)  we send the pnp irp first
    // 3)  the power thread owner waits on this thread for all the clients to go
    //     away, but this device still has a reference on it
    // 4)  this device will not release the reference b/c the owner is waiting
    //     in the same thread.
    //
    ReleasePowerThread();

    //
    // Deref the reenumeration interface
    //
    ReleaseReenumerationInterface();
}

VOID
FxPkgPnp::ReleasePowerThread(
    VOID
    )
/*++

Routine Description:
    If this device is the owner of the power thread, it kills the thread.
    Otherwise, if this device has acquired the thread from a lower device,
    release the reference now.

Arguments:
    None

Return Value:
    None

  --*/
{
    BOOLEAN hadThread;

    hadThread = m_HasPowerThread;

    //
    // Set to FALSE before cleaning up the reference or thread itself in case
    // there is some other context trying to enqueue.  The only way that could
    // be happening is if the power policy owner is not WDF and sends power irps
    // after query remove or surprise remove.
    //
    m_HasPowerThread = FALSE;

    //
    // Check for ownership
    //
    if (m_PowerThread != NULL)
    {
        FxCREvent event;

        //
        // Event on stack is used, which is fine since this code is invoked
        // only in KM. Verify this assumption.
        //
        // If this code is ever needed for UM, m_PowerThreadEvent should be
        // pre-initialized (simlar to the way m_RemoveEventUm is used)
        //
        WDF_VERIFY_KM_ONLY_CODE();

        ASSERT(m_PowerThreadEvent == NULL);
        m_PowerThreadEvent = event.GetSelfPointer();

        if (InterlockedDecrement(&m_PowerThreadInterfaceReferenceCount) > 0)
        {
            //
            // Wait for all references to go away before exitting the thread.
            // A reference will be taken for every device in the stack above this
            // one which queried for the interface.
            //
            event.EnterCRAndWaitAndLeave();
        }

        m_PowerThreadEvent = NULL;

        //
        // Wait for the thread to exit and then delete it.  Since we have
        // turned off the power policy state machine, we can safely do this here.
        // Any upper level clients will have also turned off their power policy
        // state machines.
        //
        m_PowerThread->ExitThread();
        m_PowerThread->DeleteObject();

        m_PowerThread = NULL;
    }
    else if (hadThread)
    {
        //
        // Release our reference
        //
        m_PowerThreadInterface.Interface.InterfaceDereference(
            m_PowerThreadInterface.Interface.Context
            );
    }
}
