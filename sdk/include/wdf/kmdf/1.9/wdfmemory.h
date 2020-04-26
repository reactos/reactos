/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfMemory.h

Abstract:

    Contains prototypes for managing memory objects in the driver frameworks.

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFMEMORY_H_
#define _WDFMEMORY_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef enum _WDF_MEMORY_DESCRIPTOR_TYPE {
    WdfMemoryDescriptorTypeInvalid = 0,
    WdfMemoryDescriptorTypeBuffer,
    WdfMemoryDescriptorTypeMdl,
    WdfMemoryDescriptorTypeHandle,
} WDF_MEMORY_DESCRIPTOR_TYPE;



typedef struct _WDFMEMORY_OFFSET {
    //
    // Offset into the WDFMEMORY that the operation should start at.
    //
    size_t BufferOffset;

    //
    // Number of bytes that the operation should access.  If 0, the entire
    // length of the WDFMEMORY buffer will be used in the operation or ignored
    // depending on the API.
    //
    size_t BufferLength;

} WDFMEMORY_OFFSET, *PWDFMEMORY_OFFSET;

typedef struct _WDF_MEMORY_DESCRIPTOR {
    WDF_MEMORY_DESCRIPTOR_TYPE Type;

    union {
        struct {
            PVOID Buffer;

            ULONG Length;
        } BufferType;

        struct {
            PMDL Mdl;

            ULONG BufferLength;
        } MdlType;

        struct {
            WDFMEMORY Memory;
            PWDFMEMORY_OFFSET Offsets;
        } HandleType;
    } u;

} WDF_MEMORY_DESCRIPTOR, *PWDF_MEMORY_DESCRIPTOR;

VOID
FORCEINLINE
WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
    __out PWDF_MEMORY_DESCRIPTOR Descriptor,
    __in  PVOID Buffer,
    __in  ULONG BufferLength
    )
{
    RtlZeroMemory(Descriptor, sizeof(WDF_MEMORY_DESCRIPTOR));

    Descriptor->Type = WdfMemoryDescriptorTypeBuffer;
    Descriptor->u.BufferType.Buffer = Buffer;
    Descriptor->u.BufferType.Length = BufferLength;
}

VOID
FORCEINLINE
WDF_MEMORY_DESCRIPTOR_INIT_MDL(
    __out PWDF_MEMORY_DESCRIPTOR Descriptor,
    __in PMDL Mdl,
    __in ULONG BufferLength
    )
{
    RtlZeroMemory(Descriptor, sizeof(WDF_MEMORY_DESCRIPTOR));

    Descriptor->Type = WdfMemoryDescriptorTypeMdl;
    Descriptor->u.MdlType.Mdl = Mdl;
    Descriptor->u.MdlType.BufferLength = BufferLength;
}

VOID
FORCEINLINE
WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
    __out PWDF_MEMORY_DESCRIPTOR Descriptor,
    __in WDFMEMORY Memory,
    __in_opt PWDFMEMORY_OFFSET Offsets
    )
{
    RtlZeroMemory(Descriptor, sizeof(WDF_MEMORY_DESCRIPTOR));

    Descriptor->Type = WdfMemoryDescriptorTypeHandle;
    Descriptor->u.HandleType.Memory = Memory;
    Descriptor->u.HandleType.Offsets = Offsets;
}

//
// WDF Function: WdfMemoryCreate
//
typedef
__checkReturn
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
(*PFN_WDFMEMORYCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    POOL_TYPE PoolType,
    __in_opt
    ULONG PoolTag,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out
    WDFMEMORY* Memory,
    __out_opt
    PVOID* Buffer
    );

__checkReturn
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
NTSTATUS
FORCEINLINE
WdfMemoryCreate(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    POOL_TYPE PoolType,
    __in_opt
    ULONG PoolTag,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out
    WDFMEMORY* Memory,
    __out_opt
    PVOID* Buffer
    )
{
    return ((PFN_WDFMEMORYCREATE) WdfFunctions[WdfMemoryCreateTableIndex])(WdfDriverGlobals, Attributes, PoolType, PoolTag, BufferSize, Memory, Buffer);
}

