/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/mminit.c
 * PURPOSE:         Memory Manager Initialization
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PCHAR
MemType[] =
{
    "ExceptionBlock    ",
    "SystemBlock       ",
    "Free              ",
    "Bad               ",
    "LoadedProgram     ",
    "FirmwareTemporary ",
    "FirmwarePermanent ",
    "OsloaderHeap      ",
    "OsloaderStack     ",
    "SystemCode        ",
    "HalCode           ",
    "BootDriver        ",
    "ConsoleInDriver   ",
    "ConsoleOutDriver  ",
    "StartupDpcStack   ",
    "StartupKernelStack",
    "StartupPanicStack ",
    "StartupPcrPage    ",
    "StartupPdrPage    ",
    "RegistryData      ",
    "MemoryData        ",
    "NlsData           ",
    "SpecialMemory     ",
    "BBTMemory         ",
    "LoaderReserve     "
};

BOOLEAN IsThisAnNtAsSystem = FALSE;
MM_SYSTEMSIZE MmSystemSize = MmSmallSystem;
PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
PVOID MiNonPagedPoolStart;
ULONG MiNonPagedPoolLength;
ULONG MmNumberOfPhysicalPages;
extern KMUTANT MmSystemLoadLock;
BOOLEAN MiDbgEnableMdDump =
#ifdef _ARM_
TRUE;
#else
FALSE;
#endif

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
MiShutdownMemoryManager(VOID)
{

}

VOID
INIT_FUNCTION
NTAPI
MmInitVirtualMemory(ULONG_PTR LastKernelAddress,
                    ULONG KernelLength)
{
   PVOID BaseAddress;
   ULONG Length;
   NTSTATUS Status;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PMEMORY_AREA MArea;

   DPRINT("MmInitVirtualMemory(%x, %x)\n",LastKernelAddress, KernelLength);

   BoundaryAddressMultiple.QuadPart = 0;
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);

   MmInitMemoryAreas();

   /*
    * FreeLDR Marks 6MB "in use" at the start of the kernel base,
    * so start the non-paged pool at a boundary of 6MB from where
    * the last driver was loaded. This should be the end of the
    * FreeLDR-marked region.
    */
   MiNonPagedPoolStart = (PVOID)ROUND_UP((ULONG_PTR)LastKernelAddress + PAGE_SIZE, 0x600000);
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

   BaseAddress = (PVOID)PCR;
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
   MmSharedDataPagePhysicalAddress.QuadPart = 2 << PAGE_SHIFT;
   RtlZeroMemory(BaseAddress, Length);

   /*
    *
    */
   MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
}

ULONG
NTAPI
MiCountFreePagesInLoaderBlock(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG TotalPages = 0;

    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
        NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
        NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

        if (Md->MemoryType == LoaderBad ||
            Md->MemoryType == LoaderFirmwarePermanent ||
            Md->MemoryType == LoaderSpecialMemory ||
            Md->MemoryType == LoaderBBTMemory)
        {
            /* Don't count these blocks */
            continue;
        }

        TotalPages += Md->PageCount;
    }

    return TotalPages;
}

VOID
NTAPI
MiDbgDumpMemoryDescriptors(VOID)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG TotalPages = 0;
    
    DPRINT1("Base\t\tLength\t\tType\n");
    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
        DPRINT1("%08lX\t%08lX\t%s\n", Md->BasePage, Md->PageCount, MemType[Md->MemoryType]);
        TotalPages += Md->PageCount;
    }

    DPRINT1("Total: %08lX (%d MB)\n", TotalPages, (TotalPages * PAGE_SIZE) / 1024 / 1024);
}

