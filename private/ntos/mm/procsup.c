/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   procsup.c

Abstract:

    This module contains routines which support the process structure.

Author:

    Lou Perazzoli (loup) 25-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/


#include "mi.h"

#if defined (_WIN64)

#include "wow64t.h"

#if !defined(_IA64_)
#define MM_PROCESS_COMMIT_CHARGE 4

#define MM_PROCESS_CREATE_CHARGE 6
#else
#define MM_PROCESS_COMMIT_CHARGE 5

#define MM_PROCESS_CREATE_CHARGE 7
#endif

#else

#if !defined (_X86PAE_)
#define MM_PROCESS_COMMIT_CHARGE 3
#define MM_PROCESS_CREATE_CHARGE 5
#else
#define MM_PROCESS_COMMIT_CHARGE 7
#define MM_PROCESS_CREATE_CHARGE 9
#define MM_HIGHEST_PAE_PAGE      0xFFFFF

#define PAES_PER_PAGE  (PAGE_SIZE / sizeof(PAE_ENTRY))

#define MINIMUM_PAE_THRESHOLD  (PAES_PER_PAGE * 4)
#define EXCESS_PAE_THRESHOLD   (PAES_PER_PAGE * 8)

PAE_ENTRY MiFirstFreePae;
ULONG MiFreePaes;

//
// Turn off U/S, R/W and any other appropriate bits required by the processor.
//

#define MM_PAE_PDPTE_MASK         0x1e6

ULONG
MiPaeAllocate (
    PPAE_ENTRY *
    );

PVOID
MiPaeFree (
    PPAE_ENTRY Pae
    );

VOID
MiPaeFreeEntirePage (
    PVOID VirtualAddress
    );

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

#endif

#endif

#define HEADER_FILE

extern ULONG MmProductType;

extern ULONG MmWorkingSetReductionMax;

extern MM_SYSTEMSIZE MmSystemSize;

extern PVOID BBTBuffer;

SIZE_T MmProcessCommit;

ULONG MmKernelStackPages;
PFN_NUMBER MmKernelStackResident;
ULONG MmLargeStacks;
ULONG MmSmallStacks;

MMPTE KernelDemandZeroPte = {MM_KERNEL_DEMAND_ZERO_PTE};

CCHAR MmRotatingUniprocessorNumber;

extern ULONG MiFaultRetries;

ULONG
MiGetSystemPteListCount (
    IN ULONG ListSize
    );

PFN_NUMBER
MiMakeOutswappedPageResident (
    IN PMMPTE ActualPteAddress,
    IN PMMPTE PointerTempPte,
    IN ULONG Global,
    IN PFN_NUMBER ContainingPage
    );

PVOID
MiCreatePebOrTeb (
    IN PEPROCESS TargetProcess,
    IN ULONG Size
    );

VOID
MiDeleteAddressesInWorkingSet (
    IN PEPROCESS Process
    );

VOID
MiDeleteValidAddress (
    IN PVOID Va,
    IN PEPROCESS CurrentProcess
    );

VOID
MiDeleteFreeVm (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    );

VOID
VadTreeWalk (
    IN PMMVAD Start
    );

PMMVAD
MiAllocateVad(
    IN ULONG_PTR StartingVirtualAddress,
    IN ULONG_PTR EndingVirtualAddress,
    IN LOGICAL Deletable
    );

PVOID
MiPaeReplenishList (
    VOID
    );

extern LOGICAL MiNoLowMemory;

PVOID
MiAllocateLowMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PVOID CallingAddress,
    IN ULONG Tag
    );

LOGICAL
MiFreeLowMemory (
    IN PVOID BaseAddress,
    IN ULONG Tag
    );

#ifdef ALLOC_PRAGMA
#if defined (_X86PAE_)
#pragma alloc_text(INIT,MiPaeInitialize)
#endif
#pragma alloc_text(PAGE,MmCreateTeb)
#pragma alloc_text(PAGE,MmCreatePeb)
#pragma alloc_text(PAGE,MiCreatePebOrTeb)
#pragma alloc_text(PAGE,MmDeleteTeb)
#endif


BOOLEAN
MmCreateProcessAddressSpace (
    IN ULONG MinimumWorkingSetSize,
    IN PEPROCESS NewProcess,
    OUT PULONG_PTR DirectoryTableBase
    )

/*++

Routine Description:

    This routine creates an address space which maps the system
    portion and contains a hyper space entry.

Arguments:

    MinimumWorkingSetSize - Supplies the minimum working set size for
                            this address space.  This value is only used
                            to ensure that ample physical pages exist
                            to create this process.

    NewProcess - Supplies a pointer to the process object being created.

    DirectoryTableBase - Returns the value of the newly created
                         address space's Page Directory (PD) page and
                         hyper space page.

Return Value:

    Returns TRUE if an address space was successfully created, FALSE
    if ample physical pages do not exist.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PFN_NUMBER HyperDirectoryIndex;
    PFN_NUMBER PageDirectoryIndex;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PFN_NUMBER HyperSpaceIndex;
    PFN_NUMBER PageContainingWorkingSet;
    MMPTE TempPte;
    PMMPTE LastPte;
    PMMPTE PointerFillPte;
    PMMPTE CurrentAddressSpacePde;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    ULONG Color;
#if defined (_X86PAE_)
    ULONG TopQuad;
    MMPTE TopPte;
    PPAE_ENTRY PaeVa;
    PFN_NUMBER PageDirectoryIndex2;
    KIRQL OldIrql2;
    ULONG i;
    PFN_NUMBER HyperSpaceIndex2;
    PVOID PoolBlock;
#endif
#if defined(_IA64_)
    PFN_NUMBER SessionParentIndex;
#endif

    //
    // Get the PFN LOCK to prevent another thread in this
    // process from using hyper space and to get physical pages.
    //

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Charge commitment for the page directory pages, working set page table
    // page, and working set list.
    //

    if (MiChargeCommitment (MM_PROCESS_COMMIT_CHARGE, NULL) == FALSE) {
        return FALSE;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_PROCESS_CREATE, MM_PROCESS_COMMIT_CHARGE);

    NewProcess->NextPageColor = (USHORT)(RtlRandom(&MmProcessColorSeed));
    KeInitializeSpinLock (&NewProcess->HyperSpaceLock);

#if defined (_X86PAE_)
    TopQuad = MiPaeAllocate (&PaeVa);
    if (TopQuad == 0) {
        MiReturnCommitment (MM_PROCESS_COMMIT_CHARGE);
        return FALSE;
    }

    //
    // This page must be in the first 4GB of RAM.
    //

    ASSERT ((TopQuad >> PAGE_SHIFT) <= MM_HIGHEST_PAE_PAGE);
#endif

    LOCK_WS (CurrentProcess);

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if (MmResidentAvailablePages <= (SPFN_NUMBER)MinimumWorkingSetSize) {

#if defined (_X86PAE_)
        PoolBlock = MiPaeFree (PaeVa);
#endif

        UNLOCK_PFN (OldIrql);
        UNLOCK_WS (CurrentProcess);
        MiReturnCommitment (MM_PROCESS_COMMIT_CHARGE);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PROCESS_CREATE_FAILURE1, MM_PROCESS_COMMIT_CHARGE);

#if defined (_X86PAE_)
        if (PoolBlock != NULL) {
            MiPaeFreeEntirePage (PoolBlock);
        }
#endif
        //
        // Indicate no directory base was allocated.
        //

        return FALSE;
    }

    MmResidentAvailablePages -= MinimumWorkingSetSize;
    MM_BUMP_COUNTER(6, MinimumWorkingSetSize);
    MmProcessCommit += MM_PROCESS_COMMIT_CHARGE;

    NewProcess->AddressSpaceInitialized = 1;
    NewProcess->Vm.MinimumWorkingSetSize = MinimumWorkingSetSize;

    //
    // Allocate a page directory (parent for 64-bit systems) page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color =  MI_PAGE_COLOR_PTE_PROCESS (PDE_BASE,
                                        &CurrentProcess->NextPageColor);

    PageDirectoryIndex = MiRemoveZeroPageIfAny (Color);
    if (PageDirectoryIndex == 0) {
        PageDirectoryIndex = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageDirectoryIndex, Color);
        LOCK_PFN (OldIrql);
    }

#if defined (_X86PAE_)
    TempPte = ValidPdePde;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {

        MiEnsureAvailablePageOrWait (CurrentProcess, NULL);
    
        Color =  MI_PAGE_COLOR_PTE_PROCESS (PDE_BASE,
                                            &CurrentProcess->NextPageColor);
    
        PageDirectoryIndex2 = MiRemoveZeroPageIfAny (Color);
        if (PageDirectoryIndex2 == 0) {
            PageDirectoryIndex2 = MiRemoveAnyPage (Color);
            UNLOCK_PFN (OldIrql);
            MiZeroPhysicalPage (PageDirectoryIndex2, Color);
            LOCK_PFN (OldIrql);
        }

        //
        // Recursively map each page directory page so it points to itself.
        //

        TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex2;
        PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageDirectoryIndex,
                                                    &OldIrql2);
        PointerPte[i] = TempPte;
        MiUnmapPageInHyperSpace (OldIrql2);
        TopPte.u.Long = TempPte.u.Long & ~MM_PAE_PDPTE_MASK;
        PaeVa->PteEntry[i].u.Long = TopPte.u.Long;
    }

    //
    // Recursively map the topmost page directory page so it points to itself.
    //

    TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex;
    PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageDirectoryIndex, &OldIrql2);
    PointerPte[PD_PER_SYSTEM - 1] = TempPte;
    MiUnmapPageInHyperSpace (OldIrql2);
    TopPte.u.Long = TempPte.u.Long & ~MM_PAE_PDPTE_MASK;
    PaeVa->PteEntry[PD_PER_SYSTEM - 1].u.Long = TopPte.u.Long;
    NewProcess->PaePageDirectoryPage = PageDirectoryIndex;
    NewProcess->PaeTop = (PVOID)PaeVa;
    DirectoryTableBase[0] = TopQuad;
#else
    INITIALIZE_DIRECTORY_TABLE_BASE(&DirectoryTableBase[0], PageDirectoryIndex);
#endif

#if defined (_WIN64)

    PointerPpe = KSEG_ADDRESS (PageDirectoryIndex);
    TempPte = ValidPdePde;

    //
    // Map the top level page directory parent page recursively onto itself.
    //

    TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex;

#if defined (_AXP64_)
    ASSERT (TempPte.u.Hard.Global == 0);
    PointerPpe[MiGetPpeOffset(PDE_TBASE)] = TempPte;
#endif

#if defined(_IA64_)

    //
    // For IA64, the self-mapped entry is forced to be the last entry of
    // PPE table.
    //

    PointerPpe[(PDE_SELFMAP &
                ((sizeof(MMPTE)*PTE_PER_PAGE) - 1))/sizeof(MMPTE)] = TempPte;

#endif

    //
    // Allocate the page directory for hyper space and map this directory
    // page into the page directory parent page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPpeAddress(HYPER_SPACE),
                                       &CurrentProcess->NextPageColor);

    HyperDirectoryIndex = MiRemoveZeroPageIfAny (Color);
    if (HyperDirectoryIndex == 0) {
        HyperDirectoryIndex = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (HyperDirectoryIndex, Color);
        LOCK_PFN (OldIrql);
    }

    TempPte.u.Hard.PageFrameNumber = HyperDirectoryIndex;
    PointerPpe[MiGetPpeOffset(HYPER_SPACE)] = TempPte;

#if defined (_IA64_)

    //
    // Allocate the page directory parent for the session space
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPpeAddress(SESSION_SPACE_DEFAULT),
                                       &CurrentProcess->NextPageColor);

    SessionParentIndex = MiRemoveZeroPageIfAny (Color);
    if (SessionParentIndex == 0) {
        SessionParentIndex = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (SessionParentIndex, Color);
        LOCK_PFN (OldIrql);
    }

    INITIALIZE_DIRECTORY_TABLE_BASE(&NewProcess->Pcb.SessionParentBase, SessionParentIndex);

    PointerPpe = KSEG_ADDRESS (SessionParentIndex);

    TempPte.u.Hard.PageFrameNumber = SessionParentIndex;

    PointerPpe[(PDE_SSELFMAP &
                ((sizeof(MMPTE)*PTE_PER_PAGE) - 1))/sizeof(MMPTE)] = TempPte;

#endif // _IA64_

#endif

    //
    // Allocate the hyper space page table page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPdeAddress(HYPER_SPACE),
                                       &CurrentProcess->NextPageColor);

    HyperSpaceIndex = MiRemoveZeroPageIfAny (Color);
    if (HyperSpaceIndex == 0) {
        HyperSpaceIndex = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (HyperSpaceIndex, Color);
        LOCK_PFN (OldIrql);
    }

#if defined (_WIN64)
    PointerPde = KSEG_ADDRESS (HyperDirectoryIndex);
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex;
    PointerPde[MiGetPdeOffset(HYPER_SPACE)] = TempPte;
#endif

#if defined (_X86PAE_)

    //
    // Allocate the second hyper space page table page.
    // Save it in the first PTE used by the first hyperspace PDE.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPdeAddress(HYPER_SPACE2),
                                       &CurrentProcess->NextPageColor);

    HyperSpaceIndex2 = MiRemoveZeroPageIfAny (Color);
    if (HyperSpaceIndex2 == 0) {
        HyperSpaceIndex2 = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (HyperSpaceIndex2, Color);
        LOCK_PFN (OldIrql);
    }

    //
    // Unlike DirectoryTableBase[0], the HyperSpaceIndex is stored as an
    // absolute PFN and does not need to be below 4GB.
    //

    DirectoryTableBase[1] = HyperSpaceIndex;
#else
    INITIALIZE_DIRECTORY_TABLE_BASE(&DirectoryTableBase[1], HyperSpaceIndex);
#endif

    //
    // Remove page for the working set list.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_VA_PROCESS (MmWorkingSetList,
                                      &CurrentProcess->NextPageColor);

    PageContainingWorkingSet = MiRemoveZeroPageIfAny (Color);
    if (PageContainingWorkingSet == 0) {
        PageContainingWorkingSet = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageContainingWorkingSet, Color);
        LOCK_PFN (OldIrql);
    }

    //
    // Release the PFN mutex as the needed pages have been allocated.
    //

    UNLOCK_PFN (OldIrql);

    NewProcess->WorkingSetPage = PageContainingWorkingSet;

    //
    // Initialize the page reserved for hyper space.
    //

    MI_INITIALIZE_HYPERSPACE_MAP (HyperSpaceIndex);

    //
    // Set the PTE address in the PFN for the top level page directory page.
    //

#if defined (_WIN64)

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryIndex);

    ASSERT (Pfn1->u3.e1.PageColor == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->PteAddress = MiGetPteAddress(PDE_TBASE);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    //
    // Set the PTE address in the PFN for the hyper space page directory page.
    //

    Pfn1 = MI_PFN_ELEMENT (HyperDirectoryIndex);

    ASSERT (Pfn1->u3.e1.PageColor == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->PteAddress = MiGetPpeAddress(HYPER_SPACE);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

#if defined (_AXP64_)

    //
    // All of the system mappings are global.
    //

    MI_SET_GLOBAL_STATE (TempPte, 1);

    PointerFillPte = &PointerPpe[MiGetPpeOffset(MM_SYSTEM_SPACE_START)];
    CurrentAddressSpacePde = MiGetPpeAddress(MM_SYSTEM_SPACE_START);
    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPpeAddress(MM_SYSTEM_SPACE_END) -
                      MiGetPpeAddress(MM_SYSTEM_SPACE_START))) * sizeof(MMPTE)));
    //
    // Session space and win32k.sys are local on Hydra configurations.
    // However, as an optimization, it can be made global on non-Hydra.
    //

    if (MiHydra == TRUE) {
        MI_SET_GLOBAL_STATE (TempPte, 0);
    }

    PointerFillPte = &PointerPpe[MiGetPpeOffset(MM_SESSION_SPACE_DEFAULT)];
    CurrentAddressSpacePde = MiGetPpeAddress(MM_SESSION_SPACE_DEFAULT);
    MI_WRITE_VALID_PTE (PointerFillPte, *CurrentAddressSpacePde);

#endif

#if defined(_IA64_)
    if ((MiHydra == TRUE) && (CurrentProcess->Vm.u.Flags.ProcessInSession != 0)) {
        PointerPpe = KSEG_ADDRESS(SessionParentIndex);
        PointerFillPte = &PointerPpe[MiGetPpeOffset(MM_SESSION_SPACE_DEFAULT)];
        CurrentAddressSpacePde = MiGetPpeAddress(MM_SESSION_SPACE_DEFAULT);
        MI_WRITE_VALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
    }
#endif

#else // the following is for !WIN64 only

#if defined (_X86PAE_)

    //
    // Stash the second hyperspace PDE in the first PTE for the initial
    // hyperspace entry.
    //

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex2;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (HyperSpaceIndex, &OldIrql2);
    PointerPte[0] = TempPte;
    MiUnmapPageInHyperSpace (OldIrql2);

#endif

    //
    // Set the PTE address in the PFN for the page directory page.
    //

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryIndex);

    ASSERT (Pfn1->u3.e1.PageColor == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->PteAddress = (PMMPTE)PDE_BASE;

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    //
    // Map the page directory page in hyperspace.
    // Note for PAE, this is the high 1GB virtual only.
    //

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageDirectoryIndex, &OldIrql);
    PointerPte[MiGetPdeOffset(HYPER_SPACE)] = TempPte;

#if defined (_X86PAE_)

    //
    // Map in the second hyperspace page directory.
    // The page directory page is already recursively mapped.
    //

    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex2;
    PointerPte[MiGetPdeOffset(HYPER_SPACE2)] = TempPte;

#else

    //
    // Recursively map the page directory page so it points to itself.
    //

    TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex;
    PointerPte[MiGetPdeOffset(PTE_BASE)] = TempPte;

#endif

    //
    // Map in the non paged portion of the system.
    //

#if defined(_ALPHA_)

    PointerFillPte = &PointerPte[MiGetPdeOffset(MM_SYSTEM_SPACE_START)];
    CurrentAddressSpacePde = MiGetPdeAddress(MM_SYSTEM_SPACE_START);
    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPdeAddress(MM_SYSTEM_SPACE_END) -
                      MiGetPdeAddress(MM_SYSTEM_SPACE_START))) * sizeof(MMPTE)));

    //
    // KSEG0 is identity-mapped on the Alpha.  Copy the PDEs for this region.
    //

    PointerFillPte = &PointerPte[MiGetPdeOffset(MM_KSEG0_BASE)];
    CurrentAddressSpacePde = MiGetPdeAddress(MM_KSEG0_BASE);
    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   MiGetPdeOffset(KSEG2_BASE-KSEG0_BASE) * sizeof(MMPTE));

#else // the following is for x86 only

    //
    // If the system has not been loaded at a biased address, then system PDEs
    // exist in the 2gb->3gb range which must be copied.
    //

#if defined (_X86PAE_)

    //
    // For the PAE case, only the last page directory is currently mapped, so
    // only copy the system PDEs for the last 1GB - any that need copying in
    // the 2gb->3gb range will be done a little later.
    //

    if (MmVirtualBias != 0) {
        PointerFillPte = &PointerPte[MiGetPdeOffset(CODE_START + MmVirtualBias)];
        CurrentAddressSpacePde = MiGetPdeAddress(CODE_START + MmVirtualBias);
    
        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       (((1 + CODE_END) - CODE_START) / MM_VA_MAPPED_BY_PDE) * sizeof(MMPTE));
    }
#else
    PointerFillPte = &PointerPte[MiGetPdeOffset(CODE_START + MmVirtualBias)];
    CurrentAddressSpacePde = MiGetPdeAddress(CODE_START + MmVirtualBias);

    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   (((1 + CODE_END) - CODE_START) / MM_VA_MAPPED_BY_PDE) * sizeof(MMPTE));
#endif

    LastPte = &PointerPte[MiGetPdeOffset(NON_PAGED_SYSTEM_END)];
    PointerFillPte = &PointerPte[MiGetPdeOffset(MmNonPagedSystemStart)];
    CurrentAddressSpacePde = MiGetPdeAddress(MmNonPagedSystemStart);

    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPdeAddress(NON_PAGED_SYSTEM_END) -
                      CurrentAddressSpacePde))) * sizeof(MMPTE));

    //
    // Map in the system cache page table pages.
    //

    LastPte = &PointerPte[MiGetPdeOffset(MmSystemCacheEnd)];
    PointerFillPte = &PointerPte[MiGetPdeOffset(MM_SYSTEM_CACHE_WORKING_SET)];
    CurrentAddressSpacePde = MiGetPdeAddress(MM_SYSTEM_CACHE_WORKING_SET);

    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPdeAddress(MmSystemCacheEnd) -
                      CurrentAddressSpacePde))) * sizeof(MMPTE));

#if !defined (_X86PAE_)
    //
    // Map in any additional system cache page table pages.
    //

    if (MiSystemCacheEndExtra != MmSystemCacheEnd) {
        LastPte = &PointerPte[MiGetPdeOffset(MiSystemCacheEndExtra)];
        PointerFillPte = &PointerPte[MiGetPdeOffset(MiSystemCacheStartExtra)];
        CurrentAddressSpacePde = MiGetPdeAddress(MiSystemCacheStartExtra);

        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       ((1 + (MiGetPdeAddress(MiSystemCacheEndExtra) -
                          CurrentAddressSpacePde))) * sizeof(MMPTE));
    }
#endif

#endif      // end of x86 specific else

#if !defined (_X86PAE_)
    if (MiHydra == TRUE) {

        //
        // Copy the bootstrap entry for session space.
        // The rest is faulted in as needed.
        //

        PointerFillPte = &PointerPte[MiGetPdeOffset(MmSessionSpace)];
        CurrentAddressSpacePde = MiGetPdeAddress(MmSessionSpace);
        if (CurrentAddressSpacePde->u.Hard.Valid == 1) {
            MI_WRITE_VALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
        }
        else {
            MI_WRITE_INVALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
        }
    }
#endif

#if defined(_X86_)

    //
    // Map in the additional system PTE range if present.
    //

#if !defined (_X86PAE_)
    if (MiNumberOfExtraSystemPdes) {

        PointerFillPte = &PointerPte[MiGetPdeOffset(KSTACK_POOL_START)];
        CurrentAddressSpacePde = MiGetPdeAddress(KSTACK_POOL_START);

        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       MiNumberOfExtraSystemPdes * sizeof(MMPTE));
    }
#endif
#endif

    MiUnmapPageInHyperSpace (OldIrql);

#if defined (_X86PAE_)

    //
    // Map all the virtual space in the 2GB->3GB range when it's not user space.
    //

    if (MmVirtualBias == 0) {

        PageDirectoryIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PaeVa->PteEntry[PD_PER_SYSTEM - 2]);
    
        PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageDirectoryIndex, &OldIrql);
    
        PointerFillPte = &PointerPte[MiGetPdeOffset(CODE_START)];
        CurrentAddressSpacePde = MiGetPdeAddress(CODE_START);
    
        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       (((1 + CODE_END) - CODE_START) / MM_VA_MAPPED_BY_PDE) * sizeof(MMPTE));
    
        if (MiSystemCacheEndExtra != MmSystemCacheEnd) {
            LastPte = &PointerPte[MiGetPdeOffset(MiSystemCacheEndExtra)];
            PointerFillPte = &PointerPte[MiGetPdeOffset(MiSystemCacheStartExtra)];
            CurrentAddressSpacePde = MiGetPdeAddress(MiSystemCacheStartExtra);
    
            RtlCopyMemory (PointerFillPte,
                           CurrentAddressSpacePde,
                           ((1 + (MiGetPdeAddress(MiSystemCacheEndExtra) -
                              CurrentAddressSpacePde))) * sizeof(MMPTE));
        }
    
        if (MiHydra == TRUE) {
    
            //
            // Copy the bootstrap entry for session space.
            // The rest is faulted in as needed.
            //
    
            PointerFillPte = &PointerPte[MiGetPdeOffset(MmSessionSpace)];
            CurrentAddressSpacePde = MiGetPdeAddress(MmSessionSpace);
            if (CurrentAddressSpacePde->u.Hard.Valid == 1) {
                MI_WRITE_VALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
            }
            else {
                MI_WRITE_INVALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
            }
        }
    
        if (MiNumberOfExtraSystemPdes) {
    
            PointerFillPte = &PointerPte[MiGetPdeOffset(KSTACK_POOL_START)];
            CurrentAddressSpacePde = MiGetPdeAddress(KSTACK_POOL_START);
    
            RtlCopyMemory (PointerFillPte,
                           CurrentAddressSpacePde,
                           MiNumberOfExtraSystemPdes * sizeof(MMPTE));
        }
        MiUnmapPageInHyperSpace (OldIrql);
    }
#endif

#endif  // end of !WIN64 specific else

    //
    // Up the session space reference count.
    //

    if (MiHydra == TRUE) {
        MiSessionAddProcess (NewProcess);
    }

    //
    // Release working set mutex and lower IRQL.
    //

    UNLOCK_WS (CurrentProcess);

    return TRUE;
}

NTSTATUS
MmInitializeProcessAddressSpace (
    IN PEPROCESS ProcessToInitialize,
    IN PEPROCESS ProcessToClone OPTIONAL,
    IN PVOID SectionToMap OPTIONAL,
    OUT PUNICODE_STRING * AuditName OPTIONAL
    )

/*++

Routine Description:

    This routine initializes the working set and mutexes within an
    newly created address space to support paging.

    No page faults may occur in a new process until this routine is
    completed.

Arguments:

    ProcessToInitialize - Supplies a pointer to the process to initialize.

    ProcessToClone - Optionally supplies a pointer to the process whose
                     address space should be copied into the
                     ProcessToInitialize address space.

    SectionToMap - Optionally supplies a section to map into the newly
                   initialized address space.

        Only one of ProcessToClone and SectionToMap may be specified.


Return Value:

    None.


Environment:

    Kernel mode.  APCs Disabled.

--*/


