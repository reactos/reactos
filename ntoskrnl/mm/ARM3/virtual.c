/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/virtual.c
 * PURPOSE:         ARM Memory Manager Virtual Memory Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

#define MI_MAPPED_COPY_PAGES  14
#define MI_POOL_COPY_BYTES    512
#define MI_MAX_TRANSFER_SIZE  64 * 1024

NTSTATUS NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PSIZE_T NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection  OPTIONAL);

VOID
NTAPI
MiFlushTbAndCapture(IN PMMVAD FoundVad,
                    IN PMMPTE PointerPte,
                    IN ULONG ProtectionMask,
                    IN PMMPFN Pfn1,
                    IN BOOLEAN CaptureDirtyBit);


/* PRIVATE FUNCTIONS **********************************************************/

ULONG
NTAPI
MiCalculatePageCommitment(IN ULONG_PTR StartingAddress,
                          IN ULONG_PTR EndingAddress,
                          IN PMMVAD Vad,
                          IN PEPROCESS Process)
{
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
    BOOLEAN OnPdeBoundary = TRUE;
#if _MI_PAGING_LEVELS >= 3
    PMMPPE PointerPpe;
    BOOLEAN OnPpeBoundary = TRUE;
#if _MI_PAGING_LEVELS == 4
    PMMPXE PointerPxe;
    BOOLEAN OnPxeBoundary = TRUE;
#endif
#endif

    /* Make sure this all makes sense */
    ASSERT(PsGetCurrentThread()->OwnsProcessWorkingSetExclusive || PsGetCurrentThread()->OwnsProcessWorkingSetShared);
    ASSERT(EndingAddress >= StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    LastPte = MiAddressToPte(EndingAddress);

    /*
     * In case this is a committed VAD, assume the whole range is committed
     * and count the individually decommitted pages.
     * In case it is not, assume the range is not committed and count the individually committed pages.
     */
    ULONG_PTR CommittedPages = Vad->u.VadFlags.MemCommit ? BYTES_TO_PAGES(EndingAddress - StartingAddress) : 0;

    while (PointerPte <= LastPte)
    {
#if _MI_PAGING_LEVELS == 4
        /* Check if PXE was ever paged in. */
        if (OnPxeBoundary)
        {
            PointerPxe = MiPteToPxe(PointerPte);

            /* Check that this loop is sane */
            ASSERT(OnPpeBoundary);
            ASSERT(OnPdeBoundary);

            if (PointerPxe->u.Long == 0)
            {
                PointerPxe++;
                PointerPte = MiPxeToPte(PointerPde);
                continue;
            }

            if (PointerPxe->u.Hard.Valid == 0)
                MiMakeSystemAddressValid(MiPteToPpe(PointerPte), Process);
        }
        ASSERT(PointerPxe->u.Hard.Valid == 1);
#endif

#if _MI_PAGING_LEVELS >= 3
        /* Now PPE */
        if (OnPpeBoundary)
        {
            PointerPpe = MiPteToPpe(PointerPte);

            /* Sanity again */
            ASSERT(OnPdeBoundary);

            if (PointerPpe->u.Long == 0)
            {
                PointerPpe++;
                PointerPte = MiPpeToPte(PointerPpe);
#if _MI_PAGING_LEVELS == 4
                OnPxeBoundary = MiIsPteOnPxeBoundary(PointerPte);
#endif
                continue;
            }

            if (PointerPpe->u.Hard.Valid == 0)
                MiMakeSystemAddressValid(MiPteToPde(PointerPte), Process);
        }
        ASSERT(PointerPpe->u.Hard.Valid == 1);
#endif

        /* Last level is the PDE */
        if (OnPdeBoundary)
        {
            PointerPde = MiPteToPde(PointerPte);
            if (PointerPde->u.Long == 0)
            {
                PointerPde++;
                PointerPte = MiPdeToPte(PointerPde);
#if _MI_PAGING_LEVELS >= 3
                OnPpeBoundary = MiIsPteOnPpeBoundary(PointerPte);
#if _MI_PAGING_LEVELS == 4
                OnPxeBoundary = MiIsPteOnPxeBoundary(PointerPte);
#endif
#endif
                continue;
            }

            if (PointerPde->u.Hard.Valid == 0)
                MiMakeSystemAddressValid(PointerPte, Process);
        }
        ASSERT(PointerPde->u.Hard.Valid == 1);

        /* Is this PTE demand zero? */
        if (PointerPte->u.Long != 0)
        {
            /* It isn't -- is it a decommited, invalid, or faulted PTE? */
            if ((PointerPte->u.Hard.Valid == 0) &&
                (PointerPte->u.Soft.Protection == MM_DECOMMIT) &&
                ((PointerPte->u.Soft.Prototype == 0) ||
                    (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)))
            {
                /* It is, so remove it from the count of committed pages if we have to */
                if (Vad->u.VadFlags.MemCommit)
                    CommittedPages--;
            }
            else if (!Vad->u.VadFlags.MemCommit)
            {
                /* It is a valid, non-decommited, non-paged out PTE. Count it in. */
                CommittedPages++;
            }
        }

        /* Move to the next PTE */
        PointerPte++;
        /* Manage page tables */
        OnPdeBoundary = MiIsPteOnPdeBoundary(PointerPte);
#if _MI_PAGING_LEVELS >= 3
        OnPpeBoundary = MiIsPteOnPpeBoundary(PointerPte);
#if _MI_PAGING_LEVELS == 4
        OnPxeBoundary = MiIsPteOnPxeBoundary(PointerPte);
#endif
#endif
    }

    /* Make sure we didn't mess this up */
    ASSERT(CommittedPages <= BYTES_TO_PAGES(EndingAddress - StartingAddress));
    return CommittedPages;
}

ULONG
NTAPI
MiMakeSystemAddressValid(IN PVOID PageTableVirtualAddress,
                         IN PEPROCESS CurrentProcess)
{
    NTSTATUS Status;
    BOOLEAN WsShared = FALSE, WsSafe = FALSE, LockChange = FALSE;
    PETHREAD CurrentThread = PsGetCurrentThread();

    /* Must be a non-pool page table, since those are double-mapped already */
    ASSERT(PageTableVirtualAddress > MM_HIGHEST_USER_ADDRESS);
    ASSERT((PageTableVirtualAddress < MmPagedPoolStart) ||
           (PageTableVirtualAddress > MmPagedPoolEnd));

    /* Working set lock or PFN lock should be held */
    ASSERT(KeAreAllApcsDisabled() == TRUE);

    /* Check if the page table is valid */
    while (!MmIsAddressValid(PageTableVirtualAddress))
    {
        /* Release the working set lock */
        MiUnlockProcessWorkingSetForFault(CurrentProcess,
                                          CurrentThread,
                                          &WsSafe,
                                          &WsShared);

        /* Fault it in */
        Status = MmAccessFault(FALSE, PageTableVirtualAddress, KernelMode, NULL);
        if (!NT_SUCCESS(Status))
        {
            /* This should not fail */
            KeBugCheckEx(KERNEL_DATA_INPAGE_ERROR,
                         1,
                         Status,
                         (ULONG_PTR)CurrentProcess,
                         (ULONG_PTR)PageTableVirtualAddress);
        }

        /* Lock the working set again */
        MiLockProcessWorkingSetForFault(CurrentProcess,
                                        CurrentThread,
                                        WsSafe,
                                        WsShared);

        /* This flag will be useful later when we do better locking */
        LockChange = TRUE;
    }

    /* Let caller know what the lock state is */
    return LockChange;
}

ULONG
NTAPI
MiMakeSystemAddressValidPfn(IN PVOID VirtualAddress,
                            IN KIRQL OldIrql)
{
    NTSTATUS Status;
    BOOLEAN LockChange = FALSE;

    /* Must be e kernel address */
    ASSERT(VirtualAddress > MM_HIGHEST_USER_ADDRESS);

    /* Check if the page is valid */
    while (!MmIsAddressValid(VirtualAddress))
    {
        /* Release the PFN database */
        MiReleasePfnLock(OldIrql);

        /* Fault it in */
        Status = MmAccessFault(FALSE, VirtualAddress, KernelMode, NULL);
        if (!NT_SUCCESS(Status))
        {
            /* This should not fail */
            KeBugCheckEx(KERNEL_DATA_INPAGE_ERROR,
                         3,
                         Status,
                         0,
                         (ULONG_PTR)VirtualAddress);
        }

        /* This flag will be useful later when we do better locking */
        LockChange = TRUE;

        /* Lock the PFN database */
        OldIrql = MiAcquirePfnLock();
    }

    /* Let caller know what the lock state is */
    return LockChange;
}

PFN_COUNT
NTAPI
MiDeleteSystemPageableVm(IN PMMPTE PointerPte,
                         IN PFN_NUMBER PageCount,
                         IN ULONG Flags,
                         OUT PPFN_NUMBER ValidPages)
{
    PFN_COUNT ActualPages = 0;
    PETHREAD CurrentThread = PsGetCurrentThread();
    PMMPFN Pfn1, Pfn2;
    PFN_NUMBER PageFrameIndex, PageTableIndex;
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Lock the system working set */
    MiLockWorkingSet(CurrentThread, &MmSystemCacheWs);

    /* Loop all pages */
    while (PageCount)
    {
        /* Make sure there's some data about the page */
        if (PointerPte->u.Long)
        {
            /* Normally this is one possibility -- freeing a valid page */
            if (PointerPte->u.Hard.Valid)
            {
                /* Get the page PFN */
                PageFrameIndex = PFN_FROM_PTE(PointerPte);
                Pfn1 = MiGetPfnEntry(PageFrameIndex);

                /* Should not have any working set data yet */
                ASSERT(Pfn1->u1.WsIndex == 0);

                /* Actual valid, legitimate, pages */
                if (ValidPages) (*ValidPages)++;

                /* Get the page table entry */
                PageTableIndex = Pfn1->u4.PteFrame;
                Pfn2 = MiGetPfnEntry(PageTableIndex);

                /* Lock the PFN database */
                OldIrql = MiAcquirePfnLock();

                /* Delete it the page */
                MI_SET_PFN_DELETED(Pfn1);
                MiDecrementShareCount(Pfn1, PageFrameIndex);

                /* Decrement the page table too */
                MiDecrementShareCount(Pfn2, PageTableIndex);

                /* Release the PFN database */
                MiReleasePfnLock(OldIrql);

                /* Destroy the PTE */
                MI_ERASE_PTE(PointerPte);
            }
            else
            {
                /* As always, only handle current ARM3 scenarios */
                ASSERT(PointerPte->u.Soft.Prototype == 0);
                ASSERT(PointerPte->u.Soft.Transition == 0);

                /*
                 * The only other ARM3 possibility is a demand zero page, which would
                 * mean freeing some of the paged pool pages that haven't even been
                 * touched yet, as part of a larger allocation.
                 *
                 * Right now, we shouldn't expect any page file information in the PTE
                 */
                ASSERT(PointerPte->u.Soft.PageFileHigh == 0);

                /* Destroy the PTE */
                MI_ERASE_PTE(PointerPte);
            }

            /* Actual legitimate pages */
            ActualPages++;
        }

        /* Keep going */
        PointerPte++;
        PageCount--;
    }

    /* Release the working set */
    MiUnlockWorkingSet(CurrentThread, &MmSystemCacheWs);

    /* Flush the entire TLB */
    KeFlushEntireTb(TRUE, TRUE);

    /* Done */
    return ActualPages;
}

VOID
NTAPI
MiDeletePte(IN PMMPTE PointerPte,
            IN PVOID VirtualAddress,
            IN PEPROCESS CurrentProcess,
            IN PMMPTE PrototypePte)
{
    PMMPFN Pfn1;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPDE PointerPde;

    /* PFN lock must be held */
    MI_ASSERT_PFN_LOCK_HELD();

    /* WorkingSet must be exclusively locked */
    ASSERT(MM_ANY_WS_LOCK_HELD_EXCLUSIVE(PsGetCurrentThread()));

    /* This must be current process. */
    ASSERT(CurrentProcess == PsGetCurrentProcess());

    /* Capture the PTE */
    TempPte = *PointerPte;

    /* See if the PTE is valid */
    if (TempPte.u.Hard.Valid == 0)
    {
        /* Prototype and paged out PTEs not supported yet */
        ASSERT(TempPte.u.Soft.Prototype == 0);
        ASSERT((TempPte.u.Soft.PageFileHigh == 0) || (TempPte.u.Soft.Transition == 1));

        if (TempPte.u.Soft.Transition)
        {
            /* Get the PFN entry */
            PageFrameIndex = PFN_FROM_PTE(&TempPte);
            Pfn1 = MiGetPfnEntry(PageFrameIndex);

            DPRINT("Pte %p is transitional!\n", PointerPte);

            /* Make sure the saved PTE address is valid */
            ASSERT((PMMPTE)((ULONG_PTR)Pfn1->PteAddress & ~0x1) == PointerPte);

            /* Destroy the PTE */
            MI_ERASE_PTE(PointerPte);

            /* Drop the reference on the page table. */
            MiDecrementShareCount(MiGetPfnEntry(Pfn1->u4.PteFrame), Pfn1->u4.PteFrame);

            /* In case of shared page, the prototype PTE must be in transition, not the process one */
            ASSERT(Pfn1->u3.e1.PrototypePte == 0);

            /* Delete the PFN */
            MI_SET_PFN_DELETED(Pfn1);

            /* It must be either free (refcount == 0) or being written (refcount == 1) */
            ASSERT(Pfn1->u3.e2.ReferenceCount == Pfn1->u3.e1.WriteInProgress);

            /* See if we must free it ourselves, or if it will be freed once I/O is over */
            if (Pfn1->u3.e2.ReferenceCount == 0)
            {
                /* And it should be in standby or modified list */
                ASSERT((Pfn1->u3.e1.PageLocation == ModifiedPageList) || (Pfn1->u3.e1.PageLocation == StandbyPageList));

                /* Unlink it and set its reference count to one */
                MiUnlinkPageFromList(Pfn1);
                Pfn1->u3.e2.ReferenceCount++;

                /* This will put it back in free list and clean properly up */
                MiDecrementReferenceCount(Pfn1, PageFrameIndex);
            }
            return;
        }
    }

    /* Get the PFN entry */
    PageFrameIndex = PFN_FROM_PTE(&TempPte);
    Pfn1 = MiGetPfnEntry(PageFrameIndex);

    /* Check if this is a valid, prototype PTE */
    if (Pfn1->u3.e1.PrototypePte == 1)
    {
        /* Get the PDE and make sure it's faulted in */
        PointerPde = MiPteToPde(PointerPte);
        if (PointerPde->u.Hard.Valid == 0)
        {
#if (_MI_PAGING_LEVELS == 2)
            /* Could be paged pool access from a new process -- synchronize the page directories */
            if (!NT_SUCCESS(MiCheckPdeForPagedPool(VirtualAddress)))
            {
#endif
                /* The PDE must be valid at this point */
                KeBugCheckEx(MEMORY_MANAGEMENT,
                             0x61940,
                             (ULONG_PTR)PointerPte,
                             PointerPte->u.Long,
                             (ULONG_PTR)VirtualAddress);
            }
#if (_MI_PAGING_LEVELS == 2)
        }
#endif
        /* Drop the share count on the page table */
        PointerPde = MiPteToPde(PointerPte);
        MiDecrementShareCount(MiGetPfnEntry(PointerPde->u.Hard.PageFrameNumber),
            PointerPde->u.Hard.PageFrameNumber);

        /* Drop the share count */
        MiDecrementShareCount(Pfn1, PageFrameIndex);

        /* Either a fork, or this is the shared user data page */
        if ((PointerPte <= MiHighestUserPte) && (PrototypePte != Pfn1->PteAddress))
        {
            /* If it's not the shared user page, then crash, since there's no fork() yet */
            if ((PAGE_ALIGN(VirtualAddress) != (PVOID)USER_SHARED_DATA) ||
                 (MmHighestUserAddress <= (PVOID)USER_SHARED_DATA))
            {
                /* Must be some sort of memory corruption */
                KeBugCheckEx(MEMORY_MANAGEMENT,
                             0x400,
                             (ULONG_PTR)PointerPte,
                             (ULONG_PTR)PrototypePte,
                             (ULONG_PTR)Pfn1->PteAddress);
            }
        }

        /* Erase it */
        MI_ERASE_PTE(PointerPte);
    }
    else
    {
        /* Make sure the saved PTE address is valid */
        if ((PMMPTE)((ULONG_PTR)Pfn1->PteAddress & ~0x1) != PointerPte)
        {
            /* The PFN entry is illegal, or invalid */
            KeBugCheckEx(MEMORY_MANAGEMENT,
                         0x401,
                         (ULONG_PTR)PointerPte,
                         PointerPte->u.Long,
                         (ULONG_PTR)Pfn1->PteAddress);
        }

        /* Erase the PTE */
        MI_ERASE_PTE(PointerPte);

        /* There should only be 1 shared reference count */
        ASSERT(Pfn1->u2.ShareCount == 1);

        /* Drop the reference on the page table. */
        MiDecrementShareCount(MiGetPfnEntry(Pfn1->u4.PteFrame), Pfn1->u4.PteFrame);

        /* Mark the PFN for deletion and dereference what should be the last ref */
        MI_SET_PFN_DELETED(Pfn1);
        MiDecrementShareCount(Pfn1, PageFrameIndex);

        /* We should eventually do this */
        //CurrentProcess->NumberOfPrivatePages--;
    }

    /* Flush the TLB */
    KeFlushCurrentTb();
}

VOID
NTAPI
MiDeleteVirtualAddresses(IN ULONG_PTR Va,
                         IN ULONG_PTR EndingAddress,
                         IN PMMVAD Vad)
{
    PMMPTE PointerPte, PrototypePte, LastPrototypePte;
    PMMPDE PointerPde;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPPE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PMMPPE PointerPxe;
#endif
    MMPTE TempPte;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    BOOLEAN AddressGap = FALSE;
    PSUBSECTION Subsection;

    /* Get out if this is a fake VAD, RosMm will free the marea pages */
    if ((Vad) && (Vad->u.VadFlags.Spare == 1)) return;

    /* Get the current process */
    CurrentProcess = PsGetCurrentProcess();

    /* Check if this is a section VAD or a VM VAD */
    if (!(Vad) || (Vad->u.VadFlags.PrivateMemory) || !(Vad->FirstPrototypePte))
    {
        /* Don't worry about prototypes */
        PrototypePte = LastPrototypePte = NULL;
    }
    else
    {
        /* Get the prototype PTE */
        PrototypePte = Vad->FirstPrototypePte;
        LastPrototypePte = Vad->FirstPrototypePte + 1;
    }

    /* In all cases, we don't support fork() yet */
    ASSERT(CurrentProcess->CloneRoot == NULL);

    /* Loop the PTE for each VA (EndingAddress is inclusive!) */
    while (Va <= EndingAddress)
    {
#if (_MI_PAGING_LEVELS >= 4)
        /* Get the PXE and check if it's valid */
        PointerPxe = MiAddressToPxe((PVOID)Va);
        if (!PointerPxe->u.Hard.Valid)
        {
            /* Check for unmapped range and skip it */
            if (!PointerPxe->u.Long)
            {
                /* There are gaps in the address space */
                AddressGap = TRUE;

                /* Update Va and continue looping */
                Va = (ULONG_PTR)MiPxeToAddress(PointerPxe + 1);
                continue;
            }

            /* Make the PXE valid */
            MiMakeSystemAddressValid(MiPteToAddress(PointerPxe), CurrentProcess);
        }
#endif
#if (_MI_PAGING_LEVELS >= 3)
        /* Get the PPE and check if it's valid */
        PointerPpe = MiAddressToPpe((PVOID)Va);
        if (!PointerPpe->u.Hard.Valid)
        {
            /* Check for unmapped range and skip it */
            if (!PointerPpe->u.Long)
            {
                /* There are gaps in the address space */
                AddressGap = TRUE;

                /* Update Va and continue looping */
                Va = (ULONG_PTR)MiPpeToAddress(PointerPpe + 1);
                continue;
            }

            /* Make the PPE valid */
            MiMakeSystemAddressValid(MiPteToAddress(PointerPpe), CurrentProcess);
        }
#endif
        /* Skip invalid PDEs */
        PointerPde = MiAddressToPde((PVOID)Va);
        if (!PointerPde->u.Long)
        {
            /* There are gaps in the address space */
            AddressGap = TRUE;

            /* Check if all the PDEs are invalid, so there's nothing to free */
            Va = (ULONG_PTR)MiPdeToAddress(PointerPde + 1);
            continue;
        }

        /* Now check if the PDE is mapped in */
        if (!PointerPde->u.Hard.Valid)
        {
            /* It isn't, so map it in */
            PointerPte = MiPteToAddress(PointerPde);
            MiMakeSystemAddressValid(PointerPte, CurrentProcess);
        }

        /* Now we should have a valid PDE, mapped in, and still have some VA */
        ASSERT(PointerPde->u.Hard.Valid == 1);
        ASSERT(Va <= EndingAddress);

        /* Check if this is a section VAD with gaps in it */
        if ((AddressGap) && (LastPrototypePte))
        {
            /* We need to skip to the next correct prototype PTE */
            PrototypePte = MI_GET_PROTOTYPE_PTE_FOR_VPN(Vad, Va >> PAGE_SHIFT);

            /* And we need the subsection to skip to the next last prototype PTE */
            Subsection = MiLocateSubsection(Vad, Va >> PAGE_SHIFT);
            if (Subsection)
            {
                /* Found it! */
                LastPrototypePte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
            }
            else
            {
                /* No more subsections, we are done with prototype PTEs */
                PrototypePte = NULL;
            }
        }

        /* Lock the PFN Database while we delete the PTEs */
        OldIrql = MiAcquirePfnLock();
        PointerPte = MiAddressToPte(Va);
        do
        {
            /* Making sure the PDE is still valid */
            ASSERT(PointerPde->u.Hard.Valid == 1);

            /* Capture the PDE and make sure it exists */
            TempPte = *PointerPte;
            if (TempPte.u.Long)
            {
                /* Check if the PTE is actually mapped in */
                if (MI_IS_MAPPED_PTE(&TempPte))
                {
                    /* Are we dealing with section VAD? */
                    if ((LastPrototypePte) && (PrototypePte > LastPrototypePte))
                    {
                        /* We need to skip to the next correct prototype PTE */
                        PrototypePte = MI_GET_PROTOTYPE_PTE_FOR_VPN(Vad, Va >> PAGE_SHIFT);

                        /* And we need the subsection to skip to the next last prototype PTE */
                        Subsection = MiLocateSubsection(Vad, Va >> PAGE_SHIFT);
                        if (Subsection)
                        {
                            /* Found it! */
                            LastPrototypePte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
                        }
                        else
                        {
                            /* No more subsections, we are done with prototype PTEs */
                            PrototypePte = NULL;
                        }
                    }

                    /* Check for prototype PTE */
                    if ((TempPte.u.Hard.Valid == 0) &&
                        (TempPte.u.Soft.Prototype == 1))
                    {
                        /* Just nuke it */
                        MI_ERASE_PTE(PointerPte);
                    }
                    else
                    {
                        /* Delete the PTE proper */
                        MiDeletePte(PointerPte,
                                    (PVOID)Va,
                                    CurrentProcess,
                                    PrototypePte);
                    }
                }
                else
                {
                    /* The PTE was never mapped, just nuke it here */
                    MI_ERASE_PTE(PointerPte);
                }

                if (MiDecrementPageTableReferences((PVOID)Va) == 0)
                {
                    ASSERT(PointerPde->u.Long != 0);
                    /* Delete the PDE proper */
                    MiDeletePde(PointerPde, CurrentProcess);
                    /* Jump */
                    Va = (ULONG_PTR)MiPdeToAddress(PointerPde + 1);
                    break;
                }
            }

            /* Update the address and PTE for it */
            Va += PAGE_SIZE;
            PointerPte++;
            PrototypePte++;
        } while ((Va & (PDE_MAPPED_VA - 1)) && (Va <= EndingAddress));

        /* Release the lock */
        MiReleasePfnLock(OldIrql);

        if (Va > EndingAddress) return;

        /* Otherwise, we exited because we hit a new PDE boundary, so start over */
        AddressGap = FALSE;
    }
}

LONG
MiGetExceptionInfo(IN PEXCEPTION_POINTERS ExceptionInfo,
                   OUT PBOOLEAN HaveBadAddress,
                   OUT PULONG_PTR BadAddress)
{
    PEXCEPTION_RECORD ExceptionRecord;
    PAGED_CODE();

    //
    // Assume default
    //
    *HaveBadAddress = FALSE;

    //
    // Get the exception record
    //
    ExceptionRecord = ExceptionInfo->ExceptionRecord;

    //
    // Look at the exception code
    //
    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR))
    {
        //
        // We can tell the address if we have more than one parameter
        //
        if (ExceptionRecord->NumberParameters > 1)
        {
            //
            // Return the address
            //
            *HaveBadAddress = TRUE;
            *BadAddress = ExceptionRecord->ExceptionInformation[1];
        }
    }

    //
    // Continue executing the next handler
    //
    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
