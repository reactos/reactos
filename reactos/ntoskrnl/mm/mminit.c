/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel 
 * FILE:            ntoskrnl/mm/mminit.c
 * PURPOSE:         Kernel memory managment initialization functions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
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
//PVOID MiKernelMapStart;


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
{}

VOID INIT_FUNCTION
MmInitVirtualMemory(ULONG_PTR LastKernelAddress,
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
   PFN_TYPE Pfn;

   DPRINT("MmInitVirtualMemory(%x, %x)\n",LastKernelAddress, KernelLength);

   BoundaryAddressMultiple.QuadPart = 0;
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);

   MmInitMemoryAreas();

   MiNonPagedPoolStart = (char*)LastKernelAddress + PAGE_SIZE;
   MiNonPagedPoolLength = MM_NONPAGED_POOL_SIZE;

   MmPagedPoolBase = (char*)MiNonPagedPoolStart + MiNonPagedPoolLength + PAGE_SIZE;
   MmPagedPoolSize = MM_PAGED_POOL_SIZE;

   MiInitializeNonPagedPool();

   /*
    * Setup the system area descriptor list
    */
   MiInitPageDirectoryMap();

   BaseAddress = (PVOID)KPCR_BASE;
   MmCreateMemoryArea(NULL,
                      MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE * MAXIMUM_PROCESSORS,
                      0,
                      &kernel_kpcr_desc,
                      TRUE,
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
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG_PTR)&_text_end__)) - KERNEL_BASE;
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
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG_PTR)&_text_end__));
   ASSERT(BaseAddress == (PVOID)&_init_start__);
   Length = PAGE_ROUND_UP(((ULONG_PTR)&_init_end__)) -
            PAGE_ROUND_UP(((ULONG_PTR)&_text_end__));
   ParamLength = ParamLength - Length;

   MmCreateMemoryArea(NULL,
                      MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      0,
                      &kernel_init_desc,
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);

   Length = PAGE_ROUND_UP(((ULONG_PTR)&_bss_end__)) -
            PAGE_ROUND_UP(((ULONG_PTR)&_init_end__));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG_PTR)&_init_end__));
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
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG_PTR)&_bss_end__));
   Length = LastKernelAddress - (ULONG_PTR)BaseAddress;
   MmCreateMemoryArea(NULL,
                      MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      0,
                      &kernel_param_desc,
                      TRUE,
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
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)MM_KERNEL_MAP_BASE;
   Status = MmCreateMemoryArea(NULL,
                               MmGetKernelAddressSpace(),
                               MEMORY_AREA_SYSTEM,
                               &BaseAddress,
                               MM_KERNEL_MAP_SIZE,
                               0,
                               &MiKernelMapDescriptor,
                               TRUE,
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
                               TRUE,
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
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);
   Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Pfn);
   MmSharedDataPagePhysicalAddress.QuadPart = Pfn << PAGE_SHIFT;
   Status = MmCreateVirtualMapping(NULL,
                                   (PVOID)KI_USER_SHARED_DATA,
                                   PAGE_READWRITE,
                                   &Pfn,
                                   1);
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
MmInit1(ULONG_PTR FirstKrnlPhysAddr,
        ULONG_PTR LastKrnlPhysAddr,
        ULONG_PTR LastKernelAddress,
        PADDRESS_RANGE BIOSMemoryMap,
        ULONG AddressRangeCount,
        ULONG MaxMem)
/*
 * FUNCTION: Initalize memory managment
 */
{
   ULONG i;
   ULONG kernel_len;
   ULONG_PTR MappingAddress;
#ifndef CONFIG_SMP
   extern unsigned int unmap_me, unmap_me2, unmap_me3;
#endif
   extern unsigned int pagetable_start, pagetable_end;

   DPRINT("MmInit1(FirstKrnlPhysAddr, %p, LastKrnlPhysAddr %p, LastKernelAddress %p)\n",
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

   /* Set memory limits */
   MmUserProbeAddress = 0x7fff0000;
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

   /*
    * Initialize the kernel address space
    */
   MmInitializeKernelAddressSpace();
   
   MmInitGlobalKernelPageDirectory();
   
   DbgPrint("Used memory %dKb\n", (MmStats.NrTotalPages * PAGE_SIZE) / 1024);

   LastKernelAddress = (ULONG_PTR)MmInitializePageList(
                       FirstKrnlPhysAddr,
                       LastKrnlPhysAddr,
                       MmStats.NrTotalPages,
                       PAGE_ROUND_UP(LastKernelAddress),
                       BIOSMemoryMap,
                       AddressRangeCount);
   kernel_len = LastKrnlPhysAddr - FirstKrnlPhysAddr;

   /*
    * Unmap low memory
    */
#ifndef CONFIG_SMP
   /* In SMP mode we unmap the low memory in MmInit3.
      The APIC needs the mapping of the first pages
      while the processors are starting up. */
   MmDeletePageTable(NULL, 0);
#endif



   DPRINT("Invalidating between %x and %x\n",
          LastKernelAddress, KERNEL_BASE + 0x00600000);
   for (MappingAddress = LastKernelAddress;
        MappingAddress < KERNEL_BASE + 0x00600000;
        MappingAddress += PAGE_SIZE)
   {
      MmRawDeleteVirtualMapping((PVOID)MappingAddress);
   }

   for (MappingAddress = (ULONG_PTR)&pagetable_start;
        MappingAddress < (ULONG_PTR)&pagetable_end;
        MappingAddress += PAGE_SIZE)
   {
      MmDeleteVirtualMapping(NULL, (PVOID)MappingAddress, FALSE, NULL, NULL);
   }

   DPRINT("Almost done MmInit()\n");
#ifndef CONFIG_SMP
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
#ifdef CONFIG_SMP
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
                     PFN_TYPE Page, SWAPENTRY SwapEntry,
                     BOOLEAN Dirty)
{
   ASSERT(SwapEntry == 0);
   if (Page != 0)
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
   }
}

VOID
MiFreeInitMemory(VOID)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         (PVOID)&_init_start__,
                         MiFreeInitMemoryPage,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}
