/* $Id: mdl.c,v 1.50 2003/06/19 19:01:01 gvg Exp $
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

VOID
MmInitializeMdlImplementation(VOID)
{
  MEMORY_AREA* Result;
  NTSTATUS Status;
  PVOID Buffer;

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
			      FALSE);
  if (!NT_SUCCESS(Status))
    {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      KeBugCheck(0);
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
	MmUnlockPage((LARGE_INTEGER)(LONGLONG)MdlPages[i]);
	MmDereferencePage((LARGE_INTEGER)(LONGLONG)MdlPages[i]);
     }   
   Mdl->MdlFlags = Mdl->MdlFlags & (~MDL_PAGES_LOCKED);
}

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
   
   DPRINT("MmMapLockedPages(Mdl %x, AccessMode %x)\n", Mdl, AccessMode);

   if (Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL)
     {
       return(Mdl->MappedSystemVa);
     }

   if (AccessMode == UserMode)
     {
       DPRINT1("MDL mapping to user-mode not yet handled.\n");
       KeBugCheck(0);
     }

   /* Calculate the number of pages required. */
   RegionSize = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;

   /* Allocate that number of pages from the mdl mapping region. */
   KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);

   StartingOffset = RtlFindClearBitsAndSet(&MiMdlMappingRegionAllocMap, RegionSize, MiMdlMappingRegionHint);
  
   if (StartingOffset == 0xffffffff)
   {
      DPRINT1("Out of MDL mapping space\n");
      KeBugCheck(0);
   }

   Base = MiMdlMappingRegionBase + StartingOffset * PAGE_SIZE;

   if (MiMdlMappingRegionHint == StartingOffset)
   {
       MiMdlMappingRegionHint +=RegionSize; 
   }

   KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);

   /* Set the virtual mappings for the MDL pages. */
   MdlPages = (PULONG)(Mdl + 1);
   for (i = 0; i < RegionSize; i++)
     {
       NTSTATUS Status;
       Status = MmCreateVirtualMapping(NULL,
				       (PVOID)((ULONG)Base+(i*PAGE_SIZE)),
				       PAGE_READWRITE,
				       (LARGE_INTEGER)(LONGLONG)MdlPages[i],
				       FALSE);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint("Unable to create virtual mapping\n");
	   KeBugCheck(0);
	 }
     }

   /* Mark the MDL has having being mapped. */
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = Base + Mdl->ByteOffset;
   return(Base + Mdl->ByteOffset);
}

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

  DPRINT("MmUnmapLockedPages(BaseAddress %x, Mdl %x)\n", BaseAddress, Mdl);

  /*
   * In this case, the MDL has the same system address as the base address
   * so there is no need to free it
   */
  if (Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL)
    {
      return;
    }

  /* Calculate the number of pages we mapped. */
  RegionSize = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;
  BaseAddress -= Mdl->ByteOffset;

  /* Unmap all the pages. */
  for (i = 0; i < RegionSize; i++)
    {
      MmDeleteVirtualMapping(NULL, 
			     BaseAddress + (i * PAGE_SIZE),
			     FALSE,
			     NULL,
			     NULL);
    }

  KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);
  /* Deallocate all the pages used. */
  Base = (ULONG)(BaseAddress - MiMdlMappingRegionBase) / PAGE_SIZE;
  
  RtlClearBits(&MiMdlMappingRegionAllocMap, Base, RegionSize);

  MiMdlMappingRegionHint = min (MiMdlMappingRegionHint, Base);

  KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
  
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
   PEPROCESS CurrentProcess;

   DPRINT("MmProbeAndLockPages(Mdl %x)\n", Mdl);
   
   /*
    * FIXME: Check behaviour against NT
    */
   if (Mdl->MdlFlags & MDL_PAGES_LOCKED)
     {
	return;
     }
   
   CurrentProcess = PsGetCurrentProcess();

   if (Mdl->Process != CurrentProcess)
     {
       KeAttachProcess(Mdl->Process);
     }

   if (Mdl->StartVa >= (PVOID)KERNEL_BASE)
     {
       Mode = KernelMode;
     }
   else
     {
       Mode = UserMode;
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
	
	Address = Mdl->StartVa + (i*PAGE_SIZE);       
	
	if (!MmIsPagePresent(NULL, Address))
	  {
	    Status = MmNotPresentFault(Mode, (ULONG)Address, TRUE);
	    if (!NT_SUCCESS(Status))
	      {
		for (j = 0; j < i; j++)
		  {
		    MmUnlockPage((LARGE_INTEGER)(LONGLONG)MdlPages[j]);
		    MmDereferencePage((LARGE_INTEGER)(LONGLONG)MdlPages[j]);
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
			MmUnlockPage((LARGE_INTEGER)(LONGLONG)MdlPages[j]);
			MmDereferencePage(
					 (LARGE_INTEGER)(LONGLONG)MdlPages[j]);
		  }
		ExRaiseStatus(Status);
	      }
	  }
	MdlPages[i] = MmGetPhysicalAddressForProcess(NULL, Address).u.LowPart;
	MmReferencePage((LARGE_INTEGER)(LONGLONG)MdlPages[i]);
     }
   MmUnlockAddressSpace(&Mdl->Process->AddressSpace);
   if (Mdl->Process != CurrentProcess)
     {
       KeDetachProcess();
     }
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_PAGES_LOCKED;
}


ULONG STDCALL MmSizeOfMdl (PVOID	Base,
			   ULONG	Length)
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


VOID STDCALL 
MmBuildMdlForNonPagedPool (PMDL	Mdl)
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
            (MmGetPhysicalAddress(Mdl->StartVa + (va * PAGE_SIZE))).u.LowPart;
     }
   Mdl->MappedSystemVa = Mdl->StartVa + Mdl->ByteOffset;
}


PMDL STDCALL 
MmCreateMdl (PMDL	MemoryDescriptorList,
	     PVOID	Base,
	     ULONG	Length)
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
   
   MmInitializeMdl(MemoryDescriptorList,Base,Length);
   
   return(MemoryDescriptorList);
}

VOID STDCALL 
MmMapMemoryDumpMdl (PVOID	Unknown0)
/*
 * FIXME: Has something to do with crash dumps. Do we want to implement
 * this?
 */
{
   UNIMPLEMENTED;
}

/* EOF */









