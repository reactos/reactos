//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbPipe.tmh"
}

#include "Fxglobals.h"
//
//  NOTE: There are 3 different paths Requests could be sent to the lower driver
//  1) In case of reposting a successfully completed Request use the Dpc which calls SendIo.
//  2) For a failed completion use a workitem which works if the IoTarget is in Started state.
//  3) On moving to the start state the Repeater requests are inserted into the
//      Iotargets Pended Queue directly.
//
//  The function ResubmitRepeater calls SubmitLocked. If the action we get back
//  from SubmitLocked is "SubmitSend", (SubmitQueued is treated as failure because
//  of WDF_REQUEST_SEND_INTERNAL_OPTION_FAIL_ON_PEND flag) we are guaranteed to
//  call IoCallDriver in the workitem or  the DPC and hence the completion routine
//  being called.
//  This is very important because we increment the m_IoCount in SubmitLocked and
//  decrement in the completion routine.  So if there was a code path where
//  SubmitLocked was called and IoCallDriver(hence the completion routine) wasn't,
//  the IoTarget could stop responding in Dispose.
//


FxUsbPipeContinuousReader::FxUsbPipeContinuousReader(
    __in FxUsbPipe* Pipe,
    __in UCHAR NumReaders
    ) :
    m_NumReaders(NumReaders),
    m_NumFailedReaders(0)
{
    m_WorkItem = NULL;
    m_WorkItemRerunContext = NULL;
    m_WorkItemThread = NULL;
    m_WorkItemFlags = 0;
    m_WorkItemQueued = FALSE;
    m_ReadersSubmitted = FALSE;

    m_Lookaside = NULL;
    m_Pipe = Pipe;

    m_TargetDevice = m_Pipe->GetTargetDevice();

    RtlZeroMemory(&m_Readers[0], m_NumReaders * sizeof(FxUsbPipeRepeatReader));
}

FxUsbPipeContinuousReader::~FxUsbPipeContinuousReader()
{
    LONG i;

    FxUsbPipeRepeatReader * reader = &m_Readers[0];

    //
    // It is impoortant to delete the requests before the lookaside because the
    // requests may have outstanding references on memory objects allocated by
    // the lookaside.  The lookaside will not be truly deleted until the oustanding
    // memory object allocations are also freed.
    //
    for (i = 0; i < m_NumReaders; i++) {
        if (reader[i].Request != NULL) {
            DeleteMemory(reader[i].Request);

            reader[i].Request->DeleteObject();
            reader[i].Request = NULL;
        }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        reader[i].ReadCompletedEvent.Uninitialize();
        reader[i].m_ReadWorkItem.Free();
#endif
    }

    if (m_Lookaside != NULL) {
        m_Lookaside->DeleteObject();
    }

    if (m_WorkItem != NULL) {
        m_WorkItem->DeleteObject();
        m_WorkItem = NULL;
    }
}

BOOLEAN
FxUsbPipeContinuousReader::QueueWorkItemLocked(
    __in FxUsbPipeRepeatReader* Repeater
    )
{
    BOOLEAN queued;
    PFX_DRIVER_GLOBALS fxDriverGlobals;

    queued = FALSE;
    fxDriverGlobals = m_Pipe->GetDriverGlobals();

    if (m_Pipe->m_State == WdfIoTargetStarted && m_WorkItemQueued == FALSE) {
        //
        // No item queued, queue it up now.
        //
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p continuous reader queueing work item to recover "
            "from failed allocation", m_Pipe->GetHandle());

        if (m_WorkItem->Enqueue(_FxUsbPipeRequestWorkItemThunk, Repeater)) {
            m_WorkItemQueued = TRUE;
            queued = TRUE;
        }
        else {
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                        "Could not Queue workitem");
        }
    }

    //
    // We only want to queue the work item while the target is in the
    // started state.  If it is not started, then we are no longer sending
    // i/o and we should not queue the work item to try to restart.
    //
    if (FALSE == queued) {
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p continuous reader not queueing work item,"
            "WorkItemQueued = %d, target state %!WDF_IO_TARGET_STATE!",
            m_Pipe->GetHandle(), m_WorkItemQueued, m_Pipe->m_State);
    }

    return queued;
}

ULONG
FxUsbPipeContinuousReader::ResubmitRepeater(
    __in FxUsbPipeRepeatReader* Repeater,
    __out NTSTATUS* Status
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    ULONG action;
    KIRQL irql;

    action              = 0;
    pFxDriverGlobals    = m_Pipe->GetDriverGlobals();
    status              = STATUS_UNSUCCESSFUL;

    //
    // Reformat and allocate any new needed buffers
    //
    status = FormatRepeater(Repeater);

    m_Pipe->Lock(&irql);

    //
    // Do not re-submit repeaters if there is a queued/running work-item to
    // reset pipe. Work-item will restart this repeater later.
    // This check needs to be done after the FormatRepeater() call above to
    // prevent a race condition where we are not detecting when the repeater
    // is cancelled.
    //
    if (m_WorkItemQueued) {
        //
        // Return an error and no action flags to let the caller know that
        // this request was not sent.
        //
        status = STATUS_CANCELLED;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p is being reset, continuous reader %p FxRequest %p"
            " PIRP %p is deferred for later.",
            m_Pipe->GetHandle(), Repeater, Repeater->Request,
            Repeater->RequestIrp);
    }
    else if (NT_SUCCESS(status)) {
        //
        // Get ready to re-submit the repeater.
        //
        action = m_Pipe->SubmitLocked(
            Repeater->Request,
            NULL,
            WDF_REQUEST_SEND_INTERNAL_OPTION_FAIL_ON_PEND
            );

        if (action & SubmitSend) {
            //
            // Clear the event only if we are going to send the request
            //
            Repeater->ReadCompletedEvent.Clear();
        }
        else if (action & SubmitQueued) {
            //
            // Request got canceled asynchronously. The other thread is now
            // responsible for calling its completion callback.
            //
            status = STATUS_CANCELLED;
        }
        else {
            //
            // Submit failed (which is expected when we are changing the target
            // state or when the request is canceled).  It should always be an
            // error.
            //
            status = Repeater->Request->GetFxIrp()->GetStatus();
            ASSERT(!NT_SUCCESS(status));
        }
    }
    else {
        //
        // Could not allocate a new buffer
        //
        Repeater->Request->GetFxIrp()->SetStatus(status);

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p continuous reader, format failed, %!STATUS!, "
            "repeater %p", m_Pipe->GetHandle(), status, Repeater);

        if (m_Pipe->m_State == WdfIoTargetStarted) {
            m_NumFailedReaders++;
            ASSERT(m_NumFailedReaders <= m_NumReaders);

            if (m_NumFailedReaders == m_NumReaders) {
                //
                // Queue a work item to clear problem.
                //
                QueueWorkItemLocked(Repeater);
            }
            else {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                    "WDFUSBPIPE %p continuous reader, buffer alloc failed, but "
                    "there are %d readers left out of a max of %d",
                    m_Pipe->GetHandle(), m_NumReaders - m_NumFailedReaders,
                    m_NumReaders);

                //
                // There are still other pending readers, just use those for
                // now.
                //
                DO_NOTHING();
            }
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                "WDFUSBPIPE %p continuous reader, buffer alloc failed, but not "
                "in started state", m_Pipe->GetHandle());
        }
    }

    m_Pipe->Unlock(irql);

    *Status = status;

    return action;
}

