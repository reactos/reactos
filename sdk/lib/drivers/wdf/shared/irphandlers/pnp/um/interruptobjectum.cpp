/*++

Copyright (c) Microsoft Corporation

Module Name:

    InterruptObjectUm.cpp

Abstract:

    This module implements a frameworks managed interrupt object

Author:

Environment:

    User mode only

Revision History:



--*/

#include "fxmin.hpp"
#include "FxInterruptThreadpoolUm.hpp"

extern "C" {
#include "InterruptObjectUm.tmh"
}

#define STRSAFE_LIB
#include <strsafe.h>

_Must_inspect_result_
NTSTATUS
FxInterrupt::InitializeInternal(
    __in FxObject*  Parent,
    __in PWDF_INTERRUPT_CONFIG Configuration
    )
{
    IWudfDeviceStack *deviceStack;
    HRESULT hr;
    NTSTATUS status = STATUS_SUCCESS;
    FxInterruptThreadpool* pool = NULL;
    CM_SHARE_DISPOSITION shareVector;

    UNREFERENCED_PARAMETER(Parent);

    deviceStack = m_Device->GetDeviceStack();

    switch (m_ShareVector) {
    case WdfTrue:
        //
        // Override the bus driver's value, explicitly sharing this interrupt.
        //
        shareVector = CmResourceShareShared;
        break;

    case WdfFalse:
        //
        // Override the bus driver's value, explicitly claiming this interrupt
        // as non-shared.
        //
        shareVector = CmResourceShareDeviceExclusive;
        break;

    case WdfUseDefault:
    default:
        //
        // Leave the bus driver's value alone.
        //
        shareVector = CmResourceShareUndetermined;
        break;
    }

    //
    // Create a thread pool if not already created. An interrupt is created in
    // one of the pnp callbacks (OnAddDevice, OnPrepareHarwdare etc) so there is
    // no race in getting and setting the theradpool pointer.
    //
    pool = m_Device->GetInterruptThreadpool();
    if (pool == NULL) {
        hr = FxInterruptThreadpool::_CreateAndInit(GetDriverGlobals(),
                                                   &pool);
        if (FAILED(hr))
        {
            goto exit;
        }

        m_Device->SetInterruptThreadpool(pool);
    }

    //
    // create an instance of interruypt wait block
    //
    hr = FxInterruptWaitblock::_CreateAndInit(pool,
                                                this,
                                                FxInterrupt::_InterruptThunk,
                                                &m_InterruptWaitblock);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Waitblock creation failed for CWdfInterrupt object"
                    " %!hresult!", hr);
        goto exit;
    }

    //
    // Send an IOCTL to redirector to create and initialize an interrupt object
    //
    deviceStack = m_Device->GetDeviceStack();

    hr = deviceStack->InitializeInterrupt((WUDF_INTERRUPT_CONTEXT) this,
                                          m_InterruptWaitblock->GetEventHandle(),
                                          shareVector,
                                          &m_RdInterruptContext
                                          );

    if (SUCCEEDED(hr))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        PUMDF_VERSION_DATA driverVersion = deviceStack->GetMinDriverVersion();

        BOOL preserveCompat =
             deviceStack->ShouldPreserveIrpCompletionStatusCompatibility();

        status = CHostFxUtil::NtStatusFromHr(
                                        hr,
                                        driverVersion->MajorNumber,
                                        driverVersion->MinorNumber,
                                        preserveCompat
                                        );
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "failed to initialize interrupt "
            "%!STATUS!", status);

        goto exit;
    }

exit:

    //
    // Dispose will do cleanup. No need to cleanup here.
    //

    return status;
}

