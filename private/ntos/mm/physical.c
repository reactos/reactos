/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   physical.c

Abstract:

    This module contains the routines to manipulate physical memory from
    user space.

    There are restrictions on how user controlled physical memory can be used.
    Realize that all this memory is nonpaged and hence applications should
    allocate this with care as it represents a very real system resource.

    Virtual memory which maps user controlled physical memory pages must be :

    1.  Private memory only (ie: cannot be shared between processes).

    2.  The same physical page cannot be mapped at 2 different virtual
        addresses.

    3.  Callers must have LOCK_VM privilege to create these VADs.

    4.  Device drivers cannot call MmSecureVirtualMemory on it - this means
        that applications should not expect to use this memory for win32k.sys
        calls.

    5.  NtProtectVirtualMemory only allows read-write protection on this
        memory.  No other protection (no access, guard pages, readonly, etc)
        are allowed.

    6.  NtFreeVirtualMemory allows only MEM_RELEASE and NOT MEM_DECOMMIT on
        these VADs.  Even MEM_RELEASE is only allowed on entire VAD ranges -
        that is, splitting of these VADs is not allowed.

    7.  fork() style child processes don't inherit physical VADs.

    8.  The physical pages in these VADs are not subject to job limits.

Author:

    Landy Wang (landyw) 25-Jan-1999

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#ifndef DBG
#pragma alloc_text(PAGE,MiRemoveUserPhysicalPagesVad)
#endif
#pragma alloc_text(PAGE,MiCleanPhysicalProcessPages)
#endif

//
// This local stack size definition is deliberately large as ISVs have told
// us they expect to typically do this amount, and sometimes up to 1024 or more.
//

#define COPY_STACK_SIZE 256

#define BITS_IN_ULONG ((sizeof (ULONG)) * 8)
    
#define LOWEST_USABLE_PHYSICAL_ADDRESS    (16 * 1024 * 1024)
#define LOWEST_USABLE_PHYSICAL_PAGE       (LOWEST_USABLE_PHYSICAL_ADDRESS >> PAGE_SHIFT)

#define LOWEST_BITMAP_PHYSICAL_PAGE       0
#define MI_FRAME_TO_BITMAP_INDEX(x)       ((ULONG)(x))
#define MI_BITMAP_INDEX_TO_FRAME(x)       ((ULONG)(x))

ULONG_PTR MmVadPhysicalPages;

VOID
MiFlushUserPhysicalPteList (
    IN PMMPTE_FLUSH_LIST PteFlushList
    );


NTSTATUS
NtMapUserPhysicalPages(
    IN PVOID VirtualAddress,
    IN ULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray OPTIONAL
    )

