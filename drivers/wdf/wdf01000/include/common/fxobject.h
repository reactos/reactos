/*
        IMPORTANT: Common code must call Initialize method of
        FxObject before using it

        Cannot put CreateAndInitialize method on this class as it
        cannot be instantiated
*/

#ifndef _FXOBJECT_H_
#define _FXOBJECT_H_

#include "fxtypes.h"
#include "fxhandle.h"
#include "primitives/mxlock.h"
#include "fxglobals.h"
#include "fxmacros.h"
#include "fxtypedefs.h"
#include "dbgtrace.h"
#include "fxtagtracker.h"
#include "fxpoolinlines.h"
#include "ifxhascallbacks.h"
#include "wdf.h"


//
// Macros to pass line and file information to AddRef and Release calls.
//
// NOTE:  ADDREF and RELEASE can be used by any class derived from FxObject or
//        IFxMemory while the OFFSET only works with an FxObject derivation
//
#define ADDREF(_tag)    AddRef(_tag, __LINE__, __FILE__)
#define RELEASE(_tag)   Release(_tag, __LINE__, __FILE__)

#define ADDREF_OFFSET(_tag, offset)    AddRefOverride(offset, _tag, __LINE__, __FILE__)
#define RELEASE_OFFSET(_tag, offset)   ReleaseOverride(offset, _tag, __LINE__, __FILE__)

//
// This assumes that the bottom 3 bits of an FxObject are clear, ie that the
// FxObject is 8 byte aligned.  The conversion from and to a handle value
// will set / clear these bits as appropriate.
//
enum FxHandleFlags {
    FxHandleFlagIsOffset = 0x1,
    FxHandleFlagMask     = 0x7, // bottom 3 bits are free
};

//
// We cannot define FxHandleValueMask as an enumerant in FxHandleFlags because
// an enum is limited to sizeof(ULONG), which doesn't work for us on a 64 bit OS
//
extern __declspec(selectany) const ULONG_PTR FxHandleValueMask = (~((ULONG_PTR) FxHandleFlagMask));

//typedef USHORT WDFOBJECT_OFFSET, *PWDFOBJECT_OFFSET;


struct FxQueryInterfaceParams {
    //
    // We intentionally do not declare a constructor for this structure.
    // Instead, code should use an inline initializer list.  This reduces the
    // number of cycles on hot paths by removing a funciton call.
    //
    // FxQueryInterfaceParams(
    //     PVOID* Object,
    //     WDFTYPE Type
    //     ) :
    // Type(Type), Object(Object), Offset(0) {if (Object != NULL) *Object = NULL; }}

    //
    // Object to query for
    //
    PVOID* Object;

    //
    // Type for the object to query for
    //
    WDFTYPE Type;

    //
    // Offset of handle within its owning object.  If zero, the Object was the
    // handle.  If not zero, ((PUCHAR) Object)-Offset will yield the owning
    // Object.
    //
    WDFOBJECT_OFFSET Offset;
};


class FxVerifierLock;
class FxTagTracker;
class FxDisposeList;

#define DECLARE_INTERNAL_NEW_OPERATOR()                     \
    PVOID                                                   \
    __inline                                             \
    operator new(                                           \
        __in size_t Size,                                        \
        __in PFX_DRIVER_GLOBALS FxDriverGlobals                 \
        )                                                   \
    {                                                       \
        return FxObjectHandleAlloc(FxDriverGlobals,         \
                                   NonPagedPool,            \
                                   Size,                    \
                                   0,                       \
                                   WDF_NO_OBJECT_ATTRIBUTES,\
                                   0,                       \
                                   FxObjectTypeInternal);   \
    }

struct FxObjectDebugExtension {
    FxTagTracker* TagTracker;

    FxVerifierLock* VerifierLock;

    BYTE StateHistory[8];

    LONG StateHistoryIndex;

    //
    // For object leak detection, TRUE indicates the object is been counted
    // and needs it count removed
    //
    BOOLEAN ObjectCounted;

