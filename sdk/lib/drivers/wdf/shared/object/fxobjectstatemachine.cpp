/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxObjectStateMachine.cpp

Abstract:

    This module contains the implementation of the base object's state machine.

Author:





Environment:

    Both kernel and user mode

Revision History:








--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "FxObjectStateMachine.tmh"
#endif

}

VOID
FxObject::DeleteObject(
    VOID
    )
/*++

Routine Description:
    This is a public method that is called on an object to request that it Delete.

Arguments:
    None

Returns:
    NTSTATUS

--*/
{
    NTSTATUS status;
    KIRQL oldIrql;
    BOOLEAN result;

    m_SpinLock.Acquire(&oldIrql);

    result = MarkDeleteCalledLocked();

    // This method should only be called once per object
    ASSERT(result);
    UNREFERENCED_PARAMETER(result); //for fre build

    //
    // Perform the right action based on the objects current state
    //
    switch(m_ObjectState) {
    case FxObjectStateCreated:
        //
        // If we have a parent object, notify it of our deletion
        //
        if (m_ParentObject != NULL) {
            //
            // We call this holding our spinlock, the hierachy is child->parent
            // when the lock is held across calls
            //
            status = m_ParentObject->RemoveChildObjectInternal(this);

            if (status == STATUS_DELETE_PENDING) {

                //
                // We won the race to ourselves (still FxObjectStateCreated),
                // but lost the race on the parent who is going to try and
                // dispose us through the ParentDeleteEvent().
                //
                // This is OK since the state machine protects us from
                // doing improper actions, but we must not rundown and
                // release our reference count till the parent object
                // eventually calls our ParentDeleteEvent().
                //
                // So we note the state, and return waiting for the
                // parent to dispose us.
                //

                //
                // Wait for our parent to come in and dispose us through
                // the ParentDeleteEvent().
                //
                SetObjectStateLocked(FxObjectStateWaitingForEarlyDispose);
                m_SpinLock.Release(oldIrql);
                break;
            }
            else {
                //
                // We no longer have a parent object
                //
                m_ParentObject = NULL;
            }
        }

        //
        // Start Dispose, do delete state machine
        // returns with m_SpinLock released
        //
        DeleteWorkerAndUnlock(oldIrql, TRUE);
        break;

    case FxObjectStateDisposed:

        if (m_ParentObject != NULL) {
            status = m_ParentObject->RemoveChildObjectInternal(this);

            if (status == STATUS_DELETE_PENDING) {
                SetObjectStateLocked(FxObjectStateWaitingForParentDeleteAndDisposed);
                m_SpinLock.Release(oldIrql);
                break;
            }
            else {
                //
                // We no longer have a parent object
                //
                m_ParentObject = NULL;
            }
        }

        //
        // This will release the spinlock
        //
        DeletedAndDisposedWorkerLocked(oldIrql);
        break;

    case FxObjectStateDisposingDisposeChildren:
    case FxObjectStateWaitingForEarlyDispose:
    case FxObjectStateDeletedDisposing: // Do nothing, workitem will move into disposed and deleted
    case FxObjectStateDeletedAndDisposed:     // Do nothing, already deleted
        TraceDroppedEvent(FxObjectDroppedEventDeleteObject);
        m_SpinLock.Release(oldIrql);
        break;

    // These are bad states for this event
    case FxObjectStateInvalid:
    case FxObjectStateDestroyed:
    case FxObjectStateWaitingForParentDeleteAndDisposed:
    default:
        TraceDroppedEvent(FxObjectDroppedEventDeleteObject);
        // Bad state
        ASSERT(FALSE);
        m_SpinLock.Release(oldIrql);
    }
}

VOID
FxObject::DeleteEarlyDisposedObject(
    VOID
    )
