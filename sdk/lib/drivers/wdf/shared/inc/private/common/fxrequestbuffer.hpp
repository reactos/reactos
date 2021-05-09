/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRequestBuffer.hpp

Abstract:

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXREQUESTBUFFER_H_
#define _FXREQUESTBUFFER_H_

enum FxRequestBufferType {
    FxRequestBufferUnspecified,
    FxRequestBufferMemory,  // framework managed memory
    FxRequestBufferMdl,     // raw MDL
    FxRequestBufferBuffer,  // raw PVOID
    FxRequestBufferReferencedMdl, // MDL belonging to a FxMemoryObject
};

enum FxValidateMemoryDescriptorFlags {
    MemoryDescriptorNullAllowed = 0x1,
    MemoryDescriptorNoBufferAllowed = 0x2,
};

struct FxRequestBuffer {
public:
    FxRequestBuffer(VOID);

    NTSTATUS
    ValidateMemoryDescriptor(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_MEMORY_DESCRIPTOR Descriptor,
        __in ULONG Flags = 0x0
        );

    VOID
    SetMemory(
        __in IFxMemory* Memory,
        __in PWDFMEMORY_OFFSET Offsets
        );

    VOID
    SetMdl(
        __in PMDL Mdl,
        __in ULONG Length
        );

    __inline
    VOID
    SetBuffer(
        __in PVOID Buffer,
        __in ULONG Length
        )
    {
        DataType = FxRequestBufferBuffer;
        u.Buffer.Buffer = Buffer;
        u.Buffer.Length = Length;
    }

    __inline
    BOOLEAN
    HasMdl(
        VOID
        )
    {
        return (DataType == FxRequestBufferMdl ||
                DataType == FxRequestBufferReferencedMdl) ? TRUE : FALSE;
    }

    ULONG
    GetBufferLength(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    GetBuffer(
        __deref_out PVOID* Buffer
        );

    _Must_inspect_result_
    NTSTATUS
    GetOrAllocateMdl(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __deref_out_opt PMDL*   Mdl,
        __inout PMDL*           MdlToFree,
        __inout PBOOLEAN        UnlockWhenFreed,
        __in LOCK_OPERATION     Operation,
        __in BOOLEAN            ReuseMdl = FALSE,
        __inout_opt size_t*     SizeOfMdl = NULL
        );

    NTSTATUS
    GetOrAllocateMdlWorker(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __deref_out PMDL*       Mdl,
        __in BOOLEAN *          ReuseMdl,
        __in LONG               Length,
        __in PVOID              Buffer,
        __inout size_t*         SizeOfMdl,
        __in BOOLEAN            UnlockWhenFreed,
        __deref_out_opt PMDL*   MdlToFree
        );

    VOID
    AssignValues(
        __deref_out_opt PVOID* PPBuffer,
        __deref_out_opt PMDL* PPMdl,
        __out PULONG BufferLength
        );

public:
    FxRequestBufferType DataType;

    union {
        struct {
            IFxMemory* Memory;
            PWDFMEMORY_OFFSET Offsets;
        } Memory;
        struct {
            PMDL Mdl;
            ULONG Length;
        } Mdl;
        struct {
            PVOID Buffer;
            ULONG Length;
        } Buffer;
        struct {
            IFxMemory* Memory;
            PWDFMEMORY_OFFSET Offsets;
            PMDL Mdl;
        } RefMdl;
    } u;
};

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#include "fxrequestbufferkm.hpp"
#elif ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#include "fxrequestbufferum.hpp"
#endif


#endif // _FXREQUESTBUFFER_H_