    //
    // Signature lives after all the fields to avoid byte padding if it is
    // first on 64 bit systems.
    //
    ULONG Signature;

    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONG AllocationStart[1];
};

//
// type of object being allocated.  An internal object does *NOT*
// 1) have its size rounded up to an alignment value
// 2) extra size and context header appended to the allocation
//
enum FxObjectType {
    FxObjectTypeInvalid = 0,
    FxObjectTypeInternal,
    FxObjectTypeExternal,
    FxObjectTypeEmbedded,
};

enum FxObjectDebugExtensionValues {
    FxObjectDebugExtensionSize  = FIELD_OFFSET(FxObjectDebugExtension, AllocationStart),
    FxObjectDebugExtensionSignature = 'DOxF',
};

//
// FxObject state represents whether an object is
// tearing down, and what phase its in.
//
enum FxObjectState {
    FxObjectStateInvalid = 0,
    FxObjectStateCreated,
    FxObjectStateDisposed,
    FxObjectStateDisposingEarly,
    FxObjectStateDisposingDisposeChildren,
    FxObjectStateDeferedDisposing,
    FxObjectStateDeferedDeleting,
    FxObjectStateWaitingForEarlyDispose,
    FxObjectStateWaitingForParentDeleteAndDisposed,
    FxObjectStateDeletedDisposing,
    FxObjectStateDeletedAndDisposed,
    FxObjectStateDeferedDestroy,
    FxObjectStateDestroyed,
    FxObjectStateMaximum,
};

enum FxObjectDroppedEvent {
    FxObjectDroppedEventAssignParentObject = 0,
    FxObjectDroppedEventAddChildObjectInternal,
    FxObjectDroppedEventRemoveChildObjectInternal,
    FxObjectDroppedEventDeleteObject,
    FxObjectDroppedEventPerformEarlyDispose,
    FxObjectDroppedEventRemoveParentAssignment,
    FxObjectDroppedEventParentDeleteEvent,
};

// Ensures that a BOOL type is generated from a flag mask
#define FLAG_TO_BOOL(_Flags, _FlagMask) (!!((_Flags) & (_FlagMask)))

enum FxObjectLockState {
    ObjectDoNotLock = 0,
    ObjectLock = 1
};

//
// Defines for FxObject::m_ObjectFlags
//
// NOTE:  if you modify (add, remove, reassign a value) this enum in any way
//        you must also change FxObject::m_ObjectFlagsByName.* to match your changes!!
//
enum FXOBJECT_FLAGS {
    FXOBJECT_FLAGS_PASSIVE_CALLBACKS = 0x00000001, // Object must have all callbacks at passive level
                                                   // implies passive destroy
    FXOBJECT_FLAGS_NODELETEDDI      = 0x00000002,  // The WdfObjectDelete DDI is invalid
    FXOBJECT_FLAGS_DELETECALLED     = 0x00000004,  // DeleteObject method called
    FXOBJECT_FLAGS_COMMITTED        = 0x00000008,  // Commit called
    FXOBJECT_FLAGS_PASSIVE_DISPOSE  = 0x00000010, // Object must be Dispose()'d at passive level
    FXOBJECT_FLAGS_FORCE_DISPOSE_THREAD  = 0x00000020, // Object is always disposed in the dispose thread
    // UNUSED                       = 0x00000040,
    FXOBJECT_FLAGS_HAS_DEBUG        = 0x00000080, // has FxObjectDebugExtension before object pointer
    FXOBJECT_FLAGS_EARLY_DISPOSED_EXT = 0x00000100,  // Early disposed externally to the state machine
    FXOBJECT_FLAGS_TRACE_STATE      = 0x00000200, // log state changes to the IFR
    FXOBJECT_FLAGS_HAS_CLEANUP      = 0x00000400,
    FXOBJECT_FLAGS_DISPOSE_OVERRIDE = 0x00000800,
};

