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

PBOOLEAN Mm64BitPhysicalAddress = FALSE;
ULONG MmReadClusterSize;
MM_STATS MmStats;
PMMPTE MmSharedUserDataPte;
PMMSUPPORT MmKernelAddressSpace;
extern KMUTANT MmSystemLoadLock;
extern ULONG MmBootImageSize;
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

    /* Put the paged pool after the loaded modules */
    MmPagedPoolBase = (PVOID)PAGE_ROUND_UP((ULONG_PTR)MmSystemRangeStart +
                                           MmBootImageSize);
    MmPagedPoolSize = MM_PAGED_POOL_SIZE;

    //
    // Initialize ARM³ in phase 2
    //
    MmArmInitSystem(2, KeLoaderBlock);
    
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
    extern MMPTE HyperTemplatePte;
    PMMPTE PointerPte;
    MMPTE TempPte = HyperTemplatePte;
    PFN_NUMBER PageFrameNumber;
    
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

        /* Setup shared user data settings that NT does as well */
        ASSERT(SharedUserData->NumberOfPhysicalPages == 0);
        SharedUserData->NumberOfPhysicalPages = MmStats.NrTotalPages;
        SharedUserData->LargePageMinimum = 0;
        
        /* For now, we assume that we're always Server */
        SharedUserData->NtProductType = NtProductServer;
    }
    else if (Phase == 1)
    {
        MmInitializeRmapList();
        MmInitializePageOp();
        MmInitSectionImplementation();
        MmInitPagingFile();
        
        //
        // Create a PTE to double-map the shared data section. We allocate it
        // from paged pool so that we can't fault when trying to touch the PTE
        // itself (to map it), since paged pool addresses will already be mapped
        // by the fault handler.
        //
        MmSharedUserDataPte = ExAllocatePoolWithTag(PagedPool,
                                                    sizeof(MMPTE),
                                                    '  mM');
        if (!MmSharedUserDataPte) return FALSE;
        
        //
        // Now get the PTE for shared data, and read the PFN that holds it
        //
        PointerPte = MiAddressToPte(KI_USER_SHARED_DATA);
        ASSERT(PointerPte->u.Hard.Valid == 1);
        PageFrameNumber = PFN_FROM_PTE(PointerPte);
        
        //
        // Now write a copy of it
        //
        TempPte.u.Hard.Owner = 1;
        TempPte.u.Hard.PageFrameNumber = PageFrameNumber;
        *MmSharedUserDataPte = TempPte;
        
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
    }
    else if (Phase == 2)
    {

    }

    return TRUE;
}

