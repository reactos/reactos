/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxObject.hpp

Abstract:

    This is the C++ header for the FxObject

Author:




Revision History:


        Made mode agnostic

        IMPORTANT: Common code must call Initialize method of
        FxObject before using it

        Cannot put CreateAndInitialize method on this class as it
        cannot be instantiated


--*/

#ifndef _FXOBJECT_H_
#define _FXOBJECT_H_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxObject.hpp.tmh"
#endif

}

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

//
// The type itself is aligned, but the pointer is not b/c those interested in the
// offset do not need it to be aligned.
//
// The offset is aligned on an 8 byte boundary so that we have the lower 3 bits
// of the low byte to use for a bit field
//
typedef DECLSPEC_ALIGN(8) USHORT WDFOBJECT_OFFSET_ALIGNED;

typedef USHORT WDFOBJECT_OFFSET, *PWDFOBJECT_OFFSET;

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

// begin_wpp config
// CUSTOM_TYPE(FxObjectState, ItemEnum(FxObjectState));
// CUSTOM_TYPE(FxObjectDroppedEvent, ItemEnum(FxObjectDroppedEvent));
// end_wpp

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
    // Signature lives after all the fields to avoid byte padding if it is
    // first on 64 bit systems.
    //
    ULONG Signature;

    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONG AllocationStart[1];
};

enum FxObjectDebugExtensionValues {
    FxObjectDebugExtensionSize  = FIELD_OFFSET(FxObjectDebugExtension, AllocationStart),
    FxObjectDebugExtensionSignature = 'DOxF',
};

class UfxObject;

class FxObject {

    friend UfxObject; //UMDF object wrapper

    friend FxDisposeList;
    friend VOID GetTriageInfo(VOID);

private:

#if FX_CORE_MODE==FX_CORE_USER_MODE
#ifndef INLINE_WRAPPER_ALLOCATION
    PVOID         m_COMWrapper;
#endif
#endif

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
        if (m_ObjectFlags & FXOBJECT_FLAGS_TRACE_STATE) {
            DoTraceLevelMessage(
                m_Globals, TRACE_LEVEL_VERBOSE, TRACINGOBJECT,
                "Object %p, WDFOBJECT %p transitioning from %!FxObjectState! to "
                "%!FxObjectState!", this, GetObjectHandleUnchecked(),
                m_ObjectState, NewState);
            if (IsDebug()) {
                FxObjectDebugExtension* pExtension;

                pExtension = GetDebugExtension();
                pExtension->StateHistory[
                    InterlockedIncrement(&pExtension->StateHistoryIndex)] =
                    (BYTE) NewState;
            }
        }

