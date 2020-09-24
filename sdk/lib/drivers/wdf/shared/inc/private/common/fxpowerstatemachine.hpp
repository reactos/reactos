//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXPOWERSTATEMACHINE_H_
#define _FXPOWERSTATEMACHINE_H_

// @@SMVERIFY_SPLIT_BEGIN
//
// Treat these values as a bit field when comparing for known dropped events in
// the current state and treat them as values when they known transition events
// from state to the next.
//
enum FxPowerEvent {
    PowerEventInvalid               = 0x0000,
    PowerD0                         = 0x0001,
    PowerDx                         = 0x0002,
    PowerWakeArrival                = 0x0004,
    PowerWakeSucceeded              = 0x0008,
    PowerWakeFailed                 = 0x0010,
    PowerWakeCanceled               = 0x0020,
    PowerImplicitD0                 = 0x0040,
    PowerImplicitD3                 = 0x0080,
    PowerParentToD0                 = 0x0100,
    PowerMarkPageable               = 0x0200,
    PowerMarkNonpageable            = 0x0400,
    PowerCompleteD0                 = 0x0800,
    PowerCompleteDx                 = 0x1000,
    PowerWakeInterruptCompleteTransition
                                    = 0x2000,

    //
    // Not a real event, just a value that indicates all of the events which
    // goto the head of the queue and are always processed, even if the state is
    // locked.
    //
    PowerPriorityEventsMask         = PowerParentToD0 |
                                      PowerCompleteD0 |
                                      PowerCompleteDx |
                                      PowerWakeInterruptCompleteTransition,

    //
    // Not a real event, just a value that indicate all of the events which
    // should not be in the queue, if a similar event is already enqueued.
    //
    PowerSingularEventMask          = PowerParentToD0,

    PowerEventMaximum               = 0xFFFFFFFF,
};

union FxPowerStateInfo {
    struct {
        //
        // Is this a state where we rest and wait for events to bring us out
        // of that state.
        //
        // NOTE:  this value is purely notational, we don't use it anywhere in
        //        the state machine.  If need be, reuse this slot for something
        //        else without worry.
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
        // each set bit in KnownDroppedEvents maps to in the FxPowerEvent enum
        //
        ULONG PowerD0Known : 1;
        ULONG PowerDxKnown : 1;
        ULONG PowerWakeArrivalKnown : 1;
        ULONG PowerWakeSucceededKnown : 1;
        ULONG PowerWakeFailedKnown : 1;
        ULONG PowerWakeCanceledKnown : 1;
        ULONG PowerImplicitD0Known : 1;
        ULONG PowerImplicitD3Known : 1;
        ULONG PowerParentToD0Known : 1;
        ULONG PowerMarkPageableKnown : 1;
        ULONG PowerMarkNonpageableKnown : 1;
        ULONG PowerCompleteD0Known : 1;
        ULONG PowerCompleteDxKnown : 1;
    } BitsByName;
};


struct POWER_EVENT_TARGET_STATE {
    FxPowerEvent            PowerEvent;

    WDF_DEVICE_POWER_STATE  TargetState;

    EVENT_TRAP_FIELD
};

typedef const POWER_EVENT_TARGET_STATE* CPPPOWER_EVENT_TARGET_STATE;

typedef
WDF_DEVICE_POWER_STATE
(*PFN_POWER_STATE_ENTRY_FUNCTION)(
    FxPkgPnp*
    );

typedef struct POWER_STATE_TABLE {
    //
    // Function called when the state is entered
    //
    PFN_POWER_STATE_ENTRY_FUNCTION  StateFunc;

    //
    // First state transition out of this state
    //
    POWER_EVENT_TARGET_STATE FirstTargetState;

    //
    // Other state transitions out of this state if FirstTargetState is not
    // matched.  This is an array where we expect the final element to be
    // { PowerEventMaximum, WdfDevStatePowerNull }
    //
    CPPPOWER_EVENT_TARGET_STATE OtherTargetStates;

    FxPowerStateInfo StateInfo;

} *PPOWER_STATE_TABLE;

typedef const POWER_STATE_TABLE* CPPOWER_STATE_TABLE;

#if FX_STATE_MACHINE_VERIFY
#define MAX_POWER_STATE_ENTRY_FN_RETURN_STATES    (5)

struct POWER_STATE_ENTRY_FUNCTION_TARGET_STATE {
    //
    // Return value from state entry function
    //
    WDF_DEVICE_POWER_STATE State;

    //
    // type of device the returning state applies to
    //
    FxStateMachineDeviceType  DeviceType;

