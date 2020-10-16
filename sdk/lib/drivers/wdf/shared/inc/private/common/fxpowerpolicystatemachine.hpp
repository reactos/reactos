//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXPOWERPOLICYSTATEMACHINE_H_
#define _FXPOWERPOLICYSTATEMACHINE_H_

#include "fxpoweridlestatemachine.hpp"
#include "fxpoxinterface.hpp"

// @@SMVERIFY_SPLIT_BEGIN
//
// Treat these values as a bit field when comparing for known dropped events in
// the current state and treat them as values when they known transition events
// from state to the next.
//
enum FxPowerPolicyEvent {
    PwrPolInvalid                       = 0x00000000,
    PwrPolStart                         = 0x00000001,
    PwrPolStop                          = 0x00000002,
    PwrPolSx                            = 0x00000004,
    PwrPolS0                            = 0x00000008,
    PwrPolPowerDown                     = 0x00000010,
    PwrPolPowerUp                       = 0x00000020,
    PwrPolPowerDownIoStopped            = 0x00000040,
    PwrPolPowerUpHwStarted              = 0x00000080,
    PwrPolWakeArrived                   = 0x00000100,
    PwrPolWakeSuccess                   = 0x00000200,
    PwrPolWakeFailed                    = 0x00000400,
    PwrPolIoPresent                     = 0x00000800,
    PwrPolPowerTimeoutExpired           = 0x00001000,
    PwrPolS0IdlePolicyChanged           = 0x00002000,
    PwrPolSurpriseRemove                = 0x00004000,
    PwrPolUsbSelectiveSuspendCallback   = 0x00008000,
    PwrPolUsbSelectiveSuspendCompleted  = 0x00010000,
    PwrPolPowerDownFailed               = 0x00020000,
    PwrPolPowerUpFailed                 = 0x00040000,
    PwrPolImplicitPowerDown             = 0x00080000,
    PwrPolImplicitPowerDownFailed       = 0x00100000,
    PwrPolPowerUpNotSeen                = 0x00200000,
    PwrPolDevicePowerNotRequired        = 0x00400000,
    PwrPolDevicePowerRequired           = 0x00800000,
    PwrPolRemove                        = 0x01000000,
    PwrPolWakeInterruptFired            = 0x02000000,

    //
    // Not a real event, just a value that indicates all of the events which
    // goto the head of the queue and are always processed, even if the state is
    // locked.  This applies to the power policy owner state machine.
    //
    PwrPolPriorityEventsMask            = PwrPolPowerUp |
                                          PwrPolPowerDown |
                                          PwrPolPowerUpFailed |
                                          PwrPolPowerDownFailed |
                                          PwrPolPowerDownIoStopped |
                                          PwrPolPowerUpHwStarted |
                                          PwrPolImplicitPowerDown |
                                          PwrPolImplicitPowerDownFailed |
                                          PwrPolWakeArrived |
                                          PwrPolWakeSuccess |
                                          PwrPolWakeFailed |
                                          PwrPolPowerUpNotSeen |
                                          PwrPolUsbSelectiveSuspendCompleted |
                                          PwrPolWakeInterruptFired,

    //
    // Not a real event, just a value that indicates all of the events which
    // goto the head of the queue and are always processed, even if the state is
    // locked.  This applies to the not power policy owner state machine.
    //
    PwrPolNotOwnerPriorityEventsMask    = PwrPolPowerUp |
                                          PwrPolPowerUpFailed |
                                          PwrPolPowerDown |
                                          PwrPolPowerDownFailed,

    //
    // Not a real event, just a value that indicate all of the events which
    // should not be in the queue, if a similar event is already enqueued.
    //
    PowerPolSingularEventMask           = PwrPolS0IdlePolicyChanged |
    //
    // A device could have multiple wake interrupts that could each fire
    // this event.
    //
                                          PwrPolWakeInterruptFired,


    PwrPolNull                          = 0xFFFFFFFF,
};

//
// Bit packed ULONG.
//
union FxPwrPolStateInfo {
    struct {
        //
        // Indicates whether the state is open to all events
        //
        ULONG QueueOpen : 1;

        //
        // Bit of events we know we can drop in this state
        //
        ULONG KnownDroppedEvents : 31;
    } Bits;

    struct {
        //
        // Maps to the same bit location as QueueOpen.  Since we start
        // KnownDroppedEvents at the next bit, start our bits by at the next
        // bit as well.
        //
        ULONG Reserved : 1;