VOID
FxUsbPipeContinuousReader::_FxUsbPipeRequestComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS Params,
    __in WDFCONTEXT Context
    )
{
    FxUsbPipeRepeatReader* pRepeater;
    FxUsbPipeContinuousReader* pThis;
    FxUsbPipe* pPipe;
    NTSTATUS status;
    ULONG action;
    BOOLEAN readCompletedEventSet;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Params);

    readCompletedEventSet = FALSE;
    action = 0;
    pRepeater = (FxUsbPipeRepeatReader*) Context;
    pThis = (FxUsbPipeContinuousReader*) pRepeater->Parent;
    pPipe = pThis->m_Pipe;

    status = pRepeater->Request->GetFxIrp()->GetStatus();

    if (NT_SUCCESS(status)) {
        PWDF_USB_REQUEST_COMPLETION_PARAMS params;

        params  = pRepeater->Request->GetContext()->
            m_CompletionParams.Parameters.Usb.Completion;

        pThis->m_ReadCompleteCallback((WDFUSBPIPE) Target,
                                      params->Parameters.PipeRead.Buffer,
                                      params->Parameters.PipeRead.Length,
                                      pThis->m_ReadCompleteContext);

        //
        // This will release the reference on the read memory and allocate a new
        // one
        //
        action = pThis->ResubmitRepeater(pRepeater, &status);
    }
    else if (status != STATUS_CANCELLED) {
        KIRQL irql;

        DoTraceLevelMessage(
            pPipe->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p continuous reader FxRequest %p PIRP %p returned with "
            "%!STATUS!", pPipe->GetHandle(), pRepeater->Request ,
            pRepeater->RequestIrp, status);


        pPipe->Lock(&irql);

        pRepeater->ReadCompletedEvent.Set();
        readCompletedEventSet = TRUE;

        //
        // Queue a work item to clear problem.
        //
        pThis->QueueWorkItemLocked(pRepeater);

        pPipe->Unlock(irql);

        ASSERT(!NT_SUCCESS(status));
    }
    else {
        //
        // I/O was cancelled, which means internally it was cancelled so don't
        // do anything.
        //
        DoTraceLevelMessage(
            pPipe->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p continuous reader %p FxRequest %p PIRP %p canceled",
            pPipe->GetHandle(), pRepeater, pRepeater->Request , pRepeater->RequestIrp);

        DO_NOTHING();
    }

    if (action & SubmitSend) {

        //
        // We don't want to recurse on the same stack and overflow it.
        // This is especially true if the device is pushing a lot of data and
        // usb is completing everything within its dpc as soon as we send the
        // read down.  Eventually on a chk build, we will be nailed for running
        // in one DPC for too long.
        //
        // As a slower alternative, we could queue a work item and resubmit the
        // read from there.
        //

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        BOOLEAN result;
        result = KeInsertQueueDpc(&pRepeater->Dpc, NULL, NULL);

        //
        // The DPC should never be currently queued when we try to queue it.
        //
        ASSERT(result != FALSE);
#else
        pRepeater->m_ReadWorkItem.Enqueue((PMX_WORKITEM_ROUTINE)_ReadWorkItem, pRepeater);
#endif
        UNREFERENCED_PARAMETER(status); //for fre build

    }
    else if (action & SubmitQueued) {
        //
        // I/O got canceled asynchronously; the other thread is now
        // responsible for re-invoking this completion routine.
        //
        ASSERT(STATUS_CANCELLED == status);

        DoTraceLevelMessage(
            pPipe->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p continuous reader %p FxRequest %p PIRP %p got"
            " asynchronously canceled",
            pPipe->GetHandle(), pRepeater, pRepeater->Request ,
            pRepeater->RequestIrp);

        DO_NOTHING();
    }
    else if (FALSE == readCompletedEventSet) {
        ASSERT(!NT_SUCCESS(status));
        //
        // We are not sending the request and it is not queued so signal that
        // it is done.
        //
        pRepeater->ReadCompletedEvent.Set();
    }
}