        m_ObjectState = (USHORT) NewState;
    }

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
        if (Object->IsDebug()) {
            return _GetDebugBase(Object);
        }
        else {
            return (PVOID) Object;
        }
    }

    VOID
    AllocateTagTracker(
        __in WDFTYPE Type
        );

    virtual
    VOID
    SelfDestruct(
        VOID
        )
    {
        delete this;
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
        if (m_ObjectSize > 0) {
            return _ToHandle(this);
        }
        else {
            return NULL;
        }
    }

    VOID
    __inline
    DestroyChildren(
        VOID
        )
    {
        PLIST_ENTRY ple;
        FxObject *pChild;

        while (!IsListEmpty(&m_ChildListHead)) {
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

public:

#ifdef INLINE_WRAPPER_ALLOCATION

#if FX_CORE_MODE==FX_CORE_USER_MODE
    static
    USHORT
    GetWrapperSize(
        );

    virtual
    PVOID
    GetCOMWrapper(
        ) = 0;

#else
    static
    USHORT
    __inline
    GetWrapperSize(
        )
    {
        return 0;
    }

#endif

#else

#if FX_CORE_MODE==FX_CORE_USER_MODE
    PVOID GetCOMWrapper(){ return m_COMWrapper; }

    void SetCOMWrapper(__drv_aliasesMem PVOID Wrapper){ m_COMWrapper = Wrapper; }
#endif

#endif

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

    VOID
    SetNoContextHeader(
        VOID
        )
    {
        m_ObjectSize = 0;
    }

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

        if (flags & FxHandleFlagIsOffset) {
            //
            // The handle is a pointer to an offset value.  Return the offset
            // to the caller and then compute the object since the pointer
            // to the offset is part of the object we are returning.
            //
            *ObjectOffset = *(PWDFOBJECT_OFFSET) handle;

            return (FxObject*) (((PUCHAR) handle) - *ObjectOffset);
        }
        else {
            //
            // No offset, no verification.  We pass the FxObject as the handle
            //
            return (FxObject*) handle;
       }
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

    static
    VOID
    __inline
    _ReferenceActual(
        __in        WDFOBJECT Object,
        __in_opt    PVOID Tag,
        __in        LONG Line,
        __in        PSTR File
        )
    {
        FxObject* pObject;
        WDFOBJECT_OFFSET offset;

        offset = 0;
        pObject = FxObject::_GetObjectFromHandle(Object, &offset);

        if (offset == 0) {
            pObject->AddRef(Tag, Line, File);
        }
        else {
            pObject->AddRefOverride(offset, Tag, Line, File);
        }
    }

    static
    VOID
    __inline
    _DereferenceActual(
        __in        WDFOBJECT Object,
        __in_opt    PVOID Tag,
        __in        LONG Line,
        __in        PSTR File
        )
    {
        FxObject* pObject;
        WDFOBJECT_OFFSET offset;

        offset = 0;
        pObject = FxObject::_GetObjectFromHandle(Object, &offset);

        if (offset == 0) {
            pObject->Release(Tag, Line, File);
        }
        else {
            pObject->ReleaseOverride(offset, Tag, Line, File);
        }
    }

    __inline
    FxContextHeader*
    GetContextHeader(
        VOID
        )
    {
        if (m_ObjectSize == 0) {
            return NULL;
        }
        else {
            return GET_CONTEXT_HEADER();
        }
    }

    __inline
    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        )
    {
        return m_Globals;
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
        if (IsDebug()) {
            return CONTAINING_RECORD(this,
                                     FxObjectDebugExtension,
                                     AllocationStart)->TagTracker;
        }
        else {
            return NULL;
        }
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

    static
    PVOID
    _GetDebugBase(
        __in FxObject* Object
        )
    {
        return CONTAINING_RECORD(Object, FxObjectDebugExtension, AllocationStart);
    }

    __inline
    VOID
    CallCleanup(
        VOID
        )
    {
        if (m_ObjectFlags & FXOBJECT_FLAGS_HAS_CLEANUP) {
            CallCleanupCallbacks();
        }
    }

    ULONG
    __inline
    AddRef(
        __in_opt   PVOID Tag = NULL,
        __in       LONG Line = 0,
        __in_opt   PSTR File = NULL
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
        if (pTagTracker != NULL) {
            pTagTracker->UpdateTagHistory(Tag, Line, File, TagAddRef, c);
        }

        return c;
    }

    virtual
    ULONG
    Release(
        __in_opt    PVOID Tag = NULL,
        __in        LONG Line = 0,
        __in_opt    PSTR File = NULL
        )
    {
        FxTagTracker* pTagTracker;
        ULONG c;

        pTagTracker = GetTagTracker();
        if (pTagTracker != NULL) {
            pTagTracker->UpdateTagHistory(Tag, Line, File, TagRelease, m_Refcnt - 1);
        }

        c = InterlockedDecrement(&m_Refcnt);

        if (c == 0) {
            FinalRelease();
        }

        return c;
    }

    virtual
    ULONG
    AddRefOverride(
        __in        WDFOBJECT_OFFSET Offset,
        __in_opt    PVOID Tag = NULL,
        __in        LONG Line = 0,
        __in_opt    PSTR File = NULL
        )
    {
        UNREFERENCED_PARAMETER(Offset);

        return AddRef(Tag, Line, File);
    }

    virtual
    ULONG
    ReleaseOverride(
        __in        WDFOBJECT_OFFSET Offset,
        __in_opt    PVOID Tag = NULL,
        __in        LONG Line = 0,
        __in_opt    PSTR File = NULL
        )
    {
        UNREFERENCED_PARAMETER(Offset);

        return Release(Tag, Line, File);
    }

    _Must_inspect_result_
    virtual
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        );

    VOID
    MarkTraceState(
        VOID
        )
    {
        m_ObjectFlags |= FXOBJECT_FLAGS_TRACE_STATE;
    }

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
    TraceDroppedEvent(
        __in FxObjectDroppedEvent Event
        )
    {
        if (IsTraceState()) {
            DoTraceLevelMessage(
                m_Globals, TRACE_LEVEL_INFORMATION, TRACINGOBJECT,
                "Object %p, WDFOBJECT %p, state %!FxObjectState! dropping event"
                " %!FxObjectDroppedEvent!",
                this, GetObjectHandleUnchecked(), m_ObjectState, Event);
        }
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
        if (State == ObjectLock) {
            KIRQL   oldIrql;

            m_SpinLock.Acquire(&oldIrql);
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_DISPOSE;
            m_SpinLock.Release(oldIrql);
        }
        else {
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_DISPOSE;
        }
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
        if (State == ObjectLock) {
            KIRQL   oldIrql;

            m_SpinLock.Acquire(&oldIrql);
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_CALLBACKS | FXOBJECT_FLAGS_PASSIVE_DISPOSE;
            m_SpinLock.Release(oldIrql);
        }
        else {
            m_ObjectFlags |= FXOBJECT_FLAGS_PASSIVE_CALLBACKS | FXOBJECT_FLAGS_PASSIVE_DISPOSE;
        }
    }

    VOID
    MarkForceDisposeThread(
        __in FxObjectLockState State = ObjectLock
        )
    {
        //
        // Object must always be disposed in a separate thread
        // to allow waiting for some outstanding async
        // operation to complete.
        //
        if (State == ObjectLock) {
            KIRQL   oldIrql;

            m_SpinLock.Acquire(&oldIrql);
            m_ObjectFlags |= FXOBJECT_FLAGS_FORCE_DISPOSE_THREAD;
            m_SpinLock.Release(oldIrql);
        }
        else {
            m_ObjectFlags |= FXOBJECT_FLAGS_FORCE_DISPOSE_THREAD;
        }
    }

    BOOLEAN
    IsPassiveCallbacks(
        __in BOOLEAN AcquireLock = TRUE
        )
    {
        BOOLEAN result;
        KIRQL   oldIrql = PASSIVE_LEVEL;

        if (AcquireLock) {
            m_SpinLock.Acquire(&oldIrql);
        }

        result = IsPassiveCallbacksLocked();

        if (AcquireLock) {
            m_SpinLock.Release(oldIrql);
        }

        return result;
    }

    BOOLEAN
    IsPassiveDispose(
        __in BOOLEAN AcquireLock = TRUE
        )
    {
        BOOLEAN result;
        KIRQL   oldIrql = PASSIVE_LEVEL;

        if (AcquireLock) {
            m_SpinLock.Acquire(&oldIrql);
        }

        result = IsPassiveDisposeLocked();

        if (AcquireLock) {
            m_SpinLock.Release(oldIrql);
        }

        return result;
    }

    BOOLEAN
    IsForceDisposeThread(
        __in BOOLEAN AcquireLock = TRUE
        )
    {
        BOOLEAN result;
        KIRQL   oldIrql = PASSIVE_LEVEL;

        if (AcquireLock) {
            m_SpinLock.Acquire(&oldIrql);
        }

        result = IsForceDisposeThreadLocked();

        if (AcquireLock) {
            m_SpinLock.Release(oldIrql);
        }

        return result;
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
    MarkDisposeOverride(
        __in FxObjectLockState State = ObjectLock
        )
    {
        if (State == ObjectLock) {
            KIRQL irql;

            m_SpinLock.Acquire(&irql);
            m_ObjectFlags |= FXOBJECT_FLAGS_DISPOSE_OVERRIDE;
            m_SpinLock.Release(irql);
        }
        else {
            m_ObjectFlags |= FXOBJECT_FLAGS_DISPOSE_OVERRIDE;
        }
    }

    VOID
    MarkNoDeleteDDI(
        __in FxObjectLockState State = ObjectLock
        )
    {
        if (State == ObjectLock) {
            KIRQL irql;

            m_SpinLock.Acquire(&irql);
            m_ObjectFlags |= FXOBJECT_FLAGS_NODELETEDDI;
            m_SpinLock.Release(irql);
        }
        else {
            m_ObjectFlags |= FXOBJECT_FLAGS_NODELETEDDI;
        }
    }

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

    BOOLEAN
    EarlyDispose(
        VOID
        );

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

    //
    // Request that this Object be removed from the child association
    // list for its parent
    //
    _Must_inspect_result_
    NTSTATUS
    RemoveParentAssignment(
        VOID
        );

    //
    // Adds a reference to the parent object pointer if != NULL
    //
    _Must_inspect_result_
    FxObject*
    GetParentObjectReferenced(
        __in PVOID Tag
        );

    //
    // This is public to allow debug code to assert that
    // an object has been properly disposed either through
    // calling DeleteObject, or being disposed by its parent.
    //
    BOOLEAN
    IsDisposed(
        VOID
        )
    {
        KIRQL   oldIrql;
        BOOLEAN disposed;

        if (m_Globals->FxVerifierOn &&
            m_Globals->FxVerifierHandle) {
            m_SpinLock.Acquire(&oldIrql);

            if (m_ObjectState == FxObjectStateCreated) {
                disposed = FALSE;
            }
            else {
                //
                // Parent is disposing us, or we are being disposed
                //
                disposed = TRUE;
            }

            m_SpinLock.Release(oldIrql);

            return disposed;
        }
        else {
            return TRUE;
        }
    }

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
        if (FxDriverGlobals->IsPoolTrackingOn()) {
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

    //
    // Implementation for WdfObjectQuery
    //
    _Must_inspect_result_
    static
    NTSTATUS
    _ObjectQuery(
        _In_    FxObject* Object,
        _In_    CONST GUID* Guid,
        _In_    ULONG QueryBufferLength,
        _Out_writes_bytes_(QueryBufferLength)
                PVOID QueryBuffer
        );

protected:
    VOID
    DeleteEarlyDisposedObject(
        VOID
        );

private:
    VOID
    FinalRelease(
        VOID
        );

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

    BOOLEAN
    IsPassiveCallbacksLocked(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_ObjectFlags, FXOBJECT_FLAGS_PASSIVE_CALLBACKS);
    }

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
        if (IsForceDisposeThreadLocked()) {
            return TRUE;
        }
        else if (IsPassiveDisposeLocked()) {
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
            if (PreviousIrql == NULL) {                  // case 1
                if (Mx::MxGetCurrentIrql() != PASSIVE_LEVEL) {
                    return TRUE;
                }
            }
            else if (*PreviousIrql != PASSIVE_LEVEL) {   // case 2
                return TRUE;
            }
        }

        return FALSE;
    }

    VOID
    ParentDeleteEvent(
        VOID
        );

    BOOLEAN
    PerformEarlyDispose(
        VOID
        );

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    PerformEarlyDisposeWorkerAndUnlock(
        __in __drv_restoresIRQL KIRQL OldIrql,
        __in                    BOOLEAN CanDefer
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
    DeleteWorkerAndUnlock(
        __in __drv_restoresIRQL KIRQL OldIrql,
        __in                    BOOLEAN CanDefer
        );

    VOID
    QueueDeferredDisposeLocked(
        __in FxObjectState NewDeferedState
        );

    VOID
    DeferredDisposeWorkItem(
        VOID
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

    _Must_inspect_result_
    NTSTATUS
    RemoveParentAssociation(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    AddChildObjectInternal(
        __in FxObject* ChildObject
        );

   _Must_inspect_result_
   NTSTATUS
    RemoveChildObjectInternal(
        __in FxObject* ChildObject
        );

    VOID
    ProcessDestroy(
        VOID
        );

    VOID
    CallCleanupCallbacks(
        VOID
        );
};

#endif // _FXOBJECT_H_
