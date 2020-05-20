/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Pnp IRP handlers
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxirp.h"
#include "common/fxmacros.h"
#include "common/fxdeviceinit.h"
#include "common/fxautoregistry.h"
#include "common/fxwatchdog.h"
#include "common/fxdeviceinterface.h"
#include "common/fxinterrupt.h"
#include "pnppriv.h"
#include "common/fxrelateddevice.h"



/* dc7a8e51-49b3-4a3a-9e81-625205e7d729 */
const GUID FxPkgPnp::GUID_POWER_THREAD_INTERFACE = {
    0xdc7a8e51, 0x49b3, 0x4a3a, { 0x9e, 0x81, 0x62, 0x52, 0x05, 0xe7, 0xd7, 0x29 }
};

FxPkgPnp::FxPkgPnp(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice* Device,
    __in WDFTYPE Type
    ) :
    FxPackage(FxDriverGlobals, Device, Type)
{
    ULONG i;

    m_DmaEnablerList = NULL;
    m_RemovalDeviceList = NULL;
    m_UsageDependentDeviceList = NULL;

    //
    // Initialize the structures to the default state and then override the
    // non WDF std default values to the unsupported / off values.
    //
    m_PnpStateAndCaps.Value =
        FxPnpStateDisabledUseDefault         |
        FxPnpStateDontDisplayInUIUseDefault  |
        FxPnpStateFailedUseDefault           |
        FxPnpStateNotDisableableUseDefault   |
        FxPnpStateRemovedUseDefault          |
        FxPnpStateResourcesChangedUseDefault |

        FxPnpCapLockSupportedUseDefault      |
        FxPnpCapEjectSupportedUseDefault     |
        FxPnpCapRemovableUseDefault          |
        FxPnpCapDockDeviceUseDefault         |
        FxPnpCapUniqueIDUseDefault           |
        FxPnpCapSilentInstallUseDefault      |
        FxPnpCapSurpriseRemovalOKUseDefault  |
        FxPnpCapHardwareDisabledUseDefault   |
        FxPnpCapNoDisplayInUIUseDefault
        ;

    m_PnpCapsAddress = (ULONG) -1;
    m_PnpCapsUINumber = (ULONG) -1;

    RtlZeroMemory(&m_PowerCaps, sizeof(m_PowerCaps));
    m_PowerCaps.Caps =
        FxPowerCapDeviceD1UseDefault   |
        FxPowerCapDeviceD2UseDefault   |
        FxPowerCapWakeFromD0UseDefault |
        FxPowerCapWakeFromD1UseDefault |
        FxPowerCapWakeFromD2UseDefault |
        FxPowerCapWakeFromD3UseDefault
        ;

    m_PowerCaps.DeviceWake = PowerDeviceMaximum;
    m_PowerCaps.SystemWake = PowerSystemMaximum;

    m_PowerCaps.D1Latency = (ULONG) -1;
    m_PowerCaps.D2Latency = (ULONG) -1;
    m_PowerCaps.D3Latency = (ULONG) -1;

    m_PowerCaps.States = 0;
    for (i = 0; i < PowerSystemMaximum; i++)
    {
        _SetPowerCapState(i, PowerDeviceMaximum, &m_PowerCaps.States);
    }

    //RtlZeroMemory(&m_D3ColdInterface, sizeof(m_D3ColdInterface));
    RtlZeroMemory(&m_SpecialSupport[0], sizeof(m_SpecialSupport));
    RtlZeroMemory(&m_SpecialFileCount[0], sizeof(m_SpecialFileCount));

    m_PowerThreadInterface.Interface.Size = sizeof(m_PowerThreadInterface);
    m_PowerThreadInterface.Interface.Version = 1;
    m_PowerThreadInterface.Interface.Context = this;
    m_PowerThreadInterface.Interface.InterfaceReference = (PINTERFACE_REFERENCE)&FxPkgPnp::_PowerThreadInterfaceReference;
    m_PowerThreadInterface.Interface.InterfaceDereference = (PINTERFACE_DEREFERENCE)&FxPkgPnp::_PowerThreadInterfaceDereference;
    m_PowerThreadInterface.PowerThreadEnqueue = &FxPkgPnp::_PowerThreadEnqueue;
    m_PowerThread = NULL;
    m_HasPowerThread = FALSE;
    m_PowerThreadInterfaceReferenceCount = 1;
    m_PowerThreadEvent = NULL;

    m_DeviceStopCount = 0;
    m_CapsQueried = FALSE;
    m_InternalFailure = FALSE;
    m_FailedAction = WdfDeviceFailedUndefined;

    //
    // We only set the pending child count to 1 once we know we have successfully
    // created an FxDevice and initialized it fully.  If we delete an FxDevice
    // which is half baked, it cannot have any FxChildLists which have any
    // pending children on them.
    //
    m_PendingChildCount = 0;

    m_QueryInterfaceHead.Next = NULL;

    m_DeviceInterfaceHead.Next = NULL;
    m_DeviceInterfacesCanBeEnabled = FALSE;

    m_Failed = FALSE;
    m_SetDeviceRemoveProcessed = FALSE;

    m_SystemPowerState    = PowerSystemWorking;
    m_DevicePowerState    = WdfPowerDeviceD3Final;
    m_DevicePowerStateOld = WdfPowerDeviceD3Final;

    m_PendingPnPIrp = NULL;
    m_PendingSystemPowerIrp = NULL;
    m_PendingDevicePowerIrp = NULL;
    m_SystemPowerAction = (UCHAR) PowerActionNone;

    m_PnpStateCallbacks = NULL;
    m_PowerStateCallbacks = NULL;
    m_PowerPolicyStateCallbacks = NULL;

    m_SelfManagedIoMachine = NULL;

    m_EnumInfo = NULL;

    m_Resources = NULL;
    m_ResourcesRaw = NULL;

    InitializeListHead(&m_InterruptListHead);
    m_InterruptObjectCount = 0;
    m_WakeInterruptCount = 0;
    m_WakeInterruptPendingAckCount = 0;
    m_SystemWokenByWakeInterrupt = FALSE;
    m_WakeInterruptsKeepConnected = FALSE;
    m_AchievedStart = FALSE;

    m_SharedPower.m_WaitWakeIrp = NULL;
    m_SharedPower.m_WaitWakeOwner = FALSE;
    m_SharedPower.m_ExtendWatchDogTimer = FALSE;

    m_DeviceRemoveProcessed = NULL;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // Interrupt APIs for Vista and forward
    //
    //m_IoConnectInterruptEx      = FxLibraryGlobals.IoConnectInterruptEx;
    //m_IoDisconnectInterruptEx   = FxLibraryGlobals.IoDisconnectInterruptEx;

    //
    // Interrupt APIs for Windows 8 and forward
    //
    //m_IoReportInterruptActive   = FxLibraryGlobals.IoReportInterruptActive;
    //m_IoReportInterruptInactive = FxLibraryGlobals.IoReportInterruptInactive;

#endif

    m_ReleaseHardwareAfterDescendantsOnFailure = FALSE;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //m_SleepStudy = NULL;
    m_SleepStudyPowerRefIoCount = 0;

    //
    // Sleep Study relies on other OS components that do not start as early as
    // WDF. We automatically track references until we have determined if
    // Sleep Study is enabled for this driver.
    //
    m_SleepStudyTrackReferences = TRUE;

    //m_CompanionTarget = NULL;
    //m_CompanionTargetStatus = STATUS_NOT_FOUND;

    m_SetDeviceFailedAttemptRestartWorkItem = NULL;
#endif

    MarkDisposeOverride(ObjectDoNotLock);
}

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
FxPkgPnp::CompletePowerRequest(
    __inout FxIrp* Irp,
    __in    NTSTATUS Status
    )
{
    MdIrp irp;

    //
    // Once we call CompleteRequest, 2 things happen
    // 1) this object may go away
    // 2) Irp->m_Irp will be cleared
    //
    // As such, we capture the underlying WDM objects so that we can use them
    // to release the remove lock and use the PIRP *value* as a tag to release
    // the remlock.
    //
    irp = Irp->GetIrp();

    Irp->SetStatus(Status);
    Irp->StartNextPowerIrp();
    Irp->CompleteRequest(IO_NO_INCREMENT);

    Mx::MxReleaseRemoveLock(m_Device->GetRemoveLock(),
                            irp);

    return Status;
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

VOID
FxPkgPnp::FinishInitialize(
    __inout PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:
    Finish initializing the object.  All initialization up until this point
    could fail.  This function cannot fail, so all it can do is assign field
    values and take allocations from DeviceInit.

Arguments:
    DeviceInit - device initialization structure that the driver writer has
                 initialized

Return Value:
    None

  --*/

{
    //
    // Reassign the state callback arrays away from the init struct.
    // Set the init field to NULL so that it does not attempt to free the array
    // when it is destroyed.
    //
    m_PnpStateCallbacks = DeviceInit->PnpPower.PnpStateCallbacks;
    DeviceInit->PnpPower.PnpStateCallbacks = NULL;

    m_PowerStateCallbacks = DeviceInit->PnpPower.PowerStateCallbacks;
    DeviceInit->PnpPower.PowerStateCallbacks = NULL;

    m_PowerPolicyStateCallbacks = DeviceInit->PnpPower.PowerPolicyStateCallbacks;
    DeviceInit->PnpPower.PowerPolicyStateCallbacks = NULL;

    //
    // Bias the count towards one so that we can optimize the synchronous
    // cleanup case when the device is being destroyed.
    //
    m_PendingChildCount = 1;

    //
    // Now "Add" this device in the terms that the PnP state machine uses.  This
    // will be in the context of an actual AddDevice function for FDOs, and
    // something very much like it for PDOs.
    //
    // Important that the posting of the event is after setting of the state
    // callback arrays so that we can guarantee that any state transition
    // callback will be made.
    //
    PnpProcessEvent(PnpEventAddDevice);
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::Dispatch(
    __in MdIrp Irp
    )

/*++

Routine Description:

    This is the main dispatch handler for the pnp package.  This method is
    called by the framework manager when a PNP or Power IRP enters the driver.
    This function will dispatch the IRP to a function designed to handle the
    specific IRP.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    NTSTATUS status;
    FxIrp irp(Irp);

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    FX_TRACK_DRIVER(GetDriverGlobals());
#endif

    if (irp.GetMajorFunction() == IRP_MJ_PNP)
    {

        switch (irp.GetMinorFunction()) {
        case IRP_MN_START_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_EJECT:
        case IRP_MN_QUERY_PNP_DEVICE_STATE:        
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p, IRP_MJ_PNP, %!pnpmn! IRP 0x%p",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                irp.GetMinorFunction(), irp.GetIrp());
                
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p, IRP_MJ_PNP, %!pnpmn! "
                "type %!DEVICE_RELATION_TYPE! IRP 0x%p",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                irp.GetMinorFunction(),
                irp.GetParameterQDRType(), irp.GetIrp());
            
            break;

        default:
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p, IRP_MJ_PNP, %!pnpmn! IRP 0x%p",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                irp.GetMinorFunction(), irp.GetIrp());
            break;
        }

        if (irp.GetMinorFunction() <= IRP_MN_SURPRISE_REMOVAL)
        {
            status = (*GetDispatchPnp()[irp.GetMinorFunction()])(this, &irp);
        }
        else
        {
            //
            // For Pnp IRPs we don't understand, just forget about them
            //
            status = FireAndForgetIrp(&irp);
        }
    }
    else
    {
        //
        // If this isn't a PnP Irp, it must be a power irp.
        //
        switch (irp.GetMinorFunction()) {
        case IRP_MN_WAIT_WAKE:
        case IRP_MN_SET_POWER:
            if (irp.GetParameterPowerType() == SystemPowerState)
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                    "WDFDEVICE 0x%p !devobj 0x%p IRP_MJ_POWER, %!pwrmn! "
                    "IRP 0x%p for %!SYSTEM_POWER_STATE! (S%d)",
                    m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    irp.GetMinorFunction(), irp.GetIrp(),
                    irp.GetParameterPowerStateSystemState(),
                    irp.GetParameterPowerStateSystemState() - 1);
            }
            else
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                    "WDFDEVICE 0x%p !devobj 0x%p IRP_MJ_POWER, %!pwrmn! "
                    "IRP 0x%p for %!DEVICE_POWER_STATE!",
                    m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    irp.GetMinorFunction(), irp.GetIrp(),
                    irp.GetParameterPowerStateDeviceState());
            }
            break;

        default:
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p IRP_MJ_POWER, %!pwrmn! IRP 0x%p",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                irp.GetMinorFunction(), irp.GetIrp());
            break;
        }

        Mx::MxAssert(irp.GetMajorFunction() == IRP_MJ_POWER);

        if (irp.GetMinorFunction() <= IRP_MN_QUERY_POWER)
        {
            status = (*GetDispatchPower()[irp.GetMinorFunction()])(this, &irp);
        }
        else
        {
            //
            // For Power IRPs we don't understand, just forget about them
            //
            status = FireAndForgetIrp(&irp);
        }
    }

    return status;
}

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
NTSTATUS
FxPkgPnp::AllocateWorkItemForSetDeviceFailed(
    VOID
    )
{
    NTSTATUS status;

    if (m_SetDeviceFailedAttemptRestartWorkItem != NULL)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "Reusing previously created workitem for"
            "SetDeviceFailedAttemptRestart");
        return STATUS_SUCCESS;
    }

    status = FxSystemWorkItem::_Create(GetDriverGlobals(),
                                       m_Device->GetDeviceObject(),
                                       &m_SetDeviceFailedAttemptRestartWorkItem
                                       );
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not allocate workitem for "
            "SetDeviceFailedAttemptRestart: %!STATUS!", status);
    }

    return status;
}

VOID
FxPkgPnp::RemoveWorkItemForSetDeviceFailed(
    VOID
    )
{
    if (m_SetDeviceFailedAttemptRestartWorkItem != NULL)
    {
        m_SetDeviceFailedAttemptRestartWorkItem->DeleteObject();
        m_SetDeviceFailedAttemptRestartWorkItem = NULL;
    }
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgPnp::Initialize(
    __in PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This function initializes state associated with an instance of FxPkgPnp.
    This differs from the constructor because it can do operations which
    will fail, and can return failure.  (Constructors can't fail, they can
    only throw exceptions, which we can't deal with in this kernel mode
    environment.)

Arguments:

    DeviceInit - Struct that the driver initialized that contains defaults.

Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    // WDF 1.11
    //m_ReleaseHardwareAfterDescendantsOnFailure = (DeviceInit->ReleaseHardwareOrderOnFailure ==
    //            WdfReleaseHardwareOrderOnFailureAfterDescendants);

    status = m_QueryInterfaceLock.Initialize();
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize QueryInterfaceLock for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }

    status = m_DeviceInterfaceLock.Initialize();
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize DeviceInterfaceLock for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }

    //
    // Initialize preallocated events for UM
    // (For KM, events allocated on stack are used since event initialization
    // doesn't fail in KM)
    //
#if (FX_CORE_MODE==FX_CORE_USER_MODE)

    status = m_CleanupEventUm.Initialize();
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize cleanup event for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }

    status = m_RemoveEventUm.Initialize(SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize remove event for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }
#endif

    if (DeviceInit->IsPwrPolOwner())
    {
        m_PowerPolicyMachine.m_Owner = new (pFxDriverGlobals)
            FxPowerPolicyOwnerSettings(this);

        if (m_PowerPolicyMachine.m_Owner == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = m_PowerPolicyMachine.m_Owner->Init();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        QueryForD3ColdInterface();
#endif
    }

    //
    // we will change the access flags on the object later on when we build up
    // the list from the wdm resources
    //
    status = FxCmResList::_CreateAndInit(&m_Resources,
                                         pFxDriverGlobals,
                                         m_Device,
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         FxResourceNoAccess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = m_Resources->Commit(WDF_NO_OBJECT_ATTRIBUTES,
                                 WDF_NO_HANDLE,
                                 m_Device);

    //
    // This should never fail
    //
    ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        m_Resources->DeleteFromFailedCreate();
        m_Resources = NULL;
        return status;
    }

    m_Resources->ADDREF(this);

    //
    // we will change the access flags on the object later on when we build up
    // the list from the wdm resources
    //
    status = FxCmResList::_CreateAndInit(&m_ResourcesRaw,
                                         pFxDriverGlobals,
                                         m_Device,
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         FxResourceNoAccess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = m_ResourcesRaw->Commit(WDF_NO_OBJECT_ATTRIBUTES,
                                    WDF_NO_HANDLE,
                                    m_Device);

    //
    // This should never fail
    //
    ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        m_ResourcesRaw->DeleteFromFailedCreate();
        m_ResourcesRaw = NULL;
        return status;
    }

    m_ResourcesRaw->ADDREF(this);

    status = RegisterCallbacks(&DeviceInit->PnpPower.PnpPowerEventCallbacks);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (IsPowerPolicyOwner())
    {
        RegisterPowerPolicyCallbacks(&DeviceInit->PnpPower.PolicyEventCallbacks);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::AllocateEnumInfo(
    VOID
    )
{
    KIRQL irql;
    NTSTATUS status;

    if (m_EnumInfo != NULL)
    {
        return STATUS_SUCCESS;
    }

    Lock(&irql);
    if (m_EnumInfo == NULL)
    {
        m_EnumInfo = new (GetDriverGlobals()) FxEnumerationInfo(GetDriverGlobals());

        if (m_EnumInfo != NULL)
        {
            status = m_EnumInfo->Initialize();

            if (!NT_SUCCESS(status))
            {
                delete m_EnumInfo;
                m_EnumInfo = NULL;

                DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                                    TRACINGPNP,
                                    "Could not initialize enum info for "
                                    "WDFDEVICE %p, %!STATUS!",
                                    m_Device->GetHandle(), status);
            }

        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                                "Could not allocate enum info for WDFDEVICE %p, "
                                "%!STATUS!", m_Device->GetHandle(), status);
        }
    }
    else
    {
        //
        // another thread allocated the list already
        //
        status = STATUS_SUCCESS;
    }
    Unlock(irql);

    return status;
}

VOID
NTAPI
FxPkgPnp::_SetPowerCapState(
    __in  ULONG Index,
    __in  DEVICE_POWER_STATE State,
    __out PULONG Result
    )
/*++

Routine Description:
    Encodes the given device power state (State) into Result at the given Index.
    States are encoded in nibbles (4 bit chunks), starting at the bottom of the
    result and  moving upward

Arguments:
    Index - zero based index into the number of nibbles to encode the value

    State - State to encode

    Result - pointer to where the encoding will take place

Return Value:
    None

  --*/
{
    //
    // We store off state in 4 bits, starting at the lowest byte
    //
    ASSERT(Index < 8);

    //
    // Erase the old value
    //
    *Result &= ~(0xF << (Index * 4));

    //
    // Write in the new one
    //
    *Result |= (0xF & State) << (Index * 4);
}

VOID
FxPkgPnp::AddChildList(
    __in FxChildList* List
    )
{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Adding FxChildList %p, WDFCHILDLIST %p", List,
                        List->GetHandle());

    m_EnumInfo->m_ChildListList.Add(GetDriverGlobals(),
                                    &List->m_TransactionLink);
}

VOID
FxPkgPnp::QueryForD3ColdInterface(
    VOID
    )
{
    MxDeviceObject deviceObject;
    PDEVICE_OBJECT topOfStack;
    PDEVICE_OBJECT pdo;
    FxAutoIrp irp;
    NTSTATUS status;

    //
    // This function can be invoked multiple times, particularly if filters
    // send IRP_MN_QUERY_CAPABILITIES.  So bail out if the interface has already
    // been acquired.
    //

    /*if ((m_D3ColdInterface.InterfaceDereference != NULL) ||
        (m_D3ColdInterface.GetIdleWakeInfo != NULL) ||
        (m_D3ColdInterface.SetD3ColdSupport != NULL))
    {
        return;
    }*/

    pdo = m_Device->GetPhysicalDevice();

    if (pdo == NULL)
    {
        return;
    }

    //
    // Get the top of stack device object, even though normal filters and the
    // FDO may not have been added to the stack yet to ensure that this
    // query-interface is seen by bus filters.  Specifically, in a PCI device
    // which is soldered to the motherboard, ACPI will be on the stack and it
    // needs to see this IRP.
    //
    topOfStack = IoGetAttachedDeviceReference(pdo);
    deviceObject.SetObject(topOfStack);

    if (deviceObject.GetObject() != NULL)
    {
        irp.SetIrp(FxIrp::AllocateIrp(deviceObject.GetStackSize()));
        if (irp.GetIrp() == NULL)
        {

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Failed to allocate IRP to get D3COLD_SUPPORT_INTERFACE from !devobj %p", 
                pdo);
        }
        else
        {
            //
            // Initialize the Irp
            //
            irp.SetStatus(STATUS_NOT_SUPPORTED);

            irp.ClearNextStack();
            irp.SetMajorFunction(IRP_MJ_PNP);
            irp.SetMinorFunction(IRP_MN_QUERY_INTERFACE);
            //irp.SetParameterQueryInterfaceType(&GUID_D3COLD_SUPPORT_INTERFACE);
            //irp.SetParameterQueryInterfaceVersion(D3COLD_SUPPORT_INTERFACE_VERSION);
            //irp.SetParameterQueryInterfaceSize(sizeof(m_D3ColdInterface));
            irp.SetParameterQueryInterfaceInterfaceSpecificData(NULL);
            //irp.SetParameterQueryInterfaceInterface((PINTERFACE)&m_D3ColdInterface);

            status = irp.SendIrpSynchronously(deviceObject.GetObject());

            if (!NT_SUCCESS(status))
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "!devobj %p declined to supply D3COLD_SUPPORT_INTERFACE", 
                    pdo);

                //RtlZeroMemory(&m_D3ColdInterface, sizeof(m_D3ColdInterface));
            }
        }
    }
    ObDereferenceObject(topOfStack);
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::RegisterCallbacks(
    __in PWDF_PNPPOWER_EVENT_CALLBACKS DispatchTable
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN useSmIo;

    useSmIo = FALSE;

    //
    // Update the callback table.
    //
    m_DeviceD0Entry.Initialize(this, DispatchTable->EvtDeviceD0Entry);
    m_DeviceD0Exit.Initialize(this,  DispatchTable->EvtDeviceD0Exit);
    m_DevicePrepareHardware.Initialize(this,
                DispatchTable->EvtDevicePrepareHardware);
    m_DeviceReleaseHardware.Initialize(this,
                DispatchTable->EvtDeviceReleaseHardware);
    m_DeviceSurpriseRemoval.Initialize(this,
                DispatchTable->EvtDeviceSurpriseRemoval);

    m_DeviceD0EntryPostInterruptsEnabled.m_Method =
        DispatchTable->EvtDeviceD0EntryPostInterruptsEnabled;
    m_DeviceD0ExitPreInterruptsDisabled.m_Method =
        DispatchTable->EvtDeviceD0ExitPreInterruptsDisabled;

    m_DeviceQueryStop.m_Method         = DispatchTable->EvtDeviceQueryStop;
    m_DeviceQueryRemove.m_Method       = DispatchTable->EvtDeviceQueryRemove;

    m_DeviceUsageNotification.m_Method = DispatchTable->EvtDeviceUsageNotification;
    //m_DeviceUsageNotificationEx.m_Method = DispatchTable->EvtDeviceUsageNotificationEx;
    m_DeviceRelationsQuery.m_Method    = DispatchTable->EvtDeviceRelationsQuery;


    //
    // Now see if SMIO is being used
    //
    if (DispatchTable->EvtDeviceSelfManagedIoCleanup != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoFlush != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoInit != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoSuspend != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoRestart != NULL)
    {

        useSmIo = TRUE;
    }
    else if (GetDevice()->IsCxUsingSelfManagedIo())
    {
        useSmIo = TRUE;
    }


    if (useSmIo)
    {
        status = FxSelfManagedIoMachine::_CreateAndInit(&m_SelfManagedIoMachine,
                                                        this);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        m_SelfManagedIoMachine->InitializeMachine(DispatchTable);
    }

    return status;
}

