/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wstree.c

Abstract:

    This module contains the routines which manipulate the working
    set list tree.

Author:

    Lou Perazzoli (loup) 15-May-1989

Revision History:

--*/

#include "mi.h"

#if (_MSC_VER >= 800)
#pragma warning(disable:4010)           /* Allow pretty pictures without the noise */
#endif

extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;

#if DBG
ULONG MmNumberOfInserts;
#endif

ULONG
MiLookupWsleHashIndex (
    IN ULONG_PTR WsleEntry,
    IN PMMWSL WorkingSetList
    );

VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    );


VOID
FASTCALL
MiInsertWsle (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine inserts a Working Set List Entry (WSLE) into the
    working set.

Arguments:

    Entry - The index number of the WSLE to insert.

    WorkingSetList - Supplies the working set list to insert into.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMSUPPORT WsInfo;
    WSLE_NUMBER Hash;
    PMMWSLE_HASH Table;
    WSLE_NUMBER j;
    PMMPTE PointerPte;
    WSLE_NUMBER Index;
    LARGE_INTEGER TickCount;
    ULONG Size;
#if defined(_ALPHA_) && !defined(_AXP64_)
    KIRQL OldIrql;
#endif

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress);

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("inserting element %lx %lx\n", Entry, Wsle[Entry].u1.Long);
    }

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);
    ASSERT (Wsle[Entry].u1.e1.Direct != 1);
#endif //DBG

    WorkingSetList->NonDirectCount += 1;

    if ((Table = WorkingSetList->HashTable) == NULL) {
        return;
    }

#if DBG
    MmNumberOfInserts += 1;
#endif //DBG

    Hash = MI_WSLE_HASH(Wsle[Entry].u1.Long, WorkingSetList);

    //
    // Check hash table size and see if there is enough room to
    // hash or if the table should be grown.
    //

    if ((WorkingSetList->NonDirectCount + 10 +
            (WorkingSetList->HashTableSize >> 4)) >
                                 WorkingSetList->HashTableSize) {

        if (WorkingSetList == MmWorkingSetList) {
            WsInfo = &PsGetCurrentProcess()->Vm;
            ASSERT (WsInfo->u.Flags.SessionSpace == 0);
        }
        else if (WorkingSetList == MmSystemCacheWorkingSetList) {
            WsInfo = &MmSystemCacheWs;
            ASSERT (WsInfo->u.Flags.SessionSpace == 0);
        }
        else {
            WsInfo = &MmSessionSpace->Vm;
            ASSERT (WsInfo->u.Flags.SessionSpace == 1);
        }

        if ((Table + WorkingSetList->HashTableSize + ((2*PAGE_SIZE) / sizeof (MMWSLE_HASH)) <= (PMMWSLE_HASH)WorkingSetList->HighestPermittedHashAddress) &&
                (WsInfo->AllowWorkingSetAdjustment)) {

#if defined(_ALPHA_) && !defined(_AXP64_)
            LOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif
            WsInfo->AllowWorkingSetAdjustment = MM_GROW_WSLE_HASH;
#if defined(_ALPHA_) && !defined(_AXP64_)
            UNLOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif
        }

        if ((WorkingSetList->NonDirectCount +
                (WorkingSetList->HashTableSize >> 4)) >
                                     WorkingSetList->HashTableSize) {

            //
            // No more room in the hash table, remove one and add there.
            // Pick a victim within 16 of where this would hash to.
            //

            KeQueryTickCount(&TickCount);
            j = Hash + (TickCount.LowPart & 0xF);

            Size = WorkingSetList->HashTableSize;

            if (j >= Size) {
                j = TickCount.LowPart & 0xF;
            }

            do {
                if (Table[j].Key != 0) {
                    PERFINFO_PAGE_INFO_DECL();

                    PointerPte = MiGetPteAddress (Table[j].Key);
                    Index = WorkingSetList->HashTable[j].Index;
                    ASSERT (Wsle[Index].u1.e1.Valid == 1);
                    PointerPte = MiGetPteAddress (Wsle[Index].u1.VirtualAddress);

                    PERFINFO_GET_PAGE_INFO(PointerPte);
                    if (MiFreeWsle (Index, WsInfo, PointerPte)) {
                        PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_HASHFULL, WsInfo);
                        break;
                    }
                }
                j += 1;
                if (j >= Size) {
                    j = 0;
                }
            } while (TRUE);
        }
    }

    //
    // Add to the hash table.
    //

    while (Table[Hash].Key != 0) {
        Hash += 1;
        if (Hash >= WorkingSetList->HashTableSize) {
            Hash = 0;
        }
    }

    Table[Hash].Key = Wsle[Entry].u1.Long & ~(PAGE_SIZE - 1);
    Table[Hash].Index = Entry;

