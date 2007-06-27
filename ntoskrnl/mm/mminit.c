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
#if 0
extern unsigned int _image_base__;
extern unsigned int _text_start__;
extern unsigned int _text_end__;

extern unsigned int _init_start__;
extern unsigned int _init_end__;

extern unsigned int _bss_end__;
#endif

static BOOLEAN IsThisAnNtAsSystem = FALSE;
MM_SYSTEMSIZE MmSystemSize = MmSmallSystem;

PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;

PVOID MiNonPagedPoolStart;
ULONG MiNonPagedPoolLength;

VOID INIT_FUNCTION NTAPI MmInitVirtualMemory(ULONG_PTR LastKernelAddress, ULONG KernelLength);

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
MM_SYSTEMSIZE STDCALL MmQuerySystemSize(VOID)
{
   return(MmSystemSize);
}

VOID
NTAPI
MiShutdownMemoryManager(VOID)
{}

VOID
INIT_FUNCTION
NTAPI
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
   //ULONG ParamLength = KernelLength;
   NTSTATUS Status;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PFN_TYPE Pfn;
   PMEMORY_AREA MArea;

   DPRINT("MmInitVirtualMemory(%x, %x)\n",LastKernelAddress, KernelLength);

   BoundaryAddressMultiple.QuadPart = 0;
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);

   MmInitMemoryAreas();

   /* Start the paged and nonpaged pool at a 4MB boundary. */ 
   MiNonPagedPoolStart = (PVOID)ROUND_UP((ULONG_PTR)LastKernelAddress + PAGE_SIZE, 0x400000);
   MiNonPagedPoolLength = MM_NONPAGED_POOL_SIZE;

   MmPagedPoolBase = (PVOID)ROUND_UP((ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength + PAGE_SIZE, 0x400000);
   MmPagedPoolSize = MM_PAGED_POOL_SIZE;

   DPRINT("NonPagedPool %x - %x, PagedPool %x - %x\n", MiNonPagedPoolStart, (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength - 1, 
           MmPagedPoolBase, (ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize - 1);

   MiInitializeNonPagedPool();

   /*
    * Setup the system area descriptor list
    */
   MiInitPageDirectoryMap();

   BaseAddress = (PVOID)KPCR_BASE;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE * MAXIMUM_PROCESSORS,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   /* Local APIC base */
   BaseAddress = (PVOID)0xFEE00000;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   /* i/o APIC base */
   BaseAddress = (PVOID)0xFEC00000;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)0xFF3A0000;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      0x20000,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

#if 0
   DPRINT1("LD Vars: %lx %lx %lx %lx %lx %lx. Last: %lx\n",
           &_image_base__,
           &_text_start__,
           &_text_end__,
           &_init_start__,
           &_init_end__,
           &_bss_end__,
           LastKernelAddress);
   BaseAddress = (PVOID)&_image_base__;
   DPRINT1("Non-LD Vars: %lx %lx %lx %lx %lx %lx. Last: %lx\n",
           0,
           0,
           0,
           0,
           0,
           0,
           LastKernelAddress);
   Length = PAGE_ROUND_UP(((ULONG_PTR)&_text_end__)) - (ULONG_PTR)&_image_base__;
   ParamLength = ParamLength - Length;

   /*
    * No need to lock the address space at this point since no
    * other threads are running.
    */
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      PAGE_EXECUTE_READ,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG_PTR)&_text_end__));
   ASSERT(BaseAddress == (PVOID)&_init_start__);
   Length = PAGE_ROUND_UP(((ULONG_PTR)&_init_end__)) -
            PAGE_ROUND_UP(((ULONG_PTR)&_text_end__));
   ParamLength = ParamLength - Length;

   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      PAGE_EXECUTE_READ,
                      &MArea,
                      TRUE,
                      0,
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
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG_PTR)&_bss_end__));
   Length = LastKernelAddress - (ULONG_PTR)BaseAddress;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);
#endif

   BaseAddress = MiNonPagedPoolStart;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      MiNonPagedPoolLength,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = MmPagedPoolBase;
   Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                               MEMORY_AREA_PAGED_POOL,
                               &BaseAddress,
                               MmPagedPoolSize,
                               PAGE_READWRITE,
                               &MArea,
                               TRUE,
                               0,
                               BoundaryAddressMultiple);

   MmInitializePagedPool();

   /*
    * Create the kernel mapping of the user/kernel shared memory.
    */
   BaseAddress = (PVOID)KI_USER_SHARED_DATA;
   Length = PAGE_SIZE;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
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

