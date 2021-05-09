/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIrpQueue.cpp

Abstract:

    This module implements a common queue structure for the
    driver frameworks built around the Cancel Safe Queue model

Author:







Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxIrpQueue.tmh"
}

//
// Public constructors
//

FxIrpQueue::FxIrpQueue(
    VOID
    )
{
    InitializeListHead(&m_Queue);

    m_LockObject     = NULL;

    m_CancelCallback = NULL;

    m_RequestCount   = 0;
}

FxIrpQueue::~FxIrpQueue()
{
    ASSERT(IsListEmpty(&m_Queue));
}

VOID
FxIrpQueue::Initialize(
    __in FxNonPagedObject* LockObject,
    __in PFN_IRP_QUEUE_CANCEL Callback
    )

/*++

Routine Description:

    Initialize the FxIrpQueue.

    Set the callback for when an IRP gets cancelled.

    The callback function is only called when an IRP
    gets cancelled, and is called with no locks held.

    The cancel function that the caller registers is
    responsible for completing the IRP with IoCompleteRequest.

    If no Cancel Callback is set, or is set to NULL, IRP's will
    be automatically completed with STATUS_CANCELED by
    the FxIrpQueue when they are canceled.


    If the caller supplies a LockObject, this object is used
    to synchronize cancellation access to the list, but it is
    expected that the caller be holding the same lock for insert/remove
    operations. This allows the caller to perform insert/remove operations
    using its own lock in a race free manner.

    If a LockObject is not supplied, the FxIrpQueue uses its own lock.

Arguments:

    LockObject - Object whose lock controls the queue

    Callback - Driver callback function

Returns:

    None

--*/

{
    ASSERT(LockObject != NULL);

    m_CancelCallback = Callback;
    m_LockObject = LockObject;
}


_Must_inspect_result_
NTSTATUS
FxIrpQueue::InsertTailRequest(
    __inout MdIrp Irp,
    __in_opt PMdIoCsqIrpContext Context,
    __out_opt ULONG* pRequestCount
    )

/*++

Routine Description:

    Enqueue a request to the end of the queue (FIFO) and
    marks it as pending.

    The PMdIoCsqIrpContext is associated with the IRP.

    PIO_CSQ_IRP_CONTEXT must be in non-paged pool, and can
    not be released until the IRP is is finally released from
    the queue.

Arguments:

    Irp - Pointer to IRP

    Context - Pointer to caller allocated CSQ context

    pRequestCount - Location to return new request count of queue
                    after insertion

Returns:

    STATUS_SUCCESS - Operation completed.

    STATUS_CANCELLED - Request was cancelled, and not inserted
                       Call is responsible for completing it.
--*/

{
    NTSTATUS Status;

    // Note: This marks the IRP Pending
    Status = InsertIrpInQueue(
                 Irp,      // Irp to insert
                 Context,  // PIO_CSQ_IRP_CONTEXT
                 FALSE,    // InsertInHead
                 pRequestCount
                 );

    return Status;
}


_Must_inspect_result_
NTSTATUS
FxIrpQueue::InsertHeadRequest(
    __inout MdIrp Irp,
    __in_opt PMdIoCsqIrpContext Context,
    __out_opt ULONG* pRequestCount
    )

/*++

Routine Description:

    Enqueue a request to the head of the queue and
    marks it as pending.

    The PIO_CSQ_IRP_CONTEXT is associated with the IRP.

    PIO_CSQ_IRP_CONTEXT must be in non-paged pool, and can
    not be released until the IRP is is finally released from
    the queue.

Arguments:

    Irp - Pointer to IRP

    Context - Pointer to caller allocated CSQ context

    pRequestCount - Location to return new request count of queue
                    after insertion
Returns:

   STATUS_SUCCESS - Operation completed.

   STATUS_CANCELLED - Request was cancelled, and not inserted
                      Call is responsible for completing it.
--*/

