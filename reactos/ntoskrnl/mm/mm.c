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

#include <internal/stddef.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <internal/string.h>

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

/* FUNCTIONS ****************************************************************/

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
   VirtualInit(bp);
}
