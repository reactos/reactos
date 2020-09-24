/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxPkgPdo.hpp

Abstract:

    This module implements the pnp package for Pdo objects.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXPKGPDO_H_
#define _FXPKGPDO_H_

typedef NTSTATUS (FxPkgPdo::*PFN_PDO_HANDLER)(FxIrp *Irp);

class FxPkgPdo : public FxPkgPnp
{
public:
    //
    // Properties used in handling IRP_MN_QUERY_DEVICE_TEXT
    //
    SINGLE_LIST_ENTRY m_DeviceTextHead;
    LCID m_DefaultLocale;

    FxDeviceDescriptionEntry* m_Description;

    FxChildList* m_OwningChildList;

    //
    // The following structure contains the function pointer table
    // for the "events" this package can raise.
    //
    FxPnpDeviceResourcesQuery            m_DeviceResourcesQuery;
    FxPnpDeviceResourceRequirementsQuery m_DeviceResourceRequirementsQuery;
    FxPnpDeviceEject                     m_DeviceEject;
    FxPnpDeviceSetLock                   m_DeviceSetLock;

    FxPowerDeviceEnableWakeAtBus         m_DeviceEnableWakeAtBus;
    FxPowerDeviceDisableWakeAtBus        m_DeviceDisableWakeAtBus;

    // added in 1.11
    FxPnpDeviceReportedMissing           m_DeviceReportedMissing;

    BOOLEAN m_RawOK;

    BOOLEAN m_Static;

    BOOLEAN m_AddedToStaticList;

    //
    // This field is used to indicate that Requests on this PDO could be
    // forwarded to the parent.
    //
    BOOLEAN m_AllowForwardRequestToParent;

protected:
    //
    // Pointer to a null terminated string which is the device ID.  This is
    // not a pointer that can be freed.   m_IDsAllocation is the beginning of
    // the allocation that can be freed.
    //
    PWSTR m_DeviceID;

    //
    // Pointer to a null terminated string which is the instance ID.  This is
    // not a pointer that can be freed.   m_IDsAllocation is the beginning of
    // the allocation that can be freed.
    //
    PWSTR m_InstanceID;

    //
    // Pointer to a multi sz which is the hardware IDs.  This is
    // not a pointer that can be freed.   m_IDsAllocation is the beginning of
    // the allocation that can be freed.
    //
    PWSTR m_HardwareIDs;

    //
    // Pointer to a multi sz which is the compatible IDs.  This is
    // not a pointer that can be freed.   m_IDsAllocation is the beginning of
    // the allocation that can be freed.
    //
    PWSTR m_CompatibleIDs;

    //
    // Pointer to a null terminated string which is the unique ID.  This is
    // not a pointer that can be freed.   m_IDsAllocation is the beginning of
    // the allocation that can be freed.
    //
    PWSTR m_ContainerID;

    //
    // Single allocation for all static ID strings (device, instance, hw, compat)
    // for the device
    //
    PWSTR m_IDsAllocation;

    FxRelatedDeviceList* m_EjectionDeviceList;

    //
    // IRP_MN_EJECT needs to be handled synchronously because PnP manager does
    // not serialize it with other state changing PnP irps if not handled
    // synchronously. This event is used to signal the completion of
    // IRP_MN_EJECT processing.
    //
    MxEvent* m_DeviceEjectProcessed;

    BOOLEAN m_CanBeDeleted;

    //
    // Parameter to track whether the EvtDeviceEnableWakeAtBus callback was
    // invoked and to determine whether the EvtDeviceDisableWakeAtBus callback
    // should be invoked or not.  The EnableWakeAtBus callback may not get
    // invoked if an upper driver in the stack completed the wait wake irp
    // instead of passing it down the stack and the power policy owner is
    // a PDO.
    //
    // This parameter can be referenced by either the power state machine or
    // the power policy state machine when a PDO is the power policy owner
    // but there is no locking mechanism necessary to synchronize access to
    // the field.  This is because the arrival of a Dx irp will move the power
    // state machine to a state where it can no longer affect the value of
    // this field and hence provides an implicit guard that allows the power
    // policy state machine to use this field while processing the Dx irp.
    //
    BOOLEAN m_EnableWakeAtBusInvoked;

private:
    //
    // Table of internal methods to handle PnP minor function codes.
    //
    static const PFN_PNP_POWER_CALLBACK m_PdoPnpFunctionTable[IRP_MN_SURPRISE_REMOVAL + 1];

    //
    // Table of internal methods to handle power minor function codes.
    //
    static const PFN_PNP_POWER_CALLBACK  m_PdoPowerFunctionTable[IRP_MN_QUERY_POWER + 1];

public:

    FxPkgPdo(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device
        );