{
    NTSTATUS Status;

    // Note: This marks the IRP Pending
    Status = InsertIrpInQueue(
                 Irp,      // Irp to insert
                 Context,  // PIO_CSQ_IRP_CONTEXT
                 TRUE,     // InsertInHead
                 pRequestCount
                 );

    return Status;
}


MdIrp
FxIrpQueue::GetNextRequest(
    __out PMdIoCsqIrpContext* pCsqContext
    )

/*++

Routine Description:

    Returns an IRP from the queue, and if successful
    the IRP is no longer on the CSQ (m_Queue) and
    is not non-cancellable.

--*/

{
    return RemoveNextIrpFromQueue(NULL, pCsqContext);
}


_Must_inspect_result_
NTSTATUS
FxIrpQueue::GetNextRequest(
    __in_opt PMdIoCsqIrpContext TagContext,
    __in_opt  MdFileObject         FileObject,
    __out FxRequest**          ppOutRequest
    )

/*++

Routine Description:

    Returns a request from the queue using an optional
    FileObject or TagContext.

--*/

{
    MdIrp Irp;
    FxRequest* pRequest;
    PMdIoCsqIrpContext pCsqContext;

    if( TagContext == NULL ) {

        //
        // Returns an IRP from the queue, and if successful
        // the IRP is no longer on the CSQ (m_Queue) and
        // is not non-cancellable.
        //
        Irp = RemoveNextIrpFromQueue(
                  FileObject,        // PeekContext
                  &pCsqContext
                  );

        if( Irp != NULL ) {

            pRequest = FxRequest::RetrieveFromCsqContext(pCsqContext);

            *ppOutRequest = pRequest;

            return STATUS_SUCCESS;
        }
        else {
            return STATUS_NO_MORE_ENTRIES;
        }
    }
    else {

        // Handle TagRequest Case
        Irp = RemoveIrpFromQueueByContext(
                  TagContext
                  );

        if( Irp != NULL ) {
            pRequest = FxRequest::RetrieveFromCsqContext(TagContext);

            *ppOutRequest = pRequest;

            return STATUS_SUCCESS;
        }
        else {
            return STATUS_NOT_FOUND;
        }
    }
}

_Must_inspect_result_
NTSTATUS
FxIrpQueue::PeekRequest(
    __in_opt  PMdIoCsqIrpContext TagContext,
    __in_opt  MdFileObject         FileObject,
    __out FxRequest**          ppOutRequest
    )

/*++

Routine Description:

    PeekRequest allows a caller to enumerate through requests in
    a queue, optionally only returning requests that match a specified
    FileObject.

    The first call specifies TagContext == NULL, and the first request
    in the queue that matches the FileObject is returned.

    Subsequent requests specify the previous request value as the
    TagContext, and searching will continue at the request that follows.

    If the queue is empty, there are no requests after TagContext, or no
    requests match the FileObject, NULL is returned.

    If FileObject == NULL, this matches any FileObject in a request.

    If a WDF_REQUEST_PARAMETERS structure is supplied, the information
    from the request is returned to allow the driver to further examine
    the request to decide whether to service it.

    If a TagRequest is specified, and it is not found, the return
    status STATUS_NOT_FOUND means that the queue should
    be re-scanned. This is because the TagRequest was cancelled from
    the queue, or if the queue was active, delivered to the driver.
    There may still be un-examined requests on the queue that match
    the drivers search criteria, but the search marker has been lost.

    Re-scanning the queue starting with TagRequest == NULL and
    continuing until STATUS_NO_MORE_ENTRIES is returned will ensure
    all requests have been examined.

    Enumerating an active queue with this API could result in the
    driver frequently having to re-scan.

    If a successful return of a Request object handle occurs, the driver
    *must* call WdfObjectDereference when done with it.

    NOTE: Synchronization Details

    The driver is allowed to "peek" at requests that are still on
    the Cancel Safe Queue without removing them. This means that
    the peek context value used (TagRequest) could go away while
    still holding it. This does not seem bad in itself, but the request
    could immediately be re-used by a look aside list and be re-submitted
    to the queue. At this point, the "tag" value means a completely different
    request.

    This race is dealt with by reference counting the FxRequest object
    that contains our PIO_CSQ_IRP_CONTEXT, so its memory remains valid
    after a cancel, and the driver explicitly releases it with
    WdfObjectDereference.

    But if this reference is not added under the CSQ's spinlock, there
    could be a race in which the I/O gets cancelled and the cancel
    callback completes the request, before we add our reference count.
    This would then result in attempting to reference count invalid
    memory. So to close this race, this routine returns the referenced
    FxRequest object as its result.

Arguments:

    TagRequest  - If !NULL, request to begin search at

    FileObject  - If !NULL, FileObject to match in the request


Returns:

    STATUS_NOT_FOUND - TagContext was specified, but not
                                found in the queue. This could be
                                because the request was cancelled,
                                or is part of an active queue and
                                the request was passed to the driver
                                or forwarded to another queue.

    STATUS_NO_MORE_ENTRYS -     The queue is empty, or no more requests
                                match the selection criteria of TagRequest
                                and FileObject specified above.

    STATUS_SUCCESS        -     A request context was returned in
                                pOutRequest.

--*/

