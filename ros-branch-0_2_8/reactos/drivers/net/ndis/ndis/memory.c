/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/memory.c
 * PURPOSE:     Memory management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   15 Aug 2003 Vizzini - DMA support
 *   3  Oct 2003 Vizzini - formatting and minor bugfixing
 */

#include "ndissys.h"


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisAllocateMemoryWithTag(
    OUT PVOID   *VirtualAddress,
    IN  UINT    Length,
    IN  ULONG   Tag)
/*
 * FUNCTION:  Allocates a block of memory, with a 32-bit tag
 * ARGUMENTS:
 *   VirtualAddress = a pointer to the returned memory block
 *   Length         = the number of requested bytes
 *   Tag            = 32-bit pool tag
 * RETURNS:
 *   NDIS_STATUS_SUCCESS on success
 *   NDIS_STATUS_FAILURE on failure
 */
{
  PVOID Block;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  Block = ExAllocatePoolWithTag(NonPagedPool, Length, Tag);
  *VirtualAddress = Block;

  if (!Block)
    return NDIS_STATUS_FAILURE;

  return NDIS_STATUS_SUCCESS;
}



/*
 * @implemented
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
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_FAILURE on failure
 */
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (MemoryFlags & NDIS_MEMORY_NONCACHED)
    {
      *VirtualAddress = MmAllocateNonCachedMemory(Length);
      if(!*VirtualAddress)
        return NDIS_STATUS_FAILURE;

      return NDIS_STATUS_SUCCESS;
    }

  if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS)
    {
      *VirtualAddress = MmAllocateContiguousMemory(Length, HighestAcceptableAddress);
      if(!*VirtualAddress)
        return NDIS_STATUS_FAILURE;

      return NDIS_STATUS_SUCCESS;
    }

  /* Plain nonpaged memory */
  *VirtualAddress = ExAllocatePool(NonPagedPool, Length);
  if (!*VirtualAddress)
    return NDIS_STATUS_FAILURE;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
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
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (MemoryFlags & NDIS_MEMORY_NONCACHED)
    {
      MmFreeNonCachedMemory(VirtualAddress, Length);
      return;
    }

  if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS)
    {
      MmFreeContiguousMemory(VirtualAddress);
      return;
    }

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
    UNIMPLEMENTED
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
 * @implemented
 */
VOID
EXPORT
NdisMAllocateSharedMemory(
    IN	NDIS_HANDLE             MiniportAdapterHandle,
    IN	ULONG                   Length,
    IN	BOOLEAN                 Cached,
    OUT	PVOID                   *VirtualAddress,
    OUT	PNDIS_PHYSICAL_ADDRESS  PhysicalAddress)
/*
 * FUNCTION: Allocate a common buffer for DMA
 * ARGUMENTS:
 *     MiniportAdapterHandle:  Handle passed into MiniportInitialize
 *     Length:  Number of bytes to allocate
 *     Cached:  Whether or not the memory can be cached
 *     VirtualAddress:  Pointer to memory is returned here
 *     PhysicalAddress:  Physical address corresponding to virtual address
 * NOTES:
 *     - Cached is ignored; we always allocate non-cached
 */
{
  PNDIS_MINIPORT_BLOCK Adapter = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;

  NDIS_DbgPrint(MAX_TRACE,("Called.\n"));

  *VirtualAddress = Adapter->SystemAdapterObject->DmaOperations->AllocateCommonBuffer(
      Adapter->SystemAdapterObject, Length, PhysicalAddress, Cached);
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
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
    UNIMPLEMENTED

  return NDIS_STATUS_FAILURE;
}


VOID
NTAPI
NdisMFreeSharedMemoryPassive(
    PVOID Context)
/*
 * FUNCTION:  Free a common buffer
 * ARGUMENTS:
 *     Context:  Pointer to a miniport shared memory context
 * NOTES:
 *     - Called by NdisMFreeSharedMemory to do the actual work
 */
{
  PMINIPORT_SHARED_MEMORY Memory = (PMINIPORT_SHARED_MEMORY)Context;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

  Memory->AdapterObject->DmaOperations->FreeCommonBuffer(
      Memory->AdapterObject, Memory->Length, Memory->PhysicalAddress,
      Memory->VirtualAddress, Memory->Cached);

  ExFreePool(Memory);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress)
/*
 * FUNCTION:  Free a shared memory block
 * ARGUMENTS:
 *     MiniportAdapterHandle:  Handle passed into MiniportInitialize
 *     Length:  Number of bytes in the block to free
 *     Cached:  Whether or not the memory was cached
 *     VirtualAddress:  Address to free
 *     PhysicalAddress:  corresponding physical addres
 * NOTES:
 *     - This function can be called at dispatch_level or passive_level.
 *       Therefore we have to do this in a worker thread.
 */
{
  HANDLE ThreadHandle;
  PNDIS_MINIPORT_BLOCK Adapter = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
  PMINIPORT_SHARED_MEMORY Memory;

  NDIS_DbgPrint(MAX_TRACE,("Called.\n"));

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

  /* Must be NonpagedPool because by definition we're at DISPATCH_LEVEL */
  Memory = ExAllocatePool(NonPagedPool, sizeof(MINIPORT_SHARED_MEMORY));

  if(!Memory)
    {
      NDIS_DbgPrint(MID_TRACE, ("Insufficient resources\n"));
      return;
    }

  Memory->AdapterObject = Adapter->SystemAdapterObject;
  Memory->Length = Length;
  Memory->PhysicalAddress = PhysicalAddress;
  Memory->VirtualAddress = VirtualAddress;
  Memory->Cached = Cached;

  PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, 0, 0, 0, NdisMFreeSharedMemoryPassive, Memory);
  ZwClose(ThreadHandle);
}

/* EOF */