VOID
FxPkgPnp::RegisterPowerPolicyCallbacks(
    __in PWDF_POWER_POLICY_EVENT_CALLBACKS Callbacks
    )
{
    m_PowerPolicyMachine.m_Owner->m_DeviceArmWakeFromS0.m_Method =
        Callbacks->EvtDeviceArmWakeFromS0;
    m_PowerPolicyMachine.m_Owner->m_DeviceArmWakeFromSx.m_Method =
        Callbacks->EvtDeviceArmWakeFromSx;
    m_PowerPolicyMachine.m_Owner->m_DeviceArmWakeFromSx.m_MethodWithReason =
        Callbacks->EvtDeviceArmWakeFromSxWithReason;

    m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromS0.m_Method =
        Callbacks->EvtDeviceDisarmWakeFromS0;
    m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromSx.m_Method =
        Callbacks->EvtDeviceDisarmWakeFromSx;

    m_PowerPolicyMachine.m_Owner->m_DeviceWakeFromS0Triggered.m_Method =
        Callbacks->EvtDeviceWakeFromS0Triggered;
    m_PowerPolicyMachine.m_Owner->m_DeviceWakeFromSxTriggered.m_Method =
        Callbacks->EvtDeviceWakeFromSxTriggered;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PostCreateDeviceInitialize(
    VOID
    )

/*++

Routine Description:

    This function does any initialization to this object which must be done
    after the underlying device object has been attached to the device stack,
    i.e. you can send IRPs down this stack now.

Arguments:

    none

Returns:

    NTSTATUS

--*/

{
    NTSTATUS status;

    status = m_PnpMachine.Init(this, &FxPkgPnp::_PnpProcessEventInner);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "PnP State Machine init failed, %!STATUS!",
                            status);
        return status;
    }

    status = m_PowerMachine.Init(this, &FxPkgPnp::_PowerProcessEventInner);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Power State Machine init failed, %!STATUS!",
                            status);
        return status;
    }

    status = m_PowerPolicyMachine.Init(this, &FxPkgPnp::_PowerPolicyProcessEventInner);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Power Policy State Machine init failed, %!STATUS!",
                            status);
        return status;
    }

