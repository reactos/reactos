/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBuffer.cpp

Abstract:

    This module implements a frameworks managed FxMemoryBuffer

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxmemorybuffer.hpp"

extern "C" {
// #include "FxMemoryBuffer.tmh"
}

_Must_inspect_result_
NTSTATUS
FxMemoryBuffer::_Create(
    __in  PFX_DRIVER_GLOBALS DriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __in  ULONG PoolTag,
    __in  size_t BufferSize,
    __in POOL_TYPE PoolType,
    __out FxMemoryObject** Object
    )
{
    FxMemoryBuffer* pBuffer;

    pBuffer = new(DriverGlobals, Attributes, (USHORT) BufferSize, PoolTag, PoolType)
        FxMemoryBuffer(DriverGlobals, BufferSize);

    if (pBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Object = pBuffer;

    return STATUS_SUCCESS;
}

FxMemoryBuffer::FxMemoryBuffer(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t BufferSize
    ) :
    FxMemoryObject(FxDriverGlobals,
                   (USHORT) COMPUTE_OBJECT_SIZE(sizeof(*this), (ULONG) BufferSize),
                   BufferSize)
/*++

Routine Description:
    Constructor for FxMemoryBuffer.  Initializes the entire data structure.

Arguments:
    BufferSize - Size of the buffer represented by this object

    We round up the object size (via COMPUTE_OBJECT_SIZE) because the object,
    the context and the memory it returns via GetBuffer() are all in one allocation
    and the returned memory needs to match the alignment that raw pool follows.

    Note on downcasting to USHORT
    ==============================
    Note that FxMemoryBuffer is used for buffer less than a page size, and
    object-size and buffer-size are added and allocated together.
    Downcasting to USHORT is safe because size of object plus buffer (<page size)
    would be smaller than 64K that could fit in USHORT.

    For buffer larger than 1 page size, object memory is allocated separately
    from buffer memory and the object used is FxMemoryBufferFromPool.
    There is no downcasting done in that case.

    See comments in FxMemoryObject.hpp for info on various fx memory objects.

Assumes:
    FxMemoryBuffer::operator new has already initialized the entire allocation

Return Value:
    None

  --*/
{
    ASSERT(BufferSize <= USHORT_MAX);
}

FxMemoryBuffer::FxMemoryBuffer(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in size_t BufferSize
    ) :
    FxMemoryObject(FxDriverGlobals,
                   (USHORT) COMPUTE_OBJECT_SIZE(ObjectSize, (ULONG) BufferSize),
                   BufferSize)

/*++

Routine Description:
    Constructor for derivations FxMemoryBuffer.  Initializes the entire data
    structure.

    We round up the object size (via COMPUTE_OBJECT_SIZE) because the object,
    the context and the memory it returns via GetBuffer() are all in one allocation
    and the returned memory needs to match the alignment that raw pool follows.

    Note on downcasting to USHORT
    ==============================
    Note that FxMemoryBuffer is used for buffer less than a page size, and
    object-size and buffer-size are added and allocated together.
    Downcasting to USHORT is safe because size of object plus buffer (<page size)
    would be smaller than 64K that could fit in USHORT.

    For buffer larger than 1 page size, object memory is allocated separately
    from buffer memory and the object used is FxMemoryBufferFromPool.
    There is no downcasting done in that case.

    See comments in FxMemoryObject.hpp for info on various fx memory objects.

Arguments:
    ObjectSize - Size of the derived object

    BufferSize - Size of the buffer represented by this object

Return Value:
    None

  --*/
{
    ASSERT(BufferSize <= USHORT_MAX);
}

FxMemoryBuffer::~FxMemoryBuffer()
/*++

Routine Description:
    Destructor for this object.  Since the object + buffer is one
    allocation, there is no need to free any more memory.

Arguments:
    None

Return Value:
    None

  --*/
{
}

PVOID
FxMemoryBuffer::GetBuffer(
    VOID
    )
/*++

Routine Description:
    Returns the buffer associated with this object

Arguments:
    None

Return Value:
    a valid pointer, of size GetBufferSize()

  --*/
{
    //
    // The buffer follows the struct itself
    //
    return ((PUCHAR) this) + COMPUTE_RAW_OBJECT_SIZE(sizeof(*this));
}
