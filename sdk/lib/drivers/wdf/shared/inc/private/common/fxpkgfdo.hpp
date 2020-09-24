/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgFdo.hpp

Abstract:

    This module implements the pnp package for Fdo objects.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXPKGFDO_H_
#define _FXPKGFDO_H_

struct FxStaticChildDescription {
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header;

    CfxDevice* Pdo;
};

class FxPkgFdo : public FxPkgPnp
{
public:
    //
    // Pnp Properties
    //
    FxChildList* m_DefaultDeviceList;

    FxChildList* m_StaticDeviceList;

    //
    // Default target to the attached device in the stack
    //
    FxIoTarget* m_DefaultTarget;

    //
    // A virtual target to yourself. This allows a cx to send
    // the client. It may also be used by the client to send IO to itself.
    //
    FxIoTargetSelf* m_SelfTarget;

private:

    REENUMERATE_SELF_INTERFACE_STANDARD m_SurpriseRemoveAndReenumerateSelfInterface;

    //
    // The following structure contains the function pointer table
    // for the "events" this package can raise.
    //
    FxPnpDeviceFilterResourceRequirements m_DeviceFilterAddResourceRequirements;

    FxPnpDeviceFilterResourceRequirements m_DeviceFilterRemoveResourceRequirements;

    FxPnpDeviceRemoveAddedResources m_DeviceRemoveAddedResources;

    BOOLEAN m_Filter;

    //
    // Table of internal methods to handle PnP minor function codes.
    //
    static const PFN_PNP_POWER_CALLBACK m_FdoPnpFunctionTable[IRP_MN_SURPRISE_REMOVAL + 1];

    //
    // Table of internal methods to handle power minor function codes.
    //
    static const PFN_PNP_POWER_CALLBACK  m_FdoPowerFunctionTable[IRP_MN_QUERY_POWER + 1];

    FxPkgFdo(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device
        );

public:

    _Must_inspect_result_
    static
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS pGlobals,
        __in CfxDevice * Device,
        __deref_out FxPkgFdo ** PkgFdo
        );

    _Must_inspect_result_
    NTSTATUS
    RegisterCallbacks(
        __in PWDF_FDO_EVENT_CALLBACKS DispatchTable
        );

    _Must_inspect_result_
    NTSTATUS
    CreateDefaultDeviceList(
        __in PWDF_CHILD_LIST_CONFIG ListConfig,
        __in PWDF_OBJECT_ATTRIBUTES ListAttributes
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    Initialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    NTSTATUS
    PostCreateDeviceInitialize(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    SetFilter(
        __in BOOLEAN Value
        );

    BOOLEAN
    IsFilter(
        VOID
        )
    {
        return m_Filter;
    }

private:

    _Must_inspect_result_
    virtual
    NTSTATUS
    SendIrpSynchronously(
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    FireAndForgetIrp(
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpPassDown(
        __in FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpSurpriseRemoval(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryDeviceRelations(
        __inout FxPkgPnp* This,
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
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
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
        __inout FxIrp *Irp
        );

    VOID
    HandleQueryCapabilitiesCompletion(
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryCapabilitiesCompletionRoutine(
        __in    MdDeviceObject DeviceObject,
        __inout MdIrp Irp,
        __inout PVOID Context
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpFilterResourceRequirements(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpFilterResourceRequirements(
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
    _PnpQueryPnpDeviceStateCompletionRoutine(
        __in    MdDeviceObject DeviceObject,
        __inout MdIrp Irp,
        __inout PVOID Context
        );

    VOID
    HandleQueryPnpDeviceStateCompletion(
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
    PnpGetPostRemoveState(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventFdoRemovedOverload(
        VOID
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _PowerPassDown(
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
        __out BOOLEAN* ParentOn
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
    VOID
    PowerParentPowerDereference(
        VOID
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

    _Must_inspect_result_
    NTSTATUS
    DispatchSystemQueryPower(
        __in FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    DispatchDeviceQueryPower(
        __in FxIrp *Irp
        );

     _Must_inspect_result_
   NTSTATUS
    RaiseDevicePower(
        __in FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    LowerDevicePower(
        __in FxIrp *Irp
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
        );

    virtual
    VOID
    ReleaseReenumerationInterface(
        VOID
        );

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
        return m_FdoPnpFunctionTable;
    }

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPower(
        VOID
        )
    {
        return m_FdoPowerFunctionTable;
    }

    _Must_inspect_result_
    NTSTATUS
    QueryForDsfInterface(
        VOID
        );

public:

    static
    MdCompletionRoutineType
    RaiseDevicePowerCompletion;

protected:
    ~FxPkgFdo();

    static
    MdCompletionRoutineType
    _SystemPowerS0Completion;

    static
    MdCompletionRoutineType
    _SystemPowerSxCompletion;

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpFilteredStartDeviceCompletionRoutine(
        __in    MdDeviceObject DeviceObject,
        __inout MdIrp Irp,
        __inout PVOID Context
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpStartDeviceCompletionRoutine(
        __in    MdDeviceObject DeviceObject,
        __inout MdIrp Irp,
        __inout PVOID Context
        );
};

#endif // _FXPKGFDO_H