/*++

Routine Description:

    This function maps the specified nonpaged physical pages into the specified
    user address range.

    Note no WSLEs are maintained for this range as it is all nonpaged.

Arguments:

    VirtualAddress - Supplies a user virtual address within a UserPhysicalPages
                     Vad.
        
    NumberOfPages - Supplies the number of pages to map.
        
    UserPfnArray - Supplies a pointer to the page frame numbers to map in.
                   If this is zero, then the virtual addresses are set to
                   NO_ACCESS.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PMMVAD FoundVad;
    KIRQL OldIrql;
    ULONG_PTR i;
    PEPROCESS Process;
    PMMPTE PointerPte;
    PVOID EndAddress;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    NTSTATUS Status;
    MMPTE_FLUSH_LIST PteFlushList;
    PVOID PoolArea;
    PPFN_NUMBER FrameList;
    ULONG BitMapIndex;
    ULONG_PTR StackArray[COPY_STACK_SIZE];
    MMPTE OldPteContents;
    MMPTE NewPteContents;
    ULONG_PTR NumberOfBytes;
    PRTL_BITMAP BitMap;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (NumberOfPages > (MAXULONG_PTR / PAGE_SIZE)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    VirtualAddress = PAGE_ALIGN(VirtualAddress);
    EndAddress = (PVOID)((PCHAR)VirtualAddress + (NumberOfPages << PAGE_SHIFT) -1);

    if (EndAddress <= VirtualAddress) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Carefully probe and capture all user parameters.
    //

    PoolArea = (PVOID)&StackArray[0];

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        NumberOfBytes = NumberOfPages * sizeof(ULONG_PTR);

        if (NumberOfPages > COPY_STACK_SIZE) {
            PoolArea = ExAllocatePoolWithTag (NonPagedPool,
                                              NumberOfBytes,
                                              'wRmM');
    
            if (PoolArea == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    
        //
        // Capture the specified page frame numbers.
        //

        try {
            ProbeForRead (UserPfnArray,
                          NumberOfBytes,
                          sizeof(ULONG_PTR));

            RtlCopyMemory (PoolArea, UserPfnArray, NumberOfBytes);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            if (PoolArea != (PVOID)&StackArray[0]) {
                ExFreePool (PoolArea);
            }
            return GetExceptionCode();
        }
    }

    PointerPte = MiGetPteAddress (VirtualAddress);

    Process = PsGetCurrentProcess();

    //
    // The AWE lock protects insertion/removal of Vads into each process'
    // PhysicalVadList.  It also protects creation/deletion and adds/removes
    // of the VadPhysicalPagesBitMap.  Finally, it protects the PFN
    // modifications for pages in the bitmap.
    //

    LOCK_AWE (Process, OldIrql);

    //
    // The physical pages bitmap must exist.
    //

    BitMap = Process->VadPhysicalPagesBitMap;

    if (BitMap == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    //
    // Note that the PFN lock is not needed to traverse this list (even though
    // MmProbeAndLockPages uses it), because all modifications are made while
    // also holding the AWE lock.
    //
    // The PhysicalVadList should typically have just one entry - the view
    // we're looking for, so this traverse should be quick.
    //

    FoundVad = NULL;
    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1) {

            if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
                (EndAddress <= (PVOID)PhysicalView->EndVa)) {

                    FoundVad = PhysicalView->Vad;
                    break;
            }
        }

        NextEntry = NextEntry->Flink;
        continue;
    }

    if (FoundVad == (PMMVAD)NULL) {

        //
        // No virtual address is reserved at the specified base address,
        // return an error.
        //

        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    //
    // Ensure the PFN element corresponding to each specified page is owned
    // by the specified VAD.
    //
    // Since this ownership can only be changed while holding this process'
    // working set lock, the PFN can be scanned here without holding the PFN
    // lock.
    //
    // Note the PFN lock is not needed because any race with MmProbeAndLockPages
    // can only result in the I/O going to the old page or the new page.
    // If the user breaks the rules, the PFN database (and any pages being
    // windowed here) are still protected because of the reference counts
    // on the pages with inprogress I/O.  This is possible because NO pages
    // are actually freed here - they are just windowed.
    //

    PteFlushList.Count = 0;

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        //
        // By keeping the PFN bitmap in the VAD (instead of in the PFN
        // database itself), a few benefits are realized:
        //
        // 1. No need to acquire the PFN lock here.
        // 2. Faster handling of PFN databases with holes.
        // 3. Transparent support for dynamic PFN database growth.
        // 4. Less nonpaged memory is used (for the bitmap vs adding a
        //    field to the PFN) on systems with no unused pack space in
        //    the PFN database, presuming not many of these VADs get
        //    allocated.
        //

        //
        // The first pass here ensures all the frames are secure.
        //

        //
        // N.B.  This implies that PFN_NUMBER is always ULONG_PTR in width
        //       as PFN_NUMBER is not exposed to application code today.
        //

        FrameList = (PPFN_NUMBER)PoolArea;

        for (i = 0; i < NumberOfPages; i += 1, FrameList += 1) {

            PageFrameIndex = *FrameList;

            //
            // Frames past the end of the bitmap are not allowed.
            //

            BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(PageFrameIndex);

#if defined (_WIN64)
            //
            // Ensure the frame is a 32-bit number.
            //

            if (BitMapIndex != PageFrameIndex) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }
#endif
            
            if (BitMapIndex >= BitMap->SizeOfBitMap) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // Frames not in the bitmap are not allowed.
            //

            if (RtlCheckBit (BitMap, BitMapIndex) == 0) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // The frame must not be already mapped anywhere.
            // Or be passed in twice in different spots in the array.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            if (Pfn1->u2.ShareCount != 1) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn0;
            }

            ASSERT (MI_PFN_IS_AWE (Pfn1));

            //
            // Mark the frame as "about to be mapped".
            //

            Pfn1->u2.ShareCount = 3;

            ASSERT (PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE);
        }

        //
        // This pass actually inserts them all into the page table pages and
        // the TBs now that we know the frames are good.
        //

        FrameList = (PPFN_NUMBER)PoolArea;

        MI_MAKE_VALID_PTE (NewPteContents,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPte);

        MI_SET_PTE_DIRTY (NewPteContents);

        for (i = 0; i < NumberOfPages; i += 1) {

            PageFrameIndex = *FrameList;
            NewPteContents.u.Hard.PageFrameNumber = PageFrameIndex;

            OldPteContents = *PointerPte;

            //
            // Flush the TB entry for this page if it's valid.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {
                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != (PMMPTE)0);
                ASSERT (Pfn1->u2.ShareCount == 2);
                Pfn1->u2.ShareCount -= 1;
                Pfn1->PteAddress = (PMMPTE)0;

#if defined (_X86PAE_)
                (VOID)KeInterlockedSwapPte ((PHARDWARE_PTE)PointerPte,
                                            (PHARDWARE_PTE)&NewPteContents);
#else
                *PointerPte = NewPteContents;
#endif
    
                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                    PteFlushList.Count += 1;
                }
            }
            else {
                MI_WRITE_VALID_PTE (PointerPte, NewPteContents);
            }

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->PteAddress == (PMMPTE)0);
            ASSERT (Pfn1->u2.ShareCount == 3);
            Pfn1->u2.ShareCount = 2;
            Pfn1->PteAddress = PointerPte;

            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            PointerPte += 1;
            FrameList += 1;
        }
    }
    else {

        //
        // Set the specified virtual address range to no access.
        //

        for (i = 0; i < NumberOfPages; i += 1) {

            OldPteContents = *PointerPte;
    
            //
            // Flush the TB entry for this page if it's valid.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {

                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != (PMMPTE)0);
                ASSERT (Pfn1->u2.ShareCount == 2);
                ASSERT (MI_PFN_IS_AWE (Pfn1));
                Pfn1->u2.ShareCount -= 1;
                Pfn1->PteAddress = (PMMPTE)0;

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                    PteFlushList.Count += 1;
                }
            }

            MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            PointerPte += 1;
        }
    }

    //
    // Flush the TB entries for these pages.  Note ZeroPte is only used
    // when the FlushPte[0] field is nonzero or if only a single PTE is
    // being flushed.
    //

    if (PteFlushList.Count != 0) {
        MiFlushUserPhysicalPteList (&PteFlushList);
    }

    UNLOCK_AWE (Process, OldIrql);

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    return STATUS_SUCCESS;

ErrorReturn0:

    while (i != 0) {
        FrameList -= 1;
        PageFrameIndex = *FrameList;
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 3);
        Pfn1->u2.ShareCount = 1;
        i -= 1;
    }

ErrorReturn:

    UNLOCK_AWE (Process, OldIrql);

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    return Status;
}


NTSTATUS
NtMapUserPhysicalPagesScatter(
    IN PVOID *VirtualAddresses,
    IN ULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray OPTIONAL
    )

/*++

Routine Description:

    This function maps the specified nonpaged physical pages into the specified
    user address range.

    Note no WSLEs are maintained for this range as it is all nonpaged.

Arguments:

    VirtualAddresses - Supplies a pointer to an array of user virtual addresses
                       within UserPhysicalPages Vads.  Each array entry is
                       presumed to map a single page.
        
    NumberOfPages - Supplies the number of pages to map.
        
    UserPfnArray - Supplies a pointer to the page frame numbers to map in.
                   If this is zero, then the virtual addresses are set to
                   NO_ACCESS.  If the array entry is zero then just the
                   corresponding virtual address is set to NO_ACCESS.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PMMVAD FoundVad;
    KIRQL OldIrql;
    ULONG_PTR i;
    PEPROCESS Process;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    NTSTATUS Status;
    MMPTE_FLUSH_LIST PteFlushList;
    PVOID PoolArea;
    PVOID *PoolVirtualArea;
    PPFN_NUMBER FrameList;
    ULONG BitMapIndex;
    PVOID StackVirtualArray[COPY_STACK_SIZE];
    ULONG_PTR StackArray[COPY_STACK_SIZE];
    MMPTE OldPteContents;
    MMPTE NewPteContents;
    ULONG_PTR NumberOfBytes;
    PRTL_BITMAP BitMap;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PVOID VirtualAddress;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (NumberOfPages > (MAXULONG_PTR / PAGE_SIZE)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Carefully probe and capture the user virtual address array.
    //

    PoolArea = (PVOID)&StackArray[0];
    PoolVirtualArea = (PVOID)&StackVirtualArray[0];

    NumberOfBytes = NumberOfPages * sizeof(PVOID);

    if (NumberOfPages > COPY_STACK_SIZE) {
        PoolVirtualArea = ExAllocatePoolWithTag (NonPagedPool,
                                                 NumberOfBytes,
                                                 'wRmM');

        if (PoolVirtualArea == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    try {
        ProbeForRead (VirtualAddresses,
                      NumberOfBytes,
                      sizeof(PVOID));

        RtlCopyMemory (PoolVirtualArea, VirtualAddresses, NumberOfBytes);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        goto ErrorReturn2;
    }

    //
    // Carefully probe and capture the user PFN array.
    //

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        NumberOfBytes = NumberOfPages * sizeof(ULONG_PTR);

        if (NumberOfPages > COPY_STACK_SIZE) {
            PoolArea = ExAllocatePoolWithTag (NonPagedPool,
                                              NumberOfBytes,
                                              'wRmM');
    
            if (PoolArea == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn2;
            }
        }
    
        //
        // Capture the specified page frame numbers.
        //

        try {
            ProbeForRead (UserPfnArray,
                          NumberOfBytes,
                          sizeof(ULONG_PTR));

            RtlCopyMemory (PoolArea, UserPfnArray, NumberOfBytes);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            goto ErrorReturn2;
        }
    }

    Process = PsGetCurrentProcess();

    //
    // The AWE lock protects insertion/removal of Vads into each process'
    // PhysicalVadList.  It also protects creation/deletion and adds/removes
    // of the VadPhysicalPagesBitMap.  Finally, it protects the PFN
    // modifications for pages in the bitmap.
    //

    PhysicalView = NULL;

    LOCK_AWE (Process, OldIrql);

    //
    // The physical pages bitmap must exist.
    //

    BitMap = Process->VadPhysicalPagesBitMap;

    if (BitMap == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    //
    // Note that the PFN lock is not needed to traverse this list (even though
    // MmProbeAndLockPages uses it), because all modifications are made while
    // also holding the AWE lock.
    //
    // The PhysicalVadList should typically have just one entry - the view
    // we're looking for, so this traverse should be quick.
    //

    for (i = 0; i < NumberOfPages; i += 1) {

        VirtualAddress = PoolVirtualArea[i];

        if (PhysicalView != NULL) {
            ASSERT (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1);
            if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
                (VirtualAddress <= (PVOID)PhysicalView->EndVa)) {
                    continue;
            }
        }

        FoundVad = NULL;
        NextEntry = Process->PhysicalVadList.Flink;
        while (NextEntry != &Process->PhysicalVadList) {
    
            PhysicalView = CONTAINING_RECORD(NextEntry,
                                             MI_PHYSICAL_VIEW,
                                             ListEntry);
    
            if (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1) {
    
                if ((VirtualAddress >= (PVOID)PhysicalView->StartVa) &&
                    (VirtualAddress <= (PVOID)PhysicalView->EndVa)) {
    
                        FoundVad = PhysicalView->Vad;
                        break;
                }
            }
    
            NextEntry = NextEntry->Flink;
            continue;
        }
    
        if (FoundVad == (PMMVAD)NULL) {
    
            //
            // No virtual address is reserved at the specified base address,
            // return an error.
            //
    
            Status = STATUS_INVALID_PARAMETER_1;
            goto ErrorReturn;
        }
    }

    //
    // Ensure the PFN element corresponding to each specified page is owned
    // by the specified VAD.
    //
    // Since this ownership can only be changed while holding this process'
    // working set lock, the PFN can be scanned here without holding the PFN
    // lock.
    //
    // Note the PFN lock is not needed because any race with MmProbeAndLockPages
    // can only result in the I/O going to the old page or the new page.
    // If the user breaks the rules, the PFN database (and any pages being
    // windowed here) are still protected because of the reference counts
    // on the pages with inprogress I/O.  This is possible because NO pages
    // are actually freed here - they are just windowed.
    //

    PteFlushList.Count = 0;

    if (ARGUMENT_PRESENT(UserPfnArray)) {

        //
        // By keeping the PFN bitmap in the VAD (instead of in the PFN
        // database itself), a few benefits are realized:
        //
        // 1. No need to acquire the PFN lock here.
        // 2. Faster handling of PFN databases with holes.
        // 3. Transparent support for dynamic PFN database growth.
        // 4. Less nonpaged memory is used (for the bitmap vs adding a
        //    field to the PFN) on systems with no unused pack space in
        //    the PFN database, presuming not many of these VADs get
        //    allocated.
        //

        //
        // The first pass here ensures all the frames are secure.
        //

        //
        // N.B.  This implies that PFN_NUMBER is always ULONG_PTR in width
        //       as PFN_NUMBER is not exposed to application code today.
        //

        FrameList = (PPFN_NUMBER)PoolArea;

        for (i = 0; i < NumberOfPages; i += 1, FrameList += 1) {

            PageFrameIndex = *FrameList;

            //
            // Zero entries are treated as a command to unmap.
            //

            if (PageFrameIndex == 0) {
                continue;
            }

            //
            // Frames past the end of the bitmap are not allowed.
            //

            BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(PageFrameIndex);

#if defined (_WIN64)
            //
            // Ensure the frame is a 32-bit number.
            //

            if (BitMapIndex != PageFrameIndex) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }
#endif
            
            if (BitMapIndex >= BitMap->SizeOfBitMap) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // Frames not in the bitmap are not allowed.
            //

            if (RtlCheckBit (BitMap, BitMapIndex) == 0) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn0;
            }

            //
            // The frame must not be already mapped anywhere.
            // Or be passed in twice in different spots in the array.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            if (Pfn1->u2.ShareCount != 1) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn0;
            }

            ASSERT (MI_PFN_IS_AWE (Pfn1));

            //
            // Mark the frame as "about to be mapped".
            //

            Pfn1->u2.ShareCount = 3;

            ASSERT (PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE);
        }

        //
        // This pass actually inserts them all into the page table pages and
        // the TBs now that we know the frames are good.
        //

        FrameList = (PPFN_NUMBER)PoolArea;

        MI_MAKE_VALID_PTE (NewPteContents,
                           PageFrameIndex,
                           MM_READWRITE,
                           0);

        MI_SET_PTE_DIRTY (NewPteContents);

        for (i = 0; i < NumberOfPages; i += 1, FrameList += 1) {

            PageFrameIndex = *FrameList;

            VirtualAddress = PoolVirtualArea[i];
            PointerPte = MiGetPteAddress (VirtualAddress);
            OldPteContents = *PointerPte;

            //
            // Flush the TB entry for this page if it's valid.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {
                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != (PMMPTE)0);
                ASSERT (Pfn1->u2.ShareCount == 2);
                ASSERT (MI_PFN_IS_AWE (Pfn1));
                Pfn1->u2.ShareCount -= 1;
                Pfn1->PteAddress = (PMMPTE)0;

                if (PageFrameIndex != 0) {

                    NewPteContents.u.Hard.PageFrameNumber = PageFrameIndex;
#if defined (_X86PAE_)
                    (VOID)KeInterlockedSwapPte ((PHARDWARE_PTE)PointerPte,
                                                (PHARDWARE_PTE)&NewPteContents);
#else
                    *PointerPte = NewPteContents;
#endif
                }
                else {
                    MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
                }
    
                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                    PteFlushList.Count += 1;
                }
            }
            else {
                if (PageFrameIndex != 0) {
                    NewPteContents.u.Hard.PageFrameNumber = PageFrameIndex;
                    MI_WRITE_VALID_PTE (PointerPte, NewPteContents);
                }
            }

            if (PageFrameIndex != 0) {
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->PteAddress == (PMMPTE)0);
                ASSERT (Pfn1->u2.ShareCount == 3);
                Pfn1->u2.ShareCount = 2;
                Pfn1->PteAddress = PointerPte;
            }
        }
    }
    else {

        //
        // Set the specified virtual address range to no access.
        //

        for (i = 0; i < NumberOfPages; i += 1) {

            VirtualAddress = PoolVirtualArea[i];
            PointerPte = MiGetPteAddress (VirtualAddress);
            OldPteContents = *PointerPte;
    
            //
            // Flush the TB entry for this page if it's valid.
            //
        
            if (OldPteContents.u.Hard.Valid == 1) {

                Pfn1 = MI_PFN_ELEMENT (OldPteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->PteAddress != (PMMPTE)0);
                ASSERT (Pfn1->u2.ShareCount == 2);
                ASSERT (MI_PFN_IS_AWE (Pfn1));
                Pfn1->u2.ShareCount -= 1;
                Pfn1->PteAddress = (PMMPTE)0;

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                    PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                    PteFlushList.Count += 1;
                }
            }

            MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
        }
    }

    //
    // Flush the TB entries for these pages.  Note ZeroPte is only used
    // when the FlushPte[0] field is nonzero or if only a single PTE is
    // being flushed.
    //

    if (PteFlushList.Count != 0) {
        MiFlushUserPhysicalPteList (&PteFlushList);
    }

    Status = STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_AWE (Process, OldIrql);

ErrorReturn2:

    if (PoolArea != (PVOID)&StackArray[0]) {
        ExFreePool (PoolArea);
    }

    if (PoolVirtualArea != (PVOID)&StackVirtualArray[0]) {
        ExFreePool (PoolVirtualArea);
    }

    return Status;

ErrorReturn0:

    while (i != 0) {
        FrameList -= 1;
        PageFrameIndex = *FrameList;
        if (PageFrameIndex != 0) {
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u2.ShareCount == 3);
            ASSERT (MI_PFN_IS_AWE (Pfn1));
            Pfn1->u2.ShareCount = 1;
        }
        i -= 1;
    }
    goto ErrorReturn;
}


NTSTATUS
NtAllocateUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG_PTR NumberOfPages,
    OUT PULONG_PTR UserPfnArray
    )

/*++

Routine Description:

    This function allocates nonpaged physical pages for the specified
    subject process.

    No WSLEs are maintained for this range.

    The caller must check the NumberOfPages returned to determine how many
    pages were actually allocated (this number may be less than the requested
    amount).

    On success, the user array is filled with the allocated physical page
    frame numbers (only up to the returned NumberOfPages is filled in).

    No PTEs are filled here - this gives the application the flexibility
    to order the address space with no metadata structure imposed by the Mm.
    Applications do this via NtMapUserPhysicalPages - ie:

        - Each physical page allocated is set in the process's bitmap.
          This provides remap, free and unmap a way to validate and rundown
          these frames.

          Unmaps may result in a walk of the entire bitmap, but that's ok as
          unmaps should be less frequent.  The win is it saves us from
          using up system virtual address space to manage these frames.

        - Note that the same physical frame may NOT be mapped at two different
          virtual addresses in the process.  This makes frees and unmaps
          substantially faster as no checks for aliasing need be performed.

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    NumberOfPages - Supplies a pointer to a variable that supplies the
                    desired size in pages of the allocation.  This is filled
                    with the actual number of pages allocated.
        
    UserPfnArray - Supplies a pointer to user memory to store the allocated
                   frame numbers into.

Return Value:

    Various NTSTATUS codes.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    KIRQL OldIrqlPfn;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LOGICAL Attached;
    LOGICAL WsHeld;
    ULONG_PTR CapturedNumberOfPages;
    ULONG_PTR AllocatedPages;
    ULONG_PTR MdlRequestInPages;
    ULONG_PTR TotalAllocatedPages;
    PMDL MemoryDescriptorList;
    PMDL MemoryDescriptorList2;
    PMDL MemoryDescriptorHead;
    PPFN_NUMBER MdlPage;
    PRTL_BITMAP BitMap;
    ULONG BitMapSize;
    ULONG BitMapIndex;
    PMMPFN Pfn1;
    PHYSICAL_ADDRESS LowAddress;
    PHYSICAL_ADDRESS MdlLowAddress;
    PHYSICAL_ADDRESS HighAddress;
    PHYSICAL_ADDRESS SkipBytes;
    PMMPTE PointerPte;
    MMPTE OldPteContents;
    MMPTE_FLUSH_LIST PteFlushList;
    ULONG SizeOfBitMap;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    Attached = FALSE;
    WsHeld = FALSE;

    //
    // Check the allocation type field.
    //

    PreviousMode = KeGetPreviousMode();

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {

        //
        // Capture the number of pages.
        //

        if (PreviousMode != KernelMode) {

            ProbeForWritePointer (NumberOfPages);

            CapturedNumberOfPages = *NumberOfPages;

            if (CapturedNumberOfPages == 0) {
                return STATUS_SUCCESS;
            }

            if (CapturedNumberOfPages > (MAXULONG_PTR / sizeof(ULONG_PTR))) {
                return STATUS_INVALID_PARAMETER_2;
            }

            ProbeForWrite (UserPfnArray,
                           (ULONG)(CapturedNumberOfPages * sizeof (ULONG_PTR)),
                           sizeof(PULONG_PTR));

        }
        else {
            CapturedNumberOfPages = *NumberOfPages;
        }

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

    //
    // Reference the specified process handle for VM_OPERATION access.
    //

    if (ProcessHandle == NtCurrentProcess()) {
        Process = PsGetCurrentProcess();
    }
    else {
        Status = ObReferenceObjectByHandle ( ProcessHandle,
                                             PROCESS_VM_OPERATION,
                                             PsProcessType,
                                             PreviousMode,
                                             (PVOID *)&Process,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    if (!SeSinglePrivilegeCheck (SeLockMemoryPrivilege, PreviousMode)) {
        if (ProcessHandle != NtCurrentProcess()) {
            ObDereferenceObject (Process);
        }
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    BitMapSize = 0;

    //
    // Get the working set mutex to synchronize.  This also blocks APCs so
    // an APC which takes a page fault does not corrupt various structures.
    //

    LOCK_WS (Process);

    WsHeld = TRUE;

    //
    // Make sure the address space was not deleted, If so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    //
    // Create the physical pages bitmap if it does not already exist.
    // LockMemory privilege is required.
    //

    BitMap = Process->VadPhysicalPagesBitMap;

    if (BitMap == NULL) {

        BitMapSize = sizeof(RTL_BITMAP) + (ULONG)((((MmHighestPossiblePhysicalPage + 1) + 31) / 32) * 4);

        BitMap = ExAllocatePoolWithTag (NonPagedPool, BitMapSize, 'LdaV');

        if (BitMap == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorReturn;
        }

        RtlInitializeBitMap (BitMap,
                             (PULONG)(BitMap + 1),
                             (ULONG)(MmHighestPossiblePhysicalPage + 1));

        RtlClearAllBits (BitMap);

        try {

            //
            // Charge quota for the nonpaged pool for the bitmap.  This is
            // done here rather than by using ExAllocatePoolWithQuota
            // so the process object is not referenced by the quota charge.
            //

            PsChargePoolQuota (Process, NonPagedPool, BitMapSize);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            ExFreePool (BitMap);
            goto ErrorReturn;
        }

        SizeOfBitMap = BitMap->SizeOfBitMap;
    }
    else {

        //
        // It's ok to snap this without a lock.
        //

        SizeOfBitMap = Process->VadPhysicalPagesBitMap->SizeOfBitMap;
    }

    AllocatedPages = 0;
    TotalAllocatedPages = 0;
    MemoryDescriptorHead = NULL;

    SkipBytes.QuadPart = 0;

    //
    // Allocate from the top of memory going down to preserve low pages
    // for 32/24-bit device drivers.  Just under 4gb is the maximum allocation
    // per MDL so the ByteCount field does not overflow.
    //

    HighAddress.QuadPart = ((ULONGLONG)(SizeOfBitMap - 1)) << PAGE_SHIFT;

    if (HighAddress.QuadPart > (ULONGLONG)0x100000000) {
        LowAddress.QuadPart = (ULONGLONG)0x100000000;
    }
    else {
        LowAddress.QuadPart = LOWEST_USABLE_PHYSICAL_ADDRESS;
        if (LowAddress.QuadPart >= HighAddress.QuadPart) {
            if (BitMapSize) {
                ExFreePool (BitMap);
                PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
            }
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorReturn;
        }
    }

    MdlLowAddress = LowAddress;

    do {

        MdlRequestInPages = CapturedNumberOfPages - TotalAllocatedPages;

        if (MdlRequestInPages > (ULONG_PTR)((MAXULONG - PAGE_SIZE) >> PAGE_SHIFT)) {
            MdlRequestInPages = (ULONG_PTR)((MAXULONG - PAGE_SIZE) >> PAGE_SHIFT);
        }

        //
        // Note this allocation returns zeroed pages.
        //

        MemoryDescriptorList = MmAllocatePagesForMdl (MdlLowAddress,
                                                      HighAddress,
                                                      SkipBytes,
                                                      MdlRequestInPages << PAGE_SHIFT);

        if (MemoryDescriptorList != NULL) {
            MemoryDescriptorList->Next = MemoryDescriptorHead;
            MemoryDescriptorHead = MemoryDescriptorList;

            MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

            AllocatedPages = MemoryDescriptorList->ByteCount >> PAGE_SHIFT;
            TotalAllocatedPages += AllocatedPages;

            LOCK_PFN (OldIrqlPfn);
            MmVadPhysicalPages += AllocatedPages;
            UNLOCK_PFN (OldIrqlPfn);

            //
            // The per-process WS lock guards updates to
            // Process->VadPhysicalPages.
            //

            Process->VadPhysicalPages += AllocatedPages;

#if defined(_ALPHA_) && !defined(_AXP64_)
            if (BitMapSize == 0) {
                LOCK_AWE (Process, OldIrql);
            }
#endif

            //
            // Update the allocation bitmap for each allocated frame.
            // Note the PFN lock is not needed to modify the PteAddress below.
            // In fact, even the AWE lock is not needed (except on Alpha32 due
            // to word tearing in an already existing bitmap) as these pages
            // are brand new.
            //

            for (i = 0; i < AllocatedPages; i += 1) {

                ASSERT (*MdlPage >= LOWEST_USABLE_PHYSICAL_PAGE);

                BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(*MdlPage);

                ASSERT (BitMapIndex < BitMap->SizeOfBitMap);
                ASSERT (RtlCheckBit (BitMap, BitMapIndex) == 0);

#if defined (_WIN64)
                //
                // This may become a problem for 64-bit systems with > 32tb
                // of physical memory as the 2nd parameter to RtlSetBits is
                // a ULONG.
                //

                ASSERT (*MdlPage < 0x100000000);
#endif

                Pfn1 = MI_PFN_ELEMENT (*MdlPage);
                ASSERT (MI_PFN_IS_AWE (Pfn1));
                Pfn1->PteAddress = (PMMPTE)0;
                ASSERT (Pfn1->u2.ShareCount == 1);

                RtlSetBits (BitMap, BitMapIndex, 1L);

                MdlPage += 1;
            }

#if defined(_ALPHA_) && !defined(_AXP64_)
            if (BitMapSize == 0) {
                UNLOCK_AWE (Process, OldIrql);
            }
#endif

            ASSERT (TotalAllocatedPages <= CapturedNumberOfPages);

            if (TotalAllocatedPages == CapturedNumberOfPages) {
                break;
            }

            //
            // Try the same memory range again - there might be more pages
            // left in it that can be claimed as a truncated MDL had to be
            // used for the last request.
            //

            continue;
        }

        if (LowAddress.QuadPart == LOWEST_USABLE_PHYSICAL_ADDRESS) {

            //
            // No (more) pages available.  If this becomes a common situation,
            // all the working sets could be flushed here.
            //

            if (TotalAllocatedPages == 0) {
                if (BitMapSize) {
                    ExFreePool (BitMap);
                    PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
                }
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }

            //
            // Make do with what we've gotten so far.
            //

            break;
        }

        ASSERT (HighAddress.QuadPart > (ULONGLONG)0x100000000);

        HighAddress.QuadPart = (ULONGLONG)0x100000000 - 1;
        LowAddress.QuadPart = LOWEST_USABLE_PHYSICAL_ADDRESS;

        MdlLowAddress = LowAddress;

    } while (TRUE);

    ASSERT (TotalAllocatedPages != 0);

    if (BitMapSize != 0) {

        //
        // If this API resulted in the creation of the bitmap, then set it
        // in the process structure now.  No need for locking around this.
        //

        Process->VadPhysicalPagesBitMap = BitMap;
    }

    UNLOCK_WS (Process);
    WsHeld = FALSE;

    if (Attached == TRUE) {
        KeDetachProcess();
        Attached = FALSE;
    }

    //
    // Establish an exception handler and carefully write out the
    // number of pages and the frame numbers.
    //

    try {

        ASSERT (TotalAllocatedPages <= CapturedNumberOfPages);

        *NumberOfPages = TotalAllocatedPages;

        MemoryDescriptorList = MemoryDescriptorHead;

        while (MemoryDescriptorList != NULL) {

            MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
            AllocatedPages = MemoryDescriptorList->ByteCount >> PAGE_SHIFT;

            for (i = 0; i < AllocatedPages; i += 1) {
                *UserPfnArray = *(PULONG_PTR)MdlPage;
                ASSERT (MI_PFN_ELEMENT(*MdlPage)->u2.ShareCount == 1);
                UserPfnArray += 1;
                MdlPage += 1;
            }
            MemoryDescriptorList = MemoryDescriptorList->Next;
        }

        Status = STATUS_SUCCESS;

    } except (ExSystemExceptionFilter()) {

        //
        // If anything went wrong communicating the pages back to the user
        // then the entire system service is rolled back.
        //

        Status = GetExceptionCode();

        MemoryDescriptorList = MemoryDescriptorHead;

        PteFlushList.Count = 0;

        if (PsGetCurrentProcess() != Process) {
            KeAttachProcess (&Process->Pcb);
            Attached = TRUE;
        }

        LOCK_WS (Process);
        WsHeld = TRUE;

        if (Process->AddressSpaceDeleted != 0) {
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto ErrorReturn;
        }

        //
        // AWE lock protection is needed here to prevent the malicious app
        // that is mapping these pages between our allocation and our free
        // below.
        //

        LOCK_AWE (Process, OldIrql);

        while (MemoryDescriptorList != NULL) {

            AllocatedPages = MemoryDescriptorList->ByteCount >> PAGE_SHIFT;
            MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

            for (i = 0; i < AllocatedPages; i += 1) {

                BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(*MdlPage);

                ASSERT (BitMapIndex < BitMap->SizeOfBitMap);
                ASSERT (RtlCheckBit (BitMap, BitMapIndex) == 1);

#if defined (_WIN64)
                //
                // This may become a problem for 64-bit systems with > 32tb
                // of physical memory as the 2nd parameter to RtlSetBits is
                // a ULONG.
                //

                ASSERT (*MdlPage < 0x100000000);
#endif
                RtlClearBits (BitMap, BitMapIndex, 1L);

                //
                // Note the PFN lock is not needed for the operations below.
                //

                Pfn1 = MI_PFN_ELEMENT (*MdlPage);
                ASSERT (MI_PFN_IS_AWE (Pfn1));

                //
                // The frame cannot be currently mapped in any Vad unless a
                // malicious app is trying random pages in an attempt to
                // corrupt the system.  Prevent this behavior by checking
                // the sharecount and handling it properly here.
                //

                if (Pfn1->u2.ShareCount != 1) {
        
                    ASSERT (Pfn1->u2.ShareCount == 2);
    
                    Pfn1->u2.ShareCount -= 1;
        
                    PointerPte = Pfn1->PteAddress;
                    Pfn1->PteAddress = (PMMPTE)0;

                    OldPteContents = *PointerPte;
            
                    ASSERT (OldPteContents.u.Hard.Valid == 1);
        
                    if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                        PteFlushList.FlushVa[PteFlushList.Count] =
                            MiGetVirtualAddressMappedByPte (PointerPte);
                        PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                        PteFlushList.Count += 1;
                    }
    
                    MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
                }

                ASSERT (Pfn1->u2.ShareCount == 1);

                MI_SET_PFN_DELETED(Pfn1);

                MdlPage += 1;
            }

            Process->VadPhysicalPages -= AllocatedPages;
            LOCK_PFN (OldIrqlPfn);
            MmVadPhysicalPages -= AllocatedPages;
            UNLOCK_PFN (OldIrqlPfn);
            MemoryDescriptorList = MemoryDescriptorList->Next;
        }

        //
        // Flush the TB entries for any pages which a malicious user may
        // have mapped.  Note ZeroPte is only used when the FlushPte[0]
        // field is nonzero or if only a single PTE is being flushed.
        //
    
        MiFlushUserPhysicalPteList (&PteFlushList);
    
        //
        // Carefully check to see if the bitmap can be freed.
        // Remember the working set mutex was dropped, so other threads may
        // have created or added to the bitmap.  If there is no one else using
        // the bitmap and it's empty, free the bitmap and return its quota.
        // Keep in mind that this thread may not have even been the creator.
        //

        if (Process->VadPhysicalPages == 0) {

            BitMap = Process->VadPhysicalPagesBitMap;
    
            ASSERT (BitMap != NULL);
            ASSERT (RtlFindSetBits (BitMap, 1, 0) == 0xFFFFFFFF);

            BitMapSize = sizeof(RTL_BITMAP) + (ULONG)((((MmHighestPossiblePhysicalPage + 1) + 31) / 32) * 4);
            Process->VadPhysicalPagesBitMap = NULL;
            ExFreePool (BitMap);
            PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
        }
        else {
            ASSERT (Process->VadPhysicalPagesBitMap != NULL);
        }

        UNLOCK_AWE (Process, OldIrql);

        UNLOCK_WS (Process);

        WsHeld = FALSE;

        //
        // Now that we're back at APC level or below, free the pages.
        //

        MemoryDescriptorList = MemoryDescriptorHead;
        while (MemoryDescriptorList != NULL) {
            MmFreePagesFromMdl (MemoryDescriptorList);
            MemoryDescriptorList = MemoryDescriptorList->Next;
        }

        //
        // Fall through...
        //
    }

    //
    // Free the space consumed by the MDLs now that the page frame numbers
    // have been saved in the bitmap and copied to the user.
    //

    MemoryDescriptorList = MemoryDescriptorHead;
    while (MemoryDescriptorList != NULL) {
        MemoryDescriptorList2 = MemoryDescriptorList->Next;
        ExFreePool (MemoryDescriptorList);
        MemoryDescriptorList = MemoryDescriptorList2;
    }

ErrorReturn:

    if (WsHeld == TRUE) {
        UNLOCK_WS (Process);
    }

    if (Attached == TRUE) {
        KeDetachProcess();
    }

    if (ProcessHandle != NtCurrentProcess()) {
        ObDereferenceObject (Process);
    }

    return Status;
}


NTSTATUS
NtFreeUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray
    )

/*++

Routine Description:

    This function frees the nonpaged physical pages for the specified
    subject process.  Any PTEs referencing these pages are also invalidated.

    Note there is no need to walk the entire VAD tree to clear the PTEs that
    match each page as each physical page can only be mapped at a single
    virtual address (alias addresses within the VAD are not allowed).

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    NumberOfPages - Supplies the size in pages of the allocation to delete.
                    Returns the actual number of pages deleted.
        
    UserPfnArray - Supplies a pointer to memory to retrieve the page frame
                   numbers from.

Return Value:

    Various NTSTATUS codes.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    KIRQL OldIrqlPfn;
    ULONG_PTR CapturedNumberOfPages;
    PMDL MemoryDescriptorList;
    PPFN_NUMBER MdlPage;
    PFN_NUMBER PagesInMdl;
    PFN_NUMBER PageFrameIndex;
    PRTL_BITMAP BitMap;
    ULONG BitMapIndex;
    ULONG_PTR PagesProcessed;
    PFN_NUMBER MdlHack[(sizeof(MDL) / sizeof(PFN_NUMBER)) + COPY_STACK_SIZE];
    ULONG_PTR MdlPages;
    ULONG_PTR NumberOfBytes;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LOGICAL Attached;
    PMMPFN Pfn1;
    LOGICAL WsHeld;
    LOGICAL AweLockHeld;
    LOGICAL OnePassComplete;
    LOGICAL ProcessReferenced;
    MMPTE_FLUSH_LIST PteFlushList;
    PMMPTE PointerPte;
    MMPTE OldPteContents;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Establish an exception handler, probe the specified addresses
    // for read access and capture the page frame numbers.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWritePointer (NumberOfPages);

            CapturedNumberOfPages = *NumberOfPages;

            //
            // Initialize the NumberOfPages freed to zero so the user can be
            // reasonably informed about errors that occur midway through
            // the transaction.
            //

            *NumberOfPages = 0;

        } except (ExSystemExceptionFilter()) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //
    
            return GetExceptionCode();
        }
    }
    else {
        CapturedNumberOfPages = *NumberOfPages;
    }

    if (CapturedNumberOfPages == 0) {
        return STATUS_INVALID_PARAMETER_2;
    }

    OnePassComplete = FALSE;
    PagesProcessed = 0;

    MemoryDescriptorList = (PMDL)0;

    if (CapturedNumberOfPages > COPY_STACK_SIZE) {

        //
        // Ensure the number of pages can fit into an MDL's ByteCount.
        //

        if (CapturedNumberOfPages > ((ULONG)MAXULONG >> PAGE_SHIFT)) {
            MdlPages = (ULONG_PTR)((ULONG)MAXULONG >> PAGE_SHIFT);
        }
        else {
            MdlPages = CapturedNumberOfPages;
        }

        while (MdlPages > COPY_STACK_SIZE) {
            MemoryDescriptorList = MmCreateMdl (NULL,
                                                0,
                                                MdlPages << PAGE_SHIFT);
    
            if (MemoryDescriptorList != NULL) {
                break;
            }

            MdlPages >>= 1;
        }
    }

    if (MemoryDescriptorList == NULL) {
        MdlPages = COPY_STACK_SIZE;
        MemoryDescriptorList = (PMDL)&MdlHack[0];
    }

    WsHeld = FALSE;
    AweLockHeld = FALSE;
    ProcessReferenced = FALSE;

repeat:

    if (CapturedNumberOfPages < MdlPages) {
        MdlPages = CapturedNumberOfPages;
    }

    MmInitializeMdl (MemoryDescriptorList, 0, MdlPages << PAGE_SHIFT);

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    NumberOfBytes = MdlPages * sizeof(ULONG_PTR);

    Attached = FALSE;

    //
    // Establish an exception handler, probe the specified addresses
    // for read access and capture the page frame numbers.
    //

    if (PreviousMode != KernelMode) {

        try {

            //
            // Update the user's count so if anything goes wrong, the user can
            // be reasonably informed about how far into the transaction it
            // occurred.
            //

            *NumberOfPages = PagesProcessed;

            ProbeForRead (UserPfnArray,
                          NumberOfBytes,
                          sizeof(PULONG_PTR));

            RtlCopyMemory ((PVOID)MdlPage,
                           UserPfnArray,
                           NumberOfBytes);

        } except (ExSystemExceptionFilter()) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            Status = GetExceptionCode();
            goto ErrorReturn;
        }
    }
    else {
        RtlCopyMemory ((PVOID)MdlPage,
                       UserPfnArray,
                       NumberOfBytes);
    }

    if (OnePassComplete == FALSE) {

        //
        // Reference the specified process handle for VM_OPERATION access.
        //
    
        if (ProcessHandle == NtCurrentProcess()) {
            Process = PsGetCurrentProcess();
        }
        else {
            Status = ObReferenceObjectByHandle ( ProcessHandle,
                                                 PROCESS_VM_OPERATION,
                                                 PsProcessType,
                                                 PreviousMode,
                                                 (PVOID *)&Process,
                                                 NULL );
    
            if (!NT_SUCCESS(Status)) {
                goto ErrorReturn;
            }
            ProcessReferenced = TRUE;
        }
    }
    
    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.  Block APCs so an APC which takes a page
    // fault does not corrupt various structures.
    //

    LOCK_WS (Process);
    WsHeld = TRUE;

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    LOCK_AWE (Process, OldIrql);
    AweLockHeld = TRUE;

    //
    // The physical pages bitmap must exist.
    //

    BitMap = Process->VadPhysicalPagesBitMap;

    if (BitMap == NULL) {
        Status = STATUS_INVALID_PARAMETER_2;
        goto ErrorReturn;
    }

    PteFlushList.Count = 0;

    Status = STATUS_SUCCESS;

    for (i = 0; i < MdlPages; i += 1, MdlPage += 1) {

        PageFrameIndex = *MdlPage;
        BitMapIndex = MI_FRAME_TO_BITMAP_INDEX(PageFrameIndex);

#if defined (_WIN64)
        //
        // Ensure the frame is a 32-bit number.
        //

        if (BitMapIndex != PageFrameIndex) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }
#endif
            
        //
        // Frames past the end of the bitmap are not allowed.
        //

        if (BitMapIndex >= BitMap->SizeOfBitMap) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }

        //
        // Frames not in the bitmap are not allowed.
        //

        if (RtlCheckBit (BitMap, BitMapIndex) == 0) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }

        ASSERT (PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE);

        PagesProcessed += 1;

#if defined (_WIN64)
        //
        // This may become a problem for 64-bit systems with > 32tb
        // of physical memory as the 2nd parameter to RtlClearBits is
        // a ULONG.
        //

        ASSERT (PageFrameIndex < 0x100000000);
#endif

        RtlClearBits (BitMap, BitMapIndex, 1L);

        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

        ASSERT (MI_PFN_IS_AWE (Pfn1));

#if DBG
        if (Pfn1->u2.ShareCount == 1) {
            ASSERT (Pfn1->PteAddress == (PMMPTE)0);
        }
        else if (Pfn1->u2.ShareCount == 2) {
            ASSERT (Pfn1->PteAddress != (PMMPTE)0);
        }
        else {
            ASSERT (FALSE);
        }
#endif

        //
        // If the frame is currently mapped in the Vad then the PTE must
        // be cleared and the TB entry flushed.
        //

        if (Pfn1->u2.ShareCount != 1) {

            Pfn1->u2.ShareCount -= 1;

            PointerPte = Pfn1->PteAddress;
            Pfn1->PteAddress = (PMMPTE)0;

            OldPteContents = *PointerPte;
    
            ASSERT (OldPteContents.u.Hard.Valid == 1);

            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushVa[PteFlushList.Count] =
                    MiGetVirtualAddressMappedByPte (PointerPte);
                PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                PteFlushList.Count += 1;
            }

            MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
        }

        MI_SET_PFN_DELETED(Pfn1);
    }

    //
    // Flush the TB entries for these pages.  Note ZeroPte is only used
    // when the FlushPte[0] field is nonzero or if only a single PTE is
    // being flushed.
    //

    MiFlushUserPhysicalPteList (&PteFlushList);

    //
    // Free the actual pages (this may be a partially filled MDL).
    //

    PagesInMdl = MdlPage - (PPFN_NUMBER)(MemoryDescriptorList + 1);

    //
    // Set the ByteCount to the actual number of validated pages - the caller
    // may have lied and we have to sync up here to account for any bogus
    // frames.
    //

    MemoryDescriptorList->ByteCount = (ULONG)(PagesInMdl << PAGE_SHIFT);

    if (PagesInMdl != 0) {
        Process->VadPhysicalPages -= PagesInMdl;
        UNLOCK_AWE (Process, OldIrql);
        AweLockHeld = FALSE;

        LOCK_PFN2 (OldIrqlPfn);
        MmVadPhysicalPages -= PagesInMdl;
        UNLOCK_PFN2 (OldIrqlPfn);

        MmFreePagesFromMdl (MemoryDescriptorList);
    }
    else {
        if (AweLockHeld == TRUE) {
            UNLOCK_AWE (Process, OldIrql);
            AweLockHeld = FALSE;
        }
    }

    CapturedNumberOfPages -= PagesInMdl;

    if ((Status == STATUS_SUCCESS) && (CapturedNumberOfPages != 0)) {

        UNLOCK_WS (Process);
        WsHeld = FALSE;

        if (Attached == TRUE) {
            KeDetachProcess();
            Attached = FALSE;
        }

        OnePassComplete = TRUE;
        ASSERT (MdlPages == PagesInMdl);
        UserPfnArray += MdlPages;

        //
        // Do it all again until all the pages are freed or an error occurs.
        //

        goto repeat;
    }

    //
    // Fall through.
    //

ErrorReturn:

    if (AweLockHeld == TRUE) {
        UNLOCK_AWE (Process, OldIrql);
    }

    if (WsHeld == TRUE) {
        UNLOCK_WS (Process);
    }

    //
    // Free any pool acquired for holding MDLs.
    //

    if (MemoryDescriptorList != (PMDL)&MdlHack[0]) {
        ExFreePool (MemoryDescriptorList);
    }

    if (Attached == TRUE) {
        KeDetachProcess();
    }

    //
    // Establish an exception handler and carefully write out the
    // number of pages actually processed.
    //

    try {

        *NumberOfPages = PagesProcessed;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Return success at this point even if the results
        // cannot be written.
        //

        NOTHING;
    }

    if (ProcessReferenced == TRUE) {
        ObDereferenceObject (Process);
    }

    return Status;
}


VOID
MiRemoveUserPhysicalPagesVad (
    IN PMMVAD_SHORT Vad
    )

/*++

Routine Description:

    This function removes the user-physical-pages mapped region from the
    current process's address space.  This mapped region is private memory.

    The physical pages of this Vad are unmapped here, but not freed.

    Pagetable pages are freed and their use/commitment counts/quotas are
    managed by our caller.

Arguments:

    Vad - Supplies the VAD which manages the address space.

Return Value:

    None.

Environment:

    APC level, working set mutex and address creation mutex held.

--*/