VOID
FxUsbPipeContinuousReader::FxUsbPipeRequestWorkItemHandler(
    __in FxUsbPipeRepeatReader* FailedRepeater
    )
{
    FxUsbDevice* pDevice;
    NTSTATUS status, failedStatus;
    USBD_STATUS usbdStatus;
    LONG i;
    KIRQL irql;
    BOOLEAN restart;
    FxRequestContext* context;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = m_Pipe->GetDriverGlobals();

    //
    // Get failed info.
    //
    failedStatus = FailedRepeater->Request->GetStatus();

    //
    // Context is allocated at config time and gets reused so context
    // will never be NULL.
    //
    context = FailedRepeater->Request->GetContext();
    usbCompletionParams = context->m_CompletionParams.Parameters.Usb.Completion;

    //
    // In case FormatRepeater fails to allocate memory usbCompletionParams
    // pointer is not set.
    //
    // usbCompletionParams are part of the context and
    // not really allocated at the time of every Format but
    // the pointer gets cleared by request->Reuse and gets set again by
    // context->SetUsbType.
    //
    // In FormatRepeater, context->SetUsbType is skipped
    // if a memory failure occurs before this step.
    //
    // Hence retrieve usbdStatus only when usbCompletionParams is set.
    //
    if (usbCompletionParams) {
        usbdStatus = usbCompletionParams->UsbdStatus;
    }
    else {
        //
        // Set usbdStatus to success as we didn't receive a failure from
        // USB stack.
        //
        // This path is reached during memory allocation failure. In such
        // case failedStatus would already be set appropriately. (usbdStatus
        // and failedStatus are passed to m_ReadersFailedCallback below.)
        //
        usbdStatus = STATUS_SUCCESS;
    }

    //
    // No read requests should be in progress when the framework calls the
    // EvtUsbTargetPipeReadersFailed callback function. This is part of the
    // contract so that the Driver doesn't need to bother with the
    // Completion calllback while taking corrective action.
    //
    CancelRepeaters();
    pDevice = m_Pipe->m_UsbDevice;

    if (m_ReadersFailedCallback != NULL) {
        //
        // Save the current thread object pointer. This value is
        // used for not deadlocking when misbehaved drivers (< v1.9) call
        // WdfIoTargetStop from EvtUsbTargetPipeReadersFailed callback
        //
        ASSERT(NULL == m_WorkItemThread);
        m_WorkItemThread = Mx::MxGetCurrentThread();

        restart = m_ReadersFailedCallback(
            (WDFUSBPIPE) m_Pipe->GetHandle(),
            failedStatus,
            usbdStatus
            );

        m_WorkItemThread = NULL;
    }
    else {
        //
        // By default, we restart the readers
        //
        restart = TRUE;
    }

    if (restart) {
        status = pDevice->IsConnected();

        if (NT_SUCCESS(status)) {

            //
            // for v1.9 or higher use the error recovery procedure prescribed
            // by the USB team.
            //
            if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {

                if (pDevice->IsEnabled()) {
                    //
                    // Reset the pipe if port status is enabled
                    //
                    m_Pipe->Reset();
                }
                else {
                    //
                    // Reset the device if port status is disabled
                    //
                    status = pDevice->Reset();
                }
            }
            else {
                //
                // Reset the device if port status is disabled
                //
                status = pDevice->Reset();
            }
        }
        else {
            //
            // if port status is disconnected we would get back
            // a !NT_SUCCESS. This would mean that we would not
            // send the readers again and treat it like a failed reader.
            //
            DO_NOTHING();
        }

    }
    else {
        //
        // By setting status to !NT_SUCCESS, we will not send the readers
        // again and treat it like a failed reader.
        //
        status = STATUS_UNSUCCESSFUL;
    }

    //
    // Work item is no longer queued.  We set this before resubmitting the
    // repeaters so that if they all complete and fail, we will requeue the
    // work item.
    //
    m_Pipe->Lock(&irql);
    m_WorkItemQueued = FALSE;
    m_Pipe->Unlock(irql);

    if (NT_SUCCESS(status)) {
        ULONG action;

        //
        // Reset the count to zero.  This is safe since we stopped all the
        // readers at the beginning of this function.
        //
        m_NumFailedReaders = 0;

        //
        // restart the readers
        //
        for (i = 0; i < m_NumReaders; i++) {
            FxUsbPipeRepeatReader* pRepeater;

            pRepeater = &m_Readers[i];

            action = ResubmitRepeater(pRepeater, &status);

            if (action & SubmitSend) {
                //
                // Ignore the return value because once we have sent the
                // request, we want all processing to be done in the
                // completion routine.
                //
                (void) pRepeater->Request->GetSubmitFxIrp()->CallDriver(m_Pipe->m_TargetDevice);
            }
        }
    }
}

VOID
FxUsbPipeContinuousReader::_FxUsbPipeRequestWorkItemThunk(
    __in PVOID Context
    )
/*
    Only one work-item can be in-progress at any given time and
    only one additional work-item can be queued at any given time.
    This logic and m_WorkItemQueued makes this happen.
*/
{
    FxUsbPipeRepeatReader* pFailedRepeater;
    FxUsbPipeContinuousReader* pThis;
    FxUsbPipe* pPipe;
    KIRQL irql;
    BOOLEAN rerun, inprogress;

    pFailedRepeater = (FxUsbPipeRepeatReader*) Context;
    pThis = (FxUsbPipeContinuousReader*) pFailedRepeater->Parent;
    pPipe = pThis->m_Pipe;

    //
    // Check if a work item is already in progress.
    //
    pPipe->Lock(&irql);
    if (pThis->m_WorkItemFlags & FX_USB_WORKITEM_IN_PROGRESS) {
        //
        // Yes, just let the other thread re-run this logic.
        //
        inprogress = TRUE;

        ASSERT((pThis->m_WorkItemFlags & FX_USB_WORKITEM_RERUN) == 0);
        pThis->m_WorkItemFlags |= FX_USB_WORKITEM_RERUN;

        ASSERT(NULL == pThis->m_WorkItemRerunContext);
        pThis->m_WorkItemRerunContext = Context;
    }
    else {
        //
        // No, it not running.
        //
        inprogress = FALSE;

        pThis->m_WorkItemFlags |= FX_USB_WORKITEM_IN_PROGRESS;
        ASSERT((pThis->m_WorkItemFlags & FX_USB_WORKITEM_RERUN) == 0);
    }
    pPipe->Unlock(irql);

    if (inprogress) {
        return;
    }

    //
    // OK, this thread is responsible for running the work item logic.
    //
    do {
        //
        // Cleanup and restart the repeters.
        //
        pThis->FxUsbPipeRequestWorkItemHandler(pFailedRepeater);

        //
        // Check if callback needs to be re-run.
        //
        pPipe->Lock(&irql);
        if (pThis->m_WorkItemFlags & FX_USB_WORKITEM_RERUN) {
            //
            // Yes, a new work item was requested while it was already running.
            //
            rerun = TRUE;

            pThis->m_WorkItemFlags &= ~FX_USB_WORKITEM_RERUN;

            ASSERT(pThis->m_WorkItemRerunContext != NULL);
            pFailedRepeater = (FxUsbPipeRepeatReader*)pThis->m_WorkItemRerunContext;
            pThis->m_WorkItemRerunContext = NULL;

            ASSERT(pThis == (FxUsbPipeContinuousReader*)pFailedRepeater->Parent);
        }
        else {
            //
            // No, all done.
            //
            rerun = FALSE;

            ASSERT(pThis->m_WorkItemFlags & FX_USB_WORKITEM_IN_PROGRESS);
            pThis->m_WorkItemFlags &= ~FX_USB_WORKITEM_IN_PROGRESS;

            ASSERT(NULL == pThis->m_WorkItemRerunContext);
        }
        pPipe->Unlock(irql);

    }
    while (rerun);
}

