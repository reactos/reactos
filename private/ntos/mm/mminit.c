/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mminit.c

Abstract:

    This module contains the initialization for the memory management
    system.

Author:

    Lou Perazzoli (loup) 20-Mar-1989
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

MMPTE MmSharedUserDataPte;

extern MMINPAGE_SUPPORT_LIST MmInPageSupportList;
extern MMEVENT_COUNT_LIST MmEventCountList;
extern KMUTANT MmSystemLoadLock;
extern ULONG_PTR MmSystemPtesStart[MaximumPtePoolTypes];
extern ULONG MiSpecialPoolPtes;
extern ULONG MmPagedPoolCommit;

extern PVOID BBTBuffer;
extern ULONG BBTPagesToReserve;

ULONG_PTR MmSubsectionBase;
ULONG_PTR MmSubsectionTopPage;
ULONG MmDataClusterSize;
ULONG MmCodeClusterSize;
PFN_NUMBER MmResidentAvailableAtInit;
KEVENT MmImageMappingPteEvent;
PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;
LIST_ENTRY MmLockConflictList;
LOGICAL MmPagedPoolMaximumDesired = FALSE;

PERFINFO_MMINIT_DECL;

VOID
MiMapBBTMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiInitializeSpecialPool(
    VOID
    );

VOID
MiEnablePagingTheExecutive(
    VOID
    );

VOID
MiEnablePagingOfDriverAtInit (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte
    );

VOID
MiBuildPagedPool (
    );

VOID
MiMergeMemoryLimit (
    IN OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory,
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER NumberOfPages
    );

VOID
MiWriteProtectSystemImage (
    IN PVOID DllBase
    );

#ifndef NO_POOL_CHECKS
VOID
MiInitializeSpecialPoolCriteria (
    IN VOID
    );
#endif

#if defined (_X86PAE_)
VOID
MiCheckPaeLicense (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludeType,
    OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MmInitSystem)
#pragma alloc_text(INIT,MiMapBBTMemory)
#pragma alloc_text(INIT,MmInitializeMemoryLimits)
#pragma alloc_text(INIT,MiMergeMemoryLimit)
#pragma alloc_text(INIT,MmFreeLoaderBlock)
#pragma alloc_text(INIT,MiBuildPagedPool)
#pragma alloc_text(INIT,MiFindInitializationCode)
#pragma alloc_text(INIT,MiEnablePagingTheExecutive)
#pragma alloc_text(INIT,MiEnablePagingOfDriverAtInit)
#if defined (_X86PAE_)
#pragma alloc_text(INIT,MiCheckPaeLicense)
#endif
#pragma alloc_text(PAGELK,MiFreeInitializationCode)
#endif

#define MM_MAX_LOADER_BLOCKS 30

//
// Default is a 300 second life span for modified mapped pages -
// This can be overridden in the registry.
//

ULONG MmModifiedPageLifeInSeconds = 300;

LARGE_INTEGER MiModifiedPageLife;

BOOLEAN MiTimerPending = FALSE;

KEVENT MiMappedPagesTooOldEvent;

KDPC MiModifiedPageWriterTimerDpc;

KTIMER MiModifiedPageWriterTimer;

//
// The following constants are based on the number PAGES not the
// memory size.  For convenience the number of pages is calculated
// based on a 4k page size.  Hence 12mb with 4k page is 3072.
//

#define MM_SMALL_SYSTEM ((13*1024*1024) / 4096)

#define MM_MEDIUM_SYSTEM ((19*1024*1024) / 4096)

#define MM_MIN_INITIAL_PAGED_POOL ((32*1024*1024) >> PAGE_SHIFT)

#define MM_DEFAULT_IO_LOCK_LIMIT (2 * 1024 * 1024)

extern ULONG MmMaximumWorkingSetSize;

extern ULONG MmEnforceWriteProtection;

extern CHAR MiPteStr[];

extern LONG MiTrimInProgressCount;

#if !defined (_WIN64)

#if !defined (_X86PAE_)
PFN_NUMBER MmSystemPageDirectory;
#else
PFN_NUMBER MmSystemPageDirectory[PD_PER_SYSTEM];
#endif

PMMPTE MmSystemPagePtes;
#endif

ULONG MmTotalSystemCodePages;

MM_SYSTEMSIZE MmSystemSize;

ULONG MmLargeSystemCache;

ULONG MmProductType;

LIST_ENTRY MmLoadedUserImageList;
PPAGE_FAULT_NOTIFY_ROUTINE MmPageFaultNotifyRoutine;
PHARD_FAULT_NOTIFY_ROUTINE MmHardFaultNotifyRoutine;


BOOLEAN
MmInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PPHYSICAL_MEMORY_DESCRIPTOR PhysicalMemoryBlock
    )

/*++

Routine Description:

    This function is called during Phase 0, phase 1 and at the end
    of phase 1 ("phase 2") initialization.

    Phase 0 initializes the memory management paging functions,
    nonpaged and paged pool, the PFN database, etc.

    Phase 1 initializes the section objects, the physical memory
    object, and starts the memory management system threads.

    Phase 2 frees memory used by the OsLoader.

Arguments:

    Phase - System initialization phase.

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    Returns TRUE if the initialization was successful.

Environment:

    Kernel Mode Only.  System initialization.

--*/