{
    PMMPTE PointerPte;
    MMPTE TempPte;
    PVOID BaseAddress;
    SIZE_T ViewSize;
    KIRQL OldIrql;
    NTSTATUS Status;
    PFN_NUMBER PpePhysicalPage;
    PFN_NUMBER PdePhysicalPage;
    PFN_NUMBER PageContainingWorkingSet;
    LARGE_INTEGER SectionOffset;
    PSECTION_IMAGE_INFORMATION ImageInfo;
    PMMVAD VadShare;
    PMMVAD VadReserve;
    PLOCK_HEADER LockedPagesHeader;
#if defined (_X86PAE_)
    ULONG i;
    PFN_NUMBER PdePhysicalPage2;
#endif
#if defined (_WIN64)
    PWOW64_PROCESS Wow64Process;
#endif

    //
    // Initialize Working Set Mutex in process header.
    //

    KeAttachProcess (&ProcessToInitialize->Pcb);
    ProcessToInitialize->AddressSpaceInitialized = 2;

    ExInitializeFastMutex(&ProcessToInitialize->AddressCreationLock);

    ExInitializeFastMutex(&ProcessToInitialize->WorkingSetLock);

    //
    // NOTE:  The process block has been zeroed when allocated, so
    // there is no need to zero fields and set pointers to NULL.
    //

    ASSERT (ProcessToInitialize->VadRoot == NULL);

    KeQuerySystemTime(&ProcessToInitialize->Vm.LastTrimTime);
    ProcessToInitialize->Vm.VmWorkingSetList = MmWorkingSetList;

    //
    // Obtain a page to map the working set and initialize the
    // working set.  Get PFN mutex to allocate physical pages.
    //

    LOCK_PFN (OldIrql);

    //
    // Initialize the PFN database for the Page Directory and the
    // PDE which maps hyper space.
    //

#if defined (_WIN64)

    PointerPte = MiGetPteAddress (PDE_TBASE);
    PpePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    MiInitializePfn (PpePhysicalPage, PointerPte, 1);

    PointerPte = MiGetPpeAddress (HYPER_SPACE);
    MiInitializePfn (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte), PointerPte, 1);

#if defined(_IA64_)
    PointerPte = MiGetPteAddress (PDE_STBASE);
    MiInitializePfn (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte), PointerPte, 1);
#endif

#else

#if defined (_X86PAE_)
    PointerPte = MiGetPdeAddress (PDE_BASE);
#else
    PointerPte = MiGetPteAddress (PDE_BASE);
#endif
    PdePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    MiInitializePfn (PdePhysicalPage, PointerPte, 1);

#endif

    PointerPte = MiGetPdeAddress (HYPER_SPACE);
    MiInitializePfn (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte), PointerPte, 1);

#if defined (_X86PAE_)

    for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
        PointerPte = MiGetPteAddress (PDE_BASE + (i << PAGE_SHIFT));
        PdePhysicalPage2 = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        MiInitializePfn (PdePhysicalPage2, PointerPte, 1);
    }

    PointerPte = MiGetPdeAddress (HYPER_SPACE2);
    MiInitializePfn (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte), PointerPte, 1);
#endif

    PageContainingWorkingSet = ProcessToInitialize->WorkingSetPage;

    PointerPte = MiGetPteAddress (MmWorkingSetList);
    PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    MiInitializePfn (PageContainingWorkingSet, PointerPte, 1);

    UNLOCK_PFN (OldIrql);

    MI_MAKE_VALID_PTE (TempPte,
                       PageContainingWorkingSet,
                       MM_READWRITE,
                       PointerPte );

    MI_SET_PTE_DIRTY (TempPte);
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    ASSERT (ProcessToInitialize->LockedPagesList == NULL);

    if (MmTrackLockedPages == TRUE) {
        LockedPagesHeader = ExAllocatePoolWithTag (NonPagedPool,
                                                   sizeof(LOCK_HEADER),
                                                   'xTmM');

        if (LockedPagesHeader) {
            RtlZeroMemory (LockedPagesHeader, sizeof(LOCK_HEADER));
            ProcessToInitialize->LockedPagesList = (PVOID)LockedPagesHeader;
            InitializeListHead (&LockedPagesHeader->ListHead);
        }
    }

    MiInitializeWorkingSetList (ProcessToInitialize);

    KeInitializeSpinLock (&ProcessToInitialize->AweLock);
    InitializeListHead (&ProcessToInitialize->PhysicalVadList);

    //
    // Page faults may be taken now.
    //
    // If the system has been biased to an alternate base address to allow
    // 3gb of user address space and a process is not being cloned, then
    // create a VAD for the shared memory page.
    //

#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)

    if ((MmVirtualBias != 0) && (ProcessToClone == NULL)) {

        //
        // Allocate a VAD to map the shared memory page. If a VAD cannot be
        // allocated, then detach from the target process and return a failure
        // status.  This VAD is marked as not deletable.
        //

        VadShare = MiAllocateVad (MM_SHARED_USER_DATA_VA,
                                  MM_SHARED_USER_DATA_VA,
                                  FALSE);

        if (VadShare == NULL) {
            KeDetachProcess ();
            return STATUS_NO_MEMORY;
        }

        //
        // If a section is being mapped and the executable is not large
        // address space aware, then create a VAD that reserves the address
        // space between 2gb and the highest user address.
        //

        if (SectionToMap != NULL) {
            if (!((PSECTION)SectionToMap)->u.Flags.Image) {
                KeDetachProcess ();
                ExFreePool (VadShare);
                return STATUS_SECTION_NOT_IMAGE;
            }
            ImageInfo = ((PSECTION)SectionToMap)->Segment->ImageInformation;
            if ((ExVerifySuite(Enterprise) == FALSE) ||
                ((ImageInfo->ImageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) == 0)) {

                //
                // Allocate a VAD to map the address space between 2gb and
                // the highest user address. If a VAD can not be allocated,
                // then deallocate the shared address space VAD, detach from
                // the target process, and return a failure status.
                // This VAD is marked as not deletable.
                //

                VadReserve = MiAllocateVad (_2gb,
                                            (ULONG_PTR)MM_HIGHEST_USER_ADDRESS,
                                            FALSE);

                if (VadReserve == NULL) {
                    KeDetachProcess ();
                    ExFreePool (VadShare);
                    return STATUS_NO_MEMORY;
                }

                //
                // Insert the VAD.
                //
                // N.B. No exception can occur since there is no commit charge.
                //

                MiInsertVad (VadReserve);
            }
        }

        //
        // Insert the VAD.
        //
        // N.B. No exception can occur since there is no commit charge.
        //

        MiInsertVad (VadShare);
    }

#endif

#if defined(_WIN64)

    if (ProcessToClone == NULL) {

        //
        // Reserve the address space just below KUSER_SHARED_DATA as the
        // compatibility area.  This range can be unreserved by user mode
        // code such as WOW64 or csrss.
        //

        ASSERT(MiCheckForConflictingVad(WOW64_COMPATIBILITY_AREA_ADDRESS, MM_SHARED_USER_DATA_VA) == NULL);

        VadShare = MiAllocateVad (WOW64_COMPATIBILITY_AREA_ADDRESS,
                                  MM_SHARED_USER_DATA_VA,
                                  TRUE);

    	if (VadShare == NULL) {
           KeDetachProcess ();
           return STATUS_NO_MEMORY;
    	}

        //
        // Reserve the memory above 2GB to prevent 32 bit (WOW64) process
        // access.
        //

    	if (SectionToMap != NULL) {
            if (!((PSECTION)SectionToMap)->u.Flags.Image) {
                KeDetachProcess ();
                ExFreePool (VadShare);
                return STATUS_SECTION_NOT_IMAGE;
            }
            ImageInfo = ((PSECTION)SectionToMap)->Segment->ImageInformation;

            if ((ImageInfo->ImageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) == 0 ||
#if defined(_AXP64_)
                ImageInfo->Machine == IMAGE_FILE_MACHINE_ALPHA ||
#endif
                ImageInfo->Machine == IMAGE_FILE_MACHINE_I386) {

                //
            	// Allocate a VAD to reserve the address space between 2gb and
            	// the highest user address.  If a VAD cannot be allocated,
            	// then deallocate the compatibility VAD, detach from the target
                // process and return a failure status.
            	//

                VadReserve = MiAllocateVad (_2gb,
                                            (ULONG_PTR)MM_HIGHEST_USER_ADDRESS,
                                            TRUE);

            	if (VadReserve == NULL) {
                    KeDetachProcess ();
                    ExFreePool (VadShare);
                    return STATUS_NO_MEMORY;
            	}

            	//
            	// Insert the VAD.
                //
                // N.B. No exception can occur since there is no commit charge.
            	//

                MiInsertVad (VadReserve);

                //
                // Initialize Wow64 Process structure
                //

                Wow64Process = 
                    (PWOW64_PROCESS) ExAllocatePoolWithTag (NonPagedPool,
                                                            sizeof(WOW64_PROCESS),
                                                            'WowM');

                if (Wow64Process == (PWOW64_PROCESS) NULL) {
                    KeDetachProcess ();
                    return STATUS_NO_MEMORY;
                }

                RtlZeroMemory(Wow64Process, sizeof(WOW64_PROCESS));

                ProcessToInitialize->Wow64Process = Wow64Process;
                 
#if defined(_MIALT4K_)

                //
                // Initialize the alternate page table for the 4kb page function
                //

                Status = MiInitializeAlternateTable (ProcessToInitialize);
                if (Status != STATUS_SUCCESS) {
                    KeDetachProcess ();
                    return Status;
                }

#endif
            }
        }

    	//
        // Insert the VAD.
    	//
        // N.B. No exception can occur since there is no commit charge.
    	//

    	MiInsertVad (VadShare);
    }

#endif

    if (SectionToMap != (PSECTION)NULL) {

        //
        // Map the specified section into the address space of the
        // process but only if it is an image section.
        //

        if (!((PSECTION)SectionToMap)->u.Flags.Image) {
            Status = STATUS_SECTION_NOT_IMAGE;
        } else {
            UNICODE_STRING UnicodeString;
            ULONG n;
            PWSTR Src;
            PCHAR Dst;

            UnicodeString = ((PSECTION)SectionToMap)->Segment->ControlArea->FilePointer->FileName;
            Src = (PWSTR)((PCHAR)UnicodeString.Buffer + UnicodeString.Length);
            n = 0;
            if (UnicodeString.Buffer != NULL) {
                while (Src > UnicodeString.Buffer) {
                    if (*--Src == OBJ_NAME_PATH_SEPARATOR) {
                        Src += 1;
                        break;
                    }
                    else {
                        n += 1;
                    }
                }
            }
            Dst = ProcessToInitialize->ImageFileName;
            if (n >= sizeof( ProcessToInitialize->ImageFileName )) {
                n = sizeof( ProcessToInitialize->ImageFileName ) - 1;
            }

            while (n--) {
                *Dst++ = (UCHAR)*Src++;
            }
            *Dst = '\0';

            if (AuditName) {
                *AuditName = &((PSECTION)SectionToMap)->Segment->ControlArea->FilePointer->FileName ;
            }

            ProcessToInitialize->SubSystemMajorVersion =
                (UCHAR)((PSECTION)SectionToMap)->Segment->ImageInformation->SubSystemMajorVersion;
            ProcessToInitialize->SubSystemMinorVersion =
                (UCHAR)((PSECTION)SectionToMap)->Segment->ImageInformation->SubSystemMinorVersion;

            BaseAddress = NULL;
            ViewSize = 0;
            ZERO_LARGE (SectionOffset);

            Status = MmMapViewOfSection ( (PSECTION)SectionToMap,
                                          ProcessToInitialize,
                                          &BaseAddress,
                                          0,                // ZeroBits,
                                          0,                // CommitSize,
                                          &SectionOffset,   //SectionOffset,
                                          &ViewSize,
                                          ViewShare,        //InheritDisposition,
                                          0,                //allocation type
                                          PAGE_READWRITE    // Protect
                                          );

            ProcessToInitialize->SectionBaseAddress = BaseAddress;

#if DBG
            if (MmDebug & MM_DBG_PTE_UPDATE) {
                DbgPrint("mapped image section vads\n");
                VadTreeWalk(ProcessToInitialize->VadRoot);
            }
#endif //DBG
        }

        KeDetachProcess ();
        return Status;
    }

    if (ProcessToClone != (PEPROCESS)NULL) {
#if DEVL
        strcpy( ProcessToInitialize->ImageFileName, ProcessToClone->ImageFileName );
#endif // DEVL

        //
        // Clone the address space of the specified process.
        //

        //
        // As the page directory and page tables are private to each
        // process, the physical pages which map the directory page
        // and the page table usage must be mapped into system space
        // so they can be updated while in the context of the process
        // we are cloning.
        //

        KeDetachProcess ();
        return MiCloneProcessAddressSpace (ProcessToClone,
                                           ProcessToInitialize,
#if defined (_WIN64)
                                           PpePhysicalPage,
#else
                                           PdePhysicalPage,
#endif
                                           PageContainingWorkingSet
                                           );

    }

    //
    // System Process.
    //

    KeDetachProcess ();
    return STATUS_SUCCESS;
}

VOID
MmDeleteProcessAddressSpace (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine deletes a process's Page Directory and working set page.

Arguments:

    Process - Supplies a pointer to the deleted process.

Return Value:

    None.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageFrameIndex2;
#if defined (_WIN64)
    PMMPTE PageDirectoryParent;
    PMMPTE Ppe;
#endif
#if defined (_X86PAE_)
    ULONG i;
    KIRQL OldIrql2;
    PMMPTE PointerPte;
    PVOID PoolBlock;
    PFN_NUMBER PageDirectories[PD_PER_SYSTEM];

    PoolBlock = NULL;
#endif

    //
    // Return commitment.
    //

    MiReturnCommitment (MM_PROCESS_COMMIT_CHARGE);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PROCESS_DELETE, MM_PROCESS_COMMIT_CHARGE);
    ASSERT (Process->CommitCharge == 0);

    //
    // Remove the working set list page from the deleted process.
    //

    Pfn1 = MI_PFN_ELEMENT (Process->WorkingSetPage);

    LOCK_PFN (OldIrql);
    MmProcessCommit -= MM_PROCESS_COMMIT_CHARGE;

    if (Process->AddressSpaceInitialized == 2) {

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MiDecrementShareCountOnly (Process->WorkingSetPage);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        //
        // Remove the hyper space page table page from the deleted process.
        //

#if defined (_X86PAE_)

        PageFrameIndex = (PFN_NUMBER)Process->Pcb.DirectoryTableBase[1];
        //
        // Remove the second hyper space page table page.
        //

        PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageFrameIndex, &OldIrql2);
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
        MiUnmapPageInHyperSpace (OldIrql2);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex2);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex2);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));
#else
        PageFrameIndex =
            MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(Process->Pcb.DirectoryTableBase[1])));
#endif

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex);
        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        //
        // Remove the page directory page.
        //

        PageFrameIndex = MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process);

#if defined (_X86PAE_)

        PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageFrameIndex, &OldIrql2);
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageDirectories[i] = MI_GET_PAGE_FRAME_FROM_PTE(&PointerPte[i]);
        }
        MiUnmapPageInHyperSpace (OldIrql2);

        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            Pfn1 = MI_PFN_ELEMENT (PageDirectories[i]);
    
            MI_SET_PFN_DELETED (Pfn1);
    
            MiDecrementShareAndValidCount (PageDirectories[i]);
            MiDecrementShareAndValidCount (Pfn1->PteFrame);
    
            ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));
        }
#endif

#if defined (_WIN64)

        //
        // Get a pointer to the top-level page directory parent page via
        // its KSEG0 address.
        //

        PageDirectoryParent = KSEG_ADDRESS (PageFrameIndex);

        //
        // Remove the hyper space page directory page from the deleted process.
        //

        Ppe = &PageDirectoryParent[MiGetPpeOffset(HYPER_SPACE)];
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(Ppe);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex2);

        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex2);
        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));
#endif

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (PageFrameIndex);

        MiDecrementShareCountOnly (PageFrameIndex);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

#if defined (_X86PAE_)

        //
        // Free the page directory page pointers.
        //

        PoolBlock = MiPaeFree ((PPAE_ENTRY)Process->PaeTop);
