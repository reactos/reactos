/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferFromLookaside.cpp

Abstract:

    This module implements a frameworks managed FxMemoryBufferFromLookaside

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "FxNPagedLookasideList.hpp"
#include "FxMemoryBufferFromLookaside.hpp"

FxMemoryBufferFromLookaside::FxMemoryBufferFromLookaside(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout FxLookasideList* Lookaside,
    __in size_t BufferSize
    ) :
    FxMemoryObject(FxDriverGlobals,
                   COMPUTE_OBJECT_SIZE(sizeof(*this), (ULONG) BufferSize),
                   BufferSize),
    m_pLookaside(Lookaside)
/*++

Routine Description:
    Constructor for this object.  Remembers which lookaside list this object
    was allocated from

    We round up the object size (via COMPUTE_OBJECT_SIZE) because the object,
    the context and the memory it returns via GetBuffer() are all in one allocation
    and the returned memory needs to match the alignment that raw pool follows.

Arguments:
    Lookaside - The lookaside list which this object will return itself to when
        its last reference is removed

    BufferSize - The buffer size associated with this object

Return Value:
    None

  --*/
{
    Init();
}

FxMemoryBufferFromLookaside::FxMemoryBufferFromLookaside(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout FxLookasideList* Lookaside,
    __in size_t BufferSize,
    __in USHORT ObjectSize
    ) :
    FxMemoryObject(FxDriverGlobals, ObjectSize, BufferSize),
    m_pLookaside(Lookaside)
/*++

Routine Description:
    Constructor for this object.  Remembers which lookaside list this object
    was allocated from

    There is no round up of the ObjectSize because the derived class may not
    embed the buffer in the same allocation as the object.  If it were to do so,
    the derived object should do the round up on its own.

Arguments:
    Lookaside - The lookaside list which this object will return itself to when
        its last reference is removed

    BufferSize - The buffer size associated with this object

    ObjectSize - The size of this object.

Return Value:
    None

  --*/
{
    Init();
}

VOID
FxMemoryBufferFromLookaside::Init(
    VOID
    )
{
    m_pLookaside->ADDREF(this);
}



FxMemoryBufferFromLookaside::~FxMemoryBufferFromLookaside()
/*++

Routine Description:
    Destructor for this object.  This function does nothing, it lets
    SelfDestruct do all the work.

Arguments:

Return Value:

  --*/
{
}

_Must_inspect_result_
PVOID
FxMemoryBufferFromLookaside::operator new(
    __in size_t Size,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout PVOID ValidMemory,
    __in size_t BufferSize,
    __in PWDF_OBJECT_ATTRIBUTES Attributes
    )
/*++

Routine Description:
    Displacement new operator overload.  Since the lookaside list actually
    allocates the memory block, we just return the block given to us.

Arguments:
    Size - Compiler supplied parameter indicating the
        sizeof(FxMemoryBufferFromLookaside)

    FxDriverGlobals - Driver's globals.

    ValidMemory - Previously allocated block

    BufferSize - The buffer size associated with this object

    Attributes - Description of context.

Return Value:
    ValidMemory pointer value

  --*/
{
    size_t objectSize;

    UNREFERENCED_PARAMETER(Size);

    ASSERT(Size >= sizeof(FxMemoryBufferFromLookaside));

    //
    // We round up the object size (via COMPUTE_OBJECT_SIZE) because the object,
    // and the buffer are all in one allocation and the returned memory needs to
    // match the alignment that raw pool follows.
    //
    // Note that FxMemoryBufferFromLookaside is used for buffer less than a page
    // size so downcasting to USHORT (via COMPUTE_OBJECT_SIZE) is safe because
    // size of object plus buffer (<page size) would be smaller than 64K that
    // could fit in USHORT. See comments in FxMemoryObject.hpp for info on
    // various fx memory objects.
    //
    objectSize = COMPUTE_OBJECT_SIZE(sizeof(FxMemoryBufferFromLookaside),
                                     (ULONG) BufferSize);

    return FxObjectAndHandleHeaderInit(
        FxDriverGlobals,
        ValidMemory,
        (USHORT) objectSize,
        Attributes,
        FxObjectTypeExternal
        );
}

PVOID
FxMemoryBufferFromLookaside::GetBuffer()
{
    //
    // The buffer follows the struct itself
    //
    return ((PUCHAR) this) + COMPUTE_RAW_OBJECT_SIZE(sizeof(*this));
}

VOID
FxMemoryBufferFromLookaside::SelfDestruct()
/*++

Routine Description:
    Self destruction routine called when the last reference has been removed
    on this object.  This functions behaves as if "delete this" has been called
    without calling operator delete.  It manually calls the destructor and then
    returns the memory to the lookaside.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxLookasideList* pLookaside;

    //
    // Call the destructor first.
    //
    // Since we were allocated from a lookaside list, return ourself to this
    // list and then free the reference we have on it.  We can't call this from
    // within the destructor b/c then all parent objects would be destructing on
    // freed pool.
    //
    FxMemoryBufferFromLookaside::~FxMemoryBufferFromLookaside();

    //
    // After FxLookaside::Reclaim, this no longer points to valid memory so we
    // must stash off m_pLookaside first so we can use it after the reclaim.
    //
    pLookaside = m_pLookaside;
    pLookaside->Reclaim(this);
    pLookaside->RELEASE(this);
}

FxMemoryBufferFromPoolLookaside::FxMemoryBufferFromPoolLookaside(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout FxLookasideList* Lookaside,
    __in size_t BufferSize,
    __in_bcount(BufferSize) PVOID Buffer
    ) :
    FxMemoryBufferFromLookaside(FxDriverGlobals,
                                Lookaside,
                                BufferSize,
                                sizeof(*this)),
    m_Pool(Buffer)
{
}

FxMemoryBufferFromPoolLookaside::FxMemoryBufferFromPoolLookaside(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout FxLookasideList* Lookaside,
    __in size_t BufferSize,
    __in_bcount(BufferSize) PVOID Buffer,
    __in USHORT ObjectSize
    ) :
    FxMemoryBufferFromLookaside(FxDriverGlobals,
                                Lookaside,
                                BufferSize,
                                ObjectSize),
    m_Pool(Buffer)
{
}

VOID
FxMemoryBufferFromPoolLookaside::SelfDestruct(
    VOID
    )
{
    //
    // Free the 2ndary allocation
    //
    ((FxLookasideListFromPool*) m_pLookaside)->ReclaimPool(m_Pool);

    //
    // Free the object itself
    //
    __super::SelfDestruct();
}

_Must_inspect_result_
PVOID
FxMemoryBufferFromPoolLookaside::operator new(
    __in size_t Size,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout PVOID ValidMemory,
    __in PWDF_OBJECT_ATTRIBUTES Attributes
    )
{
    size_t objectSize;

    UNREFERENCED_PARAMETER(Size);

    ASSERT(Size >= sizeof(FxMemoryBufferFromPoolLookaside));

    objectSize = COMPUTE_OBJECT_SIZE(sizeof(FxMemoryBufferFromPoolLookaside), 0);

    return FxObjectAndHandleHeaderInit(
        FxDriverGlobals,
        ValidMemory,
        (USHORT) objectSize,
        Attributes,
        FxObjectTypeExternal
        );
}
