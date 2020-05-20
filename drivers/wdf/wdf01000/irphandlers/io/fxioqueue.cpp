/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IoQueue implementation
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */



#include "common/fxioqueue.h"
#include "common/fxdevice.h"
#include "common/fxrequest.h"



//
// Public constructors
//
FxIoQueue::FxIoQueue(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxPkgIo*  PkgIo
    ) :
    FxNonPagedObject(FX_TYPE_QUEUE, sizeof(FxIoQueue), FxDriverGlobals),
    m_CallbackSpinLock(FxDriverGlobals),
    m_CallbackMutexLock(FxDriverGlobals),
    m_IoPkgListNode(FxIoQueueNodeTypeQueue)
 {
    m_Configured = FALSE;
    m_Disposing = FALSE;
    m_PowerManaged = FALSE;
    m_PowerState = FxIoQueuePowerOn;
    m_PowerReferenced = FALSE;
    m_AllowZeroLengthRequests = FALSE;
    m_IsDevicePowerPolicyOwner = FALSE;
    m_Type = WdfIoQueueDispatchSequential;

    // A newly created queue can not accept requests until initialized
    m_QueueState = (FX_IO_QUEUE_STATE)0;

    //
    // Set our Cancel callbacks
    //
    m_Queue.Initialize(this, _IrpCancelForQueue);

    m_DriverCancelable.Initialize(this, _IrpCancelForDriver);

    InitializeListHead(&m_Cancelled);

    InitializeListHead(&m_CanceledOnQueueList);

    InitializeListHead(&m_DriverOwned);

    InitializeListHead(&m_PowerNotify);

    InitializeListHead(&m_PowerDriverNotified);

    m_PowerSListEntry.Next = NULL;

    //
    // We do not reference count the I/O package instance
    // since it contains us. The fact we exist, the I/O
    // package instance exists.
    //
    m_PkgIo = PkgIo;
    m_CxDeviceInfo = NULL;

    m_Device = PkgIo->GetDevice();

    m_IsDevicePowerPolicyOwner = (m_Device->IsPnp() &&
                                  m_Device->m_PkgPnp->IsPowerPolicyOwner());

    m_Dispatching = 0L;

    m_TransitionFromEmpty = FALSE;
    m_ForceTransitionFromEmptyWhenAddingNewRequest = FALSE;

    m_DriverIoCount = 0L;
    m_TwoPhaseCompletions = 0L;

    m_SystemWorkItem = NULL;

    m_IdleComplete.Method = NULL;
    m_IdleCompleteContext = NULL;

    m_PurgeComplete.Method = NULL;
    m_PurgeCompleteContext = NULL;

    m_ReadyNotify.Method   = NULL;
    m_ReadyNotifyContext   = NULL;

    m_CallbackLockPtr      = NULL;
    m_CallbackLockObjectPtr = NULL;

#if FX_IS_KERNEL_MODE

    // Initialize the DPC used for deferrals
    KeInitializeDpc(
        &m_Dpc,
        _DeferredDispatchDpcThunk,
        this
        );
#endif

    m_DpcQueued = FALSE;

    m_WorkItemQueued = FALSE;

    m_RequeueDeferredDispatcher = FALSE;

    m_Deleted = FALSE;
    m_SupportForwardProgress = FALSE;
    m_PassiveLevel = FALSE;

    m_ExecutionLevel = WdfExecutionLevelInheritFromParent;
    m_SynchronizationScope = WdfSynchronizationScopeInheritFromParent;

    m_FwdProgContext = NULL;
    MarkPassiveDispose(ObjectDoNotLock);
    m_MaxParallelQueuePresentedRequests = (ULONG)-1;

    return;
}

FxIoQueue::~FxIoQueue()
{
    ASSERT(m_SystemWorkItem == NULL);

    if (m_PkgIo != NULL)
    {
        m_PkgIo = NULL;
    }

    ASSERT(IsListEmpty(&m_Cancelled));
    ASSERT(IsListEmpty(&m_CanceledOnQueueList));
    ASSERT(IsListEmpty(&m_DriverOwned));
    ASSERT(IsListEmpty(&m_PowerNotify));
    ASSERT(IsListEmpty(&m_PowerDriverNotified));
    ASSERT(!m_PowerReferenced);
    ASSERT(!m_DpcQueued);
    ASSERT(!m_WorkItemQueued);
    ASSERT(!m_RequeueDeferredDispatcher);
    ASSERT(m_TwoPhaseCompletions == 0);
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueRequest(
    __in FxRequest* pRequest
    )

/*++

    Routine Description:

    Enqueue a request to the end of the queue.

    Note: This routine owns the final disposition of
          the Request object, and must handle it and
          dereference even if the driver does not.

    Arguments:

        pRequest - Pointer to Request object

    Returns:

        NTSTATUS
--*/

{
    NTSTATUS Status;
    KIRQL    irql;
    MdIrp    pIrp;
    FxIrp*   pFxIrp;

    // Get IoQueue Object Lock
    Lock(&irql);

    ASSERT(pRequest->GetRefCnt() == 1);

    //
    // If the request is reserved, take an additional reference. This reference
    // will be released when the request is completed. This additional reference
    // enables us to detect 2 to 1 transition in the completion path so that
    // we can reclaim the reserved request for reuse.
    //
    if (pRequest->IsReserved())
    {
        pRequest->ADDREF(FXREQUEST_FWDPRG_TAG);
    }

    //
    // If queue is not taking new requests, fail now
    //
    if (!IsState(WdfIoQueueAcceptRequests))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "WDFQUEUE 0x%p is not accepting requests, "
                            "state is %!WDF_IO_QUEUE_STATE!, %s"
                            "completing WDFREQUEST 0x%p %!STATUS!",
                            GetObjectHandle(), m_QueueState,
                            IsState((WDF_IO_QUEUE_STATE)FxIoQueueShutdown) ? 
                                "power stopping (Drain) in progress," : "",
                            pRequest->GetHandle(),
                            STATUS_INVALID_DEVICE_STATE);

        // Must release IoQueue object Lock
        Unlock(irql);

        Status = STATUS_INVALID_DEVICE_STATE;

        // Complete it with error
        pRequest->CompleteWithInformation(Status, 0);

        // Dereference request object
        pRequest->RELEASE(FXREQUEST_COMPLETE_TAG);

        return Status;
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Queuing WDFREQUEST 0x%p on WDFQUEUE 0x%p",
                        pRequest->GetHandle(),GetObjectHandle());

    (VOID)pRequest->GetIrp(&pIrp);

    pFxIrp = pRequest->GetFxIrp();

    pFxIrp->MarkIrpPending();
    
    //
    // If the request is reserved, we may be called to dispatch 
    // a pending reserved IRP from within the context of the completion routine.
    // So to avoid recursion, we will insert the request in the queue and try
    // to dispatch in the return path. If the request is not reserved then we 
    // will dispatch it directly because this path is meant for dispatching new
    // incoming I/O. There is no concern for running into recursion in that
    // scenario.
    //
    if (pRequest->IsReserved() && m_Dispatching != 0)
    { 
        InsertNewRequestLocked(&pRequest, irql);
        Unlock(irql);
    }
    else
    {
        DispatchEvents(irql, pRequest);
    }
    
    // We always return status pending through the frameworks
    return STATUS_PENDING;
}

NTSTATUS
FxIoQueue::InsertNewRequestLocked(
    __deref_in FxRequest** Request,
    __in KIRQL PreviousIrql
    )
/*++

    Routine Description:

    Purpose of this function is to insert the request that's dispatched
    by the IoPkg into FxIrpQueue. This function has been added to improve
    the performance of queueing logic. Prior to version 1.7, when a
    request is dispatched to a queue, it was first inserted into queue,
    various checks for the readiness of queue made, and then the request
    is removed from the queue to be presented to the driver. 

    To improve the I/O performance, dispatching logic has been changed
    such that the request will not be inserted into the queue if the queue 
    is ready to dispatch the request. If the queue is not ready or if there 
    are other events to be dispatched before dispatching the new incoming request, 
    we will queue the request first using this function before releasing the lock
    so that we don't change the ordering of requests in the queue.

--*/
{
    NTSTATUS status;
    
    status = (*Request)->InsertTailIrpQueue(&m_Queue, NULL);

    if (!NT_SUCCESS(status))
    {
        //
        // Request was never presented to the driver
        // so there is no need to call CancelForQueue
        // in this case.
        //
        ASSERT(status == STATUS_CANCELLED);
        
        Unlock(PreviousIrql);
        
        (*Request)->CompleteWithInformation(status, 0);
                
        (*Request)->RELEASE(FXREQUEST_COMPLETE_TAG);

        Lock(&PreviousIrql);        
    }
    else
    {
        (*Request)->SetCurrentQueue(this);            
        
        // Check if went from no requests to have requests
        CheckTransitionFromEmpty();
    }

    //
    // Request is either inserted into the queue or completed. Clear
    // the field to prevent touching the request.
    //
    *Request = NULL;
    
    return status;
}

VOID
FxIoQueue::SetTransitionFromEmpty(
    VOID
    )
{
    m_TransitionFromEmpty = TRUE;
    m_ForceTransitionFromEmptyWhenAddingNewRequest = FALSE;
    if (m_IsDevicePowerPolicyOwner &&
       m_PowerManaged &&
       m_PowerReferenced == FALSE)
    {
        if (NT_SUCCESS(m_Device->m_PkgPnp->PowerReference(FALSE)))
        {
            m_PowerReferenced = TRUE;
        }
    }
}

_Releases_lock_(this->m_SpinLock.m_Lock)
__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxIoQueue::DispatchEvents(
    __in __drv_restoresIRQL KIRQL PreviousIrql,
    __in_opt FxRequest* NewRequest
    )
/*++

    Routine Description:

    Dispatch events and requests from the queue to the driver.

    The IoQueue object lock must be held on entry, but this routine can release
    and re-acquire the lock multiple times while processing.

    It returns to the caller with the lock released, but queue state may have 
    changed.

    The main processing loop checks for various Queue state change events
    delivering them to the driver, and then finally any WDFREQUEST objects
    that are pending in the Queue.

    The design also handles the recursive case with the m_Dispatching
    field so that a driver that completes requests from within the
    callback does not cause a stack or lock recursion.

    All event callbacks to the device driver are provided though the
    FxCallback object which manages lock acquire and release as required
    by the locking model.

    In addition these may be passive or dispatch level locks.
    If configured for passive level callbacks,
    must defer to a work item if current thread is DISPATCH_LEVEL
    when not owning the current FxIoQueue lock
    
    Arguments:

    NewRequest - This is a new incoming request from the driver above.
                 It will be either presented to the driver or saved into
                 a queue if the conditions are not right to dispatch.

    Returns:

    FALSE if the queue is in a deleted state else TRUE.
    Caller should check for return value only if it's waiting
    on the some events to be invoked by this call.

--*/
{
    FxRequest*  pRequest;
    ULONG       totalIoCount;
    NTSTATUS    status;
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals(); 

    if (m_Deleted)
    {
        ASSERT(NewRequest == NULL);
        Unlock(PreviousIrql);
        return FALSE;
    }

    //        
    // The design of the I/O Queue allows all "events" to notify the driver of
    // to be deferred until the opportune time to deliver them.
    // Depending on the drivers configured locking and threading
    // mode, this may have to be deferred to a worker thread or a DPC
    // to be in a compatible IRQL level, or to prevent a lock recursion
    // when a parent objects lock is in effect. 
    //
    
    if (CanThreadDispatchEventsLocked(PreviousIrql) == FALSE)
    {
        //
        // Previous workitem or Dpc might be running the DispatchEvents right now.
        // But it may be at a point where it might miss out to process the event
        // that we have been asked to dispatch. This is possible because the
        // DispatchEvent is reentrant as it acquires and drops lock along
        // the way. So we make a note of this, so that when the current Dpc or
        // workItem runs to completion, it will requeue itself to handle our message.
        //
        m_RequeueDeferredDispatcher = TRUE;
        
        //
        // Queue the request in to FxIrpQueue and return.
        //
        InsertNewRequest(&NewRequest, PreviousIrql);        
        Unlock(PreviousIrql);
        return TRUE;
    }
       
    //
    // This must be incremented before attempting to deliver any
    // events to the driver. This prevents recursion on the presentation lock, 
    // and limits the stack depth in a Start/Complete/Start/... recursion
    //
    m_Dispatching++;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Thread %p is processing WDFQUEUE 0x%p",
                        Mx::MxGetCurrentThread(), GetObjectHandle());

    //
    // At this point all constaints such as IRQL level, locks held,
    // and stack recursion protection has been satisfied, and we can
    // make callbacks into the device driver.
    //
    // Process events and requests until we either have an empty queue, 
    // the driver stops taking requests, or some queue state does not
    // allow the driver to take new requests
    //
#ifdef _MSC_VER
    #pragma warning(disable:4127)
#endif
    while (TRUE)
    {
#ifdef _MSC_VER
    #pragma warning(default:4127)
#endif
        //
        // totoalIoCount is sum of requests pending in the queue and requests
        // currently owned by the driver.
        //
        totalIoCount = m_Queue.GetRequestCount() + m_DriverIoCount;

        //
        // Increment the count if there is a new request to be dispatched.
        //
        totalIoCount += ((NewRequest != NULL) ? 1 : 0);
        
        if (!IsListEmpty(&this->m_Cancelled))
        {
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }

            //
            // This can drop and re-acquire the queue lock
            // ProcessCancelledRequests returns FALSE if the queue is
            // notifying driver about power state changes.
            //
            if (ProcessCancelledRequests(&PreviousIrql))
            {
                continue;
            }
        }

        if (!IsListEmpty(&this->m_CanceledOnQueueList))
        {
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }

            //
            // This can drop and re-acquire the queue lock
            // ProcessCancelledRequests returns FALSE if the queue is
            // notifying driver about power state changes.
            //
            if (ProcessCancelledRequestsOnQueue(&PreviousIrql))
            {
                continue;
            }
        }

        if (m_IdleComplete.Method != NULL &&
            m_Dispatching == 1L &&
            m_DriverIoCount == 0L)
        {
            InsertNewRequest(&NewRequest, PreviousIrql);

            // no more driver owned requests, we can clear the following flag:
            m_CancelDispatchedRequests = FALSE;
            
            // This can drop and re-acquire the queue lock
            ProcessIdleComplete(&PreviousIrql);
            continue;
        }

        if (m_PurgeComplete.Method != NULL  &&
            totalIoCount == 0L           &&
            m_Dispatching == 1L)
        {
            InsertNewRequest(&NewRequest, PreviousIrql);

            // no more driver owned requests, we can clear the following flag:
            m_CancelDispatchedRequests = FALSE;
            
            // This can drop and re-acquire the queue lock
            ProcessPurgeComplete(&PreviousIrql);
            continue;
        }

        if (m_IsDevicePowerPolicyOwner       &&
            m_PowerManaged                   &&
            m_PowerReferenced                &&
            totalIoCount == 0L               &&
            m_Dispatching == 1L)
        {
            //
            // Queue has no requests, and is going idle. Notify
            // PNP/Power.
            //
            m_Device->m_PkgPnp->PowerDereference();
            m_PowerReferenced = FALSE;
            continue;
        }

        //
        // Look for power state transitions
        //
        if (m_PowerState != FxIoQueuePowerOn &&
            m_PowerState != FxIoQueuePowerOff)
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "WDFQUEUE 0x%p Power Transition State "
                                "%!FxIoQueuePowerState!", GetObjectHandle(),
                                m_PowerState);
            
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }

            // Process intermediate power state
            // This can drop and re-acquire the queue lock
            if (ProcessPowerEvents(&PreviousIrql))
            {
                continue;
            }
            else
            {
                //
                // Return, awaiting some response from the driver
                //
                goto Done;
            }
        }
        else
        {
            // Queue is either in PowerOn or PowerOff state
            DO_NOTHING();
        }

        //
        // Check for queue disposing should be made after processing all
        // the events.
        //
        if (m_Disposing  &&
            totalIoCount == 0L &&
            m_Dispatching == 1L)
        {
            m_Deleted = TRUE;
            
            //
            // After this point, no other thread will be able to dispatch
            // events from this queue. Also threads that are about to call
            // this function as soon as we drop the lock below should have
            // a reference on the queue to prevent queue object from being
            // freed when we signal the dispose thread to run through.
            //
            Unlock(PreviousIrql);

            m_FinishDisposing.Set();
            return TRUE;
        }


        //
        // Return if power is off, can't deliver any request oriented events 
        // to the driver.
        //            
        if (m_PowerState == FxIoQueuePowerOff)
        {
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }
            
            goto Done;    
        }

        //
        // See if the queue is (still) processing requests
        //
        if (!IsState(WdfIoQueueDispatchRequests))
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIO,
                                "WDFQUEUE 0x%p not in dispatching state, "
                                "current state is %!WDF_IO_QUEUE_STATE!",
                                GetObjectHandle(), m_QueueState);

            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }
            
            goto Done;    
        }

        //
        // A manual dispatch queue can have a request ready notification
        // 
        if (m_Type == WdfIoQueueDispatchManual)
        {            
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }

            if (m_ReadyNotify.Method != NULL && m_TransitionFromEmpty)
            {
                // This can drop and re-acquire the lock to callback to the driver
                ProcessReadyNotify(&PreviousIrql);
                continue;
            }

            goto Done;
        }

        if (m_Type == WdfIoQueueDispatchSequential &&  m_DriverIoCount > 0)
        {
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status)) {
                continue;   // totalIoCount may be zero now.
            }
            
            goto Done;    
        }

        //
        // For counted Queue's dont dispatch request to driver if the  
        // m_DriverIoCount exceeds the one set by the driver writer.
        //
        if (m_Type == WdfIoQueueDispatchParallel && 
            (ULONG)m_DriverIoCount >= m_MaxParallelQueuePresentedRequests)
        {
            status = InsertNewRequest(&NewRequest, PreviousIrql);
            if (!NT_SUCCESS(status))
            {
                continue;   // totalIoCount may be zero now.
            }
            
            goto Done;    
        }

        //
        // If there is a request in the queue, then retrieve that.
        //
        pRequest = NULL;
        if (m_Queue.GetRequestCount() > 0L)
        {
            pRequest = FxRequest::GetNextRequest(&m_Queue);
        } 

        //
        // The request from the queue should be dispatched first
        // to preserve the ordering.
        //
        if (pRequest != NULL)
        {
            InsertNewRequest(&NewRequest, PreviousIrql);                
        } 
        else
        {
            //
            // If there is no request in the queue then dispatch
            // the incoming one.
            //
            pRequest = NewRequest;
            if (pRequest != NULL)
            {
                pRequest->SetCurrentQueue(this);              
                SetTransitionFromEmpty();
                NewRequest = NULL;
            } 
            else
            {
                goto Done;
            }
        }

        //
        // pRequest is not cancellable now
        //
        InsertInDriverOwnedList(pRequest);

        Unlock(PreviousIrql);

        DispatchRequestToDriver(pRequest);

        Lock(&PreviousIrql);
    }