#endif

#if defined(_IA64_)

        //
        // Free the session space page directory parent page 
        //

        PageFrameIndex = 
            MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(Process->Pcb.SessionParentBase)));

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        
        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->PteFrame);

        MiDecrementShareCountOnly (PageFrameIndex);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

#endif

    } else {

        //
        // Process initialization never completed, just return the pages
        // to the free list.
        //

        MiInsertPageInList (MmPageLocationList[FreePageList],
                            Process->WorkingSetPage);

#if defined (_WIN64)

        //
        // Get a pointer to the top-level page directory parent page via
        // its KSEG0 address.
        //

        PageFrameIndex =
            MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(Process->Pcb.DirectoryTableBase[0])));

        PageDirectoryParent = KSEG_ADDRESS (PageFrameIndex);

        Ppe = &PageDirectoryParent[MiGetPpeOffset(HYPER_SPACE)];
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(Ppe);

        MiInsertPageInList (MmPageLocationList[FreePageList],
                            PageFrameIndex2);
#endif

#if defined (_X86PAE_)
        PageFrameIndex = MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process);

        PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageFrameIndex, &OldIrql2);
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageDirectories[i] = MI_GET_PAGE_FRAME_FROM_PTE(&PointerPte[i]);
        }
        MiUnmapPageInHyperSpace (OldIrql2);

        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            MiInsertPageInList (MmPageLocationList[FreePageList],
                                PageDirectories[i]);
        }

        //
        // Free the second hyper space page table page.
        //

        PageFrameIndex = (PFN_NUMBER)Process->Pcb.DirectoryTableBase[1];

        PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageFrameIndex, &OldIrql2);
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
        MiUnmapPageInHyperSpace (OldIrql2);
        MiInsertPageInList (MmPageLocationList[FreePageList], PageFrameIndex2);

        //
        // Free the first hyper space page table page.
        //

        MiInsertPageInList (MmPageLocationList[FreePageList],
                            (PFN_NUMBER)Process->Pcb.DirectoryTableBase[1]);

        MiInsertPageInList (MmPageLocationList[FreePageList],
            MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process));

        //
        // Free the page directory page pointers.
        //

        PoolBlock = MiPaeFree ((PPAE_ENTRY)Process->PaeTop);
#else

        MiInsertPageInList (MmPageLocationList[FreePageList],
            MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(Process->Pcb.DirectoryTableBase[1]))));

        MiInsertPageInList (MmPageLocationList[FreePageList],
            MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(Process->Pcb.DirectoryTableBase[0]))));
#endif
#if defined(_IA64_)
        MiInsertPageInList (MmPageLocationList[FreePageList], Process->Pcb.SessionParentBase);
#endif
    }

    MmResidentAvailablePages += MM_PROCESS_CREATE_CHARGE;
    MM_BUMP_COUNTER(7, MM_PROCESS_CREATE_CHARGE);

    UNLOCK_PFN (OldIrql);

#if defined (_X86PAE_)
    if (PoolBlock != NULL) {
        MiPaeFreeEntirePage (PoolBlock);
    }
#endif

    //
    // Check to see if the paging files should be contracted.
    //

    MiContractPagingFiles ();

    return;
}

VOID
MmCleanProcessAddressSpace (
    )

/*++

Routine Description:

    This routine cleans an address space by deleting all the
    user and pagable portion of the address space.  At the
    completion of this routine, no page faults may occur within
    the process.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PEPROCESS Process;
    PMMVAD Vad;
    KEVENT Event;
    KIRQL OldIrql;
#if defined(_ALPHA_) && !defined(_AXP64_)
    KIRQL OldIrql2;
#endif
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPFN Pfn1;
    PVOID TempVa;
    LONG AboveWsMin;
    MMPTE_FLUSH_LIST PteFlushList;

    PteFlushList.Count = 0;
    Process = PsGetCurrentProcess();
    if ((Process->AddressSpaceDeleted != 0) ||
        (Process->AddressSpaceInitialized == 0)) {

        //
        // This process's address space has already been deleted.  However,
        // this process can still have a session space.  Get rid of it now.
        //

        if (MiHydra == TRUE) {
            MiSessionRemoveProcess ();
        }

        return;
    }

    if (Process->AddressSpaceInitialized == 1) {

        //
        // The process has been created but not fully initialized.
        // Return partial resources now.
        //

        MmResidentAvailablePages += (Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE);

        MM_BUMP_COUNTER(41, Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE);

        //
        // This process's address space has already been deleted.  However,
        // this process can still have a session space.  Get rid of it now.
        //

        if (MiHydra == TRUE) {
            MiSessionRemoveProcess ();
        }

        return;
    }

    //
    // If working set expansion for this process is allowed, disable
    // it and remove the process from expanded process list if it
    // is on it.
    //

    LOCK_EXPANSION (OldIrql);

    if (Process->Vm.u.Flags.BeingTrimmed) {

        //
        // Initialize an event and put the event address
        // in the blink field.  When the trimming is complete,
        // this event will be set.
        //

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        Process->Vm.WorkingSetExpansionLinks.Blink = (PLIST_ENTRY)&Event;

        //
        // Release the mutex and wait for the event.
        //

        KeEnterCriticalRegion();
        UNLOCK_EXPANSION_AND_THEN_WAIT (OldIrql);

        KeWaitForSingleObject(&Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        KeLeaveCriticalRegion();
    }
    else if (Process->Vm.WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION) {

        //
        // No trimming is in progress and no expansion allowed, so this cannot
        // be on any lists.
        //

        ASSERT (Process->Vm.WorkingSetExpansionLinks.Blink != MM_WS_EXPANSION_IN_PROGRESS);

        UNLOCK_EXPANSION (OldIrql);
    } else {

        RemoveEntryList (&Process->Vm.WorkingSetExpansionLinks);

        //
        // Disable expansion.
        //

        Process->Vm.WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;

        //
        // Release the pfn mutex.
        //

        UNLOCK_EXPANSION (OldIrql);
    }

    if (MiHydra == TRUE) {
        MiSessionRemoveProcess ();
    }

    //
    // Delete all the user owned pagable virtual addresses in the process.
    //

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Synchronize address space delete with NtReadVirtualMemory and
    // NtWriteVirtualMemory.
    //

    MiLockSystemSpace(OldIrql);
    Process->AddressSpaceDeleted = 1;
    if ( Process->VmOperation != 0) {

        //
        // A Vm operation is in progress, set the event and
        // indicate this process is being deleted to stop other
        // Vm operations.
        //

        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Process->VmOperationEvent = &Event;

        do {

            MiUnlockSystemSpace(OldIrql);

            UNLOCK_WS_AND_ADDRESS_SPACE (Process);
            KeWaitForSingleObject(&Event,
                                  WrVirtualMemory,
                                  KernelMode,
                                  FALSE,
                                  (PLARGE_INTEGER)NULL);

            LOCK_WS_AND_ADDRESS_SPACE (Process);

            //
            // Synchronize address space delete with NtReadVirtualMemory and
            // NtWriteVirtualMemory.
            //

            MiLockSystemSpace(OldIrql);

        } while (Process->VmOperation != 0);

        MiUnlockSystemSpace(OldIrql);

    } else {
        MiUnlockSystemSpace(OldIrql);
    }

    //
    // Delete all the valid user mode addresses from the working set
    // list.  At this point NO page faults are allowed on user space
    // addresses.  Faults are allowed on page tables for user space, which
    // requires that we keep the working set structure consistent until we
    // finally take it all down.
    //

    MiDeleteAddressesInWorkingSet (Process);

    //
    // Remove hash table pages, if any.  This is the first time we do this
    // during the deletion path, but we need to do it again before we finish
    // because we may fault in some page tables during the VAD clearing.  We
    // could have maintained the hash table validity during the WorkingSet
    // deletion above in order to avoid freeing the hash table twice, but since
    // we're just deleting it all anyway, it's faster to do it this way.  Note
    // that if we don't do this or maintain the validity, we can trap later
    // in MiGrowWsleHash.
    //

    PointerPte = MiGetPteAddress (&MmWsle[MM_MAXIMUM_WORKING_SET]) + 1;
    LastPte = MiGetPteAddress (MmWorkingSetList->HighestPermittedHashAddress);

#if defined (_WIN64)
    PointerPpe = MiGetPdeAddress (PointerPte);
    PointerPde = MiGetPteAddress (PointerPte);

    if ((PointerPpe->u.Hard.Valid == 1) &&
        (PointerPde->u.Hard.Valid == 1) &&
        (PointerPte->u.Hard.Valid == 1)) {

        PteFlushList.Count = 0;
        LOCK_PFN (OldIrql);
        while (PointerPte->u.Hard.Valid) {
            TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
            MiDeletePte (PointerPte,
                         TempVa,
                         FALSE,
                         Process,
                         NULL,
                         &PteFlushList);

            PointerPte += 1;
            Process->NumberOfPrivatePages += 1;

            //
            // If all the entries have been removed from the previous page
            // table page, delete the page table page itself.  Likewise with
            // the page directory page.
            //

            if ((MiIsPteOnPdeBoundary(PointerPte)) ||
                ((MiGetPdeAddress(PointerPte))->u.Hard.Valid == 0) ||
                ((MiGetPteAddress(PointerPte))->u.Hard.Valid == 0) ||
                (PointerPte->u.Hard.Valid == 0)) {

                MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

                PointerPde = MiGetPteAddress (PointerPte - 1);

                ASSERT (PointerPde->u.Hard.Valid == 1);

                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

                if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                {
                    MiDeletePte (PointerPde,
                                 PointerPte - 1,
                                 FALSE,
                                 Process,
                                 NULL,
                                 NULL);
                    Process->NumberOfPrivatePages += 1;
                }

                if (MiIsPteOnPpeBoundary(PointerPte)) {

                    PointerPpe = MiGetPteAddress (PointerPde);

                    ASSERT (PointerPpe->u.Hard.Valid == 1);

                    Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe));

                    if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                    {
                        MiDeletePte (PointerPpe,
                                     PointerPde,
                                     FALSE,
                                     Process,
                                     NULL,
                                     NULL);
                        Process->NumberOfPrivatePages += 1;
                    }
                }
                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPdeAddress (PointerPte);
                if ((PointerPpe->u.Hard.Valid == 0) ||
                    (PointerPde->u.Hard.Valid == 0)) {
                        break;
                }
            }
        }
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);
        UNLOCK_PFN (OldIrql);
    }
#else
    if (PointerPte->u.Hard.Valid) {
        PteFlushList.Count = 0;
        LOCK_PFN (OldIrql);
        while ((PointerPte < LastPte) && (PointerPte->u.Hard.Valid)) {
            TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
            MiDeletePte (PointerPte,
                         TempVa,
                         FALSE,
                         Process,
                         NULL,
                         &PteFlushList);

            PointerPte += 1;
            Process->NumberOfPrivatePages += 1;
        }
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);
        UNLOCK_PFN (OldIrql);
    }
#endif

    //
    // Clear the hash fields as a fault may occur below on the page table
    // pages during VAD clearing and resolution of the fault may result in
    // adding a hash table.  Thus these fields must be consistent with the
    // clearing just done above.
    //

    MmWorkingSetList->HashTableSize = 0;
    MmWorkingSetList->HashTable = NULL;

    //
    // Delete the virtual address descriptors and dereference any
    // section objects.
    //

    Vad = Process->VadRoot;

    while (Vad != (PMMVAD)NULL) {

        MiRemoveVad (Vad);

        //
        // If the system has been biased to an alternate base address to
        // allow 3gb of user address space, then check if the current VAD
        // describes the shared memory page.
        //

#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)

        if (MmVirtualBias != 0) {

            //
            // If the VAD describes the shared memory page, then free the
            // VAD and continue with the next entry.
            //

            if (Vad->StartingVpn == MI_VA_TO_VPN (MM_SHARED_USER_DATA_VA)) {
                goto LoopEnd;
            }
        }
#endif

        if (((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != NULL)) ||
            (Vad->u.VadFlags.PhysicalMapping == 1)) {

            //
            // This VAD represents a mapped view or a driver-mapped physical
            // view - delete the view and perform any section related cleanup
            // operations.
            //

            MiRemoveMappedView (Process, Vad);

        } else {

            if (Vad->u.VadFlags.UserPhysicalPages == 1) {

                //
                // Free all the physical pages that this VAD might be mapping.
                // Since only the AWE lock synchronizes the remap API, carefully
                // remove this VAD from the list first.
                //

                MiPhysicalViewRemover (Process, Vad);

                MiRemoveUserPhysicalPagesVad ((PMMVAD_SHORT)Vad);
  
                MiDeletePageTablesForPhysicalRange (
                        MI_VPN_TO_VA (Vad->StartingVpn),
                        MI_VPN_TO_VA_ENDING (Vad->EndingVpn));
            }
            else {

                if (Vad->u.VadFlags.WriteWatch == 1) {
                    MiPhysicalViewRemover (Process, Vad);
                }

                LOCK_PFN (OldIrql);
    
                //
                // Don't specify address space deletion as TRUE as
                // the working set must be consistent as page faults may
                // be taken during clone removal, protoPTE lookup, etc.
                //
    
                MiDeleteVirtualAddresses (MI_VPN_TO_VA (Vad->StartingVpn),
                                          MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                          FALSE,
                                          Vad);
    
                UNLOCK_PFN (OldIrql);
            }
        }

#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)
LoopEnd:
#endif

        ExFreePool (Vad);
        Vad = Process->VadRoot;
    }

    ASSERT (IsListEmpty (&Process->PhysicalVadList) != 0);
    
    MiCleanPhysicalProcessPages (Process);

    //
    // Delete the shared data page, if any.
    //

    LOCK_PFN (OldIrql);

#if defined(MM_SHARED_USER_DATA_VA)
    MiDeleteVirtualAddresses ((PVOID) MM_SHARED_USER_DATA_VA,
                              (PVOID) MM_SHARED_USER_DATA_VA,
                              FALSE,
                              NULL);
#endif

    //
    // Delete the system portion of the address space.
    // Only now is it safe to specify TRUE to MiDelete because now that the
    // VADs have been deleted we can no longer fault on user space pages.
    //

#if defined(_ALPHA_) && !defined(_AXP64_)
    LOCK_EXPANSION_IF_ALPHA (OldIrql2);
#endif
    Process->Vm.AddressSpaceBeingDeleted = 1;
#if defined(_ALPHA_) && !defined(_AXP64_)
    UNLOCK_EXPANSION_IF_ALPHA (OldIrql2);
#endif

    //
    // Adjust the count of pages above working set maximum.  This
    // must be done here because the working set list is not
    // updated during this deletion.
    //

    AboveWsMin = (LONG)Process->Vm.WorkingSetSize - (LONG)Process->Vm.MinimumWorkingSetSize;
    if (AboveWsMin > 0) {
        MmPagesAboveWsMinimum -= AboveWsMin;
    }

    UNLOCK_PFN (OldIrql);

    //
    // Return commitment for page table pages.
    //

#ifdef _WIN64
    ASSERT (MmWorkingSetList->NumberOfCommittedPageTables == 0);
#else
    MiReturnCommitment (MmWorkingSetList->NumberOfCommittedPageTables);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PROCESS_CLEAN_PAGETABLES,
                     MmWorkingSetList->NumberOfCommittedPageTables);

    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
        PsChangeJobMemoryUsage(-(SSIZE_T)MmWorkingSetList->NumberOfCommittedPageTables);
    }
    Process->CommitCharge -= MmWorkingSetList->NumberOfCommittedPageTables;
#endif

    //
    // Check to make sure all the clone descriptors went away.
    //

    ASSERT (Process->CloneRoot == (PMMCLONE_DESCRIPTOR)NULL);

    if (Process->NumberOfLockedPages != 0) {
        if (Process->LockedPagesList) {

            PLIST_ENTRY NextEntry;
            PLOCK_TRACKER Tracker;
            PLOCK_HEADER LockedPagesHeader;

            LockedPagesHeader = (PLOCK_HEADER)Process->LockedPagesList;
            if ((LockedPagesHeader->Count != 0) && (MiTrackingAborted == FALSE)) {
                ASSERT (IsListEmpty (&LockedPagesHeader->ListHead) == 0);
                NextEntry = LockedPagesHeader->ListHead.Flink;

                Tracker = CONTAINING_RECORD (NextEntry,
                                             LOCK_TRACKER,
                                             ListEntry);
        
                KeBugCheckEx (DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS,
                              (ULONG_PTR)Tracker->CallingAddress,
                              (ULONG_PTR)Tracker->CallersCaller,
                              (ULONG_PTR)Tracker->Mdl,
                              Process->NumberOfLockedPages);
            }
        }

        KeBugCheckEx (PROCESS_HAS_LOCKED_PAGES,
                      0,
                      (ULONG_PTR)Process,
                      Process->NumberOfLockedPages,
                      (ULONG_PTR)Process->LockedPagesList);
        return;
    }

    if (Process->LockedPagesList) {
        ASSERT (MmTrackLockedPages == TRUE);
        ExFreePool (Process->LockedPagesList);
        Process->LockedPagesList = NULL;
    }

#if DBG
    if ((Process->NumberOfPrivatePages != 0) && (MmDebug & MM_DBG_PRIVATE_PAGES)) {
        DbgPrint("MM: Process contains private pages %ld\n",
               Process->NumberOfPrivatePages);
        DbgBreakPoint();
    }
#endif //DBG


#if defined(_WIN64)
    //
    // Delete the WowProcess structure
    //

    if (Process->Wow64Process != NULL) {
#if defined(_MIALT4K_)
        MiDeleteAlternateTable(Process);
#endif
        ExFreePool(Process->Wow64Process);
        Process->Wow64Process = NULL;
    }
#endif

    //
    // Remove the working set list pages (except for the first one).
    // These pages are not removed because DPCs could still occur within
    // the address space.  In a DPC, nonpagedpool could be allocated
    // which could require removing a page from the standby list, requiring
    // hyperspace to map the previous PTE.
    //

    PointerPte = MiGetPteAddress (MmWorkingSetList) + 1;

    PteFlushList.Count = 0;

    LOCK_PFN (OldIrql);
    while (PointerPte->u.Hard.Valid) {
        TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
        MiDeletePte (PointerPte,
                     TempVa,
                     TRUE,
                     Process,
                     NULL,
                     &PteFlushList);

        PointerPte += 1;
#if defined (_WIN64)
        //
        // If all the entries have been removed from the previous page
        // table page, delete the page table page itself.  Likewise with
        // the page directory page.
        //

        if ((MiIsPteOnPdeBoundary(PointerPte)) ||
            ((MiGetPdeAddress(PointerPte))->u.Hard.Valid == 0) ||
            ((MiGetPteAddress(PointerPte))->u.Hard.Valid == 0) ||
            (PointerPte->u.Hard.Valid == 0)) {

            MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

            PointerPde = MiGetPteAddress (PointerPte - 1);

            ASSERT (PointerPde->u.Hard.Valid == 1);

            Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

            if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
            {
                MiDeletePte (PointerPde,
                             PointerPte - 1,
                             TRUE,
                             Process,
                             NULL,
                             NULL);
            }

            if (MiIsPteOnPpeBoundary(PointerPte)) {

                PointerPpe = MiGetPteAddress (PointerPde);

                ASSERT (PointerPpe->u.Hard.Valid == 1);

                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe));

                if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                {
                    MiDeletePte (PointerPpe,
                                 PointerPde,
                                 TRUE,
                                 Process,
                                 NULL,
                                 NULL);
                }
            }
        }
#endif
    }

    //
    // Remove hash table pages, if any.  Yes, we've already done this once
    // during the deletion path, but we need to do it again because we may
    // have faulted in some page tables during the VAD clearing.
    //

    PointerPte = MiGetPteAddress (&MmWsle[MM_MAXIMUM_WORKING_SET]) + 1;

#if defined (_WIN64)
    PointerPpe = MiGetPdeAddress (PointerPte);
    PointerPde = MiGetPteAddress (PointerPte);

    if ((PointerPpe->u.Hard.Valid == 1) &&
        (PointerPde->u.Hard.Valid == 1) &&
        (PointerPte->u.Hard.Valid == 1)) {

        while (PointerPte->u.Hard.Valid) {
            TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
            MiDeletePte (PointerPte,
                         TempVa,
                         TRUE,
                         Process,
                         NULL,
                         &PteFlushList);

            PointerPte += 1;

            //
            // If all the entries have been removed from the previous page
            // table page, delete the page table page itself.  Likewise with
            // the page directory page.
            //

            if ((MiIsPteOnPdeBoundary(PointerPte)) ||
                ((MiGetPdeAddress(PointerPte))->u.Hard.Valid == 0) ||
                ((MiGetPteAddress(PointerPte))->u.Hard.Valid == 0) ||
                (PointerPte->u.Hard.Valid == 0)) {

                MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

                PointerPde = MiGetPteAddress (PointerPte - 1);

                ASSERT (PointerPde->u.Hard.Valid == 1);

                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

                if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                {
                    MiDeletePte (PointerPde,
                                 PointerPte - 1,
                                 TRUE,
                                 Process,
                                 NULL,
                                 NULL);
                }

                if (MiIsPteOnPpeBoundary(PointerPte)) {

                    PointerPpe = MiGetPteAddress (PointerPde);

                    ASSERT (PointerPpe->u.Hard.Valid == 1);

                    Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe));

                    if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                    {
                        MiDeletePte (PointerPpe,
                                     PointerPde,
                                     TRUE,
                                     Process,
                                     NULL,
                                     NULL);
                    }
                }
            }
        }
    }
#else
    while ((PointerPte < LastPte) && (PointerPte->u.Hard.Valid)) {
        TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
        MiDeletePte (PointerPte,
                     TempVa,
                     TRUE,
                     Process,
                     NULL,
                     &PteFlushList);

        PointerPte += 1;
    }
#endif

    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

    //
    // Update the count of available resident pages.
    //

    ASSERT (Process->Vm.MinimumWorkingSetSize >= MM_PROCESS_CREATE_CHARGE);
    MmResidentAvailablePages += Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE;
    MM_BUMP_COUNTER(8, Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE);
    ASSERT (Process->Vm.WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);
    UNLOCK_PFN (OldIrql);

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    return;
}


#if DBG
typedef struct _MMKSTACK {
    PMMPFN Pfn;
    PMMPTE Pte;
} MMKSTACK, *PMMKSTACK;
MMKSTACK MmKstacks[10];
#endif //DBG

PVOID
MmCreateKernelStack (
    IN BOOLEAN LargeStack
    )

/*++

Routine Description:

    This routine allocates a kernel stack and a no-access page within
    the non-pagable portion of the system address space.

Arguments:

    LargeStack - Supplies the value TRUE if a large stack should be
                 created.  FALSE if a small stack is to be created.

Return Value:

    Returns a pointer to the base of the kernel stack.  Note, that the
    base address points to the guard page, so space must be allocated
    on the stack before accessing the stack.

    If a kernel stack cannot be created, the value NULL is returned.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PMMPTE PointerPte;
    MMPTE TempPte;
    PFN_NUMBER NumberOfPages;
    ULONG NumberOfPtes;
    ULONG ChargedPtes;
    ULONG RequestedPtes;
#if defined(_IA64_)
    ULONG NumberOfBStorePtes;
#endif
    PFN_NUMBER PageFrameIndex;
    ULONG i;
    PVOID StackVa;
    KIRQL OldIrql;

    //
    // Acquire the PFN mutex to synchronize access to the dead stack
    // list and to the pfn database.
    //

    LOCK_PFN (OldIrql);

    //
    // Check to see if any "unused" stacks are available.
    //

    if ((!LargeStack) && (MmNumberDeadKernelStacks != 0)) {

#if DBG
        {
            ULONG i = MmNumberDeadKernelStacks;
            PMMPFN PfnList = MmFirstDeadKernelStack;

            while (i > 0) {
                i--;
                if ((PfnList != MmKstacks[i].Pfn) ||
                   (PfnList->PteAddress != MmKstacks[i].Pte))  {
                   DbgPrint("MMPROCSUP: kstacks %p %ld. %p\n",
                       PfnList, i, MmKstacks[i].Pfn);
                   DbgBreakPoint();
                }
                PfnList = PfnList->u1.NextStackPfn;
            }
        }
#if defined(_IA64_)
        NumberOfPages = BYTES_TO_PAGES (KERNEL_STACK_SIZE + KERNEL_BSTORE_SIZE);
#else
        NumberOfPages = BYTES_TO_PAGES (KERNEL_STACK_SIZE);
#endif
#endif //DBG

        MmNumberDeadKernelStacks -= 1;
        PointerPte = MmFirstDeadKernelStack->PteAddress;
        MmFirstDeadKernelStack = MmFirstDeadKernelStack->u1.NextStackPfn;

    } else {

        UNLOCK_PFN (OldIrql);

#if defined(_IA64_)
        if (LargeStack) {
            NumberOfPtes = BYTES_TO_PAGES (KERNEL_LARGE_STACK_SIZE);
            NumberOfBStorePtes = BYTES_TO_PAGES (KERNEL_LARGE_BSTORE_SIZE);
            NumberOfPages = BYTES_TO_PAGES (KERNEL_LARGE_STACK_COMMIT
                                            + KERNEL_LARGE_BSTORE_COMMIT);

        } else {
            NumberOfPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE);
            NumberOfBStorePtes = BYTES_TO_PAGES (KERNEL_BSTORE_SIZE);
            NumberOfPages = NumberOfPtes + NumberOfBStorePtes;
        }
        ChargedPtes = NumberOfPtes + NumberOfBStorePtes;
        RequestedPtes = ChargedPtes + 2 + (MM_STACK_ALIGNMENT ? 1 : 0);
#else
        if (LargeStack) {
            NumberOfPtes = BYTES_TO_PAGES (KERNEL_LARGE_STACK_SIZE);
            NumberOfPages = BYTES_TO_PAGES (KERNEL_LARGE_STACK_COMMIT);
        } else {
            NumberOfPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE);
            NumberOfPages = NumberOfPtes;
        }
        ChargedPtes = NumberOfPtes;
        RequestedPtes = ChargedPtes + 1 + (MM_STACK_ALIGNMENT ? 1 : 0);
#endif // _IA64_

        //
        // Make sure there are at least 8 of the appropriate system PTE pool.
        //

        if (MiGetSystemPteListCount (RequestedPtes) < 8) {
            return NULL;
        }

        //
        // Charge commitment for the page file space for the kernel stack.
        //

        if (MiChargeCommitment (ChargedPtes, NULL) == FALSE) {

            //
            // Commitment exceeded, return NULL, indicating no kernel
            // stacks are available.
            //

            return NULL;
        }
        MM_TRACK_COMMIT (MM_DBG_COMMIT_KERNEL_STACK_CREATE, ChargedPtes);

        LOCK_PFN (OldIrql);

        //
        // Check to make sure the physical pages are available.
        //

        if (MmResidentAvailablePages <= (SPFN_NUMBER)NumberOfPages) {
            UNLOCK_PFN (OldIrql);
            MiReturnCommitment (ChargedPtes);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_KERNEL_STACK_FAILURE1,
                             ChargedPtes);
            return NULL;
        }

        MmResidentAvailablePages -= NumberOfPages;
        MM_BUMP_COUNTER(9, NumberOfPages);

        UNLOCK_PFN (OldIrql);

        //
        // Obtain enough pages to contain the stack plus a guard page from
        // the system PTE pool.  The system PTE pool contains non-paged PTEs
        // which are currently empty.
        //

        MmKernelStackPages += RequestedPtes;

        PointerPte = MiReserveSystemPtes (RequestedPtes,
                                          SystemPteSpace,
                                          MM_STACK_ALIGNMENT,
                                          MM_STACK_OFFSET,
                                          FALSE);

        if (PointerPte == NULL) {

            LOCK_PFN (OldIrql);
            MmResidentAvailablePages += NumberOfPages;
            MM_BUMP_COUNTER(13, NumberOfPages);
            UNLOCK_PFN (OldIrql);

            MiReturnCommitment (ChargedPtes);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_KERNEL_STACK_FAILURE2,
                             ChargedPtes);
            return NULL;
        }

#if defined(_IA64_)

        //
        // StackVa is calculated here
        //

        StackVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte + NumberOfPtes + 1);

        //
        // The PTEs are divided between kernel stack and RSE space.
        //
        // The kernel stack grows downward and the RSE grows upward.
        //
        // For large stacks, one chunk is allocated in the middle of the PTE
        // range and split here.
        //
        // need better algorithm for RSE

        if (LargeStack) {
            PointerPte += BYTES_TO_PAGES (KERNEL_LARGE_STACK_SIZE - KERNEL_LARGE_STACK_COMMIT -1);
        }

#else
        PointerPte += (NumberOfPtes - NumberOfPages);
#endif // _IA64_

        LOCK_PFN (OldIrql);

        for (i = 0; i < NumberOfPages; i += 1) {
            PointerPte += 1;
            ASSERT (PointerPte->u.Hard.Valid == 0);
            MiEnsureAvailablePageOrWait (NULL, NULL);
            PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

            PointerPte->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

#ifdef PROTECT_KSTACKS
            PointerPte->u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
#endif

            MiInitializePfn (PageFrameIndex, PointerPte, 1);

            MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               MM_READWRITE,
                               PointerPte );
            MI_SET_PTE_DIRTY (TempPte);

            MI_WRITE_VALID_PTE (PointerPte, TempPte);
        }
        MmProcessCommit += ChargedPtes;
        MmKernelStackResident += NumberOfPages;
        MmLargeStacks += LargeStack;
        MmSmallStacks += !LargeStack;

#if defined(_IA64_)

        UNLOCK_PFN (OldIrql);

        return StackVa;
#endif

    }

    UNLOCK_PFN (OldIrql);

    PointerPte += 1;
    StackVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
#if !defined(_IA64_)
#if DBG
    {
        PULONG p;
        ULONG_PTR i;

        p = (PULONG)((ULONG_PTR)StackVa - ((ULONG_PTR)NumberOfPages * PAGE_SIZE));
        i = ((ULONG_PTR)NumberOfPages * PAGE_SIZE) >> 2;
        while(i--) {
            *p++ = 0x12345678;
        }

    }
#endif // DBG
#endif // _IA64_

    return StackVa;
}

VOID
MmDeleteKernelStack (
    IN PVOID PointerKernelStack,
    IN BOOLEAN LargeStack
    )

/*++

Routine Description:

    This routine deletes a kernel stack and the no-access page within
    the non-pagable portion of the system address space.

Arguments:

    PointerKernelStack - Supplies a pointer to the base of the kernel stack.

    LargeStack - Supplies the value TRUE if a large stack is being deleted.
                 FALSE if a small stack is to be deleted.

Return Value:

    None.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PFN_NUMBER NumberOfPages;
    ULONG NumberOfPtes;
    PFN_NUMBER PageFrameIndex;
    ULONG i;
    KIRQL OldIrql;
    MMPTE PteContents;

    if (LargeStack) {
#if defined(_IA64_)
        NumberOfPtes = BYTES_TO_PAGES (KERNEL_LARGE_STACK_SIZE + KERNEL_LARGE_BSTORE_SIZE);
#else
        NumberOfPtes = BYTES_TO_PAGES (KERNEL_LARGE_STACK_SIZE);
#endif
    } else {
#if defined(_IA64_)
        NumberOfPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE + KERNEL_BSTORE_SIZE);
#else
        NumberOfPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE);
#endif
    }

    PointerPte = MiGetPteAddress (PointerKernelStack);

    //
    // PointerPte points to the guard page, point to the previous
    // page before removing physical pages.
    //

    PointerPte -= 1;

    LOCK_PFN (OldIrql);

    //
    // Check to see if the stack page should be placed on the dead
    // kernel stack page list.  The dead kernel stack list is a
    // singly linked list of kernel stacks from terminated threads.
    // The stacks are saved on a linked list up to a maximum number
    // to avoid the overhead of flushing the entire TB on all processors
    // everytime a thread terminates.  The TB on all processors must
    // be flushed as kernel stacks reside in the non paged system part
    // of the address space.
    //

    if ((!LargeStack) &&
        (MmNumberDeadKernelStacks < MmMaximumDeadKernelStacks)) {

        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

#if DBG
        {
            ULONG i = MmNumberDeadKernelStacks;
            PMMPFN PfnList = MmFirstDeadKernelStack;

            while (i > 0) {
                i--;
                if ((PfnList != MmKstacks[i].Pfn) ||
                   (PfnList->PteAddress != MmKstacks[i].Pte))  {
                   DbgPrint("MMPROCSUP: kstacks %p %ld. %p\n",
                       PfnList, i, MmKstacks[i].Pfn);
                   DbgBreakPoint();
                }
                PfnList = PfnList->u1.NextStackPfn;
            }
            MmKstacks[MmNumberDeadKernelStacks].Pte = Pfn1->PteAddress;
            MmKstacks[MmNumberDeadKernelStacks].Pfn = Pfn1;
        }
#endif //DBG

        MmNumberDeadKernelStacks += 1;
        Pfn1->u1.NextStackPfn = MmFirstDeadKernelStack;
        MmFirstDeadKernelStack = Pfn1;

        PERFINFO_DELETE_STACK(PointerPte, NumberOfPtes);

        UNLOCK_PFN (OldIrql);

        return;
    }

#if defined(_IA64_)

    //
    // Since PointerKernelStack points to the center of the stack space,
    // the size of kernel backing store needs to be added to get the
    // top of the stack space.
    //

    PointerPte = MiGetPteAddress (LargeStack ?
                  (PCHAR)PointerKernelStack+KERNEL_LARGE_BSTORE_SIZE :
                  (PCHAR)PointerKernelStack+KERNEL_BSTORE_SIZE);

    //
    // PointerPte points to the guard page, point to the previous
    // page before removing physical pages.
    //

    PointerPte -= 1;

#endif

    //
    // We have exceeded the limit of dead kernel stacks or this is a large
    // stack, delete this kernel stack.
    //

    NumberOfPages = 0;
    for (i = 0; i < NumberOfPtes; i += 1) {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            MiDecrementShareAndValidCount (Pfn1->PteFrame);

            //
            // Set the pointer to PTE as empty so the page
            // is deleted when the reference count goes to zero.
            //

            MI_SET_PFN_DELETED (Pfn1);
            MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (&PteContents));
            NumberOfPages += 1;
        }
        PointerPte -= 1;
    }

#if defined(_IA64_)
    MmKernelStackPages  -= NumberOfPtes + 2 + (MM_STACK_ALIGNMENT?1:0);

    MiReleaseSystemPtes (PointerPte,
                         NumberOfPtes + 2 + (MM_STACK_ALIGNMENT?1:0),
                         SystemPteSpace);
#else
    MmKernelStackPages  -= NumberOfPtes + 1 + (MM_STACK_ALIGNMENT?1:0);

    MiReleaseSystemPtes (PointerPte,
                         NumberOfPtes + 1 + (MM_STACK_ALIGNMENT?1:0),
                         SystemPteSpace);
#endif

    //
    // Update the count of available resident pages.
    //

    MmKernelStackResident -= NumberOfPages;
    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(10, NumberOfPages);
    MmProcessCommit -= NumberOfPtes;

    MmLargeStacks -= LargeStack;
    MmSmallStacks -= !LargeStack;
    UNLOCK_PFN (OldIrql);

    //
    // Return commitment.
    //

    MiReturnCommitment (NumberOfPtes);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_KERNEL_STACK_DELETE, NumberOfPtes);

    return;
}


NTSTATUS
MmGrowKernelStack (
    IN PVOID CurrentStack
    )

/*++

Routine Description:

    This function attempts to grows the current thread's kernel stack
    such that there is always KERNEL_LARGE_STACK_COMMIT bytes below
    the current stack pointer.

Arguments:

    CurrentStack - Supplies a pointer to the current stack pointer.

Return Value:

    STATUS_SUCCESS is returned if the stack was grown.

    STATUS_STACK_OVERFLOW is returned if there was not enough space reserved
    for the commitment.

    STATUS_NO_MEMORY is returned if there was not enough physical memory
    in the system.

--*/

