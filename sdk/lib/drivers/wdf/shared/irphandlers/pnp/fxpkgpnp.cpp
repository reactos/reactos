/*++


Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxPkgPnp.cpp

Abstract:

    This module implements the pnp IRP handlers for the driver framework.

Author:



Environment:

    Both kernel and user mode

Revision History:





--*/

#include "pnppriv.hpp"

#include <initguid.h>
#include <wdmguid.h>

extern "C" {

#if defined(EVENT_TRACING)
#include "FxPkgPnp.tmh"
#endif

}

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
    for (i = 0; i < PowerSystemMaximum; i++) {
        _SetPowerCapState(i, PowerDeviceMaximum, &m_PowerCaps.States);
    }

    RtlZeroMemory(&m_D3ColdInterface, sizeof(m_D3ColdInterface));
    RtlZeroMemory(&m_SpecialSupport[0], sizeof(m_SpecialSupport));
    RtlZeroMemory(&m_SpecialFileCount[0], sizeof(m_SpecialFileCount));

    m_PowerThreadInterface.Interface.Size = sizeof(m_PowerThreadInterface);
    m_PowerThreadInterface.Interface.Version = 1;
    m_PowerThreadInterface.Interface.Context = this;
    m_PowerThreadInterface.Interface.InterfaceReference = &FxPkgPnp::_PowerThreadInterfaceReference;
    m_PowerThreadInterface.Interface.InterfaceDereference = &FxPkgPnp::_PowerThreadInterfaceDereference;
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
    m_IoConnectInterruptEx      = FxLibraryGlobals.IoConnectInterruptEx;
    m_IoDisconnectInterruptEx   = FxLibraryGlobals.IoDisconnectInterruptEx;

    //
    // Interrupt APIs for Windows 8 and forward
    //
    m_IoReportInterruptActive   = FxLibraryGlobals.IoReportInterruptActive;
    m_IoReportInterruptInactive = FxLibraryGlobals.IoReportInterruptInactive;

#endif

    m_ReleaseHardwareAfterDescendantsOnFailure = FALSE;

    MarkDisposeOverride(ObjectDoNotLock);
}

FxPkgPnp::~FxPkgPnp()
{
    PSINGLE_LIST_ENTRY ple;

    Mx::MxAssert(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // We should either have zero pending children or we never made it out of
    // the init state during a failed WDFDEVICE create or failed EvtDriverDeviceAdd
    //
    Mx::MxAssert(m_PendingChildCount == 0 ||
           m_Device->GetDevicePnpState() == WdfDevStatePnpInit);


    ple = m_DeviceInterfaceHead.Next;
    while (ple != NULL) {
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

    if (m_DmaEnablerList != NULL) {
        delete m_DmaEnablerList;
        m_DmaEnablerList = NULL;
    }

    if (m_RemovalDeviceList != NULL) {
        delete m_RemovalDeviceList;
        m_RemovalDeviceList = NULL;
    }

    if (m_UsageDependentDeviceList != NULL) {
        delete m_UsageDependentDeviceList;
        m_UsageDependentDeviceList = NULL;
    }

    if (m_PnpStateCallbacks != NULL) {
        delete m_PnpStateCallbacks;
    }

    if (m_PowerStateCallbacks != NULL) {
        delete m_PowerStateCallbacks;
    }

    if (m_PowerPolicyStateCallbacks != NULL) {
        delete m_PowerPolicyStateCallbacks;
    }

    if (m_SelfManagedIoMachine != NULL) {
        delete m_SelfManagedIoMachine;
        m_SelfManagedIoMachine = NULL;
    }

    if (m_EnumInfo != NULL) {
        delete m_EnumInfo;
        m_EnumInfo = NULL;
    }

    if (m_Resources != NULL) {
        m_Resources->RELEASE(this);
        m_Resources = NULL;
    }

    if (m_ResourcesRaw != NULL) {
        m_ResourcesRaw->RELEASE(this);
        m_ResourcesRaw = NULL;
    }

    ASSERT(IsListEmpty(&m_InterruptListHead));
}

BOOLEAN
FxPkgPnp::Dispose(
    VOID
    )
{
    PSINGLE_LIST_ENTRY ple;

    //
    // All of the interrupts were freed during this object's dispose path.  This
    // is because all of the interrupts were attached as children to this object.
    //
    // It is safe to just reinitialize the list head.
    //
    InitializeListHead(&m_InterruptListHead);

    m_QueryInterfaceLock.AcquireLock(GetDriverGlobals());

    //
    // A derived class can insert an embedded FxQueryInterface into the QI list,
    // so clear out the list before the destructor chain runs.  (The derived
    // class will be destructed first, as such, the embedded FxQueryInterface
    // will also destruct first and complain it is still in a list.
    //
    ple = m_QueryInterfaceHead.Next;

    //
    // Clear out the head while holding the lock so that we synchronize against
    // processing a QI while deleting the list.
    //
    m_QueryInterfaceHead.Next = NULL;

    m_QueryInterfaceLock.ReleaseLock(GetDriverGlobals());

    while (ple != NULL) {
        FxQueryInterface* pQI;

        pQI = FxQueryInterface::_FromEntry(ple);

        //
        // Advance before we potentiall free the structure
        //
        ple = ple->Next;

        //
        // FxQueryInterface's destructor requires Next be NULL
        //
        pQI->m_Entry.Next = NULL;

        if (pQI->m_EmbeddedInterface == FALSE) {
            delete pQI;
        }
    }

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    DropD3ColdInterface();
#endif

    //
    // Call up the hierarchy
    //
    return FxPackage::Dispose(); // __super call
}


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

    m_ReleaseHardwareAfterDescendantsOnFailure = (DeviceInit->ReleaseHardwareOrderOnFailure ==
                WdfReleaseHardwareOrderOnFailureAfterDescendants);

    status = m_QueryInterfaceLock.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize QueryInterfaceLock for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }

    status = m_DeviceInterfaceLock.Initialize();
    if (!NT_SUCCESS(status)) {
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
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize cleanup event for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }

    status = m_RemoveEventUm.Initialize(SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Could not initialize remove event for "
                            "WDFDEVICE %p, %!STATUS!",
                            m_Device->GetHandle(), status);
        return status;
    }