{
    PLIST_ENTRY         nextEntry;
    FxIrp               nextIrp(NULL);
    PMdIoCsqIrpContext  pCsqContext;
    BOOLEAN FoundTag =  (TagContext == NULL) ? TRUE : FALSE;
    FxRequest*          pRequest;

    for( nextEntry = m_Queue.Flink; nextEntry != &this->m_Queue; nextEntry = nextEntry->Flink) {

        nextIrp.SetIrp(FxIrp::GetIrpFromListEntry(nextEntry));

        if(nextIrp.IsCanceled()) {
            //
            // This IRP is cancelled and the WdmCancelRoutine is about to run or waiting
            // for us to drop the lock. So skip this one.
            //
            continue;
        }

        pCsqContext = (PMdIoCsqIrpContext)nextIrp.GetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY);

        if( FoundTag ) {

            if( FileObject != NULL ) {

                if(nextIrp.GetFileObject() == FileObject ) {

                    pRequest = FxRequest::RetrieveFromCsqContext(pCsqContext);

                    //
                    // Must add the reference here under the protection
                    // of the cancel safe queues spinlock
                    //
                    pRequest->ADDREF(NULL);

                    *ppOutRequest = pRequest;

                    return STATUS_SUCCESS;
                }
            }
            else {

                pRequest = FxRequest::RetrieveFromCsqContext(pCsqContext);

                //
                // Must add the reference here under the protection
                // of the cancel safe queues spinlock
                //
                pRequest->ADDREF(NULL);

                *ppOutRequest = pRequest;

                return STATUS_SUCCESS;
            }
        }
        else {

            // If we found the tag, we want the *next* entry
            if( pCsqContext == TagContext ) {
                FoundTag = TRUE;
            }
        }

    }

    //
    // If the caller supplied a tag, and it was
    // not found, return a different code since
    // the caller needs to re-scan the queue.
    //
    if( (TagContext != NULL) && !FoundTag ) {
        return STATUS_NOT_FOUND;
    }
    else {
        return STATUS_NO_MORE_ENTRIES;
    }
}

MdIrp
FxIrpQueue::RemoveRequest(
    __in PMdIoCsqIrpContext Context
    )
/*++

Routine Description:

    Returns a request from the queue.

--*/
{
    MdIrp Irp;

    ASSERT(Context != NULL);

    //
    // Returns an IRP from the queue, and if success
    // the IRP is no longer on the CSQ (m_Queue) and
    // is not non-cancellable.
    //
    Irp = RemoveIrpFromQueueByContext(Context);

    return Irp;
}

