/* $Id: mdl.c,v 1.63 2004/05/16 22:27:57 navaraf Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/mdl.c
 * PURPOSE:      Manipulates MDLs
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MDL    TAG('M', 'M', 'D', 'L')

#define MI_MDL_MAPPING_REGION_SIZE       (256*1024*1024)

static PVOID MiMdlMappingRegionBase = NULL;
static RTL_BITMAP MiMdlMappingRegionAllocMap;
static ULONG MiMdlMappingRegionHint;
static KSPIN_LOCK MiMdlMappingRegionLock;
extern ULONG MmPageArraySize;

/*
MDL Flags desc.

MDL_PAGES_LOCKED              MmProbelAndLockPages has been called for this mdl
MDL_SOURCE_IS_NONPAGED_POOL   mdl has been build by MmBuildMdlForNonPagedPool
MDL_PARTIAL                   mdl has been built by IoBuildPartialMdl
MDL_MAPPING_CAN_FAIL          in case of an error, MmMapLockedPages will return NULL instead of to bugcheck
MDL_MAPPED_TO_SYSTEM_VA       mdl has been mapped into kernel space using MmMapLockedPages
MDL_PARTIAL_HAS_BEEN_MAPPED   mdl flagged MDL_PARTIAL has been mapped into kernel space using MmMapLockedPages
*/

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
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
   Status = MmCreateMemoryArea(NULL,
                               MmGetKernelAddressSpace(),
                               MEMORY_AREA_MDL_MAPPING,
                               &MiMdlMappingRegionBase,
                               MI_MDL_MAPPING_REGION_SIZE,
                               0,
                               &Result,
                               FALSE,
                               FALSE,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      KEBUGCHECK(0);
   }
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   Buffer = ExAllocatePool(NonPagedPool, MI_MDL_MAPPING_REGION_SIZE / (PAGE_SIZE * 8));

   RtlInitializeBitMap(&MiMdlMappingRegionAllocMap, Buffer, MI_MDL_MAPPING_REGION_SIZE / PAGE_SIZE);
   RtlClearAllBits(&MiMdlMappingRegionAllocMap);

   KeInitializeSpinLock(&MiMdlMappingRegionLock);
}

PVOID
MmGetMdlPageAddress(PMDL Mdl, PVOID Offset)
{
   PULONG MdlPages;

   MdlPages = (PULONG)(Mdl + 1);

   return((PVOID)MdlPages[((ULONG)Offset) / PAGE_SIZE]);
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
   PULONG MdlPages;

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


   MdlPages = (PULONG)(Mdl + 1);
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount+Mdl->ByteOffset)/PAGE_SIZE); i++)
   {
#if defined(__GNUC__)
      MmUnlockPage((LARGE_INTEGER)(LONGLONG)MdlPages[i]);
      MmDereferencePage((LARGE_INTEGER)(LONGLONG)MdlPages[i]);
#else

      PHYSICAL_ADDRESS dummyJunkNeeded;
      dummyJunkNeeded.QuadPart = MdlPages[i];
      MmUnlockPage(dummyJunkNeeded);
      MmDereferencePage(dummyJunkNeeded);
#endif

   }
   
   Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
}



/*
 * @implemented
 */