#if DBG
    if ((MmNumberOfInserts % 1000) == 0) {
        MiCheckWsleHash (WorkingSetList);
    }
#endif //DBG
    return;
}

#if DBG
VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    )

{
    ULONG j;
    ULONG found = 0;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    for (j =0; j < WorkingSetList->HashTableSize ; j++ ) {
        if (WorkingSetList->HashTable[j].Key != 0) {
            found += 1;
            ASSERT (WorkingSetList->HashTable[j].Key ==
                     (Wsle[WorkingSetList->HashTable[j].Index].u1.Long &
                                                            ~(PAGE_SIZE -1)));
        }
    }
    if (found != WorkingSetList->NonDirectCount) {
        DbgPrint("MMWSLE: Found %lx, nondirect %lx\n",
                    found, WorkingSetList->NonDirectCount);
        DbgBreakPoint();
    }
}
#endif //dbg


WSLE_NUMBER
FASTCALL
MiLocateWsle (
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER WsPfnIndex
    )

/*++

Routine Description:

    This function locates the specified virtual address within the
    working set list.

Arguments:

    VirtualAddress - Supplies the virtual to locate within the working
                     set list.

    WorkingSetList - Supplies the working set list to search.

    WsPfnIndex - Supplies a hint to try before hashing or walking linearly.

Return Value:

    Returns the index into the working set list which contains the entry.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    WSLE_NUMBER i;
    PMMWSLE Wsle;
    ULONG Hash;
    PMMWSLE_HASH Table;
    ULONG Tries;
    WSLE_NUMBER WsPteIndex;
    PMMPTE PointerPte;

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(VirtualAddress);

#if defined (_WIN64)
    PointerPte = MiGetPteAddress (VirtualAddress);
    WsPteIndex = MI_GET_WORKING_SET_FROM_PTE (PointerPte);
    if (WsPteIndex != 0) {
        while (WsPteIndex <= WorkingSetList->LastInitializedWsle) {
            if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPteIndex].u1.VirtualAddress)) &&
                (Wsle[WsPteIndex].u1.e1.Valid == 1)) {
                    return WsPteIndex;
            }
            WsPteIndex += MI_MAXIMUM_PTE_WORKING_SET_INDEX;
        }

        //
        // No working set index for this PTE !
        //

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x41283,
                      (ULONG_PTR)VirtualAddress,
                      PointerPte->u.Long,
                      (ULONG_PTR)WorkingSetList);
    }
#endif

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) &&
            (Wsle[WsPfnIndex].u1.e1.Valid == 1)) {
            return WsPfnIndex;
        }
    }

    if (WorkingSetList->HashTable) {
        Tries = 0;
        Table = WorkingSetList->HashTable;

        Hash = MI_WSLE_HASH(VirtualAddress, WorkingSetList);

        while (Table[Hash].Key != (ULONG_PTR)VirtualAddress) {
            Hash += 1;
            if (Hash >= WorkingSetList->HashTableSize) {
                Hash = 0;
                if (Tries != 0) {
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                                  0x41284,
                                  (ULONG_PTR)VirtualAddress,
                                  WsPfnIndex,
                                  (ULONG_PTR)WorkingSetList);
                }
                Tries = 1;
            }
        }
        ASSERT (WorkingSetList->Wsle[Table[Hash].Index].u1.e1.Direct == 0);
        return Table[Hash].Index;
    }

    i = 0;

    for (; ; ) {
        if ((VirtualAddress == PAGE_ALIGN(Wsle[i].u1.VirtualAddress)) &&
            (Wsle[i].u1.e1.Valid == 1)) {
            ASSERT (WorkingSetList->Wsle[i].u1.e1.Direct == 0);
            return i;
        }
        i += 1;
    }
}


#if 0

ULONG
MiLocateWsleAndParent (
    IN PVOID VirtualAddress,
    OUT PULONG Parent,
    IN PMMWSL WorkingSetList,
    IN ULONG WsPfnIndex
    )

/*++

Routine Description:

    This routine locates both the working set list entry (via index) and
    it's parent.

Arguments:

    VirtualAddress - Supplies the virtual address of the WSLE to locate.

    Parent - Returns the index into the working set list for the parent.

    WorkingSetList - Supplies a pointer to the working set list.

    WsPfnIndex - Supplies the index field from the PFN database for
                 the physical page that maps the specified virtual address.

Return Value:

    Returns the index of the virtual address in the working set list.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    ULONG Previous;
    ULONG Entry;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    //
    // Check to see if the PfnIndex field refers to the WSLE in question.
    // Make sure the index is within the specified working set list.
    //

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if (VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) {

            //
            // The index field points to the WSLE, however, this could
            // have been just a coincidence, so check to ensure it
            // really doesn't have a parent.
            //

            if (Wsle[WsPfnIndex].u2.BothPointers == 0) {

                //
                // Not in tree, therefore has no parent.
                //

                *Parent = WSLE_NULL_INDEX;
                return WsPfnIndex;
            }
        }
    }

    //
    // Search the tree for the entry remembering the parents.
    //

    Entry = WorkingSetList->Root;
    Previous = Entry;

    for (;;) {

        ASSERT (Entry != WSLE_NULL_INDEX);

        if (VirtualAddress == PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress)) {
            break;
        }

        if (VirtualAddress < PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress)) {
            Previous = Entry;
            Entry = Wsle[Entry].u2.s.LeftChild;
        } else {
            Previous = Entry;
            Entry = Wsle[Entry].u2.s.RightChild;
        }
    }

    *Parent = Previous;
    return Entry;
}
#endif //0


VOID
FASTCALL
MiRemoveWsle (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a Working Set List Entry (WSLE) from the
    working set.

Arguments:

    Entry - The index number of the WSLE to remove.


Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/
{
    PMMWSLE Wsle;
    PVOID VirtualAddress;
    PMMWSLE_HASH Table;
    ULONG Hash;
    ULONG Tries;

    Wsle = WorkingSetList->Wsle;

    //
    // Locate the entry in the tree.
    //

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("removing wsle %lx   %lx\n",
            Entry, Wsle[Entry].u1.Long);
    }
    if (MmDebug & MM_DBG_DUMP_WSL) {
        MiDumpWsl();
        DbgPrint(" \n");
    }

#endif //DBG

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);

    VirtualAddress = PAGE_ALIGN (Wsle[Entry].u1.VirtualAddress);

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        //
        // count system space inserts and removals.
        //

#if defined(_X86_)
        if (MI_IS_SYSTEM_CACHE_ADDRESS(VirtualAddress)) {
            MmSystemCachePage -= 1;
        } else
#endif
        if (VirtualAddress < MmSystemCacheStart) {
            MmSystemCodePage -= 1;
        } else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage -= 1;
        } else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage -= 1;
        } else {
            MmSystemDriverPage -= 1;
        }
    }

    Wsle[Entry].u1.e1.Valid = 0;

    if (Wsle[Entry].u1.e1.Direct == 0) {

        WorkingSetList->NonDirectCount -= 1;

        if (WorkingSetList->HashTable) {
            Hash = MI_WSLE_HASH(Wsle[Entry].u1.Long, WorkingSetList);
            Table = WorkingSetList->HashTable;
            Tries = 0;

            while (Table[Hash].Key != (ULONG_PTR)VirtualAddress) {
                Hash += 1;
                if (Hash >= WorkingSetList->HashTableSize) {
                    Hash = 0;
                    if (Tries != 0) {
                        KeBugCheckEx (MEMORY_MANAGEMENT,
                                      0x41784,
                                      (ULONG_PTR)VirtualAddress,
                                      Entry,
                                      (ULONG_PTR)WorkingSetList);
                    }
                    Tries = 1;
                }
            }
            Table[Hash].Key = 0;

        }
    }

    return;
}


VOID
MiSwapWslEntries (
    IN WSLE_NUMBER SwapEntry,
    IN WSLE_NUMBER Entry,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine swaps the working set list entries Entry and SwapEntry
    in the specified working set list (process or system cache).

Arguments:

    SwapEntry - Supplies the first entry to swap.  This entry must be
                valid, i.e. in the working set at the current time.

    Entry - Supplies the other entry to swap.  This entry may be valid
            or invalid.

    WsInfo - Supplies the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held (if system cache),
                 APCs disabled.

--*/