/*++

Routine Description:
    Deletes an object which has already been explicitly early disposed.

Arguments:
    None

Return Value:
    None

  --*/
{
    BOOLEAN result;

    ASSERT(m_ObjectFlags & FXOBJECT_FLAGS_EARLY_DISPOSED_EXT);
    ASSERT(m_ObjectState == FxObjectStateDisposed);

    result = MarkDeleteCalledLocked();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result); //for fre build

    if (m_ParentObject != NULL) {
        NTSTATUS status;
        KIRQL irql;

        m_SpinLock.Acquire(&irql);

        if (m_ParentObject != NULL) {
            status = m_ParentObject->RemoveChildObjectInternal(this);

            if (status == STATUS_DELETE_PENDING) {
                SetObjectStateLocked(FxObjectStateWaitingForParentDeleteAndDisposed);
                m_SpinLock.Release(irql);
                return;
            }
            else {
                //
                // We no longer have a parent object
                //
                m_ParentObject = NULL;
            }
        }

        m_SpinLock.Release(irql);
    }

    //
    // This will release the spinlock
    //
    DeletedAndDisposedWorkerLocked(PASSIVE_LEVEL, FALSE);
}

BOOLEAN
FxObject::Dispose(
    VOID
    )
/*++

Routine Description:
    This is a virtual function overriden by sub-classes if they want
    Dispose notifications.

Arguments:
    None

Returns:
    TRUE if the registered cleanup routines on this object should be called
         when this funciton returns

--*/
{
    return TRUE;
}

VOID
FxObject::ProcessDestroy(
    VOID
    )
{
    FxTagTracker* pTagTracker;

    //
    // Set the debug info to NULL so that we don't use it after the
    // SelfDestruct call.  Setting the DestroyFunction to NULL
    // will also prevent reuse of the REF_OBJ after it has been destroyed.
    //
    pTagTracker = GetTagTracker();

    //
    // We will free debug info later.  It useful to hang on to the debug
    // info after the destructor has been called for debugging purposes.
    //
    if (pTagTracker != NULL) {
        pTagTracker->CheckForAbandondedTags();
    }

    //
    // Call the destroy callback *before* any desctructor is called.  This
    // way the callback has access to a full fledged object that hasn't been
    // cleaned up yet.
    //
    // We only do this for committed objects.  A non committed object will
    // *NOT* have additional contexts to free.
    //
    if (m_ObjectSize > 0 && IsCommitted()) {
        FxContextHeader* pHeader, *pNext;
        WDFOBJECT h;
        BOOLEAN first;

        //
        // We are getting the object handle when the ref count is zero.  We
        // don't want to ASSERT in this case.
        //
        h = GetObjectHandleUnchecked();













        for (pHeader = GetContextHeader();
             pHeader != NULL;
             pHeader = pHeader->NextHeader) {

            //
            // Cleanup *may* have been called earlier in the objects
            // destruction, and in this case the EvtCleanupCallback will
            // be set to NULL. Ensuring its always called provides
            // symmetry to the framework.
            //

            //
            // No need to interlockexchange out the value of
            // EvtCleanupCallback because any codepath that might be calling
            // CallCleanup must have a valid reference and no longer have
            // any outstanding references
            //
            if (pHeader->EvtCleanupCallback != NULL) {
                pHeader->EvtCleanupCallback(h);
                pHeader->EvtCleanupCallback = NULL;
            }

            if (pHeader->EvtDestroyCallback != NULL) {
                pHeader->EvtDestroyCallback(h);
                pHeader->EvtDestroyCallback = NULL;
            }
        }

        first = TRUE;
        for (pHeader = GetContextHeader(); pHeader != NULL; pHeader = pNext) {

            pNext = pHeader->NextHeader;

            //
            // The first header is embedded, so it will be freed with the
            // object
            //
            if (first == FALSE) {
                FxPoolFree(pHeader);
            }

            first = FALSE;
        }
    }










    //
    // NOTE:  The delete of the tag tracker *MUST* occur before the SelfDestruct()
    // of this object.  The tag tracker has a back pointer to this object which
    // it dereferences in its own destructor.  If SelfDestruct() is called
    // first, then ~FxTagTracker will touch freed pool and bugcheck.
    //
    if (pTagTracker != NULL) {
        GetDebugExtension()->TagTracker = NULL;
        delete pTagTracker;
    }

    //
    // See NOTE above.
    //
    SelfDestruct();
}