NTSTATUS
FxInterrupt::ConnectInternal(
    VOID
    )
{
    HRESULT hr;
    NTSTATUS status;
    IWudfDeviceStack2 *deviceStack;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    BOOLEAN isRdConnectingOrConnected = FALSE;

    pFxDriverGlobals = GetDriverGlobals();
    deviceStack = m_Device->GetDeviceStack2();

    //
    // reset the interrupt event to non-signaled state to start with a
    // clean slate.
    //
    m_InterruptWaitblock->ResetEvent();

    //
    // Open the queue and enqueue an interrupt event wait to the threadpool
    // before connecting the interrupt in the reflector. This minimizes the
    // processing delay for interrupts that fire as soon as they are connected,
    // like GPIO button devices that have no means to explicitly enable and
    // disable interrupts at the hardware level.
    //
    StartThreadpoolWaitQueue();

    //
    // Tell the PnP Manager to connect the interrupt. Send a message to
    // redirector to do so. When ConnectInterrupt returns a failure code,
    // we use isRdConnectingOrConnected to check if the failure was due
    // to an already connected or currently connecting interrupt.
    //
    hr = deviceStack->ConnectInterrupt(m_RdInterruptContext,
                                       &isRdConnectingOrConnected);
    if (FAILED(hr))
    {
        if (isRdConnectingOrConnected) {
            //
            // The connect call failed because we asked the Reflector to connect
            // an already connected or currently connecting interrupt. Perhaps the
            // client made a redundant call to WdfInterruptEnable. In this case,
            // we want to keep the threadpool active so that we continue to receive
            // and acknowledge interrupts - otherwise RdIsrPassiveLevel may time out.
            //
            DoTraceLevelMessage(pFxDriverGlobals,
                TRACE_LEVEL_ERROR, TRACINGPNP,
                "Multiple connection attempts for !WDFINTERRUPT 0x%p",
                GetHandle());

            if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(2, 19)) {
                FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
                    CHECK("Multiple interrupt connection attempts", FALSE),
                    pFxDriverGlobals->Public.DriverName);
            }
        }
        else {
            //
            // Connecting the interrupt in the reflector failed, which means
            // that IoConnectInterruptEx either failed or was not called at all.
            // All we need to do in this case is revert the actions done by
            // StartThreadpoolWaitQueue above, which are closing the queue
            // and removing the enqueued interrupt event wait.
            //
            StopAndFlushThreadpoolWaitQueue();
        }

        PUMDF_VERSION_DATA driverVersion = deviceStack->GetMinDriverVersion();
        BOOL preserveCompat =
             deviceStack->ShouldPreserveIrpCompletionStatusCompatibility();

        status = CHostFxUtil::NtStatusFromHr(
                                        hr,
                                        driverVersion->MajorNumber,
                                        driverVersion->MinorNumber,
                                        preserveCompat
                                        );

        DoTraceLevelMessage(pFxDriverGlobals,
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "Connect message to reflector returned failure "
            "%!hresult!", hr);

        return status;
    }

    status = STATUS_SUCCESS;

    return status;
}

VOID
FxInterrupt::DisconnectInternal(
    VOID
    )
{
    IWudfDeviceStack *deviceStack;
    HRESULT hr;
    InterruptControlType controlType;

    //
    // Tell the PnP Manager to disconnect the interrupt.
    // Send a message to redirector to do so.
    //
    deviceStack = m_Device->GetDeviceStack();

    controlType = InterruptControlTypeDisconnect;
    hr = deviceStack->ControlInterrupt(m_RdInterruptContext, controlType);
    if (FAILED(hr))
    {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "Disconnect message to reflector returned failure "
            "%!hresult!", hr);

        FX_VERIFY_WITH_NAME(INTERNAL, TRAPMSG("Disconnect message to reflector returned failure. "),
            GetDriverGlobals()->Public.DriverName);
    }

    //
    // Now that interrupt has been disconnected, flush the threadpool. Note that
    // we need to do this after disconnect because if we did it before disconnect,
    // we might drop any spurious interrupts that were generated after
    // the driver disabled interrupt generation in its Disable callback,
    // and after the DPCs were flushed. Fx can't drop spurious interrupt
    // because if the interrupt is level-triggered then refelctor would be waiting
    // for acknowledgement. The fact that disconnect command to reflector has
    // returned to fx guarantees that there are no more interrupts pending in
    // reflector.
    //
    StopAndFlushThreadpoolWaitQueue();

    //
    // There might still be WorkItemForIsr running as a result of
    // the handling of spurious interrupt by driver, so we need to flush the
    // workitem as well.
    //
    FlushQueuedWorkitem();

    return;
}

VOID
FxInterrupt::SetPolicyInternal(
    __in WDF_INTERRUPT_POLICY   Policy,
    __in WDF_INTERRUPT_PRIORITY Priority,
    __in PGROUP_AFFINITY        TargetProcessorSet
    )
{
    IWudfDeviceStack *deviceStack;
    HRESULT hr;

    deviceStack = m_Device->GetDeviceStack();

    //
    // Tell reflector to set the policy of interrupt.
    //
    hr = deviceStack->SetInterruptPolicy(m_RdInterruptContext,
                                         Policy,
                                         Priority,
                                         TargetProcessorSet);
    if (FAILED(hr))
    {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "SetPolicy message to reflector returned failure "
            "%!hresult!", hr);
    }

    FX_VERIFY_WITH_NAME(INTERNAL, CHECK_HR(hr), GetDriverGlobals()->Public.DriverName);

    return;
}