{
    PMMPFN Pfn1;
    PEPROCESS Process;
    PFN_NUMBER PageFrameIndex;
    MMPTE_FLUSH_LIST PteFlushList;
    PMMPTE PointerPte;
    MMPTE PteContents;
    PMMPTE EndingPte;
#if DBG
    KIRQL OldIrql;
    KIRQL OldIrql2;
    ULONG_PTR ActualPages;
    ULONG_PTR ExpectedPages;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
#endif

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    ASSERT (Vad->u.VadFlags.UserPhysicalPages == 1);

    Process = PsGetCurrentProcess();

    //
    // If the physical pages count is zero, nothing needs to be done.
    // On checked systems, verify the list anyway.
    //

#if DBG
    ActualPages = 0;
    ExpectedPages = Process->VadPhysicalPages;
#else
    if (Process->VadPhysicalPages == 0) {
        return;
    }
#endif

    //
    // The caller must have removed this Vad from the physical view list,
    // otherwise another thread could immediately remap pages back into the Vad.
    //
    // This allows us to proceed without acquiring the AWE or PFN locks -
    // everything can be done under the WS lock which is already held.
    //

#if DBG
    LOCK_AWE (Process, OldIrql);

    LOCK_PFN2 (OldIrql2);

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad == (PMMVAD)Vad) {
            DbgPrint ("MiRemoveUserPhysicalPagesVad : Vad %p still in list!\n",
                Vad);
            DbgBreakPoint ();
        }

        NextEntry = NextEntry->Flink;
    }

    UNLOCK_PFN2 (OldIrql2);
    UNLOCK_AWE (Process, OldIrql);
