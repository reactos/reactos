/* $Id: mminit.c,v 1.62 2004/03/16 22:45:56 dwelch Exp $
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
#include <roscfg.h>
#include <internal/i386/segment.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/*
 * Compiler defined symbols
 */
extern unsigned int _text_start__;
extern unsigned int _text_end__;

extern unsigned int _init_start__;
extern unsigned int _init_end__;

extern unsigned int _bss_end__;


static BOOLEAN IsThisAnNtAsSystem = FALSE;
static MM_SYSTEM_SIZE MmSystemSize = MmSmallSystem;

static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_init_desc = NULL;
static MEMORY_AREA* kernel_map_desc = NULL;
static MEMORY_AREA* kernel_kpcr_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;
static MEMORY_AREA* kernel_shared_data_desc = NULL;
static MEMORY_AREA* kernel_mapped_vga_framebuffer_desc = NULL; 
static MEMORY_AREA* MiKernelMapDescriptor = NULL;
static MEMORY_AREA* MiPagedPoolDescriptor = NULL;

PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;

PVOID MiNonPagedPoolStart;
ULONG MiNonPagedPoolLength;
PVOID MiKernelMapStart;
ULONG MiKernelMapLength;

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOLEAN STDCALL MmIsThisAnNtAsSystem(VOID)
{
   return(IsThisAnNtAsSystem);
}

/*
 * @implemented
 */
MM_SYSTEM_SIZE STDCALL MmQuerySystemSize(VOID)
{
   return(MmSystemSize);
}

VOID MiShutdownMemoryManager(VOID)
{
}

VOID INIT_FUNCTION
MmInitVirtualMemory(ULONG LastKernelAddress,
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
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   //ULONG i;
   
   DPRINT("MmInitVirtualMemory(%x, %x)\n",LastKernelAddress, KernelLength);
   
   BoundaryAddressMultiple.QuadPart = 0;
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);

   MmInitMemoryAreas();

   /* Don't change the start of kernel map. Pte's must always exist for this region. */
   MiKernelMapStart = (char*)LastKernelAddress + PAGE_SIZE;
   MiKernelMapLength = MM_KERNEL_MAP_SIZE;

   MiNonPagedPoolStart = (char*)MiKernelMapStart + MiKernelMapLength + PAGE_SIZE;
   MiNonPagedPoolLength = MM_NONPAGED_POOL_SIZE;

   MmPagedPoolBase = (char*)MiNonPagedPoolStart + MiNonPagedPoolLength + PAGE_SIZE;
   MmPagedPoolSize = MM_PAGED_POOL_SIZE;


   MiInitKernelMap();
   MiInitializeNonPagedPool();

   /*
    * Setup the system area descriptor list
    */
   BaseAddress = (PVOID)0xf0000000;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      0x400000,
		      0,
		      &kernel_map_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   BaseAddress = (PVOID)KPCR_BASE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      PAGE_SIZE * MAXIMUM_PROCESSORS,
		      0,
		      &kernel_kpcr_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   BaseAddress = (PVOID)0xFF3A0000;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      0x20000,
		      0,
		      &kernel_mapped_vga_framebuffer_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG)&_text_end__)) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   
   /*
    * No need to lock the address space at this point since no
    * other threads are running.
    */
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_text_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_text_end__));
   assert (BaseAddress == (PVOID)&_init_start__);
   Length = PAGE_ROUND_UP(((ULONG)&_init_end__)) -
            PAGE_ROUND_UP(((ULONG)&_text_end__));
   ParamLength = ParamLength - Length;

   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_init_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   Length = PAGE_ROUND_UP(((ULONG)&_bss_end__)) - 
            PAGE_ROUND_UP(((ULONG)&_init_end__));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_init_end__));
   DPRINT("BaseAddress %x\n",BaseAddress);

   /*
    * No need to lock the address space at this point since we are
    * the only thread running.
    */
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_bss_end__));
   Length = LastKernelAddress - (ULONG)BaseAddress;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_param_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);
   
   BaseAddress = MiNonPagedPoolStart;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      MiNonPagedPoolLength,
		      0,
		      &kernel_pool_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   BaseAddress = MiKernelMapStart;
   Status = MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      MiKernelMapLength,
		      0,
		      &MiKernelMapDescriptor,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);
   
   BaseAddress = MmPagedPoolBase;
   Status = MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_PAGED_POOL,
		      &BaseAddress,
		      MmPagedPoolSize,
		      0,
		      &MiPagedPoolDescriptor,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);

   MmInitializePagedPool();

   /*
    * Create the kernel mapping of the user/kernel shared memory.
    */
   BaseAddress = (PVOID)KI_USER_SHARED_DATA;
   Length = PAGE_SIZE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_shared_data_desc,
		      FALSE,
		      FALSE,
		      BoundaryAddressMultiple);
   Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, 
					&MmSharedDataPagePhysicalAddress);
   Status = MmCreateVirtualMapping(NULL,
				   (PVOID)KI_USER_SHARED_DATA,
				   PAGE_READWRITE,
				   MmSharedDataPagePhysicalAddress,
				   TRUE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Unable to create virtual mapping\n");
	KEBUGCHECK(0);
     }
   RtlZeroMemory(BaseAddress, Length);

   /*
    *
    */
   MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
}