{
    PMMPTE NewLimit;
    PMMPTE StackLimit;
    PMMPTE EndStack;
    PETHREAD Thread;
    PFN_NUMBER NumberOfPages;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    MMPTE TempPte;

    Thread = PsGetCurrentThread ();
    ASSERT (((PCHAR)Thread->Tcb.StackBase - (PCHAR)Thread->Tcb.StackLimit) <=
            (KERNEL_LARGE_STACK_SIZE + PAGE_SIZE));
    NewLimit = MiGetPteAddress ((PVOID)((PUCHAR)CurrentStack -
                                                    KERNEL_LARGE_STACK_COMMIT));

    StackLimit = MiGetPteAddress (Thread->Tcb.StackLimit);

    //
    // If the new stack limit exceeds the reserved region for the kernel
    // stack, then return an error.
    //

    EndStack = MiGetPteAddress ((PVOID)((PUCHAR)Thread->Tcb.StackBase -
                                                    KERNEL_LARGE_STACK_SIZE));

    if (NewLimit < EndStack) {

        //
        // Don't go into guard page.
        //

        return STATUS_STACK_OVERFLOW;

    }

    ASSERT (StackLimit->u.Hard.Valid == 1);

    //
    // Lock the PFN database and attempt to expand the kernel stack.
    //

    StackLimit -= 1;

    NumberOfPages = (PFN_NUMBER) (StackLimit - NewLimit + 1);

    LOCK_PFN (OldIrql);

    if (MmResidentAvailablePages <= (SPFN_NUMBER)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        return STATUS_NO_MEMORY;
    }

    //
    // Note MmResidentAvailablePages must be charged before calling
    // MiEnsureAvailablePageOrWait as it may let go of the PFN lock.
    //

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(11, NumberOfPages);

    while (StackLimit >= NewLimit) {

        ASSERT (StackLimit->u.Hard.Valid == 0);

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (StackLimit));
        StackLimit->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

#ifdef PROTECT_KSTACKS
        StackLimit->u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
#endif

        MiInitializePfn (PageFrameIndex, StackLimit, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           StackLimit );

        MI_SET_PTE_DIRTY (TempPte);
        *StackLimit = TempPte;
        StackLimit -= 1;
    }

    MmKernelStackResident += NumberOfPages;
    UNLOCK_PFN (OldIrql);

#if DBG
    ASSERT (NewLimit->u.Hard.Valid == 1);
    if (NewLimit != EndStack) {
        ASSERT ((NewLimit - 1)->u.Hard.Valid == 0);
    }
#endif

    Thread->Tcb.StackLimit = MiGetVirtualAddressMappedByPte (NewLimit);

    PERFINFO_GROW_STACK(Thread);

    return STATUS_SUCCESS;
}

#if defined(_IA64_)

NTSTATUS
MmGrowKernelBackingStore (
    IN PVOID CurrentStack
    )

/*++

Routine Description:

    This function attempts to grows the current thread's kernel stack
    such that there is always KERNEL_LARGE_STACK_COMMIT bytes below
    the current stack pointer.

Arguments:

    CurrentStack - Supplies a pointer to the current stack pointer.

Return Value:

    STATUS_SUCCESS is returned if the stack was grown,
    STATUS_STACK_OVERFLOW is returned if there was not enough space reserved
    for the commitment.

--*/

{
    PMMPTE NewLimit;
    PMMPTE StackLimit;
    PMMPTE EndStack;
    PETHREAD Thread;
    PFN_NUMBER NumberOfPages;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    MMPTE TempPte;

    Thread = PsGetCurrentThread ();
    ASSERT (((PCHAR)Thread->Tcb.BStoreLimit - (PCHAR)Thread->Tcb.StackBase) <=
            (KERNEL_LARGE_BSTORE_SIZE + PAGE_SIZE));
    NewLimit = MiGetPteAddress ((PVOID)((PUCHAR)CurrentStack +
                                                    KERNEL_LARGE_BSTORE_COMMIT-1));

    StackLimit = MiGetPteAddress ((PCHAR)Thread->Tcb.BStoreLimit - (PCHAR)1);

    //
    // If the new stack limit is exceeds the reserved region for the kernel
    // stack, then return an error.
    //

    EndStack = MiGetPteAddress ((PVOID)((PUCHAR)Thread->Tcb.StackBase +
                                                    KERNEL_LARGE_BSTORE_SIZE-1));

    if (NewLimit > EndStack) {

        //
        // Don't go into guard page.
        //

        return STATUS_STACK_OVERFLOW;

    }

    ASSERT (StackLimit->u.Hard.Valid == 1);

    //
    // Lock the PFN database and attempt to expand the kernel stack.
    //

    StackLimit += 1;

    NumberOfPages = (PFN_NUMBER)(NewLimit - StackLimit + 1);

    LOCK_PFN (OldIrql);

    if (MmResidentAvailablePages <= (SPFN_NUMBER)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        return STATUS_NO_MEMORY;
    }

    //
    // Note we must charge MmResidentAvailablePages before calling
    // MiEnsureAvailablePageOrWait as it may let go of the PFN lock.
    //

    MmResidentAvailablePages -= NumberOfPages;

    while (StackLimit <= NewLimit) {

        ASSERT (StackLimit->u.Hard.Valid == 0);

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (StackLimit));
        StackLimit->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

#ifdef PROTECT_KSTACKS
        StackLimit->u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
#endif

        MiInitializePfn (PageFrameIndex, StackLimit, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           StackLimit );

        MI_SET_PTE_DIRTY (TempPte);
        *StackLimit = TempPte;
        StackLimit += 1;
    }

    MmKernelStackResident += NumberOfPages;
    UNLOCK_PFN (OldIrql);

#if DBG
    ASSERT (NewLimit->u.Hard.Valid == 1);
    if (NewLimit != EndStack) {
        ASSERT ((NewLimit + 1)->u.Hard.Valid == 0);
    }
#endif

    Thread->Tcb.BStoreLimit = MiGetVirtualAddressMappedByPte (NewLimit+1);

    return STATUS_SUCCESS;
}
#endif // defined(_IA64_)


