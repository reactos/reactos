/* $Id: mdl.c,v 1.61 2004/04/10 22:35:25 gdalsnes Exp $
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
 * @unimplemented
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
 */
{
   ULONG i;
   PULONG MdlPages;

   /*
    * FIXME: I don't know whether this right, but it looks sensible 
    */
   if ((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) ||
         (Mdl->MdlFlags & MDL_IO_PAGE_READ))
   {
      return;
   }

   /*
    * FIXME: Seems sensible 
    */
   if (!(Mdl->MdlFlags & MDL_PAGES_LOCKED))
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
   Mdl->MdlFlags = Mdl->MdlFlags & (~MDL_PAGES_LOCKED);
}

/*
 * @implemented
 */
PVOID STDCALL
MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Maps the physical pages described by a given MDL
 * ARGUMENTS:
 *       Mdl = Points to an MDL updated by MmProbeAndLockPages
 *       AccessMode = Specifies the portion of the address space to map the
 *                    pages.
 * RETURNS: The base virtual address that maps the locked pages for the
 * range described by the MDL
 */
{
   PVOID Base;
   ULONG i;
   PULONG MdlPages;
   KIRQL oldIrql;
   ULONG RegionSize;
   ULONG StartingOffset;
   PEPROCESS CurrentProcess, OldProcess;

   DPRINT("MmMapLockedPages(Mdl %x, AccessMode %x)\n", Mdl, AccessMode);

   if ((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) && AccessMode != UserMode)
   {
      return(Mdl->MappedSystemVa);
   }

   /* Calculate the number of pages required. */
   RegionSize = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;

   if (AccessMode == UserMode)
   {
      MEMORY_AREA *Result;
      LARGE_INTEGER BoundaryAddressMultiple;
      NTSTATUS Status;

      BoundaryAddressMultiple.QuadPart = 0;
      Base = NULL;

      CurrentProcess = OldProcess = PsGetCurrentProcess();
      if (Mdl->Process != CurrentProcess)
      {
         KeAttachProcess(Mdl->Process);
         CurrentProcess = Mdl->Process;
      }

      MmLockAddressSpace(&CurrentProcess->AddressSpace);
      Status = MmCreateMemoryArea(CurrentProcess,
                                  &CurrentProcess->AddressSpace,
                                  MEMORY_AREA_MDL_MAPPING,
                                  &Base,
                                  RegionSize * PAGE_SIZE,
                                  0, /* PAGE_READWRITE? */
                                  &Result,
                                  FALSE,
                                  FALSE,
                                  BoundaryAddressMultiple);
      MmUnlockAddressSpace(&CurrentProcess->AddressSpace);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
         /* FIXME: handle this? */
      }
   }
   else
   {
      CurrentProcess = OldProcess = NULL;

      /* Allocate that number of pages from the mdl mapping region. */
      KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);

      StartingOffset = RtlFindClearBitsAndSet(&MiMdlMappingRegionAllocMap, RegionSize, MiMdlMappingRegionHint);

      if (StartingOffset == 0xffffffff)
      {
         DPRINT1("Out of MDL mapping space\n");
         KEBUGCHECK(0);
      }

      Base = (char*)MiMdlMappingRegionBase + StartingOffset * PAGE_SIZE;

      if (MiMdlMappingRegionHint == StartingOffset)
      {
         MiMdlMappingRegionHint +=RegionSize;
      }

      KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
   }



   /* Set the virtual mappings for the MDL pages. */
   MdlPages = (PULONG)(Mdl + 1);
   for (i = 0; i < RegionSize; i++)
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
         KEBUGCHECK(0);
      }
   }

   if (AccessMode == UserMode && CurrentProcess != OldProcess)
   {
      KeDetachProcess();
   }

   /* Mark the MDL has having being mapped. */
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = (char*)Base + Mdl->ByteOffset;
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
 */
{
   KIRQL oldIrql;
   ULONG i;
   ULONG RegionSize;
   ULONG Base;
   PEPROCESS CurrentProcess, OldProcess;

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

   if ((ULONG_PTR)BaseAddress >= KERNEL_BASE)
   {
      CurrentProcess = OldProcess = NULL;
   }
   else
   {
      CurrentProcess = OldProcess = PsGetCurrentProcess();
      if (Mdl->Process != CurrentProcess)
      {
         KeAttachProcess(Mdl->Process);
         CurrentProcess = Mdl->Process;
      }
   }

   /* Calculate the number of pages we mapped. */
   RegionSize = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;
#if defined(__GNUC__)

   BaseAddress -= Mdl->ByteOffset;
#else

   {
      char* pTemp = BaseAddress;
      pTemp -= Mdl->ByteOffset;
      BaseAddress = pTemp;
   }
#endif

   /* Unmap all the pages. */
   for (i = 0; i < RegionSize; i++)
   {
      MmDeleteVirtualMapping(NULL,
                             (char*)BaseAddress + (i * PAGE_SIZE),
                             FALSE,
                             NULL,
                             NULL);
   }

   if ((DWORD)BaseAddress >= KERNEL_BASE)
   {
      KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);
      /* Deallocate all the pages used. */
      Base = (ULONG)((char*)BaseAddress - (char*)MiMdlMappingRegionBase) / PAGE_SIZE;

      RtlClearBits(&MiMdlMappingRegionAllocMap, Base, RegionSize);

      MiMdlMappingRegionHint = min (MiMdlMappingRegionHint, Base);

      KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
   }
   else
   {
      MEMORY_AREA *Marea;

      Marea = MmOpenMemoryAreaByAddress( &CurrentProcess->AddressSpace, BaseAddress );
      if (Marea == NULL)
      {
         DPRINT1( "Couldn't open memory area when unmapping user-space pages!\n" );
         KEBUGCHECK(0);
      }

      MmFreeMemoryArea( &CurrentProcess->AddressSpace, Marea->BaseAddress, 0, NULL, NULL );

      if (CurrentProcess != OldProcess)
      {
         KeDetachProcess();
      }
   }

   /* Reset the MDL state. */
   Mdl->MdlFlags = Mdl->MdlFlags & ~MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = NULL;
}