Done:
    m_Dispatching--;
    Unlock(PreviousIrql);
    return TRUE;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
FxIoQueue::ProcessReadyNotify(
    __out PKIRQL PreviousIrql
    )
/*++

Routine Description:

    Callback the driver for a Queue ready notify
    Called with the Queue lock held, and may release
    and re-acquire it in calling back the driver

Arguments:

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    WDFCONTEXT              ctx;
    FxIoQueueIoState       callback;

    //
    // A callback to the driver "consumes" the notification.
    // Since we drop the lock when we call the driver, there is
    // a chance for the queue to be stopped or powered-off before
    // the driver tries to retrieve the request. So make sure
    // to set this flag when you power-on or start the queue to 
    // avoid abandoning the requests in the queue.
    //
    m_TransitionFromEmpty = FALSE;

    //
    // Save a local copy since another thread could
    // cancel the ready notification out from under us
    // when we drop the lock
    //
    callback = m_ReadyNotify;

    ctx  = m_ReadyNotifyContext;

    Unlock(*PreviousIrql);

    if (callback.Method != NULL)
    {
        callback.Invoke(GetHandle(), ctx);
    }
    else
    {
        if (FxDriverGlobals->FxVerifierOn)
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "ReadyNotify notify method is NULL "
                                "on WDFQUEUE 0x%p", GetObjectHandle());
            FxVerifierDbgBreakPoint(FxDriverGlobals);
        }
    }

    Lock(PreviousIrql);

    return;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
FxIoQueue::ProcessPurgeComplete(
    __out PKIRQL PreviousIrql
    )
/*++

Routine Description:


    Handle PurgeComplete.

    Calls back the driver if conditions are met.

    Called with Queue lock held, but can drop and re-acquire
    it when delivering the event callback to the device driver.


Arguments:

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    WDFCONTEXT              ctx;
    FxIoQueueIoState       callback;

    callback = m_PurgeComplete;
    ctx = m_PurgeCompleteContext;

    m_PurgeComplete.Method = NULL;
    m_PurgeCompleteContext = NULL;

    Unlock(*PreviousIrql);

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "WDFQUEUE 0x%p is purged, calling driver callback",
                        GetObjectHandle());

    // Notify driver by callback
    if (callback.Method != NULL)
    {
        callback.Invoke(GetHandle(), ctx);
    }

    Lock(PreviousIrql);

    return;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
FxIoQueue::ProcessIdleComplete(
    __out PKIRQL PreviousIrql
    )
/*++

Routine Description:

    Handle IdleComplete.
    Calls back the driver if conditions are met.
    Called with Queue lock held, but can drop and re-acquire
    it when delivering the event callback to the device driver.

Arguments:
    IrpQueue - Queue the request was on

    Irp - the irp being cancelled

    CsqContext - the context associated with the irp

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    WDFCONTEXT              ctx;
    FxIoQueueIoState       callback;


    callback = m_IdleComplete;
    ctx = m_IdleCompleteContext;

    m_IdleComplete.Method = NULL;
    m_IdleCompleteContext = NULL;

    Unlock(*PreviousIrql);

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "WDFQUEUE 0x%p is idle, calling driver callback",
                        GetHandle());

    // Notify driver by callback
    if (callback.Method != NULL)
    {
        callback.Invoke(GetHandle(), ctx);
    }

    Lock(PreviousIrql);

    return;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxIoQueue::ProcessCancelledRequestsOnQueue(
    __out PKIRQL PreviousIrql
    )
/*++

    Routine Description:

    Process any cancelled requests
    Called with the Queue lock held
    Can drop and re-acquire the queue lock
    Returns with the Queue lock held

    Arguments:

    Return Value:
    None

  --*/
{
    PLIST_ENTRY   pEntry;
    FxRequest*  pRequest;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    if (IsPowerStateNotifyingDriver())
    {
        //
        // We will not process cancelled request while the driver is being
        // notified to stop/resume processing request to avoid double
        // completion of the request.
        //
        //return FALSE;
    }

    while (!IsListEmpty(&m_CanceledOnQueueList))
    {
        pEntry = m_CanceledOnQueueList.Flink;

        RemoveEntryList(pEntry);

        // FxRequest ensures its not on any list on checked builds
        InitializeListHead(pEntry);

        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryQueueOwned, pEntry);

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIO,
                            "Calling CanceledOnQueue routine "
                            "for WDFREQUEST 0x%p on WDFQUEUE 0x%p",
                            pRequest->GetHandle(), GetObjectHandle());

        if (FxDriverGlobals->FxVerifierOn)
        {

            // Set cancelled status, otherwise verifier in FxRequest::Complete will complain
            pRequest->SetVerifierFlags(FXREQUEST_FLAG_CANCELLED);
        }

        Unlock(*PreviousIrql);

        if (FxDriverGlobals->FxVerifierOn)
        {
            ASSERT((pRequest->GetVerifierFlags() & FXREQUEST_FLAG_DRIVER_OWNED) != 0);                                
        }

        //
        // Notify the driver
        //
        m_IoCanceledOnQueue.Invoke(GetHandle(), pRequest->GetHandle());

        Lock(PreviousIrql);
    }

    return TRUE;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxIoQueue::ProcessPowerEvents(
    __out PKIRQL PreviousIrql
    )
/*++

Routine Description:

    Processes the power state machine for I/O queues.

    Called with Queue lock held, but can drop and re-acquire
    it when it has to deliver event callbacks to the device driver.

    This can modify queue state as it transits the state machine, and
    is called from the main event processing loop DispatchEvents().

    Handling "in-flight" I/O requests in the device driver due
    to a power state transition (stopping) is tricky, and
    involves a complex state machine.

    The device driver must ensure that all in-flight I/O is stopped
    before it can release the thread calling into the driver to
    perform the power state transition, otherwise a system crash
    can result from accessing hardware after resources have been removed.

    In WDF, this burden is placed on FxIoQueue, so that the device driver
    does not have to implement the more complex aspects of this code,
    but can rely on notifications from the queue serialized under the
    IRQL level and locking it has configured.


    Implementation of FxIoQueue Power state machine:
    ------------------------------------------

    Since we must drop our lock to callback into the device
    driver for power notifications, the processing must occur
    as a state machine with three lists.

    On entry to the power stop state, any "in-flight" I/O requests
    are recorded on the m_DriverOwned list using
    FxRequest::FxListEntryDriverOwned for linkage.

    All of the requests on m_DriverOwned are moved to m_PowerNotify
    while holding the lock, with m_DriverOwned cleared. The state is changed
    to indicate that the driver is now being notified of requests.

    While in the driver notification state, requests are taken from the
    m_PowerNotify list, and moved on to the m_PowerDriverNotified list while
    under the lock, and the request is notified to the device driver
    by the callback while dropping the lock and re-acquiring the lock.

    As the driver acknowledges the power notification, it calls
    WdfRequestStopAcknowledge (FxIoQueue::StopAcknowledge) which moves the
    request from the m_PowerDriverNotified list back to the m_DriverOwned
    list.

    The device driver could also complete requests, in which case they
    just dis-appear from the lists by the completion code doing
    a RemoveEntryList on FxRequest::FxListEntryDriverOwned.

    This occurs until the m_PowerNotify list is empty, in which case
    the state is changed to driver notified.

    While in the driver notified state, the queue event processing continues
    until the m_PowerDriverNotified list is empty, and when it is, the
    stopped state is set, and the event m_PowerIdle is set. This releases
    the thread waiting on the power state transition in which all of the
    device drivers "in-flight" I/O has been stopped and accounted for.

    State Transitions:
    --------------

    During Stop:

    FxIoQueuePowerOn
        --> FxIoQueuePowerStartingTransition
            --> FxIoQueuePowerStopping
                --> FxIoQueuePowerStoppingNotifyingDriver
                    --> FxIoQueuePowerStoppingDriverNotified
                        --> FxIoQueuePowerOff

    During Purge:

    FxIoQueuePowerPurge
        --> FxIoQueuePowerPurgeNotifyingDriver
            --> FxIoQueuePowerPurgeDriverNotified
                --> FxIoQueuePowerOff


    During Resume:

    FxIoQueuePowerOff
        --> FxIoQueuePowerRestarting
            --> FxIoQueuePowerRestartingNotifyingDriver
                --> FxIoQueuePowerRestartingDriverNotified
                    --> FxIoQueuePowerOn

Arguments:

Return Value:

    TRUE - Continue processing the event loop
    FALSE - Stop processing event loop

--*/
{
    PLIST_ENTRY   Entry;
    FxRequest*    pRequest;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    switch(m_PowerState) {

    case FxIoQueuePowerStartingTransition:
        if (m_Dispatching == 1)
        {

            //
            // If we are the last routine actively dispatching callbacks to
            // the device driver under a Power stop, then we must set the
            // event specifying no more callbacks are active.
            //
            m_PowerIdle.Set();
        }

        return FALSE;

    case FxIoQueuePowerStopping:
        //
        // Set state to FxIoQueuePowerPurgeNotifyingDriver
        //
        m_PowerState = FxIoQueuePowerStoppingNotifyingDriver;

        // This should be empty on entry to this state
        ASSERT(IsListEmpty(&this->m_PowerNotify));

        if (!IsListEmpty(&m_DriverOwned))
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Stop: WDFQUEUE 0x%p is powering off "
                                "with in-flight requests",
                                GetObjectHandle());


            // Ensure the logic of m_DriverOwned is correct
            ASSERT(m_DriverOwned.Flink->Blink == &m_DriverOwned);
            ASSERT(m_DriverOwned.Blink->Flink == &m_DriverOwned);

            //
            // Move all requests on m_DriverOwned to m_PowerNotify
            //
            m_PowerNotify.Flink = m_DriverOwned.Flink;
            m_PowerNotify.Blink = m_DriverOwned.Blink;
            m_PowerNotify.Flink->Blink = &m_PowerNotify;
            m_PowerNotify.Blink->Flink = &m_PowerNotify;

            // This is now empty
            InitializeListHead(&m_DriverOwned);
        }
        else
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Stop: WDFQUEUE 0x%p is powering off without "
                                "in-flight requests",GetObjectHandle());
        }

        //
        // Return to main processing loop which will callback to
        // process notifications
        //
        return TRUE;

    case FxIoQueuePowerPurge:
        //
        // Set state to FxIoQueuePowerPurgeNotifyingDriver
        //
        m_PowerState = FxIoQueuePowerPurgeNotifyingDriver;

        // This should be empty on entry to this state
        ASSERT(IsListEmpty(&this->m_PowerNotify));

        if (!IsListEmpty(&m_DriverOwned))
        {

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Stop: WDFQUEUE 0x%p is purging with "
                                "in-flight requests",GetObjectHandle());

            // Ensure the logic of m_DriverOwned is correct
            ASSERT(m_DriverOwned.Flink->Blink == &m_DriverOwned);
            ASSERT(m_DriverOwned.Blink->Flink == &m_DriverOwned);

            //
            // Move all requests on m_DriverOwned to m_PowerNotify
            //
            m_PowerNotify.Flink = m_DriverOwned.Flink;
            m_PowerNotify.Blink = m_DriverOwned.Blink;
            m_PowerNotify.Flink->Blink = &m_PowerNotify;
            m_PowerNotify.Blink->Flink = &m_PowerNotify;

            // This is now empty
            InitializeListHead(&m_DriverOwned);
        }
        else
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power purge: WDFQUEUE 0x%p is purging without "
                                "in-flight requests", GetObjectHandle());
        }

        //
        // Return to main processing loop which will callback to
        // process notifications
        //
        return TRUE;

    case FxIoQueuePowerStoppingNotifyingDriver:  {

        FxIoQueueIoStop   stopCallback;

        //
        // If the list of requests to notify the driver about is
        // empty, change to the notified state.
        //
        if (IsListEmpty(&m_PowerNotify))
        {
            m_PowerState = FxIoQueuePowerStoppingDriverNotified;

            //
            // Return to main processing loop which will callback to
            // process the wait/signaling for the driver to acknowledge
            // all stops.
            //
            return TRUE;
        }

        //
        // Notify each entry in m_PowerNotify into the driver
        //

        // Remove from the notify list, place it on the driver notified list
        Entry = RemoveHeadList(&m_PowerNotify);

        InsertTailList(&m_PowerDriverNotified, Entry);

        // Retrieve the FxRequest
        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryDriverOwned, Entry);

        stopCallback = m_IoStop;

        //
        // Notify driver by callback.
        //
        // If no callback is registered, the power thread will in effect
        // wait until the driver completes or cancels all requests.
        //
        if (stopCallback.Method != NULL && pRequest->m_Canceled == FALSE)
        {
            ULONG ActionFlags = WdfRequestStopActionSuspend;

            if(pRequest->IsInIrpQueue(&m_DriverCancelable))
            {
                ActionFlags |= WdfRequestStopRequestCancelable;
            }

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                "Power Stop Notifying Driver, WDFQUEUE 0x%p, WDFREQUEST 0x%p",
                GetObjectHandle(), pRequest->GetObjectHandle());

            // Driver could be calling RequestComplete as we attempt to stop
            pRequest->ADDREF(FXREQUEST_HOLD_TAG);

            Unlock(*PreviousIrql);

            if (FxDriverGlobals->FxVerifierOn)
            {
                pRequest->SetVerifierFlags(FXREQUEST_FLAG_DRIVER_IN_EVTIOSTOP_CONTEXT);
            }

            stopCallback.Invoke(GetHandle(), pRequest->GetHandle(), ActionFlags);

            pRequest->RELEASE(FXREQUEST_HOLD_TAG);

            Lock(PreviousIrql);
        }

        //
        // As they are acknowledged, they will move back to m_DriverOwned.
        //
        // If the driver completes them, they go away.
        //

        // Return to main processing loop and continue processing notifications
        return TRUE;
    }

    case FxIoQueuePowerPurgeNotifyingDriver: {

        FxIoQueueIoStop   stopCallback;

        //
        // If the list of requests to notify the driver about is
        // empty, change to the notified state.
        //
        if (IsListEmpty(&m_PowerNotify))
        {
            m_PowerState = FxIoQueuePowerPurgeDriverNotified;

            //
            // Return to main processing loop which will callback to
            // process the wait/signaling for the driver to acknowledge
            // all stops.
            //
            return TRUE;
        }

        //
        // Notify each entry in m_PowerNotify into the driver
        //

        // Remove from the notify list, place it on the driver notified list
        Entry = RemoveHeadList(&m_PowerNotify);

        InsertTailList(&m_PowerDriverNotified, Entry);

        // Retrieve the FxRequest
        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryDriverOwned, Entry);

        stopCallback = m_IoStop;

        //
        // Make sure power stop state is cleared before invoking the stop callback.
        //
        pRequest->ClearPowerStopState();
        
        //
        // Notify driver by callback.
        //
        // If no callback is registered, the power thread will in effect
        // wait until the driver completes or cancels all requests.
        //
        if (stopCallback.Method != NULL && pRequest->m_Canceled == FALSE)
        {
            ULONG ActionFlags = WdfRequestStopActionPurge;

            if(pRequest->IsInIrpQueue(&m_DriverCancelable))
            {
                ActionFlags |= WdfRequestStopRequestCancelable;
            }

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Purge Notifying Driver "
                                "WDFQUEUE 0x%p, WDFREQUEST 0x%p",
                                GetObjectHandle(),pRequest->GetHandle());

            // Driver could be calling RequestComplete as we attempt to stop
            pRequest->ADDREF(FXREQUEST_HOLD_TAG);

            Unlock(*PreviousIrql);

            if (FxDriverGlobals->FxVerifierOn)
            {
                pRequest->SetVerifierFlags(FXREQUEST_FLAG_DRIVER_IN_EVTIOSTOP_CONTEXT);
            }

            stopCallback.Invoke(GetHandle(), pRequest->GetHandle(), ActionFlags);

            pRequest->RELEASE(FXREQUEST_HOLD_TAG);
            
            Lock(PreviousIrql);
        }

        //
        // As they are acknowledged, they will move back to m_DriverOwned.
        //
        // If the driver completes them, they go away.
        //

        // Return to main processing loop and continue processing notifications
        return TRUE;
    }

    case FxIoQueuePowerStoppingDriverNotified:
    case FxIoQueuePowerPurgeDriverNotified: {

        PLIST_ENTRY thisEntry, nextEntry, listHead;
        LIST_ENTRY acknowledgedList;
        BOOLEAN continueProcessing = FALSE;

        InitializeListHead(&acknowledgedList);

        //
        // First move all the acknowledged requests to local list and then
        // process the local list. We have do that in two steps because
        // ProcessAcknowledgedRequests drops and reacquires the lock.
        //
        listHead = &m_PowerDriverNotified;

        for (thisEntry = listHead->Flink;
             thisEntry != listHead;
             thisEntry = nextEntry)
        {
            // Retrieve the FxRequest
            pRequest = FxRequest::_FromOwnerListEntry(FxListEntryDriverOwned, thisEntry);

            nextEntry = thisEntry->Flink;

            if (pRequest->IsPowerStopAcknowledged())
            {
                RemoveEntryList(thisEntry);
                InsertTailList(&acknowledgedList, thisEntry);
            }
        }

        //
        // Process all the acknowledged request from the local list.
        //
        while (!IsListEmpty(&acknowledgedList))
        {   
            thisEntry = RemoveHeadList(&acknowledgedList);
            pRequest = FxRequest::_FromOwnerListEntry(FxListEntryDriverOwned, thisEntry);
            ProcessAcknowledgedRequests(pRequest, PreviousIrql);

            //
            // When this function drops the lock, other threads may attempt
            // to dispatch but fail since we are currently dispatching.
            // We need to be sure to process any pending events that other
            // threads initiated but could not be dispatch.  The acknowledged
            // list will eventually be cleared out, allowing exit paths from
            // this function to return control to the driver.
            //
            continueProcessing = TRUE;
        }

        //
        // Check to see if there are any unacknowledged requests.
        //
        if (!IsListEmpty(&m_PowerDriverNotified))
        {
            //
            // If there are still entries on the list, we potentially return
            // FALSE to tell the main event dispatching loop to return to
            // the device driver, since we are awaiting response from
            // the driver while in this state.
            //

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Stop: Waiting for Driver to complete or "
                                "acknowledge in-flight requests on WDFQUEUE 0x%p",
                                GetObjectHandle());

            return continueProcessing;
        }
        
        //
        // Check to see if there are any requests in the middle of two-phase-completion.
        // If so, bail out and wait.
        //
        if (m_TwoPhaseCompletions != 0)
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Stop: Waiting for Driver to complete or "
                                "acknowledge in-flight requests on WDFQUEUE 0x%p",
                                GetObjectHandle());

            return continueProcessing;
        }

        //
        // All the requests are acknowledged. We will signal the pnp thread waiting
        // in StopProcessingForPower to continue if we are the last one to visit
        // the dispatch event loop.
        //
        //
        if ( m_Dispatching == 1)
        {
            //
            // If we are the last routine actively dispatching callbacks to
            // the device driver under a Power stop, then we must set the
            // event specifying no more callbacks are active.
            //

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Stop: WDFQUEUE 0x%p is now powered off with no "
                                "in-flight requests",GetObjectHandle());

            m_PowerState = FxIoQueuePowerOff;

            m_PowerIdle.Set();

            return TRUE;
        }

        //
        // The driver has acknowledged all requests, and the
        // notification list is empty. But, there are still outstanding
        // dispatch calls into the driver (m_Dispatching != 1), so we potentially
        // return false here to hopefully unwind to the final dispatch routine,
        // which will set the power off state.
        //

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Power Stop: Driver has acknowledged all in-flight "
                            "requests, but WDFQUEUE 0x%p has outstanding callbacks",
                            GetObjectHandle());

        return continueProcessing;
    }
    case FxIoQueuePowerRestarting:

        //
        // Power is being resumed to the device. We notify the
        // device driver by an event callback for all driver
        // owned requests that it has idled due to a previous
        // power stop.
        //
        m_PowerState = FxIoQueuePowerRestartingNotifyingDriver;

        // This should be empty on entry to this state
        ASSERT(IsListEmpty(&this->m_PowerNotify));

        if (!IsListEmpty(&m_DriverOwned))
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Resume: Driver has power paused requests "
                                "on WDFQUEUE 0x%p",GetObjectHandle());

            // Ensure the logic of m_DriverOwned is correct
            ASSERT(m_DriverOwned.Flink->Blink == &m_DriverOwned);
            ASSERT(m_DriverOwned.Blink->Flink == &m_DriverOwned);

            //
            // Move all requests on m_DriverOwned to m_PowerNotify
            //
            m_PowerNotify.Flink = m_DriverOwned.Flink;
            m_PowerNotify.Blink = m_DriverOwned.Blink;
            m_PowerNotify.Flink->Blink = &m_PowerNotify;
            m_PowerNotify.Blink->Flink = &m_PowerNotify;

            // This is now empty
            InitializeListHead(&m_DriverOwned);
        }
        else
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Resume: Driver has no power paused requests "
                                "on WDFQUEUE 0x%p", GetObjectHandle());
        }

        //
        // Return to main processing loop which will callback to
        // process notifications
        //
        return TRUE;


    case FxIoQueuePowerRestartingNotifyingDriver:  {

        FxIoQueueIoResume   resumeCallback;

        //
        // If the list of requests to notify the driver about is
        // empty, change to the notified state.
        //
        if (IsListEmpty(&m_PowerNotify))
        {
            m_PowerState = FxIoQueuePowerRestartingDriverNotified;

            //
            // Return to main processing loop which will callback to
            // process the next state
            //
            return TRUE;
        }

        //
        // Notify each entry in m_PowerNotify into the driver, placing them
        // back on the m_DriverOwned list.
        //

        // Remove from the notify list, place it on the driver owned list
        Entry = RemoveHeadList(&m_PowerNotify);

        InsertTailList(&m_DriverOwned, Entry);

        // Retrieve the FxRequest
        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryDriverOwned, Entry);

        resumeCallback = m_IoResume;

        // Notify driver by callback
        if (resumeCallback.Method != NULL && pRequest->m_Canceled == FALSE)
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Resume, Notifying Driver, WDFQUEUE 0x%p, "
                                "WDFREQUEST 0x%p",
                                GetObjectHandle(),
                                pRequest->GetObjectHandle());

            // Driver could be calling RequestComplete as we attempt to resume
            pRequest->ADDREF(FXREQUEST_HOLD_TAG);

            Unlock(*PreviousIrql);

            resumeCallback.Invoke(GetHandle(), pRequest->GetHandle());

            pRequest->RELEASE(FXREQUEST_HOLD_TAG);
            
            Lock(PreviousIrql);
        }
        else
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Power Resume: Driver has no callback for "
                                "EvtIoResume registered on WDFQUEUE 0x%p",GetObjectHandle());
        }

        // Return to main processing loop and continue processing notifications
        return TRUE;
    }

    case FxIoQueuePowerRestartingDriverNotified:

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Power Resume: WDFQUEUE 0x%p is now powered on and "
                            "I/O has resumed",GetObjectHandle());

        // Power state has resumed
        m_PowerState = FxIoQueuePowerOn;

        //
        // We will resume dispatching I/O after all the queues
        // are moved into PowerOn state.
        //
        return FALSE;


    default:
        // Called on invalid state
        ASSERT(FALSE);
        return FALSE;
    }

    /* NOTREACHED*/
}

