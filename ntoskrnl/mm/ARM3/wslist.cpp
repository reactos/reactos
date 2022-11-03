/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD-3-Clause (https://spdx.org/licenses/BSD-3-Clause)
 * FILE:            ntoskrnl/mm/ARM3/wslist.cpp
 * PURPOSE:         Working set list management
 * PROGRAMMERS:     Jérôme Gardou
 */

/* INCLUDES *******************************************************************/
#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "miarm.h"

/* GLOBALS ********************************************************************/
PMMWSL MmWorkingSetList;
KEVENT MmWorkingSetManagerEvent;

/* LOCAL FUNCTIONS ************************************************************/

static MMPTE GetPteTemplateForWsList(PMMWSL WsList)
{
    return (WsList == MmSystemCacheWorkingSetList) ? ValidKernelPte : ValidKernelPteLocal;
}

static ULONG GetNextPageColorForWsList(PMMWSL WsList)
{
    return (WsList == MmSystemCacheWorkingSetList) ? MI_GET_NEXT_COLOR() : MI_GET_NEXT_PROCESS_COLOR(PsGetCurrentProcess());
}

static void FreeWsleIndex(PMMWSL WsList, ULONG Index)
{
    PMMWSLE Wsle = WsList->Wsle;
    ULONG& LastEntry = WsList->LastEntry;
    ULONG& FirstFree = WsList->FirstFree;
    ULONG& LastInitializedWsle = WsList->LastInitializedWsle;

    /* Erase it now */
    Wsle[Index].u1.Long = 0;

    if (Index == (LastEntry - 1))
    {
        /* We're freeing the last index of our list. */
        while (Wsle[Index].u1.e1.Valid == 0)
            Index--;

        /* Should we bother about the Free entries */
        if (FirstFree < Index)
        {
            /* Try getting the index of the last free entry */
            ASSERT(Wsle[Index + 1].u1.Free.MustBeZero == 0);
            ULONG PreviousFree = Wsle[Index + 1].u1.Free.PreviousFree;
            ASSERT(PreviousFree < LastEntry);
            ULONG LastFree = Index + 1 - PreviousFree;
#ifdef MMWSLE_PREVIOUS_FREE_JUMP
            while (Wsle[LastFree].u1.e1.Valid)
            {
                ASSERT(LastFree > MMWSLE_PREVIOUS_FREE_JUMP);
                LastFree -= MMWSLE_PREVIOUS_FREE_JUMP;
            }
#endif
            /* Update */
            ASSERT(LastFree >= FirstFree);
            Wsle[FirstFree].u1.Free.PreviousFree = (Index + 1 - LastFree) & MMWSLE_PREVIOUS_FREE_MASK;
            Wsle[LastFree].u1.Free.NextFree = 0;
        }
        else
        {
            /* No more free entries in our array */
            FirstFree = ULONG_MAX;
        }
        /* This is the new size of our array */
        LastEntry = Index + 1;
        /* Should we shrink the alloc? */
        while ((LastInitializedWsle - LastEntry) > (PAGE_SIZE / sizeof(MMWSLE)))
        {
            PMMPTE PointerPte = MiAddressToPte(Wsle + LastInitializedWsle - 1);
            /* We must not free ourself! */
            ASSERT(MiPteToAddress(PointerPte) != WsList);

            PFN_NUMBER Page = PFN_FROM_PTE(PointerPte);

            {
                ntoskrnl::MiPfnLockGuard PfnLock;

                PMMPFN Pfn = MiGetPfnEntry(Page);
                MI_SET_PFN_DELETED(Pfn);
                MiDecrementShareCount(MiGetPfnEntry(Pfn->u4.PteFrame), Pfn->u4.PteFrame);
                MiDecrementShareCount(Pfn, Page);
            }

            PointerPte->u.Long = 0;

            KeInvalidateTlbEntry(Wsle + LastInitializedWsle - 1);
            LastInitializedWsle -= PAGE_SIZE / sizeof(MMWSLE);
        }
        return;
    }

    if (FirstFree == ULONG_MAX)
    {
        /* We're the first one. */
        FirstFree = Index;
        Wsle[FirstFree].u1.Free.PreviousFree = (LastEntry - FirstFree) & MMWSLE_PREVIOUS_FREE_MASK;
        return;
    }

    /* We must find where to place ourself */
    ULONG NextFree = FirstFree;
    ULONG PreviousFree = 0;
    while (NextFree < Index)
    {
        ASSERT(Wsle[NextFree].u1.Free.MustBeZero == 0);
        if (Wsle[NextFree].u1.Free.NextFree == 0)
            break;
        PreviousFree = NextFree;
        NextFree += Wsle[NextFree].u1.Free.NextFree;
    }

    if (NextFree < Index)
    {
        /* This is actually the last free entry */
        Wsle[NextFree].u1.Free.NextFree = Index - NextFree;
        Wsle[Index].u1.Free.PreviousFree = (Index - NextFree) & MMWSLE_PREVIOUS_FREE_MASK;
        Wsle[FirstFree].u1.Free.PreviousFree = (LastEntry - Index) & MMWSLE_PREVIOUS_FREE_MASK;
        return;
    }

    if (PreviousFree == 0)
    {
        /* This is the first free */
        Wsle[Index].u1.Free.NextFree = FirstFree - Index;
        Wsle[Index].u1.Free.PreviousFree = Wsle[FirstFree].u1.Free.PreviousFree;
        Wsle[FirstFree].u1.Free.PreviousFree = (FirstFree - Index) & MMWSLE_PREVIOUS_FREE_MASK;
        FirstFree = Index;
        return;
    }

    /* Insert */
    Wsle[PreviousFree].u1.Free.NextFree = (Index - PreviousFree);
    Wsle[Index].u1.Free.PreviousFree = (Index - PreviousFree) & MMWSLE_PREVIOUS_FREE_MASK;
    Wsle[Index].u1.Free.NextFree = NextFree - Index;
    Wsle[NextFree].u1.Free.PreviousFree = (NextFree - Index) & MMWSLE_PREVIOUS_FREE_MASK;
}

static ULONG GetFreeWsleIndex(PMMWSL WsList)
{
    ULONG Index;
    if (WsList->FirstFree != ULONG_MAX)
    {
        Index = WsList->FirstFree;
        ASSERT(Index < WsList->LastInitializedWsle);
        MMWSLE_FREE_ENTRY& FreeWsle = WsList->Wsle[Index].u1.Free;
        ASSERT(FreeWsle.MustBeZero == 0);
        if (FreeWsle.NextFree != 0)
        {
            WsList->FirstFree += FreeWsle.NextFree;
            WsList->Wsle[WsList->FirstFree].u1.Free.PreviousFree = FreeWsle.PreviousFree;
        }
        else
        {
            WsList->FirstFree = ULONG_MAX;
        }
    }
    else
    {
        Index = WsList->LastEntry++;
        if (Index >= WsList->LastInitializedWsle)
        {
            /* Grow our array */
            PMMPTE PointerPte = MiAddressToPte(&WsList->Wsle[WsList->LastInitializedWsle]);
            ASSERT(PointerPte->u.Hard.Valid == 0);
            MMPTE TempPte = GetPteTemplateForWsList(WsList);
            {
                ntoskrnl::MiPfnLockGuard PfnLock;

                TempPte.u.Hard.PageFrameNumber = MiRemoveAnyPage(GetNextPageColorForWsList(WsList));
                MiInitializePfnAndMakePteValid(TempPte.u.Hard.PageFrameNumber, PointerPte, TempPte);
            }

            WsList->LastInitializedWsle += PAGE_SIZE / sizeof(MMWSLE);
        }
    }

    WsList->Wsle[Index].u1.Long = 0;
    return Index;
}

static
VOID
RemoveFromWsList(PMMWSL WsList, PVOID Address)
{
    /* Make sure that we are holding the right locks. */
    ASSERT(MM_ANY_WS_LOCK_HELD_EXCLUSIVE(PsGetCurrentThread()));

    PMMPTE PointerPte = MiAddressToPte(Address);

    /* Make sure we are removing a paged-in address */
    ASSERT(PointerPte->u.Hard.Valid == 1);
    PMMPFN Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
    ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);

    /* Shared pages not supported yet */
    ASSERT(Pfn1->u3.e1.PrototypePte == 0);

    /* Nor are "ROS PFN" */
    ASSERT(MI_IS_ROS_PFN(Pfn1) == FALSE);

    /* And we should have a valid index here */
    ASSERT(Pfn1->u1.WsIndex != 0);

    FreeWsleIndex(WsList, Pfn1->u1.WsIndex);
}

