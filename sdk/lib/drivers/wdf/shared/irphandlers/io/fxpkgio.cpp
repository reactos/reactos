/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgIo.cpp

Abstract:

    This module implements the I/O package for the driver frameworks.

Author:



Environment:

    Both kernel and user mode

Revision History:



--*/

#include "ioprivshared.hpp"

// Tracing support
extern "C" {
#if defined(EVENT_TRACING)
#include "FxPkgIo.tmh"
#endif
}

//
// This package is initialized by the FxPkgIo::Install virtual method
// being invoked.
//
// A reference is held on it by the FxDevice which owns it. When the
// FxDevice is destroyed, its destructor FxDevice::~FxDevice will release
// its reference to this package, so that FxPkgIo::~FxPkgIo can run.
//
// There is no other package remove, or un-install call.
//

FxPkgIo::FxPkgIo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice *Device
    ) :
    FxPackage(FxDriverGlobals, Device, FX_TYPE_PACKAGE_IO),
    m_InCallerContextCallback(FxDriverGlobals)
{
    LARGE_INTEGER  tickCount;

    m_Device = Device;

    m_DefaultQueue = NULL;

    RtlZeroMemory(m_DispatchTable, sizeof(m_DispatchTable));

    m_Filter = FALSE;

    m_PowerStateOn = FALSE;

    m_QueuesAreShuttingDown = FALSE;

    InitializeListHead(&m_IoQueueListHead);

    InitializeListHead(&m_DynamicDispatchInfoListHead);

    Mx::MxQueryTickCount(&tickCount);

    m_RandomSeed = tickCount.LowPart;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Constructed FxPkgIo 0x%p",this);
}

FxPkgIo::~FxPkgIo()
{
    PLIST_ENTRY next;

    m_DefaultQueue = NULL;

    m_Device = NULL;

    while (!IsListEmpty(&m_DynamicDispatchInfoListHead)) {
        next = RemoveHeadList(&m_DynamicDispatchInfoListHead);
        FxIrpDynamicDispatchInfo* info;
        info = CONTAINING_RECORD(next, FxIrpDynamicDispatchInfo, ListEntry);
        InitializeListHead(next);
        delete info;
    }

    ASSERT(IsListEmpty(&m_IoQueueListHead));

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Destroyed FxPkgIo 0x%p",this);
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::Dispatch(
    __inout MdIrp Irp
    )
{
    FxIrp fxIrp(Irp);
    FX_TRACK_DRIVER(GetDriverGlobals());

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
        "WDFDEVICE 0x%p !devobj 0x%p %!IRPMJ!, IRP_MN %x, IRP 0x%p",
        m_Device->GetHandle(), m_Device->GetDeviceObject(),
        fxIrp.GetMajorFunction(),
        fxIrp.GetMinorFunction(), Irp);

    return DispatchStep1(Irp, m_DynamicDispatchInfoListHead.Flink);
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxPkgIo, VerifyDispatchContext) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ WDFCONTEXT DispatchContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN ctxValid;
    PLIST_ENTRY next;

    PAGED_CODE_LOCKED();

    //
    // Make sure context is valid.
    //
    ctxValid = (PLIST_ENTRY)DispatchContext ==
                    &m_DynamicDispatchInfoListHead ?
                        TRUE : FALSE;

    for (next = m_DynamicDispatchInfoListHead.Flink;
         next != &m_DynamicDispatchInfoListHead;
         next = next->Flink) {
        if ((PLIST_ENTRY)DispatchContext == next) {
            ctxValid = TRUE;
            break;
        }
    }

    if (FALSE == ctxValid) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "DispatchContext 0x%p is invalid, %!STATUS!",
                DispatchContext, status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
__fastcall
FxPkgIo::DispatchStep1(
    __inout MdIrp       Irp,
    __in    WDFCONTEXT  DispatchContext
    )
/*++

    Routine Description:

    Checks for any registered dynamic dispatch callbacks that handles this type of request, else
    selects the default queue based on the IRP's major code.

Arguments:

    Irp - WDM request.

    DispatchContext -  Is the next FxIrpDynamicDispatchInfo element.

Return Value:

    Irp's status.

--*/

