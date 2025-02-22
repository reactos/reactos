/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pool.cpp
 * PURPOSE:         Memory functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    return ExAllocatePoolZero(PoolType, NumberOfBytes, Tag);
}

VOID
FreeItem(
    IN PVOID Item,
    IN ULONG Tag)
{
    ExFreePoolWithTag(Item, Tag);
}