NTAPI
MiDoMappedCopy(IN PEPROCESS SourceProcess,
               IN PVOID SourceAddress,
               IN PEPROCESS TargetProcess,
               OUT PVOID TargetAddress,
               IN SIZE_T BufferSize,
               IN KPROCESSOR_MODE PreviousMode,
               OUT PSIZE_T ReturnSize)
{
    PFN_NUMBER MdlBuffer[(sizeof(MDL) / sizeof(PFN_NUMBER)) + MI_MAPPED_COPY_PAGES + 1];
    PMDL Mdl = (PMDL)MdlBuffer;
    SIZE_T TotalSize, CurrentSize, RemainingSize;
    volatile BOOLEAN FailedInProbe = FALSE;
    volatile BOOLEAN PagesLocked = FALSE;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    volatile PVOID MdlAddress = NULL;
    KAPC_STATE ApcState;
    BOOLEAN HaveBadAddress;
    ULONG_PTR BadAddress;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    //
    // Calculate the maximum amount of data to move
    //
    TotalSize = MI_MAPPED_COPY_PAGES * PAGE_SIZE;
    if (BufferSize <= TotalSize) TotalSize = BufferSize;
    CurrentSize = TotalSize;
    RemainingSize = BufferSize;

    //
    // Loop as long as there is still data
    //
    while (RemainingSize > 0)
    {
        //
        // Check if this transfer will finish everything off
        //
        if (RemainingSize < CurrentSize) CurrentSize = RemainingSize;

        //
        // Attach to the source address space
        //
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

        //
        // Check state for this pass
        //
        ASSERT(MdlAddress == NULL);
        ASSERT(PagesLocked == FALSE);
        ASSERT(FailedInProbe == FALSE);

        //
        // Protect user-mode copy
        //
        _SEH2_TRY
        {
            //
            // If this is our first time, probe the buffer
            //
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForRead(SourceAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Initialize and probe and lock the MDL
            //
            MmInitializeMdl(Mdl, CurrentAddress, CurrentSize);
            MmProbeAndLockPages(Mdl, PreviousMode, IoReadAccess);
            PagesLocked = TRUE;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        /* Detach from source process */
        KeUnstackDetachProcess(&ApcState);

        if (Status != STATUS_SUCCESS)
        {
            goto Exit;
        }

        //
        // Now map the pages
        //
        MdlAddress = MmMapLockedPagesSpecifyCache(Mdl,
                                                  KernelMode,
                                                  MmCached,
                                                  NULL,
                                                  FALSE,
                                                  HighPagePriority);
        if (!MdlAddress)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        //
        // Grab to the target process
        //
        KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

        _SEH2_TRY
        {
            //
            // Check if this is our first time through
            //
            if ((CurrentTargetAddress == TargetAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Now do the actual move
            //
            RtlCopyMemory(CurrentTargetAddress, MdlAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            *ReturnSize = BufferSize - RemainingSize;
            //
            // Check if we failed during the probe
            //
            if (FailedInProbe)
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
            }
            else
            {
                //
                // Othewise we failed during the move.
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
                //
                // Return partial copy
                //
                Status = STATUS_PARTIAL_COPY;
            }
        }
        _SEH2_END;

        /* Detach from target process */
        KeUnstackDetachProcess(&ApcState);

        //
        // Check for SEH status
        //
        if (Status != STATUS_SUCCESS)
        {
            goto Exit;
        }

        //
        // Unmap and unlock
        //
        MmUnmapLockedPages(MdlAddress, Mdl);
        MdlAddress = NULL;
        MmUnlockPages(Mdl);
        PagesLocked = FALSE;

        //
        // Update location and size
        //
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress + CurrentSize);
    }

Exit:
    if (MdlAddress != NULL)
        MmUnmapLockedPages(MdlAddress, Mdl);
    if (PagesLocked)
        MmUnlockPages(Mdl);

    //
    // All bytes read
    //
    if (Status == STATUS_SUCCESS)
        *ReturnSize = BufferSize;
    return Status;
}

NTSTATUS
NTAPI
MiDoPoolCopy(IN PEPROCESS SourceProcess,
             IN PVOID SourceAddress,
             IN PEPROCESS TargetProcess,
             OUT PVOID TargetAddress,
             IN SIZE_T BufferSize,
             IN KPROCESSOR_MODE PreviousMode,
             OUT PSIZE_T ReturnSize)
{
    UCHAR StackBuffer[MI_POOL_COPY_BYTES];
    SIZE_T TotalSize, CurrentSize, RemainingSize;
    volatile BOOLEAN FailedInProbe = FALSE, HavePoolAddress = FALSE;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    PVOID PoolAddress;
    KAPC_STATE ApcState;
    BOOLEAN HaveBadAddress;
    ULONG_PTR BadAddress;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    DPRINT("Copying %Iu bytes from process %p (address %p) to process %p (Address %p)\n",
        BufferSize, SourceProcess, SourceAddress, TargetProcess, TargetAddress);

    //
    // Calculate the maximum amount of data to move
    //
    TotalSize = MI_MAX_TRANSFER_SIZE;
    if (BufferSize <= MI_MAX_TRANSFER_SIZE) TotalSize = BufferSize;
    CurrentSize = TotalSize;
    RemainingSize = BufferSize;

    //
    // Check if we can use the stack
    //
    if (BufferSize <= MI_POOL_COPY_BYTES)
    {
        //
        // Use it
        //
        PoolAddress = (PVOID)StackBuffer;
    }
    else
    {
        //
        // Allocate pool
        //
        PoolAddress = ExAllocatePoolWithTag(NonPagedPool, TotalSize, 'VmRw');
        if (!PoolAddress) ASSERT(FALSE);
        HavePoolAddress = TRUE;
    }

    //
    // Loop as long as there is still data
    //
    while (RemainingSize > 0)
    {
        //
        // Check if this transfer will finish everything off
        //
        if (RemainingSize < CurrentSize) CurrentSize = RemainingSize;

        //
        // Attach to the source address space
        //
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

        /* Check that state is sane */
        ASSERT(FailedInProbe == FALSE);
        ASSERT(Status == STATUS_SUCCESS);

        //
        // Protect user-mode copy
        //
        _SEH2_TRY
        {
            //
            // If this is our first time, probe the buffer
            //
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForRead(SourceAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Do the copy
            //
            RtlCopyMemory(PoolAddress, CurrentAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            *ReturnSize = BufferSize - RemainingSize;

            //
            // Check if we failed during the probe
            //
            if (FailedInProbe)
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
            }
            else
            {
                //
                // We failed during the move.
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
                //
                // Return partial copy
                //
                Status = STATUS_PARTIAL_COPY;
            }
        }
        _SEH2_END

        /* Let go of the source */
        KeUnstackDetachProcess(&ApcState);

        if (Status != STATUS_SUCCESS)
        {
            goto Exit;
        }

        /* Grab the target process */
        KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

        _SEH2_TRY
        {
            //
            // Check if this is our first time through
            //
            if ((CurrentTargetAddress == TargetAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Now do the actual move
            //
            RtlCopyMemory(CurrentTargetAddress, PoolAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            *ReturnSize = BufferSize - RemainingSize;
            //
            // Check if we failed during the probe
            //
            if (FailedInProbe)
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
            }
            else
            {
                //
                // Otherwise we failed during the move.
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
                //
                // Return partial copy
                //
                Status = STATUS_PARTIAL_COPY;
            }
        }
        _SEH2_END;

        //
        // Detach from target
        //
        KeUnstackDetachProcess(&ApcState);

        //
        // Check for SEH status
        //
        if (Status != STATUS_SUCCESS)
        {
            goto Exit;
        }

        //
        // Update location and size
        //
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress +
                                       CurrentSize);
    }

Exit:
    //
    // Check if we had allocated pool
    //
    if (HavePoolAddress)
        ExFreePoolWithTag(PoolAddress, 'VmRw');

    //
    // All bytes read
    //
    if (Status == STATUS_SUCCESS)
        *ReturnSize = BufferSize;
    return Status;
}

NTSTATUS
NTAPI
MmCopyVirtualMemory(IN PEPROCESS SourceProcess,
                    IN PVOID SourceAddress,
                    IN PEPROCESS TargetProcess,
                    OUT PVOID TargetAddress,
                    IN SIZE_T BufferSize,
                    IN KPROCESSOR_MODE PreviousMode,
                    OUT PSIZE_T ReturnSize)
{
    NTSTATUS Status;
    PEPROCESS Process = SourceProcess;

    //
    // Don't accept zero-sized buffers
    //
    if (!BufferSize) return STATUS_SUCCESS;

    //
    // If we are copying from ourselves, lock the target instead
    //
    if (SourceProcess == PsGetCurrentProcess()) Process = TargetProcess;

    //
    // Acquire rundown protection
    //
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        //
        // Fail
        //
        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    // See if we should use the pool copy
    //
    if (BufferSize > MI_POOL_COPY_BYTES)
    {
        //
        // Use MDL-copy
        //
        Status = MiDoMappedCopy(SourceProcess,
                                SourceAddress,
                                TargetProcess,
                                TargetAddress,
                                BufferSize,
                                PreviousMode,
                                ReturnSize);
    }
    else
    {
        //
        // Do pool copy
        //
        Status = MiDoPoolCopy(SourceProcess,
                              SourceAddress,
                              TargetProcess,
                              TargetAddress,
                              BufferSize,
                              PreviousMode,
                              ReturnSize);
    }

    //
    // Release the lock
    //
    ExReleaseRundownProtection(&Process->RundownProtect);
    return Status;
}

NTSTATUS
NTAPI
MmFlushVirtualMemory(IN PEPROCESS Process,
                     IN OUT PVOID *BaseAddress,
                     IN OUT PSIZE_T RegionSize,
                     OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PAGED_CODE();

    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}

ULONG
NTAPI
MiGetPageProtection(IN PMMPTE PointerPte)
{
    MMPTE TempPte;
    PMMPFN Pfn;
    PEPROCESS CurrentProcess;
    PETHREAD CurrentThread;
    BOOLEAN WsSafe, WsShared;
    ULONG Protect;
    KIRQL OldIrql;
    PAGED_CODE();

    /* Copy this PTE's contents */
    TempPte = *PointerPte;

    /* Assure it's not totally zero */
    ASSERT(TempPte.u.Long);

    /* Check for a special prototype format */
    if ((TempPte.u.Soft.Valid == 0) &&
        (TempPte.u.Soft.Prototype == 1))
    {
        /* Check if the prototype PTE is not yet pointing to a PTE */
        if (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)
        {
            /* The prototype PTE contains the protection */
            return MmProtectToValue[TempPte.u.Soft.Protection];
        }

        /* Get a pointer to the underlying shared PTE */
        PointerPte = MiProtoPteToPte(&TempPte);

        /* Since the PTE we want to read can be paged out at any time, we need
           to release the working set lock first, so that it can be paged in */
        CurrentThread = PsGetCurrentThread();
        CurrentProcess = PsGetCurrentProcess();
        MiUnlockProcessWorkingSetForFault(CurrentProcess,
                                          CurrentThread,
                                          &WsSafe,
                                          &WsShared);

        /* Now read the PTE value */
        TempPte = *PointerPte;

        /* Check if that one is invalid */
        if (!TempPte.u.Hard.Valid)
        {
            /* We get the protection directly from this PTE */
            Protect = MmProtectToValue[TempPte.u.Soft.Protection];
        }
        else
        {
            /* The PTE is valid, so we might need to get the protection from
               the PFN. Lock the PFN database */
            OldIrql = MiAcquirePfnLock();

            /* Check if the PDE is still valid */
            if (MiAddressToPte(PointerPte)->u.Hard.Valid == 0)
            {
                /* It's not, make it valid */
                MiMakeSystemAddressValidPfn(PointerPte, OldIrql);
            }

            /* Now it's safe to read the PTE value again */
            TempPte = *PointerPte;
            ASSERT(TempPte.u.Long != 0);

            /* Check again if the PTE is invalid */
            if (!TempPte.u.Hard.Valid)
            {
                /* The PTE is not valid, so we can use it's protection field */
                Protect = MmProtectToValue[TempPte.u.Soft.Protection];
            }
            else
            {
                /* The PTE is valid, so we can find the protection in the
                   OriginalPte field of the PFN */
                Pfn = MI_PFN_ELEMENT(TempPte.u.Hard.PageFrameNumber);
                Protect = MmProtectToValue[Pfn->OriginalPte.u.Soft.Protection];
            }

            /* Release the PFN database */
            MiReleasePfnLock(OldIrql);
        }

        /* Lock the working set again */
        MiLockProcessWorkingSetForFault(CurrentProcess,
                                        CurrentThread,
                                        WsSafe,
                                        WsShared);

        return Protect;
    }

    /* In the easy case of transition or demand zero PTE just return its protection */
    if (!TempPte.u.Hard.Valid) return MmProtectToValue[TempPte.u.Soft.Protection];

    /* If we get here, the PTE is valid, so look up the page in PFN database */
    Pfn = MiGetPfnEntry(TempPte.u.Hard.PageFrameNumber);
    if (!Pfn->u3.e1.PrototypePte)
    {
        /* Return protection of the original pte */
        ASSERT(Pfn->u4.AweAllocation == 0);
        return MmProtectToValue[Pfn->OriginalPte.u.Soft.Protection];
    }

    /* This is software PTE */
    DPRINT("Prototype PTE: %lx %p\n", TempPte.u.Hard.PageFrameNumber, Pfn);
    DPRINT("VA: %p\n", MiPteToAddress(&TempPte));
    DPRINT("Mask: %lx\n", TempPte.u.Soft.Protection);
    DPRINT("Mask2: %lx\n", Pfn->OriginalPte.u.Soft.Protection);
    return MmProtectToValue[TempPte.u.Soft.Protection];
}

ULONG
NTAPI
MiQueryAddressState(IN PVOID Va,
                    IN PMMVAD Vad,
                    IN PEPROCESS TargetProcess,
                    OUT PULONG ReturnedProtect,
                    OUT PVOID *NextVa)
{

    PMMPTE PointerPte, ProtoPte;
    PMMPDE PointerPde;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPPE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PMMPXE PointerPxe;
#endif
    MMPTE TempPte, TempProtoPte;
    BOOLEAN DemandZeroPte = TRUE, ValidPte = FALSE;
    ULONG State = MEM_RESERVE, Protect = 0;
    ASSERT((Vad->StartingVpn <= ((ULONG_PTR)Va >> PAGE_SHIFT)) &&
           (Vad->EndingVpn >= ((ULONG_PTR)Va >> PAGE_SHIFT)));

    /* Only normal VADs supported */
    ASSERT(Vad->u.VadFlags.VadType == VadNone);

    /* Get the PDE and PTE for the address */
    PointerPde = MiAddressToPde(Va);
    PointerPte = MiAddressToPte(Va);
#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiAddressToPpe(Va);
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PointerPxe = MiAddressToPxe(Va);
#endif

    /* Return the next range */
    *NextVa = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);

    do
    {
#if (_MI_PAGING_LEVELS >= 4)
        /* Does the PXE exist? */
        if (PointerPxe->u.Long == 0)
        {
            /* It does not, next range starts at the next PXE */
            *NextVa = MiPxeToAddress(PointerPxe + 1);
            break;
        }

        /* Is the PXE valid? */
        if (PointerPxe->u.Hard.Valid == 0)
        {
            /* Is isn't, fault it in (make the PPE accessible) */
            MiMakeSystemAddressValid(PointerPpe, TargetProcess);
        }
#endif
#if (_MI_PAGING_LEVELS >= 3)
        /* Does the PPE exist? */
        if (PointerPpe->u.Long == 0)
        {
            /* It does not, next range starts at the next PPE */
            *NextVa = MiPpeToAddress(PointerPpe + 1);
            break;
        }

        /* Is the PPE valid? */
        if (PointerPpe->u.Hard.Valid == 0)
        {
            /* Is isn't, fault it in (make the PDE accessible) */
            MiMakeSystemAddressValid(PointerPde, TargetProcess);
        }
#endif

        /* Does the PDE exist? */
        if (PointerPde->u.Long == 0)
        {
            /* It does not, next range starts at the next PDE */
            *NextVa = MiPdeToAddress(PointerPde + 1);
            break;
        }

        /* Is the PDE valid? */
        if (PointerPde->u.Hard.Valid == 0)
        {
            /* Is isn't, fault it in (make the PTE accessible) */
            MiMakeSystemAddressValid(PointerPte, TargetProcess);
        }

        /* We have a PTE that we can access now! */
        ValidPte = TRUE;

    } while (FALSE);

    /* Is it safe to try reading the PTE? */
    if (ValidPte)
    {
        /* FIXME: watch out for large pages */
        ASSERT(PointerPde->u.Hard.LargePage == FALSE);

        /* Capture the PTE */
        TempPte = *PointerPte;
        if (TempPte.u.Long != 0)
        {
            /* The PTE is valid, so it's not zeroed out */
            DemandZeroPte = FALSE;

            /* Is it a decommited, invalid, or faulted PTE? */
            if ((TempPte.u.Soft.Protection == MM_DECOMMIT) &&
                (TempPte.u.Hard.Valid == 0) &&
                ((TempPte.u.Soft.Prototype == 0) ||
                 (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)))
            {
                /* Otherwise our defaults should hold */
                ASSERT(Protect == 0);
                ASSERT(State == MEM_RESERVE);
            }
            else
            {
                /* This means it's committed */
                State = MEM_COMMIT;

                /* We don't support these */
                ASSERT(Vad->u.VadFlags.VadType != VadDevicePhysicalMemory);
                ASSERT(Vad->u.VadFlags.VadType != VadRotatePhysical);
                ASSERT(Vad->u.VadFlags.VadType != VadAwe);

                /* Get protection state of this page */
                Protect = MiGetPageProtection(PointerPte);

                /* Check if this is an image-backed VAD */
                if ((TempPte.u.Soft.Valid == 0) &&
                    (TempPte.u.Soft.Prototype == 1) &&
                    (Vad->u.VadFlags.PrivateMemory == 0) &&
                    (Vad->ControlArea))
                {
                    DPRINT1("Not supported\n");
                    ASSERT(FALSE);
                }
            }
        }
    }

    /* Check if this was a demand-zero PTE, since we need to find the state */
    if (DemandZeroPte)
    {
        /* Not yet handled */
        ASSERT(Vad->u.VadFlags.VadType != VadDevicePhysicalMemory);
        ASSERT(Vad->u.VadFlags.VadType != VadAwe);

        /* Check if this is private commited memory, or an section-backed VAD */
        if ((Vad->u.VadFlags.PrivateMemory == 0) && (Vad->ControlArea))
        {
            /* Tell caller about the next range */
            *NextVa = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);

            /* Get the prototype PTE for this VAD */
            ProtoPte = MI_GET_PROTOTYPE_PTE_FOR_VPN(Vad,
                                                    (ULONG_PTR)Va >> PAGE_SHIFT);
            if (ProtoPte)
            {
                /* We should unlock the working set, but it's not being held! */

                /* Is the prototype PTE actually valid (committed)? */
                TempProtoPte = *ProtoPte;
                if (TempProtoPte.u.Long)
                {
                    /* Unless this is a memory-mapped file, handle it like private VAD */
                    State = MEM_COMMIT;
                    ASSERT(Vad->u.VadFlags.VadType != VadImageMap);
                    Protect = MmProtectToValue[Vad->u.VadFlags.Protection];
                }

                /* We should re-lock the working set */
            }
        }
        else if (Vad->u.VadFlags.MemCommit)
        {
            /* This is committed memory */
            State = MEM_COMMIT;

            /* Convert the protection */
            Protect = MmProtectToValue[Vad->u.VadFlags.Protection];
        }
    }

    /* Return the protection code */
    *ReturnedProtect = Protect;
    return State;
}

NTSTATUS
NTAPI
MiQueryMemoryBasicInformation(IN HANDLE ProcessHandle,
                              IN PVOID BaseAddress,
                              OUT PVOID MemoryInformation,
                              IN SIZE_T MemoryInformationLength,
                              OUT PSIZE_T ReturnLength)
{
    PEPROCESS TargetProcess;
    NTSTATUS Status = STATUS_SUCCESS;
    PMMVAD Vad = NULL;
    PVOID Address, NextAddress;
    BOOLEAN Found = FALSE;
    ULONG NewProtect, NewState;
    ULONG_PTR BaseVpn;
    MEMORY_BASIC_INFORMATION MemoryInfo;
    KAPC_STATE ApcState;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PMEMORY_AREA MemoryArea;
    SIZE_T ResultLength;

    /* Check for illegal addresses in user-space, or the shared memory area */
    if ((BaseAddress > MM_HIGHEST_VAD_ADDRESS) ||
        (PAGE_ALIGN(BaseAddress) == (PVOID)MM_SHARED_USER_DATA_VA))
    {
        Address = PAGE_ALIGN(BaseAddress);

        /* Make up an info structure describing this range */
        MemoryInfo.BaseAddress = Address;
        MemoryInfo.AllocationProtect = PAGE_READONLY;
        MemoryInfo.Type = MEM_PRIVATE;

        /* Special case for shared data */
        if (Address == (PVOID)MM_SHARED_USER_DATA_VA)
        {
            MemoryInfo.AllocationBase = (PVOID)MM_SHARED_USER_DATA_VA;
            MemoryInfo.State = MEM_COMMIT;
            MemoryInfo.Protect = PAGE_READONLY;
            MemoryInfo.RegionSize = PAGE_SIZE;
        }
        else
        {
            MemoryInfo.AllocationBase = (PCHAR)MM_HIGHEST_VAD_ADDRESS + 1;
            MemoryInfo.State = MEM_RESERVE;
            MemoryInfo.Protect = PAGE_NOACCESS;
            MemoryInfo.RegionSize = (ULONG_PTR)MM_HIGHEST_USER_ADDRESS + 1 - (ULONG_PTR)Address;
        }

        /* Return the data, NtQueryInformation already probed it*/
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                *(PMEMORY_BASIC_INFORMATION)MemoryInformation = MemoryInfo;
                if (ReturnLength) *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
            }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            *(PMEMORY_BASIC_INFORMATION)MemoryInformation = MemoryInfo;
            if (ReturnLength) *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
        }

        return Status;
    }

    /* Check if this is for a local or remote process */
    if (ProcessHandle == NtCurrentProcess())
    {
        TargetProcess = PsGetCurrentProcess();
    }
    else
    {
        /* Reference the target process */
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_INFORMATION,
                                           PsProcessType,
                                           ExGetPreviousMode(),
                                           (PVOID*)&TargetProcess,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;

        /* Attach to it now */
        KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);
    }

    /* Lock the address space and make sure the process isn't already dead */
    MmLockAddressSpace(&TargetProcess->Vm);
    if (TargetProcess->VmDeleted)
    {
        /* Unlock the address space of the process */
        MmUnlockAddressSpace(&TargetProcess->Vm);

        /* Check if we were attached */
        if (ProcessHandle != NtCurrentProcess())
        {
            /* Detach and dereference the process */
            KeUnstackDetachProcess(&ApcState);
            ObDereferenceObject(TargetProcess);
        }

        /* Bail out */
        DPRINT1("Process is dying\n");
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Loop the VADs */
    ASSERT(TargetProcess->VadRoot.NumberGenericTableElements);
    if (TargetProcess->VadRoot.NumberGenericTableElements)
    {
        /* Scan on the right */
        Vad = (PMMVAD)TargetProcess->VadRoot.BalancedRoot.RightChild;
        BaseVpn = (ULONG_PTR)BaseAddress >> PAGE_SHIFT;
        while (Vad)
        {
            /* Check if this VAD covers the allocation range */
            if ((BaseVpn >= Vad->StartingVpn) &&
                (BaseVpn <= Vad->EndingVpn))
            {
                /* We're done */
                Found = TRUE;
                break;
            }

            /* Check if this VAD is too high */
            if (BaseVpn < Vad->StartingVpn)
            {
                /* Stop if there is no left child */
                if (!Vad->LeftChild) break;

                /* Search on the left next */
                Vad = Vad->LeftChild;
            }
            else
            {
                /* Then this VAD is too low, keep searching on the right */
                ASSERT(BaseVpn > Vad->EndingVpn);

                /* Stop if there is no right child */
                if (!Vad->RightChild) break;

                /* Search on the right next */
                Vad = Vad->RightChild;
            }
        }
    }

    /* Was a VAD found? */
    if (!Found)
    {
        Address = PAGE_ALIGN(BaseAddress);

        /* Calculate region size */
        if (Vad)
        {
            if (Vad->StartingVpn >= BaseVpn)
            {
                /* Region size is the free space till the start of that VAD */
                MemoryInfo.RegionSize = (ULONG_PTR)(Vad->StartingVpn << PAGE_SHIFT) - (ULONG_PTR)Address;
            }
            else
            {
                /* Get the next VAD */
                Vad = (PMMVAD)MiGetNextNode((PMMADDRESS_NODE)Vad);
                if (Vad)
                {
                    /* Region size is the free space till the start of that VAD */
                    MemoryInfo.RegionSize = (ULONG_PTR)(Vad->StartingVpn << PAGE_SHIFT) - (ULONG_PTR)Address;
                }
                else
                {
                    /* Maximum possible region size with that base address */
                    MemoryInfo.RegionSize = (PCHAR)MM_HIGHEST_VAD_ADDRESS + 1 - (PCHAR)Address;
                }
            }
        }
        else
        {
            /* Maximum possible region size with that base address */
            MemoryInfo.RegionSize = (PCHAR)MM_HIGHEST_VAD_ADDRESS + 1 - (PCHAR)Address;
        }

        /* Unlock the address space of the process */
        MmUnlockAddressSpace(&TargetProcess->Vm);

        /* Check if we were attached */
        if (ProcessHandle != NtCurrentProcess())
        {
            /* Detach and dereference the process */
            KeUnstackDetachProcess(&ApcState);
            ObDereferenceObject(TargetProcess);
        }

        /* Build the rest of the initial information block */
        MemoryInfo.BaseAddress = Address;
        MemoryInfo.AllocationBase = NULL;
        MemoryInfo.AllocationProtect = 0;
        MemoryInfo.State = MEM_FREE;
        MemoryInfo.Protect = PAGE_NOACCESS;
        MemoryInfo.Type = 0;

        /* Return the data, NtQueryInformation already probed it*/
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                *(PMEMORY_BASIC_INFORMATION)MemoryInformation = MemoryInfo;
                if (ReturnLength) *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
            }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            *(PMEMORY_BASIC_INFORMATION)MemoryInformation = MemoryInfo;
            if (ReturnLength) *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
        }

        return Status;
    }

    /* Set the correct memory type based on what kind of VAD this is */
    if ((Vad->u.VadFlags.PrivateMemory) ||
        (Vad->u.VadFlags.VadType == VadRotatePhysical))
    {
        MemoryInfo.Type = MEM_PRIVATE;
    }
    else if (Vad->u.VadFlags.VadType == VadImageMap)
    {
        MemoryInfo.Type = MEM_IMAGE;
    }
    else
    {
        MemoryInfo.Type = MEM_MAPPED;
    }

    /* Find the memory area the specified address belongs to */
    MemoryArea = MmLocateMemoryAreaByAddress(&TargetProcess->Vm, BaseAddress);
    ASSERT(MemoryArea != NULL);

    /* Determine information dependent on the memory area type */
    if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
    {
        Status = MmQuerySectionView(MemoryArea, BaseAddress, &MemoryInfo, &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MmQuerySectionView failed. MemoryArea=%p (%p-%p), BaseAddress=%p\n",
                    MemoryArea, MA_GetStartingAddress(MemoryArea), MA_GetEndingAddress(MemoryArea), BaseAddress);
            ASSERT(NT_SUCCESS(Status));
        }
    }
    else
    {
        /* Build the initial information block */
        Address = PAGE_ALIGN(BaseAddress);
        MemoryInfo.BaseAddress = Address;
        MemoryInfo.AllocationBase = (PVOID)(Vad->StartingVpn << PAGE_SHIFT);
        MemoryInfo.AllocationProtect = MmProtectToValue[Vad->u.VadFlags.Protection];
        MemoryInfo.Type = MEM_PRIVATE;

        /* Acquire the working set lock (shared is enough) */
        MiLockProcessWorkingSetShared(TargetProcess, PsGetCurrentThread());

        /* Find the largest chunk of memory which has the same state and protection mask */
        MemoryInfo.State = MiQueryAddressState(Address,
                                               Vad,
                                               TargetProcess,
                                               &MemoryInfo.Protect,
                                               &NextAddress);
        Address = NextAddress;
        while (((ULONG_PTR)Address >> PAGE_SHIFT) <= Vad->EndingVpn)
        {
            /* Keep going unless the state or protection mask changed */
            NewState = MiQueryAddressState(Address, Vad, TargetProcess, &NewProtect, &NextAddress);
            if ((NewState != MemoryInfo.State) || (NewProtect != MemoryInfo.Protect)) break;
            Address = NextAddress;
        }

        /* Release the working set lock */
        MiUnlockProcessWorkingSetShared(TargetProcess, PsGetCurrentThread());

        /* Check if we went outside of the VAD */
         if (((ULONG_PTR)Address >> PAGE_SHIFT) > Vad->EndingVpn)
         {
            /* Set the end of the VAD as the end address */
            Address = (PVOID)((Vad->EndingVpn + 1) << PAGE_SHIFT);
         }

        /* Now that we know the last VA address, calculate the region size */
        MemoryInfo.RegionSize = ((ULONG_PTR)Address - (ULONG_PTR)MemoryInfo.BaseAddress);
    }

    /* Unlock the address space of the process */
    MmUnlockAddressSpace(&TargetProcess->Vm);

    /* Check if we were attached */
    if (ProcessHandle != NtCurrentProcess())
    {
        /* Detach and dereference the process */
        KeUnstackDetachProcess(&ApcState);
        ObDereferenceObject(TargetProcess);
    }

    /* Return the data, NtQueryInformation already probed it */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            *(PMEMORY_BASIC_INFORMATION)MemoryInformation = MemoryInfo;
            if (ReturnLength) *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
        }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        *(PMEMORY_BASIC_INFORMATION)MemoryInformation = MemoryInfo;
        if (ReturnLength) *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
    }

    /* All went well */
    DPRINT("Base: %p AllocBase: %p AllocProtect: %lx Protect: %lx "
            "State: %lx Type: %lx Size: %lx\n",
            MemoryInfo.BaseAddress, MemoryInfo.AllocationBase,
            MemoryInfo.AllocationProtect, MemoryInfo.Protect,
            MemoryInfo.State, MemoryInfo.Type, MemoryInfo.RegionSize);

    return Status;
}

