//
//    Copyright (C) Microsoft.  All rights reserved.
//
/*++

Module Name:

    IFxMemory.hpp

Abstract:

    Abstract base class for a memory object.  It is necessary to split the
    memory interface away from an FxObject derived base class so that we can
    hand out WDFMEMORY handles that are embedded within other FxObject derived
    classes without having to embed another FxObject derived class within the
    parent.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef __IFX_MEMORY_HPP__
#define __IFX_MEMORY_HPP__

// begin_wpp enum
enum IFxMemoryFlags {
    IFxMemoryFlagReadOnly = 0x0001,
};
// end_wpp


class IFxMemory {
public:
    virtual
    PVOID
    GetBuffer(
        VOID
        ) =0;

    virtual
    size_t
    GetBufferSize(
        VOID
        ) =0;

    virtual
    PMDL
    GetMdl(
        VOID
        ) =0;

    virtual
    WDFMEMORY
    GetHandle(
        VOID
        ) =0;

    //
    // Value returned is a bit field from the enum IFxMemoryFlags
    //
    virtual
    USHORT
    GetFlags(
        VOID
        ) =0;

    virtual
    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        ) =0;

    virtual
    ULONG
    AddRef(
        __in PVOID Tag,
        __in LONG Line,
        __in_opt PSTR File
        ) =0;

    virtual
    ULONG
    Release(
        __in PVOID Tag,
        __in LONG Line,
        __in_opt PSTR File
        ) =0;

    virtual
    VOID
    Delete(
        VOID
        ) =0;

    _Must_inspect_result_
    NTSTATUS
    ValidateMemoryOffsets(
        __in_opt PWDFMEMORY_OFFSET Offsets
        )
    {
        NTSTATUS status;
        size_t total;

        if (Offsets == NULL) {
            return STATUS_SUCCESS;
        }

        status = RtlSizeTAdd(Offsets->BufferLength, Offsets->BufferOffset, &total);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (total > GetBufferSize()) {
            return STATUS_INTEGER_OVERFLOW;
        }

        return STATUS_SUCCESS;
    }

    _Must_inspect_result_
    NTSTATUS
    CopyFromPtr(
        __in_opt PWDFMEMORY_OFFSET DestinationOffsets,
        __in_bcount(SourceBufferLength) PVOID SourceBuffer,
        __in size_t SourceBufferLength,
        __in_opt PWDFMEMORY_OFFSET SourceOffsets
        );

    _Must_inspect_result_
    NTSTATUS
    CopyToPtr(
        __in_opt PWDFMEMORY_OFFSET SourceOffsets,
        __out_bcount(DestinationBufferLength) PVOID DestinationBuffer,
        __in size_t DestinationBufferLength,
        __in_opt PWDFMEMORY_OFFSET DestinationOffsets
        );

protected:
    static
    _Must_inspect_result_
    NTSTATUS
    _CopyPtrToPtr(
        __in_bcount(SourceBufferLength)  PVOID SourceBuffer,
        __in size_t SourceBufferLength,
        __in_opt PWDFMEMORY_OFFSET SourceOffsets,
        __out_bcount(DestinationBufferLength) PVOID DestinationBuffer,
        __in size_t DestinationBufferLength,
        __in_opt PWDFMEMORY_OFFSET DestinationOffsets
        );
};

#endif // __IFX_MEMORY_HPP__
