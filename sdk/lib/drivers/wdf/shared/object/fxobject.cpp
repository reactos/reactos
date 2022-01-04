/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxObject.cpp

Abstract:

    This module contains the implementation of the base object

Author:





Environment:

    Both kernel and user mode

Revision History:










--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "FxObject.tmh"
#else
ULONG DebugLevel = TRACE_LEVEL_INFORMATION;
ULONG DebugFlag = 0xff;
#endif

}

FxObject::FxObject(
    __in WDFTYPE Type,
    __in USHORT Size,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    m_Type(Type),
    m_ObjectSize((USHORT) WDF_ALIGN_SIZE_UP(Size, MEMORY_ALLOCATION_ALIGNMENT)),
    m_Globals(FxDriverGlobals)
#if FX_CORE_MODE==FX_CORE_USER_MODE
#ifndef INLINE_WRAPPER_ALLOCATION
    ,m_COMWrapper(NULL)
#endif
#endif
{
    ASSERT((((ULONG_PTR) this) & FxHandleFlagMask) == 0x0);

    Construct(FALSE);
}

FxObject::FxObject(
    __in WDFTYPE Type,
    __in USHORT Size,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxObjectType ObjectType
    ) :
    m_Type(Type),
    m_ObjectSize((USHORT) WDF_ALIGN_SIZE_UP(Size, MEMORY_ALLOCATION_ALIGNMENT)),
    m_Globals(FxDriverGlobals)
{
     if (ObjectType != FxObjectTypeEmbedded) {
         //
         // only for non embedded objects
         //
         ASSERT((((ULONG_PTR) this) & FxHandleFlagMask) == 0x0);
     }

    Construct(ObjectType == FxObjectTypeEmbedded ? TRUE : FALSE);
}

FxObject::~FxObject()
{
    FxTagTracker *pTagTracker;







    pTagTracker = GetTagTracker();

    if (pTagTracker != NULL) {
        delete pTagTracker;
    }

    ASSERT(m_DisposeSingleEntry.Next == NULL);

    //
    // We need to ensure there are no leaked child objects, or
    // parent associations.
    //
    // This can occur if someone calls the C++ operator delete,
    // or Release() to destroy the object.
    //
    // This is generally invalid in the framework, though certain
    // objects may understand the underlying contract, but must
    // make sure there are no left over un-Disposed associations.
    //
    // Embedded FxObject's also don't have delete called on them,
    // and have to manually manage any child associations when
    // their parent object disposes by calling PerformEarlyDispose.
    //
    // They don't have an associated lifetime parent since they
    // are embedded.
    //
    if (m_ParentObject != NULL ||
        !IsListEmpty(&m_ChildListHead) || !IsListEmpty(&m_ChildEntry)) {
        PCSTR pHandleName;

        pHandleName = FxObjectTypeToHandleName(m_Type);
        if (pHandleName == NULL) {
            pHandleName = "WDFOBJECT";
        }

        ASSERTMSG(
            "Object was freed using WdfObjectDereference, not WdfObjectDelete\n",
            m_ParentObject == NULL &&
            IsListEmpty(&m_ChildEntry) &&
            IsListEmpty(&m_ChildListHead)
            );

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_FATAL, TRACINGOBJECT,
            "Handle %s %p (raw object %p) was freed using "
            "WdfObjectDereference(), not WdfObjectDelete()",
            pHandleName, GetObjectHandleUnchecked(), this);

        FxVerifierBugCheck(GetDriverGlobals(),
                           WDF_OBJECT_ERROR,
                           (ULONG_PTR) GetObjectHandleUnchecked(),
                           (ULONG_PTR) this);
    }

    //
    // This is called when the reference count goes to zero
    //
    SetObjectStateLocked(FxObjectStateDestroyed);
}


