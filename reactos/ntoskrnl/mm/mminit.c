/* $Id: mminit.c,v 1.9 2000/10/22 13:28:20 ekohl Exp $
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
#include <napi/shared_data.h>

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
extern unsigned int _text_start__;
extern unsigned int _text_end__;

static BOOLEAN IsThisAnNtAsSystem = FALSE;
static MM_SYSTEM_SIZE MmSystemSize = MmSmallSystem;

extern unsigned int _bss_end__;

static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;
static MEMORY_AREA* kernel_shared_data_desc = NULL;

PVOID MmSharedDataPagePhysicalAddress = NULL;

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

VOID MmInitVirtualMemory(ULONG LastKernelAddress,
			 ULONG KernelLength)
/*
 * FUNCTION: Intialize the memory areas list
 * ARGUMENTS:
 *           bp = Pointer to the boot parameters
 *           kernel_len = Length of the kernel
 */
{
   PVOID BaseAddress;
   ULONG Length;
   ULONG ParamLength = KernelLength;
   NTSTATUS Status;
   
   DPRINT("MmInitVirtualMemory(%x)\n",bp);
   
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   
   MmInitMemoryAreas();
//   ExInitNonPagedPool(KERNEL_BASE + PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   ExInitNonPagedPool(LastKernelAddress + PAGESIZE);
   
   
   /*
    * Setup the system area descriptor list
    */
   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG)&_text_end__)) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_text_desc);
   
   Length = PAGE_ROUND_UP(((ULONG)&_bss_end__)) - 
            PAGE_ROUND_UP(((ULONG)&_text_end__));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_text_end__));
   DPRINT("BaseAddress %x\n",BaseAddress);
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),		      
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc);
   
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_bss_end__));
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
   
   BaseAddress = (PVOID)KERNEL_SHARED_DATA_BASE;
   Length = PAGESIZE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_shared_data_desc);
   MmSharedDataPagePhysicalAddress = MmAllocPage(0);
   Status = MmCreateVirtualMapping(NULL,
				   (PVOID)KERNEL_SHARED_DATA_BASE,
				   PAGE_READWRITE,
				   (ULONG)MmSharedDataPagePhysicalAddress);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Unable to create virtual mapping\n");
	KeBugCheck(0);
     }
   ((PKUSER_SHARED_DATA)KERNEL_SHARED_DATA_BASE)->TickCountLow = 0xdeadbeef;
   
//   MmDumpMemoryAreas();
   DPRINT("MmInitVirtualMemory() done\n");
}

VOID MmInit1(ULONG FirstKrnlPhysAddr, 
	     ULONG LastKrnlPhysAddr,
	     ULONG LastKernelAddress)
/*
 * FUNCTION: Initalize memory managment
 */
{
   ULONG i;
   ULONG kernel_len;
   
   DPRINT("MmInit1(bp %x, LastKernelAddress %x)\n", bp, 
	  LastKernelAddress);
   
   /*
    * FIXME: Set this based on the system command line
    */
   MmUserProbeAddress = (PVOID)0x7fff0000;
   MmHighestUserAddress = (PVOID)0x7ffeffff;
   
   /*
    * Initialize memory managment statistics
    */
   MmStats.NrTotalPages = 0;
   MmStats.NrSystemPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrReservedPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrFreePages = 0;
   MmStats.NrLockedPages = 0;
   MmStats.PagingRequestsInLastMinute = 0;
   MmStats.PagingRequestsInLastFiveMinutes = 0;
   MmStats.PagingRequestsInLastFifteenMinutes = 0;
   
   /*
    * Initialize the kernel address space
    */
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
   DPRINT("first krnl %x\nlast krnl %x\n",FirstKrnlPhysAddr,
	  LastKrnlPhysAddr);
   
   /*
    * Free physical memory not used by the kernel
    */
   LastKernelAddress = (ULONG)MmInitializePageList(
					   (PVOID)FirstKrnlPhysAddr,
					   (PVOID)LastKrnlPhysAddr,
					   1024,
					   PAGE_ROUND_UP(LastKernelAddress));
   kernel_len = LastKrnlPhysAddr - FirstKrnlPhysAddr;
   
   /*
    * Create a trap for null pointer references and protect text
    * segment
    */
   CHECKPOINT;
   DPRINT("stext %x etext %x\n",(int)&stext,(int)&etext);
   for (i=PAGE_ROUND_UP(((int)&_text_start__));
	i<PAGE_ROUND_DOWN(((int)&_text_end__));i=i+PAGESIZE)
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
	MmDeleteVirtualMapping(NULL, (PVOID)(i), FALSE);
     }
   DPRINT("Almost done MmInit()\n");
   
   /*
    * Intialize memory areas
    */
   MmInitVirtualMemory(LastKernelAddress, kernel_len);
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

