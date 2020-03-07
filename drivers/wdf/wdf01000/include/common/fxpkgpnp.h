#ifndef _FXPKGPNP_H_
#define _FXPKGPNP_H_

#include "common/fxpackage.h"
#include "common/mxevent.h"
#include "common/fxpnpstatemachine.h"
#include "common/fxpnpcallbacks.h"
#include "common/fxpowerstatemachine.h"
#include "common/fxpowerpolicystatemachine.h"
#include "common/fxsystemthread.h"
#include "common/fxsystemworkitem.h"
#include "common/fxwaitlock.h"
#include "common/fxtransactionedlist.h"
#include "common/fxchildlist.h"
#include "common/fxresource.h"
#include "common/fxselfmanagediostatemachine.h"
#include "common/fxrelateddevicelist.h"



//
// Information shared between the power and power policy state machines.
//
struct SharedPowerData {
    //
    // Current wait wake irp in this device object.  Access to this field is
    // determined by m_WaitWakeOwner.
    //
    // m_WaitWakeOwner == TRUE, access is guarded by
    // FxPowerMachine::m_WaitWakeLock
    //
    //
    // m_WaitWakeOwner == FALSE, access is guarded InterlockedExchange operations
    // and the ability to cancel the request is guarded through
    // FxPowerPolicyMachine::m_WaitWakeCancelCompletionOwnership
    //
    // Any devobj can be both the power policy owner and the wait wake owner,
    // but usually this dual role would only be for raw PDOs.
    //
    MdIrp m_WaitWakeIrp;

    //
    // Indication whether this power machine owns wait wake irps (and calls
    // (dis)arm at bus level callbacks and handles cancellation logic.
    //
    BOOLEAN m_WaitWakeOwner;

    //
    // If TRUE  the watchdog timer should be extended to a very long period of
    // time during debugging for a NP power operation.
    //
    // NOTE:  nowhere in the code do we set this value to TRUE.  The debugger
    //        extension !wdfextendwatchdog will set it to TRUE, so the field must
    //        stay.  If moved, the extension must obviously be updated as well.
    //
    BOOLEAN m_ExtendWatchDogTimer;
};

//
// Bit-flags for tracking which callback is currently executing.
//
enum FxDeviceCallbackFlags {
    // Prepare hardware callback is running.
    FXDEVICE_CALLBACK_IN_PREPARE_HARDWARE = 0x01,
};

typedef
_Must_inspect_result_
NTSTATUS
(*PFN_PNP_POWER_CALLBACK)(
     __inout FxPkgPnp* This,
     __inout FxIrp* Irp
     );

//
// The naming of these values is very important.   The following macros rely on
// it:
//
// (state related:)
// SET_PNP_DEVICE_STATE_BIT
// SET_TRI_STATE_FROM_STATE_BITS
// GET_PNP_STATE_BITS_FROM_STRUCT
//
// (caps related:)
// GET_PNP_CAP_BITS_FROM_STRUCT
// SET_PNP_CAP_IF_TRUE
// SET_PNP_CAP_IF_FALSE
// SET_PNP_CAP
//
// They using the naming convention to generically  map the field name in
// WDF_DEVICE_PNP_CAPABILITIES and WDF_DEVICE_STATE to the appropriate bit
// values.
//
enum FxPnpStateAndCapValues {
    FxPnpStateDisabledFalse             = 0x00000000,
    FxPnpStateDisabledTrue              = 0x00000001,
    FxPnpStateDisabledUseDefault        = 0x00000002,
    FxPnpStateDisabledMask              = 0x00000003,

    FxPnpStateDontDisplayInUIFalse      = 0x00000000,
    FxPnpStateDontDisplayInUITrue       = 0x00000004,
    FxPnpStateDontDisplayInUIUseDefault = 0x00000008,
    FxPnpStateDontDisplayInUIMask       = 0x0000000C,

    FxPnpStateFailedFalse               = 0x00000000,
    FxPnpStateFailedTrue                = 0x00000010,
    FxPnpStateFailedUseDefault          = 0x00000020,
    FxPnpStateFailedMask                = 0x00000030,

    FxPnpStateNotDisableableFalse       = 0x00000000,
    FxPnpStateNotDisableableTrue        = 0x00000040,
    FxPnpStateNotDisableableUseDefault  = 0x00000080,
    FxPnpStateNotDisableableMask        = 0x000000C0,

    FxPnpStateRemovedFalse              = 0x00000000,
    FxPnpStateRemovedTrue               = 0x00000100,
    FxPnpStateRemovedUseDefault         = 0x00000200,
    FxPnpStateRemovedMask               = 0x00000300,

    FxPnpStateResourcesChangedFalse     = 0x00000000,
    FxPnpStateResourcesChangedTrue      = 0x00000400,
    FxPnpStateResourcesChangedUseDefault= 0x00000800,
    FxPnpStateResourcesChangedMask      = 0x00000C00,

