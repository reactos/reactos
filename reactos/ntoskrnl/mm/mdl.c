/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mdl.c
 * PURPOSE:         Manipulates MDLs
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeMdlImplementation)
#endif

/* GLOBALS *******************************************************************/

#define TAG_MDL    TAG('M', 'D', 'L', ' ')

#define MI_MDL_MAPPING_REGION_SIZE       (256*1024*1024)

static PVOID MiMdlMappingRegionBase = NULL;
static RTL_BITMAP MiMdlMappingRegionAllocMap;
static ULONG MiMdlMappingRegionHint;
static KSPIN_LOCK MiMdlMappingRegionLock;
extern ULONG MmPageArraySize;

/*
MDL Flags desc.

MDL_PAGES_LOCKED              MmProbeAndLockPages has been called for this mdl
MDL_SOURCE_IS_NONPAGED_POOL   mdl has been build by MmBuildMdlForNonPagedPool
MDL_PARTIAL                   mdl has been built by IoBuildPartialMdl
MDL_MAPPING_CAN_FAIL          in case of an error, MmMapLockedPages will return NULL instead of to bugcheck
MDL_MAPPED_TO_SYSTEM_VA       mdl has been mapped into kernel space using MmMapLockedPages
MDL_PARTIAL_HAS_BEEN_MAPPED   mdl flagged MDL_PARTIAL has been mapped into kernel space using MmMapLockedPages
*/

/* FUNCTIONS *****************************************************************/


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmAdvanceMdl (
    IN PMDL Mdl,
    IN ULONG NumberOfBytes
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


VOID
INIT_FUNCTION
NTAPI
MmInitializeMdlImplementation(VOID)
{
   MEMORY_AREA* Result;
   NTSTATUS Status;
   PVOID Buffer;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;
   MiMdlMappingRegionHint = 0;
   MiMdlMappingRegionBase = NULL;

   MmLockAddressSpace(MmGetKernelAddressSpace());
   Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                               MEMORY_AREA_MDL_MAPPING,
                               &MiMdlMappingRegionBase,
                               MI_MDL_MAPPING_REGION_SIZE,
                               PAGE_READWRITE,
                               &Result,
                               FALSE,
                               0,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      KEBUGCHECK(0);
   }
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   Buffer = ExAllocatePoolWithTag(NonPagedPool, 
                                  MI_MDL_MAPPING_REGION_SIZE / (PAGE_SIZE * 8),
                                  TAG_MDL);

   RtlInitializeBitMap(&MiMdlMappingRegionAllocMap, Buffer, MI_MDL_MAPPING_REGION_SIZE / PAGE_SIZE);
   RtlClearAllBits(&MiMdlMappingRegionAllocMap);

   KeInitializeSpinLock(&MiMdlMappingRegionLock);
}


PVOID
NTAPI
MmGetMdlPageAddress(PMDL Mdl, PVOID Offset)
{
   PPFN_NUMBER MdlPages;

   MdlPages = (PPFN_NUMBER)(Mdl + 1);

   return((PVOID)MdlPages[((ULONG_PTR)Offset) / PAGE_SIZE]);
}


/*
 * @implemented
 */