{
    MMWSLE WsleEntry;
    MMWSLE WsleSwap;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;
    PMMWSLE_HASH Table;
#if DBG
    WSLE_NUMBER CurrentSize = WsInfo->WorkingSetSize;
#endif //DBG
#if PFN_CONSISTENCY
    KIRQL OldIrql;
#endif

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    WsleSwap = Wsle[SwapEntry];

    ASSERT (WsleSwap.u1.e1.Valid != 0);

    WsleEntry = Wsle[Entry];

    Table = WorkingSetList->HashTable;

    if (WsleEntry.u1.e1.Valid == 0) {

        //
        // Entry is not on any list. Remove it from the free list.
        //

        MiRemoveWsleFromFreeList (Entry, Wsle, WorkingSetList);

        //
        // Copy the Entry to this free one.
        //

        Wsle[Entry] = WsleSwap;

        PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);

        if (WsleSwap.u1.e1.Direct) {
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if PFN_CONSISTENCY
            OldIrql = 99;
            if (PFN_LOCK_OWNED_BY_ME() == 0) {
                LOCK_PFN (OldIrql);
            }
#endif
            Pfn1->u1.WsIndex = Entry;
#if PFN_CONSISTENCY
            if (OldIrql != 99) {
                UNLOCK_PFN (OldIrql);
            }
#endif
        } else {

            //
            // Update hash table.
            //

            if (Table) {
                Table [ MiLookupWsleHashIndex (WsleSwap.u1.Long,
                                           WorkingSetList)].Index = Entry;
            }
        }

        MI_SET_PTE_IN_WORKING_SET (PointerPte, Entry);

        //
        // Put entry on free list.
        //

        ASSERT (WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle);
        Wsle[SwapEntry].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
        WorkingSetList->FirstFree = SwapEntry;
        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    } else {

        //
        // Both entries are valid.
        //

        Wsle[SwapEntry] = WsleEntry;

        PointerPte = MiGetPteAddress (WsleEntry.u1.VirtualAddress);

        if (WsleEntry.u1.e1.Direct) {

            //
            // Swap the PFN WsIndex element to point to the new slot.
            //

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if PFN_CONSISTENCY
            OldIrql = 99;
            if (PFN_LOCK_OWNED_BY_ME() == 0) {
                LOCK_PFN (OldIrql);
            }
#endif
            Pfn1->u1.WsIndex = SwapEntry;
#if PFN_CONSISTENCY
            if (OldIrql != 99) {
                UNLOCK_PFN (OldIrql);
            }
#endif
        } else {

            //
            // Update hash table.
            //

            if (Table) {
                Table[ MiLookupWsleHashIndex (WsleEntry.u1.Long,
                                           WorkingSetList)].Index = SwapEntry;
            }
        }

        MI_SET_PTE_IN_WORKING_SET (PointerPte, SwapEntry);

        Wsle[Entry] = WsleSwap;

        PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);

        if (WsleSwap.u1.e1.Direct) {

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if PFN_CONSISTENCY
            OldIrql = 99;
            if (PFN_LOCK_OWNED_BY_ME() == 0) {
                LOCK_PFN (OldIrql);
            }
#endif
            Pfn1->u1.WsIndex = Entry;
#if PFN_CONSISTENCY
            if (OldIrql != 99) {
                UNLOCK_PFN (OldIrql);
            }
#endif
        } else {
            if (Table) {
                Table[ MiLookupWsleHashIndex (WsleSwap.u1.Long,
                                           WorkingSetList)].Index = Entry;
            }
        }
        MI_SET_PTE_IN_WORKING_SET (PointerPte, Entry);
    }
    ASSERT (CurrentSize == WsInfo->WorkingSetSize);
    return;
}

