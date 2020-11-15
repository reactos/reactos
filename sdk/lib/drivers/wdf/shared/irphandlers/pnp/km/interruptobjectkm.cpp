/*++

Copyright (c) Microsoft Corporation

Module Name:

    InterruptObject.cpp

Abstract:

    This module implements a frameworks managed interrupt object

Author:




Environment:

    Kernel mode only

Revision History:






--*/

#include "pnppriv.hpp"

// Tracing support
// #include "InterruptObjectKm.tmh"

_Must_inspect_result_
NTSTATUS
FxInterrupt::InitializeInternal(
    __in FxObject*  Parent,
    __in PWDF_INTERRUPT_CONFIG Configuration
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Configuration);

    DO_NOTHING();

    return STATUS_SUCCESS;
}

VOID
FxInterrupt::DpcHandler(
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ASSERT(m_EvtInterruptDpc != NULL);

    FX_TRACK_DRIVER(GetDriverGlobals());

    //
    // Call the drivers registered DpcForIsr event callback
    //
    if (m_CallbackLock != NULL) {
        KIRQL irql = 0;

        m_CallbackLock->Lock(&irql);
        m_EvtInterruptDpc(GetHandle(), m_Device->GetHandle());
        m_CallbackLock->Unlock(irql);
    }
    else {
        m_EvtInterruptDpc(GetHandle(), m_Device->GetHandle());
    }

    return;
}

BOOLEAN
FxInterrupt::QueueDpcForIsr(
    VOID
    )
{
    BOOLEAN queued;

    //
    // Using this function is optional,
    // but the caller better have registered a handler
    //
    ASSERT(m_EvtInterruptDpc != NULL);

    queued = KeInsertQueueDpc(&m_Dpc, this, NULL);

    return queued;
}