#endif

    //
    // If the physical pages bitmap doesn't exist, nothing needs to be done.
    //

    if (Process->VadPhysicalPagesBitMap == NULL) {
        ASSERT (ExpectedPages == 0);
        return;
    }

    PointerPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->StartingVpn));
    EndingPte = MiGetPteAddress (MI_VPN_TO_VA_ENDING (Vad->EndingVpn));

    PteFlushList.Count = 0;
    
    while (PointerPte <= EndingPte) {
        PteContents = *PointerPte;
        if (PteContents.u.Hard.Valid == 0) {
            PointerPte += 1;
            continue;
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        ASSERT (PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE);
        ASSERT (ExpectedPages != 0);

        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

        ASSERT (MI_PFN_IS_AWE (Pfn1));
        ASSERT (Pfn1->u2.ShareCount == 2);
        ASSERT (Pfn1->PteAddress == PointerPte);

        //
        // The frame is currently mapped in this Vad so the PTE must
        // be cleared and the TB entry flushed.
        //

        Pfn1->u2.ShareCount -= 1;
        Pfn1->PteAddress = (PMMPTE)0;

        if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
            PteFlushList.FlushVa[PteFlushList.Count] =
                MiGetVirtualAddressMappedByPte (PointerPte);
            PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
            PteFlushList.Count += 1;
        }

        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

        PointerPte += 1;
#if DBG
        ActualPages += 1;
#endif
        ASSERT (ActualPages <= ExpectedPages);
    }

    //
    // Flush the TB entries for these pages.  Note ZeroPte is only used
    // when the FlushPte[0] field is nonzero or if only a single PTE is
    // being flushed.
    //

    MiFlushUserPhysicalPteList (&PteFlushList);

    return;
}

