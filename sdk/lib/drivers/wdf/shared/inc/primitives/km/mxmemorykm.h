/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxMemoryKm.h

Abstract:

    Kernel mode implementation of memory
    class defined in MxMemory.h

Author:



Revision History:



--*/

#pragma once

#include "mxmemory.h"

__inline
PVOID
MxMemory::MxAllocatePoolWithTag(
    __in POOL_TYPE  PoolType,
    __in SIZE_T  NumberOfBytes,
    __in ULONG  Tag
    )
{
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

__inline
VOID
MxMemory::MxFreePool(
    __in PVOID Ptr
    )
{
    ExFreePool(Ptr);
}