VOID
FX_VF_METHOD(FxObject, VerifyConstruct) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ BOOLEAN Embedded
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    PAGED_CODE_LOCKED();
#endif

    ASSERTMSG(
        "this object's type is not listed in FxObjectsInfo\n",
        FxVerifyObjectTypeInTable(m_Type));

    //
    // If this is an embedded object, there is no possibility of having
    // a debug extension, so do not set FXOBJECT_FLAGS_HAS_DEBUG *ever*
    // in this case.
    //
    if (m_Globals->IsObjectDebugOn() && Embedded == FALSE) {
        FxObjectDebugExtension* pExtension;

        m_ObjectFlags |= FXOBJECT_FLAGS_HAS_DEBUG;

        pExtension = GetDebugExtension();
        ASSERT(pExtension->Signature == FxObjectDebugExtensionSignature);

        //
        // Assume that zero is an invalid state.
        //
        WDFCASSERT(FxObjectStateInvalid == 0x0);
        RtlZeroMemory(&pExtension->StateHistory[0],
                      ARRAY_SIZE(pExtension->StateHistory));
        pExtension->StateHistoryIndex = 0;

        //
        // Setup the first slot to our new state.
        //
        pExtension->StateHistory[0] = FxObjectStateCreated;

        AllocateTagTracker(m_Type);
    }
}


VOID
FxObject::FinalRelease(
    VOID
    )
{












    //
    // No other access, OK to test flag without grabbing spinlock
    // since it can only be set at create.
    //
    if (ShouldDeferDisposeLocked()) {
        //
        // If this is a passive level only object, ensure we only destroy
        // it from passive level.  No need to hold the object lock when
        // changing to this state since no other thread can change our
        // state.
        //
        SetObjectStateLocked(FxObjectStateDeferedDestroy);

        //
        // Note, we cannot be holding a lock while making this call b/c by
        // the time it returns, it could have freed the object and the
        // KeReleaseSpinLock call would have touched freed pool.
        //

        //FxToObjectItf::FxAddToDriverDisposeList(m_Globals, this);


        m_Globals->Driver->GetDisposeList()->Add(this);

    }
    else {
        ProcessDestroy();
    }
}

_Must_inspect_result_
NTSTATUS
FxObject::QueryInterface(
    __in FxQueryInterfaceParams* Params
    )
{
    NTSTATUS status;

    if (Params->Type == FX_TYPE_OBJECT) {
        *Params->Object = this;
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_NOINTERFACE;
    }

    return status;
}

VOID
FxObject::AllocateTagTracker(
    __in WDFTYPE Type
    )
{
    ASSERT(IsDebug());

    if (m_Globals->DebugExtension != NULL &&
        m_Globals->DebugExtension->ObjectDebugInfo != NULL &&
        FxVerifierGetTrackReferences(
            m_Globals->DebugExtension->ObjectDebugInfo,
            Type)) {
        //
        // Failure to CreateAndInitialize a tag tracker is no big deal, we just
        // don't track references.
        //

        (void) FxTagTracker::CreateAndInitialize(
            &GetDebugExtension()->TagTracker,
            m_Globals,
            FxTagTrackerTypeHandle,
            FALSE,
            this
            );

        //
        // For now we overload the requirement of a tag tracker as also tracing
        // state changes.
        //
        m_ObjectFlags |= FXOBJECT_FLAGS_TRACE_STATE;
    }
}

VOID
FxObject::operator delete(
    __in PVOID Memory
    )
{
    ASSERT(Memory != NULL);













    FxPoolFree(_GetBase((FxObject*) Memory));
}

VOID
FxObject::CallCleanupCallbacks(
    VOID
    )
{
    FxContextHeader* pHeader;
    WDFOBJECT h;

    //
    // Deleted before Commit or it is an internal object
    //
    if (IsCommitted() == FALSE) {
        return;
    }

    //
    // We only should have an object handle when we have an external object
    //
    h = GetObjectHandle();

    for (pHeader = GetContextHeader();
         pHeader != NULL;
         pHeader = pHeader->NextHeader) {
        if (pHeader->EvtCleanupCallback != NULL) {
            pHeader->EvtCleanupCallback(h);
            pHeader->EvtCleanupCallback = NULL;
        }
    }

    m_ObjectFlags &= ~FXOBJECT_FLAGS_HAS_CLEANUP;
}