PVOID
FxUsbPipeContinuousReader::operator new(
    __in size_t Size,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __range(1, NUM_PENDING_READS_MAX) ULONG NumReaders
    )
{
    ASSERT(NumReaders >= 1);

    return FxPoolAllocate(
        FxDriverGlobals,
        NonPagedPool,
        Size + (NumReaders-1) * sizeof(FxUsbPipeRepeatReader)
        );
}

_Must_inspect_result_
NTSTATUS
FxUsbPipeContinuousReader::FormatRepeater(
    __in FxUsbPipeRepeatReader* Repeater
    )
{
    WDF_REQUEST_REUSE_PARAMS params;
    FxRequestBuffer buf;
    FxUsbPipeTransferContext* pContext;
    FxMemoryObject* pMemory;
    FxRequest* pRequest;
    NTSTATUS status;

    pRequest = Repeater->Request;
    //
    // The repeater owns the request memory.  If there is a memory on the
    // context, delete it now.  the memory will still be referencable since
    // it will still have a reference against it until FormatTransferRequest is
    // called or the request is freed and the context releases its references
    //
    DeleteMemory(pRequest);

    WDF_REQUEST_REUSE_PARAMS_INIT(&params, 0, STATUS_NOT_SUPPORTED);

    pRequest->Reuse(&params);

    //
    // pMemory will be deleted when either
    // a) The request completes
    // or
    // b) The continuous reader is destroyed and we delete the lookaside.  since
    //    the lookaside is the parent object for pMemory, pMemory will be disposed
    //    of when the parent is Disposed
    //
    status = m_Lookaside->Allocate(&pMemory);
    if (!NT_SUCCESS(status)) {
        FxRequestContext* pContext;

        pContext = pRequest->GetContext();
        if (pContext != NULL ) {
           pContext->m_RequestMemory = NULL;
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pMemory->GetBuffer(), pMemory->GetBufferSize());

    buf.SetMemory(pMemory, &m_Offsets);

    status = m_Pipe->FormatTransferRequest(
        pRequest,
        &buf,
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK
        );

    if (!NT_SUCCESS(status)) {
        //
        // In the case of failure, the context in the request will delete the
        // memory.  If there is no context, delete the memory here.
        //
        if (pRequest->GetContext() == NULL) {
            //
            // Use DeleteFromFailedCreate because the driver never saw the
            // buffer, so they shouldn't be told about it going away.
            //
            pMemory->DeleteFromFailedCreate();
        }

        return status;
    }

    pContext = (FxUsbPipeTransferContext*) pRequest->GetContext();
    pContext->SetUsbType(WdfUsbRequestTypePipeRead);
    pContext->m_UsbParameters.Parameters.PipeRead.Buffer = (WDFMEMORY)
        pMemory->GetObjectHandle();

    pRequest->SetCompletionRoutine(_FxUsbPipeRequestComplete, Repeater);
    return status;
}


VOID
FxUsbPipeContinuousReader::CancelRepeaters(
    VOID
    )
{
    LONG i;

    Mx::MxEnterCriticalRegion();

    for (i = 0; i < m_NumReaders; i++) {
        m_Readers[i].Request->Cancel();
        m_Pipe->GetDriverGlobals()->WaitForSignal(
                m_Readers[i].ReadCompletedEvent.GetSelfPointer(),
                "waiting for continuous reader to finish, WDFUSBPIPE",
                m_Pipe->GetHandle(),
                m_Pipe->GetDriverGlobals()->FxVerifierDbgWaitForSignalTimeoutInSec,
                WaitSignalBreakUnderVerifier);

    }

    Mx::MxLeaveCriticalRegion();
    //
    // Checking for IO Count <= 1 is not a good idea here because there could be always other IO
    // besides that from the continous reader going on the Read Pipe.
    //
}

FxUsbPipeTransferContext::FxUsbPipeTransferContext(
    __in FX_URB_TYPE FxUrbType
    ) :
    FxUsbRequestContext(FX_RCT_USB_PIPE_XFER)
{
    m_UnlockPages = FALSE;
    m_PartialMdl = NULL;
    m_USBDHandle = NULL;

    if (FxUrbType == FxUrbTypeLegacy) {
        m_Urb = &m_UrbLegacy;
    }
    else {
        m_Urb = NULL;
    }

}

FxUsbPipeTransferContext::~FxUsbPipeTransferContext(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)) {
        USBD_UrbFree(m_USBDHandle, (PURB)m_Urb);
    }
    m_Urb = NULL;
    m_USBDHandle = NULL;
}

__checkReturn
NTSTATUS
FxUsbPipeTransferContext::AllocateUrb(
    __in USBD_HANDLE USBDHandle
    )
{
    NTSTATUS status;

    if (m_Urb) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = USBD_UrbAllocate(USBDHandle, (PURB*)&m_Urb);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    m_USBDHandle = USBDHandle;

Done:
    return status;
}

VOID
FxUsbPipeTransferContext::Dispose(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)){
        USBD_UrbFree(m_USBDHandle, (PURB) m_Urb);
        m_Urb = NULL;
        m_USBDHandle = NULL;
    }
}

VOID
FxUsbPipeTransferContext::ReleaseAndRestore(
    __in FxRequestBase* Request
    )
{
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // Check now because Init will NULL out the field
    //
    if (m_PartialMdl != NULL) {
        if (m_UnlockPages) {
            MmUnlockPages(m_PartialMdl);
            m_UnlockPages = FALSE;
        }

        FxMdlFree(Request->GetDriverGlobals(), m_PartialMdl);
        m_PartialMdl = NULL;
    }
#endif
    __super::ReleaseAndRestore(Request);
}

VOID
FxUsbPipeTransferContext::CopyParameters(
    __in FxRequestBase* Request
    )
{
    m_CompletionParams.IoStatus.Information = GetUrbTransferLength();

    //
    // If both are at the same offset, we don't have to compare type for
    // Read or Write
    //
    WDFCASSERT(FIELD_OFFSET(WDF_USB_REQUEST_COMPLETION_PARAMS,
                            Parameters.PipeRead.Length) ==
               FIELD_OFFSET(WDF_USB_REQUEST_COMPLETION_PARAMS,
                            Parameters.PipeWrite.Length));

    m_UsbParameters.Parameters.PipeRead.Length = GetUrbTransferLength();
    __super::CopyParameters(Request);
}

VOID
FxUsbPipeTransferContext::SetUrbInfo(
    __in USBD_PIPE_HANDLE PipeHandle,
    __in ULONG TransferFlags
    )
{
    m_Urb->TransferFlags = TransferFlags;
    m_Urb->PipeHandle = PipeHandle;
}

