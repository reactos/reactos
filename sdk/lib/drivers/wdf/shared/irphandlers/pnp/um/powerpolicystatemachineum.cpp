/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerPolicyStateMachineUM.cpp

Abstract:


Environment:

    User mode only

Revision History:

--*/

#include "../pnppriv.hpp"

#include "FxUsbIdleInfo.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerPolicyStateMachineUM.tmh"
#endif
}

VOID
FxPkgPnp::PowerPolicyUpdateSystemWakeSource(
    __in FxIrp* Irp
    )
/*++

Routine Description:
    Gets source of wake if OS supports this.

Arguments:
    Irp

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    UNREFERENCED_PARAMETER(Irp);

    if (m_Device->IsPdo()) {

        pFxDriverGlobals = GetDriverGlobals();

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "For PDOs, FxPkgPnp::PowerPolicyUpdateSystemWakeSource should NOT "
            "be a no-op!");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

BOOLEAN
FxPkgPnp::ShouldProcessPowerPolicyEventOnDifferentThread(
    __in KIRQL CurrentIrql,
    __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
    )
/*++
Routine Description:

    This function returns whether the power policy state machine should process
    the current event on the same thread or on a different one.

    This function has been added to work around a bug in the state machines.
    The idle state machine always calls PowerPolicyProcessEvent with the idle
    state machine lock held. Some events sent by the idle state machine can
    cause the power policy state machine to invoke
    FxPowerIdleMachine::QueryReturnToIdle().
    FxPowerIdleMachine::QueryReturnToIdle() will try to acquire the idle state
    machine lock, which is already being held, so it will result in a recursive
    acquire of the idle state machine lock.

    The above bug only affects UMDF, but not KMDF. In KMDF, the idle state
    machine lock is a spinlock. When PowerPolicyProcessEvent is called, it is
    called with the spinlock held and hence at dispatch level. Note that if
    called at a non-passive IRQL, PowerPolicyProcessEvent will always queue a
    work item to process the event at passive IRQL later. Queuing a work item
    forces processing to happen on a different thread and hence we don't
    attempt to recursively acquire the spinlock. On the other hand, with UMDF
    we are always at passive IRQL and hence we process the event on the same
    thread and run into the recursive acquire problem.








Arguments:

    CurrentIrql - The current IRQL

    CallerSpecifiedProcessingOnDifferentThread - Whether or not caller of
        PowerPolicyProcessEvent specified that the event be processed on a
        different thread.

Returns:
    TRUE if the power policy state machine should process the event on a
       different thread.

    FALSE if the power policy state machine should process the event on the
       same thread

--*/
{
    //
    // For UMDF, we ignore the IRQL and just do what the caller of
    // PowerPolicyProcessEvent wants.
    //
    UNREFERENCED_PARAMETER(CurrentIrql);

    return CallerSpecifiedProcessingOnDifferentThread;
}

_Must_inspect_result_
NTSTATUS
FxUsbIdleInfo::Initialize(
    VOID
    )
{
    HRESULT hr;
    NTSTATUS status;
    FxDevice* device;
    IWudfDeviceStack *devStack;

    device = ((FxPkgPnp*)m_CallbackInfo.IdleContext)->GetDevice();
    devStack = device->GetDeviceStack();

    hr = devStack->InitializeUsbSS();
    if (S_OK == hr) {
        status = STATUS_SUCCESS;
    }
    else {
        PUMDF_VERSION_DATA driverVersion = devStack->GetMinDriverVersion();
        BOOL preserveCompat =
             devStack->ShouldPreserveIrpCompletionStatusCompatibility();

        status = CHostFxUtil::NtStatusFromHr(hr,
                                             driverVersion->MajorNumber,
                                             driverVersion->MinorNumber,
                                             preserveCompat);
    }

    return status;
}

VOID
FxPkgPnp::PowerPolicySubmitUsbIdleNotification(
    VOID
    )
{
    //
    // This will be set to TRUE if USBSS completion event gets dropped.
    //
    m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_EventDropped = FALSE;

    m_Device->GetDeviceStack()->SubmitUsbIdleNotification(
        &(m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_CallbackInfo),
        _PowerPolicyUsbSelectiveSuspendCompletionRoutine,
        this);
}

VOID
FxPkgPnp::PowerPolicyCancelUsbSS(
    VOID
    )
{
    m_Device->GetDeviceStack()->CancelUsbSS();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FxUsbIdleInfo::_UsbIdleCallback(
    __in PVOID Context
    )
{
    FxPkgPnp* pPkgPnp;

    pPkgPnp = (FxPkgPnp*) Context;

    DoTraceLevelMessage(
        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Entering USB Selective Suspend Idle callback");

    pPkgPnp->PowerPolicyProcessEvent(PwrPolUsbSelectiveSuspendCallback);
}


VOID
FxPowerPolicyMachine::UsbSSCallbackProcessingComplete(
    VOID
    )
{
    FxDevice* device = m_PkgPnp->GetDevice();

    DoTraceLevelMessage(
        m_PkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "USB Selective Suspend Idle callback processing is complete");

    device->GetDeviceStack()->SignalUsbSSCallbackProcessingComplete();
}

