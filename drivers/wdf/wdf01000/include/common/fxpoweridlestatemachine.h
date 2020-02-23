#ifndef _FXPOWERIDLESTATEMACHINE_H_
#define _FXPOWERIDLESTATEMACHINE_H_

#include "common/fxstump.h"
#include "common/mxgeneral.h"
#include "common/mxtimer.h"
#include "common/fxtagtracker.h"
#include "common/fxwaitlock.h"

//
// This is a magical number based on inspection.  If the queue overflows,
// it is OK to increase these numbers without fear of either dependencies or
// weird side affects.
//
const UCHAR FxPowerIdleEventQueueDepth = 8;

enum FxPowerIdleEvents {
    // CAN BE REUSED                        = 0x0001,
    PowerIdleEventPowerUpFailed             = 0x0002,
    PowerIdleEventPowerUpComplete           = 0x0004,
    PowerIdleEventPowerDown                 = 0x0008,
    PowerIdleEventPowerDownFailed           = 0x0010,
    PowerIdleEventTimerExpired              = 0x0020,
    PowerIdleEventEnabled                   = 0x0040,
    PowerIdleEventDisabled                  = 0x0080,
    PowerIdleEventIoDecrement               = 0x0100,
    PowerIdleEventIoIncrement               = 0x0200,
    PowerIdleEventStart                     = 0x0400,
    PowerIdleEventStop                      = 0x0800,
    PowerIdleNull                           = 0x0000,
};

// begin_wpp config
// CUSTOM_TYPE(FxPowerIdleEvents, ItemEnum(FxPowerIdleEvents));
// end_wpp

enum FxPowerIdleStates {
    FxIdleStopped = 1,
    FxIdleStarted,
    FxIdleStartedPowerUp,
    FxIdleStartedPowerFailed,
    FxIdleDisabled,
    FxIdleCheckIoCount,
    FxIdleBusy,
    FxIdleDecrementIo,
    FxIdleStartTimer,
    FxIdleTimerRunning,
    FxIdleTimingOut,
    FxIdleTimedOut,
    FxIdleTimedOutIoIncrement,
    FxIdleTimedOutPowerDown,
    FxIdleTimedOutPowerDownFailed,
    FxIdleGoingToDx,
    FxIdleInDx,
    FxIdleInDxIoIncrement,
    FxIdleInDxPowerUpFailure,
    FxIdleInDxStopped,
    FxIdleInDxDisabled,
    FxIdleInDxEnabled,
    FxIdlePowerUp,
    FxIdlePowerUpComplete,
    FxIdleTimedOutDisabled,
    FxIdleTimedOutEnabled,
    FxIdleCancelTimer,
    FxIdleWaitForTimeout,
    FxIdleTimerExpired,
    FxIdleDisabling,
    FxIdleDisablingWaitForTimeout,
    FxIdleDisablingTimerExpired,
    FxIdlePowerFailedWaitForTimeout,
    FxIdlePowerFailed,
    FxIdleMax,
};

//
// NOTE:  if you change these flags (order, values, etc), you must also modify
//        m_FlagsByName to match your changes.
//
enum FxPowerIdleFlags {
    FxPowerIdleTimerEnabled        = 0x01,
    FxPowerIdleInDx                = 0x02,
    FxPowerIdleTimerCanceled       = 0x04,
    FxPowerIdleTimerStarted        = 0x08,
    FxPowerIdlePowerFailed         = 0x10,
    FxPowerIdleIsStarted           = 0x20,
    FxPowerIdleIoPresentSent       = 0x40,
    FxPowerIdleSendPnpPowerUpEvent = 0x80,
};

enum FxPowerReferenceFlags {
    FxPowerReferenceDefault             = 0x0,
    FxPowerReferenceSendPnpPowerUpEvent = 0x1
};

class FxPowerIdleMachine;

typedef
FxPowerIdleStates
(*PFN_POWER_IDLE_STATE_ENTRY_FUNCTION)(
    FxPowerIdleMachine*
    );