#endif

    if (DeviceInit->IsPwrPolOwner()) {
        m_PowerPolicyMachine.m_Owner = new (pFxDriverGlobals)
            FxPowerPolicyOwnerSettings(this);

        if (m_PowerPolicyMachine.m_Owner == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = m_PowerPolicyMachine.m_Owner->Init();
        if (!NT_SUCCESS(status)) {
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
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_Resources->Commit(WDF_NO_OBJECT_ATTRIBUTES,
                                 WDF_NO_HANDLE,
                                 m_Device);

    //
    // This should never fail
    //
    ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status)) {
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
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_ResourcesRaw->Commit(WDF_NO_OBJECT_ATTRIBUTES,
                                    WDF_NO_HANDLE,
                                    m_Device);

    //
    // This should never fail
    //
    ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status)) {
        m_ResourcesRaw->DeleteFromFailedCreate();
        m_ResourcesRaw = NULL;
        return status;
    }

    m_ResourcesRaw->ADDREF(this);

    status = RegisterCallbacks(&DeviceInit->PnpPower.PnpPowerEventCallbacks);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (IsPowerPolicyOwner()) {
        RegisterPowerPolicyCallbacks(&DeviceInit->PnpPower.PolicyEventCallbacks);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
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

    if (irp.GetMajorFunction() == IRP_MJ_PNP) {

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

        if (irp.GetMinorFunction() <= IRP_MN_SURPRISE_REMOVAL) {
            status = (*GetDispatchPnp()[irp.GetMinorFunction()])(this, &irp);
        }
        else {
            //
            // For Pnp IRPs we don't understand, just forget about them
            //
            status = FireAndForgetIrp(&irp);
        }
    }
    else {
        //
        // If this isn't a PnP Irp, it must be a power irp.
        //
        switch (irp.GetMinorFunction()) {
        case IRP_MN_WAIT_WAKE:
        case IRP_MN_SET_POWER:
            if (irp.GetParameterPowerType() == SystemPowerState) {
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
            else {
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

        if (irp.GetMinorFunction() <= IRP_MN_QUERY_POWER) {
            status = (*GetDispatchPower()[irp.GetMinorFunction()])(this, &irp);
        }
        else {
            //
            // For Power IRPs we don't understand, just forget about them
            //
            status = FireAndForgetIrp(&irp);
        }
    }

    return status;
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

    if ((state & FxPnpStateDontDisplayInUIMask) == FxPnpStateDontDisplayInUIUseDefault) {
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
        if (caps == FxPnpCapNoDisplayInUITrue) {
            PnpDeviceState |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
        }
        else if (caps == FxPnpCapNoDisplayInUIFalse) {
            PnpDeviceState &= ~PNP_DEVICE_DONT_DISPLAY_IN_UI;
        }
    }

    //
    // Return device state maintained by frameworks.
    //
    if (IsInSpecialUse()) {
        PnpDeviceState |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    //
    // If there is an internal failure, then indicate that up to pnp.
    //
    if (m_InternalFailure || m_Failed) {
        PnpDeviceState |= PNP_DEVICE_FAILED;
    }

    return PnpDeviceState;
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

    if (m_EnumInfo != NULL) {
        pList = &m_EnumInfo->m_ChildListList;

        pList->LockForEnum(GetDriverGlobals());
    }
    else {
        pList = NULL;
    }

    ple = NULL;
    while (pList != NULL && (ple = pList->GetNextEntry(ple)) != NULL) {
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
        if (listStatus == STATUS_NOT_SUPPORTED) {
            continue;
        }


        if (!NT_SUCCESS(listStatus)) {
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
    if (NT_SUCCESS(status)) {

        ple = NULL;
        while (pList != NULL && (ple = pList->GetNextEntry(ple)) != NULL) {
            FxChildList* pChildList;

            pChildList = FxChildList::_FromEntry(ple);

            //
            // invoke the ReportedMissing callback for for children that are
            // being reporte missing
            //
            pChildList->InvokeReportedMissingCallback();
        }
    }

    if (pList != NULL) {
        pList->UnlockFromEnum(GetDriverGlobals());
    }

    if (NT_SUCCESS(status) && changed == FALSE) {
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

    if (NT_SUCCESS(status) && pRelations != NULL) {
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
        for (i = 0; i < pRelations->Count && GetDriverGlobals()->FxVerboseOn; i += 4) {
            if (i + 3 < pRelations->Count) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p PDO %p PDO %p PDO %p",
                    pRelations->Objects[i],
                    pRelations->Objects[i+1],
                    pRelations->Objects[i+2],
                    pRelations->Objects[i+3]
                    );
            }
            else if (i + 2 < pRelations->Count) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p PDO %p PDO %p",
                    pRelations->Objects[i],
                    pRelations->Objects[i+1],
                    pRelations->Objects[i+2]
                    );
            }
            else if (i + 1 < pRelations->Count) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "PDO %p PDO %p",
                    pRelations->Objects[i],
                    pRelations->Objects[i+1]
                    );
            }
            else {
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
FxPkgPnp::HandleQueryBusInformation(
    __inout FxIrp* Irp
    )
{
    NTSTATUS status;

    //
    // Probably is a better check then this to see if the driver set the bus
    // information
    //
    if (m_BusInformation.BusTypeGuid.Data1 != 0x0) {
        PPNP_BUS_INFORMATION pBusInformation;
        PFX_DRIVER_GLOBALS pFxDriverGlobals;

        pFxDriverGlobals = GetDriverGlobals();
        pBusInformation = (PPNP_BUS_INFORMATION) MxMemory::MxAllocatePoolWithTag(
                PagedPool, sizeof(PNP_BUS_INFORMATION), pFxDriverGlobals->Tag);

        if (pBusInformation != NULL) {
            //
            // Initialize the PNP_BUS_INFORMATION structure with the data
            // from PDO properties.
            //
            RtlCopyMemory(pBusInformation,
                          &m_BusInformation,
                          sizeof(PNP_BUS_INFORMATION));

            Irp->SetInformation((ULONG_PTR) pBusInformation);
            status = STATUS_SUCCESS;
        }
        else {
            Irp->SetInformation(NULL);
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p could not allocate PNP_BUS_INFORMATION string, "
                " %!STATUS!", m_Device->GetHandle(), status);
        }
    }
    else {
        status = Irp->GetStatus();
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

    if (List == NULL) {
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
    for (entry = NULL; (entry = List->GetNextEntry(entry)) != NULL; count++) {
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
        (List->m_NeedReportMissing == 0 || pPriorRelations != NULL)) {
        List->UnlockFromEnum(pFxDriverGlobals);
        return STATUS_NOT_SUPPORTED;
    }

    if (pPriorRelations != NULL) {
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

    if (pNewRelations == NULL) {
        //
        // Dereference any previously reported relations before exiting.  They
        // are dereferenced here because the PNP manager will see error and not
        // do anything while the driver which added these objects expects the
        // pnp manager to do the dereference.  Since this device is changing the
        // status, it must act like the pnp manager.
        //
        if (pPriorRelations != NULL) {
            ULONG i;

            for (i = 0; i < pPriorRelations->Count; i++) {
                Mx::MxDereferenceObject(pPriorRelations->Objects[i]);
            }
        }

        if (List->IncrementRetries() < 3) {
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
    if (pPriorRelations != NULL && pPriorRelations->Count > 0) {
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
         pNewRelations->Count++) {
        MdDeviceObject pdo;

        pdo = entry->GetDevice();

        if (entry->m_State == RelatedDeviceStateNeedsReportPresent) {
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
    if (NT_SUCCESS(status)) {
        List->ZeroRetries();
    }

    List->UnlockFromEnum(GetDriverGlobals());

    if (pPriorRelations != NULL) {
        MxMemory::MxFreePool(pPriorRelations);
    }

    if (retry) {
        MxDeviceObject physicalDeviceObject(
                                    m_Device->GetPhysicalDevice()
                                    );

        physicalDeviceObject.InvalidateDeviceRelations(type);
    }

    Irp->SetStatus(status);
    Irp->SetInformation((ULONG_PTR) pNewRelations);

    return status;
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
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "PnP State Machine init failed, %!STATUS!",
                            status);
        return status;
    }

    status = m_PowerMachine.Init(this, &FxPkgPnp::_PowerProcessEventInner);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Power State Machine init failed, %!STATUS!",
                            status);
        return status;
    }

    status = m_PowerPolicyMachine.Init(this, &FxPkgPnp::_PowerPolicyProcessEventInner);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Power Policy State Machine init failed, %!STATUS!",
                            status);
        return status;
    }

    return status;
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
FxPkgPnp::SetSpecialFileSupport(
    __in WDF_SPECIAL_FILE_TYPE FileType,
    __in BOOLEAN Supported
    )

/*++

Routine Description:

    This function marks the device as capable of handling the paging path,
    hibernation or crashdumps.  Any device that is necessary for one of these
    three things will get notification.  It is then responsible for forwarding
    the notification to its parent. The Framework handles that.  This
    function just allows a driver to tell the Framework how to respond.


Arguments:

    FileType - identifies which of the special paths the device is in
    Supported - Yes or No

Returns:

    void

--*/

{
    switch (FileType) {
    case WdfSpecialFilePaging:
    case WdfSpecialFileHibernation:
    case WdfSpecialFileDump:
    case WdfSpecialFileBoot:
        SetUsageSupport(_SpecialTypeToUsage(FileType), Supported);
        break;

    default:
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Invalid special file type %x", FileType);
    }
}

_Must_inspect_result_
NTSTATUS
PnpPassThroughQI(
    __in    CfxDevice* Device,
    __inout FxIrp* Irp
    )

/*++

Routine Description:

    This driver may be sent IRP_MN_QUERY_INTERFACE, which is a way of getting
    a direct-call table from some driver in a device stack.  In some cases, the
    right response is to turn around and send a similar query to the device's
    parent.  This function does that.


Arguments:

    Device - This WDFDEVICE.
    Irp - The IRP that was sent to us.

Returns:

    NTSTATUS

--*/

{
    MdIrp pFwdIrp;
    NTSTATUS status;
    NTSTATUS prevStatus;
    MxDeviceObject pTopOfStack;

    prevStatus = Irp->GetStatus();












    pTopOfStack.SetObject(Device->m_ParentDevice->GetAttachedDeviceReference());

    pFwdIrp = FxIrp::AllocateIrp(pTopOfStack.GetStackSize());

    if (pFwdIrp != NULL) {
        FxAutoIrp fxFwdIrp(pFwdIrp);

        //
        // The worker routine copies stack parameters to forward-Irp, sends it
        // down the stack synchronously, then copies back the stack parameters
        // from forward-irp to original-irp
        //
        PnpPassThroughQIWorker(&pTopOfStack, Irp, &fxFwdIrp);

        if (fxFwdIrp.GetStatus() != STATUS_NOT_SUPPORTED) {
            status = fxFwdIrp.GetStatus();
        }
        else {
            status = prevStatus;
        }

        Irp->SetStatus(status);
        Irp->SetInformation(fxFwdIrp.GetInformation());
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            Device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p could not allocate IRP to send QI to parent !devobj "
            "%p, %!STATUS!", Device->GetHandle(), pTopOfStack.GetObject(),
            status);
    }

    pTopOfStack.DereferenceObject();

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::HandleQueryInterfaceForPowerThread(
    __inout FxIrp* Irp,
    __out   PBOOLEAN CompleteRequest
    )
{
    NTSTATUS status;

    *CompleteRequest = TRUE;

    //
    // Send the request down the stack first.  If someone lower handles it
    // or failed trying to, just return their status
    //
    status = SendIrpSynchronously(Irp);

    if (NT_SUCCESS(status) || status != STATUS_NOT_SUPPORTED) {
        //
        // Success or failure trying to handle it
        //
        return status;
    }

    //
    // The semantic of this QI is that it sent down while processing start or
    // a device usage notification on the way *up* the stack.  That means that
    // by the time the QI gets to the lower part of the stack, the power thread
    // will have already been allocated and exported.
    //
    ASSERT(HasPowerThread());

    if (Irp->GetParameterQueryInterfaceVersion() == 1 &&
        Irp->GetParameterQueryInterfaceSize() >=
                                        m_PowerThreadInterface.Interface.Size) {
        //
        // Expose the interface to the requesting driver.
        //
        CopyQueryInterfaceToIrpStack(&m_PowerThreadInterface, Irp);

        status = STATUS_SUCCESS;

        //
        // Caller assumes a reference has been taken.
        //
        m_PowerThreadInterface.Interface.InterfaceReference(
            m_PowerThreadInterface.Interface.Context
            );
    }
    else {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::HandleQueryInterface(
    __inout FxIrp* Irp,
    __out   PBOOLEAN CompleteRequest
    )

/*++

Routine Description:

    This driver may be sent IRP_MN_QUERY_INTERFACE, which is a way of getting
    a direct-call table from some driver in a device stack.  This function
    looks into a list of interfaces that the driver has registered and, if
    the interface that is being sought is present, it answers the IRP.

Arguments:

    Irp - The IRP that was sent to us.
    CompleteRequest - tells the caller whether the IRP should be completed

Returns:

    NTSTATUS

--*/

{
    FxDeviceProcessQueryInterfaceRequest callback;
    const GUID* pInterfaceType;
    PSINGLE_LIST_ENTRY ple;
    FxQueryInterface *pQI;
    PVOID pFound;
    PINTERFACE pExposedInterface;
    PVOID pExposedInterfaceSpecificData;
    BOOLEAN sendToParent;

    NTSTATUS status;

    *CompleteRequest = FALSE;

    pFound = NULL;
    pQI = NULL;
    pExposedInterface = NULL;
    pExposedInterfaceSpecificData = NULL;
    sendToParent = FALSE;

    pInterfaceType = Irp->GetParameterQueryInterfaceType();
    //
    // The power thread is special cased because it has a different semantic
    // then the usual QI irp that we expose to the driver writer.  In this case,
    // we want to fill in the interface if the lower stack does not support it.
    //
    if (FxIsEqualGuid(pInterfaceType, &GUID_POWER_THREAD_INTERFACE)) {
        return HandleQueryInterfaceForPowerThread(Irp, CompleteRequest);
    }
    else if (FxIsEqualGuid(pInterfaceType, &GUID_REENUMERATE_SELF_INTERFACE_STANDARD)) {
        if (m_Device->IsPdo()) {
            return ((FxPkgPdo*) this)->HandleQueryInterfaceForReenumerate(
                Irp, CompleteRequest);
        }
    }

    status = Irp->GetStatus();

    //
    // Walk the interface collection and return the appropriate interface.
    //
    m_QueryInterfaceLock.AcquireLock(GetDriverGlobals());

    for (ple = m_QueryInterfaceHead.Next; ple != NULL; ple = ple->Next) {
        pQI = FxQueryInterface::_FromEntry(ple);

        if (FxIsEqualGuid(Irp->GetParameterQueryInterfaceType(),
                          &pQI->m_InterfaceType)) {

            pExposedInterface = Irp->GetParameterQueryInterfaceInterface();
            pExposedInterfaceSpecificData =
                Irp->GetParameterQueryInterfaceInterfaceSpecificData();

            if (pQI->m_Interface != NULL) {
                //
                // NOTE:  If a driver has exposed the same interface GUID with
                //        different sizes as a ways of versioning, then the driver
                //        writer can specify the minimum size and version number
                //        and then fill in the remaining fields in the callback
                //        below.
                //
                if (pQI->m_Interface->Size <= Irp->GetParameterQueryInterfaceSize() &&
                    pQI->m_Interface->Version <= Irp->GetParameterQueryInterfaceVersion()) {

                    if (pQI->m_ImportInterface == FALSE) {
                        //
                        // Expose the interface to the requesting driver.
                        //
                        RtlCopyMemory(pExposedInterface,
                                      pQI->m_Interface,
                                      pQI->m_Interface->Size);
                    }
                    else {
                        //
                        // The interface contains data which the driver wants
                        // before copying over its information, so don't do a
                        // copy and let the event callback do the copy
                        //
                        DO_NOTHING();
                    }
                }
                else {
                    status = STATUS_INVALID_BUFFER_SIZE;
                    break;
                }
            }

            callback.m_Method = pQI->m_ProcessRequest.m_Method;
            sendToParent = pQI->m_SendQueryToParentStack;
            pFound = pQI;

            status = STATUS_SUCCESS;
            break;
        }
    }

    m_QueryInterfaceLock.ReleaseLock(GetDriverGlobals());

    if (!NT_SUCCESS(status) || pFound == NULL) {
        goto Done;
    }

    //
    // Let the driver see the interface before it is handed out.
    //
    status = callback.Invoke(m_Device->GetHandle(),
                             (LPGUID) Irp->GetParameterQueryInterfaceType(),
                             pExposedInterface,
                             pExposedInterfaceSpecificData);

    //
    // STATUS_NOT_SUPPORTED is a special cased error code which indicates that
    // the QI should travel down the rest of the stack.
    //
    if (!NT_SUCCESS(status) && status != STATUS_NOT_SUPPORTED) {
        goto Done;
    }

    //
    // If it is meant for the parent, send it down the parent stack
    //
    if (sendToParent) {
        status = PnpPassThroughQI(m_Device, Irp);
        goto Done;
    }

    //
    // Reference the interface before returning it to the requesting driver.
    // If this is an import interface, the event callback is free to not fill
    // in the InterfaceReference function pointer.
    //
    if (pExposedInterface->InterfaceReference != NULL) {
        pExposedInterface->InterfaceReference(pExposedInterface->Context);
    }

    //
    // If we are not a PDO in the stack, then send the fully formatted QI request
    // down the stack to allow others to filter the interface.
    //
    if (m_Device->IsPdo() == FALSE) {
        ASSERT(NT_SUCCESS(status) || status == STATUS_NOT_SUPPORTED);

        Irp->SetStatus(status);
        Irp->CopyCurrentIrpStackLocationToNext();
        status = Irp->SendIrpSynchronously(m_Device->GetAttachedDevice());
    }

Done:
    if (pFound != NULL) {
        *CompleteRequest = TRUE;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::QueryForCapabilities(
    VOID
    )
{
    STACK_DEVICE_CAPABILITIES caps;
    NTSTATUS status;

    MxDeviceObject deviceObject;

    deviceObject.SetObject(m_Device->GetDeviceObject());

    status = GetStackCapabilities(GetDriverGlobals(),
                                  &deviceObject,
                                  &m_D3ColdInterface,
                                  &caps);

    if (NT_SUCCESS(status)) {
        ULONG states, i;

        ASSERT(caps.DeviceCaps.DeviceWake <= 0xFF && caps.DeviceCaps.SystemWake <= 0xFF);

        m_SystemWake = (BYTE) caps.DeviceCaps.SystemWake;

        //
        // Initialize the array of wakeable D-states to say that all system
        // states down to the one identified in the caps can generate wake.
        // This will be overridden below if the BIOS supplied more information.
        //
        // Compatibility Note: Non-ACPI bus drivers (root-enumerated drivers)
        // or other bus drivers that haven't set the power settings correctly
        // for their PDO may end up with a valid value for DeviceWake in the
        // device capabilities but a value of PowerSystemUnspecified for
        // SystemWake, in which case a call to WdfDeviceAssignS0IdleSettings or
        // WdfDeviceAssignSxWakeSettings DDIs will fail on 1.11+ resulting in
        // device compat issues. The failure is expected and right thing to do
        // but has compatibility implications - drivers that worked earlier now
        // fail on 1.11. Note that earlier versions of WDF did not have
        // m_DeviceWake as an array and stored just the capabilities->DeviceWake
        // value without regard to the SystemWake but the current implementation
        // introduces dependency on systemWake value). So for compat reasons,
        // for pre-1.11 compiled drivers we initilaize the array with DeviceWake
        // value ignoring SystemWake, removing any dependency of DeviceWake
        // on SystemWake value and thus preserving previous behavior for
        // pre-1.11 compiled drivers.
        //
        if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1,11)) {

            RtlFillMemory(m_DeviceWake, DeviceWakeStates, DeviceWakeDepthNotWakeable);

            for (i = PowerSystemWorking; i <= m_SystemWake; i++) {

                //
                // Note that this cast is hiding a conversion between two slightly
                // incompatible types.  DeviceWake is in terms of DEVICE_POWER_STATE
                // which is defined this way:
                //
                //  typedef enum _DEVICE_POWER_STATE {
                //      PowerDeviceUnspecified = 0,
                //      PowerDeviceD0,
                //      PowerDeviceD1,
                //      PowerDeviceD2,
                //      PowerDeviceD3,
                //      PowerDeviceMaximum
                //  } DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;
                //
                // m_DeviceWake is defined in terms of DEVICE_WAKE_DEPTH which is
                // defined this way:
                //
                //  typedef enum _DEVICE_WAKE_DEPTH {
                //      DeviceWakeDepthNotWakeable    = 0,
                //      DeviceWakeDepthD0,
                //      DeviceWakeDepthD1,
                //      DeviceWakeDepthD2,
                //      DeviceWakeDepthD3hot,
                //      DeviceWakeDepthD3cold,
                //      DeviceWakeDepthMaximum
                //  } DEVICE_WAKE_DEPTH, *PDEVICE_WAKE_DEPTH;
                //
                // The result is that the conversion below will map D3 onto D3hot,
                // which is a safe assumption to start with, one which may be
                // overridden later.
                //
                C_ASSERT(PowerDeviceD0 == static_cast<DEVICE_POWER_STATE>(DeviceWakeDepthD0));
                m_DeviceWake[i - PowerSystemWorking] = (BYTE) caps.DeviceCaps.DeviceWake;
            }
        }
        else {
            //
            // See comments above for information on mapping of device power
            // state to device wake depth.
            //
            RtlFillMemory(m_DeviceWake,
                          DeviceWakeStates,
                          (BYTE) caps.DeviceCaps.DeviceWake);
        }

        //
        // Capture the S -> D state mapping table as a ULONG for use in the
        // power policy owner state machine when the machine moves into Sx and
        // the device is not armed for wake and has set an IdealDxStateForSx
        // value
        //
        states = 0x0;

        for (i = 0; i < ARRAY_SIZE(caps.DeviceCaps.DeviceState); i++) {
            _SetPowerCapState(i,  caps.DeviceCaps.DeviceState[i], &states);
        }

        m_PowerPolicyMachine.m_Owner->m_SystemToDeviceStateMap = states;

        //
        // Query for the D3cold support interface.  If present, it will tell
        // us specifically which D-states will work for generating wake signals
        // from specific S-states.
        //
        // Earlier versions of WDF didn't make this query, so for compatibility,
        // we only make it now if the driver was built against WDF 1.11 or
        // later.  In truth, this just shifts the failure from initialization
        // time to run time, because the information that we're presumably
        // getting from the BIOS with this interrogation is saying that the
        // code in earlier verisions of WDF would have blindly enabled a device
        // for wake which simply wasn't capable of generating its wake signal
        // from the chosen D-state.  Thus the device would have been put into
        // a low power state and then failed to resume in response to its wake
        // signal.
        //

        if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1,11)) {

            //
            // Cycle through all the system states that this device can wake
            // from.  There's no need to look at deeper sleep states than
            // m_SystemWake because the driver will not arm for wake in
            // those states.
            //
            for (i = PowerSystemWorking; i <= m_SystemWake; i++) {
                if (caps.DeepestWakeableDstate[i] != DeviceWakeDepthMaximum) {
                    m_DeviceWake[i - PowerSystemWorking] = (BYTE)caps.DeepestWakeableDstate[i];
                }
            }
        }
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
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
    //
    // Keep this IRP around, since we're going to deal with it later.
    //
    This->SetPendingPnpIrp(Irp);

    //
    // Now run the state machine on this thread.
    //
    This->PnpProcessEvent(PnpEventQueryStop);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
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
    //
    // Seed the irp with success
    //
    Irp->SetStatus(STATUS_SUCCESS);

    //
    // Pend it and transition the state machine
    //
    This->SetPendingPnpIrp(Irp);
    This->PnpProcessEvent(PnpEventCancelStop);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
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
    //
    // Seed the irp with success
    //
    Irp->SetStatus(STATUS_SUCCESS);

    //
    // Pend and transition the state machine
    //
    This->SetPendingPnpIrp(Irp);
    This->PnpProcessEvent(PnpEventStop);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
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
    //
    // By default we handle this state.
    //
    Irp->SetStatus(STATUS_SUCCESS);

    //
    // Keep this IRP around, since we're going to deal with it later.
    //
    This->SetPendingPnpIrp(Irp);

    //
    // Now run the state machine on this thread.
    //
    This->PnpProcessEvent(PnpEventQueryRemove);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
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
    //
    // Seed the irp with success
    //
    Irp->SetStatus(STATUS_SUCCESS);

    //
    // Pend it and transition the state machine
    //

    This->SetPendingPnpIrp(Irp);
    This->PnpProcessEvent(PnpEventCancelRemove);

    return STATUS_PENDING;
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
            ) == FALSE) {
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

    if (m_PowerPolicyMachine.SetFinished(event) == FALSE) {
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

    if (m_PowerMachine.SetFinished(event) == FALSE) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p, !devobj %p waiting for pwr state machine to finish",
            m_Device->GetHandle(), m_Device->GetDeviceObject());

        event->EnterCRAndWaitAndLeave();
    }

    if (IsPowerPolicyOwner()) {
        //
        // See previous comment about why we Clear()
        //
        event->Clear();

        if (NULL != m_PowerPolicyMachine.m_Owner->m_PoxInterface.
                                        m_DevicePowerRequirementMachine) {

            if (FALSE == m_PowerPolicyMachine.m_Owner->m_PoxInterface.
                          m_DevicePowerRequirementMachine->SetFinished(event)) {

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

_Must_inspect_result_
NTSTATUS
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
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    MxEvent * event = This->m_RemoveEventUm.GetSelfPointer();
#else
    MxEvent eventOnStack;
    eventOnStack.Initialize(SynchronizationEvent, FALSE);
    MxEvent * event = eventOnStack.GetSelfPointer();
#endif

    NTSTATUS status;

    status = Mx::MxAcquireRemoveLock(
        This->m_Device->GetRemoveLock(),
        Irp->GetIrp());

#if DBG
    ASSERT(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    //
    // Keep this object around after m_Device has RELEASE'ed its reference to
    // this package.
    //
    This->ADDREF(Irp);

    //
    // Removes are always success
    //
    Irp->SetStatus(STATUS_SUCCESS);

    ASSERT(This->m_DeviceRemoveProcessed == NULL);
    This->m_DeviceRemoveProcessed = event;

    //
    // Post the event and wait for the FxDevice to destroy itself or determine
    // it has not been reported missing yet (for PDOs and bus filters).
    //
    This->PnpProcessEvent(PnpEventRemove);

    DoTraceLevelMessage(
        This->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "WDFDEVICE %p, !devobj %p waiting for remove event to finish processing",
        This->m_Device->GetHandle(), This->m_Device->GetDeviceObject());

    //
    // No need to wait in a critical region because we are in the context of a
    // pnp request which is in the system context.
    //
    event->WaitFor(Executive, KernelMode, FALSE, NULL);

    This->m_DeviceRemoveProcessed = NULL;

    status = This->ProcessRemoveDeviceOverload(Irp);

    //
    // Release the reference added at the top.  This is most likely going to be
    // the last reference on the package for KMDF. For UMDF, host manages the
    // lifetime of FxDevice so this may not be the last release for UMDF.
    //
    This->RELEASE(Irp);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpSurpriseRemoval(
    __inout FxIrp* Irp
    )

/*++

Routine Description:

    Notification that the device has been surprise removed.  Kick the state
    machine.

Arguments:

    Irp - pointer to FxIrp representing this notification

Return Value:

    STATUS_PENDING

--*/

{
    //
    // Package specific handling
    //
    Irp->SetStatus(STATUS_SUCCESS);
    SetPendingPnpIrp(Irp);
    PnpProcessEvent(PnpEventSurpriseRemove);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
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
    return This->DispatchWaitWake(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::DispatchWaitWake(
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    Handles wait wake requests in a generic fashion

Arguments:


Return Value:

  --*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    NTSTATUS status;
    PIRP oldIrp;
    KIRQL irql;

    if (IsPowerPolicyOwner()) {
        if (m_PowerPolicyMachine.m_Owner->m_RequestedWaitWakeIrp == FALSE) {
            //
            // A power irp arrived, but we did not request it.  log and bugcheck
            //
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Received wait wake power irp %p on device %p, but the irp was "
                "not requested by the device (the power policy owner)",
                Irp->GetIrp(), m_Device->GetDeviceObject());

            FxVerifierBugCheck(GetDriverGlobals(),  // globals
                   WDF_POWER_MULTIPLE_PPO, // specific type
                   (ULONG_PTR)m_Device->GetDeviceObject(), //parm 2
                   (ULONG_PTR)Irp->GetIrp());  // parm 3

            /* NOTREACHED */
        }

        //
        // We are no longer requesting a power irp because we received the one
        // we requested.
        //
        m_PowerPolicyMachine.m_Owner->m_RequestedWaitWakeIrp = FALSE;
    }

    //
    // The Framework has the concept of a "Wait/Wake Owner."  This is the layer
    // in the stack that is enabling and disabling wake at the bus level.  This
    // is probably the PDO or a bus filter like ACPI.sys.  This is distinct
    // from being the "Power Policy Owner," which would mean that this driver
    // is deciding what D-state is appropriate for the device, and also
    // sending Wait/Wake IRPs.
    //

    if (m_SharedPower.m_WaitWakeOwner) {
        m_PowerMachine.m_WaitWakeLock.Acquire(&irql);

        if (m_SharedPower.m_WaitWakeIrp != NULL) {
            //
            // We only allow one pended wait wake irp in the stack at a time.
            // Fail this secondary wait wake request.
            //
            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Failing wait wake irp %p with %!STATUS! because wait wake irp "
                "%p already pended",
                Irp->GetIrp(), status, m_SharedPower.m_WaitWakeIrp);
        }
        else {
            MdCancelRoutine pRoutine;

            //
            // No wait wake irp is currently in the stack, so attempt to set
            // a cancel routine and transition the power state machine into a
            // a state where it can arm the device at the bus level for this
            // child.
            //
            // The power state machine expects the wait wake irp to have a cancel
            // routine set in all states.  For those states which require a
            // non cancelable irp, those states clear the cancel routine as
            // appropriate.
            //
            pRoutine = Irp->SetCancelRoutine(_PowerWaitWakeCancelRoutine);
#if DBG
            ASSERT(pRoutine == NULL);
#else
            UNREFERENCED_PARAMETER(pRoutine);
#endif
            status = STATUS_PENDING;

            if (Irp->IsCanceled()) {
                DoTraceLevelMessage(GetDriverGlobals(),
                                    TRACE_LEVEL_ERROR, TRACINGPNP,
                                    "wait wake irp %p already canceled", Irp->GetIrp());

                //
                // This IRP has already been cancelled, we must clear the cancel
                // routine before completing the IRP.
                //
                pRoutine = Irp->SetCancelRoutine(NULL);

                if (pRoutine != NULL) {
                    //
                    // Our cancel routine will not be called
                    //
#if DBG
                    ASSERT(pRoutine == _PowerWaitWakeCancelRoutine);
#else
                    UNREFERENCED_PARAMETER(pRoutine);
#endif
                    Irp->SetStatus(STATUS_CANCELLED);
                    status = STATUS_CANCELLED;
                }
            }

            if (status == STATUS_PENDING) {
                //
                // Either we successfully set the cancel routine or the irp
                // was canceled and the cancel routine is about to run when
                // we drop the lock.  If the routine is about to run, we still
                // need to setup m_SharedPower to values that it expects.
                //
                Irp->MarkIrpPending();
                m_SharedPower.m_WaitWakeIrp = Irp->GetIrp();
            }
        }
        m_PowerMachine.m_WaitWakeLock.Release(irql);

        if (NT_SUCCESS(status)) {
            //
            // Post to the appropriate matchines
            //
            PowerProcessEvent(PowerWakeArrival);

            if (IsPowerPolicyOwner()) {
                PowerPolicyProcessEvent(PwrPolWakeArrived);
            }
        }
        else {
            CompletePowerRequest(Irp, status);
        }

        return status;
    }
    else if (IsPowerPolicyOwner()) {
        //
        // Try to set m_WaitWakeIrp to the new IRP value only if the current
        // value of m_WaitWakeIrp is NULL because there can only be one
        // active wait wake irp in the stack at a time.  Since the power policy
        // state machine never sends more then one, this is really a guard
        // against some other device in the stack sending a wait wake irp to
        // this device in the wrong state.
        //
        oldIrp = (PIRP) InterlockedCompareExchangePointer(
            (PVOID*) &m_SharedPower.m_WaitWakeIrp,
            Irp->GetIrp(),
            NULL
            );

        //
        // If oldIrp is NULL then there was no previous irp and we successfully
        // exchanged the new PIRP value into m_WaitWakeIrp
        //
        if (oldIrp == NULL) {
            m_PowerPolicyMachine.SetWaitWakeUnclaimed();

            //
            // NOTE:  There is a built in race condition here that WDF cannot
            //        solve with the given WDM primitives.  After arming the
            //        device for wake, there is a window where the wait wake irp
            //        has not yet been processed by the wait wake owner.  Until
            //        the wake request is processed, wake events could be generated
            //        and lost.  There is nothing we can do about this until we
            //        have synchronous "goto Dx and arm" command available.
            //
            Irp->CopyCurrentIrpStackLocationToNext();
            Irp->SetCompletionRoutineEx(m_Device->GetDeviceObject(),
                                        _PowerPolicyWaitWakeCompletionRoutine,
                                        this);

            //
            // Technically, there should be a PDO vs FDO overload here which
            // does the right thing w/regard to this request and if it is at the
            // bottom of the stack or not.  But, by design, we check for
            // m_WaitWakeOwner first which has the direct side affect of making
            // it impossible for a PDO to get to this point.
            //
            ASSERT(m_Device->IsFdo());

            status = Irp->PoCallDriver(m_Device->GetAttachedDevice());

            //
            // Send the wake arrived after sending the request as commented above.
            // This window between sending the request and sending the event to
            // the state machine allows the wait owner to complete the request
            // immediately.   When completed synchronously, it has an effect on
            //  both wake scenarios:
            //
            // 1) wake from S0:  the device never transitions to Dx and the idle
            //                   timer is resumed if no i/o is present
            //
            // 2) wake from sx:  the device is disarmed for wake from Sx and
            //                   put into Dx with being armed for wake.
            //
            PowerPolicyProcessEvent(PwrPolWakeArrived);
        }
        else {
            status = STATUS_POWER_STATE_INVALID;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "already have a ww irp %p, failing new ww irp %p with %!STATUS!",
                oldIrp, Irp->GetIrp(), status);

            CompletePowerRequest(Irp, status);
        }
    }
    else {
        //
        // Not the power policy owner, not the wait wake irp owner, ignore the
        // irp
        //
        // This will release the remove lock.
        //
        status = FireAndForgetIrp(Irp);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::RegisterCallbacks(
    __in PWDF_PNPPOWER_EVENT_CALLBACKS DispatchTable
    )
{
    NTSTATUS status;

    //
    // Update the callback table.
    //
    m_DeviceD0Entry.m_Method           = DispatchTable->EvtDeviceD0Entry;
    m_DeviceD0EntryPostInterruptsEnabled.m_Method =
                                         DispatchTable->EvtDeviceD0EntryPostInterruptsEnabled;
    m_DeviceD0ExitPreInterruptsDisabled.m_Method =
                                         DispatchTable->EvtDeviceD0ExitPreInterruptsDisabled;
    m_DeviceD0Exit.m_Method            = DispatchTable->EvtDeviceD0Exit;

    m_DevicePrepareHardware.m_Method   = DispatchTable->EvtDevicePrepareHardware;
    m_DeviceReleaseHardware.m_Method   = DispatchTable->EvtDeviceReleaseHardware;

    m_DeviceQueryStop.m_Method         = DispatchTable->EvtDeviceQueryStop;
    m_DeviceQueryRemove.m_Method       = DispatchTable->EvtDeviceQueryRemove;

    m_DeviceSurpriseRemoval.m_Method   = DispatchTable->EvtDeviceSurpriseRemoval;

    m_DeviceUsageNotification.m_Method = DispatchTable->EvtDeviceUsageNotification;
    m_DeviceUsageNotificationEx.m_Method = DispatchTable->EvtDeviceUsageNotificationEx;
    m_DeviceRelationsQuery.m_Method    = DispatchTable->EvtDeviceRelationsQuery;

    if (DispatchTable->EvtDeviceSelfManagedIoCleanup != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoFlush != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoInit != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoSuspend != NULL ||
        DispatchTable->EvtDeviceSelfManagedIoRestart != NULL) {

        status = FxSelfManagedIoMachine::_CreateAndInit(&m_SelfManagedIoMachine,
                                                        this);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        m_SelfManagedIoMachine->InitializeMachine(DispatchTable);
    }

    return STATUS_SUCCESS;
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

NTSTATUS
FxPkgPnp::RegisterPowerPolicyWmiInstance(
    __in  const GUID* Guid,
    __in  FxWmiInstanceInternalCallbacks* Callbacks,
    __out FxWmiInstanceInternal** Instance
    )
{
    // WDF_WMI_PROVIDER_CONFIG config;
    // NTSTATUS status;

    // WDF_WMI_PROVIDER_CONFIG_INIT(&config, Guid);

    // //
    // // We are assuming we are registering either for the wait wake or device
    // // timeout GUIDs which both operate on BOOLEANs.  If we expand this API in
    // // the future, have the caller pass in a config structure for the provider
    // // GUID.
    // //
    // config.MinInstanceBufferSize = sizeof(BOOLEAN);

    // status = m_Device->m_PkgWmi->AddPowerPolicyProviderAndInstance(
    //     &config,
    //     Callbacks,
    //     Instance);

    // if (status == STATUS_OBJECT_NAME_COLLISION) {
    //     status = STATUS_SUCCESS;
    // }

    // if (!NT_SUCCESS(status)) {
    //     DoTraceLevelMessage(
    //         GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
    //         "Failed to register WMI power GUID %!STATUS!", status);
    // }
    //
    // return status;
    ROSWDFNOTIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::PowerPolicySetS0IdleSettings(
    __in PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings
    )
/*++

Routine Description:

    Updates the S0 Idle settings for the device and then posts an update event
    to the power policy state machine.  The first this function is called, the
    ability to allow the user to control this setting is set.

Arguments:

    Settings - The settings to apply.

Return Value:

    NTSTATUS

  --*/
{
    DEVICE_POWER_STATE dxState;
    ULONG idleTimeout;
    NTSTATUS status;
    BOOLEAN enabled, s0Capable, overridable, firstTime;
    WDF_TRI_STATE powerUpOnSystemWake;
    const LONGLONG negliblySmallIdleTimeout = -1; // 100 nanoseconds

    s0Capable = FALSE;
    dxState = PowerDeviceD3;
    overridable = FALSE;
    firstTime = TRUE;

    if (Settings->Enabled == WdfTrue) {
        enabled = TRUE;

    } else if (Settings->Enabled == WdfUseDefault) {
        enabled = TRUE;

        if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
            DECLARE_CONST_UNICODE_STRING(valueName, WDF_S0_IDLE_DEFAULT_VALUE_NAME);

            //
            // Read registry. If registry value is not found, the value of "enabled"
            // remains unchanged
            //
            ReadRegistryS0Idle(&valueName, &enabled);
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                "If registry value WdfDefaultIdleInWorkingState was present, "
                "it was not read because DDI WdfDeviceAssignS0IdleSettings "
                "was not called at PASSIVE_LEVEL");
        }
    }
    else {
        enabled = FALSE;
    }

    if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.Set) {
        firstTime = FALSE;
    }

    if (m_CapsQueried == FALSE && Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
        status = QueryForCapabilities();
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Do not set m_CapsQueried to TRUE yet because we will do that once we
        // know the entire stack has been built we will do the query again.
        //
    }

    switch (Settings->IdleCaps) {
    case IdleUsbSelectiveSuspend:
    case IdleCanWakeFromS0:
        s0Capable = TRUE;

        if (Settings->DxState == PowerDeviceMaximum) {
            dxState = PowerPolicyGetDeviceDeepestDeviceWakeState(PowerSystemWorking);

            //
            // Some bus drivers

            // incorrectly report DeviceWake=D0 to
            // indicate that it does not support wake instead of specifying
            // PowerDeviceUnspecified and KMDF ends up requesting
            // a D0 irp when going to Dx. The check prevents this bug.
            //
            if (dxState < PowerDeviceD1 ||
                dxState > PowerDeviceD3 ||
                (dxState > PowerDeviceD2 && Settings->IdleCaps == IdleUsbSelectiveSuspend)
                ) {
                status = STATUS_POWER_STATE_INVALID;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "DeviceWake power state reported in device capabilities "
                    "%!DEVICE_POWER_STATE! indicates that device can not signal"
                    " a wake event, %!STATUS!",
                    dxState, status);
                return status;
            }
        }
        else {
            DEVICE_POWER_STATE dxDeepest;

            dxState = Settings->DxState;
            dxDeepest = PowerPolicyGetDeviceDeepestDeviceWakeState(PowerSystemWorking);

            if (dxState > dxDeepest) {
                status = STATUS_POWER_STATE_INVALID;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "DxState specified by driver %!DEVICE_POWER_STATE! cannot "
                    "be lighter than lightest available device wake state"
                    " %!DEVICE_POWER_STATE!, %!STATUS!", dxState,
                    dxDeepest, status);
                return status;
            }

            //
            // Can only perform wait wake from D2 on a USB device
            //
            if (dxState > PowerDeviceD2 &&
                Settings->IdleCaps == IdleUsbSelectiveSuspend) {
                status = STATUS_POWER_STATE_INVALID;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "DxState specified by driver %!DEVICE_POWER_STATE! cannot "
                    "be lighter than PowerDeviceD2 for USB selective suspend "
                    "%!STATUS!",
                    dxState, status);
                return status;
            }
        }

        if (Settings->IdleCaps == IdleUsbSelectiveSuspend) {
            status = m_PowerPolicyMachine.InitUsbSS();

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Failed to initialize USB selective suspend %!STATUS!",
                    status);
                return status;
            }
        }

        break;

    case IdleCannotWakeFromS0:
        s0Capable = FALSE;

        if (Settings->DxState == PowerDeviceMaximum) {
            dxState = PowerDeviceD3;
        }
        else {
            dxState = Settings->DxState;
        }

        break;

    default:
        ASSERT(FALSE);
        break;
    }

    if (Settings->IdleTimeout == IdleTimeoutDefaultValue) {
        idleTimeout = FxPowerPolicyDefaultTimeout;
    }
    else {
        idleTimeout = Settings->IdleTimeout;
    }

    if (Settings->UserControlOfIdleSettings == IdleAllowUserControl) {

        // status = UpdateWmiInstanceForS0Idle(AddInstance);
        // if (!NT_SUCCESS(status)) {
        //     return status;
        // } __REACTOS__

        if (Settings->Enabled == WdfUseDefault) {
            //
            // Read the registry entry for idle enabled if it's the first time.
            //
            if (firstTime && Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
                DECLARE_CONST_UNICODE_STRING(valueName, WDF_S0_IDLE_ENABLED_VALUE_NAME);

                //
                // Read registry. If registry value is not found, the value of
                // "enabled" remains unchanged
                //
                ReadRegistryS0Idle(&valueName, &enabled);
            }
            else {
                //
                // Use the saved value for idle enabled.
                //
                enabled = m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled;
            }
        }

        overridable = TRUE;
    }
    else if (Settings->UserControlOfIdleSettings == IdleDoNotAllowUserControl) {
        //
        // No user control
        //
        overridable = FALSE;

        // (void) UpdateWmiInstanceForS0Idle(RemoveInstance); __REACTOS__
    }

    //
    // !!!! DO NOT INTRODUCE FAILURES BEYOND THIS POINT !!!!
    //
    // We should not introduce any failures that are not due to driver errors
    // beyond this point. This is because we are going to commit the driver's
    // S0-idle settings now and any failure in the midst of that could leave us
    // in a bad state. Therefore, all failable code where the failure is beyond
    // the driver's control should be placed above this point.
    //
    // For example, a driver may want wake-from-S0 support, but the device may
    // not support it. We already checked for that failure above, before we
    // started committing any of the driver's S0-idle settings.
    //
    // Any failures below this point should only be due to driver errors, i.e.
    // the driver incorrectly calling the AssignS0IdleSettings DDI.
    //

    if (firstTime) {
        m_PowerPolicyMachine.m_Owner->m_IdleSettings.Set = TRUE;
        m_PowerPolicyMachine.m_Owner->m_IdleSettings.Overridable = overridable;
    }

    //
    // IdleTimeoutType is available only on > 1.9
    //
#ifndef __REACTOS__
    if (Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9)) {
        if (firstTime) {
             if ((SystemManagedIdleTimeout == Settings->IdleTimeoutType) ||
                 (SystemManagedIdleTimeoutWithHint ==
                                        Settings->IdleTimeoutType)) {
                //
                // This is the first time S0-idle policy is being specified and
                // the caller has asked for the idle timeout to be determined
                // by the power manager.
                //
                status = m_PowerPolicyMachine.m_Owner->m_IdleSettings.
                            m_TimeoutMgmt.UseSystemManagedIdleTimeout(
                                                            GetDriverGlobals()
                                                            );
                if (!NT_SUCCESS(status)) {
                    return status;
                }
            }
        } else {
            //
            // This is not the first time S0-idle policy is being specified.
            // Verify that the caller is not trying to change their mind about
            // whether the idle timeout is determined by the power manager.
            //
            BOOLEAN currentlyUsingSystemManagedIdleTimeout;
            BOOLEAN callerWantsSystemManagedIdleTimeout;

            currentlyUsingSystemManagedIdleTimeout =
                m_PowerPolicyMachine.m_Owner->m_IdleSettings.m_TimeoutMgmt.
                                                UsingSystemManagedIdleTimeout();
            callerWantsSystemManagedIdleTimeout =
              ((SystemManagedIdleTimeout == Settings->IdleTimeoutType) ||
               (SystemManagedIdleTimeoutWithHint == Settings->IdleTimeoutType));

            //
            // UMDF currently does not implement
            // IdleTimeoutManagement::_SystemManagedIdleTimeoutAvailable. So
            // that method must be called only as part of the second check in
            // the "if" statement below. Since UMDF currently does not support
            // system managed idle timeout, the first check will always evaluate
            // to 'FALSE', so the second check never gets executed for UMDF.
            //
            if ((callerWantsSystemManagedIdleTimeout !=
                            currentlyUsingSystemManagedIdleTimeout)
                 &&
                (IdleTimeoutManagement::_SystemManagedIdleTimeoutAvailable())
                ) {

                status = STATUS_INVALID_DEVICE_REQUEST;
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "A previous call to assign S0-idle policy specified that "
                    "the idle timeout %s be determined by the power manager. "
                    "This decision cannot be changed. %!STATUS!",
                    currentlyUsingSystemManagedIdleTimeout ?
                      "should" :
                      "should not",
                    status);
                FxVerifierDbgBreakPoint(GetDriverGlobals());
                return status;
            }
        }
    }
#endif

    if (Settings->IdleCaps == IdleCannotWakeFromS0) {
        //
        // PowerUpIdleDeviceOnSystemWake field added after v1.7.
        // By default KMDF uses an optimization where the device is not powered
        // up when resuming from Sx if it is idle. The field
        // PowerUpIdleDeviceOnSystemWake is used to turn off this optimization and allow
        // device to power up when resuming from Sx. Note that this optimization
        // is applicable only for IdleCannotWakeFromS0. In other cases the
        // device is always powered up in order to arm for wake.
        //
        powerUpOnSystemWake =
            (Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_7)) ?
            Settings->PowerUpIdleDeviceOnSystemWake :
            WdfUseDefault;

        switch(powerUpOnSystemWake) {
        case WdfTrue:
            m_PowerPolicyMachine.m_Owner->m_IdleSettings.PowerUpIdleDeviceOnSystemWake = TRUE;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "Driver turned off S0Idle optimization. Device will be "
                "powered up on resume from Sx even when it is idle");
            break;
        case WdfFalse:
            m_PowerPolicyMachine.m_Owner->m_IdleSettings.PowerUpIdleDeviceOnSystemWake = FALSE;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "Driver turned on S0Idle optimization. Device will remain "
                "powered off if idle when resuming from Sx");
            break;
        case WdfUseDefault:
            DO_NOTHING();
            break;
        default:
            break;
        }
    }

    if (FALSE ==
            m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapabilityKnown)
    {
        if (Settings->IdleCaps == IdleUsbSelectiveSuspend) {
            m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable = TRUE;
            m_PowerPolicyMachine.m_Owner->
                m_IdleSettings.UsbSSCapabilityKnown = TRUE;

        } else if (Settings->IdleCaps == IdleCanWakeFromS0) {

            m_PowerPolicyMachine.m_Owner->
                m_IdleSettings.UsbSSCapabilityKnown = TRUE;
        }
    }

    //
    // Wake FromS0Capable is set every time because we want to allow the driver
    // to swap between idle wake capable and idle not wake capable.  This should
    // be allowed so that a scenario similar to the following can be implemented:
    //
    // a) when the device has an outstanding open, the device should arm itself
    //    for wake when idle
    //
    // b)  when the device does not have an outstanding open, the device should
    //     be off and not armed.
    //
    // The only way to be off is to assign S0 wake settings, so the
    // WakeFromS0Capable field must change on each DDI call.  This is not a
    // problem for the power policy state machine because it evaluates
    // WakeFromS0Capable state before enabling of idle.  If we are not
    // WakeFromS0Capable and USB SS capable (ie a have a torn/unsynchronized
    // state) in m_IdleSettings, we will recover from it when processing
    // PwrPolS0IdlePolicyChanged in the state machine (b/c this event causes
    // both fields to be reevaluated).
    //
    m_PowerPolicyMachine.m_Owner->m_IdleSettings.WakeFromS0Capable = s0Capable;

    m_PowerPolicyMachine.m_Owner->m_IdleSettings.DxState = dxState;

    if (m_PowerPolicyMachine.m_Owner->
            m_IdleSettings.m_TimeoutMgmt.UsingSystemManagedIdleTimeout()) {
        //
        // With system managed idle timeout, we don't want to apply an idle
        // timeout of our own on top of that. Effectively, our idle timeout is
        // 0.
        // But we apply a negligibly small timeout value as this allows us to
        // keep the same logic in the idle state machine, regardless of whether
        // we're using system-managed idle timeout or driver-managed idle
        // timeout.
        //
        if (firstTime) {
            m_PowerPolicyMachine.m_Owner->
                m_PowerIdleMachine.m_PowerTimeout.QuadPart =
                                            negliblySmallIdleTimeout;
        }

        if (SystemManagedIdleTimeoutWithHint == Settings->IdleTimeoutType) {
            //
            // We save the idle timeout hint, but we don't provide the hint to
            // the power framework immediately. This is because currently we may
            // or may not be registered with the power framework. Note that
            // WdfDeviceAssignS0IdleSettings might get called even when we are
            // not registered with the power framework.
            //
            // Therefore, we provide the hint to the power framework only when
            // we get to the WdfDevStatePwrPolStartingDecideS0Wake state. This
            // state is a good choice for providing the hint because:
            //   1. We know we would be registered with the power framework when
            //      we are in this state.
            //   2. Any change in S0-idle settings causes us to go through this
            //      state.
            //
            m_PowerPolicyMachine.m_Owner->m_PoxInterface.m_NextIdleTimeoutHint =
                                                                    idleTimeout;
        }

    } else {
        m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.m_PowerTimeout.QuadPart
            = WDF_REL_TIMEOUT_IN_MS(idleTimeout);
    }

    //
    // If the driver is 1.11 or later, update the bus drivers with the client's
    // choice on the topic of D3hot or D3cold.
    //
    if ((Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9)) &&
        (Settings->ExcludeD3Cold != WdfUseDefault)) {
        MxDeviceObject deviceObject;
        BOOLEAN enableD3Cold;

        deviceObject.SetObject(m_Device->GetDeviceObject());

        switch (Settings->ExcludeD3Cold) {
        case WdfFalse:
            enableD3Cold = TRUE;
            break;
        default:
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Invalid tri-state value for ExcludeD3Cold %d",
                Settings->ExcludeD3Cold);
            __fallthrough;
        case WdfTrue:
            enableD3Cold = FALSE;
            break;
        }

        SetD3ColdSupport(GetDriverGlobals(),
                         &deviceObject,
                         &m_D3ColdInterface,
                         enableD3Cold);
    }

    PowerPolicySetS0IdleState(enabled);

    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::PowerPolicySetSxWakeSettings(
    __in PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Settings,
    __in BOOLEAN ArmForWakeIfChildrenAreArmedForWake,
    __in BOOLEAN IndicateChildWakeOnParentWake
    )
