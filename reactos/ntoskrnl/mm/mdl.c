/*
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
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


VOID MmUnlockPages(PMDL MemoryDescriptorList)
/*
 * FUNCTION: Unlocks the physical pages described by a given MDL
 * ARGUMENTS:
 *      MemoryDescriptorList = MDL describing the buffer to be unlocked
 * NOTES: The memory described by the specified MDL must have been locked
 * previously by a call to MmProbeAndLockPages. As the pages unlocked, the
 * MDL is updated
 */
{
   /* It is harmless to leave this one as a stub */
}

PVOID MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Maps the physical pages described by a given MDL
 * ARGUMENTS:
 *       Mdl = Points to an MDL updated by MmProbeAndLockPages
 *       AccessMode = Specifies the access mode in which to map the MDL
 * RETURNS: The base virtual address that maps the locked pages for the
 * range described by the MDL
 */
{
   PVOID base = NULL;
   unsigned int i;
   ULONG* mdl_pages=NULL;
   MEMORY_AREA* Result;
   
   DPRINT("Mdl->ByteCount %x\n",Mdl->ByteCount);
   DPRINT("PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE) %x\n",
	  PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE);
   
   MmCreateMemoryArea(KernelMode,
		      PsGetCurrentProcess(),
		      MEMORY_AREA_MDL_MAPPING,
		      &base,
		      Mdl->ByteCount + Mdl->ByteOffset,
		      0,
		      &Result);
   CHECKPOINT;
   mdl_pages = (ULONG *)(Mdl + 1);
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount+Mdl->ByteOffset)/PAGESIZE); i++)
     {
	DPRINT("Writing %x with physical address %x\n",
	       base+(i*PAGESIZE),mdl_pages[i]);
	MmSetPage(NULL,
		  (PVOID)((DWORD)base+(i*PAGESIZE)),
		  PAGE_READWRITE,
		  mdl_pages[i]);
     }
   DPRINT("base %x\n",base);
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
   return(base + Mdl->ByteOffset);
}

VOID MmUnmapLockedPages(PVOID BaseAddress, PMDL Mdl)
/*
 * FUNCTION: Releases a mapping set up by a preceding call to MmMapLockedPages
 * ARGUMENTS:
 *         BaseAddress = Base virtual address to which the pages were mapped
 *         MemoryDescriptorList = MDL describing the mapped pages
 */
{
   (void)MmFreeMemoryArea(PsGetCurrentProcess(),BaseAddress-Mdl->ByteOffset,
			  Mdl->ByteCount,FALSE);
}

VOID MmPrepareMdlForReuse(PMDL Mdl)
{
   UNIMPLEMENTED;
}

VOID KeFlushIoBuffers(PMDL Mdl, BOOLEAN ReadOperation, BOOLEAN DmaOperation)
{
   /* NOP on the x86 */
   /* See ntddk.h from Windows 98 DDK */
}

VOID MmProbeAndLockPages(PMDL Mdl, KPROCESSOR_MODE AccessMode,
			 LOCK_OPERATION Operation)
/*
 * FUNCTION: Probes the specified pages, makes them resident and locks them
 * ARGUMENTS:
 *          Mdl = MDL to probe
 *          AccessMode = Access at which to probe the buffer
 *          Operation = Operation to probe for
 */
{
   ULONG* mdl_pages=NULL;
   int i;
   MEMORY_AREA* marea;
   PVOID Address;
   
   DPRINT("MmProbeAndLockPages(Mdl %x)\n",Mdl);
   DPRINT("StartVa %x\n",Mdl->StartVa);
   
   marea = MmOpenMemoryAreaByAddress(PsGetCurrentProcess(),
				     Mdl->StartVa);
   DPRINT("marea %x\n",marea);
  
   
   
   /*
    * Check the area is valid
    */
   if (marea==NULL )
     {
	DbgPrint("(%s:%d) Area is invalid\n",__FILE__,__LINE__);
	ExRaiseStatus(STATUS_INVALID_PARAMETER);
     }
      
   /*
    * Lock the memory area
    * (We can't allow it to be freed while an I/O operation to it is
    * ongoing)
    */
   
   /*
    * Lock the pages
    */
   mdl_pages = (ULONG *)(Mdl + 1);
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGESIZE);i++)
     {
	Address = Mdl->StartVa + (i*PAGESIZE);
	mdl_pages[i] = GET_LARGE_INTEGER_LOW_PART(MmGetPhysicalAddress(Address));
	DPRINT("mdl_pages[i] %x\n",mdl_pages[i]);
     }
}

