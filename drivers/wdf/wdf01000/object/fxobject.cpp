#include "common/fxobject.h"
#include "common/fxglobals.h"
#include "common/fxtrace.h"
#include "common/fxverifier.h"
#include "common/fxtagtracker.h"
#include "common/fxdriver.h"


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
     if (ObjectType != FxObjectTypeEmbedded)
     {
         //
         // only for non embedded objects
         //
         ASSERT((((ULONG_PTR) this) & FxHandleFlagMask) == 0x0);
     }

    Construct(ObjectType == FxObjectTypeEmbedded ? TRUE : FALSE);
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

FxObject::~FxObject()
{
    FxTagTracker *pTagTracker;

    pTagTracker = GetTagTracker();

    if (pTagTracker != NULL)
    {
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
        !IsListEmpty(&m_ChildListHead) || !IsListEmpty(&m_ChildEntry))
    {
        PCSTR pHandleName;

        pHandleName = FxObjectTypeToHandleName(m_Type);
        if (pHandleName == NULL)
        {
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

BOOLEAN
FxObject::IsDisposed(
    VOID
    )
{
    KIRQL   oldIrql;
    BOOLEAN disposed;

    if (m_Globals->FxVerifierOn &&
        m_Globals->FxVerifierHandle)
    {
        m_SpinLock.Acquire(&oldIrql);

        if (m_ObjectState == FxObjectStateCreated)
        {
            disposed = FALSE;
        }
        else
        {
            //
            // Parent is disposing us, or we are being disposed
            //
            disposed = TRUE;
        }

        m_SpinLock.Release(oldIrql);
        return disposed;
    }
    else
    {
        return TRUE;
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

#ifdef __GNUC__
VOID
FxObject::Vf_VerifyConstruct (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ BOOLEAN Embedded
    )
#else
VOID
FX_VF_METHOD(FxObject, VerifyConstruct) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ BOOLEAN Embedded
    )
#endif
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
    if (m_Globals->IsObjectDebugOn() && Embedded == FALSE)
    {
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
FxObject::AllocateTagTracker(
    __in WDFTYPE Type
    )
{
    ASSERT(IsDebug());

    if (m_Globals->DebugExtension != NULL &&
        //m_Globals->FxVerifyTagTrackingEnabled != FALSE &&
        m_Globals->DebugExtension->ObjectDebugInfo != NULL &&
        FxVerifierIsDebugInfoFlagSetForType(
            m_Globals->DebugExtension->ObjectDebugInfo,
            Type,
            FxObjectDebugTrackReferences))
    {
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
         pHeader = pHeader->NextHeader)
    {
            
        pHeader->EvtDestroyCallback = NULL;
        pHeader->EvtCleanupCallback = NULL;
    }
         
    m_ObjectFlags &= ~FXOBJECT_FLAGS_HAS_CLEANUP;
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
    if (m_ObjectState != FxObjectStateCreated)
    {
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
    if (IsCommitted() == FALSE)
    {
        return;
    }

    //
    // We only should have an object handle when we have an external object
    //
    h = GetObjectHandle();

    for (pHeader = GetContextHeader();
         pHeader != NULL;
         pHeader = pHeader->NextHeader)
    {
        if (pHeader->EvtCleanupCallback != NULL)
        {
            pHeader->EvtCleanupCallback(h);
            pHeader->EvtCleanupCallback = NULL;
        }
    }

    m_ObjectFlags &= ~FXOBJECT_FLAGS_HAS_CLEANUP;
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
    if (ShouldDeferDisposeLocked())
    {
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
    else
    {
        ProcessDestroy();
    }
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

    if (m_ObjectSize == 0)
    {
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
        Attributes->ExecutionLevel == WdfExecutionLevelPassive)
    {
        MarkPassiveCallbacks();
    }

    //
    // Assign parent if supplied
    //
    if (Parent != NULL)
    {
        parent = Parent;
    }
    else if (Attributes != NULL && Attributes->ParentObject != NULL)
    {
        FxObjectHandleGetPtr(
            m_Globals,
            Attributes->ParentObject,
            FX_TYPE_OBJECT,
            (PVOID*)&parent
            );
    }
    else
    {
        //
        // If the object already does not have a parent, and
        // one has not been specified we default it to FxDriver.
        //
        // We check to ensure we are not FxDriver being created.
        //
        if (AssignDriverAsDefaultParent &&
            m_ParentObject == NULL)
        {
            //parent = FxToObjectItf::FxGetDriverAsDefaultParent(m_Globals, this);
            if (m_Globals->Driver != this)
            {
                parent = m_Globals->Driver;
            }
        }
    }

    ASSERT(parent != this);

    if (parent != NULL)
    {
        //
        // Make it the parent of this object
        //
        status = AssignParentObject(parent);

        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    //
    // Now assign the optional EvtObjectCleanup, EvtObjectDestroy callbacks
    //
    if (Attributes != NULL)
    {
        FxContextHeader* pHeader;

        pHeader = GetContextHeader();

        if (Attributes->EvtDestroyCallback != NULL)
        {
            pHeader->EvtDestroyCallback = Attributes->EvtDestroyCallback;
        }

        if (Attributes->EvtCleanupCallback != NULL)
        {
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

    if (ObjectHandle != NULL)
    {
        *ObjectHandle = object;
    }

    VerifyLeakDetectionConsiderObject(m_Globals);

    return STATUS_SUCCESS;
}

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
    if (m_ParentObject != NULL)
    {
        m_SpinLock.Release(oldIrql);
        return STATUS_WDF_PARENT_ALREADY_ASSIGNED;
    }

    if (m_ParentObject == this)
    {
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

    if (NT_SUCCESS(status))
    {
        m_ParentObject = ParentObject;
    }

    m_SpinLock.Release(oldIrql);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxObject::QueryInterface(
    __in FxQueryInterfaceParams* Params
    )
{
    NTSTATUS status;

    if (Params->Type == FX_TYPE_OBJECT)
    {
        *Params->Object = this;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_NOINTERFACE;
    }

    return status;
}

#ifdef __GNUC__
VOID
FxObject::Vf_VerifyLeakDetectionConsiderObject (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
#else
VOID
FX_VF_METHOD(FxObject, VerifyLeakDetectionConsiderObject) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
#endif
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);

    //
    // Check to see if we are potentially leaking objects.
    // Verify leak detection is enabled and that the object type
    // is configured to be counted. We always count WDFDEVICE because
    // we need it to scale the limit if there are multiple devices.
    // 
    /*if ((m_Globals->FxVerifyLeakDetection != NULL) &&
        (m_Globals->FxVerifyLeakDetection->Enabled) &&
        (FxVerifierIsDebugInfoFlagSetForType(
            m_Globals->DebugExtension->ObjectDebugInfo,
            m_Type,
            FxObjectDebugTrackObjectCount) ||
            (m_Type == FX_TYPE_DEVICE))
        )
    {

        LONG c;
        FxObjectDebugLeakDetection *leakDetection = m_Globals->FxVerifyLeakDetection;
        FxObjectDebugExtension* pExtension = GetDebugExtension();

        switch (m_Type) {
        case FX_TYPE_REQUEST:
        {
            FxRequestBase* requestBase = static_cast<FxRequestBase*>(this);
            if (!requestBase->IsAllocatedDriver())
            {
                //
                // Only count driver allocated requests, not framwork or 
                // IO from callers
                //
                return;
            }
            break;
        }
        case FX_TYPE_DEVICE:
        {
            // 
            // Scale the threshold the verify check happens at. For every
            // device created we increase the limit by a multiple.
            //
            if (m_Type == FX_TYPE_DEVICE)
            {
                c = InterlockedIncrement(&leakDetection->DeviceCnt);
                if (c >= 2)
                {
                    //
                    // We skip 0->1 because LimitScaled is initialized
                    // to Limit
                    //
                    InterlockedExchangeAdd(&leakDetection->LimitScaled,
                        leakDetection->Limit);
                }
            }
            break;
        }
        }

        pExtension->ObjectCounted = TRUE;
        c = InterlockedIncrement(&leakDetection->ObjectCnt);

        //
        // Check for exceeding the limit (no interlocked protection)
        //
        if (c == leakDetection->LimitScaled)
        {

            //
            // Potential leak of objects detected
            // Device has exceeded WDF Verifiers peak threshold for 
            // objects to be allocated. Use !wdfDriverInfo <drivername> 
            // with flags 0x41 or 0x50 to see a list and count of objects 
            // currently allocated. 
            //
            // To adjust this setting modify registry key 
            // "ObjectLeakDetectionLimit", which is a REG_DWORD, 
            // under the drivers Parameters\Wdf subkey. 0xFFFFFFFF 
            // will disable the check, any other value to set the
            // threshold.
            //
            // NOTE: the limit will be scaled based on the number of
            // WDFDEVICE objects present under the driver.
            //
            DoTraceLevelMessage(
                m_Globals, TRACE_LEVEL_ERROR, TRACINGOBJECT,
                "WDF Verifier has detected an excessive number of "
                "allocated WDF objects. Investigate with !wdfDriverInfo "
                "<driverName> 0x41 or 0x50");

            DoTraceLevelMessage(
                m_Globals, TRACE_LEVEL_ERROR, TRACINGOBJECT,
                "WDF Verifier found %u objects allocated, limit=%u,"
                " and the scaled limit=%u",
                c, leakDetection->Limit, leakDetection->LimitScaled);

            FxVerifierDbgBreakPoint(m_Globals);

            //
            // Disable the check going forward
            //
            leakDetection->Enabled = FALSE;
        }
    }*/
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
    if (m_ObjectState != FxObjectStateCreated)
    {
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

    if (ChildObject->GetDeviceBase() == NULL)
    {
        //
        // Propagate the device base downward to the child
        //
        ChildObject->SetDeviceBase(GetDeviceBase());
    }
    
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

    if (m_ObjectState == FxObjectStateCreated)
    {
        parentObject = m_ParentObject;
    }
    else
    {
        // Parent is disposing us, or we are being disposed
        parentObject = NULL;
    }

    if (parentObject != NULL)
    {
        parentObject->ADDREF(Tag);
    }

    m_SpinLock.Release(oldIrql);

    return parentObject;
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

    if (m_ObjectState == FxObjectStateCreated && pCur != NULL)
    {
        //
        // Iterate over the list of contexts already on this object and see if
        // this type already is attached.
        //
        for (ppLast = &pCur->NextHeader;
             pCur != NULL;
             ppLast = &pCur->NextHeader, pCur = pCur->NextHeader)
        {
            if (pCur->ContextTypeInfo == Header->ContextTypeInfo)
            {
                //
                // Dupe found, return error but give the caller the context
                // pointer
                //
                if (Context != NULL)
                {
                    *Context = &pCur->Context[0];
                }

                status = STATUS_OBJECT_NAME_EXISTS;
                break;
            }
        }

        if (pCur == NULL)
        {
            //
            // By using the interlocked to update, we don't need to use a lock
            // when walking the list to find the context.   The only reason
            // we are holding the object lock is to lock the current state
            // (m_ObjectState) of the object.
            //
            InterlockedExchangePointer((PVOID*) ppLast, Header);
            status = STATUS_SUCCESS;

            if (Context != NULL)
            {
                *Context = &Header->Context[0];
            }

            //
            // FxContextHeaderInit does not set these callbacks.  If this were
            // the creation of the object itself, FxObject::Commit would have done
            // this assignment.
            //
            Header->EvtDestroyCallback = Attributes->EvtDestroyCallback;

            if (Attributes->EvtCleanupCallback != NULL)
            {
                Header->EvtCleanupCallback = Attributes->EvtCleanupCallback;
                m_ObjectFlags |= FXOBJECT_FLAGS_HAS_CLEANUP;
            }

        }
    }
    else
    {
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
    if (AutomaticLocking == FALSE)
    {
        return STATUS_SUCCESS;
    }

    //
    // Objects that have callback locks must support this interface.
    //
    if (Callbacks == NULL)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Get the callback constraints in effect for the object
    //
    Callbacks->GetConstraints(&parentLevel, &parentScope);

    if (parentScope == WdfSynchronizationScopeInheritFromParent ||
        parentScope == WdfSynchronizationScopeNone)
    {
        //
        // Do nothing, no synchronization specified
        //
        DO_NOTHING();
    }
    else
    {
        //
        // If the caller wants passive callbacks and the object does not support
        // it, failure.
        //
        // If the caller wants non passive callbacks and the object supports
        // passive only callbacks, failure.
        //
        if ((PassiveCallbacks && Object->IsPassiveCallbacks() == FALSE) ||
            (PassiveCallbacks == FALSE && Object->IsPassiveCallbacks()))
        {
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL;
        }

        *CallbackLock = Callbacks->GetCallbackLockPtr(CallbackLockObject);
    }

    return STATUS_SUCCESS;
}