VOID
MiUpdateVadPhysicalPages (
    IN ULONG_PTR TotalFreedPages
    )

/*++

Routine Description:

    Nonpaged helper routine to update the VadPhysicalPages count.

Arguments:

    TotalFreedPages - Supplies the number of pages just freed.

Return Value:

    None.

Environment:

    Kernel mode, APC level or below.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    MmVadPhysicalPages -= TotalFreedPages;
    UNLOCK_PFN (OldIrql);

    return;
}

VOID
MiCleanPhysicalProcessPages (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine frees the VadPhysicalBitMap, any remaining physical pages (as
    they may not have been currently mapped into any Vads) and returns the
    bitmap quota.

Arguments:

    Process - Supplies the process to clean.

Return Value:

    None.

Environment:

    Kernel mode, APC level, working set mutex held.  Called only on process
    exit, so the AWE lock is not needed here.

--*/

{
    PMMPFN Pfn1;
    ULONG BitMapSize;
    ULONG BitMapIndex;
    ULONG BitMapHint;
    PRTL_BITMAP BitMap;
    PPFN_NUMBER MdlPage;
    PFN_NUMBER MdlHack[(sizeof(MDL) / sizeof(PFN_NUMBER)) + COPY_STACK_SIZE];
    ULONG_PTR MdlPages;
    ULONG_PTR NumberOfPages;
    ULONG_PTR TotalFreedPages;
    PMDL MemoryDescriptorList;
    PFN_NUMBER PageFrameIndex;
#if DBG
    ULONG_PTR ActualPages = 0;
    ULONG_PTR ExpectedPages = 0;
#endif

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

#if DBG
    ExpectedPages = Process->VadPhysicalPages;
#else
    if (Process->VadPhysicalPages == 0) {
        return;
    }
#endif

    TotalFreedPages = 0;
    BitMap = Process->VadPhysicalPagesBitMap;

    if (BitMap != NULL) {

        MdlPages = COPY_STACK_SIZE;
        MemoryDescriptorList = (PMDL)&MdlHack[0];

        MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
        NumberOfPages = 0;
    
        BitMapHint = 0;

        while (TRUE) {

            BitMapIndex = RtlFindSetBits (BitMap, 1, BitMapHint);

            if (BitMapIndex < BitMapHint) {
                break;
            }

            if (BitMapIndex == 0xFFFFFFFF) {
                break;
            }

            PageFrameIndex = MI_BITMAP_INDEX_TO_FRAME(BitMapIndex);

#if defined (_WIN64)

            //
            // This may become a problem for 64-bit systems with > 32tb
            // of physical memory as the 3rd parameter to RtlFindSetBits is
            // a ULONG.
            //

            ASSERT (PageFrameIndex < 0x100000000);
#endif

            //
            // The bitmap search wraps, so handle it here.
            // Note PFN 0 is illegal.
            //
    
            ASSERT (PageFrameIndex != 0);
            ASSERT (PageFrameIndex >= LOWEST_USABLE_PHYSICAL_PAGE);

            ASSERT (ExpectedPages != 0);
            Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
            ASSERT (Pfn1->u2.ShareCount == 1);
            ASSERT (Pfn1->PteAddress == (PMMPTE)0);

            ASSERT (MI_PFN_IS_AWE (Pfn1));

            MI_SET_PFN_DELETED(Pfn1);

            *MdlPage = PageFrameIndex;
            MdlPage += 1;
            NumberOfPages += 1;
#if DBG
            ActualPages += 1;
#endif

            if (NumberOfPages == COPY_STACK_SIZE) {

                //
                // Free the pages in the full MDL.
                //

                MmInitializeMdl (MemoryDescriptorList,
                                 0,
                                 NumberOfPages << PAGE_SHIFT);

                MmFreePagesFromMdl (MemoryDescriptorList);

                MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
                Process->VadPhysicalPages -= NumberOfPages;
                TotalFreedPages += NumberOfPages;
                NumberOfPages = 0;
            }

            BitMapHint = BitMapIndex + 1;
            if (BitMapHint >= BitMap->SizeOfBitMap) {
                break;
            }
        }

        //
        // Free any straggling MDL pages here.
        //

        if (NumberOfPages != 0) {
            MmInitializeMdl (MemoryDescriptorList,
                             0,
                             NumberOfPages << PAGE_SHIFT);

            MmFreePagesFromMdl (MemoryDescriptorList);
            Process->VadPhysicalPages -= NumberOfPages;
            TotalFreedPages += NumberOfPages;
        }

        ASSERT (ExpectedPages == ActualPages);

        BitMapSize = sizeof(RTL_BITMAP) + (ULONG)((((MmHighestPossiblePhysicalPage + 1) + 31) / 32) * 4);

        Process->VadPhysicalPagesBitMap = NULL;
        ExFreePool (BitMap);
        PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
    }

    ASSERT (ExpectedPages == ActualPages);
    ASSERT (Process->VadPhysicalPages == 0);

    if (TotalFreedPages != 0) {
        MiUpdateVadPhysicalPages (TotalFreedPages);
    }

    return;
}