_Must_inspect_result_
NTSTATUS
FxIrpQueue::InsertIrpInQueue(
    __inout   MdIrp                 Irp,
    __in_opt  PMdIoCsqIrpContext    Context,
    __in      BOOLEAN               InsertInHead,
    __out_opt ULONG*                pRequestCount
    )
/*++

Routine Description:

    Insert the IRP in the queue. If the IRP is already cancelled
    it removes the IRP from the queue and returns STATUS_CANCELLED.

    If the IRP is cancelled and the CancelRoutine has already started
    execution, then this function returns STATUS_SUCCESS.

Arguments:

    Irp - Pointer to IRP

    Context - Pointer to caller allocated CSQ context

    InsertInHead - TRUE for head, FALSE for tail.

    pRequestCount - Location to return new request count of queue
                    after insertion

Returns:

    STATUS_SUCCESS - Operation completed.

    STATUS_CANCELLED - if the request is already cancelled.

--*/
{
    FxIrp irp(Irp);
    MdCancelRoutine cancelRoutine;
    NTSTATUS        status = STATUS_SUCCESS;

    //
    // Set the association between the context and the IRP.
    //

    if (Context) {
        irp.SetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY, Context);
        Context->Irp = Irp;
        Context->Csq = (PIO_CSQ)this;










        Context->Type = FX_IRP_QUEUE_ENTRY_IDENTIFIER;
    } else {

        //
        // Currently always require context, but this will change when we
        // allow queuing or low level IRP's without FxRequest headers allocated
        //
        ASSERT(FALSE);

        irp.SetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY, this);
    }

    // See if it's a head insertion
    if( InsertInHead ) {
        InsertHeadList(
            &m_Queue,
            irp.ListEntry()
            );
    }
    else {
        InsertTailList(
            &m_Queue,
            irp.ListEntry()
            );
    }

    m_RequestCount++;

    if( pRequestCount != NULL ) {
        *pRequestCount = m_RequestCount;
    }

    irp.MarkIrpPending();

    cancelRoutine = irp.SetCancelRoutine(_WdmCancelRoutineInternal);

    ASSERT(!cancelRoutine);
    UNREFERENCED_PARAMETER(cancelRoutine);

    if (irp.IsCanceled()) {

        cancelRoutine = irp.SetCancelRoutine(NULL);

        if (cancelRoutine) {

            // Remove the IRP from the list
            RemoveIrpFromListEntry(&irp);

            if (Context) {
                Context->Irp = NULL;
            }

            irp.SetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY, NULL);

            //
            // Caller does not want us to recurse on their lock by invoking
            // the m_CancelCallback on the insert path. So they will complete
            // the IRP themselves.
            //
            return STATUS_CANCELLED;
        } else {

            //
            // The cancel routine beat us to it.
            //
            DO_NOTHING();
        }

    }

    return status;
}

VOID
FX_VF_METHOD(FxIrpQueue, VerifyRemoveIrpFromQueueByContext)(
    __in PFX_DRIVER_GLOBALS  FxDriverGlobals,
    __in PMdIoCsqIrpContext Context
    )
/*++

Routine Description:

    Makes sure that the specified Request (context) belongs to this IRP queue.

--*/
{
    PAGED_CODE_LOCKED();

    if (FxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) {
        if (Context->Irp != NULL &&
            (Context->Type != FX_IRP_QUEUE_ENTRY_IDENTIFIER ||
             Context->Csq  != (PIO_CSQ)this)) {

            //
            // This should never happen. Bugcheck before corrupting memory.
            //
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                                "Irp 0x%p (Context 0x%p) not on IRP queue 0x%p\n",
                                Context->Irp, Context, this);

            FxVerifierBugCheck(FxDriverGlobals,
                               WDF_REQUEST_FATAL_ERROR,
                               WDF_REQUEST_FATAL_ERROR_REQUEST_NOT_IN_QUEUE,
                               (ULONG_PTR) Context);
        }
    }
}