    FxPnpStateMask                      = 0x00000FFF,

    FxPnpCapLockSupportedFalse          = 0x00000000,
    FxPnpCapLockSupportedTrue           = 0x00001000,
    FxPnpCapLockSupportedUseDefault     = 0x00002000,
    FxPnpCapLockSupportedMask           = 0x00003000,

    FxPnpCapEjectSupportedFalse         = 0x00000000,
    FxPnpCapEjectSupportedTrue          = 0x00004000,
    FxPnpCapEjectSupportedUseDefault    = 0x00008000,
    FxPnpCapEjectSupportedMask          = 0x0000C000,

    FxPnpCapRemovableFalse              = 0x00000000,
    FxPnpCapRemovableTrue               = 0x00010000,
    FxPnpCapRemovableUseDefault         = 0x00020000,
    FxPnpCapRemovableMask               = 0x00030000,

    FxPnpCapDockDeviceFalse             = 0x00000000,
    FxPnpCapDockDeviceTrue              = 0x00040000,
    FxPnpCapDockDeviceUseDefault        = 0x00080000,
    FxPnpCapDockDeviceMask              = 0x000C0000,

    FxPnpCapUniqueIDFalse               = 0x00000000,
    FxPnpCapUniqueIDTrue                = 0x00100000,
    FxPnpCapUniqueIDUseDefault          = 0x00200000,
    FxPnpCapUniqueIDMask                = 0x00300000,

    FxPnpCapSilentInstallFalse          = 0x00000000,
    FxPnpCapSilentInstallTrue           = 0x00400000,
    FxPnpCapSilentInstallUseDefault     = 0x00800000,
    FxPnpCapSilentInstallMask           = 0x00C00000,

    FxPnpCapSurpriseRemovalOKFalse      = 0x00000000,
    FxPnpCapSurpriseRemovalOKTrue       = 0x01000000,
    FxPnpCapSurpriseRemovalOKUseDefault = 0x02000000,
    FxPnpCapSurpriseRemovalOKMask       = 0x03000000,

    FxPnpCapHardwareDisabledFalse       = 0x00000000,
    FxPnpCapHardwareDisabledTrue        = 0x04000000,
    FxPnpCapHardwareDisabledUseDefault  = 0x08000000,
    FxPnpCapHardwareDisabledMask        = 0x0C000000,

    FxPnpCapNoDisplayInUIFalse          = 0x00000000,
    FxPnpCapNoDisplayInUITrue           = 0x10000000,
    FxPnpCapNoDisplayInUIUseDefault     = 0x20000000,
    FxPnpCapNoDisplayInUIMask           = 0x30000000,

    FxPnpCapMask                        = 0x3FFFF000,
};

union FxPnpStateAndCaps {
    struct {
        // States
        WDF_TRI_STATE Disabled : 2;
        WDF_TRI_STATE DontDisplayInUI : 2;
        WDF_TRI_STATE Failed : 2;
        WDF_TRI_STATE NotDisableable : 2;
        WDF_TRI_STATE Removed : 2;
        WDF_TRI_STATE ResourcesChanged : 2;

        // Caps
        WDF_TRI_STATE LockSupported : 2;
        WDF_TRI_STATE EjectSupported : 2;
        WDF_TRI_STATE Removable : 2;
        WDF_TRI_STATE DockDevice : 2;
        WDF_TRI_STATE UniqueID : 2;
        WDF_TRI_STATE SilentInstall : 2;
        WDF_TRI_STATE SurpriseRemovalOK : 2;
        WDF_TRI_STATE HardwareDisabled : 2;
        WDF_TRI_STATE NoDisplayInUI : 2;
    } ByEnum;

    //
    // The bottom 3 nibbles (0xFFF) are the pnp state tri state values encoded
    // down to 2 bits each.
    //
    // The remaining portion (0x3FFFF000) are the pnp caps tri state values
    // encoded down to 2 bits each as well.
    //
    LONG Value;
};

//
// The naming of these values is very important.   The following macros rely on it:
// GET_POWER_CAP_BITS_FROM_STRUCT
// SET_POWER_CAP
//
// They using the naming convention to generically  map the field name in
// WDF_DEVICE_POWER_CAPABILITIES to the appropriate bit values.
//
enum FxPowerCapValues {
    FxPowerCapDeviceD1False       = 0x0000,
    FxPowerCapDeviceD1True        = 0x0001,
    FxPowerCapDeviceD1UseDefault  = 0x0002,
    FxPowerCapDeviceD1Mask        = 0x0003,

    FxPowerCapDeviceD2False       = 0x0000,
    FxPowerCapDeviceD2True        = 0x0004,
    FxPowerCapDeviceD2UseDefault  = 0x0008,
    FxPowerCapDeviceD2Mask        = 0x000C,