static
ULONG
TrimWsList(PMMWSL WsList)
{
    /* This should be done under WS lock */
    ASSERT(MM_ANY_WS_LOCK_HELD(PsGetCurrentThread()));

    ULONG Ret = 0;

    /* Walk the array */
    for (ULONG i = WsList->FirstDynamic; i < WsList->LastEntry; i++)
    {
        MMWSLE& Entry = WsList->Wsle[i];
        if (!Entry.u1.e1.Valid)
            continue;

        /* Only direct entries for now */
        ASSERT(Entry.u1.e1.Direct == 1);

        /* Check the PTE */
        PMMPTE PointerPte = MiAddressToPte(Entry.u1.VirtualAddress);

        /* This must be valid */
        ASSERT(PointerPte->u.Hard.Valid);

        /* If the PTE was accessed, simply reset and that's the end of it */
        if (PointerPte->u.Hard.Accessed)
        {
            Entry.u1.e1.Age = 0;
            PointerPte->u.Hard.Accessed = 0;
            KeInvalidateTlbEntry(Entry.u1.VirtualAddress);
            continue;
        }

        /* If the entry is not so old, just age it */
        if (Entry.u1.e1.Age < 3)
        {
            Entry.u1.e1.Age++;
            continue;
        }

        if ((Entry.u1.e1.LockedInMemory) || (Entry.u1.e1.LockedInWs))
        {
            /* This one is locked. Next time, maybe... */
            continue;
        }

        /* FIXME: Invalidating PDEs breaks legacy MMs */
        if (MI_IS_PAGE_TABLE_ADDRESS(Entry.u1.VirtualAddress))
            continue;

        /* Please put yourself aside and make place for the younger ones */
        PFN_NUMBER Page = PFN_FROM_PTE(PointerPte);
        {
            ntoskrnl::MiPfnLockGuard PfnLock;

            PMMPFN Pfn = MiGetPfnEntry(Page);

            /* Not supported yet */
            ASSERT(Pfn->u3.e1.PrototypePte == 0);
            ASSERT(!MI_IS_ROS_PFN(Pfn));

            /* FIXME: Remove this hack when possible */
            if (Pfn->Wsle.u1.e1.LockedInMemory || (Pfn->Wsle.u1.e1.LockedInWs))
            {
                continue;
            }

            /* We can remove it from the list. Save Protection first */
            ULONG Protection = Entry.u1.e1.Protection;
            RemoveFromWsList(WsList, Entry.u1.VirtualAddress);

            /* Dirtify the page, if needed */
            if (PointerPte->u.Hard.Dirty)
                Pfn->u3.e1.Modified = 1;

            /* Make this a transition PTE */
            MI_MAKE_TRANSITION_PTE(PointerPte, Page, Protection);
            KeInvalidateTlbEntry(MiAddressToPte(PointerPte));

            /* Drop the share count. This will take care of putting it in the standby or modified list. */
            MiDecrementShareCount(Pfn, Page);
        }

        Ret++;
    }
    return Ret;
}

