/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/memory.c
 * PURPOSE:     Memory management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisAllocateMemoryWithTag(
    OUT PVOID   *VirtualAddress,
    IN  UINT    Length,
    IN  ULONG   Tag)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCreateLookaheadBufferFromSharedMemory(
    IN  PVOID   pSharedMemory,
    IN  UINT    LookaheadLength,
    OUT PVOID   *pLookaheadBuffer)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisDestroyLookaheadBufferFromSharedMemory(
    IN  PVOID   pLookaheadBuffer)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMoveFromMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMoveMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length)
{
    RtlCopyMemory(Destination,Source,Length);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMoveToMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMUpdateSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisAllocateMemory(
    OUT PVOID                   *VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress)
/*
 * FUNCTION: Allocates a block of memory
 * ARGUMENTS:
 *     VirtualAddress           = Address of buffer to place virtual
 *                                address of the allocated memory
 *     Length                   = Size of the memory block to allocate
 *     MemoryFlags              = Flags to specify special restrictions
 *     HighestAcceptableAddress = Specifies -1
 */
{
    PVOID Block;

    if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS) {
        /* FIXME */
        *VirtualAddress = NULL;
        return NDIS_STATUS_FAILURE;
    }

    if (MemoryFlags & NDIS_MEMORY_NONCACHED) {
        /* FIXME */
        *VirtualAddress = NULL;
        return NDIS_STATUS_FAILURE;
    }

    /* Plain nonpaged memory */
    Block           = ExAllocatePool(NonPagedPool, Length);
    *VirtualAddress = Block;
    if (!Block)
        return NDIS_STATUS_FAILURE;

	return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisFreeMemory(
    IN  PVOID   VirtualAddress,
    IN  UINT    Length,
    IN  UINT    MemoryFlags)
/*
 * FUNCTION: Frees a memory block allocated with NdisAllocateMemory
 * ARGUMENTS:
 *     VirtualAddress = Pointer to the base virtual address of the allocated memory
 *     Length         = Size of the allocated memory block as passed to NdisAllocateMemory
 *     MemoryFlags    = Memory flags passed to NdisAllocateMemory
 */
{
    if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS) {
        /* FIXME */
        return;
    }

    if (MemoryFlags & NDIS_MEMORY_NONCACHED) {
        /* FIXME */
        return;
    }

    /* Plain nonpaged memory */
    ExFreePool(VirtualAddress);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisImmediateReadSharedMemory(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SharedMemoryAddress,
    OUT PUCHAR      Buffer,
    IN  ULONG       Length)
{
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisImmediateWriteSharedMemory(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SharedMemoryAddress,
    IN  PUCHAR      Buffer,
    IN  ULONG       Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMAllocateSharedMemory(
    IN	NDIS_HANDLE             MiniportAdapterHandle,
    IN	ULONG                   Length,
    IN	BOOLEAN                 Cached,
    OUT	PVOID                   *VirtualAddress,
    OUT	PNDIS_PHYSICAL_ADDRESS  PhysicalAddress)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMAllocateSharedMemoryAsync(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  ULONG       Length,
    IN  BOOLEAN     Cached,
    IN  PVOID       Context)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress)
{
    UNIMPLEMENTED
}

/* EOF */