    FxPowerCapWakeFromD0False     = 0x0000,
    FxPowerCapWakeFromD0True      = 0x0010,
    FxPowerCapWakeFromD0UseDefault= 0x0020,
    FxPowerCapWakeFromD0Mask      = 0x0030,

    FxPowerCapWakeFromD1False     = 0x0000,
    FxPowerCapWakeFromD1True      = 0x0040,
    FxPowerCapWakeFromD1UseDefault= 0x0080,
    FxPowerCapWakeFromD1Mask      = 0x00C0,

    FxPowerCapWakeFromD2False     = 0x0000,
    FxPowerCapWakeFromD2True      = 0x0100,
    FxPowerCapWakeFromD2UseDefault= 0x0200,
    FxPowerCapWakeFromD2Mask      = 0x0300,

    FxPowerCapWakeFromD3False     = 0x0000,
    FxPowerCapWakeFromD3True      = 0x0400,
    FxPowerCapWakeFromD3UseDefault= 0x0800,
    FxPowerCapWakeFromD3Mask      = 0x0C00,
};

struct FxPowerCaps {
    //
    // Encoded with FxPowerCapValues, which encodes the WDF_TRI_STATE values
    //
    USHORT Caps;

    //
    // Default value PowerDeviceMaximum, PowerSystemMaximum indicates not to
    // set this value.
    //
    BYTE DeviceWake; // DEVICE_POWER_STATE
    BYTE SystemWake; // SYSTEM_POWER_STATE

    //
    // Each state is encoded in a nibble in a byte
    //
    ULONG States;

    //
    // Default values of -1 indicate not to set this value
    //
    ULONG D1Latency;
    ULONG D2Latency;
    ULONG D3Latency;
};

typedef
VOID
(*PFN_POWER_THREAD_ENQUEUE)(
     __in PVOID Context,
     __in PWORK_QUEUE_ITEM WorkItem
     );
     
//
// workaround overloaded definition (rpc generated headers all define INTERFACE
// to match the class name).
//
#undef INTERFACE

typedef struct _POWER_THREAD_INTERFACE {
    //
    // generic interface header
    //
    INTERFACE Interface;

    PFN_POWER_THREAD_ENQUEUE PowerThreadEnqueue;

} POWER_THREAD_INTERFACE, *PPOWER_THREAD_INTERFACE;


struct FxEnumerationInfo : public FxStump {

    FxEnumerationInfo(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) : m_ChildListList(FxDriverGlobals)
    {
    }

    NTSTATUS
    Initialize(
        VOID
        )
    {
        NTSTATUS status;
        
        status = m_PowerStateLock.Initialize();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = m_ChildListList.Initialize();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        return STATUS_SUCCESS;
    }

    _Acquires_lock_(_Global_critical_region_)
    VOID
    AcquireParentPowerStateLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        (VOID) m_PowerStateLock.AcquireLock(FxDriverGlobals);
    }

    _Releases_lock_(_Global_critical_region_)
    VOID
    ReleaseParentPowerStateLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        m_PowerStateLock.ReleaseLock(FxDriverGlobals);
    }


    FxWaitLockInternal m_PowerStateLock;

    //
    // List of FxChildList objects which contain enumerated children
    //
    FxWaitLockTransactionedList m_ChildListList;
};

class FxPkgPnp : public FxPackage {

    friend FxPowerPolicyOwnerSettings;

protected:

    FxPkgPnp(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice* Device,
        __in WDFTYPE Type
        );

    ~FxPkgPnp();

    _Must_inspect_result_
    virtual
    NTSTATUS
    Dispatch(
        __in MdIrp Irp
        );

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPnp(
        VOID
        ) =0;

    virtual
    const PFN_PNP_POWER_CALLBACK*
    GetDispatchPower(
        VOID
        ) =0;

    MdIrp
    ClearPendingPnpIrp(
        VOID
        )
    {
        MdIrp irp;
    
        irp = m_PendingPnPIrp;
        m_PendingPnPIrp = NULL;
    
        return irp;
    }

    VOID
    PnpFinishProcessingIrp(
        __in BOOLEAN IrpMustBePresent = TRUE
        );

    VOID
    PnpEnterNewState(
        __in WDF_DEVICE_PNP_STATE State
        );

    BOOLEAN
    IsPresentPendingPnpIrp(
        VOID
        )
    {
        return (m_PendingPnPIrp != NULL) ? TRUE : FALSE;
    }

    MdIrp
    GetPendingPnpIrp(
        VOID
        )
    {
        return m_PendingPnPIrp;
    }

    VOID
    SetPendingDevicePowerIrp(
        __inout FxIrp* Irp
        )
    {
        ASSERT(m_PendingDevicePowerIrp == NULL);

        Irp->MarkIrpPending();
        m_PendingDevicePowerIrp = Irp->GetIrp();

        if (Irp->GetParameterPowerStateDeviceState() > PowerDeviceD0)
        {
            //
            // We are powering down, capture the current power action.  We will
            // reset it to PowerActionNone once we have powered up.
            //
            m_SystemPowerAction = (UCHAR) Irp->GetParameterPowerShutdownType();
        }
    }

