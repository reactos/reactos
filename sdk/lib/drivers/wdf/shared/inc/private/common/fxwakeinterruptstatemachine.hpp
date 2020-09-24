//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXWAKEINTERRUPTSTATEMACHINE_H_
#define _FXWAKEINTERRUPTSTATEMACHINE_H_

//
// This is a magical number based on inspection.  If the queue overflows,
// it is OK to increase these numbers without fear of either dependencies or
// weird side affects.
//
const UCHAR FxWakeInterruptEventQueueDepth = 8;

enum FxWakeInterruptEvents {
    WakeInterruptEventInvalid                  = 0x00,
    WakeInterruptEventIsr                      = 0x01,
    WakeInterruptEventEnteringD0               = 0x02,
    WakeInterruptEventLeavingD0                = 0x04,
    WakeInterruptEventD0EntryFailed            = 0x08,
    WakeInterruptEventLeavingD0NotArmedForWake = 0x10,
    WakeInterruptEventNull                     = 0xFF,
};

enum FxWakeInterruptStates {
    WakeInterruptInvalid = 0,
    WakeInterruptFailed,
    WakeInterruptD0,
    WakeInterruptDx,
    WakeInterruptWaking,
    WakeInterruptInvokingEvtIsrPostWake,
    WakeInterruptCompletingD0,
    WakeInterruptInvokingEvtIsrInD0,
    WakeInterruptDxNotArmedForWake,
    WakeInterruptInvokingEvtIsrInDxNotArmedForWake,
    WakeInterruptMax
};

//
// Forward declaration
//
class FxWakeInterruptMachine;
class FxInterrupt;

typedef
_Must_inspect_result_
FxWakeInterruptStates
(*PFN_WAKE_INTERRUPT_STATE_ENTRY_FUNCTION)(
    __in FxWakeInterruptMachine* This
    );

struct FxWakeInterruptTargetState {
    FxWakeInterruptEvents WakeInterruptEvent;

    FxWakeInterruptStates WakeInterruptState;

#if FX_SUPER_DBG
    BOOLEAN EventDebugged;
#endif
};

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxWakeInterruptMachineStateHistory {
    struct {
        FxWakeInterruptStates State1 : 8;
        FxWakeInterruptStates State2 : 8;
        FxWakeInterruptStates State3 : 8;
        FxWakeInterruptStates State4 : 8;
        FxWakeInterruptStates State5 : 8;
        FxWakeInterruptStates State6 : 8;
        FxWakeInterruptStates State7 : 8;
        FxWakeInterruptStates State8 : 8;
    } S;

    UCHAR History[FxWakeInterruptEventQueueDepth];
};

struct FxWakeInterruptStateTable {
    PFN_WAKE_INTERRUPT_STATE_ENTRY_FUNCTION StateFunc;

    const FxWakeInterruptTargetState* TargetStates;

    ULONG TargetStatesCount;
};

class FxWakeInterruptMachine : public FxThreadedEventQueue {

    friend FxInterrupt;

public:
    FxWakeInterruptMachine(
        __in FxInterrupt * Interrupt
        );

    VOID
    ProcessEvent(
        __in FxWakeInterruptEvents Event
        );

    static
    VOID
    _ProcessEventInner(
        __inout FxPkgPnp* PkgPnp,
        __inout FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

private:
    VOID
    ProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    static
    FxWakeInterruptStates
    Waking(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    Dx(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    DxNotArmedForWake(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    Failed(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    InvokingEvtIsrPostWake(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    InvokingEvtIsrInDxNotArmedForWake(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    InvokingEvtIsrInD0(
        __in FxWakeInterruptMachine* This
        );

    static
    FxWakeInterruptStates
    CompletingD0(
        __in FxWakeInterruptMachine* This
        );

protected:
    //FxPkgPnp* m_PkgPnp;
    FxInterrupt* m_Interrupt;
    //
    // Set if the interrupt was left active during Dx transition to handle wake
    // events.
    //
    BOOLEAN m_ActiveForWake;
    BOOLEAN m_Claimed;
    MxEvent m_IsrEvent;

    // uses FxWakeInterruptStates values
    BYTE m_CurrentState;

    // three extra padded bytes are put in here by the compiler

    FxWakeInterruptEvents m_Queue[FxWakeInterruptEventQueueDepth];
    FxWakeInterruptMachineStateHistory m_States;

    static const FxWakeInterruptStateTable m_StateTable[];

    static const FxWakeInterruptTargetState m_FailedStates[];
    static const FxWakeInterruptTargetState m_D0States[];
    static const FxWakeInterruptTargetState m_DxStates[];
    static const FxWakeInterruptTargetState m_DxNotArmedForWakeStates[];
    static const FxWakeInterruptTargetState m_WakingStates[];
    static const FxWakeInterruptTargetState m_InvokingEvtIsrPostWakeStates[];
    static const FxWakeInterruptTargetState m_CompletingD0States[];
    static const FxWakeInterruptTargetState m_InvokingIsrInD0[];
};

#endif // _FXWAKEINTERRUPTSTATEMACHINE_H_