/*
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    FxCompanionLibrary* companionLib = FxLibraryGlobals.CompanionLibrary;
    PCWSTR companionName = NULL;

    //
    // Check if a companion is needed
    //
    if (companionLib->IsCompanionRequiredForDevice(
                                        m_Device,
                                        &companionName)) {

        NTSTATUS companionTargetStatus;

        //
        // We dont want to fail WdfDeviceCreate so we dont propagate the
        // failure. Also, upon failure AllocateCompanionTarget will write a error trace
        // message.
        //

        companionTargetStatus = m_Device->AllocateCompanionTarget(&m_CompanionTarget);

        if (NT_SUCCESS(companionTargetStatus)) {
            //
            // Take a reference that will be released in ~FxPkgPnp
            //
            m_CompanionTarget->ADDREF(this);
        }
        else {
            WCHAR insertString[EVTLOG_MESSAGE_SIZE];
            if (NT_SUCCESS(RtlStringCchPrintfW(insertString,
                            RTL_NUMBER_OF(insertString),
                            L"Service:%S, Companion:%s, Status:0x%x",
                            GetDriverGlobals()->Public.DriverName,
                            companionName,
                            companionTargetStatus))) {

                LibraryLogEvent(FxLibraryGlobals.DriverObject,
                    WDFVER_DRIVER_COMPANION_FAIL_TO_LOAD,
                    companionTargetStatus,
                    insertString,
                    NULL,
                    0);
            }
        }

        m_CompanionTargetStatus = companionTargetStatus;
    }

    if (companionName != NULL) {
        FxPoolFree((PVOID)companionName);
    }
#endif
*/

    return status;
}

VOID
FxPkgPnp::SaveState(
    __in BOOLEAN UseCanSaveState
    )
/*++

Routine Description:
    Saves any permanent state of the device out to the registry

Arguments:
    None

Return Value:
    None

  --*/