        //
        // These are defined so that we can easily tell in the debugger what
        // each set bit in KnownDroppedEvents maps to in the FxPowerPolicyEvent
        // enum
        //
        ULONG PwrPolStartKnown : 1;
        ULONG PwrPolStopKnown : 1;
        ULONG PwrPolSxKnown : 1;
        ULONG PwrPolS0Known : 1;
        ULONG PwrPolPowerDownKnown : 1;
        ULONG PwrPolPowerUpKnown : 1;
        ULONG PwrPolPowerDownIoStoppedKnown : 1;
        ULONG PwrPolPowerUpHwStartedKnown : 1;
        ULONG PwrPolWakeArrivedKnown : 1;
        ULONG PwrPolWakeSuccessKnown : 1;
        ULONG PwrPolWakeFailedKnown : 1;
        ULONG PwrPolIoPresentKnown : 1;
        ULONG PwrPolPowerTimeoutExpiredKnown : 1;
        ULONG PwrPolS0IdlePolicyChangedKnown : 1;
        ULONG PwrPolSurpriseRemoveKnown : 1;
        ULONG PwrPolUsbSelectiveSuspendCallbackKnown : 1;
        ULONG PwrPolUsbSelectiveSuspendCompletedKnown : 1;
        ULONG PwrPolPowerDownFailedKnown : 1;
        ULONG PwrPolPowerUpFailedKnown : 1;
    } BitsByName;
};

struct POWER_POLICY_EVENT_TARGET_STATE {
    FxPowerPolicyEvent PowerPolicyEvent;

    WDF_DEVICE_POWER_POLICY_STATE TargetState;

    EVENT_TRAP_FIELD
};

typedef const POWER_POLICY_EVENT_TARGET_STATE* CPPOWER_POLICY_EVENT_TARGET_STATE;

typedef
WDF_DEVICE_POWER_POLICY_STATE
(*PFN_POWER_POLICY_STATE_ENTRY_FUNCTION)(
    FxPkgPnp* This
    );

typedef struct POWER_POLICY_STATE_TABLE {
    //
    // Framework internal function to handle the transition into this state
    //
    PFN_POWER_POLICY_STATE_ENTRY_FUNCTION StateFunc;

    //
    // First state transition out of this state
    //
    POWER_POLICY_EVENT_TARGET_STATE FirstTargetState;

    //
    // Other state transitions out of this state if FirstTargetState is not
    // matched.  This is an array where we expect the final element to be
    // { PwrPolNull, WdfDevStatePwrPolNull }
    //
    CPPOWER_POLICY_EVENT_TARGET_STATE OtherTargetStates;

    //
    // Whether we allow transitions out of this state that are not D state
    // related events, ie if this is a green dot state, TRUE, if this is a red
    // dot state, FALSE.  D state events (PwrPolPowerUp, PwrPolPowerDown)
    // are never affected by the queue state and are always processed.
    //
    FxPwrPolStateInfo StateInfo;

} *PPOWER_POLICY_STATE_TABLE;

typedef const POWER_POLICY_STATE_TABLE* CPPOWER_POLICY_STATE_TABLE;

typedef
WDF_DEVICE_POWER_POLICY_STATE
(*PFN_NOT_POWER_POLICY_OWNER_STATE_ENTRY_FUNCTION)(
    FxPkgPnp* This
    );

typedef struct NOT_POWER_POLICY_OWNER_STATE_TABLE {
    //
    // The current power policy state that this entry applies to
    //
    WDF_DEVICE_POWER_POLICY_STATE CurrentTargetState;

    //
    // Framework internal function to handle the transition into this state
    //
    PFN_NOT_POWER_POLICY_OWNER_STATE_ENTRY_FUNCTION StateFunc;

    //
    // Only state transition out of this state
    //
    CPPOWER_POLICY_EVENT_TARGET_STATE TargetStates;

    UCHAR TargetStatesCount;

    BOOLEAN QueueOpen;

} *PNOT_POWER_POLICY_OWNER_STATE_TABLE;

typedef const NOT_POWER_POLICY_OWNER_STATE_TABLE* CPNOT_POWER_POLICY_OWNER_STATE_TABLE;

#if FX_STATE_MACHINE_VERIFY
#define MAX_PWR_POL_STATE_ENTRY_FN_RETURN_STATES    (5)

struct PWR_POL_STATE_ENTRY_FUNCTION_TARGET_STATE {
    //
    // Return value from state entry function
    //
    WDF_DEVICE_POWER_POLICY_STATE State;

