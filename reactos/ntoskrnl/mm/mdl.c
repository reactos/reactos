/* $Id: mdl.c,v 1.28 2001/02/10 22:51:10 dwelch Exp $
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
#include <internal/mmhal.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset)
{
   PULONG MdlPages;
   
   MdlPages = (PULONG)(Mdl + 1);
   
   return((PVOID)MdlPages[((ULONG)Offset) / PAGESIZE]);
}

VOID STDCALL MmUnlockPages(PMDL Mdl)
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
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount+Mdl->ByteOffset)/PAGESIZE); i++)
     {
	MmUnlockPage((PVOID)MdlPages[i]);
	MmDereferencePage((PVOID)MdlPages[i]);
     }   
   Mdl->MdlFlags = Mdl->MdlFlags & (~MDL_PAGES_LOCKED);
}

PVOID STDCALL MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Maps the physical pages described by a given MDL
 * ARGUMENTS:
 *       Mdl = Points to an MDL updated by MmProbeAndLockPages
 *       AccessMode = Specifies the access mode in which to map the MDL
 * RETURNS: The base virtual address that maps the locked pages for the
 * range described by the MDL
 * FIXME: What does AccessMode do?
 */
{
   PVOID Base;
   ULONG i;
   PULONG MdlPages;
   MEMORY_AREA* Result;
   NTSTATUS Status;
   
   DPRINT("MmMapLockedPages(Mdl %x, AccessMode %x)\n", Mdl, AccessMode);
   
   MmLockAddressSpace(MmGetKernelAddressSpace());
   
   Base = NULL;
   Status = MmCreateMemoryArea(NULL,
			       MmGetKernelAddressSpace(),
			       MEMORY_AREA_MDL_MAPPING,
			       &Base,
			       Mdl->ByteCount + Mdl->ByteOffset,
			       0,
			       &Result);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(MmGetKernelAddressSpace());
	KeBugCheck(0);
	return(STATUS_SUCCESS);
     }
   
   MdlPages = (PULONG)(Mdl + 1);
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount+Mdl->ByteOffset)/PAGESIZE); i++)
     {
	Status = MmCreateVirtualMapping(NULL,
					(PVOID)((ULONG)Base+(i*PAGESIZE)),
					PAGE_READWRITE,
					MdlPages[i]);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
     }
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = Base + Mdl->ByteOffset;
   return(Base + Mdl->ByteOffset);
}


VOID STDCALL MmUnmapLockedPages(PVOID BaseAddress, PMDL Mdl)
/*
 * FUNCTION: Releases a mapping set up by a preceding call to MmMapLockedPages
 * ARGUMENTS:
 *         BaseAddress = Base virtual address to which the pages were mapped
 *         MemoryDescriptorList = MDL describing the mapped pages
 */
{
   DPRINT("MmUnmapLockedPages(BaseAddress %x, Mdl %x)\n", Mdl, BaseAddress);
   MmLockAddressSpace(MmGetKernelAddressSpace());
   (VOID)MmFreeMemoryArea(MmGetKernelAddressSpace(),
			  BaseAddress - Mdl->ByteOffset,
			  Mdl->ByteCount,
			  NULL,
			  NULL);
   Mdl->MdlFlags = Mdl->MdlFlags & ~MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = NULL;
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}


VOID MmBuildMdlFromPages(PMDL Mdl, PULONG Pages)
{
   ULONG i;
   PULONG MdlPages;
   
   Mdl->MdlFlags = Mdl->MdlFlags | 
     (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
   
   MdlPages = (PULONG)(Mdl + 1);
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGESIZE);i++)
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
   ULONG i;
   PMEMORY_AREA MArea;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("MmProbeAndLockPages(Mdl %x)\n", Mdl);
   
   /*
    * FIXME: Check behaviour against NT
    */
   if (Mdl->MdlFlags & MDL_PAGES_LOCKED)
     {
	return;
     }
   
   if (Mdl->StartVa > (PVOID)KERNEL_BASE)
     {
	AddressSpace = MmGetKernelAddressSpace();
     }
   else
     {
	AddressSpace = &Mdl->Process->AddressSpace;
     }
   MmLockAddressSpace(AddressSpace);
   MArea = MmOpenMemoryAreaByAddress(AddressSpace,
				     Mdl->StartVa);

   /*
    * Check the area is valid
    */
   if (MArea == NULL)
     {
	DbgPrint("(%s:%d) Area is invalid\n",__FILE__,__LINE__);
	MmUnlockAddressSpace(AddressSpace);
	/* FIXME: ExRaiseStatus doesn't do anything sensible at the moment */
	ExRaiseStatus(STATUS_INVALID_PARAMETER);
     }

   /*
    * Lock the pages
    */
   MdlPages = (ULONG *)(Mdl + 1);
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGESIZE);i++)
     {
	PVOID Address;
	
	Address = Mdl->StartVa + (i*PAGESIZE);
	MdlPages[i] = (MmGetPhysicalAddress(Address)).u.LowPart;
	MmReferencePage((PVOID)MdlPages[i]);
	MmLockPage((PVOID)MdlPages[i]);
     }
   MmUnlockAddressSpace(AddressSpace);
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


VOID STDCALL MmBuildMdlForNonPagedPool (PMDL	Mdl)
/*
 * FUNCTION: Fills in the corresponding physical page array of a given 
 * MDL for a buffer in nonpaged system space
 * ARGUMENTS:
 *        Mdl = Points to an MDL that supplies a virtual address, 
 *              byte offset and length
 */
{
   int va;
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_SOURCE_IS_NONPAGED_POOL;
   for (va=0; va<Mdl->Size; va++)
     {
        ((PULONG)(Mdl + 1))[va] =
            (MmGetPhysicalAddress(Mdl->StartVa + (va * PAGESIZE))).u.LowPart;
     }
   Mdl->MappedSystemVa = Mdl->StartVa;
}


PMDL STDCALL MmCreateMdl (PMDL	MemoryDescriptorList,
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
	MemoryDescriptorList = (PMDL)ExAllocatePool(NonPagedPool,Size);
	if (MemoryDescriptorList == NULL)
	  {
	     return(NULL);
	  }
     }
   
   MmInitializeMdl(MemoryDescriptorList,Base,Length);
   
   return(MemoryDescriptorList);
}


VOID STDCALL MmMapMemoryDumpMdl (PVOID	Unknown0)
/*
 * FIXME: Has something to do with crash dumps. Do we want to implement
 * this?
 */
{
   UNIMPLEMENTED;
}

/* EOF */