BOOLEAN
NTAPI
MiIsEntireRangeCommitted(IN ULONG_PTR StartingAddress,
                         IN ULONG_PTR EndingAddress,
                         IN PMMVAD Vad,
                         IN PEPROCESS Process)
{
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
    BOOLEAN OnPdeBoundary = TRUE;
#if _MI_PAGING_LEVELS >= 3
    PMMPPE PointerPpe;
    BOOLEAN OnPpeBoundary = TRUE;
#if _MI_PAGING_LEVELS == 4
    PMMPXE PointerPxe;
    BOOLEAN OnPxeBoundary = TRUE;
#endif
#endif

    PAGED_CODE();

    /* Check that we hols the right locks */
    ASSERT(PsGetCurrentThread()->OwnsProcessWorkingSetExclusive || PsGetCurrentThread()->OwnsProcessWorkingSetShared);

    /* Get the PTE addresses */
    PointerPte = MiAddressToPte(StartingAddress);
    LastPte = MiAddressToPte(EndingAddress);

    /* Loop all the PTEs */
    while (PointerPte <= LastPte)
    {
#if _MI_PAGING_LEVELS == 4
        /* Check for new PXE boundary */
        if (OnPxeBoundary)
        {
            PointerPxe = MiPteToPxe(PointerPte);

            /* Check that this loop is sane */
            ASSERT(OnPpeBoundary);
            ASSERT(OnPdeBoundary);

            if (PointerPxe->u.Long != 0)
            {
                /* Make it valid if needed */
                if (PointerPxe->u.Hard.Valid == 0)
                    MiMakeSystemAddressValid(MiPteToPpe(PointerPte), Process);
            }
            else
            {
                /* Is the entire VAD committed? If not, fail */
                if (!Vad->u.VadFlags.MemCommit) return FALSE;

                PointerPxe++;
                PointerPte = MiPxeToPte(PointerPte);
                continue;
            }
        }
#endif

#if _MI_PAGING_LEVELS >= 3
        /* Check for new PPE boundary */
        if (OnPpeBoundary)
        {
            PointerPpe = MiPteToPpe(PointerPte);

            /* Check that this loop is sane */
            ASSERT(OnPdeBoundary);

            if (PointerPpe->u.Long != 0)
            {
                /* Make it valid if needed */
                if (PointerPpe->u.Hard.Valid == 0)
                    MiMakeSystemAddressValid(MiPteToPde(PointerPte), Process);
            }
            else
            {
                /* Is the entire VAD committed? If not, fail */
                if (!Vad->u.VadFlags.MemCommit) return FALSE;

                PointerPpe++;
                PointerPte = MiPpeToPte(PointerPpe);
#if _MI_PAGING_LEVELS == 4
                OnPxeBoundary = MiIsPteOnPxeBoundary(PointerPte);
#endif
                continue;
            }
        }
#endif
        /* Check if we've hit a new PDE boundary */
        if (OnPdeBoundary)
        {
            /* Is this PDE demand zero? */
            PointerPde = MiPteToPde(PointerPte);
            if (PointerPde->u.Long != 0)
            {
                /* It isn't -- is it valid? */
                if (PointerPde->u.Hard.Valid == 0)
                {
                    /* Nope, fault it in */
                    MiMakeSystemAddressValid(PointerPte, Process);
                }
            }
            else
            {
                /* Is the entire VAD committed? If not, fail */
                if (!Vad->u.VadFlags.MemCommit) return FALSE;

                /* The PTE was already valid, so move to the next one */
                PointerPde++;
                PointerPte = MiPdeToPte(PointerPde);
#if _MI_PAGING_LEVELS >= 3
                OnPpeBoundary = MiIsPteOnPpeBoundary(PointerPte);
#if _MI_PAGING_LEVELS == 4
                OnPxeBoundary = MiIsPteOnPxeBoundary(PointerPte);
#endif
#endif

                /* New loop iteration with our new, on-boundary PTE. */
                continue;
            }
        }

        /* Is the PTE demand zero? */
        if (PointerPte->u.Long == 0)
        {
            /* Is the entire VAD committed? If not, fail */
            if (!Vad->u.VadFlags.MemCommit) return FALSE;
        }
        else
        {
            /* It isn't -- is it a decommited, invalid, or faulted PTE? */
            if ((PointerPte->u.Soft.Protection == MM_DECOMMIT) &&
                (PointerPte->u.Hard.Valid == 0) &&
                ((PointerPte->u.Soft.Prototype == 0) ||
                 (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)))
            {
                /* Then part of the range is decommitted, so fail */
                return FALSE;
            }
        }

        /* Move to the next PTE */
        PointerPte++;
        OnPdeBoundary = MiIsPteOnPdeBoundary(PointerPte);
#if _MI_PAGING_LEVELS >= 3
        OnPpeBoundary = MiIsPteOnPpeBoundary(PointerPte);
#if _MI_PAGING_LEVELS == 4
        OnPxeBoundary = MiIsPteOnPxeBoundary(PointerPte);
#endif
#endif
    }

    /* All PTEs seem valid, and no VAD checks failed, the range is okay */
    return TRUE;
}