BOOLEAN
FxObject::EarlyDispose(
    VOID
    )
/*++

Routine Description:
    Public early dipose functionality.  Removes the object from the parent's
    list of children.  This assumes the caller or someone else will eventually
    invoke DeleteObject() on this object.

Arguments:
    None

Return Value:
    BOOLEAN - same semantic as DisposeChildrenWorker.
        TRUE - dispose of this object and its children occurred synchronously in
               this call
        FALSE - the dispose was pended to a work item

  --*/
{
    NTSTATUS status;
    KIRQL oldIrql;
    BOOLEAN result;

    //
    // By default, we assume a synchronous diposal
    //
    result = TRUE;

    m_SpinLock.Acquire(&oldIrql);

    switch(m_ObjectState) {
    case FxObjectStateCreated:
        //
        // If we have a parent object, notify it of our deletion
        //
        if (m_ParentObject != NULL) {
            //
            // We call this holding our spinlock, the hierachy is child->parent
            // when the lock is held across calls
            //
            status = m_ParentObject->RemoveChildObjectInternal(this);

            if (status == STATUS_DELETE_PENDING) {

                //
                // We won the race to ourselves (still FxObjectStateCreated),
                // but lost the race on the parent who is going to try and
                // dispose us through the PerformEarlyDipose().
                //
                // This is OK since the state machine protects us from
                // doing improper actions, but we must not rundown and
                // release our reference count till the parent object
                // eventually calls our ParentDeleteEvent().
                //
                // So we note the state, and return waiting for the
                // parent to dispose us.
                //

                //
                // Wait for our parent to come in and dispose us through
                // the PerformEarlyDipose().
                //
                SetObjectStateLocked(FxObjectStateWaitingForEarlyDispose);
                m_SpinLock.Release(oldIrql);

                return FALSE;
            }
            else {
                //
                // We no longer have a parent object
                //
                m_ParentObject = NULL;
            }
        }

        //
        // Mark that this object was early disposed externally wrt the
        // state machine.
        //
        m_ObjectFlags |= FXOBJECT_FLAGS_EARLY_DISPOSED_EXT;

        //
        // Start the dispose path.  This call will release the spinlock.
        //
        result = PerformEarlyDisposeWorkerAndUnlock(oldIrql, TRUE);
        break;

    default:
        //
        // Not in the right state for an early dispose
        //
        result = FALSE;
        m_SpinLock.Release(oldIrql);
    }

    return result;
}

BOOLEAN
FxObject::PerformEarlyDispose(
    VOID
    )