VOID
MmOutPageKernelStack (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine makes the specified kernel stack non-resident and
    puts the pages on the transition list.  Note, that if the
    CurrentStackPointer is within the first page of the stack, the
    contents of the second page of the stack is not useful and the
    page is freed.

Arguments:

    Thread - Supplies a pointer to the thread whose stack should be
             removed.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

#if defined(_IA64_)
#define MAX_STACK_PAGES ((KERNEL_LARGE_STACK_SIZE + KERNEL_LARGE_BSTORE_SIZE) / PAGE_SIZE)
#else
#define MAX_STACK_PAGES (KERNEL_LARGE_STACK_SIZE / PAGE_SIZE)
#endif

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE EndOfStackPte;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    MMPTE TempPte;
    PVOID BaseOfKernelStack;
    PMMPTE FlushPte[MAX_STACK_PAGES];
    PVOID FlushVa[MAX_STACK_PAGES];
    MMPTE FlushPteSave[MAX_STACK_PAGES];
    ULONG StackSize;
    ULONG Count;
    PMMPTE LimitPte;
    PMMPTE LowestLivePte;

    ASSERT (((PCHAR)Thread->StackBase - (PCHAR)Thread->StackLimit) <=
            (KERNEL_LARGE_STACK_SIZE + PAGE_SIZE));

    if (NtGlobalFlag & FLG_DISABLE_PAGE_KERNEL_STACKS) {
        return;
    }

    //
    // The first page of the stack is the page before the base
    // of the stack.
    //

    BaseOfKernelStack = ((PCHAR)Thread->StackBase - PAGE_SIZE);
    PointerPte = MiGetPteAddress (BaseOfKernelStack);
    LastPte = MiGetPteAddress ((PULONG)Thread->KernelStack - 1);
    if (Thread->LargeStack) {
        StackSize = KERNEL_LARGE_STACK_SIZE >> PAGE_SHIFT;

        //
        // The stack pagein won't necessarily bring back all the pages.
        // Make sure that we account now for the ones that will disappear.
        //

        LimitPte = MiGetPteAddress (Thread->StackLimit);

        LowestLivePte = MiGetPteAddress ((PVOID)((PUCHAR)Thread->InitialStack -
                                            KERNEL_LARGE_STACK_COMMIT));

        if (LowestLivePte < LimitPte) {
            LowestLivePte = LimitPte;
        }
    } else {
        StackSize = KERNEL_STACK_SIZE >> PAGE_SHIFT;
        LowestLivePte = MiGetPteAddress (Thread->StackLimit);
    }
    EndOfStackPte = PointerPte - StackSize;

    ASSERT (LowestLivePte <= LastPte);

    //
    // Put a signature at the current stack location - 4.
    //

    *((PULONG_PTR)Thread->KernelStack - 1) = (ULONG_PTR)Thread;

    Count = 0;

    LOCK_PFN (OldIrql);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        TempPte = *PointerPte;
        MI_MAKE_VALID_PTE_TRANSITION (TempPte, 0);

#ifdef PROTECT_KSTACKS
        TempPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
        {
            PMMPFN x;
            x = MI_PFN_ELEMENT(PageFrameIndex);
            x->OriginalPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
        }
#endif

        FlushPteSave[Count] = TempPte;
        FlushPte[Count] = PointerPte;
        FlushVa[Count] = BaseOfKernelStack;

        MiDecrementShareCount (PageFrameIndex);
        PointerPte -= 1;
        Count += 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack - PAGE_SIZE);
    } while (PointerPte >= LastPte);

    while (PointerPte != EndOfStackPte) {
        if (PointerPte->u.Hard.Valid == 0) {
            break;
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));

        FlushPteSave[Count] = KernelDemandZeroPte;

#ifdef PROTECT_KSTACKS
        FlushPteSave[Count].u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
#endif

        FlushPte[Count] = PointerPte;

        FlushVa[Count] = BaseOfKernelStack;
        Count += 1;

        //
        // Account for any pages that won't ever come back in.
        //

        if (PointerPte < LowestLivePte) {
            ASSERT (Thread->LargeStack);
            MmResidentAvailablePages += 1;
            MM_BUMP_COUNTER(12, 1);
        }

        PointerPte -= 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack - PAGE_SIZE);
    }

#if defined(_IA64_)
    //
    // do for RSE stack space too.
    //

    BaseOfKernelStack = Thread->StackBase;
    PointerPte = MiGetPteAddress (BaseOfKernelStack);
    LastPte = MiGetPteAddress ((PULONG)Thread->KernelBStore);

    if (Thread->LargeStack) {
        StackSize = KERNEL_LARGE_BSTORE_SIZE >> PAGE_SHIFT;
    } else {
        StackSize = KERNEL_BSTORE_SIZE >> PAGE_SHIFT;
    }
    EndOfStackPte = PointerPte + StackSize;

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        TempPte = *PointerPte;
        MI_MAKE_VALID_PTE_TRANSITION (TempPte, 0);

#ifdef PROTECT_KSTACKS
        TempPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
        {
            PMMPFN x;
            x = MI_PFN_ELEMENT(PageFrameIndex);
            x->OriginalPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
        }
#endif

        FlushPteSave[Count] = TempPte;
        FlushPte[Count] = PointerPte;
        FlushVa[Count] = BaseOfKernelStack;

        MiDecrementShareCount (PageFrameIndex);
        PointerPte += 1;
        Count += 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack + PAGE_SIZE);
    } while (PointerPte <= LastPte);

    while (PointerPte != EndOfStackPte) {
        if (PointerPte->u.Hard.Valid == 0) {
            break;
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));

        FlushPteSave[Count] = KernelDemandZeroPte;

#ifdef PROTECT_KSTACKS
        FlushPteSave[Count].u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
#endif

        FlushPte[Count] = PointerPte;
        FlushVa[Count] = BaseOfKernelStack;
        Count += 1;

        PointerPte += 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack + PAGE_SIZE);
    }

#endif // _IA64_

    ASSERT (Count <= MAX_STACK_PAGES);

    if (Count < MM_MAXIMUM_FLUSH_COUNT) {
        KeFlushMultipleTb (Count,
                           &FlushVa[0],
                           TRUE,
                           TRUE,
                           &((PHARDWARE_PTE)FlushPte[0]),
                           ZeroPte.u.Flush);
    } else {
        KeFlushEntireTb (TRUE, TRUE);
    }

    //
    // Increase the available pages by the number of pages that where
    // deleted and turned into demand zero.
    //

    MmKernelStackResident -= Count;

    //
    // Put the right contents back into the PTEs
    //

    do {
        Count -= 1;
        *FlushPte[Count] = FlushPteSave[Count];
    } while (Count != 0);


    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MmInPageKernelStack (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine makes the specified kernel stack resident.

Arguments:

    Supplies a pointer to the base of the kernel stack.

Return Value:

    Thread - Supplies a pointer to the thread whose stack should be
             made resident.

Environment:

    Kernel mode.

--*/

{
    PVOID BaseOfKernelStack;
    PMMPTE PointerPte;
    PMMPTE EndOfStackPte;
    PMMPTE SignaturePte;
    ULONG DiskRead;
    PFN_NUMBER ContainingPage;
    KIRQL OldIrql;

    ASSERT (((PCHAR)Thread->StackBase - (PCHAR)Thread->StackLimit) <=
            (KERNEL_LARGE_STACK_SIZE + PAGE_SIZE));

    if (NtGlobalFlag & FLG_DISABLE_PAGE_KERNEL_STACKS) {
        return;
    }

    //
    // The first page of the stack is the page before the base
    // of the stack.
    //

    if (Thread->LargeStack) {
        PointerPte = MiGetPteAddress ((PVOID)((PUCHAR)Thread->StackLimit));

        EndOfStackPte = MiGetPteAddress ((PVOID)((PUCHAR)Thread->InitialStack -
                                            KERNEL_LARGE_STACK_COMMIT));
        //
        // Trim back the stack.  Make sure that the stack does not grow, i.e.
        // StackLimit remains the limit.
        //

        if (EndOfStackPte < PointerPte) {
            EndOfStackPte = PointerPte;
        }
        Thread->StackLimit = MiGetVirtualAddressMappedByPte (EndOfStackPte);
    } else {
        EndOfStackPte = MiGetPteAddress (Thread->StackLimit);
    }

#if defined(_IA64_)

    if (Thread->LargeStack) {
       
        PVOID TempAddress = (PVOID)((PUCHAR)Thread->BStoreLimit);

        BaseOfKernelStack = (PVOID)(((ULONG_PTR)Thread->InitialBStore + 
                               KERNEL_LARGE_BSTORE_COMMIT) &
                               ~(ULONG_PTR)(PAGE_SIZE - 1));

        //
        // Make sure the guard page is not set to valid.
        //

        if (BaseOfKernelStack > TempAddress) {
            BaseOfKernelStack = TempAddress;
        }
        Thread->BStoreLimit = BaseOfKernelStack;
    }
    BaseOfKernelStack = ((PCHAR)Thread->BStoreLimit - PAGE_SIZE);
#else
    BaseOfKernelStack = ((PCHAR)Thread->StackBase - PAGE_SIZE);
#endif // _IA64_

    PointerPte = MiGetPteAddress (BaseOfKernelStack);

    DiskRead = 0;
    SignaturePte = MiGetPteAddress ((PULONG_PTR)Thread->KernelStack - 1);
    ASSERT (SignaturePte->u.Hard.Valid == 0);
    if ((SignaturePte->u.Long != MM_KERNEL_DEMAND_ZERO_PTE) &&
        (SignaturePte->u.Soft.Transition == 0)) {
            DiskRead = 1;
    }

    LOCK_PFN (OldIrql);

    while (PointerPte >= EndOfStackPte) {

#ifdef PROTECT_KSTACKS
        if (!((PointerPte->u.Long == KernelDemandZeroPte.u.Long) ||
                (PointerPte->u.Soft.Protection == MM_KSTACK_OUTSWAPPED))) {
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x3451,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)Thread,
                          0);
        }
        ASSERT (PointerPte->u.Hard.Valid == 0);
        if (PointerPte->u.Soft.Protection == MM_KSTACK_OUTSWAPPED) {
            PointerPte->u.Soft.Protection = PAGE_READWRITE;
        }
#endif

        ContainingPage = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress (PointerPte));
        MiMakeOutswappedPageResident (PointerPte,
                                      PointerPte,
                                      1,
                                      ContainingPage);

        PointerPte -= 1;
        MmKernelStackResident += 1;
    }

    //
    // Check the signature at the current stack location - 4.
    //

    if (*((PULONG_PTR)Thread->KernelStack - 1) != (ULONG_PTR)Thread) {
        KeBugCheckEx (KERNEL_STACK_INPAGE_ERROR,
                      DiskRead,
                      *((PULONG_PTR)Thread->KernelStack - 1),
                      0,
                      (ULONG_PTR)Thread->KernelStack);
    }

    UNLOCK_PFN (OldIrql);
    return;
}


VOID
MmOutSwapProcess (
    IN PKPROCESS Process
    )

/*++

Routine Description:

    This routine out swaps the specified process.

Arguments:

    Process - Supplies a pointer to the process that is swapped out of memory.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PEPROCESS OutProcess;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PFN_NUMBER HyperSpacePageTable;
    PMMPTE HyperSpacePageTableMap;
    PFN_NUMBER PpePage;
    PFN_NUMBER PdePage;
    PMMPTE PageDirectoryMap;
    PFN_NUMBER ProcessPage;
    MMPTE TempPte;
#if defined (_X86PAE_)
    ULONG i;
    MMPTE TempPte2;
    PFN_NUMBER PdePage2;
    PFN_NUMBER HyperPage2;
    PPAE_ENTRY PaeVa;
#endif

    OutProcess = CONTAINING_RECORD (Process, EPROCESS, Pcb);

    OutProcess->ProcessOutswapEnabled = TRUE;

#if DBG
    if ((MmDebug & MM_DBG_SWAP_PROCESS) != 0) {
        return;
    }
#endif //DBG

    if (MiHydra == TRUE && OutProcess->Vm.u.Flags.ProcessInSession == 1) {
        MiSessionOutSwapProcess (OutProcess);
    }

    if ((OutProcess->Vm.WorkingSetSize == MM_PROCESS_COMMIT_CHARGE) &&
        (OutProcess->Vm.AllowWorkingSetAdjustment)) {

        LOCK_EXPANSION (OldIrql);

        ASSERT (OutProcess->ProcessOutswapped == FALSE);

        if (OutProcess->Vm.u.Flags.BeingTrimmed == TRUE) {

            //
            // An outswap is not allowed at this point because the process
            // has been attached to and is being trimmed.
            //

            UNLOCK_EXPANSION (OldIrql);
            return;
        }

        //
        // Swap the process working set info and page parent/directory/table
        // pages from memory.
        //

        OutProcess->ProcessOutswapped = TRUE;

        UNLOCK_EXPANSION (OldIrql);

        LOCK_PFN (OldIrql);

        //
        // Remove the working set list page from the process.
        //

#if !defined (_X86PAE_)
        HyperSpacePageTable =
             MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(OutProcess->Pcb.DirectoryTableBase[1])));
#else
        HyperSpacePageTable = (PFN_NUMBER)OutProcess->Pcb.DirectoryTableBase[1];
#endif

        HyperSpacePageTableMap = MiMapPageInHyperSpace (HyperSpacePageTable, &OldIrql2);

        TempPte = HyperSpacePageTableMap[MiGetPteOffset(MmWorkingSetList)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      MM_READWRITE);

        HyperSpacePageTableMap[MiGetPteOffset(MmWorkingSetList)] = TempPte;

#if defined (_X86PAE_)
        TempPte2 = HyperSpacePageTableMap[0];

        HyperPage2 = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte2);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte2,
                                      MM_READWRITE);

        HyperSpacePageTableMap[0] = TempPte2;
#endif

        MiUnmapPageInHyperSpace (OldIrql2);

#if DBG
        Pfn1 = MI_PFN_ELEMENT (OutProcess->WorkingSetPage);
        ASSERT (Pfn1->u3.e1.Modified == 1);
#endif
        MiDecrementShareCount (OutProcess->WorkingSetPage);

        //
        // Remove the hyper space page from the process.
        //

        Pfn1 = MI_PFN_ELEMENT (HyperSpacePageTable);
        PdePage = Pfn1->PteFrame;
        ASSERT (PdePage);

        PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);

        TempPte = PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)];

        ASSERT (TempPte.u.Hard.Valid == 1);
        ASSERT (TempPte.u.Hard.PageFrameNumber == HyperSpacePageTable);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      MM_READWRITE);

        PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)] = TempPte;

        ASSERT (Pfn1->u3.e1.Modified == 1);

        MiDecrementShareCount (HyperSpacePageTable);

#if defined (_X86PAE_)

        //
        // Remove the second hyper space page from the process.
        //

        Pfn1 = MI_PFN_ELEMENT (HyperPage2);

        ASSERT (Pfn1->u3.e1.Modified == 1);

        PdePage = Pfn1->PteFrame;
        ASSERT (PdePage);

        PageDirectoryMap[MiGetPdeOffset(HYPER_SPACE2)] = TempPte2;

        MiDecrementShareCount (HyperPage2);

        //
        // Remove the additional page directory pages.
        //

        PaeVa = (PPAE_ENTRY)OutProcess->PaeTop;
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
    
            TempPte = PageDirectoryMap[i];
            PdePage2 = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte);
    
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          MM_READWRITE);
    
            PageDirectoryMap[i] = TempPte;
            Pfn1 = MI_PFN_ELEMENT (PdePage2);
            ASSERT (Pfn1->u3.e1.Modified == 1);

            MiDecrementShareCount (PdePage2);
            PaeVa->PteEntry[i].u.Long = TempPte.u.Long;
        }

#if DBG
        TempPte = PageDirectoryMap[i];
        PdePage2 = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte);
        Pfn1 = MI_PFN_ELEMENT (PdePage2);
        ASSERT (Pfn1->u3.e1.Modified == 1);
#endif

#endif

#if defined (_WIN64)

        MiUnmapPageInHyperSpace (OldIrql2);

        //
        // Remove the page directory page (64-bit version).
        //

        Pfn1 = MI_PFN_ELEMENT (PdePage);
        PpePage = Pfn1->PteFrame;
        ASSERT (PpePage);
        ASSERT (PpePage == MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(OutProcess->Pcb.DirectoryTableBase[0]))));

        PageDirectoryMap = MiMapPageInHyperSpace (PpePage, &OldIrql2);

        TempPte = PageDirectoryMap[MiGetPpeOffset(MmWorkingSetList)];

        ASSERT (TempPte.u.Hard.Valid == 1);
        ASSERT (TempPte.u.Hard.PageFrameNumber == PdePage);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      MM_READWRITE);

        PageDirectoryMap[MiGetPpeOffset(MmWorkingSetList)] = TempPte;

        ASSERT (Pfn1->u3.e1.Modified == 1);

        MiDecrementShareCount (HyperSpacePageTable);

        //
        // Remove the top level page directory parent page.
        //

        TempPte = PageDirectoryMap[MiGetPpeOffset(PDE_TBASE)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      MM_READWRITE);

        PageDirectoryMap[MiGetPpeOffset(PDE_TBASE)] = TempPte;

        Pfn1 = MI_PFN_ELEMENT (PpePage);

#else

        //
        // Remove the top level page directory page.
        //

        TempPte = PageDirectoryMap[MiGetPdeOffset(PDE_BASE)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      MM_READWRITE);

        PageDirectoryMap[MiGetPdeOffset(PDE_BASE)] = TempPte;

        Pfn1 = MI_PFN_ELEMENT (PdePage);

#endif

        MiUnmapPageInHyperSpace (OldIrql2);

        //
        // Decrement share count so the top level page directory page gets
        // removed.  This can cause the PteCount to equal the sharecount as the
        // page directory page no longer contains itself, yet can have
        // itself as a transition page.
        //

        Pfn1->u2.ShareCount -= 2;
        Pfn1->PteAddress = (PMMPTE)&OutProcess->PageDirectoryPte;

        OutProcess->PageDirectoryPte = TempPte.u.Flush;

#if defined (_X86PAE_)
        PaeVa->PteEntry[i].u.Long = TempPte.u.Long;
#endif

        if (MI_IS_PHYSICAL_ADDRESS(OutProcess)) {
            ProcessPage = MI_CONVERT_PHYSICAL_TO_PFN (OutProcess);
        } else {
            PointerPte = MiGetPteAddress (OutProcess);
            ProcessPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        }

        Pfn1->PteFrame = ProcessPage;
        Pfn1 = MI_PFN_ELEMENT (ProcessPage);

        //
        // Increment the share count for the process page.
        //

        Pfn1->u2.ShareCount += 1;
        UNLOCK_PFN (OldIrql);

        LOCK_EXPANSION (OldIrql);
        if (OutProcess->Vm.WorkingSetExpansionLinks.Flink >
                                                       MM_IO_IN_PROGRESS) {

            //
            // The entry must be on the list.
            //
            RemoveEntryList (&OutProcess->Vm.WorkingSetExpansionLinks);
            OutProcess->Vm.WorkingSetExpansionLinks.Flink = MM_WS_SWAPPED_OUT;
        }
        UNLOCK_EXPANSION (OldIrql);

        OutProcess->WorkingSetPage = 0;
        OutProcess->Vm.WorkingSetSize = 0;
#if defined(_IA64_)

        //
        // Force assignment of new PID as we have removed
        // the page directory page.
        // Note that a TB flush would not work here as we
        // are in the wrong process context.
        //

        Process->ProcessRegion.SequenceNumber = 0;
#endif _IA64_

    }

    return;
}

VOID
MmInSwapProcess (
    IN PKPROCESS Process
    )

/*++

Routine Description:

    This routine in swaps the specified process.

Arguments:

    Process - Supplies a pointer to the process that is to be swapped
              into memory.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PEPROCESS OutProcess;
    PFN_NUMBER PdePage;
    PFN_NUMBER PageDirectoryPage;
    PMMPTE PageDirectoryMap;
    PMMPTE PageDirectoryParentMap;
    MMPTE TempPte;
    MMPTE TempPte2;
    PFN_NUMBER HyperSpacePageTable;
    PMMPTE HyperSpacePageTableMap;
    PFN_NUMBER WorkingSetPage;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PFN_NUMBER ProcessPage;
#if defined (_X86PAE_)
    ULONG i;
    PPAE_ENTRY PaeVa;
    PFN_NUMBER PdePage2;
#endif

    OutProcess = CONTAINING_RECORD (Process, EPROCESS, Pcb);

    if (OutProcess->ProcessOutswapped == TRUE) {

        //
        // The process is out of memory, rebuild the initialized page
        // structure.
        //

        if (MI_IS_PHYSICAL_ADDRESS(OutProcess)) {
            ProcessPage = MI_CONVERT_PHYSICAL_TO_PFN (OutProcess);
        } else {
            PointerPte = MiGetPteAddress (OutProcess);
            ProcessPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        }

        LOCK_PFN (OldIrql);

#if defined (_WIN64)
        PdePage = MiMakeOutswappedPageResident (MiGetPteAddress (PDE_TBASE),
                                        (PMMPTE)&OutProcess->PageDirectoryPte,
                                        0,
                                        ProcessPage);
#else
        PdePage = MiMakeOutswappedPageResident (MiGetPteAddress (PDE_BASE),
                                        (PMMPTE)&OutProcess->PageDirectoryPte,
                                        0,
                                        ProcessPage);
#endif

        //
        // Adjust the counts for the process page.
        //

        Pfn1 = MI_PFN_ELEMENT (ProcessPage);
        Pfn1->u2.ShareCount -= 1;

        ASSERT ((LONG)Pfn1->u2.ShareCount >= 1);

        //
        // Adjust the counts properly for the page directory page.
        //

        Pfn1 = MI_PFN_ELEMENT (PdePage);
        Pfn1->u2.ShareCount += 1;
#if !defined (_WIN64)
        Pfn1->u1.Event = (PVOID)OutProcess;
#endif
        Pfn1->PteFrame = PdePage;
        Pfn1->PteAddress = MiGetPteAddress (PDE_BASE);

#if defined (_WIN64)

        //
        // Only the page directory parent page has really been read in above.
        // Get the page directory page now also.
        //

        PageDirectoryParentMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);

        TempPte = PageDirectoryParentMap[MiGetPpeOffset(MmWorkingSetList)];

        MiUnmapPageInHyperSpace (OldIrql2);

        PageDirectoryPage = MiMakeOutswappedPageResident (
                                 MiGetPpeAddress (MmWorkingSetList),
                                 &TempPte,
                                 0,
                                 PdePage);

        ASSERT (PageDirectoryPage == TempPte.u.Hard.PageFrameNumber);
        ASSERT (Pfn1->u2.ShareCount >= 3);

        PageDirectoryParentMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);

        PageDirectoryParentMap[MiGetPpeOffset(PDE_TBASE)].u.Flush =
                                              OutProcess->PageDirectoryPte;
        PageDirectoryParentMap[MiGetPpeOffset(MmWorkingSetList)] = TempPte;

        MiUnmapPageInHyperSpace (OldIrql2);

        PdePage = PageDirectoryPage;
#endif

#if defined (_X86PAE_)

        OutProcess->PaePageDirectoryPage = PdePage;

        //
        // Locate the additional page directory pages and make them resident.
        //

        PaeVa = (PPAE_ENTRY)OutProcess->PaeTop;
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);
    
            TempPte = PageDirectoryMap[i];
    
            MiUnmapPageInHyperSpace (OldIrql2);
    
            PdePage2 = MiMakeOutswappedPageResident (
                                     MiGetPteAddress (PDE_BASE + (i << PAGE_SHIFT)),
                                     &TempPte,
                                     0,
                                     PdePage);
    
            ASSERT (Pfn1->u2.ShareCount >= 1);
    
            PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);
            PageDirectoryMap[i] = TempPte;
            MiUnmapPageInHyperSpace (OldIrql2);
            PaeVa->PteEntry[i].u.Long = (TempPte.u.Long & ~MM_PAE_PDPTE_MASK);
        }

        TempPte.u.Flush = OutProcess->PageDirectoryPte;
        TempPte.u.Long &= ~MM_PAE_PDPTE_MASK;
        PaeVa->PteEntry[i].u.Flush = TempPte.u.Flush;

        //
        // Locate the second page table page for hyperspace & make it resident.
        //

        PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);

        TempPte = PageDirectoryMap[MiGetPdeOffset(HYPER_SPACE2)];

        MiUnmapPageInHyperSpace (OldIrql2);

        HyperSpacePageTable = MiMakeOutswappedPageResident (
                                 MiGetPdeAddress (HYPER_SPACE2),
                                 &TempPte,
                                 0,
                                 PdePage);

        ASSERT (Pfn1->u2.ShareCount >= 1);

        PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);
        PageDirectoryMap[MiGetPdeOffset(HYPER_SPACE2)] = TempPte;
        MiUnmapPageInHyperSpace (OldIrql2);
        TempPte2 = TempPte;
#endif

        //
        // Locate the page table page for hyperspace and make it resident.
        //

        PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);

        TempPte = PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)];

        MiUnmapPageInHyperSpace (OldIrql2);

        HyperSpacePageTable = MiMakeOutswappedPageResident (
                                 MiGetPdeAddress (HYPER_SPACE),
                                 &TempPte,
                                 0,
                                 PdePage);

        ASSERT (Pfn1->u2.ShareCount >= 3);

        PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql2);

#if !defined (_WIN64)
        PageDirectoryMap[MiGetPdeOffset(PDE_BASE)].u.Flush =
                                              OutProcess->PageDirectoryPte;
#endif

        PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)] = TempPte;

        MiUnmapPageInHyperSpace (OldIrql2);

        //
        // Map in the hyper space page table page and retrieve the
        // PTE that maps the working set list.
        //

        HyperSpacePageTableMap = MiMapPageInHyperSpace (HyperSpacePageTable, &OldIrql2);
        TempPte = HyperSpacePageTableMap[MiGetPteOffset(MmWorkingSetList)];
        MiUnmapPageInHyperSpace (OldIrql2);
        Pfn1 = MI_PFN_ELEMENT (HyperSpacePageTable);

        Pfn1->u1.WsIndex = 1;

        WorkingSetPage = MiMakeOutswappedPageResident (
                                 MiGetPteAddress (MmWorkingSetList),
                                 &TempPte,
                                 0,
                                 HyperSpacePageTable);

        HyperSpacePageTableMap = MiMapPageInHyperSpace (HyperSpacePageTable, &OldIrql2);
        HyperSpacePageTableMap[MiGetPteOffset(MmWorkingSetList)] = TempPte;
#if defined (_X86PAE_)
        HyperSpacePageTableMap[0] = TempPte2;
#endif
        MiUnmapPageInHyperSpace (OldIrql2);

        Pfn1 = MI_PFN_ELEMENT (WorkingSetPage);

        Pfn1->u1.WsIndex = 2;

        UNLOCK_PFN (OldIrql);

        LOCK_EXPANSION (OldIrql);

        //
        // Allow working set trimming on this process.
        //

        OutProcess->Vm.AllowWorkingSetAdjustment = TRUE;
        if (OutProcess->Vm.WorkingSetExpansionLinks.Flink == MM_WS_SWAPPED_OUT) {
            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &OutProcess->Vm.WorkingSetExpansionLinks);
        }
        UNLOCK_EXPANSION (OldIrql);

        //
        // Set up process structures.
        //

        OutProcess->WorkingSetPage = WorkingSetPage;

#if !defined (_X86PAE_)
        OutProcess->Vm.WorkingSetSize = 3;

        INITIALIZE_DIRECTORY_TABLE_BASE (&Process->DirectoryTableBase[0],
                                         PdePage);
        INITIALIZE_DIRECTORY_TABLE_BASE (&Process->DirectoryTableBase[1],
                                         HyperSpacePageTable);
#else
        //
        // The DirectoryTableBase[0] never changes for PAE processes.
        //

        OutProcess->Vm.WorkingSetSize = 7;
        Process->DirectoryTableBase[1] = HyperSpacePageTable;
#endif

        OutProcess->ProcessOutswapped = FALSE;
    }

    if (MiHydra == TRUE && OutProcess->Vm.u.Flags.ProcessInSession == 1) {
        MiSessionInSwapProcess (OutProcess);
    }

    OutProcess->ProcessOutswapEnabled = FALSE;
    return;
}

PVOID
MiCreatePebOrTeb (
    IN PEPROCESS TargetProcess,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine creates a TEB or PEB page within the target process.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to create
                    the structure.

    Size - Supplies the size of the structure to create a VAD for.

Return Value:

    Returns the address of the base of the newly created TEB or PEB.

Environment:

    Kernel mode, attached to the specified process.

--*/