NTSTATUS
NTAPI
MiRosProtectVirtualMemory(IN PEPROCESS Process,
                          IN OUT PVOID *BaseAddress,
                          IN OUT PSIZE_T NumberOfBytesToProtect,
                          IN ULONG NewAccessProtection,
                          OUT PULONG OldAccessProtection OPTIONAL)
{
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace;
    ULONG OldAccessProtection_;
    NTSTATUS Status;

    *NumberOfBytesToProtect = PAGE_ROUND_UP((ULONG_PTR)(*BaseAddress) + (*NumberOfBytesToProtect)) - PAGE_ROUND_DOWN(*BaseAddress);
    *BaseAddress = (PVOID)PAGE_ROUND_DOWN(*BaseAddress);

    AddressSpace = &Process->Vm;
    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, *BaseAddress);
    if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_UNSUCCESSFUL;
    }

    if (OldAccessProtection == NULL) OldAccessProtection = &OldAccessProtection_;

    ASSERT(MemoryArea->Type == MEMORY_AREA_SECTION_VIEW);
    Status = MmProtectSectionView(AddressSpace,
                                  MemoryArea,
                                  *BaseAddress,
                                  *NumberOfBytesToProtect,
                                  NewAccessProtection,
                                  OldAccessProtection);

    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

