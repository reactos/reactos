/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/iospace.c
 * PURPOSE:         Mapping I/O space
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME       EXPORTED
 * MmMapIoSpace@16
 *
 * DESCRIPTION
 *  Maps a physical memory range into system space.
 *
 * ARGUMENTS
 * PhysicalAddress
 *  First physical address to map;
 *
 * NumberOfBytes
 *  Number of bytes to map;
 *
 * CacheEnable
 *  Type of memory caching.
 *
 * RETURN VALUE
 * The base virtual address which maps the region.
 *
 * NOTE
 *  Description moved here from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
PVOID NTAPI
MmMapIoSpace (IN PHYSICAL_ADDRESS PhysicalAddress,
              IN ULONG NumberOfBytes,
              IN MEMORY_CACHING_TYPE CacheEnable)
{
   PVOID Result;
   ULONG Offset;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Protect;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PFN_TYPE Pfn;

   DPRINT("MmMapIoSpace(%lx, %d, %d)\n", PhysicalAddress, NumberOfBytes, CacheEnable);

   if (CacheEnable != MmNonCached &&
       CacheEnable != MmCached &&
       CacheEnable != MmWriteCombined)
   {
      return NULL;
   }

   BoundaryAddressMultiple.QuadPart = 0;
   Result = NULL;
   Offset = PhysicalAddress.u.LowPart % PAGE_SIZE;
   NumberOfBytes += Offset;
   PhysicalAddress.QuadPart -= Offset;
   Protect = PAGE_EXECUTE_READWRITE | PAGE_SYSTEM;
   if (CacheEnable != MmCached)
   {
      Protect |= (PAGE_NOCACHE | PAGE_WRITETHROUGH);
   }

   MmLockAddressSpace(MmGetKernelAddressSpace());
   Status = MmCreateMemoryArea (MmGetKernelAddressSpace(),
                                MEMORY_AREA_IO_MAPPING,
                                &Result,
                                NumberOfBytes,
                                Protect,
                                &marea,
                                0,
                                FALSE,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
   {
      DPRINT("MmMapIoSpace failed (%lx)\n", Status);
      return (NULL);
   }
   Pfn = PhysicalAddress.LowPart >> PAGE_SHIFT;
   for (i = 0; i < PAGE_ROUND_UP(NumberOfBytes); i += PAGE_SIZE, Pfn++)
   {
      Status = MmCreateVirtualMappingForKernel((char*)Result + i,
                                               Protect,
                                               &Pfn,
					       1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         ASSERT(FALSE);
      }
   }
   return (PVOID)((ULONG_PTR)Result + Offset);
}


/**********************************************************************
 * NAME       EXPORTED
 * MmUnmapIoSpace@8
 *
 * DESCRIPTION
 *  Unmaps a physical memory range from system space.
 *
 * ARGUMENTS
 * BaseAddress
 *  The base virtual address which maps the region;
 *
 * NumberOfBytes
 *  Number of bytes to unmap.
 *
 * RETURN VALUE
 * None.
 *
 * NOTE
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
VOID NTAPI
MmUnmapIoSpace (IN PVOID BaseAddress,
                IN SIZE_T NumberOfBytes)
{
   LONG Offset;
   PVOID Address = BaseAddress;

   Offset = (ULONG_PTR)Address % PAGE_SIZE;
   Address = RVA(Address, - Offset);
   NumberOfBytes += Offset;

   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         Address,
                         NULL,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}


/**********************************************************************
 * NAME       EXPORTED
 * MmMapVideoDisplay@16
 *
 * @implemented
 */
PVOID NTAPI
MmMapVideoDisplay (IN PHYSICAL_ADDRESS PhysicalAddress,
                   IN SIZE_T   NumberOfBytes,
                   IN MEMORY_CACHING_TYPE CacheType)
{
   return MmMapIoSpace (PhysicalAddress, NumberOfBytes, (BOOLEAN)CacheType);
}


/*
 * @implemented
 */
VOID NTAPI
MmUnmapVideoDisplay (IN PVOID BaseAddress,
                     IN SIZE_T NumberOfBytes)
{
   MmUnmapIoSpace (BaseAddress, NumberOfBytes);
}


/* EOF */