/* GLOBAL FUNCTIONS ***********************************************************/
extern "C"
{

_Use_decl_annotations_
VOID
NTAPI
MiInsertInWorkingSetList(
    _Inout_ PMMSUPPORT Vm,
    _In_ PVOID Address,
    _In_ ULONG Protection)
{
    PMMWSL WsList = Vm->VmWorkingSetList;

    /* Make sure that we are holding the WS lock. */
    ASSERT(MM_ANY_WS_LOCK_HELD_EXCLUSIVE(PsGetCurrentThread()));

    PMMPTE PointerPte = MiAddressToPte(Address);

    /* Make sure we are adding a paged-in address */
    ASSERT(PointerPte->u.Hard.Valid == 1);
    PMMPFN Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
    ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);

    /* Shared pages not supported yet */
    ASSERT(Pfn1->u1.WsIndex == 0);
    ASSERT(Pfn1->u3.e1.PrototypePte == 0);

    /* Nor are "ROS PFN" */
    ASSERT(MI_IS_ROS_PFN(Pfn1) == FALSE);

    Pfn1->u1.WsIndex = GetFreeWsleIndex(WsList);
    MMWSLENTRY& NewWsle = WsList->Wsle[Pfn1->u1.WsIndex].u1.e1;
    NewWsle.VirtualPageNumber = reinterpret_cast<ULONG_PTR>(Address) >> PAGE_SHIFT;
    NewWsle.Protection = Protection;
    NewWsle.Direct = 1;
    NewWsle.Hashed = 0;
    NewWsle.LockedInMemory = 0;
    NewWsle.LockedInWs = 0;
    NewWsle.Age = 0;
    NewWsle.Valid = 1;

    Vm->WorkingSetSize += PAGE_SIZE;
    if (Vm->WorkingSetSize > Vm->PeakWorkingSetSize)
        Vm->PeakWorkingSetSize = Vm->WorkingSetSize;
}

_Use_decl_annotations_
VOID
NTAPI
MiRemoveFromWorkingSetList(
    _Inout_ PMMSUPPORT Vm,
    _In_ PVOID Address)
{
    RemoveFromWsList(Vm->VmWorkingSetList, Address);

    Vm->WorkingSetSize -= PAGE_SIZE;
}

