/*
Abstract:

    Abstract base class for a memory object.  It is necessary to split the
    memory interface away from an FxObject derived base class so that we can
    hand out WDFMEMORY handles that are embedded within other FxObject derived
    classes without having to embed another FxObject derived class within the
    parent.

Author:



Environment:

    kernel mode only

Revision History:

--*/

#ifndef __IFX_MEMORY_H__
#define __IFX_MEMORY_H__

#include "common/fxglobals.h"

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

#endif //__IFX_MEMORY_H__