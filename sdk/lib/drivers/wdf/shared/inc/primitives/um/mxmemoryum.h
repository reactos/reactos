/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxMemoryUm.h

Abstract:

    User mode implementation of memory
    class defined in MxMemory.h

Author:



Revision History:



--*/

#pragma once

#include "MxMemory.h"

__inline
PVOID
MxMemory::MxAllocatePoolWithTag(
    __in POOL_TYPE  PoolType,
    __in SIZE_T  NumberOfBytes,
    __in ULONG  Tag
    )
{
    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(Tag);

    return ::HeapAlloc(
                GetProcessHeap(),
                0,
                NumberOfBytes
                );
}

__inline
VOID
MxMemory::MxFreePool(
    __in PVOID Ptr
    )
{
    ::HeapFree(
            GetProcessHeap(),
            0,
            Ptr
            );
}

