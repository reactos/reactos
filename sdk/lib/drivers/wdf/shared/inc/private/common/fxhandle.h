/*++

Copyright (c) Microsoft Corporation

Module Name:

    fxhandle.h

Abstract:

    Private interfaces for Driver Frameworks handle functions.

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _WDFPHANDLE_H_
#define _WDFPHANDLE_H_

/**

 Framework Object Memory Layout (x86 32 bit)

 Note: The handle is XOR'ed with (-1) to scramble it

                        ---------------------- handle + sizeof(ContextSize)
                        |                    |
                        | Drivers Context    |
                        | Memory             |
                        |                    |
WDFOBJECT handle ->     ---------------------- this + sizeof(C++_OBJECT) + _extrasize + sizeof(FxContextHeader)
                        |                    |
                        | FxContextHeader    |
                        |                    |
                        ---------------------- this + sizeof(C++_OBJECT) + _extrasize (m_ObjectSize)
                        |                    |
                        |  C++ Object        |
                        |  "extra" memory    |
                        |                    |
                        |--------------------| this + sizeof(C++_OBJECT)
                        |                    |
                        |  C++ Object        |
                        |                    |
                        |                    |
Base Pool Allocation -> ---------------------- this
C++ this pointer

**/

struct FxContextHeader;

struct FxContextHeader {
    //
    // Backpointer to the object that this is a context for
    //
    FxObject* Object;

    //
    // Next context in the chain
    //
    FxContextHeader* NextHeader;

    //
    // Function to call when object is deleted
    //
    PFN_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanupCallback;

    //
    // Function to call when the object's memory is destroyed
    // when the last reference count goes to zero
    //
    PFN_WDF_OBJECT_CONTEXT_DESTROY EvtDestroyCallback;

    //
    // Type associated with this context
    //
    PCWDF_OBJECT_CONTEXT_TYPE_INFO ContextTypeInfo;

    //
    // Start of client's context
    //
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONG_PTR Context[1];
};

//
// We want all the way up to the aligned field, but not the field itself
//
#define FX_CONTEXT_HEADER_SIZE FIELD_OFFSET(FxContextHeader, Context)

#define COMPUTE_RAW_OBJECT_SIZE(_rawObjectSize) \
    ((USHORT) WDF_ALIGN_SIZE_UP(_rawObjectSize, MEMORY_ALLOCATION_ALIGNMENT))

//
// Computes the size required for a fixed size object plus any extra buffer space
// it requires.  The extra buffer space is aligned on a process natural boundary.
//
#define COMPUTE_OBJECT_SIZE(_rawObjectSize, _extraSize) \
   (COMPUTE_RAW_OBJECT_SIZE(_rawObjectSize) + (USHORT) WDF_ALIGN_SIZE_UP(_extraSize, MEMORY_ALLOCATION_ALIGNMENT))

//
// Gets size of the context associated with the specified attributes structure.
//
size_t
FxGetContextSize(
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes
    );

//
// Computes the total size of an object taking into account its fixed size,
// any additional size after the fixed, and an object header, so the memory looks
// like:
//
// Object
// additional optional memory
// WDF_HANDLE_HEADER
//
_Must_inspect_result_
NTSTATUS
FxCalculateObjectTotalSize2(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        USHORT RawObjectSize,
    __in        USHORT ExtraSize,
    __in        size_t ContextSize,
    __out       size_t* Total
    );

_Must_inspect_result_
NTSTATUS
FxCalculateObjectTotalSize(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        USHORT RawObjectSize,
    __in        USHORT ExtraSize,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out       size_t* Total
    );

PVOID
FxObjectHandleAlloc(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        POOL_TYPE PoolType,
    __in        size_t Size,
    __in        ULONG Tag,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        USHORT ExtraSize,
    __in        FxObjectType ObjectType
    );

VOID
FxContextHeaderInit(
    __in        FxContextHeader* Header,
    __in        FxObject* Object,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes
    );

PVOID
FxObjectAndHandleHeaderInit(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        PVOID AllocationStart,
    __in        USHORT ObjectSize,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        FxObjectType ObjectType
    );

VOID
__inline
FxObjectHandleCreate(
    __in  FxObject* Object,
    __out PWDFOBJECT Handle
    )
{
#if FX_SUPER_DBG
    if (Object->GetDriverGlobals()->FxVerifierHandle) {






        ASSERT(Object->GetObjectSize() > 0);
        ASSERT(Object == Object->GetContextHeader()->Object);
    }
#endif

    ASSERT((((ULONG_PTR) Object) & FxHandleFlagMask) == 0x0);
    *Handle = Object->GetObjectHandle();
}

/*++

Routine Description:

    Retrieves the object pointer from the handle if valid.

    This does not change an objects reference count.

Arguments:

    handle - The object handle created by WdfObjectCreateHandle

    type - Type of the object from FxTypes.h

    ppObj - Pointer to location to store the returned object.

Returns:

    NTSTATUS

--*/

VOID
FxObjectHandleGetPtrQI(
    __in FxObject* Object,
    __out PVOID* PPObject,
    __in WDFOBJECT Handle,
    __in WDFTYPE Type,
    __in WDFOBJECT_OFFSET Offset
    );

_Must_inspect_result_
NTSTATUS
FxObjectAllocateContext(
    __in        FxObject*               Object,
    __in        PWDF_OBJECT_ATTRIBUTES  Attributes,
    __in        BOOLEAN                 AllowCallbacksOnly,
    __deref_opt_out PVOID*              Context
    );

__inline
BOOLEAN
FxObjectCheckType(
    __in FxObject* Object,
    __in WDFTYPE  Type
    )
