/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxPkgPnp.hpp

Abstract:

    This module implements the pnp package for the driver frameworks.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXPKGPNP_H_
#define _FXPKGPNP_H_

//
// These are all magical numbers based on inspection.  If the queue overflows,
// it is OK to increase these numbers without fear of either dependencies or
// weird side effects.
//
const UCHAR PnpEventQueueDepth = 8;
const UCHAR PowerEventQueueDepth = 8;
const UCHAR FxPowerPolicyEventQueueDepth = 8;

// @@SMVERIFY_SPLIT_BEGIN

//
// DEBUGGED_EVENT is a TRUE value rather then a FALSE value (and having the field
// name be TrapOnEvent) so that the initializer for the entry in the table has
// to explicitly set this value, otherwise, if left out, the compiler will zero
// out the field, which would lead to mistakenly saying the event was debugged
// if DEBUGGED_EVENT is FALSE.
//
// Basically, we are catching folks who update the table but forget to specify
// TRAP_ON_EVENT when they add the new entry.
//
#if FX_SUPER_DBG
    #define DEBUGGED_EVENT     , TRUE
    #define TRAP_ON_EVENT      , FALSE

    #define DO_EVENT_TRAP(x) if ((x)->EventDebugged == FALSE) { COVERAGE_TRAP(); }

    #define EVENT_TRAP_FIELD  BOOLEAN EventDebugged;

#else // FX_SUPER_DBG

    #define DEBUGGED_EVENT
    #define TRAP_ON_EVENT
    #define DO_EVENT_TRAP(x)    (0)

    // intentionally blank
    #define EVENT_TRAP_FIELD

#endif // FX_SUPER_DBG

#if FX_STATE_MACHINE_VERIFY
enum FxStateMachineDeviceType {
    FxSmDeviceTypeInvalid = 0,
    FxSmDeviceTypePnp,
    FxSmDeviceTypePnpFdo,
    FxSmDeviceTypePnpPdo,
};
#endif  // FX_STATE_MACHINE_VERIFY

// @@SMVERIFY_SPLIT_END

#include "FxPnpCallbacks.hpp"

#include "FxEventQueue.hpp"

//
// Bit-flags for tracking which callback is currently executing.
//
enum FxDeviceCallbackFlags {
    // Prepare hardware callback is running.
    FXDEVICE_CALLBACK_IN_PREPARE_HARDWARE = 0x01,
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

//
// What follows here is a series of structures that define three state
// machines.  The first is the PnP state machine, followed by the
// Power state machine, followed by the Power Policy state machine.
// The first two will be instantiated in every driver.  The third will
// be instantiated only in drivers which are the power policy owners
// for their device stacks, which usually amounts to the being the FDO.
//
#include "FxPnpStateMachine.hpp"
#include "FxPowerStateMachine.hpp"
#include "FxPowerPolicyStateMachine.hpp"

#include "FxSelfManagedIoStateMachine.hpp"

//
// Group these here instead of in the individual headers because, for some reason,
// tracewpp.exe, can't handle such an arrangement.
//
// begin_wpp config
// CUSTOM_TYPE(FxPnpEvent, ItemEnum(FxPnpEvent));
// CUSTOM_TYPE(FxPowerEvent, ItemEnum(FxPowerEvent));
// CUSTOM_TYPE(FxPowerPolicyEvent, ItemEnum(FxPowerPolicyEvent));
// CUSTOM_TYPE(FxPowerIdleFlags, ItemEnum(FxPowerIdleFlags));
// CUSTOM_TYPE(FxPowerIdleStates, ItemEnum(FxPowerIdleStates));
// CUSTOM_TYPE(FxPowerIdleEvents, ItemEnum(FxPowerIdleEvents));
// CUSTOM_TYPE(FxDevicePwrRequirementEvents, ItemEnum(FxDevicePwrRequirementEvents));
// CUSTOM_TYPE(FxDevicePwrRequirementStates, ItemEnum(FxDevicePwrRequirementStates));
// CUSTOM_TYPE(FxWakeInterruptEvents, ItemEnum(FxWakeInterruptEvents));
// CUSTOM_TYPE(FxWakeInterruptStates, ItemEnum(FxWakeInterruptStates));

// CUSTOM_TYPE(FxSelfManagedIoEvents, ItemEnum(FxSelfManagedIoEvents));
// CUSTOM_TYPE(FxSelfManagedIoStates, ItemEnum(FxSelfManagedIoStates));
// end_wpp

//
// These are defined in ntddk.h so its wpp custom type should be
// added to a common header along with other public enums. Howeever until that
// is done, define custom type here.
//
// begin_wpp config
// CUSTOM_TYPE(DEVICE_POWER_STATE, ItemEnum(_DEVICE_POWER_STATE));
// CUSTOM_TYPE(SYSTEM_POWER_STATE, ItemEnum(_SYSTEM_POWER_STATE));
// CUSTOM_TYPE(BUS_QUERY_ID_TYPE, ItemEnum(BUS_QUERY_ID_TYPE));
// CUSTOM_TYPE(DEVICE_RELATION_TYPE, ItemEnum(_DEVICE_RELATION_TYPE));
// CUSTOM_TYPE(pwrmn, ItemListByte(IRP_MN_WAIT_WAKE,IRP_MN_POWER_SEQUENCE,IRP_MN_SET_POWER,IRP_MN_QUERY_POWER));
// end_wpp

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

const UCHAR DeviceWakeStates = PowerSystemHibernate - PowerSystemWorking + 1;

enum SendDeviceRequestAction {
    NoRetry = 0,
    Retry,
};

enum NotifyResourcesFlags {
    NotifyResourcesNoFlags            = 0x00,
    NotifyResourcesNP                 = 0x01,
    NotifyResourcesSurpriseRemoved    = 0x02,
    NotifyResourcesForceDisconnect    = 0x04,
    NotifyResourcesExplicitPowerup    = 0x08,
    NotifyResourcesExplicitPowerDown  = 0x10,
    NotifyResourcesDisconnectInactive = 0x20,
    NotifyResourcesArmedForWake       = 0x40,
};

enum FxPowerDownType {
    FxPowerDownTypeExplicit = 0,
    FxPowerDownTypeImplicit,
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

struct FxEnumerationInfo : public FxStump {
public:
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
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = m_ChildListList.Initialize();
        if (!NT_SUCCESS(status)) {
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

public:
    FxWaitLockInternal m_PowerStateLock;

    //
    // List of FxChildList objects which contain enumerated children
    //
    FxWaitLockTransactionedList m_ChildListList;
};

class FxPkgPnp : public FxPackage {

    friend FxPnpMachine;
    friend FxPowerMachine;
    friend FxPowerPolicyMachine;
    friend FxPowerPolicyOwnerSettings;
    friend FxInterrupt;

protected:
    FxPkgPnp(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice* Device,
        __in WDFTYPE Type
        );

    ~FxPkgPnp();

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

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

    VOID
    DeleteDevice(
        VOID
        );

    VOID
    SetInternalFailure(
        VOID
        );

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

    PNP_DEVICE_STATE
    HandleQueryPnpDeviceState(
        __in PNP_DEVICE_STATE PnpDeviceState
        );