/*++

Routine Description:
    Allows Dispose() processing on an object to occur before calling DeleteObject().

Arguments:
    CanDefer - if TRUE, can defer to a dispose list if IRQL requirements are
               incorrect.  If FALSE, the caller has guaranteed that we are at
               the correct IRQL

Returns:
    None

--*/
{
    KIRQL oldIrql;
    BOOLEAN result;

    //
    // By default we assume that the dispose was synchronous
    //
    result = TRUE;


    //
    // It's OK for an object to already be disposing due to
    // a parent delete call.
    //
    // To check for verifier errors in which two calls to
    // PerformEarlyDispose() occur, a separate flag is used
    // rather than complicating the state machine.
    //
    m_SpinLock.Acquire(&oldIrql);

    //
    // Perform the right action based on the objects current state
    //
    switch(m_ObjectState) {
    case FxObjectStateCreated:
        //
        // Start dispose, move into Disposing state
        // returns with m_SpinLock released
        //
        result = PerformEarlyDisposeWorkerAndUnlock(oldIrql, FALSE);
        break;

    case FxObjectStateWaitingForEarlyDispose:
        //
        // Start the dispose path.
        //
        result = PerformEarlyDisposeWorkerAndUnlock(oldIrql, FALSE);
        break;

    case FxObjectStateDeferedDisposing:
        //
        // We should only get an early dispose in this state once we have thunked
        // to passive level via the dispose list.
        //
        result = PerformDisposingDisposeChildrenLocked(oldIrql, FALSE);
        break;

    case FxObjectStateWaitingForParentDeleteAndDisposed: // Do nothing, parent object will delete and dispose
    case FxObjectStateDisposed:               // Do nothing
    case FxObjectStateDisposingEarly:         // Do nothing
    case FxObjectStateDeletedDisposing:       // Do nothing, workitem will moved into disposed
    case FxObjectStateDeletedAndDisposed:
        TraceDroppedEvent(FxObjectDroppedEventPerformEarlyDispose);
        m_SpinLock.Release(oldIrql);
        break;

    // These are bad states for this event
    case FxObjectStateInvalid:
    case FxObjectStateDestroyed:

    default:
        TraceDroppedEvent(FxObjectDroppedEventPerformEarlyDispose);
        // Bad state
        ASSERT(FALSE);
        m_SpinLock.Release(oldIrql);
        break;
    }

    return result;
}

_Must_inspect_result_
NTSTATUS
FxObject::RemoveParentAssignment(
    VOID
    )
/*++

Routine Description:
    Remove the current objects ParentObject.

Arguments:
    None

Returns:
    NTSTATUS of action

--*/
{
    KIRQL oldIrql;
    NTSTATUS status;

    m_SpinLock.Acquire(&oldIrql);

    //
    // Object is already being deleted, this object will be removed as a child
    // by the parents Dispose()
    //
    if (m_ObjectState != FxObjectStateCreated) {
        TraceDroppedEvent(FxObjectDroppedEventRemoveParentAssignment);
        m_SpinLock.Release(oldIrql);
        return STATUS_DELETE_PENDING;
    }

    // We should have a parent
    ASSERT(m_ParentObject != NULL);

    status = m_ParentObject->RemoveChildObjectInternal(this);
    if (NT_SUCCESS(status)) {
        m_ParentObject = NULL;
    }

    m_SpinLock.Release(oldIrql);

    return status;
}

VOID
FxObject::ParentDeleteEvent(
    VOID
    )
/*++

Routine Description:

    This is invoked by the parent object when it is Dispose()'ing us.

Arguments:
    None

Returns:
    None

--*/
{
    KIRQL oldIrql;

    //
    // Note: It's ok for an object to already be in the delete
    //       state since there can be an allowed race between
    //       parent disposing an object, and the DeleteObject()
    //       call on the object itself.
    //
    m_SpinLock.Acquire(&oldIrql);

    //
    // We no longer have a parent object
    //
    m_ParentObject = NULL;

    //
    // Perform the right action based on the objects current state
    //
    switch(m_ObjectState) {
    case FxObjectStateWaitingForParentDeleteAndDisposed:
    case FxObjectStateDisposed:
        //
        // This will release the spinlock
        //
        DeletedAndDisposedWorkerLocked(oldIrql, TRUE);
        break;

    case FxObjectStateDeletedDisposing:
        //
        // Do nothing, workitem will move into disposed
        //
        TraceDroppedEvent(FxObjectDroppedEventParentDeleteEvent);
        m_SpinLock.Release(oldIrql);
        break;

    case FxObjectStateDeletedAndDisposed:     // Do nothing, already deleted
        m_SpinLock.Release(oldIrql);
        break;

    case FxObjectStateDisposingDisposeChildren:
        //
        // In the process of deleting, ignore it
        //
        m_SpinLock.Release(oldIrql);
        break;

    // These are bad states for this event





    case FxObjectStateCreated:
    case FxObjectStateWaitingForEarlyDispose:

    case FxObjectStateInvalid:
    case FxObjectStateDestroyed:
    default:
        //
        // Bad state
        //
        ASSERT(FALSE);
        m_SpinLock.Release(oldIrql);
        break;
    }
}