MdIrp
FxIrpQueue::RemoveIrpFromQueueByContext(
    __in PMdIoCsqIrpContext Context
    )
/*++

Routine Description:

    Using the context it remove the associated IRP from the queue.

--*/
{
    MdIrp    irp;
    MdCancelRoutine cancelRoutine;

    if (Context->Irp ) {
        //
        // Make sure the Irp belongs to this queue.
        //
        ASSERT(Context->Csq == (PIO_CSQ)this);
        VerifyRemoveIrpFromQueueByContext(m_LockObject->GetDriverGlobals(),
                                          Context);

        irp = Context->Irp;

        FxIrp fxIrp(irp);

        cancelRoutine = fxIrp.SetCancelRoutine(NULL);
        if (!cancelRoutine) {
            return NULL;
        }

        ASSERT(Context == fxIrp.GetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY));

        RemoveIrpFromListEntry(&fxIrp);

        //
        // Break the association.
        //

        Context->Irp = NULL;
        fxIrp.SetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY, NULL);

        ASSERT(Context->Csq == (PIO_CSQ)this);

        return irp;

    } else {
        return NULL;
    }
}


MdIrp
FxIrpQueue::PeekNextIrpFromQueue(
    __in_opt MdIrp    Irp,
    __in_opt PVOID   PeekContext
    )
/*++

Routine Description:

    This API look up IRP's in the queue.

    If Irp == NULL, it returns one from the head
    of the queue.

    If Irp != NULL, it is a "peek context", and the
    routine returns the *next* IRP in the queue.

--*/
{
    PLIST_ENTRY        nextEntry;
    PLIST_ENTRY        listHead;
    FxIrp              irp(Irp);
    FxIrp              nextIrp(NULL);

    listHead = &m_Queue;

    //
    // If the IRP is NULL, we will start peeking from the listhead, else
    // we will start from that IRP onwards. This is done under the
    // assumption that new IRPs are always inserted at the tail.
    //

    if(Irp == NULL) {
        nextEntry = listHead->Flink;
    } else {
        nextEntry = irp.ListEntry()->Flink;
    }

    while(nextEntry != listHead) {

        nextIrp.SetIrp(FxIrp::GetIrpFromListEntry(nextEntry));

        //
        // If PeekContext is supplied, it's a search for an IRP associated
        // with a particular file object.
        //
        if(PeekContext) {

            if(nextIrp.GetFileObject() == (MdFileObject) PeekContext) {
                break;
            }
        } else {
            break;
        }

        nextIrp.SetIrp(NULL);

        nextEntry = nextEntry->Flink;
    }

    return nextIrp.GetIrp();
}

MdIrp
FxIrpQueue::RemoveNextIrpFromQueue(
    __in_opt  PVOID                 PeekContext,
    __out_opt PMdIoCsqIrpContext*  pCsqContext
    )
/*++

Routine Description:

    This routine will return a pointer to the next IRP in the queue adjacent to
    the irp passed as a parameter. If the irp is NULL, it returns the IRP at the head of
    the queue.

--*/
{
    PMdIoCsqIrpContext context;
    MdCancelRoutine cancelRoutine;
    FxIrp    fxIrp(NULL);

    fxIrp.SetIrp(PeekNextIrpFromQueue(NULL, PeekContext));

    for (;;) {

        if (!fxIrp.GetIrp()) {
            return NULL;
        }

        cancelRoutine = fxIrp.SetCancelRoutine(NULL);
        if (!cancelRoutine) {
            fxIrp.SetIrp(PeekNextIrpFromQueue(fxIrp.GetIrp(), PeekContext));
            continue;
        }

        RemoveIrpFromListEntry(&fxIrp);    // Remove this IRP from the queue

        break;
    }

    context = (PMdIoCsqIrpContext)fxIrp.GetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY);
    if (context->Type == FX_IRP_QUEUE_ENTRY_IDENTIFIER) {
        context->Irp = NULL;
        ASSERT(context->Csq == (PIO_CSQ)this);
    }

    if(pCsqContext != NULL) {
        *pCsqContext = context;
    }

    fxIrp.SetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY, NULL);

    return fxIrp.GetIrp();
}