//
// WDF Function: WdfMemoryCreatePreallocated
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFMEMORYCREATEPREALLOCATED)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in __drv_aliasesMem
    PVOID Buffer,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out
    WDFMEMORY* Memory
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfMemoryCreatePreallocated(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in __drv_aliasesMem
    PVOID Buffer,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out
    WDFMEMORY* Memory
    )
{
    return ((PFN_WDFMEMORYCREATEPREALLOCATED) WdfFunctions[WdfMemoryCreatePreallocatedTableIndex])(WdfDriverGlobals, Attributes, Buffer, BufferSize, Memory);
}

//
// WDF Function: WdfMemoryGetBuffer
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PVOID
(*PFN_WDFMEMORYGETBUFFER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY Memory,
    __out_opt
    size_t* BufferSize
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PVOID
FORCEINLINE
WdfMemoryGetBuffer(
    __in
    WDFMEMORY Memory,
    __out_opt
    size_t* BufferSize
    )
{
    return ((PFN_WDFMEMORYGETBUFFER) WdfFunctions[WdfMemoryGetBufferTableIndex])(WdfDriverGlobals, Memory, BufferSize);
}

//
// WDF Function: WdfMemoryAssignBuffer
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFMEMORYASSIGNBUFFER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY Memory,
    __in
    PVOID Buffer,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfMemoryAssignBuffer(
    __in
    WDFMEMORY Memory,
    __in
    PVOID Buffer,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize
    )
{
    return ((PFN_WDFMEMORYASSIGNBUFFER) WdfFunctions[WdfMemoryAssignBufferTableIndex])(WdfDriverGlobals, Memory, Buffer, BufferSize);
}

//
// WDF Function: WdfMemoryCopyToBuffer
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFMEMORYCOPYTOBUFFER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY SourceMemory,
    __in
    size_t SourceOffset,
    __out_bcount( NumBytesToCopyTo )
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfMemoryCopyToBuffer(
    __in
    WDFMEMORY SourceMemory,
    __in
    size_t SourceOffset,
    __out_bcount( NumBytesToCopyTo )
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    )
{
    return ((PFN_WDFMEMORYCOPYTOBUFFER) WdfFunctions[WdfMemoryCopyToBufferTableIndex])(WdfDriverGlobals, SourceMemory, SourceOffset, Buffer, NumBytesToCopyTo);
}

//
// WDF Function: WdfMemoryCopyFromBuffer
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFMEMORYCOPYFROMBUFFER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY DestinationMemory,
    __in
    size_t DestinationOffset,
    __in
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfMemoryCopyFromBuffer(
    __in
    WDFMEMORY DestinationMemory,
    __in
    size_t DestinationOffset,
    __in
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    )
{
    return ((PFN_WDFMEMORYCOPYFROMBUFFER) WdfFunctions[WdfMemoryCopyFromBufferTableIndex])(WdfDriverGlobals, DestinationMemory, DestinationOffset, Buffer, NumBytesToCopyFrom);
}

//
// WDF Function: WdfLookasideListCreate
//
typedef
__checkReturn
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
(*PFN_WDFLOOKASIDELISTCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES LookasideAttributes,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __in
    __drv_strictTypeMatch(__drv_typeExpr)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    __in_opt
    ULONG PoolTag,
    __out
    WDFLOOKASIDE* Lookaside
    );

__checkReturn
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
NTSTATUS
FORCEINLINE
WdfLookasideListCreate(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES LookasideAttributes,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __in
    __drv_strictTypeMatch(__drv_typeExpr)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    __in_opt
    ULONG PoolTag,
    __out
    WDFLOOKASIDE* Lookaside
    )
{
    return ((PFN_WDFLOOKASIDELISTCREATE) WdfFunctions[WdfLookasideListCreateTableIndex])(WdfDriverGlobals, LookasideAttributes, BufferSize, PoolType, MemoryAttributes, PoolTag, Lookaside);
}

//
// WDF Function: WdfMemoryCreateFromLookaside
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFMEMORYCREATEFROMLOOKASIDE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFLOOKASIDE Lookaside,
    __out
    WDFMEMORY* Memory
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfMemoryCreateFromLookaside(
    __in
    WDFLOOKASIDE Lookaside,
    __out
    WDFMEMORY* Memory
    )
{
    return ((PFN_WDFMEMORYCREATEFROMLOOKASIDE) WdfFunctions[WdfMemoryCreateFromLookasideTableIndex])(WdfDriverGlobals, Lookaside, Memory);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFMEMORY_H_