VOID
FxIoQueue::DispatchRequestToDriver(
    __in FxRequest* pRequest
    )

/*++

    Routine Description:

    Dispatch the next request to the driver.

    The IoQueue object lock is *not* held.

    It returns to the caller with the lock *not* held.

    This is called by DispatchRequests(), and should not be
    called directly in order to maintain queue processing model.

    Arguments:

    Returns:

--*/

{
    PFX_DRIVER_GLOBALS FxDriverGlobals;
    NTSTATUS   Status;
    FxRequestCompletionState oldState;
    WDFREQUEST hRequest;
    FxIrp* pIrp;

    FxDriverGlobals = GetDriverGlobals();

    (VOID)pRequest->GetCurrentIrpStackLocation();

    pIrp = pRequest->GetFxIrp();

    // The Irp does not have a cancel function right now
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    ASSERT(pIrp->GetCurrentIrpStackLocation() != NULL);
#endif

    //
    // Set our completion callback on the request now before
    // calling the driver since the driver can complete the
    // request in the callback handler, and to avoid races with
    // the drivers completion thread.
    //
    // This takes a reference on the request object.
    //
    oldState = pRequest->SetCompletionState(FxRequestCompletionStateQueue);
    ASSERT(oldState == FxRequestCompletionStateNone);
    UNREFERENCED_PARAMETER(oldState);

    if (FxDriverGlobals->FxVerifierOn)
    {
        //
        // If the verifier is on, we do not release the extra
        // reference so we can mark the request as no longer
        // being dispatched to the driver on return from the
        // event callback to the driver
        //

        ASSERT((pRequest->GetVerifierFlags() & FXREQUEST_FLAG_DRIVER_OWNED) == 0);            

        // Mark the request as being "owned" by the driver
        pRequest->SetVerifierFlags(FXREQUEST_FLAG_DRIVER_OWNED |
                                   FXREQUEST_FLAG_DRIVER_DISPATCH);
    }
    else
    {
        //
        // Release our original reference. The FxRequest::Complete
        // will release the final one since we have registered a completion
        // callback handler
        //
        // We now have one reference count on the FxRequest object until
        // its completion routine runs since the completion event made
        // an extra reference, and will dereference it when it fires, or
        // its canceled.
        //

        pRequest->RELEASE(FXREQUEST_STATE_TAG);
    }

    //
    // Attempt to dispatch it to the driver
    //

    //
    // Note: A driver that changes its callback pointers at runtime
    //       could run into a race here since we released the queue
    //       lock. Currently, changing parameters on a processing
    //       queue is undefined.
    //
    //       The C DDI's force the callbacks to be registered at
    //       queue creation time and avoid this race.
    //

    hRequest = pRequest->GetHandle();

    UCHAR majorFunction = pIrp->GetMajorFunction();

    if ((majorFunction == IRP_MJ_READ) && m_IoRead.Method)
    {
        ULONG readLength = pIrp->GetParameterReadLength();

        //
        // Complete zero length reads with STATUS_SUCCESS unless the
        // driver specified it wants them delivered.
        //
        if ((readLength == 0) &&
            !m_AllowZeroLengthRequests)
        {

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                "Zero length WDFREQUEST 0x%p completed automatically "
                "by WDFQUEUE 0x%p", hRequest, GetObjectHandle());

            pRequest->Complete(STATUS_SUCCESS);
            if (FxDriverGlobals->FxVerifierOn)
            {
                //
                // Release the reference taken in the call to SetCompletionState
                // at the top of the function.
                //
                pRequest->RELEASE(FXREQUEST_STATE_TAG);
            }
            return;
        }

        pRequest->SetPresented();

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Calling driver EvtIoRead for WDFREQUEST 0x%p",
                            hRequest);

        m_IoRead.Invoke(
            GetHandle(),
            hRequest,
            readLength
            );
    }
    else if ((majorFunction == IRP_MJ_WRITE) && m_IoWrite.Method)
    {
        ULONG writeLength = pIrp->GetParameterWriteLength();

        //
        // Complete zero length writes with STATUS_SUCCESS unless the
        // driver specified it wants them delivered.
        //
        if ((writeLength == 0) &&
            !m_AllowZeroLengthRequests)
        {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                "Zero length WDFREQUEST 0x%p completed automatically "
                "by WDFQUEUE 0x%p", hRequest, GetObjectHandle());

            pRequest->Complete(STATUS_SUCCESS);

            if (FxDriverGlobals->FxVerifierOn)
            {
                //
                // Release the reference taken in the call to SetCompletionState
                // at the top of the function.
                //
                pRequest->RELEASE(FXREQUEST_STATE_TAG);
            }
            return;
        }

        pRequest->SetPresented();

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Calling driver EvtIoWrite for WDFREQUEST 0x%p",
                            pRequest->GetObjectHandle());

        m_IoWrite.Invoke(
            GetHandle(),
            hRequest,
            writeLength
            );
    }
    else if ((majorFunction == IRP_MJ_DEVICE_CONTROL) && m_IoDeviceControl.Method)
    {
        pRequest->SetPresented();

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Calling driver EvtIoDeviceControl for "
                            "WDFREQUEST 0x%p", hRequest);

        m_IoDeviceControl.Invoke(
            GetHandle(),
            hRequest,
            pIrp->GetParameterIoctlOutputBufferLength(),
            pIrp->GetParameterIoctlInputBufferLength(),
            pIrp->GetParameterIoctlCode()
            );
    }
    else if ( (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) && m_IoInternalDeviceControl.Method)
    {
        pRequest->SetPresented();

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
            "Calling driver EvtIoInternalDeviceControl for WDFREQUEST 0x%p",
            hRequest);

        m_IoInternalDeviceControl.Invoke(
            GetHandle(),
            hRequest,
            pIrp->GetParameterIoctlOutputBufferLength(),
            pIrp->GetParameterIoctlInputBufferLength(),
            pIrp->GetParameterIoctlCode()
            );
    }
    else
    {
        //
        // If we have an IoStart registered, call it
        //
        if (m_IoDefault.Method)
        {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                "Calling driver EvtIoDefault for WDFREQUEST 0x%p", hRequest);


            //
            // If we don't allow zero length requests, we must dig in whether
            // its a read or a write
            //
            if (!m_AllowZeroLengthRequests)
            {
                if (majorFunction == IRP_MJ_READ)
                {
                    if (pIrp->GetParameterReadLength() == 0)
                    {
                        DoTraceLevelMessage(
                            FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Zero length WDFREQUEST 0x%p completed automatically "
                            "by WDFQUEUE 0x%p", hRequest, GetObjectHandle());

                        pRequest->Complete(STATUS_SUCCESS);
                        if (FxDriverGlobals->FxVerifierOn)
                        {
                            //
                            // Release the reference taken in the call to SetCompletionState
                            // at the top of the function.
                            //
                            pRequest->RELEASE(FXREQUEST_STATE_TAG);
                        }
                        return;
                    }
                }
                else if (majorFunction == IRP_MJ_WRITE)
                {
                    if (pIrp->GetParameterWriteLength() == 0)
                    {
                        pRequest->Complete(STATUS_SUCCESS);

                        DoTraceLevelMessage(
                            FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Zero length WDFREQUEST 0x%p completed automatically "
                            "by WDFQUEUE 0x%p", hRequest, GetObjectHandle());

                        if (FxDriverGlobals->FxVerifierOn)
                        {
                            //
                            // Release the reference taken in the call to SetCompletionState
                            // at the top of the function.
                            //
                            pRequest->RELEASE(FXREQUEST_STATE_TAG);
                        }
                        return;
                    }
                }
            }

            pRequest->SetPresented();

            m_IoDefault.Invoke(GetHandle(), hRequest);
        }
        else
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "Driver has no event callback "
                                "for %!WDF_REQUEST_TYPE!, completing WDFREQUEST 0x%p with "
                                "%!STATUS!",
                                majorFunction,
                                pRequest,
                                Status);

            pRequest->Complete(Status);

            if (FxDriverGlobals->FxVerifierOn)
            {
                //
                // Release our extra verifier reference now
                //
                // Release the reference taken in the call to SetCompletionState
                // at the top of the function.
                //
                pRequest->RELEASE(FXREQUEST_STATE_TAG);
            }

            return;
        }
    }

    // ******************************
    // Request may now be a freed object unless verifier is on. Only touch
    // request if verifier is on.
    // ******************************

    if (FxDriverGlobals->FxVerifierOn)
    {
        //
        // If the request has been forwarded, don't clear this
        // since the new queue may already be dispatching in a new thread or DPC
        //
        if ((pRequest->GetVerifierFlags() & FXREQUEST_FLAG_FORWARDED) == 0x0)
        {
            pRequest->ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_DISPATCH);
        }

        //
        // Release our extra verifier reference now
        //
        // Release the reference taken in the call to SetCompletionState

        // at the top of the function.
        //
        pRequest->RELEASE(FXREQUEST_STATE_TAG);
    }

    // Driver accepted a request
    return;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxIoQueue::ProcessCancelledRequests(
    __out PKIRQL PreviousIrql
    )
/*++

    Routine Description:

    Process any cancelled requests
    Called with the Queue lock held
    Can drop and re-acquire the queue lock
    Returns with the Queue lock held

    Arguments:

    Return Value:
    None

  --*/
{
    PLIST_ENTRY   pEntry;
    FxRequest*  pRequest;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    if (IsPowerStateNotifyingDriver())
    {
        //
        // We will not process cancelled request while the driver is being
        // notified to stop processing request to avoid double completion
        // of the request.
        //
        return FALSE;
    }

    while (!IsListEmpty(&m_Cancelled))
    {
        pEntry = m_Cancelled.Flink;

        RemoveEntryList(pEntry);

        // FxRequest ensures its not on any list on checked builds
        InitializeListHead(pEntry);

        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryQueueOwned, pEntry);

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIO,
                            "Calling CancelRoutine routine "
                            "for WDFREQUEST 0x%p on WDFQUEUE 0x%p",
                            pRequest->GetHandle(), GetObjectHandle());

        if (FxDriverGlobals->FxVerifierOn)
        {
            // Set cancelled status, otherwise verifier in FxRequest::Complete will complain
            pRequest->SetVerifierFlags(FXREQUEST_FLAG_CANCELLED);
        }

        Unlock(*PreviousIrql);

        if (FxDriverGlobals->FxVerifierOn)
        {
            ASSERT((pRequest->GetVerifierFlags() & FXREQUEST_FLAG_DRIVER_OWNED) != 0);                                
        }

        //
        // Notify the driver of cancel desire
        //
        pRequest->m_CancelRoutine.InvokeCancel(
            m_IoCancelCallbackLockPtr,
            pRequest->GetHandle()
            );

        //
        // Release the reference that FxRequest took out on itself
        // when it was placed onto the FxIrpQueue. It is now leaving
        // the FxIrpQueue due to cancel, and we own this final
        // release.
        //
        pRequest->RELEASE(FXREQUEST_QUEUE_TAG);

        Lock(PreviousIrql);
    }

    return TRUE;
}

__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
FxIoQueue::ProcessAcknowledgedRequests(
    __in FxRequest* Request,
    __out PKIRQL PreviousIrql
    )
/*++

Routine Description:

    Process requests that are acknowledged by the driver.

    Called with the queue lock held. This function can drop
    and reacquire the lock if needed.

Return Value:

--*/
{
    PLIST_ENTRY Entry;
    BOOLEAN requeue;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    ASSERT(Request->IsPowerStopAcknowledged());

    requeue = Request->IsPowerStopAcknowledgedWithRequeue();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
            "Acknowledging WDFREQUEST %p on WDFQUEUE %p %s requeue option",
            Request->GetObjectHandle(), GetObjectHandle(),
            (requeue ? "with" : "without"));

    Request->ClearPowerStopState();

    //
    // Remove the request from the m_PowerDriverNotified list and
    // place it back on the m_DriverOwned list.
    //
    // N.B.  Our caller guarantees that we have already been removed, thus we
    //       must not explicitly remove here.
    //
    Entry = Request->GetListEntry(FxListEntryDriverOwned);

    InitializeListHead(Entry);

    InsertTailList(&this->m_DriverOwned, Entry);

    if (pFxDriverGlobals->FxVerifierOn)
    {
        //
        // As soon as we drop the lock below the request may get completed.
        // So take an addition reference so that we can safely clear the
        // flag.
        //
        Request->ADDREF(FXREQUEST_HOLD_TAG);
    }

    Unlock(*PreviousIrql);

    if (pFxDriverGlobals->FxVerifierOn)
    {
        Request->ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_IN_EVTIOSTOP_CONTEXT);
        Request->RELEASE(FXREQUEST_HOLD_TAG);
    }

    if (requeue)
    {
        FxRequestCompletionState oldState;
        NTSTATUS status;

        if (pFxDriverGlobals->FxVerifierOn)
        {
            Request->ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_OWNED |
                                         FXREQUEST_FLAG_DRIVER_DISPATCH);
        }

        //
        // If the device driver requests it back on the queue, we will place it
        // on the front and it will be re-delivered in the normal EvtIoDefault, ... path.
        //
        // EvtIoResume *will not* be called on power resume for this request.
        //

        //
        // The request has only one reference, held by the completion
        // callback function. We need to take another one before cancelling
        // this function, otherwise we will lose the request object
        //
        Request->ADDREF(FXREQUEST_STATE_TAG);

        // Cancel the request complete callback (deletes a reference)
        oldState = Request->SetCompletionState(FxRequestCompletionStateNone);
        ASSERT(oldState == FxRequestCompletionStateQueue);
        UNREFERENCED_PARAMETER(oldState);

        Lock(PreviousIrql);

        //
        // We are going to place the request back on the queue
        //

        // Driver is returning I/O
        RemoveFromDriverOwnedList(Request);
        
        //
        // Check if we need to delete this request.
        //
        if (m_CancelDispatchedRequests)
        {
            //
            // Do not requeue this request.
            //
            status = STATUS_CANCELLED;
        }
        else
        {
            //
            // Place the request back at the head of the main queue
            // so as not to re-order requests
            //
            status = Request->InsertHeadIrpQueue(&m_Queue, NULL);
        }
        
        if (!NT_SUCCESS(status))
        {
            // Request not placed in queue, cancel it
            ASSERT(status == STATUS_CANCELLED);

            status = STATUS_SUCCESS;

            //
            // We must add a reference since the CancelForQueue path
            // assumes we were on the FxIrpQueue with the extra reference
            //
            Request->ADDREF(FXREQUEST_QUEUE_TAG);

            //
            // Mark the request as cancelled, place it on the cancel list,
            // and schedule the cancel event to the driver
            //
            CancelForQueue(Request, *PreviousIrql);

            //
            // Reacquire the lock because CancelForQueue visits the dispatch-loop
            // and releases the lock.
            //
            Lock(PreviousIrql);
        }
        else
        {
            // Check if went from no requests to have requests
            CheckTransitionFromEmpty();
        }

    }
    else
    {
        Lock(PreviousIrql);
    }

    return;
}