struct FxPowerIdleTargetState {
    FxPowerIdleEvents PowerIdleEvent;

    FxPowerIdleStates PowerIdleState;

#if FX_SUPER_DBG
    BOOLEAN EventDebugged;
#endif
};

struct FxIdleStateTable {
    PFN_POWER_IDLE_STATE_ENTRY_FUNCTION StateFunc;

    const FxPowerIdleTargetState* TargetStates;

    ULONG TargetStatesCount;
};

class FxPowerIdleMachine : public FxStump {

public:
    FxPowerIdleMachine(
        VOID
        );

    ~FxPowerIdleMachine(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Init(
        VOID
        );

    VOID
    Reset(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PowerReference(
        __in BOOLEAN WaitForD0,
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PCSTR File = NULL
        )
    {
        return PowerReferenceWorker(WaitForD0, FxPowerReferenceDefault, Tag, Line, File);
    }

    _Must_inspect_result_
    NTSTATUS
    IoIncrementWithFlags(
        __in FxPowerReferenceFlags Flags,
        __out_opt PULONG Count = NULL
        );

    VOID
    WaitForD0(
        VOID
        )
    {
        m_D0NotificationEvent.EnterCRAndWaitAndLeave();
    }

    VOID
    IoDecrement(
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PCSTR File = NULL
        );


protected:

    static
    MdDeferredRoutineType
    _PowerTimeoutDpcRoutine;

    
public:
    LARGE_INTEGER m_PowerTimeout;

protected:
    //
    // Lock which guards state
    //
    MxLock m_Lock;

    //
    // Number of pending requests which require being in D0
    //
    ULONG m_IoCount;

    //
    // Tracks power references and releases.
    //
    FxTagTracker* m_TagTracker;

    //
    // Timer which will be set when the I/O count goes to zero
    //
    MxTimer m_PowerTimeoutTimer;

    //
    // Event to wait on when transitioning from Dx to D0
    //
    FxCREvent m_D0NotificationEvent;

    union {
        //
        // Combintaion of FxPowerIdleFlags enum values
        //
        UCHAR m_Flags;

        //
        // Not used in the code.  Here so that you can easily decode m_Flags in
        // the debugger without needing the enum definition.
        //
        struct  {
            UCHAR TimerEnabled : 1;
            UCHAR InDx : 1;
            UCHAR TimerCanceled : 1;
            UCHAR TimerStarted : 1;
            UCHAR TimerPowerFailed : 1;
            UCHAR IsStarted : 1;
            UCHAR IoPresentSent : 1;
            UCHAR SendPnpPowerUpEvent : 1;
        } m_FlagsByName;
    };

    //
    // Index into m_EventHistory where to place the next value
    //
    UCHAR m_EventHistoryIndex;

    //
    // Index into m_StateHistory where to place the next value
    UCHAR m_StateHistoryIndex;

    //
    // our current state
    //
    FxPowerIdleStates m_CurrentIdleState;

    //
    // Circular history of events fed into this state machine
    //
    FxPowerIdleEvents m_EventHistory[FxPowerIdleEventQueueDepth];

    //
    // Circular history of states the state machine was in
    //
    FxPowerIdleStates m_StateHistory[FxPowerIdleEventQueueDepth];

    static const FxIdleStateTable m_StateTable[];


    VOID
    SendD0Notification(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PowerReferenceWorker(
        __in BOOLEAN WaitForD0,
        __in FxPowerReferenceFlags Flags,
        __in_opt PVOID Tag = NULL,
        __in_opt LONG Line = 0,
        __in_opt PCSTR File = NULL
        );

    VOID
    ProcessEventLocked(
        __in FxPowerIdleEvents Event
        );

    BOOLEAN
    InD0Locked(
        VOID
        )
    {
        return m_D0NotificationEvent.ReadState() ? TRUE : FALSE;
    }
};

#endif //_FXPOWERIDLESTATEMACHINE_H_