VOID
FxObject::DeferredDisposeWorkItem(
    VOID
    )
/*++

Routine Description:
    Invoked by deferred dispose workitem.   This is invoked at PASSIVE_LEVEL,
    and returns at PASSIVE_LEVEL

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL oldIrql;
    BOOLEAN result, destroy;

    destroy = FALSE;

    m_SpinLock.Acquire(&oldIrql);

    ASSERT(oldIrql == PASSIVE_LEVEL);

    //
    // Perform the right action based on the objects current state
    //
    // DisposeChildrenWorker return result can be ignored because we are
    // guaranteed to be calling it at PASSIVE.
    //
    switch (m_ObjectState) {
    case FxObjectStateDeferedDisposing:
        //
        // This will drop the spinlock and move the object to the right state
        // before returning.
        //
        result = PerformDisposingDisposeChildrenLocked(oldIrql, FALSE);

        //
        // The substree should never defer to the dispose list if we pass FALSE.
        //
        ASSERT(result);
        UNREFERENCED_PARAMETER(result); //for fre build

        return;

    case FxObjectStateDeferedDeleting:
        SetObjectStateLocked(FxObjectStateDeletedDisposing);
        result = DisposeChildrenWorker(FxObjectStateDeferedDeleting, oldIrql, FALSE);
        ASSERT(result);
        UNREFERENCED_PARAMETER(result); //for fre build

        //
        // This will release the spinlock
        //
        DeletedAndDisposedWorkerLocked(oldIrql, FALSE);
        return;

    case FxObjectStateDeferedDestroy:
        // Perform final destroy actions now that we are at passive level
        destroy = TRUE;
        break;

    // These are bad states for this event
    case FxObjectStateDeletedAndDisposed:     // Do nothing
    case FxObjectStateDisposed:
    case FxObjectStateWaitingForParentDeleteAndDisposed: // Do nothing
    case FxObjectStateCreated:
    case FxObjectStateWaitingForEarlyDispose:
    case FxObjectStateInvalid:
    case FxObjectStateDestroyed:

    default:
        // Bad state
        ASSERT(FALSE);
        break;
    }

    m_SpinLock.Release(oldIrql);

    if (destroy) {
        ProcessDestroy();
    }
}

_Releases_lock_(this->m_SpinLock.m_Lock)
__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxObject::PerformDisposingDisposeChildrenLocked(
    __in __drv_restoresIRQL KIRQL OldIrql,
    __in                    BOOLEAN CanDefer
    )
/*++

Routine Description:
    This is entered with m_SpinLock held, and returns with it released.

Arguments:
    OldIrql - the IRQL before m_SpinLock was acquired

    CanDefer - if TRUE, can defer to a dispose list if IRQL requirements are
               incorrect.  If FALSE, the caller has guaranteed that we are at
               the correct IRQL

Return Value:
    BOOLEAN - same semantic as DisposeChildrenWorker.
        TRUE - delete of this object and its children occurred synchronously in
               this call
        FALSE - the delete was pended to a work item

  --*/
{
    static const USHORT edFlags = (FXOBJECT_FLAGS_DELETECALLED |
                                   FXOBJECT_FLAGS_EARLY_DISPOSED_EXT);

    SetObjectStateLocked(FxObjectStateDisposingDisposeChildren);

    if (DisposeChildrenWorker(FxObjectStateDeferedDisposing, OldIrql, CanDefer)) {
        //
        // Upon returning TRUE, the lock is still held
        //

        //
        // If this object was early disposed externally, destroy the children
        // immediately (FxRequest relies on this so that the WDFMEMORYs created
        // for probed and locked buffers are freed before the request is
        // completed.)
        //
        // Otherwise, wait for DeleteObject or ParentDeleteEvent() to occur.
        //
        if ((m_ObjectFlags & edFlags) == edFlags) {
            //
            // This will drop the lock
            //
            DeletedAndDisposedWorkerLocked(OldIrql, FALSE);
        }
        else {
            //
            // Will wait for the parent deleted event
            //
            SetObjectStateLocked(FxObjectStateDisposed);
        }

        return TRUE;
    }
    else {
        //
        // Upon return FALSE, the lock was released and a work item was
        // queued to dispose of children at passive level
        //
        DO_NOTHING();

        return FALSE;
    }
}