ULONG
MiLookupWsleHashIndex (
    IN ULONG_PTR WsleEntry,
    IN PMMWSL WorkingSetList
    )

{
    ULONG Hash;
    ULONG_PTR VirtualAddress;
    PMMWSLE_HASH Table;
    ULONG Tries = 0;

    Table = WorkingSetList->HashTable;
    VirtualAddress = WsleEntry & ~(PAGE_SIZE - 1);

    Hash = MI_WSLE_HASH(WsleEntry, WorkingSetList);

    while (Table[Hash].Key != (ULONG_PTR)VirtualAddress) {
        Hash += 1;
        if (Hash >= WorkingSetList->HashTableSize) {
            Hash = 0;
            if (Tries != 0) {
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x41884,
                              (ULONG_PTR)VirtualAddress,
                              WsleEntry,
                              (ULONG_PTR)WorkingSetList);
            }
            Tries = 1;
        }
    }
    return Hash;
}

VOID
MiRemoveWsleFromFreeList (
    IN ULONG Entry,
    IN PMMWSLE Wsle,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a working set list entry from the free list.
    It is used when the entry required is not the first element
    in the free list.

Arguments:

    Entry - Supplies the index of the entry to remove.

    Wsle - Supplies a pointer to the array of WSLEs.

    WorkingSetList - Supplies a pointer to the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APCs disabled.

--*/

{
    WSLE_NUMBER Free;
    WSLE_NUMBER ParentFree;

    Free = WorkingSetList->FirstFree;

    if (Entry == Free) {
        ASSERT ((Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT) <= WorkingSetList->LastInitializedWsle);
        WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT);

    } else {
        do {
            ParentFree = Free;
            ASSERT (Wsle[Free].u1.e1.Valid == 0);
            Free = (WSLE_NUMBER)(Wsle[Free].u1.Long >> MM_FREE_WSLE_SHIFT);
        } while (Free != Entry);

        Wsle[ParentFree].u1.Long = Wsle[Entry].u1.Long;
    }
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    return;
}