/*++

Routine Description:

    Updates the Sx wake settings for the device.  No event is posted to the
    state machine because this setting is statically checked when the machine
    is entering an Sx state (unlike S0 idle which can be checked at any time).

    The first this function is called, the ability to allow the user to control
    this setting is set.

Arguments:

    Settings - the new settings to apply

    ArmForWakeIfChildrenAreArmedForWake - Inidicates whether the device
        should arm for wake when one or more children are armed for wake

    IndicateChildWakeOnParentWake - Indicates whether the device should
        propagate the wake status to its children

Return Value:

    NTSTATUS

  --*/
{
    DEVICE_POWER_STATE dxState;
    NTSTATUS status;
    BOOLEAN overridable, firstTime, enabled;

    dxState = PowerDeviceD3;
    overridable = FALSE;
    firstTime = TRUE;

    if (Settings->Enabled == WdfTrue) {
        enabled = TRUE;

    }
    else if (Settings->Enabled == WdfUseDefault) {
        enabled = TRUE;

        if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
            DECLARE_CONST_UNICODE_STRING(valueName, WDF_SX_WAKE_DEFAULT_VALUE_NAME);

            //
            // Read registry. If registry value is not found, the value of "enabled"
            // remains unchanged
            //
            ReadRegistrySxWake(&valueName, &enabled);
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                "If registry value WdfDefaultWakeFromSleepState was present, "
                "it was not read because DDI WdfDeviceAssignSxWakeSettings "
                "was not called at PASSIVE_LEVEL");
        }
    }
    else {
        enabled = FALSE;
    }

    if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.Set) {
        firstTime = FALSE;
    }

    if (m_CapsQueried == FALSE && Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
        status = QueryForCapabilities();
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Do not set m_CapsQueried to TRUE yet because we will do that once we
        // know the entire stack has been built we will do the query again.
        //
    }

    if (Settings->DxState == PowerDeviceMaximum) {
        dxState = PowerPolicyGetDeviceDeepestDeviceWakeState((SYSTEM_POWER_STATE)m_SystemWake);

        //
        // Some bus drivers

        // incorrectly report DeviceWake=D0 to
        // indicate that it does not support wake instead of specifying
        // PowerDeviceUnspecified and KMDF ends up requesting
        // a D0 irp when going to Dx. The check prevents this bug.
        //
        if (dxState < PowerDeviceD1 || dxState > PowerDeviceD3) {
            status = STATUS_POWER_STATE_INVALID;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "DeviceWake power state reported in device capabilities "
                "%!DEVICE_POWER_STATE! indicates that device can not signal a "
                "wake event, %!STATUS!",
                dxState, status);
            return status;
        }
    }
    else {
        DEVICE_POWER_STATE dxDeepest;

        dxState = Settings->DxState;
        dxDeepest = PowerPolicyGetDeviceDeepestDeviceWakeState((SYSTEM_POWER_STATE)m_SystemWake);

        if (dxState > dxDeepest) {
            status = STATUS_POWER_STATE_INVALID;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "DxState specified by driver %!DEVICE_POWER_STATE! cannot be"
                " lighter than lightest available device wake state "
                "%!DEVICE_POWER_STATE!, %!STATUS!", dxState,
                dxDeepest, status);
            return status;
        }
    }

    if (Settings->UserControlOfWakeSettings == WakeAllowUserControl) {

        // status = UpdateWmiInstanceForSxWake(AddInstance); __REACTOS__

        // if (!NT_SUCCESS(status)) {
        //     return status;
        // }

        if (Settings->Enabled == WdfUseDefault) {
            //
            // Read the registry entry for wake enabled if it's the first time.
            //
            if (firstTime && Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
                DECLARE_CONST_UNICODE_STRING(valueName, WDF_SX_WAKE_ENABLED_VALUE_NAME);

                //
                // Read registry. If registry value is not found, the value of
                // "enabled" remains unchanged
                //
                ReadRegistrySxWake(&valueName, &enabled);
            }
            else {
                //
                // Use the saved value for wake enabled.
                //
                enabled = m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled;
            }
        }

        overridable = TRUE;
    }
    else if (Settings->UserControlOfWakeSettings == WakeDoNotAllowUserControl) {
        //
        // No user control, just set to enabled
        //
        overridable = FALSE;

        // (void) UpdateWmiInstanceForSxWake(RemoveInstance); __REACTOS__
    }

    if (firstTime) {
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Set = TRUE;
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Overridable = overridable;

        //
        // If ArmForWakeIfChildrenAreArmedForWake setting is set to FALSE,
        // then we use the legacy framework behavior which did not depend
        // on the child device being capable of arming for wake or not.
        //
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.ArmForWakeIfChildrenAreArmedForWake =
            ArmForWakeIfChildrenAreArmedForWake;

        //
        // If IndicateChildWakeOnParentWake setting is set to FALSE, then
        // we use the legacy framework behavior wherein the wake status
        // is not propagated from the parent device to the child device.
        //
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.IndicateChildWakeOnParentWake =
            IndicateChildWakeOnParentWake;
    }

    m_PowerPolicyMachine.m_Owner->m_WakeSettings.DxState = dxState;

    PowerPolicySetSxWakeState(enabled);

    return STATUS_SUCCESS;
}

