USBD_STATUS
FxUsbPipeTransferContext::GetUsbdStatus(
    VOID
    )
{
    return m_Urb->Hdr.Status;
}

FxUsbUrbContext::FxUsbUrbContext(
    VOID
    ) :
    FxUsbRequestContext(FX_RCT_USB_URB_REQUEST),
    m_pUrb(NULL)
{
}

USBD_STATUS
FxUsbUrbContext::GetUsbdStatus(
    VOID
    )
{
    return m_pUrb == NULL ? 0 : m_pUrb->UrbHeader.Status;
}

VOID
FxUsbUrbContext::StoreAndReferenceMemory(
    __in FxRequestBuffer* Buffer
    )
{
    ULONG dummy;

    FxUsbRequestContext::StoreAndReferenceMemory(Buffer);

    //
    // make sure it is framework managed memory or raw PVOID
    //
    ASSERT(Buffer->DataType == FxRequestBufferMemory ||
           Buffer->DataType == FxRequestBufferBuffer);

    Buffer->AssignValues((PVOID*) &m_pUrb, NULL, &dummy);
}

VOID
FxUsbUrbContext::ReleaseAndRestore(
    __in FxRequestBase* Request
    )
{
    m_pUrb = NULL;
    __super::ReleaseAndRestore(Request);
}


FxUsbPipeRequestContext::FxUsbPipeRequestContext(
    __in FX_URB_TYPE FxUrbType
    ) :
    FxUsbRequestContext(FX_RCT_USB_PIPE_REQUEST)
{
    m_USBDHandle = NULL;

    if (FxUrbType == FxUrbTypeLegacy) {
        m_Urb = &m_UrbLegacy;
    }
    else {
        m_Urb = NULL;
    }
}

FxUsbPipeRequestContext::~FxUsbPipeRequestContext(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)) {
        USBD_UrbFree(m_USBDHandle, (PURB)m_Urb);
    }
    m_Urb = NULL;
    m_USBDHandle = NULL;
}

__checkReturn
NTSTATUS
FxUsbPipeRequestContext::AllocateUrb(
    __in USBD_HANDLE USBDHandle
    )
{
    NTSTATUS status;

    if (m_Urb) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = USBD_UrbAllocate(USBDHandle, (PURB*)&m_Urb);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    m_USBDHandle = USBDHandle;

Done:
    return status;
}

VOID
FxUsbPipeRequestContext::Dispose(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)){
        USBD_UrbFree(m_USBDHandle, (PURB) m_Urb);
        m_Urb = NULL;
        m_USBDHandle = NULL;
    }
}

VOID
FxUsbPipeRequestContext::SetInfo(
    __in WDF_USB_REQUEST_TYPE Type,
    __in USBD_PIPE_HANDLE PipeHandle,
    __in USHORT Function
    )
{
    RtlZeroMemory(m_Urb, sizeof(*m_Urb));
    m_Urb->Hdr.Length = sizeof(*m_Urb);
    m_Urb->Hdr.Function = Function;
    m_Urb->PipeHandle = PipeHandle;

    SetUsbType(Type);
}

USBD_STATUS
FxUsbPipeRequestContext::GetUsbdStatus(
    VOID
    )
{
    return m_Urb->Hdr.Status;
}

FxUsbPipe::FxUsbPipe(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxUsbDevice* UsbDevice
    ) :
    FxIoTarget(FxDriverGlobals, sizeof(FxUsbPipe), FX_TYPE_IO_TARGET_USB_PIPE),
    m_UsbDevice(UsbDevice)
{
    InitializeListHead(&m_ListEntry);
    RtlZeroMemory(&m_PipeInformation, sizeof(m_PipeInformation));
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    RtlZeroMemory(&m_PipeInformationUm, sizeof(m_PipeInformationUm));
#endif
    m_InterfaceNumber = 0;
    m_Reader = NULL;
    m_UsbInterface = NULL;
    m_CheckPacketSize = TRUE;
    m_USBDHandle = UsbDevice->m_USBDHandle;
    m_UrbType = UsbDevice->m_UrbType;

    MarkNoDeleteDDI(ObjectDoNotLock);
}

VOID
FxUsbPipe::InitPipe(
    __in PUSBD_PIPE_INFORMATION PipeInfo,
    __in UCHAR InterfaceNumber,
    __in FxUsbInterface* UsbInterface
    )
{
    RtlCopyMemory(&m_PipeInformation, PipeInfo, sizeof(m_PipeInformation));
    m_InterfaceNumber = InterfaceNumber;

    if (m_UsbInterface != NULL) {
        m_UsbInterface->RELEASE(this);
        m_UsbInterface = NULL;
    }

    m_UsbInterface = UsbInterface;
    m_UsbInterface->ADDREF(this);
}

FxUsbPipe::~FxUsbPipe()
{
    if (m_UsbInterface != NULL) {
        m_UsbInterface->RemoveDeletedPipe(this);
        m_UsbInterface->RELEASE(this);
    }

    ASSERT(IsListEmpty(&m_ListEntry));
}