{
    NTSTATUS                status;
    FxIrp                   fxIrp(Irp);

    ASSERT(((UCHAR)(ULONG_PTR)DispatchContext & FX_IN_DISPATCH_CALLBACK) == 0);

    ASSERT(fxIrp.GetMajorFunction() <= IRP_MJ_MAXIMUM_FUNCTION);

    //
    // Look for I/O dynamic dispatch callbacks.
    //
    if ((PLIST_ENTRY)DispatchContext != &m_DynamicDispatchInfoListHead) {
        int     index;
        index = FxIrpDynamicDispatchInfo::Mj2Index(fxIrp.GetMajorFunction());

        //
        // Only read/writes/ctrls/internal_ctrls IRPs are allowed, i.e., request cannot
        // IRP type in its callback.
        //
        if (index >= (int)FxIrpDynamicDispatchInfo::DynamicDispatchMax) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                    "Driver cannot change the IRP type in its dispatch "
                    "callback Irp 0x%p, %!IRPMJ!, IRP_MN %x, Device 0x%p, "
                    "%!STATUS!",
                    Irp, fxIrp.GetMajorFunction(), fxIrp.GetMinorFunction(),
                    m_Device->GetHandle(), status);
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            goto CompleteIrp;
        }

        //
        // Verifier checks.
        //
        status = VerifyDispatchContext(GetDriverGlobals(), DispatchContext);
        if( !NT_SUCCESS(status)){
            goto CompleteIrp;
        }

        do {
            FxIrpDynamicDispatchInfo* info;

            info = CONTAINING_RECORD(DispatchContext,
                                     FxIrpDynamicDispatchInfo,
                                     ListEntry);
            //
            // Advance to next node.
            //
            DispatchContext = (WDFCONTEXT)(((PLIST_ENTRY)DispatchContext)->Flink);
            ASSERT(((UCHAR)(ULONG_PTR)DispatchContext & FX_IN_DISPATCH_CALLBACK) == 0);

            ASSERT(fxIrp.GetMajorFunction() == IRP_MJ_READ ||
                   fxIrp.GetMajorFunction() == IRP_MJ_WRITE ||
                   fxIrp.GetMajorFunction() == IRP_MJ_DEVICE_CONTROL ||
                   fxIrp.GetMajorFunction() == IRP_MJ_INTERNAL_DEVICE_CONTROL);

            //
            // If registered, invoke dispatch callback for this major function.
            //
            ASSERT(index < (int)FxIrpDynamicDispatchInfo::DynamicDispatchMax);
            if (NULL != info->Dispatch[index].EvtDeviceDynamicDispatch){
                return info->Dispatch[index].EvtDeviceDynamicDispatch(
                                m_Device->GetHandle(),
                                fxIrp.GetMajorFunction(),
                                fxIrp.GetMinorFunction(),
                                fxIrp.GetParameterIoctlCode(),
                                info->Dispatch[index].DriverContext,
                                reinterpret_cast<PIRP> (fxIrp.GetIrp()),
                                (WDFCONTEXT)((ULONG_PTR)DispatchContext |
                                              FX_IN_DISPATCH_CALLBACK)
                                );
            }
         } while ((PLIST_ENTRY)DispatchContext !=
                                &m_DynamicDispatchInfoListHead);
    }

    //
    // Only now push these local variables on the stack, this is to keep the
    // stack from growing unnecessarily in the dynamic dispatch path above.
    //
    FxIoQueue*              queue;
    FxIoInCallerContext*    ioInCallerCtx;

    //
    // Get the queue from the dispatch-table
    //
    queue = m_DispatchTable[fxIrp.GetMajorFunction()];
    if (queue == NULL) {
        ioInCallerCtx = GetIoInCallerContextCallback(NULL);
        if (ioInCallerCtx->m_Method == NULL) {
            //
            // No queue configured yet, fail request unless the driver is a filter.
            //
            if (m_Filter) {
                goto Forward;
            }

            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                "No queue configured for WDFDEVICE 0x%p, failing IRP 0x%p,"
                " %!STATUS!",
                m_Device->GetHandle(), Irp, status);

            goto CompleteIrp;
        }
    }
    else {
        ioInCallerCtx = GetIoInCallerContextCallback(queue->GetCxDeviceInfo());
    }

    //
    // If the driver is filter and queue is a default-queue then before
    // calling the queue, we should make sure the queue can dispatch
    // requests to the driver. If the queue cannot dispatch request,
    // we should forward it down to the lower driver ourself.
    // This is to cover the scenario where the driver has registered only
    // type specific handler and expect the framework to auto-forward other
    // requests.
    //
    if (m_Filter &&
        ioInCallerCtx->m_Method == NULL &&
        queue == m_DefaultQueue &&
        queue->IsIoEventHandlerRegistered((WDF_REQUEST_TYPE)fxIrp.GetMajorFunction()) == FALSE) {
        //
        // Default queue doesn't have callback events registered to
        // handle this request. So forward it down.
        //
        goto Forward;
    }

    //
    // Finally queue request.
    //
    return DispatchStep2(Irp, ioInCallerCtx, queue);

Forward:

    fxIrp.SkipCurrentIrpStackLocation();
    return fxIrp.CallDriver(m_Device->GetAttachedDevice());

CompleteIrp:

    fxIrp.SetStatus(status);
    fxIrp.SetInformation(0);
    fxIrp.CompleteRequest(IO_NO_INCREMENT);

    return status;
}

_Must_inspect_result_
NTSTATUS
__fastcall
FxPkgIo::DispatchStep2(
    __inout  MdIrp       Irp,
    __in_opt FxIoInCallerContext* IoInCallerCtx,
    __in_opt FxIoQueue*  Queue
    )
{
    NTSTATUS            status;
    FxRequest*          request;
    BOOLEAN             isForwardProgressQueue;
    BOOLEAN             inCriticalRegion;
    PWDF_OBJECT_ATTRIBUTES reqAttribs;
    FxIrp               fxIrp(Irp);

    request = NULL;
    inCriticalRegion = FALSE;
    isForwardProgressQueue = Queue != NULL && Queue->IsForwardProgressQueue();

    ASSERT(fxIrp.GetMajorFunction() <= IRP_MJ_MAXIMUM_FUNCTION);
    ASSERT((IoInCallerCtx != NULL && IoInCallerCtx->m_Method != NULL) ||
            Queue != NULL);
    //
    // The request inserted into the queue can be retrieved and processed
    // by an arbitrary thread. So we need to make sure that such a thread doesn't
    // get suspended and deadlock the driver and potentially the system by
    // entering critical region.The KeEnterCriticalRegion temporarily disables
    // the delivery of normal kernel APCs used to suspend a thread.
    // Kernel APCs queued to this thread will get executed when we leave the
    // critical region.
    //
    if (Mx::MxGetCurrentIrql() <= APC_LEVEL) {
        Mx::MxEnterCriticalRegion();
        inCriticalRegion = TRUE;
    }

    if (Queue != NULL && Queue->GetCxDeviceInfo() != NULL) {
        reqAttribs = &Queue->GetCxDeviceInfo()->RequestAttributes;
    }
    else {
        reqAttribs = m_Device->GetRequestAttributes();
    }

    status = FxRequest::_CreateForPackage(m_Device, reqAttribs, Irp, &request);

    //
    // Check if it is forward progress queue and the EnhancedVerifierOption for
    // testing forward progress are set.
    //
    if (isForwardProgressQueue &&
        NT_SUCCESS(status) &&
        IsFxVerifierTestForwardProgress(GetDriverGlobals())) {
        //
        // This function returns STATUS_INSUFFICIENT_RESOURCES
        // if testing forward progress is enabled and free's the passed in request.
        //
        status = VerifierFreeRequestToTestForwardProgess(request);
    }

    if (!NT_SUCCESS(status)) {
        if (m_Filter && Queue == NULL) {
           goto CompleteIrp;
        }

        if (isForwardProgressQueue) {
            status = Queue->GetReservedRequest(Irp, &request);
            if (status == STATUS_PENDING) {
                goto IrpIsGone;
            }
            else if (!NT_SUCCESS(status)) {
                goto CompleteIrp;
            }
        }
        else {
            //
            // Fail the request
            //
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                "Could not create WDFREQUEST, %!STATUS!", status);

            goto CompleteIrp;
        }
    }
    else {
        if (isForwardProgressQueue) {
            status = Queue->InvokeAllocateResourcesCallback(request);
            if (!NT_SUCCESS(status)) {
                //
                // Failure of the callback means the driver wasn't able to
                // allocate resources for the request. In that case free the
                // request allocated earlier and use the reserved one.
                //
                request->FreeRequest();
                request = NULL;

                status = Queue->GetReservedRequest(Irp, &request);
                if (status == STATUS_PENDING) {
                    goto IrpIsGone;
                }
                else if (!NT_SUCCESS(status)) {
                    goto CompleteIrp;
                }
            }
        }
    }

    //
    // Since we can't guarantee the callback to be called in the context of the
    // caller for reserved requests, we will skip calling InCallerContextCallback
    // for reserverd request.
    //
    if (IoInCallerCtx != NULL &&
        IoInCallerCtx->m_Method != NULL &&
        request->IsReserved() == FALSE) {

        request->SetInternalContext(Queue);
        status = DispathToInCallerContextCallback(IoInCallerCtx, request, Irp);

        //
        // The driver is responsible for calling WdfDeviceEnqueueRequest to
        // insert it back into the I/O processing pipeline, or completing it.
        //
        goto IrpIsGone;
    }

    ASSERT(Queue != NULL);
    status = Queue->QueueRequest(request);
    goto IrpIsGone;