NTSTATUS
FxPkgPnp::_S0IdleQueryInstance(
    __in  CfxDevice* Device,
    __in  FxWmiInstanceInternal* /* Instance */,
    __in  ULONG /* OutBufferSize */,
    __out PVOID OutBuffer,
    __out PULONG BufferUsed
    )
{
    *((BOOLEAN*) OutBuffer) =
        (Device->m_PkgPnp)->m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled;
    *BufferUsed = sizeof(BOOLEAN);

    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::_S0IdleSetInstance(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* /* Instance */,
    __in ULONG /* InBufferSize */,
    __in PVOID InBuffer
    )
{
    BOOLEAN value;


    //
    // FxWmiIrpHandler makes sure the buffer is at least one byte big, so we
    // don't check the buffer size.
    //

    value = *(PBOOLEAN) InBuffer;

    (Device->m_PkgPnp)->PowerPolicySetS0IdleState(value);

    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::_S0IdleSetItem(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* /* Instance */,
    __in ULONG DataItemId,
    __in ULONG InBufferSize,
    __in PVOID InBuffer
    )
{
    BOOLEAN value;

    if (DataItemId != 0) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (InBufferSize < sizeof(BOOLEAN)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    value = *(BOOLEAN*) InBuffer;
    (Device->m_PkgPnp)->PowerPolicySetS0IdleState(value);

    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::_SxWakeQueryInstance(
    __in  CfxDevice* Device,
    __in  FxWmiInstanceInternal* /* Instance */,
    __in  ULONG /* OutBufferSize */,
    __out PVOID OutBuffer,
    __out PULONG BufferUsed
    )
{
    *((BOOLEAN*) OutBuffer) =
        (Device->m_PkgPnp)->m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled;
    *BufferUsed = sizeof(BOOLEAN);

    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::_SxWakeSetInstance(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* /* Instance */,
    __in ULONG /* InBufferSize */,
    __in PVOID InBuffer
    )
{
    BOOLEAN value;

    //
    // FxWmiIrpHandler makes sure that the buffer is at least one byte big, so
    // we don't check the buffer size
    //
    value = *(PBOOLEAN) InBuffer;

    (Device->m_PkgPnp)->PowerPolicySetSxWakeState(value);

    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::_SxWakeSetItem(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* /* Instance */,
    __in ULONG DataItemId,
    __in ULONG InBufferSize,
    __in PVOID InBuffer
    )
{
    BOOLEAN value;

    if (DataItemId != 0) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (InBufferSize < sizeof(BOOLEAN)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    value = *(BOOLEAN*) InBuffer;
    (Device->m_PkgPnp)->PowerPolicySetSxWakeState(value);

    return STATUS_SUCCESS;
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
        PowerPolicyCanWakeFromSystemState(QueryState)) {
        //
        // If the query is for the machine's minimum S state or we going into
        // hibernate or off, always succeed it.
        //
        status = STATUS_SUCCESS;
    }
    else {

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

VOID
FxPkgPnp::PowerPolicySetS0IdleState(
    __in BOOLEAN State
    )
{
    m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled = State ? TRUE : FALSE;
    m_PowerPolicyMachine.m_Owner->m_IdleSettings.Dirty = TRUE;
    PowerPolicyProcessEvent(PwrPolS0IdlePolicyChanged);
}

VOID
FxPkgPnp::PowerPolicySetSxWakeState(
    __in BOOLEAN State
    )
/*++

Routine Description:
    Sets the wake from Sx state

    No need to post an event to the power policy state machine because we
    will not change any active due to a change in this setting.  We only
    evaluate this state when going into Sx and once in this state we will not
    change our behavior until the next Sx, which will then evaluate this state.

Arguments:
    State - New state

Return Value:
    VOID

  --*/
{
    m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled = State ? TRUE : FALSE;
    m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty = TRUE;

    //
    // Since we are not posting an event to the power policy state machine, try
    // to write out the value now, otherwise it will be written when we
    // transition
    //
    if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
        NTSTATUS status;
        LONGLONG timeout;

        timeout = 0;

        //
        // If the lock is already acquired on this thread, this will fail, which
        // is OK.
        //
        status = m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
            GetDriverGlobals(),
            &timeout
            );

        if (FxWaitLockInternal::IsLockAcquired(status)) {
            SaveState(TRUE);

            m_PowerPolicyMachine.m_StateMachineLock.ReleaseLock(
                GetDriverGlobals()
                );
        }
    }
}

VOID
FxPkgPnp::SetDeviceFailed(
    __in WDF_DEVICE_FAILED_ACTION FailedAction
    )
/*++

Routine Description:
    Marks the device as a victim of catastrophic failure, either in software
    or in hardware.

    If AttemptToRestart is TRUE, then we should try to get the stack re-built
    after it has been torn down.  This would typically be the case the failure
    was in the software, and possibly not be the case if the failure was in
    the hardware.

Arguments:
    FailedAction - action to take once the stack has been removed

Return Value:
    None

  --*/
{
    NTSTATUS    status;
    MdDeviceObject pdo;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(2, 15) == FALSE &&
        FailedAction == WdfDeviceFailedAttemptRestart) {

        FailedAction = WdfDeviceFailedNoRestart;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGDEVICE,
            "WdfDeviceFailedAttemptRestart is only available for UMDF 2.15 "
            "and later drivers. Reverting to WdfDeviceFailedNoRestart.");
    }
#endif

    m_FailedAction = (BYTE) FailedAction;

    //
    // This will cause the PnP manager to tear down this stack, even if
    // the PDO can't be surprise-removed.
    //
    m_Failed = TRUE;

    if (FailedAction == WdfDeviceFailedAttemptRestart) {
        //
        // Attempt to get the PDO surprise-removed.
        //
        status = AskParentToRemoveAndReenumerate();

        if (NT_SUCCESS(status)) {
            return;
        }
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // In between creating a PDO WDFDEVICE and it starting, if this DDI is called,
    // we will not have a valid PDO.  Make sure it is valid before we proceed.
    //
    pdo = m_Device->GetSafePhysicalDevice();

    if (pdo != NULL) {
        //
        // Now tell the PnP manager to re-query us for our state.
        //
        MxDeviceObject physicalDeviceObject(pdo);

        physicalDeviceObject.InvalidateDeviceState(
            m_Device->GetDeviceObject()
            );
    }
#else // USER_MODE
    m_Device->GetMxDeviceObject()->InvalidateDeviceState(
        m_Device->GetDeviceObject());
    UNREFERENCED_PARAMETER(pdo);
#endif
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::_PnpDeviceUsageNotification(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return This->PnpDeviceUsageNotification(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpDeviceUsageNotification(
    __inout FxIrp* Irp
    )
{
    FxRelatedDeviceList* pList;
    FxRelatedDevice *pDependent;
    FxAutoIrp relatedIrp(NULL), parentIrp(NULL);
    MxDeviceObject topOfParentStack;
    DEVICE_USAGE_NOTIFICATION_TYPE type;
    NTSTATUS status;
    MxDeviceObject pAttached;
    MdIrp pNewIrp;
    CCHAR maxStack;
    BOOLEAN inPath, supported;
    ULONG oldFlags;
    MxAutoWorkItem workItem;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering DeviceUsageNotification handler");

    status = STATUS_SUCCESS;

    type = Irp->GetParameterUsageNotificationType();
    inPath = Irp->GetParameterUsageNotificationInPath();
    supported = FALSE;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "type %x, in path %x, can support paging %x, dump file %x, "
        "hiber file %x, boot file %x",
        type, inPath,
        IsUsageSupported(_SpecialTypeToUsage(WdfSpecialFilePaging)),
        IsUsageSupported(_SpecialTypeToUsage(WdfSpecialFileDump)),
        IsUsageSupported(_SpecialTypeToUsage(WdfSpecialFileHibernation)),
        IsUsageSupported(_SpecialTypeToUsage(WdfSpecialFileBoot)));


    if (type >= static_cast<DEVICE_USAGE_NOTIFICATION_TYPE>(WdfSpecialFilePaging)
        && type < static_cast<DEVICE_USAGE_NOTIFICATION_TYPE>(WdfSpecialFileMax)) {
        if (inPath) {
            if (m_Device->IsFilter()) {
                //
                // Filters always support usage notifications
                //
                supported = TRUE;
            }
            else {
                supported = IsUsageSupported(type);
            }
        }
        else {
            //
            // We always handle notifications where we are out of the path
            //
            supported = TRUE;
        }
    }

    if (supported == FALSE) {
        status = STATUS_NOT_IMPLEMENTED;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Usage type %x not supported, %!STATUS!", type, status);

        return CompletePnpRequest(Irp, status);
    }

    //
    // Usage notification IRP gets forwarded to parent stack or to
    // dependent stack. Since in such cases (with deep device tree) the stack
    // may run out quickly, ensure there is enough stack, otherwise use a
    // workitem.
    //
    if (Mx::MxHasEnoughRemainingThreadStack() == FALSE &&
        (m_Device->IsPdo() ||
        m_UsageDependentDeviceList != NULL)) {

        status = workItem.Allocate(m_Device->GetDeviceObject());
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p !devobj %p could not allocate workitem "
                "to send usage notification type %d, inpath %d, %!STATUS!",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                type, inPath, status);

            return CompletePnpRequest(Irp, status);
        }
    }

    //
    // Usage notification is supported.  Set the flags on the device object
    // before processing this any further and save the current flags on the
    // device object.
    //
    oldFlags = SetUsageNotificationFlags(type, inPath);

    if (m_Device->IsPdo()) {












        topOfParentStack.SetObject(
            m_Device->m_ParentDevice->GetAttachedDeviceReference());

        pNewIrp = FxIrp::AllocateIrp(topOfParentStack.GetStackSize());
        if (pNewIrp != NULL) {
            parentIrp.SetIrp(pNewIrp);

            //
            // parentIrp now owns the irp
            //
            pNewIrp = NULL;

            status = SendDeviceUsageNotification(&topOfParentStack,
                                                 &parentIrp,
                                                 &workItem,
                                                 Irp,
                                                 FALSE);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p could not allocate PIRP for parent !devobj %p to "
                "send usage notification type %d, inpath %d, %!STATUS!",
                m_Device->GetHandle(), topOfParentStack.GetObject(),
                type, inPath, status);
        }
        topOfParentStack.DereferenceObject();
        topOfParentStack.SetObject(NULL);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "Exit %!STATUS!", status);

            RevertUsageNotificationFlags(type, inPath, oldFlags);
            return CompletePnpRequest(Irp, status);
        }
    }

    maxStack = 0;
    pDependent = NULL;

    //
    // If the driver supports the given special file, lets notify dependent
    // stacks.
    //

    //
    // LockForEnum will lock out new changes to the list until we unlock the list.
    // We remain at passive level once we are locked.
    //
    if (m_UsageDependentDeviceList != NULL) {
        //
        // We capture the m_UsageDependentDeviceList pointer value so that we
        // always use the same pointer value and that we have matched actions
        // (lock for enum / unlock from enum).  What we are trying to avoid is
        // this:
        // 1) we do not lock for enum because m_UsageDependentDeviceList == NULL
        // 2) in the middle of this function, m_UsageDependentDeviceList is
        //    assigned a pointer value
        // 3) we try to unlock from enum later (or iterate, thinking the enum
        //    lock is held) by checking m_UsageDependentDeviceList for NULL, and
        //    now that is != NULL, use it.
        //
        // By capturing the pointer now, we either have a list or not and we don't
        // hit situations 2 or 3.  So, the rule is every subseqeunt time we need
        // to check if there is valid m_UsageDependentDeviceList pointer, we
        // use pList, but when we use the list, we can use m_UsageDependentDeviceList
        // directly.
        //
        pList = m_UsageDependentDeviceList;

        m_UsageDependentDeviceList->LockForEnum(GetDriverGlobals());

        while ((pDependent = m_UsageDependentDeviceList->GetNextEntry(pDependent)) != NULL) {

            MxDeviceObject deviceObject(pDependent->GetDevice());













            pAttached.SetObject(deviceObject.GetAttachedDeviceReference());

            if (pAttached.GetStackSize() > maxStack) {
                maxStack = pAttached.GetStackSize();
            }

            pAttached.DereferenceObject();
        }
    }
    else {
        pList = NULL;
    }

    if (maxStack > 0) {
        //
        // If we have a maxStack size, we have a list as well
        //
        ASSERT(m_UsageDependentDeviceList != NULL);

        //
        // Allocate one irp for all the stacks so that we don't have an
        // allocation later.  This way, once we have the irp, we can send the
        // usage notification to all stacks reliably, as well as the reverting
        // of the notification if we encounter failure.
        //
        pNewIrp = FxIrp::AllocateIrp(maxStack);
        if (pNewIrp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p could not allocate IRP to send usage notifications"
                " to related stacks, type %d, inpath %d, status %!STATUS!",
                m_Device->GetHandle(), type, inPath, status);
        }
        else {
            MxDeviceObject dependentDevice;

            //
            // relatedIrp will free the irp when it goes out of scope
            //
            relatedIrp.SetIrp(pNewIrp);

            //
            // Walk our collection of dependent device stacks, and notify
            // each stack of the device notification.  If any fail, notify
            // the stacks who already were told of the reverted behavior.
            //
            pDependent = NULL;
            while ((pDependent = m_UsageDependentDeviceList->GetNextEntry(pDependent)) != NULL) {
                dependentDevice.SetObject(pDependent->GetDevice());
                status = SendDeviceUsageNotification(&dependentDevice,
                                                     &relatedIrp,
                                                     &workItem,
                                                     Irp,
                                                     FALSE);

                if (!NT_SUCCESS(status)) {
                    FxRelatedDevice* pDependent2;

                    pDependent2 = NULL;

                    //
                    // A device failed the device usage notification request.
                    // Notify the stacks that didn't fail, so they can unwind
                    // the operation.
                    //
                    while ((pDependent2 = m_UsageDependentDeviceList->GetNextEntry(pDependent2)) != NULL &&
                           pDependent2 != pDependent) {
                        dependentDevice.SetObject(pDependent2->GetDevice());

                        //
                        // We're already in a failure path. We can't do anything
                        // about yet another failure. So we ignore the return
                        // value.
                        //
                        (void) SendDeviceUsageNotification(&dependentDevice,
                                                           &relatedIrp,
                                                           &workItem,
                                                           Irp,
                                                           TRUE);
                    }

                    //
                    // Now break out of our outter loop.
                    //
                    break;
                }
            }
        }
    }

    //
    // If we are successful to this point, then send the IRP down the
    // stack.
    //
    if (NT_SUCCESS(status)) {
        BOOLEAN referenceSucceeded, sendDown;

        referenceSucceeded = FALSE;
        sendDown = TRUE;

        //
        // Make sure the stack is in D0 before sending down the request.  This
        // will at least guarantee that all devices below this one are in D0
        // when the make the transition from power pageable to non or vice versa.
        //
        if (IsPowerPolicyOwner()) {
            status = PowerReference(TRUE);

            if (NT_SUCCESS(status)) {
                referenceSucceeded = TRUE;
            }
            else {
                Irp->SetStatus(status);
                sendDown = FALSE;
            }
        }

        if (sendDown) {
            //
            // If we supported the usage, set the status to success, otherwise we
            // keep the status in the irp as it arrived to this device
            //
            if (supported) {
                Irp->SetStatus(status);
            }
            status = SendIrpSynchronously(Irp);
        }

        //
        // Transitioning from a thread which was power pagable to non power
        // pagable.  We now need a power thread for the stack, ask for it.
        // Note that there is no need for power thread in case of "boot"
        // notification since boot notification doesn't require clearing device's
        // DO_POWER_PAGABLE flag (power thread is required when handling power
        // irp at dispatch level which can happen if the DO_POWER_PAGABLE flag
        // is cleared).
        //
        // NOTE:  Once we have a power thread, we never go back to using work
        //        items even though the stack may revert to power pagable.
        //        This is an acceptable tradeoff between resource usage and
        //        WDF complexity.
        //
        //
        if (NT_SUCCESS(status) &&
            inPath &&
            (HasPowerThread() == FALSE) &&
            type != static_cast<DEVICE_USAGE_NOTIFICATION_TYPE>(WdfSpecialFileBoot)
            ) {
            status = QueryForPowerThread();

            if (!NT_SUCCESS(status)) {
                //
                // Keep status the same through out so we can set it back in
                // the irp when we are done.
                //
                if (m_Device->IsPdo()) {
                    //
                    // need to revert our parent's stack
                    //












                    topOfParentStack.SetObject(
                        m_Device->m_ParentDevice->GetAttachedDeviceReference());

                    //
                    // Ignore the status because we can't do anything on failure
                    //
                    (void) SendDeviceUsageNotification(&topOfParentStack,
                                                       &parentIrp,
                                                       &workItem,
                                                       Irp,
                                                       TRUE);

                    topOfParentStack.DereferenceObject();
                }
                else {
                    //
                    // Notify the stack below us
                    //
                    Irp->CopyCurrentIrpStackLocationToNext();
                    Irp->SetParameterUsageNotificationInPath(FALSE);

                    //
                    // Required for pnp irps
                    //
                    Irp->SetStatus(STATUS_NOT_SUPPORTED);

                    //
                    // Ignore the status because we can't do anything on failure
                    //
                    (void) Irp->SendIrpSynchronously(m_Device->GetAttachedDevice());
                }

                Irp->SetStatus(status);
            }
        }

        //
        // Now check whether the lower devices succeeded or failed.  If they
        // failed, back out our changes and propogate the failure.
        //
        if (!NT_SUCCESS(status)) {
            //
            // Revert the flags set on the device object.
            //
            RevertUsageNotificationFlags(type, inPath, oldFlags);

            //
            // Notify dependent stacks of the failure.
            //
            pDependent = NULL;

            //
            // See pList initiatilazation as to why we compare pList for != NULL
            // and not m_UsageDependentDeviceList.
            //
            if (pList != NULL) {
                MxDeviceObject dependentDevice;

                while ((pDependent = m_UsageDependentDeviceList->GetNextEntry(pDependent)) != NULL) {
                    dependentDevice.SetObject(pDependent->GetDevice());

                    //
                    // We're already in a failure path. We can't do anything
                    // about yet another failure. So we ignore the return value.
                    //
                    (void) SendDeviceUsageNotification(&dependentDevice,
                                                       &relatedIrp,
                                                       &workItem,
                                                       Irp,
                                                       TRUE);
                }
            }
        }

        //
        // By this point, we have propagated the notification to dependent devices
        // and lower stack, and if anyone failed during that time, we also
        // propagated failure to dependent stacks and lower stack.
        // If status is success at this point, invoke the driver's callback.
        //
        if (NT_SUCCESS(status)) {
            //
            // Invoke callback. Note that only one of the callbacks
            // DeviceUsageNotification or DeviceUsgeNotificationEx will get
            // invoked since only one of the callbacks at a time is supported.
            // We ensured that during registration of the callback.
            // Note that Ex callback will return success if driver did not
            // supply any callback.
            //
            m_DeviceUsageNotification.Invoke(m_Device->GetHandle(),
                                             _UsageToSpecialType(type),
                                             inPath);

            status = m_DeviceUsageNotificationEx.Invoke(
                                        m_Device->GetHandle(),
                                        _UsageToSpecialType(type),
                                        inPath
                                        );

            if (!NT_SUCCESS(status)) {
                //
                // Driver's callback returned failure. We need to propagate
                // failure to lower stack and dependent stacks.
                //
                //
                // Keep status the same through out so we can set it back in
                // the irp when we are done.
                //
                if (m_Device->IsPdo()) {
                    //
                    // need to revert our parent's stack
                    //












                    topOfParentStack.SetObject(
                        m_Device->m_ParentDevice->GetAttachedDeviceReference());

                    //
                    // Ignore the status because we can't do anything on failure
                    //
                    (void) SendDeviceUsageNotification(&topOfParentStack,
                                                       &parentIrp,
                                                       &workItem,
                                                       Irp,
                                                       TRUE);

                    topOfParentStack.DereferenceObject();
                }
                else {
                    //
                    // Notify the stack below us
                    //
                    Irp->CopyCurrentIrpStackLocationToNext();
                    Irp->SetParameterUsageNotificationInPath(FALSE);

                    //
                    // Required for pnp irps
                    //
                    Irp->SetStatus(STATUS_NOT_SUPPORTED);

                    //
                    // Ignore the status because we can't do anything on failure
                    //
                    (void) Irp->SendIrpSynchronously(m_Device->GetAttachedDevice());
                }

                Irp->SetStatus(status);

                //
                // Revert the flags set on the device object.
                //
                RevertUsageNotificationFlags(type, inPath, oldFlags);

                //
                // Notify dependent stacks of the failure.
                //
                pDependent = NULL;

                //
                // See pList initiatilazation as to why we compare pList for != NULL
                // and not m_UsageDependentDeviceList.
                //
                if (pList != NULL) {
                    MxDeviceObject dependentDevice;

                    while ((pDependent = m_UsageDependentDeviceList->GetNextEntry(pDependent)) != NULL) {
                        dependentDevice.SetObject(pDependent->GetDevice());

                        //
                        // We're already in a failure path. We can't do anything
                        // about yet another failure. So we ignore the return value.
                        //
                        (void) SendDeviceUsageNotification(&dependentDevice,
                                                           &relatedIrp,
                                                           &workItem,
                                                           Irp,
                                                           TRUE);
                    }
                }
            }

            if (NT_SUCCESS(status)) {

                CommitUsageNotification(type, oldFlags);

                //
                // If we are in the dump file path, we cannot idle out because we
                // can experience a crash dump at any time.
                //
                if (IsPowerPolicyOwner() && type == DeviceUsageTypeDumpFile) {
                    //
                    // Add a reference everytime we are notified of being in the
                    // path, no need to match the first inPath notification and the
                    // last !inPath notification.
                    //
                    if (inPath) {
                        NTSTATUS refStatus;

                        ASSERT(GetUsageCount(type) > 0);

                        //
                        // Since our previous synchronous power reference succeeded,
                        // an addtional reference while we are powered up should
                        // never fail.
                        //
                        refStatus = PowerReference(FALSE);
#if DBG
                        ASSERT(NT_SUCCESS(refStatus));
#else
                        UNREFERENCED_PARAMETER(refStatus);
#endif
                    }
                    else {
                        ASSERT(GetUsageCount(type) >= 0);
                        PowerDereference();
                    }
                }
            }
        }

        //
        // We no longer need to be in D0 if we don't have to be.
        //
        if (referenceSucceeded) {
            ASSERT(IsPowerPolicyOwner());
            PowerDereference();
        }
    }

    //
    // See pList initiatilazation as to why we compare pList for != NULL
    // and not m_UsageDependentDeviceList.
    //
    if (pList != NULL) {
        m_UsageDependentDeviceList->UnlockFromEnum(GetDriverGlobals());
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit %!STATUS!", status);

    return CompletePnpRequest(Irp, status);
}

ULONG
FxPkgPnp::SetUsageNotificationFlags(
    __in DEVICE_USAGE_NOTIFICATION_TYPE Type,
    __in BOOLEAN InPath
    )
/*++

Routine Description:

    This routine sets the usage notification flags on the device object (for
    non-boot usages) and updates the special file usage count .

Arguments:

    Type - the special file type - paging, hibernate, dump or boot file.

    InPath - indicates whether the system is creating or removing the special
        file on the device.

Return Value:

    Returns the old flags on the device object.

--*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    ULONG oldFlags, newFlags;

    oldFlags = m_Device->GetDeviceObjectFlags();

    //

    //
    DoTraceLevelMessage(
        FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Before:  type %d, in path %d, special count %d, flags 0x%x, "
        "device %p (WDFDEVICE %p), is pageable capable %d",
        Type, InPath, GetUsageCount(Type), oldFlags,
        m_Device->GetDeviceObject(), m_Device->GetHandle(),
        m_Device->IsPowerPageableCapable());

    //
    // Adjust our "special file count".
    //
    AdjustUsageCount(Type, InPath);

    //
    // Boot notification doesn't require updating device flags.
    //
    if (Type == static_cast<DEVICE_USAGE_NOTIFICATION_TYPE>(WdfSpecialFileBoot)) {
        return oldFlags;
    }

    if (m_Device->IsFilter()) {
        //
        // Clear the previous flags and reset them to the attached
        // device's flags
        //
        newFlags = oldFlags & ~(DO_POWER_PAGABLE | DO_POWER_INRUSH);
        newFlags |= m_Device->GetAttachedDeviceObjectFlags() &
                (DO_POWER_PAGABLE | DO_POWER_INRUSH);
        m_Device->SetDeviceObjectFlags(newFlags);
    }
    else {
        if (InPath) {
            m_Device->SetDeviceObjectFlags(
                m_Device->GetDeviceObjectFlags() & ~DO_POWER_PAGABLE
                );
        }
        else {
            if (m_Device->IsPowerPageableCapable() && IsInSpecialUse() == FALSE) {
                m_Device->SetDeviceObjectFlags(
                    m_Device->GetDeviceObjectFlags() | DO_POWER_PAGABLE
                    );
            }
        }
    }

    return oldFlags;
}

VOID
FxPkgPnp::RevertUsageNotificationFlags(
    __in DEVICE_USAGE_NOTIFICATION_TYPE Type,
    __in BOOLEAN InPath,
    __in ULONG OldFlags
    )
/*++

Routine Description:

    This routine reverts the usage notification flags to the old flags on
    the device object and updates the special file usage count.

Arguments:

    Type - the special file type - paging, hibernate or dump file.

    InPath - indicates whether the system is creating or removing the special
        file on the device.

    OldFlags - the previous flags on the device object.

--*/
{
    //
    // Re-adjust our "special file count".
    //
    InPath = !InPath;
    AdjustUsageCount(Type, InPath);

    //
    // Restore the flags on the device object.
    //
    m_Device->SetDeviceObjectFlags(OldFlags);
}

VOID
FxPkgPnp::CommitUsageNotification(
    __in DEVICE_USAGE_NOTIFICATION_TYPE Type,
    __in ULONG OldFlags
    )
/*++

Routine Description:

    This routine commits the usage notification flags on the device object
    and invokes the usage notification callbacks.  If the current flags on
    the device object indicates that there was a transition from power-pagable
    to non power-pagable, or vice-versa, then an event is posted to the power
    state machine to notify it of the change.  After this routine is called
    the PNP manager will no longer be able to disable the device.

Arguments:

    Type - the special file type - paging, hibernation or crash dump file.

    InPath - indicates whether the system is creating or removing the special
        file on the device.

    OldFlags - the previous flags on the device object.

--*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    ULONG newFlags;

    newFlags = m_Device->GetDeviceObjectFlags();

    if ((OldFlags & DO_POWER_PAGABLE) == DO_POWER_PAGABLE &&
        (newFlags & DO_POWER_PAGABLE) == 0) {
        //
        // We transitioned from a power pageable to a non power pageable
        // device.  Move the power state machine to the appropriate
        // state.
        //
        PowerProcessEvent(PowerMarkNonpageable);
    }

    if ((OldFlags & DO_POWER_PAGABLE) == 0 &&
        (newFlags & DO_POWER_PAGABLE) == DO_POWER_PAGABLE) {
        //
        // We transitioned from a non power pageable to a power pageable
        // device.  Move the power state machine to the appropriate
        // state.
        //
        PowerProcessEvent(PowerMarkPageable);
    }

    //
    // Notify PNP that it should no longer be able
    // to disable this device.
    //
    MxDeviceObject physicalDeviceObject(
                                m_Device->GetPhysicalDevice()
                                );
    physicalDeviceObject.InvalidateDeviceState(
        m_Device->GetDeviceObject()
        );

    DoTraceLevelMessage(
        FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "After:  special count %d, flags 0x%x, device %p (WDFDEVICE %p)",
        GetUsageCount(Type), newFlags,
        m_Device->GetDeviceObject(), m_Device->GetHandle());
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::AddUsageDevice(
    __in MdDeviceObject DependentDevice
    )
{
    FxRelatedDevice* pRelated;
    NTSTATUS status;

    if (m_UsageDependentDeviceList == NULL) {
        KIRQL irql;

        Lock(&irql);
        if (m_UsageDependentDeviceList == NULL) {
            m_UsageDependentDeviceList = new (GetDriverGlobals()) FxRelatedDeviceList();

            if (m_UsageDependentDeviceList != NULL) {
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Could not allocate usage device list for WDFDEVICE %p, "
                    "%!STATUS!", m_Device->GetHandle(), status);
            }

        }
        else {
            //
            // another thread allocated the list already
            //
            status = STATUS_SUCCESS;
        }
        Unlock(irql);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    pRelated = new(GetDriverGlobals())
        FxRelatedDevice(DependentDevice, GetDriverGlobals());

    if (pRelated == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = m_UsageDependentDeviceList->Add(GetDriverGlobals(), pRelated);

    if (!NT_SUCCESS(status)) {
        pRelated->DeleteFromFailedCreate();
    }

    return status;
}

VOID
FxPkgPnp::RemoveUsageDevice(
    __in MdDeviceObject DependentDevice
    )
{
    if (m_UsageDependentDeviceList != NULL) {
        m_UsageDependentDeviceList->Remove(GetDriverGlobals(), DependentDevice);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::AddRemovalDevice(
    __in MdDeviceObject DependentDevice
    )
{
    FxRelatedDevice* pRelated;
    NTSTATUS status;

    if (m_RemovalDeviceList == NULL) {
        KIRQL irql;

        Lock(&irql);
        if (m_RemovalDeviceList == NULL) {
            m_RemovalDeviceList = new (GetDriverGlobals()) FxRelatedDeviceList();

            if (m_RemovalDeviceList != NULL) {
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Could not allocate removal device list for WDFDEVICE %p, "
                    "%!STATUS!", m_Device->GetHandle(), status);
            }
        }
        else {
            //
            // another thread allocated the list already
            //
            status = STATUS_SUCCESS;
        }
        Unlock(irql);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    pRelated = new(GetDriverGlobals())
        FxRelatedDevice(DependentDevice, GetDriverGlobals());
    if (pRelated == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = m_RemovalDeviceList->Add(GetDriverGlobals(), pRelated);

    if (NT_SUCCESS(status)) {
        //
        // RemovalRelations are queried automatically by PnP when the device is
        // going to be query removed.  No need to tell pnp that the list changed
        // until it needs to query for it.
        //
        DO_NOTHING();
    }
    else {
        pRelated->DeleteFromFailedCreate();
    }

    return status;
}

VOID
FxPkgPnp::RemoveRemovalDevice(
    __in MdDeviceObject DependentDevice
    )
{
    if (m_RemovalDeviceList != NULL) {
        m_RemovalDeviceList->Remove(GetDriverGlobals(), DependentDevice);
    }

    //
    // RemovalRelations are queried automatically by PnP when the device is
    // going to be query removed.  No need to tell pnp that the list changed
    // until it needs to query for it.
    //
}

VOID
FxPkgPnp::ClearRemovalDevicesList(
    VOID
    )
{
    FxRelatedDevice* pEntry;

    if (m_RemovalDeviceList == NULL) {
        return;
    }

    m_RemovalDeviceList->LockForEnum(GetDriverGlobals());
    while ((pEntry = m_RemovalDeviceList->GetNextEntry(NULL)) != NULL) {
        m_RemovalDeviceList->Remove(GetDriverGlobals(), pEntry->GetDevice());
    }
    m_RemovalDeviceList->UnlockFromEnum(GetDriverGlobals());

    //
    // RemovalRelations are queried automatically by PnP when the device is
    // going to be query removed.  No need to tell pnp that the list changed
    // until it needs to query for it.
    //
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

VOID
FxPkgPnp::SetPendingPnpIrp(
    __inout FxIrp* Irp,
    __in    BOOLEAN MarkIrpPending
    )
{
    if (m_PendingPnPIrp != NULL ) {
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
    if (MarkIrpPending) {
        Irp->MarkIrpPending();
    }
    m_PendingPnPIrp = Irp->GetIrp();
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::AllocateEnumInfo(
    VOID
    )
{
    KIRQL irql;
    NTSTATUS status;

    if (m_EnumInfo != NULL) {
        return STATUS_SUCCESS;
    }

    Lock(&irql);
    if (m_EnumInfo == NULL) {
        m_EnumInfo = new (GetDriverGlobals()) FxEnumerationInfo(GetDriverGlobals());

        if (m_EnumInfo != NULL) {
            status = m_EnumInfo->Initialize();

            if (!NT_SUCCESS(status)) {
                delete m_EnumInfo;
                m_EnumInfo = NULL;

                DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                                    TRACINGPNP,
                                    "Could not initialize enum info for "
                                    "WDFDEVICE %p, %!STATUS!",
                                    m_Device->GetHandle(), status);
            }

        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                                "Could not allocate enum info for WDFDEVICE %p, "
                                "%!STATUS!", m_Device->GetHandle(), status);
        }
    }
    else {
        //
        // another thread allocated the list already
        //
        status = STATUS_SUCCESS;
    }
    Unlock(irql);

    return status;
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
FxPkgPnp::RemoveChildList(
    __in FxChildList* List
    )
{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Removing FxChildList %p, WDFCHILDLIST %p", List,
                        List->GetHandle());

    m_EnumInfo->m_ChildListList.Remove(GetDriverGlobals(), &List->m_TransactionLink);

    //

    //
}

VOID
FxPkgPnp::ChildListNotifyRemove(
    __inout PLONG PendingCount
    )
{
    FxTransactionedEntry* ple;

    ple = NULL;

    if (m_EnumInfo != NULL) {
        m_EnumInfo->m_ChildListList.LockForEnum(GetDriverGlobals());
        while ((ple = m_EnumInfo->m_ChildListList.GetNextEntry(ple)) != NULL) {
            FxChildList::_FromEntry(ple)->NotifyDeviceRemove(PendingCount);
        }
        m_EnumInfo->m_ChildListList.UnlockFromEnum(GetDriverGlobals());
    }
}

VOID
FxPkgPnp::AddQueryInterface(
    __in FxQueryInterface* QI,
    __in BOOLEAN Lock
    )
/*++

Routine Description:
    Add a query interface structure to the list of interfaces supported

Arguments:
    QI - the interface to add

    Lock - indication of the list lock should be acquired or not

Return Value:
    None

  --*/
{
    SINGLE_LIST_ENTRY **ppPrev, *pCur;

    if (Lock) {
        m_QueryInterfaceLock.AcquireLock(GetDriverGlobals());
    }

    ASSERT(QI->m_Entry.Next == NULL);

    //
    // Iterate until we find the end of the list and then append the new
    // structure.  ppPrev is the pointer to the Next pointer value.  When we
    // get to the end, ppPrev will be equal to the Next field which points to NULL.
    //
    ppPrev = &m_QueryInterfaceHead.Next;
    pCur = m_QueryInterfaceHead.Next;

    while (pCur != NULL) {
        ppPrev = &pCur->Next;
        pCur = pCur->Next;
    }

    *ppPrev = &QI->m_Entry;

    if (Lock) {
        m_QueryInterfaceLock.ReleaseLock(GetDriverGlobals());
    }
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

    if (!NT_SUCCESS(status)) {
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

    if (pOld != NULL) {
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
    if (m_PowerThread != NULL) {

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

        if (InterlockedDecrement(&m_PowerThreadInterfaceReferenceCount) > 0) {
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
    else if (hadThread) {
        //
        // Release our reference
        //
        m_PowerThreadInterface.Interface.InterfaceDereference(
            m_PowerThreadInterface.Interface.Context
            );
    }
}

VOID
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

VOID
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

    if (InterlockedDecrement(&pThis->m_PowerThreadInterfaceReferenceCount) == 0) {
        pThis->m_PowerThreadEvent->Set();
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpPowerReferenceSelf(
    VOID
    )
/*++

Routine Description:
    Take a power reference during a query pnp transition.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (IsPowerPolicyOwner()) {
        //
        // We want to synchronously wait to move into D0
        //
        return PowerReference(TRUE);
    }
    else {
        return STATUS_SUCCESS;
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpPowerReferenceDuringQueryPnp(
    VOID
    )
/*++

Routine Description:
    Take a power reference during a query pnp transition.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (IsPowerPolicyOwner()) {
        //
        // We want to synchronously wait to move into D0
        //
        return m_PowerPolicyMachine.m_Owner->
            m_PowerIdleMachine.PowerReferenceWithFlags(
                FxPowerReferenceSendPnpPowerUpEvent
                );
    }
    else {
        return STATUS_SUCCESS;
    }
}

VOID
FxPkgPnp::PnpPowerDereferenceSelf(
    VOID
    )
/*++

Routine Description:
    Release the power reference taken during a query pnp transition

Arguments:
    None

Return Value:
    None

  --*/
{
    if (IsPowerPolicyOwner()) {
        PowerDereference();
    }
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


VOID
FxPkgPnp::SetPnpCaps(
    __in PWDF_DEVICE_PNP_CAPABILITIES PnpCapabilities
    )
/*++

Routine Description:
    Encode the driver provided pnp capabilities into our internal capabilities
    bit field and store the result.

Arguments:
    PnpCapabilities - capabilities as reported by the driver writer

Return Value:
    None

  --*/
{
    LONG pnpCaps;
    KIRQL irql;

    pnpCaps = 0;
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, LockSupported);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, EjectSupported);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, Removable);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, DockDevice);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, UniqueID);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, SilentInstall);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, SurpriseRemovalOK);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, HardwareDisabled);
    pnpCaps |= GET_PNP_CAP_BITS_FROM_STRUCT(PnpCapabilities, NoDisplayInUI);

    //
    // Since the caller of IRP_MN_QUERY_CAPABILITIES sets these 2 values to -1,
    // we can reuse the -1 as the default/no override value since the associated
    // PDEVICE_CAPAPBILITIES structure will have been predisposed to these values
    //
    if (PnpCapabilities->Address != (ULONG) -1) {
        m_PnpCapsAddress = PnpCapabilities->Address;
    }
    if (PnpCapabilities->UINumber != (ULONG) -1) {
        m_PnpCapsUINumber = PnpCapabilities->UINumber;
    }

    //
    // Use the FxPnpStateMask to keep the state mask while applying the new
    // pnp capabilities.
    //
    Lock(&irql);
    m_PnpStateAndCaps.Value = (m_PnpStateAndCaps.Value & FxPnpStateMask) | pnpCaps;
    Unlock(irql);
}

