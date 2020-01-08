#include "common/fxhandle.h"
#include "common/fxmacros.h"
#include "common/fxobject.h"
#include "common/fxglobals.h"
#include "common/dbgtrace.h"
#include "common/fxverifier.h"
#include <ntintsafe.h>


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
    
    if (Attributes != NULL)
    {
        if (Attributes->ContextTypeInfo != NULL)
        {
            if (Attributes->ContextSizeOverride != 0)
            {
                contextSize = Attributes->ContextSizeOverride;
            }
            else
            {
                contextSize = Attributes->ContextTypeInfo->ContextSize;
            }
        }
    }
        
    return contextSize;
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

    if (Tag == 0)
    {
        Tag = FxDriverGlobals->Tag;
        ASSERT(Tag != 0);
    }

    if (ObjectType == FxObjectTypeInternal)
    {
        //
        // An internal object might need the debug extension size added to it,
        // but that's it.  No extra size, no FxContextHeader.
        //
        if (FxDriverGlobals->IsObjectDebugOn())
        {
            status = RtlSizeTAdd(Size,
                                 FxObjectDebugExtensionSize,
                                 &totalSize);
        }
        else
        {
            totalSize = Size;
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        status = FxCalculateObjectTotalSize(FxDriverGlobals,
                                            (USHORT) Size,
                                            ExtraSize,
                                            Attributes,
                                            &totalSize);
    }

    if (!NT_SUCCESS(status))
    {
        return NULL;
    }

    blob = FxPoolAllocateWithTag(FxDriverGlobals, PoolType, totalSize, Tag);

    if (blob != NULL)
    {
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

    if (NT_SUCCESS(status))
    {
        if (ContextSize != 0)
        {
            size_t alignUp;

            alignUp = ALIGN_UP(ContextSize, PVOID);

            //
            // overflow after aligning up to a pointer boundary;
            //
            if (alignUp < ContextSize)
            {
                return STATUS_INTEGER_OVERFLOW;
            }

            status = RtlSizeTAdd(*Total, alignUp, Total);
        }
    }

    if (NT_SUCCESS(status) && FxDriverGlobals->IsObjectDebugOn())
    {
        //
        // Attempt to add in the debug extension
        //
        status = RtlSizeTAdd(*Total,
                             FxObjectDebugExtensionSize,
                             Total);
    }

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGOBJECT,
            "Size overflow, object size 0x%x, extra object size 0x%x, "
            "context size 0x%I64x, %!STATUS!",
            RawObjectSize, ExtraSize, ContextSize, status);
    }

    return status;
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

    if (FxDriverGlobals->IsObjectDebugOn())
    {
        FxObjectDebugExtension* pExtension;

        pExtension = (FxObjectDebugExtension*) AllocationStart;

        RtlZeroMemory(pExtension, FxObjectDebugExtensionSize);
        pExtension->Signature = FxObjectDebugExtensionSignature;

        pObject = (FxObject*) &pExtension->AllocationStart[0];
    }
    else
    {
        pObject = (FxObject*) AllocationStart;
    }

    if (ObjectType == FxObjectTypeExternal)
    {
        FxContextHeaderInit(
            (FxContextHeader*) WDF_PTR_ADD_OFFSET(pObject, ObjectSize),
            pObject,
            Attributes
            );
    }

    return pObject;
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

    if (Attributes != NULL)
    {
        if (Attributes->ContextTypeInfo != NULL)
        {
            size_t contextSize;

            if (Attributes->ContextSizeOverride != 0)
            {
                contextSize = Attributes->ContextSizeOverride;
            }
            else
            {
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

//__inline
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

    if (Handle == NULL)
    {
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
    if (pObject->GetType() == Type)
    {
        *PPObject = pObject;
        ASSERT(offset == 0x0);
        return;
    }
    else
    {
        FxObjectHandleGetPtrQI(pObject, PPObject, Handle, Type, offset);
    }
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

    if (!NT_SUCCESS(status))
    {
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

//__inline
VOID
FxObjectHandleCreate(
    __in  FxObject* Object,
    __out PWDFOBJECT Handle
    )
{
#if FX_SUPER_DBG
    if (Object->GetDriverGlobals()->FxVerifierHandle)
    {
        ASSERT(Object->GetObjectSize() > 0);
        ASSERT(Object == Object->GetContextHeader()->Object);
    }
#endif
    ASSERT((((ULONG_PTR) Object) & FxHandleFlagMask) == 0x0);
    *Handle = Object->GetObjectHandle();
}