/*++

Routine Description:
    Checks if the FxObject is of a the Type.

Arguments:
    FxObject - the object being checked
    Type - The type value to be checked

Returns:
    TRUE if the Object is of the type 'Type'
    FALSE otherwise
  --*/
{
    NTSTATUS status;
    PVOID tmpObject;
    FxQueryInterfaceParams params = {&tmpObject, Type, 0};

    //
    // Do a quick non virtual call for the type and only do the slow QI if
    // the first types do not match
    //

    if (Object->GetType() == Type) {
        return TRUE;
    }

    status = Object->QueryInterface(&params);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;

}

__inline
VOID
FxObjectHandleGetPtr(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFOBJECT Handle,
    __in WDFTYPE Type,
    __out PVOID* PPObject
    )
/*++

Routine Description:
    Converts an externally facing WDF handle into its internal object.

Arguments:
    FxDriverGlobals - caller's globals
    Handle - handle to convert into an object
    Type - required type of the underlying object
    PPObject - pointer to receive the underlying object

  --*/
{
    WDFOBJECT_OFFSET offset;
    FxObject* pObject;

    if (Handle == NULL) {





        FxVerifierBugCheck(FxDriverGlobals,
                           WDF_INVALID_HANDLE,
                           (ULONG_PTR) Handle,
                           Type);

        /* NOTREACHED */
        return;
    }

    offset = 0;
    pObject = FxObject::_GetObjectFromHandle(Handle, &offset);

    //
    // The only DDI you can call on an object which has a ref count of zero is
    // WdfObjectGetTypedContextWorker().
    //
    ASSERT(pObject->GetRefCnt() > 0);

    //
    // Do a quick non virtual call for the type and only do the slow QI if
    // the first types do not match
    //
    if (pObject->GetType() == Type) {
        *PPObject = pObject;
        ASSERT(offset == 0x0);
        return;
    }
    else {
        FxObjectHandleGetPtrQI(pObject, PPObject, Handle, Type, offset);
    }
}

__inline
VOID
FxObjectHandleGetPtrOffset(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFOBJECT Handle,
    __in WDFTYPE Type,
    __out PVOID* PPObject,
    __out PWDFOBJECT_OFFSET Offset
    )
/*++

Routine Description:
    This function  probably should be removed and the optional Offset paramter
    moved into the signature for  FxObjectHandleGetPtr(). The distinction
    between these 2 functions was important when both of them were *not*
    FORCEINLINE functions (since there was an additional parameter to push onto
    the stack).  Now that they are both __inlined, there is no longer a
    parameter to push, eliminating this optimization.

Arguments:
    FxDriverGlobals - caller's globals
    Handle - handle to convert into an object
    Type - required type of the underlying object
    PPObject - pointer to receive the underlying object
    Offset - offset into the object which the handle resided.  Nearly all objects
        will have a zero offset

  --*/
{
    FxObject* pObject;

    if (Handle == NULL) {





        FxVerifierBugCheck(FxDriverGlobals,
                           WDF_INVALID_HANDLE,
                           (ULONG_PTR) Handle,
                           Type);

        /* NOTREACHED */
        return;
    }

    *Offset = 0;
    pObject = FxObject::_GetObjectFromHandle(Handle, Offset);

    //
    // The only DDI you can call on an object which has a ref count of zero is
    // WdfObjectGetTypedContextWorker().
    //
    ASSERT(pObject->GetRefCnt() > 0);

    //
    // Do a quick non virtual call for the type and only do the slow QI if
    // the first types do not match
    //
    if (pObject->GetType() == Type) {
        *PPObject = pObject;
        ASSERT(*Offset == 0x0);
        return;
    }
    else {
        FxObjectHandleGetPtrQI(pObject, PPObject, Handle, Type, *Offset);
    }
}

VOID
__inline
FxObjectHandleGetPtrAndGlobals(
    __in  PFX_DRIVER_GLOBALS CallersGlobals,
    __in  WDFOBJECT Handle,
    __in  WDFTYPE   Type,
    __out PVOID*    PPObject,
    __out PFX_DRIVER_GLOBALS* ObjectGlobals
    )
/*++

Routine Description:
    Converts an externally facing WDF handle into its internal object.

Arguments:
    FxDriverGlobals - caller's globals
    Handle - handle to convert into an object
    Type - required type of the underlying object
    PPObject - pointer to receive the underlying object
    ObjectGlobals - pointer to receive the underlying object's globals.

  --*/
{
    //
    // All FX_TYPEs except for IFX_TYPE_MEMORY derive from FxObject, so our cast
    // below will work with all types but IFX_TYPE_MEMORY (in which case the caller
    // should call FxObjectHandleGetPtr and then get the globals on their own
    // from the IFxMemory interface).
    //
    ASSERT(Type != IFX_TYPE_MEMORY);

    FxObjectHandleGetPtr(CallersGlobals,
                         Handle,
                         Type,
                         PPObject);

    *ObjectGlobals = ((FxObject*) (*PPObject))->GetDriverGlobals();
}

VOID
__inline
FxObjectHandleGetGlobals(
    __in  PFX_DRIVER_GLOBALS CallersGlobals,
    __in  WDFOBJECT Handle,
    __out PFX_DRIVER_GLOBALS* ObjectGlobals
    )
/*++

Routine Description:
    Converts an externally facing WDF handle into its internal object and
    returns its globals.

Arguments:
    FxDriverGlobals - caller's globals
    Handle - handle to convert into an object
    ObjectGlobals - pointer to receive the underlying object's globals.

  --*/
{
    PVOID pObject;

    FxObjectHandleGetPtrAndGlobals(CallersGlobals,
                                   Handle,
                                   FX_TYPE_OBJECT,
                                   &pObject,
                                   ObjectGlobals);

    UNREFERENCED_PARAMETER(pObject);
}

#endif // _WDFPHANDLE_H_