VOID STDCALL
MmUnlockPages(PMDL Mdl)
/*
 * FUNCTION: Unlocks the physical pages described by a given MDL
 * ARGUMENTS:
 *      MemoryDescriptorList = MDL describing the buffer to be unlocked
 * NOTES: The memory described by the specified MDL must have been locked
 * previously by a call to MmProbeAndLockPages. As the pages unlocked, the
 * MDL is updated
 *
 * May be called in any process context.
 */
{
   ULONG i;
   PPFN_NUMBER MdlPages;
   PFN_NUMBER Page;

   /*
    * MmProbeAndLockPages MUST have been called to lock this mdl!
    *
    * Windows will bugcheck if you pass MmUnlockPages an mdl that hasn't been
    * locked with MmLockAndProbePages, but (for now) we'll be more forgiving...
    */
   if (!(Mdl->MdlFlags & MDL_PAGES_LOCKED))
   {
      DPRINT1("MmUnlockPages called for non-locked mdl!\n");
      return;
   }

   /* If mdl buffer is mapped io space -> do nothing */
   if (Mdl->MdlFlags & MDL_IO_SPACE)
   {
      Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
      return;
   }

   /* Automagically undo any calls to MmGetSystemAddressForMdl's for this mdl */
   if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
   {
      MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
   }

   /*
    * FIXME: I don't know whether this right, but it looks sensible
    */
   if ((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) ||
         (Mdl->MdlFlags & MDL_IO_PAGE_READ))
   {
      return;
   }


   MdlPages = (PPFN_NUMBER)(Mdl + 1);
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount+Mdl->ByteOffset)/PAGE_SIZE); i++)
   {
      Page = MdlPages[i];
      MmUnlockPage(Page);
      MmDereferencePage(Page);
   }

   Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
}


/*
 * @unimplemented
 */
PVOID
STDCALL
MmMapLockedPagesWithReservedMapping (
    IN PVOID MappingAddress,
    IN ULONG PoolTag,
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    )
{
	UNIMPLEMENTED;
	return 0;
}


/*
 * @implemented
 */
VOID STDCALL
MmUnmapLockedPages(PVOID BaseAddress, PMDL Mdl)
/*
 * FUNCTION: Releases a mapping set up by a preceding call to MmMapLockedPages
 * ARGUMENTS:
 *         BaseAddress = Base virtual address to which the pages were mapped
 *         MemoryDescriptorList = MDL describing the mapped pages
 *
 * User space unmappings _must_ be done from the original process context!
 */
{
   KIRQL oldIrql;
   ULONG i;
   ULONG PageCount;
   ULONG Base;

   DPRINT("MmUnmapLockedPages(BaseAddress %x, Mdl %x)\n", BaseAddress, Mdl);

   /*
    * In this case, the MDL has the same system address as the base address
    * so there is no need to free it
    */
   if ((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) &&
         (BaseAddress >= MmSystemRangeStart))
   {
      return;
   }


   /* Calculate the number of pages we mapped. */
   PageCount = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;

   /*
    * Docs says that BaseAddress should be a _base_ address, but every example
    * I've seen pass the actual address. -Gunnar
    */
   BaseAddress = PAGE_ALIGN(BaseAddress);

   /* Unmap all the pages. */
   for (i = 0; i < PageCount; i++)
   {
      MmDeleteVirtualMapping(Mdl->Process,
                             (char*)BaseAddress + (i * PAGE_SIZE),
                             FALSE,
                             NULL,
                             NULL);
   }

   if (BaseAddress >= MmSystemRangeStart)
   {
      ASSERT(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA);

      KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);
      /* Deallocate all the pages used. */
      Base = (ULONG)((char*)BaseAddress - (char*)MiMdlMappingRegionBase) / PAGE_SIZE;

      RtlClearBits(&MiMdlMappingRegionAllocMap, Base, PageCount);

      MiMdlMappingRegionHint = min (MiMdlMappingRegionHint, Base);

      KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);

      /* Reset the MDL state. */
      Mdl->MdlFlags &= ~MDL_MAPPED_TO_SYSTEM_VA;
      Mdl->MappedSystemVa = NULL;

   }
   else
   {
      MEMORY_AREA *Marea;

      ASSERT(Mdl->Process == PsGetCurrentProcess());

      Marea = MmLocateMemoryAreaByAddress( (PMADDRESS_SPACE)&(Mdl->Process)->VadRoot, BaseAddress );
      if (Marea == NULL)
      {
         DPRINT1( "Couldn't open memory area when unmapping user-space pages!\n" );
         KEBUGCHECK(0);
      }

      MmFreeMemoryArea( (PMADDRESS_SPACE)&(Mdl->Process)->VadRoot, Marea, NULL, NULL );

      Mdl->Process = NULL;
   }

}