{
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    PMMPTE StartingPte;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    MMPTE Pointer;
    PFN_NUMBER i, j;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER DirectoryFrameIndex;
    MMPTE TempPte;
    KIRQL OldIrql;
    LOGICAL First;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG MaximumSystemCacheSize;
    ULONG MaximumSystemCacheSizeTotal;
    PEPROCESS Process;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG_PTR SystemPteMultiplier;

    BOOLEAN IncludeType[LoaderMaximum];
    ULONG MemoryAlloc[(sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
            sizeof(PHYSICAL_MEMORY_RUN)*MAX_PHYSICAL_MEMORY_FRAGMENTS) /
              sizeof(ULONG)];

    PPHYSICAL_MEMORY_DESCRIPTOR Memory;

    //
    // Make sure structure alignment is okay.
    //

    if (Phase == 0) {
        MmThrottleTop = 450;
        MmThrottleBottom = 127;

        //
        // Set the highest user address, the system range start address, the
        // user probe address, and the virtual bias.
        //

#if defined(_AXP64_) || defined(_IA64_)

        MmHighestUserAddress = MM_HIGHEST_USER_ADDRESS;
        MmUserProbeAddress = MM_USER_PROBE_ADDRESS;
        MmSystemRangeStart = MM_SYSTEM_RANGE_START;

#else

        MmHighestUserAddress = (PVOID)(KSEG0_BASE - 0x10000 - 1);
        MmUserProbeAddress = KSEG0_BASE - 0x10000;
        MmSystemRangeStart = (PVOID)KSEG0_BASE;

#endif

        MiHighestUserPte = MiGetPteAddress (MmHighestUserAddress);
        MiHighestUserPde = MiGetPdeAddress (MmHighestUserAddress);

        MmVirtualBias = 0;

        //
        // Set the highest section base address.
        //
        // N.B. In 32-bit systems this address must be 2gb or less even for
        //      systems that run with 3gb enabled. Otherwise, it would not
        //      be possible to map based sections identically in all processes.
        //

        MmHighSectionBase = ((PCHAR)MmHighestUserAddress - 0x800000);

        if (ExVerifySuite(TerminalServer) == TRUE) {
            MiHydra = TRUE;
            MiSystemViewStart = MM_SYSTEM_VIEW_START_IF_HYDRA;
            MmSessionBase = (ULONG_PTR)MM_SESSION_SPACE_DEFAULT;

        } else {
            MiSystemViewStart = MM_SYSTEM_VIEW_START;
            MiHydra = FALSE;
        }

        MaximumSystemCacheSize = (MM_SYSTEM_CACHE_END - MM_SYSTEM_CACHE_START) >> PAGE_SHIFT;

        //
        // If the system has been biased to an alternate base address to
        // allow 3gb of user address space, then set the user probe address
        // and the maximum system cache size.
        //

#if defined(_X86_)

        MmVirtualBias = LoaderBlock->u.I386.VirtualBias;

        if (MmVirtualBias != 0) {
            MmHighestUserAddress = ((PCHAR)MmHighestUserAddress + 0x40000000);
            MmSystemRangeStart = ((PCHAR)MmSystemRangeStart + 0x40000000);
            MmUserProbeAddress += 0x40000000;
            MiMaximumWorkingSet += 0x40000000 >> PAGE_SHIFT;

            MiHighestUserPte = MiGetPteAddress (MmHighestUserAddress);
            MiHighestUserPde = MiGetPdeAddress (MmHighestUserAddress);

            MaximumSystemCacheSize -= MM_BOOT_IMAGE_SIZE >> PAGE_SHIFT;

            if (MiHydra == TRUE) {

                //
                // Moving to 3GB means moving session space to just above
                // the system cache (and lowering the system cache max size
                // accordingly).
                //

                MaximumSystemCacheSize -= (MI_SESSION_SPACE_TOTAL_SIZE + MM_SYSTEM_VIEW_SIZE_IF_HYDRA) >> PAGE_SHIFT;

                MiSystemViewStart = (ULONG_PTR)(MM_SYSTEM_CACHE_START +
                                      (MaximumSystemCacheSize << PAGE_SHIFT));

                MmSessionBase = MiSystemViewStart + MM_SYSTEM_VIEW_SIZE_IF_HYDRA + MM_BOOT_IMAGE_SIZE;

            } else {
                MaximumSystemCacheSize -= MM_SYSTEM_VIEW_SIZE >> PAGE_SHIFT;
                MiSystemViewStart = (ULONG_PTR)(MM_SYSTEM_CACHE_START +
                                      (MaximumSystemCacheSize << PAGE_SHIFT) +
                                      MM_BOOT_IMAGE_SIZE);
            }
        }

#else

        if (MiHydra == TRUE) {
            MaximumSystemCacheSize -= MM_SYSTEM_VIEW_SIZE_IF_HYDRA >> PAGE_SHIFT;
            MiSystemViewStart = MM_SYSTEM_VIEW_START_IF_HYDRA;
        }

#endif

        if (MiHydra == TRUE) {
            MmSessionSpace = (PMM_SESSION_SPACE)((ULONG_PTR)MmSessionBase + MI_SESSION_IMAGE_SIZE);

            MiSessionBasePte = MiGetPteAddress (MmSessionBase);
            MiSessionLastPte = MiGetPteAddress (MI_SESSION_SPACE_END);
        }

        //
        // A few sanity checks to ensure things are as they should be.
        //

#if DBG
        if ((sizeof(MMWSL) % 8) != 0) {
            DbgPrint("working set list is not a quadword sized structure\n");
        }

        if ((sizeof(CONTROL_AREA) % 8) != 0) {
            DbgPrint("control area list is not a quadword sized structure\n");
        }

        if ((sizeof(SUBSECTION) % 8) != 0) {
            DbgPrint("subsection list is not a quadword sized structure\n");
        }

        //
        // Some checks to make sure prototype PTEs can be placed in
        // either paged or nonpaged (prototype PTEs for paged pool are here)
        // can be put into pte format.
        //

        PointerPte = (PMMPTE)MmPagedPoolStart;
        Pointer.u.Long = MiProtoAddressForPte (PointerPte);
        TempPte = Pointer;
        PointerPde = MiPteToProto(&TempPte);
        if (PointerPte != PointerPde) {
            DbgPrint("unable to map start of paged pool as prototype pte %p %p\n",
                     PointerPde,
                     PointerPte);
        }

        PointerPte =
                (PMMPTE)((ULONG_PTR)MM_NONPAGED_POOL_END & ~((1 << PTE_SHIFT) - 1));

        Pointer.u.Long = MiProtoAddressForPte (PointerPte);
        TempPte = Pointer;
        PointerPde = MiPteToProto(&TempPte);
        if (PointerPte != PointerPde) {
            DbgPrint("unable to map end of nonpaged pool as prototype pte %p %p\n",
                     PointerPde,
                     PointerPte);
        }

        PointerPte = (PMMPTE)(((ULONG_PTR)NON_PAGED_SYSTEM_END -
                        0x37000 + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

        for (j = 0; j < 20; j++) {
            Pointer.u.Long = MiProtoAddressForPte (PointerPte);
            TempPte = Pointer;
            PointerPde = MiPteToProto(&TempPte);
            if (PointerPte != PointerPde) {
                DbgPrint("unable to map end of nonpaged pool as prototype pte %p %p\n",
                         PointerPde,
                         PointerPte);
            }

            PointerPte++;
        }

        PointerPte = (PMMPTE)(((ULONG_PTR)MM_NONPAGED_POOL_END - 0x133448) & ~(ULONG_PTR)7);
        Pointer.u.Long = MiGetSubsectionAddressForPte (PointerPte);
        TempPte = Pointer;
        PointerPde = (PMMPTE)MiGetSubsectionAddress(&TempPte);
        if (PointerPte != PointerPde) {
            DbgPrint("unable to map end of nonpaged pool as section pte %p %p\n",
                     PointerPde,
                     PointerPte);

            MiFormatPte(&TempPte);
        }

        //
        // End of sanity checks.
        //

#endif //dbg

        if (MmEnforceWriteProtection) {
            MiPteStr[0] = (CHAR)1;
        }
    
        InitializeListHead( &MmLoadedUserImageList );
        InitializeListHead( &MmLockConflictList );

        MmCriticalSectionTimeout.QuadPart = Int32x32To64(
                                                 MmCritsectTimeoutSeconds,
                                                -10000000);


        //
        // Initialize PFN database mutex and System Address Space creation
        // mutex.
        //

        ExInitializeFastMutex (&MmSectionCommitMutex);
        ExInitializeFastMutex (&MmSectionBasedMutex);
        ExInitializeFastMutex (&MmDynamicMemoryMutex);

        KeInitializeMutant (&MmSystemLoadLock, FALSE);

        KeInitializeEvent (&MmAvailablePagesEvent, NotificationEvent, TRUE);
        KeInitializeEvent (&MmAvailablePagesEventHigh, NotificationEvent, TRUE);
        KeInitializeEvent (&MmMappedFileIoComplete, NotificationEvent, FALSE);
        KeInitializeEvent (&MmImageMappingPteEvent, NotificationEvent, FALSE);
        KeInitializeEvent (&MmZeroingPageEvent, SynchronizationEvent, FALSE);
        KeInitializeEvent (&MmCollidedFlushEvent, NotificationEvent, FALSE);
        KeInitializeEvent (&MmCollidedLockEvent, NotificationEvent, FALSE);
        KeInitializeEvent (&MiMappedPagesTooOldEvent, NotificationEvent, FALSE);

        KeInitializeDpc( &MiModifiedPageWriterTimerDpc, MiModifiedPageWriterTimerDispatch, NULL );
        KeInitializeTimerEx( &MiModifiedPageWriterTimer, SynchronizationTimer );

        MiModifiedPageLife.QuadPart = Int32x32To64(
                                                 MmModifiedPageLifeInSeconds,
                                                -10000000);

        InitializeListHead (&MmWorkingSetExpansionHead.ListHead);
        InitializeListHead (&MmInPageSupportList.ListHead);
        InitializeListHead (&MmEventCountList.ListHead);

        MmZeroingPageThreadActive = FALSE;

        //
        // Compute physical memory blocks yet again
        //

        Memory = (PPHYSICAL_MEMORY_DESCRIPTOR)&MemoryAlloc;
        Memory->NumberOfRuns = MAX_PHYSICAL_MEMORY_FRAGMENTS;

        // include all memory types ...
        for (i=0; i < LoaderMaximum; i++) {
            IncludeType[i] = TRUE;
        }

        // ... expect these..
        IncludeType[LoaderBad] = FALSE;
        IncludeType[LoaderFirmwarePermanent] = FALSE;
        IncludeType[LoaderSpecialMemory] = FALSE;
        IncludeType[LoaderBBTMemory] = FALSE;

        MmInitializeMemoryLimits(LoaderBlock, IncludeType, Memory);

#if defined (_X86PAE_)
        MiCheckPaeLicense (LoaderBlock, IncludeType, Memory);
#endif

#if defined (_X86PAE_) || defined (_WIN64)
        Mm64BitPhysicalAddress = TRUE;
#endif

        //
        // Add all memory runs in PhysicalMemoryBlock to Memory
        //

        for (i = 0; i < PhysicalMemoryBlock->NumberOfRuns; i += 1) {
            MiMergeMemoryLimit (Memory,
                                PhysicalMemoryBlock->Run[i].BasePage,
                                PhysicalMemoryBlock->Run[i].PageCount
                                );
        }

        //
        // Sort and merge adjacent runs.
        //

        for (i=0; i < Memory->NumberOfRuns; i++) {
            for (j=i+1; j < Memory->NumberOfRuns; j++) {
                if (Memory->Run[j].BasePage < Memory->Run[i].BasePage) {
                    // swap runs
                    PhysicalMemoryBlock->Run[0] = Memory->Run[j];
                    Memory->Run[j] = Memory->Run[i];
                    Memory->Run[i] = PhysicalMemoryBlock->Run[0];
                }

                if (Memory->Run[i].BasePage + Memory->Run[i].PageCount ==
                    Memory->Run[j].BasePage) {
                    // merge runs
                    Memory->NumberOfRuns -= 1;
                    Memory->Run[i].PageCount += Memory->Run[j].PageCount;
                    Memory->Run[j] = Memory->Run[Memory->NumberOfRuns];
                    i -= 1;
                    break;
                }
            }
        }

        //
        // When safebooting, don't enable special pool, the verifier or any
        // other options that track corruption regardless of registry settings.
        //
    
        if (strstr(LoaderBlock->LoadOptions, SAFEBOOT_LOAD_OPTION_A)) {
            MmVerifyDriverBufferLength = (ULONG)-1;
            MmDontVerifyRandomDrivers = TRUE;
            MmSpecialPoolTag = (ULONG)-1;
            MmSnapUnloads = FALSE;
            MmProtectFreedNonPagedPool = FALSE;
            MmEnforceWriteProtection = 0;
            MmTrackLockedPages = FALSE;
            MmTrackPtes = FALSE;
        }
        else {
            MiTriageSystem (LoaderBlock);
        }

        SystemPteMultiplier = 0;

        if (MmNumberOfSystemPtes == 0) {
#if defined (_WIN64)

            //
            // 64-bit NT is not contrained by virtual address space.  No
            // tradeoffs between nonpaged pool, paged pool and system PTEs
            // need to be made.  So just allocate PTEs on a linear scale as
            // a function of the amount of RAM.
            //
            // For example on Alpha64, 4gb of RAM gets 128gb of PTEs by default.
            // The page table cost is the inversion of the multiplier based
            // on the PTE_PER_PAGE.
            //

            if ((MiHydra == TRUE) && (ExpMultiUserTS == TRUE)) {
                SystemPteMultiplier = 128;
            }
            else {
                SystemPteMultiplier = 64;
            }
            if (Memory->NumberOfPages < 0x8000) {
                SystemPteMultiplier >>= 1;
            }
#else
            if (Memory->NumberOfPages < MM_MEDIUM_SYSTEM) {
                MmNumberOfSystemPtes = MM_MINIMUM_SYSTEM_PTES;
            } else {
                MmNumberOfSystemPtes = MM_DEFAULT_SYSTEM_PTES;
                if (Memory->NumberOfPages > 8192) {
                    MmNumberOfSystemPtes += MmNumberOfSystemPtes;

                    //
                    // Any reasonable Hydra machine gets the maximum.
                    //

                    if ((MiHydra == TRUE) && (ExpMultiUserTS == TRUE)) {
                        MmNumberOfSystemPtes = MM_MAXIMUM_SYSTEM_PTES;
                    }
                }
            }
#endif
        }
        else if (MmNumberOfSystemPtes == (ULONG)-1) {

            //
            // This registry setting indicates the maximum number of
            // system PTEs possible for this machine must be allocated.
            // Snap this for later reference.
            //
        
            MiRequestedSystemPtes = MmNumberOfSystemPtes;

#if defined (_WIN64)
            SystemPteMultiplier = 256;
#else
            MmNumberOfSystemPtes = MM_MAXIMUM_SYSTEM_PTES;
#endif
        }

        if (SystemPteMultiplier != 0) {
            if (Memory->NumberOfPages * SystemPteMultiplier > MM_MAXIMUM_SYSTEM_PTES) {
                MmNumberOfSystemPtes = MM_MAXIMUM_SYSTEM_PTES;
            }
            else {
                MmNumberOfSystemPtes = (ULONG)(Memory->NumberOfPages * SystemPteMultiplier);
            }
        }

        if (MmNumberOfSystemPtes > MM_MAXIMUM_SYSTEM_PTES)  {
            MmNumberOfSystemPtes = MM_MAXIMUM_SYSTEM_PTES;
        }

        if (MmNumberOfSystemPtes < MM_MINIMUM_SYSTEM_PTES) {
            MmNumberOfSystemPtes = MM_MINIMUM_SYSTEM_PTES;
        }

        if (MmHeapSegmentReserve == 0) {
            MmHeapSegmentReserve = 1024 * 1024;
        }

        if (MmHeapSegmentCommit == 0) {
            MmHeapSegmentCommit = PAGE_SIZE * 2;
        }

        if (MmHeapDeCommitTotalFreeThreshold == 0) {
            MmHeapDeCommitTotalFreeThreshold = 64 * 1024;
        }

        if (MmHeapDeCommitFreeBlockThreshold == 0) {
            MmHeapDeCommitFreeBlockThreshold = PAGE_SIZE;
        }

#ifndef NO_POOL_CHECKS
        MiInitializeSpecialPoolCriteria ();
#endif

        //
        // If the registry indicates drivers are in the suspect list,
        // extra system PTEs need to be allocated to support special pool
        // for their allocations.
        //

        if ((MmVerifyDriverBufferLength != (ULONG)-1) ||
            ((MmSpecialPoolTag != 0) && (MmSpecialPoolTag != (ULONG)-1))) {
            MmNumberOfSystemPtes += MM_SPECIAL_POOL_PTES;
        }

        MmNumberOfSystemPtes += BBTPagesToReserve;

        //
        // Initialize the machine dependent portion of the hardware.
        //

        ExInitializeResource (&MmSystemWsLock);

        MiInitMachineDependent (LoaderBlock);

#if PFN_CONSISTENCY
        MiPfnProtectionEnabled = TRUE;
#endif

        MiReloadBootLoadedDrivers (LoaderBlock);

        MiInitializeDriverVerifierList (LoaderBlock);

        j = (sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
             (sizeof(PHYSICAL_MEMORY_RUN) *
                    (Memory->NumberOfRuns - 1)));

        MmPhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPoolMustSucceed,
                                                       j,
                                                       '  mM');

        RtlCopyMemory (MmPhysicalMemoryBlock, Memory, j);

        //
        // Setup the system size as small, medium, or large depending
        // on memory available.
        //
        // For internal MM tuning, the following applies
        //
        // 12Mb  is small
        // 12-19 is medium
        // > 19 is large
        //
        //
        // For all other external tuning,
        // < 19 is small
        // 19 - 31 is medium for workstation
        // 19 - 63 is medium for server
        // >= 32 is large for workstation
        // >= 64 is large for server
        //

        MmReadClusterSize = 7;
        if (MmNumberOfPhysicalPages <= MM_SMALL_SYSTEM ) {
            MmSystemSize = MmSmallSystem;
            MmMaximumDeadKernelStacks = 0;
            MmModifiedPageMinimum = 40;
            MmModifiedPageMaximum = 100;
            MmDataClusterSize = 0;
            MmCodeClusterSize = 1;
            MmReadClusterSize = 2;

        } else if (MmNumberOfPhysicalPages <= MM_MEDIUM_SYSTEM ) {
            MmSystemSize = MmSmallSystem;
            MmMaximumDeadKernelStacks = 2;
            MmModifiedPageMinimum = 80;
            MmModifiedPageMaximum = 150;
            MmSystemCacheWsMinimum += 100;
            MmSystemCacheWsMaximum += 150;
            MmDataClusterSize = 1;
            MmCodeClusterSize = 2;
            MmReadClusterSize = 4;

        } else {
            MmSystemSize = MmMediumSystem;
            MmMaximumDeadKernelStacks = 5;
            MmModifiedPageMinimum = 150;
            MmModifiedPageMaximum = 300;
            MmSystemCacheWsMinimum += 400;
            MmSystemCacheWsMaximum += 800;
            MmDataClusterSize = 3;
            MmCodeClusterSize = 7;
        }

        if (MmNumberOfPhysicalPages < ((24*1024*1024)/PAGE_SIZE)) {
            MmSystemCacheWsMinimum = 32;
        }

        if (MmNumberOfPhysicalPages >= ((32*1024*1024)/PAGE_SIZE)) {

            //
            // If we are on a workstation, 32Mb and above are considered large systems
            //
            if ( MmProductType == 0x00690057 ) {
                MmSystemSize = MmLargeSystem;

            } else {

                //
                // For servers, 64Mb and greater is a large system
                //

                if (MmNumberOfPhysicalPages >= ((64*1024*1024)/PAGE_SIZE)) {
                    MmSystemSize = MmLargeSystem;
                }
            }
        }

        if (MmNumberOfPhysicalPages > ((33*1024*1024)/PAGE_SIZE)) {
            MmModifiedPageMinimum = 400;
            MmModifiedPageMaximum = 800;
            MmSystemCacheWsMinimum += 500;
            MmSystemCacheWsMaximum += 900;
        }

        //
        // determine if we are on an AS system ( Winnt is not AS)
        //

        if (MmProductType == 0x00690057) {
            SharedUserData->NtProductType = NtProductWinNt;
            MmProductType = 0;
            MmThrottleTop = 250;
            MmThrottleBottom = 30;

        } else {
            if ( MmProductType == 0x0061004c ) {
                SharedUserData->NtProductType = NtProductLanManNt;

            } else {
                SharedUserData->NtProductType = NtProductServer;
            }

            MmProductType = 1;
            MmThrottleTop = 450;
            MmThrottleBottom = 80;
            MmMinimumFreePages = 81;
        }

        MiAdjustWorkingSetManagerParameters((BOOLEAN)(MmProductType == 0 ? TRUE : FALSE));

        //
        // Set the ResidentAvailablePages to the number of available
        // pages minus the fluid value.
        //

        MmResidentAvailablePages = MmAvailablePages - MM_FLUID_PHYSICAL_PAGES;

        //
        // Subtract off the size of the system cache working set.
        //

        MmResidentAvailablePages -= MmSystemCacheWsMinimum;
        MmResidentAvailableAtInit = MmResidentAvailablePages;


        if (MmResidentAvailablePages < 0) {
#if DBG
            DbgPrint("system cache working set too big\n");
#endif
            return FALSE;
        }

        //
        // Initialize spin lock for charging and releasing page file
        // commitment.
        //

        KeInitializeSpinLock (&MmChargeCommitmentLock);

        MiInitializeIoTrackers ();

        //
        // Initialize spin lock for allowing working set expansion.
        //

        KeInitializeSpinLock (&MmExpansionLock);

        ExInitializeFastMutex (&MmPageFileCreationLock);

        //
        // Initialize resource for extending sections.
        //

        ExInitializeResource (&MmSectionExtendResource);
        ExInitializeResource (&MmSectionExtendSetResource);

        //
        // Build the system cache structures.
        //

        StartPde = MiGetPdeAddress (MmSystemCacheWorkingSetList);
        PointerPte = MiGetPteAddress (MmSystemCacheWorkingSetList);

#if defined (_WIN64)

        StartPpe = MiGetPteAddress(StartPde);

        TempPte = ValidKernelPte;

        if (StartPpe->u.Hard.Valid == 0) {

            //
            // Map in a page directory page for the system cache working set.
            // Note that we only populate one page table for this.
            //

            DirectoryFrameIndex = MiRemoveAnyPage(
                MI_GET_PAGE_COLOR_FROM_PTE (StartPpe));
            TempPte.u.Hard.PageFrameNumber = DirectoryFrameIndex;
            *StartPpe = TempPte;


            Pfn1 = MI_PFN_ELEMENT(DirectoryFrameIndex);
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (
                                  MiGetPteAddress(PDE_KTBASE));
            Pfn1->PteAddress = StartPpe;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

            MiFillMemoryPte (StartPde,
                             PAGE_SIZE,
                             ZeroKernelPte.u.Long);
        }

        //
        // Map in a page table page.
        //

        ASSERT (StartPde->u.Hard.Valid == 0);

        PageFrameIndex = MiRemoveAnyPage(
                                MI_GET_PAGE_COLOR_FROM_PTE (StartPde));
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (StartPde, TempPte);

        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        Pfn1->PteFrame = DirectoryFrameIndex;
        Pfn1->PteAddress = StartPde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

        MiFillMemoryPte (MiGetVirtualAddressMappedByPte (StartPde),
                         PAGE_SIZE,
                         ZeroKernelPte.u.Long);

        StartPpe = MiGetPpeAddress(MmSystemCacheStart);
        StartPde = MiGetPdeAddress(MmSystemCacheStart);
        PointerPte = MiGetVirtualAddressMappedByPte (StartPde);

#else
#if !defined(_X86PAE_)
        ASSERT ((StartPde + 1) == MiGetPdeAddress (MmSystemCacheStart));
#endif
#endif

        MaximumSystemCacheSizeTotal = MaximumSystemCacheSize;

#if defined(_X86_)
        MaximumSystemCacheSizeTotal += MiMaximumSystemCacheSizeExtra;
#endif

        //
        // Size the system cache based on the amount of physical memory.
        //

        i = (MmNumberOfPhysicalPages + 65) / 1024;

        if (i >= 4) {

            //
            // System has at least 4032 pages.  Make the system
            // cache 128mb + 64mb for each additional 1024 pages.
            //

            MmSizeOfSystemCacheInPages = (PFN_COUNT)(
                            ((128*1024*1024) >> PAGE_SHIFT) +
                            ((i - 4) * ((64*1024*1024) >> PAGE_SHIFT)));
            if (MmSizeOfSystemCacheInPages > MaximumSystemCacheSizeTotal) {
                MmSizeOfSystemCacheInPages = MaximumSystemCacheSizeTotal;
            }
        }

        MmSystemCacheEnd = (PVOID)(((PCHAR)MmSystemCacheStart +
                    MmSizeOfSystemCacheInPages * PAGE_SIZE) - 1);

#if defined(_X86_)
        if (MmSizeOfSystemCacheInPages > MaximumSystemCacheSize) {
            ASSERT (MiMaximumSystemCacheSizeExtra != 0);
            MmSystemCacheEnd = (PVOID)(((PCHAR)MmSystemCacheStart +
                        MaximumSystemCacheSize * PAGE_SIZE) - 1);

            MiSystemCacheStartExtra = (PVOID)MM_SYSTEM_CACHE_START_EXTRA;
            MiSystemCacheEndExtra = (PVOID)(((PCHAR)MiSystemCacheStartExtra +
                        (MmSizeOfSystemCacheInPages - MaximumSystemCacheSize) * PAGE_SIZE) - 1);
        }
		else {
            MiSystemCacheStartExtra = MmSystemCacheStart;
            MiSystemCacheEndExtra = MmSystemCacheEnd;
		}
#endif

        EndPde = MiGetPdeAddress(MmSystemCacheEnd);

        TempPte = ValidKernelPte;

#if defined(_WIN64)
        First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;
#endif

#if !defined (_WIN64)
        DirectoryFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PDE_BASE));
#endif

        LOCK_PFN (OldIrql);
        while (StartPde <= EndPde) {

#if defined (_WIN64)
            if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
                First = FALSE;
                StartPpe = MiGetPteAddress(StartPde);

                //
                // Map in a page directory page.
                //

                DirectoryFrameIndex = MiRemoveAnyPage(
                                        MI_GET_PAGE_COLOR_FROM_PTE (StartPpe));
                TempPte.u.Hard.PageFrameNumber = DirectoryFrameIndex;
                *StartPpe = TempPte;

                Pfn1 = MI_PFN_ELEMENT(DirectoryFrameIndex);
                Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (
                                        MiGetPteAddress(PDE_KTBASE));
                Pfn1->PteAddress = StartPpe;
                Pfn1->u2.ShareCount += 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

                MiFillMemoryPte (StartPde,
                                 PAGE_SIZE,
                                 ZeroKernelPte.u.Long);
            }
#endif

            ASSERT (StartPde->u.Hard.Valid == 0);

            //
            // Map in a page table page.
            //

            PageFrameIndex = MiRemoveAnyPage(
                                    MI_GET_PAGE_COLOR_FROM_PTE (StartPde));
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE (StartPde, TempPte);

            Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
            Pfn1->PteFrame = DirectoryFrameIndex;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

            MiFillMemoryPte (PointerPte,
                             PAGE_SIZE,
                             ZeroKernelPte.u.Long);

            StartPde += 1;
            PointerPte += PTE_PER_PAGE;
        }