_Releases_lock_(this->m_SpinLock.m_Lock)
VOID
FxIoQueue::CancelForQueue(
    __in FxRequest* pRequest,
    __in __drv_restoresIRQL KIRQL PreviousIrql
    )
/*++

    Routine Description:

    This routine performs the actions when notified of a cancel
     on a request that has not been presented to the driver

    Return Value:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    FxRequestCompletionState oldState;

    // This is not an error, but want to make sure cancel testing works
    if (FxDriverGlobals->FxVerifierOn)
    {
        // Clear cancellable status, otherwise verifier in FxRequest::Complete will complain
        pRequest->ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_CANCELABLE);

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIO,
                            "WDFREQUEST 0x%p "
                            "was cancelled while on WDFQUEUE 0x%p",
                            pRequest->GetHandle(),GetObjectHandle());
    }

    pRequest->m_Canceled = TRUE;

    pRequest->MarkRemovedFromIrpQueue();

    //
    // Drop the extra reference taken when it was added to the queue
    // because the request is now leaving the queue.
    //
    pRequest->RELEASE(FXREQUEST_QUEUE_TAG);

    //
    // If the driver has registered m_CanceledOnQueue callback, and if
    // the request was ever presented to the driver then we need to
    // notify the driver
    //
    if (m_IoCanceledOnQueue.Method && pRequest->m_Presented)
    {
        //
        // Set the state to indicate the request has come from a queue.
        //
        oldState = pRequest->SetCompletionState(FxRequestCompletionStateQueue);
        ASSERT(oldState == FxRequestCompletionStateNone);
        UNREFERENCED_PARAMETER(oldState);
        
        // Insert it on the driver owned list
        InsertInDriverOwnedList(pRequest);

        if (FxDriverGlobals->FxVerifierOn)
        {
            ASSERT((pRequest->GetVerifierFlags() & FXREQUEST_FLAG_DRIVER_OWNED) == 0);                        
            pRequest->SetVerifierFlags(FXREQUEST_FLAG_DRIVER_OWNED);
        }

        //
        // Also insert the request in to m_CanceledOnQueueList so
        // that we can notify the driver when we visit the DispatchEvents
        //
        InsertTailList(&m_CanceledOnQueueList, pRequest->GetListEntry(FxListEntryQueueOwned));

        //
        // Release the reference taken in the call to SetCompletionState.
        //
        pRequest->RELEASE(FXREQUEST_STATE_TAG);
    }
    else
    {
        Unlock(PreviousIrql);

        // Its gone from our list, so complete it cancelled
        pRequest->CompleteWithInformation(STATUS_CANCELLED, 0);

        // Dereference the request objects final reference
        pRequest->RELEASE(FXREQUEST_COMPLETE_TAG);

        Lock(&PreviousIrql);
    }

    // This may have caused the queue to be emptied
    DispatchInternalEvents(PreviousIrql);

    return;
}

_Must_inspect_result_
BOOLEAN
FxIoQueue::CanThreadDispatchEventsLocked(
    __in KIRQL PreviousIrql
    )
/*++

    Routine Description:

    Dispatch events and requests from the queue to the driver.

    The IoQueue object lock must be held on entry. This routine
    should not drop and reacquire the lock to ensure the request
    is not queued out of order.

    Returns:

        TRUE - if the thread meets all the sychronization and 
               execution contraints to dispatch the events.
        FALSE - if the dispatching of events to be defered to 
               another thread - either DPC or workitem.
--*/
{
    //
    // If the current irql is not at passive-level and the queue is configured
    // to receive events only at passive-level then we should queue a
    // workitem to defer the processing.
    //
    if ((PreviousIrql > PASSIVE_LEVEL) && m_PassiveLevel)
    {
        ASSERT(PreviousIrql <= DISPATCH_LEVEL); 
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIO,
                            "Current thread 0x%p is not at the passive-level"
                            " %!irql!, posting to worker thread for WDFQUEUE"
                            " 0x%p",
                            Mx::MxGetCurrentThread(), 
                            PreviousIrql, 
                            GetObjectHandle());
        //
        // We only need to post this once
        //
        if (m_WorkItemQueued == FALSE)
        {
            m_WorkItemQueued = TRUE;

            if (!m_SystemWorkItem->Enqueue(_DeferredDispatchThreadThunk, this))
            {
                ASSERT(FALSE);
                m_WorkItemQueued = FALSE;
            }
        }

        return FALSE;
    }

    //
    // If the current thread is holding the presentation lock, we
    // must defer to a DPC or work item.
    // This is the result of the device driver calling
    // WdfRequestForwardToIoQueue, or WdfIoQueueStart/Stop from
    // within I/O dispatch handler. This can also occur if a driver 
    // attempts to forward a request among a circular series of Queues
    // that are configured to have locking constraints.
    //
    if (m_CallbackLockPtr && m_CallbackLockPtr->IsOwner())
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIO,
                            "Presentation lock for WDFQUEUE 0x%p is "
                            "already held, deferring to dpc or workitem",
                            GetObjectHandle());

        if (m_PassiveLevel)
        {
            if (m_WorkItemQueued == FALSE)
            {
                m_WorkItemQueued = TRUE;

                if (!m_SystemWorkItem->Enqueue(_DeferredDispatchThreadThunk, this))
                {
                    ASSERT(FALSE);
                    m_WorkItemQueued = FALSE;
                }
            }
        }
        else
        {
            //
            // We only need to post this once
            //
            if (m_DpcQueued == FALSE)
            {
                m_DpcQueued = TRUE;

                InsertQueueDpc();
            }
        }

        return FALSE;
    }

    return TRUE;
}

VOID
FxIoQueue::_DeferredDispatchThreadThunk(
    __in PVOID Parameter
    )
/*++

Routine Description:
   Thunk used when requests must be posted to a workitem.

--*/
{
    FxIoQueue* pQueue = (FxIoQueue*)Parameter;

    PFX_DRIVER_GLOBALS FxDriverGlobals = pQueue->GetDriverGlobals();

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Dispatching requests from worker thread");

    pQueue->DeferredDispatchRequestsFromWorkerThread();

    return;
}

VOID
FxIoQueue::DeferredDispatchRequestsFromWorkerThread(
    )

/*++

    Routine Description:

    Dispatch requests from the queue to the driver
    from within the m_WorkItem.

    Arguments:

    Returns:

--*/

{
    KIRQL irql;

    Lock(&irql);

    ASSERT(m_WorkItemQueued != FALSE);

    m_RequeueDeferredDispatcher = FALSE;
    
    DispatchEvents(irql);

    //
    // DispatchEvents drops the lock before returning. So reacquire
    // the lock.
    //
    Lock(&irql);

    if (m_Deleted == FALSE && 
        m_RequeueDeferredDispatcher &&
        m_SystemWorkItem->Enqueue(_DeferredDispatchThreadThunk, this))
    {
        //
        // Workitem is queued.
        //
        DO_NOTHING();
    }
    else
    {
        m_RequeueDeferredDispatcher = FALSE;
        m_WorkItemQueued = FALSE;    
    }
    
    Unlock(irql);

    return;
}

VOID
FxIoQueue::InsertQueueDpc(
    VOID
    )
/*++

Routine Description:

    This is the kernel mode routine to insert a dpc.

Arguments:

Return Value:

--*/
{
    KeInsertQueueDpc(&m_Dpc, NULL, NULL);            
}

_Must_inspect_result_
NTSTATUS
FxRequest::InsertHeadIrpQueue(
    __in FxIrpQueue* IrpQueue,
    __out_opt ULONG*      pRequestCount
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    // No locking required since only one accessor till inserted on queue

    status = VerifyInsertIrpQueue(pFxDriverGlobals, IrpQueue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // If a request is on an IrpQueue, it must be referenced
    //
    ADDREF(FXREQUEST_QUEUE_TAG);

    ASSERT(m_Completed == FALSE);
    ASSERT(m_IrpQueue == NULL);

    m_IrpQueue = IrpQueue;

    status = IrpQueue->InsertHeadRequest(m_Irp.GetIrp(),
                                         &m_CsqContext,
                                         pRequestCount);

    //
    // If this insert failed, we must release the extra reference we took
    //
    if (!NT_SUCCESS(status))
    {
        MarkRemovedFromIrpQueue();
        RELEASE(FXREQUEST_QUEUE_TAG);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::GetReservedRequest(
    __in MdIrp Irp,
    __deref_out_opt FxRequest **ReservedRequest       
    )
/*++

Routine Description:
    Use the policy configured on the queue to decide whether to allocate a 
    reserved request.

--*/
{
    WDF_IO_FORWARD_PROGRESS_ACTION action;
    FxRequest *pRequest;
    KIRQL      oldIrql;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();
    
    *ReservedRequest = NULL;
    action = WdfIoForwardProgressActionInvalid;
   
    switch (m_FwdProgContext->m_Policy) {
        
    case WdfIoForwardProgressReservedPolicyPagingIO:
        if (IsPagingIo(Irp))
        {
            action = WdfIoForwardProgressActionUseReservedRequest;
        }
        else
        {
            action =  WdfIoForwardProgressActionFailRequest;
        }
        
        break;
    case WdfIoForwardProgressReservedPolicyAlwaysUseReservedRequest:
        action = WdfIoForwardProgressActionUseReservedRequest;        
        break;
        
    case WdfIoForwardProgressReservedPolicyUseExamine:
        
        //
        // Call the driver to figure out what action to take.
        //
        if (m_FwdProgContext->m_IoExamineIrp.Method != NULL)
        {            
            action = m_FwdProgContext->m_IoExamineIrp.Invoke(GetHandle(), Irp);

            //
            // Make sure the use has returned a valid action
            //
            if ((action < WdfIoForwardProgressActionFailRequest || 
                action > WdfIoForwardProgressActionUseReservedRequest))
            {
                status = STATUS_UNSUCCESSFUL;
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "EvtIoExamineIrp callback on WDFQUEUE %p returned an "
                    "invalid action %d, %!STATUS!",
                    GetHandle(), action, status);
                
                FxVerifierDbgBreakPoint(pFxDriverGlobals);
                    
                return status;
             }

        }            
        break;

    default:
            ASSERTMSG("Invalid forward progress setting ", FALSE);
            break;           
    }

    if (action == WdfIoForwardProgressActionFailRequest)
    {
        status = STATUS_UNSUCCESSFUL;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Forward action on WDFQUEUE %p says that framework should fail "
            "the Irp %p, %!STATUS!", 
            GetHandle(), Irp, status);
        
        return status;
    }

    pRequest = NULL;

    m_FwdProgContext->m_PendedReserveLock.Acquire(&oldIrql);

    if (!IsListEmpty(&m_FwdProgContext->m_ReservedRequestList))
    {
        PLIST_ENTRY pEntry;

        pEntry = RemoveHeadList(&m_FwdProgContext->m_ReservedRequestList);

        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryForwardProgress, 
                                                  pEntry); 
        ASSERT(pRequest != NULL);
        ASSERT(pRequest->GetRefCnt() == 1);

        InsertTailList(&m_FwdProgContext->m_ReservedRequestInUseList, 
                    pRequest->GetListEntry(FxListEntryForwardProgress));      

        pRequest->m_Irp.SetIrp(Irp);
        pRequest->AssignMemoryBuffers(m_Device->GetIoType());

        if (pFxDriverGlobals->FxVerifierOn)
        {
            pRequest->SetVerifierFlags(FXREQUEST_FLAG_RESERVED_REQUEST_ASSOCIATED_WITH_IRP);                             
        }

        //
        // if *ReservedRequest is non-null the caller needs to free the 
        // previous request it allocated.
        //
        *ReservedRequest = pRequest;

        status = STATUS_SUCCESS;
    }
    else
    {        
        status = QueueForwardProgressIrpLocked(Irp);        
        ASSERT(*ReservedRequest == NULL);
    }
    
    m_FwdProgContext->m_PendedReserveLock.Release(oldIrql);      

    return status;
}

__inline
BOOLEAN
FxIoQueue::IsPagingIo(
    __in MdIrp Irp
    )
/*++

  Routine Description:
  Paging IO is treated especially depending on what Forward Progress policy 
  was set on the Queue
  --*/
{   
    //
    // NOTE: IRP_INPUT_OPERATION has the same value as IRP_SYNCHRONOUS_PAGING_IO
    // and IRP_MOUNT_COMPLETION the same as  IRP_PAGING_IO  so how does one know if 
    // the IO is a paging IO ?
    //

    // One can assume that if IRP_PAGING_IO is set and the MJ code is not 
    // FILE_SYSTEM_CONTROL then it is a paging I/O.    
    //   
    if (Irp->Flags & IRP_PAGING_IO)
    {
        if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction 
            != IRP_MJ_FILE_SYSTEM_CONTROL)
        {       
            return  TRUE;
        }
    }
  
    return FALSE;    
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueForwardProgressIrpLocked(
    __in PIRP Irp
    )
{
    InsertTailList(&m_FwdProgContext->m_PendedIrpList, 
                   &Irp->Tail.Overlay.ListEntry);      

    Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = 
            m_FwdProgContext;

    IoSetCancelRoutine(Irp, _WdmCancelRoutineForReservedIrp);

    if (Irp->Cancel)
    {
        if (IoSetCancelRoutine(Irp, NULL) != NULL)
        {
            Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = NULL;

            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
            
            return STATUS_CANCELLED;                
        } 
        else
        {
            //
            // CancelRoutine will complete the IRP
            //
            DO_NOTHING();
        }
    }

    IoMarkIrpPending(Irp);

    return STATUS_PENDING;
}

VOID
FxIoQueue::_WdmCancelRoutineForReservedIrp(
    __inout struct _DEVICE_OBJECT * DeviceObject,
    __in __drv_useCancelIRQL PIRP   Irp
    )
/*++

Routine Description:

    Cancel routine for forward progress irp.

--*/
{
    KIRQL irql;
    PFXIO_FORWARD_PROGRESS_CONTEXT m_FwdPrgContext;
    
    UNREFERENCED_PARAMETER (DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    m_FwdPrgContext = (PFXIO_FORWARD_PROGRESS_CONTEXT)
        Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY];
    
    Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = NULL;

    m_FwdPrgContext->m_PendedReserveLock.Acquire(&irql);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    InitializeListHead(&Irp->Tail.Overlay.ListEntry);
    
    m_FwdPrgContext->m_PendedReserveLock.Release(irql);

    Irp->IoStatus.Status = STATUS_CANCELLED; 
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;
}

BOOLEAN
FxIoQueue::IsIoEventHandlerRegistered(
    __in WDF_REQUEST_TYPE RequestType
    )
/*++

Routine Description:
   Given a request type, this function checks to see if the appropriate
   event handler is registered to receive dispatched requests.

Return Value:

    TRUE - yes the queue is configured to dispatch requests of given RequestType
    FALSE - no, the queue cannot dispatch requests of given RequestType

--*/
{
    if (m_Type == WdfIoQueueDispatchManual)
    {
        //
        // Manual queues wouldn't have any IoEvent callbacks registered.
        //
        return TRUE;
    }

    //
    // Default handler is a catch all handler.
    //
    if (m_IoDefault.Method != NULL)
    {
        return TRUE;
    }

    //
    // Default handle is not registered. So check to see if request specific
    // handler is registered.
    //
    switch(RequestType) {
    case WdfRequestTypeRead:
        if(m_IoRead.Method == NULL)
        {
            return FALSE;
        }
        break;
    case WdfRequestTypeWrite:
        if(m_IoWrite.Method == NULL)
        {
            return FALSE;
        }
        break;
    case WdfRequestTypeDeviceControl:
        if(m_IoDeviceControl.Method == NULL)
        {
            return FALSE;
        }
        break;
    case WdfRequestTypeDeviceControlInternal:
        if(m_IoInternalDeviceControl.Method == NULL)
        {
            return FALSE;
        }
        break;
    case WdfRequestTypeCreate: // Fall through. Must have default handler.
    default:
        return FALSE;
    }

    return TRUE;
}

VOID
FxIoQueue::SetState(
    __in FX_IO_QUEUE_SET_STATE NewStatus
    )
{
   int AllowedBits;

   PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

   //
   // Only allow setting of valid bits
   //
   AllowedBits = (int)(FxIoQueueSetAcceptRequests     |
                       FxIoQueueClearAcceptRequests   |
                       FxIoQueueSetDispatchRequests   |
                       FxIoQueueClearDispatchRequests |
                       FxIoQueueSetShutdown           |
                       FxIoQueueClearShutdown
                       );

   if ((int)NewStatus & ~AllowedBits)
   {
       DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                           "Invalid WDFQUEUE 0x%p state",
                           GetObjectHandle());
       FxVerifierDbgBreakPoint(FxDriverGlobals);
       return;
   }

   //
   // Clear the high bit used to prevent accidental mixing of
   // WDF_IO_QUEUE_STATE and FX_IO_QUEUE_SET_STATE
   //
   NewStatus = (FX_IO_QUEUE_SET_STATE)((int)NewStatus & 0x7FFFFFFF);
   
   if (NewStatus & (int)FxIoQueueClearShutdown)
   {
       m_QueueState = (FX_IO_QUEUE_STATE)((int)m_QueueState & ~(int)FxIoQueueShutdown);
   }

   if (NewStatus & (int)FxIoQueueSetShutdown)
   {
       m_QueueState = (FX_IO_QUEUE_STATE)((int)m_QueueState | (int)FxIoQueueShutdown);
   }

   if (NewStatus & (int)FxIoQueueSetAcceptRequests)
   {
       if (IsState((WDF_IO_QUEUE_STATE)FxIoQueueShutdown) == FALSE)
       {
           m_QueueState = (FX_IO_QUEUE_STATE)((int)m_QueueState | (int)WdfIoQueueAcceptRequests);
       }
       else
       {
           DoTraceLevelMessage(FxDriverGlobals, 
                               TRACE_LEVEL_INFORMATION, TRACINGIO,
                               "WDFQUEUE 0x%p is shut down, preventing queue "
                               "from accepting requests",
                               GetObjectHandle());
       }
   }

   if (NewStatus & (int)FxIoQueueClearAcceptRequests)
   {
       m_QueueState = (FX_IO_QUEUE_STATE)((int)m_QueueState & ~(int)WdfIoQueueAcceptRequests);
   }

   if (NewStatus & (int)FxIoQueueSetDispatchRequests)
   {
       m_QueueState = (FX_IO_QUEUE_STATE)((int)m_QueueState | (int)WdfIoQueueDispatchRequests);
       //
       // If the queue is allowed to dispatch new requests, we must clear this flag.  
       // See also WdfIoQueueStopAndPurge for more info about the flag.
       //
       m_CancelDispatchedRequests = FALSE;
   }

   if (NewStatus & (int)FxIoQueueClearDispatchRequests)
   {
       m_QueueState = (FX_IO_QUEUE_STATE)((int)m_QueueState & ~(int)WdfIoQueueDispatchRequests);
   }

   return;
}