VOID
FxInterrupt::WorkItemHandler(
    VOID
    )
{
    ASSERT(m_EvtInterruptWorkItem != NULL );

    FX_TRACK_DRIVER(GetDriverGlobals());

    //
    // Call the drivers registered WorkItemForIsr event callback
    //
    if (m_CallbackLock != NULL) {
        KIRQL irql = 0;

        m_CallbackLock->Lock(&irql);

        FxPerfTraceWorkItem(&m_EvtInterruptWorkItem);
        m_EvtInterruptWorkItem(GetHandle(), m_Device->GetHandle());
        m_CallbackLock->Unlock(irql);
    }
    else {
        FxPerfTraceWorkItem(&m_EvtInterruptWorkItem);
        m_EvtInterruptWorkItem(GetHandle(), m_Device->GetHandle());
    }

    return;
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::ConnectInternal(
    VOID
    )
{
    IO_CONNECT_INTERRUPT_PARAMETERS connectParams;
    FxPkgPnp* fxPkgPnp;

    fxPkgPnp = m_Device->m_PkgPnp;

    //
    // Tell the PnP Manager to connect the interrupt.
    //
    ASSERT(fxPkgPnp->m_IoConnectInterruptEx != NULL);

    //
    // We're running on Longhorn or later (or somebody backported the new
    // interrupt code,) so tell the PnP manager everything we can about this
    // device.
    //
    RtlZeroMemory(&connectParams, sizeof(connectParams));

    if (FxIsProcessorGroupSupported()) {
        connectParams.Version = CONNECT_FULLY_SPECIFIED_GROUP;
    }
    else {
        connectParams.Version = CONNECT_FULLY_SPECIFIED;
    }

    connectParams.FullySpecified.PhysicalDeviceObject = m_Device->GetPhysicalDevice();
    connectParams.FullySpecified.InterruptObject      = &m_Interrupt;
    connectParams.FullySpecified.ServiceRoutine       = _InterruptThunk;
    connectParams.FullySpecified.ServiceContext       = this;
    connectParams.FullySpecified.SpinLock             = m_SpinLock;
    connectParams.FullySpecified.FloatingSave         = m_FloatingSave;
    connectParams.FullySpecified.Vector               = m_InterruptInfo.Vector;
    connectParams.FullySpecified.Irql                 = m_InterruptInfo.Irql;
    connectParams.FullySpecified.ProcessorEnableMask  = m_InterruptInfo.TargetProcessorSet;
    connectParams.FullySpecified.Group                = m_InterruptInfo.Group;
    connectParams.FullySpecified.InterruptMode        = m_InterruptInfo.Mode;
    connectParams.FullySpecified.ShareVector          =
        m_InterruptInfo.ShareDisposition == CmResourceShareShared ? TRUE : FALSE;
    connectParams.FullySpecified.SynchronizeIrql      = m_SynchronizeIrql;

    return fxPkgPnp->m_IoConnectInterruptEx(&connectParams);
}

VOID
FxInterrupt::DisconnectInternal(
    VOID
    )
{
    IO_DISCONNECT_INTERRUPT_PARAMETERS params;
    PKINTERRUPT interruptObject;
    FxPkgPnp* fxPkgPnp;

    fxPkgPnp = m_Device->m_PkgPnp;

    //
    // Now null these pointers so that we can catch anyone trying to use them
    // erroneously.
    //
    interruptObject = m_Interrupt;
    m_Interrupt = NULL;

    //
    // Disconnect the interrupt.
    //
    ASSERT(fxPkgPnp->m_IoDisconnectInterruptEx != NULL);

    RtlZeroMemory(&params, sizeof(params));

    if (FxIsProcessorGroupSupported()) {
        params.Version = CONNECT_FULLY_SPECIFIED_GROUP;
    }
    else {
        params.Version = CONNECT_FULLY_SPECIFIED;
    }

    params.ConnectionContext.InterruptObject = interruptObject;

    fxPkgPnp->m_IoDisconnectInterruptEx(&params);

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
    //
    // Set sharing policy.
    //
    switch (m_ShareVector) {
    case WdfTrue:
        //
        // Override the bus driver's value, explicitly sharing this interrupt.
        //
        IoResourceDescriptor->ShareDisposition = CmResourceShareShared;
        break;

    case WdfFalse:
        //
        // Override the bus driver's value, explicitly claiming this interrupt
        // as non-shared.
        //
        IoResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        break;

    case WdfUseDefault:
    default:
        //
        // Leave the bus driver's value alone.
        //
        break;
    }

    //
    // Apply policy.  Only do this if we are running on an OS which supports
    // the notion of Interrupt Policy and if the policy is not already included
    // by the bus driver based on registry settings.
    //
    if (FxLibraryGlobals.IoConnectInterruptEx != NULL &&
        m_SetPolicy &&
        (IoResourceDescriptor->Flags & CM_RESOURCE_INTERRUPT_POLICY_INCLUDED) == 0x0) {

        IoResourceDescriptor->Flags |= CM_RESOURCE_INTERRUPT_POLICY_INCLUDED;
        IoResourceDescriptor->u.Interrupt.AffinityPolicy      = (IRQ_DEVICE_POLICY)m_Policy;
        IoResourceDescriptor->u.Interrupt.PriorityPolicy      = (IRQ_PRIORITY)m_Priority;
        IoResourceDescriptor->u.Interrupt.TargetedProcessors  = m_Processors.Mask;
        IoResourceDescriptor->u.Interrupt.Group               = m_Processors.Group;
    }
}

VOID
FxInterrupt::FlushQueuedDpcs(
    VOID
    )
{
    KeFlushQueuedDpcs();
}

BOOLEAN
FxInterrupt::_InterruptThunk(
    __in struct _KINTERRUPT *Interrupt,
    __in PVOID              ServiceContext
    )

/*++

Routine Description:

    This is the C routine called by the kernels INTERRUPT handler

Arguments:

Return Value:

--*/

{
    FxInterrupt*    interrupt;
    BOOLEAN         result;

    UNREFERENCED_PARAMETER(Interrupt);

    interrupt = (FxInterrupt*)ServiceContext;
    ASSERT( interrupt->m_EvtInterruptIsr != NULL );

    if (interrupt->m_IsEdgeTriggeredNonMsiInterrupt == TRUE) {
        //
        // If KMDF is in the process of disconnecting this interrupt, discard it.
        //
        if (interrupt->m_Disconnecting == TRUE) {
            return FALSE;
        }

        //
        // Captures the interrupt object as a backup in case interrupts start
        // to arrive before IoConnectInterruptEx sets FxInterrupt.m_Interrupt.
        //
        interrupt->m_InterruptCaptured = Interrupt;
    }
    //
    // If the interrupt is not connected, treat this as spurious interrupt.
    //
    else if (NULL == interrupt->m_Interrupt) {
        return FALSE;
    }

    if (interrupt->IsWakeCapable()) {
        //
        // if it is a wake capable interrupt, we will hand it off
        // to the state machine so that it can power up the device
        // if required and then invoke the ISR callback
        //
        ASSERT(interrupt->m_PassiveHandling);
        FxPerfTracePassiveInterrupt(&interrupt->m_EvtInterruptIsr);
        return interrupt->WakeInterruptIsr();
    }

    if (interrupt->m_PassiveHandling) {
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        //
        // Acquire our internal passive-lock after the kernel acquired its own
        // passive-lock and before invoking the callback.
        //
        interrupt->AcquireLock();

        FxPerfTracePassiveInterrupt(&interrupt->m_EvtInterruptIsr);

        result = interrupt->m_EvtInterruptIsr(
                                interrupt->GetHandle(),
                                interrupt->m_InterruptInfo.MessageNumber);
        interrupt->ReleaseLock();
    }
    else {

        FxPerfTraceInterrupt(&interrupt->m_EvtInterruptIsr);

        result = interrupt->m_EvtInterruptIsr(
                                interrupt->GetHandle(),
                                interrupt->m_InterruptInfo.MessageNumber);
    }

    return result;
}

VOID
FxInterrupt::_InterruptDpcThunk(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:

   Thunk used to invoke EvtInterruptDpc at DPC-level, or to queue a work-item
   for invoking EvtInterruptWorkItem at passive-level.

--*/
{
    FxInterrupt* interrupt;

    UNREFERENCED_PARAMETER(Dpc);

    ASSERT(DeferredContext != NULL);
    interrupt = (FxInterrupt*) DeferredContext;

    if (interrupt->m_SystemWorkItem == NULL) {

        FxPerfTraceDpc(&interrupt->m_EvtInterruptDpc);

        interrupt->DpcHandler(SystemArgument1, SystemArgument2);
    }
    else {
        interrupt->m_SystemWorkItem->TryToEnqueue(_InterruptWorkItemCallback,
                                                  interrupt);
    }
}

VOID
FxInterrupt::FlushAndRundownInternal(
    VOID
    )
{
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
}

BOOLEAN
FxInterrupt::_InterruptMarkDisconnecting(
    _In_opt_ PVOID SyncContext
    )
{
    FxInterrupt* pFxInterrupt;

    ASSERT(SyncContext != NULL);
    pFxInterrupt = (FxInterrupt*)SyncContext;

    //
    // This callback is invoked only if m_IsEdgeTriggeredNonMsiInterrupt
    // is TRUE. This will cause _InterruptThunk to discard subsequent
    // interrupts until m_Disconnecting is reset to FALSE.
    //
    pFxInterrupt->m_Disconnecting = TRUE;

    return TRUE;
}

VOID
FxInterrupt::ReportActive(
    _In_ BOOLEAN Internal
    )
{
    IO_REPORT_INTERRUPT_ACTIVE_STATE_PARAMETERS parameters;
    FxPkgPnp* fxPkgPnp;

    fxPkgPnp = m_Device->m_PkgPnp;

    if (Internal == FALSE) {
        //
        // if the interrupt is not connected, you can't report active or inactive
        //
        if(m_Connected == FALSE || m_Interrupt == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                "Driver is reporting WDFINTERRUPT %p as being active even though"
                " it is not connected.",  GetHandle());
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return;
        }

        if (fxPkgPnp->m_IoReportInterruptActive == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                "Driver is calling DDI WdfInterruptReportActive() on an OS that "
                "doesn't support the DDI.");
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return;
        }
    }

    //
    // No need to report active if interrupt is already active
    //
    if (m_Active) {
        return;
    }

    RtlZeroMemory(&parameters, sizeof(parameters));

    if (FxIsProcessorGroupSupported()) {
        parameters.Version = CONNECT_FULLY_SPECIFIED_GROUP;
    }
    else {
        parameters.Version = CONNECT_FULLY_SPECIFIED;
    }

    parameters.ConnectionContext.InterruptObject = m_Interrupt;

    fxPkgPnp->m_IoReportInterruptActive(&parameters);
    m_Active = TRUE;

    return;
}