{
    UNICODE_STRING name;
    FxAutoRegKey hKey;
    NTSTATUS status;
    ULONG value;

    //
    // We only have settings to save if we are the power policy owner
    //
    if (IsPowerPolicyOwner() == FALSE)
    {
        return;
    }

    if (UseCanSaveState &&
        m_PowerPolicyMachine.m_Owner->m_CanSaveState == FALSE)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Not saving wake settings for WDFDEVICE %p due to system power "
            "transition", m_Device->GetHandle());
        
        return;
    }

    //
    // Check to see if there is anything to write out
    //
    if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.Dirty == FALSE &&
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty == FALSE)
    {
        return;
    }

    //
    // No need to write out if user control is not enabled
    //
    if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.Overridable == FALSE &&
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Overridable == FALSE)
    {
        return;
    }

    //
    // If the device is in paging path we should not be touching registry during
    // power up because it may incur page fault which won't be satisfied if the
    // device is still not powered up, blocking power Irp. User control state
    // change will not get written if any at this time but will be flushed out
    // to registry during device disable/remove in the remove path.
    //
    if (IsUsageSupported(DeviceUsageTypePaging) && IsDevicePowerUpIrpPending())
    {
        return;
    }

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))

    status = m_Device->OpenSettingsKey(&hKey.m_Key, STANDARD_RIGHTS_WRITE);
    if (!NT_SUCCESS(status))
    {
        return;
    }
#else
    status = STATUS_SUCCESS;
#endif

    if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.Overridable &&
        m_PowerPolicyMachine.m_Owner->m_IdleSettings.Dirty)
    {
        RtlInitUnicodeString(&name, WDF_S0_IDLE_ENABLED_VALUE_NAME);
        value = m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled;

        WriteStateToRegistry(hKey.m_Key, &name, value);

        m_PowerPolicyMachine.m_Owner->m_IdleSettings.Dirty = FALSE;
    }

    if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.Overridable &&
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty)
    {
        RtlInitUnicodeString(&name, WDF_SX_WAKE_ENABLED_VALUE_NAME);
        value = m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled;

        WriteStateToRegistry(hKey.m_Key, &name, value);

        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty = FALSE;
    }
}