VOID
MmBuildMdlFromPages(PMDL Mdl, PULONG Pages)
{
   ULONG i;
   PULONG MdlPages;

   Mdl->MdlFlags = Mdl->MdlFlags |
                   (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

   MdlPages = (PULONG)(Mdl + 1);

   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGE_SIZE);i++)
   {
      MdlPages[i] = Pages[i];
   }
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
 */
{
   PULONG MdlPages;
   ULONG i, j;
   ULONG NrPages;
   NTSTATUS Status;
   KPROCESSOR_MODE Mode;
   PEPROCESS CurrentProcess = NULL;

   DPRINT("MmProbeAndLockPages(Mdl %x)\n", Mdl);

   /*
    * FIXME: Check behaviour against NT
    */
   if (Mdl->MdlFlags & MDL_PAGES_LOCKED)
   {
      return;
   }



   if (Mdl->StartVa >= (PVOID)KERNEL_BASE)
   {
      Mode = KernelMode;
   }
   else
   {
      Mode = UserMode;
      CurrentProcess = PsGetCurrentProcess();
      if (Mdl->Process != CurrentProcess)
      {
         KeAttachProcess(Mdl->Process);
      }
   }

   /*
    * Lock the pages
    */

   MmLockAddressSpace(&Mdl->Process->AddressSpace);
   MdlPages = (ULONG *)(Mdl + 1);
   NrPages = PAGE_ROUND_UP(Mdl->ByteOffset + Mdl->ByteCount) / PAGE_SIZE;
   for (i = 0; i < NrPages; i++)
   {
      PVOID Address;

      Address = (char*)Mdl->StartVa + (i*PAGE_SIZE);

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
   MmUnlockAddressSpace(&Mdl->Process->AddressSpace);
   if (Mode == UserMode && Mdl->Process != CurrentProcess)
   {
      KeDetachProcess();
   }
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_PAGES_LOCKED;
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
 */
{
   ULONG va;
   Mdl->MdlFlags = Mdl->MdlFlags |
                   (MDL_SOURCE_IS_NONPAGED_POOL | MDL_PAGES_LOCKED);
   for (va=0; va < ((Mdl->Size - sizeof(MDL)) / sizeof(ULONG)); va++)
   {
      ((PULONG)(Mdl + 1))[va] =
         (MmGetPhysicalAddress((char*)Mdl->StartVa + (va * PAGE_SIZE))).u.LowPart;
   }
   Mdl->MappedSystemVa = (char*)Mdl->StartVa + Mdl->ByteOffset;
}


/*
 * @implemented
 */
PMDL STDCALL
MmCreateMdl (PMDL MemoryDescriptorList,
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
   if (MemoryDescriptorList == NULL)
   {
      ULONG Size;

      Size = MmSizeOfMdl(Base,Length);
      MemoryDescriptorList =
         (PMDL)ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MDL);
      if (MemoryDescriptorList == NULL)
      {
         return(NULL);
      }
   }

   MmInitializeMdl(MemoryDescriptorList, (char*)Base, Length);

   return(MemoryDescriptorList);
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
   DPRINT1("MmAllocatePagesForMdl(): Unimplemented.\n");
   return(NULL);
}

VOID STDCALL
MmFreePagesFromMdl ( IN PMDL Mdl )
{
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