VOID
FxObject::ClearEvtCallbacks(
    VOID
    )
/*++

Routine Description:

    Clears out any assigned callbacks on the object.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxContextHeader *pHeader;

    for (pHeader = GetContextHeader();
         pHeader != NULL;
         pHeader = pHeader->NextHeader) {

        pHeader->EvtDestroyCallback = NULL;
        pHeader->EvtCleanupCallback = NULL;
    }

    m_ObjectFlags &= ~FXOBJECT_FLAGS_HAS_CLEANUP;
}

VOID
FxObject::DeleteFromFailedCreate(
    VOID
    )
/*++

Routine Description:
    Clears out any assigned callbacks on the object and then deletes it.  Clearing
    out the callbacks are necessary so that the driver's callbacks are not called
    on a buffer that they didn't initialize.

Arguments:
    None

Return Value:
    None

  --*/
{
    ClearEvtCallbacks();

    //
    // After this call returns "this" is destroyed is not a valid pointer!
    //
    DeleteObject();

    //
    // "this" is now freed memory, do not touch it!
    //
}

//
// FxObject Parent/Child Rules:
//
// The FxObject state machine protects state transitions when
// objects are being associated with each other, and races
// that can occur when a child and parent object are being
// deleted at the same time. This provides the backbone of
// assurance on the objects state.
//
// While transiting object states, the following must be taken
// into consideration:
//
// Reference counts:
//
//  When an object is created with operator new(), it has a reference count
//  of one.
//
//  When a DeleteObject is done on it, the reference count is decremented
//  after delete processing is done (which can include disposing children),
//  and this results in the objects destruction.
//
//  When an object is created by operator new() and immediately associated
//  with a parent object, its reference count remains one.
//
//  When either the DeleteObject() method is invoked, or the parent object
//  disposes the child object with ParentDeleteEvent(), eventually the
//  object will dereference itself after delete processing is done only
//  once.
//
//  Even in the face of race conditions, the object state machine ensures that
//  only one set of the proper delete conditions occur (dispose children, invoke
//  driver and class dispose callbacks, dereference object).
//
//  An object is responsible for releasing its own reference count on its
//  self when it goes into the FxObjectStateDeletedAndDisposed.
//
//  This has been *carefully* designed so that only the original object
//  references are required for this automatic lifetime case to avoid
//  extra interlocked operations in AddRef and Release frequently. These
//  extra interlocked operations can have a big performance impact on
//  the WDF main I/O paths, in which object relationship's are being used.
//
//  A simpler implementation may try and have the parent add a reference
//  to the child, and the child add a reference to the parent, but this
//  is a tightly coupled implementation controlled by the object state
//  machine. A circular reference pattern is OK for objects that
//  do not have a formal designed in relationship that can be expressed
//  in the complex tear down contract implemented in the state machine.
//
// SpinLocks:
//
// The m_SpinLock field protects each indivdual objects m_ObjectState
// variable, the list of child objects that it has, and the m_ParentObject
// field.
//
// In addition, if any object associates itself with a parent object,
// it effectively "lends" its m_ChildEntry field to the parent object,
// and these are manipulated and protected by the parent objects m_SpinLock.
//
// Lock Order:
//
// Currently the lock order is Child -> Parent, meaning a child
// can call the AddChildObjectInternal, RemoveChildObjectInternal, methods with
// the child lock held.
//
// The parent object will not invoke any child object ParentDeleteEvent()
// while holding the parents m_SpinLock.
//
// This order allows potential races between child DeleteObject and parent
// Dispose to be resolved without extra reference counts and multiple
// acquires and releases of the spinlocks in the normal cases.
//
//
// AddChildObjectInternal/RemoveChildObjectInternal:
//
// When a child object is added to the parent, the parent can delete
// the child object at any time when the parent object is deleted
// or disposed.
//
// If a call to RemoveChildObjectInternal is made, the caller may not "win"
// the inherent race with a parent object Dispose() occuring. This
// is similar to the WDM IRP cancel race, and is handled in exactly
// the same fashion.
//
// If an object is associated with a parent, and later removal
// is desired, the return status of RemoveChildObjectInternal must be
// tested, and if not success, the caller should not delete the
// child object itself, since the parent is in the process
// of doing so. The caller "lost the Dispose race".
//
//
// m_ParentObject field:
//
// This field is set by the child object after it successfully
// adds a parent object, and clear it when it is disposed by
// the parent, or removes the parent association.
//
// It is protected by the childs m_SpinLock field.
//