// forward definitions
//typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;

//
// The type itself is aligned, but the pointer is not b/c those interested in the
// offset do not need it to be aligned.
//
// The offset is aligned on an 8 byte boundary so that we have the lower 3 bits
// of the low byte to use for a bit field
//
typedef DECLSPEC_ALIGN(8) USHORT WDFOBJECT_OFFSET_ALIGNED;

class FxObject {

friend FxDisposeList;

private:

    WDFTYPE       m_Type;

    //
    // This is the already computed value of
    // WDF_ALIGN_SIZE_UP(sizeof(derived object), MEMORY_ALLOCATION_ALIGNMENT) +
    // WDF_ALIGN_SIZE_UP(extraContext, MEMORY_ALLOCATION_ALIGNMENT)
    //
    USHORT        m_ObjectSize;

    // Reference count is operated on with interlocked operations
    LONG          m_Refcnt;

    PFX_DRIVER_GLOBALS m_Globals;

        // Flags are protected by m_SpinLock, bits defined by the enum FXOBJECT_FLAGS
    union {
        USHORT         m_ObjectFlags;

        // Each field in this struct correspond to ascending enumerant values
        // in FXOBJECT_FLAGS.
        //
        // NOTE:  you should never touch these fields in any code, they are here
        //        strictly for use by the debugger extension so that it doesn't
        //       have to result FXOBJECT_FLAGS enumerant name strings into values.
        //
        struct {
            USHORT PassiveCallbacks : 1;
            USHORT NoDeleteDDI : 1;
            USHORT DeleteCalled : 1;
            USHORT Committed : 1;
            USHORT PassiveDispose : 1;
            USHORT ForceDisposeThread : 1;
            USHORT Unused  : 1;
            USHORT HasDebug : 1;
            USHORT EarlyDisposedExt : 1;
            USHORT TraceState : 1;
        } m_ObjectFlagsByName;
    };

    //
    // m_ObjectState is protected by m_SpinLock.  Contains values in the
    // FxObjectState enum.
    //
    USHORT m_ObjectState;

    // List of Child Objects
    LIST_ENTRY m_ChildListHead;

    // SpinLock protects m_ObjectState, m_ObjectFlags, m_ChildListHead, and m_ParentObject
    MxLock m_SpinLock;

    //
    // Parent object when this object is a child.
    //
    // This is protected by the child objects m_SpinLock
    //
    FxObject*  m_ParentObject;

    //
    // Link for when this object is a child
    //
    // This is accessed by the parent of this object, and
    // protected by the parents spinlock.
    //
    LIST_ENTRY m_ChildEntry;

    //
    //
    //
    SINGLE_LIST_ENTRY m_DisposeSingleEntry;

protected:
    union {
        //
        // This field is set when the object is being contructed or before
        // Commit() and from then on is read only.
        //
        // FxDeviceBase* is used by the core object state machine.  Derived
        // objects might need the fuller FxDevice* (and they can safely get at
        // it since they know they are a part of the derived FxDevice*).
        //
        CfxDeviceBase* m_DeviceBase;
        CfxDevice* m_Device;
    };


private:

    FxObject(
        VOID
        )
    {
        // Always make the caller supply a type and size
    }
    
    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    VOID, 
    VerifyConstruct, 
        _In_ BOOLEAN
        );

    FX_DECLARE_VF_FUNCTION(
    VOID,
    VerifyLeakDetectionConsiderObject
        );

    VOID
    __inline
    Construct(
        __in BOOLEAN Embedded
        )
    {
        m_Refcnt = 1;
        m_ObjectState = FxObjectStateCreated;
        m_ObjectFlags = 0;
        m_ParentObject = NULL;

        InitializeListHead(&m_ChildListHead);
        InitializeListHead(&m_ChildEntry);
        m_DisposeSingleEntry.Next = NULL;

        m_DeviceBase = NULL;

        VerifyConstruct(m_Globals, Embedded);
    }

    VOID
    __inline
    SetObjectStateLocked(
        __in FxObjectState NewState
        )
    {
        if (m_ObjectFlags & FXOBJECT_FLAGS_TRACE_STATE)
        {
            DoTraceLevelMessage(
                m_Globals, TRACE_LEVEL_VERBOSE, TRACINGOBJECT,
                "Object %p, WDFOBJECT %p transitioning from %!FxObjectState! to "
                "%!FxObjectState!", this, GetObjectHandleUnchecked(),
                m_ObjectState, NewState);
            if (IsDebug())
            {
                FxObjectDebugExtension* pExtension;

                pExtension = GetDebugExtension();
                pExtension->StateHistory[
                    InterlockedIncrement(&pExtension->StateHistoryIndex)] =
                    (BYTE) NewState;
            }
        }

        m_ObjectState = (USHORT) NewState;
    }

    //
    // This is used by verifier to ensure that DeleteObject
    // is only called once.
    //
    // It must be accessed under the m_SpinLock.
    //
    // It returns TRUE if this is the first call.
    //
    BOOLEAN
    MarkDeleteCalledLocked(
        VOID
        )
    {
        BOOLEAN retval;

        retval = !(m_ObjectFlags & FXOBJECT_FLAGS_DELETECALLED);

        m_ObjectFlags |= FXOBJECT_FLAGS_DELETECALLED;

        return retval;
    }

    _Must_inspect_result_
    NTSTATUS
    RemoveChildObjectInternal(
        __in FxObject* ChildObject
        );

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    DeleteWorkerAndUnlock(
        __in __drv_restoresIRQL KIRQL OldIrql,
        __in                    BOOLEAN CanDefer
        );

    BOOLEAN
    IsPassiveDisposeLocked(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_PASSIVE_DISPOSE);
    }

    BOOLEAN
    IsForceDisposeThreadLocked(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_FORCE_DISPOSE_THREAD);
    }

    BOOLEAN
    ShouldDeferDisposeLocked(
        __out_opt PKIRQL PreviousIrql = NULL
        )
    {
        if (IsForceDisposeThreadLocked())
        {
            return TRUE;
        }
        else if (IsPassiveDisposeLocked())
        {
            //
            // Only call KeGetCurrentIrql() if absolutely necessary.  It is an
            // expensive call and we want to minimize the cycles required when
            // destroying an object  that requires passive rundown
            //

            //
            // Cases:
            // 1)  Caller does not know the current IRQL, so we must query for it
            //
            // 2)  Caller knew prev IRQL, so we used the caller's value
            //
            if (PreviousIrql == NULL)
            {                  // case 1
                if (Mx::MxGetCurrentIrql() != PASSIVE_LEVEL)
                {
                    return TRUE;
                }
            }
            else if (*PreviousIrql != PASSIVE_LEVEL)
            {   // case 2
                return TRUE;
            }
        }

        return FALSE;
    }

    VOID
    QueueDeferredDisposeLocked(
        __in FxObjectState NewDeferedState
        );

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    DisposeChildrenWorker(
        __in                    FxObjectState NewDeferedState,
        __in __drv_restoresIRQL KIRQL OldIrql,
        __in                    BOOLEAN CanDefer
        );

    _When_(Unlock, _Releases_lock_(this->m_SpinLock.m_Lock))
    __drv_when(Unlock, __drv_requiresIRQL(DISPATCH_LEVEL))
    VOID
    DeletedAndDisposedWorkerLocked(
        __in __drv_when(Unlock, __drv_restoresIRQL) KIRQL OldIrql,
        __in                                        BOOLEAN Unlock = TRUE
        );

    VOID
    DeferredDisposeWorkItem(
        VOID
        );

    BOOLEAN
    PerformEarlyDispose(
        VOID
        );

    VOID
    CallCleanupCallbacks(
        VOID
        );

    VOID
    FinalRelease(
        VOID
        );

    VOID
    ParentDeleteEvent(
        VOID
        );

    VOID
    ProcessDestroy(
        VOID
        );

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    PerformDisposingDisposeChildrenLocked(
        __in __drv_restoresIRQL KIRQL OldIrql,
        __in                    BOOLEAN CanDefer
        );

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    PerformEarlyDisposeWorkerAndUnlock(
        __in __drv_restoresIRQL KIRQL OldIrql,
        __in                    BOOLEAN CanDefer
        );

    _Must_inspect_result_
    NTSTATUS
    AddChildObjectInternal(
        __in FxObject* ChildObject
        );

    BOOLEAN
    IsPassiveCallbacksLocked(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_PASSIVE_CALLBACKS);
    }

    