VOID
FxIoQueue::SetStateForShutdown(
    VOID
    )
/*++

Routine Description:
    The queue is shutting down. Disable WdfQueueStart/Stop from re-enabling
    the AcceptRequest bit.

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // No need to take a lock since caller is responsible for providing the 
    // required synchronization.
    //

    //
    // Do not allow request to be queued.
    //
    SetState((FX_IO_QUEUE_SET_STATE)(FxIoQueueSetShutdown | FxIoQueueClearAcceptRequests));
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::_Create(
    __in PFX_DRIVER_GLOBALS         DriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __in PWDF_IO_QUEUE_CONFIG       Config,
    __in_opt FxDriver*              Caller,
    __in FxPkgIo*                   PkgIo,
    __in BOOLEAN                    InitialPowerStateOn,
    __deref_out FxIoQueue**         Object
    )
{
    NTSTATUS status;
    FxIoQueue* pQueue;

    *Object = NULL;

    pQueue = new(DriverGlobals, Attributes) FxIoQueue(DriverGlobals, PkgIo);

    if (pQueue == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Memory allocation failed: %!STATUS!", status);
        return status;
    }

    //
    // Initialize it, creating the handle to pass the driver
    // and configuring its callbacks and queue type
    //
    status = pQueue->Initialize(Config, 
                                Attributes,
                                Caller,
                                InitialPowerStateOn);
    
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Could not configure queue: %!STATUS!", status);
        goto Done;
    }
Done:
    if (NT_SUCCESS(status))
    {
        *Object = pQueue;
    }
    else
    {
        //
        // Release our newly allocated Queue object
        //
        pQueue->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::Initialize(
    __in PWDF_IO_QUEUE_CONFIG       pConfig,
    __in_opt PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __in_opt FxDriver*              Caller,
    __in BOOLEAN                    InitialPowerStateOn
    )

/*++

Routine Description:

    Initialize the IoQueue after creating.

    This creates the handle required for passing to the driver.

Arguments:

Returns:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    Status = m_PowerIdle.Initialize(NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = m_FinishDisposing.Initialize(NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    Status = m_RequestWaitEventUm.Initialize(SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
#endif

    MarkDisposeOverride(ObjectDoNotLock);

    //
    // Set the execution level and callback synchronization based on
    // configuration
    //
    Status = ConfigureConstraints(QueueAttributes, Caller);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    
    //
    // Validate dispatch type.
    //
    if (pConfig->DispatchType <= WdfIoQueueDispatchInvalid || 
        pConfig->DispatchType >= WdfIoQueueDispatchMax)
    {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Invalid dispatch type "
                            "specified %d, Queue 0x%p %!STATUS!",
                            pConfig->DispatchType,
                            GetObjectHandle(),
                            STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    //
    // UMDF does not support internal IOCTLs
    //
    if (pConfig->EvtIoInternalDeviceControl != NULL)
    {
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(2, 19))
        {
            Status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "EvtIoInternalDeviceControl is not supported "
                                "for UMDF, Queue 0x%p %!STATUS!",
                                GetObjectHandle(), Status);
            return Status;
        }
        else
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIO,
                                "EvtIoInternalDeviceControl is not supported "
                                "for UMDF, Queue 0x%p",
                                GetObjectHandle());
        }
    }
#endif

    //
    // If not a manual queue, must set at least IoStart, or one of
    // read|write|devicecontrol
    //
    if ((pConfig->DispatchType != WdfIoQueueDispatchManual) &&
        (pConfig->EvtIoDefault == NULL))
    {
        if ((pConfig->EvtIoDefault == NULL) &&
            (pConfig->EvtIoRead == NULL) &&
            (pConfig->EvtIoWrite == NULL) &&
            (pConfig->EvtIoDeviceControl == NULL) &&
            (pConfig->EvtIoInternalDeviceControl == NULL))
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "At least one of EvtIoDefault|EvtIoRead|EvtIoWrite|"
                                "EvtIoDeviceControl|EvtIoInternalDeviceControl "
                                "must be set %!STATUS!", STATUS_WDF_NO_CALLBACK);
            return STATUS_WDF_NO_CALLBACK;
        }
    }

    //
    // A manual queue should not set any callback function
    // pointers since they will not get invoked.
    //
    if (pConfig->DispatchType == WdfIoQueueDispatchManual)
    {
        if ((pConfig->EvtIoDefault != NULL) ||
            (pConfig->EvtIoRead != NULL)  ||
            (pConfig->EvtIoWrite != NULL) ||
            (pConfig->EvtIoDeviceControl != NULL) ||
            (pConfig->EvtIoInternalDeviceControl != NULL))
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "Cannot set io callback events "
                                "on a manual WDFQUEUE 0x%p %!STATUS!",
                                GetObjectHandle(),
                                STATUS_INVALID_PARAMETER);
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // For version less than v1.9  m_MaxParallelQueuePresentedRequests is set to 
    // -1 by the FxIoQueue Constructor.
    // By checking > below we mean v1.9 and above (public API already did the official
    // validation).
    //
    //if (pConfig->Size > sizeof(WDF_IO_QUEUE_CONFIG_V1_7))
    if (pConfig->Size >= sizeof(WDF_IO_QUEUE_CONFIG_V1_9))
    {
        if (pConfig->Settings.Parallel.NumberOfPresentedRequests != 0 && 
             (pConfig->DispatchType == WdfIoQueueDispatchSequential || 
               pConfig->DispatchType == WdfIoQueueDispatchManual))
        {
            Status =  STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "Cannot have  NumberOfPresentedRequests other "
                                "than 0 on a Sequential or manual WDFQUEUE 0x%p."
                                "Make Sure you set NumberOfPresentedRequests"
                                " to 0, %!STATUS!",
                                GetObjectHandle(),
                                Status
                                );
            return Status;

        }
        else
        { 
            m_MaxParallelQueuePresentedRequests = 
                pConfig->Settings.Parallel.NumberOfPresentedRequests;
        }
    }

    //
    // Initialize our workitem if we have to support passive callbacks
    //
    if (m_PassiveLevel)
    {
        Status = FxSystemWorkItem::_Create(FxDriverGlobals,
                                          m_Device->GetDeviceObject(),
                                          &m_SystemWorkItem
                                          );

        if (!NT_SUCCESS(Status))
        {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "Could not allocate workitem: %!STATUS!", Status);
            return Status;
        }
    }

    m_Type = pConfig->DispatchType;

    switch(pConfig->PowerManaged) {

        case WdfUseDefault:
            if(m_Device->IsFilter())
            {
                m_PowerManaged = FALSE;
            }
            else
            {
                m_PowerManaged = TRUE;
            }
            break;

        case WdfTrue:
            m_PowerManaged = TRUE;
            break;

        case WdfFalse:
            m_PowerManaged = FALSE;
            break;
        default:
            ASSERTMSG("Invalid value in WDF_IO_QUEUE_CONFIG PowerManaged field\n", FALSE);
            break;
    }

    //
    // Queues for NonPnp devices can't be power managed.
    //
    if (m_Device->IsLegacy())
    {
        m_PowerManaged = FALSE;
    }

    //
    // If power managed queue, ensure its initial power state
    // is same as the device.
    //
    if (m_PowerManaged)
    {
        if (InitialPowerStateOn)
        {
            m_PowerState = FxIoQueuePowerOn;
        }
        else
        {
            m_PowerState = FxIoQueuePowerOff;
        }
    }
    else
    {
        m_PowerState = FxIoQueuePowerOn;
    }

    m_AllowZeroLengthRequests = pConfig->AllowZeroLengthRequests;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "EvtIoDefault 0x%p, EvtIoRead 0x%p, EvtIoWrite 0x%p, "
                        "EvtIoDeviceControl 0x%p for WDFQUEUE 0x%p",
                        pConfig->EvtIoDefault,
                        pConfig->EvtIoRead,
                        pConfig->EvtIoWrite,
                        pConfig->EvtIoDeviceControl, GetObjectHandle());

    m_IoDefault.Method  = pConfig->EvtIoDefault;
    m_IoStop.Method   = pConfig->EvtIoStop;
    m_IoResume.Method = pConfig->EvtIoResume;
    m_IoRead.Method   = pConfig->EvtIoRead;
    m_IoWrite.Method  = pConfig->EvtIoWrite;
    m_IoDeviceControl.Method         = pConfig->EvtIoDeviceControl;
    m_IoInternalDeviceControl.Method = pConfig->EvtIoInternalDeviceControl;
    m_IoCanceledOnQueue.Method = pConfig->EvtIoCanceledOnQueue;

    
    // A newly created queue can accept and dispatch requests once initialized
    SetState((FX_IO_QUEUE_SET_STATE)(FxIoQueueSetAcceptRequests|FxIoQueueSetDispatchRequests));

    m_Configured = TRUE;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::ConfigureConstraints(
    __in_opt PWDF_OBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt FxDriver*              Caller
    )

/*++

Routine Description:

    Configures the locking and threading model for the
    Queue according to parameters specified by the device
    driver when it initialized.

Arguments:

Returns:

    NTSTATUS

--*/

{
    WDF_EXECUTION_LEVEL ParentLevel;
    WDF_SYNCHRONIZATION_SCOPE ParentScope;
    BOOLEAN AutomaticLockingRequired;

    AutomaticLockingRequired = FALSE;
    ASSERT(m_Device != NULL);

    //
    // Initialize both spin and mutex locks
    //
    m_CallbackSpinLock.Initialize(this);
    m_CallbackMutexLock.Initialize(this);

    //
    // If WDF_OBJECT_ATTRIBUTES is specified, these override any
    // default settings.
    //
    if (ObjectAttributes != NULL)
    {
        m_ExecutionLevel = ObjectAttributes->ExecutionLevel;
        m_SynchronizationScope = ObjectAttributes->SynchronizationScope;
    }

    //
    // If no WDFQUEUE specific attributes are specified, we
    // get them from WDFDEVICE, which allows WDFDEVICE to
    // provide a default for all WDFQUEUE's created.
    //
    m_Device->GetConstraints(&ParentLevel, &ParentScope);
    ASSERT(ParentLevel != WdfExecutionLevelInheritFromParent);
    ASSERT(ParentScope != WdfSynchronizationScopeInheritFromParent);

    if (m_ExecutionLevel == WdfExecutionLevelInheritFromParent)
    {
        m_ExecutionLevel = ParentLevel;
    }

    if (m_SynchronizationScope == WdfSynchronizationScopeInheritFromParent)
    {
        m_SynchronizationScope = ParentScope;
    }

    //
    // For backward compatibility purposes always have a lock associated with the 
    // object even for WdfSynchronizationScopeNone.  This is so that we return a non-null
    // presentation lock for the WDFQUEUE object.
    //
    if (m_ExecutionLevel == WdfExecutionLevelPassive)
    {
        //
        // Mark FxObject as passive level to ensure that Dispose and Destroy
        // callbacks are passive to the driver
        //
        MarkPassiveCallbacks(ObjectDoNotLock);
        m_PassiveLevel = TRUE;

        //
        // Passive Callbacks constraint has been set, we use a mutex for the
        // callback lock.
        //
        m_CallbackLockPtr = &m_CallbackMutexLock;
        m_CallbackLockObjectPtr = this;
    }
    else
    {
        //
        // If no passive level constraint is specified, then spinlocks
        // are used for callbacks since they are lightweight and work with
        // DPC's and Timer's
        //
        m_CallbackLockPtr = &m_CallbackSpinLock;
        m_CallbackLockObjectPtr = this;
    }

    //
    // Configure synchronization scope
    //
    if (m_SynchronizationScope == WdfSynchronizationScopeDevice)
    {
        NTSTATUS status;
        
        //
        // WDF extensions are not allowed to use this type of synchronization.
        //
        if (Caller != NULL &&  Caller != GetDriverGlobals()->Driver)
        {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFQUEUE 0x%p Synchronization scope is set to "
                            "device; WDF extension drivers are not allowed "
                            "to use this type of synchronization, %!STATUS!",
                            GetObjectHandle(), status);
            return status;
        }
        
        //
        // If we inherit the Sync. scope from parent or device
        // and if the parent/device has Exec. Level different  from Queue
        // then disallow that case.
        // FUTURE PROOF NOTE: Adding a new Execution Level will need reevaluation 
        // of the check below.
        //       
        if (ParentLevel != m_ExecutionLevel)
        {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFQUEUE 0x%p Synchronization scope is set to device"
                            " but the Device ExecutionLevel: 0x%x"
                            " doesn't match Queue ExecutionLevel: 0x%x, %!STATUS!",
                            GetObjectHandle(), ParentLevel, 
                            m_ExecutionLevel, status);
            return status;
        }
        //
        // Per device automatic callback synchronization, so we update our
        // callback lock ptr to point to the devices lock
        //
        AutomaticLockingRequired = TRUE;

        //
        // Get the callback lock and object from the device
        //
        m_CallbackLockPtr = m_Device->GetCallbackLockPtr(&m_CallbackLockObjectPtr);
    }
    else if (m_SynchronizationScope == WdfSynchronizationScopeQueue)
    {
        //
        // Per object automatic serialization
        //
        AutomaticLockingRequired = TRUE;
        
        // m_CallbackLockPtr has been set above in execution level constraint
    }
    
    if (AutomaticLockingRequired)
    {
        //
        // If automatic locking has been configured, set the lock
        // on the FxCallback object delegates
        //
        m_IoDefault.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoStop.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoResume.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoRead.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoWrite.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoDeviceControl.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoInternalDeviceControl.SetCallbackLockPtr(m_CallbackLockPtr);
        m_PurgeComplete.SetCallbackLockPtr(m_CallbackLockPtr);
        m_ReadyNotify.SetCallbackLockPtr(m_CallbackLockPtr);
        m_IoCanceledOnQueue.SetCallbackLockPtr(m_CallbackLockPtr);
        
        m_IoCancelCallbackLockPtr = m_CallbackLockPtr;
    }
    else
    {
        //
        // No automatic locking specified
        //
        m_IoDefault.SetCallbackLockPtr(NULL);
        m_IoStop.SetCallbackLockPtr(NULL);
        m_IoResume.SetCallbackLockPtr(NULL);
        m_IoRead.SetCallbackLockPtr(NULL);
        m_IoWrite.SetCallbackLockPtr(NULL);
        m_IoDeviceControl.SetCallbackLockPtr(NULL);
        m_IoInternalDeviceControl.SetCallbackLockPtr(NULL);
        m_PurgeComplete.SetCallbackLockPtr(NULL);
        m_ReadyNotify.SetCallbackLockPtr(NULL);
        m_IoCanceledOnQueue.SetCallbackLockPtr(NULL);
        
        m_IoCancelCallbackLockPtr = NULL;

    }

    return STATUS_SUCCESS;
}

VOID
FxIoQueue::_DeferredDispatchDpcThunk(
    __in PKDPC Dpc,
    __in PVOID DeferredContext,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
    )
/*++

Routine Description:
   Thunk used when requests must be posted to a DPC.

--*/
{
    FxIoQueue* pQueue = (FxIoQueue*)DeferredContext;

    PFX_DRIVER_GLOBALS FxDriverGlobals = pQueue->GetDriverGlobals();

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Dispatching requests from DPC");

    pQueue->DeferredDispatchRequestsFromDpc();

    return;
}

VOID
FxIoQueue::DeferredDispatchRequestsFromDpc(
    )

/*++

    Routine Description:

    Dispatch requests from the queue to the driver
    from within the m_Dpc

    Arguments:

    Returns:

--*/

{
    KIRQL irql;

    Lock(&irql);

    ASSERT(m_DpcQueued != FALSE);

    m_RequeueDeferredDispatcher = FALSE;

    DispatchEvents(irql);

    //
    // DispatchEvents drops the lock before returning. So reacquire the lock.
    //
    Lock(&irql);

    if (m_Deleted == FALSE && m_RequeueDeferredDispatcher)
    {
        InsertQueueDpc();            
    }
    else
    {
        m_RequeueDeferredDispatcher = FALSE;
        m_DpcQueued = FALSE;
    }
    
    Unlock(irql);
    
    return;
}

VOID
FxIoQueue::CancelForDriver(
    __in FxRequest* pRequest
    )
/*++

Routine Description:

    This is called when a driver-owned cancelable request is canceled.
    This routine will add the request to m_Canceled list so that the
    dispatcher and call the driver cancel-routine to notify the driver.

    Queue lock is not held.

Arguments:

    pRequest - is a driver owned cancelable request.

Return Value:

--*/
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    KIRQL irql;

    // This is not an error, but want to make sure cancel testing works
    VerifyCancelForDriver(FxDriverGlobals, pRequest);

    //
    // We are called with no locks held, but
    // can be in arbitrary thread context from
    // a cancel occuring from another driver within
    // a driver stack.
    //

    //
    // The Csq has removed this request from the driver pending
    // queue, and it no longer has a cancel function if the
    // driver does not accept the cancel right now.
    //
    // When callside eventually goes to remove it from the queue
    // by CsqContext, the Csq's will return NULL.
    //
    // Irp and FxRequest is still valid until the driver calls
    // WdfRequestComplete either as a result of this cancel
    // callback, or at its leasure if it chooses to ignore it.
    //
    // The insert of FxRequest onto the FxIrpQueue took out a
    // reference, and when an IRP gets cancelled, we are responsible
    // for this final dereference after calling into the driver.
    //

    //
    // Cancellations are dispatched as events to the device driver
    // using the standard DispatchEvents processing loop. In order
    // to support this, we must defer it by linking this request
    // into a list of cancelled requests.
    //
    // The requests will be removed from this list and cancel notified
    // to the device driver by the processing loop.
    //

    pRequest->MarkRemovedFromIrpQueue();

    //
    // Queue it on the cancelled list
    //
    Lock(&irql);

    InsertTailList(&m_Cancelled, pRequest->GetListEntry(FxListEntryQueueOwned));

    //
    // Visit the event dispatcher
    //
    DispatchInternalEvents(irql);

    return;
}