_Releases_lock_(this->m_SpinLock.m_Lock)
__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxObject::PerformEarlyDisposeWorkerAndUnlock(
    __in __drv_restoresIRQL KIRQL OldIrql,
    __in                    BOOLEAN CanDefer
    )
/*++

Routine Description:
    This is entered with m_SpinLock held, and returns with it released.

Arguments:
    OldIrql - the previous IRQL before the object lock was acquired

    CanDefer - if TRUE, can defer to a dispose list if IRQL requirements are
               incorrect.  If FALSE, the caller has guaranteed that we are at
               the correct IRQL

Return Value:
    BOOLEAN - same semantic as DisposeChildrenWorker.
        TRUE - delete of this object and its children occurred synchronously in
               this call
        FALSE - the delete was pended to a work item

  --*/
{
    ASSERT(m_ObjectState == FxObjectStateCreated ||
           m_ObjectState == FxObjectStateWaitingForEarlyDispose);

    SetObjectStateLocked(FxObjectStateDisposingEarly);

    if (CanDefer && ShouldDeferDisposeLocked(&OldIrql))  {
        QueueDeferredDisposeLocked(FxObjectStateDeferedDisposing);
        m_SpinLock.Release(OldIrql);

        return FALSE;
    }
    else {
        return PerformDisposingDisposeChildrenLocked(OldIrql, CanDefer);
    }
}

_Releases_lock_(this->m_SpinLock.m_Lock)
__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxObject::DeleteWorkerAndUnlock(
    __in __drv_restoresIRQL KIRQL OldIrql,
    __in                    BOOLEAN CanDefer
    )
/*++

Routine Description:
    This is entered with m_SpinLock held, and returns with it released.


Arguments:
    OldIrql - the IRQL before m_SpinLock was acquired

    CanDefer - if TRUE, can defer to a dispose list if IRQL requirements are
               incorrect.  If FALSE, the caller has guaranteed that we are at
               the correct IRQL

Return Value:
    BOOLEAN - same semantic as DisposeChildrenWorker.
        TRUE - delete of this object and its children occurred synchronously in
               this call
        FALSE - the delete was pended to a work item

  --*/
{
    ASSERT(m_ObjectState == FxObjectStateCreated);
           // m_ObjectState == FxObjectStateWaitingForParentDelete);

    if (CanDefer && ShouldDeferDisposeLocked(&OldIrql)) {
        QueueDeferredDisposeLocked(FxObjectStateDeferedDeleting);
        m_SpinLock.Release(OldIrql);

        return FALSE;
    }
    else {
        SetObjectStateLocked(FxObjectStateDeletedDisposing);

        if (DisposeChildrenWorker(FxObjectStateDeferedDeleting, OldIrql, CanDefer)) {
            //
            // This will release the spinlock
            //
            DeletedAndDisposedWorkerLocked(OldIrql, FALSE);

            return TRUE;
        }
        else {
            //
            // Upon return FALSE, the lock was released and a work item was
            // queued to dispose of children at passive level
            //
            DO_NOTHING();

            return FALSE;
        }
    }
}

VOID
FxObject::QueueDeferredDisposeLocked(
    __in FxObjectState NewDeferedState
    )