public:
    //
    // Request that an object be deleted.
    //
    // This can be the result of a WDF API or a WDM event.
    //
    virtual
    VOID
    DeleteObject(
        VOID
        );

    FxObject(
        __in WDFTYPE Type,
        __in USHORT Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxObject(
        VOID
        );

    PVOID
    __inline
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObjectType Type
        )
    {
        UNREFERENCED_PARAMETER(Type);

        return FxObjectHandleAlloc(FxDriverGlobals,
                                   NonPagedPool,
                                   Size,
                                   0,
                                   WDF_NO_OBJECT_ATTRIBUTES,
                                   0,
                                   Type);
    }

    PVOID
    __inline
    operator new(
        __in        size_t Size,
        __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
        __in USHORT ExtraSize = 0
        )
    {
        return FxObjectHandleAlloc(FxDriverGlobals,
                                   NonPagedPool,
                                   Size,
                                   0,
                                   Attributes,
                                   ExtraSize,
                                   FxObjectTypeExternal);
    }

    VOID
    operator delete(
        __in PVOID Memory
        );

    PVOID
    __inline
    GetObjectHandle(
        VOID
        )
    {
        ASSERT(GetRefCnt() > 0);
        return GetObjectHandleUnchecked();
    }

    static
    FxObject*
    _FromDisposeEntry(
        __in PSINGLE_LIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxObject, m_DisposeSingleEntry);
    }

    //
    // m_ObjectSize contains the size of the object + extra size.  m_ObjectSize
    // was already rounded up to the correct alignment when it was contructed.
    //
    #define GET_CONTEXT_HEADER() WDF_PTR_ADD_OFFSET_TYPE(this, m_ObjectSize, FxContextHeader*)

    BOOLEAN
    IsNoDeleteDDI(
        VOID
        )
    {
        // No need for lock since its only set in constructor/init
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_NODELETEDDI);
    }

    //
    // Commit the WDF object before returning handle to the caller.
    //
    _Must_inspect_result_
    NTSTATUS
    Commit(
        __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
        __out_opt   WDFOBJECT*             ObjectHandle,
        __in_opt    FxObject* Parent = NULL,
        __in        BOOLEAN  AssignDriverAsDefaultParent = TRUE
        );

    VOID
    DeleteFromFailedCreate(
        VOID
        );

    VOID
    ClearEvtCallbacks(
        VOID
        );

    //
    // This is public to allow debug code to assert that
    // an object has been properly disposed either through
    // calling DeleteObject, or being disposed by its parent.
    //
    BOOLEAN
    IsDisposed(
        VOID
        );

    static
    PFX_POOL_HEADER
    _CleanupPointer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject* Object
        )
    {
        PFX_POOL_HEADER pHeader;
        PVOID pObjectBase;

        pObjectBase = _GetBase(Object);

        pHeader = CONTAINING_RECORD(pObjectBase, FX_POOL_HEADER, AllocationStart);

        //
        // If PoolTracker is on then do....
        //
        if (FxDriverGlobals->IsPoolTrackingOn())
        {
            //
            // Decommission this NonPaged Allocation tracker
            //
            FxPoolRemoveNonPagedAllocateTracker((PFX_POOL_TRACKER) pHeader->Base);
        }

        return pHeader;
    }

    _Must_inspect_result_
    static
    NTSTATUS
    _GetEffectiveLock(
        __in        FxObject* Object,
        __in_opt    IFxHasCallbacks* Callbacks,
        __in        BOOLEAN AutomaticLocking,
        __in        BOOLEAN PassiveCallbacks,
        __out       FxCallbackLock** CallbackLock,
        __out_opt   FxObject** CallbackLockObject
        );

    BOOLEAN
    IsDebug(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_HAS_DEBUG);
    }

    static
    PVOID
    _GetBase(
        __in FxObject* Object
        )
    {
        if (Object->IsDebug())
        {
            return _GetDebugBase(Object);
        }
        else
        {
            return (PVOID) Object;
        }
    }

    WDFTYPE
    GetType(
        VOID
        )
    {
        return m_Type;
    }

    USHORT
    GetObjectSize(
        VOID
        )
    {
        return m_ObjectSize;
    }

    LONG
    GetRefCnt(
        VOID
        )
    {
        return m_Refcnt;
    }

    FxTagTracker*
    GetTagTracker(
        VOID
        )
    {
        if (IsDebug())
        {
            return CONTAINING_RECORD(this,
                                     FxObjectDebugExtension,
                                     AllocationStart)->TagTracker;
        }
        else
        {
            return NULL;
        }
    }

    static
    PVOID
    _GetDebugBase(
        __in FxObject* Object
        )
    {
        return CONTAINING_RECORD(Object, FxObjectDebugExtension, AllocationStart);
    }

    static
    PVOID
    __inline
    _ToHandle(
        __in FxObject* Object
        )
    {
        //
        // Always XOR the constant. Faster than checking
        // FxDriverGlobals->FxVerifierHandle.
        //
        return (PVOID) (((ULONG_PTR) Object) ^ FxHandleValueMask);
    }

    BOOLEAN
    IsCommitted(
        VOID
        )
    {
        //
        // No need to acquire the lock because it is assumed the caller is
        // calling on an object whose ref count has gone to zero so there are
        // no other callers to contend with who might set this flag or modify
        // m_ObjectFlags.
        //
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_COMMITTED);
    }

    VOID
    __inline
    TraceDroppedEvent(
        __in FxObjectDroppedEvent Event
        )
    {
        if (IsTraceState())
        {
            DoTraceLevelMessage(
                m_Globals, TRACE_LEVEL_INFORMATION, TRACINGOBJECT,
                "Object %p, WDFOBJECT %p, state %!FxObjectState! dropping event"
                " %!FxObjectDroppedEvent!",
                this, GetObjectHandleUnchecked(), m_ObjectState, Event);
        }
    }

    //
    // Invoked by FxObject *once* when the object is either
    // being deleted, or rundown due to its parent object
    // being deleted.
    //
    // An object can override this to perform per object
    // cleanup if required.
    //
    // TRUE means that the cleanup callbacks should be called when the function
    // returns.  FALSE indicates that the caller will take care of calling the
    // cleanup callbacks on behalf of the state machine.
    //
    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    __inline
    VOID
    CallCleanup(
        VOID
        )
    {
        if (m_ObjectFlags & FXOBJECT_FLAGS_HAS_CLEANUP)
        {
            CallCleanupCallbacks();
        }
    }

    virtual
    ULONG
    Release(
        __in_opt    PVOID Tag = NULL,
        __in        LONG Line = 0,
        __in_opt    PCSTR File = NULL
        )
    {
        FxTagTracker* pTagTracker;
        ULONG c;

        pTagTracker = GetTagTracker();
        if (pTagTracker != NULL)
        {
            pTagTracker->UpdateTagHistory(Tag, Line, File, TagRelease, m_Refcnt - 1);
        }

        c = InterlockedDecrement(&m_Refcnt);

        if (c == 0)
        {
            FinalRelease();
        }

        return c;
    }

    //
    // Sets that the object is a passive level only object who
    // accesses page-able pool, routines, or has a driver
    // specified passive constraint on callbacks such as
    // Cleanup and Destroy.
    //
    VOID
    MarkPassiveCallbacks(
        __in FxObjectLockState State = ObjectLock
        )
    {
        if (State == ObjectLock)
        {
            KIRQL   oldIrql;

            m_SpinLock.Acquire(&oldIrql);
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_CALLBACKS | FXOBJECT_FLAGS_PASSIVE_DISPOSE;
            m_SpinLock.Release(oldIrql);
        }
        else
        {
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_CALLBACKS | FXOBJECT_FLAGS_PASSIVE_DISPOSE;
        }
    }

    BOOLEAN
    IsPassiveCallbacks(
        __in BOOLEAN AcquireLock = TRUE
        )
    {
        BOOLEAN result;
        KIRQL   oldIrql = PASSIVE_LEVEL;

        if (AcquireLock)
        {
            m_SpinLock.Acquire(&oldIrql);
        }

        result = IsPassiveCallbacksLocked();

        if (AcquireLock)
        {
            m_SpinLock.Release(oldIrql);
        }

        return result;
    }

    static
    FxObject*
    _GetObjectFromHandle(
        __in    WDFOBJECT Handle,
        __inout PWDFOBJECT_OFFSET ObjectOffset
        )
    {
        ULONG_PTR handle, flags;

        handle = (ULONG_PTR) Handle;

        //
        // store the flags and then clear them off so we have a valid value
        //
        flags = FxHandleFlagMask & handle;
        handle &= ~FxHandleFlagMask;

        //
        // It is assumed the caller has already set the offset to zero
        //
        // *ObjectOffset = 0;

        //
        // We always apply the mask
        //
        handle = handle ^ FxHandleValueMask;

        if (flags & FxHandleFlagIsOffset)
        {
            //
            // The handle is a pointer to an offset value.  Return the offset
            // to the caller and then compute the object since the pointer
            // to the offset is part of the object we are returning.
            //
            *ObjectOffset = *(PWDFOBJECT_OFFSET) handle;

            return (FxObject*) (((PUCHAR) handle) - *ObjectOffset);
        }
        else
        {
            //
            // No offset, no verification.  We pass the FxObject as the handle
            //
            return (FxObject*) handle;
       }
    }

    //
    // Request to make ParentObject the parent for this object.
    //
    _Must_inspect_result_
    NTSTATUS
    AssignParentObject(
        __in FxObject* ParentObject
        );

    _Must_inspect_result_
    NTSTATUS
    AddContext(
        __in FxContextHeader *Header,
        __in PVOID* Context,
        __in PWDF_OBJECT_ATTRIBUTES Attributes
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        );

    __inline
    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        )
    {
        return m_Globals;
    }

    VOID
    MarkCommitted(
        VOID
        )
    {
        //
        // Since no client code is accessing the handle yet, we don't need to
        // acquire the object state lock to set the flag since this will be
        // the only thread touching m_ObjectFlags.
        //
        m_ObjectFlags |= FXOBJECT_FLAGS_COMMITTED;
    }

    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Device;
    }

    CfxDeviceBase*
    GetDeviceBase(
        VOID
        )
    {
        return m_DeviceBase;
    }

    VOID
    SetDeviceBase(
        __in CfxDeviceBase* DeviceBase
        )
    {
        m_DeviceBase = DeviceBase;
    }

    __inline
    FxContextHeader*
    GetContextHeader(
        VOID
        )
    {
        if (m_ObjectSize == 0)
        {
            return NULL;
        }
        else
        {
            return GET_CONTEXT_HEADER();
        }
    }

    VOID
    MarkNoDeleteDDI(
        __in FxObjectLockState State = ObjectLock
        )
    {
        if (State == ObjectLock)
        {
            KIRQL irql;

            m_SpinLock.Acquire(&irql);
            m_ObjectFlags |= FXOBJECT_FLAGS_NODELETEDDI;
            m_SpinLock.Release(irql);
        }
        else
        {
            m_ObjectFlags |= FXOBJECT_FLAGS_NODELETEDDI;
        }
    }

    VOID
    MarkDisposeOverride(
        __in FxObjectLockState State = ObjectLock
        )
    {
        if (State == ObjectLock)
        {
            KIRQL irql;

            m_SpinLock.Acquire(&irql);
            m_ObjectFlags |= FXOBJECT_FLAGS_DISPOSE_OVERRIDE;
            m_SpinLock.Release(irql);
        }
        else
        {
            m_ObjectFlags |= FXOBJECT_FLAGS_DISPOSE_OVERRIDE;
        }
    }

    BOOLEAN
    EarlyDispose(
        VOID
        );

    ULONG
    __inline
    AddRef(
        __in_opt   PVOID Tag = NULL,
        __in       LONG Line = 0,
        __in_opt   PCSTR File = NULL
        )
    {
        FxTagTracker* pTagTracker;
        ULONG c;

        c = InterlockedIncrement(&m_Refcnt);

        //
        // Catch the transition from 0 to 1.  Since the REF_OBJ starts off at 1,
        // we should never have to increment to get to this value.
        //
        ASSERT(c > 1);

        pTagTracker = GetTagTracker();
        if (pTagTracker != NULL)
        {
            pTagTracker->UpdateTagHistory(Tag, Line, File, TagAddRef, c);
        }

        return c;
    }

    VOID
    MarkPassiveDispose(
        __in FxObjectLockState State = ObjectLock
        )
    {
        //
        // Object which can have > passive level locks, but needs to be Dispose()'d
        // at passive level.  This means that the object's client cleanup
        // routines will also be guaranteed to run at passive.
        //
        if (State == ObjectLock)
        {
            KIRQL   oldIrql;

            m_SpinLock.Acquire(&oldIrql);
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_DISPOSE;
            m_SpinLock.Release(oldIrql);
        }
        else
        {
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_DISPOSE;
        }
    }

    //
    // Adds a reference to the parent object pointer if != NULL
    //
    _Must_inspect_result_
    FxObject*
    GetParentObjectReferenced(
        __in PVOID Tag
        );

