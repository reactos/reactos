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
ExFreePoolWithTag(
    IN PVOID Pool,
    IN ULONG Tag
);

#define ExAllocatePool(p,n) ExAllocatePoolWithTag(p,n, TAG_RTL)
#define ExFreePool(P) ExFreePoolWithTag(P, TAG_RTL)

/* EOF */
