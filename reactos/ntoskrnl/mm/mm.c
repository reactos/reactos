/* $Id: mm.c,v 1.30 2000/05/24 22:29:36 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mm.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal/io.h>
#include <internal/i386/segment.h>
#include <internal/stddef.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <internal/string.h>
#include <internal/io.h>

#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/*
 * Size of extended memory (kb) (fixed for now)
 */
#define EXTENDED_MEMORY_SIZE  (3*1024*1024)

/*
 * Compiler defined symbol 
 */
extern unsigned int stext;
extern unsigned int etext;
extern unsigned int end;

static BOOLEAN IsThisAnNtAsSystem = FALSE;
static MM_SYSTEM_SIZE MmSystemSize = MmSmallSystem;

extern unsigned int etext;
extern unsigned int _bss_end__;

static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;

ULONG EXPORTED MmUserProbeAddress [PAGESIZE] = {0,}; /* FIXME */
PVOID EXPORTED MmHighestUserAddress = NULL; /* FIXME */

/* FUNCTIONS ****************************************************************/

VOID MiShutdownMemoryManager(VOID)
{
}

VOID MmInitVirtualMemory(boot_param* bp)
/*
 * FUNCTION: Intialize the memory areas list
 * ARGUMENTS:
 *           bp = Pointer to the boot parameters
 *           kernel_len = Length of the kernel
 */
{
   unsigned int kernel_len = bp->end_mem - bp->start_mem;
   PVOID BaseAddress;
   ULONG Length;
   ULONG ParamLength = kernel_len;
   
   DPRINT("MmInitVirtualMemory(%x)\n",bp);
   
   MmInitMemoryAreas();
   ExInitNonPagedPool(KERNEL_BASE + PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   
   
   /*
    * Setup the system area descriptor list
    */
   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG)&etext)) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_text_desc);
   
   Length = PAGE_ROUND_UP(((ULONG)&_bss_end__)) - 
            PAGE_ROUND_UP(((ULONG)&etext));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&etext));
   DPRINT("BaseAddress %x\n",BaseAddress);
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),		      
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc);
   
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&end));
   Length = ParamLength;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),		      
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_param_desc);

   BaseAddress = (PVOID)(KERNEL_BASE + PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   Length = NONPAGED_POOL_SIZE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_pool_desc);

//   MmDumpMemoryAreas();
   DPRINT("MmInitVirtualMemory() done\n");
}

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG Address)
{
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   
   DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);
   
   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
     {
	DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
	return(STATUS_UNSUCCESSFUL);
     }
   if (PsGetCurrentProcess() == NULL)
     {
	DbgPrint("No current process\n");
	return(STATUS_UNSUCCESSFUL);
     }
   
   /*
    * Find the memory area for the faulting address
    */
   if (Address >= KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (Mode != KernelMode)
	  {
	     DbgPrint("%s:%d\n",__FILE__,__LINE__);
	     return(STATUS_UNSUCCESSFUL);
	  }
	AddressSpace = MmGetKernelAddressSpace();
     }
   else
     {
	AddressSpace = &PsGetCurrentProcess()->Pcb.AddressSpace;
     }
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	MmUnlockAddressSpace(AddressSpace);
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmNotPresentFaultSectionView(AddressSpace,
					      MemoryArea, 
					      (PVOID)Address);
	break;
	
      case MEMORY_AREA_COMMIT:
	Status = MmNotPresentFaultVirtualMemory(AddressSpace,
						MemoryArea,
						(PVOID)Address);
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   DPRINT("Completed page fault handling\n");
   MmUnlockAddressSpace(AddressSpace);
   return(Status);
}

BOOLEAN STDCALL MmIsThisAnNtAsSystem(VOID)
{
   return(IsThisAnNtAsSystem);
}

MM_SYSTEM_SIZE STDCALL MmQuerySystemSize(VOID)
{
   return(MmSystemSize);
}

void MmInitialize(boot_param* bp, ULONG LastKernelAddress)
/*
 * FUNCTION: Initalize memory managment
 */
{
   unsigned int first_krnl_phys_addr;
   unsigned int last_krnl_phys_addr;
   int i;
   unsigned int kernel_len;
   
   DPRINT("MmInitialize(bp %x, LastKernelAddress %x)\n", bp, 
	  LastKernelAddress);

   /*
    * Unmap low memory
    */
   MmDeletePageTable(NULL, 0);
   
   /*
    * Free all pages not used for kernel memory
    * (we assume the kernel occupies a continuous range of physical
    * memory)
    */
   first_krnl_phys_addr = bp->start_mem;
   last_krnl_phys_addr = bp->end_mem;
   DPRINT("first krnl %x\nlast krnl %x\n",first_krnl_phys_addr,
	  last_krnl_phys_addr);
   
   /*
    * Free physical memory not used by the kernel
    */
   LastKernelAddress = (ULONG)MmInitializePageList(
					   (PVOID)first_krnl_phys_addr,
					   (PVOID)last_krnl_phys_addr,
					   1024,
					   PAGE_ROUND_UP(LastKernelAddress));
   kernel_len = last_krnl_phys_addr - first_krnl_phys_addr;
   
   /*
    * Create a trap for null pointer references and protect text
    * segment
    */
   CHECKPOINT;
   DPRINT("stext %x etext %x\n",(int)&stext,(int)&etext);
   for (i=PAGE_ROUND_UP(((int)&stext));
	i<PAGE_ROUND_DOWN(((int)&etext));i=i+PAGESIZE)
     {
	MmSetPageProtect(NULL,
			 (PVOID)i,
			 PAGE_EXECUTE_READ);
     }
   
   DPRINT("Invalidating between %x and %x\n",
	  LastKernelAddress,
	  KERNEL_BASE + PAGE_TABLE_SIZE);
   for (i=(LastKernelAddress); 
	i<(KERNEL_BASE + PAGE_TABLE_SIZE); 
	i=i+PAGESIZE)
     {
	MmSetPage(NULL, (PVOID)(i), PAGE_NOACCESS, 0);
     }
   DPRINT("Almost done MmInit()\n");
   
   /*
    * Intialize memory areas
    */
   MmInitVirtualMemory(bp);
}

VOID MmInitSystem (ULONG Phase, boot_param* bp, ULONG LastKernelAddress)
{
   if (Phase == 0)
     {
	/* Phase 0 Initialization */
	MmInitializeKernelAddressSpace();
	MmInitialize (bp, LastKernelAddress);
     }
   else
     {
	/* Phase 1 Initialization */
	MmInitSectionImplementation();
	MmInitPagingFile();
     }
}


/* Miscellanea functions: they may fit somewhere else */

DWORD
STDCALL
MmAdjustWorkingSetSize (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED;
	return (0);
}


DWORD
STDCALL
MmDbgTranslatePhysicalAddress (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (0);
}


NTSTATUS
STDCALL
MmGrowKernelStack (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


BOOLEAN
STDCALL
MmSetAddressRangeModified (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}

/* EOF */