VOID
FxIoQueue::_IrpCancelForDriver(
    __in FxIrpQueue* IrpQueue,
    __in MdIrp        Irp,
    __in PMdIoCsqIrpContext CsqContext,
    __in KIRQL Irql
    )
/*++

Routine Description:
    This is our Cancel Safe Queue Callback from FxIrpQueue notifying us of an
    I/O cancellation on a driver owned request (driver queue)

    Note this callback is called with the queue lock held.

Arguments:
    IrpQueue - Queue the request was on

    Irp - the irp being cancelled

    CsqContext - the context associated with the irp


Return Value:
    None

  --*/
{
    FxIoQueue* ioQueue;
    FxRequest* pRequest;

    ioQueue = CONTAINING_RECORD(IrpQueue, FxIoQueue, m_DriverCancelable);
    pRequest = FxRequest::RetrieveFromCsqContext(CsqContext);

    pRequest->m_Canceled = TRUE;

    //
    // Must reference the queue since this could be the final
    // request on a deleting queue.
    //
    ioQueue->ADDREF(Irp);

    //
    // We can drop the lock because this request is a driver owned request and
    // it is tracked by m_DriverIoCount. As a result delete will be blocked
    // until the request is completed.
    //
    ioQueue->Unlock(Irql);

    ioQueue->CancelForDriver(pRequest);

    ioQueue->RELEASE(Irp);
}

#ifdef __GNUC__
VOID
FxIoQueue::Vf_VerifyCancelForDriver (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request
    )
#else
VOID
FX_VF_METHOD(FxIoQueue, VerifyCancelForDriver) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request
    )
#endif
{
    PLIST_ENTRY pEntry;

    PAGED_CODE_LOCKED();

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIO,
                        "WDFREQUEST 0x%p "
                        "was cancelled in driver for WDFQUEUE 0x%p",
                        Request->GetHandle(), GetObjectHandle());
    
    // Verifier code assures this is available to the cancel processing
    pEntry = Request->GetListEntry(FxListEntryQueueOwned);
    if (!IsListEmpty(pEntry))
    {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFREQUEST 0x%p is "
                            "already on list, FxRequest::m_ListEntry is busy!, "
                            "WDFQUEUE 0x%p",
                            Request->GetHandle(), GetObjectHandle());
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
}

VOID
FxIoQueue::_IrpCancelForQueue(
    __in FxIrpQueue* IrpQueue,
    __in MdIrp        Irp,
    __in PMdIoCsqIrpContext CsqContext,
    __in KIRQL Irql
    )
/*++

Routine Description:
    This is our Cancel Safe Queue Callback from FxIrpQueue notifying us of an
    I/O cancellation on the main (pre driver) Queue.

    Note this callback is called with the queue lock held.

Arguments:
    IrpQueue - Queue the request was on

    Irp - the irp being cancelled

    CsqContext - the context associated with the irp

Return Value:
    None

  --*/
{
    FxIoQueue* ioQueue;
    FxRequest* pRequest;

    ioQueue = CONTAINING_RECORD(IrpQueue, FxIoQueue, m_Queue);
    pRequest = FxRequest::RetrieveFromCsqContext(CsqContext);

    //
    // Must reference the queue since this could be the final
    // request on a deleting queue
    //
    ioQueue->ADDREF(Irp);

    //
    // We cannot drop the lock here because we may have to insert the
    // request in the driver owned list if the driver has registered
    // for canceled-on-queue callback. If we drop the lock and if the request
    // happens to be last request, the delete will run thru and put
    // the state of the queue to deleted state and prevent further dispatching
    // of requests.
    //
    ioQueue->CancelForQueue(pRequest, Irql);

    ioQueue->RELEASE(Irp);
}

VOID
FxIoQueue::FlushByFileObject(
    __in MdFileObject FileObject
    )
/*++

Routine Description:

    Scan the queue and cancel all the requests that have
    the same fileobject as the input argument.

    This function is called when the IoPkg receives a
    IRP_MJ_CLEANUP requests.

    Additional reference is already taken on the object by the caller
    to prevent the queue from being deleted.

Return Value:

--*/
{
    FxRequest*  pRequest = NULL;
    NTSTATUS status;
    KIRQL irql;

    if (IsForwardProgressQueue())
    {
        PurgeForwardProgressIrps(FileObject);
    }
    
    Lock(&irql);

#ifdef _MSC_VER
    #pragma warning(disable:4127)
#endif
    while (TRUE)
    {
#ifdef _MSC_VER        
    #pragma warning(default:4127)
#endif
        //
        // Get the next FxRequest from the cancel safe queue
        //
        status = FxRequest::GetNextRequest(&m_Queue, FileObject, NULL, &pRequest);
        if(status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        if (!NT_SUCCESS(status))
        {
            ASSERTMSG("GetNextRequest failed\n", FALSE);
            break;
        }

        //
        // We must add a reference since the CancelForQueue path
        // assumes we were on the FxIrpQueue with the extra reference
        //
        pRequest->ADDREF(FXREQUEST_QUEUE_TAG);

        //
        // Mark the request as cancelled, place it on the cancel list,
        // and schedule the cancel event to the driver
        //
        CancelForQueue(pRequest, irql);

        //
        // Reacquire the lock because CancelForQueue visits the dispatch-loop
        // and releases the lock.
        //
        Lock(&irql);
    }

    DispatchEvents(irql);

    return;
}

VOID 
FxIoQueue::PurgeForwardProgressIrps(
    __in_opt MdFileObject FileObject
    )
/*++

Routine Description:

    This function is called when the queue is purged.
    
--*/
{
    LIST_ENTRY  cleanupList;
    
    InitializeListHead(&cleanupList);
    GetForwardProgressIrps(&cleanupList, FileObject);
    CancelIrps(&cleanupList);
}

VOID 
FxIoQueue::CancelIrps(
    __in PLIST_ENTRY    IrpListHead
    )
/*++

Routine Description:

    This function is called to purge (cancel) the specified list of IRPs.
    The IRP's Tail.Overlay.ListEntry field must be used to link these structs together.

--*/
{
    MdIrp        irp;
    FxIrp        fxIrp;
    PLIST_ENTRY entry;

    while (!IsListEmpty(IrpListHead))
    {

        entry = RemoveHeadList(IrpListHead);

        irp = fxIrp.GetIrpFromListEntry(entry);

        fxIrp.SetIrp(irp);

        fxIrp.SetInformation(0);
        fxIrp.SetStatus(STATUS_CANCELLED);
        fxIrp.CompleteRequest(IO_NO_INCREMENT);
    }
}

VOID 
FxIoQueue::GetForwardProgressIrps(
    __in     PLIST_ENTRY    IrpListHead,
    __in_opt MdFileObject   FileObject
    )
/*++

Routine Description:

    This function is called to retrieve the list of reserved queued IRPs.
    The IRP's Tail.Overlay.ListEntry field is used to link these structs together.
    
--*/
{
    PIRP    irp;
    KIRQL   irql;
    
    m_FwdProgContext->m_PendedReserveLock.Acquire(&irql);
    
    do
    {        
        irp = GetForwardProgressIrpLocked(FileObject);
        if (irp != NULL)
        {
            //
            // Add it to the cleanupList. We will complete the IRP after
            // releasing the lock.
            //
            InsertTailList(IrpListHead, &irp->Tail.Overlay.ListEntry);
        } 
    } while (irp != NULL);

    m_FwdProgContext->m_PendedReserveLock.Release(irql);
}

_Must_inspect_result_
PIRP
FxIoQueue::GetForwardProgressIrpLocked(
    __in_opt PFILE_OBJECT FileObject
    )
/*++

    Routine Description:
        Remove an IRP from the pending irp list if it matches with the input
        fileobject. If the fileobject value is NULL, return the first one from
        the pending list.
        
--*/
{
    PIRP pIrp;
    PLIST_ENTRY thisEntry, nextEntry, listHead;
    PIO_STACK_LOCATION  pIrpStack;

    pIrp = NULL;
    listHead = &m_FwdProgContext->m_PendedIrpList;
    
    for (thisEntry = listHead->Flink;
        thisEntry != listHead;
        thisEntry = nextEntry)
    {
        nextEntry = thisEntry->Flink;
        
        pIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);
        pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

        if (FileObject == NULL || FileObject == pIrpStack->FileObject)
        {
            RemoveEntryList(thisEntry);
            InitializeListHead(thisEntry);
            
            if (NULL != IoSetCancelRoutine (pIrp, NULL))
            {
                pIrp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = NULL;
                break;
            } 
            else
            {
                //
                // Irp is canceled and the cancel routines is waiting to run.
                // Continue to get the next the irp in the list.
                //
                DO_NOTHING();
            }
        }

        pIrp = NULL;
    }
    
    return pIrp;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::ForwardRequest(
    __in FxIoQueue* pDestQueue,
    __in FxRequest* pRequest
    )
/*++

Routine Description:

    ForwardRequest is called from the drivers EvtIoDefault routine
    and the following conditions apply:

    Request is not on a CSQ and not cancellable

    Request is FXREQUEST_FLAG_DRIVER_OWNED

    m_DriverIoCount has been incremented to reflect the request

    Request has an I/O completion callback function pointing to
    FxIoQueueRequestComplete with the context for this Queue

    The Request has one reference count from the I/O completion callback.

    If a driver calls this API, it will not complete the request
    as a result of this queues EvtIoDefault, and does not own
    the request until it has been re-presented by the Destination
    Queue.

Arguments:

Returns:

    NTSTATUS

--*/
{
    NTSTATUS status;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    status = VerifyForwardRequest(FxDriverGlobals, pDestQueue, pRequest);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = ForwardRequestWorker(pRequest, pDestQueue);
    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::ForwardRequestWorker(
    __in FxRequest* Request,
    __in FxIoQueue* DestQueue
    )
{
    NTSTATUS status;
    FxRequestCompletionState oldState;
    PLIST_ENTRY ple;
    SHORT     OldFlags;
    KIRQL irql;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    OldFlags = 0;

    //
    // The request has only one reference, held by the completion
    // callback function. We need to take another one before cancelling
    // this function, otherwise we will lose the request object
    //
    Request->ADDREF(FXREQUEST_STATE_TAG);

    //
    // Cancel its current completion event for this queue
    //
    oldState = Request->SetCompletionState(FxRequestCompletionStateNone);
    ASSERT(oldState == FxRequestCompletionStateQueue);

    OldFlags = VerifyForwardRequestUpdateFlags(FxDriverGlobals, Request);
    
    //
    // Remove it from this queues driver owned list.
    //
    // This must be done before forward since new queue will
    // use the list entry in the FxRequest
    //
    // We can't use RemoveFromDriverOwnedList since we want the
    // m_DriverIoCount to be left alone in case the forward fails.
    // If we don't, another thread can run when we drop the lock, notice
    // that there are no more requests, and raise the purged and empty
    // events. But if the forward fails, the request will wind up back
    // on the queue! So m_DriverIoCount is used as a gate to prevent
    // these events from firing until we are really sure this queue
    // is done with the request.
    //

    Lock(&irql);
    ple = Request->GetListEntry(FxListEntryDriverOwned);
    RemoveEntryList(ple);
    InitializeListHead(ple);
    Unlock(irql);

    //
    // Attempt to pass the request onto the target queue
    //
    status = DestQueue->QueueRequestFromForward(Request);
    if (!NT_SUCCESS(status))
    {
        //
        // Target queue did not accept the request, so we
        // restore the original completion callback function
        // and flags
        //
        oldState = Request->SetCompletionState(oldState);
        ASSERT(oldState == FxRequestCompletionStateNone);
        UNREFERENCED_PARAMETER(oldState);

        if (FxDriverGlobals->FxVerifierOn)
        {
            Request->SetVerifierFlags(OldFlags);
        }

        // Release the extra reference we took
        Request->RELEASE(FXREQUEST_STATE_TAG);

        Lock(&irql);
        // Place it back on the driver owned list
        InsertTailList(&m_DriverOwned, ple);
        Unlock(irql);
    }
    else
    {
        Lock(&irql);

        // Request is no longer part of the I/O count for this queue
        m_DriverIoCount--;

        ASSERT(m_DriverIoCount >= 0);

        //
        // Don't run the event dispatcher if we are called from a
        // dispath routine in order to prevent stack recursion.
        // Since some other thread (possibly this thread higher on
        // the stack) is running the dispatcher, no events will get lost.
        //
        //
        // This returns with the IoQueue lock released
        //
        DispatchInternalEvents(irql);

        //
        // We don't dereference the request object since the new IoQueue
        // will release it when it is done.
        //
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueRequestFromForward(
    __in FxRequest* pRequest
    )

/*++

    Routine Description:

    Enqueue a request to the end of the queue.

    This is an internal version that does not fail
    the request if it can not be enqueued.

    Arguments:

        pRequest - Pointer to Request object

    Returns:

        STATUS_SUCCESS on success
--*/

{
    NTSTATUS status;
    KIRQL    irql;
    BOOLEAN  fromIo;

    // Get IoQueue Object Lock
    Lock(&irql);

    //
    // If queue is not taking new requests, fail now
    //
    if (!IsState(WdfIoQueueAcceptRequests))
    {
        status = STATUS_WDF_BUSY;

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIO,
                            "WDFQUEUE 0x%p is not accepting requests "
                            "state is %!WDF_IO_QUEUE_STATE!, %s"
                            "WDFREQUEST 0x%p %!STATUS!",
                            GetObjectHandle(), m_QueueState,
                            IsState((WDF_IO_QUEUE_STATE)FxIoQueueShutdown) ? 
                                "power stopping (Drain) in progress," : "",                            
                            pRequest->GetHandle(), status);

        Unlock(irql);

        return status;
    }
#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Queuing WDFREQUEST 0x%p on WDFQUEUE 0x%p",
                        pRequest->GetHandle(), GetObjectHandle());
#endif
    //
    // The Request has one reference count, and no completion
    // callback function. It has been completely removed from
    // its previous queue.
    //

    //
    // Cache this info b/c the request can be delete and freed by the time we use it.
    //
    fromIo = pRequest->IsAllocatedFromIo();
    
    //
    // Insert it in the Cancel Safe Queue
    //
    // This will mark the IRP pending
    //
    status = pRequest->InsertTailIrpQueue(&m_Queue, NULL);

    if (!NT_SUCCESS(status)) {

        pRequest->SetCurrentQueue(this);

        ASSERT(status == STATUS_CANCELLED);

        //
        // We must add a reference since the CancelForQueue path
        // assumes we were on the FxIrpQueue with the extra reference
        //
        pRequest->ADDREF(FXREQUEST_QUEUE_TAG);

        //
        // Mark the request as cancelled, place it on the cancel list,
        // and schedule the cancel event to the driver
        //
        CancelForQueue(pRequest, irql);

        Lock(&irql);
    }
    else
    {
        pRequest->SetCurrentQueue(this);

        // Check if went from no requests to have requests
        CheckTransitionFromEmpty();
    }

    //
    // If the request is driver-created, we may be called to dispatch 
    // a request from within the context of the completion routine.
    // So to avoid recursion, we will try to dispatch in the return path. 
    // If the request is not driver-created then we will dispatch it directly because 
    // this path is meant for dispatching new incoming I/O. There is no concern for 
    // running into recursion in that scenario.
    //
    if (fromIo == FALSE && m_Dispatching != 0)
    { 
        Unlock(irql);
    }
    else
    {
        //
        // Attempt to dispatch any new requests.
        //
        // This releases, and re-acquires the IoQueue lock
        //
        DispatchEvents(irql);
    }

    return STATUS_SUCCESS;
}

#ifdef __GNUC__
SHORT
FxIoQueue::Vf_VerifyForwardRequestUpdateFlags (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request
    )
#else
SHORT
FX_VF_METHOD(FxIoQueue, VerifyForwardRequestUpdateFlags) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request
    )
#endif
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    SHORT OldFlags = 0;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    Request->Lock(&irql);
    
    // Save old flags to put them back if forward fails
    OldFlags = Request->GetVerifierFlagsLocked();
    
    //
    // Set that the request was forwarded. This effects
    // cancel behavior.
    //
    Request->SetVerifierFlagsLocked(FXREQUEST_FLAG_FORWARDED);
    
    ASSERT((Request->GetVerifierFlagsLocked() & FXREQUEST_FLAG_DRIVER_OWNED) != 0);
    
    // Set that the request is no longer driver owned
    Request->ClearVerifierFlagsLocked(
        FXREQUEST_FLAG_DRIVER_OWNED | FXREQUEST_FLAG_DRIVER_DISPATCH);
    
    Request->Unlock(irql);

    return OldFlags;
}

#ifdef __GNUC__
_Must_inspect_result_
NTSTATUS
FxIoQueue::Vf_VerifyForwardRequest (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxIoQueue* pDestQueue,
    _In_ FxRequest* pRequest
    )
#else
_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxIoQueue, VerifyForwardRequest) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxIoQueue* pDestQueue,
    _In_ FxRequest* pRequest
    )
