/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgPdo.cpp

Abstract:

    This module implements the Pnp package for Pdo devices.

Author:



Environment:

    Both kernel and user mode

Revision History:




--*/

#include "pnppriv.hpp"
#include <wdmguid.h>

// Tracing support
#if defined(EVENT_TRACING)
extern "C" {
#include "FxPkgPdo.tmh"
}
#endif

const PFN_PNP_POWER_CALLBACK FxPkgPdo::m_PdoPnpFunctionTable[IRP_MN_SURPRISE_REMOVAL + 1] =
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
    _PnpQueryResources,             // IRP_MN_QUERY_RESOURCES
    _PnpQueryResourceRequirements,  // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    _PnpQueryDeviceText,            // IRP_MN_QUERY_DEVICE_TEXT
    _PnpFilterResourceRequirements, // IRP_MN_FILTER_RESOURCE_REQUIREMENTS

    _PnpCompleteIrp,                // 0x0E

    _PnpCompleteIrp,                // IRP_MN_READ_CONFIG
    _PnpCompleteIrp,                // IRP_MN_WRITE_CONFIG
    _PnpEject,                      // IRP_MN_EJECT
    _PnpSetLock,                    // IRP_MN_SET_LOCK
    _PnpQueryId,                    // IRP_MN_QUERY_ID
    _PnpQueryPnpDeviceState,        // IRP_MN_QUERY_PNP_DEVICE_STATE
    _PnpQueryBusInformation,        // IRP_MN_QUERY_BUS_INFORMATION
    _PnpDeviceUsageNotification,    // IRP_MN_DEVICE_USAGE_NOTIFICATION
    _PnpSurpriseRemoval,            // IRP_MN_SURPRISE_REMOVAL
};

const PFN_PNP_POWER_CALLBACK  FxPkgPdo::m_PdoPowerFunctionTable[IRP_MN_QUERY_POWER + 1] =
{
    _DispatchWaitWake,      // IRP_MN_WAIT_WAKE
    _DispatchPowerSequence, // IRP_MN_POWER_SEQUENCE
    _DispatchSetPower,      // IRP_MN_SET_POWER
    _DispatchQueryPower,    // IRP_MN_QUERY_POWER
};

//#if defined(ALLOC_PRAGMA)
//#pragma code_seg("PAGE")
//#endif

FxPkgPdo::FxPkgPdo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice *Device
    ) :
    FxPkgPnp(FxDriverGlobals, Device, FX_TYPE_PACKAGE_PDO)
/*++

Routine Description:

    This is the constructor for the FxPkgPdo.  Don't do any initialization
    that might fail here.

Arguments:

    none

Returns:

    none

--*/

{
    m_DeviceTextHead.Next = NULL;

    m_DeviceID   = NULL;
    m_InstanceID = NULL;
    m_HardwareIDs = NULL;
    m_CompatibleIDs = NULL;
    m_ContainerID = NULL;
    m_IDsAllocation = NULL;

    m_RawOK = FALSE;
    m_Static = FALSE;
    m_AddedToStaticList = FALSE;

    //
    // By default the PDO is the owner of wait wake irps (only case where this
    // wouldn't be the case is for a bus filter to be sitting above us).
    //
    m_SharedPower.m_WaitWakeOwner = TRUE;

    m_Description = NULL;
    m_OwningChildList = NULL;

    m_EjectionDeviceList = NULL;
    m_DeviceEjectProcessed = NULL;

    m_CanBeDeleted = FALSE;
    m_EnableWakeAtBusInvoked = FALSE;
}

FxPkgPdo::~FxPkgPdo(
    VOID
    )
