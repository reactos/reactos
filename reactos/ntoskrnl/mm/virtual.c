/*
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     implementing the Virtualxxx section of the win32 api
 * PROGRAMMER:  David Welch
 * UPDATE HISTORY:
 *              09/4/98: Created
 *              10/6/98: Corrections from Fatahi (i_fatahi@hotmail.com)
 */
 
/* INCLUDE *****************************************************************/

#include <windows.h>

#include <internal/hal/segment.h>
#include <internal/mm.h>
#include <internal/hal/page.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

extern unsigned int etext;
extern unsigned int end;

/*
 * These two are statically declared because mm is initalized before the
 * memory pool
 */
static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;

/* FUNCTIONS ****************************************************************/

void VirtualInit(boot_param* bp)
/*
 * FUNCTION: Intialize the memory areas list
 * ARGUMENTS:
 *           bp = Pointer to the boot parameters
 *           kernel_len = Length of the kernel
 */
{
   unsigned int kernel_len = bp->end_mem - bp->start_mem;
   ULONG BaseAddress;
   ULONG Length;
   ULONG ParamLength = kernel_len;
   
   DPRINT("VirtualInit() %x\n",bp);
   
   MmInitMemoryAreas();
   ExInitNonPagedPool(KERNEL_BASE+ PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   
   
   /*
    * Setup the system area descriptor list
    */
   BaseAddress = KERNEL_BASE;
   Length = ((ULONG)&etext) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   MmCreateMemoryArea(KernelMode,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_text_desc);
   
   Length = ((ULONG)&end) - ((ULONG)&etext);
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = PAGE_ROUND_UP(((ULONG)&etext));
   MmCreateMemoryArea(KernelMode,
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc);
   
   
   BaseAddress = PAGE_ROUND_UP(((ULONG)&end));
   Length = ParamLength;
   MmCreateMemoryArea(KernelMode,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_param_desc);
   
   BaseAddress = KERNEL_BASE+ PAGE_ROUND_UP(kernel_len) + PAGESIZE;
   Length = NONPAGED_POOL_SIZE;
   MmCreateMemoryArea(KernelMode,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_pool_desc);
   
   MmDumpMemoryAreas();
   CHECKPOINT;
}

NTSTATUS MmSectionHandleFault(MEMORY_AREA* MemoryArea, ULONG Address)
{
   set_page(Address,0x7,get_free_page());
   return(STATUS_SUCCESS);
}

asmlinkage int page_fault_handler(unsigned int edi,
                                  unsigned int esi, unsigned int ebp,
                                  unsigned int esp, unsigned int ebx,
                                  unsigned int edx, unsigned int ecx,
                                  unsigned int eax, 
                                  unsigned int type,
                                  unsigned int ds,
                                  unsigned short int error_code,
                                  unsigned int eip,
                                  unsigned int cs, unsigned int eflags,
                                  unsigned int esp0, unsigned int ss0)
/*
 * FUNCTION: Handle a page fault
 */
{
   KPROCESSOR_MODE FaultMode;
   MEMORY_AREA* MemoryArea;
   
   /*
    * Get the address for the page fault
    */
   unsigned int cr2;
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));                
   DPRINT("Page fault at address %x with eip %x\n",cr2,eip);
   for(;;);

   cr2 = PAGE_ROUND_DOWN(cr2);
   
   assert_irql(DISPATCH_LEVEL);
   
   /*
    * Find the memory area for the faulting address
    */
   if (cr2>=KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (cs!=KERNEL_CS)
	  {
	     printk("%s:%d\n",__FILE__,__LINE__);
	     return(0);
	  }
	FaultMode = UserMode;
     }
   else
     {
	FaultMode = KernelMode;
     }
   
   MemoryArea = MmOpenMemoryAreaByAddress(cr2);
   if (MemoryArea==NULL)
     {
	printk("%s:%d\n",__FILE__,__LINE__);
	return(0);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	return(0);
	
      case MEMORY_AREA_SECTION_VIEW:
	return(MmSectionHandleFault(MemoryArea,cr2));
     }
   return(0);
}


BOOLEAN MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   UNIMPLEMENTED;
}

BOOLEAN MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   UNIMPLEMENTED;
}

