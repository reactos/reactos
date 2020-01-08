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

