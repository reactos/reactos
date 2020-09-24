/*++

Copyright (c) Microsoft Corporation

Module Name:

    HandleApi.cpp

Abstract:

    This module implements the driver frameworks handle functions.

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "HandleAPI.tmh"
#endif

}

size_t
FxGetContextSize(
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes
    )
/*++

Routine Description:
    Get a context size from an object's attributes settings.

Arguments:
    Attributes - attributes which will describe the size requirements for the
        associated context.

Return Value:
    Size requirements for the  associated context.

  --*/
{
    size_t contextSize = 0;

    if (Attributes != NULL) {
        if (Attributes->ContextTypeInfo != NULL) {
            if (Attributes->ContextSizeOverride != 0) {
                contextSize = Attributes->ContextSizeOverride;
            }
            else {
                contextSize = Attributes->ContextTypeInfo->ContextSize;
            }
        }
    }

    return contextSize;
}

_Must_inspect_result_
NTSTATUS
FxCalculateObjectTotalSize2(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        USHORT RawObjectSize,
    __in        USHORT ExtraSize,
    __in        size_t ContextSize,
    __out       size_t* Total
    )
/*++

Routine Description:
    Calculates the size of an allocation for an FxObject that will contain the
    object, its initial context and any addtional headers.

Arguments:
    FxDriverGlobals - driver's globals
    RawObjectSize - the size of the FxObject derived object
    ExtraSize - additional size required by the derived object
    ContextSize - Size requirements for the associated context (see FxGetContextSize() above).
    Total - pointer which will receive the final size requirement

Return Value:
    NT_SUCCESS on success, !NT_SUCCESS on failure

  --*/
{
    NTSTATUS status;

    *Total = 0;

    status = RtlSizeTAdd(
        WDF_ALIGN_SIZE_UP(COMPUTE_OBJECT_SIZE(RawObjectSize, ExtraSize), MEMORY_ALLOCATION_ALIGNMENT),
        FX_CONTEXT_HEADER_SIZE,
        Total
        );

    if (NT_SUCCESS(status)) {
        if (ContextSize != 0) {
            size_t alignUp;

            alignUp = ALIGN_UP(ContextSize, PVOID);

            //
            // overflow after aligning up to a pointer boundary;
            //
            if (alignUp < ContextSize) {
                return STATUS_INTEGER_OVERFLOW;
            }

            status = RtlSizeTAdd(*Total, alignUp, Total);
        }
    }

    if (NT_SUCCESS(status) && FxDriverGlobals->IsObjectDebugOn()) {
        //
        // Attempt to add in the debug extension
        //
        status = RtlSizeTAdd(*Total,
                             FxObjectDebugExtensionSize,
                             Total);
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGOBJECT,
            "Size overflow, object size 0x%x, extra object size 0x%x, "
            "context size 0x%I64x, %!STATUS!",
            RawObjectSize, ExtraSize, ContextSize, status);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxCalculateObjectTotalSize(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        USHORT RawObjectSize,
    __in        USHORT ExtraSize,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out       size_t* Total
    )
{
    return FxCalculateObjectTotalSize2(FxDriverGlobals,
                                       RawObjectSize,
                                       ExtraSize,
                                       FxGetContextSize(Attributes),
                                       Total);
}

PVOID
FxObjectHandleAlloc(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        POOL_TYPE PoolType,
    __in        size_t Size,
    __in        ULONG Tag,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        USHORT ExtraSize,
    __in        FxObjectType ObjectType
    )
/*++

Routine Description:
    Allocates an FxObject derived object, it's associated context, and any
    framework required headers and footers.

Arguments:
    FxDriverGlobals - caller's globals
    PoolType - type of pool to be used in allocating the object's memory
    Size - size of the object (as passed to operator new() by the compiler)
    Tag - tag to use when allocating the object's memory
    Attributes - attributes which describe the context to be associated with the
        object
    ExtraSize - any addtional storage required by the object itself
    ObjectType - the type (internal or external) of object being allocated.  An
        internal object is a worker object which will never have an associated
        type or be Commit()'ed into an external object handle that the client
        driver will see.

Return Value:
    A valid pointer on success, NULL otherwise

  --*/
{
    PVOID   blob;
    size_t  totalSize;
    NTSTATUS status;

    if (Tag == 0) {
        Tag = FxDriverGlobals->Tag;
        ASSERT(Tag != 0);
    }

    if (ObjectType == FxObjectTypeInternal) {
        //
        // An internal object might need the debug extension size added to it,
        // but that's it.  No extra size, no FxContextHeader.
        //
        if (FxDriverGlobals->IsObjectDebugOn()) {
            status = RtlSizeTAdd(Size,
                                 FxObjectDebugExtensionSize,
                                 &totalSize);
        }
        else {
            totalSize = Size;
            status = STATUS_SUCCESS;
        }
    }
    else {
        status = FxCalculateObjectTotalSize(FxDriverGlobals,
                                            (USHORT) Size,
                                            ExtraSize,
                                            Attributes,
                                            &totalSize);
    }

    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    blob = FxPoolAllocateWithTag(FxDriverGlobals, PoolType, totalSize, Tag);

    if (blob != NULL) {
        blob = FxObjectAndHandleHeaderInit(
            FxDriverGlobals,
            blob,
            COMPUTE_OBJECT_SIZE((USHORT) Size, ExtraSize),
            Attributes,
            ObjectType
            );
    }

    return blob;
}

VOID
FxContextHeaderInit(
    __in        FxContextHeader* Header,
    __in        FxObject* Object,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes
    )
/*++

Routine Description:
    Initializes a context header which describes a typed context.

Arguments:
    Header - the header to initialize
    Object - the object on which the context is being associated with
    Attributes - description of the typed context

  --*/
{
    RtlZeroMemory(Header, FX_CONTEXT_HEADER_SIZE);

    Header->Object = Object;

    if (Attributes != NULL) {
        if (Attributes->ContextTypeInfo != NULL) {
            size_t contextSize;

            if (Attributes->ContextSizeOverride != 0) {
                contextSize = Attributes->ContextSizeOverride;

            }
            else {
                contextSize = Attributes->ContextTypeInfo->ContextSize;
            }

            //
            // Zero initialize the entire context
            //
            RtlZeroMemory(&Header->Context[0], ALIGN_UP(contextSize, PVOID));
        }

        Header->ContextTypeInfo = Attributes->ContextTypeInfo;
    }
}

PVOID
FxObjectAndHandleHeaderInit(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        PVOID AllocationStart,
    __in        USHORT ObjectSize,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        FxObjectType ObjectType
    )
/*++

Routine Description:
    Initialize the object and its associated context.

Arguments:
    FxDriverGlobals - caller's globals
    AllocationStart - start of the memory block allocated to represent the object.
        This will not be the same as the pointer which represents the address of
        the object itself in memory
    ObjectSize - size of the object
    Attributes - description of its context
    ObjectType - type (internal or external) of object being initialized

Return Value:
    The address of the object withing AllocationStart

  --*/

{
    FxObject* pObject;

    if (FxDriverGlobals->IsObjectDebugOn()) {
        FxObjectDebugExtension* pExtension;

        pExtension = (FxObjectDebugExtension*) AllocationStart;

        RtlZeroMemory(pExtension, FxObjectDebugExtensionSize);
        pExtension->Signature = FxObjectDebugExtensionSignature;

        pObject = (FxObject*) &pExtension->AllocationStart[0];
    }
    else {
        pObject = (FxObject*) AllocationStart;
    }

    if (ObjectType == FxObjectTypeExternal) {
        FxContextHeaderInit(
            (FxContextHeader*) WDF_PTR_ADD_OFFSET(pObject, ObjectSize),
            pObject,
            Attributes
            );
    }

    return pObject;
}

VOID
FxObjectHandleGetPtrQI(
    __in    FxObject* Object,
    __out   PVOID* PPObject,
    __in    WDFOBJECT Handle,
    __in    WDFTYPE Type,
    __in    WDFOBJECT_OFFSET Offset
    )
/*++

Routine Description:
    Worker function for FxObjectHandleGetPtrXxx which will call
    FxObject::QueryInterface when the Type does not match the object's built in
    Type.

Arguments:
    Object - object to query
    PPObject - pointer which will recieve the queried for object
    Handle - handle which the caller passed to the framework
    Type - required object type
    Offset - offset of the handle within the object.  Nearly all handles will have
        a zero object.

  --*/
{
    FxQueryInterfaceParams params = { PPObject, Type, Offset };
    NTSTATUS status;

    *PPObject = NULL;

    status = Object->QueryInterface(&params);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            Object->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Object Type Mismatch, Handle 0x%p, Type 0x%x, "
            "Obj 0x%p, SB 0x%x",
            Handle, Type, Object, Object->GetType());

        FxVerifierBugCheck(Object->GetDriverGlobals(),
                           WDF_INVALID_HANDLE,
                           (ULONG_PTR) Handle,
                           Type);

        /* NOTREACHED */
        return;
    }
}