CompleteIrp:

    fxIrp.SetStatus(status);
    fxIrp.SetInformation(0);
    fxIrp.CompleteRequest(IO_NO_INCREMENT);
    //
    // fallthrough
    //
IrpIsGone:

    if (inCriticalRegion) {
        Mx::MxLeaveCriticalRegion();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::InitializeDefaultQueue(
    __in    CfxDevice               * Device,
    __inout FxIoQueue               * Queue
    )

/*++

    Routine Description:

    Make the input queue as the default queue. There can be
    only one queue as the default queue.

    The default queue is the place all requests go to
    automatically if a specific queue was not configured
    for them.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    ULONG index;

    if (m_DefaultQueue != NULL) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Default Queue Already Configured for "
                            "FxPkgIo 0x%p, WDFDEVICE 0x%p %!STATUS!",this,
                            Device->GetHandle(), STATUS_UNSUCCESSFUL);
        return STATUS_UNSUCCESSFUL;
    }

    for (index=0; index <= IRP_MJ_MAXIMUM_FUNCTION; index++) {
        if (m_DispatchTable[index] == NULL) {
            m_DispatchTable[index] = Queue;
        }
    }

    m_DefaultQueue = Queue;

    //
    // Default queue can't be deleted. So mark the object to fail WdfObjectDelete on
    // the default queue.
    //
    Queue->MarkNoDeleteDDI();
    return STATUS_SUCCESS;
}

__inline
FxDriver*
FxPkgIo::GetDriver(
    VOID
    )
{
    return m_Device->GetDriver();
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxPkgIo, VerifyEnqueueRequestUpdateFlags) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request,
    _Inout_ SHORT* OrigVerifierFlags
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE_LOCKED();

    KIRQL irql;

    Request->Lock(&irql);

    *OrigVerifierFlags = Request->GetVerifierFlagsLocked();

    status = Request->VerifyRequestIsInCallerContext(FxDriverGlobals);
    if (NT_SUCCESS(status)) {
        status = Request->VerifyRequestIsDriverOwned(FxDriverGlobals);
    }

    if (NT_SUCCESS(status)) {
        Request->ClearVerifierFlagsLocked(
                FXREQUEST_FLAG_DRIVER_INPROCESS_CONTEXT |
                FXREQUEST_FLAG_DRIVER_OWNED);
    }

    Request->Unlock(irql);
    return status;
}

VOID
FX_VF_METHOD(FxPkgIo, VerifyEnqueueRequestRestoreFlags) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request,
    _In_ SHORT OrigVerifierFlags
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    KIRQL irql;

    PAGED_CODE_LOCKED();

    Request->Lock(&irql);
    Request->ClearVerifierFlagsLocked(~OrigVerifierFlags);
    Request->SetVerifierFlagsLocked(OrigVerifierFlags);
    Request->Unlock(irql);
}


//
// This inserts a request into the I/O processing pipeline
//
_Must_inspect_result_
NTSTATUS
FxPkgIo::EnqueueRequest(
    __in    CfxDevice* Device,
    __inout FxRequest* pRequest
    )
{
    NTSTATUS status;
    FxIoQueue* pQueue;
    FxIrp*     Irp = NULL;
    FxRequestCompletionState oldState;
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    SHORT origVerifierFlags = 0;

    //
    // Request is owned by the driver, and has a reference count of == 1
    // (or > 1 if EvtIoInCallerContext callback took an additional reference),
    // with a FxPkgIoInProcessRequestComplete callback registered.
    //
    ASSERT(pRequest->GetRefCnt() >= 1);

    Irp = pRequest->GetFxIrp();

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "WDFREQUEST 0x%p", pRequest->GetObjectHandle());

    status = VerifyEnqueueRequestUpdateFlags(FxDriverGlobals,
                                             pRequest,
                                             &origVerifierFlags);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the associated queue
    //
    pQueue = (FxIoQueue*) pRequest->GetInternalContext();
    if (NULL == pQueue) {
        pQueue = m_DispatchTable[Irp->GetMajorFunction()];
        if (pQueue == NULL) {
            //
            // No queue configured yet, fail request unless the driver is a filter.
            //
            if (m_Filter) {
                goto Forward;
            }

            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "No queue configured for WDFDEVICE 0x%p, "
                                "failing WDFREQUEST 0x%p %!STATUS!",
                                Device->GetHandle(),
                                pRequest->GetObjectHandle(),
                                status);

            FxVerifierDbgBreakPoint(FxDriverGlobals);

            //
            // Return it back to the driver to decide the outcome
            //
            goto Error;
        }
    }

    //
    // If the queue is a default-queue and driver is a filter then before
    // calling the queue we should make sure the queue can dispatch
    // requests to the driver. If the queue cannot dispatch request,
    // we should forward it down to the lower driver ourself.
    if (m_Filter &&
        pQueue == m_DefaultQueue &&
        pQueue->IsIoEventHandlerRegistered((WDF_REQUEST_TYPE)Irp->GetMajorFunction()) == FALSE) {
        //
        // Default queue doesn't have callback events registered to
        // handle this request. So forward it down.
        //
        goto Forward;
    }

    pQueue->AddRef();

    // Must add a reference before releasing the callback and its reference
    pRequest->ADDREF(FXREQUEST_STATE_TAG);

    // Release the callback
    oldState = pRequest->SetCompletionState(FxRequestCompletionStateNone);
    ASSERT(oldState != FxRequestCompletionStateNone);
    UNREFERENCED_PARAMETER(oldState);

    status = pQueue->QueueRequestFromForward(pRequest);

    pQueue->Release();

    //
    // If not successfull, must place the request back
    // to the state it was in on entry so that the driver
    // can decide what to do next with it
    //
    if (!NT_SUCCESS(status)) {

        //
        // If the request comes back to us, it should still
        // have a reference count of 1
        //
        oldState = pRequest->SetCompletionState(FxRequestCompletionStateIoPkg);

        ASSERT(oldState == FxRequestCompletionStateNone);
        UNREFERENCED_PARAMETER(oldState);

        //
        // Release the reference count on the request since
        // the callback will hold the only one that gets
        // decremented when the request is completed
        //
        pRequest->RELEASE(FXREQUEST_STATE_TAG);
        goto Error;
    }
    else {
        //
        // On success, can not touch the request since it
        // may have already been completed
        //
    }

    return status;

Forward:

    //
    // Cannot send-and-forget a request with a formatted IO context.
    //
    if (pRequest->HasContext()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Cannot send-and-forget WDFREQUEST 0x%p with formatted IO"
            " context for filter WDFDEVICE 0x%p, %!STATUS!",
            pRequest->GetObjectHandle(),
            Device->GetHandle(),
            status );

        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Error;
    }

    //
    // This will skip the current stack location and perform
    // early dispose on the request.
    //
    pRequest->PreProcessSendAndForget();

    (VOID)Irp->CallDriver(Device->GetAttachedDevice());

    //
    // This will delete the request and free the memory back
    // to the device lookaside list.
    //
    pRequest->PostProcessSendAndForget();

    //
    // Return a success status in this code path even if the previous call
    // to send the request to the lower driver failed. The status code returned
    // by this function should reflect the status of enqueuing the request and
    // not the status returned by the lower driver.
    //
    return STATUS_SUCCESS;

Error:

    //
    // If not successful, we must set the original verifier flags.
    //
    VerifyEnqueueRequestRestoreFlags(FxDriverGlobals, pRequest, origVerifierFlags);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::ConfigureDynamicDispatching(
    __in UCHAR               MajorFunction,
    __in_opt FxCxDeviceInfo* CxDeviceInfo,
    __in PFN_WDFDEVICE_WDM_IRP_DISPATCH EvtDeviceWdmIrpDispatch,
    __in_opt WDFCONTEXT      DriverContext
    )
{
    NTSTATUS                    status;
    PFX_DRIVER_GLOBALS          fxDriverGlobals;
    PLIST_ENTRY                 next;
    FxIrpDynamicDispatchInfo*   dispatchInfo;
    LONG                        mjIndex;
    CCHAR                       driverIndex;
    BOOLEAN                     addNew;

    fxDriverGlobals = GetDriverGlobals();
    addNew = TRUE;

    mjIndex = FxIrpDynamicDispatchInfo::Mj2Index(MajorFunction);

    //
    // Indirect MajorFunction validation.
    //
    if (mjIndex >= FxIrpDynamicDispatchInfo::DynamicDispatchMax) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Invalid MajorFunction %!IRPMJ!, %!STATUS!",
                            MajorFunction, status);
        goto Done;
    }

    //
    // Get driver I/O device path index.
    //
    driverIndex = FxDevice::GetCxDriverIndex(CxDeviceInfo);

    //
    // Insert new info into correct slot in the I/O path.
    // Index goes from higher to lower (..., 2, 1, 0) b/c cx's callback is called before
    // client's one.
    //
    for (next = m_DynamicDispatchInfoListHead.Flink;
         next != &m_DynamicDispatchInfoListHead;
         next = next->Flink) {

        CCHAR curIndex = 0;

        dispatchInfo = CONTAINING_RECORD(next,
                                         FxIrpDynamicDispatchInfo,
                                         ListEntry);
        //
        // Get current I/O device path index.
        //
        curIndex = FxDevice::GetCxDriverIndex(dispatchInfo->CxDeviceInfo);
        if (driverIndex == curIndex) {
            //
            // Found it.
            //
            if (dispatchInfo->Dispatch[mjIndex].EvtDeviceDynamicDispatch != NULL) {
                status = STATUS_INVALID_PARAMETER;
                DoTraceLevelMessage(
                    fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Driver %p has already set a dispatch callback for "
                    "%!IRPMJ!, %!STATUS!",
                    CxDeviceInfo == NULL ?
                        GetDriver()->GetHandle() :
                        CxDeviceInfo->Driver->GetHandle(),
                    MajorFunction, status);
                goto Done;
            }

            dispatchInfo->Dispatch[mjIndex].DriverContext = DriverContext;
            dispatchInfo->Dispatch[mjIndex].EvtDeviceDynamicDispatch =
                EvtDeviceWdmIrpDispatch;

            ASSERT(dispatchInfo->CxDeviceInfo == CxDeviceInfo);

            addNew = FALSE;
            break;
        }
        else if (driverIndex > curIndex) {
            //
            // Not found (past valid range), add one before current.
            //
            break;
        }
        else if (driverIndex < curIndex) {
            //
            // Keep looking, too high.
            //
            continue;
        }
    }

    if (addNew) {
        dispatchInfo = new(fxDriverGlobals) FxIrpDynamicDispatchInfo();
        if (dispatchInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                    fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Couldn't create object DynamicDispatchInfo, %!STATUS!",
                    status);
            goto Done;
        }

        dispatchInfo->CxDeviceInfo = CxDeviceInfo;
        dispatchInfo->Dispatch[mjIndex].DriverContext = DriverContext;
        dispatchInfo->Dispatch[mjIndex].EvtDeviceDynamicDispatch =
            EvtDeviceWdmIrpDispatch;

        //
        // We must insert it before 'next' element.
        //
        InsertTailList(next, &dispatchInfo->ListEntry);
    }

    status = STATUS_SUCCESS;

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::ConfigureForwarding(
    __inout FxIoQueue* TargetQueue,
    __in    WDF_REQUEST_TYPE RequestType
    )
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    KIRQL irql;
    NTSTATUS status;

    ASSERT(RequestType <= IRP_MJ_MAXIMUM_FUNCTION);

    if(TargetQueue->IsIoEventHandlerRegistered(RequestType) == FALSE){
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Must have EvtIoDefault or %!WDF_REQUEST_TYPE! "
                            "specific dispatch event registered for "
                            "WDFQUEUE 0x%p, %!STATUS!", RequestType,
                            TargetQueue->GetObjectHandle(),
                            status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return status;
    }

    // Lock IoPackage data structure

    Lock(&irql);

    if (TargetQueue == m_DefaultQueue) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Default WDFQUEUE 0x%p cannot be configured to "
                            "dispatch specific type of request, %!STATUS!",
                            TargetQueue->GetObjectHandle(),
                            status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        Unlock(irql);
        return status;
    }

    // Error if already has an entry
    if (m_DispatchTable[RequestType] != NULL &&
        m_DispatchTable[RequestType] != m_DefaultQueue) {
        status = STATUS_WDF_BUSY;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "%!WDF_REQUEST_TYPE! is already configured for"
                            "WDFQUEUE 0x%p, %!STATUS!", RequestType,
                            TargetQueue->GetObjectHandle(),
                            status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        Unlock(irql);
        return status;
    }

    //
    // We don't take an extra reference count since we already
    // have one from our associated list (DriverQueues)
    //
    m_DispatchTable[RequestType] = TargetQueue;

    //
    // Queues configured to auto-dispatch requests cannot be deleted
    //
    TargetQueue->MarkNoDeleteDDI();

    Unlock(irql);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::CreateQueue(
    __in  PWDF_IO_QUEUE_CONFIG     Config,
    __in  PWDF_OBJECT_ATTRIBUTES   QueueAttributes,
    __in_opt FxDriver*             Caller,
    __deref_out FxIoQueue**        ppQueue
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxObject* pParent;
    FxIoQueue* pQueue;
    NTSTATUS status;
    FxDriver* pDriver;

    pParent = NULL;
    pQueue = NULL;
    pDriver = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    if (QueueAttributes != NULL && QueueAttributes->ParentObject != NULL) {
        CfxDeviceBase* pSearchDevice;

        FxObjectHandleGetPtr(pFxDriverGlobals,
                             QueueAttributes->ParentObject,
                             FX_TYPE_OBJECT,
                             (PVOID*)&pParent);

        pSearchDevice = FxDeviceBase::_SearchForDevice(pParent, NULL);

        if (pSearchDevice == NULL) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "QueueAttributes->ParentObject 0x%p must have WDFDEVICE as an "
                "eventual ancestor, %!STATUS!",
                QueueAttributes->ParentObject, status);

            return status;
        }
        else if (pSearchDevice != m_DeviceBase) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Attributes->ParentObject 0x%p ancestor is WDFDEVICE %p, but "
                "not the same WDFDEVICE 0x%p passed to WdfIoQueueCreate, "
                "%!STATUS!",
                QueueAttributes->ParentObject, pSearchDevice->GetHandle(),
                m_Device->GetHandle(), status);

            return status;
        }
    }
    else {
        //
        // By default, use the package as the parent
        //
        pParent = this;
    }

    //
    // v1.11 and up: get driver object if driver handle is specified.
    // Client driver can also specify a driver handle, the end result in this case is
    // a PkgIoContext that is NULL (see below), i.e., the same as if driver handle
    // was NULL.
    //
    if (Config->Size > sizeof(WDF_IO_QUEUE_CONFIG_V1_9) &&
        Config->Driver != NULL) {

        FxObjectHandleGetPtr(GetDriverGlobals(),
                             Config->Driver,
                             FX_TYPE_DRIVER,
                             (PVOID*)&pDriver);
    }

    status = FxIoQueue::_Create(pFxDriverGlobals,
                            QueueAttributes,
                            Config,
                            Caller,
                            this,
                            m_PowerStateOn,
                            &pQueue
                            );

    if (!NT_SUCCESS(status)) {
        ASSERT(pQueue == NULL);
        return status;
    }

    //
    // Class extension support: associate queue with a specific cx layer.
    //
    if (pDriver != NULL) {
        pQueue->SetCxDeviceInfo(m_Device->GetCxDeviceInfo(pDriver));
    }

    status = pQueue->Commit(QueueAttributes, NULL, pParent);
    if (!NT_SUCCESS(status)) {
       pQueue->DeleteFromFailedCreate();
       return status;
    }

    AddIoQueue(pQueue);
    *ppQueue = pQueue;

    return status;
}


VOID
FxPkgIo::RemoveQueueReferences(
    __inout FxIoQueue* pQueue
    )
/*++

Routine Description:

    This is called from FxIoQueue::Dispose to remove
    the queue from the transaction list.

    Since this acquires the FxPkgIo lock, it assumes that
    no calls are made into FxIoQueue while holding the
    FxPkgIo lock.

Arguments:

    None

Return Value:
    None
  --*/
{
    // Remove it from transacation list
    RemoveIoQueue(pQueue);
    return;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::SetFilter(
    __in BOOLEAN Value
    )
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    if (m_DefaultQueue != NULL) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "I/O Package already has a default queue. "
                            "SetFilter must be called before creating "
                            "a default queue %!STATUS!",
                            STATUS_INVALID_DEVICE_REQUEST);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    m_Filter = Value;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::StopProcessingForPower(
    __in FxIoStopProcessingForPowerAction Action
    )

/*++

    Routine Description:

    Stops all PowerManaged queues automatic I/O processing due to
        a power event that requires I/O to stop.

    This is called on a PASSIVE_LEVEL thread that can block until
    I/O has been stopped, or completed/cancelled.

Arguments:

    Action -

    FxIoStopProcessingForPowerHold:
    the function returns when the driver has acknowledged that it has
    stopped all I/O processing, but may have outstanding "in-flight" requests
    that have not been completed.

    FxIoStopProcessingForPowerPurgeManaged:
    the function returns when all requests from a power managed queue have
    been completed and/or cancelled., and there are no more in-flight requests.

    FxIoStopProcessingForPowerPurgeNonManaged:
    the function returns when all requests from a non-power managed queue have
    been completed and/or cancelled., and there are no more in-flight requests.
    Only called during device-remove.

Return Value:

    NTSTATUS

--*/

{
    KIRQL irql;
    FxIoQueue* queue;
    SINGLE_LIST_ENTRY queueList, *ple;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIO,
                "Perform %!FxIoStopProcessingForPowerAction! for all queues of "
                "WDFDEVICE 0x%p", Action, m_Device->GetHandle());

    queueList.Next = NULL;

    Lock(&irql);
    //
    // Device is moving into low power state. So any new queues
    // created after this point would start off as powered off.
    //
    m_PowerStateOn = FALSE;

    //
    // If queues are shutting down, any new queue created after
    // this point would not accept any requests.
    //
    switch(Action) {
    case FxIoStopProcessingForPowerPurgeManaged:
    case FxIoStopProcessingForPowerPurgeNonManaged:
        m_QueuesAreShuttingDown = TRUE;
        break;
    }

    GetIoQueueListLocked(&queueList, FxIoQueueIteratorListPowerOff);

    Unlock(irql);

    //
    // If the power action is hold then first change the state of all the queues
    // to PowerStartTransition. This will prevent the queues from dispatching
    // new requests before we start asking each queue to stop processing
    // inflight requests.
    //
    if(Action == FxIoStopProcessingForPowerHold) {

        //
        // Walk the list without popping entries because we need to scan
        // the list again.
        //
        for(ple = queueList.Next; ple != NULL; ple = ple->Next) {

            queue = FxIoQueue::_FromPowerSListEntry(ple);

            queue->StartPowerTransitionOff();
        }
    }

    //
    // Ask the queues to stop processing inflight requests.
    //
    for (ple = PopEntryList(&queueList); ple != NULL;
                        ple = PopEntryList(&queueList)) {

        queue = FxIoQueue::_FromPowerSListEntry(ple);

        queue->StopProcessingForPower(Action);

        ple->Next = NULL;

        queue->RELEASE(IO_ITERATOR_POWER_TAG);
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::ResumeProcessingForPower()

/*++

    Routine Description:

    Resumes all PowerManaged queues for automatic I/O processing due to
        a power event that allows I/O to resume.

    Non-PowerManaged queues are left alone.

Arguments:

Return Value:

    NTSTATUS

--*/

{
    KIRQL irql;
    FxIoQueue* queue;
    SINGLE_LIST_ENTRY queueList, *ple;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIO,
                "Power resume all queues of WDFDEVICE 0x%p",
                m_Device->GetHandle());

    queueList.Next = NULL;

    Lock(&irql);

    GetIoQueueListLocked(&queueList, FxIoQueueIteratorListPowerOn);
    //
    // Change the state so that new queues created while we
    // are resuming the existing queues can be in a powered-on state.
    //
    m_PowerStateOn = TRUE;

    //
    // Change the accepting state so that new queues created while we
    // are resuming the existing queues can be accept request.
    //
    m_QueuesAreShuttingDown = FALSE;

    Unlock(irql);

    //
    // We will power-up the queues in two steps. First we will resume
    // the power of all the queues and then we will start dispatching I/Os.
    // This is to avoid a queue being powered up at the begining of the list
    // trying to forward a request to another queue lower in the list that's
    // not powered up yet.
    //
    for(ple = queueList.Next; ple != NULL; ple = ple->Next) {

        queue = FxIoQueue::_FromPowerSListEntry(ple);

        queue->ResumeProcessingForPower();
    }

    for (ple = PopEntryList(&queueList);
         ple != NULL;
         ple = PopEntryList(&queueList)) {

        queue = FxIoQueue::_FromPowerSListEntry(ple);

        queue->StartPowerTransitionOn();

        ple->Next = NULL;

        queue->RELEASE(IO_ITERATOR_POWER_TAG);
    }

    return STATUS_SUCCESS;
}

VOID
FxPkgIo::ResetStateForRestart(
    VOID
    )
/*++

Routine Description:
    This is called on a device which has been restarted from the removed
    state.  It will reset purged queues so that they can accept new requests
    when ResumeProcessingForPower is called afterwards.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;
    FxIoQueue* queue;
    SINGLE_LIST_ENTRY queueList, *ple;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIO,
                "Restart queues from purged state for WDFDEVICE 0x%p due to "
                "device restart", m_Device->GetHandle());

    queueList.Next = NULL;

    Lock(&irql);

    GetIoQueueListLocked(&queueList, FxIoQueueIteratorListPowerOn);

    Unlock(irql);

    for (ple = PopEntryList(&queueList);
            ple != NULL;
            ple = PopEntryList(&queueList)) {

        queue = FxIoQueue::_FromPowerSListEntry(ple);

        queue->ResetStateForRestart();

        ple->Next = NULL;

        queue->RELEASE(IO_ITERATOR_POWER_TAG);
    }

    Lock(&irql);

    m_PowerStateOn = TRUE;

    m_QueuesAreShuttingDown = FALSE;

    Unlock(irql);

    return;

}

_Must_inspect_result_
NTSTATUS
FxPkgIo::FlushAllQueuesByFileObject(
    __in MdFileObject FileObject
    )

/*++

    Routine Description:

        Enumerate all the queues and cancel the requests that have
        the same fileobject as the Cleanup IRP.

        We are making an assumption that cleanup irps are sent only
        at passive-level.

    Return Value:

    NTSTATUS

--*/
{
    FxIoQueue* queue = NULL;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    FxIoQueueNode flushBookmark(FxIoQueueNodeTypeBookmark);
    KIRQL irql;

    if(Mx::MxGetCurrentIrql() != PASSIVE_LEVEL) {

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                        "Currently framework allow flushing of queues "
                        "by fileobject on cleanup only at PASSIVE_LEVEL");

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return STATUS_SUCCESS;
    }

    //
    // Iterate through the queue list and flush each one.
    //
    Lock(&irql);
    queue = GetFirstIoQueueLocked(&flushBookmark, IO_ITERATOR_FLUSH_TAG);
    Unlock(irql);

    while(queue != NULL) {

        queue->FlushByFileObject(FileObject);

        queue->RELEASE(IO_ITERATOR_FLUSH_TAG);

        Lock(&irql);
        queue = GetNextIoQueueLocked(&flushBookmark, IO_ITERATOR_FLUSH_TAG);
        Unlock(irql);
    }

    return STATUS_SUCCESS;
}