ULONG_PTR
NTAPI
MiGetLastKernelAddress(VOID)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG_PTR LastKrnlPhysAddr = 0;
        
    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
        if (Md->MemoryType == LoaderBootDriver ||
            Md->MemoryType == LoaderSystemCode ||
            Md->MemoryType == LoaderHalCode)
        {
            if (Md->BasePage+Md->PageCount > LastKrnlPhysAddr)
                LastKrnlPhysAddr = Md->BasePage+Md->PageCount;
            
        }
    }

    /* Convert to a physical address */
    return LastKrnlPhysAddr << PAGE_SHIFT;
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
{
    ULONG kernel_len;
    ULONG_PTR MappingAddress;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    /* Dump memory descriptors */
    if (MiDbgEnableMdDump) MiDbgDumpMemoryDescriptors();

    /* Set the page directory */
    PsGetCurrentProcess()->Pcb.DirectoryTableBase.LowPart = (ULONG)MmGetPageDirectory();

    /* NTLDR Hacks */
    if (!MmFreeLdrPageDirectoryEnd) MmFreeLdrPageDirectoryEnd = 0x40000;

    /* Get the first physical address */
    LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks);
    FirstKrnlPhysAddr = (ULONG_PTR)LdrEntry->DllBase - KSEG0_BASE;

    /* Get the last kernel address */ 
    LastKrnlPhysAddr = MiGetLastKernelAddress();
    LastKernelAddress = LastKrnlPhysAddr | KSEG0_BASE;

    /* Set memory limits */
    MmSystemRangeStart = (PVOID)KSEG0_BASE;
    MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
    MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);
    DPRINT("MmSystemRangeStart:  %08x\n", MmSystemRangeStart);
    DPRINT("MmUserProbeAddress:  %08x\n", MmUserProbeAddress);
    DPRINT("MmHighestUserAddress:%08x\n", MmHighestUserAddress);

    /* Initialize memory managment statistics */
    RtlZeroMemory(&MmStats, sizeof(MmStats));
    
    /* Count RAM */
    MmStats.NrTotalPages = MiCountFreePagesInLoaderBlock(KeLoaderBlock);
    MmNumberOfPhysicalPages = MmStats.NrTotalPages;
    if (!MmStats.NrTotalPages)
    {
        DbgPrint("Memory not detected, default to 8 MB\n");
        MmStats.NrTotalPages = 2048;
    }
    else
    {
        /* HACK: add 1MB for standard memory (not extended). Why? */
        DbgPrint("Used memory %dKb\n", (MmStats.NrTotalPages * PAGE_SIZE) / 1024);
        MmStats.NrTotalPages += 256;
    }
    
    /* Initialize the kernel address space */
    MmInitializeKernelAddressSpace();
    MmInitGlobalKernelPageDirectory();

    /* Initialize the page list */
    LastKernelAddress = (ULONG_PTR)MmInitializePageList(FirstKrnlPhysAddr,
                                                        LastKrnlPhysAddr,
                                                        MmStats.NrTotalPages,
                                                        PAGE_ROUND_UP(LastKernelAddress),
                                                        BIOSMemoryMap,
                                                        AddressRangeCount);
    kernel_len = LastKrnlPhysAddr - FirstKrnlPhysAddr;
    
    /* Unmap low memory */
    MmDeletePageTable(NULL, 0);

    /* Unmap FreeLDR's 6MB allocation */
    DPRINT("Invalidating between %p and %p\n", LastKernelAddress, KSEG0_BASE + 0x00600000);
    for (MappingAddress = LastKernelAddress;
         MappingAddress < KSEG0_BASE + 0x00600000;
         MappingAddress += PAGE_SIZE)
    {
        MmRawDeleteVirtualMapping((PVOID)MappingAddress);
    }

    /* Intialize memory areas */
    MmInitVirtualMemory(LastKernelAddress, kernel_len);

    /* Initialize MDLs */
    MmInitializeMdlImplementation();
}

BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Flags = 0;
    if (Phase == 0)
    {
        /* Initialize the Loader Lock */
        KeInitializeMutant(&MmSystemLoadLock, FALSE);

        /* Initialize the address space for the system process */
        MmInitializeProcessAddressSpace(PsGetCurrentProcess(),
                                        NULL,
                                        NULL,
                                        &Flags,
                                        NULL);

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
        MmInitializePageOp();
        MmInitSectionImplementation();
        MmInitPagingFile();
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


/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(VOID)
{
   return IsThisAnNtAsSystem;
}

/*
 * @implemented
 */
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(VOID)
{
   return MmSystemSize;
}