/*
 * @unimplemented
 */
VOID
STDCALL
MmUnmapReservedMapping (
     IN PVOID BaseAddress,
     IN ULONG PoolTag,
     IN PMDL MemoryDescriptorList
     )
{
	UNIMPLEMENTED;
}


VOID
NTAPI
MmBuildMdlFromPages(PMDL Mdl, PPFN_TYPE Pages)
{
   memcpy(Mdl + 1, Pages, sizeof(PFN_TYPE) * (PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGE_SIZE));

   /* FIXME: this flag should be set by the caller perhaps? */
   Mdl->MdlFlags |= MDL_IO_PAGE_READ;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmPrefetchPages (
    IN ULONG NumberOfLists,
    IN PREAD_LIST *ReadLists
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmProtectMdlSystemAddress (
    IN PMDL MemoryDescriptorList,
    IN ULONG NewProtect
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL MmProbeAndLockPages (PMDL Mdl,
                                  KPROCESSOR_MODE AccessMode,
                                  LOCK_OPERATION Operation)
/*
 * FUNCTION: Probes the specified pages, makes them resident and locks them
 * ARGUMENTS:
 *          Mdl = MDL to probe
 *          AccessMode = Access at which to probe the buffer
 *          Operation = Operation to probe for
 *
 * This function can be seen as a safe version of MmBuildMdlForNonPagedPool
 * used in cases where you know that the mdl address is paged memory or
 * you don't know where the mdl address comes from. MmProbeAndLockPages will
 * work no matter what kind of mdl address you have.
 */
{
   PPFN_TYPE MdlPages;
   ULONG i, j;
   ULONG NrPages;
   NTSTATUS Status;
   KPROCESSOR_MODE Mode;
   PFN_TYPE Page;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   PMADDRESS_SPACE AddressSpace;

   DPRINT("MmProbeAndLockPages(Mdl %x)\n", Mdl);

   ASSERT(!(Mdl->MdlFlags & (MDL_PAGES_LOCKED|MDL_MAPPED_TO_SYSTEM_VA|MDL_PARTIAL|
            MDL_IO_SPACE|MDL_SOURCE_IS_NONPAGED_POOL)));

   MdlPages = (PPFN_TYPE)(Mdl + 1);
   NrPages = PAGE_ROUND_UP(Mdl->ByteOffset + Mdl->ByteCount) / PAGE_SIZE;

   /* mdl must have enough page entries */
   ASSERT(NrPages <= (Mdl->Size - sizeof(MDL))/sizeof(PFN_TYPE));


   if (Mdl->StartVa >= MmSystemRangeStart &&
       MmGetPfnForProcess(NULL, Mdl->StartVa) >= MmPageArraySize)
   {
       /* phys addr is not phys memory so this must be io memory */

      for (i = 0; i < NrPages; i++)
      {
         MdlPages[i] = MmGetPfnForProcess(NULL, (char*)Mdl->StartVa + (i*PAGE_SIZE));
      }

      Mdl->MdlFlags |= MDL_PAGES_LOCKED|MDL_IO_SPACE;
      return;
   }


   if (Mdl->StartVa >= MmSystemRangeStart)
   {
      /* FIXME: why isn't AccessMode used? */
      Mode = KernelMode;
      Mdl->Process = NULL;
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      /* FIXME: why isn't AccessMode used? */
      Mode = UserMode;
      Mdl->Process = CurrentProcess;
      AddressSpace = (PMADDRESS_SPACE)&(CurrentProcess)->VadRoot;
   }


   /*
    * Lock the pages
    */
   MmLockAddressSpace(AddressSpace);

   for (i = 0; i < NrPages; i++)
   {
      PVOID Address;

      Address = (char*)Mdl->StartVa + (i*PAGE_SIZE);

      /*
       * FIXME: skip the probing/access stuff if buffer is nonpaged kernel space?
       * -Gunnar
       */

      if (!MmIsPagePresent(NULL, Address))
      {
         Status = MmAccessFault(FALSE, Address, Mode, NULL);
         if (!NT_SUCCESS(Status))
         {
            for (j = 0; j < i; j++)
            {
	       Page = MdlPages[j];
               if (Page < MmPageArraySize)
               {
                  MmUnlockPage(Page);
                  MmDereferencePage(Page);
               }
            }
            MmUnlockAddressSpace(AddressSpace);
            ExRaiseStatus(STATUS_ACCESS_VIOLATION);
         }
      }
      else
      {
         MmLockPage(MmGetPfnForProcess(NULL, Address));
      }

      if ((Operation == IoWriteAccess || Operation == IoModifyAccess) &&
          (!(MmGetPageProtect(NULL, (PVOID)Address) & PAGE_READWRITE)))
      {
         Status = MmAccessFault(TRUE, Address, Mode, NULL);
         if (!NT_SUCCESS(Status))
         {
            for (j = 0; j < i; j++)
            {
	       Page = MdlPages[j];
               if (Page < MmPageArraySize)
               {
                  MmUnlockPage(Page);
                  MmDereferencePage(Page);
               }
            }
            MmUnlockAddressSpace(AddressSpace);
            ExRaiseStatus(STATUS_ACCESS_VIOLATION);
         }
      }
      Page = MmGetPfnForProcess(NULL, Address);
      MdlPages[i] = Page;
      if (Page >= MmPageArraySize)
         Mdl->MdlFlags |= MDL_IO_SPACE;
      else
         MmReferencePage(Page);
   }

   MmUnlockAddressSpace(AddressSpace);
   Mdl->MdlFlags |= MDL_PAGES_LOCKED;
}


/*
 * @unimplemented
 */
VOID
STDCALL
MmProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )
{
	UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
STDCALL
MmProbeAndLockSelectedPages(
	IN OUT PMDL MemoryDescriptorList,
	IN LARGE_INTEGER PageList[],
	IN KPROCESSOR_MODE AccessMode,
	IN LOCK_OPERATION Operation
	)
{
	UNIMPLEMENTED;
}


/*
 * @implemented
 */
ULONG STDCALL MmSizeOfMdl (PVOID Base,
                           ULONG Length)
/*
 * FUNCTION: Returns the number of bytes to allocate for an MDL describing
 * the given address range
 * ARGUMENTS:
 *         Base = base virtual address
 *         Length = number of bytes to map
 */
{
   ULONG len;

   len = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base,Length);

   return(sizeof(MDL)+(len*sizeof(PFN_TYPE)));
}


/*
 * @implemented
 */
VOID STDCALL
MmBuildMdlForNonPagedPool (PMDL Mdl)
/*
 * FUNCTION: Fills in the corresponding physical page array of a given
 * MDL for a buffer in nonpaged system space
 * ARGUMENTS:
 *        Mdl = Points to an MDL that supplies a virtual address,
 *              byte offset and length
 *
 * This function can be seen as a fast version of MmProbeAndLockPages in case
 * you _know_ that the mdl address is within nonpaged kernel space.
 */
{
   ULONG i;
   ULONG PageCount;
   PPFN_TYPE MdlPages;

   /*
    * mdl buffer must (at least) be in kernel space, thou this doesn't
    * necesarely mean that the buffer in within _nonpaged_ kernel space...
    */
   ASSERT(Mdl->StartVa >= MmSystemRangeStart);

   PageCount = PAGE_ROUND_UP(Mdl->ByteOffset + Mdl->ByteCount) / PAGE_SIZE;
   MdlPages = (PPFN_TYPE)(Mdl + 1);

   /* mdl must have enough page entries */
   ASSERT(PageCount <= (Mdl->Size - sizeof(MDL))/sizeof(PFN_TYPE));

   for (i=0; i < PageCount; i++)
   {
      *MdlPages++ = MmGetPfnForProcess(NULL, (char*)Mdl->StartVa + (i * PAGE_SIZE));
   }

   Mdl->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;
   Mdl->Process = NULL;
   Mdl->MappedSystemVa = (char*)Mdl->StartVa + Mdl->ByteOffset;
}


/*
 * @implemented
 */
PMDL STDCALL
MmCreateMdl (PMDL Mdl,
             PVOID Base,
             ULONG Length)
/*
 * FUNCTION: Allocates and initalizes an MDL
 * ARGUMENTS:
 *          MemoryDescriptorList = Points to MDL to initalize. If this is
 *                                 NULL then one is allocated
 *          Base = Base virtual address of the buffer
 *          Length = Length in bytes of the buffer
 * RETURNS: A pointer to initalized MDL
 */
{
   if (Mdl == NULL)
   {
      ULONG Size;

      Size = MmSizeOfMdl(Base,Length);
      Mdl =
         (PMDL)ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MDL);
      if (Mdl == NULL)
      {
         return(NULL);
      }
   }

   MmInitializeMdl(Mdl, (char*)Base, Length);

   return(Mdl);
}


/*
 * @unimplemented
 */
VOID STDCALL
MmMapMemoryDumpMdl (PVOID Unknown0)
/*
 * FIXME: Has something to do with crash dumps. Do we want to implement
 * this?
 */
{
   UNIMPLEMENTED;
}


/*
 * @implemented
 */
PMDL STDCALL
MmAllocatePagesForMdl ( IN PHYSICAL_ADDRESS LowAddress,
                        IN PHYSICAL_ADDRESS HighAddress,
                        IN PHYSICAL_ADDRESS SkipBytes,
                        IN SIZE_T Totalbytes )
{
      /*
      MmAllocatePagesForMdl allocates zero-filled, nonpaged, physical memory pages to an MDL

      MmAllocatePagesForMdlSearch the PFN database for free, zeroed or standby
      pagesAllocates pages and puts in MDLDoes not map pages (caller responsibility)
      Designed to be used by an AGP driver

      LowAddress is the lowest acceptable physical address it wants to allocate
      and HighAddress is the highest. SkipBytes are the number of bytes that the
      kernel should keep free above LowAddress and below the address at which it
      starts to allocate physical memory. TotalBytes are the number of bytes that
      the driver wants to allocate. The return value of the function is a MDL
      that if non-zero describes the physical memory the kernel has given the
      driver. To access portions of the memory the driver must create sub-MDLs
      from the returned MDL that describe appropriate portions of the physical
      memory. When a driver wants to access physical memory described by a
      sub-MDL it must map the sub-MDL using MmGetSystemAddressForMdlSafe.

      Konstantin Gusev
      */

   PMDL Mdl;
   PPFN_TYPE Pages;
   ULONG NumberOfPagesWanted, NumberOfPagesAllocated;
   ULONG Ret;

   DPRINT("MmAllocatePagesForMdl - LowAddress = 0x%I64x, HighAddress = 0x%I64x, "
          "SkipBytes = 0x%I64x, Totalbytes = 0x%x\n",
          LowAddress.QuadPart, HighAddress.QuadPart,
          SkipBytes.QuadPart, Totalbytes);

   /* SkipBytes must be a multiple of the page size */
   if ((SkipBytes.QuadPart % PAGE_SIZE) != 0)
   {
      DPRINT1("Warning: SkipBytes is not a multiple of PAGE_SIZE\n");
      return NULL;
   }

   /* Allocate memory for the MDL */
   Mdl = MmCreateMdl(NULL, 0, Totalbytes);
   if (Mdl == NULL)
   {
      return NULL;
   }

   /* Allocate pages into the MDL */
   NumberOfPagesAllocated = 0;
   NumberOfPagesWanted = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;
   Pages = (PPFN_TYPE)(Mdl + 1);
   while (NumberOfPagesWanted > 0)
   {
      Ret = MmAllocPagesSpecifyRange(
                        MC_NPPOOL,
                        LowAddress,
                        HighAddress,
                        NumberOfPagesWanted,
                        Pages + NumberOfPagesAllocated);
      if (Ret == (ULONG)-1)
         break;

      NumberOfPagesAllocated += Ret;
      NumberOfPagesWanted -= Ret;

      if (SkipBytes.QuadPart == 0)
         break;
      LowAddress.QuadPart += SkipBytes.QuadPart;
      HighAddress.QuadPart += SkipBytes.QuadPart;
   }

   if (NumberOfPagesAllocated == 0)
   {
      ExFreePool(Mdl);
      Mdl = NULL;
   }
   else if (NumberOfPagesWanted > 0)
   {
      Mdl->ByteCount = (ULONG)(NumberOfPagesAllocated * PAGE_SIZE);
      /* FIXME: I don't know if Mdl->Size should also be changed -- blight */
   }
   return Mdl;
}


/*
 * @implemented
 */
VOID STDCALL
MmFreePagesFromMdl ( IN PMDL Mdl )
{
      /*
      Drivers use the MmFreePagesFromMdl, the kernel-mode equivalent of
      FreeUserPhysicalPages, to free the physical memory it has allocated with
      MmAllocatePagesForMdl. This function is also prototyped in ntddk.h:

      Note that a driver is responsible for deallocating the MDL returned by
      MmAllocatePagesForMdl with a call to ExFreePool, since MmFreePagesFromMdl
      does not free the MDL.

       Konstantin Gusev

   */
   PPFN_TYPE Pages;
   LONG NumberOfPages;

   NumberOfPages = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;
   Pages = (PPFN_TYPE)(Mdl + 1);

   while (--NumberOfPages >= 0)
   {
      MmDereferencePage(Pages[NumberOfPages]);
   }
}


/*
 * @implemented
 */
PVOID STDCALL
MmMapLockedPagesSpecifyCache ( IN PMDL Mdl,
                               IN KPROCESSOR_MODE AccessMode,
                               IN MEMORY_CACHING_TYPE CacheType,
                               IN PVOID BaseAddress,
                               IN ULONG BugCheckOnFailure,
                               IN MM_PAGE_PRIORITY Priority)
{
   PVOID Base;
   PULONG MdlPages;
   KIRQL oldIrql;
   ULONG PageCount;
   ULONG StartingOffset;
   PEPROCESS CurrentProcess;
   NTSTATUS Status;
   ULONG Protect;

   DPRINT("MmMapLockedPagesSpecifyCache(Mdl 0x%x, AccessMode 0x%x, CacheType 0x%x, "
          "BaseAddress 0x%x, BugCheckOnFailure 0x%x, Priority 0x%x)\n",
          Mdl, AccessMode, CacheType, BaseAddress, BugCheckOnFailure, Priority);

   /* FIXME: Implement Priority */
   (void) Priority;

   Protect = PAGE_READWRITE;
   if (CacheType == MmNonCached)
      Protect |= PAGE_NOCACHE;
   else if (CacheType == MmWriteCombined)
      DPRINT("CacheType MmWriteCombined not supported!\n");

   /* Calculate the number of pages required. */
   PageCount = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;

   if (AccessMode != KernelMode)
   {
      MEMORY_AREA *Result;
      LARGE_INTEGER BoundaryAddressMultiple;
      NTSTATUS Status;

      /* pretty sure you can't map partial mdl's to user space */
      ASSERT(!(Mdl->MdlFlags & MDL_PARTIAL));

      BoundaryAddressMultiple.QuadPart = 0;
      Base = BaseAddress;

      CurrentProcess = PsGetCurrentProcess();

      MmLockAddressSpace((PMADDRESS_SPACE)&CurrentProcess->VadRoot);
      Status = MmCreateMemoryArea((PMADDRESS_SPACE)&CurrentProcess->VadRoot,
                                  MEMORY_AREA_MDL_MAPPING,
                                  &Base,
                                  PageCount * PAGE_SIZE,
                                  Protect,
                                  &Result,
                                  (Base != NULL),
                                  0,
                                  BoundaryAddressMultiple);
      MmUnlockAddressSpace((PMADDRESS_SPACE)&CurrentProcess->VadRoot);
      if (!NT_SUCCESS(Status))
      {
         if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL)
         {
            return NULL;
         }

         /* Throw exception */
         ExRaiseStatus(STATUS_ACCESS_VIOLATION);
         ASSERT(0);
      }

      Mdl->Process = (PEPROCESS)CurrentProcess;
   }
   else /* if (AccessMode == KernelMode) */
   {
      /* can't map mdl twice */
      ASSERT(!(Mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA|MDL_PARTIAL_HAS_BEEN_MAPPED)));
      /* can't map mdl buildt from non paged pool into kernel space */
      ASSERT(!(Mdl->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL)));

      CurrentProcess = NULL;

      /* Allocate that number of pages from the mdl mapping region. */
      KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);

      StartingOffset = RtlFindClearBitsAndSet(&MiMdlMappingRegionAllocMap, PageCount, MiMdlMappingRegionHint);

      if (StartingOffset == 0xffffffff)
      {
         KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);

         DPRINT1("Out of MDL mapping space\n");

         if ((Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL) || !BugCheckOnFailure)
         {
            return NULL;
         }

         KEBUGCHECK(0);
      }

      Base = (PVOID)((ULONG_PTR)MiMdlMappingRegionBase + StartingOffset * PAGE_SIZE);

      if (MiMdlMappingRegionHint == StartingOffset)
      {
         MiMdlMappingRegionHint += PageCount;
      }

      KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);

      Mdl->Process = NULL;
   }

   /* Set the virtual mappings for the MDL pages. */
   MdlPages = (PULONG)(Mdl + 1);

   if (Mdl->MdlFlags & MDL_IO_SPACE)
      Status = MmCreateVirtualMappingUnsafe(CurrentProcess,
                                            Base,
                                            Protect,
                                            MdlPages,
                                            PageCount);
   else
      Status = MmCreateVirtualMapping(CurrentProcess,
                                      Base,
                                      Protect,
                                      MdlPages,
                                      PageCount);
   if (!NT_SUCCESS(Status))
   {
      DbgPrint("Unable to create virtual mapping\n");
      if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL)
      {
         return NULL;
      }
      if (AccessMode != KernelMode)
      {
         /* Throw exception */
         ExRaiseStatus(STATUS_ACCESS_VIOLATION);
         ASSERT(0);
      }
      else /* AccessMode == KernelMode */
      {
         if (!BugCheckOnFailure)
            return NULL;

         /* FIXME: Use some bugcheck code instead of 0 */
         KEBUGCHECK(0);
      }
   }

   /* Mark the MDL has having being mapped. */
   if (AccessMode == KernelMode)
   {
      if (Mdl->MdlFlags & MDL_PARTIAL)
      {
         Mdl->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
      }
      else
      {
         Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
      }
      Mdl->MappedSystemVa = (char*)Base + Mdl->ByteOffset;
   }
   else
      DPRINT1("UserMode mapping - returning 0x%x\n", (ULONG)Base + Mdl->ByteOffset);

   return((char*)Base + Mdl->ByteOffset);
}


/*
 * @implemented
 */
PVOID STDCALL
MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Maps the physical pages described by a given MDL
 * ARGUMENTS:
 *       Mdl = Points to an MDL updated by MmProbeAndLockPages, MmBuildMdlForNonPagedPool,
 *             MmAllocatePagesForMdl or IoBuildPartialMdl.
 *       AccessMode = Specifies the portion of the address space to map the
 *                    pages.
 * RETURNS: The base virtual address that maps the locked pages for the
 * range described by the MDL
 *
 * If mapping into user space, pages are mapped into current address space.
 */
{
   return MmMapLockedPagesSpecifyCache(Mdl,
                                       AccessMode,
                                       MmCached,
                                       NULL,
                                       TRUE,
                                       NormalPagePriority);
}


/* EOF */