ULONG MmGetMdlByteCount(PMDL Mdl)
/*
 *
 */
{
   return(Mdl->ByteCount);
}

ULONG MmGetMdlByteOffset(PMDL Mdl)
/*
 * FUNCTION: Returns the byte offset within its page of the buffer described
 *           by the given MDL
 * ARGUMENTS:
 *           Mdl = the mdl to query
 * RETURNS: The offset in bytes
 */
{
   return(Mdl->ByteOffset);
}

ULONG MmSizeOfMdl(PVOID Base, ULONG Length)
/*
 * FUNCTION: Returns the number of bytes to allocate for an MDL describing
 * the given address range
 * ARGUMENTS:
 *         Base = base virtual address
 *         Length = number of bytes to map
 */
{
   unsigned int len=ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base,Length);
   
   DPRINT("MmSizeOfMdl() %x\n",sizeof(MDL)+(len*sizeof(ULONG)));
   return(sizeof(MDL)+(len*sizeof(ULONG)));
}

PVOID MmGetMdlVirtualAddress(PMDL Mdl)
{
   return(Mdl->StartVa + Mdl->ByteOffset);
}

PVOID MmGetSystemAddressForMdl(PMDL Mdl)
/*
 * FUNCTION: Returns a nonpaged system-space virtual address for the buffer
 * described by the MDL. It maps the physical pages described by a given
 * MDL into system space, if they are not already mapped to system space.
 * ARGUMENTS:
 *        Mdl = Mdl to map
 * RETURNS: The base system-space virtual address that maps the physical
 * pages described by the given MDL.
 */
{
   if (!( (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) ||
	  (Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) ))
     {
	Mdl->MappedSystemVa = MmMapLockedPages(Mdl,KernelMode);
     }
   return(Mdl->MappedSystemVa);
}

VOID MmBuildMdlForNonPagedPool(PMDL Mdl)
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
	((PULONG)(Mdl + 1))[va] = GET_LARGE_INTEGER_LOW_PART(
          MmGetPhysicalAddress(Mdl->StartVa + (va * PAGESIZE)));
     }
   Mdl->MappedSystemVa = Mdl->StartVa;
}

VOID MmInitializeMdl(PMDL MemoryDescriptorList, PVOID Base, ULONG Length)
/*
 * FUNCTION: Initializes the header of an MDL
 * ARGUMENTS:
 *        MemoryDescriptorList = Points to the MDL to be initialized
 *        BaseVa = Points to the base virtual address of a buffer
 *        Length = Specifies the length (in bytes) of a buffer
 */
{
   memset(MemoryDescriptorList,0,sizeof(MDL));
   MemoryDescriptorList->StartVa = (PVOID)PAGE_ROUND_DOWN(Base);
   MemoryDescriptorList->ByteOffset = (ULONG)(Base - PAGE_ROUND_DOWN(Base));
   MemoryDescriptorList->MdlFlags = 0;
   MemoryDescriptorList->ByteCount = Length;
   MemoryDescriptorList->Size = sizeof(MDL) + 
             (ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base,Length) * sizeof(ULONG));
   MemoryDescriptorList->Process = PsGetCurrentProcess();
}

PMDL MmCreateMdl(PMDL MemoryDescriptorList, PVOID Base, ULONG Length)
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
	if (MemoryDescriptorList==NULL)
	  {
	     return(NULL);
	  }
     }
   
   MmInitializeMdl(MemoryDescriptorList,Base,Length);
   
   return(MemoryDescriptorList);
}