VOID
FxIrpQueue::_WdmCancelRoutineInternal(
    __inout MdDeviceObject DeviceObject,
    __in __drv_useCancelIRQL MdIrp   Irp
    )
/*++

Routine Description:

    This is the function called by WDM on the IRP when a cancel occurs

--*/
{
    PMdIoCsqIrpContext irpContext;
    FxIrpQueue* p;
    KIRQL irql;
    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER (DeviceObject);

    Mx::ReleaseCancelSpinLock(irp.GetCancelIrql());

    irpContext = (PMdIoCsqIrpContext)irp.GetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY);

    //
    // Decide if we have a PIO_CSQ_IRP_CONTEXT or an FxIrpQueue*
    //
    if (irpContext->Type == FX_IRP_QUEUE_ENTRY_IDENTIFIER) {
        p = (FxIrpQueue*)irpContext->Csq;
    } else {
        ASSERT(FALSE);
        p = (FxIrpQueue*)irpContext;
    }

    ASSERT(p);

    p->LockFromCancel(&irql);

    // Remove the IRP from the list
    p->RemoveIrpFromListEntry(&irp);

    //
    // Break the association if necessary.
    //

    if (irpContext != (PMdIoCsqIrpContext)p) {
        irpContext->Irp = NULL;

        irp.SetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY, NULL);
    }

    //
    // We are calling cancel-callback of the owning object with the lock
    // held so that it can successfully deliver the canceled request to the driver
    // if needed. If we don't hold the lock, we run into a race condition between
    // thread that's deleting the queue and this routine that's trying to deliver
    // a request to the queue being deleted. So the way it happens is that the dispose
    // call of queue waits for the request count to go zero. When we remove the
    // last Irp from the list above, we end up dropping the count to zero. This causes
    // the delete thread to run thru and destroy the FxIoQueue object. So to avoid that
    // after popping the request from the FxIrpQueue, we have to call into the FxIoQueue
    // with the lock and insert the request back into the FxIoQueue list so that delete
    // thread will wait until the request is delivered to the driver.
    //
    if( p->m_CancelCallback != NULL ) {
        p->m_CancelCallback(p, Irp, irpContext, irql);
    }
    else {

        p->UnlockFromCancel(irql);

        //
        // Dispose of the IRP ourselves
        //
        irp.SetStatus(STATUS_CANCELLED);
        irp.SetInformation(0);

        DoTraceLevelMessage(p->m_LockObject->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Irp 0x%p on Queue 0x%p Cancelled\n", Irp, p);

        //
        // Breakpoint for now. This usually means that someone
        // is going to leak some driver frameworks state...
        //
        FxVerifierDbgBreakPoint(p->m_LockObject->GetDriverGlobals());

        irp.CompleteRequest(IO_NO_INCREMENT);
    }
}

BOOLEAN
FxIrpQueue::IsIrpInQueue(
    __in PMdIoCsqIrpContext Context
    )
/*++

Routine Description:
    Enumerates the list to see if any of the IRPs there has
    a context that matches the input one.

--*/
{
    PLIST_ENTRY         nextEntry;
    FxIrp               nextIrp(NULL);
    PMdIoCsqIrpContext  pCsqContext;

    nextEntry = m_Queue.Flink;

    while( nextEntry != &m_Queue ) {
        nextIrp.SetIrp(FxIrp::GetIrpFromListEntry(nextEntry));

        pCsqContext = (PMdIoCsqIrpContext)nextIrp.GetContext(FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY);

        if( pCsqContext == Context ) {
            ASSERT(Context->Irp == nextIrp.GetIrp());
            return TRUE;
        }

        nextEntry = nextEntry->Flink;
    }

    return FALSE;
}