VOID
INIT_FUNCTION
NTAPI
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
   PLDR_DATA_TABLE_ENTRY LdrEntry;

   DPRINT("MmInit1(FirstKrnlPhysAddr, %p, LastKrnlPhysAddr %p, LastKernelAddress %p)\n",
          FirstKrnlPhysAddr,
          LastKrnlPhysAddr,
          LastKernelAddress);

   /* Set the page directory */
   PsGetCurrentProcess()->Pcb.DirectoryTableBase.LowPart = (ULONG)MmGetPageDirectory();

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
      if ((last - 256) * 4 > MmFreeLdrMemHigher)
      {
         MmFreeLdrMemHigher = (last - 256) * 4;
      }
   }

   /* NTLDR Hacks */
   if (!MmFreeLdrMemHigher) MmFreeLdrMemHigher = 65536;
   if (!MmFreeLdrPageDirectoryEnd) MmFreeLdrPageDirectoryEnd = 0x40000;
   if (!FirstKrnlPhysAddr)
   {
       /* Get the kernel entry */
       LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink,
                                    LDR_DATA_TABLE_ENTRY,
                                    InLoadOrderLinks);

       /* Get the addresses */
       FirstKrnlPhysAddr = (ULONG_PTR)LdrEntry->DllBase - KSEG0_BASE;

       /* FIXME: How do we get the last address? */
   }

   if (MmFreeLdrMemHigher >= (MaxMem - 1) * 1024)
   {
      MmFreeLdrMemHigher = (MaxMem - 1) * 1024;
   }

   /* Set memory limits */
   MmSystemRangeStart = (PVOID)KSEG0_BASE;
   MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
   MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);

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
   MmStats.NrTotalPages = MmFreeLdrMemHigher/4;
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
#ifdef CONFIG_SMP
   /* In SMP mode we unmap the low memory pagetable in MmInit3.
      The APIC needs the mapping of the first pages
      while the processors are starting up.
      We unmap all pages except page 2 and 3. */
   for (MappingAddress = 0;
        MappingAddress < 1024 * PAGE_SIZE;
        MappingAddress += PAGE_SIZE)
   {
      if (MappingAddress != 2 * PAGE_SIZE &&
          MappingAddress != 3 * PAGE_SIZE)
      {
         MmRawDeleteVirtualMapping((PVOID)MappingAddress);
      }
   }
#else
   MmDeletePageTable(NULL, 0);
#endif

   DPRINT("Invalidating between %x and %x\n",
          LastKernelAddress, KSEG0_BASE + 0x00600000);
   for (MappingAddress = LastKernelAddress;
        MappingAddress < KSEG0_BASE + 0x00600000;
        MappingAddress += PAGE_SIZE)
   {
      MmRawDeleteVirtualMapping((PVOID)MappingAddress);
   }

   DPRINT("Almost done MmInit()\n");
   /*
    * Intialize memory areas
    */
   MmInitVirtualMemory(LastKernelAddress, kernel_len);

   MmInitializeMdlImplementation();
}

BOOLEAN RmapReady, PageOpReady, SectionsReady, PagingReady;
extern KMUTANT MmSystemLoadLock;

BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (Phase == 0)
    {
        /* Initialize the Loader Lock */
        KeInitializeMutant(&MmSystemLoadLock, FALSE);

        /* Reload boot drivers */
        MiReloadBootLoadedDrivers(LoaderBlock);

        /* Initialize the loaded module list */
        MiInitializeLoadedModuleList(LoaderBlock);

        /* We're done, for now */
        DPRINT("Mm0: COMPLETE\n");
    }
    else if (Phase == 1)
    {
        MmInitializeRmapList();
        RmapReady = TRUE;
        MmInitializePageOp();
        PageOpReady = TRUE;
        MmInitSectionImplementation();
        SectionsReady = TRUE;
        MmInitPagingFile();
        PagingReady = TRUE;
        MmCreatePhysicalMemorySection();

        /* Setup shared user data settings that NT does as well */
        ASSERT(SharedUserData->NumberOfPhysicalPages == 0);
        SharedUserData->NumberOfPhysicalPages = MmStats.NrTotalPages;
        SharedUserData->LargePageMinimum = 0;

        /* For now, we assume that we're always Workstation */
        SharedUserData->NtProductType = NtProductWinNt;
    }
    else if (Phase == 2)
    {
        /*
        * Unmap low memory
        */
        MiInitBalancerThread();

        /*
        * Initialise the modified page writer.
        */
        MmInitMpwThread();

        /* Initialize the balance set manager */
        MmInitBsmThread();

        /* FIXME: Read parameters from memory */
    }

    return TRUE;
}

#if 0

VOID static
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
NTAPI
MiFreeInitMemory(VOID)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         (PVOID)&_init_start__,
                         MiFreeInitMemoryPage,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}
#endif
