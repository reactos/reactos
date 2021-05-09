/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxMemory.h

Abstract:

    Mode agnostic definition of memory
    allocation/deallocation functions

    See MxMemoryKm.h and MxMemoryUm.h for mode
    specific implementations

Author:



Revision History:



--*/

#pragma once

class MxMemory
{
public:

    __inline
    static
    PVOID
    MxAllocatePoolWithTag(
        __in POOL_TYPE  PoolType,
        __in SIZE_T  NumberOfBytes,
        __in ULONG  Tag
        );

    __inline
    static
    VOID
    MxFreePool(
        __in PVOID Ptr
        );
};

