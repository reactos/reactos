 /*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/mdl.c
 * PURPOSE:      Manipulates MDLs
 * PROGRAMMER:   David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/hal/page.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


VOID MmUnlockPages(PMDL MemoryDescriptorList)
/*
 * FUNCTION: Unlocks the physical pages described by a given MDL
 * ARGUMENTS:
 *      MemoryDescriptorList = MDL describing the buffer to be unlocked
 */
{
   UNIMPLEMENTED;
}

PVOID MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
{
   PVOID base;
   unsigned int i;
   ULONG* mdl_pages=NULL;
   
   DPRINT("Mdl->ByteCount %x\n",Mdl->ByteCount);
   DPRINT("PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE) %x\n",
	  PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE);
   
   base = VirtualAlloc((LPVOID)0,Mdl->ByteCount,MEM_COMMIT,
		       PAGE_SYSTEM + PAGE_EXECUTE_READWRITE);
   mdl_pages = (ULONG *)(Mdl + 1);
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE); i++)
     {
	DPRINT("Writing %x with physical address %x\n",
	       base+(i*PAGESIZE),mdl_pages[i]);
	DPRINT("&((PULONG)(Mdl+1))[i] %x\n",&mdl_pages[i]);
	set_page(base+(i*PAGESIZE),PA_READ + PA_SYSTEM,
		 mdl_pages[i]);
     }
   DPRINT("base %x\n",base);
   return(base + Mdl->ByteOffset);
}

VOID MmUnmapLockedPages(PVOID BaseAddress, PMDL MemoryDescriptorList)
{
   UNIMPLEMENTED;
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
   memory_area* marea;
   
   DPRINT("MmProbeAndLockPages(Mdl %x)\n",Mdl);
   DPRINT("StartVa %x\n",Mdl->StartVa);
   
   if (Mdl->StartVa > KERNEL_BASE)
     {
   	marea=find_first_marea(system_memory_area_list_head,
			       (ULONG)Mdl->StartVa,
			       Mdl->ByteCount);   
     }
   else
     {
	marea=find_first_marea(memory_area_list_head,
			       (ULONG)Mdl->StartVa,
			       Mdl->ByteCount);      
     }
   
   DPRINT("marea %x\n",marea);
  
   
   
   /*
    * Check the area is valid
    */
   if (marea==NULL || (marea->base+marea->length) < ((ULONG)Mdl->StartVa))
     {
	printk("Area is invalid\n");
	ExRaiseStatus(STATUS_INVALID_PARAMETER);
     }
   
   /*
    * Check the permissions
    */
   switch(Operation)
     {
      case IoReadAccess:
	if (marea->access&PAGE_GUARD || marea->access&PAGE_NOACCESS)
	  {
	     ExRaiseStatus(STATUS_INVALID_PARAMETER);
	  }
	break;
	
      case IoWriteAccess:
      case IoModifyAccess:
	if (marea->access&PAGE_GUARD || marea->access&PAGE_READONLY)
	  {
	     printk("Invalid area protections\n");
	     ExRaiseStatus(STATUS_INVALID_PARAMETER);
	  }
	break;
	
      default:
	printk("Invalid operation type at %s:%d in %s\n",__FILE__,__LINE__,
	       __FUNCTION__);
	KeBugCheck(UNEXPECTED_KERNEL_MODE_TRAP);
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
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE);i++)
     {
	if (!is_page_present(PAGE_ROUND_DOWN(Mdl->StartVa) + (i*PAGESIZE)))
	  {
	     marea->load_page(marea,PAGE_ROUND_DOWN(Mdl->StartVa) + (i*PAGESIZE)
			      - marea->base);
	  }
	mdl_pages[i]=MmGetPhysicalAddress((PVOID)(PAGE_ROUND_DOWN(Mdl->StartVa)
					  +(i*PAGESIZE)));
	DPRINT("mdl_pages[i] %x\n",mdl_pages[i]);
	DPRINT("&mdl_pages[i] %x\n",&mdl_pages[i]);
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
   for (va=0; va<Mdl->Size; va++)
     {
	((PULONG)(Mdl + 1))[va] = MmGetPhysicalAddress(
					Mdl->StartVa+ (va * PAGESIZE));
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
   MemoryDescriptorList->StartVa = PAGE_ROUND_DOWN(Base);
   MemoryDescriptorList->ByteOffset = Base - PAGE_ROUND_DOWN(Base);
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