NTSTATUS
NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PSIZE_T NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection OPTIONAL)
{
    PMEMORY_AREA MemoryArea;
    PMMVAD Vad;
    PMMSUPPORT AddressSpace;
    ULONG_PTR StartingAddress, EndingAddress;
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
    MMPTE PteContents;
    PMMPFN Pfn1;
    ULONG ProtectionMask, OldProtect;
    BOOLEAN Committed;
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread = PsGetCurrentThread();
    TABLE_SEARCH_RESULT Result;

    /* Calculate base address for the VAD */
    StartingAddress = (ULONG_PTR)PAGE_ALIGN((*BaseAddress));
    EndingAddress = (((ULONG_PTR)*BaseAddress + *NumberOfBytesToProtect - 1) | (PAGE_SIZE - 1));

    /* Calculate the protection mask and make sure it's valid */
    ProtectionMask = MiMakeProtectionMask(NewAccessProtection);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid protection mask\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Check for ROS specific memory area */
    MemoryArea = MmLocateMemoryAreaByAddress(&Process->Vm, *BaseAddress);
    if ((MemoryArea) && (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3))
    {
        /* Evil hack */
        return MiRosProtectVirtualMemory(Process,
                                         BaseAddress,
                                         NumberOfBytesToProtect,
                                         NewAccessProtection,
                                         OldAccessProtection);
    }

    /* Lock the address space and make sure the process isn't already dead */
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);
    if (Process->VmDeleted)
    {
        DPRINT1("Process is dying\n");
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto FailPath;
    }

    /* Get the VAD for this address range, and make sure it exists */
    Result = MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                       EndingAddress >> PAGE_SHIFT,
                                       &Process->VadRoot,
                                       (PMMADDRESS_NODE*)&Vad);
    if (Result != TableFoundNode)
    {
        DPRINT("Could not find a VAD for this allocation\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    /* Make sure the address is within this VAD's boundaries */
    if ((((ULONG_PTR)StartingAddress >> PAGE_SHIFT) < Vad->StartingVpn) ||
        (((ULONG_PTR)EndingAddress >> PAGE_SHIFT) > Vad->EndingVpn))
    {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    /* These kinds of VADs are not supported atm  */
    if ((Vad->u.VadFlags.VadType == VadAwe) ||
        (Vad->u.VadFlags.VadType == VadDevicePhysicalMemory) ||
        (Vad->u.VadFlags.VadType == VadLargePages))
    {
        DPRINT1("Illegal VAD for attempting to set protection\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    /* Check for a VAD whose protection can't be changed */
    if (Vad->u.VadFlags.NoChange == 1)
    {
        DPRINT1("Trying to change protection of a NoChange VAD\n");
        Status = STATUS_INVALID_PAGE_PROTECTION;
        goto FailPath;
    }

    /* Is this section, or private memory? */
    if (Vad->u.VadFlags.PrivateMemory == 0)
    {
        /* Not yet supported */
        if (Vad->u.VadFlags.VadType == VadLargePageSection)
        {
            DPRINT1("Illegal VAD for attempting to set protection\n");
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto FailPath;
        }

        /* Rotate VADs are not yet supported */
        if (Vad->u.VadFlags.VadType == VadRotatePhysical)
        {
            DPRINT1("Illegal VAD for attempting to set protection\n");
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto FailPath;
        }

        /* Not valid on section files */
        if (NewAccessProtection & (PAGE_NOCACHE | PAGE_WRITECOMBINE))
        {
            /* Fail */
            DPRINT1("Invalid protection flags for section\n");
            Status = STATUS_INVALID_PARAMETER_4;
            goto FailPath;
        }

        /* Check if data or page file mapping protection PTE is compatible */
        if (!Vad->ControlArea->u.Flags.Image)
        {
            /* Not yet */
            DPRINT1("Fixme: Not checking for valid protection\n");
        }

        /* This is a section, and this is not yet supported */
        DPRINT1("Section protection not yet supported\n");
        OldProtect = 0;
    }
    else
    {
        /* Private memory, check protection flags */
        if ((NewAccessProtection & PAGE_WRITECOPY) ||
            (NewAccessProtection & PAGE_EXECUTE_WRITECOPY))
        {
            DPRINT1("Invalid protection flags for private memory\n");
            Status = STATUS_INVALID_PARAMETER_4;
            goto FailPath;
        }

        /* Lock the working set */
        MiLockProcessWorkingSetUnsafe(Process, Thread);

        /* Check if all pages in this range are committed */
        Committed = MiIsEntireRangeCommitted(StartingAddress,
                                             EndingAddress,
                                             Vad,
                                             Process);
        if (!Committed)
        {
            /* Fail */
            DPRINT1("The entire range is not committed\n");
            Status = STATUS_NOT_COMMITTED;
            MiUnlockProcessWorkingSetUnsafe(Process, Thread);
            goto FailPath;
        }

        /* Compute starting and ending PTE and PDE addresses */
        PointerPde = MiAddressToPde(StartingAddress);
        PointerPte = MiAddressToPte(StartingAddress);
        LastPte = MiAddressToPte(EndingAddress);

        /* Make this PDE valid */
        MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);

        /* Save protection of the first page */
        if (PointerPte->u.Long != 0)
        {
            /* Capture the page protection and make the PDE valid */
            OldProtect = MiGetPageProtection(PointerPte);
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }
        else
        {
            /* Grab the old protection from the VAD itself */
            OldProtect = MmProtectToValue[Vad->u.VadFlags.Protection];
        }

        /* Loop all the PTEs now */
        while (PointerPte <= LastPte)
        {
            /* Check if we've crossed a PDE boundary and make the new PDE valid too */
            if (MiIsPteOnPdeBoundary(PointerPte))
            {
                PointerPde = MiPteToPde(PointerPte);
                MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
            }

            /* Capture the PTE and check if it was empty */
            PteContents = *PointerPte;
            if (PteContents.u.Long == 0)
            {
                /* This used to be a zero PTE and it no longer is, so we must add a
                   reference to the pagetable. */
                MiIncrementPageTableReferences(MiPteToAddress(PointerPte));
            }

            /* Check what kind of PTE we are dealing with */
            if (PteContents.u.Hard.Valid == 1)
            {
                /* Get the PFN entry */
                Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(&PteContents));

                /* We don't support this yet */
                ASSERT(Pfn1->u3.e1.PrototypePte == 0);

                /* Check if the page should not be accessible at all */
                if ((NewAccessProtection & PAGE_NOACCESS) ||
                    (NewAccessProtection & PAGE_GUARD))
                {
                    KIRQL OldIrql = MiAcquirePfnLock();

                    /* Mark the PTE as transition and change its protection */
                    PteContents.u.Hard.Valid = 0;
                    PteContents.u.Soft.Transition = 1;
                    PteContents.u.Trans.Protection = ProtectionMask;
                    /* Decrease PFN share count and write the PTE */
                    MiDecrementShareCount(Pfn1, PFN_FROM_PTE(&PteContents));
                    // FIXME: remove the page from the WS
                    MI_WRITE_INVALID_PTE(PointerPte, PteContents);
#ifdef CONFIG_SMP
                    // FIXME: Should invalidate entry in every CPU TLB
                    ASSERT(KeNumberProcessors == 1);
#endif
                    KeInvalidateTlbEntry(MiPteToAddress(PointerPte));

                    /* We are done for this PTE */
                    MiReleasePfnLock(OldIrql);
                }
                else
                {
                    /* Write the protection mask and write it with a TLB flush */
                    Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                    MiFlushTbAndCapture(Vad,
                                        PointerPte,
                                        ProtectionMask,
                                        Pfn1,
                                        TRUE);
                }
            }
            else
            {
                /* We don't support these cases yet */
                ASSERT(PteContents.u.Soft.Prototype == 0);
                //ASSERT(PteContents.u.Soft.Transition == 0);

                /* The PTE is already demand-zero, just update the protection mask */
                PteContents.u.Soft.Protection = ProtectionMask;
                MI_WRITE_INVALID_PTE(PointerPte, PteContents);
                ASSERT(PointerPte->u.Long != 0);
            }

            /* Move to the next PTE */
            PointerPte++;
        }

        /* Unlock the working set */
        MiUnlockProcessWorkingSetUnsafe(Process, Thread);
    }

    /* Unlock the address space */
    MmUnlockAddressSpace(AddressSpace);

    /* Return parameters and success */
    *NumberOfBytesToProtect = EndingAddress - StartingAddress + 1;
    *BaseAddress = (PVOID)StartingAddress;
    *OldAccessProtection = OldProtect;
    return STATUS_SUCCESS;

FailPath:
    /* Unlock the address space and return the failure code */
    MmUnlockAddressSpace(AddressSpace);
    return Status;
}

VOID
NTAPI
MiMakePdeExistAndMakeValid(IN PMMPDE PointerPde,
                           IN PEPROCESS TargetProcess,
                           IN KIRQL OldIrql)
{
   PMMPTE PointerPte;
#if _MI_PAGING_LEVELS >= 3
   PMMPPE PointerPpe = MiPdeToPpe(PointerPde);
#if _MI_PAGING_LEVELS == 4
   PMMPXE PointerPxe = MiPdeToPxe(PointerPde);
#endif
#endif

   //
   // Sanity checks. The latter is because we only use this function with the
   // PFN lock not held, so it may go away in the future.
   //
   ASSERT(KeAreAllApcsDisabled() == TRUE);
   ASSERT(OldIrql == MM_NOIRQL);

   //
   // If everything is already valid, there is nothing to do.
   //
   if (
#if _MI_PAGING_LEVELS == 4
       (PointerPxe->u.Hard.Valid) &&
#endif
#if _MI_PAGING_LEVELS >= 3
       (PointerPpe->u.Hard.Valid) &&
#endif
       (PointerPde->u.Hard.Valid))
   {
       return;
   }

   //
   // At least something is invalid, so begin by getting the PTE for the PDE itself
   // and then lookup each additional level. We must do it in this precise order
   // because the pagfault.c code (as well as in Windows) depends that the next
   // level up (higher) must be valid when faulting a lower level
   //
   PointerPte = MiPteToAddress(PointerPde);
   do
   {
       //
       // Make sure APCs continued to be disabled
       //
       ASSERT(KeAreAllApcsDisabled() == TRUE);

#if _MI_PAGING_LEVELS == 4
       //
       // First, make the PXE valid if needed
       //
       if (!PointerPxe->u.Hard.Valid)
       {
           MiMakeSystemAddressValid(PointerPpe, TargetProcess);
           ASSERT(PointerPxe->u.Hard.Valid == 1);
       }
#endif

#if _MI_PAGING_LEVELS >= 3
       //
       // Next, the PPE
       //
       if (!PointerPpe->u.Hard.Valid)
       {
           MiMakeSystemAddressValid(PointerPde, TargetProcess);
           ASSERT(PointerPpe->u.Hard.Valid == 1);
       }
#endif

       //
       // And finally, make the PDE itself valid.
       //
       MiMakeSystemAddressValid(PointerPte, TargetProcess);

       /* Do not increment Page table refcount here for the PDE, this must be managed by caller */

       //
       // This should've worked the first time so the loop is really just for
       // show -- ASSERT that we're actually NOT going to be looping.
       //
       ASSERT(PointerPde->u.Hard.Valid == 1);
   } while (
#if _MI_PAGING_LEVELS == 4
        !PointerPxe->u.Hard.Valid ||
#endif
#if _MI_PAGING_LEVELS >= 3
        !PointerPpe->u.Hard.Valid ||
#endif
        !PointerPde->u.Hard.Valid);
}

VOID
NTAPI
MiProcessValidPteList(IN PMMPTE *ValidPteList,
                      IN ULONG Count)
{
    KIRQL OldIrql;
    ULONG i;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1, Pfn2;

    //
    // Acquire the PFN lock and loop all the PTEs in the list
    //
    OldIrql = MiAcquirePfnLock();
    for (i = 0; i != Count; i++)
    {
        //
        // The PTE must currently be valid
        //
        TempPte = *ValidPteList[i];
        ASSERT(TempPte.u.Hard.Valid == 1);

        //
        // Get the PFN entry for the page itself, and then for its page table
        //
        PageFrameIndex = PFN_FROM_PTE(&TempPte);
        Pfn1 = MiGetPfnEntry(PageFrameIndex);
        Pfn2 = MiGetPfnEntry(Pfn1->u4.PteFrame);

        //
        // Decrement the share count on the page table, and then on the page
        // itself
        //
        MiDecrementShareCount(Pfn2, Pfn1->u4.PteFrame);
        MI_SET_PFN_DELETED(Pfn1);
        MiDecrementShareCount(Pfn1, PageFrameIndex);

        //
        // Make the page decommitted
        //
        MI_WRITE_INVALID_PTE(ValidPteList[i], MmDecommittedPte);
    }

    //
    // All the PTEs have been dereferenced and made invalid, flush the TLB now
    // and then release the PFN lock
    //
    KeFlushCurrentTb();
    MiReleasePfnLock(OldIrql);
}

ULONG
NTAPI
MiDecommitPages(IN PVOID StartingAddress,
                IN PMMPTE EndingPte,
                IN PEPROCESS Process,
                IN PMMVAD Vad)
{
    PMMPTE PointerPte, CommitPte = NULL;
    PMMPDE PointerPde;
    ULONG CommitReduction = 0;
    PMMPTE ValidPteList[256];
    ULONG PteCount = 0;
    PMMPFN Pfn1;
    MMPTE PteContents;
    PETHREAD CurrentThread = PsGetCurrentThread();

    //
    // Get the PTE and PTE for the address, and lock the working set
    // If this was a VAD for a MEM_COMMIT allocation, also figure out where the
    // commited range ends so that we can do the right accounting.
    //
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    if (Vad->u.VadFlags.MemCommit) CommitPte = MiAddressToPte(Vad->EndingVpn << PAGE_SHIFT);
    MiLockProcessWorkingSetUnsafe(Process, CurrentThread);

    //
    // Make the PDE valid, and now loop through each page's worth of data
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= EndingPte)
    {
        //
        // Check if we've crossed a PDE boundary
        //
        if (MiIsPteOnPdeBoundary(PointerPte))
        {
            //
            // Get the new PDE and flush the valid PTEs we had built up until
            // now. This helps reduce the amount of TLB flushing we have to do.
            // Note that Windows does a much better job using timestamps and
            // such, and does not flush the entire TLB all the time, but right
            // now we have bigger problems to worry about than TLB flushing.
            //
            PointerPde = MiAddressToPde(StartingAddress);
            if (PteCount)
            {
                MiProcessValidPteList(ValidPteList, PteCount);
                PteCount = 0;
            }

            //
            // Make this PDE valid
            //
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }

        //
        // Read this PTE. It might be active or still demand-zero.
        //
        PteContents = *PointerPte;
        if (PteContents.u.Long)
        {
            //
            // The PTE is active. It might be valid and in a working set, or
            // it might be a prototype PTE or paged out or even in transition.
            //
            if (PointerPte->u.Long == MmDecommittedPte.u.Long)
            {
                //
                // It's already decommited, so there's nothing for us to do here
                //
                CommitReduction++;
            }
            else
            {
                //
                // Remove it from the counters, and check if it was valid or not
                //
                //Process->NumberOfPrivatePages--;
                if (PteContents.u.Hard.Valid)
                {
                    //
                    // It's valid. At this point make sure that it is not a ROS
                    // PFN. Also, we don't support ProtoPTEs in this code path.
                    //
                    Pfn1 = MiGetPfnEntry(PteContents.u.Hard.PageFrameNumber);
                    ASSERT(MI_IS_ROS_PFN(Pfn1) == FALSE);
                    ASSERT(Pfn1->u3.e1.PrototypePte == FALSE);

                    //
                    // Flush any pending PTEs that we had not yet flushed, if our
                    // list has gotten too big, then add this PTE to the flush list.
                    //
                    if (PteCount == 256)
                    {
                        MiProcessValidPteList(ValidPteList, PteCount);
                        PteCount = 0;
                    }
                    ValidPteList[PteCount++] = PointerPte;
                }
                else
                {
                    //
                    // We do not support any of these other scenarios at the moment
                    //
                    ASSERT(PteContents.u.Soft.Prototype == 0);
                    ASSERT(PteContents.u.Soft.Transition == 0);
                    ASSERT(PteContents.u.Soft.PageFileHigh == 0);

                    //
                    // So the only other possibility is that it is still a demand
                    // zero PTE, in which case we undo the accounting we did
                    // earlier and simply make the page decommitted.
                    //
                    //Process->NumberOfPrivatePages++;
                    MI_WRITE_INVALID_PTE(PointerPte, MmDecommittedPte);
                }
            }
        }
        else
        {
            //
            // This used to be a zero PTE and it no longer is, so we must add a
            // reference to the pagetable.
            //
            MiIncrementPageTableReferences(StartingAddress);

            //
            // Next, we account for decommitted PTEs and make the PTE as such
            //
            if (PointerPte > CommitPte) CommitReduction++;
            MI_WRITE_INVALID_PTE(PointerPte, MmDecommittedPte);
        }

        //
        // Move to the next PTE and the next address
        //
        PointerPte++;
        StartingAddress = (PVOID)((ULONG_PTR)StartingAddress + PAGE_SIZE);
    }

    //
    // Flush any dangling PTEs from the loop in the last page table, and then
    // release the working set and return the commit reduction accounting.
    //
    if (PteCount) MiProcessValidPteList(ValidPteList, PteCount);
    MiUnlockProcessWorkingSetUnsafe(Process, CurrentThread);
    return CommitReduction;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
PVOID
NTAPI
MmGetVirtualForPhysical(IN PHYSICAL_ADDRESS PhysicalAddress)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmSecureVirtualMemory(IN PVOID Address,
                      IN SIZE_T Length,
                      IN ULONG Mode)
{
    static ULONG Warn; if (!Warn++) UNIMPLEMENTED;
    return Address;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnsecureVirtualMemory(IN PVOID SecureMem)
{
    static ULONG Warn; if (!Warn++) UNIMPLEMENTED;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtReadVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID BaseAddress,
                    OUT PVOID Buffer,
                    IN SIZE_T NumberOfBytesToRead,
                    OUT PSIZE_T NumberOfBytesRead OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T BytesRead = 0;
    PAGED_CODE();

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Validate the read addresses
        //
        if ((((ULONG_PTR)BaseAddress + NumberOfBytesToRead) < (ULONG_PTR)BaseAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToRead) < (ULONG_PTR)Buffer) ||
            (((ULONG_PTR)BaseAddress + NumberOfBytesToRead) > MmUserProbeAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToRead) > MmUserProbeAddress))
        {
            //
            // Don't allow to write into kernel space
            //
            return STATUS_ACCESS_VIOLATION;
        }

        //
        // Enter SEH for probe
        //
        _SEH2_TRY
        {
            //
            // Probe the output value
            //
            if (NumberOfBytesRead) ProbeForWriteSize_t(NumberOfBytesRead);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    //
    // Don't do zero-byte transfers
    //
    if (NumberOfBytesToRead)
    {
        //
        // Reference the process
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_READ,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)(&Process),
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            //
            // Do the copy
            //
            Status = MmCopyVirtualMemory(Process,
                                         BaseAddress,
                                         PsGetCurrentProcess(),
                                         Buffer,
                                         NumberOfBytesToRead,
                                         PreviousMode,
                                         &BytesRead);

            //
            // Dereference the process
            //
            ObDereferenceObject(Process);
        }
    }

    //
    // Check if the caller sent this parameter
    //
    if (NumberOfBytesRead)
    {
        //
        // Enter SEH to guard write
        //
        _SEH2_TRY
        {
            //
            // Return the number of bytes read
            //
            *NumberOfBytesRead = BytesRead;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }

    //
    // Return status
    //
    return Status;
}

NTSTATUS
NTAPI
NtWriteVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress,
                     IN PVOID Buffer,
                     IN SIZE_T NumberOfBytesToWrite,
                     OUT PSIZE_T NumberOfBytesWritten OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T BytesWritten = 0;
    PAGED_CODE();

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Validate the read addresses
        //
        if ((((ULONG_PTR)BaseAddress + NumberOfBytesToWrite) < (ULONG_PTR)BaseAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToWrite) < (ULONG_PTR)Buffer) ||
            (((ULONG_PTR)BaseAddress + NumberOfBytesToWrite) > MmUserProbeAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToWrite) > MmUserProbeAddress))
        {
            //
            // Don't allow to write into kernel space
            //
            return STATUS_ACCESS_VIOLATION;
        }

        //
        // Enter SEH for probe
        //
        _SEH2_TRY
        {
            //
            // Probe the output value
            //
            if (NumberOfBytesWritten) ProbeForWriteSize_t(NumberOfBytesWritten);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    //
    // Don't do zero-byte transfers
    //
    if (NumberOfBytesToWrite)
    {
        //
        // Reference the process
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_WRITE,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            //
            // Do the copy
            //
            Status = MmCopyVirtualMemory(PsGetCurrentProcess(),
                                         Buffer,
                                         Process,
                                         BaseAddress,
                                         NumberOfBytesToWrite,
                                         PreviousMode,
                                         &BytesWritten);

            //
            // Dereference the process
            //
            ObDereferenceObject(Process);
        }
    }

    //
    // Check if the caller sent this parameter
    //
    if (NumberOfBytesWritten)
    {
        //
        // Enter SEH to guard write
        //
        _SEH2_TRY
        {
            //
            // Return the number of bytes written
            //
            *NumberOfBytesWritten = BytesWritten;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }

    //
    // Return status
    //
    return Status;
}

NTSTATUS
NTAPI
NtFlushInstructionCache(_In_ HANDLE ProcessHandle,
                        _In_opt_ PVOID BaseAddress,
                        _In_ SIZE_T FlushSize)
{
    KAPC_STATE ApcState;
    PKPROCESS Process;
    NTSTATUS Status;
    PAGED_CODE();

    /* Is a base address given? */
    if (BaseAddress != NULL)
    {
        /* If the requested size is 0, there is nothing to do */
        if (FlushSize == 0)
        {
            return STATUS_SUCCESS;
        }

        /* Is this a user mode call? */
        if (ExGetPreviousMode() != KernelMode)
        {
            /* Make sure the base address is in user space */
            if (BaseAddress > MmHighestUserAddress)
            {
                DPRINT1("Invalid BaseAddress 0x%p\n", BaseAddress);
                return STATUS_ACCESS_VIOLATION;
            }
        }
    }

    /* Is another process requested? */
    if (ProcessHandle != NtCurrentProcess())
    {
        /* Reference the process */
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_WRITE,
                                           PsProcessType,
                                           ExGetPreviousMode(),
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference the process %p\n", ProcessHandle);
            return Status;
        }

        /* Attach to the process */
        KeStackAttachProcess(Process, &ApcState);
    }

    /* Forward to Ke */
    KeSweepICache(BaseAddress, FlushSize);

    /* Check if we attached */
    if (ProcessHandle != NtCurrentProcess())
    {
        /* Detach from the process and dereference it */
        KeUnstackDetachProcess(&ApcState);
        ObDereferenceObject(Process);
    }

    /* All done, return to caller */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtProtectVirtualMemory(IN HANDLE ProcessHandle,
                       IN OUT PVOID *UnsafeBaseAddress,
                       IN OUT SIZE_T *UnsafeNumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG UnsafeOldAccessProtection)
{
    PEPROCESS Process;
    ULONG OldAccessProtection;
    ULONG Protection;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    PVOID BaseAddress = NULL;
    SIZE_T NumberOfBytesToProtect = 0;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    PAGED_CODE();

    //
    // Check for valid protection flags
    //
    Protection = NewAccessProtection & ~(PAGE_GUARD|PAGE_NOCACHE);
    if (Protection != PAGE_NOACCESS &&
        Protection != PAGE_READONLY &&
        Protection != PAGE_READWRITE &&
        Protection != PAGE_WRITECOPY &&
        Protection != PAGE_EXECUTE &&
        Protection != PAGE_EXECUTE_READ &&
        Protection != PAGE_EXECUTE_READWRITE &&
        Protection != PAGE_EXECUTE_WRITECOPY)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Enter SEH for probing
        //
        _SEH2_TRY
        {
            //
            // Validate all outputs
            //
            ProbeForWritePointer(UnsafeBaseAddress);
            ProbeForWriteSize_t(UnsafeNumberOfBytesToProtect);
            ProbeForWriteUlong(UnsafeOldAccessProtection);

            //
            // Capture them
            //
            BaseAddress = *UnsafeBaseAddress;
            NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        //
        // Capture directly
        //
        BaseAddress = *UnsafeBaseAddress;
        NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
    }

    //
    // Catch illegal base address
    //
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER_2;

    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < NumberOfBytesToProtect)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // 0 is also illegal
    //
    if (!NumberOfBytesToProtect) return STATUS_INVALID_PARAMETER_3;

    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Check if we should attach
    //
    if (CurrentProcess != Process)
    {
        //
        // Do it
        //
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    //
    // Do the actual work
    //
    Status = MiProtectVirtualMemory(Process,
                                    &BaseAddress,
                                    &NumberOfBytesToProtect,
                                    NewAccessProtection,
                                    &OldAccessProtection);

    //
    // Detach if needed
    //
    if (Attached) KeUnstackDetachProcess(&ApcState);

    //
    // Release reference
    //
    ObDereferenceObject(Process);

    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *UnsafeOldAccessProtection = OldAccessProtection;
        *UnsafeBaseAddress = BaseAddress;
        *UnsafeNumberOfBytesToProtect = NumberOfBytesToProtect;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    //
    // Return status
    //
    return Status;
}

FORCEINLINE
BOOLEAN
MI_IS_LOCKED_VA(
    PMMPFN Pfn1,
    ULONG LockType)
{
    // HACK until we have proper WSLIST support
    PMMWSLE Wsle = &Pfn1->Wsle;

    if ((LockType & MAP_PROCESS) && (Wsle->u1.e1.LockedInWs))
        return TRUE;
    if ((LockType & MAP_SYSTEM) && (Wsle->u1.e1.LockedInMemory))
        return TRUE;

    return FALSE;
}

FORCEINLINE
VOID
MI_LOCK_VA(
    PMMPFN Pfn1,
    ULONG LockType)
{
    // HACK until we have proper WSLIST support
    PMMWSLE Wsle = &Pfn1->Wsle;

    if (!Wsle->u1.e1.LockedInWs &&
        !Wsle->u1.e1.LockedInMemory)
    {
        MiReferenceProbedPageAndBumpLockCount(Pfn1);
    }

    if (LockType & MAP_PROCESS)
        Wsle->u1.e1.LockedInWs = 1;
    if (LockType & MAP_SYSTEM)
        Wsle->u1.e1.LockedInMemory = 1;
}

FORCEINLINE
VOID
MI_UNLOCK_VA(
    PMMPFN Pfn1,
    ULONG LockType)
{
    // HACK until we have proper WSLIST support
    PMMWSLE Wsle = &Pfn1->Wsle;

    if (LockType & MAP_PROCESS)
        Wsle->u1.e1.LockedInWs = 0;
    if (LockType & MAP_SYSTEM)
        Wsle->u1.e1.LockedInMemory = 0;

    if (!Wsle->u1.e1.LockedInWs &&
        !Wsle->u1.e1.LockedInMemory)
    {
        MiDereferencePfnAndDropLockCount(Pfn1);
    }
}

static
NTSTATUS
MiCheckVadsForLockOperation(
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _Inout_ PVOID *EndAddress)