BOOLEAN
FxUsbPipe::Dispose()
{
    BOOLEAN callCleanup;

    //
    // Call base class: callbacks, terminates I/Os, etc.
    //
    callCleanup = __super::Dispose();

    //
    // Don't need the reader anymore. The reader is deleted after calling the
    // parent class Dispose() to preserve the existing deletion order (it was
    // deleted in the Pipe's dtor() before this change).
    //
    if (m_Reader != NULL)
    {
        delete m_Reader;

        //
        // By doing this assignment we prevent misbehaved drivers
        // from crashing the system when they call WdfIoTargetStop from their
        // usb pipe's destroy callback.
        //
        m_Reader = NULL;
    }

    return callCleanup;
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::GotoStartState(
    __in PLIST_ENTRY    RequestListHead,
    __in BOOLEAN        Lock
    )
{
    NTSTATUS status;
    LONG i;

    if (m_Reader != NULL) {
        if (m_Reader->m_ReadersSubmitted == FALSE) {
            ASSERT(IsListEmpty(&m_SentIoListHead));

            for (i = 0; i < m_Reader->m_NumReaders; i++) {
                FxRequest* pRequest;

                pRequest = m_Reader->m_Readers[i].Request;

                UNREFERENCED_PARAMETER(pRequest); //for fre build
                ASSERT(IsListEmpty(&pRequest->m_ListEntry));
                ASSERT(pRequest->m_DrainSingleEntry.Next == NULL);
            }
        }
    }

    status = FxIoTarget::GotoStartState(RequestListHead, Lock);

    if (m_Reader == NULL || !NT_SUCCESS(status)) {
        return status;
    }

    //
    // Add the repeater requests to the list head so that they are sent by the
    // caller of this function when this function returns IFF they have not yet
    // been queued.  (They can be queued on a start -> start transition.)
    //
    if (m_Reader->m_ReadersSubmitted == FALSE) {
        for (i = 0; i < m_Reader->m_NumReaders; i++) {
            //
            // This will clear ReadCompletedEvent as well
            //
            status = m_Reader->FormatRepeater(&m_Reader->m_Readers[i]);

            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        //
        // Reset the number of failed readers in case we had failure in a
        // previously started state.
        //
        m_Reader->m_NumFailedReaders = 0;

        for (i = 0; i < m_Reader->m_NumReaders; i++) {
            FxRequest* pRequest;

            pRequest = m_Reader->m_Readers[i].Request;
            pRequest->SetTarget(this);
            pRequest->ADDREF(this);

            //
            // NOTE: This is an elusive backdoor to send the Request down
            // since it is inserted directly into the IoTargets pended list.
            // The IoTarget is not started so we add the request to the
            // pended list so that it is processed when the IoTarget starts.
            //
            m_Reader->m_Pipe->IncrementIoCount();
            InsertTailList(RequestListHead, &pRequest->m_ListEntry);

            //
            // Clear the event only when we know it will be submitted to the
            // target.  It will be set when  the request is submitted to the
            // target and the submit fails or if it is cancelled.
            //
            m_Reader->m_Readers[i].ReadCompletedEvent.Clear();
        }

        m_Reader->m_ReadersSubmitted = TRUE;
    }

    return status;
}

VOID
FxUsbPipe::GotoStopState(
    __in WDF_IO_TARGET_SENT_IO_ACTION   Action,
    __in PSINGLE_LIST_ENTRY             SentRequestListHead,
    __out PBOOLEAN                      Wait,
    __in BOOLEAN                        LockSelf
    )
{
    KIRQL irql;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    irql = PASSIVE_LEVEL;
    pFxDriverGlobals = GetDriverGlobals();

    if (LockSelf) {
        Lock(&irql);
    }

    if (m_Reader != NULL) {
        //
        // If we are a continuous reader, always cancel the sent io so that we
        // can resubmit it later on a restart.
        //
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p converting stop action %!WDF_IO_TARGET_SENT_IO_ACTION!"
            " to %!WDF_IO_TARGET_SENT_IO_ACTION!", GetHandle(), Action,
            WdfIoTargetCancelSentIo);

        Action = WdfIoTargetCancelSentIo;
    }

    __super::GotoStopState(Action, SentRequestListHead, Wait, FALSE);

    if (m_Reader != NULL) {
        //
        // The continuous reader requests are no longer enqueued.  Remember that
        // state, so when we restart, we resend them.
        //
        m_Reader->m_ReadersSubmitted = FALSE;

        //
        // Log a message when misbehaved drivers call WdfIoTargetStop
        // from EvtUsbTargetPipeReadersFailed callback.
        //
        if (m_Reader->m_WorkItemThread == Mx::MxGetCurrentThread()) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFUSBPIPE %p is stopped from EvtUsbTargetPipeReadersFailed"
                " callback", GetHandle());

            if (pFxDriverGlobals->IsVerificationEnabled(1, 9, OkForDownLevel)) {
                FxVerifierDbgBreakPoint(pFxDriverGlobals);
            }
        }

        //
        // Do not deadlock when misbehaved drivers (< v1.9) call
        // WdfIoTargetStop from EvtUsbTargetPipeReadersFailed callback.
        //
        if (m_Reader->m_WorkItemThread != Mx::MxGetCurrentThread() ||
            pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {
            //
            // Make sure work item is done. It is possible for the upper class
            // to return wait = false if the list of sent requests is empty. We
            // still want to wait anyway for making sure work item is not about
            // to run or it is running.
            //
            *Wait = TRUE;
        }
    }

    if (LockSelf) {
        Unlock(irql);
    }
}

VOID
FxUsbPipe::GotoPurgeState(
    __in WDF_IO_TARGET_PURGE_IO_ACTION  Action,
    __in PLIST_ENTRY                    PendedRequestListHead,
    __in PSINGLE_LIST_ENTRY             SentRequestListHead,
    __out PBOOLEAN                      Wait,
    __in BOOLEAN                        LockSelf
    )
{
    KIRQL irql;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    irql = PASSIVE_LEVEL;
    pFxDriverGlobals = GetDriverGlobals();

    if (LockSelf) {
        Lock(&irql);
    }

    if (m_Reader != NULL) {
        //
        // If we are a continuous reader, always wait for the sent io, so that we
        // can resubmit it later on a restart.
        //
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFUSBPIPE %p converting purge action %!WDF_IO_TARGET_PURGE_IO_ACTION!"
            " to %!WDF_IO_TARGET_PURGE_IO_ACTION!", GetHandle(), Action,
            WdfIoTargetPurgeIoAndWait);

        Action = WdfIoTargetPurgeIoAndWait;
    }

    __super::GotoPurgeState(Action,
                            PendedRequestListHead,
                            SentRequestListHead,
                            Wait,
                            FALSE);

    if (m_Reader != NULL) {
        //
        // The continuous reader requests are no longer enqueued.  Remember that
        // state, so when we restart, we resend them.
        //
        m_Reader->m_ReadersSubmitted = FALSE;

        //
        // Log a message when misbehaved drivers call WdfIoTargetPurge
        // from EvtUsbTargetPipeReadersFailed callback.
        //
        if (m_Reader->m_WorkItemThread == Mx::MxGetCurrentThread()) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFUSBPIPE %p is purged from EvtUsbTargetPipeReadersFailed"
                " callback", GetHandle());

            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }

        //
        // Make sure work item is done. It is possible for the upper class
        // to return wait = false if the list of sent requests is empty. We
        // still want to wait anyway for making sure work item is not about
        // to run or it is running.
        //
        *Wait = TRUE;
    }

    if (LockSelf) {
        Unlock(irql);
    }
}