_Must_inspect_result_
NTSTATUS
FxObject::AssignParentObject(
    __in FxObject* ParentObject
    )
/*++

Routine Description:
    Assign a parent to the current object.      The parent can not be the same
    object.

Arguments:
    ParentObject - Object to become the parent for this object

Returns:

Comments:

    The caller "passes" its initial object reference to us, so we
    do not take an additional reference on the object.

    If we are deleted, Dispose() will be invoked on the object, and
    its reference will be released.

    This provides automatic deletion of associated child objects if
    the object does not keep any extra references.

--*/
{
    KIRQL oldIrql;
    NTSTATUS status;

    m_SpinLock.Acquire(&oldIrql);

    //
    // Can't add a parent if the current object is being deleted
    //
    if (m_ObjectState != FxObjectStateCreated) {
        TraceDroppedEvent(FxObjectDroppedEventAssignParentObject);
        m_SpinLock.Release(oldIrql);
        return STATUS_DELETE_PENDING;
    }

    //
    // Current Object can't already have a parent, and can't
    // be its own parent.
    //
    if (m_ParentObject != NULL) {
        m_SpinLock.Release(oldIrql);
        return STATUS_WDF_PARENT_ALREADY_ASSIGNED;
    }

    if (m_ParentObject == this) {
        m_SpinLock.Release(oldIrql);
        return STATUS_WDF_PARENT_IS_SELF;
    }

    //
    // We don't allow a parent object to be assigned after
    // FxObject::Commit().
    //
    ASSERTMSG("Parent object can not be assigned after Commit()\n", !IsCommitted());

    ASSERT(IsListEmpty(&this->m_ChildEntry));

    status = ParentObject->AddChildObjectInternal(this);

    if (NT_SUCCESS(status)) {
        m_ParentObject = ParentObject;
    }

    m_SpinLock.Release(oldIrql);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxObject::AddContext(
    __in FxContextHeader *Header,
    __in PVOID* Context,
    __in PWDF_OBJECT_ATTRIBUTES Attributes
    )
{
    FxContextHeader *pCur, **ppLast;
    NTSTATUS status;
    KIRQL irql;

    status = STATUS_UNSUCCESSFUL;

    pCur = GetContextHeader();

    //
    // This should never happen since all outward facing objects have a
    // context header; framework never calls this function on internal
    // objects.
    //
    ASSERT(pCur != NULL);

    //
    // Acquire the lock to lock the object's state.  A side affect of grabbing
    // the lock is that all updaters who want to add a context are serialized.
    // All callers who want to find a context do not need to acquire the lock
    // becuase they are not going to update the list, just read from it.
    //
    // Once a context has been added, it will not be removed until the object
    // has been deleted.
    //
    m_SpinLock.Acquire(&irql);

    if (m_ObjectState == FxObjectStateCreated && pCur != NULL) {
        //
        // Iterate over the list of contexts already on this object and see if
        // this type already is attached.
        //
        for (ppLast = &pCur->NextHeader;
             pCur != NULL;
             ppLast = &pCur->NextHeader, pCur = pCur->NextHeader) {

            if (pCur->ContextTypeInfo == Header->ContextTypeInfo) {
                //
                // Dupe found, return error but give the caller the context
                // pointer
                //
                if (Context != NULL) {
                    *Context = &pCur->Context[0];
                }

                status = STATUS_OBJECT_NAME_EXISTS;
                break;
            }
        }

        if (pCur == NULL) {
            //
            // By using the interlocked to update, we don't need to use a lock
            // when walking the list to find the context.   The only reason
            // we are holding the object lock is to lock the current state
            // (m_ObjectState) of the object.
            //
            InterlockedExchangePointer((PVOID*) ppLast, Header);
            status = STATUS_SUCCESS;

            if (Context != NULL) {
                *Context = &Header->Context[0];
            }

            //
            // FxContextHeaderInit does not set these callbacks.  If this were
            // the creation of the object itself, FxObject::Commit would have done
            // this assignment.
            //
            Header->EvtDestroyCallback = Attributes->EvtDestroyCallback;

            if (Attributes->EvtCleanupCallback != NULL) {
                Header->EvtCleanupCallback = Attributes->EvtCleanupCallback;
                m_ObjectFlags |= FXOBJECT_FLAGS_HAS_CLEANUP;
            }

        }
    }
    else {
        //
        // Object is being torn down, adding a context is a bad idea because we
        // cannot guarantee that the cleanup or destroy routines will be called
        //
        status = STATUS_DELETE_PENDING;
    }

    m_SpinLock.Release(irql);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxObject::AddChildObjectInternal(
    __in FxObject* ChildObject
    )

/*++

Routine Description:
    Called by an object to be added to this objects child list

Arguments:
    ChildObject - Object to add this this objects child list

Returns:
    NTSTATUS

Comments:
    The caller "passes" its initial object reference to us, so we
    do not take an additional reference on the object.

    If we are deleted, Dispose() will be invoked on the object, and
    its reference will be released.

    This provides automatic deletion of associated child objects if
    the object does not keep any extra references.

--*/
{
    KIRQL oldIrql;

    m_SpinLock.Acquire(&oldIrql);

    //
    // Can't add child if the current object is being deleted
    //
    if (m_ObjectState != FxObjectStateCreated) {
        TraceDroppedEvent(FxObjectDroppedEventAddChildObjectInternal);
        m_SpinLock.Release(oldIrql);
        return STATUS_DELETE_PENDING;
    }

    //
    // ChildObject can't already have a parent, and can't
    // be its own parent.
    //
    ASSERT(ChildObject->m_ParentObject == NULL);
    ASSERT(IsListEmpty(&ChildObject->m_ChildEntry));
    ASSERT(ChildObject != this);

    //
    // Add to our m_ChildList
    //
    InsertTailList(&m_ChildListHead, &ChildObject->m_ChildEntry);

    if (ChildObject->GetDeviceBase() == NULL) {
        //
        // Propagate the device base downward to the child
        //
        ChildObject->SetDeviceBase(GetDeviceBase());
    }

    m_SpinLock.Release(oldIrql);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxObject::RemoveChildObjectInternal(
    __in FxObject* ChildObject
    )
/*++

Routine Description:

    Remove a ChildObject from our child associations list.

    The ChildObject must exist in our list if we are not
    otherwise disposing or deleting ourselves.

    If we are not disposing, the child is removed from the list
    and its reference count is unmodified.

    If we are disposing, a failure is returned so that the caller
    does not delete or dereference the child object itself, since
    this is similar to a cancel IRP race condition.

Arguments:
    ChildObject - the object to remove this object's list of children

Returns:

    STATUS_SUCCESS - Child was removed from the list, no parent Dispose()
                     can occur.

    !STATUS_SUCCESS - Child can not be removed from the list, and is being
                      Disposed by the parent. The caller must *not* delete
                      the object itself.

--*/

{
    KIRQL oldIrql;

    m_SpinLock.Acquire(&oldIrql);

    //
    // Object is already being deleted, this object will be removed as a child
    // by the parents Dispose()
    //
    if (m_ObjectState != FxObjectStateCreated) {
        TraceDroppedEvent(FxObjectDroppedEventRemoveChildObjectInternal);
        m_SpinLock.Release(oldIrql);
        return STATUS_DELETE_PENDING;
    }

    //
    // We should be the child object's parent
    //
    ASSERT(ChildObject->m_ParentObject == this);

    //
    // Child should be on our list
    //
    ASSERT(!IsListEmpty(&ChildObject->m_ChildEntry));

    //
    // We should have entries if someone wants to remove from our list
    //
    ASSERT(!IsListEmpty(&m_ChildListHead));

    RemoveEntryList(&ChildObject->m_ChildEntry);

    //
    // Mark it removed
    //
    InitializeListHead(&ChildObject->m_ChildEntry);

    //
    // We did not take a reference on the child object when it
    // was added to the list, so we do not dereference it
    // on the remove call.
    //
    // Note: We only dereference child objects when we are deleted
    //       ourselves, not when the child object manually breaks the
    //       association by calling this method.
    //
    m_SpinLock.Release(oldIrql);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
FxObject*
FxObject::GetParentObjectReferenced(
    __in PVOID Tag
    )

/*++

Routine Description:
     Return this objects parent, which could be NULL if
     the object is not part of an association, or this or
     the parent object is deleting.

     An extra reference is taken on the parent object which
     must eventually be released by the caller.

Arguments:
    Tag - Tag to use when referencing the parent

Returns:

    Parent object, otherwise NULL if no parent for this object

--*/

{
    KIRQL oldIrql;
    FxObject* parentObject;

    m_SpinLock.Acquire(&oldIrql);

    if (m_ObjectState == FxObjectStateCreated) {
        parentObject = m_ParentObject;
    }
    else {
        // Parent is disposing us, or we are being disposed
        parentObject = NULL;
    }

    if (parentObject != NULL) {
        parentObject->ADDREF(Tag);
    }

    m_SpinLock.Release(oldIrql);

    return parentObject;
}

_Must_inspect_result_
NTSTATUS
FxObject::Commit(
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out_opt   WDFOBJECT*             ObjectHandle,
    __in_opt    FxObject* Parent,
    __in        BOOLEAN  AssignDriverAsDefaultParent
    )
/*++

Routine Description:
     Commit the object before returning the handle to the caller.

Arguments:
     Attributes - PWDF_OBJECT_ATTRIBUTES to assign to this object

     ObjectHandle - Location to return the objects handle

Returns:
     NTSTATUS of the result. STATUS_SUCCESS if success.

     Returns WDFOBJECT handle if success.

--*/
{
    NTSTATUS status;
    WDFOBJECT object;
    FxObject* parent;

    parent = NULL;

    if (m_ObjectSize == 0) {
        ASSERTMSG("Only external objects can call Commit()\n",
                  m_ObjectSize != 0);
        return STATUS_INVALID_HANDLE;
    }

    //
    // For an object to be committed into a handle, it needs to have an object
    // size.  Internal objects set their size to zero as the indication they
    // are internal and will not be converted into handles.
    //
    ASSERT(m_ObjectSize != 0);

    //
    // Caller has already validated basic WDF_OBJECT_ATTRIBUTES
    // with FxValidateObjectAttributes
    //

    //
    // Set object execution level constraint if specified
    //
    if (Attributes != NULL &&
        Attributes->ExecutionLevel == WdfExecutionLevelPassive) {
        MarkPassiveCallbacks();
    }

    //
    // Assign parent if supplied
    //
    if (Parent != NULL) {
        parent = Parent;
    }
    else if (Attributes != NULL && Attributes->ParentObject != NULL) {
        FxObjectHandleGetPtr(
            m_Globals,
            Attributes->ParentObject,
            FX_TYPE_OBJECT,
            (PVOID*)&parent
            );
    }
    else {

        //
        // If the object already does not have a parent, and
        // one has not been specified we default it to FxDriver.
        //
        // We check to ensure we are not FxDriver being created.
        //
        if (AssignDriverAsDefaultParent &&
            m_ParentObject == NULL) {

                //parent = FxToObjectItf::FxGetDriverAsDefaultParent(m_Globals, this);


                if (m_Globals->Driver != this) {
                    parent = m_Globals->Driver;
                }

        }
    }

    ASSERT(parent != this);

    if (parent != NULL) {
        //
        // Make it the parent of this object
        //
        status = AssignParentObject(parent);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    //
    // Now assign the optional EvtObjectCleanup, EvtObjectDestroy callbacks
    //
    if (Attributes != NULL) {
        FxContextHeader* pHeader;

        pHeader = GetContextHeader();

        if (Attributes->EvtDestroyCallback != NULL) {
            pHeader->EvtDestroyCallback = Attributes->EvtDestroyCallback;
        }

        if (Attributes->EvtCleanupCallback != NULL) {
            pHeader->EvtCleanupCallback = Attributes->EvtCleanupCallback;
            m_ObjectFlags |= FXOBJECT_FLAGS_HAS_CLEANUP;
        }
    }

    //
    // We mark the handle as committed so that we can create the handle.
    //
    MarkCommitted();

    //
    // Create the object handle, assign EvtObjectCleanup, EvtObjectDestroy
    //
    FxObjectHandleCreate(this, &object);

    if (ObjectHandle != NULL) {
        *ObjectHandle = object;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxObject::_GetEffectiveLock(
    __in        FxObject* Object,
    __in_opt    IFxHasCallbacks* Callbacks,
    __in        BOOLEAN AutomaticLocking,
    __in        BOOLEAN PassiveCallbacks,
    __out       FxCallbackLock** CallbackLock,
    __out_opt   FxObject** CallbackLockObject
    )
/*++

Routine Description:

    This gets the effective lock based on the callback constraints
    configuration of the supplied object.

    This is a common routine shared by all FxObject's that utilize
    DPC's. Currently, this is FxDpc, FxTimer, and FxInterrupt.

    This contains the common serialization configuration logic for these
    objects.

Arguments:

    Object - Object to serialize with

    Callbacks - Optional interface for acquiring constraints and locking pointers

    AutomaticLocking - TRUE if automatic serialization with Object is required

    PassiveCallbacks - TRUE if the caller requires passive level callback, FALSE
                       if the
    CallbackLock - Lock that is in effect for Object

    CallbackLockOjbect - FxObject that contains the callback lock

Returns:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDF_EXECUTION_LEVEL parentLevel;
    WDF_SYNCHRONIZATION_SCOPE parentScope;

    pFxDriverGlobals = Object->GetDriverGlobals();
    *CallbackLock = NULL;
    *CallbackLockObject = NULL;

    //
    // No automatic locking, nothing to do
    //
    if (AutomaticLocking == FALSE) {
        return STATUS_SUCCESS;
    }

    //
    // Objects that have callback locks must support this interface.
    //
    if (Callbacks == NULL) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Get the callback constraints in effect for the object
    //
    Callbacks->GetConstraints(&parentLevel, &parentScope);

    if (parentScope == WdfSynchronizationScopeInheritFromParent ||
        parentScope == WdfSynchronizationScopeNone) {
        //
        // Do nothing, no synchronization specified
        //
        DO_NOTHING();
    }
    else {
        //
        // If the caller wants passive callbacks and the object does not support
        // it, failure.
        //
        // If the caller wants non passive callbacks and the object supports
        // passive only callbacks, failure.
        //
        if ((PassiveCallbacks && Object->IsPassiveCallbacks() == FALSE) ||
            (PassiveCallbacks == FALSE && Object->IsPassiveCallbacks())) {
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL;
        }

        *CallbackLock = Callbacks->GetCallbackLockPtr(CallbackLockObject);
    }

    return STATUS_SUCCESS;
}
