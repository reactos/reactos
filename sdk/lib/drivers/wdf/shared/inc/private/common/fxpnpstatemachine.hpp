//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXPNPSTATEMACHINE_H_
#define _FXPNPSTATEMACHINE_H_

// @@SMVERIFY_SPLIT_BEGIN

typedef
WDF_DEVICE_PNP_STATE
(*PFN_PNP_STATE_ENTRY_FUNCTION)(
    FxPkgPnp* This
    );

//
// Treat these values as a bit field when comparing for known dropped events in
// the current state and treat them as values when they known transition events
// from state to the next.
//
enum FxPnpEvent {
    PnpEventInvalid                 = 0x000000,
    PnpEventAddDevice               = 0x000001,
    PnpEventStartDevice             = 0x000002,
    PnpEventStartDeviceComplete     = 0x000004,
    PnpEventStartDeviceFailed       = 0x000008,
    PnpEventQueryRemove             = 0x000010,
    PnpEventQueryStop               = 0x000020,
    PnpEventCancelRemove            = 0x000040,
    PnpEventCancelStop              = 0x000080,
    PnpEventStop                    = 0x000100,
    PnpEventRemove                  = 0x000200,
    PnpEventSurpriseRemove          = 0x000400,
    PnpEventEject                   = 0x000800,
    PnpEventPwrPolStopped           = 0x001000,
    PnpEventPwrPolStopFailed        = 0x002000,
    PnpEventPowerUpFailed           = 0x004000,
    PnpEventPowerDownFailed         = 0x008000,
    PnpEventParentRemoved           = 0x010000,
    PnpEventChildrenRemovalComplete = 0x020000,
    PnpEventPwrPolStarted           = 0x040000,
    PnpEventPwrPolStartFailed       = 0x080000,
    PnpEventDeviceInD0              = 0x100000,
    PnpEventPwrPolRemoved           = 0x200000,

    //
    // Not a real event, just a value that shows all of the events which have
    // queued the pnp irp.  If we drop one of these events, we *must* complete
    // the pended pnp irp.  See PnpProcessEventInner.
    //
    PnpEventPending                 = PnpEventStartDeviceComplete |
                                      PnpEventQueryRemove         |
                                      PnpEventQueryStop           |
                                      PnpEventCancelRemove        |
                                      PnpEventCancelStop          |
                                      PnpEventStop                |
                                      PnpEventSurpriseRemove      |
                                      PnpEventEject,

    //
    // Not a real event, just a value that indicates all of the events which
    // goto the head of the queue and are always processed, even if the state is
    // locked.
    //
    PnpPriorityEventsMask            = PnpEventPwrPolStarted |
                                       PnpEventPwrPolStartFailed |
                                       PnpEventPwrPolStopped |
                                       PnpEventPwrPolStopFailed |
                                       PnpEventDeviceInD0 |
                                       PnpEventPwrPolRemoved,

    PnpEventNull                    = 0xFFFFFFFF,
};

//
// Bit packed ULONG.
//
union FxPnpStateInfo {
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
        // each set bit in KnownDroppedEvents maps to in the FxPnpEvent enum
        //
        ULONG PnpEventAddDeviceKnown : 1;
        ULONG PnpEventStartDeviceKnown : 1;
        ULONG PnpEventStartDeviceCompleteKnown : 1;
        ULONG PnpEventStartDeviceFailedKnown : 1;
        ULONG PnpEventQueryRemoveKnown : 1;
        ULONG PnpEventQueryStopKnown : 1;
        ULONG PnpEventCancelRemoveKnown : 1;
        ULONG PnpEventCancelStopKnown : 1;
        ULONG PnpEventStopKnown : 1;
        ULONG PnpEventRemoveKnown : 1;
        ULONG PnpEventSurpriseRemoveKnown : 1;
        ULONG PnpEventEjectKnown : 1;
        ULONG PnpEventPwrPolStopped : 1;
        ULONG PnpEventPwrPolStopFailed : 1;
        ULONG PnpEventPowerUpFailedKnown : 1;
        ULONG PnpEventPowerDownFailedKnown : 1;
        ULONG PnpEventParentRemovedKnown : 1;
        ULONG PnpEventChildrenRemovalCompleteKnown : 1;
        ULONG PnpEventPwrPolStarted : 1;
        ULONG PnpEventPwrPolStartFailed : 1;
    } BitsByName;
};