VOID
FxUsbPipe::GotoRemoveState(
    __in WDF_IO_TARGET_STATE    NewState,
    __in PLIST_ENTRY            PendedRequestListHead,
    __in PSINGLE_LIST_ENTRY     SentRequestListHead,
    __in BOOLEAN                LockSelf,
    __out PBOOLEAN              Wait
    )
{
    KIRQL irql;

    irql = PASSIVE_LEVEL;

    if (LockSelf) {
        Lock(&irql);
    }

    if (m_Reader != NULL && m_Reader->m_ReadersSubmitted &&
        WdfIoTargetStarted == m_State) {
        //
        // Driver forgot to stop the pipe on D0Exit.
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBPIPE %p was not stopped in EvtDeviceD0Exit callback",
            GetHandle());

        if (GetDriverGlobals()->IsVerificationEnabled(1,9,OkForDownLevel)) {
            FxVerifierDbgBreakPoint(GetDriverGlobals());
        }
    }

    __super::GotoRemoveState(NewState,
                             PendedRequestListHead,
                             SentRequestListHead,
                             FALSE,
                             Wait);
    if (m_Reader != NULL) {
        //
        // Make sure work item is done. It is possible for the upper class to
        // return wait = false if the list of sent requests is empty. We still
        // want to wait anyway for making sure work item is not about to run or
        // it is running.
        //
        *Wait = TRUE;
    }

    if (LockSelf) {
        Unlock(irql);
    }
}

