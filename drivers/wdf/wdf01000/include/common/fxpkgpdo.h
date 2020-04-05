#ifndef _FXPKGPDO_H_
#define _FXPKGPDO_H_

#include "common/fxpkgpnp.h"
#include "common/fxirp.h"
#include "common/fxrelateddevicelist.h"


class FxPkgPdo : public FxPkgPnp {

public:

    FxPkgPdo(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device
        );

    ~FxPkgPdo();

    __inline
    BOOLEAN
    IsForwardRequestToParentEnabled(
        VOID
        )
    {
        return m_AllowForwardRequestToParent;
    }


    // added in 1.11
    //FxPnpDeviceReportedMissing           m_DeviceReportedMissing;

    BOOLEAN m_RawOK;

    BOOLEAN m_Static;

    BOOLEAN m_AddedToStaticList;
    
    //
    // This field is used to indicate that Requests on this PDO could be 
    // forwarded to the parent.
    //
    BOOLEAN m_AllowForwardRequestToParent;

    //
    // Properties used in handling IRP_MN_QUERY_DEVICE_TEXT
    //
    SINGLE_LIST_ENTRY m_DeviceTextHead;
    LCID m_DefaultLocale;

    FxDeviceDescriptionEntry* m_Description;

    FxChildList* m_OwningChildList;

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

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPnp(
        VOID
        )
    {
        //
        // TODO: remove when create function table
        //
        __debugbreak();
        return m_PdoPnpFunctionTable;
    }

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPower(
        VOID
        )
    {
        //
        // TODO: remove when create function table
        //
	__debugbreak();
        return m_PdoPowerFunctionTable;
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
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        RemoveWorkItemForSetDeviceFailed();
#else
        DO_NOTHING();
#endif
    }

    _Must_inspect_result_
    virtual
    NTSTATUS
    FireAndForgetIrp(
        __inout FxIrp* Irp
        );

    virtual
    VOID
    PowerReleasePendingDeviceIrp(
        __in BOOLEAN IrpMustBePresent = TRUE
        );

    virtual
    BOOLEAN
    PnpSendStartDeviceDownTheStackOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventCheckForDevicePresenceOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventEjectHardwareOverload(
        VOID
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpGetPostRemoveState(
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
    NTSTATUS
    QueryForReenumerationInterface(
        VOID
        )
    {
        //
        // As the PDO, we already have the interface built in
        //
        NTSTATUS status;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        status = AllocateWorkItemForSetDeviceFailed();
#else
        DO_NOTHING();
        status = STATUS_SUCCESS;
#endif

        return status;
    }

    _Must_inspect_result_
    virtual
    NTSTATUS
    QueryForPowerThread(
        VOID
        );

    virtual
    VOID
    PowerParentPowerDereference(
        VOID
        );

    virtual
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceTypeOverload(
        VOID
        );
};

typedef NTSTATUS (FxPkgPdo::*PFN_PDO_HANDLER)(FxIrp *Irp);

#endif //_FXPKGPDO_H_
