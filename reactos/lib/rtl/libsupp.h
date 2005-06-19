/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/libsupp.h
 * PURPOSE:         Run-Time Library Kernel Support Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#define TAG_RTL TAG('R','t', 'l', ' ')

PVOID
STDCALL
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

VOID
STDCALL
ExFreePool(
    IN PVOID Pool
);

#define ExAllocatePool(p,n) ExAllocatePoolWithTag(p,n, TAG_RTL)

/* EOF */
