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
ULONG MmNumberOfPhysicalPages, MmHighestPhysicalPage, MmLowestPhysicalPage;
ULONG_PTR MmPfnDatabaseEnd;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorOrg;
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
MiCountFreePagesInLoaderBlock(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG FreePages = 0;

    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
        NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
        NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

        /* Skip invisible memory */
        if ((Md->MemoryType != LoaderFirmwarePermanent) &&
            (Md->MemoryType != LoaderSpecialMemory) &&
            (Md->MemoryType != LoaderHALCachedMemory) &&
            (Md->MemoryType != LoaderBBTMemory))
        {
            /* Check if BURNMEM was used */
            if (Md->MemoryType != LoaderBad)
            {
                /* Count this in the total of pages */
                MmNumberOfPhysicalPages += Md->PageCount;
            }
            
            /* Check if this is the new lowest page */
            if (Md->BasePage < MmLowestPhysicalPage)
            {
                /* Update the lowest page */
                MmLowestPhysicalPage = Md->BasePage;
            }
            
            /* Check if this is the new highest page */
            if ((Md->BasePage + Md->PageCount) > MmHighestPhysicalPage)
            {
                /* Update the highest page */
                MmHighestPhysicalPage = Md->BasePage + Md->PageCount - 1;
            }

            /* Check if this is free memory */
            if ((Md->MemoryType == LoaderFree) ||
                (Md->MemoryType == LoaderLoadedProgram) ||
                (Md->MemoryType == LoaderFirmwareTemporary) ||
                (Md->MemoryType == LoaderOsloaderStack))
            {
                /* Check if this is the largest memory descriptor */
                if (Md->PageCount > FreePages)
                {
                    /* For now, it is */
                    FreePages = Md->PageCount;
                    MiFreeDescriptor = Md;
                }
            }
        }
    }

    /* Save original values of the free descriptor, since it'll be
       altered by early allocations */
    MiFreeDescriptorOrg = *MiFreeDescriptor;
}

VOID
NTAPI
MiDbgKernelLayout(VOID)
{
    DPRINT1("%8s%12s\t\t%s\n", "Start", "End", "Type");
    DPRINT1("0x%p - 0x%p\t%s\n",
            MmSystemRangeStart, (ULONG_PTR)MmSystemRangeStart + MmBootImageSize,
            "Boot Image Mapping Region");
    DPRINT1("0x%p - 0x%p\t%s\n",
            MmPfnDatabase, MmPfnDatabaseEnd,
            "PFN Database region");
    DPRINT1("0x%p - 0x%p\t%s\n",
             MiNonPagedPoolStart, (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength,
            "Non paged pool region");
    DPRINT1("0x%p - 0x%p\t%s\n",
            MmPagedPoolBase, (ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize,
            "Paged pool region");
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
    /* Dump memory descriptors */
    if (MiDbgEnableMdDump) MiDbgDumpMemoryDescriptors();

    /* Get the size of FreeLDR's image allocations */
    MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
    MmBootImageSize *= PAGE_SIZE;

    /* Set memory limits */
    MmSystemRangeStart = (PVOID)KSEG0_BASE;
    MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
    MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);
    DPRINT("MmSystemRangeStart:  %08x\n", MmSystemRangeStart);
    DPRINT("MmUserProbeAddress:  %08x\n", MmUserProbeAddress);
    DPRINT("MmHighestUserAddress:%08x\n", MmHighestUserAddress);
    
    /* Count RAM */
    MiCountFreePagesInLoaderBlock(KeLoaderBlock);
    DbgPrint("Used memory %dKb\n", (MmNumberOfPhysicalPages * PAGE_SIZE) / 1024);
    
    /* Initialize the kernel address space */
    KeInitializeGuardedMutex(&PsGetCurrentProcess()->AddressCreationLock);
    MmKernelAddressSpace = MmGetCurrentAddressSpace();
    MmInitGlobalKernelPageDirectory();

    /* We'll put the PFN array right after the loaded modules */
    MmPfnDatabase = (PVOID)((ULONG_PTR)MmSystemRangeStart + MmBootImageSize);
    MmPfnDatabaseEnd = (ULONG_PTR)MmPfnDatabase + (MmHighestPhysicalPage * sizeof(MMPFN));
    MmPfnDatabaseEnd = PAGE_ROUND_UP(MmPfnDatabaseEnd);
    
    /* Put nonpaged pool after the PFN database */
    MiNonPagedPoolStart = (PVOID)MmPfnDatabaseEnd;
    
    /* Length of non-paged pool */
    MiNonPagedPoolLength = MM_NONPAGED_POOL_SIZE;

    /* Put the paged pool after the non-paged pool */
    MmPagedPoolBase = (PVOID)PAGE_ROUND_UP((ULONG_PTR)MiNonPagedPoolStart +
                                           MiNonPagedPoolLength);
    MmPagedPoolSize = MM_PAGED_POOL_SIZE;
    
    /* Dump kernel memory layout */
    MiDbgKernelLayout();
    
    /* Intialize system memory areas */
    MiInitSystemMemoryAreas();

    /* Initialize the page list */
    MmInitializePageList();
    
    //
    // Initialize ARM³ in phase 0
    //
    MmArmInitSystem(0, KeLoaderBlock);

    /* Initialize nonpaged pool */
    MiInitializeNonPagedPool();
    
    /* Initialize paged pool */
    MmInitializePagedPool();
    
    /* Initialize working sets */
    MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
    
    //
    // Initialize ARM³ in phase 1
    //
    MmArmInitSystem(1, KeLoaderBlock);
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