VOID
FxPkgPnp::GetPnpState(
    __out PWDF_DEVICE_STATE State
    )
/*++

Routine Description:
    Decodes our internal pnp state bitfield into the external WDF_DEVICE_STATE
    structure

Arguments:
    State - the structure to decode into

Return Value:
    None

  --*/
{
    LONG state;

    state = GetPnpStateInternal();

    SET_TRI_STATE_FROM_STATE_BITS(state, State, Disabled);
    SET_TRI_STATE_FROM_STATE_BITS(state, State, DontDisplayInUI);
    SET_TRI_STATE_FROM_STATE_BITS(state, State, Failed);
    SET_TRI_STATE_FROM_STATE_BITS(state, State, NotDisableable);
    SET_TRI_STATE_FROM_STATE_BITS(state, State, Removed);
    SET_TRI_STATE_FROM_STATE_BITS(state, State, ResourcesChanged);
}

VOID
FxPkgPnp::SetPnpState(
    __in PWDF_DEVICE_STATE State
    )
/*++

Routine Description:
    Encodes the driver writer provided state into our internal bit field.

Arguments:
    State - the states to encode

Return Value:
    None

  --*/
{
    LONG pnpState;
    KIRQL irql;

    pnpState = 0x0;
    pnpState |= GET_PNP_STATE_BITS_FROM_STRUCT(State, Disabled);
    pnpState |= GET_PNP_STATE_BITS_FROM_STRUCT(State, DontDisplayInUI);
    pnpState |= GET_PNP_STATE_BITS_FROM_STRUCT(State, Failed);
    pnpState |= GET_PNP_STATE_BITS_FROM_STRUCT(State, NotDisableable);
    pnpState |= GET_PNP_STATE_BITS_FROM_STRUCT(State, Removed);
    pnpState |= GET_PNP_STATE_BITS_FROM_STRUCT(State, ResourcesChanged);

    //
    // Mask off FxPnpCapMask to keep the capabilities part of the bitfield
    // the same while change the pnp state.
    //
    Lock(&irql);
    m_PnpStateAndCaps.Value = (m_PnpStateAndCaps.Value & FxPnpCapMask) | pnpState;
    Unlock(irql);
}