{
    PMMVAD Vad;
    PVOID CurrentVa;

    /* Get the base address and align the start address */
    *EndAddress = (PUCHAR)*BaseAddress + *RegionSize;
    *EndAddress = ALIGN_UP_POINTER_BY(*EndAddress, PAGE_SIZE);
    *BaseAddress = ALIGN_DOWN_POINTER_BY(*BaseAddress, PAGE_SIZE);

    /* First loop and check all VADs */
    CurrentVa = *BaseAddress;
    while (CurrentVa < *EndAddress)
    {
        /* Get VAD */
        Vad = MiLocateAddress(CurrentVa);
        if (Vad == NULL)
        {
            /// FIXME: this might be a memory area for a section view...
            return STATUS_ACCESS_VIOLATION;
        }

        /* Check VAD type */
        if ((Vad->u.VadFlags.VadType != VadNone) &&
            (Vad->u.VadFlags.VadType != VadImageMap) &&
            (Vad->u.VadFlags.VadType != VadWriteWatch))
        {
            *EndAddress = CurrentVa;
            *RegionSize = (PUCHAR)*EndAddress - (PUCHAR)*BaseAddress;
            return STATUS_INCOMPATIBLE_FILE_MAP;
        }

        CurrentVa = (PVOID)((Vad->EndingVpn + 1) << PAGE_SHIFT);
    }

    *RegionSize = (PUCHAR)*EndAddress - (PUCHAR)*BaseAddress;
    return STATUS_SUCCESS;
}

static
NTSTATUS
MiLockVirtualMemory(
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG MapType)
{
    PEPROCESS CurrentProcess;
    PMMSUPPORT AddressSpace;
    PVOID CurrentVa, EndAddress;
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPDE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS == 4)
    PMMPDE PointerPxe;
#endif
    PMMPFN Pfn1;
    NTSTATUS Status, TempStatus;

    /* Lock the address space */
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);

    /* Make sure we still have an address space */
    CurrentProcess = PsGetCurrentProcess();
    if (CurrentProcess->VmDeleted)
    {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto Cleanup;
    }

    /* Check the VADs in the requested range */
    Status = MiCheckVadsForLockOperation(BaseAddress, RegionSize, &EndAddress);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Enter SEH for probing */
    _SEH2_TRY
    {
        /* Loop all pages and probe them */
        CurrentVa = *BaseAddress;
        while (CurrentVa < EndAddress)
        {
            (void)(*(volatile CHAR*)CurrentVa);
            CurrentVa = (PUCHAR)CurrentVa + PAGE_SIZE;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        goto Cleanup;
    }
    _SEH2_END;

    /* All pages were accessible, since we hold the address space lock, nothing
       can be de-committed. Assume success for now. */
    Status = STATUS_SUCCESS;

    /* Get the PTE and PDE */
    PointerPte = MiAddressToPte(*BaseAddress);
    PointerPde = MiAddressToPde(*BaseAddress);
#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiAddressToPpe(*BaseAddress);
#endif
#if (_MI_PAGING_LEVELS == 4)
    PointerPxe = MiAddressToPxe(*BaseAddress);
#endif

    /* Get the last PTE */
    LastPte = MiAddressToPte((PVOID)((ULONG_PTR)EndAddress - 1));

    /* Lock the process working set */
    MiLockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());

    /* Loop the pages */
    do
    {
        /* Check for a page that is not accessible */
        while (
#if (_MI_PAGING_LEVELS == 4)
               (PointerPxe->u.Hard.Valid == 0) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
               (PointerPpe->u.Hard.Valid == 0) ||
#endif
               (PointerPde->u.Hard.Valid == 0) ||
               (PointerPte->u.Hard.Valid == 0))
        {
            /* Release process working set */
            MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());

            /* Access the page */
            CurrentVa = MiPteToAddress(PointerPte);

            //HACK: Pass a placeholder TrapInformation so the fault handler knows we're unlocked
            TempStatus = MmAccessFault(TRUE, CurrentVa, KernelMode, (PVOID)(ULONG_PTR)0xBADBADA3BADBADA3ULL);
            if (!NT_SUCCESS(TempStatus))
            {
                // This should only happen, when remote backing storage is not accessible
                ASSERT(FALSE);
                Status = TempStatus;
                goto Cleanup;
            }

            /* Lock the process working set */
            MiLockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
        }

        /* Get the PFN */
        Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
        ASSERT(Pfn1 != NULL);

        /* Check the previous lock status */
        if (MI_IS_LOCKED_VA(Pfn1, MapType))
        {
            Status = STATUS_WAS_LOCKED;
        }

        /* Lock it */
        MI_LOCK_VA(Pfn1, MapType);

        /* Go to the next PTE */
        PointerPte++;

        /* Check if we're on a PDE boundary */
        if (MiIsPteOnPdeBoundary(PointerPte)) PointerPde++;
#if (_MI_PAGING_LEVELS >= 3)
        if (MiIsPteOnPpeBoundary(PointerPte)) PointerPpe++;
#endif
#if (_MI_PAGING_LEVELS == 4)
        if (MiIsPteOnPxeBoundary(PointerPte)) PointerPxe++;
#endif
    } while (PointerPte <= LastPte);

    /* Release process working set */
    MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());

Cleanup:
    /* Unlock address space */
    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

