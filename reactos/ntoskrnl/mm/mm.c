/*
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

/* FUNCTIONS ****************************************************************/

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
   ExInitNonPagedPool(KERNEL_BASE+ PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   
   
   /*
    * Setup the system area descriptor list
    */
   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG)&etext)) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   MmCreateMemoryArea(KernelMode,NULL,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_text_desc);
   
   Length = PAGE_ROUND_UP(((ULONG)&_bss_end__)) - 
            PAGE_ROUND_UP(((ULONG)&etext));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&etext));
   DPRINT("BaseAddress %x\n",BaseAddress);
   MmCreateMemoryArea(KernelMode,
		      NULL,
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc);
   
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&end));
   Length = ParamLength;
   MmCreateMemoryArea(KernelMode,NULL,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_param_desc);
   
   BaseAddress = (PVOID)(KERNEL_BASE + PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   Length = NONPAGED_POOL_SIZE;
   MmCreateMemoryArea(KernelMode,NULL,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_pool_desc);
   
//   MmDumpMemoryAreas();
   CHECKPOINT;
   
//  while (inb_p(0x60)!=0x1); inb_p(0x60);
   
   MmInitSectionImplementation();
}

NTSTATUS MmCommitedSectionHandleFault(MEMORY_AREA* MemoryArea, PVOID Address)
{
   MmSetPage(PsGetCurrentProcess(),
	     Address,
	     MemoryArea->Attributes,
	     (ULONG)MmAllocPage());
   return(STATUS_SUCCESS);
}

NTSTATUS MmSectionHandleFault(MEMORY_AREA* MemoryArea, PVOID Address)
{
   LARGE_INTEGER Offset;
   IO_STATUS_BLOCK IoStatus;
   
   DPRINT("MmSectionHandleFault(MemoryArea %x, Address %x)\n",
	  MemoryArea,Address);
   
   MmSetPage(NULL,
	     Address,
	     MemoryArea->Attributes,
	     (ULONG)MmAllocPage());
   
   Offset.QuadPart = (Address - MemoryArea->BaseAddress) + 
     MemoryArea->Data.SectionData.ViewOffset;
   
   DPRINT("MemoryArea->Data.SectionData.Section->FileObject %x\n",
	    MemoryArea->Data.SectionData.Section->FileObject);
   
   if (MemoryArea->Data.SectionData.Section->FileObject == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   IoPageRead(MemoryArea->Data.SectionData.Section->FileObject,
	      (PVOID)Address,
	      &Offset,
	      &IoStatus);
   
   DPRINT("Returning from MmSectionHandleFault()\n");
   
   return(STATUS_SUCCESS);
}

asmlinkage int page_fault_handler(unsigned int cs,
                                  unsigned int eip)
/*
 * FUNCTION: Handle a page fault
 */
{
   KPROCESSOR_MODE FaultMode;
   MEMORY_AREA* MemoryArea;
   KIRQL oldlvl;
   NTSTATUS Status;
   unsigned int cr2;
   
   /*
    * Get the address for the page fault
    */
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));                
   DPRINT("Page fault at address %x with eip %x in process %x\n",cr2,eip,
	  PsGetCurrentProcess());

   cr2 = PAGE_ROUND_DOWN(cr2);
   
//   DbgPrint("(%%");
   
   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
     {
	DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
	return(0);
//	KeBugCheck(0);
     }
   
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   
   /*
    * Find the memory area for the faulting address
    */
   if (cr2 >= KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (cs != KERNEL_CS)
	  {
	     DbgPrint("%s:%d\n",__FILE__,__LINE__);
	     return(0);
	  }
	FaultMode = UserMode;
     }
   else
     {
	FaultMode = KernelMode;
     }
   
   MemoryArea = MmOpenMemoryAreaByAddress(PsGetCurrentProcess(),(PVOID)cr2);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	return(0);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmSectionHandleFault(MemoryArea, (PVOID)cr2);
	break;
	
      case MEMORY_AREA_COMMIT:
	Status = MmCommitedSectionHandleFault(MemoryArea,(PVOID)cr2);
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   DPRINT("Completed page fault handling\n");
   if (NT_SUCCESS(Status))
     {
	KeLowerIrql(oldlvl);
     }
//   DbgPrint("%%)");
   return(NT_SUCCESS(Status));
}

BOOLEAN MmIsThisAnNtAsSystem(VOID)
{
   return(IsThisAnNtAsSystem);
}

MM_SYSTEM_SIZE MmQuerySystemSize(VOID)
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
