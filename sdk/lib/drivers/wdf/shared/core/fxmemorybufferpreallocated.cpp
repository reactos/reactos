/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferPreallocated.cpp

Abstract:

    This module implements a frameworks managed FxMemoryBufferPreallocated

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "FxMemoryBufferPreallocated.hpp"

FxMemoryBufferPreallocated::FxMemoryBufferPreallocated(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID Buffer,
    _In_ size_t BufferSize
    ) :
    FxMemoryObject(FxDriverGlobals, sizeof(*this), BufferSize),
    m_pBuffer(Buffer)
/*++

Routine Description:
    Contstructor for this object.  Stores off all the pointers and sizes passed
    in by the caller.

Arguments:
    Buffer  - Buffer to associate with this object

    BufferSize - Size of Buffer in bytes

Return Value:
    None

  --*/
{
}

FxMemoryBufferPreallocated::FxMemoryBufferPreallocated(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ USHORT ObjectSize,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID Buffer,
    _In_ size_t BufferSize
    ) :
    FxMemoryObject(FxDriverGlobals, ObjectSize, BufferSize),
    m_pBuffer(Buffer)
/*++

Routine Description:
    Contstructor for this object.  Stores off all the pointers and sizes passed
    in by the caller.

Arguments:
    ObjectSize  - Size of the derived object

    Buffer  - Buffer to associate with this object

    BufferSize - Size of Buffer in bytes

Return Value:
    None

  --*/
{
}


FxMemoryBufferPreallocated::FxMemoryBufferPreallocated(
    __in USHORT ObjectSize,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) : FxMemoryObject(FxDriverGlobals, ObjectSize, 0), m_pBuffer(NULL)
/*++

Routine Description:
    Contstructor for this object.  Stores off all the pointers and sizes passed
    in by the caller.

Arguments:
    ObjectSize  - Size of the derived object.

Return Value:
    None

  --*/
{
}

FxMemoryBufferPreallocated::~FxMemoryBufferPreallocated()
/*++

Routine Description:
    Destructor for this object.  Does nothing with the client memory since
    the client owns it.

Arguments:
    None

Return Value:
    None

  --*/
{
}

_Must_inspect_result_
NTSTATUS
FxMemoryBufferPreallocated::QueryInterface(
    __inout FxQueryInterfaceParams* Params
    )
{
    if (Params->Type == FX_TYPE_MEMORY_PREALLOCATED) {
        *Params->Object = (FxMemoryBufferPreallocated*) this;
        return STATUS_SUCCESS;
    }
    else {
        return __super::QueryInterface(Params);
    }
}

VOID
FxMemoryBufferPreallocated::UpdateBuffer(
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID Buffer,
    _In_ size_t BufferSize
    )
/*++

Routine Description:
    Updates the internal pointer to a new value.


Arguments:
    Buffer - new buffer

    BufferSize - length of Buffer in bytes

Return Value:
    None.

  --*/

{
    m_pBuffer = Buffer;
    m_BufferSize = BufferSize;
}