#if defined(_X86_)
        if (MiSystemCacheEndExtra != MmSystemCacheEnd) {

            StartPde = MiGetPdeAddress (MiSystemCacheStartExtra);
			EndPde = MiGetPdeAddress(MiSystemCacheEndExtra);

			PointerPte = MiGetPteAddress (MiSystemCacheStartExtra);

			while (StartPde <= EndPde) {

				ASSERT (StartPde->u.Hard.Valid == 0);

                //
                // Map in a page directory page.
                //

                PageFrameIndex = MiRemoveAnyPage(
                                        MI_GET_PAGE_COLOR_FROM_PTE (StartPde));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (StartPde, TempPte);

                Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
                Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (
                                        MiGetPdeAddress(PDE_BASE));
                Pfn1->PteAddress = StartPde;
                Pfn1->u2.ShareCount += 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

                MiFillMemoryPte (PointerPte,
                                 PAGE_SIZE,
                                 ZeroKernelPte.u.Long);

                StartPde += 1;
                PointerPte += PTE_PER_PAGE;
            }
        }
#endif

        UNLOCK_PFN (OldIrql);

        //
        // Initialize the system cache.  Only set the large system cache if
        // we have a large amount of physical memory.
        //

        if (MmLargeSystemCache != 0 && MmNumberOfPhysicalPages > 0x7FF0) {
            if ((MmAvailablePages >
                    MmSystemCacheWsMaximum + ((64*1024*1024) >> PAGE_SHIFT))) {
                MmSystemCacheWsMaximum =
                            MmAvailablePages - ((32*1024*1024) >> PAGE_SHIFT);
                ASSERT ((LONG)MmSystemCacheWsMaximum > (LONG)MmSystemCacheWsMinimum);
                MmMoreThanEnoughFreePages = 256;
            }
        }

        if (MmSystemCacheWsMaximum > (MM_MAXIMUM_WORKING_SET - 5)) {
            MmSystemCacheWsMaximum = MM_MAXIMUM_WORKING_SET - 5;
        }

        if (MmSystemCacheWsMaximum > MmSizeOfSystemCacheInPages) {
            MmSystemCacheWsMaximum = MmSizeOfSystemCacheInPages;
            if ((MmSystemCacheWsMinimum + 500) > MmSystemCacheWsMaximum) {
                MmSystemCacheWsMinimum = MmSystemCacheWsMaximum - 500;
            }
        }

        MiInitializeSystemCache ((ULONG)MmSystemCacheWsMinimum,
                                 (ULONG)MmSystemCacheWsMaximum);

        //
        // Set the commit page limit to four times the number of available
        // pages. This value is updated as paging files are created.
        //

        MmTotalCommitLimit = MmAvailablePages << 2;
        MmTotalCommitLimitMaximum = MmTotalCommitLimit;

        MmAttemptForCantExtend.Segment = NULL;
        MmAttemptForCantExtend.RequestedExpansionSize = 1;
        MmAttemptForCantExtend.ActualExpansion = 1;
        MmAttemptForCantExtend.InProgress = FALSE;
        MmAttemptForCantExtend.PageFileNumber = MI_EXTEND_ANY_PAGEFILE;

        KeInitializeEvent (&MmAttemptForCantExtend.Event,
                           NotificationEvent,
                           FALSE);

        if (MmOverCommit == 0) {

            // If this value was not set via the registry, set the
            // over commit value to the number of available pages
            // minus 1024 pages (4mb with 4k pages).
            //

            if (MmAvailablePages > 1024) {
                MmOverCommit = MmAvailablePages - 1024;
            }
        }

        //
        // Set maximum working set size to 512 pages less total available
        // memory.  2mb on machine with 4k pages.
        //

        MmMaximumWorkingSetSize = (ULONG)(MmAvailablePages - 512);

        if (MmMaximumWorkingSetSize > (MM_MAXIMUM_WORKING_SET - 5)) {
            MmMaximumWorkingSetSize = MM_MAXIMUM_WORKING_SET - 5;
        }

        //
        // Create the modified page writer event.
        //

        KeInitializeEvent (&MmModifiedPageWriterEvent, NotificationEvent, FALSE);

        //
        // Build paged pool.
        //

        MiBuildPagedPool ();

        //
        // Initialize the loaded module list.  This cannot be done until
        // paged pool has been built.
        //

        if (MiInitializeLoadedModuleList (LoaderBlock) == FALSE) {
#if DBG
            DbgPrint("Loaded module list initialization failed\n");
#endif
            return FALSE;
        }

        //
        // Initialize the unused segment thresholds.  The assumption is made
        // that the filesystem will tack on approximately a 1024-byte paged
        // pool charge (regardless of file size) for each file in the cache.
        //

        if (MmUnusedSegmentTrimLevel < 5) {
            MmUnusedSegmentTrimLevel = 5;
        }
        else if (MmUnusedSegmentTrimLevel > 40) {
            MmUnusedSegmentTrimLevel = 40;
        }
	
        MmMaxUnusedSegmentPagedPoolUsage = (MmSizeOfPagedPoolInBytes / 100) * (MmUnusedSegmentTrimLevel << 1);
        MmUnusedSegmentPagedPoolReduction = MmMaxUnusedSegmentPagedPoolUsage >> 2;

        MmMaxUnusedSegmentNonPagedPoolUsage = (MmMaximumNonPagedPoolInBytes / 100) * (MmUnusedSegmentTrimLevel << 1);
        MmUnusedSegmentNonPagedPoolReduction = MmMaxUnusedSegmentNonPagedPoolUsage >> 2;

        //
        // Add more system PTEs if this is a large memory system.
        // Note that 64 bit systems can determine the right value at the
        // beginning since there is no virtual address space crunch.
        //