/*++

Routine Description:

    This is the destructor for the FxPkgPdo

Arguments:

    none

Returns:

    none

--*/
{
    //
    // If IoCreateDevice fails on a dynamically created PDO, m_Description will
    // be != NULL b/c we will not go through the pnp state machine in its entirety
    // for cleanup.  FxChildList will need a valid m_Description to cleanup upon
    // failure from EvtChildListDeviceCrate, so we just leave m_Description alone
    // here if != NULL.
    //
    // ASSERT(m_Description == NULL);

    FxDeviceText::_CleanupList(&m_DeviceTextHead);

    //
    // This will free the underlying memory for m_DeviceID, m_InstanceID,
    // m_HardwareIDs, m_CompatibleIDs and m_ContainerID
    //
    if (m_IDsAllocation != NULL) {
        FxPoolFree(m_IDsAllocation);
        m_IDsAllocation = NULL;
    }

    if (m_OwningChildList != NULL) {
        m_OwningChildList->RELEASE(this);
    }

    if (m_EjectionDeviceList != NULL) {
        delete m_EjectionDeviceList;
        m_EjectionDeviceList = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::Initialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Do initialization that might fail here.

Arguemnts:

    DeviceInit - the structure the driver passed in with initialization data

Returns:

    NTSTATUS

--*/
{
    NTSTATUS status;
    size_t cbLength, cbStrLength;
    PWSTR pCur;
    PdoInit* pPdo;

    status = FxPkgPnp::Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    cbLength = 0;

    //
    // Compute the total number of bytes required for all strings and then
    // make one allocation and remember pointers w/in the string so we can
    // retrieve them individually later.
    //
    pPdo = &DeviceInit->Pdo;
    cbLength += FxCalculateTotalStringSize(&pPdo->HardwareIDs);
    cbLength += FxCalculateTotalStringSize(&pPdo->CompatibleIDs);

    if (pPdo->DeviceID != NULL) {
        cbLength += pPdo->DeviceID->ByteLength(TRUE);
    }
    if (pPdo->InstanceID != NULL) {
        cbLength += pPdo->InstanceID->ByteLength(TRUE);
    }
    if (pPdo->ContainerID != NULL) {
        cbLength += pPdo->ContainerID->ByteLength(TRUE);
    }

    m_IDsAllocation = (PWSTR) FxPoolAllocate(GetDriverGlobals(),
                                             PagedPool,
                                             cbLength);

    if (m_IDsAllocation == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "DeviceInit %p could not allocate string for device IDs "
            "(length %I64d), %!STATUS!", DeviceInit, cbLength, status);

        return status;
    }

    pCur = m_IDsAllocation;

    m_HardwareIDs = pCur;
    pCur = FxCopyMultiSz(m_HardwareIDs, &pPdo->HardwareIDs);

    m_CompatibleIDs = pCur;
    pCur = FxCopyMultiSz(m_CompatibleIDs, &pPdo->CompatibleIDs);

    if (pPdo->DeviceID != NULL) {
        m_DeviceID = pCur;

        //
        // Copy the bytes and then null terminate the buffer
        //
        cbStrLength = pPdo->DeviceID->ByteLength(FALSE);

        RtlCopyMemory(m_DeviceID,
                      pPdo->DeviceID->Buffer(),
                      cbStrLength);

        m_DeviceID[cbStrLength / sizeof(WCHAR)] = UNICODE_NULL;

        pCur = (PWSTR) WDF_PTR_ADD_OFFSET(m_DeviceID,
                                          cbStrLength + sizeof(UNICODE_NULL));
    }

    if (pPdo->InstanceID != NULL) {
        m_InstanceID = pCur;

        //
        // Copy the bytes and then null terminate the buffer
        //
        cbStrLength = pPdo->InstanceID->ByteLength(FALSE);

        RtlCopyMemory(m_InstanceID,
                      pPdo->InstanceID->Buffer(),
                      cbStrLength);

        m_InstanceID[cbStrLength / sizeof(WCHAR)] = UNICODE_NULL;

        pCur = (PWSTR) WDF_PTR_ADD_OFFSET(m_InstanceID,
                                          cbStrLength + sizeof(UNICODE_NULL));
    }

    if (pPdo->ContainerID != NULL) {
        m_ContainerID = pCur;

        //
        // Copy the bytes and then null terminate the buffer
        //
        cbStrLength = pPdo->ContainerID->ByteLength(FALSE);

        RtlCopyMemory(m_ContainerID,
                      pPdo->ContainerID->Buffer(),
                      cbStrLength);

        m_ContainerID[cbStrLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    m_Static = pPdo->Static;

    if (m_Static) {
        //
        // Statically enumerated children do not support reenumeration requests
        // from the stack on top of them.
        //

        //
        // The only way we can have static children is if an FDO enumerates them.
        //





        Mx::MxAssert(m_Device->m_ParentDevice->IsFdo());
        m_OwningChildList = m_Device->m_ParentDevice->GetFdoPkg()->m_StaticDeviceList;

        m_OwningChildList->ADDREF(this);
    }
    else {
        m_Description = pPdo->DescriptionEntry;

        m_OwningChildList = m_Description->GetParentList();
        m_OwningChildList->ADDREF(this);
    }

    return STATUS_SUCCESS;
}

VOID
FxPkgPdo::FinishInitialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    PdoInit* pdoInit;

    pdoInit = &DeviceInit->Pdo;

    m_DefaultLocale = pdoInit->DefaultLocale;
    m_DeviceTextHead.Next = pdoInit->DeviceText.Next;
    pdoInit->DeviceText.Next = NULL;

    //
    // Important to do this last since this will cause a pnp state machine
    // transition
    //
    __super::FinishInitialize(DeviceInit);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::SendIrpSynchronously(
    __in FxIrp* Irp
    )
/*++

Routine Description:
    Virtual override for synchronously sending a request down the stack and
    catching it on the way back up.  For PDOs, we are the bottom, so this is a
    no-op.

Arguments:
    Irp - The request

Return Value:
    Status in the Irp

  --*/
{
    return Irp->GetStatus();
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::FireAndForgetIrp(
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    Virtual override for sending a request down the stack and forgetting about
    it.  Since we are the bottom of the stack, just complete the request.

Arguments:

Return Value:

--*/
{
    NTSTATUS status;

    status = Irp->GetStatus();

    if (Irp->GetMajorFunction() == IRP_MJ_POWER) {
        return CompletePowerRequest(Irp, status);
    }
    else {
        return CompletePnpRequest(Irp, status);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpCompleteIrp(
    __in    FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->CompletePnpRequest(Irp, Irp->GetStatus());
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryDeviceRelations(
    __in    FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->PnpQueryDeviceRelations(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PnpQueryDeviceRelations(
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    This method is called in response to a PnP QDR. PDOs handle Ejection
    Relations and Target Relations.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/
{
    PDEVICE_RELATIONS pDeviceRelations;
    NTSTATUS status;
    DEVICE_RELATION_TYPE type;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    status = Irp->GetStatus();
    pFxDriverGlobals = GetDriverGlobals();

    type = Irp->GetParameterQDRType();
    switch (type) {
    case BusRelations:
        status = HandleQueryBusRelations(Irp);
        break;

    case EjectionRelations:
    case RemovalRelations:
        status = HandleQueryDeviceRelations(
            Irp,
            (type == RemovalRelations) ? m_RemovalDeviceList
                                       : m_EjectionDeviceList);

        //
        // STATUS_NOT_SUPPORTED is a special value. It means that
        // HandleQueryDeviceRelations did not modify the irp at all and it
        // should be sent off as is.
        //
        if (status == STATUS_NOT_SUPPORTED) {
            //
            // Complete the request with the status it was received with
            //
            status = Irp->GetStatus();
        }
        break;

    case TargetDeviceRelation:
        pDeviceRelations = (PDEVICE_RELATIONS) MxMemory::MxAllocatePoolWithTag(
                PagedPool, sizeof(DEVICE_RELATIONS), pFxDriverGlobals->Tag);

        if (pDeviceRelations != NULL) {
            PDEVICE_OBJECT pDeviceObject;

            pDeviceObject = reinterpret_cast<PDEVICE_OBJECT> (m_Device->GetDeviceObject());

            Mx::MxReferenceObject(pDeviceObject);

            pDeviceRelations->Count = 1;
            pDeviceRelations->Objects[0] = pDeviceObject;

            Irp->SetInformation((ULONG_PTR) pDeviceRelations);
            status = STATUS_SUCCESS;
        }
        else {
            Irp->SetInformation(NULL);
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p failing TargetDeviceRelations, %!STATUS!",
                m_Device->GetHandle(), status);
        }
        break;
    }

    return CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryInterface(
    IN FxPkgPnp* This,
    IN FxIrp *Irp
    )
/*++

Routine Description:
    Query interface handler for the PDO

Arguments:
    This - the package

    Irp - the QI request

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;
    BOOLEAN completeIrp;

    status = ((FxPkgPdo*) This)->HandleQueryInterface(Irp, &completeIrp);

    return ((FxPkgPdo*) This)->CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryCapabilities(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->PnpQueryCapabilities(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PnpQueryCapabilities(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryCapabilities IRP.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    PDEVICE_CAPABILITIES pDeviceCapabilities;
    STACK_DEVICE_CAPABILITIES parentStackCapabilities = {0};
    NTSTATUS status;

    status = STATUS_UNSUCCESSFUL;

    pDeviceCapabilities = Irp->GetParameterDeviceCapabilities();

    //
    // Confirm this is a valid DeviceCapabilities structure.
    //
    ASSERT(pDeviceCapabilities->Size >= sizeof(DEVICE_CAPABILITIES));
    ASSERT(pDeviceCapabilities->Version == 1);

    if ((pDeviceCapabilities->Version == 1) &&
        (pDeviceCapabilities->Size >= sizeof(DEVICE_CAPABILITIES))) {

        //
        // Since query caps must be sent to the parent stack until it reaches
        // the root, we can quickly run out of stack space.  If that happens,
        // then move to a work item to get a fresh stack with plenty of stack
        // space.
        //
        if (Mx::MxHasEnoughRemainingThreadStack() == FALSE) {
            MxWorkItem workItem;

            status = workItem.Allocate(m_Device->GetDeviceObject());

            if (NT_SUCCESS(status)) {
                //
                // Store off the work item so we can free it in the worker routine
                //
                Irp->SetContext(0, (PVOID)workItem.GetWorkItem());

                //
                // Mark the irp as pending because it will be completed in
                // another thread
                //
                Irp->MarkIrpPending();

                //
                // Kick off to another thread
                //
                workItem.Enqueue(_QueryCapsWorkItem, Irp->GetIrp());

                return STATUS_PENDING;
            }
            else {
                //
                // Not enough for a work item, return error
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            MxDeviceObject parentDeviceObject;

            parentDeviceObject.SetObject(
                m_Device->m_ParentDevice->GetDeviceObject());
            status = GetStackCapabilities(
                GetDriverGlobals(),
                &parentDeviceObject,
                NULL,   // D3ColdInterface
                &parentStackCapabilities);

            if (NT_SUCCESS(status)) {
#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "prefast is confused")
                HandleQueryCapabilities(pDeviceCapabilities,
                                        &parentStackCapabilities.DeviceCaps);

                //
                // The check above does not guarantee STATUS_SUCCESS explicitly
                // (ie the verifier can change the value to something other then
                // STATUS_SUCCESS) so set it here
                //
                status = STATUS_SUCCESS;
            }
        }
    }

    return CompletePnpRequest(Irp, status);
}

VOID
FxPkgPdo::HandleQueryCapabilities(
    __inout PDEVICE_CAPABILITIES ReportedCaps,
    __in_bcount(ParentCaps->size) PDEVICE_CAPABILITIES ParentCaps
    )
{
    LONG pnpCaps;
    ULONG i;

    //
    // PowerSystemUnspecified is reserved for system use as per the DDK
    //
    for (i = PowerSystemWorking; i < PowerSystemMaximum; i++) {
        DEVICE_POWER_STATE state;

        state = _GetPowerCapState(i, m_PowerCaps.States);

        if (state == PowerDeviceMaximum) {
            //
            // PDO did not specify any value, use parent's cap
            //
#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "Esp:675")
            ReportedCaps->DeviceState[i] = ParentCaps->DeviceState[i];
        }
        else {
            //
            // Use PDO's reported value
            //
            ReportedCaps->DeviceState[i] = state;
        }
    }

    pnpCaps = GetPnpCapsInternal();

    //
    // Appropriately fill the DeviceCapabilities structure.
    //
    SET_PNP_CAP(pnpCaps, ReportedCaps, LockSupported);
    SET_PNP_CAP(pnpCaps, ReportedCaps, EjectSupported);
    SET_PNP_CAP(pnpCaps, ReportedCaps, Removable);
    SET_PNP_CAP(pnpCaps, ReportedCaps, DockDevice);
    SET_PNP_CAP(pnpCaps, ReportedCaps, UniqueID);

    if ((pnpCaps & FxPnpCapSilentInstallMask) != FxPnpCapSilentInstallUseDefault) {
        SET_PNP_CAP(pnpCaps, ReportedCaps, SilentInstall);
    }
    else if (m_RawOK) {
        //
        // By default, we report raw devices as silent install devices
        // because if they are raw, they don't need any further
        // installation.
        //
        ReportedCaps->SilentInstall = TRUE;
    }

    SET_PNP_CAP(pnpCaps, ReportedCaps, SurpriseRemovalOK);
    SET_PNP_CAP(pnpCaps, ReportedCaps, HardwareDisabled);
    SET_PNP_CAP(pnpCaps, ReportedCaps, NoDisplayInUI);

    SET_POWER_CAP(m_PowerCaps.Caps , ReportedCaps, WakeFromD0);
    SET_POWER_CAP(m_PowerCaps.Caps , ReportedCaps, WakeFromD1);
    SET_POWER_CAP(m_PowerCaps.Caps , ReportedCaps, WakeFromD2);
    SET_POWER_CAP(m_PowerCaps.Caps , ReportedCaps, WakeFromD3);
    SET_POWER_CAP(m_PowerCaps.Caps , ReportedCaps, DeviceD1);
    SET_POWER_CAP(m_PowerCaps.Caps , ReportedCaps, DeviceD2);

    if (m_RawOK) {
        ReportedCaps->RawDeviceOK = TRUE;
    }

    ReportedCaps->UINumber = m_PnpCapsUINumber;
    ReportedCaps->Address  = m_PnpCapsAddress;

    if (m_PowerCaps.SystemWake != PowerSystemMaximum) {
        ReportedCaps->SystemWake = (SYSTEM_POWER_STATE) m_PowerCaps.SystemWake;
    }
    else {
        ReportedCaps->SystemWake = ParentCaps->SystemWake;
    }

    //
    // Set the least-powered device state from which the device can
    // wake the system.
    //
    if (m_PowerCaps.DeviceWake != PowerDeviceMaximum) {
        ReportedCaps->DeviceWake = (DEVICE_POWER_STATE) m_PowerCaps.DeviceWake;
    }
    else {
        ReportedCaps->DeviceWake = ParentCaps->DeviceWake;
    }

    //
    // Set the Device wake up latencies.
    //
    if (m_PowerCaps.D1Latency != (ULONG) -1) {
        ReportedCaps->D1Latency = m_PowerCaps.D1Latency;
    }
    else {
        ReportedCaps->D1Latency = 0;
    }

    if (m_PowerCaps.D2Latency != (ULONG) -1) {
        ReportedCaps->D2Latency = m_PowerCaps.D2Latency;
    }
    else {
        ReportedCaps->D2Latency = 0;
    }

    if (m_PowerCaps.D3Latency != (ULONG) -1) {
        ReportedCaps->D3Latency = m_PowerCaps.D3Latency;
    }
}

VOID
FxPkgPdo::_QueryCapsWorkItem(
    __in MdDeviceObject DeviceObject,
    __in PVOID Context
    )
{
    STACK_DEVICE_CAPABILITIES parentCapabilities;
    MdWorkItem pItem;
    FxPkgPdo* pPkgPdo;
    FxIrp irp;
    NTSTATUS status;
    MxDeviceObject parentDeviceObject;

    irp.SetIrp((MdIrp)Context);
    pItem = (MdWorkItem) irp.GetContext(0);

    pPkgPdo = FxDevice::GetFxDevice(DeviceObject)->GetPdoPkg();

    parentDeviceObject.SetObject(
        pPkgPdo->m_Device->m_ParentDevice->GetDeviceObject());

    status = GetStackCapabilities(
        pPkgPdo->m_Device->GetDriverGlobals(),
        &parentDeviceObject,
        NULL, // D3ColdInterface
        &parentCapabilities);

    if (NT_SUCCESS(status)) {
#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "prefast is confused")
        pPkgPdo->HandleQueryCapabilities(
            irp.GetParameterDeviceCapabilities(),
            &parentCapabilities.DeviceCaps
            );
        status = STATUS_SUCCESS;
    }

    pPkgPdo->CompletePnpRequest(&irp, status);

    MxWorkItem::_Free(pItem);
}

FxDeviceText *
FindObjectForGivenLocale(
    __in PSINGLE_LIST_ENTRY Head,
    __in LCID LocaleId
    )

/*++

Routine Description:



Arguments:

Returns:

--*/

{
    PSINGLE_LIST_ENTRY ple;

    for (ple = Head->Next; ple != NULL; ple = ple->Next) {
        FxDeviceText *pDeviceText;

        pDeviceText= FxDeviceText::_FromEntry(ple);

        if (pDeviceText->m_LocaleId == LocaleId) {
            //
            // We found our object!
            //
            return pDeviceText;
        }
    }

    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryDeviceText(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryDeviceText IRP.
    We return the decription or the location.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    FxPkgPdo* pThis;
    LCID localeId;
    FxDeviceText *pDeviceTextObject;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pThis = (FxPkgPdo*) This;
    pFxDriverGlobals = pThis->GetDriverGlobals();

    localeId = Irp->GetParameterQueryDeviceTextLocaleId();
    status = Irp->GetStatus();

    //
    // The PDO package maintains a collection of "DeviceText" objects.  We
    // will look up the item in the collection with the "appropriate" locale.
    //
    // If no entries are found in the collection for the given locale, then
    // we will use the "default locale" property of the PDO and use the
    // entry for the "default locale".
    //

    //
    // Try to find the FxDeviceText object for the given locale.
    //
    pDeviceTextObject = FindObjectForGivenLocale(
        &pThis->m_DeviceTextHead, localeId);

    if (pDeviceTextObject == NULL) {
        pDeviceTextObject = FindObjectForGivenLocale(
            &pThis->m_DeviceTextHead, pThis->m_DefaultLocale);
    }

    if (pDeviceTextObject != NULL) {
        PWCHAR pInformation;

        pInformation = NULL;

        switch (Irp->GetParameterQueryDeviceTextType()) {
        case DeviceTextDescription:
            pInformation = pDeviceTextObject->m_Description;
            break;

        case DeviceTextLocationInformation:
            pInformation = pDeviceTextObject->m_LocationInformation;
            break;
        }

        //
        // Information should now point to a valid unicode string.
        //
        if (pInformation != NULL) {
            PWCHAR pBuffer;
            size_t length;

            length = (wcslen(pInformation) + 1) * sizeof(WCHAR);

            //
            // Make sure the information field of the IRP isn't already set.
            //
            ASSERT(Irp->GetInformation() == NULL);

            pBuffer = (PWCHAR) MxMemory::MxAllocatePoolWithTag(
                PagedPool, length, pFxDriverGlobals->Tag);

            if (pBuffer != NULL) {
                RtlCopyMemory(pBuffer, pInformation, length);
                Irp->SetInformation((ULONG_PTR) pBuffer);

                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFDEVICE %p failing Query Device Text, type %d, %!STATUS!",
                    pThis->m_Device->GetHandle(),
                    Irp->GetParameterQueryDeviceTextType(), status);
            }
        }
    }

    return pThis->CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpEject(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    Ejection is handled by the PnP state machine. Handle it synchronously.
    Don't pend it since PnP manager does not serilaize it with other state
    changing pnp irps if handled asynchronously.

Arguments:

    This - the package

    Irp - the request

Return Value:

    NTSTATUS

  --*/
{
    MxEvent event;
    FxPkgPdo* pdoPkg;
    NTSTATUS status;

    pdoPkg = (FxPkgPdo*)This;

    //
    // This will make sure no new state changing pnp irps arrive while
    // we are still processing this one. Also, note that irp is not being
    // marked pending.
    //
    pdoPkg->SetPendingPnpIrp(Irp, FALSE);

    status = event.Initialize(SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {

        DoTraceLevelMessage(
            pdoPkg->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Event allocation failed while processing eject for WDFDEVICE %p,"
            " %!STATUS!",
            pdoPkg->m_Device->GetHandle(), status);
    }
    else {
        ASSERT(pdoPkg->m_DeviceEjectProcessed == NULL);
        pdoPkg->m_DeviceEjectProcessed = event.GetSelfPointer();

        //
        // let state machine process eject
        //
        pdoPkg->PnpProcessEvent(PnpEventEject);

        //
        // No need to wait in a critical region because we are in the context of a
        // pnp request which is in the system context.
        //
        event.WaitFor(Executive, KernelMode, FALSE, NULL);

        pdoPkg->m_DeviceEjectProcessed = NULL;

        status = Irp->GetStatus();
    }

    //
    // complete request
    //

    pdoPkg->ClearPendingPnpIrp();
    pdoPkg->CompletePnpRequest(Irp, status);

    return status;
}

WDF_DEVICE_PNP_STATE
FxPkgPdo::PnpEventEjectHardwareOverload(
    VOID
    )
/*++

Routine Description:

    This function implements the EjectHardware state.  This
    function overloads the base PnP State machine handler.  Its
    job is to call EvtDeviceEject.  If that succeeds, then we
    transition immediately to EjectedWaitingForRemove.  If not,
    then to EjectFailed.

Arguments:

    none

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    WDF_DEVICE_PNP_STATE state;

    status = m_DeviceEject.Invoke(m_Device->GetHandle());

    if (NT_SUCCESS(status)) {

        //
        // Upon a successful eject, mark the child as missing so that when we
        // get another QDR, it is not re-reported.
        //
        FxChildList* pList;
        MxEvent* event;

        pList = m_Description->GetParentList();

        status = pList->UpdateAsMissing(m_Description->GetId());
        if (NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "PDO WDFDEVICE %p !devobj %p marked missing as a result of eject",
                m_Device->GetHandle(), m_Device->GetDeviceObject());
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Failed to mark PDO WDFDEVICE %p !devobj %p missing after eject %!STATUS!",
                m_Device->GetHandle(), m_Device->GetDeviceObject(),
                status);
        }

        //
        // We must wait for any pending scans to finish so that the previous
        // update as missing is enacted into the list and reported to the
        // OS.  Otherwise, if we don't wait we could be in the middle of a
        // scan, complete the eject, report the child again and it will be
        // reenumerated.
        //
        event = pList->GetScanEvent();

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "waiting on event %p for device to finish scanning",
                            &event);

        //
        // No need to wait in a crtical region because we are in the context of a
        // pnp request which is in the system context.
        //
        event->WaitFor(Executive, KernelMode, FALSE, NULL);

        //
        // Change the state.
        //
        state = WdfDevStatePnpEjectedWaitingForRemove;
    }
    else {
        state = WdfDevStatePnpEjectFailed;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Eject failed since driver's EvtDeviceEject returned %!STATUS!", status);

        if (status == STATUS_NOT_SUPPORTED) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "EvtDeviceEject returned an invalid status STATUS_NOT_SUPPORTED");

            if (GetDriverGlobals()->IsVerificationEnabled(1, 11, OkForDownLevel)) {
                FxVerifierDbgBreakPoint(GetDriverGlobals());
            }
        }
    }

    //
    // set irp status
    //
    SetPendingPnpIrpStatus(status);

    //
    // Pnp dispatch routine is waiting on this event, and it will complete
    // the Eject irp
    //
    m_DeviceEjectProcessed->Set();

    return state;
}

WDF_DEVICE_PNP_STATE
FxPkgPdo::PnpEventCheckForDevicePresenceOverload(
    VOID
    )
/*++

Routine Description:

    This function implements the CheckForDevicePresence state.  This
    function overloads the base PnP State machine handler.  It's
    job is to figure out whether the removed device is actually
    still attached.  It then changes state based on that result.

Arguments:

    none

Return Value:

    NTSTATUS

--*/

{
    if (m_Description != NULL) {
        if (m_Description->IsDeviceRemoved()) {
            //
            // The description freed itself now that the device has been reported
            // missing.
            //
            return WdfDevStatePnpPdoRemoved;
        }
        else {
            //
            // Device was not reported as missing, keep it alive
            //
            return WdfDevStatePnpRemovedPdoWait;
        }
    }
    else {
        //
        // Only static children can get this far without having an m_Description
        //
        ASSERT(m_Static);

        //
        // The description freed itself now that the device has been reported
        // missing.
        //
        return WdfDevStatePnpPdoRemoved;
    }
}

WDF_DEVICE_PNP_STATE
FxPkgPdo::PnpEventPdoRemovedOverload(
    VOID
    )
/*++

Routine Description:

    This function implements the Removed state.  This
    function overloads the base PnP State machine handler.

Arguments:

    none

Return Value:

    NTSTATUS

--*/
{
    m_CanBeDeleted = TRUE;

    //
    // Unconditionally delete the symbolic link now (vs calling
    // DeleteSymbolicLinkOverload()) because we know that the device has been
    // reported as missing.
    //
    m_Device->DeleteSymbolicLink();

    //
    // Do that which all device stacks need to do upon removal.
    //
    PnpEventRemovedCommonCode();

    //
    // m_Device is Release()'ed in FxPkgPnp::PnpEventFinal
    //
    if (m_Description != NULL) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Removing entry reference %p on FxPkgPnp %p",
                            m_Description, this);

        m_Description->ProcessDeviceRemoved();
        m_Description = NULL;
    }

    return WdfDevStatePnpFinal;
}

WDF_DEVICE_PNP_STATE
FxPkgPdo::PnpGetPostRemoveState(
    VOID
    )
{
    //
    // Transition to the check for device presence state.
    //
    return WdfDevStatePnpCheckForDevicePresence;
}

WDF_DEVICE_PNP_STATE
FxPkgPdo::PnpEventFdoRemovedOverload(
    VOID
    )
{
    ASSERT(!"This should only be implemented for FDOs.");

    //
    // Do something safe.  Act like the device is not present.
    //
    return WdfDevStatePnpFinal;
}

VOID
FxPkgPdo::PnpEventSurpriseRemovePendingOverload(
    VOID
    )
{
    if (m_Description != NULL) {
        m_Description->DeviceSurpriseRemoved();
    }

    FxPkgPnp::PnpEventSurpriseRemovePendingOverload();
}

BOOLEAN
FxPkgPdo::PnpSendStartDeviceDownTheStackOverload(
    VOID
    )
/*++

Routine Description:
    Process the start irp on the way down the stack during a start.

    Since the PDO is the bottom, just move to the new state where we
    are processing the irp up the stack.

Arguments:
    None

Return Value:
    TRUE, the completion of the start irp down the stack was synchronous

  --*/
{
    //
    // We are successful so far, indicate this in the irp.
    //
    SetPendingPnpIrpStatus(STATUS_SUCCESS);

    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpSetLock(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:
    Set lock

Arguments:
    This - the package

    Irp - the irp

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;
    BOOLEAN lock;

    lock = Irp->GetParameterSetLockLock();

    status = ((FxPkgPdo*) This)->m_DeviceSetLock.Invoke(
        ((FxPkgPdo*) This)->m_Device->GetHandle(), lock);

    if (NT_SUCCESS(status)) {
        Irp->SetInformation(NULL);
    }

    return ((FxPkgPdo*) This)->CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryId(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    FxPkgPdo* pThis;
    NTSTATUS status;
    PWCHAR pBuffer;
    PCWSTR pSrc;
    size_t cbLength;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    BUS_QUERY_ID_TYPE queryIdType;

    pThis = (FxPkgPdo*) This;
    pFxDriverGlobals  = pThis->GetDriverGlobals();
    status = Irp->GetStatus();
    cbLength = 0;

    queryIdType = Irp->GetParameterQueryIdType();

    switch (queryIdType) {
    case BusQueryDeviceID:
    case BusQueryInstanceID:
    case BusQueryContainerID:
        if (queryIdType == BusQueryDeviceID) {
            pSrc = pThis->m_DeviceID;
        }
        else if (queryIdType == BusQueryInstanceID) {
            pSrc = pThis->m_InstanceID;
        }
        else {
            pSrc = pThis->m_ContainerID;
        }

        if (pSrc != NULL) {
            cbLength = (wcslen(pSrc) + 1) * sizeof(WCHAR);

            pBuffer = (PWCHAR) MxMemory::MxAllocatePoolWithTag(
                PagedPool, cbLength, pFxDriverGlobals->Tag);
        }
        else {
            status = Irp->GetStatus();
            break;
        }

        if (pBuffer != NULL) {

            //
            // This will copy the NULL terminator too
            //
            RtlCopyMemory(pBuffer, pSrc, cbLength);
            Irp->SetInformation((ULONG_PTR) pBuffer);
            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        break;

    case BusQueryHardwareIDs:
    case BusQueryCompatibleIDs:
        if (queryIdType == BusQueryHardwareIDs) {
            pSrc = pThis->m_HardwareIDs;
        }
        else {
            pSrc = pThis->m_CompatibleIDs;
        }

        if (pSrc != NULL) {
            cbLength = FxCalculateTotalMultiSzStringSize(pSrc);
        }
        else {
            //
            // Must return an empty list
            //
            cbLength = 2 * sizeof(UNICODE_NULL);
        }

        pBuffer = (PWCHAR) MxMemory::MxAllocatePoolWithTag(
            PagedPool, cbLength, pFxDriverGlobals->Tag);

        if (pBuffer != NULL) {
            if (pSrc != NULL) {
                RtlCopyMemory(pBuffer, pSrc, cbLength);
            }
            else {
                RtlZeroMemory(pBuffer, cbLength);
            }

            Irp->SetInformation((ULONG_PTR) pBuffer);
            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        break;
    }

    if (!NT_SUCCESS(status)) {
        Irp->SetInformation(NULL);

        if (status == STATUS_NOT_SUPPORTED) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p does not have a string for PnP query IdType "
                "%!BUS_QUERY_ID_TYPE!, %!STATUS!",
                pThis->m_Device->GetHandle(),
                queryIdType, status);
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p could not alloc string for PnP query IdType "
                "%!BUS_QUERY_ID_TYPE!, %!STATUS!",
                pThis->m_Device->GetHandle(),
                queryIdType, status);
        }
    }

    return ((FxPkgPdo*) pThis)->CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryPnpDeviceState(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:
    indicates the current device state

Arguments:
    This - the package

    Irp - the request

Return Value:
    NTSTATUS

  --*/
{
    PNP_DEVICE_STATE pnpDeviceState;
    PFX_DRIVER_GLOBALS FxDriverGlobals = This->GetDriverGlobals();

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering QueryPnpDeviceState handler");

    pnpDeviceState = ((FxPkgPdo*) This)->HandleQueryPnpDeviceState(
        (PNP_DEVICE_STATE) Irp->GetInformation());

    Irp->SetInformation((ULONG_PTR) pnpDeviceState);

    DoTraceLevelMessage(
        FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p returning PNP_DEVICE_STATE 0x%d IRP 0x%p",
        This->GetDevice()->GetHandle(),
        This->GetDevice()->GetDeviceObject(),
        pnpDeviceState,
        Irp->GetIrp());

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting QueryPnpDeviceState handler");

    return ((FxPkgPdo*) This)->CompletePnpRequest(Irp, STATUS_SUCCESS);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryBusInformation(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:
    Returns the bus information for this child

Arguments:
    This - the package

    Irp - the request

Return Value:
    NTSTATUS

  --*/
{
    FxPkgPdo* pThis;
    NTSTATUS status;

    pThis = (FxPkgPdo*) This;

    status = pThis->m_Device->m_ParentDevice->m_PkgPnp->
        HandleQueryBusInformation(Irp);

    return pThis->CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpSurpriseRemoval(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    FxPkgPdo* pThis;

    pThis = (FxPkgPdo*) This;

    pThis->m_Description->DeviceSurpriseRemoved();

    return pThis->PnpSurpriseRemoval(Irp);
}

VOID
FxPkgPdo::RegisterCallbacks(
    __in PWDF_PDO_EVENT_CALLBACKS DispatchTable
    )
{
    m_DeviceResourcesQuery.m_Method            = DispatchTable->EvtDeviceResourcesQuery;
    m_DeviceResourceRequirementsQuery.m_Method = DispatchTable->EvtDeviceResourceRequirementsQuery;
    m_DeviceEject.m_Method                     = DispatchTable->EvtDeviceEject;
    m_DeviceSetLock.m_Method                   = DispatchTable->EvtDeviceSetLock;

    m_DeviceEnableWakeAtBus.m_Method           = DispatchTable->EvtDeviceEnableWakeAtBus;
    m_DeviceDisableWakeAtBus.m_Method          = DispatchTable->EvtDeviceDisableWakeAtBus;

    //
    // this callback was added in V1.11
    //
    if (DispatchTable->Size > sizeof(WDF_PDO_EVENT_CALLBACKS_V1_9)) {
        m_DeviceReportedMissing.m_Method       = DispatchTable->EvtDeviceReportedMissing;
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::AskParentToRemoveAndReenumerate(
    VOID
    )
/*++

Routine Description:
    This routine asks the PDO to ask its parent bus driver to Surprise-Remove
    and re-enumerate the PDO.  This will be done only at the point of
    catastrophic software failure, and occasionally after catastrophic hardware
    failure.

Arguments:
    None

Return Value:
    status

  --*/
{
    //
    // Static children do not support reenumeration.
    //
    if (m_Description != NULL && m_Static == FALSE) {
        m_Description->GetParentList()->ReenumerateEntry(m_Description);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}


VOID
FxPkgPdo::DeleteSymbolicLinkOverload(
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
    if (GracefulRemove) {
        //
        // We will remove the symbolic link when we determine if the PDO was
        // reported missing or not.
        //
        return;
    }
    else if (m_Description->IsDeviceReportedMissing()) {
        //
        // Surprise removed and we have reported the PDO as missing
        //

        m_Device->DeleteSymbolicLink();
    }
}


VOID
FxPkgPdo::_RemoveAndReenumerateSelf(
    __in PVOID Context
    )
/*++

Routine Description:
    This routine is passed out to higher-level drivers as part of the
    re-enumeration interface.

Arguments:
    This

Return Value:
    void

  --*/
{
    ((FxPkgPdo*) Context)->AskParentToRemoveAndReenumerate();
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::HandleQueryInterfaceForReenumerate(
    __in  FxIrp* Irp,
    __out PBOOLEAN CompleteRequest
    )
/*++

Routine Description:
    Handles a query interface on the PDO for the self reenumeration interface.

Arguments:
    Irp - the request containing the QI

    CompleteRequest - whether the caller should complete the request when this
                      call returns

Return Value:
    status to complete the irp with

  --*/
{
    PREENUMERATE_SELF_INTERFACE_STANDARD pInterface;
    NTSTATUS status;

    *CompleteRequest = TRUE;

    if (m_Static) {
        //
        // Return the embedded status in the irp since this is a statically
        // enumerated child.  Only dynamically enuemrated child support self
        // reenumeration.
        //
        return Irp->GetStatus();
    }

    if (Irp->GetParameterQueryInterfaceVersion() == 1 &&
        Irp->GetParameterQueryInterfaceSize() >= sizeof(*pInterface)) {

        pInterface = (PREENUMERATE_SELF_INTERFACE_STANDARD)
            Irp->GetParameterQueryInterfaceInterface();

        //
        // Expose the interface to the requesting driver.
        //
        pInterface->Version = 1;
        pInterface->Size = sizeof(*pInterface);
        pInterface->Context = this;
        pInterface->InterfaceReference = FxDevice::_InterfaceReferenceNoOp;
        pInterface->InterfaceDereference = FxDevice::_InterfaceDereferenceNoOp;
        pInterface->SurpriseRemoveAndReenumerateSelf = &FxPkgPdo::_RemoveAndReenumerateSelf;

        status = STATUS_SUCCESS;

        //
        // Caller assumes a reference has been taken.
        //
        pInterface->InterfaceReference(pInterface->Context);
    }
    else {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::ProcessRemoveDeviceOverload(
    __inout FxIrp* Irp
    )
{
    if (m_CanBeDeleted) {
        //
        // After this is called, any irp dispatched to FxDevice::DispatchWithLock
        // will fail with STATUS_INVALID_DEVICE_REQUEST.
        //







        Mx::MxReleaseRemoveLockAndWait(
            m_Device->GetRemoveLock(),
            Irp->GetIrp()
            );

        //
        // Cleanup the state machines and release the power thread.
        //
        CleanupStateMachines(TRUE);

        //
        // Detach and delete the device object.
        //
        DeleteDevice();

        //
        // Can't call CompletePnpRequest because we just released the tag in
        // IoReleaseRemoveLockAndWait above.
        //
        Irp->CompleteRequest(IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }
    else {
        //
        // This was a PDO which was not reported missing, so do not free the
        // memory and clear out our stack local address.
        //
        m_DeviceRemoveProcessed = NULL;
        return CompletePnpRequest(Irp, Irp->GetStatus());
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::QueryForPowerThread(
    VOID
    )
/*++

Routine Description:
    Since the PDO is the lowest device in the stack, it does not have to send
    a query down the stack.  Rather, it just creates the thread and returns.

Arguments:
    None

Return Value:
    NTSTATUS

  --*/
{
    return CreatePowerThread();
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::AddEjectionDevice(
    __in MdDeviceObject DependentDevice
    )
{
    FxRelatedDevice* pRelated;
    NTSTATUS status;

    if (m_EjectionDeviceList == NULL) {
        KIRQL irql;

        Lock(&irql);
        if (m_EjectionDeviceList == NULL) {
            m_EjectionDeviceList = new (GetDriverGlobals()) FxRelatedDeviceList();

            if (m_EjectionDeviceList != NULL) {
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Could not allocate ejection device list for PDO WDFDEVICE %p",

                    m_Device->GetHandle());
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

    status = m_EjectionDeviceList->Add(GetDriverGlobals(), pRelated);

    if (NT_SUCCESS(status)) {
        //
        // EjectRelations are queried automatically by PnP when the device is
        // going to be ejected.  No need to tell pnp that the list changed
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
FxPkgPdo::RemoveEjectionDevice(
    __in MdDeviceObject DependentDevice
    )
{
    if (m_EjectionDeviceList != NULL) {
        m_EjectionDeviceList->Remove(GetDriverGlobals(), DependentDevice);
    }

    //
    // EjectRelations are queried automatically by PnP when the device is
    // going to be ejected.  No need to tell pnp that the list changed
    // until it needs to query for it.
    //
}

VOID
FxPkgPdo::ClearEjectionDevicesList(
    VOID
    )
{
    FxRelatedDevice* pEntry;

    if (m_EjectionDeviceList != NULL) {
        m_EjectionDeviceList->LockForEnum(GetDriverGlobals());
        while ((pEntry = m_EjectionDeviceList->GetNextEntry(NULL)) != NULL) {
            m_EjectionDeviceList->Remove(GetDriverGlobals(),
                                         pEntry->GetDevice());
        }
        m_EjectionDeviceList->UnlockFromEnum(GetDriverGlobals());
    }

    //
    // EjectRelations are queried automatically by PnP when the device is
    // going to be ejected.  No need to tell pnp that the list changed
    // until it needs to query for it.
    //
}