NTSTATUS
NTAPI
NtLockVirtualMemory(IN HANDLE ProcessHandle,
                    IN OUT PVOID *BaseAddress,
                    IN OUT PSIZE_T NumberOfBytesToLock,
                    IN ULONG MapType)
{
    PEPROCESS Process;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    NTSTATUS Status;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID CapturedBaseAddress;
    SIZE_T CapturedBytesToLock;
    PAGED_CODE();

    //
    // Validate flags
    //
    if ((MapType & ~(MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // Invalid set of flags
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // At least one flag must be specified
    //
    if (!(MapType & (MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // No flag given
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Enter SEH for probing
    //
    _SEH2_TRY
    {
        //
        // Validate output data
        //
        ProbeForWritePointer(BaseAddress);
        ProbeForWriteSize_t(NumberOfBytesToLock);

        //
        // Capture it
        //
        CapturedBaseAddress = *BaseAddress;
        CapturedBytesToLock = *NumberOfBytesToLock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Catch illegal base address
    //
    if (CapturedBaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;

    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)CapturedBaseAddress) < CapturedBytesToLock)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // 0 is also illegal
    //
    if (!CapturedBytesToLock) return STATUS_INVALID_PARAMETER;

    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Check if this is is system-mapped
    //
    if (MapType & MAP_SYSTEM)
    {
        //
        // Check for required privilege
        //
        if (!SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode))
        {
            //
            // Fail: Don't have it
            //
            ObDereferenceObject(Process);
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    //
    // Check if we should attach
    //
    if (CurrentProcess != Process)
    {
        //
        // Do it
        //
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    //
    // Call the internal function
    //
    Status = MiLockVirtualMemory(&CapturedBaseAddress,
                                 &CapturedBytesToLock,
                                 MapType);

    //
    // Detach if needed
    //
    if (Attached) KeUnstackDetachProcess(&ApcState);

    //
    // Release reference
    //
    ObDereferenceObject(Process);

    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *BaseAddress = CapturedBaseAddress;
        *NumberOfBytesToLock = CapturedBytesToLock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Return status
    //
    return Status;
}


static
NTSTATUS
MiUnlockVirtualMemory(
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG MapType)
{
    PEPROCESS CurrentProcess;
    PMMSUPPORT AddressSpace;
    PVOID EndAddress;
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPDE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS == 4)
    PMMPDE PointerPxe;
#endif
    PMMPFN Pfn1;
    NTSTATUS Status;

    /* Lock the address space */
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);

    /* Make sure we still have an address space */
    CurrentProcess = PsGetCurrentProcess();
    if (CurrentProcess->VmDeleted)
    {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto Cleanup;
    }

    /* Check the VADs in the requested range */
    Status = MiCheckVadsForLockOperation(BaseAddress, RegionSize, &EndAddress);

    /* Note: only bail out, if we hit an area without a VAD. If we hit an
       incompatible VAD we continue, like Windows does */
    if (Status == STATUS_ACCESS_VIOLATION)
    {
        Status = STATUS_NOT_LOCKED;
        goto Cleanup;
    }

    /* Get the PTE and PDE */
    PointerPte = MiAddressToPte(*BaseAddress);
    PointerPde = MiAddressToPde(*BaseAddress);
#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiAddressToPpe(*BaseAddress);
#endif
#if (_MI_PAGING_LEVELS == 4)
    PointerPxe = MiAddressToPxe(*BaseAddress);
#endif

    /* Get the last PTE */
    LastPte = MiAddressToPte((PVOID)((ULONG_PTR)EndAddress - 1));

    /* Lock the process working set */
    MiLockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());

    /* Loop the pages */
    do
    {
        /* Check for a page that is not present */
        if (
#if (_MI_PAGING_LEVELS == 4)
               (PointerPxe->u.Hard.Valid == 0) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
               (PointerPpe->u.Hard.Valid == 0) ||
#endif
               (PointerPde->u.Hard.Valid == 0) ||
               (PointerPte->u.Hard.Valid == 0))
        {
            /* Remember it, but keep going */
            Status = STATUS_NOT_LOCKED;
        }
        else
        {
            /* Get the PFN */
            Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
            ASSERT(Pfn1 != NULL);

            /* Check if all of the requested locks are present */
            if (((MapType & MAP_SYSTEM) && !MI_IS_LOCKED_VA(Pfn1, MAP_SYSTEM)) ||
                ((MapType & MAP_PROCESS) && !MI_IS_LOCKED_VA(Pfn1, MAP_PROCESS)))
            {
                /* Remember it, but keep going */
                Status = STATUS_NOT_LOCKED;

                /* Check if no lock is present */
                if (!MI_IS_LOCKED_VA(Pfn1, MAP_PROCESS | MAP_SYSTEM))
                {
                    DPRINT1("FIXME: Should remove the page from WS\n");
                }
            }
        }

        /* Go to the next PTE */
        PointerPte++;

        /* Check if we're on a PDE boundary */
        if (MiIsPteOnPdeBoundary(PointerPte)) PointerPde++;
#if (_MI_PAGING_LEVELS >= 3)
        if (MiIsPteOnPpeBoundary(PointerPte)) PointerPpe++;
#endif
#if (_MI_PAGING_LEVELS == 4)
        if (MiIsPteOnPxeBoundary(PointerPte)) PointerPxe++;
#endif
    } while (PointerPte <= LastPte);

    /* Check if we hit a page that was not locked */
    if (Status == STATUS_NOT_LOCKED)
    {
        goto CleanupWithWsLock;
    }

    /* All pages in the region were locked, so unlock them all */

    /* Get the PTE and PDE */
    PointerPte = MiAddressToPte(*BaseAddress);
    PointerPde = MiAddressToPde(*BaseAddress);
#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiAddressToPpe(*BaseAddress);
#endif
#if (_MI_PAGING_LEVELS == 4)
    PointerPxe = MiAddressToPxe(*BaseAddress);
#endif

    /* Loop the pages */
    do
    {
        /* Unlock it */
        Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
        MI_UNLOCK_VA(Pfn1, MapType);

        /* Go to the next PTE */
        PointerPte++;

        /* Check if we're on a PDE boundary */
        if (MiIsPteOnPdeBoundary(PointerPte)) PointerPde++;
#if (_MI_PAGING_LEVELS >= 3)
        if (MiIsPteOnPpeBoundary(PointerPte)) PointerPpe++;
#endif
#if (_MI_PAGING_LEVELS == 4)
        if (MiIsPteOnPxeBoundary(PointerPte)) PointerPxe++;
#endif
    } while (PointerPte <= LastPte);

    /* Everything is done */
    Status = STATUS_SUCCESS;

CleanupWithWsLock:

    /* Release process working set */
    MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());

Cleanup:
    /* Unlock address space */
    MmUnlockAddressSpace(AddressSpace);

    return Status;
}


NTSTATUS
NTAPI
NtUnlockVirtualMemory(IN HANDLE ProcessHandle,
                      IN OUT PVOID *BaseAddress,
                      IN OUT PSIZE_T NumberOfBytesToUnlock,
                      IN ULONG MapType)
{
    PEPROCESS Process;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    NTSTATUS Status;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID CapturedBaseAddress;
    SIZE_T CapturedBytesToUnlock;
    PAGED_CODE();

    //
    // Validate flags
    //
    if ((MapType & ~(MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // Invalid set of flags
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // At least one flag must be specified
    //
    if (!(MapType & (MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // No flag given
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Enter SEH for probing
    //
    _SEH2_TRY
    {
        //
        // Validate output data
        //
        ProbeForWritePointer(BaseAddress);
        ProbeForWriteSize_t(NumberOfBytesToUnlock);

        //
        // Capture it
        //
        CapturedBaseAddress = *BaseAddress;
        CapturedBytesToUnlock = *NumberOfBytesToUnlock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Catch illegal base address
    //
    if (CapturedBaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;

    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)CapturedBaseAddress) < CapturedBytesToUnlock)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // 0 is also illegal
    //
    if (!CapturedBytesToUnlock) return STATUS_INVALID_PARAMETER;

    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Check if this is is system-mapped
    //
    if (MapType & MAP_SYSTEM)
    {
        //
        // Check for required privilege
        //
        if (!SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode))
        {
            //
            // Fail: Don't have it
            //
            ObDereferenceObject(Process);
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    //
    // Check if we should attach
    //
    if (CurrentProcess != Process)
    {
        //
        // Do it
        //
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    //
    // Call the internal function
    //
    Status = MiUnlockVirtualMemory(&CapturedBaseAddress,
                                   &CapturedBytesToUnlock,
                                   MapType);

    //
    // Detach if needed
    //
    if (Attached) KeUnstackDetachProcess(&ApcState);

    //
    // Release reference
    //
    ObDereferenceObject(Process);

    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *BaseAddress = CapturedBaseAddress;
        *NumberOfBytesToUnlock = CapturedBytesToUnlock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Return status
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtFlushVirtualMemory(IN HANDLE ProcessHandle,
                     IN OUT PVOID *BaseAddress,
                     IN OUT PSIZE_T NumberOfBytesToFlush,
                     OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PEPROCESS Process;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID CapturedBaseAddress;
    SIZE_T CapturedBytesToFlush;
    IO_STATUS_BLOCK LocalStatusBlock;
    PAGED_CODE();

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Enter SEH for probing
        //
        _SEH2_TRY
        {
            //
            // Validate all outputs
            //
            ProbeForWritePointer(BaseAddress);
            ProbeForWriteSize_t(NumberOfBytesToFlush);
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            //
            // Capture them
            //
            CapturedBaseAddress = *BaseAddress;
            CapturedBytesToFlush = *NumberOfBytesToFlush;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        //
        // Capture directly
        //
        CapturedBaseAddress = *BaseAddress;
        CapturedBytesToFlush = *NumberOfBytesToFlush;
    }

    //
    // Catch illegal base address
    //
    if (CapturedBaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;

    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)CapturedBaseAddress) < CapturedBytesToFlush)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Do it
    //
    Status = MmFlushVirtualMemory(Process,
                                  &CapturedBaseAddress,
                                  &CapturedBytesToFlush,
                                  &LocalStatusBlock);

    //
    // Release reference
    //
    ObDereferenceObject(Process);

    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *BaseAddress = PAGE_ALIGN(CapturedBaseAddress);
        *NumberOfBytesToFlush = 0;
        *IoStatusBlock = LocalStatusBlock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    //
    // Return status
    //
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtGetWriteWatch(IN HANDLE ProcessHandle,
                IN ULONG Flags,
                IN PVOID BaseAddress,
                IN SIZE_T RegionSize,
                IN PVOID *UserAddressArray,
                OUT PULONG_PTR EntriesInUserAddressArray,
                OUT PULONG Granularity)
{
    PEPROCESS Process;
    NTSTATUS Status;
    PVOID EndAddress;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG_PTR CapturedEntryCount;
    PAGED_CODE();

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Enter SEH for probing
        //
        _SEH2_TRY
        {
            //
            // Catch illegal base address
            //
            if (BaseAddress > MM_HIGHEST_USER_ADDRESS) _SEH2_YIELD(return STATUS_INVALID_PARAMETER_2);

            //
            // Catch illegal region size
            //
            if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < RegionSize)
            {
                //
                // Fail
                //
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER_3);
            }

            //
            // Validate all data
            //
            ProbeForWriteSize_t(EntriesInUserAddressArray);
            ProbeForWriteUlong(Granularity);

            //
            // Capture them
            //
            CapturedEntryCount = *EntriesInUserAddressArray;

            //
            // Must have a count
            //
            if (CapturedEntryCount == 0) _SEH2_YIELD(return STATUS_INVALID_PARAMETER_5);

            //
            // Can't be larger than the maximum
            //
            if (CapturedEntryCount > (MAXULONG_PTR / sizeof(ULONG_PTR)))
            {
                //
                // Fail
                //
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER_5);
            }

            //
            // Probe the actual array
            //
            ProbeForWrite(UserAddressArray,
                          CapturedEntryCount * sizeof(PVOID),
                          sizeof(PVOID));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        //
        // Capture directly
        //
        CapturedEntryCount = *EntriesInUserAddressArray;
        ASSERT(CapturedEntryCount != 0);
    }

    //
    // Check if this is a local request
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        //
        // No need to reference the process
        //
        Process = PsGetCurrentProcess();
    }
    else
    {
        //
        // Reference the target
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Compute the last address and validate it
    //
    EndAddress = (PVOID)((ULONG_PTR)BaseAddress + RegionSize - 1);
    if (BaseAddress > EndAddress)
    {
        //
        // Fail
        //
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Oops :(
    //
    UNIMPLEMENTED;

    //
    // Dereference if needed
    //
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *EntriesInUserAddressArray = 0;
        *Granularity = PAGE_SIZE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtResetWriteWatch(IN HANDLE ProcessHandle,
                  IN PVOID BaseAddress,
                  IN SIZE_T RegionSize)
{
    PVOID EndAddress;
    PEPROCESS Process;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Catch illegal base address
    //
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER_2;

    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < RegionSize)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // Check if this is a local request
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        //
        // No need to reference the process
        //
        Process = PsGetCurrentProcess();
    }
    else
    {
        //
        // Reference the target
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Compute the last address and validate it
    //
    EndAddress = (PVOID)((ULONG_PTR)BaseAddress + RegionSize - 1);
    if (BaseAddress > EndAddress)
    {
        //
        // Fail
        //
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // Oops :(
    //
    UNIMPLEMENTED;

    //
    // Dereference if needed
    //
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtQueryVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress,
                     IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
                     OUT PVOID MemoryInformation,
                     IN SIZE_T MemoryInformationLength,
                     OUT PSIZE_T ReturnLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode;

    DPRINT("Querying class %d about address: %p\n", MemoryInformationClass, BaseAddress);

    /* Bail out if the address is invalid */
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;

    /* Probe return buffer */
    PreviousMode =  ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWrite(MemoryInformation,
                          MemoryInformationLength,
                          sizeof(ULONG_PTR));

            if (ReturnLength) ProbeForWriteSize_t(ReturnLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    switch(MemoryInformationClass)
    {
        case MemoryBasicInformation:
            /* Validate the size information of the class */
            if (MemoryInformationLength < sizeof(MEMORY_BASIC_INFORMATION))
            {
                /* The size is invalid */
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            Status = MiQueryMemoryBasicInformation(ProcessHandle,
                                                   BaseAddress,
                                                   MemoryInformation,
                                                   MemoryInformationLength,
                                                   ReturnLength);
            break;

        case MemorySectionName:
            /* Validate the size information of the class */
            if (MemoryInformationLength < sizeof(MEMORY_SECTION_NAME))
            {
                /* The size is invalid */
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            Status = MiQueryMemorySectionName(ProcessHandle,
                                              BaseAddress,
                                              MemoryInformation,
                                              MemoryInformationLength,
                                              ReturnLength);
            break;
        case MemoryWorkingSetList:
        case MemoryBasicVlmInformation:
        default:
            DPRINT1("Unhandled memory information class %d\n", MemoryInformationClass);
            break;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAllocateVirtualMemory(IN HANDLE ProcessHandle,
                        IN OUT PVOID* UBaseAddress,
                        IN ULONG_PTR ZeroBits,
                        IN OUT PSIZE_T URegionSize,
                        IN ULONG AllocationType,
                        IN ULONG Protect)
{
    PEPROCESS Process;
    PMEMORY_AREA MemoryArea;
    PMMVAD Vad = NULL, FoundVad;
    NTSTATUS Status;
    PMMSUPPORT AddressSpace;
    PVOID PBaseAddress;
    ULONG_PTR PRegionSize, StartingAddress, EndingAddress;
    ULONG_PTR HighestAddress = (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD CurrentThread = PsGetCurrentThread();
    KAPC_STATE ApcState;
    ULONG ProtectionMask, QuotaCharge = 0, QuotaFree = 0;
    BOOLEAN Attached = FALSE, ChangeProtection = FALSE;
    MMPTE TempPte;
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
    TABLE_SEARCH_RESULT Result;
    PAGED_CODE();

    /* Check for valid Zero bits */
    if (ZeroBits > MI_MAX_ZERO_BITS)
    {
        DPRINT1("Too many zero bits\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check for valid Allocation Types */
    if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE | MEM_RESET | MEM_PHYSICAL |
                    MEM_TOP_DOWN | MEM_WRITE_WATCH | MEM_LARGE_PAGES)))
    {
        DPRINT1("Invalid Allocation Type\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check for at least one of these Allocation Types to be set */
    if (!(AllocationType & (MEM_COMMIT | MEM_RESERVE | MEM_RESET)))
    {
        DPRINT1("No memory allocation base type\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* MEM_RESET is an exclusive flag, make sure that is valid too */
    if ((AllocationType & MEM_RESET) && (AllocationType != MEM_RESET))
    {
        DPRINT1("Invalid use of MEM_RESET\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check if large pages are being used */
    if (AllocationType & MEM_LARGE_PAGES)
    {
        /* Large page allocations MUST be committed */
        if (!(AllocationType & MEM_COMMIT))
        {
            DPRINT1("Must supply MEM_COMMIT with MEM_LARGE_PAGES\n");
            return STATUS_INVALID_PARAMETER_5;
        }

        /* These flags are not allowed with large page allocations */
        if (AllocationType & (MEM_PHYSICAL | MEM_RESET | MEM_WRITE_WATCH))
        {
            DPRINT1("Using illegal flags with MEM_LARGE_PAGES\n");
            return STATUS_INVALID_PARAMETER_5;
        }
    }

    /* MEM_WRITE_WATCH can only be used if MEM_RESERVE is also used */
    if ((AllocationType & MEM_WRITE_WATCH) && !(AllocationType & MEM_RESERVE))
    {
        DPRINT1("MEM_WRITE_WATCH used without MEM_RESERVE\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check for valid MEM_PHYSICAL usage */
    if (AllocationType & MEM_PHYSICAL)
    {
        /* MEM_PHYSICAL can only be used if MEM_RESERVE is also used */
        if (!(AllocationType & MEM_RESERVE))
        {
            DPRINT1("MEM_PHYSICAL used without MEM_RESERVE\n");
            return STATUS_INVALID_PARAMETER_5;
        }

        /* Only these flags are allowed with MEM_PHYSIAL */
        if (AllocationType & ~(MEM_RESERVE | MEM_TOP_DOWN | MEM_PHYSICAL))
        {
            DPRINT1("Using illegal flags with MEM_PHYSICAL\n");
            return STATUS_INVALID_PARAMETER_5;
        }

        /* Then make sure PAGE_READWRITE is used */
        if (Protect != PAGE_READWRITE)
        {
            DPRINT1("MEM_PHYSICAL used without PAGE_READWRITE\n");
            return STATUS_INVALID_PARAMETER_6;
        }
    }

    /* Calculate the protection mask and make sure it's valid */
    ProtectionMask = MiMakeProtectionMask(Protect);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid protection mask\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Check for user-mode parameters */
        if (PreviousMode != KernelMode)
        {
            /* Make sure they are writable */
            ProbeForWritePointer(UBaseAddress);
            ProbeForWriteSize_t(URegionSize);
        }

        /* Capture their values */
        PBaseAddress = *UBaseAddress;
        PRegionSize = *URegionSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Make sure the allocation isn't past the VAD area */
    if (PBaseAddress > MM_HIGHEST_VAD_ADDRESS)
    {
        DPRINT1("Virtual allocation base above User Space\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Make sure the allocation wouldn't overflow past the VAD area */
    if ((((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) - (ULONG_PTR)PBaseAddress) < PRegionSize)
    {
        DPRINT1("Region size would overflow into kernel-memory\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Make sure there's a size specified */
    if (!PRegionSize)
    {
        DPRINT1("Region size is invalid (zero)\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // If this is for the current process, just use PsGetCurrentProcess
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        Process = CurrentProcess;
    }
    else
    {
        //
        // Otherwise, reference the process with VM rights and attach to it if
        // this isn't the current process. We must attach because we'll be touching
        // PTEs and PDEs that belong to user-mode memory, and also touching the
        // Working Set which is stored in Hyperspace.
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
        if (CurrentProcess != Process)
        {
            KeStackAttachProcess(&Process->Pcb, &ApcState);
            Attached = TRUE;
        }
    }

    DPRINT("NtAllocateVirtualMemory: Process 0x%p, Address 0x%p, Zerobits %lu , RegionSize 0x%x, Allocation type 0x%x, Protect 0x%x.\n",
        Process, PBaseAddress, ZeroBits, PRegionSize, AllocationType, Protect);

    //
    // Check for large page allocations and make sure that the required privilege
    // is being held, before attempting to handle them.
    //
    if ((AllocationType & MEM_LARGE_PAGES) &&
        !(SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode)))
    {
        /* Fail without it */
        DPRINT1("Privilege not held for MEM_LARGE_PAGES\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto FailPathNoLock;
    }

    //
    // Fail on the things we don't yet support
    //
    if ((AllocationType & MEM_LARGE_PAGES) == MEM_LARGE_PAGES)
    {
        DPRINT1("MEM_LARGE_PAGES not supported\n");
        Status = STATUS_INVALID_PARAMETER;
        goto FailPathNoLock;
    }
    if ((AllocationType & MEM_PHYSICAL) == MEM_PHYSICAL)
    {
        DPRINT1("MEM_PHYSICAL not supported\n");
        Status = STATUS_INVALID_PARAMETER;
        goto FailPathNoLock;
    }
    if ((AllocationType & MEM_WRITE_WATCH) == MEM_WRITE_WATCH)
    {
        DPRINT1("MEM_WRITE_WATCH not supported\n");
        Status = STATUS_INVALID_PARAMETER;
        goto FailPathNoLock;
    }

    //
    // Check if the caller is reserving memory, or committing memory and letting
    // us pick the base address
    //
    if (!(PBaseAddress) || (AllocationType & MEM_RESERVE))
    {
        //
        //  Do not allow COPY_ON_WRITE through this API
        //
        if (Protect & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
        {
            DPRINT1("Copy on write not allowed through this path\n");
            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto FailPathNoLock;
        }

        //
        // Does the caller have an address in mind, or is this a blind commit?
        //
        if (!PBaseAddress)
        {
            //
            // This is a blind commit, all we need is the region size
            //
            PRegionSize = ROUND_TO_PAGES(PRegionSize);
            EndingAddress = 0;
            StartingAddress = 0;

            //
            // Check if ZeroBits were specified
            //
            if (ZeroBits != 0)
            {
                //
                // Calculate the highest address and check if it's valid
                //
                HighestAddress = MAXULONG_PTR >> ZeroBits;
                if (HighestAddress > (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS)
                {
                    Status = STATUS_INVALID_PARAMETER_3;
                    goto FailPathNoLock;
                }
            }
        }
        else
        {
            //
            // This is a reservation, so compute the starting address on the
            // expected 64KB granularity, and see where the ending address will
            // fall based on the aligned address and the passed in region size
            //
            EndingAddress = ((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
            PRegionSize = EndingAddress + 1 - ROUND_DOWN((ULONG_PTR)PBaseAddress, _64K);
            StartingAddress = (ULONG_PTR)PBaseAddress;
        }

        //
        // Allocate and initialize the VAD
        //
        Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'SdaV');
        if (Vad == NULL)
        {
            DPRINT1("Failed to allocate a VAD!\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FailPathNoLock;
        }

        RtlZeroMemory(Vad, sizeof(MMVAD_LONG));
        if (AllocationType & MEM_COMMIT) Vad->u.VadFlags.MemCommit = 1;
        Vad->u.VadFlags.Protection = ProtectionMask;
        Vad->u.VadFlags.PrivateMemory = 1;
        Vad->ControlArea = NULL; // For Memory-Area hack

        //
        // Insert the VAD
        //
        Status = MiInsertVadEx(Vad,
                               &StartingAddress,
                               PRegionSize,
                               HighestAddress,
                               MM_VIRTMEM_GRANULARITY,
                               AllocationType);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert the VAD!\n");
            goto FailPathNoLock;
        }

        //
        // Detach and dereference the target process if
        // it was different from the current process
        //
        if (Attached) KeUnstackDetachProcess(&ApcState);
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

        //
        // Use SEH to write back the base address and the region size. In the case
        // of an exception, we do not return back the exception code, as the memory
        // *has* been allocated. The caller would now have to call VirtualQuery
        // or do some other similar trick to actually find out where its memory
        // allocation ended up
        //
        _SEH2_TRY
        {
            *URegionSize = PRegionSize;
            *UBaseAddress = (PVOID)StartingAddress;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Ignore exception!
            //
        }
        _SEH2_END;
        DPRINT("Reserved %x bytes at %p.\n", PRegionSize, StartingAddress);
        return STATUS_SUCCESS;
    }

    //
    // This is a MEM_COMMIT on top of an existing address which must have been
    // MEM_RESERVED already. Compute the start and ending base addresses based
    // on the user input, and then compute the actual region size once all the
    // alignments have been done.
    //
    EndingAddress = (((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1));
    StartingAddress = (ULONG_PTR)PAGE_ALIGN(PBaseAddress);
    PRegionSize = EndingAddress - StartingAddress + 1;

    //
    // Lock the address space and make sure the process isn't already dead
    //
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);
    if (Process->VmDeleted)
    {
        DPRINT1("Process is dying\n");
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto FailPath;
    }

    //
    // Get the VAD for this address range, and make sure it exists
    //
    Result = MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                       EndingAddress >> PAGE_SHIFT,
                                       &Process->VadRoot,
                                       (PMMADDRESS_NODE*)&FoundVad);
    if (Result != TableFoundNode)
    {
        DPRINT1("Could not find a VAD for this allocation\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    if ((AllocationType & MEM_RESET) == MEM_RESET)
    {
        /// @todo HACK: pretend success
        DPRINT("MEM_RESET not supported\n");
        Status = STATUS_SUCCESS;
        goto FailPath;
    }

    //
    // These kinds of VADs are illegal for this Windows function when trying to
    // commit an existing range
    //
    if ((FoundVad->u.VadFlags.VadType == VadAwe) ||
        (FoundVad->u.VadFlags.VadType == VadDevicePhysicalMemory) ||
        (FoundVad->u.VadFlags.VadType == VadLargePages))
    {
        DPRINT1("Illegal VAD for attempting a MEM_COMMIT\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    //
    // Make sure that this address range actually fits within the VAD for it
    //
    if (((StartingAddress >> PAGE_SHIFT) < FoundVad->StartingVpn) ||
        ((EndingAddress >> PAGE_SHIFT) > FoundVad->EndingVpn))
    {
        DPRINT1("Address range does not fit into the VAD\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    //
    // Make sure this is an ARM3 section
    //
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)PAGE_ROUND_DOWN(PBaseAddress));
    ASSERT(MemoryArea != NULL);
    if (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3)
    {
        DPRINT1("Illegal commit of non-ARM3 section!\n");
        Status = STATUS_ALREADY_COMMITTED;
        goto FailPath;
    }

    // Is this a previously reserved section being committed? If so, enter the
    // special section path
    //
    if (FoundVad->u.VadFlags.PrivateMemory == FALSE)
    {
        //
        // You cannot commit large page sections through this API
        //
        if (FoundVad->u.VadFlags.VadType == VadLargePageSection)
        {
            DPRINT1("Large page sections cannot be VirtualAlloc'd\n");
            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto FailPath;
        }

        //
        // You can only use caching flags on a rotate VAD
        //
        if ((Protect & (PAGE_NOCACHE | PAGE_WRITECOMBINE)) &&
            (FoundVad->u.VadFlags.VadType != VadRotatePhysical))
        {
            DPRINT1("Cannot use caching flags with anything but rotate VADs\n");
            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto FailPath;
        }

        //
        // We should make sure that the section's permissions aren't being
        // messed with
        //
        if (FoundVad->u.VadFlags.NoChange)
        {
            //
            // Make sure it's okay to touch it
            // Note: The Windows 2003 kernel has a bug here, passing the
            // unaligned base address together with the aligned size,
            // potentially covering a region larger than the actual allocation.
            // Might be exposed through NtGdiCreateDIBSection w/ section handle
            // For now we keep this behavior.
            // TODO: analyze possible implications, create test case
            //
            Status = MiCheckSecuredVad(FoundVad,
                                       PBaseAddress,
                                       PRegionSize,
                                       ProtectionMask);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Secured VAD being messed around with\n");
                goto FailPath;
            }
        }

        //
        // ARM3 does not support file-backed sections, only shared memory
        //
        ASSERT(FoundVad->ControlArea->FilePointer == NULL);

        //
        // Rotate VADs cannot be guard pages or inaccessible, nor copy on write
        //
        if ((FoundVad->u.VadFlags.VadType == VadRotatePhysical) &&
            (Protect & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY | PAGE_NOACCESS | PAGE_GUARD)))
        {
            DPRINT1("Invalid page protection for rotate VAD\n");
            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto FailPath;
        }

        //
        // Compute PTE addresses and the quota charge, then grab the commit lock
        //
        PointerPte = MI_GET_PROTOTYPE_PTE_FOR_VPN(FoundVad, StartingAddress >> PAGE_SHIFT);
        LastPte = MI_GET_PROTOTYPE_PTE_FOR_VPN(FoundVad, EndingAddress >> PAGE_SHIFT);
        QuotaCharge = (ULONG)(LastPte - PointerPte + 1);
        KeAcquireGuardedMutexUnsafe(&MmSectionCommitMutex);

        //
        // Get the segment template PTE and start looping each page
        //
        TempPte = FoundVad->ControlArea->Segment->SegmentPteTemplate;
        ASSERT(TempPte.u.Long != 0);
        while (PointerPte <= LastPte)
        {
            //
            // For each non-already-committed page, write the invalid template PTE
            //
            if (PointerPte->u.Long == 0)
            {
                MI_WRITE_INVALID_PTE(PointerPte, TempPte);
            }
            else
            {
                QuotaFree++;
            }
            PointerPte++;
        }

        //
        // Now do the commit accounting and release the lock
        //
        ASSERT(QuotaCharge >= QuotaFree);
        QuotaCharge -= QuotaFree;
        FoundVad->ControlArea->Segment->NumberOfCommittedPages += QuotaCharge;
        KeReleaseGuardedMutexUnsafe(&MmSectionCommitMutex);

        //
        // We are done with committing the section pages
        //
        Status = STATUS_SUCCESS;
        goto FailPath;
    }

    //
    // This is a specific ReactOS check because we only use normal VADs
    //
    ASSERT(FoundVad->u.VadFlags.VadType == VadNone);

    //
    // While this is an actual Windows check
    //
    ASSERT(FoundVad->u.VadFlags.VadType != VadRotatePhysical);

    //
    // Throw out attempts to use copy-on-write through this API path
    //
    if ((Protect & PAGE_WRITECOPY) || (Protect & PAGE_EXECUTE_WRITECOPY))
    {
        DPRINT1("Write copy attempted when not allowed\n");
        Status = STATUS_INVALID_PAGE_PROTECTION;
        goto FailPath;
    }

    //
    // Initialize a demand-zero PTE
    //
    TempPte.u.Long = 0;
    TempPte.u.Soft.Protection = ProtectionMask;
    ASSERT(TempPte.u.Long != 0);

    //
    // Get the PTE, PDE and the last PTE for this address range
    //
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    LastPte = MiAddressToPte(EndingAddress);

    //
    // Update the commit charge in the VAD as well as in the process, and check
    // if this commit charge was now higher than the last recorded peak, in which
    // case we also update the peak
    //
    FoundVad->u.VadFlags.CommitCharge += (1 + LastPte - PointerPte);
    Process->CommitCharge += (1 + LastPte - PointerPte);
    if (Process->CommitCharge > Process->CommitChargePeak)
    {
        Process->CommitChargePeak = Process->CommitCharge;
    }

    //
    // Lock the working set while we play with user pages and page tables
    //
    MiLockProcessWorkingSetUnsafe(Process, CurrentThread);

    //
    // Make the current page table valid, and then loop each page within it
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= LastPte)
    {
        //
        // Have we crossed into a new page table?
        //
        if (MiIsPteOnPdeBoundary(PointerPte))
        {
            //
            // Get the PDE and now make it valid too
            //
            PointerPde = MiPteToPde(PointerPte);
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }

        //
        // Is this a zero PTE as expected?
        //
        if (PointerPte->u.Long == 0)
        {
            //
            // First increment the count of pages in the page table for this
            // process
            //
            MiIncrementPageTableReferences(MiPteToAddress(PointerPte));

            //
            // And now write the invalid demand-zero PTE as requested
            //
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        }
        else if (PointerPte->u.Long == MmDecommittedPte.u.Long)
        {
            //
            // If the PTE was already decommitted, there is nothing else to do
            // but to write the new demand-zero PTE
            //
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        }
        else if (!(ChangeProtection) && (Protect != MiGetPageProtection(PointerPte)))
        {
            //
            // We don't handle these scenarios yet
            //
            if (PointerPte->u.Soft.Valid == 0)
            {
                ASSERT(PointerPte->u.Soft.Prototype == 0);
                ASSERT((PointerPte->u.Soft.PageFileHigh == 0) || (PointerPte->u.Soft.Transition == 1));
            }

            //
            // There's a change in protection, remember this for later, but do
            // not yet handle it.
            //
            ChangeProtection = TRUE;
        }

        //
        // Move to the next PTE
        //
        PointerPte++;
    }

    //
    // Release the working set lock, unlock the address space, and detach from
    // the target process if it was not the current process. Also dereference the
    // target process if this wasn't the case.
    //
    MiUnlockProcessWorkingSetUnsafe(Process, CurrentThread);
    Status = STATUS_SUCCESS;
FailPath:
    MmUnlockAddressSpace(AddressSpace);

    if (!NT_SUCCESS(Status))
    {
        if (Vad != NULL)
        {
            ExFreePoolWithTag(Vad, 'SdaV');
        }
    }

    //
    // Check if we need to update the protection
    //
    if (ChangeProtection)
    {
        PVOID ProtectBaseAddress = (PVOID)StartingAddress;
        SIZE_T ProtectSize = PRegionSize;
        ULONG OldProtection;

        //
        // Change the protection of the region
        //
        MiProtectVirtualMemory(Process,
                               &ProtectBaseAddress,
                               &ProtectSize,
                               Protect,
                               &OldProtection);
    }

FailPathNoLock:
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

    //
    // Only write back results on success
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Use SEH to write back the base address and the region size. In the case
        // of an exception, we strangely do return back the exception code, even
        // though the memory *has* been allocated. This mimics Windows behavior and
        // there is not much we can do about it.
        //
        _SEH2_TRY
        {
            *URegionSize = PRegionSize;
            *UBaseAddress = (PVOID)StartingAddress;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtFreeVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID* UBaseAddress,
                    IN PSIZE_T URegionSize,
                    IN ULONG FreeType)
{
    PMEMORY_AREA MemoryArea;
    SIZE_T PRegionSize;
    PVOID PBaseAddress;
    LONG_PTR AlreadyDecommitted, CommitReduction = 0;
    ULONG_PTR StartingAddress, EndingAddress;
    PMMVAD Vad;
    NTSTATUS Status;
    PEPROCESS Process;
    PMMSUPPORT AddressSpace;
    PETHREAD CurrentThread = PsGetCurrentThread();
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    KAPC_STATE ApcState;
    BOOLEAN Attached = FALSE;
    PAGED_CODE();

    //
    // Only two flags are supported, exclusively.
    //
    if (FreeType != MEM_RELEASE && FreeType != MEM_DECOMMIT)
    {
        DPRINT1("Invalid FreeType (0x%08lx)\n", FreeType);
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Enter SEH for probe and capture. On failure, return back to the caller
    // with an exception violation.
    //
    _SEH2_TRY
    {
        //
        // Check for user-mode parameters and make sure that they are writeable
        //
        if (PreviousMode != KernelMode)
        {
            ProbeForWritePointer(UBaseAddress);
            ProbeForWriteUlong(URegionSize);
        }

        //
        // Capture the current values
        //
        PBaseAddress = *UBaseAddress;
        PRegionSize = *URegionSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Make sure the allocation isn't past the user area
    //
    if (PBaseAddress >= MM_HIGHEST_USER_ADDRESS)
    {
        DPRINT1("Virtual free base above User Space\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Make sure the allocation wouldn't overflow past the user area
    //
    if (((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (ULONG_PTR)PBaseAddress) < PRegionSize)
    {
        DPRINT1("Region size would overflow into kernel-memory\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // If this is for the current process, just use PsGetCurrentProcess
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        Process = CurrentProcess;
    }
    else
    {
        //
        // Otherwise, reference the process with VM rights and attach to it if
        // this isn't the current process. We must attach because we'll be touching
        // PTEs and PDEs that belong to user-mode memory, and also touching the
        // Working Set which is stored in Hyperspace.
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
        if (CurrentProcess != Process)
        {
            KeStackAttachProcess(&Process->Pcb, &ApcState);
            Attached = TRUE;
        }
    }

    DPRINT("NtFreeVirtualMemory: Process 0x%p, Address 0x%p, Size 0x%Ix, FreeType 0x%08lx\n",
           Process, PBaseAddress, PRegionSize, FreeType);

    //
    // Lock the address space
    //
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);

    //
    // If the address space is being deleted, fail the de-allocation since it's
    // too late to do anything about it
    //
    if (Process->VmDeleted)
    {
        DPRINT1("Process is dead\n");
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto FailPath;
    }

    //
    // Compute start and end addresses, and locate the VAD
    //
    StartingAddress = (ULONG_PTR)PAGE_ALIGN(PBaseAddress);
    EndingAddress = ((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
    Vad = MiLocateAddress((PVOID)StartingAddress);
    if (!Vad)
    {
        DPRINT1("Unable to find VAD for address 0x%p\n", StartingAddress);
        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto FailPath;
    }

    //
    // If the range exceeds the VAD's ending VPN, fail this request
    //
    if (Vad->EndingVpn < (EndingAddress >> PAGE_SHIFT))
    {
        DPRINT1("Address 0x%p is beyond the VAD\n", EndingAddress);
        Status = STATUS_UNABLE_TO_FREE_VM;
        goto FailPath;
    }

    //
    // Only private memory (except rotate VADs) can be freed through here */
    //
    if ((!(Vad->u.VadFlags.PrivateMemory) &&
         (Vad->u.VadFlags.VadType != VadRotatePhysical)) ||
        (Vad->u.VadFlags.VadType == VadDevicePhysicalMemory))
    {
        DPRINT("Attempt to free section memory\n");
        Status = STATUS_UNABLE_TO_DELETE_SECTION;
        goto FailPath;
    }

    //
    // ARM3 does not yet handle protected VM
    //
    ASSERT(Vad->u.VadFlags.NoChange == 0);

    //
    // Finally, make sure there is a ReactOS Mm MEMORY_AREA for this allocation
    // and that is is an ARM3 memory area, and not a section view, as we currently
    // don't support freeing those though this interface.
    //
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)StartingAddress);
    ASSERT(MemoryArea);
    ASSERT(MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3);

    //
    //  Now we can try the operation. First check if this is a RELEASE or a DECOMMIT
    //
    if (FreeType & MEM_RELEASE)
    {
        //
        // ARM3 only supports this VAD in this path
        //
        ASSERT(Vad->u.VadFlags.VadType == VadNone);

        //
        // Is the caller trying to remove the whole VAD, or remove only a portion
        // of it? If no region size is specified, then the assumption is that the
        // whole VAD is to be destroyed
        //
        if (!PRegionSize)
        {
            //
            // The caller must specify the base address identically to the range
            // that is stored in the VAD.
            //
            if (((ULONG_PTR)PBaseAddress >> PAGE_SHIFT) != Vad->StartingVpn)
            {
                DPRINT1("Address 0x%p does not match the VAD\n", PBaseAddress);
                Status = STATUS_FREE_VM_NOT_AT_BASE;
                goto FailPath;
            }

            //
            // Now compute the actual start/end addresses based on the VAD
            //
            StartingAddress = Vad->StartingVpn << PAGE_SHIFT;
            EndingAddress = (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1);

            //
            // Finally lock the working set and remove the VAD from the VAD tree
            //
            MiLockProcessWorkingSetUnsafe(Process, CurrentThread);
            ASSERT(Process->VadRoot.NumberGenericTableElements >= 1);
            MiRemoveNode((PMMADDRESS_NODE)Vad, &Process->VadRoot);
        }
        else
        {
            //
            // This means the caller wants to release a specific region within
            // the range. We have to find out which range this is -- the following
            // possibilities exist plus their union (CASE D):
            //
            // STARTING ADDRESS                                   ENDING ADDRESS
            // [<========][========================================][=========>]
            //   CASE A                  CASE B                       CASE C
            //
            //
            // First, check for case A or D
            //
            if ((StartingAddress >> PAGE_SHIFT) == Vad->StartingVpn)
            {
                //
                // Check for case D
                //
                if ((EndingAddress >> PAGE_SHIFT) == Vad->EndingVpn)
                {
                    //
                    // This is the easiest one to handle -- it is identical to
                    // the code path above when the caller sets a zero region size
                    // and the whole VAD is destroyed
                    //
                    MiLockProcessWorkingSetUnsafe(Process, CurrentThread);
                    ASSERT(Process->VadRoot.NumberGenericTableElements >= 1);
                    MiRemoveNode((PMMADDRESS_NODE)Vad, &Process->VadRoot);
                }
                else
                {
                    //
                    // This case is pretty easy too -- we compute a bunch of
                    // pages to decommit, and then push the VAD's starting address
                    // a bit further down, then decrement the commit charge
                    //
                    // NOT YET IMPLEMENTED IN ARM3.
                    //
                    DPRINT1("Case A not handled\n");
                    Status = STATUS_FREE_VM_NOT_AT_BASE;
                    goto FailPath;

                    //
                    // After analyzing the VAD, set it to NULL so that we don't
                    // free it in the exit path
                    //
                    Vad = NULL;
                }
            }
            else
            {
                //
                // This is case B or case C. First check for case C
                //
                if ((EndingAddress >> PAGE_SHIFT) == Vad->EndingVpn)
                {
                    PMEMORY_AREA MemoryArea;

                    //
                    // This is pretty easy and similar to case A. We compute the
                    // amount of pages to decommit, update the VAD's commit charge
                    // and then change the ending address of the VAD to be a bit
                    // smaller.
                    //
                    MiLockProcessWorkingSetUnsafe(Process, CurrentThread);
                    CommitReduction = MiCalculatePageCommitment(StartingAddress,
                                                                EndingAddress,
                                                                Vad,
                                                                Process);
                    Vad->u.VadFlags.CommitCharge -= CommitReduction;
                    // For ReactOS: shrink the corresponding memory area
                    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)StartingAddress);
                    ASSERT(Vad->StartingVpn == MemoryArea->VadNode.StartingVpn);
                    ASSERT(Vad->EndingVpn == MemoryArea->VadNode.EndingVpn);
                    Vad->EndingVpn = (StartingAddress - 1) >> PAGE_SHIFT;
                    MemoryArea->VadNode.EndingVpn = Vad->EndingVpn;
                }
                else
                {
                    //
                    // This is case B and the hardest one. Because we are removing
                    // a chunk of memory from the very middle of the VAD, we must
                    // actually split the VAD into two new VADs and compute the
                    // commit charges for each of them, and reinsert new charges.
                    //
                    // NOT YET IMPLEMENTED IN ARM3.
                    //
                    DPRINT1("Case B not handled\n");
                    Status = STATUS_FREE_VM_NOT_AT_BASE;
                    goto FailPath;
                }

                //
                // After analyzing the VAD, set it to NULL so that we don't
                // free it in the exit path
                //
                Vad = NULL;
            }
        }

        //
        // Now we have a range of pages to dereference, so call the right API
        // to do that and then release the working set, since we're done messing
        // around with process pages.
        //
        MiDeleteVirtualAddresses(StartingAddress, EndingAddress, NULL);
        MiUnlockProcessWorkingSetUnsafe(Process, CurrentThread);
        Status = STATUS_SUCCESS;

FinalPath:
        //
        // Update the process counters
        //
        PRegionSize = EndingAddress - StartingAddress + 1;
        Process->CommitCharge -= CommitReduction;
        if (FreeType & MEM_RELEASE) Process->VirtualSize -= PRegionSize;

        //
        // Unlock the address space and free the VAD in failure cases. Next,
        // detach from the target process so we can write the region size and the
        // base address to the correct source process, and dereference the target
        // process.
        //
        MmUnlockAddressSpace(AddressSpace);
        if (Vad) ExFreePool(Vad);
        if (Attached) KeUnstackDetachProcess(&ApcState);
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

        //
        // Use SEH to safely return the region size and the base address of the
        // deallocation. If we get an access violation, don't return a failure code
        // as the deallocation *has* happened. The caller will just have to figure
        // out another way to find out where it is (such as VirtualQuery).
        //
        _SEH2_TRY
        {
            *URegionSize = PRegionSize;
            *UBaseAddress = (PVOID)StartingAddress;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
        return Status;
    }

    //
    // This is the decommit path. You cannot decommit from the following VADs in
    // Windows, so fail the vall
    //
    if ((Vad->u.VadFlags.VadType == VadAwe) ||
        (Vad->u.VadFlags.VadType == VadLargePages) ||
        (Vad->u.VadFlags.VadType == VadRotatePhysical))
    {
        DPRINT1("Trying to decommit from invalid VAD\n");
        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto FailPath;
    }

    //
    // If the caller did not specify a region size, first make sure that this
    // region is actually committed. If it is, then compute the ending address
    // based on the VAD.
    //
    if (!PRegionSize)
    {
        if (((ULONG_PTR)PBaseAddress >> PAGE_SHIFT) != Vad->StartingVpn)
        {
            DPRINT1("Decomitting non-committed memory\n");
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto FailPath;
        }
        EndingAddress = (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1);
    }

    //
    // Decommit the PTEs for the range plus the actual backing pages for the
    // range, then reduce that amount from the commit charge in the VAD
    //
    AlreadyDecommitted = MiDecommitPages((PVOID)StartingAddress,
                                         MiAddressToPte(EndingAddress),
                                         Process,
                                         Vad);
    CommitReduction = MiAddressToPte(EndingAddress) -
                      MiAddressToPte(StartingAddress) +
                      1 -
                      AlreadyDecommitted;

    ASSERT(CommitReduction >= 0);
    ASSERT(Vad->u.VadFlags.CommitCharge >= CommitReduction);
    Vad->u.VadFlags.CommitCharge -= CommitReduction;

    //
    // We are done, go to the exit path without freeing the VAD as it remains
    // valid since we have not released the allocation.
    //
    Vad = NULL;
    Status = STATUS_SUCCESS;
    goto FinalPath;

    //
    // In the failure path, we detach and dereference the target process, and
    // return whatever failure code was sent.
    //
FailPath:
    MmUnlockAddressSpace(AddressSpace);
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
    return Status;
}


PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(PVOID Address)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    MMPDE TempPde;
    MMPTE TempPte;

    /* Check if the PXE/PPE/PDE is valid */
    if (
#if (_MI_PAGING_LEVELS == 4)
        (MiAddressToPxe(Address)->u.Hard.Valid) &&
#endif
#if (_MI_PAGING_LEVELS >= 3)
        (MiAddressToPpe(Address)->u.Hard.Valid) &&
#endif
        (MiAddressToPde(Address)->u.Hard.Valid))
    {
        /* Check for large pages */
        TempPde = *MiAddressToPde(Address);
        if (TempPde.u.Hard.LargePage)
        {
            /* Physical address is base page + large page offset */
            PhysicalAddress.QuadPart = (ULONG64)TempPde.u.Hard.PageFrameNumber << PAGE_SHIFT;
            PhysicalAddress.QuadPart += ((ULONG_PTR)Address & (PAGE_SIZE * PTE_PER_PAGE - 1));
            return PhysicalAddress;
        }

        /* Check if the PTE is valid */
        TempPte = *MiAddressToPte(Address);
        if (TempPte.u.Hard.Valid)
        {
            /* Physical address is base page + page offset */
            PhysicalAddress.QuadPart = (ULONG64)TempPte.u.Hard.PageFrameNumber << PAGE_SHIFT;
            PhysicalAddress.QuadPart += ((ULONG_PTR)Address & (PAGE_SIZE - 1));
            return PhysicalAddress;
        }
    }

    KeRosDumpStackFrames(NULL, 20);
    DPRINT1("MM:MmGetPhysicalAddressFailed base address was %p\n", Address);
    PhysicalAddress.QuadPart = 0;
    return PhysicalAddress;
}


/* EOF */