_Must_inspect_result_
NTSTATUS
FxObjectAllocateContext(
    __in        FxObject*               Object,
    __in        PWDF_OBJECT_ATTRIBUTES  Attributes,
    __in        BOOLEAN                 AllowCallbacksOnly,
    __deref_opt_out PVOID*              Context
    )
/*++

Routine Description:
    Allocates an additional context on the object if it is in the correct state.

Arguments:
    Object - object on which to add a context
    Attributes - attributes which describe the type and size of the new context
    AllowEmptyContext -TRUE to allow logic to allocate the context's header only.
    Context - optional pointer which will recieve the new context

Return Value:
    STATUS_SUCCESS upon success, STATUS_OBJECT_NAME_EXISTS if the context type
    is already attached to the handle, and !NT_SUCCESS on failure

  --*/
{
    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    NTSTATUS            status;
    FxContextHeader *   header;
    size_t              size;
    BOOLEAN             relRef;
    ULONG               flags;

    fxDriverGlobals = Object->GetDriverGlobals();
    header = NULL;
    relRef = FALSE;

    //
    // Init validation flags.
    //
    flags = FX_VALIDATE_OPTION_ATTRIBUTES_REQUIRED;
    if (fxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11)) {
        flags |= FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED;
    }

    //
    // Basic validations.
    //
    status = FxValidateObjectAttributes(fxDriverGlobals, Attributes, flags);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Check for context type!
    //
    if (Attributes->ContextTypeInfo == NULL && AllowCallbacksOnly == FALSE) {
        status = STATUS_OBJECT_NAME_INVALID;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGHANDLE,
            "Attributes %p ContextTypeInfo is NULL, %!STATUS!",
            Attributes, status);
        goto Done;
    }

    Object->ADDREF(&status);
    relRef = TRUE;

    //
    // By passing 0's for raw object size and extra size, we can compute the
    // size of only the header and its contents.
    //
    status = FxCalculateObjectTotalSize(fxDriverGlobals, 0, 0, Attributes, &size);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    header = (FxContextHeader*)
                FxPoolAllocate(fxDriverGlobals, NonPagedPool, size);

    if (header == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    FxContextHeaderInit(header, Object, Attributes);

    status = Object->AddContext(header, Context, Attributes);

    //
    // STATUS_OBJECT_NAME_EXISTS will not fail NT_SUCCESS, so check
    // explicitly for STATUS_SUCCESS.
    //
    if (status != STATUS_SUCCESS) {
        FxPoolFree(header);
    }

Done:

    if (relRef) {
        Object->RELEASE(&status);
    }

    return status;
}