VOID
FxInterrupt::ReportInactive(
    _In_ BOOLEAN Internal
    )
{
    IO_REPORT_INTERRUPT_ACTIVE_STATE_PARAMETERS parameters;
    FxPkgPnp* fxPkgPnp;

    fxPkgPnp = m_Device->m_PkgPnp;

    if (Internal == FALSE) {
        //
        // if the interrupt is not connected, you can't report active or inactive
        //
        if(m_Connected == FALSE || m_Interrupt == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                "Driver is reporting WDFINTERRUPT %p as being inactive even though"
                " it is not connected.",  GetHandle());
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return;
        }

        if (fxPkgPnp->m_IoReportInterruptInactive == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                "Driver is calling DDI WdfInterruptReportInactive() on an OS that "
                "doesn't support the DDI.");
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return;
        }
    }

    //
    // No need to report Inactive if interrupt is already Inactive
    //
    if (m_Active == FALSE) {
        return;
    }

    RtlZeroMemory(&parameters, sizeof(parameters));

    if (FxIsProcessorGroupSupported()) {
        parameters.Version = CONNECT_FULLY_SPECIFIED_GROUP;
    }
    else {
        parameters.Version = CONNECT_FULLY_SPECIFIED;
    }

    parameters.ConnectionContext.InterruptObject = m_Interrupt;

    fxPkgPnp->m_IoReportInterruptInactive(&parameters);
    m_Active = FALSE;

    return;
}