    //
    // Info about the state transition
    //
    PSTR Comment;
};

struct POWER_STATE_ENTRY_FN_RETURN_STATE_TABLE {
    //
    // array of state transitions caused by state entry function
    //
    POWER_STATE_ENTRY_FUNCTION_TARGET_STATE TargetStates[MAX_POWER_STATE_ENTRY_FN_RETURN_STATES];
};

typedef const POWER_STATE_ENTRY_FN_RETURN_STATE_TABLE*  CPPOWER_STATE_ENTRY_FN_RETURN_STATE_TABLE;
#endif  // FX_STATE_MACHINE_VERIFY

// @@SMVERIFY_SPLIT_END

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxPowerMachineEventQueue {
    struct {
        FxPowerEvent Event1 : 16;
        FxPowerEvent Event2 : 16;
        FxPowerEvent Event3 : 16;
        FxPowerEvent Event4 : 16;
        FxPowerEvent Event5 : 16;
        FxPowerEvent Event6 : 16;
        FxPowerEvent Event7 : 16;
        FxPowerEvent Event8 : 16;
    } E;

    USHORT Events[PowerEventQueueDepth];
};

//
// Same as FxPowerMachineEventQueue
//
union FxPowerMachineStateHistory {
    struct {
        WDF_DEVICE_POWER_STATE State1 : 16;
        WDF_DEVICE_POWER_STATE State2 : 16;
        WDF_DEVICE_POWER_STATE State3 : 16;
        WDF_DEVICE_POWER_STATE State4 : 16;
        WDF_DEVICE_POWER_STATE State5 : 16;
        WDF_DEVICE_POWER_STATE State6 : 16;
        WDF_DEVICE_POWER_STATE State7 : 16;
        WDF_DEVICE_POWER_STATE State8 : 16;
    } S;

    USHORT History[PowerEventQueueDepth];
};

struct FxPowerMachine : public FxThreadedEventQueue {
    FxPowerMachine(
        VOID
        ) : FxThreadedEventQueue(PowerEventQueueDepth)
    {
        //
        // m_WaitWakeLock can not be initialized here since Initiliaze can
        // return failure for UM. It's now being initialized in Init() function.
        //

        InitializeListHead(&m_WaitWakeIrpToBeProcessedList);

        RtlZeroMemory(&m_Queue, sizeof(m_Queue));
        RtlZeroMemory(&m_States, sizeof(m_States));

        m_States.History[IncrementHistoryIndex()] = WdfDevStatePowerObjectCreated;
        m_IoCallbackFailure = FALSE;
        m_PowerDownFailure = FALSE;
        m_SingularEventsPresent = 0x0;
    }

    _Must_inspect_result_
    NTSTATUS
    Init(
        __inout FxPkgPnp* Pnp,
        __in PFN_PNP_EVENT_WORKER WorkerRoutine
        );

    FxPowerMachineEventQueue m_Queue;

    FxPowerMachineStateHistory m_States;

    //
    // Lock to guard wait wake irp
    //
    MxLock m_WaitWakeLock;

    //
    // List of wait wake requests which have either been completed or cancelled
    // and we are waiting for the state machine to process and complete the irp.
    //
    // We require a list of irps (instead of just storage for one irp) because
    // the power policy owner might be misbehaving and sending wake requests
    // successively down the stack and we want the state machine to be able
    // to keep track of all the requests.
    //
    LIST_ENTRY m_WaitWakeIrpToBeProcessedList;

    union {
        USHORT m_SingularEventsPresent;

        union {
            //
            // These are defined so that we can easily tell in the debugger what
            // each set bit in m_SingularEventsPresent maps to in the
            // FxPowerEvent enum.
            //
            USHORT PowerD0Known : 1;
            USHORT PowerDxKnown : 1;
            USHORT PowerWakeArrivalKnown : 1;
            USHORT PowerWakeSucceededKnown : 1;
            USHORT PowerWakeFailedKnown : 1;
            USHORT PowerWakeCanceledKnown : 1;
            USHORT PowerImplicitD0Known : 1;
            USHORT PowerImplicitD3Known : 1;
            USHORT PowerParentToD0Known : 1;
            USHORT PowerMarkPageableKnown : 1;
            USHORT PowerMarkNonpageableKnown : 1;
            USHORT PowerCompleteD0Known : 1;
            USHORT PowerCompleteDxKnown : 1;
        } m_SingularEventsPresentByName;
    };

    BOOLEAN m_IoCallbackFailure;

    BOOLEAN m_PowerDownFailure;
};

#endif // _FXPOWERSTATEMACHINE_H_