VOID
FxUsbPipe::WaitForSentIoToComplete(
    VOID
    )
{
    if (m_Reader != NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBPIPE %p, waiting for continuous reader work item to complete",
            GetHandle());

        //
        // First, wait for the work item to complete if it is running.
        //
        // NOTE: We don't wait for the DPC  to complete because
        //  they are flushed in FxUsbDevice::Dispose
        //
        m_Reader->m_WorkItem->WaitForExit();

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBPIPE %p, cancelling for continuous reader (max of %d)",
            GetHandle(), m_Reader->m_NumReaders);

        //
        // Now that the work item is not running, make sure all the readers are
        // truly canceled and *NOT* in the pended queue.  In between the call to
        // GotoStopState and here, the work item could have run and retried to
        // send the I/O.
        //
        m_Reader->CancelRepeaters();
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFUSBPIPE %p, waiting for all i/o to complete", GetHandle());

    //
    // Finally, let the parent class wait for all I/O to complete
    //
    __super::WaitForSentIoToComplete();
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::InitContinuousReader(
    __in PWDF_USB_CONTINUOUS_READER_CONFIG Config,
    __in size_t TotalBufferLength
    )
{
    FxUsbPipeContinuousReader* pReader;
    NTSTATUS status;
    UCHAR numReaders;

    pReader = NULL;

    if (m_Reader != NULL) {
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Continuous reader already initialized on WDFUSBPIPE %p %!STATUS!",
            GetHandle(), status);

        return status;
    }

    numReaders = Config->NumPendingReads;

    if (numReaders == 0) {
        numReaders = NUM_PENDING_READS_DEFAULT;
    }
    else if (numReaders > NUM_PENDING_READS_MAX) {
        numReaders = NUM_PENDING_READS_MAX;
    }

    pReader = new(GetDriverGlobals(), numReaders)
        FxUsbPipeContinuousReader(this, numReaders);

    if (pReader == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate all of the structurs and objects required
    //
    status = pReader->Config(Config, TotalBufferLength);

    if (!NT_SUCCESS(status)) {
        delete pReader;
        return status;
    }

    pReader->m_ReadCompleteCallback = Config->EvtUsbTargetPipeReadComplete;
    pReader->m_ReadCompleteContext = Config->EvtUsbTargetPipeReadCompleteContext;

    pReader->m_ReadersFailedCallback = Config->EvtUsbTargetPipeReadersFailed;

    if (InterlockedCompareExchangePointer((PVOID*) &m_Reader,
                                          pReader,
                                          NULL) == NULL) {
        //
        // We set the field, do nothing.
        //
        DO_NOTHING();
    }
    else {
        //
        // Some other thread came in and set the field, free our allocation.
        //
        delete pReader;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::_FormatTransfer(
    __in  PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFUSBPIPE Pipe,
    __in WDFREQUEST Request,
    __in_opt WDFMEMORY TransferMemory,
    __in_opt PWDFMEMORY_OFFSET TransferOffsets,
    __in ULONG Flags
    )
{
    FxRequestBuffer buf;
    IFxMemory* pMemory;
    FxUsbPipe* pUsbPipe;
    FxRequest* pRequest;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(FxDriverGlobals,
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &FxDriverGlobals);

    FxObjectHandleGetPtr(FxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    //
    // We allow zero length transfers (which are indicated by TransferMemory == NULL)
    //
    if (TransferMemory != NULL) {
        FxObjectHandleGetPtr(FxDriverGlobals,
                             TransferMemory,
                             IFX_TYPE_MEMORY,
                             (PVOID*) &pMemory);

        status = pMemory->ValidateMemoryOffsets(TransferOffsets);
        if (!NT_SUCCESS(status)) {
            goto Done;
        }

        buf.SetMemory(pMemory, TransferOffsets);
    }
    else {
        pMemory = NULL;
    }

    status = pUsbPipe->FormatTransferRequest(pRequest,  &buf, Flags);

    if (NT_SUCCESS(status)) {
        FxUsbPipeTransferContext* pContext;

        pContext = (FxUsbPipeTransferContext*) pRequest->GetContext();

        //
        // By assuming the fields are at the same offset, we can use simpler
        // logic (w/out comparisons for type) to set them.
        //
        WDFCASSERT(
            FIELD_OFFSET(WDF_USB_REQUEST_COMPLETION_PARAMS, Parameters.PipeWrite.Buffer) ==
            FIELD_OFFSET(WDF_USB_REQUEST_COMPLETION_PARAMS, Parameters.PipeRead.Buffer)
            );

        WDFCASSERT(
            FIELD_OFFSET(WDF_USB_REQUEST_COMPLETION_PARAMS, Parameters.PipeWrite.Offset) ==
            FIELD_OFFSET(WDF_USB_REQUEST_COMPLETION_PARAMS, Parameters.PipeRead.Offset)
            );

        pContext->m_UsbParameters.Parameters.PipeWrite.Buffer = TransferMemory;
        pContext->m_UsbParameters.Parameters.PipeWrite.Length = buf.GetBufferLength();

        pContext->m_UsbParameters.Parameters.PipeWrite.Offset =
            (TransferOffsets != NULL) ? TransferOffsets->BufferOffset
                                      : 0;
        pContext->SetUsbType(
            (Flags & USBD_TRANSFER_DIRECTION_IN) ? WdfUsbRequestTypePipeRead
                                                 : WdfUsbRequestTypePipeWrite
            );
    }

Done:
    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p, WDFREQUEST %p, WDFMEMORY %p, %!STATUS!",
                        Pipe, Request, TransferMemory, status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::_SendTransfer(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFUSBPIPE Pipe,
    __in_opt WDFREQUEST Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt PULONG BytesTransferred,
    __in ULONG Flags
    )
{
    FxRequestBuffer buf;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(FxDriverGlobals,
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &FxDriverGlobals);

    FxUsbPipeTransferContext context(FxUrbTypeLegacy);

    FxSyncRequest request(FxDriverGlobals, &context, Request);

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    if (BytesTransferred != NULL) {
        *BytesTransferred = 0;
    }

    status = FxVerifierCheckIrqlLevel(FxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(FxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // We allow zero length writes (which are indicated by MemoryDescriptor == NULL)
    //
    if (MemoryDescriptor != NULL) {
        status = buf.ValidateMemoryDescriptor(FxDriverGlobals, MemoryDescriptor);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = pUsbPipe->FormatTransferRequest(request.m_TrueRequest, &buf, Flags);

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBPIPE %p, WDFREQUEST %p being submitted",
            Pipe, request.m_TrueRequest->GetTraceObjectHandle());

        status = pUsbPipe->SubmitSync(request.m_TrueRequest, RequestOptions);

        //
        // Even on error we want to set this value.  USBD should be clearing
        // it if the transfer fails.
        //
        if (BytesTransferred != NULL) {
            *BytesTransferred = context.GetUrbTransferLength();
        }
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p, %!STATUS!", Pipe, status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::FormatAbortRequest(
    __in FxRequestBase* Request
    )
{
    FxUsbPipeRequestContext* pContext;
    NTSTATUS status;
    FX_URB_TYPE urbType;

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Pipe %p, Request %p, setting target failed, "
                            "status %!STATUS!", this, Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_PIPE_REQUEST)) {
        pContext = (FxUsbPipeRequestContext*) Request->GetContext();
    }
    else {

        urbType = m_UsbDevice->GetFxUrbTypeForRequest(Request);
        pContext = new(GetDriverGlobals()) FxUsbPipeRequestContext(urbType);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        if (urbType == FxUrbTypeUsbdAllocated) {
            status = pContext->AllocateUrb(m_USBDHandle);
            if (!NT_SUCCESS(status)) {
                delete pContext;
                return status;
            }
            //
            // Since the AllocateUrb routine calls USBD_xxxUrbAllocate APIs to allocate an Urb, it is
            // important to release those resorces before the devnode is removed. Those
            // resoruces are removed at the time Request is disposed.
            //
            Request->EnableContextDisposeNotification();
        }
#endif

        Request->SetContext(pContext);
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    pContext->SetInfo(WdfUsbRequestTypePipeAbort,
                      m_PipeInformation.PipeHandle,
                      URB_FUNCTION_ABORT_PIPE);

    if (pContext->m_Urb == &pContext->m_UrbLegacy) {
        urbType = FxUrbTypeLegacy;
    }
    else {
        urbType = FxUrbTypeUsbdAllocated;
    }

    FxFormatUsbRequest(Request, (PURB)pContext->m_Urb, urbType, m_USBDHandle);
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    pContext->SetInfo(WdfUsbRequestTypePipeAbort,
                      m_UsbInterface->m_WinUsbHandle,
                      m_PipeInformationUm.PipeId,
                      UMURB_FUNCTION_ABORT_PIPE);
    FxUsbUmFormatRequest(Request, &pContext->m_UmUrb.UmUrbPipeRequest.Hdr, m_UsbDevice->m_pHostTargetFile);
#endif

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::FormatResetRequest(
    __in FxRequestBase* Request
    )
{
    FxUsbPipeRequestContext* pContext;
    NTSTATUS status;
    FX_URB_TYPE urbType;

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Pipe %p, Request %p, setting target failed, "
                            "status %!STATUS!", this, Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_PIPE_REQUEST)) {
        pContext = (FxUsbPipeRequestContext*) Request->GetContext();
    }
    else {
        urbType = m_UsbDevice->GetFxUrbTypeForRequest(Request);
        pContext = new(GetDriverGlobals()) FxUsbPipeRequestContext(urbType);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        if (urbType == FxUrbTypeUsbdAllocated) {
            status = pContext->AllocateUrb(m_USBDHandle);
            if (!NT_SUCCESS(status)) {
                delete pContext;
                return status;
            }
            //
            // Since the AllocateUrb routine calls USBD_xxxUrbAllocate APIs to allocate an Urb, it is
            // important to release those resorces before the devnode is removed. Those
            // resoruces are removed at the time Request is disposed.
            //
            Request->EnableContextDisposeNotification();
        }
#endif

        Request->SetContext(pContext);
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // URB_FUNCTION_RESET_PIPE and URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL
    // are the same value
    //
    pContext->SetInfo(WdfUsbRequestTypePipeReset,
                      m_PipeInformation.PipeHandle,
                      URB_FUNCTION_RESET_PIPE);

    if (pContext->m_Urb == &pContext->m_UrbLegacy) {
        urbType = FxUrbTypeLegacy;
    }
    else {
        urbType = FxUrbTypeUsbdAllocated;
    }

    FxFormatUsbRequest(Request, (PURB)pContext->m_Urb, urbType, m_USBDHandle);
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    pContext->SetInfo(WdfUsbRequestTypePipeReset,
                      m_UsbInterface->m_WinUsbHandle,
                      m_PipeInformationUm.PipeId,
                      UMURB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL);
    FxUsbUmFormatRequest(Request, &pContext->m_UmUrb.UmUrbPipeRequest.Hdr, m_UsbDevice->m_pHostTargetFile);
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
FxUsbPipe::Reset(
    VOID
    )
{
    FxUsbPipeRequestContext context(FxUrbTypeLegacy);

    FxSyncRequest request(GetDriverGlobals(), &context);
    NTSTATUS status;

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    status = FormatResetRequest(request.m_TrueRequest);
    if (NT_SUCCESS(status)) {
        if (m_Reader != NULL) {
            //
            // This assumes that no other I/O besides reader I/O is going on.
            //
            m_Reader->CancelRepeaters();
        }
        else {
             CancelSentIo();
        }
        status = SubmitSyncRequestIgnoreTargetState(request.m_TrueRequest, NULL);
    }
    return status;
}

