#ifndef _FXSELFMANAGEDIOSTATEMACHINE_H_
#define _FXSELFMANAGEDIOSTATEMACHINE_H_

#include "common/fxstump.h"
#include "common/fxwaitlock.h"
#include "common/fxcxpnppowercallbacks.h"
#include "common/fxpnpcallbacks.h"


//
// This is a magical number based on inspection.  If the queue overflows,
// it is OK to increase these numbers without fear of either dependencies or
// weird side affects.
//
const UCHAR FxSelfManagedIoEventQueueDepth = 8;

enum FxSelfManagedIoEvents {
    SelfManagedIoEventInvalid   = 0x00,
    SelfManagedIoEventStart     = 0x01,
    SelfManagedIoEventCleanup   = 0x02,
    SelfManagedIoEventSuspend   = 0x04,
    SelfManagedIoEventFlush     = 0x08,
    SelfManagedIoEventNull      = 0xFF,
};

enum FxSelfManagedIoStates {
    FxSelfManagedIoInvalid = 0,
    FxSelfManagedIoCreated,
    FxSelfManagedIoInit,
    FxSelfManagedIoInitFailed,
    FxSelfManagedIoInitStartedFailedPost,
    FxSelfManagedIoStarted,
    FxSelfManagedIoSuspending,
    FxSelfManagedIoStopped,
    FxSelfManagedIoRestarting,
    FxSelfManagedIoRestartedFailedPost,
    FxSelfManagedIoFailed,
    FxSelfManagedIoFlushing,
    FxSelfManagedIoFlushed,
    FxSelfManagedIoCleanup,
    FxSelfManagedIoFinal,
    FxSelfManagedIoMax,
};

class FxSelfManagedIoMachine;
class FxPkgPnp;

typedef
_Must_inspect_result_
FxSelfManagedIoStates
(*PFN_SELF_MANAGED_IO_STATE_ENTRY_FUNCTION)(
    _In_  FxSelfManagedIoMachine*,
    _Inout_ PNTSTATUS Status,
    _Inout_opt_ FxCxCallbackProgress* Progress
    );

struct FxSelfManagedIoTargetState {
    FxSelfManagedIoEvents SelfManagedIoEvent;

    FxSelfManagedIoStates SelfManagedIoState;

#if FX_SUPER_DBG
    BOOLEAN EventDebugged;
#endif
};

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxSelfManagedIoMachineEventHistory {
    struct {
        FxSelfManagedIoEvents Event1 : 8;
        FxSelfManagedIoEvents Event2 : 8;
        FxSelfManagedIoEvents Event3 : 8;
        FxSelfManagedIoEvents Event4 : 8;
        FxSelfManagedIoEvents Event5 : 8;
        FxSelfManagedIoEvents Event6 : 8;
        FxSelfManagedIoEvents Event7 : 8;
        FxSelfManagedIoEvents Event8 : 8;
    } E;

    UCHAR History[FxSelfManagedIoEventQueueDepth];
};

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxSelfManagedIoMachineStateHistory {
    struct {
        FxSelfManagedIoStates State1 : 8;
        FxSelfManagedIoStates State2 : 8;
        FxSelfManagedIoStates State3 : 8;
        FxSelfManagedIoStates State4 : 8;
        FxSelfManagedIoStates State5 : 8;
        FxSelfManagedIoStates State6 : 8;
        FxSelfManagedIoStates State7 : 8;
        FxSelfManagedIoStates State8 : 8;
    } S;

    UCHAR History[FxSelfManagedIoEventQueueDepth];
};

struct FxSelfManagedIoStateTable {
    PFN_SELF_MANAGED_IO_STATE_ENTRY_FUNCTION StateFunc;

    const FxSelfManagedIoTargetState* TargetStates;

    ULONG TargetStatesCount;
};

class FxSelfManagedIoMachine : public FxStump {

public:
    FxSelfManagedIoMachine(
        _In_ FxPkgPnp* PkgPnp
        );

    static
    NTSTATUS
    _CreateAndInit(
        _Inout_ FxSelfManagedIoMachine** SelfManagedIoMachine,
        _In_ FxPkgPnp* PkgPnp
        );

    //
    // Sets event callbacks
    //
    VOID
    InitializeMachine(
        _In_ PWDF_PNPPOWER_EVENT_CALLBACKS Callbacks
        );

    _Must_inspect_result_
    NTSTATUS
    Start(
        _Out_opt_ FxCxCallbackProgress* Progress
        )
    {
        return ProcessEvent(SelfManagedIoEventStart, Progress);
    }

    _Must_inspect_result_
    NTSTATUS
    Suspend(
        VOID
        )
    {
        return ProcessEvent(SelfManagedIoEventSuspend, NULL);
    }


public:
    FxPnpDeviceSelfManagedIoCleanup     m_DeviceSelfManagedIoCleanup;
    FxPnpDeviceSelfManagedIoFlush       m_DeviceSelfManagedIoFlush;
    FxPnpDeviceSelfManagedIoInit        m_DeviceSelfManagedIoInit;
    FxPnpDeviceSelfManagedIoSuspend     m_DeviceSelfManagedIoSuspend;
    FxPnpDeviceSelfManagedIoRestart     m_DeviceSelfManagedIoRestart;


protected:
    FxWaitLockInternal m_StateMachineLock;

    FxPkgPnp* m_PkgPnp;

    // uses FxSelfManagedIoStates values
    BYTE m_CurrentState;

    UCHAR m_EventHistoryIndex;

    UCHAR m_StateHistoryIndex;

    FxSelfManagedIoMachineEventHistory m_Events;

    FxSelfManagedIoMachineStateHistory m_States;

    static const FxSelfManagedIoStateTable m_StateTable[];

    static const FxSelfManagedIoTargetState m_CreatedStates[];
    static const FxSelfManagedIoTargetState m_InitFailedStates[];
    static const FxSelfManagedIoTargetState m_StartedStates[];
    static const FxSelfManagedIoTargetState m_StoppedStates[];
    static const FxSelfManagedIoTargetState m_FailedStates[];
    static const FxSelfManagedIoTargetState m_FlushedStates[];

    // Functions

    _Must_inspect_result_
    NTSTATUS
    ProcessEvent(
        _In_ FxSelfManagedIoEvents Event,
        _Out_opt_ FxCxCallbackProgress* Progress
        );

    static
    FxSelfManagedIoStates
    Init(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    static
    FxSelfManagedIoStates
    Suspending(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    static
    FxSelfManagedIoStates
    Restarting(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    static
    FxSelfManagedIoStates
    Flushing(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    static
    FxSelfManagedIoStates
    Cleanup(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    static
    FxSelfManagedIoStates
    InitStartedFailedPost(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    static 
    FxSelfManagedIoStates
    RestartedFailedPost(
        _In_  FxSelfManagedIoMachine* This,
        _Inout_ PNTSTATUS Status,
        _Inout_opt_ FxCxCallbackProgress* Progress
        );

    WDFDEVICE
    GetDeviceHandle(
        VOID
        );

};

#endif //_FXSELFMANAGEDIOSTATEMACHINE_H_