VOID
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

VOID
FxPkgPnp::SetPowerCaps(
    __in PWDF_DEVICE_POWER_CAPABILITIES PowerCapabilities
    )
/*++

Routine Description:
    Encodes the driver provided power capabilities into the object.  The device
    power states are encoded into one ULONG while the other power caps are
    encoded into their own distinct fields.

Arguments:
    PowerCapabilities - the power caps reported by the driver writer

Return Value:
    None

  --*/
{
    ULONG states, i;
    USHORT powerCaps;

    states = 0x0;

    //
    // Build up the device power state encoding into a temp var so that if we are
    // retrieving the encoding in another thread, we don't get a partial view
    //
    for (i = 0; i < ARRAY_SIZE(PowerCapabilities->DeviceState); i++) {
        _SetPowerCapState(i,  PowerCapabilities->DeviceState[i], &states);
    }

    m_PowerCaps.States = states;

    //
    // Same idea.  Build up the caps locally first so that when we assign them
    // into the object, it is assigned as a whole.
    //
    powerCaps = 0x0;
    powerCaps |= GET_POWER_CAP_BITS_FROM_STRUCT(PowerCapabilities, DeviceD1);
    powerCaps |= GET_POWER_CAP_BITS_FROM_STRUCT(PowerCapabilities, DeviceD2);
    powerCaps |= GET_POWER_CAP_BITS_FROM_STRUCT(PowerCapabilities, WakeFromD0);
    powerCaps |= GET_POWER_CAP_BITS_FROM_STRUCT(PowerCapabilities, WakeFromD1);
    powerCaps |= GET_POWER_CAP_BITS_FROM_STRUCT(PowerCapabilities, WakeFromD2);
    powerCaps |= GET_POWER_CAP_BITS_FROM_STRUCT(PowerCapabilities, WakeFromD3);

    m_PowerCaps.Caps = powerCaps;

    if (PowerCapabilities->DeviceWake != PowerDeviceMaximum) {
        m_PowerCaps.DeviceWake = (BYTE) PowerCapabilities->DeviceWake;
    }
    if (PowerCapabilities->SystemWake != PowerSystemMaximum) {
        m_PowerCaps.SystemWake = (BYTE) PowerCapabilities->SystemWake;
    }

    m_PowerCaps.D1Latency = PowerCapabilities->D1Latency;
    m_PowerCaps.D2Latency = PowerCapabilities->D2Latency;
    m_PowerCaps.D3Latency = PowerCapabilities->D3Latency;

    if (PowerCapabilities->IdealDxStateForSx != PowerDeviceMaximum) {
        //
        // Caller has already validated that IdealDxStateForSx is only set if
        // they device is the power policy owner.
        //
        m_PowerPolicyMachine.m_Owner->m_IdealDxStateForSx = (BYTE)
            PowerCapabilities->IdealDxStateForSx;
    }
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

BOOLEAN
FxPkgPnp::PowerPolicyIsWakeEnabled(
    VOID
    )
{
    if (IsPowerPolicyOwner() && PowerPolicyGetCurrentWakeReason() != 0x0) {
        return TRUE;
    }
    else {
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
        m_PowerPolicyMachine.m_Owner->m_ChildrenArmedCount > 0) {
        //
        // Wake settings depends on children and one or more children are
        // armed for wake.
        //
        wakeReason |= FxPowerPolicySxWakeChildrenArmedFlag;
    }

    if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled) {
        //
        // Wake settings is explicitly enabled.
        //
        wakeReason |= FxPowerPolicySxWakeDeviceEnabledFlag;
    }

    return wakeReason;
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
    if (IsPowerPolicyOwner() == FALSE) {
        return;
    }

    if (UseCanSaveState &&
        m_PowerPolicyMachine.m_Owner->m_CanSaveState == FALSE) {
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
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty == FALSE) {
        return;
    }

    //
    // No need to write out if user control is not enabled
    //
    if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.Overridable == FALSE &&
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Overridable == FALSE) {
        return;
    }

    //
    // If the device is in paging path we should not be touching registry during
    // power up because it may incur page fault which won't be satisfied if the
    // device is still not powered up, blocking power Irp. User control state
    // change will not get written if any at this time but will be flushed out
    // to registry during device disable/remove in the remove path.
    //
    if (IsUsageSupported(DeviceUsageTypePaging) && IsDevicePowerUpIrpPending()) {
        return;
    }

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))

    status = m_Device->OpenSettingsKey(&hKey.m_Key, STANDARD_RIGHTS_WRITE);
    if (!NT_SUCCESS(status)) {
        return;
    }