    //
    // type of device the returning state applies to
    //
    FxStateMachineDeviceType  DeviceType;

    //
    // Info about the state transition
    //
    PSTR Comment;
};

struct PWR_POL_STATE_ENTRY_FN_RETURN_STATE_TABLE {
    //
    // array of state transitions caused by state entry function
    //
    PWR_POL_STATE_ENTRY_FUNCTION_TARGET_STATE TargetStates[MAX_PWR_POL_STATE_ENTRY_FN_RETURN_STATES];
};

typedef const PWR_POL_STATE_ENTRY_FN_RETURN_STATE_TABLE*  CPPWR_POL_STATE_ENTRY_FN_RETURN_STATE_TABLE;
#endif  // FX_STATE_MACHINE_VERIFY


// @@SMVERIFY_SPLIT_END

enum FxPowerPolicyConstants {
    FxPowerPolicyNoTimeout = 0,

    FxPowerPolicyDefaultTimeout = 5000,             // Timeout in milliseconds
};

enum CancelIrpCompletionOwnership {
    CancelOwnershipUnclaimed = 0,
    CancelOwnershipClaimed = 1,
};

enum FxPowerPolicySxWakeSettingsFlags {
    FxPowerPolicySxWakeDeviceEnabledFlag = 0x1,
    FxPowerPolicySxWakeChildrenArmedFlag = 0x2,
};

struct PolicySettings {
    PolicySettings()
    {
        WmiInstance = NULL;
        DxState = PowerDeviceD3;

        Enabled = Overridable = Set = Dirty = FALSE;
    }

    ~PolicySettings();

    //
    // Dx state to put the device in when the policy is applied
    //
    DEVICE_POWER_STATE DxState;

    FxWmiInstanceInternal* WmiInstance;

    BOOLEAN Enabled;

    BOOLEAN Overridable;

    BOOLEAN Set;

    BOOLEAN Dirty;
};

typedef struct _POX_SETTINGS {
    PFN_WDFDEVICE_WDM_POST_PO_FX_REGISTER_DEVICE
                            EvtDeviceWdmPostPoFxRegisterDevice;
    PFN_WDFDEVICE_WDM_PRE_PO_FX_UNREGISTER_DEVICE
                            EvtDeviceWdmPrePoFxUnregisterDevice;
    PPO_FX_COMPONENT Component;
    PPO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK ComponentActiveConditionCallback;
    PPO_FX_COMPONENT_IDLE_CONDITION_CALLBACK ComponentIdleConditionCallback;
    PPO_FX_COMPONENT_IDLE_STATE_CALLBACK ComponentIdleStateCallback;
    PPO_FX_POWER_CONTROL_CALLBACK PowerControlCallback;
    PVOID PoFxDeviceContext;
} POX_SETTINGS, *PPOX_SETTINGS;

class IdleTimeoutManagement {

private:
    //
    // This member is used to control whether or not the idle timeout is
    // determined by the power manager when running on Windows 8 and above.
    // The value of this member is some combination of the flags defined below.
    //
    LONG volatile m_IdleTimeoutStatus;

    //
    // Flags for the m_IdleTimeoutStatus member
    //
    //    IdleTimeoutStatusFrozen - This flag implies that the decision on
    //        whether the power manager determines the idle timeout is "frozen"
    //        and can no longer be changed. The decision is frozen during start
    //        IRP completion processing, just before WDF registers with the
    //        power manager.
    //
    //    IdleTimeoutSystemManaged - This flag implies that the power manager
    //        determines the idle timeout on Windows 8 and above. If this flag
    //        is not set, the idle timeout specified by the client driver is
    //        used.
    //
    //    IdleTimeoutPoxSettingsSpecified - This flag implies that the client
    //        driver has already specified the settings that need to be used
    //        when registering with the power framework. This flag is used to
    //        track that the settings are not specified more than once.
    //
    enum IdleTimeoutStatusFlag {
        IdleTimeoutStatusFrozen         = 0x00000001,
        IdleTimeoutSystemManaged        = 0x00000002,
        IdleTimeoutPoxSettingsSpecified = 0x00000004,
    };

    //
    // Result returned by the UpdateIdleTimeoutStatus() method
    //
    enum IdleTimeoutStatusUpdateResult {
        //
        // Flags were sucessfully updated
        //
        IdleTimeoutStatusFlagsUpdated,