PVOID STDCALL
MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Maps the physical pages described by a given MDL
 * ARGUMENTS:
 *       Mdl = Points to an MDL updated by MmProbeAndLockPages, MmBuildMdlForNonPagedPool
 *             or IoBuildPartialMdl.
 *       AccessMode = Specifies the portion of the address space to map the
 *                    pages.
 * RETURNS: The base virtual address that maps the locked pages for the
 * range described by the MDL
 *
 * If mapping into user space, pages are mapped into current address space.
 */
{
   PVOID Base;
   ULONG i;
   PULONG MdlPages;
   KIRQL oldIrql;
   ULONG PageCount;
   ULONG StartingOffset;
   PEPROCESS CurrentProcess;

   DPRINT("MmMapLockedPages(Mdl %x, AccessMode %x)\n", Mdl, AccessMode);

   /* Calculate the number of pages required. */
   PageCount = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;

   if (AccessMode == UserMode)
   {
      MEMORY_AREA *Result;
      LARGE_INTEGER BoundaryAddressMultiple;
      NTSTATUS Status;

      /* pretty sure you can't map partial mdl's to user space */
      assert(!(Mdl->MdlFlags & MDL_PARTIAL));

      BoundaryAddressMultiple.QuadPart = 0;
      Base = NULL;

      CurrentProcess = PsGetCurrentProcess();

      MmLockAddressSpace(&CurrentProcess->AddressSpace);
      Status = MmCreateMemoryArea(CurrentProcess,
                                  &CurrentProcess->AddressSpace,
                                  MEMORY_AREA_MDL_MAPPING,
                                  &Base,
                                  PageCount * PAGE_SIZE,
                                  0, /* PAGE_READWRITE? */
                                  &Result,
                                  FALSE,
                                  FALSE,
                                  BoundaryAddressMultiple);
      MmUnlockAddressSpace(&CurrentProcess->AddressSpace);
      if (!NT_SUCCESS(Status))
      {
         if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL)
         {
            return NULL;            
         }
         
         KEBUGCHECK(0);
         /* FIXME: handle this? */
      }

      Mdl->Process = CurrentProcess;
   }
   else /* if (AccessMode == KernelMode) */
   {
      /* can't map mdl twice */
      assert(!(Mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA|MDL_PARTIAL_HAS_BEEN_MAPPED)));
      /* can't map mdl buildt from non paged pool into kernel space */
      assert(!(Mdl->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL)));
      
      CurrentProcess = NULL;

      /* Allocate that number of pages from the mdl mapping region. */
      KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);

      StartingOffset = RtlFindClearBitsAndSet(&MiMdlMappingRegionAllocMap, PageCount, MiMdlMappingRegionHint);

      if (StartingOffset == 0xffffffff)
      {
         KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
         
         DPRINT1("Out of MDL mapping space\n");
         
         if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL)
         {
            return NULL;            
         }
         
         KEBUGCHECK(0);
      }

      Base = (char*)MiMdlMappingRegionBase + StartingOffset * PAGE_SIZE;

      if (MiMdlMappingRegionHint == StartingOffset)
      {
         MiMdlMappingRegionHint += PageCount;
      }

      KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
   }



   /* Set the virtual mappings for the MDL pages. */
   MdlPages = (PULONG)(Mdl + 1);
   for (i = 0; i < PageCount; i++)
   {
      NTSTATUS Status;
#if !defined(__GNUC__)

      PHYSICAL_ADDRESS dummyJunkNeeded;
      dummyJunkNeeded.QuadPart = MdlPages[i];
#endif

      Status = MmCreateVirtualMapping(CurrentProcess,
                                      (PVOID)((ULONG)Base+(i*PAGE_SIZE)),
                                      PAGE_READWRITE,
#if defined(__GNUC__)
                                      (LARGE_INTEGER)(LONGLONG)MdlPages[i],
#else
                                      dummyJunkNeeded,
#endif
                                      FALSE);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL)
         {
            return NULL;            
         }
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
   
   return((char*)Base + Mdl->ByteOffset);
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
         ((ULONG_PTR)BaseAddress >= KERNEL_BASE))
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
      MmDeleteVirtualMapping(NULL,
                             (char*)BaseAddress + (i * PAGE_SIZE),
                             FALSE,
                             NULL,
                             NULL);
   }

   if ((DWORD)BaseAddress >= KERNEL_BASE)
   {
      assert(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA);
      
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
      
      assert(Mdl->Process == PsGetCurrentProcess());

      Marea = MmOpenMemoryAreaByAddress( &Mdl->Process->AddressSpace, BaseAddress );
      if (Marea == NULL)
      {
         DPRINT1( "Couldn't open memory area when unmapping user-space pages!\n" );
         KEBUGCHECK(0);
      }

      MmFreeMemoryArea( &Mdl->Process->AddressSpace, Marea->BaseAddress, 0, NULL, NULL );

      Mdl->Process = NULL;
   }

}


VOID
MmBuildMdlFromPages(PMDL Mdl, PULONG Pages)
{
   ULONG i;
   PULONG MdlPages;

   MdlPages = (PULONG)(Mdl + 1);

   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGE_SIZE);i++)
   {
      MdlPages[i] = Pages[i];
   }
   
   //FIXME: this flag should be set by the caller perhaps?
   Mdl->MdlFlags |= MDL_IO_PAGE_READ;
}