#if 0

VOID
MiSwapWslEntries (
    IN ULONG Entry,
    IN ULONG Parent,
    IN ULONG SwapEntry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This function swaps the specified entry and updates its parent with
    the specified swap entry.

    The entry must be valid, i.e., the page is resident.  The swap entry
    can be valid or on the free list.

Arguments:

    Entry - The index of the WSLE to swap.

    Parent - The index of the parent of the WSLE to swap.

    SwapEntry - The index to swap the entry with.

Return Value:

    None.

Environment:

    Kernel mode, working set mutex held, APCs disabled.

--*/

{

    ULONG SwapParent;
    ULONG SavedRight;
    ULONG SavedLeft;
    ULONG Free;
    ULONG ParentFree;
    ULONG SavedLong;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    PMMPTE PointerPte;

    Wsle = WorkingSetList->Wsle;

    if (Wsle[SwapEntry].u1.e1.Valid == 0) {

        //
        // This entry is not in use and must be removed from
        // the free list.
        //

        Free = WorkingSetList->FirstFree;

        if (SwapEntry == Free) {
            WorkingSetList->FirstFree = Entry;
            ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        } else {

            while (Free != SwapEntry) {
                ParentFree = Free;
                Free = Wsle[Free].u2.s.LeftChild;
            }

            Wsle[ParentFree].u2.s.LeftChild = Entry;
        }

        //
        // Swap the previous entry and the new unused entry.
        //

        SavedLeft = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = SavedLeft;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = 0;

        //
        // Make the parent point to the new entry.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
            return;
        }

        if (Parent == Entry) {

            //
            // This element is the root, update the root pointer.
            //

            WorkingSetList->Root = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                ASSERT (Wsle[Parent].u2.s.RightChild == Entry);

                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

    } else {

        if ((Parent == WSLE_NULL_INDEX) &&
            (Wsle[SwapEntry].u2.BothPointers == 0)) {

            //
            // Neither entry is in the tree, just swap their pointers.
            //

            SavedLong = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = SavedLong;

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;

            return;
        }

        //
        // The entry at FirstDynamic is valid; swap it with this one and
        // update both parents.
        //

        SwapParent = WorkingSetList->Root;

        if (SwapParent == SwapEntry) {

            //
            // The entry we are swapping with is at the root.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //      (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                    //
                    // The entry we are going to swap is the right child of this
                    // entry.
                    //
                    //              R(SwapEntry)
                    //             / \
                    //                (entry)
                    //

                    WorkingSetList->Root = Entry;

                    Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                    Wsle[Entry].u2.s.RightChild = SwapEntry;
                    SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                    Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                    Wsle[Entry].u2.s.LeftChild = SavedLeft;


                    SavedLong = Wsle[Entry].u1.Long;
                    Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                    Wsle[SwapEntry].u1.Long = SavedLong;

                    return;
                }
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (Parent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u1.WsIndex = SwapEntry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[Parent].u2.s.RightChild == Entry) {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            }

            return;

        }

        //
        // The SwapEntry is not the root, find its parent.
        //

        if (Wsle[SwapEntry].u2.BothPointers == 0) {

            //
            // Entry is not in tree, therefore no parent.

            SwapParent = WSLE_NULL_INDEX;

        } else {

            VirtualAddress = PAGE_ALIGN(Wsle[SwapEntry].u1.VirtualAddress);

            for (;;) {

                ASSERT (SwapParent != WSLE_NULL_INDEX);

                if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                    break;
                }
                if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                    break;
                }


                if (VirtualAddress < PAGE_ALIGN(Wsle[SwapParent].u1.VirtualAddress)) {
                    SwapParent = Wsle[SwapParent].u2.s.LeftChild;
                } else {
                    SwapParent = Wsle[SwapParent].u2.s.RightChild;
                }
            }
        }

        if (Parent == WorkingSetList->Root) {

            //
            // The entry is at the root.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //  (SwapEntry)
                //

                WorkingSetList->Root = SwapEntry;

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //                (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (SwapParent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                ASSERT (Pfn1->u1.WsIndex == SwapEntry);
                Pfn1->u1.WsIndex = Entry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            }

            return;

        }

        //
        // Neither entry is the root.
        //

        if (Parent == SwapEntry) {

            //
            // The parent of the entry is the swap entry.
            //
            //
            //              R
            //            .....
            //
            //              (SwapParent)
            //              |
            //              (SwapEntry)
            //              |
            //              (Entry)
            //

            //
            // Update the parent pointer for the swapentry.
            //

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //             (SwapParent)
                //
                //             (SwapEntry)  [Parent(entry)]
                //            / \
                //     (entry)
                //

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[SwapEntry].u2.s.RightChild == Entry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (SwapParent)
                //               \
                //                (SwapEntry)
                //               / \
                //                  (entry)
                //

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }


        }
        if (SwapParent == Entry) {


            //
            // The parent of the swap entry is the entry.
            //
            //              R
            //            .....
            //
            //              (Parent)
            //              |
            //              (Entry)
            //              |
            //              (SwapEntry)
            //

            //
            // Update the parent pointer for the entry.
            //

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (Parent)
                //              |
                //              (Entry)
                //              /
                //   (SwapEntry)
                //

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[Entry].u2.s.RightChild == SwapEntry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //                (SwapEntry)
                //

                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Entry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

        }

        //
        // Neither entry is the parent of the other.  Just swap them
        // and update the parent entries.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == Entry);
            Pfn1->u1.WsIndex = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

        if (SwapParent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == SwapEntry);
            Pfn1->u1.WsIndex = Entry;
        } else {

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }
        }

        SavedRight = Wsle[SwapEntry].u2.s.RightChild;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[Entry].u2.s.RightChild = SavedRight;
        SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = SavedLeft;

        SavedLong = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
        Wsle[SwapEntry].u1.Long = SavedLong;

        return;
    }
}
#endif //0