VOID
FxPkgPnp::WriteStateToRegistry(
    __in HANDLE RegKey,
    __in PUNICODE_STRING ValueName,
    __in ULONG Value
    )
{
    ZwSetValueKey(RegKey, ValueName, 0, REG_DWORD, &Value, sizeof(Value));
}

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_requiresIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
VOID
FxWatchdog::_WatchdogDpc(
    __in     PKDPC Dpc,
    __in_opt PVOID Context,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine's job is to crash the machine, attempting to get some data
    into the crashdump file (or minidump) about why the machine stopped
    responding during an attempt to put the machine to sleep.

Arguments:

    This - the instance of FxPkgPnp

Return Value:

    this routine never returns

--*/
{
    WDF_POWER_ROUTINE_TIMED_OUT_DATA data;
    FxWatchdog* pThis;
    CfxDevice* pDevice;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    pThis = (FxWatchdog*) Context;
    pDevice = pThis->m_PkgPnp->GetDevice();

    DoTraceLevelMessage(pDevice->GetDriverGlobals(),
                        TRACE_LEVEL_ERROR, TRACINGPNP,
                        "The driver failed to return from a callback routine "
                        "in a reasonable period of time.  This prevented the "
                        "machine from going to sleep or from hibernating.  The "
                        "machine crashed because that was the best way to get "
                        "data about the cause of the crash into a minidump file.");

    data.PowerState = pDevice->GetDevicePowerState();
    data.PowerPolicyState = pDevice->GetDevicePowerPolicyState();
    data.DeviceObject = reinterpret_cast<PDEVICE_OBJECT>(pDevice->GetDeviceObject());
    data.Device = pDevice->GetHandle();
    data.TimedOutThread = reinterpret_cast<PKTHREAD>(pThis->m_CallingThread);

    FxVerifierBugCheck(pDevice->GetDriverGlobals(),
                       WDF_POWER_ROUTINE_TIMED_OUT,
                       (ULONG_PTR) &data);
}

FxPkgPnp::~FxPkgPnp()
{
    PSINGLE_LIST_ENTRY ple;

    Mx::MxAssert(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //SleepStudyStop();

    /*if (m_CompanionTarget != NULL)
    {
        m_CompanionTarget->RELEASE(this);
    }*/
#endif

    //
    // We should either have zero pending children or we never made it out of
    // the init state during a failed WDFDEVICE create or failed EvtDriverDeviceAdd
    //
    Mx::MxAssert(m_PendingChildCount == 0 ||
           m_Device->GetDevicePnpState() == WdfDevStatePnpInit);


    ple = m_DeviceInterfaceHead.Next;
    while (ple != NULL)
    {
        FxDeviceInterface* pDI;

        pDI = FxDeviceInterface::_FromEntry(ple);

        //
        // Advance to the next before deleting the current
        //
        ple = ple->Next;

        //
        // No longer in the list
        //
        pDI->m_Entry.Next = NULL;

        delete pDI;
    }
    m_DeviceInterfaceHead.Next = NULL;

    if (m_DmaEnablerList != NULL)
    {
        delete m_DmaEnablerList;
        m_DmaEnablerList = NULL;
    }

    if (m_RemovalDeviceList != NULL)
    {
        delete m_RemovalDeviceList;
        m_RemovalDeviceList = NULL;
    }

    if (m_UsageDependentDeviceList != NULL)
    {
        delete m_UsageDependentDeviceList;
        m_UsageDependentDeviceList = NULL;
    }

    if (m_PnpStateCallbacks != NULL)
    {
        delete m_PnpStateCallbacks;
    }

    if (m_PowerStateCallbacks != NULL)
    {
        delete m_PowerStateCallbacks;
    }

    if (m_PowerPolicyStateCallbacks != NULL)
    {
        delete m_PowerPolicyStateCallbacks;
    }

    if (m_SelfManagedIoMachine != NULL)
    {
        delete m_SelfManagedIoMachine;
        m_SelfManagedIoMachine = NULL;
    }

    if (m_EnumInfo != NULL)
    {
        delete m_EnumInfo;
        m_EnumInfo = NULL;
    }

    if (m_Resources != NULL)
    {
        m_Resources->RELEASE(this);
        m_Resources = NULL;
    }

    if (m_ResourcesRaw != NULL)
    {
        m_ResourcesRaw->RELEASE(this);
        m_ResourcesRaw = NULL;
    }

    ASSERT(IsListEmpty(&m_InterruptListHead));
}

VOID
NTAPI
FxPkgPnp::_PowerThreadInterfaceDereference(
    __inout PVOID Context
    )
/*++

Routine Description:
    Interface deref for the thread interface.  If this is the last reference
    released, an event is set so that the thread which waiting for the last ref
    to go away can unblock.

Arguments:
    Context - FxPkgPnp*

Return Value:
    None

  --*/

{
    FxPkgPnp* pThis;

    pThis = (FxPkgPnp*) Context;

    if (InterlockedDecrement(&pThis->m_PowerThreadInterfaceReferenceCount) == 0)
    {
        pThis->m_PowerThreadEvent->Set();
    }
}

VOID
NTAPI
FxPkgPnp::_PowerThreadInterfaceReference(
    __inout PVOID Context
    )
/*++

Routine Description:
    Increments the ref count on the thread interface.

Arguments:
    Context - FxPkgPnp*

Return Value:
    None

  --*/
{
    LONG count;

    count = InterlockedIncrement(
        &((FxPkgPnp*) Context)->m_PowerThreadInterfaceReferenceCount
        );

#if DBG
    ASSERT(count >= 2);
#else
    UNREFERENCED_PARAMETER(count);
#endif
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpStartDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:
    This method is called in response to a PnP StartDevice IRP coming down the
    stack.

Arguments:
    This - device instance
    Irp - a pointer to the FxIrp

Returns:
    STATUS_PENDING

--*/
{
    This->SetPendingPnpIrp(Irp);
    This->PnpProcessEvent(PnpEventStartDevice);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpQueryStopDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    Pnp callback querying to see if the device can be stopped.

    The Framework philosophy surrounding Query Stop (and Query Remove) is that
    it's impossible to really know if you can stop unless you've tried to stop.
    This may not always be true, but it's hard to find a general strategy that
    works that is less conservative.  Furthermore, I couldn't find good examples
    of drivers that would really benefit from continuing to handle requests
    until the actual Stop IRP arrived, particularly when you consider that
    most QueryStops are followed immediately by Stops.

    So this function sends an event to the PnP State machine that begins the
    stopping process.  If it is successful, then ultimately the QueryStop IRP
    will be successfully completed.

Arguments:

    This - a pointer to the PnP package

    Irp - a pointer to the FxIrp

Return Value:

    STATUS_PENDING

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpCancelStopDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    This routine is invoked in response to a query stop failing, somewhere in
    the stack.  Note that we can receive a cancel stop without being in the
    query stop state if a driver above us in the stack failed the query stop.

    Again, this function just exists to bridge the gap between the WDM IRPs
    and the PnP state machine.  This function does little more than send an
    event to the machine.

Arguments:

    This - the package

    Irp - a pointer to the FxIrp

Returns:

    STATUS_PENDING

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpStopDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    This method is invoked in response to a Pnp StopDevice IRP.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    STATUS_PENDING

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpQueryRemoveDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    Again, the Framework handles QueryRemove by stopping everything going on
    related to the device and then asking the driver whether it can be
    removed.  This function just kicks the state machine.  Final completion
    of the IRP will come (much) later.

Arguments:

    This - the package

    Irp - a pointer to the FxIrp

Returns:

    STATUS_PENDING

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpCancelRemoveDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    Notification of a previous remove being canceled.  Kick the state machine.

Arguments:

    This - the package

    Irp - FxIrp representing the notification

Return Value:

    STATUS_PENDING

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpRemoveDevice(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    Notification of a remove.  Kick the state machine.

Arguments:

    This - the package

    Irp - FxIrp representing the notification

Return Value:

    status

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_PnpDeviceUsageNotification(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
    //return This->PnpDeviceUsageNotification(Irp);
}

NTSTATUS
FxPkgPnp::FilterResourceRequirements(
    __in IO_RESOURCE_REQUIREMENTS_LIST **IoList
    )
/*++

Routine Description:

    This routine traverses one or more alternate _IO_RESOURCE_LISTs in the input
    IO_RESOURCE_REQUIREMENTS_LIST looking for interrupt descriptor and applies
    the policy set by driver in the interrupt object to the resource descriptor.

    LBI - Line based interrupt
    MSI - Message Signalled interrupt

    Here are the assumptions made about the order of descriptors.

    - An IoRequirementList can have one or more alternate IoResourceList
    - Each IoResourceList can have one or more resource descriptors
    - A descriptor can be default (unique), preferred, or alternate descriptors
    - A preferred descriptor can have zero or more alternate descriptors (P, A, A, A..)
    - In an IoResourceList, there can be one or more LBI descriptors
      (non-pci devices)(P,A,P,A)
    - In an IoResourceList, there can be only one preferred MSI 2.2
      (single or multi message) descriptor
    - In an IoResourceList, there cannot be MSI2.2 and MSI-X descriptors
    - In an IoResourceList, there can be one or more MSI-X descriptor
    - An alternate descriptor cannot be a very first descriptor in the list


    Now with that assumption, this routines parses the list looking for interrupt
    descriptor.

    - If it finds a LBI, it starts with the very first interrupt object and applies
      the policy set by the driver to the resource descriptor.
    - If it's finds an MSI2.2 then it starts with the first interrupt object and applies
      the policy. If the MSI2.2 is a multi-message one then it loops thru looking for
      as many interrupt object as there are messages. It doesn't fail the IRP, if the
      interrupt objects are less than the messages.
    - If there is an alternate descriptor then it applies the same policy from the
      interrupt object that it used for the preceding preferred descriptor.
    - Framework always uses FULLY_SPECIFIED connection type for both LBI and MSI
      interrupts including MSI-X
    - Framework will apply the policy on the descriptor set by the driver only
      if the policy is already not included in the resource descriptor. This is
      to allow the policy set in the registry to take precedence over the hard
      coded driver policy.
    - If the driver registers filter resource requirement and applies the policy
      on its own (by escaping to WDM) then framework doesn't override that.

Arguments:

    IoList - Pointer to the list part of an IRP_MN_FILTER_RESOURCE_REQUIREMENTS.

Return Value:

    NTSTATUS

--*/
{
    ULONG altResListIndex;
    PIO_RESOURCE_REQUIREMENTS_LIST pIoRequirementList;
    PIO_RESOURCE_LIST pIoResList;

    pIoRequirementList = *IoList;

    if (pIoRequirementList == NULL)
    {
        return STATUS_SUCCESS;
    }

    if (IsListEmpty(&m_InterruptListHead))
    {
        //
        // No interrupt objects created to filter resource requirements.
        //
        return STATUS_SUCCESS;
    }

    pIoResList = pIoRequirementList->List;

    //
    // Parse one or more alternative resource lists.
    //
    for (altResListIndex = 0;
         altResListIndex < pIoRequirementList->AlternativeLists;
         altResListIndex++)
    {
        PLIST_ENTRY pIntListEntryForMSI;
        PLIST_ENTRY pIntListEntryForLBI;
        BOOLEAN multiMessageMSI22Found;
        BOOLEAN previousDescMSI;
        ULONG descIndex;

        multiMessageMSI22Found = FALSE;
        previousDescMSI = FALSE;

        pIntListEntryForMSI = &m_InterruptListHead;
        pIntListEntryForLBI = &m_InterruptListHead;

        //
        // Traverse each _IO_RESOURCE_LISTs looking for interrupt descriptors
        // and call FilterResourceRequirements method so that it can apply
        // policy set on the interrupt object into the resource-descriptor.
        //

        for (descIndex = 0; descIndex < pIoResList->Count; descIndex++)
        {
            ULONG messageCount;
            PIO_RESOURCE_DESCRIPTOR pIoDesc;
            FxInterrupt* pInterruptInstance;

            pIoDesc = &pIoResList->Descriptors[descIndex];

            switch (pIoDesc->Type) {
            case CmResourceTypeInterrupt:

                if (FxInterrupt::_IsMessageInterrupt(pIoDesc->Flags))
                {
                    previousDescMSI = TRUE;

                    //
                    // We will advance to the next interrupt object if the resource
                    // is not an alternate resource descriptor. A resource list can
                    // have a preferred and zero or more alternate resource descriptors
                    // for the same resource. We need to apply the same policy on the
                    // alternate desc that we applied on the preferred one in case one
                    // of the alernate desc is selected for this device. An alternate
                    // resource descriptor can't be the first descriptor in a list.
                    //
                    if ((pIoDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0)
                    {
                        pIntListEntryForMSI = pIntListEntryForMSI->Flink;
                    }

                    if (pIntListEntryForMSI == &m_InterruptListHead)
                    {
                        DoTraceLevelMessage(
                            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                            "Not enough interrupt objects created for MSI by WDFDEVICE 0x%p ",
                            m_Device->GetHandle());
                        break;
                    }

                    pInterruptInstance = CONTAINING_RECORD(pIntListEntryForMSI, FxInterrupt, m_PnpList);
                    messageCount = pIoDesc->u.Interrupt.MaximumVector - pIoDesc->u.Interrupt.MinimumVector + 1;

                    if (messageCount > 1)
                    {
                        //
                        //  PCI spec guarantees that there can be only one preferred/default
                        //  MSI 2.2 descriptor in a single list.
                        //
                        if ((pIoDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0)
                        {
#if DBG
                            ASSERT(multiMessageMSI22Found == FALSE);
#else
                            UNREFERENCED_PARAMETER(multiMessageMSI22Found);
#endif
                            multiMessageMSI22Found = TRUE;

                        }
                    }
                    else
                    {
                        //
                        //  This is either single message MSI 2.2 or MSI-X interrupts
                        //
                        DO_NOTHING();
                    }

                    pInterruptInstance->FilterResourceRequirements(pIoDesc);
                }
                else
                {
                    //
                    // We will advance to next interrupt object if the desc is not an alternate
                    // descriptor. For non PCI devices, the first LBI interrupt desc can't be an
                    // alternate descriptor.
                    //
                    if ((pIoDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0)
                    {
                        pIntListEntryForLBI = pIntListEntryForLBI->Flink;
                    }

                    //
                    // An LBI can be first alternate resource if there are preceding MSI(X) descriptors
                    // listed in the list. In that case, this descriptor is the alternate interrupt resource
                    // for all of the MSI messages. As a result, we will use the first interrupt object from
                    // the list if this ends up being assigned by the system instead of MSI.
                    //
                    if (previousDescMSI)
                    {
                        ASSERT(pIoDesc->Option & IO_RESOURCE_ALTERNATIVE);
                        pIntListEntryForLBI = m_InterruptListHead.Flink;
                        previousDescMSI = FALSE;
                    }

                    //
                    // There can be one or more LBI interrupts and each LBI interrupt
                    // could have zero or more alternate descriptors.
                    //
                    if (pIntListEntryForLBI == &m_InterruptListHead)
                    {
                        DoTraceLevelMessage(
                            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                            "Not enough interrupt objects created for LBI by WDFDEVICE 0x%p ",
                            m_Device->GetHandle());
                        break;
                    }

                    pInterruptInstance = CONTAINING_RECORD(pIntListEntryForLBI, FxInterrupt, m_PnpList);

                    pInterruptInstance->FilterResourceRequirements(pIoDesc);
                }

                break;

            default:
                break;
            }
        }

        //
        // Since the Descriptors is a variable length list, you cannot get to the next
        // alternate list by doing pIoRequirementList->List[altResListIndex].
        // Descriptors[descIndex] will now point to the end of the descriptor list.
        // If there is another alternate list, it would be begin there.
        //
        pIoResList = (PIO_RESOURCE_LIST) &pIoResList->Descriptors[descIndex];
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
NTAPI
FxPkgPnp::_DispatchWaitWake(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This the first-level dispatch routine for IRP_MN_WAIT_WAKE.  What one
    does with a WaitWake IRP depends very much on whether one is an FDO, a PDO
    or a filter.  So dispatch immediately to a subclassable function.

Arguments:

    This - the package

    Irp - pointer to FxIrp representing this notification

Return Value:

    status

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
    //return This->DispatchWaitWake(Irp);
}

VOID
FxPkgPnp::SetPendingPnpIrp(
    __inout FxIrp* Irp,
    __in    BOOLEAN MarkIrpPending
    )
{
    if (m_PendingPnPIrp != NULL )
    {
        FxIrp pendingIrp(m_PendingPnPIrp);

        //
        // A state changing pnp irp is already pended. If we don't bugcheck
        // the pended pnp irp will be overwritten with new pnp irp and the old
        // one may never get completed, which may have drastic implications (
        // unresponsive system, power manager not sending Sx Irp etc.)
        //
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "A new state changing pnp irp %!pnpmn! IRP %p arrived while another "
            "pnp irp %!pnpmn! IRP %p is still pending WDFDEVICE %p\n",
            Irp->GetMinorFunction(), Irp->GetIrp(),
            pendingIrp.GetMinorFunction(),pendingIrp.GetIrp(),
            m_Device->GetHandle());

        FxVerifierBugCheck(GetDriverGlobals(),  // globals
                           WDF_PNP_FATAL_ERROR, // specific type
                           (ULONG_PTR)m_Device->GetHandle(), //parm 2
                           (ULONG_PTR)Irp->GetIrp());  // parm 3

        /* NOTREACHED */
        return;
    }
    if (MarkIrpPending)
    {
        Irp->MarkIrpPending();
    }
    m_PendingPnPIrp = Irp->GetIrp();
}

VOID
FxPkgPnp::SetInternalFailure(
    VOID
    )
/*++

Routine Description:
    Sets the failure field and then optionally invalidates the device state.

Arguments:
    InvalidateState - If TRUE, the state is invalidated

Return Value:
    None

  --*/
{
    m_InternalFailure = TRUE;

    MxDeviceObject physicalDeviceObject(
                                m_Device->GetPhysicalDevice()
                                );
    physicalDeviceObject.InvalidateDeviceState(
        m_Device->GetDeviceObject()
        );
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::QueryForCapabilities(
    VOID
    )
{
    DEVICE_CAPABILITIES caps;
    NTSTATUS status;

    MxDeviceObject deviceObject;

    deviceObject.SetObject(m_Device->GetDeviceObject());

    status = GetStackCapabilities(GetDriverGlobals(),
                                  &deviceObject,                                  
                                  &caps);

    if (NT_SUCCESS(status))
    {
        ULONG states, i;
        
        ASSERT(caps.DeviceWake <= 0xFF && caps.SystemWake <= 0xFF);
        
        m_SystemWake = (BYTE) caps.SystemWake;

        //
        // Capture the S -> D state mapping table as a ULONG for use in the
        // power policy owner state machine when the machine moves into Sx and
        // the device is not armed for wake and has set an IdealDxStateForSx
        // value
        //
        states = 0x0;

        for (i = 0; i < ARRAY_SIZE(caps.DeviceState); i++)
        {
            _SetPowerCapState(i,  caps.DeviceState[i], &states);
        }

        m_PowerPolicyMachine.m_Owner->m_SystemToDeviceStateMap = states;
        
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::CreatePowerThread(
    VOID
    )
/*++

Routine Description:
    Creates a power thread for the device node.  This thread is share among all
    the devices in the stack through the POWER_THREAD_INTERFACE structure and
    PnP query interface.

Arguments:
    None

Return Value:
    NTSTATUS

  --*/
{
    FxSystemThread *pThread, *pOld;
    NTSTATUS status;

    status = FxSystemThread::_CreateAndInit(
                &pThread,
                GetDriverGlobals(),
                m_Device->GetHandle(),
                m_Device->GetDeviceObject());

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Simple locking logic in case N requests are conncurrent.  (The requests
    // never should be concurrent, but in the case that there are 2 usage
    // notifications coming in from two different sources, it could
    // theoritically happen.)
    //
    pOld = (FxSystemThread*) InterlockedCompareExchangePointer(
        (PVOID*) &m_PowerThread, pThread, NULL);

    if (pOld != NULL)
    {
        //
        // Someone also set the thread pointer value at the same time, free
        // our new one here.
        //
        pThread->ExitThread();
        pThread->DeleteObject();
    }

    m_HasPowerThread = TRUE;

    return STATUS_SUCCESS;
}

LONG
FxPkgPnp::GetPnpCapsInternal(
    VOID
    )
/*++

Routine Description:
    Returns the pnp device capabilities encoded into a LONG.  This state is used
    in reporting device capabilities via IRP_MN_QUERY_CAPABILITIES and filling
    in the PDEVICE_CAPABILITIES structure.

Arguments:
    None

Return Value:
    the current pnp cap bits

  --*/
{
    LONG caps;
    KIRQL irql;

    Lock(&irql);
    caps = m_PnpStateAndCaps.Value & FxPnpCapMask;
    Unlock(irql);

    return caps;
}

DEVICE_POWER_STATE
FxPkgPnp::_GetPowerCapState(
    __in ULONG Index,
    __in ULONG State
    )
/*++

Routine Description:
    Decodes our internal device state encoding and returns a normalized device
    power state for the given index.

Arguments:
    Index - nibble (4 bit chunk) index into the State

    State - value which has the device states encoded into it

Return Value:
    device power state for the given Index

  --*/
{
    ASSERT(Index < 8);
                                // isolate the value            and normalize it
    return (DEVICE_POWER_STATE) ((State & (0xF << (Index * 4))) >> (Index * 4));
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::NotifyResourceObjectsD0(
    __in ULONG NotifyFlags
    )
/*++

Routine Description:

    This routine traverses all resource objects and tells them that the device
    is entering D0.  If an error is encountered, the walking of the list is
    halted.

Arguments:

    none

Return Value:

    VOID

--*/
{
    FxInterrupt* pInterrupt;
    PLIST_ENTRY ple;
    NTSTATUS status;

    for (ple = m_InterruptListHead.Flink;
         ple != &m_InterruptListHead;
         ple = ple->Flink)
    {
        //
        // Connect the interrupts
        //
        pInterrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

        status = pInterrupt->Connect(NotifyFlags);

        if (!NT_SUCCESS(status))
        {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                                "WDFINTERRUPT %p failed to connect, %!STATUS!",
                                pInterrupt->GetHandle(), status);

            return status;
        }
    }

    return STATUS_SUCCESS;
}

PNP_DEVICE_STATE
FxPkgPnp::HandleQueryPnpDeviceState(
    __in PNP_DEVICE_STATE PnpDeviceState
    )

/*++

Routine Description:

    This function handled IRP_MN_QUERY_DEVICE_STATE.  Most of the bits are
    just copied from internal Framework state.

Arguments:

    PnpDeviceState - Bitfield that will be returned to the sender of the IRP.

Returns:

    NTSTATUS

--*/

{
    LONG state;

    state = GetPnpStateInternal();

    //
    // Return device state set by driver.
    //
    SET_PNP_DEVICE_STATE_BIT(&PnpDeviceState,
                             PNP_DEVICE_DISABLED,
                             state,
                             Disabled);
    SET_PNP_DEVICE_STATE_BIT(&PnpDeviceState,
                             PNP_DEVICE_DONT_DISPLAY_IN_UI,
                             state,
                             DontDisplayInUI);
    SET_PNP_DEVICE_STATE_BIT(&PnpDeviceState,
                             PNP_DEVICE_FAILED,
                             state,
                             Failed);
    SET_PNP_DEVICE_STATE_BIT(&PnpDeviceState,
                             PNP_DEVICE_NOT_DISABLEABLE,
                             state,
                             NotDisableable);
    SET_PNP_DEVICE_STATE_BIT(&PnpDeviceState,
                             PNP_DEVICE_REMOVED,
                             state,
                             Removed);
    SET_PNP_DEVICE_STATE_BIT(&PnpDeviceState,
                             PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED,
                             state,
                             ResourcesChanged);

    if ((state & FxPnpStateDontDisplayInUIMask) == FxPnpStateDontDisplayInUIUseDefault)
    {
        LONG caps;

        //
        // Mask off all caps except for NoDispalyInUI
        //
        caps = GetPnpCapsInternal() & FxPnpCapNoDisplayInUIMask;

        //
        // If the driver didn't specify pnp state, see if they specified no
        // display as a capability.  For raw PDOs and just usability, it is not
        // always clear to the driver writer that they must set both the pnp cap
        // and the pnp state for the no display in UI property to stick after
        // the device has been started.
        //
        if (caps == FxPnpCapNoDisplayInUITrue)
        {
            PnpDeviceState |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
        }
        else if (caps == FxPnpCapNoDisplayInUIFalse)
        {
            PnpDeviceState &= ~PNP_DEVICE_DONT_DISPLAY_IN_UI;
        }
    }

    //
    // Return device state maintained by frameworks.
    //
    if (IsInSpecialUse())
    {
        PnpDeviceState |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    //
    // If there is an internal failure, then indicate that up to pnp.
    //
    if (m_InternalFailure || m_Failed)
    {
        PnpDeviceState |= PNP_DEVICE_FAILED;
    }

    return PnpDeviceState;
}

LONG
FxPkgPnp::GetPnpStateInternal(
    VOID
    )
/*++

Routine Description:
    Returns the pnp device state encoded into a ULONG.  This state is the state
    that is reported to PNp via IRP_MN_QUERY_PNP_DEVICE_STATE after it has been
    decoded into the bits pnp expects

Arguments:
    None

Return Value:
    the current state bits

  --*/
{
    LONG state;
    KIRQL irql;

    //
    // State is shared with the caps bits.  Use a lock to guard against
    // corruption of the value between these 2 values
    //
    Lock(&irql);
    state = m_PnpStateAndCaps.Value & FxPnpStateMask;
    Unlock(irql);

    return state;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::HandleQueryBusRelations(
    __inout FxIrp* Irp
    )
/*++

Routine Description:
    Handles a query device relations for the bus relations type (all other types
    are handled by HandleQueryDeviceRelations).  This function will call into
    each of the device's FxChildList objects to process the request.

Arguments:
    Irp - the request contain the query device relations

Return Value:
    NTSTATUS

  --*/
{
    FxWaitLockTransactionedList* pList;
    PDEVICE_RELATIONS pRelations;
    FxTransactionedEntry* ple;
    NTSTATUS status, listStatus;
    BOOLEAN changed;

    //
    // Before we do anything, callback into the driver
    //
    m_DeviceRelationsQuery.Invoke(m_Device->GetHandle(), BusRelations);

    //
    // Default to success unless list processing fails
    //
    status = STATUS_SUCCESS;

    //
    // Keep track of changes made by any list object.  If anything changes,
    // remember it for post-processing.
    //
    changed = FALSE;

    pRelations = (PDEVICE_RELATIONS) Irp->GetInformation();

    if (m_EnumInfo != NULL)
    {
        pList = &m_EnumInfo->m_ChildListList;

        pList->LockForEnum(GetDriverGlobals());
    }
    else
    {
        pList = NULL;
    }

    ple = NULL;
    while (pList != NULL && (ple = pList->GetNextEntry(ple)) != NULL)
    {
        FxChildList* pChildList;

        pChildList = FxChildList::_FromEntry(ple);

        //
        // ProcessBusRelations will free and reallocate pRelations if necessary
        //
        listStatus = pChildList->ProcessBusRelations(&pRelations);

        //
        // STATUS_NOT_SUPPORTED is a special value.  It indicates that the call
        // to ProcessBusRelations did not modify pRelations in any way.
        //
        if (listStatus == STATUS_NOT_SUPPORTED)
        {
            continue;
        }


        if (!NT_SUCCESS(listStatus))
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p, WDFCHILDLIST %p returned %!STATUS! from "
                "processing bus relations",
                m_Device->GetHandle(), pChildList->GetHandle(), listStatus);
            status = listStatus;
            break;
        }

        //
        // We updated pRelations, change the status later
        //
        changed = TRUE;
    }

    //
    // By checking for NT_SUCCESS(status) below we account for
    // both the cases - list changed, as well as list unchanged but possibly
    // children being reported missing (that doesn't involve list change).
    //
    
    //
    // InvokeReportedMissingCallback used PFN_WDF_DEVICE_REPORTED_MISSING, added in WDF 1.11
    //
    /*if (NT_SUCCESS(status))
    {
        ple = NULL;
        while (pList != NULL && (ple = pList->GetNextEntry(ple)) != NULL)
        {
            FxChildList* pChildList;

            pChildList = FxChildList::_FromEntry(ple);

            //
            // invoke the ReportedMissing callback for for children that are
            // being reporte missing
            //
            pChildList->InvokeReportedMissingCallback();
        }
    }*/

    if (pList != NULL)
    {
        pList->UnlockFromEnum(GetDriverGlobals());
    }

    if (NT_SUCCESS(status) && changed == FALSE)
    {
        //
        // Went through the entire list of FxChildList objects, but no object
        // modified the pRelations, so restore the caller's NTSTATUS.
        //
        status = Irp->GetStatus();
    }

    //
    // Re-set the relations into the structure so that any changes that any call
    // to FxChildList::ProcessBusRelations takes effect and is reported.
    //
    Irp->SetInformation((ULONG_PTR) pRelations);
    Irp->SetStatus(status);

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "WDFDEVICE %p, returning %!STATUS! from processing bus relations",
        m_Device->GetHandle(), status
        );

    if (NT_SUCCESS(status) && pRelations != NULL)
    {
        ULONG i;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p returning %d devices in relations %p",
            m_Device->GetHandle(), pRelations->Count, pRelations
            );

        //
        // Try to not consume an IFR entry per DO reported.  Instead, report up
        // to 4 at a time.
        //
        for (i = 0; i < pRelations->Count && GetDriverGlobals()->FxVerboseOn; i += 4)
        {
            if (i + 3 < pRelations->Count)
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p PDO %p PDO %p PDO %p",
                    pRelations->Objects[i],
                    pRelations->Objects[i+1],
                    pRelations->Objects[i+2],
                    pRelations->Objects[i+3]
                    );
            }
            else if (i + 2 < pRelations->Count)
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p PDO %p PDO %p",
                    pRelations->Objects[i],
                    pRelations->Objects[i+1],
                    pRelations->Objects[i+2]
                    );
            }
            else if (i + 1 < pRelations->Count)
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p PDO %p",
                    pRelations->Objects[i],
                    pRelations->Objects[i+1]
                    );
            }
            else
            {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p",
                    pRelations->Objects[i]
                    );
            }
        }
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::HandleQueryDeviceRelations(
    __inout FxIrp* Irp,
    __inout FxRelatedDeviceList* List
    )
/*++

Routine Description:
    Handles the query device relations request for all relation types *except*
    for bus relations (HandleQueryBusRelations handles that type exclusively).

    This function will allocate a PDEVICE_RELATIONS structure if the passed in
    FxRelatedDeviceList contains any devices to add to the relations list.

Arguments:
    Irp - the request

    List - list containing devices to report in the relations

Return Value:
    NTSTATUS

  --*/
{
    PDEVICE_RELATIONS pPriorRelations, pNewRelations;
    FxRelatedDevice* entry;
    DEVICE_RELATION_TYPE type;
    ULONG count;
    size_t size;
    NTSTATUS status;
    BOOLEAN retry;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    if (List == NULL)
    {
        //
        // Indicate that we didn't modify the irp at all since we have no list
        //
        return STATUS_NOT_SUPPORTED;
    }

    pFxDriverGlobals = GetDriverGlobals();
    type = Irp->GetParameterQDRType();
    status = STATUS_SUCCESS;

    //
    // Notify driver that he should re-scan for device relations.
    //
    m_DeviceRelationsQuery.Invoke(m_Device->GetHandle(), type);

    pPriorRelations = (PDEVICE_RELATIONS) Irp->GetInformation();
    retry = FALSE;

    count = 0;

    List->LockForEnum(pFxDriverGlobals);

    //
    // Count how many entries there are in the list
    //
    for (entry = NULL; (entry = List->GetNextEntry(entry)) != NULL; count++)
    {
        DO_NOTHING();
    }

    //
    // If we have
    // 1)  no devices in the list AND
    //   a) we have nothing to report OR
    //   b) we have something to report and there are previous relations (which
    //      if left unchanged will be used to report our missing devices)
    //
    // THEN we have nothing else to do, just return
    //
    if (count == 0 &&
        (List->m_NeedReportMissing == 0 || pPriorRelations != NULL))
    {
        List->UnlockFromEnum(pFxDriverGlobals);
        return STATUS_NOT_SUPPORTED;
    }

    if (pPriorRelations != NULL)
    {
        //
        // Looks like another driver in the stack has already added some
        // entries.  Make sure we allocate space for these additional entries.
        //
        count += pPriorRelations->Count;
    }

    //
    // Allocate space for the device relations structure (which includes
    // space for one PDEVICE_OBJECT, and then allocate enough additional
    // space for the extra PDEVICE_OBJECTS we need.
    //
    // (While no FxChildList objects are used in this function, this static
    // function from the class computes what we need.)
    //
    size = FxChildList::_ComputeRelationsSize(count);

    pNewRelations = (PDEVICE_RELATIONS) MxMemory::MxAllocatePoolWithTag(
        PagedPool, size, pFxDriverGlobals->Tag);

    if (pNewRelations == NULL)
    {
        //
        // Dereference any previously reported relations before exiting.  They
        // are dereferenced here because the PNP manager will see error and not
        // do anything while the driver which added these objects expects the
        // pnp manager to do the dereference.  Since this device is changing the
        // status, it must act like the pnp manager.
        //
        if (pPriorRelations != NULL)
        {
            ULONG i;

            for (i = 0; i < pPriorRelations->Count; i++)
            {
                Mx::MxDereferenceObject(pPriorRelations->Objects[i]);
            }
        }

        if (List->IncrementRetries() < 3)
        {
            retry = TRUE;
        }

        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p could not allocate device relations for type %d string, "
            " %!STATUS!", m_Device->GetHandle(), type, status);

        goto Done;
    }

    RtlZeroMemory(pNewRelations, size);

    //
    // If there was an existing device relations structure, copy
    // the entries to the new structure.
    //
    if (pPriorRelations != NULL && pPriorRelations->Count > 0)
    {
        RtlCopyMemory(
            pNewRelations,
            pPriorRelations,
            FxChildList::_ComputeRelationsSize(pPriorRelations->Count)
            );
    }

    //
    // Walk the list and return the relations here
    //
    for (entry = NULL;
         (entry = List->GetNextEntry(entry)) != NULL;
         pNewRelations->Count++)
    {
        MdDeviceObject pdo;

        pdo = entry->GetDevice();

        if (entry->m_State == RelatedDeviceStateNeedsReportPresent)
        {
            entry->m_State = RelatedDeviceStateReportedPresent;
        }

        //
        // Add it to the DEVICE_RELATIONS structure.  Pnp dictates that each
        // PDO in the list be referenced.
        //
        pNewRelations->Objects[pNewRelations->Count] = reinterpret_cast<PDEVICE_OBJECT>(pdo);
        Mx::MxReferenceObject(pdo);
    }

Done:
    if (NT_SUCCESS(status))
    {
        List->ZeroRetries();
    }

    List->UnlockFromEnum(GetDriverGlobals());

    if (pPriorRelations != NULL)
    {
        MxMemory::MxFreePool(pPriorRelations);
    }

    if (retry)
    {
        MxDeviceObject physicalDeviceObject(
                                    m_Device->GetPhysicalDevice()
                                    );

        physicalDeviceObject.InvalidateDeviceRelations(type);
    }

    Irp->SetStatus(status);
    Irp->SetInformation((ULONG_PTR) pNewRelations);

    return status;
}

BOOLEAN
FxPkgPnp::PowerPolicyIsWakeEnabled(
    VOID
    )
{
    if (IsPowerPolicyOwner() && PowerPolicyGetCurrentWakeReason() != 0x0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

ULONG
FxPkgPnp::PowerPolicyGetCurrentWakeReason(
    VOID
    )
/*++

Routine Description:
    This routine determines the reasons for whether wake should be enabled or
    not.  Wake could be enabled because it is explicitly enabled for the device
    in the wake policy settings or, because the device opted to depend on its
    children being armed for wake.

Arguments:
    None

Return Value:
    Returns a combination of FxPowerPolicySxWakeChildrenArmedFlag, to indicate
    that wake can be enabled because of more than one children being armed for
    wake, and FxPowerPolicySxWakeDeviceEnabledFlag, to indicate that wake can
    be enabled because the device was explicitly enabled in the wake policy
    settings.

    Returns Zero to indicate that wake is currently disabled for the device.

--*/
{
    ULONG wakeReason;

    wakeReason = 0x0;

    if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.ArmForWakeIfChildrenAreArmedForWake &&
        m_PowerPolicyMachine.m_Owner->m_ChildrenArmedCount > 0)
    {
        //
        // Wake settings depends on children and one or more children are
        // armed for wake.
        //
        wakeReason |= FxPowerPolicySxWakeChildrenArmedFlag;
    }

    if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled)
    {
        //
        // Wake settings is explicitly enabled.
        //
        wakeReason |= FxPowerPolicySxWakeDeviceEnabledFlag;
    }

    return wakeReason;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PowerPolicySendDevicePowerRequest(
    __in DEVICE_POWER_STATE DeviceState,
    __in SendDeviceRequestAction Action
    )
/*++

Routine Description:
    Attempts to send a D irp to the stack.  The caller can specify if the
    request allocation is retried in case of failure.

Design Notes:
    The timeout and number of retries are somewhat magical numbers.  They were
    picked so that the total amount of time spent attempting to retry would be
    under a minute.  Any memory pressure the machine would be under after the
    first failure should be over by the end of the minute; if not, there are
    bigger problems.

    If you look at each of the callers who specify Retry for the Action, nearly
    each one transitions to the WdfDevStatePwrPolDevicePowerRequestFailed state
    if the request cannot be allocated.  This transition could be placed in this
    function, but it is not for 2 reasons:

    1)  Keep this function simple

    2)  It makes the flow of the state transition function easier to understand;
        all transitions out of the current state happen within top level
        transition function

Arguments:
    DeviceState - The new D state being request

    Action - Whether to retry upon failure to allocate the request

Return Value:
    NT_SUCCESS if the request was allocated, !NT_SUCCESS otherwise

  --*/
{
    MdRequestPowerComplete pCompletionRoutine;
    LARGE_INTEGER interval;
    NTSTATUS status;
    POWER_STATE state;
    ULONG i;

    status = STATUS_UNSUCCESSFUL;
    interval.QuadPart = WDF_REL_TIMEOUT_IN_MS(500);
    state.DeviceState = DeviceState;

    if (DeviceState == PowerDeviceD0)
    {
        //
        // We are powering up, we do not synchronize the completion of the D0 irp
        // with a potential S0 irp. However we need to ensure that if an upper filter 
        // driver fails the power irp, we handle it gracefully rather than keep waiting
        // for the power irp to arrive.
        //
        pCompletionRoutine = _PowerPolDevicePowerUpComplete;
    }
    else
    {
        //
        // We are powering down, we synchronize the completion of the Dx irp
        // with a potential Sx irp.  If there is no pending Sx irp, the state
        // machine takes care of it.
        //
        pCompletionRoutine = _PowerPolDevicePowerDownComplete;
    }

    //
    // We track when we request power irps to catch someone other then ourselves
    // sending power irps to our own stack.
    //
    if (DeviceState == PowerDeviceD0)
    {
        m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp = TRUE;
    } 
    else
    {
        m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp = TRUE;
    }

    for (i = 0; i < 100; i++)
    {
        status = FxIrp::RequestPowerIrp(m_Device->GetDeviceObject(),
                                   IRP_MN_SET_POWER,
                                   state,
                                   pCompletionRoutine,
                                   this);

        //
        // If we are not retrying, we always break out
        //
        if (NT_SUCCESS(status) || Action == NoRetry)
        {
            break;
        }

        Mx::MxDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    if (!NT_SUCCESS(status))
    {
        //
        // We are no longer requesting a power irp
        //
        if (DeviceState == PowerDeviceD0)
        {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp = FALSE;
        } 
        else
        {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp = FALSE;
        }

        if (Action == Retry)
        {
            COVERAGE_TRAP();

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Could not request D%d irp for device %p (WDFDEVICE %p), "
                "%!STATUS!", DeviceState-1, 
                m_Device->GetDeviceObject(),
                m_Device->GetHandle(), status);
        }
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Requesting D%d irp, %!STATUS!",
                        DeviceState-1, status);

    return status;
}

NTSTATUS
FxPkgPnp::NotifyResourceObjectsDx(
    __in ULONG NotifyFlags
    )
/*++

Routine Description:
    This routine traverses all resource objects and tells them that the device
    is leaving D0.  If there is an error, the remaining resources in the list
    are still notified of exitting D0.

Arguments:
    NotifyFlags - combination of values from the enum NotifyResourcesFlags

Return Value:
    None

--*/
{
    FxInterrupt* pInterrupt;
    PLIST_ENTRY ple;
    NTSTATUS status, finalStatus;

    finalStatus = STATUS_SUCCESS;

    //
    // Disconect in the reverse order in which we connected the interrupts
    //
    for (ple = m_InterruptListHead.Blink;
         ple != &m_InterruptListHead;
         ple = ple->Blink)
    {
        //
        // Disconnect interrupts and then tell them that they no longer
        // own their resources.
        //
        pInterrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

        status = pInterrupt->Disconnect(NotifyFlags);

        if (!NT_SUCCESS(status))
        {
            //
            // When we encounter an error we still disconnect the remaining
            // interrupts b/c we would just do it later during device tear down
            // (might as well do it now).
            //
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                                "WDFINTERRUPT %p failed to disconnect, %!STATUS!",
                                pInterrupt->GetHandle(), status);
            //
            // Overwriting a previous finalStatus is OK, we'll just report the
            // last one
            //
            finalStatus = status;
        }
    }

    return finalStatus;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PowerPolicyHandleSystemQueryPower(
    __in SYSTEM_POWER_STATE QueryState
    )
/*++

Framework Philosophy Discussion:

    WDM sends IRP_MN_QUERY_POWER for system power states (where system power
    states are S0-working, S1-light-sleep, S2-deeper-sleep, S3-deepest-sleep,
    S4-hibernation, S5-soft-off.)  The idea is that, if a driver can't support
    a particular state, it fails the query.  The problem is that this idea is
    horribly broken.

    The first problem is that WDM doesn't always send these IRPs.  In some
    situations, (very low battery, system getting critically hot) WDM will
    attempt to preserve user data by sleeping or hibernating, rather than just
    crashing.  This is good, but it makes a driver writer's life very difficult,
    since it means that you have to deal with being told to go to a particular
    state even if you think that your device won't be able to deal well with
    that state.

    The second problem is that, by the time the system is going to sleep, the
    user probably isn't still looking at the screen.  This is especially true
    for laptop computers, as the system is very likely sleeping because the
    user closed the clamshell lid.  So any attempt to ask the user how to
    resolve a situation where a driver doesn't want to go to a low power state
    is futile.  Furthermore, even when the screen is still available, users
    dislike it when they push the sleep button or the power button on their
    machines and the machines don't do what they were told to do.

    The third problem is related to the second.  While there may be completely
    legitimate reasons for the driver to want to delay or even to veto a
    transition into a sleep state, (an example of a valid scenario would be one
    in which the driver was involved in burning a CD, an operation which can't
    be interrupted,) there isn't any good way for a driver to interact with a
    user anyhow.  (Which desktop is the right one to send messages to?  What
    should the UI for problem resolution look like?  How does a driver put up
    UI anyhow?)

    All the driver really knows is that it will or won't be able to maintain
    device state, and it will or won't be able to get enough power to arm
    any wake events that it might want to deliver (like PME#.)

    Consequently, the designers of the PnP/Power model in the Framework have
    decided that all QueryPower-Sx IRPs will be completed successfully
    except if the device cannot maintain enough power to trigger its wake
    signal *AND* if the system supports lighter sleep states than the
    one that is currently being queried.  (If it does, then the kernel's power
    manager will turn right around and query for those, next.)

    This story usually brings up a few objections:

    1)  My device is important!  When it's operating, I don't want
        the machine to just fall asleep.  I need to fail QueryPower-Sx to
        prevent that.

    This objection is an unfortunate consequence of the existing DDK.  There
    is a perfectly good API that allows a driver to say that the machine
    shouldn't just fall asleep.  (See PoSetSystemState.)  If a user presses
    a button telling the machine to go to sleep, then the driver has a
    responsibility to do that.

    2)  There are certain operations that just can't be interrupted!

    While that's true, those operations started somewhere, probably in user-
    mode.  Those same user-mode components would be much, much better suited
    toward negotiating with the user or with other components to figure out
    what to do when the uninterruptable must be interrupted.  User-mode
    components get notification that the system is going to sleep and they
    can delay or veto the transition.  Get over the idea that your driver
    needs to be involved, too.

Routine Description:

    Determines if for the passed in System state, if we can wake the machine
    from it.  If the query state is the machine's minimum system state, then
    we always succeed it because we want the machine to go to at least some
    sleeping state.  We always succeed hibernate and system off requests as well.

Arguments:

    QueryState - The proposed system state

Return Value:

    NT_SUCCESS if the queried state should be allowed, !NT_SUCCESS otherwise

  --*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    NTSTATUS status;

    if (QueryState >= PowerSystemHibernate ||
        PowerPolicyCanWakeFromSystemState(QueryState))
    {
        //
        // If the query is for the machine's minimum S state or we going into
        // hibernate or off, always succeed it.
        //
        status = STATUS_SUCCESS;
    }
    else
    {
        //
        // On Windows Vista and above, its OK to return a failure status code
        // if the system is going into an S state at which the device cannot
        // wake the system.
        //
        ASSERT(FxLibraryGlobals.OsVersionInfo.dwMajorVersion >= 6);

        //
        // The S state the machine is going into is one where we can't
        // wake it up because our D state is too low for this S state.
        // Since this isn't the minimum S state the machine is capable
        // of, reject the current query.
        //
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGPNP,
            "failing system query power because the device cannot wake the "
            "machine from S%d",
            QueryState - 1);

        status = STATUS_POWER_STATE_INVALID;
    }

    return status;
}