static
VOID
GetIoQueueList_ProcessQueueListEntry(
    PLIST_ENTRY         QueueLE,
    PSINGLE_LIST_ENTRY  SListHead,
    PVOID               Tag
    )
{
    FxIoQueueNode*      listNode;
    FxIoQueue*          queue;

    //
    // Skip any nodes that are not queues. They can be bookmarks for
    // in-progress flush operations.
    //
    listNode = FxIoQueueNode::_FromListEntry(QueueLE);
    if (listNode->IsNodeType(FxIoQueueNodeTypeQueue) == FALSE) {
        return;
    }

    queue = FxIoQueue::_FromIoPkgListEntry(QueueLE);
    PushEntryList(SListHead, &queue->m_PowerSListEntry);

    //
    // Add a reference since the request will be touched outside of the
    // lock being held. We will use the enumerant value as the tag.
    //
    queue->ADDREF(Tag);
}

VOID
FxPkgIo::GetIoQueueListLocked(
    __in    PSINGLE_LIST_ENTRY SListHead,
    __inout FxIoIteratorList ListType
    )
/*++

    Routine Description:

        Called to make a temporary list of queues for iteration purpose.
        Function is called with the FxPkg lock held.

--*/
{
    PLIST_ENTRY         listHead, le;

    listHead = &m_IoQueueListHead;

    if (FxIoQueueIteratorListPowerOn == ListType ||
        (FxIoQueueIteratorListPowerOff == ListType &&  // backwards compatibility order.
          m_Device->IsCxInIoPath() == FALSE)) {
        //
        // Power up: first client driver's queues then cx's queues.
        // List is already sorted with client driver's queue first.
        // Since we are inserting into the head of the single list head, if we walked
        // over the list from first to last, we would reverse the entries.  By walking
        // the list backwards, we build the single list head in the order of m_IoQueueListHead.
        //
        for (le = listHead->Blink; le != listHead; le = le->Blink) {
            GetIoQueueList_ProcessQueueListEntry(le,
                                                 SListHead,
                                                 IO_ITERATOR_POWER_TAG);
        }
    }
    else if (FxIoQueueIteratorListPowerOff == ListType) {
        //
        // Power down: first cx's queues then client driver's queues.
        // List is already sorted with client driver's queue first.
        // Since we are inserting into the head of the single list head, if we walked
        // over the list from last to first, we would reverse the entries.  By walking
        // the list forwards, we build the single list head in the desired order
        //
        for (le = listHead->Flink; le != listHead; le = le->Flink) {
            GetIoQueueList_ProcessQueueListEntry(le,
                                                 SListHead,
                                                 IO_ITERATOR_POWER_TAG);
        }
    }
    else {
        ASSERT(FALSE);
    }
}

