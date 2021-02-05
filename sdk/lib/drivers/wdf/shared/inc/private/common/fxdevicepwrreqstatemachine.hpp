//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXDEVICEPWRREQUIRESTATEMACHINE_H_
#define _FXDEVICEPWRREQUIRESTATEMACHINE_H_

//
// This is a magical number based on inspection.  If the queue overflows,
// it is OK to increase these numbers without fear of either dependencies or
// weird side affects.
//
const UCHAR FxDevicePwrRequirementEventQueueDepth = 8;

enum FxDevicePwrRequirementEvents {
    DprEventInvalid                 = 0x00,
    DprEventRegisteredWithPox       = 0x01,
    DprEventUnregisteredWithPox     = 0x02,
    DprEventPoxRequiresPower        = 0x04,
    DprEventPoxDoesNotRequirePower  = 0x08,
    DprEventDeviceGoingToDx         = 0x10,
    DprEventDeviceReturnedToD0      = 0x20,
    DprEventNull                    = 0xFF,
};

enum FxDevicePwrRequirementStates {
    DprInvalid = 0,
    DprUnregistered,
    DprDevicePowerRequiredD0,
    DprDevicePowerNotRequiredD0,
    DprDevicePowerNotRequiredDx,
    DprDevicePowerRequiredDx,
    DprReportingDevicePowerAvailable,
    DprWaitingForDevicePowerRequiredD0,
    DprMax
};

//
// Forward declaration
//
class FxDevicePwrRequirementMachine;
class FxPoxInterface;

typedef
_Must_inspect_result_
FxDevicePwrRequirementStates
(*PFN_DEVICE_POWER_REQUIREMENT_STATE_ENTRY_FUNCTION)(
    __in FxDevicePwrRequirementMachine* This
    );

struct FxDevicePwrRequirementTargetState {
    FxDevicePwrRequirementEvents DprEvent;

    FxDevicePwrRequirementStates DprState;

#if FX_SUPER_DBG
    BOOLEAN EventDebugged;
#endif
};

//
// This type of union is done so that we can
// 1) shrink the array element to the smallest size possible
// 2) keep types within the structure so we can dump it in the debugger
//
union FxDevicePwrRequirementMachineStateHistory {
    struct {
        FxDevicePwrRequirementStates State1 : 8;
        FxDevicePwrRequirementStates State2 : 8;
        FxDevicePwrRequirementStates State3 : 8;
        FxDevicePwrRequirementStates State4 : 8;
        FxDevicePwrRequirementStates State5 : 8;
        FxDevicePwrRequirementStates State6 : 8;
        FxDevicePwrRequirementStates State7 : 8;
        FxDevicePwrRequirementStates State8 : 8;
    } S;

    UCHAR History[FxDevicePwrRequirementEventQueueDepth];
};

struct FxDevicePwrRequirementStateTable {
    PFN_DEVICE_POWER_REQUIREMENT_STATE_ENTRY_FUNCTION StateFunc;

    const FxDevicePwrRequirementTargetState* TargetStates;

    ULONG TargetStatesCount;
};

class FxDevicePwrRequirementMachine : public FxThreadedEventQueue {

public:
    FxDevicePwrRequirementMachine(
        __in FxPoxInterface * PoxInterface
        );

    VOID
    ProcessEvent(
        __in FxDevicePwrRequirementEvents Event
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
    FxDevicePwrRequirementStates
    PowerNotRequiredD0(
        __in FxDevicePwrRequirementMachine* This
        );

    static
    FxDevicePwrRequirementStates
    PowerRequiredDx(
        __in FxDevicePwrRequirementMachine* This
        );

    static
    FxDevicePwrRequirementStates
    ReportingDevicePowerAvailable(
        __in FxDevicePwrRequirementMachine* This
        );

protected:
    FxPoxInterface* m_PoxInterface;

    // uses FxDevicePwrRequirementStates values
    BYTE m_CurrentState;

    // three extra padded bytes are put in here by the compiler

    FxDevicePwrRequirementEvents m_Queue[FxDevicePwrRequirementEventQueueDepth];
    FxDevicePwrRequirementMachineStateHistory m_States;

    static const FxDevicePwrRequirementStateTable m_StateTable[];

    static const FxDevicePwrRequirementTargetState m_UnregisteredStates[];
    static const FxDevicePwrRequirementTargetState m_DevicePowerRequiredD0States[];
    static const FxDevicePwrRequirementTargetState m_DevicePowerNotRequiredD0States[];
    static const FxDevicePwrRequirementTargetState m_DevicePowerNotRequiredDxStates[];
    static const FxDevicePwrRequirementTargetState m_DevicePowerRequiredDxStates[];
    static const FxDevicePwrRequirementTargetState m_WaitingForDevicePowerRequiredD0States[];
};

#endif // _FXDEVICEPWRREQUIRESTATEMACHINE_H_