VOID INIT_FUNCTION
MmInit1(ULONG FirstKrnlPhysAddr,
	ULONG LastKrnlPhysAddr,
	ULONG LastKernelAddress,
	PADDRESS_RANGE BIOSMemoryMap,
	ULONG AddressRangeCount,
	ULONG MaxMem)
/*
 * FUNCTION: Initalize memory managment
 */
{
   ULONG i;
   ULONG kernel_len;
#ifndef MP
   extern unsigned int unmap_me, unmap_me2, unmap_me3;
#endif

   DPRINT("MmInit1(FirstKrnlPhysAddr, %x, LastKrnlPhysAddr %x, LastKernelAddress %x)\n",
		  FirstKrnlPhysAddr,
		  LastKrnlPhysAddr,
		  LastKernelAddress);


   if ((BIOSMemoryMap != NULL) && (AddressRangeCount > 0))
     {
	// If we have a bios memory map, recalulate the memory size
	ULONG last = 0;
	for (i = 0; i < AddressRangeCount; i++)
	  {
	    if (BIOSMemoryMap[i].Type == 1
	        && (BIOSMemoryMap[i].BaseAddrLow + BIOSMemoryMap[i].LengthLow + PAGE_SIZE -1) / PAGE_SIZE > last)
	      {
		 last = (BIOSMemoryMap[i].BaseAddrLow + BIOSMemoryMap[i].LengthLow + PAGE_SIZE -1) / PAGE_SIZE;
	      }
	  }
	if ((last - 256) * 4 > KeLoaderBlock.MemHigher)
	  {
	     KeLoaderBlock.MemHigher = (last - 256) * 4;
	  }
     }

   if (KeLoaderBlock.MemHigher >= (MaxMem - 1) * 1024)
     {
        KeLoaderBlock.MemHigher = (MaxMem - 1) * 1024;
     }

   /*
    * FIXME: Set this based on the system command line
    */
   MmSystemRangeStart = (PVOID)KERNEL_BASE; // 0xC0000000
   MmUserProbeAddress = (PVOID)0x7fff0000;
   MmHighestUserAddress = (PVOID)0x7ffeffff;
   
   MmInitGlobalKernelPageDirectory();

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
#ifndef MP
   /* In SMP mode we unmap the low memory in MmInit3. 
      The APIC needs the mapping of the first pages
      while the processors are starting up. */
   MmDeletePageTable(NULL, 0);
#endif
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
   MmStats.NrTotalPages = KeLoaderBlock.MemHigher/4;
   if (!MmStats.NrTotalPages)
     {
       DbgPrint("Memory not detected, default to 8 MB\n");
       MmStats.NrTotalPages = 2048;
     }
   else
     {
       /* add 1MB for standard memory (not extended) */
       MmStats.NrTotalPages += 256;
     }
#ifdef BIOS_MEM_FIX
   MmStats.NrTotalPages += 16;
#endif
   DbgPrint("Used memory %dKb\n", (MmStats.NrTotalPages * PAGE_SIZE) / 1024);

   LastKernelAddress = (ULONG)MmInitializePageList((PVOID)FirstKrnlPhysAddr,
					           (PVOID)LastKrnlPhysAddr,
					           MmStats.NrTotalPages,
					           PAGE_ROUND_UP(LastKernelAddress),
                                                   BIOSMemoryMap,
                                                   AddressRangeCount);
   kernel_len = LastKrnlPhysAddr - FirstKrnlPhysAddr;
   
   /*
    * Create a trap for null pointer references and protect text
    * segment
    */
   CHECKPOINT;
   DPRINT("_text_start__ %x _init_end__ %x\n",(int)&_text_start__,(int)&_init_end__);
   for (i=PAGE_ROUND_DOWN(((int)&_text_start__));
	i<PAGE_ROUND_UP(((int)&_init_end__));i=i+PAGE_SIZE)
     {
	MmSetPageProtect(NULL,
			 (PVOID)i,
			 PAGE_EXECUTE_READ);
     }

   DPRINT("Invalidating between %x and %x\n",
	  LastKernelAddress, 0xc0600000);
   for (i=(LastKernelAddress); i<0xc0600000; i+=PAGE_SIZE)
     {
	 MmRawDeleteVirtualMapping((PVOID)(i));
     }

   DPRINT("Invalidating between %x and %x\n",
	  0xd0100000, 0xd0400000);
   for (i=0xd0100000; i<0xd0400000; i+=PAGE_SIZE)
     {
	 MmRawDeleteVirtualMapping((PVOID)(i));
     }
   
   DPRINT("Almost done MmInit()\n");
#ifndef MP
   /* FIXME: This is broken in SMP mode */
   MmDeleteVirtualMapping(NULL, (PVOID)&unmap_me, TRUE, NULL, NULL);
   MmDeleteVirtualMapping(NULL, (PVOID)&unmap_me2, TRUE, NULL, NULL);
   MmDeleteVirtualMapping(NULL, (PVOID)&unmap_me3, TRUE, NULL, NULL);
#endif
   /*
    * Intialize memory areas
    */
   MmInitVirtualMemory(LastKernelAddress, kernel_len);

   MmInitializeMdlImplementation();
}

VOID INIT_FUNCTION
MmInit2(VOID)
{
   MmInitializeRmapList();
   MmInitializePageOp();
   MmInitSectionImplementation();
   MmInitPagingFile();
}

VOID INIT_FUNCTION
MmInit3(VOID)
{
   /*
    * Unmap low memory
    */
#ifdef MP
   /* In SMP mode we can unmap the low memory  
      if all processors are started. */
   MmDeletePageTable(NULL, 0);
#endif
   MmInitZeroPageThread();
   MmCreatePhysicalMemorySection();
   MiInitBalancerThread();

   /*
    * Initialise the modified page writer.
    */
   MmInitMpwThread();

   /* FIXME: Read parameters from memory */
}

VOID STATIC
MiFreeInitMemoryPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
		     PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry, 
		     BOOLEAN Dirty)
{
  assert(SwapEntry == 0);
  if (PhysAddr.QuadPart  != 0)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysAddr);
    }
}

VOID 
MiFreeInitMemory(VOID)
{
  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   (PVOID)&_init_start__,
		   PAGE_ROUND_UP((ULONG)&_init_end__) - (ULONG)_init_start__,
		   MiFreeInitMemoryPage,
		   NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
}