VOID
FxPkgIo::AddIoQueue(
    __inout FxIoQueue* IoQueue
    )
{
    PLIST_ENTRY         listHead, le;
    FxIoQueue*          queue;
    KIRQL               irql;
    FxIoQueueNode*      listNode;
    CCHAR               queueIndex, curIndex;

    listHead = &m_IoQueueListHead;
    queueIndex = FxDevice::GetCxDriverIndex(IoQueue->GetCxDeviceInfo());
    Lock(&irql);

    ASSERT(IoQueue->m_IoPkgListNode.IsNodeType(FxIoQueueNodeTypeQueue));

    //
    // Insert new queue in sorted list; search from last to first.
    //
    for (le = listHead->Blink; le != listHead; le = le->Blink) {
        //
        // Skip any nodes that are not queues. They can be bookmarks for
        // in-progress flush operations.
        //
        listNode = FxIoQueueNode::_FromListEntry(le);
        if (listNode->IsNodeType(FxIoQueueNodeTypeQueue) == FALSE) {
            continue;
        }

        //
        // Get current queue's driver index.
        //
        queue = FxIoQueue::_FromIoPkgListEntry(le);
        curIndex = FxDevice::GetCxDriverIndex(queue->GetCxDeviceInfo());
        //
        // Queues are inserted in order at the end of its allowed range.
        //
        if (curIndex < queueIndex || curIndex == queueIndex) {
            break;
        }
    }

    InsertHeadList(le, &IoQueue->m_IoPkgListNode.m_ListEntry);

    if (m_PowerStateOn) {
        IoQueue->SetPowerState(FxIoQueuePowerOn);
    } else {
        IoQueue->SetPowerState(FxIoQueuePowerOff);
        if (m_QueuesAreShuttingDown) {
            // Clear accept requests
            IoQueue->SetStateForShutdown();
        }
    }

    Unlock(irql);

    return;
}

