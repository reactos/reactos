/* $Id: mminit.c,v 1.1 2000/07/04 08:52:42 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mminit.c
 * PURPOSE:     kernel memory managment initialization functions
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
#include <internal/ps.h>
#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/*
 * Size of extended memory (kb) (fixed for now)
 */
#define EXTENDED_MEMORY_SIZE  (3*1024*1024)

/*
 * Compiler defined symbol s
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

BOOLEAN STDCALL MmIsThisAnNtAsSystem(VOID)
{
   return(IsThisAnNtAsSystem);
}

MM_SYSTEM_SIZE STDCALL MmQuerySystemSize(VOID)
{
   return(MmSystemSize);
}

VOID MiShutdownMemoryManager(VOID)
{
}

VOID MmInitVirtualMemory(boot_param* bp, ULONG LastKernelAddress)
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
   
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   
   MmInitMemoryAreas();
//   ExInitNonPagedPool(KERNEL_BASE + PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   ExInitNonPagedPool(LastKernelAddress + PAGESIZE);
   
   
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
//   Length = ParamLength;
   Length = LastKernelAddress - (ULONG)BaseAddress;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),		      
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_param_desc);

   BaseAddress = (PVOID)(LastKernelAddress + PAGESIZE);
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

VOID MmInit1(PLOADER_PARAMETER_BLOCK bp, ULONG LastKernelAddress)
/*
 * FUNCTION: Initalize memory managment
 */
{
   ULONG first_krnl_phys_addr;
   ULONG last_krnl_phys_addr;
   ULONG i;
   ULONG kernel_len;
   
   DPRINT("MmInit1(bp %x, LastKernelAddress %x)\n", bp, 
	  LastKernelAddress);

   MmInitializeKernelAddressSpace();
   
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
   MmInitVirtualMemory(bp, LastKernelAddress);
}

VOID MmInit2(VOID)
{
   MmInitSectionImplementation();
   MmInitPagingFile();
}

VOID MmInit3(VOID)
{
   MmInitPagerThread();
   /* FIXME: Read parameters from memory */
}