        //
        // The flag we were trying to set was already set
        //
        IdleTimeoutStatusFlagAlreadySet,

        //
        // It is too late to set the flag. The flags have already been frozen.
        // Flags are frozen the first time a device is started.
        //
        IdleTimeoutStatusFlagsAlreadyFrozen,

        //
        // Flags are being set by multiple threads in parallel. This is not
        // supported.
        //
        IdleTimeoutStatusFlagsUnexpected
    };

    //
    // This member contains the client driver's settings that will be used when
    // we register with the power manager on Windows 8 and above.
    //
    PPOX_SETTINGS m_PoxSettings;

private:
    IdleTimeoutStatusUpdateResult
    UpdateIdleTimeoutStatus(
        __in IdleTimeoutStatusFlag Flag
        );

    CfxDevice *
    GetDevice(
        VOID
        );

public:
    IdleTimeoutManagement(
        VOID
        ) : m_IdleTimeoutStatus(0),
            m_PoxSettings(NULL)
    {
    }

    ~IdleTimeoutManagement(
        VOID
        )
    {
        BYTE * buffer = NULL;
        ULONG poxSettingsOffset;

        if (NULL != m_PoxSettings) {

            buffer = (BYTE*) m_PoxSettings;

            //
            // In the function FxPkgPnp::AssignPowerFrameworkSettings, we had
            // allocated a buffer which we need to free now. Note that
            // m_PoxSettings does not necessarily point to the beginning of the
            // buffer. It points to the POX_SETTINGS structure in the buffer,
            // which may or may not be in the beginning. If it is not in the
            // beginning, figure out where the beginning of the buffer is.
            //
            if (m_PoxSettings->Component != NULL) {
                //
                // The computation below won't overflow because we already
                // performed this computation successfully using safeint
                // functions in FxPkgPnp::AssignPowerFrameworkSettings.
                //
                poxSettingsOffset =
                        (sizeof(*(m_PoxSettings->Component->IdleStates)) *
                                (m_PoxSettings->Component->IdleStateCount)) +
                        (sizeof(*(m_PoxSettings->Component)));
            }
            else {
                poxSettingsOffset = 0;
            }

            //
            // Move to the beginning of the buffer
            //
            buffer = buffer - poxSettingsOffset;

            //
            // Free the buffer
            //
            MxMemory::MxFreePool(buffer);
        }
    }

    static
    BOOLEAN
    _SystemManagedIdleTimeoutAvailable(
        VOID
        );

    NTSTATUS
    UseSystemManagedIdleTimeout(
        __in PFX_DRIVER_GLOBALS DriverGlobals
        );

    VOID
    FreezeIdleTimeoutManagementStatus(
        __in PFX_DRIVER_GLOBALS DriverGlobals
        );

    BOOLEAN
    UsingSystemManagedIdleTimeout(
        VOID
        );

    NTSTATUS
    CommitPowerFrameworkSettings(
        __in PFX_DRIVER_GLOBALS DriverGlobals,
        __in PPOX_SETTINGS PoxSettings
        );

    BOOLEAN
    DriverSpecifiedPowerFrameworkSettings(
        VOID
        );

    PPOX_SETTINGS
    GetPowerFrameworkSettings(
        VOID
        )
    {
        return m_PoxSettings;
    }
};

struct IdlePolicySettings : PolicySettings {
    IdlePolicySettings(
        VOID
        ) : PolicySettings()
    {
        WakeFromS0Capable = FALSE;
        UsbSSCapable = FALSE;
        PowerUpIdleDeviceOnSystemWake = FALSE;
        UsbSSCapabilityKnown = FALSE;
    }

    //
    // TRUE if the device capable of waking from S0
    //
    BOOLEAN WakeFromS0Capable;

    //
    // This member is meaningful only if the WakeFromS0Capable member (above) is
    // TRUE. The WakeFromS0Capable member indicates whether or not wake-from-S0
    // is currently enabled. If wake-from-S0 is currently enabled, the
    // UsbSSCapable member indicates whether the wake-from-S0 support is generic
    // or USB SS specific. If wake-from-S0 is not enabled, the UsbSSCapable
    // member is ignored.
    //
    BOOLEAN UsbSSCapable;