VOID
FxPkgIo::RemoveIoQueue(
    __inout FxIoQueue* IoQueue
    )
{
    KIRQL irql;

    Lock(&irql);

    RemoveEntryList(&IoQueue->m_IoPkgListNode.m_ListEntry);
    ASSERT(IoQueue->m_IoPkgListNode.IsNodeType(FxIoQueueNodeTypeQueue));

    InitializeListHead(&IoQueue->m_IoPkgListNode.m_ListEntry);

    Unlock(irql);
}

FxIoQueue*
FxPkgIo::GetFirstIoQueueLocked(
    __in FxIoQueueNode* QueueBookmark,
    __in PVOID Tag
    )
/*++

    Routine Description:

        Inserts the provided bookmark (FxIoQueueNode) at the beginning
        of the IO Package's queue list, and calls GetNextIoQueueLocked
        to retrieve the first queue and to advance the bookmark.

        Function is called with the FxPkg lock held.

    Return Value:

        NULL            if there are no queues in list else
        FxIoQueue*      reference to first queue in list.

--*/
{
    ASSERT(QueueBookmark->IsNodeType(FxIoQueueNodeTypeBookmark));
    ASSERT(IsListEmpty(&QueueBookmark->m_ListEntry));

    InsertHeadList(&m_IoQueueListHead, &QueueBookmark->m_ListEntry);

    return GetNextIoQueueLocked(QueueBookmark, Tag);
}

