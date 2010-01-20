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

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

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
//
// 0 | 1 is on/off paging, 2 is undocumented
//
UCHAR MmDisablePagingExecutive = 1; // Forced to off
PMMPTE MmSharedUserDataPte;
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
    NTSTATUS Status;
    BoundaryAddressMultiple.QuadPart = 0;
    
    //
    // Create the memory area to define the PTE base
    //
    BaseAddress = (PVOID)PTE_BASE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                4 * 1024 * 1024,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // Create the memory area to define Hyperspace
    //
    BaseAddress = (PVOID)HYPER_SPACE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                4 * 1024 * 1024,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // Protect the PFN database
    //
    BaseAddress = MmPfnDatabase;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                (MxPfnAllocation << PAGE_SHIFT),
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // ReactOS requires a memory area to keep the initial NP area off-bounds
    //
    BaseAddress = MmNonPagedPoolStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmSizeOfNonPagedPoolInBytes,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // And we need one more for the system NP
    //
    BaseAddress = MmNonPagedSystemStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                (ULONG_PTR)MmNonPagedPoolEnd -
                                (ULONG_PTR)MmNonPagedSystemStart,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // We also need one for system view space
    //
    BaseAddress = MiSystemViewStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmSystemViewSize,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // And another for session space
    //
    BaseAddress = MmSessionBase;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                (ULONG_PTR)MiSessionSpaceEnd -
                                (ULONG_PTR)MmSessionBase,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // One more for ARM paged pool
    //
    BaseAddress = MmPagedPoolStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmSizeOfPagedPoolInBytes,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(Status == STATUS_SUCCESS);
    
    //
    // And now, ReactOS paged pool
    //
    BaseAddress = MmPagedPoolBase;
    MmCreateMemoryArea(MmGetKernelAddressSpace(),
                       MEMORY_AREA_PAGED_POOL | MEMORY_AREA_STATIC,
                       &BaseAddress,
                       MmPagedPoolSize,
                       PAGE_READWRITE,
                       &MArea,
                       TRUE,
                       0,
                       BoundaryAddressMultiple);
    
    //
    // Next, the KPCR
    //
    BaseAddress = (PVOID)PCR;
    MmCreateMemoryArea(MmGetKernelAddressSpace(),
                       MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
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
                       MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
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
MiDbgDumpAddressSpace(VOID)
{
    //
    // Print the memory layout
    //
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmSystemRangeStart,
            (ULONG_PTR)MmSystemRangeStart + MmBootImageSize,
            "Boot Loaded Image");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmPagedPoolBase,
            (ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize,
            "Paged Pool");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmPfnDatabase,
            (ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT),
            "PFN Database");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmNonPagedPoolStart,
            (ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes,
            "ARM³ Non Paged Pool");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MiSystemViewStart,
            (ULONG_PTR)MiSystemViewStart + MmSystemViewSize,
            "System View Space");        
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmSessionBase,
            MiSessionSpaceEnd,
            "Session Space");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            PTE_BASE, PDE_BASE,
            "Page Tables");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            PDE_BASE, HYPER_SPACE,
            "Page Directories");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            HYPER_SPACE, HYPER_SPACE + (4 * 1024 * 1024),
            "Hyperspace");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmPagedPoolStart,
            (ULONG_PTR)MmPagedPoolStart + MmSizeOfPagedPoolInBytes,
            "ARM³ Paged Pool");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmNonPagedSystemStart, MmNonPagedPoolExpansionStart,
            "System PTE Space");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmNonPagedPoolExpansionStart, MmNonPagedPoolEnd,
            "Non Paged Pool Expansion PTE Space");
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
        
        /* Intialize system memory areas */
        MiInitSystemMemoryAreas();
        
        //
        // STEP 1: Allocate and free a single page, repeatedly
        // We should always get the same address back
        //
        if (1)
        {
            PULONG Test, OldTest;
            ULONG i;
        
            OldTest = Test = MiAllocatePoolPages(PagedPool, PAGE_SIZE);
            ASSERT(Test);
            for (i = 0; i < 16; i++)
            {
                MiFreePoolPages(Test);
                Test = MiAllocatePoolPages(PagedPool, PAGE_SIZE);
                ASSERT(OldTest == Test);
            }
            MiFreePoolPages(Test);
        }
        
        //
        // STEP 2: Allocate 2048 pages without freeing them
        // We should run out of space at 1024 pages, since we don't support
        // expansion yet.
        //
        if (1)
        {
            PULONG Test[2048];
            ULONG i;
            
            for (i = 0; i < 2048; i++)
            {
                Test[i] = MiAllocatePoolPages(PagedPool, PAGE_SIZE);
                if (!Test[i]) 
                {
                    ASSERT(i == 1024);
                    break;
                }
            }
            
            //
            // Cleanup
            //
            while (--i) if (Test[i]) MiFreePoolPages(Test[i]);
        }
        
        //
        // STEP 3: Allocate a page and touch it.
        // We should get an ARM3 page fault and it should handle the fault
        //
        if (1)
        {
            PULONG Test;
            
            Test = MiAllocatePoolPages(PagedPool, PAGE_SIZE);
            ASSERT(*Test == 0);
            MiFreePoolPages(Test);
        }
        
        /* Dump the address space */
        MiDbgDumpAddressSpace();
        
        /* Initialize paged pool */
        MmInitializePagedPool();
        
        /* Initialize working sets */
        MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);

        /* Initialize the user mode image list */
        InitializeListHead(&MmLoadedUserImageList);

        /* Initialize the Loader Lock */
        KeInitializeMutant(&MmSystemLoadLock, FALSE);

        /* Reload boot drivers */
        MiReloadBootLoadedDrivers(LoaderBlock);

        /* Initialize the loaded module list */
        MiInitializeLoadedModuleList(LoaderBlock);

        /* Setup shared user data settings that NT does as well */
        ASSERT(SharedUserData->NumberOfPhysicalPages == 0);
        SharedUserData->NumberOfPhysicalPages = MmNumberOfPhysicalPages;
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
        MI_MAKE_OWNER_PAGE(&TempPte);
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