/*++

Routine Description:
    Queues this object onto a work item list which will dispose it at passive
    level.  The work item will be owned by the parent device or driver.

    This is called with the object's m_SpinLock held.

    NOTE:  This function only looks at this object and the parent to attempt to
           find the owning FxDeviceBase*.  If this is a deeper hierarchy, the deeply
           rooted FxDeviceBase will not be used.

Arguments:
    Parent - the parent of this objec (it may have already been removed from
             m_ParentObject, so we can't use that field

Return Value:
    None

  --*/
{
    //
    // Queue workitem, which will run DisposeChildrenWorker()
    //
    ASSERT(m_Globals != NULL);
    ASSERT(m_Globals->Driver != NULL);

    SetObjectStateLocked(NewDeferedState);

    //FxToObjectItf::FxAddToDisposeList(m_DeviceBase, m_Globals, this);
    if (m_DeviceBase != NULL) {
        m_DeviceBase->AddToDisposeList(this);
    }
    else {
        m_Globals->Driver->GetDisposeList()->Add(this);
    }
}

_Releases_lock_(this->m_SpinLock.m_Lock)
__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
FxObject::DisposeChildrenWorker(
    __in                    FxObjectState NewDeferedState,
    __in __drv_restoresIRQL KIRQL OldIrql,
    __in                    BOOLEAN CanDefer
    )

