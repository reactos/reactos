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
#include <debug.h>

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
    "LoaderReserve     ",
    "LoaderXIPRom      "
};

PVOID MiNonPagedPoolStart;
ULONG MiNonPagedPoolLength;
ULONG MmBootImageSize;
ULONG MmUserProbeAddress = 0;
PVOID MmHighestUserAddress = NULL;
PBOOLEAN Mm64BitPhysicalAddress = FALSE;
PVOID MmSystemRangeStart = NULL;
ULONG MmReadClusterSize;
MM_STATS MmStats;
PMMSUPPORT MmKernelAddressSpace;
extern KMUTANT MmSystemLoadLock;
BOOLEAN MiDbgEnableMdDump =
#ifdef _ARM_
TRUE;
#else
FALSE;
#endif

/* PRIVATE FUNCTIONS *********************************************************/

VOID
INIT_FUNCTION
NTAPI
MiInitSystemMemoryAreas()
{
    PVOID BaseAddress;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PMEMORY_AREA MArea;
    BoundaryAddressMultiple.QuadPart = 0;
    
    //
    // First initialize the page table and hyperspace memory areas
    //
    MiInitPageDirectoryMap();
    
    //
    // Next, the KPCR
    //
    BaseAddress = (PVOID)PCR;
    MmCreateMemoryArea(MmGetKernelAddressSpace(),
                       MEMORY_AREA_SYSTEM | MEMORY_AREA_STATIC,
                       &BaseAddress,
                       PAGE_SIZE * KeNumberProcessors,
                       PAGE_READWRITE,
                       &MArea,
                       TRUE,
                       0,
                       BoundaryAddressMultiple);

    //
    // Now the KUSER_SHARED_DATA
    //
    BaseAddress = (PVOID)KI_USER_SHARED_DATA;
    MmCreateMemoryArea(MmGetKernelAddressSpace(),
                       MEMORY_AREA_SYSTEM | MEMORY_AREA_STATIC,
                       &BaseAddress,
                       PAGE_SIZE,
                       PAGE_READWRITE,
                       &MArea,
                       TRUE,
                       0,
                       BoundaryAddressMultiple);
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

NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
INIT_FUNCTION
NTAPI
MmInit1(VOID)
{
    /* Initialize the kernel address space */
    KeInitializeGuardedMutex(&PsGetCurrentProcess()->AddressCreationLock);
    MmKernelAddressSpace = MmGetCurrentAddressSpace();
    MmInitGlobalKernelPageDirectory();
    
    /* Dump memory descriptors */
    if (MiDbgEnableMdDump) MiDbgDumpMemoryDescriptors();

    /* Get the size of FreeLDR's image allocations */
    MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
    MmBootImageSize *= PAGE_SIZE;

    /* Set memory limits */
    MmSystemRangeStart = (PVOID)KSEG0_BASE;
    MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
    MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);
    
    //
    // Initialize ARM³ in phase 0
    //
    MmArmInitSystem(0, KeLoaderBlock);    
    
    /* Intialize system memory areas */
    MiInitSystemMemoryAreas();
        
    /* Initialize the page list */
    MmInitializePageList();
    
    //
    // Initialize ARM³ in phase 1
    //
    MmArmInitSystem(1, KeLoaderBlock);
                                                                  // DEPRECATED
    /* Put nonpaged pool after the loaded modules */              // DEPRECATED
    MiNonPagedPoolStart = (PVOID)((ULONG_PTR)MmSystemRangeStart + // DEPRECATED
                                  MmBootImageSize);               // DEPRECATED
    MiNonPagedPoolLength = MM_NONPAGED_POOL_SIZE;                 // DEPRECATED
                                                                  // DEPRECATED
    /* Initialize nonpaged pool */                                // DEPRECATED
    MiInitializeNonPagedPool();                                   // DEPRECATED
                                                                  // DEPRECATED
    //
    // Initialize ARM³ in phase 2
    //
    MmArmInitSystem(2, KeLoaderBlock);
    
    /* Put the paged pool after nonpaged pool */
    MmPagedPoolBase = (PVOID)PAGE_ROUND_UP((ULONG_PTR)MiNonPagedPoolStart +
                                           MiNonPagedPoolLength);
    MmPagedPoolSize = MM_PAGED_POOL_SIZE;
    
    /* Initialize paged pool */
    MmInitializePagedPool();
    
    /* Initialize working sets */
    MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
}

BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (Phase == 0)
    {
        /* Initialize Mm bootstrap */
        MmInit1();

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

