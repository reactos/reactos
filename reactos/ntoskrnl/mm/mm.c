/*
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mm.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <internal/stddef.h>
#include <internal/mm.h>
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

/* FUNCTIONS ****************************************************************/

BOOLEAN MmIsThisAnNtAsSystem()
{
   UNIMPLEMENTED
}

MM_SYSTEM_SIZE MmQuerySystemSize()
{
   UNIMPLEMENTED;
}

void MmInitialize(boot_param* bp)
/*
 * FUNCTION: Initalize memory managment
 */
{
   unsigned int kernel_len = bp->end_mem - bp->start_mem;
   unsigned int first_krnl_phys_addr;
   unsigned int last_krnl_phys_addr;
   int i;
   
   DPRINT("InitalizeMM()\n");

   CHECKPOINT;
   /*
    * Unmap low memory
    */
   (get_page_directory())[0]=0;
   FLUSH_TLB;
   CHECKPOINT;
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
   if (first_krnl_phys_addr < 0xa0000)
     {
	free_page(0x2000,(first_krnl_phys_addr/PAGESIZE)-2);
	free_page(last_krnl_phys_addr+PAGESIZE,
		  (0xa0000 - last_krnl_phys_addr - PAGESIZE)/PAGESIZE);
	free_page(1024*1024,EXTENDED_MEMORY_SIZE/4096);                                
     }
   else
     {
	free_page(0x2000,(0xa0000/PAGESIZE)-2);
	free_page(1024*1024,
		  (first_krnl_phys_addr-(1024*1024))/PAGESIZE);
	free_page(last_krnl_phys_addr+PAGESIZE,
		  ((EXTENDED_MEMORY_SIZE+(1024*1024))
		   -last_krnl_phys_addr)/PAGESIZE);
     }
   CHECKPOINT;
   
   /*
    * Create a trap for null pointer references and protect text
    * segment
    */
   CHECKPOINT;
   DPRINT("stext %x etext %x\n",(int)&stext,(int)&etext);
   for (i=PAGE_ROUND_UP(((int)&stext));
	i<PAGE_ROUND_DOWN(((int)&etext));i=i+PAGESIZE)
     {
	mark_page_not_writable(i);
     }
   DPRINT("end %x\n",(int)&end);
   for (i=PAGE_ROUND_UP(KERNEL_BASE+kernel_len);
	i<(KERNEL_BASE+PAGE_TABLE_SIZE);i=i+PAGESIZE)
     {
	set_page(i,0,0);
     }
   set_page(0,0,0);
   FLUSH_TLB;
   CHECKPOINT;
   /*
    * Intialize memory areas
    */
   VirtualInit(bp);
}