{

    PVOID Base;
    PMMVAD Vad;

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.
    //

    LOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    try {
        Vad = (PMMVAD)NULL;

        //
        // Find a VA for a PEB on a page-size boundary.
        //

        Base = MiFindEmptyAddressRangeDown (
                                ROUND_TO_PAGES (Size),
                                ((PCHAR)MM_HIGHEST_VAD_ADDRESS + 1),
                                PAGE_SIZE);

        //
        // An unoccupied address range has been found, build the virtual
        // address descriptor to describe this range.
        //

        Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool,
                                             sizeof(MMVAD),
                                             ' daV');
        if (Vad == (PMMVAD)0) {
            ExRaiseStatus (STATUS_NO_MEMORY);
        }

        Vad->StartingVpn = MI_VA_TO_VPN (Base);
        Vad->EndingVpn = MI_VA_TO_VPN ((PCHAR)Base + Size - 1);

        Vad->u.LongFlags = 0;

        Vad->u.VadFlags.CommitCharge = BYTES_TO_PAGES (Size);
        Vad->u.VadFlags.MemCommit = 1;
        Vad->u.VadFlags.PrivateMemory = 1;
        Vad->u.VadFlags.Protection = MM_EXECUTE_READWRITE;

        //
        // Mark VAD as not deletable, no protection change.
        //

        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.LongFlags2 = 0;
        Vad->u2.VadFlags2.OneSecured = 1;
        Vad->u2.VadFlags2.StoredInVad = 1;
        Vad->u2.VadFlags2.ReadOnly = 0;
        Vad->u3.Secured.StartVpn = (ULONG_PTR)Base;
        Vad->u3.Secured.EndVpn = (ULONG_PTR)MI_VPN_TO_VA_ENDING (Vad->EndingVpn);

        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception has occurred. If pool was allocated, deallocate
        // it and raise an exception for the caller.
        //

        if (Vad != (PMMVAD)NULL) {
            ExFreePool (Vad);
        }

        UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
        KeDetachProcess();
        ExRaiseStatus (GetExceptionCode ());
    }

    UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    return Base;
}

PTEB
MmCreateTeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_TEB InitialTeb,
    IN PCLIENT_ID ClientId
    )

/*++

Routine Description:

    This routine creates a TEB page within the target process
    and copies the initial TEB values into it.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to create
                    and initialize the TEB.

    InitialTeb - Supplies a pointer to the initial TEB to copy into the
                 newly created TEB.

Return Value:

    Returns the address of the base of the newly created TEB.

    Can raise exceptions if no address space is available for the TEB or
    the user has exceeded quota (non-paged, pagefile, commit).

Environment:

    Kernel mode.

--*/

{
    PTEB TebBase;

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    TebBase = (PTEB)MiCreatePebOrTeb (TargetProcess,
                                      (ULONG)sizeof(TEB));

    //
    // Initialize the TEB.
    //

#if defined(_WIN64)
    TebBase->NtTib.ExceptionList = NULL;
#else
    TebBase->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
#endif

    TebBase->NtTib.SubSystemTib = NULL;
    TebBase->NtTib.Version = OS2_VERSION;
    TebBase->NtTib.ArbitraryUserPointer = NULL;
    TebBase->NtTib.Self = (PNT_TIB)TebBase;
    TebBase->EnvironmentPointer = NULL;
    TebBase->ProcessEnvironmentBlock = TargetProcess->Peb;
    TebBase->ClientId = *ClientId;
    TebBase->RealClientId = *ClientId;

    if ((InitialTeb->OldInitialTeb.OldStackBase == NULL) &&
        (InitialTeb->OldInitialTeb.OldStackLimit == NULL)) {

        TebBase->NtTib.StackBase = InitialTeb->StackBase;
        TebBase->NtTib.StackLimit = InitialTeb->StackLimit;
        TebBase->DeallocationStack = InitialTeb->StackAllocationBase;

#if defined(_IA64_)
        TebBase->BStoreLimit = InitialTeb->BStoreLimit;
        TebBase->DeallocationBStore = (PCHAR)InitialTeb->StackBase
             + ((ULONG_PTR)InitialTeb->StackBase - (ULONG_PTR)InitialTeb->StackAllocationBase);
#endif

    }
    else {
        TebBase->NtTib.StackBase = InitialTeb->OldInitialTeb.OldStackBase;
        TebBase->NtTib.StackLimit = InitialTeb->OldInitialTeb.OldStackLimit;
    }

    TebBase->StaticUnicodeString.Buffer = TebBase->StaticUnicodeBuffer;
    TebBase->StaticUnicodeString.MaximumLength = (USHORT)sizeof( TebBase->StaticUnicodeBuffer );
    TebBase->StaticUnicodeString.Length = (USHORT)0;

    //
    // Used for BBT of ntdll and kernel32.dll.
    //

    TebBase->ReservedForPerf = BBTBuffer;

    KeDetachProcess();
    return TebBase;
}

//
// This code is built twice on the Win64 build - once for PE32+
// and once for PE32 images.
//

#define MI_INIT_PEB_FROM_IMAGE(Hdrs, ImgConfig) {                           \
    PebBase->ImageSubsystem = (Hdrs)->OptionalHeader.Subsystem;             \
    PebBase->ImageSubsystemMajorVersion =                                   \
        (Hdrs)->OptionalHeader.MajorSubsystemVersion;                       \
    PebBase->ImageSubsystemMinorVersion =                                   \
        (Hdrs)->OptionalHeader.MinorSubsystemVersion;                       \
                                                                            \
    /*                                                                   */ \
    /* See if this image wants GetVersion to lie about who the system is */ \
    /* If so, capture the lie into the PEB for the process.              */ \
    /*                                                                   */ \
                                                                            \
    if ((Hdrs)->OptionalHeader.Win32VersionValue != 0) {                    \
        PebBase->OSMajorVersion =                                           \
            (Hdrs)->OptionalHeader.Win32VersionValue & 0xFF;                \
        PebBase->OSMinorVersion =                                           \
            ((Hdrs)->OptionalHeader.Win32VersionValue >> 8) & 0xFF;         \
        PebBase->OSBuildNumber  =                                           \
            (USHORT)(((Hdrs)->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF); \
        if ((ImgConfig) != NULL && (ImgConfig)->CSDVersion != 0) {          \
            PebBase->OSCSDVersion = (ImgConfig)->CSDVersion;                \
            }                                                               \
                                                                            \
        /* Win32 API GetVersion returns the following bogus bit definitions */ \
        /* in the high two bits:                                            */ \
        /*                                                                  */ \
        /*      00 - Windows NT                                             */ \
        /*      01 - reserved                                               */ \
        /*      10 - Win32s running on Windows 3.x                          */ \
        /*      11 - Windows 95                                             */ \
        /*                                                                  */ \
        /*                                                                  */ \
        /* Win32 API GetVersionEx returns a dwPlatformId with the following */ \
        /* values defined in winbase.h                                      */ \
        /*                                                                  */ \
        /*      00 - VER_PLATFORM_WIN32s                                    */ \
        /*      01 - VER_PLATFORM_WIN32_WINDOWS                             */ \
        /*      10 - VER_PLATFORM_WIN32_NT                                  */ \
        /*      11 - reserved                                               */ \
        /*                                                                  */ \
        /*                                                                  */ \
        /* So convert the former from the Win32VersionValue field into the  */ \
        /* OSPlatformId field.  This is done by XORing with 0x2.  The       */ \
        /* translation is symmetric so there is the same code to do the     */ \
        /* reverse in windows\base\client\module.c (GetVersion)             */ \
        /*                                                                  */ \
        PebBase->OSPlatformId   =                                           \
            ((Hdrs)->OptionalHeader.Win32VersionValue >> 30) ^ 0x2;         \
        }                                                                   \
    }


#if defined(_WIN64)
VOID
MiInitializeWowPeb (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PPEB PebBase,
    IN PEPROCESS TargetProcess
    )

/*++

Routine Description:

    This routine creates a PEB32 page within the target process
    and copies the initial PEB32 values into it.

Arguments:

    NtHeaders - Supplies a pointer to the NT headers for the image.

    PebBase - Supplies a pointer to the initial PEB to derive the PEB32 values
              from.

    TargetProcess - Supplies a pointer to the process in which to create
                    and initialize the PEB32.

Return Value:

    Returns the address of the base of the newly created PEB.

    Can raise exceptions if no address space is available for the PEB32 or
    the user has exceeded quota (non-paged, pagefile, commit) or any inpage
    errors happen for the user addresses, etc.  If an exception is raised,
    note that the process detach is performed prior to returning.

Environment:

    Kernel mode.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    PPEB32 PebBase32;
    ULONG ProcessAffinityMask;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigData32;

    ProcessAffinityMask = 0;
    ImageConfigData32 = NULL;

    //
    // Image is 32-bit.
    //

    try {
        ImageConfigData32 = RtlImageDirectoryEntryToData (
                                PebBase->ImageBaseAddress,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                &ReturnedSize);

        ProbeForRead ((PVOID)ImageConfigData32,
                      sizeof (*ImageConfigData32),
                      sizeof (ULONG));

        MI_INIT_PEB_FROM_IMAGE ((PIMAGE_NT_HEADERS32)NtHeaders,
                                ImageConfigData32);

        if ((ImageConfigData32 != NULL) && (ImageConfigData32->ProcessAffinityMask != 0)) {
            ProcessAffinityMask = ImageConfigData32->ProcessAffinityMask;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeDetachProcess();
        ExRaiseStatus(STATUS_INVALID_IMAGE_PROTECT);
    }

    //
    // Create a PEB32 for the process.
    //

    PebBase32 = (PPEB32)MiCreatePebOrTeb (TargetProcess,
                                          (ULONG)sizeof (PEB32));

    //
    // Mark the process as WOW64 by storing the 32-bit PEB pointer
    // in the Wow64 field.
    //

    TargetProcess->Wow64Process->Wow64 = PebBase32;

    //
    // Clone the PEB into the PEB32.
    //

    PebBase32->InheritedAddressSpace = PebBase->InheritedAddressSpace;
    PebBase32->Mutant = PtrToUlong(PebBase->Mutant);
    PebBase32->ImageBaseAddress = PtrToUlong(PebBase->ImageBaseAddress);
    PebBase32->AnsiCodePageData = PtrToUlong(PebBase->AnsiCodePageData);
    PebBase32->OemCodePageData = PtrToUlong(PebBase->OemCodePageData);
    PebBase32->UnicodeCaseTableData = PtrToUlong(PebBase->UnicodeCaseTableData);
    PebBase32->NumberOfProcessors = PebBase->NumberOfProcessors;
    PebBase32->BeingDebugged = PebBase->BeingDebugged;
    PebBase32->NtGlobalFlag = PebBase->NtGlobalFlag;
    PebBase32->CriticalSectionTimeout = PebBase->CriticalSectionTimeout;

    if (PebBase->HeapSegmentReserve > 1024*1024*1024) { // 1gig
        PebBase32->HeapSegmentReserve = 1024*1024;      // 1meg
    } else {
        PebBase32->HeapSegmentReserve = (ULONG)PebBase->HeapSegmentReserve;
    }

    if (PebBase->HeapSegmentCommit > PebBase32->HeapSegmentReserve) {
        PebBase32->HeapSegmentCommit = 2*PAGE_SIZE;
    } else {
        PebBase32->HeapSegmentCommit = (ULONG)PebBase->HeapSegmentCommit;
    }

    PebBase32->HeapDeCommitTotalFreeThreshold = (ULONG)PebBase->HeapDeCommitTotalFreeThreshold;
    PebBase32->HeapDeCommitFreeBlockThreshold = (ULONG)PebBase->HeapDeCommitFreeBlockThreshold;
    PebBase32->NumberOfHeaps = PebBase->NumberOfHeaps;
    PebBase32->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof(PEB32)) / sizeof(ULONG);
    PebBase32->ProcessHeaps = PtrToUlong(PebBase32+1);
    PebBase32->OSMajorVersion = PebBase->OSMajorVersion;
    PebBase32->OSMinorVersion = PebBase->OSMinorVersion;
    PebBase32->OSBuildNumber = PebBase->OSBuildNumber;
    PebBase32->OSPlatformId = PebBase->OSPlatformId;
    PebBase32->OSCSDVersion = PebBase->OSCSDVersion;
    PebBase32->ImageSubsystem = PebBase->ImageSubsystem;
    PebBase32->ImageSubsystemMajorVersion = PebBase->ImageSubsystemMajorVersion;
    PebBase32->ImageSubsystemMinorVersion = PebBase->ImageSubsystemMinorVersion;
    PebBase32->SessionId = TargetProcess->SessionId;

    //
    // Leave the AffinityMask in the 32bit PEB as zero and let the
    // 64bit NTDLL set the initial mask.  This is to allow the
    // round robin scheduling of non MP safe imageing in the
    // following code to work correctly.
    //
    // Later code will set the affinity mask in the PEB32 if the
    // image actually specifies one.
    //
    // Note that the AffinityMask in the PEB is simply a mechanism
    // to pass affinity information from the image to the loader.   
    //

    //
    // Pass the affinity mask up to the 32 bit NTDLL via
    // the PEB32.  The 32 bit NTDLL will determine that the
    // affinity is not zero and try to set the affinity
    // mask from user-mode.  This call will be intercepted
    // by the wow64 thunks which will convert it
    // into a 64bit affinity mask and call the kernel.
    //

    PebBase32->ImageProcessAffinityMask = ProcessAffinityMask;
}
#endif


PPEB
MmCreatePeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_PEB InitialPeb
    )

/*++

Routine Description:

    This routine creates a PEB page within the target process
    and copies the initial PEB values into it.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to create
                    and initialize the PEB.

    InitialPeb - Supplies a pointer to the initial PEB to copy into the
                 newly created PEB.

Return Value:

    Returns the address of the base of the newly created PEB.

    Can raise exceptions if no address space is available for the PEB or
    the user has exceeded quota (non-paged, pagefile, commit).

Environment:

    Kernel mode.

--*/