#endif
{
    NTSTATUS status;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    pRequest->Lock(&irql);

    status = pRequest->VerifyRequestIsDriverOwned(FxDriverGlobals);
    if (NT_SUCCESS(status))
    {
        status = pRequest->VerifyRequestIsNotCancelable(FxDriverGlobals);
    }

    pRequest->Unlock(irql);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (pDestQueue == this)
    {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Cannot forward a request to the same WDFQUEUE 0x%p"
                            " %!STATUS!",
                            GetObjectHandle(),
                            STATUS_INVALID_DEVICE_REQUEST);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((m_Device != pDestQueue->m_Device))
    { 
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Cannot forward a request to a different WDFDEVICE 0x%p",
                            pDestQueue->m_Device->GetHandle());
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::GetRequest(
    __in_opt  MdFileObject FileObject,
    __in_opt  FxRequest*   TagRequest,
    __deref_out FxRequest**  pOutRequest
    )
/*++

Routine Description:

    This method is called by

        WdfIoQueueRetrieveNextRequest
        WdfIoQueueRetrieveRequestByFileObject
        WdfIoQueueRetrieveFoundRequest

     to retrieve a request from the queue.

Arguments:

Returns:

    NTSTATUS

--*/
{
    NTSTATUS   status;
    FxRequest*  pRequest = NULL;
    FxRequestCompletionState oldState;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    KIRQL irql;
    
    status = VerifyGetRequestUpdateFlags(pFxDriverGlobals, TagRequest);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Don't allow on parallel queues
    //
    if ((m_Type != WdfIoQueueDispatchManual) &&
        (m_Type != WdfIoQueueDispatchSequential))
    {
        status = STATUS_INVALID_DEVICE_STATE;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Cannot be called on a parallel WDFQUEUE 0x%p, %!STATUS!",
                GetObjectHandle(), status);
        return status;
    }

    Lock(&irql);

    //
    // Only if the queue state allows requests to be retrieved.
    // It's okay to retrieve requests while the queue is in a transitioning state.
    //
    if (m_PowerState == FxIoQueuePowerOff)
    {
        status = STATUS_WDF_PAUSED;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFQUEUE 0x%p is powered off, %!STATUS!",
                            GetObjectHandle(), status);
        Unlock(irql);
        return status;
    }

    //
    // See if the queue is (still) processing requests
    //
    if (!IsState(WdfIoQueueDispatchRequests))
    {
        status = STATUS_WDF_PAUSED;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFQUEUE 0x%p is stopped, %!STATUS!",
                            GetObjectHandle(), status);
        Unlock(irql);
        return status;
    }

#ifdef _MSC_VER
    #pragma warning(disable:4127)
#endif
    while (TRUE)
    {
#ifdef _MSC_VER
    #pragma warning(default:4127)
#endif
        //
        // Get the next FxRequest from the cancel safe queue
        //
        status = FxRequest::GetNextRequest(&m_Queue, FileObject, TagRequest, &pRequest);
        if (!NT_SUCCESS(status))
        {
            //
            // This code address the following race condition:
            // 1)  Queue has only one request (count 1).
            // 2)  Request in queue is cancelled.
            // 3)  Request's cancellation logic starts to run on thread 1.
            // 4)  But before cancellation logic gets the queue's lock
            //      thread 2 calls WdfIoQueueRetrieveNextRequest.
            // 5)  WdfIoQueueRetrieveNextRequest returns no more requests.
            //      Driver waits for the ReadyNotify callback. (count 1)
            // 6)  Thread 3 adds a new request in queue. (count 1->2)
            // 7)  Thread 1 finally runs. (count 2->1).
            // 8)  At this point driver stops responding b/c it never receives ReadyNotify.
            //
            // This code below forces the queue logic to send a ReadyNotify 
            // callback the next time a new request is added (in step 6 above).
            //
            if (STATUS_NO_MORE_ENTRIES == status &&
                NULL == FileObject && // WdfIoQueueRetrieveNextRequest
                NULL == TagRequest && // WdfIoQueueRetrieveNextRequest
                m_Queue.GetRequestCount() > 0L)
            {                
                m_ForceTransitionFromEmptyWhenAddingNewRequest = TRUE;
            }
                
            Unlock(irql);
            return status;
        }

        //
        // If we don't allow zero length read/write's to the driver,
        // complete it now with success and attempt to get another
        // request from the queue.
        //
        if (!m_AllowZeroLengthRequests)
        {
            (VOID)pRequest->GetCurrentIrpStackLocation();

            FxIrp* pIrp = pRequest->GetFxIrp();
            UCHAR majorFunction = pIrp->GetMajorFunction();

            if ((majorFunction == IRP_MJ_READ) &&
                (pIrp->GetParameterReadLength() == 0))
            {
                Unlock(irql);
                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                    "Zero length WDFREQUEST 0x%p completed automatically by WDFQUEUE 0x%p",
                    pRequest->GetHandle(),GetObjectHandle());
                pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
                pRequest->RELEASE(FXREQUEST_COMPLETE_TAG);

                Lock(&irql);

                // Get another request from the queue
                continue;
            }
            else if ((majorFunction == IRP_MJ_WRITE) &&
                     (pIrp->GetParameterWriteLength() == 0))
            {
                Unlock(irql);
                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                    "Zero length WDFREQUEST 0x%p completed automatically by WDFQUEUE 0x%p",
                    pRequest->GetHandle(), GetObjectHandle());

                pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
                pRequest->RELEASE(FXREQUEST_COMPLETE_TAG);

                Lock(&irql);

                // Get another request from the queue
                continue;
            }
        }

        break;
    }

    // Increase the driver owned request count
    InsertInDriverOwnedList(pRequest);

    Unlock(irql);

    //
    // We don't need to check for PurgeComplete since
    // we are giving the request to the driver
    //

    // pRequest is not cancellable now

    //
    // We are now going to return the request
    // to the driver, and it must complete it.
    //

    //
    // Set a completion event, this takes a reference
    //
    oldState = pRequest->SetCompletionState(FxRequestCompletionStateQueue);
    ASSERT(oldState == FxRequestCompletionStateNone);
    UNREFERENCED_PARAMETER(oldState);

    //
    // Track that we have given the request to the driver
    //
    VerifyGetRequestRestoreFlags(pFxDriverGlobals, pRequest);
  
    pRequest->SetPresented();

    //
    // Release our original reference. The FxRequest::Complete
    // will release the final one since we have registered a completion
    // callback handler
    //
    // We now have one reference count on the FxRequest object until
    // its completion routine runs since the completion event made
    // an extra reference, and will dereference it when it fires, or
    // its canceled.
    //

    pRequest->RELEASE(FXREQUEST_STATE_TAG);

    // Return it to the driver
    *pOutRequest = pRequest;

    return STATUS_SUCCESS;
}

#ifdef __GNUC__
_Must_inspect_result_
NTSTATUS
FxIoQueue::Vf_VerifyGetRequestUpdateFlags (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* TagRequest
    )
#else
_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxIoQueue, VerifyGetRequestUpdateFlags) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* TagRequest
    )
#endif
{
    KIRQL irql;
    NTSTATUS status;

    PAGED_CODE_LOCKED();
    
    if (TagRequest != NULL)
    {
        //
        // WdfIoQueueRetrieveFoundRequest is only valid on manual queues.
        // v1.11 and above: driver is not required to find the request 
        //     using WdfIoQueueFindRequest.
        // v1.9 and below: driver is required to find the request
        //     using WdfIoQueueFindRequest.
        //
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11))
        {
            if (m_Type != WdfIoQueueDispatchManual)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                DoTraceLevelMessage(
                        FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                        "WdfIoQueueRetrieveFoundRequest is allowed "
                        "only on a manual queue 0x%p, %!STATUS!",
                        GetHandle(), status);
                FxVerifierDbgBreakPoint(FxDriverGlobals);
                return status;
            }
        }
        else
        {
            //
            // Legacy validation.
            //
            TagRequest->Lock(&irql);
            status = TagRequest->VerifyRequestIsTagRequest(FxDriverGlobals);
            TagRequest->Unlock(irql);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    }

    Lock(&irql);
    if ((m_Type == WdfIoQueueDispatchSequential) && (m_DriverIoCount == 0))
    {

       DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
           "Driver called WdfIoQueueRetrieveNextRequest on a sequential WDFQUEUE 0x%p with no "
           "outstanding requests. This can cause a race with automatically dispatched "
           "requests. Call WdfIoQueueRetrieveNextRequest before completing the current request(s)",
           GetObjectHandle());

       FxVerifierDbgBreakPoint(FxDriverGlobals);

       // Allow them to continue, though this is a race condition in their driver
    }
    Unlock(irql);
    
    return STATUS_SUCCESS;
}

#ifdef __GNUC__
VOID
FxIoQueue::Vf_VerifyGetRequestRestoreFlags(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* pRequest
    )
#else
VOID
FX_VF_METHOD(FxIoQueue, VerifyGetRequestRestoreFlags)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* pRequest
    )
#endif
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    KIRQL irql;

    PAGED_CODE_LOCKED();
    pRequest->Lock(&irql);

    pRequest->ClearVerifierFlagsLocked(FXREQUEST_FLAG_TAG_REQUEST);
    pRequest->SetVerifierFlagsLocked(FXREQUEST_FLAG_DRIVER_OWNED);

    pRequest->Unlock(irql);
}

#ifdef __GNUC__
VOID
FxIoQueue::Vf_VerifyValidateCompletedRequest(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request
    )
#else
VOID
FX_VF_METHOD(FxIoQueue, VerifyValidateCompletedRequest)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* Request
    )
#endif
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);

    PAGED_CODE_LOCKED();

    PLIST_ENTRY pEntry;
    KIRQL irql;

    Request->Lock(&irql);

    (VOID) Request->VerifyRequestIsDriverOwned(FxDriverGlobals);
    Request->ClearVerifierFlagsLocked(FXREQUEST_FLAG_DRIVER_OWNED);

    Request->Unlock(irql);

    // Driver no longer owns it once completed

    // Request can't be on a cancel list
    pEntry = Request->GetListEntry(FxListEntryQueueOwned);
    if ( !IsListEmpty(pEntry) )
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
            "WDFREQUEST 0x%p is on a cancellation list for WDFQUEUE 0x%p",
            Request->GetHandle(), GetObjectHandle());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
}

VOID
FxIoQueue::StartPowerTransitionOn(
    VOID
    )
/*++
    Routine Description: Start dispatching I/Os

    Arguments: VOID

    Return Value:  VOID
--*/
{
    KIRQL irql;

    if (m_PowerManaged == FALSE)
    {
        ASSERT(m_PowerState == FxIoQueuePowerOn);
        return;
    }

    Lock(&irql);

    if (m_Deleted == FALSE)
    {
        ASSERT(m_PowerState == FxIoQueuePowerOn);
    }
    
    //
    // If there are requests in the queue when we power up, we
    // should set m_TransitionFromEmpty to trigger event-ready notify
    // callback on the manual queue to kick start processing of requests. 
    // If we don't set, there is a posibility for abandoning the requests in the 
    // the queue if the queue is powered off between the time we call 
    // ProcessReadNotify and the call to retrieve requests made by the driver 
    // because the retrieve call will fail and the request will be left in the 
    // queue with m_TransitionFromEmpty state cleared.
    //
    if (m_Queue.GetRequestCount() > 0L)
    {
        m_TransitionFromEmpty = TRUE;
        m_ForceTransitionFromEmptyWhenAddingNewRequest = FALSE;
    }

    DispatchEvents(irql);

    return;
}

VOID
FxIoQueue::ResumeProcessingForPower(
    VOID
    )
/*++

    Routine Description:

    Resumes a PowerManaged queue for automatic I/O processing due to
    a power event that allows I/O to resume.

    Does nothing if its a non-power managed queue.

    Additional reference is already taken on the object by the caller
    to prevent the queue from being deleted.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    KIRQL irql;

    //
    // If not power managed, leave it alone
    //
    if (!m_PowerManaged)
    {
        ASSERT(m_PowerState == FxIoQueuePowerOn);
        return;
    }

    Lock(&irql);

    if (m_PowerState == FxIoQueuePowerOn)
    {
        Unlock(irql);
        return;
    }

    ASSERT(m_PowerState == FxIoQueuePowerOff);

    m_PowerState = FxIoQueuePowerRestarting;

    //
    // We have transitioned to a status that resumes
    // processing, so call dispatch function.
    //

    DispatchEvents(irql);

    return;
}

VOID
FxIoQueue::QueueStart(
    )
{
    KIRQL irql;

    Lock(&irql);

    SetState((FX_IO_QUEUE_SET_STATE)(FxIoQueueSetAcceptRequests | FxIoQueueSetDispatchRequests) );

    //
    // We should set the flag to notify the driver on queue start in case
    // the driver stops the queue while the ReadyNotify callback is executing.
    // If that happens, the request will be left in the manual queue with 
    // m_TransitionFromEmpty cleared.
    //
    if (m_Queue.GetRequestCount() > 0L)
    {
        m_TransitionFromEmpty = TRUE;
        m_ForceTransitionFromEmptyWhenAddingNewRequest = FALSE;
    }

    //
    // We may have transitioned to a status that resumes
    // processing, so call dispatch function.
    //

    DispatchEvents(irql);

    return;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::RequestCancelable(
    __in FxRequest* pRequest,
    __in BOOLEAN    Cancelable,
    __in_opt PFN_WDF_REQUEST_CANCEL  EvtRequestCancel,
    __in BOOLEAN    FailIfIrpIsCancelled
   )
/*++

    Routine Description:

        This is called to mark or unmark the request cancelable.

    Arguments:

        FxRequest* - Request that is completing

        Cancelable - if TRUE, mark the request cancellable
                     if FALSE, mark the request not cancelable
                        if it's previously marked canceelable.
                        
        EvtRequestCancel - points to driver provided cancel routine
                              if the cancelable flag is TRUE.
                              
        FailIfIrpIsCancelled - if FALSE and the IRP is already cancelled,
                                  call the provided cancel routine and 
                                  return success.
                               if TRUE and the IRP is already cancelled,
                                  return STATUS_CANCELLED.
                                    
    Returns:
    
        NTSTATUS

--*/
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    KIRQL irql;

    status = VerifyRequestCancelable(FxDriverGlobals, pRequest, Cancelable);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (Cancelable)
    {
        if (FxDriverGlobals->FxVerifierOn)
        {
            pRequest->SetVerifierFlags(FXREQUEST_FLAG_DRIVER_CANCELABLE);
        }
        //
        // Set the Request for cancel status by inserting in the driver owned
        // CSQ. Note: This could fire the cancel callback right away
        // if the IRP was already cancelled.
        //

        ASSERT(EvtRequestCancel);

        Lock(&irql);

        pRequest->m_CancelRoutine.m_Cancel = EvtRequestCancel;

        //
        // Check if we need to delete this request.
        //
        if (m_CancelDispatchedRequests)
        {
            //
            // Purge is in progress, cancel this request.
            //
            status = STATUS_CANCELLED;
        }
        else
        {
            status = pRequest->InsertTailIrpQueue(&m_DriverCancelable, NULL);
        }

        if (NT_SUCCESS(status))
        {
            Unlock(irql);
        }
        else if (FailIfIrpIsCancelled == FALSE)
        {
            ASSERT(status == STATUS_CANCELLED);

            // This is not an error to the driver
            status = STATUS_SUCCESS;

            pRequest->m_Canceled = TRUE;

            Unlock(irql);

            //
            // We must add a reference since the CancelForDriver path
            // assumes we were on the FxIrpQueue with the extra reference
            //
            pRequest->ADDREF(FXREQUEST_QUEUE_TAG);

            //
            // Mark the request as cancelled, place it on the cancel list,
            // and schedule the cancel event to the driver
            //
            CancelForDriver(pRequest);
        }
        else
        {            
            ASSERT(status == STATUS_CANCELLED);

            pRequest->m_CancelRoutine.m_Cancel = NULL;

            //
            // Let the caller complete the request with STATUS_CANCELLED.
            //
            Unlock(irql);

            if (FxDriverGlobals->FxVerifierOn)
            {
                pRequest->ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_CANCELABLE);
            }
        }

        return status;
    }
    else
    {
        //
        // This can return STATUS_CANCELLED if the request
        // has been canceled already
        //
        Lock(&irql);
        status = pRequest->RemoveFromIrpQueue(&m_DriverCancelable);

        if (NT_SUCCESS(status))
        {
            pRequest->m_CancelRoutine.m_Cancel = NULL;
        }
        else
        {
            //
            // In the failure case, the cancel routine has won the race and will
            // be invoked on another thread.
            //
            DO_NOTHING();
        }
        Unlock(irql);

        if (FxDriverGlobals->FxVerifierOn)
        {

            // We got the request back, can clear the cancelable flag
            if (NT_SUCCESS(status))
            {
                pRequest->ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_CANCELABLE);
            }
        }

        return status;
    }
}

#ifdef __GNUC__
_Must_inspect_result_
NTSTATUS
FxIoQueue::Vf_VerifyRequestCancelable (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* pRequest,
    _In_ BOOLEAN Cancelable
    )
#else
_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxIoQueue, VerifyRequestCancelable) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* pRequest,
    _In_ BOOLEAN Cancelable
    )
#endif
{
    NTSTATUS status;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    pRequest->Lock(&irql);

    // Make sure the driver owns the request
    status = pRequest->VerifyRequestIsDriverOwned(FxDriverGlobals);
    if (!NT_SUCCESS(status))
    {
        goto Done;
    }

    if (Cancelable)
    {
        //
        // Make sure the request is not cancelable for it to be made
        // cancelable.
        //
        status = pRequest->VerifyRequestIsNotCancelable(FxDriverGlobals);
        if (!NT_SUCCESS(status))
        {
            goto Done;
        }
    }
    else
    {
        //
        // Make sure the request is cancelable for it to be made
        // uncancelable.
        //
        status = pRequest->VerifyRequestIsCancelable(FxDriverGlobals);
        if (!NT_SUCCESS(status))
        {
            goto Done;
        }
    }
    
Done:
    pRequest->Unlock(irql);
    return status;
}

VOID
FxIoQueue::StartPowerTransitionOff(
    )
/*++

    Routine Description:

    Purpose of this routine is to put the queue in state that would
    prevent any new requests from being dispatched to the driver.

Arguments:

Return Value:

    VOID

--*/
{
    KIRQL irql;
    BOOLEAN result;

    if (m_PowerManaged == FALSE)
    {
        return;
    }

    Lock(&irql);

    if (m_Deleted == FALSE)
    {
        ASSERT(m_PowerState == FxIoQueuePowerOn);
    }
    
    m_PowerState = FxIoQueuePowerStartingTransition;

    // We must wait on the current thread until the queue is actually idle
    m_PowerIdle.Clear();

    //
    // Run the event dispatching loop before waiting on the event
    // in case this thread actually performs the transition
    //
    result = DispatchEvents(irql);
    if (result)
    {
        //
        // This is called from a kernel mode PNP thread, so we do not need
        // a KeEnterCriticalRegion()
        //
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Waiting for all threads to stop dispatching requests"
                            " so that WDFQUEUE 0x%p can be powered off",
                            GetObjectHandle());

        GetDriverGlobals()->WaitForSignal(m_PowerIdle.GetSelfPointer(), 
                "waiting for all threads to stop dispatching requests so "
                "that queue can be powered off, WDFQUEUE", GetHandle(),
                GetDriverGlobals()->FxVerifierDbgWaitForSignalTimeoutInSec,
                WaitSignalBreakUnderVerifier
                );
    }

    return;
}

VOID
FxIoQueue::StopProcessingForPower(
    __in FxIoStopProcessingForPowerAction Action
    )