    NTSTATUS
    CompletePowerRequest(
        __inout FxIrp* Irp,
        __in    NTSTATUS Status
        );

    NTSTATUS
    CompletePnpRequest(
        __inout FxIrp* Irp,
        __in    NTSTATUS Status
        );

    static
    VOID
    _SetPowerCapState(
        __in  ULONG Index,
        __in  DEVICE_POWER_STATE State,
        __out PULONG Result
        );

    static
    VOID
    _PowerProcessEventInner(
        __in FxPkgPnp* This,
        __in FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    VOID
    PowerProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    static
    VOID
    _PowerPolicyProcessEventInner(
        __inout FxPkgPnp* This,
        __inout FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    VOID
    PowerPolicyProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    static
    CPPOWER_STATE_TABLE
    GetPowerTableEntry(
        __in WDF_DEVICE_POWER_STATE State
        )
    {
        return &m_WdfPowerStates[WdfDevStateNormalize(State) - WdfDevStatePowerObjectCreated];
    }

    VOID
    PowerCompletePendedWakeIrp(
        VOID
        );

    virtual
    VOID
    PowerReleasePendingDeviceIrp(
        BOOLEAN IrpMustBePresent = TRUE
        ) =0;

    VOID
    PowerEnterNewState(
        __in WDF_DEVICE_POWER_STATE State
        );

    VOID
    SaveState(
        __in BOOLEAN UseCanSaveState
        );

    static
    CPPOWER_POLICY_STATE_TABLE
    GetPowerPolicyTableEntry(
        __in WDF_DEVICE_POWER_POLICY_STATE State
        )
    {
        return &m_WdfPowerPolicyStates[WdfDevStateNormalize(State) - WdfDevStatePwrPolObjectCreated];
    }

    VOID
    PowerPolicyCompleteSystemPowerIrp(
        VOID
        );

    VOID
    PowerPolicyEnterNewState(
        __in WDF_DEVICE_POWER_POLICY_STATE State
        );

    VOID
    NotPowerPolicyOwnerEnterNewState(
        __in WDF_DEVICE_POWER_POLICY_STATE NewState
        );

    _Must_inspect_result_
    static
    CPNOT_POWER_POLICY_OWNER_STATE_TABLE
    GetNotPowerPolicyOwnerTableEntry(
        __in WDF_DEVICE_POWER_POLICY_STATE State
        )
    {
        ULONG i;

        for (i = 0;
             m_WdfNotPowerPolicyOwnerStates[i].CurrentTargetState != WdfDevStatePwrPolNull;
             i++)
        {
            if (m_WdfNotPowerPolicyOwnerStates[i].CurrentTargetState == State)
            {
                return &m_WdfNotPowerPolicyOwnerStates[i];
            }
        }

        return NULL;
    }

    BOOLEAN
    IsUsageSupported(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Usage
        )
    {
        return m_SpecialSupport[((ULONG)Usage)-1];
    }

    BOOLEAN
    IsDevicePowerUpIrpPending(
        VOID
        )
    {
        DEVICE_POWER_STATE state;
        FxIrp irp(m_PendingDevicePowerIrp);
        
        if (irp.GetIrp() == NULL) {
            return FALSE;
        }

        state = irp.GetParameterPowerStateDeviceState();

        return (state == PowerDeviceD0 ? TRUE : FALSE);
    }

    VOID
    WriteStateToRegistry(
        __in HANDLE RegKey,
        __in PUNICODE_STRING ValueName,
        __in ULONG Value
        );

    MdIrp
    ClearPendingDevicePowerIrp(
        VOID
        )
    {
        MdIrp irp;

        irp = m_PendingDevicePowerIrp;
        m_PendingDevicePowerIrp = NULL;

        return irp;
    }

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID
    SleepStudyStop(
        VOID
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpStartDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryStopDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpCancelStopDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpStopDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpQueryRemoveDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpCancelRemoveDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpRemoveDevice(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpSurpriseRemoval(
        __inout FxIrp* Irp
        );

    NTSTATUS
    FilterResourceRequirements(
        __in IO_RESOURCE_REQUIREMENTS_LIST **IoList
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpDeviceUsageNotification(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _DispatchWaitWake(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    VOID
    SetPendingSystemPowerIrp(
        __inout FxIrp* Irp
        )
    {
        ASSERT(m_PendingSystemPowerIrp == NULL);
        Irp->MarkIrpPending();
        m_PendingSystemPowerIrp = Irp->GetIrp();
    }

    MdIrp
    GetPendingSystemPowerIrp(
        VOID
        )
    {
        return m_PendingSystemPowerIrp;
    }

    VOID
    SetPendingPnpIrp(
        __inout FxIrp* Irp,
        __in    BOOLEAN MarkIrpPending = TRUE
        );

// begin pnp state machine table based callbacks
    static
    WDF_DEVICE_PNP_STATE
    PnpEventCheckForDevicePresence(
        __inout FxPkgPnp* This
        );

    virtual
    BOOLEAN
    PnpSendStartDeviceDownTheStackOverload(
        VOID
        ) =0;

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventCheckForDevicePresenceOverload(
        VOID
        ) = 0;

    static
    WDF_DEVICE_PNP_STATE
    PnpEventEjectHardware(
        __inout FxPkgPnp* This
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventEjectHardwareOverload(
        VOID
        ) = 0;

    static
    WDF_DEVICE_PNP_STATE
    PnpEventInitStarting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventInitSurpriseRemoved(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventHardwareAvailable(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventEnableInterfaces(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventHardwareAvailablePowerPolicyFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryRemoveAskDriver(
        __inout FxPkgPnp* This
        );

    static 
    WDF_DEVICE_PNP_STATE 
    PnpEventQueryRemoveEnsureDeviceAwake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryRemovePending(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryRemoveStaticCheck(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueriedRemoving(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryStopAskDriver(
        __inout FxPkgPnp* This
        );

    static 
    WDF_DEVICE_PNP_STATE 
    PnpEventQueryStopEnsureDeviceAwake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryStopPending(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryStopStaticCheck(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueryCanceled(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRemoved(
        __inout FxPkgPnp* This
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpGetPostRemoveState(
        VOID
        ) =0;

    static
    WDF_DEVICE_PNP_STATE
    PnpEventPdoRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRemovedPdoWait(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRemovedPdoSurpriseRemoved(
        __inout FxPkgPnp* This
        );

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventPdoRemovedOverload(
        VOID
        ) =0;

    virtual
    WDF_DEVICE_PNP_STATE
    PnpEventFdoRemovedOverload(
        VOID
        ) =0;

    virtual
    VOID
    PnpEventSurpriseRemovePendingOverload(
        VOID
        );

    VOID
    PnpEventRemovedCommonCode(
        VOID
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRemovingDisableInterfaces(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRestarting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStarted(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStartedCancelStop(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStartedCancelRemove(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStartedRemoving(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStartingFromStopped(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStopped(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStoppedWaitForStartCompletion(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventStartedStopping(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventSurpriseRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventInitQueryRemove(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventInitQueryRemoveCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFdoRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventQueriedSurpriseRemove(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventSurpriseRemoveIoStarted(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedPowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedIoStarting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedOwnHardware(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedPowerPolicyRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedSurpriseRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedStarted(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFailedInit(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventPdoInitFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRestart(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRestartReleaseHardware(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRestartHardwareAvailable(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventPdoRestart(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventRemovedChildrenRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_PNP_STATE
    PnpEventFinal(
        __inout FxPkgPnp* This
        );
// end pnp state machine table based callbacks

public:

    VOID
    PowerProcessEvent(
        __in FxPowerEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    VOID
    CleanupDeviceFromFailedCreate(
        __in MxEvent * WaitEvent
        );

    VOID
    CleanupStateMachines(
        __in BOOLEAN ClenaupPnp
        );

    VOID
    PnpProcessEvent(
        __in FxPnpEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    BOOLEAN
    ShouldProcessPnpEventOnDifferentThread(
        __in KIRQL CurrentIrql,
        __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
        );

    VOID
    SignalDeviceRemovedEvent(
        VOID
        )
    {
        m_DeviceRemoveProcessed->Set();
    }

    virtual
    NTSTATUS
    FireAndForgetIrp(
        FxIrp* Irp
        ) =0;

    VOID
    ProcessDelayedDeletion(
        VOID
        );

    VOID
    DeleteDevice(
        VOID
        );

    BOOLEAN
    IsPowerPolicyOwner(
        VOID
        )
    {
        return m_PowerPolicyMachine.m_Owner != NULL ? TRUE : FALSE;
    }

    virtual
    VOID
    FinishInitialize(
        __inout PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    Initialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateEnumInfo(
        VOID
        );

    VOID
    AddChildList(
        __in FxChildList* List
        );

    VOID
    QueryForD3ColdInterface(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    RegisterCallbacks(
        __in PWDF_PNPPOWER_EVENT_CALLBACKS DispatchTable
        );

    VOID
    RegisterPowerPolicyCallbacks(
        __in PWDF_POWER_POLICY_EVENT_CALLBACKS Callbacks
        );

    _Must_inspect_result_
    NTSTATUS
    PostCreateDeviceInitialize(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    __inline
    PowerReference(
        __in BOOLEAN WaitForD0,
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PCSTR File = NULL
        )
    {
        return m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.PowerReference(WaitForD0, Tag, Line, File);
    }

    VOID
    __inline
    PowerDereference(
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PCSTR File = NULL
        )
    {
        m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.IoDecrement(Tag, Line, File);
    }

    VOID
    PowerPolicyProcessEvent(
        __in FxPowerPolicyEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    BOOLEAN
    ShouldProcessPowerPolicyEventOnDifferentThread(
        __in KIRQL CurrentIrql,
        __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
        );

    BOOLEAN
    HasPowerThread(
        VOID
        )
    {
        return m_HasPowerThread;
    }

    VOID
    QueueToPowerThread(
        __in PWORK_QUEUE_ITEM WorkItem
        )
    {
        m_PowerThreadInterface.PowerThreadEnqueue(
            m_PowerThreadInterface.Interface.Context,
            WorkItem
            );
    }


private:

    //
    // The current pnp state changing irp in the stack that we have pended to
    // process in the pnp state machine
    //
    MdIrp m_PendingPnPIrp;

        //
    // Non NULL when this device is exporting the power thread interface.  This
    // would be the lowest device in the stack that supports this interface.
    //
    FxSystemThread* m_PowerThread;

    LONG m_PowerThreadInterfaceReferenceCount;

    FxCREvent* m_PowerThreadEvent;

    //
    // The current system power irp in the stack that we have pended to process
    // in the power state machine
    //
    MdIrp m_PendingSystemPowerIrp;

    //
    // The current device power irp in the stack that we have pended to process
    // in the power state machine
    //
    MdIrp m_PendingDevicePowerIrp;

    FxPnpStateCallback* m_PnpStateCallbacks;

    FxPowerStateCallback* m_PowerStateCallbacks;

    FxPowerPolicyStateCallback* m_PowerPolicyStateCallbacks;

    static const PNP_STATE_TABLE          m_WdfPnpStates[];
    static const POWER_STATE_TABLE        m_WdfPowerStates[];
    static const POWER_POLICY_STATE_TABLE m_WdfPowerPolicyStates[];
    static const NOT_POWER_POLICY_OWNER_STATE_TABLE m_WdfNotPowerPolicyOwnerStates[];

    ULONG  m_InterruptObjectCount;

    LIST_ENTRY  m_InterruptListHead;
    
    //
    // Number of interrupts that are declared to be capable
    // of waking from low power
    //
    ULONG m_WakeInterruptCount;
    
    //
    // Count that keeps track of the number of wake interrupt
    // machines that have acknowledged back an event queued 
    // in to them by the device level PnP/Power code
    //
    ULONG m_WakeInterruptPendingAckCount;  

    //
    // Keeps track of whether the last system wake was due to
    // a wake interrupt, so that we can report this device as
    // the source of wake to the power manager
    //
    BOOLEAN m_SystemWokenByWakeInterrupt;

    //
    // If TRUE, do not disconnect wake interrupts even if there is no
    // pended IRP_MN_WAIT_WAKE. This works around a race condition between
    // the wake interrupt firing (and the wake ISR running) and the device
    // powering down. This flag is set when we are in a wake-enabled device
    // powering down path and is cleared when the device is powered up again.
    //
    BOOLEAN m_WakeInterruptsKeepConnected;

    //
    // If TRUE, the PNP State has reached PnpEventStarted at least once.
    //
    BOOLEAN m_AchievedStart;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // Sleep Study struct used to track session - this is all the data that can be
    // allocated dynamically.
    // 
    //PSLEEP_STUDY_INTERFACE m_SleepStudy;
    
    //
    // Count of driver requested power references
    //
    volatile LONG m_SleepStudyPowerRefIoCount;

    //
    // Flag to indicate if m_SleepStudyPowerRefIoCount should be used to track 
    // power references
    //
    BOOLEAN m_SleepStudyTrackReferences;

    static
    VOID
    _WorkItemSetDeviceFailedAttemptRestart(
        _In_ PVOID Parameter
        );

    static
    VOID
    _WorkItemSetDeviceFailedRestartAlways(
        _In_ PVOID Parameter
        );
#endif

private:

    VOID
    ReleasePowerThread(
        VOID
        );

    virtual
    VOID
    ReleaseReenumerationInterface(
        VOID
        ) =0;

    static
    VOID
    _PowerThreadEnqueue(
        __in PVOID Context,
        __in PWORK_QUEUE_ITEM WorkItem
        )
    {
        BOOLEAN result;

        result = ((FxPkgPnp*) Context)->m_PowerThread->QueueWorkItem(WorkItem);
#if DBG
        ASSERT(result);
#else
        UNREFERENCED_PARAMETER(result);
#endif
    }

    static
    VOID
    _PowerThreadInterfaceReference(
        __inout PVOID Context
        );

    static
    VOID
    _PowerThreadInterfaceDereference(
        __inout PVOID Context
        );

    

//
// Start of members
//
public:

    FxPnpStateAndCaps m_PnpStateAndCaps;

    ULONG m_PnpCapsAddress;
    ULONG m_PnpCapsUINumber;

    FxPowerCaps m_PowerCaps;

    BOOLEAN m_Failed;

    //
    // Track the current device and system power states.
    //

    // SYSTEM_POWER_STATE
    BYTE m_SystemPowerState;

    // WDF_POWER_DEVICE_STATE
    BYTE m_DevicePowerState;

    // WDF_POWER_DEVICE_STATE
    BYTE m_DevicePowerStateOld;

    //
    // All 3 state machine engines
    //
    FxPnpMachine m_PnpMachine;
    FxPowerMachine m_PowerMachine;
    FxPowerPolicyMachine m_PowerPolicyMachine;

    FxSelfManagedIoMachine* m_SelfManagedIoMachine;

    //
    // Collection of FxQueryInterface objects
    //
    FxWaitLockInternal m_QueryInterfaceLock;

    SINGLE_LIST_ENTRY m_QueryInterfaceHead;

    FxWaitLockInternal m_DeviceInterfaceLock;

    SINGLE_LIST_ENTRY m_DeviceInterfaceHead;

    BOOLEAN m_DeviceInterfacesCanBeEnabled;

    //Start from Windows 8
    //D3COLD_SUPPORT_INTERFACE m_D3ColdInterface;


    FxPnpDeviceUsageNotification        m_DeviceUsageNotification;
    //FxPnpDeviceUsageNotificationEx      m_DeviceUsageNotificationEx;
    FxPnpDeviceRelationsQuery           m_DeviceRelationsQuery;

    FxPnpDeviceD0Entry                      m_DeviceD0Entry;
    FxPnpDeviceD0EntryPostInterruptsEnabled m_DeviceD0EntryPostInterruptsEnabled;
    FxPnpDeviceD0ExitPreInterruptsDisabled  m_DeviceD0ExitPreInterruptsDisabled;
    FxPnpDeviceD0Exit                       m_DeviceD0Exit;

    FxPnpDevicePrepareHardware          m_DevicePrepareHardware;
    FxPnpDeviceReleaseHardware          m_DeviceReleaseHardware;

    FxPnpDeviceQueryStop                m_DeviceQueryStop;
    FxPnpDeviceQueryRemove              m_DeviceQueryRemove;
    FxPnpDeviceSurpriseRemoval          m_DeviceSurpriseRemoval;

    //
    // Data shared between the power and power policy machines determining how
    // we handle wait wake irps.
    //
    SharedPowerData m_SharedPower;

    //
    // Indicate the types of special files which are supported.
    //
    BOOLEAN m_SpecialSupport[WdfSpecialFileMax-1];

    //
    // Track the number of special file notifications
    // (ie. paging file, crash dump file, and hibernate file).
    //
    LONG m_SpecialFileCount[WdfSpecialFileMax-1];

    //
    // ULONG and not a BOOLEAN so the driver can match nest calls to
    // WdfDeviceSetStaticStopRemove without having to track the count on their
    // own.
    //
    ULONG m_DeviceStopCount;

protected:

    //
    // if FALSE, there is no power thread available on the devnode currently
    // and a work item should be enqueued.  if TRUE, there is a power thread
    // and the callback should be enqueued to it.
    //
    BOOLEAN m_HasPowerThread;

    //
    // Event that is set when processing a remove device is complete
    //
    MxEvent* m_DeviceRemoveProcessed;

    //
    // Set the event which indicates that the pnp state machine is done outside
    // of the state machine so that we can drain any remaining state machine
    // events in the removing thread before signaling the event.
    //
    BYTE m_SetDeviceRemoveProcessed;

    //
    // Count of children we need to fully remove when the parent (this package)
    // is being removed.
    //
    LONG m_PendingChildCount;

    //
    // Number of times we have tried to enumerate children but failed
    //
    UCHAR m_BusEnumRetries;

    //
    // Bus information for any enumerated children
    //
    PNP_BUS_INFORMATION m_BusInformation;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // Interrupt APIs for Vista and forward
    //
    //PFN_IO_CONNECT_INTERRUPT_EX     m_IoConnectInterruptEx;
    //PFN_IO_DISCONNECT_INTERRUPT_EX  m_IoDisconnectInterruptEx;
    //
    // Interrupt APIs for Windows 8 and forward
    //
    //PFN_IO_REPORT_INTERRUPT_ACTIVE     m_IoReportInterruptActive;
    //PFN_IO_REPORT_INTERRUPT_INACTIVE   m_IoReportInterruptInactive;

    //
    // Workitem to invoke AskParentToRemoveAndReenumerate at PASSIVE_LEVEL
    //
    FxSystemWorkItem* m_SetDeviceFailedAttemptRestartWorkItem;

    //
    // If TRUE, we guarantee that in *all* cases that the ReleaseHardware 
    // callback for the current device is invoked only after all descendent 
    // devices have already been removed. We do this by ensuring that
    // ReleaseHardware is only ever invoked when there is a PNP IRP such as 
    // remove, surprise-remove or stop is pending in the device. PNP already 
    // ensures that it sends us that IRP only after all child devices have 
    // processed the corresponding IRP in their stacks.
    //
    // Even if FALSE, in *most* cases, the ReleaseHardware callback of the 
    // current device should still be invoked only after all descendent devices
    // have already been stopped/removed. However, in some failure paths we 
    // might invoke the ReleaseHardware callback of the current device before 
    // all descendent devices have been stopped. In these cases, we do not wait 
    // for the surprise-remove IRP sent as a result of the failure in order to 
    // invoke ReleaseHardware. Instead, we invoke it proactively.
    //
    // The default value is FALSE.
    //
    BOOLEAN m_ReleaseHardwareAfterDescendantsOnFailure;

    //
    // Interface to queue a work item to the devnode's power thread.  Any device
    // in the stack can export the power thread, but it must be the lowest
    // device in the stack capable of doing so.  This would always be a WDF
    // enumerated PDO, but could theoretically be any PDO.  If the PDO does
    // support this interface, WDF will try to export the interface in a filter
    // or FDO.
    //
    // This export is not publicly defined because this is an internal WDF
    // implementation detail where we want to use as few threads as possible,
    // but still guarantee that non power pagable devices can operate a passive
    // level and not be blocked by paging I/O (which eliminates using work items).
    //
    POWER_THREAD_INTERFACE m_PowerThreadInterface;

    FxEnumerationInfo* m_EnumInfo;

    //
    // Translated resources
    //
    FxCmResList* m_Resources;

    //
    // Raw resources
    //
    FxCmResList* m_ResourcesRaw;

    FxSpinLockTransactionedList* m_DmaEnablerList;

    //
    // List of dependent devices for usage notifications
    //
    FxRelatedDeviceList* m_UsageDependentDeviceList;

    FxRelatedDeviceList* m_RemovalDeviceList;

    //
    // TRUE once the entire stack has been queried for the caps
    //
    BOOLEAN m_CapsQueried;

    BOOLEAN m_InternalFailure;

    // WDF_DEVICE_FAILED_ACTION
    BYTE m_FailedAction;

    //
    // The power action corresponding to the system power transition
    //
    UCHAR m_SystemPowerAction;


    static const PNP_EVENT_TARGET_STATE m_PnpInitOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpInitStartingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpHardwareAvailableOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpQueryStopPendingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpRemovedPdoWaitOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpRestartingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpStartedOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpQueryRemovePendingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpQueriedRemovingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpInitQueryRemoveOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpStoppedOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpStoppedWaitForStartCompletionOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpStartedStoppingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpStartedStoppingFailedOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpEjectFailedOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpStartedRemovingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpFailedPowerDownOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpFailedIoStartingOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpFailedWaitForRemoveOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpRestartOtherStates[];
    static const PNP_EVENT_TARGET_STATE m_PnpRestartReleaseHardware[];
    static const PNP_EVENT_TARGET_STATE m_PnpRestartHardwareAvailableOtherStates[];


    NTSTATUS
    AllocateWorkItemForSetDeviceFailed(
        VOID
        );

    VOID
    RemoveWorkItemForSetDeviceFailed(
        VOID
        );
#endif

    static
    VOID
    _PnpProcessEventInner(
        __inout FxPkgPnp* This,
        __inout FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    VOID
    PnpProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    static
    CPPNP_STATE_TABLE
    GetPnpTableEntry(
        __in WDF_DEVICE_PNP_STATE State
        )
    {
        return &m_WdfPnpStates[WdfDevStateNormalize(State) - WdfDevStatePnpObjectCreated];
    }

};

__inline
VOID
FxPostProcessInfo::Evaluate(
    __inout FxPkgPnp* PkgPnp
    )
{
    if (m_SetRemovedEvent)
    {
        ASSERT(m_DeleteObject == FALSE && m_Event == NULL && m_FireAndForgetIrp == NULL);
        PkgPnp->SignalDeviceRemovedEvent();
        return;
    }

    //
    // Process any irp that should be sent down the stack/forgotten.
    //
    if (m_FireAndForgetIrp != NULL)
    {
        FxIrp irp(m_FireAndForgetIrp);

        m_FireAndForgetIrp = NULL;
        (void) PkgPnp->FireAndForgetIrp(&irp);
    }

    if (m_DeleteObject)
    {
        PkgPnp->ProcessDelayedDeletion();
    }

    if (m_Event != NULL)
    {
        m_Event->Set();
    }
}

#endif //_FXPKGPNP_H_