struct PNP_EVENT_TARGET_STATE {
    FxPnpEvent PnpEvent;

    WDF_DEVICE_PNP_STATE TargetState;

    EVENT_TRAP_FIELD
};

typedef const PNP_EVENT_TARGET_STATE* CPPNP_EVENT_TARGET_STATE;

struct PNP_STATE_TABLE {
    //
    // Framework internal function
    //
    PFN_PNP_STATE_ENTRY_FUNCTION StateFunc;

    PNP_EVENT_TARGET_STATE FirstTargetState;

    CPPNP_EVENT_TARGET_STATE OtherTargetStates;

    FxPnpStateInfo StateInfo;

};

typedef const PNP_STATE_TABLE* CPPNP_STATE_TABLE;

#if FX_STATE_MACHINE_VERIFY
#define MAX_PNP_STATE_ENTRY_FN_RETURN_STATES            (5)

struct PNP_STATE_ENTRY_FUNCTION_TARGET_STATE {
    //
    // Return value from state entry function
    //
    WDF_DEVICE_PNP_STATE State;

    //
    // type of device the returning state applies to
    //
    FxStateMachineDeviceType  DeviceType;

    //
    // Info about the state transition
    //
    PSTR Comment;
};

typedef const PNP_STATE_ENTRY_FUNCTION_TARGET_STATE* CPPNP_STATE_ENTRY_FUNCTION_TARGET_STATE;

struct PNP_STATE_ENTRY_FN_RETURN_STATE_TABLE {
    //
    // array of state transitions caused by state entry function
    //
    PNP_STATE_ENTRY_FUNCTION_TARGET_STATE TargetStates[MAX_PNP_STATE_ENTRY_FN_RETURN_STATES];
};

typedef const PNP_STATE_ENTRY_FN_RETURN_STATE_TABLE*  CPPNP_STATE_ENTRY_FN_RETURN_STATE_TABLE;
#endif //FX_STATE_MACHINE_VERIFY

// @@SMVERIFY_SPLIT_END

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxPnpMachineStateHistory {
    struct {
        WDF_DEVICE_PNP_STATE State1 : 16;
        WDF_DEVICE_PNP_STATE State2 : 16;
        WDF_DEVICE_PNP_STATE State3 : 16;
        WDF_DEVICE_PNP_STATE State4 : 16;
        WDF_DEVICE_PNP_STATE State5 : 16;
        WDF_DEVICE_PNP_STATE State6 : 16;
        WDF_DEVICE_PNP_STATE State7 : 16;
        WDF_DEVICE_PNP_STATE State8 : 16;
    } S;

    USHORT History[PnpEventQueueDepth];
};


struct FxPnpMachine : public FxWorkItemEventQueue {
    FxPnpMachine(
        VOID
        ) : FxWorkItemEventQueue(PnpEventQueueDepth)
    {
        RtlZeroMemory(&m_Queue[0], sizeof(m_Queue));
        RtlZeroMemory(&m_States, sizeof(m_States));

        m_States.History[IncrementHistoryIndex()] = WdfDevStatePnpObjectCreated;
        m_FireAndForget = FALSE;
    }

    FxPnpEvent m_Queue[PnpEventQueueDepth];

    FxPnpMachineStateHistory m_States;

    BOOLEAN m_FireAndForget;
};

#endif // _FXPNPSTATEMACHINE_H_