#else
    status = STATUS_SUCCESS;
#endif

    if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.Overridable &&
        m_PowerPolicyMachine.m_Owner->m_IdleSettings.Dirty) {
        RtlInitUnicodeString(&name, WDF_S0_IDLE_ENABLED_VALUE_NAME);
        value = m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled;

        WriteStateToRegistry(hKey.m_Key, &name, value);

        m_PowerPolicyMachine.m_Owner->m_IdleSettings.Dirty = FALSE;
    }

    if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.Overridable &&
        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty)  {
        RtlInitUnicodeString(&name, WDF_SX_WAKE_ENABLED_VALUE_NAME);
        value = m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled;

        WriteStateToRegistry(hKey.m_Key, &name, value);

        m_PowerPolicyMachine.m_Owner->m_WakeSettings.Dirty = FALSE;
    }
}

VOID
FxPkgPnp::AddInterruptObject(
    __in FxInterrupt* Interrupt
    )
/*++

Routine Description:

    This routine adds a WDFINTERRUPT object onto the list of interrupts
    which are attached to this device.  This list is used in response to
    several IRPs.

Note:

    It shouldn't be necessary to lock this list, since the driver will add or
    remove interrupt objects only in callbacks that are effectively serialized
    by the PnP manager.

Further note:

    This list must remain sorted by order of interrupt object creation.  E.g.
    the first interrupt object created must be first in this list.

Arguments:

    Interrupt - a Framework interrupt object

Return Value:

    VOID

--*/
{
    m_InterruptObjectCount++;
    InsertTailList(&m_InterruptListHead, &Interrupt->m_PnpList);
}