{
    PPEB PebBase;
    USHORT Magic;
    USHORT Characteristics;
    NTSTATUS Status;
    PVOID ViewBase;
    LARGE_INTEGER SectionOffset;
    PIMAGE_NT_HEADERS NtHeaders;
    SIZE_T ViewSize;
    ULONG ReturnedSize;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG ProcessAffinityMask;

    ViewBase = NULL;
    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    ViewSize = 0;

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    //
    // Map the NLS tables into the application's address space.
    //

    Status = MmMapViewOfSection(
                InitNlsSectionPointer,
                TargetProcess,
                &ViewBase,
                0L,
                0L,
                &SectionOffset,
                &ViewSize,
                ViewShare,
                MEM_TOP_DOWN | SEC_NO_CHANGE,
                PAGE_READONLY
                );

    if ( !NT_SUCCESS(Status) ) {
        KeDetachProcess();
        ExRaiseStatus(Status);
    }

    PebBase = (PPEB)MiCreatePebOrTeb (TargetProcess,
                                      (ULONG)sizeof( PEB ));

    //
    // Initialize the Peb.
    //

    PebBase->InheritedAddressSpace = InitialPeb->InheritedAddressSpace;
    PebBase->Mutant = InitialPeb->Mutant;
    PebBase->ImageBaseAddress = TargetProcess->SectionBaseAddress;

    PebBase->AnsiCodePageData = (PVOID)((PUCHAR)ViewBase+InitAnsiCodePageDataOffset);
    PebBase->OemCodePageData = (PVOID)((PUCHAR)ViewBase+InitOemCodePageDataOffset);
    PebBase->UnicodeCaseTableData = (PVOID)((PUCHAR)ViewBase+InitUnicodeCaseTableDataOffset);

    PebBase->NumberOfProcessors = KeNumberProcessors;
    PebBase->BeingDebugged = (BOOLEAN)(TargetProcess->DebugPort != NULL ? TRUE : FALSE);
    PebBase->NtGlobalFlag = NtGlobalFlag;
    PebBase->CriticalSectionTimeout = MmCriticalSectionTimeout;
    PebBase->HeapSegmentReserve = MmHeapSegmentReserve;
    PebBase->HeapSegmentCommit = MmHeapSegmentCommit;
    PebBase->HeapDeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
    PebBase->HeapDeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
    PebBase->NumberOfHeaps = 0;
    PebBase->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof( PEB )) / sizeof( PVOID );
    PebBase->ProcessHeaps = (PVOID *)(PebBase+1);

    PebBase->OSMajorVersion = NtMajorVersion;
    PebBase->OSMinorVersion = NtMinorVersion;
    PebBase->OSBuildNumber = (USHORT)(NtBuildNumber & 0x3FFF);
    PebBase->OSPlatformId = 2;      // VER_PLATFORM_WIN32_NT from winbase.h
    PebBase->OSCSDVersion = (USHORT)CmNtCSDVersion;

    //
    // Every reference to NtHeaders (including the call to RtlImageNtHeader)
    // must be wrapped in try-except in case the inpage fails.  The inpage
    // can fail for any reason including network failures, low resources, etc.
    //

    try {
        NtHeaders = RtlImageNtHeader( PebBase->ImageBaseAddress );
        Magic = NtHeaders->OptionalHeader.Magic;
        Characteristics = NtHeaders->FileHeader.Characteristics;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeDetachProcess();
        ExRaiseStatus(STATUS_INVALID_IMAGE_PROTECT);
    }

    if (NtHeaders != NULL) {
    
        ProcessAffinityMask = 0;
#if defined(_WIN64)
        if (Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {

            //
            // If this call fails, an exception will be thrown and the
            // detach performed so no need to handle errors here.
            //

            MiInitializeWowPeb (NtHeaders, PebBase, TargetProcess);

        } else      // a PE32+ image
#endif
        {
            try {
                ImageConfigData = RtlImageDirectoryEntryToData (
                                        PebBase->ImageBaseAddress,
                                        TRUE,
                                        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                        &ReturnedSize);
    
                ProbeForRead ((PVOID)ImageConfigData,
                              sizeof (*ImageConfigData),
                              sizeof (ULONG));

                MI_INIT_PEB_FROM_IMAGE(NtHeaders, ImageConfigData);

                if (ImageConfigData != NULL && ImageConfigData->ProcessAffinityMask != 0) {
                    ProcessAffinityMask = ImageConfigData->ProcessAffinityMask;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                KeDetachProcess();
                ExRaiseStatus(STATUS_INVALID_IMAGE_PROTECT);
            }

        }

        //
        // Note NT4 examined the NtHeaders->FileHeader.Characteristics
        // for the IMAGE_FILE_AGGRESIVE_WS_TRIM bit, but this is not needed
        // or used for NT5 and above.
        //

        //
        // See if image wants to override the default processor affinity mask.
        //

        if (Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) {

            //
            // Image is NOT MP safe.  Assign it a processor on a rotating
            // basis to spread these processes around on MP systems.
            //

            do {
                PebBase->ImageProcessAffinityMask = (KAFFINITY)(0x1 << MmRotatingUniprocessorNumber);
                if (++MmRotatingUniprocessorNumber >= KeNumberProcessors) {
                    MmRotatingUniprocessorNumber = 0;
                }
            } while ((PebBase->ImageProcessAffinityMask & KeActiveProcessors) == 0);
        } else {

            if (ProcessAffinityMask != 0) {

                //
                // Pass the affinity mask from the image header
                // to LdrpInitializeProcess via the PEB.
                //

                PebBase->ImageProcessAffinityMask = ProcessAffinityMask;
            }
        }
    }

    PebBase->SessionId = TargetProcess->SessionId;

    KeDetachProcess();
    return PebBase;
}

VOID
MmDeleteTeb (
    IN PEPROCESS TargetProcess,
    IN PVOID TebBase
    )

/*++

Routine Description:

    This routine deletes a TEB page within the target process.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to delete
        the TEB.

    TebBase - Supplies the base address of the TEB to delete.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PVOID EndingAddress;
    PMMVAD Vad;
    NTSTATUS Status;
    PMMSECURE_ENTRY Secure;

    EndingAddress = ((PCHAR)TebBase +
                                ROUND_TO_PAGES (sizeof(TEB)) - 1);

    //
    // Attach to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.
    //

    LOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    Vad = MiLocateAddress (TebBase);

    ASSERT (Vad != (PMMVAD)NULL);

    ASSERT ((Vad->StartingVpn == MI_VA_TO_VPN (TebBase)) &&
            (Vad->EndingVpn == MI_VA_TO_VPN (EndingAddress)));

    //
    // If someone has secured the TEB (in addition to the standard securing
    // that was done by memory management on creation, then don't delete it
    // now - just leave it around until the entire process is deleted.
    //

    ASSERT (Vad->u.VadFlags.NoChange == 1);
    if (Vad->u2.VadFlags2.OneSecured) {
        Status = STATUS_SUCCESS;
    }
    else {
        ASSERT (Vad->u2.VadFlags2.MultipleSecured);
        ASSERT (IsListEmpty (&Vad->u3.List) == 0);

        //
        // If there's only one entry, then that's the one we defined when we
        // initially created the TEB.  So TEB deletion can take place right
        // now.  If there's more than one entry, let the TEB sit around until
        // the process goes away.
        //

        Secure = CONTAINING_RECORD (Vad->u3.List.Flink,
                                    MMSECURE_ENTRY,
                                    List);

        if (Secure->List.Flink == &Vad->u3.List) {
            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_NOT_FOUND;
        }
    }

    if (NT_SUCCESS(Status)) {

        MiRemoveVad (Vad);
        ExFreePool (Vad);

        MiDeleteFreeVm (TebBase, EndingAddress);
    }

    UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
    KeDetachProcess();
}

VOID
MmAllowWorkingSetExpansion (
    VOID
    )

/*++

Routine Description:

    This routine updates the working set list head FLINK field to
    indicate that working set adjustment is allowed.

    NOTE: This routine may be called more than once per process.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{

    PEPROCESS CurrentProcess;
    KIRQL OldIrql;

    //
    // Check the current state of the working set adjustment flag
    // in the process header.
    //

    CurrentProcess = PsGetCurrentProcess();

    LOCK_EXPANSION (OldIrql);

    if (!CurrentProcess->Vm.AllowWorkingSetAdjustment) {
        CurrentProcess->Vm.AllowWorkingSetAdjustment = TRUE;

        InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                        &CurrentProcess->Vm.WorkingSetExpansionLinks);
    }

    UNLOCK_EXPANSION (OldIrql);
    return;
}

#if DBG
ULONG MiDeleteLocked;
#endif


VOID
MiDeleteAddressesInWorkingSet (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine deletes all user mode addresses from the working set
    list.

Arguments:

    Process = Pointer to the current process.

Return Value:

    None.

Environment:

    Kernel mode, Working Set Lock held.

--*/

{
    PMMWSLE Wsle;
    ULONG index;
    ULONG Entry;
    PVOID Va;
    KIRQL OldIrql;
#if DBG
    PVOID SwapVa;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE LastWsle;
#endif

    //
    // Go through the working set and for any user-accessible page which is
    // in it, rip it out of the working set and free the page.
    //

    index = 2;
    Wsle = &MmWsle[index];

    MmWorkingSetList->HashTable = NULL;

    //
    // Go through the working set list and remove all pages for user
    // space addresses.
    //

    while (index <= MmWorkingSetList->LastEntry) {
        if (Wsle->u1.e1.Valid == 1) {

#if defined (_WIN64)
            ASSERT(MiGetPpeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
#endif
            ASSERT(MiGetPdeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
            ASSERT(MiGetPteAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);

            if (Wsle->u1.VirtualAddress < (PVOID)MM_HIGHEST_USER_ADDRESS) {

                //
                // This is a user mode address, for each one we remove we must
                // maintain the NonDirectCount.  This is because we may fault
                // later for page tables and need to grow the hash table when
                // updating the working set.  NonDirectCount needs to be correct
                // at that point.
                //

                if (Wsle->u1.e1.Direct == 0) {
                    Process->Vm.VmWorkingSetList->NonDirectCount -= 1;
                }

                //
                // This entry is in the working set list.
                //

                Va = Wsle->u1.VirtualAddress;

                MiReleaseWsle (index, &Process->Vm);
                LOCK_PFN (OldIrql);
                MiDeleteValidAddress (Va, Process);
                UNLOCK_PFN (OldIrql);

                if (index < MmWorkingSetList->FirstDynamic) {

                    //
                    // This entry is locked.
                    //

                    MmWorkingSetList->FirstDynamic -= 1;

                    if (index != MmWorkingSetList->FirstDynamic) {

                        Entry = MmWorkingSetList->FirstDynamic;
#if DBG
                        MiDeleteLocked += 1;
                        SwapVa = MmWsle[MmWorkingSetList->FirstDynamic].u1.VirtualAddress;
                        SwapVa = PAGE_ALIGN (SwapVa);

                        PointerPte = MiGetPteAddress (SwapVa);
                        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

                        ASSERT (Entry == MiLocateWsle (SwapVa, MmWorkingSetList, Pfn1->u1.WsIndex));
#endif
                        MiSwapWslEntries (Entry, index, &Process->Vm);
                    }
                }
            }
        }
        index += 1;
        Wsle += 1;
    }

#if DBG
    Wsle = &MmWsle[2];
    LastWsle = &MmWsle[MmWorkingSetList->LastInitializedWsle];
    while (Wsle <= LastWsle) {
        if (Wsle->u1.e1.Valid == 1) {
#if defined (_WIN64)
            ASSERT(MiGetPpeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
#endif
            ASSERT(MiGetPdeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
            ASSERT(MiGetPteAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
        }
        Wsle += 1;
    }
#endif

}


VOID
MiDeleteValidAddress (
    IN PVOID Va,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine deletes the specified virtual address.

Arguments:

    Va - Supplies the virtual address to delete.

    CurrentProcess - Supplies the current process.

Return Value:

    None.

Environment:

    Kernel mode.  PFN LOCK HELD.

    Note since this is only called during process teardown, the write watch
    bits are not updated.  If this ever called from other places, code
    will need to be added here to update those bits.

--*/

{
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMCLONE_BLOCK CloneBlock;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    PFN_NUMBER PageFrameIndex;

    PointerPte = MiGetPteAddress (Va);

#if defined (_WIN64)
    ASSERT(MiGetPpeAddress(Va)->u.Hard.Valid == 1);
#endif
    ASSERT(MiGetPdeAddress(Va)->u.Hard.Valid == 1);
    ASSERT (PointerPte->u.Hard.Valid == 1);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    CloneDescriptor = NULL;

    if (Pfn1->u3.e1.PrototypePte == 1) {

        CloneBlock = (PMMCLONE_BLOCK)Pfn1->PteAddress;

        //
        // Capture the state of the modified bit for this
        // pte.
        //

        MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

        //
        // Decrement the share and valid counts of the page table
        // page which maps this PTE.
        //

        PointerPde = MiGetPteAddress (PointerPte);
        MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

        //
        // Decrement the share count for the physical page.
        //

        MiDecrementShareCount (PageFrameIndex);

        //
        // Check to see if this is a fork prototype PTE and if so
        // update the clone descriptor address.
        //

        if (Va <= MM_HIGHEST_USER_ADDRESS) {

            //
            // Locate the clone descriptor within the clone tree.
            //

            CloneDescriptor = MiLocateCloneAddress ((PVOID)CloneBlock);
        }
    } else {

        //
        // This PTE is a NOT a prototype PTE, delete the physical page.
        //

        //
        // Decrement the share and valid counts of the page table
        // page which maps this PTE.
        //

        MiDecrementShareAndValidCount (Pfn1->PteFrame);

        MI_SET_PFN_DELETED (Pfn1);

        //
        // Decrement the share count for the physical page.  As the page
        // is private it will be put on the free list.
        //

        MiDecrementShareCountOnly (PageFrameIndex);

        //
        // Decrement the count for the number of private pages.
        //

        CurrentProcess->NumberOfPrivatePages -= 1;
    }

    //
    // Set the pointer to PTE to be a demand zero PTE.  This allows
    // the page usage count to be kept properly and handles the case
    // when a page table page has only valid PTEs and needs to be
    // deleted later when the VADs are removed.
    //

    PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    if (CloneDescriptor != NULL) {

        //
        // Decrement the reference count for the clone block,
        // note that this could release and reacquire
        // the mutexes hence cannot be done until after the
        // working set index has been removed.
        //

        if (MiDecrementCloneBlockReference ( CloneDescriptor,
                                             CloneBlock,
                                             CurrentProcess )) {

        }
    }
}

PFN_NUMBER
MiMakeOutswappedPageResident (
    IN PMMPTE ActualPteAddress,
    IN OUT PMMPTE PointerTempPte,
    IN ULONG Global,
    IN PFN_NUMBER ContainingPage
    )

/*++

Routine Description:

    This routine makes the specified PTE valid.

Arguments:

    ActualPteAddress - Supplies the actual address that the PTE will
                       reside at.  This is used for page coloring.

    PointerTempPte - Supplies the PTE to operate on, returns a valid
                     PTE.

    Global - Supplies 1 if the resulting PTE is global.

    ContainingPage - Supplies the physical page number of the page which
                     contains the resulting PTE.  If this value is 0, no
                     operations on the containing page are performed.

Return Value:

    Returns the physical page number that was allocated for the PTE.

Environment:

    Kernel mode, PFN LOCK HELD!

--*/

{
    MMPTE TempPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 2];
    PMDL Mdl;
    LARGE_INTEGER StartingOffset;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PFN_NUMBER PageFileNumber;
    NTSTATUS Status;
    PPFN_NUMBER Page;
    ULONG RefaultCount;
    PVOID HyperVa;

    MM_PFN_LOCK_ASSERT();

    OldIrql = APC_LEVEL;

    ASSERT (PointerTempPte->u.Hard.Valid == 0);

    if (PointerTempPte->u.Long == MM_KERNEL_DEMAND_ZERO_PTE) {

        //
        // Any page will do.
        //

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (
                            MI_GET_PAGE_COLOR_FROM_PTE (ActualPteAddress));

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           ActualPteAddress );
        MI_SET_PTE_DIRTY (TempPte);
        MI_SET_GLOBAL_STATE (TempPte, Global);

        MI_WRITE_VALID_PTE (PointerTempPte, TempPte);
        MiInitializePfnForOtherProcess (PageFrameIndex,
                                        ActualPteAddress,
                                        ContainingPage);

    } else if (PointerTempPte->u.Soft.Transition == 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerTempPte);
        PointerTempPte->u.Trans.Protection = MM_READWRITE;
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // PTE refers to a transition PTE.
        //

        if (Pfn1->u3.e1.PageLocation != ActiveAndValid) {
            MiUnlinkPageFromList (Pfn1);

            //
            // Even though this routine is only used to bring in special
            // system pages that are separately charged, a modified write
            // may be in progress and if so, will have applied a systemwide
            // charge against the locked pages count.  This all works out nicely
            // (with no code needed here) as the write completion will see
            // the nonzero ShareCount and remove the charge.
            //

            ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) ||
                    (Pfn1->u3.e1.LockCharged == 1));

            Pfn1->u3.e2.ReferenceCount += 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
        }

        //
        // Update the PFN database, the share count is now 1 and
        // the reference count is incremented as the share count
        // just went from zero to 1.
        //

        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e1.Modified = 1;
        if (Pfn1->u3.e1.WriteInProgress == 0) {

            //
            // Release the page file space for this page.
            //

            MiReleasePageFileSpace (Pfn1->OriginalPte);
            Pfn1->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
        }

        MI_MAKE_TRANSITION_PTE_VALID (TempPte, PointerTempPte);

        MI_SET_PTE_DIRTY (TempPte);
        MI_SET_GLOBAL_STATE (TempPte, Global);
        MI_WRITE_VALID_PTE (PointerTempPte, TempPte);

    } else {

        //
        // Page resides in a paging file.
        // Any page will do.
        //

        PointerTempPte->u.Soft.Protection = MM_READWRITE;
        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (
                            MI_GET_PAGE_COLOR_FROM_PTE (ActualPteAddress));

        //
        // Initialize the PFN database element, but don't
        // set read in progress as collided page faults cannot
        // occur here.
        //

        MiInitializePfnForOtherProcess (PageFrameIndex,
                                        ActualPteAddress,
                                        ContainingPage);

        KeInitializeEvent (&Event, NotificationEvent, FALSE);

        //
        // Calculate the VPN for the in-page operation.
        //

        TempPte = *PointerTempPte;
        PageFileNumber = GET_PAGING_FILE_NUMBER (TempPte);

        StartingOffset.QuadPart = (LONGLONG)(GET_PAGING_FILE_OFFSET (TempPte)) <<
                                    PAGE_SHIFT;

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Build MDL for request.
        //

        Mdl = (PMDL)&MdlHack[0];
        MmInitializeMdl(Mdl,
                        MiGetVirtualAddressMappedByPte (ActualPteAddress),
                        PAGE_SIZE);
        Mdl->MdlFlags |= MDL_PAGES_LOCKED;

        Page = (PPFN_NUMBER)(Mdl + 1);
        *Page = PageFrameIndex;

        UNLOCK_PFN (OldIrql);

#if DBG
        HyperVa = MiMapPageInHyperSpace (PageFrameIndex, &OldIrql);
        RtlFillMemoryUlong (HyperVa,
                            PAGE_SIZE,
                            0x34785690);
        MiUnmapPageInHyperSpace (OldIrql);
#endif

        //
        // Issue the read request.
        //

        RefaultCount = MiIoRetryLevel;

Refault:
        Status = IoPageRead ( MmPagingFile[PageFileNumber]->File,
                              Mdl,
                              &StartingOffset,
                              &Event,
                              &IoStatus
                              );

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject( &Event,
                                   WrPageIn,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)NULL);
            Status = IoStatus.Status;
        }

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
        }

        if (NT_SUCCESS(Status)) {
            if (IoStatus.Information != PAGE_SIZE) {
                KeBugCheckEx (KERNEL_STACK_INPAGE_ERROR,
                              2,
                              IoStatus.Status,
                              PageFileNumber,
                              StartingOffset.LowPart);
            }
        }

        if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(IoStatus.Status))) {
            if (((MmIsRetryIoStatus(Status)) ||
                (MmIsRetryIoStatus(IoStatus.Status))) &&
                (RefaultCount != 0)) {

                //
                // Insufficient resources, delay and reissue
                // the in page operation.
                //

                KeDelayExecutionThread (KernelMode,
                                        FALSE,
                                        &MmHalfSecond);
                KeClearEvent (&Event);
                RefaultCount -= 1;
                goto Refault;
            }
            KdPrint(("MMINPAGE: status %lx io-status %lx\n",
                      Status, IoStatus.Status));
            KeBugCheckEx (KERNEL_STACK_INPAGE_ERROR,
                          Status,
                          IoStatus.Status,
                          PageFileNumber,
                          StartingOffset.LowPart);
        }

        LOCK_PFN (OldIrql);

        //
        // Release the page file space.
        //

        MiReleasePageFileSpace (TempPte);
        Pfn1->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           ActualPteAddress );
        MI_SET_PTE_DIRTY (TempPte);
        Pfn1->u3.e1.Modified = 1;
        MI_SET_GLOBAL_STATE (TempPte, Global);

        MI_WRITE_VALID_PTE (PointerTempPte, TempPte);
    }
    return PageFrameIndex;
}


