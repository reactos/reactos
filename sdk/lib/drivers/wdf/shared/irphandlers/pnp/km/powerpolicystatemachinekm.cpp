/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerPolicyStateMachineKM.cpp

Abstract:


Environment:

    Kernel mode only

Revision History:

--*/

#include "..\pnppriv.hpp"

#if FX_IS_KERNEL_MODE
#include <usbdrivr.h>
#endif

#include "FxUsbIdleInfo.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerPolicyStateMachineKM.tmh"
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
    //
    // Check to see if this device caused the machine to wake up
    //
    m_PowerPolicyMachine.m_Owner->m_SystemWakeSource =
        PoGetSystemWake(Irp->GetIrp());

    if (m_PowerPolicyMachine.m_Owner->m_SystemWakeSource) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p WW !irp 0x%p is a source of "
            "wake",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            Irp->GetIrp());
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































Arguemnts:

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
    // For KMDF, we ignore what the caller of PowerPolicyProcessEvent specified
    // (which should always be FALSE, BTW) and base our decision on the current
    // IRQL. If we are running at PASSIVE_LEVEL, we process on the same thread
    // else we queue a work item.
    //
    UNREFERENCED_PARAMETER(CallerSpecifiedProcessingOnDifferentThread);

    ASSERT(FALSE == CallerSpecifiedProcessingOnDifferentThread);

    return (CurrentIrql == PASSIVE_LEVEL) ? FALSE : TRUE;
}

_Must_inspect_result_
NTSTATUS
FxUsbIdleInfo::Initialize(
    VOID
    )
{
    MdIrp pIrp;
    MxDeviceObject attachedDevObj;

    attachedDevObj.SetObject(((FxPkgPnp*)m_CallbackInfo.IdleContext)->GetDevice()->GetAttachedDevice());

    pIrp = FxIrp::AllocateIrp(attachedDevObj.GetStackSize());

    if (pIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_IdleIrp.SetIrp(pIrp);

    return STATUS_SUCCESS;
}

VOID
FxPkgPnp::PowerPolicySubmitUsbIdleNotification(
    VOID
    )
{
    FxIrp* usbIdleIrp;

    //
    // This will be set to TRUE if USBSS completion event gets dropped.
    //
    m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_EventDropped = FALSE;

    usbIdleIrp = &m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_IdleIrp;

    usbIdleIrp->Reuse();

    usbIdleIrp->SetCompletionRoutineEx(
        m_Device->GetDeviceObject(),
        _PowerPolicyUsbSelectiveSuspendCompletionRoutine,
        this);

    usbIdleIrp->SetMajorFunction(IRP_MJ_INTERNAL_DEVICE_CONTROL);
    usbIdleIrp->SetParameterIoctlCode(
        IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION);
    usbIdleIrp->SetParameterIoctlInputBufferLength(
        sizeof(m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_CallbackInfo));
    usbIdleIrp->SetParameterIoctlType3InputBuffer(
        (PVOID) &m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_CallbackInfo);

    m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_IdleIrp.CallDriver(
        m_Device->GetAttachedDevice()
        );
}

VOID
FxPkgPnp::PowerPolicyCancelUsbSS(
    VOID
    )
{
    m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_IdleIrp.Cancel();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FxUsbIdleInfo::_UsbIdleCallback(
    __in PVOID Context
    )
{
    FxPkgPnp* pPkgPnp;
    FxUsbIdleInfo* pThis;
    FxCREvent event;

    pPkgPnp = (FxPkgPnp*) Context;

    DoTraceLevelMessage(
        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Entering USB Selective Suspend Idle callback");

    pThis = pPkgPnp->m_PowerPolicyMachine.m_Owner->m_UsbIdle;

    ASSERT(pThis->m_IdleCallbackEvent == NULL);
    pThis->m_IdleCallbackEvent = event.GetSelfPointer();

    pPkgPnp->PowerPolicyProcessEvent(PwrPolUsbSelectiveSuspendCallback);

    event.EnterCRAndWaitAndLeave();
    pThis->m_IdleCallbackEvent = NULL;

    DoTraceLevelMessage(
        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Leaving USB Selective Suspend Idle callback");
}


VOID
FxPowerPolicyMachine::UsbSSCallbackProcessingComplete(
    VOID
    )
{
    ASSERT(m_Owner->m_UsbIdle->m_IdleCallbackEvent != NULL);
    m_Owner->m_UsbIdle->m_IdleCallbackEvent->Set();
}