/*++

Routine Description:

    Rundown list of child objects removing their entries and invoking
    their ParentDeleteEvent() on them.

    This is called with the m_SpinLock held and upon returning the lock is
    released.

Arguments:
    NewDeferedState - If the state transition requires defering to a dispose
                      list, this is the new state to move to

    OldIrql - the previous IRQL when the caller acquired the object's lock

    CanDefer - if TRUE, can defer to a dispose list if IRQL requirements are
               incorrect.  If FALSE, the caller has guaranteed that we are at
               the correct IRQL

Returns:
    TRUE:   Dispose is completed in this function.
    FALSE:  Dispose is deferred either to a workitem (if CanDefer is TRUE) or will
            be done later in the current thread (if CanDefer is FALSE).

    In either case lock is released.

    If the OldIrql == PASSIVE_LEVEL, TRUE is guaranteed to be returned

Comments:

    This routine is entered with the spinlock held, and may return with it released.

    The state machine ensures that this is only invoked once in
    an objects lifetime, regardless of races between PerformEarlyDispose,
    DeleteObject, or a ParentDeleteEvent.  If there are requirements for passive
    level dispose and the previous IRQL is != PASSIVE, this function will be
    called twice, the first at IRQL > PASSIVE, the second at PASSIVE.

    Top level code has ensured this is invoked under the right IRQL level
    for the object to perform the Dispose() callbacks.

--*/
{
    LIST_ENTRY *ple;
    FxObject*   childObject;

    //
    // Called from:
    //
    // DeferredDisposeWorkItem (will complete, and release in current thread)
    // PerformDisposeWorkerAndUnlock(PerformEarlyDispose), no release, but possible thread deferral
    // DeleteWorkerAndUnlock (will release, but may defer, its logic must change!)
    //

    ASSERT((m_ObjectState == FxObjectStateDisposingDisposeChildren) ||
           (m_ObjectState == FxObjectStateDeletedDisposing));

    //
    // This routine will attempt to dispose as many children as possible
    // in the current thread. It may have to stop if the thread is
    // not at PASSIVE_LEVEL when the object spinlock was acquired, and
    // a child object is marked as a passive level object.
    //
    // If this occurs, dispose processing is suspended and resumed in
    // a passive level workitem, which calls this routine back to
    // complete the processing.
    //
    // Once all child object's Dispose() callback has returned, we then
    // can call Dispose() on the parent object.
    //
    // This must be done in this order to guarantee the contract with the
    // device driver (and internal object system) that all child
    // EvtObjectCleanup callbacks have returned before their parents
    // EvtObjectCleanup event.
    //
    // This is important to avoid extra references
    // when child objects expect their parent object to be valid until
    // EvtObjectCleanup is called.
    //

    // Rundown list removing entries and calling Dispose on child objects

    //
    // If this object requires being forced onto the dispose thread, do it now
    //
    if (IsForceDisposeThreadLocked() && OldIrql != PASSIVE_LEVEL) {
        //
        // Workitem will re-run this function at PASSIVE_LEVEL
        //
        if (CanDefer) {
            QueueDeferredDisposeLocked(NewDeferedState);
        }
        else {
            SetObjectStateLocked(NewDeferedState);
        }

        m_SpinLock.Release(OldIrql);

        return FALSE;
    }

    for (ple = m_ChildListHead.Flink;
         ple != &m_ChildListHead;
         ple = ple->Flink) {
        //
        // Before removing the child object, we need to see if we need to defer
        // to a work item to dispose the child.
        //
        childObject = CONTAINING_RECORD(ple, FxObject, m_ChildEntry);

        //
        // Should not associate with self
        //
        ASSERT(childObject != this);

        //
        // If current threads IRQL before acquiring the spinlock is not
        // passive, and the child object is passive constrained, we must
        // defer the current disposal processing to a workitem.
        //
        // We stay in the Disposing state, which this routine will continue
        // processing when called back from the workitem.
        //
        // This code is re-entered at the proper passive_level to complete
        // processing where it left off (at the head of the m_ChildListHead).
        //
        if (OldIrql != PASSIVE_LEVEL && childObject->IsPassiveDisposeLocked()) {
            //
            // Workitem will re-run this function at PASSIVE_LEVEL
            //
            if (CanDefer) {
                QueueDeferredDisposeLocked(NewDeferedState);
            }
            else {
                SetObjectStateLocked(NewDeferedState);
            }
            m_SpinLock.Release(OldIrql);

            return FALSE;
        }
    }

    m_SpinLock.Release(OldIrql);

    for (ple = m_ChildListHead.Flink; ple != &m_ChildListHead; ple = ple->Flink) {
        childObject = CONTAINING_RECORD(ple, FxObject, m_ChildEntry);

        //
        // Inform child object of disposal.   We will release the reference on
        // the child only after we have disposed ourself.
        //
        if (childObject->PerformEarlyDispose() == FALSE) {

            m_SpinLock.Acquire(&OldIrql);
            if (CanDefer) {
                QueueDeferredDisposeLocked(NewDeferedState);
            }
            else {
                SetObjectStateLocked(NewDeferedState);
            }
            m_SpinLock.Release(OldIrql);

            return FALSE;
        }

        ASSERT(childObject->GetRefCnt() > 0);
    }

    //
    // Call Dispose virtual callback on ourselves for benefit
    // of sub-classes if it is overridden.
    //
    if ((m_ObjectFlags & FXOBJECT_FLAGS_DISPOSE_OVERRIDE) == 0x00 || Dispose()) {
        //
        // Now call Cleanup on any handle context's exposed
        // to the device driver.
        //
        CallCleanup();
    }

    return TRUE;
}

//
// Despite the name this function may not always be called with lock held
// but if Unlock is TRUE, lock must be held.
//
_When_(Unlock, _Releases_lock_(this->m_SpinLock.m_Lock))
__drv_when(Unlock, __drv_requiresIRQL(DISPATCH_LEVEL))
VOID
FxObject::DeletedAndDisposedWorkerLocked(
    __in __drv_when(Unlock, __drv_restoresIRQL) KIRQL OldIrql,
    __in                                        BOOLEAN Unlock
    )
{
    SetObjectStateLocked(FxObjectStateDeletedAndDisposed);

    if (Unlock) {
        m_SpinLock.Release(OldIrql);
    }

    DestroyChildren();

    //
    // Release the final reference on the object
    //
    RELEASE(NULL);
}