#if !defined (_WIN64)
        if (MmNumberOfPhysicalPages > ((127*1024*1024) >> PAGE_SHIFT)) {

            PointerPde = MiGetPdeAddress ((PCHAR)MmPagedPoolEnd + 1);
            StartingPte = MiGetPteAddress ((PCHAR)MmPagedPoolEnd + 1);
            j = 0;

            TempPte = ValidKernelPde;
            LOCK_PFN (OldIrql);
            while (PointerPde->u.Hard.Valid == 0) {

                MiChargeCommitmentCantExpand (1, TRUE);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_EXTRA_SYSTEM_PTES, 1);

                PageFrameIndex = MiRemoveZeroPage (
                                    MI_GET_PAGE_COLOR_FROM_PTE (PointerPde));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPde, TempPte);
                MiInitializePfn (PageFrameIndex, PointerPde, 1);
                PointerPde += 1;
                StartingPte += PAGE_SIZE / sizeof(MMPTE);
                j += PAGE_SIZE / sizeof(MMPTE);
            }

            UNLOCK_PFN (OldIrql);

            if (j != 0) {
                StartingPte = MiGetPteAddress ((PCHAR)MmPagedPoolEnd + 1);
                MmNonPagedSystemStart = MiGetVirtualAddressMappedByPte (StartingPte);
                MmNumberOfSystemPtes += j;
                MiAddSystemPtes (StartingPte, j, SystemPteSpace);
            }
        }
#endif


#if DBG
        if (MmDebug & MM_DBG_DUMP_BOOT_PTES) {
            MiDumpValidAddresses ();
            MiDumpPfn ();
        }