    _Must_inspect_result_
    NTSTATUS
    HandleQueryBusRelations(
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    NTSTATUS
    HandleQueryDeviceRelations(
        __inout FxIrp* Irp,
        __inout FxRelatedDeviceList* List
        );

    _Must_inspect_result_
    NTSTATUS
    HandleQueryInterface(
        __inout FxIrp* Irp,
        __out   PBOOLEAN CompleteRequest
        );

    _Must_inspect_result_
    NTSTATUS
    QueryForCapabilities(
        VOID
        );

    VOID
    PnpAssignInterruptsSyncIrql(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PnpMatchResources(
        VOID
        );

    __drv_when(!NT_SUCCESS(return), __drv_arg(ResourcesMatched, _Must_inspect_result_))
    NTSTATUS
    PnpPrepareHardware(
        __out PBOOLEAN ResourcesMatched
        );

    _Must_inspect_result_
    NTSTATUS
    PnpPrepareHardwareInternal(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PnpReleaseHardware(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PnpEnableInterfacesAndRegisterWmi(
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

    virtual
    VOID
    PowerReleasePendingDeviceIrp(
        BOOLEAN IrpMustBePresent = TRUE
        ) =0;

    VOID
    AddInterruptObject(
        __in FxInterrupt* Interrupt
        );

    VOID
    RemoveInterruptObject(
        __in FxInterrupt* Interrupt
        );

    VOID
    PnpProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    VOID
    PowerProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    VOID
    PowerPolicyProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    static
    VOID
    _PnpProcessEventInner(
        __inout FxPkgPnp* This,
        __inout FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    static
    VOID
    _PowerProcessEventInner(
        __in FxPkgPnp* This,
        __in FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    static
    VOID
    _PowerPolicyProcessEventInner(
        __inout FxPkgPnp* This,
        __inout FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    VOID
    PnpEnterNewState(
        __in WDF_DEVICE_PNP_STATE State
        );

    VOID
    PowerEnterNewState(
        __in WDF_DEVICE_POWER_STATE State
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
    NTSTATUS
    _DispatchWaitWake(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    NTSTATUS
    DispatchWaitWake(
        __inout FxIrp* Irp
        );

    VOID
    SaveState(
        __in BOOLEAN UseCanSaveState
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _PnpDeviceUsageNotification(
        __inout FxPkgPnp* This,
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    NTSTATUS
    PnpDeviceUsageNotification(
        __inout FxIrp* Irp
        );


    LONG
    GetPnpStateInternal(
        VOID
        );

    LONG
    GetPnpCapsInternal(
        VOID
        );

    static
    VOID
    _SetPowerCapState(
        __in  ULONG Index,
        __in  DEVICE_POWER_STATE State,
        __out PULONG Result
        );

    static
    DEVICE_POWER_STATE
    _GetPowerCapState(
        __in ULONG Index,
        __in ULONG State
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

    VOID
    PnpPowerPolicyStart(
        VOID
        );

    VOID
    PnpPowerPolicyStop(
        VOID
        );

    VOID
    PnpPowerPolicySurpriseRemove(
        VOID
        );

    VOID
    PnpPowerPolicyRemove(
        VOID
        );

    VOID
    PnpFinishProcessingIrp(
        __in BOOLEAN IrpMustBePresent = TRUE
        );

    VOID
    PnpDisableInterfaces(
        VOID
        );

    virtual
    NTSTATUS
    SendIrpSynchronously(
        FxIrp* Irp
        ) =0;

    // begin power state machine table based callbacks
    static
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceType(
        __inout FxPkgPnp* This
        );

    virtual
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceTypeOverload(
        VOID
        ) =0;

    static
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceTypeNP(
        __inout FxPkgPnp* This
        );

    virtual
    WDF_DEVICE_POWER_STATE
    PowerCheckDeviceTypeNPOverload(
        VOID
        ) =0;

    static
    WDF_DEVICE_POWER_STATE
    PowerEnablingWakeAtBus(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerEnablingWakeAtBusNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerCheckParentState(
        __inout FxPkgPnp* This
        );

    virtual
    NTSTATUS
    PowerCheckParentOverload(
        BOOLEAN* ParentOn
        ) =0;

    static
    WDF_DEVICE_POWER_STATE
    PowerCheckParentStateNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDZero(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0NP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0BusWakeOwner(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0BusWakeOwnerNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0ArmedForWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0ArmedForWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoImplicitD3DisarmWakeAtBus(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0DisarmingWakeAtBus(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0DisarmingWakeAtBusNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0Starting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0StartingConnectInterrupt(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0StartingDmaEnable(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0StartingStartSelfManagedIo(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDecideD0State(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoD3Stopped(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStartingCheckDeviceType(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStartingChild(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxDisablingWakeAtBus(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxDisablingWakeAtBusNP(
        __inout FxPkgPnp* This
        );

    virtual
    NTSTATUS
    PowerEnableWakeAtBusOverload(
        VOID
        )
    {
        return STATUS_SUCCESS;
    }

    virtual
    VOID
    PowerDisableWakeAtBusOverload(
        VOID
        )
    {
    }

    virtual
    VOID
    PowerParentPowerDereference(
        VOID
        ) =0;

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDNotZero(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDNotZeroNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDNotZeroIoStopped(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDNotZeroIoStoppedNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxNPFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxArmedForWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxArmedForWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDNotZeroNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxIoStoppedArmedForWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxIoStoppedArmedForWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerCheckParentStateArmedForWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerCheckParentStateArmedForWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStartSelfManagedIo(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStartSelfManagedIoNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStartSelfManagedIoFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStartSelfManagedIoFailedNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakePending(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakePendingNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWaking(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingConnectInterrupt(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingConnectInterruptNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingConnectInterruptFailed(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingConnectInterruptFailedNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingDmaEnable(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingDmaEnableNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingDmaEnableFailed(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerWakingDmaEnableFailedNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerReportPowerUpFailedDerefParent(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerReportPowerUpFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerPowerFailedPowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerReportPowerDownFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerInitialConnectInterruptFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerInitialDmaEnableFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerInitialSelfManagedIoFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerInitialPowerUpFailedDerefParent(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerInitialPowerUpFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxSurpriseRemovedPowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxSurpriseRemovedPowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxStoppedDisarmWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxStoppedDisarmWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxStoppedDisableInterruptNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxStopped(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoStopped(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerStoppedCompleteDx(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxStoppedDecideDxState(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxStoppedArmForWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerDxStoppedArmForWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerFinalPowerDownFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerD0SurpriseRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerSurpriseRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerUpFailedDerefParent(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerUpFailed(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxFailed(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    PowerGotoDxStoppedDisableInterrupt(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    FxPkgPnp::PowerUpFailedDerefParentNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    FxPkgPnp::PowerUpFailedNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    FxPkgPnp::PowerNotifyingD0ExitToWakeInterrupts(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    FxPkgPnp::PowerNotifyingD0EntryToWakeInterrupts(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    FxPkgPnp::PowerNotifyingD0ExitToWakeInterruptsNP(
        __inout FxPkgPnp*   This
        );

    static
    WDF_DEVICE_POWER_STATE
    FxPkgPnp::PowerNotifyingD0EntryToWakeInterruptsNP(
        __inout FxPkgPnp*   This
        );

    // end power state machine table based callbacks

    // begin power policy state machine table based callbacks
    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStarting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartingPoweredUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartingPoweredUpFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartingSucceeded(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartingFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartingDecideS0Wake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartedIdleCapable(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolIdleCapableDeviceIdle(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolDeviceIdleReturnToActive(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolDeviceIdleSleeping(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolDeviceIdleStopping(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredNoWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredNoWakeCompletePowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingUnarmedQueryIdle(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolS0NoWakePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolS0NoWakeCompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemSleepFromDeviceWaitingUnarmed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemSleepNeedWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemSleepNeedWakeCompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemSleepPowerRequestFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolCheckPowerPageable(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakeWakeArrived(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakeRevertArmWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemAsleepWakeArmed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeEnabled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeEnabledWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeDisarm(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeTriggeredS0(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWokeDisarm(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakeWakeArrivedNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakeRevertArmWakeNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakePowerDownFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakePowerDownFailedWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemAsleepWakeArmedNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeEnabledNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeDisarmNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeTriggeredS0NP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWokeDisarmNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeCompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleeping(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingNoWakePowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingNoWakeCompletePowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingNoWakeDxRequestFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingWakePowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingSendWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemAsleepNoWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeDisabled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceToD0(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceToD0CompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeQueryIdle(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartedWakeCapable(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWakeCapableDeviceIdle(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWakeCapableUsbSSCompleted(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredDecideUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapablePowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableSendWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableWakeArrived(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableCancelWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableCleanup(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableDxAllocFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableUndoPowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCompletedPowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCompletedPowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCompletedHardwareStarted(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedQueryIdle(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolIoPresentArmed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolIoPresentArmedWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolS0WakeDisarm(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolS0WakeCompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeSucceeded(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCompletedDisarm(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWakeFailedUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapablePowerDownFailedUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolCancelingWakeForSystemSleep(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolCancelingWakeForSystemSleepWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolDisarmingWakeForSystemSleepCompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolPowerUpForSystemSleepFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWokeFromS0UsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWokeFromS0(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWokeFromS0NotifyDriver(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingResetDevice(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingResetDeviceCompletePowerUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingResetDeviceFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingD0(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingD0Failed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingDisarmWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingDisarmWakeCancelWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingDisarmWakeWakeCanceled(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStopping(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingSucceeded(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingSendStatus(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppedRemoving(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolRemoved(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolRestarting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolRestartingFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingCancelTimer(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingCancelUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingCancelWake(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolCancelUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStarted(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartedCancelTimer(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartedWakeCapableCancelTimerForSleep(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartedWakeCapableSleepingUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStartedIdleCapableCancelTimerForSleep(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolDeviceD0PowerRequestFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolDevicePowerRequestFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSleepingPowerDownNotProcessed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapablePowerDownNotProcessed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredNoWakePowerDownNotProcessed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredNoWakeUndoPowerDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredNoWakeReturnToActive(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredNoWakePoweredDownDisableIdleTimer(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolPowerUpForSystemSleepNotSeen(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedStoppingCancelUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedWakeFailedCancelUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedIoPresentCancelUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedWakeSucceededCancelUsbSS(
        __inout FxPkgPnp* This
        );
    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolCancelingUsbSSForSystemSleep(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolStoppingD0CancelUsbSS(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolWaitingArmedWakeInterruptFired(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeInterruptFired(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolSystemWakeDeviceWakeInterruptFiredNP(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    PowerPolTimerExpiredWakeCapableWakeInterruptArrived(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStarting(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStarted(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerGotoDx(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerGotoDxInDx(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerGotoD0(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerGotoD0InD0(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStopping(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStoppingSendStatus(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStartingFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStoppingFailed(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStoppingPoweringUp(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerStoppingPoweringDown(
        __inout FxPkgPnp* This
        );

    static
    WDF_DEVICE_POWER_POLICY_STATE
    NotPowerPolOwnerRemoved(
        __inout FxPkgPnp* This
        );

    // end power policy state machine table based callbacks

    VOID
    PowerGotoDx(
        VOID
        );

    BOOLEAN
    PowerGotoDxIoStopped(
        VOID
        );

    BOOLEAN
    PowerGotoDxIoStoppedNP(
        VOID
        );

    BOOLEAN
    PowerDmaEnableAndScan(
        __in BOOLEAN ImplicitPowerUp
        );

    VOID
    PowerCompletePendedWakeIrp(
        VOID
        );

    VOID
    PowerCompleteWakeRequestFromWithinMachine(
        __in NTSTATUS Status
        );

    BOOLEAN
    PowerMakeWakeRequestNonCancelable(
        __in NTSTATUS Status
        );

    BOOLEAN
    PowerIsWakeRequestPresent(
        VOID
        )
    {
        //
        // We don't acquire the spinlock because when we transition to the state
        // which will attempt the arming of the device (at bus level), we will
        // gracefully handle the absence of the irp.
        //
        // On the other side if there is no irp and it is set immediately after
        // our check, the event posted by the irp's arrival will transition us
        // to the state which will attempt the arming.
        //
        return m_SharedPower.m_WaitWakeIrp != NULL ? TRUE : FALSE;
    }

    VOID
    PowerSendIdlePowerEvent(
        __in FxPowerIdleEvents Event
        );

    VOID
    PowerSendPowerDownEvents(
        __in FxPowerDownType Type
        );

    VOID
    PowerSendPowerUpEvents(
        VOID
        );

    VOID
    PowerSendPowerDownFailureEvent(
        __in FxPowerDownType Type
        );

    VOID
    PowerSendPowerUpFailureEvent(
        VOID
        );

    VOID
    PowerSetDevicePowerState(
        __in WDF_POWER_DEVICE_STATE State
        );

    _Must_inspect_result_
    BOOLEAN
    PowerDmaPowerUp(
        VOID
        );

    BOOLEAN
    PowerDmaPowerDown(
        VOID
        );

    VOID
    PowerConnectInterruptFailed(
        VOID
        );

    static
    MdCancelRoutineType
    _PowerWaitWakeCancelRoutine;

    VOID
    PowerPolicyUpdateSystemWakeSource(
        __in FxIrp* Irp
        );

    static
    VOID
    _PowerSetSystemWakeSource(
        __in FxIrp* Irp
        );

    static
    MdRequestPowerCompleteType
    _PowerPolDeviceWaitWakeComplete;

    static
    MdRequestPowerCompleteType
    _PowerPolDevicePowerDownComplete;

    static
    MdRequestPowerCompleteType
    _PowerPolDevicePowerUpComplete;

    _Must_inspect_result_
    NTSTATUS
    PowerPolicySendDevicePowerRequest(
        __in DEVICE_POWER_STATE DeviceState,
        __in SendDeviceRequestAction Action
        );

    _Must_inspect_result_
    NTSTATUS
    PowerPolicySendWaitWakeRequest(
        __in SYSTEM_POWER_STATE SystemState
        );

    VOID
    PowerPolicyCompleteSystemPowerIrp(
        VOID
        );

    BOOLEAN
    PowerPolicyCancelWaitWake(
        VOID
        );

    VOID
    PowerPolicySubmitUsbIdleNotification(
        VOID
        );

    BOOLEAN
    PowerPolicyCancelUsbSSIfCapable(
        VOID
        );

    VOID
    PowerPolicyCancelUsbSS(
        VOID
        );

    static
    MdCompletionRoutineType
    _PowerPolicyWaitWakeCompletionRoutine;

    static
    MdCompletionRoutineType
    _PowerPolicyUsbSelectiveSuspendCompletionRoutine;

    SYSTEM_POWER_STATE
    PowerPolicyGetPendingSystemState(
        VOID
        )
    {
        FxIrp irp(m_PendingSystemPowerIrp);

        //
        // In a FastS4 situation, Parameters.Power.State.SystemState will be
        // PowerSystemHibernate, while TargetSystemState will indicate the
        // true Sx state the machine is moving into.
        //
        return (SYSTEM_POWER_STATE)
            (irp.GetParameterPowerSystemPowerStateContext()).
                TargetSystemState;
    }

    _Must_inspect_result_
    NTSTATUS
    PowerPolicyHandleSystemQueryPower(
        __in SYSTEM_POWER_STATE QueryState
        );

    BOOLEAN
    PowerPolicyCanWakeFromSystemState(
        __in SYSTEM_POWER_STATE SystemState
        )
    {
        return SystemState <= PowerPolicyGetDeviceDeepestSystemWakeState();
    }

    SYSTEM_POWER_STATE
    PowerPolicyGetDeviceDeepestSystemWakeState(
        VOID
        )
    {
        return (SYSTEM_POWER_STATE) m_SystemWake;
    }

    DEVICE_POWER_STATE
    PowerPolicyGetDeviceDeepestDeviceWakeState(
        __in SYSTEM_POWER_STATE SystemState
        );

    static
    CPPNP_STATE_TABLE
    GetPnpTableEntry(
        __in WDF_DEVICE_PNP_STATE State
        )
    {
        return &m_WdfPnpStates[WdfDevStateNormalize(State) - WdfDevStatePnpObjectCreated];
    }

    static
    CPPOWER_STATE_TABLE
    GetPowerTableEntry(
        __in WDF_DEVICE_POWER_STATE State
        )
    {
        return &m_WdfPowerStates[WdfDevStateNormalize(State) - WdfDevStatePowerObjectCreated];
    }

    static
    CPPOWER_POLICY_STATE_TABLE
    GetPowerPolicyTableEntry(
        __in WDF_DEVICE_POWER_POLICY_STATE State
        )
    {
        return &m_WdfPowerPolicyStates[WdfDevStateNormalize(State) - WdfDevStatePwrPolObjectCreated];
    }

#if FX_STATE_MACHINE_VERIFY
    static
    CPPNP_STATE_ENTRY_FN_RETURN_STATE_TABLE
    GetPnpStateEntryFunctionReturnStatesTableEntry(
        __in WDF_DEVICE_PNP_STATE State
        )
    {
        return &m_WdfPnpStateEntryFunctionReturnStates[WdfDevStateNormalize(State) - WdfDevStatePnpObjectCreated];
    }

    static
    CPPOWER_STATE_ENTRY_FN_RETURN_STATE_TABLE
    GetPowerStateEntryFunctionReturnStatesTableEntry(
        __in WDF_DEVICE_POWER_STATE State
        )
    {
        return &m_WdfPowerStateEntryFunctionReturnStates[WdfDevStateNormalize(State) - WdfDevStatePowerObjectCreated];
    }

    static
    CPPWR_POL_STATE_ENTRY_FN_RETURN_STATE_TABLE
    GetPwrPolStateEntryFunctionReturnStatesTableEntry(
        __in WDF_DEVICE_POWER_POLICY_STATE State
        )
    {
        return &m_WdfPwrPolStateEntryFunctionReturnStates[WdfDevStateNormalize(State) - WdfDevStatePwrPolObjectCreated];
    }

    VOID
    ValidatePnpStateEntryFunctionReturnValue(
        WDF_DEVICE_PNP_STATE CurrentState,
        WDF_DEVICE_PNP_STATE NewState
        );

    VOID
    ValidatePowerStateEntryFunctionReturnValue(
        WDF_DEVICE_POWER_STATE CurrentState,
        WDF_DEVICE_POWER_STATE NewState
        );

    VOID
    ValidatePwrPolStateEntryFunctionReturnValue(
        WDF_DEVICE_POWER_POLICY_STATE CurrentState,
        WDF_DEVICE_POWER_POLICY_STATE NewState
        );
#endif //FX_STATE_MACHINE_VERIFY

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
             i++) {
            if (m_WdfNotPowerPolicyOwnerStates[i].CurrentTargetState == State) {
                return &m_WdfNotPowerPolicyOwnerStates[i];
            }
        }

        return NULL;
    }

    static
    NTSTATUS
    _S0IdleQueryInstance(
        __in  CfxDevice* Device,
        __in  FxWmiInstanceInternal* Instance,
        __in  ULONG OutBufferSize,
        __out PVOID OutBuffer,
        __out PULONG BufferUsed
        );

    static
    NTSTATUS
    _S0IdleSetInstance(
        __in CfxDevice* Device,
        __in FxWmiInstanceInternal* Instance,
        __in ULONG InBufferSize,
        __in PVOID InBuffer
        );

    static
    NTSTATUS
    _S0IdleSetItem(
        __in CfxDevice* Device,
        __in FxWmiInstanceInternal* Instance,
        __in ULONG DataItemId,
        __in ULONG InBufferSize,
        __in PVOID InBuffer
        );

    static
    NTSTATUS
    _SxWakeQueryInstance(
        __in  CfxDevice* Device,
        __in  FxWmiInstanceInternal* Instaace,
        __in  ULONG OutBufferSize,
        __out PVOID OutBuffer,
        __out PULONG BufferUsed
        );

    static
    NTSTATUS
    _SxWakeSetInstance(
        __in CfxDevice* Device,
        __in FxWmiInstanceInternal* Instance,
        __in ULONG InBufferSize,
        __in PVOID InBuffer
        );

    static
    NTSTATUS
    _SxWakeSetItem(
        __in CfxDevice* Device,
        __in FxWmiInstanceInternal* Instance,
        __in ULONG DataItemId,
        __in ULONG InBufferSize,
        __in PVOID InBuffer
        );

    BOOLEAN
    IsPresentPendingPnpIrp(
        VOID
        )
    {
        return (m_PendingPnPIrp != NULL) ? TRUE : FALSE;
    }

    VOID
    SetPendingPnpIrp(
        __inout FxIrp* Irp,
        __in    BOOLEAN MarkIrpPending = TRUE
        );

    VOID
    SetPendingPnpIrpStatus(
        __in NTSTATUS Status
        )
    {
        FxIrp irp(m_PendingPnPIrp);

        ASSERT(m_PendingPnPIrp != NULL);
        irp.SetStatus(Status);
    }

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

        if (Irp->GetParameterPowerStateDeviceState() > PowerDeviceD0) {
            //
            // We are powering down, capture the current power action.  We will
            // reset it to PowerActionNone once we have powered up.
            //
            m_SystemPowerAction = (UCHAR) Irp->GetParameterPowerShutdownType();
        }
    }

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
    ClearPendingSystemPowerIrp(
        VOID
        )
    {
        MdIrp irp;

        irp = m_PendingSystemPowerIrp;
        m_PendingSystemPowerIrp = NULL;

        return irp;
    }

    MdIrp
    GetPendingSystemPowerIrp(
        VOID
        )
    {
        return m_PendingSystemPowerIrp;
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

    BOOLEAN
    IsUsageSupported(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Usage
        )
    {
        return m_SpecialSupport[((ULONG)Usage)-1];
    }

    VOID
    SetUsageSupport(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Usage,
        __in BOOLEAN Supported
        )
    {
        m_SpecialSupport[((ULONG) Usage)-1] = Supported;
    }

    LONG
    AdjustUsageCount(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Usage,
        __in BOOLEAN Add
        )
    {
        if (Add) {
            return InterlockedIncrement(&m_SpecialFileCount[((ULONG)Usage)-1]);
        }
        else {
            return InterlockedDecrement(&m_SpecialFileCount[((ULONG)Usage)-1]);
        }
    }

    LONG
    GetUsageCount(
        // __range(WdfSpecialFilePaging, WdfSpecialFileBoot)
        __in __range(1, 4) ULONG Usage
        )
    {
        return m_SpecialFileCount[Usage-1];
    }

    BOOLEAN
    IsInSpecialUse(
        VOID
        )
    {
        if (GetUsageCount(WdfSpecialFilePaging) == 0 &&
            GetUsageCount(WdfSpecialFileHibernation) == 0 &&
            GetUsageCount(WdfSpecialFileDump) == 0 &&
            GetUsageCount(WdfSpecialFileBoot) == 0) {
            return FALSE;
        }
        else {
            return TRUE;
        }
    }

    static
    DEVICE_USAGE_NOTIFICATION_TYPE
    _SpecialTypeToUsage(
        __in WDF_SPECIAL_FILE_TYPE Type
        )
    {
        switch (Type) {
        case WdfSpecialFilePaging:       return DeviceUsageTypePaging;
        case WdfSpecialFileHibernation:  return DeviceUsageTypeHibernation;
        case WdfSpecialFileDump:         return DeviceUsageTypeDumpFile;
        case WdfSpecialFileBoot:         return DeviceUsageTypeBoot;
        default:           ASSERT(FALSE);return DeviceUsageTypePaging;
        }
    }

    static
    WDF_SPECIAL_FILE_TYPE
    _UsageToSpecialType(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Type
        )
    {
        switch (Type) {
        case DeviceUsageTypePaging:         return WdfSpecialFilePaging;
        case DeviceUsageTypeHibernation:    return WdfSpecialFileHibernation;
        case DeviceUsageTypeDumpFile:       return WdfSpecialFileDump;
        case DeviceUsageTypeBoot:           return WdfSpecialFileBoot;
        default:            ASSERT(FALSE);  return WdfSpecialFilePaging;
        }
    }

    ULONG
    SetUsageNotificationFlags(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Type,
        __in BOOLEAN InPath
        );

    VOID
    RevertUsageNotificationFlags(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Type,
        __in BOOLEAN InPath,
        __in ULONG OldFlags
        );

    VOID
    CommitUsageNotification(
        __in DEVICE_USAGE_NOTIFICATION_TYPE Type,
        __in ULONG OldFlags
        );

    _Must_inspect_result_
    NTSTATUS
    CreatePowerThread(
        VOID
        );

public:
    VOID
    PnpProcessEvent(
        __in FxPnpEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    VOID
    PowerProcessEvent(
        __in FxPowerEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    VOID
    PowerPolicyProcessEvent(
        __in FxPowerPolicyEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    BOOLEAN
    ShouldProcessPnpEventOnDifferentThread(
        __in KIRQL CurrentIrql,
        __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
        );

    BOOLEAN
    ShouldProcessPowerPolicyEventOnDifferentThread(
        __in KIRQL CurrentIrql,
        __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
        );

    VOID
    CleanupStateMachines(
        __in BOOLEAN ClenaupPnp
        );

    VOID
    CleanupDeviceFromFailedCreate(
        __in MxEvent * WaitEvent
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

    virtual
    VOID
    FinishInitialize(
        __inout PWDFDEVICE_INIT DeviceInit
        );

    VOID
    SetSpecialFileSupport(
        __in WDF_SPECIAL_FILE_TYPE FileType,
        __in BOOLEAN Supported
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

    NTSTATUS
    RegisterPowerPolicyWmiInstance(
        __in  const GUID* Guid,
        __in  FxWmiInstanceInternalCallbacks* Callbacks,
        __out FxWmiInstanceInternal** Instance
        );

    NTSTATUS
    PowerPolicySetS0IdleSettings(
        __in PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings
        );

    NTSTATUS
    AssignPowerFrameworkSettings(
        __in PWDF_POWER_FRAMEWORK_SETTINGS PowerFrameworkSettings
        );

    NTSTATUS
    PowerPolicySetSxWakeSettings(
        __in PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Settings,
        __in BOOLEAN ArmForWakeIfChildrenAreArmedForWake,
        __in BOOLEAN IndicateChildWakeOnParentWake
        );

private:

    VOID
    DisconnectInterruptNP(
        VOID
        );

    NTSTATUS
    UpdateWmiInstanceForS0Idle(
        __in FxWmiInstanceAction Action
        );

    VOID
    ReadRegistryS0Idle(
        __in PCUNICODE_STRING ValueName,
        __out BOOLEAN *Enabled
        );

    NTSTATUS
    UpdateWmiInstanceForSxWake(
        __in FxWmiInstanceAction Action
        );

    VOID
    ReadRegistrySxWake(
        __in PCUNICODE_STRING ValueName,
        __out BOOLEAN *Enabled
        );

    VOID
    WriteStateToRegistry(
        __in HANDLE RegKey,
        __in PUNICODE_STRING ValueName,
        __in ULONG Value
        );

    NTSTATUS
    ReadStateFromRegistry(
        _In_ PCUNICODE_STRING ValueName,
        _Out_ PULONG Value
        );

    NTSTATUS
    UpdateWmiInstance(
        _In_ FxWmiInstanceAction Action,
        _In_ BOOLEAN ForS0Idle
        );

public:
    BOOLEAN
    PowerIndicateWaitWakeStatus(
        __in NTSTATUS WaitWakeStatus
        );

    BOOLEAN
    PowerPolicyIsWakeEnabled(
        VOID
        );

    ULONG
    PowerPolicyGetCurrentWakeReason(
        VOID
        );

    BOOLEAN
    __inline
    PowerPolicyShouldPropagateWakeStatusToChildren(
        VOID
        )
    {
        return m_PowerPolicyMachine.m_Owner->m_WakeSettings.IndicateChildWakeOnParentWake;
    }

    VOID
    ChildRemoved(
        VOID
        )
    {
        LONG c;

        //
        // Called by a child that we are waiting on its removal
        //
        c = InterlockedDecrement(&m_PendingChildCount);
        ASSERT(c >= 0);

        if (c == 0) {
            PnpProcessEvent(PnpEventChildrenRemovalComplete);
        }
    }

    VOID
    PowerPolicySetS0IdleState(
        __in BOOLEAN State
        );

    VOID
    PowerPolicySetSxWakeState(
        __in BOOLEAN State
        );

    VOID
    SetPowerCaps(
        __in PWDF_DEVICE_POWER_CAPABILITIES PowerCapabilities
        );

    VOID
    SetPnpCaps(
        __in PWDF_DEVICE_PNP_CAPABILITIES PnpCapabilities
        );

    VOID
    GetPnpState(
        __out PWDF_DEVICE_STATE State
        );

    VOID
    SetPnpState(
        __in PWDF_DEVICE_STATE State
        );

    VOID
    SetDeviceFailed(
        __in WDF_DEVICE_FAILED_ACTION FailedAction
        );

    VOID
    SetChildBusInformation(
        __in PPNP_BUS_INFORMATION BusInformation
        )
    {
        RtlCopyMemory(&m_BusInformation,
                      BusInformation,
                      sizeof(PNP_BUS_INFORMATION));
    }

    _Must_inspect_result_
    NTSTATUS
    HandleQueryBusInformation(
        __inout FxIrp* Irp
        );

    _Must_inspect_result_
    NTSTATUS
    __inline
    PowerReference(
        __in BOOLEAN WaitForD0,
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PSTR File = NULL
        )
    {
        return m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.PowerReference(WaitForD0, Tag, Line, File);
    }

    VOID
    __inline
    PowerDereference(
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PSTR File = NULL
        )
    {
        m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.IoDecrement(Tag, Line, File);
    }

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

    _Must_inspect_result_
    NTSTATUS
    AddUsageDevice(
        __in MdDeviceObject DependentDevice
        );

    VOID
    RemoveUsageDevice(
        __in MdDeviceObject DependentDevice
        );

    _Must_inspect_result_
    NTSTATUS
    AddRemovalDevice(
        __in MdDeviceObject DependentDevice
        );

    VOID
    RemoveRemovalDevice(
        __in MdDeviceObject DependentDevice
        );

    VOID
    ClearRemovalDevicesList(
        VOID
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
    RemoveChildList(
        __in FxChildList* List
        );

    VOID
    ChildListNotifyRemove(
        __inout PLONG PendingCount
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateDmaEnablerList(
        VOID
        );

    VOID
    AddDmaEnabler(
        __in FxDmaEnabler* Enabler
        );

    VOID
    RemoveDmaEnabler(
        __in FxDmaEnabler* Enabler
        );

    VOID
    RevokeDmaEnablerResources(
        __in FxDmaEnabler* Enabler
        );

    VOID
    AddQueryInterface(
        __in FxQueryInterface* QI,
        __in BOOLEAN Lock
        );

    VOID
    QueryForD3ColdInterface(
        VOID
        );

    VOID
    DropD3ColdInterface(
        VOID
        );

    BOOLEAN
    IsPowerPolicyOwner(
        VOID
        )
    {
        return m_PowerPolicyMachine.m_Owner != NULL ? TRUE : FALSE;
    }

    BOOLEAN
    SupportsWakeInterrupt(
        VOID
        )
    {
        if (m_WakeInterruptCount > 0) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    BOOLEAN
    IsS0IdleWakeFromS0Enabled(
        VOID
        )
    {
        if (IsPowerPolicyOwner()) {
            return m_PowerPolicyMachine.m_Owner->m_IdleSettings.WakeFromS0Capable;
        }
        else {
            return FALSE;
        }
    }

    BOOLEAN
    IsS0IdleSystemManaged(
        VOID
        )
    {
        if (IsPowerPolicyOwner()) {
            return m_PowerPolicyMachine.m_Owner->m_IdleSettings.m_TimeoutMgmt.UsingSystemManagedIdleTimeout();
        }
        else {
            return FALSE;
        }
    }

    BOOLEAN
    IsS0IdleUsbSSEnabled(
        VOID
        )
    {
        if (IsPowerPolicyOwner()) {
            return (m_PowerPolicyMachine.m_Owner->m_IdleSettings.WakeFromS0Capable &&
                    m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable);
        }
        else {
            return FALSE;
        }
    }

    BOOLEAN
    IsSxWakeEnabled(
        VOID
        )
    {
        if (IsPowerPolicyOwner()) {
            return m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled;
        }
        else {
            return FALSE;
        }
    }

    _Must_inspect_result_
    NTSTATUS
    PowerPolicyCanChildPowerUp(
        __out PBOOLEAN PowerUp
        )
    {
        *PowerUp = FALSE;

        if (IsPowerPolicyOwner()) {
            NTSTATUS status;

            //
            // By referencing the parent (this device) we make sure that if the
            // parent is in Dx, we force it back into D0 so that this child can
            // be in D0.
            //
            status = PowerReference(FALSE);

            if (!NT_SUCCESS(status)) {
                return status;
            }

            //
            // m_EnumInfo is valid because the child device is calling into the
            // parent device and if there is a child, there is a m_EnumInfo.
            //
            m_EnumInfo->AcquireParentPowerStateLock(GetDriverGlobals());

            //
            // The caller has added a power reference to this device, so this device
            // will remain in D0 until that power reference has been removed.  This
            // count is separate from the power ref count b/c the power ref count
            // only controls when the idle timer will fire.  This count handles the
            // race that can occur after we decide that the parent is idle and act
            // on it and the child powers up before this parent actually powers
            // down.
            //
            m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount++;
            *PowerUp = m_PowerPolicyMachine.m_Owner->m_ChildrenCanPowerUp;

            m_EnumInfo->ReleaseParentPowerStateLock(GetDriverGlobals());
        }
        else {
            //
            // The parent (this device) is not the power policy owner.  That
            // means we cannot poke the parent to come back to D0 and rely on
            // the parent being in D0.  Our only recourse is to move into D0 and
            // ignore the parent's device power state.
            //
            // We will only get into this situation if the parent is not the
            // power policy owner of the stack.  This usually means the parent
            // is a filter driver in the parent stack and is creating a virtual
            // child.  Since the child is assumed virtual, it's D state is not
            // tied to real hardware and doesn't really matter.
            //
            *PowerUp = TRUE;
        }

        return STATUS_SUCCESS;
    }

    VOID
    PowerPolicyChildPoweredDown(
        VOID
        )
    {
        //
        // If this parent is the power policy owner of the child's stack, release
        // the requirement this device to be in D0 while the child is in D0.
        //
        if (IsPowerPolicyOwner()) {
            //
            // Decrement the number of children who are powered on
            //
            m_EnumInfo->AcquireParentPowerStateLock(GetDriverGlobals());
            ASSERT(m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount > 0);
            m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount--;
            m_EnumInfo->ReleaseParentPowerStateLock(GetDriverGlobals());

            PowerDereference();
        }
    }

    POWER_ACTION
    GetSystemPowerAction(
        VOID
        )
    {
        return (POWER_ACTION) m_SystemPowerAction;
    }

    VOID
    ProcessDelayedDeletion(
        VOID
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

    FxCmResList *
    GetTranslatedResourceList(
        VOID
        )
    {
        return m_Resources;
    }

    FxCmResList *
    GetRawResourceList(
        VOID
        )
    {
        return m_ResourcesRaw;
    }

    ULONG
    GetInterruptObjectCount(
        VOID
        )
    {
        return m_InterruptObjectCount;
    }

    VOID
    AckPendingWakeInterruptOperation(
        __in BOOLEAN ProcessPowerEventOnDifferentThread
        );

    VOID
    SendEventToAllWakeInterrupts(
        __in enum  FxWakeInterruptEvents WakeInterruptEvent
    );

private:
    VOID
    PowerPolicyCheckAssumptions(
        VOID
        );

    VOID
    PowerCheckAssumptions(
        VOID
        );

    VOID
    PnpCheckAssumptions(
        VOID
        );

    VOID
    NotifyResourceobjectsToReleaseResources(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    NotifyResourceObjectsD0(
        __in ULONG NotifyFlags
        );

    NTSTATUS
    NotifyResourceObjectsDx(
        __in ULONG NotifyFlags
        );

    BOOLEAN
    PnpCheckAndIncrementRestartCount(
        VOID
        );

    BOOLEAN
    PnpIncrementRestartCountLogic(
        _In_ HANDLE RestartKey,
        _In_ BOOLEAN CreatedNewKey
        );

    VOID
    PnpCleanupForRemove(
        __in BOOLEAN GracefulRemove
        );

    virtual
    NTSTATUS
    ProcessRemoveDeviceOverload(
        FxIrp* Irp
        ) =0;

    virtual
    VOID
    DeleteSymbolicLinkOverload(
        BOOLEAN GracefulRemove
        ) =0;

    virtual
    VOID
    QueryForReenumerationInterface(
        VOID
        ) =0;

    virtual
    VOID
    ReleaseReenumerationInterface(
        VOID
        ) =0;

    virtual
    NTSTATUS
    AskParentToRemoveAndReenumerate(
        VOID
        ) =0;

    _Must_inspect_result_
    NTSTATUS
    CreatePowerThreadIfNeeded(
        VOID
        );

    virtual
    NTSTATUS
    QueryForPowerThread(
        VOID
        ) =0;

    VOID
    ReleasePowerThread(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    HandleQueryInterfaceForPowerThread(
        __inout FxIrp* Irp,
        __out   PBOOLEAN CompleteRequest
        );

    _Must_inspect_result_
    NTSTATUS
    PnpPowerReferenceSelf(
        VOID
        );

    VOID
    PnpPowerDereferenceSelf(
        VOID
        );

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

    BOOLEAN
    PowerPolicyCanIdlePowerDown(
        __in DEVICE_POWER_STATE DxState
        );

    VOID
    PowerPolicyPostParentToD0ToChildren(
        VOID
        );

    VOID
    PowerPolicyChildrenCanPowerUp(
        VOID
        );

    VOID
    __inline
    PowerPolicyDisarmWakeFromSx(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PowerPolicyPowerDownForSx(
        __in DEVICE_POWER_STATE DxState,
        __in SendDeviceRequestAction Action
        )
    {
        //
        // The device is powering down because the system is moving into a lower
        // power state.
        //
        // If we have child devices, setup the guard so that they do not power
        // up while the parent is in low power.  Note that in this case (an Sx
        // transition) we do not look at the count of powered up children
        // because the power policy owner for the child's stack should not be
        // powering up the device once it has processed the Sx irp for its stack.
        //
        PowerPolicyBlockChildrenPowerUp();

        return PowerPolicySendDevicePowerRequest(DxState, Action);
    }

    VOID
    PowerPolicyBlockChildrenPowerUp(
        VOID
        )
    {
        if (m_EnumInfo != NULL) {
            m_EnumInfo->AcquireParentPowerStateLock(GetDriverGlobals());
            //
            // Setup a guard so that no children power up until we return to S0.
            //
            m_PowerPolicyMachine.m_Owner->m_ChildrenCanPowerUp = FALSE;
            m_EnumInfo->ReleaseParentPowerStateLock(GetDriverGlobals());
        }
    }

    _Must_inspect_result_
    NTSTATUS
    PnpPowerReferenceDuringQueryPnp(
        VOID
        );

public:
    _Must_inspect_result_
    NTSTATUS
    ValidateCmResource(
        __inout PCM_PARTIAL_RESOURCE_DESCRIPTOR* CmResourceRaw,
        __inout PCM_PARTIAL_RESOURCE_DESCRIPTOR* CmResource
        );

    _Must_inspect_result_
    NTSTATUS
    ValidateInterruptResourceCm(
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmIntResourceRaw,
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmIntResource,
        __in PWDF_INTERRUPT_CONFIG Configuration
        );

    BOOLEAN
    IsDefaultReleaseHardwareOrder(
        VOID
        )
    {
#if FX_IS_KERNEL_MODE
        return (m_ReleaseHardwareAfterDescendantsOnFailure == WdfReleaseHardwareOrderOnFailureEarly ? TRUE : FALSE);
#else
        return FALSE;
#endif
    }

    BOOLEAN
    HasMultipleInterrupts(
        VOID
        )
    {
        return (m_InterruptObjectCount > 1 ? TRUE : FALSE);
    }

    VOID
    WakeInterruptCreated(
        VOID
        )
    {
        ASSERT(IsPowerPolicyOwner() != FALSE);

        ++m_WakeInterruptCount;
    }

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
    // List of dependent devices for usage notifications
    //
    FxRelatedDeviceList* m_UsageDependentDeviceList;

    FxRelatedDeviceList* m_RemovalDeviceList;

    //
    // Collection of FxQueryInterface objects
    //
    FxWaitLockInternal m_QueryInterfaceLock;

    SINGLE_LIST_ENTRY m_QueryInterfaceHead;

    FxWaitLockInternal m_DeviceInterfaceLock;

    SINGLE_LIST_ENTRY m_DeviceInterfaceHead;

    BOOLEAN m_DeviceInterfacesCanBeEnabled;

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

    //
    // All 3 state machine engines
    //
    FxPnpMachine m_PnpMachine;
    FxPowerMachine m_PowerMachine;
    FxPowerPolicyMachine m_PowerPolicyMachine;

    FxSelfManagedIoMachine* m_SelfManagedIoMachine;

    //
    // Data shared between the power and power policy machines determining how
    // we handle wait wake irps.
    //
    SharedPowerData m_SharedPower;

    //
    // Interface for managing the difference between D3hot and D3cold.
    //
    D3COLD_SUPPORT_INTERFACE m_D3ColdInterface;

protected:
    //
    // Event that is set when processing a remove device is complete
    //
    MxEvent* m_DeviceRemoveProcessed;

    //
    // Count of children we need to fully remove when the parent (this package)
    // is being removed.
    //
    LONG m_PendingChildCount;

    //
    // DEVICE_WAKE_DEPTH - Indicates the lowest D-state that can successfully
    // generate a wake signal from a particular S-state.  The array is tightly-
    // packed, with index 0 corresponding to PowerSystemWorking.
    //
    BYTE m_DeviceWake[DeviceWakeStates];

    // SYSTEM_POWER_STATE
    BYTE m_SystemWake;

    // WDF_DEVICE_FAILED_ACTION
    BYTE m_FailedAction;

    //
    // Set the event which indicates that the pnp state machine is done outside
    // of the state machine so that we can drain any remaining state machine
    // events in the removing thread before signaling the event.
    //
    BYTE m_SetDeviceRemoveProcessed;

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
    // Bus information for any enumerated children
    //
    PNP_BUS_INFORMATION m_BusInformation;

    //
    // Number of times we have tried to enumerate children but failed
    //
    UCHAR m_BusEnumRetries;

    //
    // The power action corresponding to the system power transition
    //
    UCHAR m_SystemPowerAction;

    //
    // TRUE once the entire stack has been queried for the caps
    //
    BOOLEAN m_CapsQueried;

    BOOLEAN m_InternalFailure;

    //
    // if FALSE, there is no power thread available on the devnode currently
    // and a work item should be enqueued.  if TRUE, there is a power thread
    // and the callback should be enqueued to it.
    //
    BOOLEAN m_HasPowerThread;

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
    // GUID for querying for a power thread down the stack
    //
    static const GUID GUID_POWER_THREAD_INTERFACE;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // Interrupt APIs for Vista and forward
    //
    PFN_IO_CONNECT_INTERRUPT_EX     m_IoConnectInterruptEx;
    PFN_IO_DISCONNECT_INTERRUPT_EX  m_IoDisconnectInterruptEx;
    //
    // Interrupt APIs for Windows 8 and forward
    //
    PFN_IO_REPORT_INTERRUPT_ACTIVE     m_IoReportInterruptActive;
    PFN_IO_REPORT_INTERRUPT_INACTIVE   m_IoReportInterruptInactive;
#endif

private:

    //
    // For user mode we need to preallocate event since its initialization can
    // fail
    //
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    FxCREvent m_CleanupEventUm;
    MxEvent m_RemoveEventUm;
#endif

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

    //
    // Non NULL when this device is exporting the power thread interface.  This
    // would be the lowest device in the stack that supports this interface.
    //
    FxSystemThread* m_PowerThread;

    LONG m_PowerThreadInterfaceReferenceCount;

    FxCREvent* m_PowerThreadEvent;

    //
    // The current pnp state changing irp in the stack that we have pended to
    // process in the pnp state machine
    //
    MdIrp m_PendingPnPIrp;

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

    static const POWER_EVENT_TARGET_STATE m_PowerD0OtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerD0NPOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerD0BusWakeOwnerOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerD0BusWakeOwnerNPOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerD0ArmedForWakeOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerD0ArmedForWakeNPOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerDNotZeroOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerDNotZeroNPOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_DxArmedForWakeOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_DxArmedForWakeNPOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_WakePendingOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_WakePendingNPOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_DxSurpriseRemovedOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerStoppedOtherStates[];
    static const POWER_EVENT_TARGET_STATE m_PowerDxStoppedOtherStates[];

    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolObjectCreatedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStartingOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStartedIdleCapableOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolIdleCapableDeviceIdleOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredNoWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredNoWakeCompletePowerDownOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolWaitingUnarmedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolS0NoWakePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolS0NoWakeCompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemSleepNeedWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemSleepNeedWakeCompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemAsleepWakeArmedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemAsleepWakeArmedNPOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceToD0OtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceToD0CompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStartedWakeCapableOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolWakeCapableDeviceIdleOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapablePowerDownOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableSendWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableUsbSSOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolWaitingArmedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolDisarmingWakeForSystemSleepCompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolCancelingWakeForSystemSleepWakeCanceledOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolWokeFromS0OtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingResetDeviceOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingResetDeviceCompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingD0OtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingDisarmWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingDisarmWakeCancelWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolIoPresentArmedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolIoPresentArmedWakeCanceledOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolS0WakeCompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableWakeSucceededOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableWakeFailedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolCancelingWakeForSystemSleepOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableWakeArrivedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableCancelWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCompletedPowerDownOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCompletedPowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeEnabledOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeEnabledNPOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNPOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingWakeWakeArrivedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeTriggeredS0OtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeTriggeredS0NPOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeCompletePowerUpOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingNoWakePowerDownOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingNoWakeCompletePowerDownOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingWakePowerDownOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingSendWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingWakeWakeArrivedNPOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingWakePowerDownFailedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStartedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStartedWaitForIdleTimeoutOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolDevicePowerRequestFailedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolRestartingOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolStoppingCancelWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolCancelUsbSSOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingWakeRevertArmWakeOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSleepingWakeRevertArmWakeNPOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolRemovedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapableWakeInterruptArrivedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolTimerExpiredWakeCapablePowerDownFailedWakeInterruptArrivedOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolWaitingArmedWakeInterruptFiredDuringPowerDownOtherStates[];

    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerObjectCreatedStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStartingStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStartingSucceededStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStartingFailedStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerGotoDxStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerGotoDxInDxStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerDxStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerGotoD0States[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerGotoD0InD0States[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStoppedStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStoppingWaitForImplicitPowerDownStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStoppingPoweringUpStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerStoppingPoweringDownStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_NotPowerPolOwnerRemovedStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolWaitingArmedWakeInterruptFiredOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeInterruptFiredOtherStates[];
    static const POWER_POLICY_EVENT_TARGET_STATE m_PowerPolSystemWakeDeviceWakeInterruptFiredNPOtherStates[];

#if FX_STATE_MACHINE_VERIFY
    //
    // Array of possible states that can be returned by state entry functions
    //
    static const PNP_STATE_ENTRY_FN_RETURN_STATE_TABLE m_WdfPnpStateEntryFunctionReturnStates[];
    static const POWER_STATE_ENTRY_FN_RETURN_STATE_TABLE m_WdfPowerStateEntryFunctionReturnStates[];
    static const PWR_POL_STATE_ENTRY_FN_RETURN_STATE_TABLE m_WdfPwrPolStateEntryFunctionReturnStates[];
#endif // FX_STATE_MACHINE_VERIFY

    //
    // Names for registry values in which we will store the beginning of the
    // restart time period, the number of restart attempts in that period, and
    // if the device successfully started.
    //
    static const PWCHAR m_RestartStartAchievedName;
    static const PWCHAR m_RestartStartTimeName;
    static const PWCHAR m_RestartCountName;

    //
    // Time between successive restarts in which we will attempt to restart a
    // stack again.  Expressed in seconds.
    //
    static const ULONG m_RestartTimePeriodMaximum;

    //
    // Number of times in the restart time period in which we will attempt a
    // restart.
    //
    static const ULONG m_RestartCountMaximum;

    //
    // Shove the function pointers to the end of the structure so that when
    // we dump the structure while debugging, the less pertinent info is at the
    // bottom.
    //
public:
    FxPnpDeviceUsageNotification        m_DeviceUsageNotification;
    FxPnpDeviceUsageNotificationEx      m_DeviceUsageNotificationEx;
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
};

__inline
VOID
FxPostProcessInfo::Evaluate(
    __inout FxPkgPnp* PkgPnp
    )
{
    if (m_SetRemovedEvent) {
        ASSERT(m_DeleteObject == FALSE && m_Event == NULL && m_FireAndForgetIrp == NULL);
        PkgPnp->SignalDeviceRemovedEvent();
        return;
    }

    //
    // Process any irp that should be sent down the stack/forgotten.
    //
    if (m_FireAndForgetIrp != NULL) {
        FxIrp irp(m_FireAndForgetIrp);

        m_FireAndForgetIrp = NULL;
        (void) PkgPnp->FireAndForgetIrp(&irp);
    }

    if (m_DeleteObject) {
        PkgPnp->ProcessDelayedDeletion();
    }

    if (m_Event != NULL) {
        m_Event->Set();
    }
}

#endif //  _FXPKGPNP_H_