    ~FxPkgPdo();

    _Must_inspect_result_
    virtual
    NTSTATUS
    Initialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    virtual
    VOID
    FinishInitialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    VOID
    RegisterCallbacks(
        __in PWDF_PDO_EVENT_CALLBACKS DispatchTable
        );

    _Must_inspect_result_
    NTSTATUS
    AddEjectionDevice(
        __in MdDeviceObject DependentDevice
        );

    VOID
    RemoveEjectionDevice(
        __in MdDeviceObject DependentDevice
        );

    VOID
    ClearEjectionDevicesList(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    HandleQueryInterfaceForReenumerate(
        __in  FxIrp* Irp,
        __out PBOOLEAN CompleteRequest
        );

    __inline
    BOOLEAN
    IsForwardRequestToParentEnabled(
        VOID
        )
    {
        return m_AllowForwardRequestToParent;
    }


private:
    _Must_inspect_result_
    virtual
    NTSTATUS
    SendIrpSynchronously(
        __in FxIrp* Irp
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    FireAndForgetIrp(
        __inout FxIrp* Irp
        );

    //
    // The following are static member function.  The only reason
    // I've defined them as member functions at all is so they can see
    // the private data in this class.
    //

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpCompleteIrp(
        __in    FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryDeviceRelations(
        __in    FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpQueryDeviceRelations(
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryInterface(
        IN FxPkgPnp* This,
        IN FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryCapabilities(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpQueryCapabilities(
        __inout FxIrp *Irp
        );

    VOID
    HandleQueryCapabilities(
        __inout PDEVICE_CAPABILITIES ReportedCaps,
        __in    PDEVICE_CAPABILITIES ParentCaps
        );

    static
    VOID
    _QueryCapsWorkItem(
        __in MdDeviceObject DeviceObject,
        __in PVOID Context
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryResources(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpQueryResources(
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryResourceRequirements(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpQueryResourceRequirements(
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryDeviceText(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpFilterResourceRequirements(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpEject(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    virtual
    BOOLEAN
    PnpSendStartDeviceDownTheStackOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventEjectHardwareOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventCheckForDevicePresenceOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventPdoRemovedOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventFdoRemovedOverload(
        VOID
        );

    virtual
    VOID
    PnpEventSurpriseRemovePendingOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpGetPostRemoveState(
        VOID
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpSetLock(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryId(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryPnpDeviceState(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryBusInformation(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpSurpriseRemoval(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _DispatchPowerSequence(
        __inout FxPkgPnp* This,
        __in FxIrp *Irp
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _DispatchSetPower(
        __inout FxPkgPnp* This,
        __in FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    DispatchSystemSetPower(
        __in FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    DispatchDeviceSetPower(
        __in FxIrp *Irp
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _DispatchQueryPower(
        __inout FxPkgPnp* This,
        __in FxIrp *Irp
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    PowerCheckParentOverload(
        __in BOOLEAN* ParentOn
        );

    virtual
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceTypeOverload(
        VOID
        );

    virtual
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceTypeNPOverload(
        VOID
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    PowerEnableWakeAtBusOverload(
        VOID
        );

    virtual
    VOID
    PowerDisableWakeAtBusOverload(
        VOID
        );

    virtual
    VOID
    PowerParentPowerDereference(
        VOID
        );

    virtual
    VOID
    PowerReleasePendingDeviceIrp(
        __in BOOLEAN IrpMustBePresent = TRUE
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    ProcessRemoveDeviceOverload(
        __inout FxIrp* Irp
        );

    virtual
    VOID
    DeleteSymbolicLinkOverload(
        __in BOOLEAN GracefulRemove
        );

    virtual
    VOID
    QueryForReenumerationInterface(
        VOID
        )
    {
        //
        // As the PDO, we already have the interface built in
        //
        DO_NOTHING();
    }

    virtual
    VOID
    ReleaseReenumerationInterface(
        VOID
        )
    {
        //
        // As the PDO, we already have the interface built in
        //
        DO_NOTHING();
    }

    _Must_inspect_result_
    virtual
    NTSTATUS
    AskParentToRemoveAndReenumerate(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    QueryForPowerThread(
        VOID
        );

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPnp(
        VOID
        )
    {
        return m_PdoPnpFunctionTable;
    }

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPower(
        VOID
        )
    {
        return m_PdoPowerFunctionTable;
    }

    static
    VOID
    _RemoveAndReenumerateSelf(
        __in PVOID Context
        );

    VOID
    PowerNotifyParentChildWakeArmed(
        VOID
        );

    VOID
    PowerNotifyParentChildWakeDisarmed(
        VOID
        );
};

#endif // _FXPKGPDO_H