#endif

        MmPageFaultNotifyRoutine = NULL;
        MmHardFaultNotifyRoutine = NULL;

        return TRUE;
    }

    if (Phase == 1) {

#if DBG
        MmDebug |= MM_DBG_CHECK_PFN_LOCK;
#endif

#ifdef _X86_
        MiInitMachineDependent (LoaderBlock);
#endif
        MiMapBBTMemory(LoaderBlock);

        if (!MiSectionInitialization ()) {
            return FALSE;
        }

        Process = PsGetCurrentProcess ();
        if (Process->PhysicalVadList.Flink == NULL) {
            KeInitializeSpinLock (&Process->AweLock);
            InitializeListHead (&Process->PhysicalVadList);
        }

#if defined(MM_SHARED_USER_DATA_VA)

        //
        // Create double mapped page between kernel and user mode.
        //

        PointerPte = MiGetPteAddress(KI_USER_SHARED_DATA);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        MI_MAKE_VALID_PTE (MmSharedUserDataPte,
                           PageFrameIndex,
                           MM_READONLY,
                           PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        LOCK_PFN (OldIrql);

        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

        UNLOCK_PFN (OldIrql);
#endif

        if (MiHydra == TRUE) {
            MiSessionWideInitializeAddresses ();
            MiInitializeSessionWsSupport ();
            MiInitializeSessionIds ();
        }

        //
        // Set up system wide lock pages limit.
        //

        if ((MmLockPagesPercentage < 5) || (MmLockPagesPercentage >= 100)) {

            //
            // No (reasonable or max) registry override from the user
            // so default to allowing all available memory.
            //

            MmLockPagesLimit = (PFN_NUMBER)-1;
        }
        else {

            //
            // Use the registry value - note it is expressed as a percentage.
            //

            MmLockPagesLimit = (PFN_NUMBER)((MmAvailablePages * MmLockPagesPercentage) / 100);
        }

        //
        // Start the modified page writer.
        //

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        if (!NT_SUCCESS(PsCreateSystemThread(
                        &ThreadHandle,
                        THREAD_ALL_ACCESS,
                        &ObjectAttributes,
                        0L,
                        NULL,
                        MiModifiedPageWriter,
                        NULL
                        ))) {
            return FALSE;
        }
        ZwClose (ThreadHandle);

        //
        // Start the balance set manager.
        //
        // The balance set manager performs stack swapping and working
        // set management and requires two threads.
        //

        KeInitializeEvent (&MmWorkingSetManagerEvent,
                           SynchronizationEvent,
                           FALSE);

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        if (!NT_SUCCESS(PsCreateSystemThread(
                        &ThreadHandle,
                        THREAD_ALL_ACCESS,
                        &ObjectAttributes,
                        0L,
                        NULL,
                        KeBalanceSetManager,
                        NULL
                        ))) {

            return FALSE;
        }
        ZwClose (ThreadHandle);

        if (!NT_SUCCESS(PsCreateSystemThread(
                        &ThreadHandle,
                        THREAD_ALL_ACCESS,
                        &ObjectAttributes,
                        0L,
                        NULL,
                        KeSwapProcessOrStack,
                        NULL
                        ))) {

            return FALSE;
        }
        ZwClose (ThreadHandle);

#ifndef NO_POOL_CHECKS
        MiInitializeSpecialPoolCriteria ();
#endif

#if defined(_X86_)
        MiEnableKernelVerifier ();
#endif

        ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);

        NextEntry = PsLoadedModuleList.Flink;
    
        for ( ; NextEntry != &PsLoadedModuleList; NextEntry = NextEntry->Flink) {
    
            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);
    
            NtHeaders = RtlImageNtHeader(DataTableEntry->DllBase);

            if ((NtHeaders->OptionalHeader.MajorOperatingSystemVersion >= 5) &&
                (NtHeaders->OptionalHeader.MajorImageVersion >= 5)) {
                DataTableEntry->Flags |= LDRP_ENTRY_NATIVE;
            }

            MiWriteProtectSystemImage (DataTableEntry->DllBase);
        }
        ExReleaseResource (&PsLoadedModuleResource);

        InterlockedDecrement (&MiTrimInProgressCount);

        return TRUE;
    }

    if (Phase == 2) {
        MiEnablePagingTheExecutive();
        return TRUE;
    }

    return FALSE;
}

VOID
MiMapBBTMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function walks through the loader block's memory descriptor list
    and maps memory reserved for the BBT buffer into the system.

    The mapped PTEs are PDE-aligned and made user accessible.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    None.

Environment:

    Kernel Mode Only.  System initialization.