VOID
FxInterrupt::FilterResourceRequirements(
    __inout PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor
    )
/*++

Routine Description:

    This function allows an interrupt object to change the
    IoResourceRequirementsList that the PnP Manager sends during
    IRP_MN_FILTER_RESOURCE_REQUIREMENTS.  This function takes a single
    IoResourceDescriptor and applies default or user specified policy.

Arguments:

    IoResourceDescriptor - Pointer to descriptor that matches this interrupt object

Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(IoResourceDescriptor);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
FxInterrupt::ResetInternal(
    VOID
    )
{
    IWudfDeviceStack *deviceStack;
    InterruptControlType controlType;
    HRESULT hr;

    if (m_RdInterruptContext == NULL) {
        //
        // Reflector hasn't yet created a partner interrupt object so nothing
        // to do.
        //
        return;
    }

    //
    // Send a message to redirector to reset interrupt info.
    //
    deviceStack = m_Device->GetDeviceStack();

    controlType = InterruptControlTypeResetInterruptInfo;
    hr = deviceStack->ControlInterrupt(m_RdInterruptContext, controlType);
    if (FAILED(hr))
    {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "ResetInterruptInfo message to reflector returned failure "
            "%!hresult!", hr);
    }

    FX_VERIFY_WITH_NAME(INTERNAL, CHECK_HR(hr), GetDriverGlobals()->Public.DriverName);

    return;
}

VOID
FxInterrupt::RevokeResourcesInternal(
    VOID
    )
{
    IWudfDeviceStack *deviceStack;
    InterruptControlType controlType;
    HRESULT hr;

    if (m_RdInterruptContext == NULL) {
        //
        // Reflector hasn't yet created a partner interrupt object so nothing
        // to do.
        //
        return;
    }

    //
    // Send a message to redirector to revoke interrupt resources.
    //
    deviceStack = m_Device->GetDeviceStack();

    controlType = InterruptControlTypeRevokeResources;
    hr = deviceStack->ControlInterrupt(m_RdInterruptContext, controlType);
    if (FAILED(hr))
    {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "RevokeResources message to reflector returned failure "
            "%!hresult!", hr);
    }

    FX_VERIFY_WITH_NAME(INTERNAL, CHECK_HR(hr), GetDriverGlobals()->Public.DriverName);

    return;
}

VOID
FxInterrupt::FlushQueuedDpcs(
    VOID
    )
{
    IWudfDeviceStack *deviceStack;
    HRESULT hr;

    //
    // Send a message to redirector to flush DPCs.
    //
    deviceStack = m_Device->GetDeviceStack();
    hr = deviceStack->ControlInterrupt(m_RdInterruptContext,
                                       InterruptControlTypeFlushQueuedDpcs);
    if (FAILED(hr))
    {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "FlushQueuedDpcs message to reflector returned failure "
            "%!hresult!", hr);

        FX_VERIFY_WITH_NAME(INTERNAL,
            TRAPMSG("FlushQueuedDpcs message to reflector returned failure"),
            GetDriverGlobals()->Public.DriverName);
    }

    return;
}

VOID
FxInterrupt::AssignResourcesInternal(
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans,
    __in PWDF_INTERRUPT_INFO InterruptInfo
    )
{
    IWudfDeviceStack *deviceStack;
    HRESULT hr;

    //
    // level-triggered interrupt handling is supported only on win8 and newer.
    //
    if (IsLevelTriggered(CmDescTrans->Flags) &&
        FxIsPassiveLevelInterruptSupported() == FALSE) {
        hr = E_INVALIDARG;
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "Failed to assign interrupt resource to interrupt object"
            "because interrupt resource is for level-triggered interrupt"
            "which is not supported on this platform. See the docs for info on"
            "supported platforms. %!hresult!\n", hr);

        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), TRAPMSG(
            "Failed to assign interrupt resource to interrupt object"
            "because interrupt resource is for level-triggered interrupt"
            "which is not supported on this platform. See the docs for info on"
            "supported platforms."),
            GetDriverGlobals()->Public.DriverName);
    }

    //
    // Sharing is only supported for level-triggered interrupts. We allow
    // shared latched interrupts in order to accomodate incorrect device
    // firmwares that mistakenly declare their exclusive resources as shared.
    // Genuinely shared edge-triggered interrupts will cause a deadlock
    // because of how the OS handles non-passive latched interrupts with
    // multiple registered handlers. See RdInterrupt::AssignResources
    // for details.
    //
    if (IsLevelTriggered(CmDescTrans->Flags) == FALSE &&
        CmDescTrans->ShareDisposition != CmResourceShareDeviceExclusive) {

        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_WARNING, TRACINGPNP,
            "The resource descriptor indicates that this is a shared "
            "edge-triggered interrupt. UMDF only supports sharing of "
            "level-triggered interrupts. Please check if your device "
            "firmware mistakenly declares this resource as shared "
            "instead of device exclusive. If the resource is in fact "
            "shared, then UMDF does not support this device.\n");
    }

    //
    // Tell the PnP Manager to assign resources to the interrupt.
    // Send a message to redirector to do so.
    //
    deviceStack = m_Device->GetDeviceStack();

    hr = deviceStack->AssignInterruptResources(m_RdInterruptContext,
                                               CmDescRaw,
                                               CmDescTrans,
                                               InterruptInfo,
                                               m_PassiveHandlingByRedirector);
    if (FAILED(hr))
    {
        DoTraceLevelMessage(GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "AssignResources message to reflector returned failure "
            "%!hresult!\n", hr);
    }

    FX_VERIFY_WITH_NAME(INTERNAL, CHECK_HR(hr),
        GetDriverGlobals()->Public.DriverName);

}

VOID
FxInterrupt::ThreadpoolWaitCallback(
    VOID
    )
{
    BOOLEAN claimed;

    //
    // ETW event for performance measurement
    //
    EventWriteEVENT_UMDF_FX_INTERRUPT_NOTIFICATION_RECEIVED(
        m_InterruptInfo.MessageNumber
        );

    //
    // Invoke the ISR callback under interrupt lock.
    //
    if (IsWakeCapable()) {
        //
        // if it is a wake capable interrupt, we will hand it off
        // to the state machine so that it can power up the device
        // if required and then invoke the ISR callback
        //
        claimed = WakeInterruptIsr();
    } else {
        AcquireLock();
        claimed = m_EvtInterruptIsr(GetHandle(),
                                   m_InterruptInfo.MessageNumber
                                   );
        ReleaseLock();
    }

    //
    // Queue another wait. MSDN: You must re-register the event with the
    // wait object before signaling it each time to trigger the wait callback.
    //
    if (m_CanQueue) {
        QueueSingleWaitOnInterruptEvent();
    }

    //
    // Return acknowledgement to reflector if it's handled at passive level
    // by reflector.
    //
    if (m_PassiveHandlingByRedirector) {
        IWudfDeviceStack *deviceStack;
        HRESULT hr;

        deviceStack = m_Device->GetDeviceStack();

        hr = deviceStack->AcknowledgeInterrupt(m_RdInterruptContext, claimed);

        if (FAILED(hr)) {
            DoTraceLevelMessage(GetDriverGlobals(),
                TRACE_LEVEL_ERROR, TRACINGPNP,
                "AcknowledgeInterrupt message to reflector returned "
                "failure. Check UMDF log for failure reason. %!hresult!", hr);

            FX_VERIFY_WITH_NAME(INTERNAL, TRAPMSG("AcknowledgeInterrupt message to "
                "reflector returned failure. Check UMDF log for failure reason. "),
                GetDriverGlobals()->Public.DriverName);
        }
    }

    return;
}

VOID
FxInterrupt::QueueSingleWaitOnInterruptEvent(
    VOID
    )
{
    m_InterruptWaitblock->SetThreadpoolWait();
}

VOID
FxInterrupt::StartThreadpoolWaitQueue(
    VOID
    )
{
    m_CanQueue = TRUE;

    QueueSingleWaitOnInterruptEvent();
}

VOID
FxInterrupt::StopAndFlushThreadpoolWaitQueue(
    VOID
    )
{
    //
    // We need to stop the threadpool wait queue and accomplish the following:
    // 1) Prevent any new waits from being queued.
    // 2) Removed any waits already queued.
    // 3) Wait for interrupt isr callback to complete.
    //

    //
    // Prevent any more enquing now that interrupt has been disconnected.
    //
    m_CanQueue = FALSE;

    //
    // wait for isr callback
    //
    m_InterruptWaitblock->WaitForOutstandingCallbackToComplete();

    //
    // remove any waits already queued
    //
    m_InterruptWaitblock->ClearThreadpoolWait();

    //
    // wait for callback. This additional wait for callback is needed to
    // handle the follwoing race:
    // - CanQueue is set to false in this thread
    // - Callback is executing at statement after CanQueue check so it did not
    //   see false.
    // - this thread waits for callback
    // - callback thread queues a wait and returns
    // - the wait earlier queued is satisfied and callback runs
    // - this thread clears the queue (there is nothing to clear) but there is
    //   still a callback runnning and this thread needs to wait.
    //
    m_InterruptWaitblock->WaitForOutstandingCallbackToComplete();
}

VOID
CALLBACK
FxInterrupt::_InterruptThunk(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID                 Parameter,
    PTP_WAIT              Wait,
    TP_WAIT_RESULT        WaitResult
    )
{
    FxInterrupt* fxInterrupt = (FxInterrupt*) Parameter;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(WaitResult);

    fxInterrupt->ThreadpoolWaitCallback();

    return;
}

VOID
FxInterrupt::FlushAndRundownInternal(
    VOID
    )
{
    //
    // flush the threadpool callbacks
    //
    StopAndFlushThreadpoolWaitQueue();

    //
    // Rundown the workitem.
    //
    if (m_SystemWorkItem != NULL) {
        m_SystemWorkItem->DeleteObject();
        m_SystemWorkItem = NULL;
    }

    //
    // If present, delete the default passive-lock.
    //
    if (m_DisposeWaitLock) {
        ASSERT(m_WaitLock != NULL);
        m_WaitLock->DeleteObject();
        m_WaitLock = NULL;
        m_DisposeWaitLock = FALSE;
    }

    //
    // waitblock destructor will ensure event and waitblock cleanup.
    //
    if (m_InterruptWaitblock != NULL) {
        delete m_InterruptWaitblock;
        m_InterruptWaitblock = NULL;
    }

    //
    // No need to explicitly delete the COM wrapper (CWdfInterrupt).
    // The COM wrapper will get deleted in fxInterrupt's destroy callback when
    // the object tree reference taken during creation will be released.
    //
}

BOOLEAN
FxInterrupt::QueueDpcForIsr(
    VOID
    )
{
    FX_VERIFY_WITH_NAME(INTERNAL, TRAPMSG("Not implemented"),
        GetDriverGlobals()->Public.DriverName);
    return FALSE;
}

VOID
FxInterrupt::WorkItemHandler(
    VOID
    )
{
    //
    // For UMDF, we allow drivers to call WdfInterruptQueueDpcdForIsr, and
    // internally we queue a workitem and invoke EvtInterruptDpc. Only
    // one of the callbacks EvtInterruptDpc or EvtInterruptWorkitem is
    // allowed.
    //
    ASSERT(m_EvtInterruptWorkItem != NULL || m_EvtInterruptDpc != NULL);
    ASSERT((m_EvtInterruptWorkItem != NULL && m_EvtInterruptDpc != NULL) == FALSE);

    FX_TRACK_DRIVER(GetDriverGlobals());

    //
    // Call the drivers registered WorkItemForIsr event callback
    //
    if (m_CallbackLock != NULL) {
        KIRQL irql = 0;

        m_CallbackLock->Lock(&irql);
        if (m_EvtInterruptWorkItem != NULL) {
            m_EvtInterruptWorkItem(GetHandle(), m_Device->GetHandle());
        }
        else {
            m_EvtInterruptDpc(GetHandle(), m_Device->GetHandle());
        }
        m_CallbackLock->Unlock(irql);
    }
    else {
        if (m_EvtInterruptWorkItem != NULL) {
            m_EvtInterruptWorkItem(GetHandle(), m_Device->GetHandle());
        }
        else {
            m_EvtInterruptDpc(GetHandle(), m_Device->GetHandle());
        }
    }

    return;
}

BOOLEAN
_SynchronizeExecution(
    __in MdInterrupt  Interrupt,
    __in MdInterruptSynchronizeRoutine  SynchronizeRoutine,
    __in PVOID  SynchronizeContext
    )
{
    FxInterruptEnableParameters* pParams;
    BOOLEAN isPassive;

    UNREFERENCED_PARAMETER(Interrupt);

    pParams = (FxInterruptEnableParameters*) SynchronizeContext;
    isPassive = pParams->Interrupt->IsPassiveHandling();
    FX_VERIFY(INTERNAL, CHECK("Must be Passive Interrupt", isPassive));

    //
    // The internal synchronize routine will call the routine under lock
    //
    return SynchronizeRoutine(SynchronizeContext);
}