VOID
MiFlushUserPhysicalPteList (
    IN PMMPTE_FLUSH_LIST PteFlushList
    )

/*++

Routine Description:

    This routine flushes all the PTEs in the PTE flush list.
    If the list has overflowed, the entire TB is flushed.

    N.B.  The intent was for this routine to NEVER write the PTEs and have
          the caller do this instead.  There is no such export from Ke, so
          the flush of a single TB just reuses the PTE.

Arguments:

    PteFlushList - Supplies an optional pointer to the list to be flushed.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock NOT held.

--*/

{
    ULONG count;

    count = PteFlushList->Count;

    if (count == 0) {
        return;
    }

    if (count != 1) {
        if (count < MM_MAXIMUM_FLUSH_COUNT) {
            KeFlushMultipleTb (count,
                               &PteFlushList->FlushVa[0],
                               TRUE,
                               FALSE,
                               NULL,
                               ZeroPte.u.Flush);
        }
        else {

            //
            // Array has overflowed, flush the entire TB.
            //

            KeFlushEntireTb (TRUE, FALSE);
        }
    }
    else {

        //
        // This always writes the (same) value into the PTE.
        //

        KeFlushSingleTb (PteFlushList->FlushVa[0],
                         TRUE,
                         FALSE,
                         (PHARDWARE_PTE)PteFlushList->FlushPte[0],
                         *(PHARDWARE_PTE)PteFlushList->FlushPte[0]);
    }

    PteFlushList->Count = 0;
    return;
}