FxIoQueue*
FxPkgIo::GetNextIoQueueLocked(
    __in FxIoQueueNode* QueueBookmark,
    __in PVOID Tag
    )
/*++

    Routine Description:

        Moves the provided bookmark ahead in the IO Package's queue list
        and returns the next available queue (if any).

    Return Value:

        NULL            if there are no more queues in list else
        FxIoQueue*      reference to the next queue in list.

--*/
{
    PLIST_ENTRY     ple      = NULL;
    FxIoQueue*      queue    = NULL;
    FxIoQueueNode*  listNode = NULL;

    ASSERT(QueueBookmark->IsNodeType(FxIoQueueNodeTypeBookmark));
    ASSERT(IsListEmpty(&QueueBookmark->m_ListEntry) == FALSE);

    //
    // Try to advance bookmark to next location.
    //
    ple = QueueBookmark->m_ListEntry.Flink;
    RemoveEntryList(&QueueBookmark->m_ListEntry);
    InitializeListHead(&QueueBookmark->m_ListEntry);

    for (; ple != &m_IoQueueListHead; ple = ple->Flink) {
        //
        // Skip any nodes that are not queues. These nodes can be
        // bookmarks for in-progress flush operations.
        //
        listNode = FxIoQueueNode::_FromListEntry(ple);
        if (listNode->IsNodeType(FxIoQueueNodeTypeQueue)) {

            //
            // This entry is a real queue.
            //
            queue = FxIoQueue::_FromIoPkgListEntry(ple);
            queue->ADDREF(Tag);

            //
            // Insert bookmark after this entry.
            //
            InsertHeadList(ple, &QueueBookmark->m_ListEntry);

            break;
        }
    }

    return queue;
}