--*/
{
    PVOID Va;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextMd;
    PFN_NUMBER NumberOfPagesMapped;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE LastPde;
    MMPTE TempPte;

    if (BBTPagesToReserve <= 0) {
        return;
    }

    //
    // Request enough PTEs such that protection can be applied to the PDEs.
    //

    NumberOfPages = (BBTPagesToReserve + (PTE_PER_PAGE - 1)) & ~(PTE_PER_PAGE - 1);

    PointerPte = MiReserveSystemPtes((ULONG)NumberOfPages,
                                     SystemPteSpace,
                                     MM_VA_MAPPED_BY_PDE,
                                     0,
                                     FALSE);

    if (PointerPte == NULL) {
        BBTPagesToReserve = 0;
        return;
    }

    //
    // Allow user access to the buffer.
    //

    PointerPde = MiGetPteAddress (PointerPte);
    LastPde = MiGetPteAddress (PointerPte + NumberOfPages);

    ASSERT (LastPde != PointerPde);

    do {
        TempPte = *PointerPde;
        TempPte.u.Long |= MM_PTE_OWNER_MASK;
        MI_WRITE_VALID_PTE (PointerPde, TempPte);
        PointerPde += 1;
    } while (PointerPde < LastPde);

    KeFlushEntireTb (TRUE, TRUE);

    Va = MiGetVirtualAddressMappedByPte (PointerPte);

    TempPte = ValidUserPte;
    NumberOfPagesMapped = 0;

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (MemoryDescriptor->MemoryType == LoaderBBTMemory) {

            PageFrameIndex = MemoryDescriptor->BasePage;
            NumberOfPages = MemoryDescriptor->PageCount;

            if (NumberOfPagesMapped + NumberOfPages > BBTPagesToReserve) {
                NumberOfPages = BBTPagesToReserve - NumberOfPagesMapped;
            }

            NumberOfPagesMapped += NumberOfPages;

            do {

                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                PointerPte += 1;
                PageFrameIndex += 1;
                NumberOfPages -= 1;
            } while (NumberOfPages);

            if (NumberOfPagesMapped == BBTPagesToReserve) {
                break;
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    RtlZeroMemory(Va, BBTPagesToReserve << PAGE_SHIFT);

    //
    // Tell BBT_Init how many pages were allocated.
    //

    if (NumberOfPagesMapped < BBTPagesToReserve) {
        BBTPagesToReserve = (ULONG)NumberOfPagesMapped;
    }
    *(PULONG)Va = BBTPagesToReserve;

    //
    // At this point instrumentation code will detect the existence of
    // buffer and initialize the structures.
    //

    BBTBuffer = Va;

    PERFINFO_MMINIT_START();
}


VOID
MmInitializeMemoryLimits (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludeType,
    OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory
    )

/*++

Routine Description:

    This function walks through the loader block's memory
    descriptor list and builds a list of contiguous physical
    memory blocks of the desired types.

Arguments:

    LoaderBlock - Supplies a pointer the system loader block.

    IncludeType - Array of BOOLEANS of size LoaderMaximum.
                  TRUE means include this type of memory in return.

    Memory - Returns the physical memory blocks.

Return Value:

    None.

Environment:

    Kernel Mode Only.  System initialization.

--*/
{

    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextMd;
    PFN_NUMBER i;
    PFN_NUMBER LowestFound;
    PFN_NUMBER Found;
    PFN_NUMBER Merged;
    PFN_NUMBER NextPage;
    PFN_NUMBER TotalPages;

    TotalPages = 0;

    //
    // Walk through the memory descriptors and build the physical memory list.
    //

    LowestFound = 0;
    Memory->Run[0].BasePage = 0xffffffff;
    NextPage = 0xffffffff;
    Memory->Run[0].PageCount = 0;
    i = 0;

    do {
        Merged = FALSE;
        Found = FALSE;
        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            if (MemoryDescriptor->MemoryType < LoaderMaximum &&
                IncludeType [MemoryDescriptor->MemoryType] ) {

                //
                // Try to merge runs.
                //

                if (MemoryDescriptor->BasePage == NextPage) {
                    ASSERT (MemoryDescriptor->PageCount != 0);
                    Memory->Run[i - 1].PageCount += MemoryDescriptor->PageCount;
                    NextPage += MemoryDescriptor->PageCount;
                    TotalPages += MemoryDescriptor->PageCount;
                    Merged = TRUE;
                    Found = TRUE;
                    break;
                }

                if (MemoryDescriptor->BasePage >= LowestFound) {
                    if (Memory->Run[i].BasePage > MemoryDescriptor->BasePage) {
                        Memory->Run[i].BasePage = MemoryDescriptor->BasePage;
                        Memory->Run[i].PageCount = MemoryDescriptor->PageCount;
                    }
                    Found = TRUE;
                }
            }
            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        if (!Merged && Found) {
            NextPage = Memory->Run[i].BasePage + Memory->Run[i].PageCount;
            TotalPages += Memory->Run[i].PageCount;
            i += 1;
        }
        Memory->Run[i].BasePage = 0xffffffff;
        LowestFound = NextPage;

    } while (Found);
    ASSERT (i <= Memory->NumberOfRuns);
    Memory->NumberOfRuns = (ULONG)i;
    Memory->NumberOfPages = TotalPages;

    return;
}

#if defined (_X86PAE_)

static
VOID
MiCheckPaeLicense (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludeType,
    OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory
    )

/*++

Routine Description:

    This function walks through the loader block's memory descriptor list
    and removes descriptors above 4gb if the license does not allow them.

Arguments:

    LoaderBlock - Supplies a pointer the system loader block.

    IncludeType - Array of BOOLEANS of size LoaderMaximum.
                  TRUE means include this type of memory in return.

    Memory - Returns the physical memory blocks.

Return Value:

    None.

Environment:

    Kernel Mode Only.  System initialization.

--*/
{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextMd;
    PFN_NUMBER i;
    PFN_NUMBER LowestFound;
    PFN_NUMBER Found;
    PFN_NUMBER Merged;
    PFN_NUMBER NextPage;
    PFN_NUMBER LastPage;
    PFN_NUMBER MaxPage;
    PFN_NUMBER TotalPages;
    static BOOLEAN BeenHere = FALSE;

    if (BeenHere == TRUE) {
        return;
    }

    BeenHere = TRUE;

    //
    // If properly licensed for PAE (ie: DataCenter) and booted without the
    // 3gb switch, then use all available physical memory.
    //

    if (ExVerifySuite(DataCenter) == TRUE) {

        //
        // Note MmVirtualBias has not yet been initialized at the time of the
        // first call to this routine, so use the LoaderBlock directly.
        //

        if (LoaderBlock->u.I386.VirtualBias == 0) {

            //
            // Limit the maximum physical memory to the amount we have
            // actually physically seen in a machine inhouse.
            //

            MaxPage = 8 * 1024 * 1024;
        }
        else {

            //
            // The system is booting /3gb, so don't use more than 16gb of
            // physical memory.  This ensures enough virtual space to map
            // the PFN database.
            //

            MaxPage = 4 * 1024 * 1024;
        }
    }
    else if ((MmProductType != 0x00690057) &&
             (ExVerifySuite(Enterprise) == TRUE)) {

        //
        // Advanced Server is permitted a maximum of 8gb physical memory.
        // The system continues to operate in 8-byte PTE mode.
        //

        MaxPage = 2 * 1024 * 1024;
    }
    else {

        //
        // All other configurations get a maximum of 4gb physical memory.
        // The system continues to operate in 8-byte PTE mode.
        //

        MaxPage = 1024 * 1024;
    }

    TotalPages = 0;

    //
    // Walk through the memory descriptors and make sure no descriptors
    // reach or exceed the physical addressing limit.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (MemoryDescriptor->BasePage >= MaxPage) {
            MemoryDescriptor->BasePage = 0;
            MemoryDescriptor->PageCount = 0;
        }

        LastPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;

        if (LastPage >= MaxPage || LastPage < MemoryDescriptor->BasePage) {
            MemoryDescriptor->PageCount = MaxPage - MemoryDescriptor->BasePage - 1;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // Walk through the memory descriptors and truncate the physical memory
    // list.
    //

    LowestFound = 0;
    Memory->Run[0].BasePage = 0xffffffff;
    NextPage = 0xffffffff;
    Memory->Run[0].PageCount = 0;
    i = 0;

    do {
        Merged = FALSE;
        Found = FALSE;
        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);


            if (MemoryDescriptor->BasePage || MemoryDescriptor->PageCount) {

                if (MemoryDescriptor->MemoryType < LoaderMaximum &&
                    IncludeType [MemoryDescriptor->MemoryType] ) {
    
                    //
                    // Try to merge runs.
                    //
    
                    if (MemoryDescriptor->BasePage == NextPage) {
                        ASSERT (MemoryDescriptor->PageCount != 0);
                        Memory->Run[i - 1].PageCount += MemoryDescriptor->PageCount;
                        NextPage += MemoryDescriptor->PageCount;
                        TotalPages += MemoryDescriptor->PageCount;
                        Merged = TRUE;
                        Found = TRUE;
                        break;
                    }
    
                    if (MemoryDescriptor->BasePage >= LowestFound) {
                        if (Memory->Run[i].BasePage > MemoryDescriptor->BasePage) {
                            Memory->Run[i].BasePage = MemoryDescriptor->BasePage;
                            Memory->Run[i].PageCount = MemoryDescriptor->PageCount;
                        }
                        Found = TRUE;
                    }
                }
            }
            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        if (!Merged && Found) {
            NextPage = Memory->Run[i].BasePage + Memory->Run[i].PageCount;
            TotalPages += Memory->Run[i].PageCount;
            i += 1;
        }
        Memory->Run[i].BasePage = 0xffffffff;
        LowestFound = NextPage;

    } while (Found);
    ASSERT (i <= Memory->NumberOfRuns);
    Memory->NumberOfRuns = (ULONG)i;
    Memory->NumberOfPages = TotalPages;

    return;
}
#endif


VOID
MiMergeMemoryLimit (
    IN OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory,
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER NumberOfPages
    )
/*++

Routine Description:

    This function ensures the passed range is in the passed in Memory
    block adding any new data as needed.

    The passed memory block is assumed to be at least
    MAX_PHYSICAL_MEMORY_FRAGMENTS large

Arguments:

    Memory - Supplies the memory block to verify run is present in.

    StartPage - Supplies the first page of the run.

    NumberOfPages - Supplies the number of pages in the run.

Return Value:

    None.

Environment:

    Kernel Mode Only.  System initialization.

--*/
{
    PFN_NUMBER EndPage, sp, ep, i;

    EndPage = StartPage + NumberOfPages;

    //
    // Clip range to area which is not already described
    //

    for (i=0; i < Memory->NumberOfRuns; i++) {
        sp = Memory->Run[i].BasePage;
        ep = sp + Memory->Run[i].PageCount;

        if (sp < StartPage) {
            if (ep > StartPage  &&  ep < EndPage) {
                // bump beginning page of the target area
                StartPage = ep;
            }

            if (ep > EndPage) {
                //
                // Target area is contained totally within this
                // descriptor.  This range is fully accounted for.
                //

                StartPage = EndPage;
            }

        } else {
            // sp >= StartPage

            if (sp < EndPage) {
                if (ep < EndPage) {
                    //
                    // This descriptor is totally within the target area -
                    // check the area on either side of this descriptor
                    //

                    MiMergeMemoryLimit (Memory, StartPage, sp - StartPage);
                    StartPage = ep;

                }  else {
                    // clip the ending page of the target area
                    EndPage = sp;
                }
            }
        }

        //
        // Anything left of target area?
        //

        if (StartPage == EndPage) {
            return ;
        }
    }   // next descriptor

    //
    // The range StartPage - EndPage is a missing. Add it.
    //

    if (Memory->NumberOfRuns == MAX_PHYSICAL_MEMORY_FRAGMENTS) {
        return ;
    }

    Memory->Run[Memory->NumberOfRuns].BasePage  = StartPage;
    Memory->Run[Memory->NumberOfRuns].PageCount = EndPage - StartPage;
    Memory->NumberOfPages += EndPage - StartPage;
    Memory->NumberOfRuns  += 1;
}



VOID
MmFreeLoaderBlock (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function is called as the last routine in phase 1 initialization.
    It frees memory used by the OsLoader.

Arguments:

    LoadBlock - Supplies a pointer the system loader block.

Return Value:

    None.

Environment:

    Kernel Mode Only.  System initialization.

--*/

{

    PLIST_ENTRY NextMd;
    PMMPTE Pde;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MEMORY_ALLOCATION_DESCRIPTOR SavedDescriptor[MM_MAX_LOADER_BLOCKS];
    PFN_NUMBER i;
    PFN_NUMBER NextPhysicalPage;
    PMMPFN Pfn1;
    LONG BlockNumber = -1;
    KIRQL OldIrql;

    //
    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);


        switch (MemoryDescriptor->MemoryType) {
            case LoaderOsloaderHeap:
            case LoaderRegistryData:
            case LoaderNlsData:
            //case LoaderMemoryData:  //this has page table and other stuff.

                //
                // Capture the data to temporary storage so we won't
                // free memory we are referencing.  Coalesce it if
                // the blocks are adjacent and of the same type.
                //

                if (BlockNumber != -1 &&
                    MemoryDescriptor->MemoryType == SavedDescriptor[BlockNumber].MemoryType &&
                    MemoryDescriptor->BasePage == SavedDescriptor[BlockNumber].BasePage + SavedDescriptor[BlockNumber].PageCount) {

                        //
                        // these blocks are adjacent - merge them
                        //

                        SavedDescriptor[BlockNumber].PageCount += MemoryDescriptor->PageCount;
                }
                else {
                    BlockNumber += 1;
                    if (BlockNumber >= MM_MAX_LOADER_BLOCKS) {
                        KeBugCheckEx (MEMORY_MANAGEMENT, 0, 0, 0, 0);
                    }
                    SavedDescriptor[BlockNumber] = *MemoryDescriptor;
                }

                break;

            default:

                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    LOCK_PFN (OldIrql);

    while (BlockNumber >= 0) {

        i = SavedDescriptor[BlockNumber].PageCount;
        NextPhysicalPage = SavedDescriptor[BlockNumber].BasePage;

        Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
        while (i != 0) {

            if (Pfn1->u3.e2.ReferenceCount == 0) {
                if (Pfn1->u1.Flink == 0) {

                    //
                    // Set the PTE address to the physical page for
                    // virtual address alignment checking.
                    //

                    Pfn1->PteAddress =
                               (PMMPTE)(NextPhysicalPage << PTE_SHIFT);
                    MiInsertPageInList (MmPageLocationList[FreePageList],
                                        NextPhysicalPage);
                }
            } else {

                if (NextPhysicalPage != 0) {
                    //
                    // Remove PTE and insert into the free list.  If it is
                    // a physical address within the PFN database, the PTE
                    // element does not exist and therefore cannot be updated.
                    //

                    if (!MI_IS_PHYSICAL_ADDRESS (
                            MiGetVirtualAddressMappedByPte (Pfn1->PteAddress))) {

                        //
                        // Not a physical address.
                        //

                        *(Pfn1->PteAddress) = ZeroPte;
                    }

                    MI_SET_PFN_DELETED (Pfn1);
                    MiDecrementShareCountOnly (NextPhysicalPage);
                }
            }

            Pfn1++;
            i -= 1;
            NextPhysicalPage += 1;
        }

        BlockNumber -= 1;
    }

    //
    // If the kernel has been biased to allow for 3gb of user address space,
    // then the first 16mb of memory is doubly mapped to KSEG0_BASE and to
    // ALTERNATE_BASE. Therefore, the KSEG0_BASE entries must be unmapped.
    //

#if defined(_X86_)

    if (MmVirtualBias != 0) {
        Pde = MiGetPdeAddress(KSEG0_BASE);
        MI_WRITE_INVALID_PTE (Pde, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (Pde + 1, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (Pde + 2, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (Pde + 3, ZeroKernelPte);

#if defined(_X86PAE_)
        MI_WRITE_INVALID_PTE (Pde + 4, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (Pde + 5, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (Pde + 6, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (Pde + 7, ZeroKernelPte);
#endif

    }

#endif

    KeFlushEntireTb (TRUE, TRUE);
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiBuildPagedPool (
    VOID
    )

/*++

Routine Description:

    This function is called to build the structures required for paged
    pool and initialize the pool.  Once this routine is called, paged
    pool may be allocated.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel Mode Only.  System initialization.

--*/

{
    SIZE_T Size;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPpeEnd;
    MMPTE TempPte;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    ULONG i;

#if !defined (_WIN64)

    //
    // Double map system page directory page.
    //

#if defined (_X86PAE_)

    PointerPte = MiGetPteAddress(PDE_BASE);

    for (i = 0 ; i < PD_PER_SYSTEM; i += 1) {
        MmSystemPageDirectory[i] = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT(MmSystemPageDirectory[i]);
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        PointerPte += 1;
    }

    //
    // Was not mapped physically, map it virtually in system space.
    //

    PointerPte = MiReserveSystemPtes (
                                PD_PER_SYSTEM,
                                SystemPteSpace,
                                MM_COLOR_ALIGNMENT,
                                ((ULONG_PTR)PDE_BASE & MM_COLOR_MASK_VIRTUAL),
                                TRUE);

    MmSystemPagePtes = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte);

    TempPte = ValidKernelPde;

    for (i = 0 ; i < PD_PER_SYSTEM; i += 1) {
        TempPte.u.Hard.PageFrameNumber = MmSystemPageDirectory[i];
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        PointerPte += 1;
    }

#else

    MmSystemPageDirectory = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PDE_BASE));

    Pfn1 = MI_PFN_ELEMENT(MmSystemPageDirectory);

    Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    MmSystemPagePtes = (PMMPTE)MiMapPageInHyperSpace (MmSystemPageDirectory,
                                                      &OldIrql);
    MiUnmapPageInHyperSpace (OldIrql);

    if (!MI_IS_PHYSICAL_ADDRESS(MmSystemPagePtes)) {

        //
        // Was not mapped physically, map it virtually in system space.
        //

        PointerPte = MiReserveSystemPtes (
                                    1,
                                    SystemPteSpace,
                                    MM_COLOR_ALIGNMENT,
                                    ((ULONG_PTR)PDE_BASE & MM_COLOR_MASK_VIRTUAL),
                                    TRUE);
        TempPte = ValidKernelPde;
        TempPte.u.Hard.PageFrameNumber = MmSystemPageDirectory;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        MmSystemPagePtes = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte);
    }
#endif

#endif

    if (MmPagedPoolMaximumDesired == TRUE) {
        MmSizeOfPagedPoolInBytes =
                    ((PCHAR)MmNonPagedSystemStart - (PCHAR)MmPagedPoolStart);
    }

    //
    // A size of 0 means size the pool based on physical memory.
    //

    if (MmSizeOfPagedPoolInBytes == 0) {
        MmSizeOfPagedPoolInBytes = 2 * MmMaximumNonPagedPoolInBytes;
    }

    if (MmIsThisAnNtAsSystem()) {
        if ((MmNumberOfPhysicalPages > ((24*1024*1024) >> PAGE_SHIFT)) &&
            (MmSizeOfPagedPoolInBytes < MM_MINIMUM_PAGED_POOL_NTAS)) {

            MmSizeOfPagedPoolInBytes = MM_MINIMUM_PAGED_POOL_NTAS;
        }
    }

    if (MmSizeOfPagedPoolInBytes >
              (ULONG_PTR)((PCHAR)MmNonPagedSystemStart - (PCHAR)MmPagedPoolStart)) {
        MmSizeOfPagedPoolInBytes =
                    ((PCHAR)MmNonPagedSystemStart - (PCHAR)MmPagedPoolStart);
    }

    Size = BYTES_TO_PAGES(MmSizeOfPagedPoolInBytes);

    if (Size < MM_MIN_INITIAL_PAGED_POOL) {
        Size = MM_MIN_INITIAL_PAGED_POOL;
    }

    if (Size > (MM_MAX_PAGED_POOL >> PAGE_SHIFT)) {
        Size = MM_MAX_PAGED_POOL >> PAGE_SHIFT;
    }

    Size = (Size + (PTE_PER_PAGE - 1)) / PTE_PER_PAGE;
    MmSizeOfPagedPoolInBytes = (ULONG_PTR)Size * PAGE_SIZE * PTE_PER_PAGE;

    ASSERT ((MmSizeOfPagedPoolInBytes + (PCHAR)MmPagedPoolStart) <=
            (PCHAR)MmNonPagedSystemStart);

    //
    // Set size to the number of pages in the pool.
    //

    Size = Size * PTE_PER_PAGE;

    MmPagedPoolEnd = (PVOID)(((PUCHAR)MmPagedPoolStart +
                            MmSizeOfPagedPoolInBytes) - 1);

    MmPageAlignedPoolBase[PagedPool] = MmPagedPoolStart;

    //
    // Build page table page for paged pool.
    //

    PointerPde = MiGetPdeAddress (MmPagedPoolStart);
    MmPagedPoolBasePde = PointerPde;

    TempPte = ValidKernelPde;

#if defined (_WIN64)

    //
    // Map in all the page directory pages to span all of paged pool.
    // This removes the need for a system lookup directory.
    //

    PointerPpe = MiGetPpeAddress (MmPagedPoolStart);
    PointerPpeEnd = MiGetPpeAddress (MmPagedPoolEnd);

    while (PointerPpe <= PointerPpeEnd) {

        if (PointerPpe->u.Hard.Valid == 0) {
            PageFrameIndex = MiRemoveAnyPage(
                                     MI_GET_PAGE_COLOR_FROM_PTE (PointerPpe));
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPpe = TempPte;

            Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (
                                     MiGetPteAddress(PDE_KTBASE));
            Pfn1->PteAddress = PointerPpe;
            Pfn1->u2.ShareCount = 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        }

        PointerPpe += 1;
    }

#endif

    PointerPte = MiGetPteAddress (MmPagedPoolStart);
    MmPagedPoolInfo.FirstPteForPagedPool = PointerPte;
    MmPagedPoolInfo.LastPteForPagedPool = MiGetPteAddress (MmPagedPoolEnd);

    MiFillMemoryPte (PointerPde,
                     sizeof(MMPTE) *
                         (1 + MiGetPdeAddress (MmPagedPoolEnd) - PointerPde),
                     MM_KERNEL_NOACCESS_PTE);

    LOCK_PFN (OldIrql);

    //
    // Map in a page table page.
    //

    PageFrameIndex = MiRemoveAnyPage(
                            MI_GET_PAGE_COLOR_FROM_PTE (PointerPde));
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    MI_WRITE_VALID_PTE (PointerPde, TempPte);

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
#if defined (_WIN64)
    Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(MiGetPpeAddress (MmPagedPoolStart));
#else
#if !defined (_X86PAE_)
    Pfn1->PteFrame = MmSystemPageDirectory;
#else
    Pfn1->PteFrame = MmSystemPageDirectory[(PointerPde - MiGetPdeAddress(0)) / PDE_PER_PAGE];
#endif
#endif
    Pfn1->PteAddress = PointerPde;
    Pfn1->u2.ShareCount = 1;
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
    MiFillMemoryPte (PointerPte, PAGE_SIZE, MM_KERNEL_NOACCESS_PTE);

    UNLOCK_PFN (OldIrql);

    MmPagedPoolInfo.NextPdeForPagedPoolExpansion = PointerPde + 1;

    //
    // Build bitmaps for paged pool.
    //

    MiCreateBitMap (&MmPagedPoolInfo.PagedPoolAllocationMap, Size, NonPagedPool);
    RtlSetAllBits (MmPagedPoolInfo.PagedPoolAllocationMap);

    //
    // Indicate first page worth of PTEs are available.
    //

    RtlClearBits (MmPagedPoolInfo.PagedPoolAllocationMap, 0, PTE_PER_PAGE);

    MiCreateBitMap (&MmPagedPoolInfo.EndOfPagedPoolBitmap, Size, NonPagedPool);
    RtlClearAllBits (MmPagedPoolInfo.EndOfPagedPoolBitmap);

    //
    // If verifier is present then build the verifier paged pool bitmap.
    //

    if (MmVerifyDriverBufferLength != (ULONG)-1) {
        MiCreateBitMap (&VerifierLargePagedPoolMap, Size, NonPagedPool);
        RtlClearAllBits (VerifierLargePagedPoolMap);
    }

    //
    // Initialize paged pool.
    //

    InitializePool (PagedPool, 0L);

    MiInitializeSpecialPool();

    //
    // Allow mapping of views into system space.
    //

    MiInitializeSystemSpaceMap ((PVOID)0);

    return;
}


VOID
MiFindInitializationCode (
    OUT PVOID *StartVa,
    OUT PVOID *EndVa
    )

/*++

Routine Description:

    This function locates the start and end of the kernel initialization
    code.  This code resides in the "init" section of the kernel image.

Arguments:

    StartVa - Returns the starting address of the init section.

    EndVa - Returns the ending address of the init section.

Return Value:

    None.

Environment:

    Kernel Mode Only.  End of system initialization.

--*/

{
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PVOID CurrentBase;
    PVOID InitStart;
    PVOID InitEnd;
    PLIST_ENTRY Next;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    PIMAGE_SECTION_HEADER LastDiscard;
    LONG i;
    PVOID MiFindInitializationCodeAddress;

    MiFindInitializationCodeAddress = MmGetProcedureAddress((PVOID)&MiFindInitializationCode);

    *StartVa = NULL;

    //
    // Walk through the loader blocks looking for the base which
    // contains this routine.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);
    Next = PsLoadedModuleList.Flink;

    while ( Next != &PsLoadedModuleList ) {
        LdrDataTableEntry = CONTAINING_RECORD( Next,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks
                                             );
        if (LdrDataTableEntry->SectionPointer != NULL) {

            //
            // This entry was loaded by MmLoadSystemImage so it's already
            // had its init section removed.
            //

            Next = Next->Flink;
            continue;
        }

        CurrentBase = (PVOID)LdrDataTableEntry->DllBase;
        NtHeader = RtlImageNtHeader(CurrentBase);

        SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                                sizeof(ULONG) +
                                sizeof(IMAGE_FILE_HEADER) +
                                NtHeader->FileHeader.SizeOfOptionalHeader);

        //
        // From the image header, locate the section named 'INIT'.
        //

        i = NtHeader->FileHeader.NumberOfSections;

        InitStart = NULL;
        while (i > 0) {

#if DBG
            if ((*(PULONG)SectionTableEntry->Name == 'tini') ||
                (*(PULONG)SectionTableEntry->Name == 'egap')) {
                DbgPrint("driver %wZ has lower case sections (init or pagexxx)\n",
                    &LdrDataTableEntry->FullDllName);
            }
#endif //DBG

            //
            // Free any INIT sections (or relocation sections that haven't
            // been already).  Note a driver may have a relocation section
            // but not have any INIT code.
            //

            if ((*(PULONG)SectionTableEntry->Name == 'TINI') ||
                ((SectionTableEntry->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0)) {


                InitStart = (PVOID)((PCHAR)CurrentBase + SectionTableEntry->VirtualAddress);
                InitEnd = (PVOID)((PCHAR)InitStart + SectionTableEntry->SizeOfRawData - 1);
                InitEnd = (PVOID)((PCHAR)PAGE_ALIGN ((PCHAR)InitEnd +
                        (NtHeader->OptionalHeader.SectionAlignment - 1)) - 1);
                InitStart = (PVOID)ROUND_TO_PAGES (InitStart);

                //
                // Check if more sections are discardable after this one so
                // even small INIT sections can be discarded.
                //

                if (i == 1) {
                    LastDiscard = SectionTableEntry;
                }
                else {
                    LastDiscard = NULL;
                    do {
                        i -= 1;
                        SectionTableEntry += 1;
    
                        if ((SectionTableEntry->Characteristics &
                             IMAGE_SCN_MEM_DISCARDABLE) != 0) {
    
                            //
                            // Discard this too.
                            //
    
                            LastDiscard = SectionTableEntry;
                        } else {
                            break;
                        }
                    } while (i > 1);
                }

                if (LastDiscard) {
                    InitEnd = (PVOID)(((PCHAR)CurrentBase +
                                       LastDiscard->VirtualAddress) +
                                      (LastDiscard->SizeOfRawData - 1));

                    //
                    // If this isn't the last section in the driver then the
                    // the next section is not discardable.  So the last
                    // section is not rounded down, but all others must be.
                    //

                    if (i != 1) {
                        InitEnd = (PVOID)((PCHAR)PAGE_ALIGN ((PCHAR)InitEnd +
                                                             (NtHeader->OptionalHeader.SectionAlignment - 1)) - 1);
                    }
                }

                if (InitEnd > (PVOID)((PCHAR)CurrentBase +
                                      LdrDataTableEntry->SizeOfImage)) {
                    InitEnd = (PVOID)(((ULONG_PTR)CurrentBase +
                                       (LdrDataTableEntry->SizeOfImage - 1)) |
                                      (PAGE_SIZE - 1));
                }

                if (InitStart <= InitEnd) {
                    if ((MiFindInitializationCodeAddress >= InitStart) &&
                        (MiFindInitializationCodeAddress <= InitEnd)) {

                        //
                        // This init section is in the kernel, don't free it
                        // now as it would free this code!
                        //

                        *StartVa = InitStart;
                        *EndVa = InitEnd;
                    }
                    else {
                        MiFreeInitializationCode (InitStart, InitEnd);
                    }
                }
            }
            i -= 1;
            SectionTableEntry += 1;
        }
        Next = Next->Flink;
    }
    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();
    return;
}

VOID
MiFreeInitializationCode (
    IN PVOID StartVa,
    IN PVOID EndVa
    )

/*++

Routine Description:

    This function is called to delete the initialization code.

Arguments:

    StartVa - Supplies the starting address of the range to delete.

    EndVa - Supplies the ending address of the range to delete.

Return Value:

    None.

Environment:

    Kernel Mode Only.  Runs after system initialization.

--*/

{
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    PVOID UnlockHandle;

    UnlockHandle = MmLockPagableCodeSection((PVOID)MiFreeInitializationCode);
    ASSERT(UnlockHandle);

    if (MI_IS_PHYSICAL_ADDRESS(StartVa)) {
        LOCK_PFN (OldIrql);
        while (StartVa < EndVa) {

            //
            // On certain architectures (e.g., MIPS) virtual addresses
            // may be physical and hence have no corresponding PTE.
            //

            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (StartVa);

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            Pfn1->u2.ShareCount = 0;
            Pfn1->u3.e2.ReferenceCount = 0;
            MI_SET_PFN_DELETED (Pfn1);
            MiInsertPageInList (MmPageLocationList[FreePageList], PageFrameIndex);
            StartVa = (PVOID)((PUCHAR)StartVa + PAGE_SIZE);
        }
        UNLOCK_PFN (OldIrql);
    } else {
        PointerPte = MiGetPteAddress (StartVa);
        MiDeleteSystemPagableVm (PointerPte,
                                 (PFN_NUMBER) (1 + MiGetPteAddress (EndVa) -
                                     PointerPte),
                                 ZeroKernelPte,
                                 FALSE,
                                 NULL);
    }
    MmUnlockPagableImageSection(UnlockHandle);
    return;
}


VOID
MiEnablePagingTheExecutive (
    VOID
    )

/*++

Routine Description:

    This function locates the start and end of the kernel initialization
    code.  This code resides in the "init" section of the kernel image.

Arguments:

    StartVa - Returns the starting address of the init section.

    EndVa - Returns the ending address of the init section.

Return Value:

    None.

Environment:

    Kernel Mode Only.  End of system initialization.

--*/

{
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PVOID CurrentBase;
    PLIST_ENTRY Next;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    LONG i;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    BOOLEAN PageSection;
    PVOID SectionBaseAddress;

    //
    // Don't page kernel mode code if customer does not want it paged or if
    // this is a diskless remote boot client.
    //

    if (MmDisablePagingExecutive
#if defined(REMOTE_BOOT)
        || (IoRemoteBootClient && IoCscInitializationFailed)
#endif
        ) {
        return;
    }

    //
    // Walk through the loader blocks looking for the base which
    // contains this routine.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);
    Next = PsLoadedModuleList.Flink;
    while ( Next != &PsLoadedModuleList ) {
        LdrDataTableEntry = CONTAINING_RECORD( Next,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks
                                             );
        if (LdrDataTableEntry->SectionPointer != NULL) {

            //
            // This entry was loaded by MmLoadSystemImage so it's already paged.
            //

            Next = Next->Flink;
            continue;
        }

        CurrentBase = (PVOID)LdrDataTableEntry->DllBase;
        NtHeader = RtlImageNtHeader(CurrentBase);

        SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                                sizeof(ULONG) +
                                sizeof(IMAGE_FILE_HEADER) +
                                NtHeader->FileHeader.SizeOfOptionalHeader);

        //
        // From the image header, locate the section named 'PAGE' or
        // '.edata'.
        //

        i = NtHeader->FileHeader.NumberOfSections;

        PointerPte = NULL;

        while (i > 0) {

            if (MI_IS_PHYSICAL_ADDRESS (CurrentBase)) {

                //
                // Mapped physically, can't be paged.
                //

                break;
            }

            SectionBaseAddress = SECTION_BASE_ADDRESS(SectionTableEntry);

            PageSection = ((*(PULONG)SectionTableEntry->Name == 'EGAP') ||
                          (*(PULONG)SectionTableEntry->Name == 'ade.')) &&
                           (SectionBaseAddress !=
                            ((PUCHAR)CurrentBase + SectionTableEntry->VirtualAddress));

            if (*(PULONG)SectionTableEntry->Name == 'EGAP' &&
                SectionTableEntry->Name[4] == 'K'  &&
                SectionTableEntry->Name[5] == 'D') {

                //
                // Only pageout PAGEKD if KdPitchDebugger is TRUE.
                //

                PageSection = KdPitchDebugger;
            }

            if ((*(PULONG)SectionTableEntry->Name == 'EGAP') &&
                (*(PULONG)&SectionTableEntry->Name[4] == 'RDYH')) {

                //
                // Pageout PAGEHYDRA on non Hydra systems.
                //

                if (MiHydra == TRUE) {
                    PageSection = FALSE;
                }
            }

            if ((*(PULONG)SectionTableEntry->Name == 'EGAP') &&
                (*(PULONG)&SectionTableEntry->Name[4] == 'YFRV')) {

                //
                // Pageout PAGEVRFY if no drivers are being instrumented.
                //

                if (MmVerifyDriverBufferLength != (ULONG)-1) {
                    PageSection = FALSE;
                }
            }

            if ((*(PULONG)SectionTableEntry->Name == 'EGAP') &&
                (*(PULONG)&SectionTableEntry->Name[4] == 'CEPS')) {

                //
                // Pageout PAGESPEC special pool code if it's not enabled.
                //

                if (MiSpecialPoolPtes != 0) {
                    PageSection = FALSE;
                }
            }

            if (PageSection) {
                 //
                 // This section is pagable, save away the start and end.
                 //

                 if (PointerPte == NULL) {

                     //
                     // Previous section was NOT pagable, get the start address.
                     //

                     PointerPte = MiGetPteAddress (ROUND_TO_PAGES (
                                  (ULONG_PTR)CurrentBase +
                                  SectionTableEntry->VirtualAddress));
                 }
                 LastPte = MiGetPteAddress ((ULONG_PTR)CurrentBase +
                             SectionTableEntry->VirtualAddress +
                             (NtHeader->OptionalHeader.SectionAlignment - 1) +
                             SectionTableEntry->SizeOfRawData -
                             PAGE_SIZE);

            } else {

                //
                // This section is not pagable, if the previous section was
                // pagable, enable it.
                //

                if (PointerPte != NULL) {
                    MiEnablePagingOfDriverAtInit (PointerPte, LastPte);
                    PointerPte = NULL;
                }
            }
            i -= 1;
            SectionTableEntry += 1;
        } //end while

        if (PointerPte != NULL) {
            MiEnablePagingOfDriverAtInit (PointerPte, LastPte);
        }

        Next = Next->Flink;
    } //end while

    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    return;
}


VOID
MiEnablePagingOfDriverAtInit (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte
    )

/*++

Routine Description:

    This routine marks the specified range of PTEs as pagable.

Arguments:

    PointerPte - Supplies the starting PTE.

    LastPte - Supplies the ending PTE.

Return Value:

    None.

--*/

{
    PVOID Base;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn;
    MMPTE TempPte;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;
    LOGICAL SessionAddress;

    Base = MiGetVirtualAddressMappedByPte (PointerPte);
    SessionAddress = MI_IS_SESSION_PTE (PointerPte);

    LOCK_SYSTEM_WS (OldIrqlWs);
    LOCK_PFN (OldIrql);

    while (PointerPte <= LastPte) {

        //
        // The PTE must be carefully checked as drivers may call MmPageEntire
        // during their DriverEntry yet faults may occur prior to this routine
        // running which cause pages to already be resident and in the working
        // set at this point.  So checks for validity and wsindex must be
        // applied.
        //

        if (PointerPte->u.Hard.Valid == 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn->u2.ShareCount == 1);
    
            if (Pfn->u1.WsIndex == 0) {

                //
                // Set the working set index to zero.  This allows page table
                // pages to be brought back in with the proper WSINDEX.
                //
    
                MI_ZERO_WSINDEX (Pfn);

                //
                // Original PTE may need to be set for drivers loaded via
                // ntldr.
                //

                if (Pfn->OriginalPte.u.Long == 0) {
                    Pfn->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
#if defined(_IA64_)
                    Pfn->OriginalPte.u.Soft.Protection |= MM_EXECUTE;
#endif
                }

                Pfn->u3.e1.Modified = 1;
                TempPte = *PointerPte;
        
                MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                              Pfn->OriginalPte.u.Soft.Protection);
        
                KeFlushSingleTb (Base,
                                 TRUE,
                                 TRUE,
                                 (PHARDWARE_PTE)PointerPte,
                                 TempPte.u.Flush);
        
                //
                // Flush the translation buffer and decrement the number of valid
                // PTEs within the containing page table page.  Note that for a
                // private page, the page table page is still needed because the
                // page is in transition.
                //
        
                MiDecrementShareCount (PageFrameIndex);
    
                MmResidentAvailablePages += 1;
                MmTotalSystemCodePages += 1;
            }
            else {

                //
                // This would need to be taken out of the WSLEs so skip it for
                // now and let the normal paging algorithms remove it if we
                // run into memory pressure.
                //
            }

        }
        Base = (PVOID)((PCHAR)Base + PAGE_SIZE);
        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);
    UNLOCK_SYSTEM_WS (OldIrqlWs);

    if (SessionAddress == TRUE) {

        //
        // Session space has no ASN - flush the entire TB.
        //
    
        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    return;
}


MM_SYSTEMSIZE
MmQuerySystemSize(
    VOID
    )
{
    //
    // 12Mb  is small
    // 12-19 is medium
    // > 19 is large
    //
    return MmSystemSize;
}

NTKERNELAPI
BOOLEAN
MmIsThisAnNtAsSystem(
    VOID
    )
{
    return (BOOLEAN)MmProductType;
}

NTKERNELAPI
VOID
FASTCALL
MmSetPageFaultNotifyRoutine(
    PPAGE_FAULT_NOTIFY_ROUTINE NotifyRoutine
    )
{
    MmPageFaultNotifyRoutine = NotifyRoutine;
}

NTKERNELAPI
VOID
FASTCALL
MmSetHardFaultNotifyRoutine(
    PHARD_FAULT_NOTIFY_ROUTINE NotifyRoutine
    )
{
    MmHardFaultNotifyRoutine = NotifyRoutine;
}