// extern "C" all APIs
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfObjectAllocateContext)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Handle,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __deref_opt_out
    PVOID* Context
    )
/*++

Routine Description:
    Allocates an additional context on the handle if the object is in the
    correct state

Arguments:
    Handle - handle on which to add a context
    Attributes - attributes which describe the type and size of the new context
    Context - optional pointer which will recieve the new context

Return Value:
    STATUS_SUCCESS upon success, STATUS_OBJECT_NAME_EXISTS if the context type
    is already attached to the handle, and !NT_SUCCESS on failure

  --*/
{
    DDI_ENTRY_IMPERSONATION_OK();

    NTSTATUS            status;
    FxObject*           object;
    WDFOBJECT_OFFSET    offset;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), Handle);

    //
    // No need to call FxObjectHandleGetPtr( , , FX_TYPE_OBJECT) because
    // we assume that the object handle will point back to an FxObject.  (The
    // call to FxObjectHandleGetPtr will just make needless virtual call into
    // FxObject anyways).
    //
    offset = 0;
    object = FxObject::_GetObjectFromHandle(Handle, &offset);

    if (offset != 0) {



        status = STATUS_OBJECT_PATH_INVALID;
        DoTraceLevelMessage(
            object->GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGHANDLE,
            "WDFHANDLE %p cannot have contexts added to it, %!STATUS!",
            Handle, status);
        goto Done;
    }

    //
    // Internal helper function does the rest of the work.
    //
    status = FxObjectAllocateContext(object, Attributes, FALSE, Context);

