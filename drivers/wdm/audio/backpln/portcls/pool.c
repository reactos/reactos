/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pool.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    PVOID Item = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    IN PVOID Item,
    IN ULONG Tag)
{

    ExFreePoolWithTag(Item, Tag);
}