    //
    // TRUE if we know whether the device supports generic wake or USB SS wake.
    // This value is initialized to FALSE and remains FALSE until the first time
    // that the driver specifies S0-idle settings with an idle capability value
    // of IdleCanWakeFromS0 or IdleUsbSelectiveSuspend. When the driver
    // specifies one of these idle capabilities, this value is set to TRUE and
    // remains TRUE for the lifetime of the device.
    //
    BOOLEAN UsbSSCapabilityKnown;

    //
    // TRUE if idle enabled device should be powered up even when idle,
    // when resuming from Sx
    //
    BOOLEAN PowerUpIdleDeviceOnSystemWake;

    //
    // Member to manage interactions with the power manager for S0-idle support
    // on Win8 and above
    //
    IdleTimeoutManagement m_TimeoutMgmt;
};

struct WakePolicySettings : PolicySettings {
    WakePolicySettings(
        VOID
        ) : PolicySettings()
    {
        ArmForWakeIfChildrenAreArmedForWake = FALSE;
        IndicateChildWakeOnParentWake = FALSE;
    }

    //
    // TRUE if the device should arm for wake when one or more children are
    // armed for wake.
    //
    BOOLEAN ArmForWakeIfChildrenAreArmedForWake;

    //
    // TRUE if the device should propagate the wake status to its children.
    //
    BOOLEAN IndicateChildWakeOnParentWake;
};

struct FxPowerPolicyOwnerSettings : public FxStump {

friend FxPowerPolicyMachine;

public:
    FxPowerPolicyOwnerSettings(
        __in FxPkgPnp* PkgPnp
        );

    ~FxPowerPolicyOwnerSettings(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Init(
        VOID
        );

    VOID
    CleanupPowerCallback(
        VOID
        );

    VOID
    IncrementChildrenArmedForWakeCount(
        VOID
        )
    {
        InterlockedIncrement(&m_ChildrenArmedCount);
    }

    VOID
    DecrementChildrenArmedForWakeCount(
        VOID
        )
    {
        InterlockedDecrement(&m_ChildrenArmedCount);
    }

protected:
    static
    MdCallbackFunctionType
    _PowerStateCallback;

public:
    FxPowerIdleMachine m_PowerIdleMachine;
    FxPoxInterface m_PoxInterface;






    FxPowerDeviceArmWakeFromS0          m_DeviceArmWakeFromS0;
    FxPowerDeviceArmWakeFromSx          m_DeviceArmWakeFromSx;

    FxPowerDeviceDisarmWakeFromS0       m_DeviceDisarmWakeFromS0;
    FxPowerDeviceDisarmWakeFromSx       m_DeviceDisarmWakeFromSx;

    FxPowerDeviceWakeFromS0Triggered    m_DeviceWakeFromS0Triggered;
    FxPowerDeviceWakeFromSxTriggered    m_DeviceWakeFromSxTriggered;

    FxUsbIdleInfo* m_UsbIdle;

    FxPkgPnp* m_PkgPnp;

    WakePolicySettings m_WakeSettings;

    IdlePolicySettings m_IdleSettings;

    //
    // Nibble packed structure.  Each D state is encoded 4 bits.  The S state is
    // used as the "index" within the ULONG.  PowerSystemUnspecified is the
    // first 4 bits of the first byte, etc. etc. ...
    //
    ULONG m_SystemToDeviceStateMap;

    //
    // The number of children who are in the D0 state.  If this count is > 0,
    // then this parent cannot idle out while in S0.  Note that each child also
    // has an explicit call to PowerReference against this device which is used
    // to control the idle timer for this device.
    //
    ULONG m_ChildrenPoweredOnCount;

    //
    // The number of children who are currently armed for wake.  This count
    // can be used by the the wake owner to determine whether wake should be
    // enabled or not for a parent stack if arming for wake depends on
    // children being armed for wake.
    //
    LONG m_ChildrenArmedCount;

    //
    // The status of the last wait wake IRP to complete in the stack
    //
    NTSTATUS m_WaitWakeStatus;

    //
    // Dx state to put the device into when an Sx irp arrives and the device is
    // not armed for wake from Sx. DEVICE_POWER_STATE values are used.
    //
    BYTE m_IdealDxStateForSx;

    //
    // Track power requests to assert if someone other than this driver sent it
    // and to determine if this driver has received the requested irp (to catch
    // someone above completing irp w/o sending to this driver)
    //
    BOOLEAN m_RequestedPowerUpIrp;
    BOOLEAN m_RequestedPowerDownIrp;
    BOOLEAN m_RequestedWaitWakeIrp;

    //
    // Tracks wake event being dropped
    //
    BOOLEAN m_WakeCompletionEventDropped;

    BOOLEAN m_PowerFailed;

    //
    // Indicates whether we can cause paging I/O by writing to the registry
    //
    BOOLEAN m_CanSaveState;

    //
    // Guard to stop children from powering up while the parent is in Dx or
    // about to transition into Dx.
    //
    BOOLEAN m_ChildrenCanPowerUp;

    //
    // TRUE if our device caused the machine to wake up.  Access to this value
    // is not synchronized between the parent and PDO.  The parent sets it to
    // TRUE upon successful completion of the WW irp and cleared after
    // EvtDeviceDisarmWakeFromSx.  If a PDO's WW IRP is completed within this
    // window, the PDO's WW IRP will have PoSetSystemWake called on it.  It is
    // acceptable if the PDO's WW IRP completion races with the clearing of the
    // value and is not set as a source of wake.
    //
    BOOLEAN m_SystemWakeSource;

protected:
    PCALLBACK_OBJECT m_PowerCallbackObject;

    PVOID m_PowerCallbackRegistration;

    LONG m_WaitWakeCancelCompletionOwnership;

};

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxPowerPolicyMachineStateHistory {
    struct {
        WDF_DEVICE_POWER_POLICY_STATE State1 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State2 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State3 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State4 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State5 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State6 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State7 : 16;
        WDF_DEVICE_POWER_POLICY_STATE State8 : 16;
    } S;

    USHORT History[FxPowerPolicyEventQueueDepth];
};

struct FxPowerPolicyMachine : public FxThreadedEventQueue {
    FxPowerPolicyMachine(
        VOID
        );