Done:
    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL+1)
WDFAPI
PVOID
FASTCALL
WDFEXPORT(WdfObjectGetTypedContextWorker)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Handle,
    __in
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    )
/*++

Routine Description:
    Retrieves the requested type from a handle

Arguments:
    Handle - the handle to retrieve the context from
    TypeInfo - global constant pointer which describes the type.  Since the pointer
        value is unique in all of kernel space, we will perform a pointer compare
        instead of a deep structure compare

Return Value:
    A valid context pointere or NULL.  NULL is not a failure, querying for a type
    not associated with the handle is a legitimate operation.

  --*/
{
    DDI_ENTRY_IMPERSONATION_OK();

    FxContextHeader* pHeader;
    FxObject* pObject;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFOBJECT_OFFSET offset;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), Handle);

    //
    // Do not call FxObjectHandleGetPtr( , , FX_TYPE_OBJECT) because this is a
    // hot spot / workhorse function that should be as efficient as possible.
    //
    // A call to FxObjectHandleGetPtr would :
    // 1)  invoke a virtual call to QueryInterface
    //
    // 2)  ASSERT that the ref count of the object is > zero.  Since this is one
    //     of the few functions that can be called in EvtObjectDestroy where the
    //     ref count is zero, that is not a good side affect.
    //
    offset = 0;
    pObject = FxObject::_GetObjectFromHandle(Handle, &offset);

    //
    // Use the object's globals, not the caller's
    //
    pFxDriverGlobals = pObject->GetDriverGlobals();

    FxPointerNotNull(pFxDriverGlobals, TypeInfo);

    pHeader = pObject->GetContextHeader();

    for ( ; pHeader != NULL; pHeader = pHeader->NextHeader) {
        if (pHeader->ContextTypeInfo == TypeInfo) {
            return &pHeader->Context[0];
        }
    }

    PCHAR pGivenName;

    if (TypeInfo->ContextName != NULL) {
        pGivenName = TypeInfo->ContextName;
    }
    else {
        pGivenName = "<no typename given>";
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGHANDLE,
                        "Attempting to get context type %s from WDFOBJECT 0x%p",
                        pGivenName, Handle);

    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL+1)
WDFAPI
WDFOBJECT
FASTCALL
WDFEXPORT(WdfObjectContextGetObject)(
    __in
    PWDF_DRIVER_GLOBALS,
    __in
    PVOID ContextPointer
    )
/*++

Routine Description:
    Reverse of WdfObjectGetTypedContextWorker.  Function will return the handle
    associated with the provided context pointer.

Arguments:
    ContextPointer - context pointer from which to retrieve the owning handle

Return Value:
    A valid WDF handle

  --*/
{
    DDI_ENTRY_IMPERSONATION_OK();

    FxContextHeader* pHeader;
    FxObject* pObject;

    //
    // The context could be one of the chained contexts on the object and not
    // the first one, so it is easiest to go back to the object and build the
    // handle value from the FxObject*.
    //
    #pragma prefast(push);


    #pragma prefast(disable:__WARNING_BUFFER_UNDERFLOW, "No way to express that passed in ptr is at an offset");

    pHeader = CONTAINING_RECORD(ContextPointer, FxContextHeader, Context);
    pObject = pHeader->Object;

    #pragma prefast(pop);

    return (WDFOBJECT) pObject->GetObjectHandle();
}

} // extern "C"
