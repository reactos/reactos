 /*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         kernel/mm/mdl.cc
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

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


VOID MmUnlockPages(PMDL MemoryDescriptorList)
{
   UNIMPLEMENTED;
}

PVOID MmMapLockedPages(PMDL MemoryDescriptorList, KPROCESSOR_MODE AccessMode)
{
   UNIMPLEMENTED;
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
   /*
    * Find the memory area containing the buffer
    */
   ULONG* mdl_pages=NULL;
   int i;
   memory_area* marea=find_first_marea(memory_area_list_head,
				       (ULONG)Mdl->StartVa,
				       Mdl->ByteCount);
   
   /*
    * Check the area is valid
    */
   if (marea==NULL || (marea->base+marea->length) < ((ULONG)Mdl->StartVa))
     {
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
   mdl_pages = (ULONG *)(Mdl + sizeof(MDL));
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE);i++)
     {
	if (!is_page_present(PAGE_ROUND_DOWN(Mdl->StartVa) + (i*PAGESIZE)))
	  {
	     marea->load_page(marea,PAGE_ROUND_DOWN(Mdl->StartVa) + (i*PAGESIZE)
			      - marea->base);
	  }
	mdl_pages[i]=MmGetPhysicalAddress((PVOID)(PAGE_ROUND_DOWN(Mdl->StartVa)
					  +(i*PAGESIZE)));
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
   unsigned int len=PAGE_ROUND_UP(Length)/PAGESIZE;
   
   if (!IS_PAGE_ALIGNED(Base))
     {
	len++;
     }
   
   return(sizeof(MDL)+(len*sizeof(ULONG)));
}

PVOID MmGetMdlVirtualAddress(PMDL Mdl)
{
   return(Mdl->StartVa + Mdl->ByteOffset);
}

PVOID MmGetSystemAddressForMdl(PMDL Mdl)
{
   Mdl->MappedSystemVa = MmMapLockedPages(Mdl,KernelMode);
   return(Mdl->MappedSystemVa);
}

VOID MmBuildMdlForNonPagedPool(PMDL Mdl)
{
   int va;
   for (va=0; va<Mdl->Size; va++)
     {
	((PULONG)(Mdl + 1))[va] = MmGetPhysicalAddress(
					Mdl->StartVa+ (va * PAGESIZE));
     }
}

VOID MmInitializeMdl(PMDL MemoryDescriptorList, PVOID Base, ULONG Length)
{
   memset(MemoryDescriptorList,0,sizeof(MDL));
   MemoryDescriptorList->StartVa = PAGE_ROUND_DOWN(Base);
   MemoryDescriptorList->ByteOffset = Base - PAGE_ROUND_DOWN(Base);
   MemoryDescriptorList->MdlFlags = 0;
   MemoryDescriptorList->ByteCount = Length;
   MemoryDescriptorList->Size = PAGE_ROUND_UP(Length) / PAGESIZE;
   if (!IS_PAGE_ALIGNED(Base))
     {
	MemoryDescriptorList->Size = MemoryDescriptorList->Size
	  + sizeof(ULONG);
     }
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
	MemoryDescriptorList = (PMDL)ExAllocatePool(NonPagedPool,sizeof(MDL));
	if (MemoryDescriptorList==NULL)
	  {
	     return(NULL);
	  }
     }
   
   MmInitializeMdl(MemoryDescriptorList,Base,Length);
   
   return(MemoryDescriptorList);
}