_Use_decl_annotations_
VOID
NTAPI
MiInitializeWorkingSetList(_Inout_ PMMSUPPORT WorkingSet)
{
    PMMWSL WsList = WorkingSet->VmWorkingSetList;

    /* Initialize some fields */
    WsList->FirstFree = ULONG_MAX;
    WsList->Wsle = reinterpret_cast<PMMWSLE>(WsList + 1);
    WsList->LastEntry = 0;
    /* The first page is already allocated */
    WsList->LastInitializedWsle = (PAGE_SIZE - sizeof(*WsList)) / sizeof(MMWSLE);

    /* Insert the address we already know: our PDE base and the Working Set List */
    if (MI_IS_PROCESS_WORKING_SET(WorkingSet))
    {
        ASSERT(WorkingSet->VmWorkingSetList == MmWorkingSetList);
#if _MI_PAGING_LEVELS == 4
        MiInsertInWorkingSetList(WorkingSet, (PVOID)PXE_BASE, 0U);
#elif _MI_PAGING_LEVELS == 3
        MiInsertInWorkingSetList(WorkingSet, (PVOID)PPE_BASE, 0U);
#elif _MI_PAGING_LEVELS == 2
        MiInsertInWorkingSetList(WorkingSet, (PVOID)PDE_BASE, 0U);
#endif
    }

#if _MI_PAGING_LEVELS == 4
    MiInsertInWorkingSetList(WorkingSet, MiAddressToPpe(WorkingSet->VmWorkingSetList), 0UL);
#endif
#if _MI_PAGING_LEVELS >= 3
    MiInsertInWorkingSetList(WorkingSet, MiAddressToPde(WorkingSet->VmWorkingSetList), 0UL);
#endif
    MiInsertInWorkingSetList(WorkingSet, (PVOID)MiAddressToPte(WorkingSet->VmWorkingSetList), 0UL);
    MiInsertInWorkingSetList(WorkingSet, (PVOID)WorkingSet->VmWorkingSetList, 0UL);

    /* From now on, every added page can be trimmed at any time */
    WsList->FirstDynamic = WsList->LastEntry;

    /* We can add this to our list */
    ExInterlockedInsertTailList(&MmWorkingSetExpansionHead, &WorkingSet->WorkingSetExpansionLinks, &MmExpansionLock);
}

VOID
NTAPI
MmWorkingSetManager(VOID)
{
    PLIST_ENTRY VmListEntry;
    PMMSUPPORT Vm = NULL;
    KIRQL OldIrql;

    OldIrql = MiAcquireExpansionLock();

    for (VmListEntry = MmWorkingSetExpansionHead.Flink;
         VmListEntry != &MmWorkingSetExpansionHead;
         VmListEntry = VmListEntry->Flink)
    {
        BOOLEAN TrimHard = MmAvailablePages < MmMinimumFreePages;
        PEPROCESS Process = NULL;

        /* Don't do anything if we have plenty of free pages. */
        if ((MmAvailablePages + MmModifiedPageListHead.Total) >= MmPlentyFreePages)
            break;

        Vm = CONTAINING_RECORD(VmListEntry, MMSUPPORT, WorkingSetExpansionLinks);

        /* Let the legacy Mm System space alone */
        if (Vm == MmGetKernelAddressSpace())
            continue;

        if (MI_IS_PROCESS_WORKING_SET(Vm))
        {
            Process = CONTAINING_RECORD(Vm, EPROCESS, Vm);

            /* Make sure the process is not terminating abd attach to it */
            if (!ExAcquireRundownProtection(&Process->RundownProtect))
                continue;
            ASSERT(!KeIsAttachedProcess());
            KeAttachProcess(&Process->Pcb);
        }
        else
        {
            /* FIXME: Session & system space unsupported */
            continue;
        }

        MiReleaseExpansionLock(OldIrql);

        /* Share-lock for now, we're only reading */
        MiLockWorkingSetShared(PsGetCurrentThread(), Vm);

        if (((Vm->WorkingSetSize > Vm->MaximumWorkingSetSize) ||
            (TrimHard && (Vm->WorkingSetSize > Vm->MinimumWorkingSetSize))) &&
            MiConvertSharedWorkingSetLockToExclusive(PsGetCurrentThread(), Vm))
        {
            /* We're done */
            Vm->Flags.BeingTrimmed = 1;

            ULONG Trimmed = TrimWsList(Vm->VmWorkingSetList);

            /* We're done */
            Vm->WorkingSetSize -= Trimmed * PAGE_SIZE;
            Vm->Flags.BeingTrimmed = 0;
            MiUnlockWorkingSet(PsGetCurrentThread(), Vm);
        }
        else
        {
            MiUnlockWorkingSetShared(PsGetCurrentThread(), Vm);
        }

        /* Lock again */
        OldIrql = MiAcquireExpansionLock();

        if (Process)
        {
            KeDetachProcess();
            ExReleaseRundownProtection(&Process->RundownProtect);
        }
    }

    MiReleaseExpansionLock(OldIrql);
}

} // extern "C"