VOID
FxPkgPnp::RemoveInterruptObject(
    __in FxInterrupt* Interrupt
    )
/*++

Routine Description:
    This routine removes a WDFINTERRUPT object onto the list of interrupts
    which are attached to this device.  This list is used in response to
    several IRPs.

Arguments:

    Interrupt - a Framework interrupt object

Return Value:

    VOID

--*/
{
    m_InterruptObjectCount--;
    RemoveEntryList(&Interrupt->m_PnpList);
}

VOID
FxPkgPnp::NotifyResourceobjectsToReleaseResources(
    VOID
    )
/*++

Routine Description:

    This routine traverses all resource objects and tells them that their
    resources are no longer valid.

Arguments:

    none

Return Value:

    VOID

--*/
{
    FxInterrupt* interruptInstance;
    PLIST_ENTRY intListEntry;

    //
    // Revoke each of the interrupts.
    //

    intListEntry = m_InterruptListHead.Flink;

    while (intListEntry != &m_InterruptListHead) {

        //
        // Disconnect interrupts and then tell them that they no longer
        // own their resources.
        //

        interruptInstance = CONTAINING_RECORD(intListEntry, FxInterrupt, m_PnpList);
        interruptInstance->RevokeResources();
        intListEntry = intListEntry->Flink;
    }

    //
    // Now revoke each of the DMA enablers (only system-mode enablers
    // will be affected by this)
    //

    if (m_DmaEnablerList != NULL) {
        FxTransactionedEntry* listEntry;

        m_DmaEnablerList->LockForEnum(GetDriverGlobals());

        for (listEntry = m_DmaEnablerList->GetNextEntry(NULL);
             listEntry != NULL;
             listEntry = m_DmaEnablerList->GetNextEntry(listEntry)) {
            RevokeDmaEnablerResources(
                (FxDmaEnabler *) listEntry->GetTransactionedObject()
                );
        }

        m_DmaEnablerList->UnlockFromEnum(GetDriverGlobals());
    }
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
         ple = ple->Flink) {

        //
        // Connect the interrupts
        //
        pInterrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

        status = pInterrupt->Connect(NotifyFlags);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                                "WDFINTERRUPT %p failed to connect, %!STATUS!",
                                pInterrupt->GetHandle(), status);

            return status;
        }
    }

    return STATUS_SUCCESS;
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
         ple = ple->Blink) {

        //
        // Disconnect interrupts and then tell them that they no longer
        // own their resources.
        //
        pInterrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

        status = pInterrupt->Disconnect(NotifyFlags);

        if (!NT_SUCCESS(status)) {
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

VOID
FxPkgPnp::SendEventToAllWakeInterrupts(
    __in FxWakeInterruptEvents WakeInterruptEvent
    )
/*++

Routine Description:
    This routine traverses all interrupt objects and queues the
    given event into those interrupts that are marked as wake
    capable and have a state machine

Arguments:
    WakeInterruptEvent - Event to be queued in the wake interrupt
                         state machines

Return Value:
    None

--*/
{
    FxInterrupt* pInterrupt;
    PLIST_ENTRY ple;

    if (m_WakeInterruptCount == 0) {
        return;
    }

    //
    // Initialize the pending count to the wake interrupt count
    // If the event needs an acknowledgement, this variable will
    // be decremented as the interrupt machines ack.
    //
    m_WakeInterruptPendingAckCount = m_WakeInterruptCount;

    for (ple = m_InterruptListHead.Flink;
         ple != &m_InterruptListHead;
         ple = ple->Flink) {

        pInterrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

        if (pInterrupt->IsWakeCapable()) {
            pInterrupt->ProcessWakeInterruptEvent(WakeInterruptEvent);
        }

    }
}

VOID
FxPkgPnp::AckPendingWakeInterruptOperation(
    __in BOOLEAN ProcessPowerEventOnDifferentThread
    )
/*++

Routine Description:
    This routine is invoked by the interrupt wake machine to acknowledge
    back an event that was queued into it. When the last interrupt machine
    acknowledges, an event is queued back into the Pnp/power state machine

Arguments:

    ProcessPowerEventOnDifferentThread - Once all wake interrupts for the device
        have acknowledged the operation, if this is TRUE, the power state
        machine will process the PowerWakeInterruptCompleteTransition event on a
        seperate thread.

Return Value:
    None

--*/
{
    if (InterlockedDecrement((LONG *)&m_WakeInterruptPendingAckCount) == 0) {
        PowerProcessEvent(
            PowerWakeInterruptCompleteTransition,
            ProcessPowerEventOnDifferentThread
            );
    }
}


NTSTATUS
FxPkgPnp::AssignPowerFrameworkSettings(
    __in PWDF_POWER_FRAMEWORK_SETTINGS PowerFrameworkSettings
    )
{
//     NTSTATUS status;
//     PPO_FX_COMPONENT_IDLE_STATE idleStates = NULL;
//     ULONG idleStatesSize = 0;
//     PPO_FX_COMPONENT component = NULL;
//     ULONG componentSize = 0;
//     PPOX_SETTINGS poxSettings = NULL;
//     ULONG poxSettingsSize = 0;
//     BYTE * buffer = NULL;

//     if (FALSE==(IdleTimeoutManagement::_SystemManagedIdleTimeoutAvailable())) {
//         //
//         // If system-managed idle timeout is not available on this OS, then
//         // there is nothing to do.
//         //
//         DoTraceLevelMessage(
//             GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
//             "WDFDEVICE %p !devobj %p Power framework is not supported on the "
//             "current OS. Therefore, the power framework settings will not take "
//             "effect.",
//             m_Device->GetHandle(),
//             m_Device->GetDeviceObject()
//             );
//         return STATUS_SUCCESS;
//     }

//     if (NULL != PowerFrameworkSettings->Component) {
//         //
//         // Caller should ensure that IdleStateCount is not zero
//         //
//         ASSERT(0 != PowerFrameworkSettings->Component->IdleStateCount);

//         //
//         // Compute buffer size needed for storing F-states
//         //
//         status = RtlULongMult(
//                     PowerFrameworkSettings->Component->IdleStateCount,
//                     sizeof(*(PowerFrameworkSettings->Component->IdleStates)),
//                     &idleStatesSize
//                     );
//         if (FALSE == NT_SUCCESS(status)) {
//             DoTraceLevelMessage(
//                 GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
//                 "WDFDEVICE %p !devobj %p Unable to compute length of buffer "
//                 "required to store F-states. RtlULongMult failed with "
//                 "%!STATUS!",
//                 m_Device->GetHandle(),
//                 m_Device->GetDeviceObject(),
//                 status
//                 );
//             goto exit;
//         }

//         //
//         // Compute buffer size needed for storing component information
//         // (including F-states)
//         //
//         status = RtlULongAdd(idleStatesSize,
//                              sizeof(*component),
//                              &componentSize);
//         if (FALSE == NT_SUCCESS(status)) {
//             DoTraceLevelMessage(
//                 GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
//                 "WDFDEVICE %p !devobj %p Unable to compute length of buffer "
//                 "required to store driver's component information. RtlULongAdd "
//                 "failed with %!STATUS!",
//                 m_Device->GetHandle(),
//                 m_Device->GetDeviceObject(),
//                 status
//                 );
//             goto exit;
//         }
//     }

//     //
//     // Compute total buffer size needed for power framework settings
//     //
//     status = RtlULongAdd(componentSize,
//                          sizeof(*poxSettings),
//                          &poxSettingsSize);
//     if (FALSE == NT_SUCCESS(status)) {
//         DoTraceLevelMessage(
//             GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
//             "WDFDEVICE %p !devobj %p Unable to compute length of buffer "
//             "required to store driver's power framework settings. RtlULongAdd "
//             "failed with %!STATUS!",
//             m_Device->GetHandle(),
//             m_Device->GetDeviceObject(),
//             status
//             );
//         goto exit;
//     }

//     //
//     // Allocate memory to copy the settings
//     //
//     buffer = (BYTE *) MxMemory::MxAllocatePoolWithTag(NonPagedPool,
//                                                       poxSettingsSize,
//                                                       GetDriverGlobals()->Tag);
//     if (NULL == buffer) {
//         status = STATUS_INSUFFICIENT_RESOURCES;
//         DoTraceLevelMessage(
//             GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
//             "WDFDEVICE %p !devobj %p Unable to allocate buffer required to "
//             "store F-states. %!STATUS!",
//             m_Device->GetHandle(),
//             m_Device->GetDeviceObject(),
//             status
//             );
//         goto exit;
//     }

//     //
//     // Set our pointers to point to appropriate locations in the buffer.
//     //
//     // NOTES:
//     //   - The array of F-states comes first because it has ULONGLONG members
//     //     because of which it has the biggest alignment requirement.
//     //   - The logic below works even if the client driver did not specify any
//     //     component information. In that case idleStatesSize and componentSize
//     //     are both 0 and 'poxSettings' points to the beginning of the allocated
//     //     buffer
//     //
//     idleStates = (PPO_FX_COMPONENT_IDLE_STATE) buffer;
//     component = (PPO_FX_COMPONENT) (buffer + idleStatesSize);
//     poxSettings = (PPOX_SETTINGS) (buffer + componentSize);

//     //
//     // Copy the relevant parts of the settings buffer
//     //
//     poxSettings->EvtDeviceWdmPostPoFxRegisterDevice =
//         PowerFrameworkSettings->EvtDeviceWdmPostPoFxRegisterDevice;
//     poxSettings->EvtDeviceWdmPrePoFxUnregisterDevice =
//         PowerFrameworkSettings->EvtDeviceWdmPrePoFxUnregisterDevice;
//     poxSettings->Component = PowerFrameworkSettings->Component;
//     poxSettings->ComponentActiveConditionCallback =
//         PowerFrameworkSettings->ComponentActiveConditionCallback;
//     poxSettings->ComponentIdleConditionCallback =
//         PowerFrameworkSettings->ComponentIdleConditionCallback;
//     poxSettings->ComponentIdleStateCallback =
//         PowerFrameworkSettings->ComponentIdleStateCallback;
//     poxSettings->PowerControlCallback =
//         PowerFrameworkSettings->PowerControlCallback;
//     poxSettings->PoFxDeviceContext = PowerFrameworkSettings->PoFxDeviceContext;

//     if (NULL != PowerFrameworkSettings->Component) {
//         //
//         // Copy the component information
//         //
//         poxSettings->Component = component;
//         RtlCopyMemory(poxSettings->Component,
//                       PowerFrameworkSettings->Component,
//                       sizeof(*component));

//         //
//         // Caller should ensure that IdleStates is not NULL
//         //
//         ASSERT(NULL != PowerFrameworkSettings->Component->IdleStates);

//         //
//         // Copy the F-states
//         //
//         poxSettings->Component->IdleStates = idleStates;
//         RtlCopyMemory(poxSettings->Component->IdleStates,
//                       PowerFrameworkSettings->Component->IdleStates,
//                       idleStatesSize);
//     }

//     //
//     // Commit these settings
//     //
//     status = m_PowerPolicyMachine.m_Owner->
//                 m_IdleSettings.m_TimeoutMgmt.CommitPowerFrameworkSettings(
//                                                             GetDriverGlobals(),
//                                                             poxSettings
//                                                             );
//     if (FALSE == NT_SUCCESS(status)) {
//         goto exit;
//     }

//     status = STATUS_SUCCESS;

// exit:
//     if (FALSE == NT_SUCCESS(status)) {
//         if (NULL != buffer) {
//             MxMemory::MxFreePool(buffer);
//         }
//     }
//     return status;
    ROSWDFNOTIMPLEMENTED;
    return STATUS_SUCCESS;
}

DEVICE_POWER_STATE
FxPkgPnp::PowerPolicyGetDeviceDeepestDeviceWakeState(
    __in SYSTEM_POWER_STATE SystemState
    )
{
    DEVICE_POWER_STATE dxState;

    //
    // Earlier versions of WDF (pre-1.11) did not take into account SystemState
    // in figuring out the deepest wake state. So for compatibility we restrict
    // this to drivers compiled for 1.11 or newer. See comments in the
    // FxPkgPnp::QueryForCapabilities function for more information on
    // compatibility risk.
    //
    if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1,11)) {
        if ((SystemState < PowerSystemWorking) ||
            (SystemState > PowerSystemHibernate)) {
            dxState = PowerDeviceD0;
        } else {
            dxState = MapWakeDepthToDstate(
                (DEVICE_WAKE_DEPTH)m_DeviceWake[SystemState - PowerSystemWorking]
                );
        }
    }
    else {
        //
        // For pre-1.11 drivers, all of m_DeviceWake array is populated with
        // the same DeviceWake value obtained from device capabilities so we
        // just use the first one (index 0).
        //
        dxState = MapWakeDepthToDstate((DEVICE_WAKE_DEPTH)m_DeviceWake[0]);
    }

    //
    // Per WDM docs DeviceWake and SystemWake must be non-zero to support
    // wake. We warn about the violation. Ideally, a verifier check would have
    // been better but we want to avoid that because some drivers may
    // intentionally call this DDI and do not treat the DDI failure as fatal (
    // because they are aware that DDI may succeed in some cases), and a verifier
    // breakpoint would force them to avoid the verifier failure by not enabling
    // verifier.
    //
    if (dxState == PowerDeviceUnspecified ||
        m_SystemWake == PowerSystemUnspecified) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
            "SystemWake %!SYSTEM_POWER_STATE! and DeviceWake "
            "%!DEVICE_POWER_STATE! power state reported in device "
            "capabilities do not support wake. Both the SystemWake and "
            "DeviceWake members should be nonzero to support wake-up on "
            "this system.", m_SystemWake, dxState);
    }

    return dxState;
}