protected:

    FxObject(
        __in WDFTYPE Type,
        __in USHORT Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObjectType ObjectType
        );    

    FxObjectDebugExtension*
    GetDebugExtension(
        VOID
        )
    {
        return CONTAINING_RECORD(this, FxObjectDebugExtension, AllocationStart);
    }

    PVOID
    __inline
    GetObjectHandleUnchecked(
        VOID
        )
    {
        //
        // We don't support offset handles in the base FxObject implementation.
        // Offsets are specialized to internal objects of an FxObject
        //
        if (m_ObjectSize > 0)
        {
            return _ToHandle(this);
        }
        else
        {
            return NULL;
        }
    }

    VOID
    AllocateTagTracker(
        __in WDFTYPE Type
        );

    BOOLEAN
    __inline
    IsTraceState(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_TRACE_STATE);
    }

    VOID
    __inline
    DestroyChildren(
        VOID
        )
    {
        PLIST_ENTRY ple;
        FxObject *pChild;

        while (!IsListEmpty(&m_ChildListHead))
        {
            ple = RemoveHeadList(&m_ChildListHead);

            pChild = CONTAINING_RECORD(ple, FxObject, m_ChildEntry);

            //
            // Mark entry as unlinked
            //
            InitializeListHead(&pChild->m_ChildEntry);

            //
            // Inform child object of destruction. Object may be gone after return.
            //
            pChild->ParentDeleteEvent();
        }
    }

    virtual
    VOID
    SelfDestruct(
        VOID
        )
    {
        delete this;
    }

    VOID
    DeleteEarlyDisposedObject(
        VOID
        );

};

#endif //_FXOBJECT_H_