/*++

    Routine Description:

    Stops automatic I/O processing due to a power event that requires I/O to stop.

    This is called on a PASSIVE_LEVEL thread that can block until
    I/O has been stopped, or completed/cancelled.

    Additional reference is already taken on the object by the caller
    to prevent the queue from being deleted.


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

Return Value:

    NTSTATUS

--*/
{
    KIRQL irql;
    BOOLEAN result;

    switch (Action) {
    case FxIoStopProcessingForPowerPurgeNonManaged:

        //
        // If power managed, leave it alone
        //
        if (m_PowerManaged == TRUE)
        {
            // Should be powered off by now.
            ASSERT(m_PowerState == FxIoQueuePowerOff);
            return;
        }
        
        //
        // Queue is being shut down. This flag prevents the following:
        // (1) a race condition where a dispatch queue handler changes the
        //     state of the queue to accept_requests while we are in the 
        //     middle of a power stopping (purge) operation 

        // (2) another thread calling Stop or Start on a queue that is in the
        //     middle of a power stopping (purge) operation.
        //
        Lock(&irql);
        SetStateForShutdown();
        Unlock(irql);
        
        QueuePurge(TRUE, TRUE, NULL, NULL);

        Lock(&irql);
        //
        // Queue must be in PowerOn state.
        //
        ASSERT(m_PowerState == FxIoQueuePowerOn);

        m_PowerState = FxIoQueuePowerPurge;

        break;

    case FxIoStopProcessingForPowerPurgeManaged:

        //
        // If not power managed, leave it alone
        //
        if (m_PowerManaged == FALSE)
        {
            ASSERT(m_PowerState == FxIoQueuePowerOn);
            return;
        }
        
        //
        // Queue is being shut down. This flag prevents the following:
        // (1) a race condition where a dispatch queue handler changes the
        //     state of the queue to accept_requests while we are in the 
        //     middle of a power stopping (purge) operation 

        // (2) another thread calling Stop or Start on a queue that is in the
        //     middle of a power stopping (purge) operation.
        //
        Lock(&irql);
        SetStateForShutdown();
        Unlock(irql);     

        QueuePurge(TRUE, TRUE, NULL, NULL);

        Lock(&irql);

        m_PowerState = FxIoQueuePowerPurge;

        break;

    case FxIoStopProcessingForPowerHold:
        //
        // If not power managed, leave it alone
        //
        if(m_PowerManaged == FALSE) {
            ASSERT(m_PowerState == FxIoQueuePowerOn);
            return;
        }

        Lock(&irql);

        m_PowerState = FxIoQueuePowerStopping;

        break;

    default:
        ASSERT(FALSE);
        return;
    }

    // We must wait on the current thread until the queue is actually idle
    m_PowerIdle.Clear();

    //
    // Run the event dispatching loop before waiting on the event
    // in case this thread actually performs the transition
    //
    result = DispatchEvents(irql);
    if (result)
    {
        //
        // This is called from a kernel mode PNP thread, so we do not need
        // a KeEnterCriticalRegion()
        //
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Waiting for all inflight requests to be acknowledged "
                            " on WDFQUEUE 0x%p",
                            GetObjectHandle());

        GetDriverGlobals()->WaitForSignal(m_PowerIdle.GetSelfPointer(), 
                                "waiting for all inflight requests "
                                "to be acknowledged on WDFQUEUE",
                                GetHandle(),
                                GetDriverGlobals()->FxVerifierDbgWaitForSignalTimeoutInSec,
                                WaitSignalBreakUnderVerifier);
    }

    return;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueuePurge(
    __in BOOLEAN                 CancelQueueRequests,
    __in BOOLEAN                 CancelDriverRequests,
    __in_opt PFN_WDF_IO_QUEUE_STATE PurgeComplete,
    __in_opt WDFCONTEXT              Context
    )
/*++

Routine Description:

    Purge the Queue.

     If CancelQueueRequests == TRUE, any requests in the
     Queue that have not been presented to the device driver are
     completed with STATUS_CANCELLED.

     If CancelDriverRequests == TRUE, any requests that the
     driver is operating on that are cancelable will have an
     I/O Cancel done on them.

Arguments:

Returns:

--*/
{
    FxRequest*  pRequest;
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    KIRQL irql;
    NTSTATUS status;

    Lock(&irql);

    //
    // If the Queue is deleted, there can't be any requests
    // to purge, and the queue is no longer executing its
    // event dispatch loop, so we would stop responding if we 
    // registered now.
    //
    // We could try and silently succeed this, but if we do, we
    // must invoke the PurgeComplete callback, and without our
    // queue state machine excuting, we can not ensure any
    // callback constraints are handled such as locking, queueing
    // to passive level, etc. So we just fail to indicate to the
    // driver we *will not* be invoking its PurgeComplete function.
    //
    if (m_Deleted)
    {
        status = STATUS_DELETE_PENDING;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFQUEUE 0x%p is already deleted %!STATUS!",
                            GetObjectHandle(), status);
        Unlock(irql);

        return status;
    }

    //
    // If a PurgeComplete callback is supplied, we must register it up
    // front since a transition empty could occur in another thread.
    //
    if (PurgeComplete != NULL)
    {
        //
        // Only one PurgeComplete callback can be outstanding
        // at a time per Queue
        //
        if (m_PurgeComplete.Method != NULL)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "WDFQUEUE 0x%p already has a "
                                "PurgeComplete callback registered 0x%p "
                                "%!STATUS!", GetObjectHandle(),
                                m_PurgeComplete.Method, status);
            Unlock(irql);

            return status;
        }

        m_PurgeComplete.Method = PurgeComplete;
        m_PurgeCompleteContext = Context;
    }

    // Clear accept requests
    SetState(FxIoQueueClearAcceptRequests);
    
    if (CancelQueueRequests && CancelDriverRequests &&
        FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11))
    {
        //
        // Driver wants to abort/complete all queued request and cancel or 
        // wait for all requests the driver is currently handling. Thus we must
        // prevent the driver from requeuing stale requests. 
        // This flag is set here, and cleared when:
        //  (a) Driver doesn't own any more requests, or 
        //  (b) the driver calls WdfIoQueueStart again (dispatch gate is opened).
        // When set, the framework automatically deletes  any request that the
        // driver requeues.
        // For compatibility we do this only for drivers v1.11 and above.
        //
        m_CancelDispatchedRequests = TRUE;
    }

    // Unlock queue lock
    Unlock(irql);

    if (CancelQueueRequests)
    {
#ifdef _MSC_VER
        #pragma warning(disable:4127)
#endif
        while (TRUE)
        {
#ifdef _MSC_VER
            #pragma warning(default:4127)
#endif
            //
            // Get the next FxRequest from the cancel safe queue
            //
            Lock(&irql);
            pRequest = FxRequest::GetNextRequest(&m_Queue);
            if (pRequest == NULL)
            {
                DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                    "All WDFQUEUE 0x%p requests cancelled",
                                    GetObjectHandle());
                Unlock(irql);
                break;
            }

            // Irp is not cancellable now

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIO,
                                "Cancelling WDFREQUEST 0x%p, WDFQUEUE 0x%p",
                                pRequest->GetHandle(),GetObjectHandle());

            //
            // We must add a reference since the CancelForQueue path
            // assumes we were on the FxIrpQueue with the extra reference
            pRequest->ADDREF(FXREQUEST_QUEUE_TAG);
            
            //
            // Mark the request as cancelled, place it on the cancel list,
            // and schedule the cancel event to the driver
            //
            CancelForQueue(pRequest, irql);

        }
    }

    if (CancelDriverRequests)
    {
        //
        // Walk the driver cancelable list cancelling
        // the requests.
        //
#ifdef _MSC_VER
        #pragma warning(disable:4127)
#endif
        while (TRUE)
        {
#ifdef _MSC_VER
        #pragma warning(default:4127)
#endif
            //
            // Get the next request of driver cancelable requests
            //
            Lock(&irql);
            pRequest = FxRequest::GetNextRequest(&m_DriverCancelable);
            if (pRequest == NULL)
            {
                DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                                    "All driver cancellable requests cancelled "
                                    " in WDFQUEUE 0x%p",
                                    GetObjectHandle());
                Unlock(irql);
                break;
            }

            pRequest->m_Canceled = TRUE;

            Unlock(irql);

            //
            // If the driver follows the pattern of removing cancel status
            // from the request before completion, then there is no race
            // with this routine since we will not be able to retrieve any
            // requests the driver has made non-cancellable in preparation
            // for completion.
            //
            pRequest->ADDREF(FXREQUEST_QUEUE_TAG);

            CancelForDriver(pRequest);

            // The request could have been completed and released by the driver
        }
    }

    if (IsForwardProgressQueue())
    {
        PurgeForwardProgressIrps(NULL);
    }
    
    //
    // Since we set that no new requests may be enqueued,
    // if both m_Queue.GetRequestCount() and m_DriverIoCount == 0 right
    // now the queue is completely purged.
    //

    //
    // We check if our m_PurgeComplete callback is still set
    // since it may have been called by another thread when
    // we dropped the lock above
    //
    Lock(&irql);

    DispatchEvents(irql);

    //
    // If the driver registered a PurgeComplete callback, and it was
    // not empty in the above check, it will be called when a
    // request complete from the device driver completes the
    // final request.
    //
    return STATUS_SUCCESS;
}

//__declspec(noreturn)
VOID
#ifndef __GNUC__
DECLSPEC_NORETURN
#endif
FxIoQueue::FatalError(
    __in NTSTATUS Status
    )
{
    WDF_QUEUE_FATAL_ERROR_DATA data;

    RtlZeroMemory(&data, sizeof(data));

    data.Queue = GetHandle();
    data.Request = NULL;
    data.Status = Status;

    FxVerifierBugCheck(GetDriverGlobals(),
                       WDF_QUEUE_FATAL_ERROR,
                       (ULONG_PTR) &data);
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueIdleSynchronously(
    __in BOOLEAN    CancelRequests
    )
/*++

Routine Description:

    Idle the Queue and wait for the driver-owned requests to complete.

Arguments:

    CancelRequests - If TRUE, functions tries to cancel outstanding requests.
    
Returns:

--*/
{
    NTSTATUS status;
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    MxEvent* event = this->m_RequestWaitEventUm.GetSelfPointer();
#else
    MxEvent eventOnStack;
    //
    // Note that initialize always succeeds in KM so return is not checked. 
    //
    eventOnStack.Initialize(NotificationEvent, FALSE);             
    MxEvent* event = eventOnStack.GetSelfPointer();
#endif 

    status = QueueIdle(CancelRequests, _IdleComplete, event->GetSelfPointer());

    if(NT_SUCCESS(status))
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Waiting for %d requests to complete "
                        "on WDFQUEUE 0x%p",
                        m_DriverIoCount,
                        GetObjectHandle());

        Mx::MxEnterCriticalRegion();

        GetDriverGlobals()->WaitForSignal(event->GetSelfPointer(), 
                "waiting for queue to stop, WDFQUEUE", GetHandle(),
                GetDriverGlobals()->FxVerifierDbgWaitForSignalTimeoutInSec,
                WaitSignalBreakUnderVerifier);


        Mx::MxLeaveCriticalRegion();
    }

    return status;

}

VOID
FxIoQueue::_IdleComplete(
    __in WDFQUEUE       Queue,
    __in WDFCONTEXT     Context
    )
/*++

Routine Description:
   Callback function when a stop completes

--*/
{
    MxEvent* event = (MxEvent*)Context;

    UNREFERENCED_PARAMETER(Queue);

    event->SetWithIncrement(EVENT_INCREMENT);

    return;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueIdle(
    __in BOOLEAN                    CancelRequests,
    __in_opt PFN_WDF_IO_QUEUE_STATE IdleComplete,
    __in_opt WDFCONTEXT             Context
    )

/*++

Routine Description:

    Idle (stop) the Queue.
    
    If CancelRequests == TRUE, 
        1) any requests in the Queue that have not been presented to the device driver are
            completed with STATUS_CANCELLED.
        2) any requests that the driver is operating on that are cancelable will have an
            I/O Cancel done on them.
        3) any forward progress queued IRPs are completed with STATUS_CANCELLED.

Arguments:

Returns:

--*/

{
    PFX_DRIVER_GLOBALS  fxDriverGlobals = GetDriverGlobals();
    KIRQL               irql;
    NTSTATUS            status;
    LIST_ENTRY          fwrIrpList = {0};
    FxRequest*          request;
    

    Lock(&irql);

    // If the queue is deleted, requests will not be serviced anymore
    if (m_Deleted)
    {
        status = STATUS_DELETE_PENDING;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDFQUEUE 0x%p is already deleted, %!STATUS!",
                            GetObjectHandle(), status);
        Unlock(irql);

        return status;
    }

    //
    // If a IdleComplete callback is supplied, we must register it up
    // front since a transition empty could occur in another thread.
    //
    if (IdleComplete != NULL)
    {
        //
        // Only one Idle or Purge Complete callback can be outstanding
        // at a time per Queue
        //
        if (m_IdleComplete.Method != NULL)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "WDFQUEUE 0x%p already has a "
                                "IdleComplete callback registered 0x%p, "
                                "%!STATUS!", GetObjectHandle(),
                                m_IdleComplete.Method,
                                status);
            Unlock(irql);

            return status;
        }

        m_IdleComplete.Method = IdleComplete;
        m_IdleCompleteContext = Context;
    }

    // Set Accept request and Clear dispatch requests
    SetState((FX_IO_QUEUE_SET_STATE)(FxIoQueueSetAcceptRequests | FxIoQueueClearDispatchRequests));

    //
    // Get ready to cancel current queued requests. Note that we don't want to 
    // prevent new requests from being queue, i.e., it is legal for an upper 
    // driver can resend another request in its completion routine.
    //
    if (CancelRequests)
    {
        //
        // Driver wants to abort/complete all queued request and cancel or 
        // wait for all requests the driver is currently handling. Thus we must
        // prevent the driver from requeuing stale requests. 
        // The 'cancel driver requests'  field gives us this ability. 
        // It is set here, and cleared when:
        //  (a) Driver doesn't own any more requests, or 
        //  (b) the driver calls WdfIoQueueStart again (dispatch gate is opened).
        // When set, the framework automatically deletes  any request that the
        // driver requeues.
        //
        m_CancelDispatchedRequests = TRUE;

        request = NULL; // Initial tag used by PeekRequest.
#ifdef _MSC_VER
        #pragma warning(disable:4127)
#endif
        while (TRUE)
        {
#ifdef _MSC_VER
        #pragma warning(default:4127)
#endif
            status = FxRequest::PeekRequest(&m_Queue,       // in:queue
                                            request,        // in:tag. 
                                            NULL,           // in:file_obj
                                            NULL,           // out:parameters
                                            &request);      // out:request.
            if (status != STATUS_SUCCESS)
            {
                ASSERT(status != STATUS_NOT_FOUND);
                break;
            }
            
            //
            // Tag this request and release the extra ref that Peek() takes.
            //
            request->m_Canceled = TRUE;

            request->RELEASE(NULL);
        }        
        
        //
        // Move forward progress IRPs to a temp list; we use this logic to 
        // allow new  IRPs to be pended to the original list.
        //
        if (IsForwardProgressQueue())
        {
            InitializeListHead(&fwrIrpList);
            GetForwardProgressIrps(&fwrIrpList, NULL);
        }
    }

    // Unlock queue lock
    Unlock(irql);

    if (CancelRequests)
    {
#ifdef _MSC_VER
        #pragma warning(disable:4127)
#endif
        while (TRUE)
        {
#ifdef _MSC_VER
        #pragma warning(default:4127)
#endif
            //
            // Get the next FxRequest from the cancel safe queue
            //
            Lock(&irql);
            request = FxRequest::GetNextRequest(&m_Queue);
            if (request == NULL)
            {
                DoTraceLevelMessage(
                            fxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "All WDFQUEUE 0x%p requests cancelled",
                            GetObjectHandle());
                Unlock(irql);
                break;
            }

            // Irp is not cancellable now

            //
            // Make sure to purged requests only if:
            // (a) the request was present when we started this operation.
            // (b) any following request that is marked as cancelled.
            //
            if (request->IsCancelled() == FALSE)
            {
                status = request->InsertHeadIrpQueue(&m_Queue, NULL);
                if (NT_SUCCESS(status))
                {
                    Unlock(irql);
                    break;
                }

                ASSERT(status == STATUS_CANCELLED);
            }
            DoTraceLevelMessage(
                        fxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIO,
                        "Cancelling WDFREQUEST 0x%p, WDFQUEUE 0x%p",
                        request->GetHandle(),GetObjectHandle());
            
            //
            // We must add a reference since the CancelForQueue path
            // assumes we were on the FxIrpQueue with the extra reference
            //
            request->ADDREF(FXREQUEST_QUEUE_TAG);
            
            //
            // Mark the request as cancelled, place it on the cancel list,
            // and schedule the cancel event to the driver
            //
            CancelForQueue(request, irql);
        }

        //
        // Walk the driver cancelable list cancelling the requests.
        //
#ifdef _MSC_VER
        #pragma warning(disable:4127)
#endif
        while (TRUE)
        {
#ifdef _MSC_VER
        #pragma warning(default:4127)
#endif
            //
            // Get the next request of driver cancelable requests
            //
            Lock(&irql);
            request = FxRequest::GetNextRequest(&m_DriverCancelable);
            if (request == NULL)
            {
                DoTraceLevelMessage(
                            fxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "All driver cancellable requests cancelled "
                            " in WDFQUEUE 0x%p",
                            GetObjectHandle());
                Unlock(irql);
                break;
            }

            request->m_Canceled = TRUE;

            Unlock(irql);

            //
            // If the driver follows the pattern of removing cancel status
            // from the request before completion, then there is no race
            // with this routine since we will not be able to retrieve any
            // requests the driver has made non-cancellable in preparation
            // for completion.
            //
            request->ADDREF(FXREQUEST_QUEUE_TAG);

            CancelForDriver(request);

            // The request could have been completed and released by the driver
        }

        //
        // Cleanup forward progress IRP list.
        //
        if (IsForwardProgressQueue())
        {
            CancelIrps(&fwrIrpList);
        }
    }

    //
    // Since we set that no new requests may be dispatched,
    // if both m_Queue.GetRequestCount(), m_DriverIoCount == 0, and
    // m_Dispatch == 0, right now the queue is completely idle.
    //

    //
    // We check if our m_PurgeComplete callback is still set
    // since it may have been called by another thread when
    // we dropped the lock above
    //
    Lock(&irql);
    DispatchEvents(irql);

    //
    // If the driver registered an IdleComplete callback, and it was
    // not idle in the above check, it will be called when the final
    // callback handler from the device driver returns.
    //
    return STATUS_SUCCESS;
}
