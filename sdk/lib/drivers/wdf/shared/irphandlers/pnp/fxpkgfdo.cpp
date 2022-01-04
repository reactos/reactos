/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgFdo.cpp

Abstract:

    This module implements the pnp/power package for the driver
    framework.

Author:



Environment:

    Both kernel and user mode

Revision History:




--*/

#include "pnppriv.hpp"


//#include <FxIoTarget.hpp>

#include <initguid.h>
#include <wdmguid.h>

#if defined(EVENT_TRACING)
// Tracing support
extern "C" {
#include "fxpkgfdo.tmh"
}
#endif

struct FxFilteredStartContext : public FxStump {
    FxFilteredStartContext(
        VOID
        )
    {
        ResourcesRaw = NULL;
        ResourcesTranslated = NULL;
    }

    ~FxFilteredStartContext()
    {
        if (ResourcesRaw != NULL) {
            MxMemory::MxFreePool(ResourcesRaw);
        }
        if (ResourcesTranslated != NULL) {
            MxMemory::MxFreePool(ResourcesTranslated);
        }
    }

    FxPkgFdo* PkgFdo;

    PCM_RESOURCE_LIST ResourcesRaw;

    PCM_RESOURCE_LIST ResourcesTranslated;
};

const PFN_PNP_POWER_CALLBACK FxPkgFdo::m_FdoPnpFunctionTable[IRP_MN_SURPRISE_REMOVAL + 1] =
{
    _PnpStartDevice,                // IRP_MN_START_DEVICE
    _PnpQueryRemoveDevice,          // IRP_MN_QUERY_REMOVE_DEVICE
    _PnpRemoveDevice,               // IRP_MN_REMOVE_DEVICE
    _PnpCancelRemoveDevice,         // IRP_MN_CANCEL_REMOVE_DEVICE
    _PnpStopDevice,                 // IRP_MN_STOP_DEVICE
    _PnpQueryStopDevice,            // IRP_MN_QUERY_STOP_DEVICE
    _PnpCancelStopDevice,           // IRP_MN_CANCEL_STOP_DEVICE

    _PnpQueryDeviceRelations,       // IRP_MN_QUERY_DEVICE_RELATIONS
    _PnpQueryInterface,             // IRP_MN_QUERY_INTERFACE
    _PnpQueryCapabilities,          // IRP_MN_QUERY_CAPABILITIES
    _PnpPassDown,                   // IRP_MN_QUERY_RESOURCES
    _PnpPassDown,                   // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    _PnpPassDown,                   // IRP_MN_QUERY_DEVICE_TEXT
    _PnpFilterResourceRequirements, // IRP_MN_FILTER_RESOURCE_REQUIREMENTS

    _PnpPassDown,                   // 0x0E

    _PnpPassDown,                   // IRP_MN_READ_CONFIG
    _PnpPassDown,                   // IRP_MN_WRITE_CONFIG
    _PnpPassDown,                   // IRP_MN_EJECT
    _PnpPassDown,                   // IRP_MN_SET_LOCK
    _PnpPassDown,                   // IRP_MN_QUERY_ID
    _PnpQueryPnpDeviceState,        // IRP_MN_QUERY_PNP_DEVICE_STATE
    _PnpPassDown,                   // IRP_MN_QUERY_BUS_INFORMATION
    _PnpDeviceUsageNotification,    // IRP_MN_DEVICE_USAGE_NOTIFICATION
    _PnpSurpriseRemoval,            // IRP_MN_SURPRISE_REMOVAL

};

const PFN_PNP_POWER_CALLBACK FxPkgFdo::m_FdoPowerFunctionTable[IRP_MN_QUERY_POWER + 1] =
{
    _DispatchWaitWake,      // IRP_MN_WAIT_WAKE
    _PowerPassDown,         // IRP_MN_POWER_SEQUENCE
    _DispatchSetPower,      // IRP_MN_SET_POWER
    _DispatchQueryPower,    // IRP_MN_QUERY_POWER
};

//#if defined(ALLOC_PRAGMA)
//#pragma code_seg("PAGE")
//#endif

FxPkgFdo::FxPkgFdo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice *Device
    ) :
    FxPkgPnp(FxDriverGlobals, Device, FX_TYPE_PACKAGE_FDO)
/*++

Routine Description:

    This is the constructor for the FxPkgFdo.  Don't do any initialization
    that might fail here.

Arguments:

    none

Returns:

    none

--*/