NTSTATUS
FxPkgIo::DispathToInCallerContextCallback(
    __in    FxIoInCallerContext *InCallerContextInfo,
    __in    FxRequest *Request,
    __inout MdIrp      Irp
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequestCompletionState oldState;
    FxIrp fxIrp(Irp);

    pFxDriverGlobals = GetDriverGlobals();

    //
    // Mark the IRP pending since we are going
    // to return STATUS_PENDING regardless of whether
    // the driver completes the request right away
    // or not
    //
    fxIrp.MarkIrpPending();

    if (pFxDriverGlobals->FxVerifierOn) {
        Request->SetVerifierFlags(FXREQUEST_FLAG_DRIVER_INPROCESS_CONTEXT |
                                  FXREQUEST_FLAG_DRIVER_OWNED);
    }

    //
    // Set a completion callback to manage the reference
    // count on the request
    //
    oldState = Request->SetCompletionState(FxRequestCompletionStateIoPkg);

    ASSERT(oldState == FxRequestCompletionStateNone);
    UNREFERENCED_PARAMETER(oldState);

    //
    // Release the reference count on the request since
    // the callback will hold the only one that gets
    // decremented when the request is completed
    //
    Request->RELEASE(FXREQUEST_STATE_TAG);

    Request->SetPresented();

    //
    // Drivers that use this API are responsible for handling
    // all locking, threading, and IRQL level issues...
    //
    InCallerContextInfo->Invoke(m_Device->GetHandle(),
                                Request->GetHandle());
    //
    // The driver is responsible for calling WdfDeviceEnqueueRequest to insert
    // it back into the I/O processing pipeline, or completing it.
    //

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
FxPkgIo::VerifierFreeRequestToTestForwardProgess(
    __in FxRequest* Request
    )
{
    BOOLEAN failAllocation;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();
    failAllocation = FALSE;
    //
    // forwardProgressTestFailAll takes precedence over forwardProgressTestFailRandom
    //
    if (IsFxVerifierTestForwardProgressFailAll(pFxDriverGlobals)) {
        failAllocation = TRUE;
    }
    else if (IsFxVerifierTestForwardProgressFailRandom(pFxDriverGlobals)) {
        //
        // Modulo 17 makes the probability of failure ~6% just like verifier.exe
        //
        failAllocation = (FxRandom(&m_RandomSeed) % 17 == 0);
    }

    if (failAllocation) {
        //
        // Don't use DeleteObject() here as the Request wasn't presented to the
        // driver and the cleanup /dispose callback can be invoked unless
        // we use FreeRequest() which clears the cleanup /dispose callbacks
        //
        Request->FreeRequest();

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        return STATUS_SUCCESS;
    }
}