/*
 * @unimplemented
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
   PULONG MdlPages;
   ULONG i, j;
   ULONG NrPages;
   NTSTATUS Status;
   KPROCESSOR_MODE Mode;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   DPRINT("MmProbeAndLockPages(Mdl %x)\n", Mdl);

   assert(!(Mdl->MdlFlags & (MDL_PAGES_LOCKED|MDL_MAPPED_TO_SYSTEM_VA|MDL_PARTIAL|
            MDL_IO_SPACE|MDL_SOURCE_IS_NONPAGED_POOL)));

   MdlPages = (ULONG *)(Mdl + 1);
   NrPages = PAGE_ROUND_UP(Mdl->ByteOffset + Mdl->ByteCount) / PAGE_SIZE;
   
   /* mdl must have enough page entries */
   assert(NrPages <= (Mdl->Size - sizeof(MDL))/sizeof(ULONG));


   if (Mdl->StartVa >= (PVOID)KERNEL_BASE && 
       (MmGetPhysicalAddressForProcess(NULL, Mdl->StartVa).u.LowPart >> PAGE_SHIFT) > MmPageArraySize)
   {
       /* phys addr is not phys memory so this must be io memory */
       
      for (i = 0; i < NrPages; i++)
      {
         MdlPages[i] = MmGetPhysicalAddressForProcess(NULL, (char*)Mdl->StartVa + (i*PAGE_SIZE)).u.LowPart;
      }
      
      Mdl->MdlFlags |= MDL_PAGES_LOCKED|MDL_IO_SPACE;
      return;
   }


   if (Mdl->StartVa >= (PVOID)KERNEL_BASE)
   {
      //FIXME: why isn't AccessMode used?
      Mode = KernelMode;
      Mdl->Process = NULL;
   }
   else
   {
      //FIXME: why isn't AccessMode used?
      Mode = UserMode;
      Mdl->Process = CurrentProcess;      
   }


   /*
    * Lock the pages
    */
   MmLockAddressSpace(&CurrentProcess->AddressSpace);

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
         Status = MmNotPresentFault(Mode, (ULONG)Address, TRUE);
         if (!NT_SUCCESS(Status))
         {
            for (j = 0; j < i; j++)
            {
#if defined(__GNUC__)
               MmUnlockPage((LARGE_INTEGER)(LONGLONG)MdlPages[j]);
               MmDereferencePage((LARGE_INTEGER)(LONGLONG)MdlPages[j]);
#else

               PHYSICAL_ADDRESS dummyJunkNeeded;
               dummyJunkNeeded.QuadPart = MdlPages[j];
               MmUnlockPage(dummyJunkNeeded);
               MmDereferencePage(dummyJunkNeeded);
#endif

            }
            ExRaiseStatus(Status);
         }
      }
      else
      {
         MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
      }
      
      if ((Operation == IoWriteAccess || Operation == IoModifyAccess) &&
            (!(MmGetPageProtect(NULL, (PVOID)Address) & PAGE_READWRITE)))
      {
         Status = MmAccessFault(Mode, (ULONG)Address, TRUE);
         if (!NT_SUCCESS(Status))
         {
            for (j = 0; j < i; j++)
            {
#if defined(__GNUC__)
               MmUnlockPage((LARGE_INTEGER)(LONGLONG)MdlPages[j]);
               MmDereferencePage(
                  (LARGE_INTEGER)(LONGLONG)MdlPages[j]);
#else

               PHYSICAL_ADDRESS dummyJunkNeeded;
               dummyJunkNeeded.QuadPart = MdlPages[j];
               MmUnlockPage(dummyJunkNeeded);
               MmDereferencePage(dummyJunkNeeded);
#endif

            }
            ExRaiseStatus(Status);
         }
      }
      MdlPages[i] = MmGetPhysicalAddressForProcess(NULL, Address).u.LowPart;
#if defined(__GNUC__)

      MmReferencePage((LARGE_INTEGER)(LONGLONG)MdlPages[i]);
#else

      {
         PHYSICAL_ADDRESS dummyJunkNeeded;
         dummyJunkNeeded.QuadPart = MdlPages[i];
         MmReferencePage(dummyJunkNeeded);
      }
#endif

   }
   
   MmUnlockAddressSpace(&CurrentProcess->AddressSpace);
   Mdl->MdlFlags |= MDL_PAGES_LOCKED;
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

   return(sizeof(MDL)+(len*sizeof(ULONG)));
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
   
   /* 
    * mdl buffer must (at least) be in kernel space, thou this doesn't 
    * necesarely mean that the buffer in within _nonpaged_ kernel space...
    */
   assert((ULONG)Mdl->StartVa >= KERNEL_BASE);
   
   PageCount = PAGE_ROUND_UP(Mdl->ByteOffset + Mdl->ByteCount) / PAGE_SIZE;
   
   /* mdl must have enough page entries */
   assert(PageCount <= (Mdl->Size - sizeof(MDL))/sizeof(ULONG));
   
   for (i=0; i < PageCount; i++)
   {
      ((PULONG)(Mdl + 1))[i] =
         (MmGetPhysicalAddress((char*)Mdl->StartVa + (i * PAGE_SIZE))).u.LowPart;
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
   
   /* SkipBytes must be a multiple of the page size */
   assert((SkipBytes.QuadPart % PAGE_SIZE) == 0);
   
   DPRINT1("MmAllocatePagesForMdl(): Unimplemented.\n");
   return(NULL);
}

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

   DPRINT1("MmFreePagesFromMdl(): Unimplemented.\n");
}

PVOID STDCALL
MmMapLockedPagesSpecifyCache ( IN PMDL Mdl,
                               IN KPROCESSOR_MODE AccessMode,
                               IN MEMORY_CACHING_TYPE CacheType,
                               IN PVOID BaseAddress,
                               IN ULONG BugCheckOnFailure,
                               IN ULONG Priority )
{
   DPRINT1("MmMapLockedPagesSpecifyCache(): Ignoring extra parameters.\n");
   return MmMapLockedPages (Mdl, AccessMode);
}

/* EOF */