{
    m_DefaultDeviceList = NULL;
    m_StaticDeviceList = NULL;

    m_DefaultTarget = NULL;
    m_SelfTarget = NULL;

    m_BusEnumRetries = 0;

    //
    // Since we will always have a valid PDO when we are the FDO, we can do
    // any device interface related activity at any time
    //
    m_DeviceInterfacesCanBeEnabled = TRUE;

    m_Filter = FALSE;

    RtlZeroMemory(&m_BusInformation, sizeof(m_BusInformation));

    RtlZeroMemory(&m_SurpriseRemoveAndReenumerateSelfInterface,
        sizeof(m_SurpriseRemoveAndReenumerateSelfInterface));
}

FxPkgFdo::~FxPkgFdo(
    VOID
    )
/*++

Routine Description:

    This is the destructor for the FxPkgFdo

Arguments:

    none

Returns:

    none

--*/

{
    if (m_DefaultDeviceList != NULL) {
        m_DefaultDeviceList->RELEASE(this);
    }
    if (m_StaticDeviceList != NULL) {
        m_StaticDeviceList->RELEASE(this);
    }
    if (m_SelfTarget != NULL) {
        m_SelfTarget->RELEASE(this);
    }
    if (m_DefaultTarget != NULL) {
        m_DefaultTarget->RELEASE(this);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_Create(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in CfxDevice * Device,
    __deref_out FxPkgFdo ** PkgFdo
    )
{
    NTSTATUS status;
    FxPkgFdo * fxPkgFdo;
    FxEventQueue *eventQueue;

    fxPkgFdo = new(DriverGlobals) FxPkgFdo(DriverGlobals, Device);

    if (NULL == fxPkgFdo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Memory allocation failed: %!STATUS!", status);
        return status;
    }

    //
    // Initialize the event queues in the PnP, power and power policy state
    // machines
    //
    eventQueue = static_cast<FxEventQueue*> (&(fxPkgFdo->m_PnpMachine));
    status = eventQueue->Initialize(DriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    eventQueue = static_cast<FxEventQueue*> (&(fxPkgFdo->m_PowerMachine));
    status = eventQueue->Initialize(DriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    eventQueue = static_cast<FxEventQueue*> (&(fxPkgFdo->m_PowerPolicyMachine));
    status = eventQueue->Initialize(DriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    *PkgFdo = fxPkgFdo;

exit:
    if (!NT_SUCCESS(status)) {
        fxPkgFdo->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::SendIrpSynchronously(
    __inout FxIrp* Irp
    )

/*++

Routine Description:

    Helper function that makes it easy to handle events which need to be
    done as an IRP comes back up the stack.

Arguments:

    Irp - The request

Returns:

    results from IoCallDriver

--*/

{
    Irp->CopyCurrentIrpStackLocationToNext();
    return Irp->SendIrpSynchronously(m_Device->GetAttachedDevice());
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::FireAndForgetIrp(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    Virtual override which sends the request down the stack and forgets about it.

Arguments:

    Irp - The request

Returns:

    results from IoCallDriver

--*/

{
    if (Irp->GetMajorFunction() == IRP_MJ_POWER) {
        return _PowerPassDown(this, Irp);
    }
    else {
        return _PnpPassDown(this, Irp);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpPassDown(
    __in FxPkgPnp* This,
    __inout FxIrp* Irp
    )
/*++

Routine Description:
    Functionally equivalent to FireAndForgetIrp except this isn't a virtual
    call

Arguments:
    Irp - The request

Return Value:
    result from IoCallDriver

  --*/
{
    NTSTATUS status;
    FxDevice* device;
    MdIrp pIrp;

    device = ((FxPkgFdo*)This)->m_Device;
    pIrp = Irp->GetIrp();

    Irp->CopyCurrentIrpStackLocationToNext();
    status = Irp->CallDriver(
        ((FxPkgFdo*)This)->m_Device->GetAttachedDevice());

    Mx::MxReleaseRemoveLock(device->GetRemoveLock(),
                            pIrp
                            );

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpSurpriseRemoval(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    This method is called in response to a PnP SurpriseRemoval IRP.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/
{
    return ((FxPkgFdo*) This)->PnpSurpriseRemoval(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryDeviceRelations(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    This method is called in response to a PnP QDR.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/
{
    return ((FxPkgFdo*) This)->PnpQueryDeviceRelations(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::PnpQueryDeviceRelations(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This routine is called in response to a QueryDeviceRelations IRP.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NSTATUS

--*/

{
    NTSTATUS status;
    DEVICE_RELATION_TYPE type;

    type = Irp->GetParameterQDRType();

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering QueryDeviceRelations handler, "
                        "type %!DEVICE_RELATION_TYPE!",
                        type);

    status = STATUS_SUCCESS;


    //
    // Set up the type of relations.
    //
    switch (type) {
    case BusRelations:
        status = HandleQueryBusRelations(Irp);

        //
        // STATUS_NOT_SUPPORTED is a special value. It means that
        // HandleQueryBusRelations did not modify the irp at all and it should
        // be sent off as is.
        //
        if (status == STATUS_NOT_SUPPORTED) {
            //
            // We set status to STATUS_SUCCESS so that we send the requqest down
            // the stack in the comparison below.
            //
            status = STATUS_SUCCESS;
        }
        break;

    case RemovalRelations:
        status = HandleQueryDeviceRelations(Irp, m_RemovalDeviceList);

        //
        // STATUS_NOT_SUPPORTED is a special value. It means that
        // HandleQueryDeviceRelations did not modify the irp at all and it should
        // be sent off as is.
        //
        if (status == STATUS_NOT_SUPPORTED) {
            //
            // We set status to STATUS_SUCCESS so that we send the requqest down
            // the stack in the comparison below.
            //
            status = STATUS_SUCCESS;
        }
        break;
    }

    if (NT_SUCCESS(status)) {
        status = _PnpPassDown(this, Irp);
    }
    else {
        CompletePnpRequest(Irp, status);
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting QueryDeviceRelations handler, status %!STATUS!",
                        status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryInterface(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    This method is invoked in response to a Pnp QueryInterface IRP.

Arguments:

    This - The package

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/
{
    FxPkgFdo* pThis;
    NTSTATUS status;
    BOOLEAN completeIrp;

    pThis = (FxPkgFdo*) This;

    DoTraceLevelMessage(pThis->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering QueryInterface handler");

    status = pThis->HandleQueryInterface(Irp, &completeIrp);

    DoTraceLevelMessage(pThis->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting QueryInterface handler, %!STATUS!",
                        status);
    //
    // If we understand the irp, we'll complete it.  Otherwise we
    // pass it down.
    //
    if (completeIrp == FALSE) {
        status = _PnpPassDown(pThis, Irp);
    }
    else {
        Irp->SetInformation(NULL);
        pThis->CompletePnpRequest(Irp, status);
    }

    //
    // Remlock is released in _PnpPassDown and  CompletePnpRequest. If this
    // irp is racing with remove on another thread, it's possible for the device
    // to get deleted right after the lock is released and before anything
    // after this point is executed. So make sure to not touch any memory in
    // the return path.
    //

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryCapabilities(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{












    return ((FxPkgFdo*) This)->PnpQueryCapabilities(Irp);
}

VOID
FxPkgFdo::HandleQueryCapabilities(
    __inout FxIrp *Irp
    )
{
    PDEVICE_CAPABILITIES pCaps;
    LONG pnpCaps;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering QueryCapabilities handler");

    pCaps = Irp->GetParameterDeviceCapabilities();

    pnpCaps = GetPnpCapsInternal();

    //
    // Add Capabilities.  These need to be done as the IRP goes down, since
    // lower drivers need to see them.
    //
    if ((pCaps->Size >= sizeof(DEVICE_CAPABILITIES)) && (pCaps->Version == 1)) {
        SET_PNP_CAP_IF_TRUE(pnpCaps, pCaps, LockSupported);
        SET_PNP_CAP_IF_TRUE(pnpCaps, pCaps, EjectSupported);
        SET_PNP_CAP_IF_TRUE(pnpCaps, pCaps, Removable);
        SET_PNP_CAP_IF_TRUE(pnpCaps, pCaps, DockDevice);
        SET_PNP_CAP_IF_TRUE(pnpCaps, pCaps, SurpriseRemovalOK);
        SET_PNP_CAP_IF_TRUE(pnpCaps, pCaps, NoDisplayInUI);
        //
        // If the driver has declared a wake capable interrupt,
        // we need to set this bit in the capabilities so that
        // ACPI will relinquish control of wake responsiblity
        //
        if (SupportsWakeInterrupt()) {
            pCaps->WakeFromInterrupt = TRUE;
        }
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting QueryCapabilities handler");
    return;
}

VOID
FxPkgFdo::HandleQueryCapabilitiesCompletion(
    __inout FxIrp *Irp
    )
{
    PDEVICE_CAPABILITIES pCaps;
    LONG pnpCaps;
    ULONG i;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering QueryCapabilities completion handler");

    pCaps = Irp->GetParameterDeviceCapabilities();

    pnpCaps = GetPnpCapsInternal();

    //
    // Confirm this is a valid DeviceCapabilities structure.
    //
    ASSERT(pCaps->Size >= sizeof(DEVICE_CAPABILITIES));
    ASSERT(pCaps->Version >= 1);

    if ((pCaps->Size >= sizeof(DEVICE_CAPABILITIES)) &&
        (pCaps->Version == 1)) {
        ULONG states;

        //
        // Remove Capabilities
        //

        //
        // Re-add SOME capibilties that the bus driver may have accidentally
        // stomped.





        SET_PNP_CAP_IF_FALSE(pnpCaps, pCaps, LockSupported);
        SET_PNP_CAP_IF_FALSE(pnpCaps, pCaps, EjectSupported);
        SET_PNP_CAP_IF_FALSE(pnpCaps, pCaps, DockDevice);

        SET_PNP_CAP(pnpCaps, pCaps, Removable);
        SET_PNP_CAP(pnpCaps, pCaps, SurpriseRemovalOK);

        //
        // The DeviceState array contains a table of D states that correspond
        // to a given S state.  If the driver writer initialized entries of the
        // array, and if the new value is a deeper sleep state than the
        // original, we will use the new driver supplied value.
        //
        states = m_PowerCaps.States;

        for (i = PowerSystemWorking; i < PowerSystemMaximum; i++) {
            DEVICE_POWER_STATE state;

            //
            // PowerDeviceMaximum indicates to use the default value
            //
            // We are only allowed to deepen the D states, not lighten them,
            // hence the > compare.
            //
            state = _GetPowerCapState(i, states);

            if (state != PowerDeviceMaximum && state > pCaps->DeviceState[i]) {
                pCaps->DeviceState[i] = state;
            }
        }

        //
        // If the driver supplied SystemWake value is lighter than the current
        // value, then use the driver supplied value.
        //
        // PowerSystemMaximum indicates to use the default value
        //
        // We are only allowed to lighten the S state, not deepen it, hence
        // the < compare.
        //
        if (m_PowerCaps.SystemWake != PowerSystemMaximum &&
            m_PowerCaps.SystemWake < pCaps->SystemWake) {
            pCaps->SystemWake = (SYSTEM_POWER_STATE) m_PowerCaps.SystemWake;
        }

        //
        // Same for DeviceWake
        //
        if (m_PowerCaps.DeviceWake != PowerDeviceMaximum &&
            m_PowerCaps.DeviceWake < pCaps->DeviceWake) {
            pCaps->DeviceWake = (DEVICE_POWER_STATE) m_PowerCaps.DeviceWake;
        }

        //
        // Set the Device wake up latencies.  A value of -1 indicates to
        // use the default values provided by the lower stack.
        //
        if (m_PowerCaps.D1Latency != (ULONG) -1 &&
            m_PowerCaps.D1Latency > pCaps->D1Latency) {
            pCaps->D1Latency = m_PowerCaps.D1Latency;
        }

        if (m_PowerCaps.D2Latency != (ULONG) -1 &&
            m_PowerCaps.D2Latency > pCaps->D2Latency) {
            pCaps->D2Latency = m_PowerCaps.D2Latency;
        }

        if (m_PowerCaps.D3Latency != (ULONG) -1 &&
            m_PowerCaps.D3Latency > pCaps->D3Latency) {
            pCaps->D3Latency = m_PowerCaps.D3Latency;
        }

        //
        // Set the Address and the UI number values.  A value of -1 indicates
        // to use the default values provided by the lower stack.
        //
        if (m_PnpCapsAddress != (ULONG) -1) {
            pCaps->Address = m_PnpCapsAddress;
        }

        if (m_PnpCapsUINumber != (ULONG) -1) {
            pCaps->UINumber= m_PnpCapsUINumber;
        }
    }

    //
    // CompletePnpRequest is called after return from this function
    // so it is safe to log here
    //

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting QueryCapabilities completion handler");

    return;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpFilterResourceRequirements(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgFdo*) This)->PnpFilterResourceRequirements(Irp);
}







VOID
FxPkgFdo::HandleQueryPnpDeviceStateCompletion(
    __inout FxIrp *Irp
    )
{
    PNP_DEVICE_STATE pnpDeviceState;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Entering QueryPnpDeviceState completion handler");

    pnpDeviceState = HandleQueryPnpDeviceState(
        (PNP_DEVICE_STATE) Irp->GetInformation()
        );

    Irp->SetInformation((ULONG_PTR) pnpDeviceState);

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p returning PNP_DEVICE_STATE 0x%d IRP 0x%p",
        m_Device->GetHandle(),
        m_Device->GetDeviceObject(),
        pnpDeviceState,
        Irp->GetIrp());

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Exiting QueryPnpDeviceState completion handler");

    return;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::RegisterCallbacks(
    __in PWDF_FDO_EVENT_CALLBACKS DispatchTable
    )
{
    //
    // Walk the table, and only update the pkg table *if* the dispatch
    // table has a non-null entry.
    //
    m_DeviceFilterAddResourceRequirements.m_Method =
        DispatchTable->EvtDeviceFilterAddResourceRequirements;
    m_DeviceFilterRemoveResourceRequirements.m_Method =
        DispatchTable->EvtDeviceFilterRemoveResourceRequirements;
    m_DeviceRemoveAddedResources.m_Method =
        DispatchTable->EvtDeviceRemoveAddedResources;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::CreateDefaultDeviceList(
    __in PWDF_CHILD_LIST_CONFIG ListConfig,
    __in PWDF_OBJECT_ATTRIBUTES ListAttributes
    )

/*++

Routine Description:



Arguments:


Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFCHILDLIST hList;
    size_t totalDescriptionSize = 0;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    ASSERT(m_EnumInfo != NULL);

    //
    // This should not fail, we already validated the total size when we
    // validated the config (we just had no place to store the total size, so
    // we recompute it again).
    //
    status = FxChildList::_ComputeTotalDescriptionSize(
        pFxDriverGlobals,
        ListConfig,
        &totalDescriptionSize
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxChildList::_CreateAndInit(&m_DefaultDeviceList,
                                         pFxDriverGlobals,
                                         ListAttributes,
                                         totalDescriptionSize,
                                         m_Device,
                                         ListConfig);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_DefaultDeviceList->Commit(ListAttributes,
                                         (WDFOBJECT*)&hList,
                                         m_Device);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not convert object to handle");
        m_DefaultDeviceList->DeleteFromFailedCreate();
        m_DefaultDeviceList = NULL;
        return status;
    }

    //
    // This will be released in the destructor
    //
    m_DefaultDeviceList->ADDREF(this);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::SetFilter(
    __in BOOLEAN Value
    )

/*++

Routine Description:

    The Framework behaves differently depending on whether this device is a
    filter in the stack.  This method is the way we keep track.

Arguments:

    Value - Filter or not

Returns:

    NTSTATUS

--*/

{
    m_Filter = Value;
    return STATUS_SUCCESS;
}

BOOLEAN
FxPkgFdo::PnpSendStartDeviceDownTheStackOverload(
    VOID
    )
/*++

Routine Description:
    Send the start irp down the stack.

Arguments:
    None

Return Value:
    FALSE, the transition will occur in the irp's completion routine.

  --*/
{
    //
    // We will re-set the pending pnp irp when the start irp returns
    //
    FxIrp irp(ClearPendingPnpIrp());
    PCM_RESOURCE_LIST pWdmRaw, pWdmTranslated;
    NTSTATUS status;
    BOOLEAN setFilteredCompletion = FALSE;
    FxFilteredStartContext* pContext = NULL;

    pWdmRaw = irp.GetParameterAllocatedResources();
    pWdmTranslated = irp.GetParameterAllocatedResourcesTranslated();

    //
    // Always setup the irp to be sent down the stack.  In case of an error,
    // this does no harm and it keeps everything simple.
    //
    irp.CopyCurrentIrpStackLocationToNext();

    //
    // If the driver registered for a callback to remove its added resources
    // and there are resources to remove, call the driver and set the next
    // stack location to the filtered resources lists
    //
    if (m_DeviceRemoveAddedResources.m_Method != NULL &&
        pWdmRaw != NULL && pWdmTranslated != NULL) {

        //
        // Since we reuse these 2 fields for both the removal and the normal
        // reporting (and can then be subsequently reused on a restart), we
        // set the changed status back to FALSE.
        //
        m_ResourcesRaw->m_Changed = FALSE;
        m_Resources->m_Changed = FALSE;

        status = m_ResourcesRaw->BuildFromWdmList(pWdmRaw,
                                                  FxResourceAllAccessAllowed);

        if (NT_SUCCESS(status)) {
            status = m_Resources->BuildFromWdmList(pWdmTranslated,
                                                   FxResourceAllAccessAllowed);
        }

        if (NT_SUCCESS(status)) {
            status = m_DeviceRemoveAddedResources.Invoke(
                m_Device->GetHandle(),
                m_ResourcesRaw->GetHandle(),
                m_Resources->GetHandle()
                );
        }

        if (NT_SUCCESS(status) &&
            (m_ResourcesRaw->IsChanged() || m_Resources->IsChanged())) {

            pContext = new(GetDriverGlobals()) FxFilteredStartContext();

            if (pContext != NULL) {
                pContext->PkgFdo = this;

                //
                // Allocate the raw and translated resources.  Upon failure for
                // either, fail the start irp.  We allocate from NonPagedPool
                // because we are going to free the list in a completion routine
                // which maybe running at an IRQL > PASSIVE_LEVEL.
                //
                if (m_ResourcesRaw->Count() > 0) {
                    pContext->ResourcesRaw =
                        m_ResourcesRaw->CreateWdmList(NonPagedPool);

                    if (pContext->ResourcesRaw == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (NT_SUCCESS(status) && m_Resources->Count() > 0) {
                    pContext->ResourcesTranslated =
                        m_Resources->CreateWdmList(NonPagedPool);

                    if (pContext->ResourcesTranslated == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (NT_SUCCESS(status)) {
                    //
                    // We copied to the next stack location at the start
                    //
                    irp.SetParameterAllocatedResources(
                        pContext->ResourcesRaw);
                    irp.SetParameterAllocatedResourcesTranslated(
                        pContext->ResourcesTranslated);

                    //
                    // Completion routine will free the resource list
                    // allocations.
                    //
                    setFilteredCompletion = TRUE;
                }
                else {
                    //
                    // Allocation failure.  The destructor will free any
                    // outstanding allocations.
                    //
                    delete pContext;
                }
            }
        }
    }
    else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        //
        // The completion of the start irp will move the state machine into a new
        // state.
        //
        // After calling IoSetCompletionRoutineEx the driver must call
        // IoCallDriver, otherwise a memory leak would occur. So call this API
        // as close to IoCallDriver as possible to avoid failure paths.
        //
        if (setFilteredCompletion) {
            ASSERT(pContext != NULL);
            irp.SetCompletionRoutineEx(
                m_Device->GetDeviceObject(),
                _PnpFilteredStartDeviceCompletionRoutine,
                pContext);
        }
        else {
            irp.SetCompletionRoutineEx(
                m_Device->GetDeviceObject(),
                _PnpStartDeviceCompletionRoutine,
                this);
        }

        irp.CallDriver(m_Device->GetAttachedDevice());
    }
    else {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "PNP start preprocessing failed with %!STATUS!",
                            status);

        //
        // Move the state machine to the failed state.
        //
        // Process the event *before* completing the irp so that this even is in
        // the queue before the device remove event which will be be processed
        // right after the start irp has been completed.
        //
        PnpProcessEvent(PnpEventStartDeviceFailed);

        //
        // All states which handle the PnpEventStartDeviceFailed event
        // transition do not expect a pended start irp.
        //
        CompletePnpRequest(&irp, status);
    }

    //
    // Indicate to the caller that the transition is asynchronous.
    //
    return FALSE;
}

WDF_DEVICE_PNP_STATE
FxPkgFdo::PnpEventEjectHardwareOverload(
    VOID
    )

/*++

Routine Description:

    This method implements the Eject Hardware state in the PnP State Machine.
    Since FDO and PDO behavior is different, this is overloaded.  In fact,
    this state shouldn't be reachable for FDOs, as the FDO should have been
    torn off the stack before the device was cleanly ejected.

Arguments:

    none

Returns:

    WdfDevStatePnpEjectedWaitingForRemove

--*/

{
    ASSERT(!"This should only be reachable for PDOs.");

    //
    // Do something safe.  Act like the device got
    // ejected.
    //
    return WdfDevStatePnpEjectedWaitingForRemove;
}

WDF_DEVICE_PNP_STATE
FxPkgFdo::PnpEventCheckForDevicePresenceOverload(
    VOID
    )

/*++

Routine Description:

    This method implements the Check For Device Presence state in the PnP State
    Machine.  Since FDO and PDO behavior is different, this is overloaded.  In
    fact, this state shouldn't be reachable for FDOs, as the FDO should have
    been torn off the stack before the device was cleanly ejected.

Arguments:

    none

Returns:

    WdfDevStatePnpFinal

--*/

{
    ASSERT(!"This should only be implemented for PDOs.");

    //
    // Do something safe.  Act like the device is not
    // present.
    //
    return WdfDevStatePnpFinal;
}

WDF_DEVICE_PNP_STATE
FxPkgFdo::PnpEventPdoRemovedOverload(
    VOID
    )

/*++

Routine Description:

    This method implements the PDO Removed state in the PnP State Machine.
    Since FDO and PDO behavior is different, this is overloaded.  In fact,
    this state shouldn't be reachable for FDOs, as the FDO should have been
    torn off the stack before the PDO is removed.

Arguments:

    none

Returns:

    none

--*/

{
    ASSERT(!"This should only be implemented for PDOs.");

    return WdfDevStatePnpFinal;
}

WDF_DEVICE_PNP_STATE
FxPkgFdo::PnpGetPostRemoveState(
    VOID
    )

/*++

Routine Description:

    This method implements the Removed state in the PnP State Machine.
    Since FDO and PDO behavior is different, this is overloaded.  This state
    just moves us toward the FDO-specific removal states.

Arguments:

    none

Returns:

    none

--*/

{
    return WdfDevStatePnpFdoRemoved;
}

WDF_DEVICE_PNP_STATE
FxPkgFdo::PnpEventFdoRemovedOverload(
    VOID
    )
/*++

Routine Description:
    FDO is being removed, see if there are any children that need to be removed

Arguments:
    None

Return Value:
    New device pnp state

  --*/
{
    //
    // Do that which all device stacks need to do upon removal.
    //
    PnpEventRemovedCommonCode();

    return WdfDevStatePnpFinal;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::ProcessRemoveDeviceOverload(
    __inout FxIrp* Irp
    )
{
    NTSTATUS status;

    //
    // After this is called, any irp dispatched to FxDevice::DispatchWithLock
    // will fail with STATUS_INVALID_DEVICE_REQUEST.
    //
    Mx::MxReleaseRemoveLockAndWait(m_Device->GetRemoveLock(),
                                   Irp->GetIrp());

    //
    // Cleanup the state machines and release the power thread.
    //
    CleanupStateMachines(TRUE);

    Irp->CopyCurrentIrpStackLocationToNext();
    status = Irp->CallDriver(m_Device->GetAttachedDevice());

    //
    // Detach and delete the device object.
    //
    DeleteDevice();

    return status;
}

VOID
FxPkgFdo::DeleteSymbolicLinkOverload(
    __in BOOLEAN GracefulRemove
    )
/*++

Routine Description:
    Role specific virtual function which determines if the symbolic link for a
    device should be deleted.

Arguments:
    None

Return Value:
    None

  --*/
{
    UNREFERENCED_PARAMETER(GracefulRemove);

    //
    // We always remove the symbolic link for an FDO since there is no presence
    // state to check.
    //
    m_Device->DeleteSymbolicLink();
}

VOID
FxPkgFdo::QueryForReenumerationInterface(
    VOID
    )
{
    PREENUMERATE_SELF_INTERFACE_STANDARD pInterface;
    NTSTATUS status;

    pInterface = &m_SurpriseRemoveAndReenumerateSelfInterface;

    if (pInterface->SurpriseRemoveAndReenumerateSelf != NULL) {
        //
        // Already got it, just return.  This function can be called again during
        // stop -> start, so we must check.
        //
        return;
    }

    RtlZeroMemory(pInterface, sizeof(*pInterface));
    pInterface->Size =  sizeof(*pInterface);
    pInterface->Version = 1;

    //
    // Since there are some stacks that are not PnP re-entrant

    // we specify that the QI goes only to our attached device and
    // not to the top of the stack as a normal QI irp would.  For the
    // reenumerate self interface, having someone on top of us filter the
    // IRP makes no sense anyways.
    //
    // We also do this as a preventative measure for other stacks we don't know
    // about internally and do not have access to when testing.
    //
    status = m_Device->QueryForInterface(&GUID_REENUMERATE_SELF_INTERFACE_STANDARD,
        (PINTERFACE) pInterface,
        sizeof(*pInterface),
        1,
        NULL,
        m_Device->GetAttachedDevice()
        );

    //
    // Failure to get this interface is not fatal.
    // Note that an implicit reference has been taken on the interface. We
    // must release the reference when we are done with the interface.
    //
    UNREFERENCED_PARAMETER(status); // for analyis tools.
}

VOID
FxPkgFdo::ReleaseReenumerationInterface(
    VOID
    )
/*++

Routine Description:

    Releases the implicit reference taken on REENUMERATE_SELF interface.
    NOOP for PDO.

Arguments:
    None

Return Value:
    VOID

  --*/
{
    PREENUMERATE_SELF_INTERFACE_STANDARD pInterface;

    pInterface = &m_SurpriseRemoveAndReenumerateSelfInterface;

    pInterface->SurpriseRemoveAndReenumerateSelf = NULL;

    if (pInterface->InterfaceDereference != NULL) {
        pInterface->InterfaceDereference(pInterface->Context);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::QueryForPowerThread(
    VOID
    )
/*++

Routine Description:
    Sends a query for a power thread down the stack.  If it can't query for one,
    it attempts to create one on its own.

Arguments:
    None

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;

    //
    // Query down the stack to see if a lower device already created a thread.
    // Do NOT send this to the top of the stack, it is sent to the attached
    // device b/c we need the thread from a device below us b/c its lifetime
    // will outlast this device's.
    //
    status = m_Device->QueryForInterface(&GUID_POWER_THREAD_INTERFACE,
        &m_PowerThreadInterface.Interface,
        sizeof(m_PowerThreadInterface),
        1,
        NULL,
        m_Device->GetAttachedDevice());

    if (NT_SUCCESS(status)) {
        //
        // A lower thread exists, use it
        //
        m_HasPowerThread = TRUE;
    }
    else {
        //
        // No thread exists yet, attempt to create our own
        //
        status = CreatePowerThread();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
STDCALL
FxPkgFdo::_PnpFilteredStartDeviceCompletionRoutine(
    __in    MdDeviceObject DeviceObject,
    __inout MdIrp Irp,
    __inout PVOID Context
    )
{
    FxFilteredStartContext *pContext;
    FxPkgFdo* pPkgFdo;

    pContext = (FxFilteredStartContext*) Context;

    //
    // Save off the package so we can use it after we free the context
    //
    pPkgFdo = pContext->PkgFdo;

    delete pContext;

    return _PnpStartDeviceCompletionRoutine(DeviceObject, Irp, pPkgFdo);
}

_Must_inspect_result_
NTSTATUS
STDCALL
FxPkgFdo::_PnpStartDeviceCompletionRoutine(
    __in    MdDeviceObject DeviceObject,
    __inout MdIrp Irp,
    __inout PVOID Context
    )
{
    FxPkgFdo* pThis;
    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pThis = (FxPkgFdo*) Context;

    if (NT_SUCCESS(irp.GetStatus())) {
        pThis->SetPendingPnpIrp(&irp);

        //
        // Only raise irql if we are the power policy owner.  Only the p.p.o.
        // does this so that we only have one driver in the device stack moving
        // to another thread.
        //
        if (pThis->IsPowerPolicyOwner()) {
            KIRQL irql;

            //
            // By raising to dispatch level we are forcing the pnp state machine
            // to move to another thread.  On NT 6.0 PnP supports asynchronous
            // starts, so this will other starts to proceed while WDF processes
            // this device starting.
            //
            Mx::MxRaiseIrql(DISPATCH_LEVEL, &irql);
            pThis->PnpProcessEvent(PnpEventStartDeviceComplete);
            Mx::MxLowerIrql(irql);
        }
        else {
            pThis->PnpProcessEvent(PnpEventStartDeviceComplete);
        }
    }
    else {
        //
        // Just complete the request, the current pnp state can handle the remove
        // which will be sent b/c of the failed start.
        //
        DoTraceLevelMessage(
            pThis->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "PNP start failed with %!STATUS!", irp.GetStatus());

        //
        // Process the event *before* completing the irp so that this even is in
        // the queue before the device remove event which will be be processed
        // right after the start irp has been completed.
        //
        pThis->PnpProcessEvent(PnpEventStartDeviceFailed);

        pThis->CompletePnpRequest(&irp, irp.GetStatus());
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::PostCreateDeviceInitialize(
    VOID
    )
{
    NTSTATUS status;

    status = FxPkgPnp::PostCreateDeviceInitialize(); // __super call
    if (!NT_SUCCESS(status)) {
        return status;
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // Allow device simulation framework to hook interrupt routines.
    //
    if (GetDriverGlobals()->FxDsfOn) {
        status = QueryForDsfInterface();
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

#endif

    status = m_Device->AllocateTarget(&m_DefaultTarget,
                                      FALSE /*SelfTarget=FALSE*/);
    if (NT_SUCCESS(status)) {
        //
        // This will be released in the destructor
        //
        m_DefaultTarget->ADDREF(this);
    }

    if (m_Device->m_SelfIoTargetNeeded) {
        status = m_Device->AllocateTarget((FxIoTarget**)&m_SelfTarget,
                                          TRUE /*SelfTarget*/);
        if (NT_SUCCESS(status)) {
            //
            // This will be released in the destructor
            //
            m_SelfTarget->ADDREF(this);
        }
    }


    return status;
}