    ~FxPowerPolicyMachine(
        VOID
        );

    VOID
    UsbSSCallbackProcessingComplete(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    InitUsbSS(
        VOID
        );

    VOID
    SetWaitWakeUnclaimed(
        VOID
        )
    {
        m_Owner->m_WaitWakeCancelCompletionOwnership = CancelOwnershipUnclaimed;
    }

    BOOLEAN
    CanCompleteWaitWakeIrp(
        VOID
        )
    {
        //
        // We have 2 potential call sites racing on trying to complete the wait
        // wake irp.  The first is the cancelling call site.  The other is the
        // irp's completion routine.  What we want is for the *2nd* (and last)
        // call site to actually complete the irp.  This is why we check to see
        // if the result of the exchange is that the ownership is already claimed
        // (and not unclaimed as one might first be led to think).
        //
        if (InterlockedExchange(&m_Owner->m_WaitWakeCancelCompletionOwnership,
                                CancelOwnershipClaimed) == CancelOwnershipClaimed) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

public:
    FxPowerPolicyEvent m_Queue[FxPowerPolicyEventQueueDepth];

    FxPowerPolicyMachineStateHistory m_States;

    FxPowerPolicyOwnerSettings* m_Owner;

    union {
        ULONG m_SingularEventsPresent;

        union {
            //
            // These are defined so that we can easily tell in the debugger what
            // each set bit in m_SingularEventsPresent maps to in the
            // FxPowerPolicyEvent enum.
            //
            ULONG PwrPolStartKnown : 1;
            ULONG PwrPolStopKnown : 1;
            ULONG PwrPolSxKnown : 1;
            ULONG PwrPolS0Known : 1;
            ULONG PwrPolPowerDownKnown : 1;
            ULONG PwrPolPowerUpKnown : 1;
            ULONG PwrPolPowerDownIoStoppedKnown : 1;
            ULONG PwrPolPowerUpHwStartedKnown : 1;
            ULONG PwrPolWakeArrivedKnown : 1;
            ULONG PwrPolWakeSuccessKnown : 1;
            ULONG PwrPolWakeFailedKnown : 1;
            ULONG PwrPolIoPresentKnown : 1;
            ULONG PwrPolPowerTimeoutExpiredKnown : 1;
            ULONG PwrPolS0IdlePolicyChangedKnown : 1;
            ULONG PwrPolSurpriseRemoveKnown : 1;
            ULONG PwrPolUsbSelectiveSuspendCallbackKnown : 1;
            ULONG PwrPolUsbSelectiveSuspendCompletedKnown : 1;
            ULONG PwrPolPowerDownFailedKnown : 1;
            ULONG PwrPolPowerUpFailedKnown : 1;
        } m_SingularEventsPresentByName;
    };
};

#endif // _FXPOWERPOLICYSTATEMACHINE_H_