VOID
MmSetMemoryPriorityProcess(
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
    )

/*++

Routine Description:

    Sets the memory priority of a process.

Arguments:

    Process - Supplies the process to update

    MemoryPriority - Supplies the new memory priority of the process

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    UCHAR OldPriority;

    if (MmSystemSize == MmSmallSystem && MmNumberOfPhysicalPages < ((15*1024*1024)/PAGE_SIZE)) {

        //
        // If this is a small system, make every process BACKGROUND.
        //

        MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
    }

    LOCK_EXPANSION (OldIrql);

    OldPriority = Process->Vm.MemoryPriority;
    Process->Vm.MemoryPriority = MemoryPriority;

    UNLOCK_EXPANSION (OldIrql);

#ifndef _MI_USE_CLAIMS_
    if (OldPriority > MemoryPriority && MmAvailablePages < MmMoreThanEnoughFreePages) {
        //
        // The priority is being lowered, see if the working set
        // should be trimmed.
        //

        PMMSUPPORT VmSupport;
        ULONG i;
        ULONG Trim;
        LOGICAL Attached;

        VmSupport = &Process->Vm;
        i = VmSupport->WorkingSetSize - VmSupport->MaximumWorkingSetSize;
        if ((LONG)i > 0) {
            Trim = i;
            if (Trim > MmWorkingSetReductionMax) {
                Trim = MmWorkingSetReductionMax;
            }
            if (Process != PsGetCurrentProcess()) {
                KeAttachProcess (&Process->Pcb);
                Attached = TRUE;
            }
            else {
                Attached = FALSE;
            }
            LOCK_WS (Process);

            Trim = MiTrimWorkingSet (Trim,
                                     VmSupport,
                                     FALSE);

            MmWorkingSetList->Quota = VmSupport->WorkingSetSize;
            if (MmWorkingSetList->Quota < VmSupport->MinimumWorkingSetSize) {
                MmWorkingSetList->Quota = VmSupport->MinimumWorkingSetSize;
            }

            UNLOCK_WS (Process);
            if (Attached == TRUE) {
                KeDetachProcess();
            }
        }
    }
#endif
    return;
}


PMMVAD
MiAllocateVad(
    IN ULONG_PTR StartingVirtualAddress,
    IN ULONG_PTR EndingVirtualAddress,
    IN LOGICAL Deletable
    )

/*++

Routine Description:

    Reserve the specified range of address space.

Arguments:

    StartingVirtualAddress - Supplies the starting virtual address.

    EndingVirtualAddress - Supplies the ending virtual address.

    Deletable - Supplies TRUE if the VAD is to be marked as deletable, FALSE
                if deletions of this VAD should be disallowed.

Return Value:

    A VAD pointer on success, NULL on failure.

--*/

{
    PMMVAD Vad;

    ASSERT (StartingVirtualAddress <= EndingVirtualAddress);

    Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD), ' daV');

    if (Vad == NULL) {
       return NULL;
    }

    //
    // Set the starting and ending virtual page numbers of the VAD.
    //

    Vad->StartingVpn = MI_VA_TO_VPN (StartingVirtualAddress);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingVirtualAddress);

    //
    // Mark VAD as no commitment, private, and readonly.
    //

    Vad->u.LongFlags = 0;
    Vad->u.VadFlags.CommitCharge = MM_MAX_COMMIT;
    Vad->u.VadFlags.Protection = MM_READONLY;
    Vad->u.VadFlags.PrivateMemory = 1;

    Vad->u2.LongFlags2 = 0;

    if (Deletable == TRUE) {
        Vad->u.VadFlags.NoChange = 0;
        Vad->u2.VadFlags2.OneSecured = 0;
        Vad->u2.VadFlags2.StoredInVad = 0;
        Vad->u2.VadFlags2.ReadOnly = 0;
        Vad->u3.Secured.StartVpn = 0;
        Vad->u3.Secured.EndVpn = 0;
    }
    else {
        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.VadFlags2.OneSecured = 1;
        Vad->u2.VadFlags2.StoredInVad = 1;
        Vad->u2.VadFlags2.ReadOnly = 1;
        Vad->u3.Secured.StartVpn = StartingVirtualAddress;
        Vad->u3.Secured.EndVpn = EndingVirtualAddress;
    }

    return Vad;
}

#if 0
VOID
MiVerifyReferenceCounts (
    IN ULONG PdePage
    )

    //
    // Verify the share and valid PTE counts for page directory page.
    //

{
    PMMPFN Pfn1;
    PMMPFN Pfn3;
    PMMPTE Pte1;
    ULONG Share = 0;
    ULONG Valid = 0;
    ULONG i, ix, iy;
    PMMPTE PageDirectoryMap;
    KIRQL OldIrql;

    PageDirectoryMap = (PMMPTE)MiMapPageInHyperSpace (PdePage, &OldIrql);
    Pfn1 = MI_PFN_ELEMENT (PdePage);
    Pte1 = (PMMPTE)PageDirectoryMap;

    //
    // Map in the non paged portion of the system.
    //

    ix = MiGetPdeOffset(CODE_START);

    for (i = 0;i < ix; i += 1) {
        if (Pte1->u.Hard.Valid == 1) {
            Valid += 1;
        } else if ((Pte1->u.Soft.Prototype == 0) &&
                   (Pte1->u.Soft.Transition == 1)) {
            Pfn3 = MI_PFN_ELEMENT (Pte1->u.Trans.PageFrameNumber);
            if (Pfn3->u3.e1.PageLocation == ActiveAndValid) {
                ASSERT (Pfn1->u2.ShareCount > 1);
                Valid += 1;
            } else {
                Share += 1;
            }
        }
        Pte1 += 1;
    }

    iy = MiGetPdeOffset(PTE_BASE);
    Pte1 = &PageDirectoryMap[iy];
    ix  = MiGetPdeOffset(HYPER_SPACE_END) + 1;

    for (i = iy; i < ix; i += 1) {
        if (Pte1->u.Hard.Valid == 1) {
            Valid += 1;
        } else if ((Pte1->u.Soft.Prototype == 0) &&
                   (Pte1->u.Soft.Transition == 1)) {
            Pfn3 = MI_PFN_ELEMENT (Pte1->u.Trans.PageFrameNumber);
            if (Pfn3->u3.e1.PageLocation == ActiveAndValid) {
                ASSERT (Pfn1->u2.ShareCount > 1);
                Valid += 1;
            } else {
                Share += 1;
            }
        }
        Pte1 += 1;
    }

    if (Pfn1->u2.ShareCount != (Share+Valid+1)) {
        DbgPrint ("MMPROCSUP - PDE page %lx ShareCount %lx found %lx\n",
                PdePage, Pfn1->u2.ShareCount, Valid+Share+1);
    }

    MiUnmapPageInHyperSpace (OldIrql);
    ASSERT (Pfn1->u2.ShareCount == (Share+Valid+1));
    return;
}
#endif //0

#if defined (_X86PAE_)

VOID
MiPaeInitialize (
    VOID
    )
{
    InitializeListHead (&MiFirstFreePae.PaeEntry.ListHead);
}

ULONG
MiPaeAllocate (
    OUT PPAE_ENTRY *Va
    )

/*++

Routine Description:

    This routine allocates the top level page directory pointer structure.
    This structure will contain 4 PDPTEs.

Arguments:

    Va - Supplies a place to put the virtual address this page can be accessed
         at.

Return Value:

    Returns a virtual and physical address suitable for use as a top
    level page directory pointer page.

    Returns 0 if no page was allocated.

    Note that on success, the page returned must be below physical 4GB.

Environment:

    Kernel mode.  No locks may be held.

--*/

{
    ULONG i;
    PVOID Entry;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    LOGICAL FlushedOnce;
    PPAE_ENTRY Pae;
    PPAE_ENTRY PaeBase;

    FlushedOnce = FALSE;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    LOCK_PFN (OldIrql);

    do {

        if (MiFreePaes != 0) {

            ASSERT (IsListEmpty (&MiFirstFreePae.PaeEntry.ListHead) == 0);

            Pae = (PPAE_ENTRY) RemoveHeadList (&MiFirstFreePae.PaeEntry.ListHead);

            PaeBase = (PPAE_ENTRY)PAGE_ALIGN(Pae);
            PaeBase->PaeEntry.EntriesInUse += 1;
#if DBG
            RtlZeroMemory ((PVOID)Pae, sizeof(PAE_ENTRY));

            Pfn1 = MI_PFN_ELEMENT (PaeBase->PaeEntry.PageFrameNumber);
            ASSERT (Pfn1->u2.ShareCount == 1);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
            ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
#endif

            MiFreePaes -= 1;
            UNLOCK_PFN (OldIrql);

            ASSERT (KeGetCurrentIrql() <= APC_LEVEL);
            *Va = Pae;

            return (PaeBase->PaeEntry.PageFrameNumber << PAGE_SHIFT) + BYTE_OFFSET (Pae);
        }

        UNLOCK_PFN (OldIrql);

        if (FlushedOnce == TRUE) {
            break;
        }

        //
        // No free pages in the cachelist, replenish the list now.
        //

        Entry = MiPaeReplenishList ();

        if (Entry == NULL) {

            InterlockedIncrement (&MiDelayPageFaults);
    
            //
            // Attempt to move pages to the standby list.
            //
    
            MiEmptyAllWorkingSets ();
            MiFlushAllPages();
    
            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    &MmHalfSecond);

            InterlockedDecrement (&MiDelayPageFaults);

            FlushedOnce = TRUE;

            LOCK_PFN (OldIrql);

            //
            // Since all the working sets have been trimmed, check whether
            // another thread has replenished our list.  If not, then attempt
            // to do so since the working set pain has already been absorbed.
            //

            if (MiFreePaes < MINIMUM_PAE_THRESHOLD) {
                UNLOCK_PFN (OldIrql);
                MiPaeReplenishList ();
                LOCK_PFN (OldIrql);
            }

            continue;
        }
        LOCK_PFN (OldIrql);

    } while (TRUE);
    
    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    return 0;
}

PVOID
MiPaeFree (
    PPAE_ENTRY Pae
    )

/*++

Routine Description:

    This routine releases the top level page directory pointer page.

Arguments:

    PageFrameIndex - Supplies the top level page directory pointer page.

Return Value:

    A non-NULL pool address for the caller to free after releasing the PFN
    lock.  NULL if the caller needs to take no action.

Environment:

    Kernel mode.  The PFN lock is held on entry.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    ULONG i;
    PLIST_ENTRY NextEntry;
    PFN_NUMBER PageFrameIndex;
    PPAE_ENTRY PaeBase;

    MM_PFN_LOCK_ASSERT();

    if (MI_IS_PHYSICAL_ADDRESS(Pae) == 0) {
        PointerPte = MiGetPteAddress (Pae);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }
    else {
        PointerPte = NULL;
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (Pae);
    }

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT (Pfn1->u2.ShareCount == 1);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
    ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);

    //
    // This page must be in the first 4GB of RAM.
    //

    ASSERT (PageFrameIndex <= MM_HIGHEST_PAE_PAGE);

    PaeBase = (PPAE_ENTRY)PAGE_ALIGN(Pae);
    PaeBase->PaeEntry.EntriesInUse -= 1;

    if ((PaeBase->PaeEntry.EntriesInUse == 0) &&
        (MiFreePaes > EXCESS_PAE_THRESHOLD)) {

        //
        // Free the entire page.
        //

        i = 1;
        NextEntry = MiFirstFreePae.PaeEntry.ListHead.Flink;
        while (NextEntry != &MiFirstFreePae.PaeEntry.ListHead) {

            Pae = CONTAINING_RECORD (NextEntry,
                                     PAE_ENTRY,
                                     PaeEntry.ListHead);

            if (PAGE_ALIGN(Pae) == PaeBase) {
                RemoveEntryList (NextEntry);
                i += 1;
            }
            NextEntry = Pae->PaeEntry.ListHead.Flink;
        }
        ASSERT (i == PAES_PER_PAGE - 1);
        MiFreePaes -= (PAES_PER_PAGE - 1);

        return (PVOID)PaeBase;
    }

    InsertTailList (&MiFirstFreePae.PaeEntry.ListHead, &Pae->PaeEntry.ListHead);
    MiFreePaes += 1;

    return NULL;
}

PVOID
MiPaeReplenishList (
    VOID
    )

/*++

Routine Description:

    This routine searches the PFN database for free, zeroed or standby pages
    to satisfy the request.

Arguments:

    NumberOfPages - Supplies the number of pages desired.

Return Value:

    The virtual address of the allocated page, FALSE if no page was allocated.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    LONG start;
    ULONG i;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LowPage;
    PFN_NUMBER HighPage;
    MMLISTS PageListType;
    PMMPTE PointerPte;
    PVOID BaseAddress;
    PPAE_ENTRY Pae;
    ULONG NumberOfPages;
    MMPTE TempPte;
    ULONG PageColor;

    if (MiNoLowMemory == TRUE) {
        BaseAddress = MiAllocateLowMemory (PAGE_SIZE,
                                           0,
                                           0xFFFFF,
                                           0,
                                           (PVOID)0x123,
                                           'DeaP');
        if (BaseAddress == NULL) {
            return NULL;
        }

        Page = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(BaseAddress));

        Pae = (PPAE_ENTRY) BaseAddress;
        Pae->PaeEntry.EntriesInUse = 0;
        Pae->PaeEntry.PageFrameNumber = Page;
        Pae += 1;

        LOCK_PFN (OldIrql);

        for (i = 1; i < PAES_PER_PAGE; i += 1) {
            InsertTailList (&MiFirstFreePae.PaeEntry.ListHead,
                            &Pae->PaeEntry.ListHead);
            Pae += 1;
            MiFreePaes += 1;
        }

        UNLOCK_PFN (OldIrql);
        return BaseAddress;
    }

    HighPage = MM_HIGHEST_PAE_PAGE;
    NumberOfPages = 1;
    TempPte = ValidKernelPte;

    ExAcquireFastMutex (&MmDynamicMemoryMutex);
    start = (LONG)MmPhysicalMemoryBlock->NumberOfRuns - 1;

    LOCK_PFN (OldIrql);

    if (MmResidentAvailablePages <= 1) {
        UNLOCK_PFN (OldIrql);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return NULL;
    }

    MmResidentAvailablePages -= 1;
    MM_BUMP_COUNTER(57, 1);
                        
    //
    // Careful incrementing is done of the PageListType enum so the page cache
    // is not prematurely cannibalized.
    //
    // Pages are scanned from high descriptors first.
    //

    PageListType = FreePageList;

    do {
        while (start >= 0) {
    
            count = MmPhysicalMemoryBlock->Run[start].PageCount;
            Page = MmPhysicalMemoryBlock->Run[start].BasePage;
    
            if (count && (Page < HighPage)) {
    
                Pfn1 = MI_PFN_ELEMENT (Page);
                do {

                    if ((ULONG)Pfn1->u3.e1.PageLocation <= (ULONG)PageListType) {
                        if ((Pfn1->u1.Flink != 0) &&
                            (Pfn1->u2.Blink != 0) &&
                            (Pfn1->u3.e2.ReferenceCount == 0)) {
    
                            if (Page >= MmKseg2Frame) {
    
                                PointerPte = MiReserveSystemPtes (1,
                                                                  SystemPteSpace,
                                                                  0,
                                                                  0,
                                                                  FALSE);

                                if (PointerPte == NULL) {
                                    goto alldone;
                                }
                                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                                TempPte.u.Hard.PageFrameNumber = Page;
                                MI_WRITE_VALID_PTE (PointerPte, TempPte);
                            }
                            else {
                                PointerPte = NULL;
                                BaseAddress = (PVOID)(KSEG0_BASE + (Page << PAGE_SHIFT));
                            }

                            MiChargeCommitmentCantExpand (1, TRUE);
                            MM_TRACK_COMMIT (MM_DBG_COMMIT_CONTIGUOUS_PAGES, 1);

                            MmAllocatedNonPagedPool += 1;
                            NonPagedPoolDescriptor.TotalBigPages += 1;

                            //
                            // This page is in the desired range - grab it.
                            //
    
                            if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
                                MiUnlinkPageFromList (Pfn1);
                                MiRestoreTransitionPte (Page);
                            } else {
                                MiUnlinkFreeOrZeroedPage (Page);
                            }
    
                            Pfn1->u3.e2.ShortFlags = 0;
                            PageColor = MI_GET_PAGE_COLOR_FROM_VA(BaseAddress);
                            MI_CHECK_PAGE_ALIGNMENT(Page,
                                                    PageColor & MM_COLOR_MASK);
                            Pfn1->u3.e1.PageColor = PageColor & MM_COLOR_MASK;
                            PageColor += 1;

                            Pfn1->u3.e2.ReferenceCount = 1;
                            Pfn1->u2.ShareCount = 1;
                            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

                            if (PointerPte != NULL) {
                                Pfn1->PteAddress = PointerPte;
                                Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));
                            }
                            else {
                                Pfn1->PteAddress = BaseAddress;
                                Pfn1->PteFrame = (PFN_NUMBER)-1;
                            }

                            Pfn1->u3.e1.PageLocation = ActiveAndValid;
                            Pfn1->u3.e1.VerifierAllocation = 0;
                            Pfn1->u3.e1.LargeSessionAllocation = 0;
                            Pfn1->u3.e1.StartOfAllocation = 1;
                            Pfn1->u3.e1.EndOfAllocation = 1;

                            Pae = (PPAE_ENTRY) BaseAddress;
                            Pae->PaeEntry.EntriesInUse = 0;
                            Pae->PaeEntry.PageFrameNumber = Page;
                            Pae += 1;

                            for (i = 1; i < PAES_PER_PAGE; i += 1) {
                                InsertTailList (&MiFirstFreePae.PaeEntry.ListHead,
                                                &Pae->PaeEntry.ListHead);
                                Pae += 1;
                                MiFreePaes += 1;
                            }

                            //
                            // All the pages requested are available.
                            //

                            UNLOCK_PFN (OldIrql);

                            ExReleaseFastMutex (&MmDynamicMemoryMutex);

                            ExInsertPoolTag ('DeaP',
                                             BaseAddress,
                                             PAGE_SIZE,
                                             NonPagedPool);

                            return BaseAddress;
                        }
                    }
                    Page += 1;
                    Pfn1 += 1;
                    count -= 1;
    
                } while (count && (Page < HighPage));
            }
            start -= 1;
        }

        PageListType += 1;
        start = (LONG)MmPhysicalMemoryBlock->NumberOfRuns - 1;

    } while (PageListType <= StandbyPageList);

alldone:

    MmResidentAvailablePages += 1;
    MM_BUMP_COUNTER(57, -1);
    UNLOCK_PFN (OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);
                        
    return NULL;
}

VOID
ExRemovePoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes
    );

VOID
MiPaeFreeEntirePage (
    PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine releases a page that previously contained top level
    page directory pointer pages.

Arguments:

    VirtualAddress - Supplies the virtual address of the page that contained
                     top level page directory pointer pages.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.

--*/

{
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    KIRQL OldIrql;

#if defined (_X86PAE_)
    if (MiNoLowMemory == TRUE) {
        if (MiFreeLowMemory (VirtualAddress, 'DeaP') == TRUE) {
            return;
        }
    }
#endif

    ExRemovePoolTag ('DeaP', VirtualAddress, PAGE_SIZE);

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress) == 0) {
        PointerPte = MiGetPteAddress (VirtualAddress);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }
    else {
        PointerPte = NULL;
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (VirtualAddress);
    }

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    LOCK_PFN (OldIrql);

    ASSERT (Pfn1->u1.WsIndex == 0);
    ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
    ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
    ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
    ASSERT (Pfn1->u3.e1.StartOfAllocation == 1);
    ASSERT (Pfn1->u3.e1.EndOfAllocation == 1);
    ASSERT (Pfn1->u2.ShareCount == 1);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);

    Pfn1->u2.ShareCount = 0;
    MI_SET_PFN_DELETED (Pfn1);
#if DBG
    Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
    MiDecrementReferenceCount (PageFrameIndex);

    if (PointerPte != NULL) {
        KeFlushSingleTb (VirtualAddress,
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         ZeroKernelPte.u.Flush);
    }

    MmResidentAvailablePages += 1;
    MM_BUMP_COUNTER(57, -1);

    MmAllocatedNonPagedPool -= 1;
    NonPagedPoolDescriptor.TotalBigPages -= 1;

    UNLOCK_PFN (OldIrql);
                        
    if (PointerPte != NULL) {
        MiReleaseSystemPtes (PointerPte,
                             1,
                             SystemPteSpace);
    }

    MiReturnCommitment (1);
}
#endif
