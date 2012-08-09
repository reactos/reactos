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
#include "../ARM3/miarm.h"

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
    PMMPTE PointerPte, LastPte, PointerPde;
    ULONG CommittedPages;

    /* Compute starting and ending PTE and PDE addresses */
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    LastPte = MiAddressToPte(EndingAddress);

    /* Handle commited pages first */
    if (Vad->u.VadFlags.MemCommit == 1)
    {
        /* This is a committed VAD, so Assume the whole range is committed */
        CommittedPages = BYTES_TO_PAGES(EndingAddress - StartingAddress);

        /* Is the PDE demand-zero? */
        PointerPde = MiAddressToPte(PointerPte);
        if (PointerPde->u.Long != 0)
        {
            /* It is not. Is it valid? */
            if (PointerPde->u.Hard.Valid == 0)
            {
                /* Fault it in */
                PointerPte = MiPteToAddress(PointerPde);
                MiMakeSystemAddressValid(PointerPte, Process);
            }
        }
        else
        {
            /* It is, skip it and move to the next PDE, unless we're done */
            PointerPde++;
            PointerPte = MiPteToAddress(PointerPde);
            if (PointerPte > LastPte) return CommittedPages;
        }

        /* Now loop all the PTEs in the range */
        while (PointerPte <= LastPte)
        {
            /* Have we crossed a PDE boundary? */
            if (MiIsPteOnPdeBoundary(PointerPte))
            {
                /* Is this PDE demand zero? */
                PointerPde = MiAddressToPte(PointerPte);
                if (PointerPde->u.Long != 0)
                {
                    /* It isn't -- is it valid? */
                    if (PointerPde->u.Hard.Valid == 0)
                    {
                        /* Nope, fault it in */
                        PointerPte = MiPteToAddress(PointerPde);
                        MiMakeSystemAddressValid(PointerPte, Process);
                    }
                }
                else
                {
                    /* It is, skip it and move to the next PDE */
                    PointerPde++;
                    PointerPte = MiPteToAddress(PointerPde);
                    continue;
                }
            }

            /* Is this PTE demand zero? */
            if (PointerPte->u.Long != 0)
            {
                /* It isn't -- is it a decommited, invalid, or faulted PTE? */
                if ((PointerPte->u.Soft.Protection == MM_DECOMMIT) &&
                    (PointerPte->u.Hard.Valid == 0) &&
                    ((PointerPte->u.Soft.Prototype == 0) ||
                     (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)))
                {
                    /* It is, so remove it from the count of commited pages */
                    CommittedPages--;
                }
            }

            /* Move to the next PTE */
            PointerPte++;
        }

        /* Return how many committed pages there still are */
        return CommittedPages;
    }

    /* This is a non-commited VAD, so assume none of it is committed */
    CommittedPages = 0;

    /* Is the PDE demand-zero? */
    PointerPde = MiAddressToPte(PointerPte);
    if (PointerPde->u.Long != 0)
    {
        /* It isn't -- is it invalid? */
        if (PointerPde->u.Hard.Valid == 0)
        {
            /* It is, so page it in */
            PointerPte = MiPteToAddress(PointerPde);
            MiMakeSystemAddressValid(PointerPte, Process);
        }
    }
    else
    {
        /* It is, so skip it and move to the next PDE */
        PointerPde++;
        PointerPte = MiPteToAddress(PointerPde);
        if (PointerPte > LastPte) return CommittedPages;
    }

    /* Loop all the PTEs in this PDE */
    while (PointerPte <= LastPte)
    {
        /* Have we crossed a PDE boundary? */
        if (MiIsPteOnPdeBoundary(PointerPte))
        {
            /* Is this new PDE demand-zero? */
            PointerPde = MiAddressToPte(PointerPte);
            if (PointerPde->u.Long != 0)
            {
                /* It isn't. Is it valid? */
                if (PointerPde->u.Hard.Valid == 0)
                {
                    /* It isn't, so make it valid */
                    PointerPte = MiPteToAddress(PointerPde);
                    MiMakeSystemAddressValid(PointerPte, Process);
                }
            }
            else
            {
                /* It is, so skip it and move to the next one */
                PointerPde++;
                PointerPte = MiPteToAddress(PointerPde);
                continue;
            }
        }

        /* Is this PTE demand-zero? */
        if (PointerPte->u.Long != 0)
        {
            /* Nope. Is it a valid, non-decommited, non-paged out PTE? */
            if ((PointerPte->u.Soft.Protection != MM_DECOMMIT) ||
                (PointerPte->u.Hard.Valid == 1) ||
                ((PointerPte->u.Soft.Prototype == 1) &&
                 (PointerPte->u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED)))
            {
                /* It is! So we'll treat this as a committed page */
                CommittedPages++;
            }
        }

        /* Move to the next PTE */
        PointerPte++;
    }

    /* Return how many committed pages we found in this VAD */
    return CommittedPages;
}

ULONG
NTAPI
MiMakeSystemAddressValid(IN PVOID PageTableVirtualAddress,
                         IN PEPROCESS CurrentProcess)
{
    NTSTATUS Status;
    BOOLEAN WsWasLocked = FALSE, LockChange = FALSE;
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
        /* Check if the WS is locked */
        if (CurrentThread->OwnsProcessWorkingSetExclusive)
        {
            /* Unlock the working set and remember it was locked */
            MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
            WsWasLocked = TRUE;
        }

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
        if (WsWasLocked) MiLockProcessWorkingSet(CurrentProcess, CurrentThread);

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
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

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
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
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
            /* As always, only handle current ARM3 scenarios */
            ASSERT(PointerPte->u.Soft.Prototype == 0);
            ASSERT(PointerPte->u.Soft.Transition == 0);

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
                OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

                /* Delete it the page */
                MI_SET_PFN_DELETED(Pfn1);
                MiDecrementShareCount(Pfn1, PageFrameIndex);

                /* Decrement the page table too */
                MiDecrementShareCount(Pfn2, PageTableIndex);

                /* Release the PFN database */
                KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

                /* Destroy the PTE */
                PointerPte->u.Long = 0;
            }

            /* Actual legitimate pages */
            ActualPages++;
        }
        else
        {
            /*
             * The only other ARM3 possibility is a demand zero page, which would
             * mean freeing some of the paged pool pages that haven't even been
             * touched yet, as part of a larger allocation.
             *
             * Right now, we shouldn't expect any page file information in the PTE
             */
            ASSERT(PointerPte->u.Soft.PageFileHigh == 0);

            /* Destroy the PTE */
            PointerPte->u.Long = 0;
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
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Capture the PTE */
    TempPte = *PointerPte;

    /* We only support valid PTEs for now */
    ASSERT(TempPte.u.Hard.Valid == 1);
    if (TempPte.u.Hard.Valid == 0)
    {
        /* Invalid PTEs not supported yet */
        ASSERT(TempPte.u.Soft.Prototype == 0);
        ASSERT(TempPte.u.Soft.Transition == 0);
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

    /* Destroy the PTE and flush the TLB */
    PointerPte->u.Long = 0;
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
    MMPTE TempPte;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    BOOLEAN AddressGap = FALSE;
    PSUBSECTION Subsection;
    PUSHORT UsedPageTableEntries;

    /* Get out if this is a fake VAD, RosMm will free the marea pages */
    if ((Vad) && (Vad->u.VadFlags.Spare == 1)) return;

    /* Grab the process and PTE/PDE for the address being deleted */
    CurrentProcess = PsGetCurrentProcess();
    PointerPde = MiAddressToPde(Va);
    PointerPte = MiAddressToPte(Va);

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

    /* Loop the PTE for each VA */
    while (TRUE)
    {
        /* First keep going until we find a valid PDE */
        while (!PointerPde->u.Long)
        {
            /* There are gaps in the address space */
            AddressGap = TRUE;

            /* Still no valid PDE, try the next 4MB (or whatever) */
            PointerPde++;

            /* Update the PTE on this new boundary */
            PointerPte = MiPteToAddress(PointerPde);

            /* Check if all the PDEs are invalid, so there's nothing to free */
            Va = (ULONG_PTR)MiPteToAddress(PointerPte);
            if (Va > EndingAddress) return;
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
        UsedPageTableEntries = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Va)];

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
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        do
        {
            /* Capture the PDE and make sure it exists */
            TempPte = *PointerPte;
            if (TempPte.u.Long)
            {
                DPRINT("Decrement used PTEs by address: %lx\n", Va);
                (*UsedPageTableEntries)--;
                ASSERT((*UsedPageTableEntries) < PTE_COUNT);
                DPRINT("Refs: %lx\n", (*UsedPageTableEntries));

                /* Check if the PTE is actually mapped in */
                if (TempPte.u.Long & 0xFFFFFC01)
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
                        PointerPte->u.Long = 0;
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
                    PointerPte->u.Long = 0;
                }
            }

            /* Update the address and PTE for it */
            Va += PAGE_SIZE;
            PointerPte++;
            PrototypePte++;

            /* Making sure the PDE is still valid */
            ASSERT(PointerPde->u.Hard.Valid == 1);
        }
        while ((Va & (PDE_MAPPED_VA - 1)) && (Va <= EndingAddress));

        /* The PDE should still be valid at this point */
        ASSERT(PointerPde->u.Hard.Valid == 1);

        DPRINT("Should check if handles for: %p are zero (PDE: %lx)\n", Va, PointerPde->u.Hard.PageFrameNumber);
        if (!(*UsedPageTableEntries))
        {
            DPRINT("They are!\n");
            if (PointerPde->u.Long != 0)
            {
                DPRINT("PDE active: %lx in %16s\n", PointerPde->u.Hard.PageFrameNumber, CurrentProcess->ImageFileName);

                /* Delete the PTE proper */
                MiDeletePte(PointerPde,
                            MiPteToAddress(PointerPde),
                            CurrentProcess,
                            NULL);
            }
        }

        /* Release the lock and get out if we're done */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        if (Va > EndingAddress) return;

        /* Otherwise, we exited because we hit a new PDE boundary, so start over */
        PointerPde = MiAddressToPde(Va);
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
    volatile BOOLEAN FailedInProbe = FALSE, FailedInMapping = FALSE, FailedInMoving;
    volatile BOOLEAN PagesLocked;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    volatile PVOID MdlAddress;
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
        // Reset state for this pass
        //
        MdlAddress = NULL;
        PagesLocked = FALSE;
        FailedInMoving = FALSE;
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
                //
                // Use our SEH handler to pick this up
                //
                FailedInMapping = TRUE;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            //
            // Now let go of the source and grab to the target process
            //
            KeUnstackDetachProcess(&ApcState);
            KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

            //
            // Check if this is our first time through
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
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Now do the actual move
            //
            FailedInMoving = TRUE;
            RtlCopyMemory(CurrentTargetAddress, MdlAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            //
            // Detach from whoever we may be attached to
            //
            KeUnstackDetachProcess(&ApcState);

            //
            // Check if we had mapped the pages
            //
            if (MdlAddress) MmUnmapLockedPages(MdlAddress, Mdl);

            //
            // Check if we had locked the pages
            //
            if (PagesLocked) MmUnlockPages(Mdl);

            //
            // Check if we hit working set quota
            //
            if (_SEH2_GetExceptionCode() == STATUS_WORKING_SET_QUOTA)
            {
                //
                // Return the error
                //
                return STATUS_WORKING_SET_QUOTA;
            }

            //
            // Check if we failed during the probe or mapping
            //
            if ((FailedInProbe) || (FailedInMapping))
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(return Status);
            }

            //
            // Otherwise, we failed  probably during the move
            //
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                //
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
            }

            //
            // Return partial copy
            //
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH2_END;

        //
        // Check for SEH status
        //
        if (Status != STATUS_SUCCESS) return Status;

        //
        // Detach from target
        //
        KeUnstackDetachProcess(&ApcState);

        //
        // Unmap and unlock
        //
        MmUnmapLockedPages(MdlAddress, Mdl);
        MmUnlockPages(Mdl);

        //
        // Update location and size
        //
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress + CurrentSize);
    }

    //
    // All bytes read
    //
    *ReturnSize = BufferSize;
    return STATUS_SUCCESS;
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
    volatile BOOLEAN FailedInProbe = FALSE, FailedInMoving, HavePoolAddress = FALSE;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    PVOID PoolAddress;
    KAPC_STATE ApcState;
    BOOLEAN HaveBadAddress;
    ULONG_PTR BadAddress;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

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

        //
        // Reset state for this pass
        //
        FailedInMoving = FALSE;
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
            // Do the copy
            //
            RtlCopyMemory(PoolAddress, CurrentAddress, CurrentSize);

            //
            // Now let go of the source and grab to the target process
            //
            KeUnstackDetachProcess(&ApcState);
            KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

            //
            // Check if this is our first time through
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
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Now do the actual move
            //
            FailedInMoving = TRUE;
            RtlCopyMemory(CurrentTargetAddress, PoolAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            //
            // Detach from whoever we may be attached to
            //
            KeUnstackDetachProcess(&ApcState);

            //
            // Check if we had allocated pool
            //
            if (HavePoolAddress) ExFreePool(PoolAddress);

            //
            // Check if we failed during the probe
            //
            if (FailedInProbe)
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(return Status);
            }

            //
            // Otherwise, we failed, probably during the move
            //
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                //
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
            }

            //
            // Return partial copy
            //
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH2_END;

        //
        // Check for SEH status
        //
        if (Status != STATUS_SUCCESS) return Status;

        //
        // Detach from target
        //
        KeUnstackDetachProcess(&ApcState);

        //
        // Update location and size
        //
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress +
                                       CurrentSize);
    }

    //
    // Check if we had allocated pool
    //
    if (HavePoolAddress) ExFreePool(PoolAddress);

    //
    // All bytes read
    //
    *ReturnSize = BufferSize;
    return STATUS_SUCCESS;
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

    //
    // Fake success
    //
    return STATUS_SUCCESS;
}

ULONG
NTAPI
MiGetPageProtection(IN PMMPTE PointerPte)
{
    MMPTE TempPte;
    PMMPFN Pfn;
    PAGED_CODE();

    /* Copy this PTE's contents */
    TempPte = *PointerPte;

    /* Assure it's not totally zero */
    ASSERT(TempPte.u.Long);

    /* Check for a special prototype format */
    if (TempPte.u.Soft.Valid == 0 &&
        TempPte.u.Soft.Prototype == 1)
    {
        /* Unsupported now */
        UNIMPLEMENTED;
        ASSERT(FALSE);
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
    DPRINT1("Prototype PTE: %lx %p\n", TempPte.u.Hard.PageFrameNumber, Pfn);
    DPRINT1("VA: %p\n", MiPteToAddress(&TempPte));
    DPRINT1("Mask: %lx\n", TempPte.u.Soft.Protection);
    DPRINT1("Mask2: %lx\n", Pfn->OriginalPte.u.Soft.Protection);
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

    /* Return the next range */
    *NextVa = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);

    /* Is the PDE demand-zero? */
    if (PointerPde->u.Long != 0)
    {
        /* It is not. Is it valid? */
        if (PointerPde->u.Hard.Valid == 0)
        {
            /* Is isn't, fault it in */
            PointerPte = MiPteToAddress(PointerPde);
            MiMakeSystemAddressValid(PointerPte, TargetProcess);
            ValidPte = TRUE;
        }
    }
    else
    {
        /* It is, skip it and move to the next PDE */
        *NextVa = MiPdeToAddress(PointerPde + 1);
    }

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

        /* Check if we were attached */
        if (ProcessHandle != NtCurrentProcess())
        {
            /* Detach and derefernece the process */
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

    /* Lock the address space of the process */
    MmLockAddressSpace(&TargetProcess->Vm);

    /* Find the memory area the specified address belongs to */
    MemoryArea = MmLocateMemoryAreaByAddress(&TargetProcess->Vm, BaseAddress);
    ASSERT(MemoryArea != NULL);

    /* Determine information dependent on the memory area type */
    if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
    {
        Status = MmQuerySectionView(MemoryArea, BaseAddress, &MemoryInfo, &ResultLength);
        ASSERT(NT_SUCCESS(Status));
    }
    else
    {
        /* Build the initial information block */
        Address = PAGE_ALIGN(BaseAddress);
        MemoryInfo.BaseAddress = Address;
        MemoryInfo.AllocationBase = (PVOID)(Vad->StartingVpn << PAGE_SHIFT);
        MemoryInfo.AllocationProtect = MmProtectToValue[Vad->u.VadFlags.Protection];
        MemoryInfo.Type = MEM_PRIVATE;

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

        /* Now that we know the last VA address, calculate the region size */
        MemoryInfo.RegionSize = ((ULONG_PTR)Address - (ULONG_PTR)MemoryInfo.BaseAddress);
    }

    /* Unlock the address space of the process */
    MmUnlockAddressSpace(&TargetProcess->Vm);

    /* Check if we were attached */
    if (ProcessHandle != NtCurrentProcess())
    {
        /* Detach and derefernece the process */
        KeUnstackDetachProcess(&ApcState);
        ObDereferenceObject(TargetProcess);
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

    /* All went well */
    DPRINT("Base: %p AllocBase: %p AllocProtect: %lx Protect: %lx "
            "State: %lx Type: %lx Size: %lx\n",
            MemoryInfo.BaseAddress, MemoryInfo.AllocationBase,
            MemoryInfo.AllocationProtect, MemoryInfo.Protect,
            MemoryInfo.State, MemoryInfo.Type, MemoryInfo.RegionSize);

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
    PMMPTE PointerPde, PointerPte, LastPte;
    MMPTE PteContents;
    //PUSHORT UsedPageTableEntries;
    PMMPFN Pfn1;
    ULONG ProtectionMask;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check for ROS specific memory area */
    MemoryArea = MmLocateMemoryAreaByAddress(&Process->Vm, *BaseAddress);
    if ((MemoryArea) && (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW))
    {
        return MiRosProtectVirtualMemory(Process,
                                         BaseAddress,
                                         NumberOfBytesToProtect,
                                         NewAccessProtection,
                                         OldAccessProtection);
    }

    /* Calcualte base address for the VAD */
    StartingAddress = (ULONG_PTR)PAGE_ALIGN((*BaseAddress));
    EndingAddress = (((ULONG_PTR)*BaseAddress + *NumberOfBytesToProtect - 1) | (PAGE_SIZE - 1));

    /* Calculate the protection mask and make sure it's valid */
    ProtectionMask = MiMakeProtectionMask(NewAccessProtection);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid protection mask\n");
        return STATUS_INVALID_PAGE_PROTECTION;
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
    Vad = (PMMVAD)MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                            EndingAddress >> PAGE_SHIFT,
                                            &Process->VadRoot);
    if (!Vad)
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

    if (Vad->u.VadFlags.PrivateMemory == 0)
    {
        /* This is a section, handled by the ROS specific code above */
        UNIMPLEMENTED;
    }
    else
    {
        /* Private memory, check protection flags */
        if ((NewAccessProtection & PAGE_WRITECOPY) ||
            (NewAccessProtection & PAGE_EXECUTE_WRITECOPY))
        {
            Status = STATUS_INVALID_PARAMETER_4;
            goto FailPath;
        }

        //MiLockProcessWorkingSet(Thread, Process);

        /* TODO: Check if all pages in this range are committed */

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
            *OldAccessProtection = MiGetPageProtection(PointerPte);
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }
        else
        {
            /* Grab the old protection from the VAD itself */
            *OldAccessProtection = MmProtectToValue[Vad->u.VadFlags.Protection];
        }

        /* Loop all the PTEs now */
        while (PointerPte <= LastPte)
        {
            /* Check if we've crossed a PDE boundary and make the new PDE valid too */
            if ((((ULONG_PTR)PointerPte) & (SYSTEM_PD_SIZE - 1)) == 0)
            {
                PointerPde = MiAddressToPte(PointerPte);
                MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
            }

            /* Capture the PTE and see what we're dealing with */
            PteContents = *PointerPte;
            if (PteContents.u.Long == 0)
            {
                /* This used to be a zero PTE and it no longer is, so we must add a
                   reference to the pagetable. */
                //UsedPageTableEntries = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(MiPteToAddress(PointerPte))];
                //(*UsedPageTableEntries)++;
                //ASSERT((*UsedPageTableEntries) <= PTE_COUNT);
                DPRINT1("HACK: Not increasing UsedPageTableEntries count!\n");
            }
            else if (PteContents.u.Hard.Valid == 1)
            {
                /* Get the PFN entry */
                Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(&PteContents));

                /* We don't support this yet */
                ASSERT(Pfn1->u3.e1.PrototypePte == 0);

                /* Check if the page should not be accessible at all */
                if ((NewAccessProtection & PAGE_NOACCESS) ||
                    (NewAccessProtection & PAGE_GUARD))
                {
                    /* TODO */
                    UNIMPLEMENTED;
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
                ASSERT(PteContents.u.Soft.Transition == 0);

                /* The PTE is already demand-zero, just update the protection mask */
                PointerPte->u.Soft.Protection = ProtectionMask;
            }

            PointerPte++;
        }

        /* Unlock the working set and update quota charges if needed, then return */
        //MiUnlockProcessWorkingSet(Thread, Process);
    }

FailPath:
    /* Unlock the address space */
    MmUnlockAddressSpace(AddressSpace);

    /* Return parameters */
    *NumberOfBytesToProtect = (SIZE_T)((PUCHAR)EndingAddress - (PUCHAR)StartingAddress + 1);
    *BaseAddress = (PVOID)StartingAddress;

    return Status;
}

VOID
NTAPI
MiMakePdeExistAndMakeValid(IN PMMPTE PointerPde,
                           IN PEPROCESS TargetProcess,
                           IN KIRQL OldIrql)
{
   PMMPTE PointerPte, PointerPpe, PointerPxe;

   //
   // Sanity checks. The latter is because we only use this function with the
   // PFN lock not held, so it may go away in the future.
   //
   ASSERT(KeAreAllApcsDisabled() == TRUE);
   ASSERT(OldIrql == MM_NOIRQL);

   //
   // Also get the PPE and PXE. This is okay not to #ifdef because they will
   // return the same address as the PDE on 2-level page table systems.
   //
   // If everything is already valid, there is nothing to do.
   //
   PointerPpe = MiAddressToPte(PointerPde);
   PointerPxe = MiAddressToPde(PointerPde);
   if ((PointerPxe->u.Hard.Valid) &&
       (PointerPpe->u.Hard.Valid) &&
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

       //
       // First, make the PXE valid if needed
       //
       if (!PointerPxe->u.Hard.Valid)
       {
           MiMakeSystemAddressValid(PointerPpe, TargetProcess);
           ASSERT(PointerPxe->u.Hard.Valid == 1);
       }

       //
       // Next, the PPE
       //
       if (!PointerPpe->u.Hard.Valid)
       {
           MiMakeSystemAddressValid(PointerPde, TargetProcess);
           ASSERT(PointerPpe->u.Hard.Valid == 1);
       }

       //
       // And finally, make the PDE itself valid.
       //
       MiMakeSystemAddressValid(PointerPte, TargetProcess);

       //
       // This should've worked the first time so the loop is really just for
       // show -- ASSERT that we're actually NOT going to be looping.
       //
       ASSERT(PointerPxe->u.Hard.Valid == 1);
       ASSERT(PointerPpe->u.Hard.Valid == 1);
       ASSERT(PointerPde->u.Hard.Valid == 1);
   } while (!(PointerPxe->u.Hard.Valid) ||
            !(PointerPpe->u.Hard.Valid) ||
            !(PointerPde->u.Hard.Valid));
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
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
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
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

ULONG
NTAPI
MiDecommitPages(IN PVOID StartingAddress,
                IN PMMPTE EndingPte,
                IN PEPROCESS Process,
                IN PMMVAD Vad)
{
    PMMPTE PointerPde, PointerPte, CommitPte = NULL;
    ULONG CommitReduction = 0;
    PMMPTE ValidPteList[256];
    ULONG PteCount = 0;
    PMMPFN Pfn1;
    MMPTE PteContents;
    PUSHORT UsedPageTableEntries;
    PETHREAD CurrentThread = PsGetCurrentThread();

    //
    // Get the PTE and PTE for the address, and lock the working set
    // If this was a VAD for a MEM_COMMIT allocation, also figure out where the
    // commited range ends so that we can do the right accounting.
    //
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    if (Vad->u.VadFlags.MemCommit) CommitPte = MiAddressToPte(Vad->EndingVpn << PAGE_SHIFT);
    MiLockWorkingSet(CurrentThread, &Process->Vm);

    //
    // Make the PDE valid, and now loop through each page's worth of data
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= EndingPte)
    {
        //
        // Check if we've crossed a PDE boundary
        //
        if ((((ULONG_PTR)PointerPte) & (SYSTEM_PD_SIZE - 1)) == 0)
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
            UsedPageTableEntries = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(StartingAddress)];
            (*UsedPageTableEntries)++;
            ASSERT((*UsedPageTableEntries) <= PTE_COUNT);

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
    MiUnlockWorkingSet(CurrentThread, &Process->Vm);
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
    static BOOLEAN Warn; if (!Warn++) UNIMPLEMENTED;
    return Address;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnsecureVirtualMemory(IN PVOID SecureMem)
{
    static BOOLEAN Warn; if (!Warn++) UNIMPLEMENTED;
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
    // Oops :(
    //
    UNIMPLEMENTED;

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
        *NumberOfBytesToLock = 0;
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
    // Oops :(
    //
    UNIMPLEMENTED;

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
        *BaseAddress = PAGE_ALIGN(CapturedBaseAddress);
        *NumberOfBytesToUnlock = 0;
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
            if (CapturedEntryCount == 0) return STATUS_INVALID_PARAMETER_5;

            //
            // Can't be larger than the maximum
            //
            if (CapturedEntryCount > (MAXULONG_PTR / sizeof(ULONG_PTR)))
            {
                //
                // Fail
                //
                return STATUS_INVALID_PARAMETER_5;
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
    PFN_NUMBER PageCount;
    PMMVAD Vad, FoundVad;
    PUSHORT UsedPageTableEntries;
    NTSTATUS Status;
    PMMSUPPORT AddressSpace;
    PVOID PBaseAddress;
    ULONG_PTR PRegionSize, StartingAddress, EndingAddress;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD CurrentThread = PsGetCurrentThread();
    KAPC_STATE ApcState;
    ULONG ProtectionMask, QuotaCharge = 0, QuotaFree = 0;
    BOOLEAN Attached = FALSE, ChangeProtection = FALSE;
    MMPTE TempPte;
    PMMPTE PointerPte, PointerPde, LastPte;
    PAGED_CODE();

    /* Check for valid Zero bits */
    if (ZeroBits > 21)
    {
        DPRINT1("Too many zero bits\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check for valid Allocation Types */
    if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE | MEM_RESET | MEM_PHYSICAL |
                    MEM_TOP_DOWN | MEM_WRITE_WATCH)))
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

    /* MEM_PHYSICAL can only be used if MEM_RESERVE is also used */
    if ((AllocationType & MEM_PHYSICAL) && !(AllocationType & MEM_RESERVE))
    {
        DPRINT1("MEM_WRITE_WATCH used without MEM_RESERVE\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check for valid MEM_PHYSICAL usage */
    if (AllocationType & MEM_PHYSICAL)
    {
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

    //
    // Force PAGE_READWRITE for everything, for now
    //
    Protect = PAGE_READWRITE;

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
            ProbeForWriteUlong(URegionSize);
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
    if (PBaseAddress >= MM_HIGHEST_VAD_ADDRESS)
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
    // Assert on the things we don't yet support
    //
    ASSERT(ZeroBits == 0);
    ASSERT((AllocationType & MEM_LARGE_PAGES) == 0);
    ASSERT((AllocationType & MEM_PHYSICAL) == 0);
    ASSERT((AllocationType & MEM_WRITE_WATCH) == 0);
    ASSERT((AllocationType & MEM_TOP_DOWN) == 0);
    ASSERT((AllocationType & MEM_RESET) == 0);
    ASSERT(Process->VmTopDown == 0);

    //
    // Check if the caller is reserving memory, or committing memory and letting
    // us pick the base address
    //
    if (!(PBaseAddress) || (AllocationType & MEM_RESERVE))
    {
        //
        //  Do not allow COPY_ON_WRITE through this API
        //
        if ((Protect & PAGE_WRITECOPY) || (Protect & PAGE_EXECUTE_WRITECOPY))
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
            PageCount = BYTES_TO_PAGES(PRegionSize);
            EndingAddress = 0;
            StartingAddress = 0;
        }
        else
        {
            //
            // This is a reservation, so compute the starting address on the
            // expected 64KB granularity, and see where the ending address will
            // fall based on the aligned address and the passed in region size
            //
            StartingAddress = ROUND_DOWN((ULONG_PTR)PBaseAddress, _64K);
            EndingAddress = ((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
            PageCount = BYTES_TO_PAGES(EndingAddress - StartingAddress);
        }

        //
        // Allocate and initialize the VAD
        //
        Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'SdaV');
        ASSERT(Vad != NULL);
        Vad->u.LongFlags = 0;
        if (AllocationType & MEM_COMMIT) Vad->u.VadFlags.MemCommit = 1;
        Vad->u.VadFlags.Protection = ProtectionMask;
        Vad->u.VadFlags.PrivateMemory = 1;
        Vad->u.VadFlags.CommitCharge = AllocationType & MEM_COMMIT ? PageCount : 0;

        //
        // Lock the address space and make sure the process isn't already dead
        //
        AddressSpace = MmGetCurrentAddressSpace();
        MmLockAddressSpace(AddressSpace);
        if (Process->VmDeleted)
        {
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto FailPath;
        }

        //
        // Did we have a base address? If no, find a valid address that is 64KB
        // aligned in the VAD tree. Otherwise, make sure that the address range
        // which was passed in isn't already conflicting with an existing address
        // range.
        //
        if (!PBaseAddress)
        {
            Status = MiFindEmptyAddressRangeInTree(PRegionSize,
                                                   _64K,
                                                   &Process->VadRoot,
                                                   (PMMADDRESS_NODE*)&Process->VadFreeHint,
                                                   &StartingAddress);
            if (!NT_SUCCESS(Status)) goto FailPath;

            //
            // Now we know where the allocation ends. Make sure it doesn't end up
            // somewhere in kernel mode.
            //
            EndingAddress = ((ULONG_PTR)StartingAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
            if ((PVOID)EndingAddress > MM_HIGHEST_VAD_ADDRESS)
            {
                Status = STATUS_NO_MEMORY;
                goto FailPath;
            }
        }
        else if (MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                           EndingAddress >> PAGE_SHIFT,
                                           &Process->VadRoot))
        {
            //
            // The address specified is in conflict!
            //
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto FailPath;
        }

        //
        // Write out the VAD fields for this allocation
        //
        Vad->StartingVpn = (ULONG_PTR)StartingAddress >> PAGE_SHIFT;
        Vad->EndingVpn = (ULONG_PTR)EndingAddress >> PAGE_SHIFT;

        //
        // FIXME: Should setup VAD bitmap
        //
        Status = STATUS_SUCCESS;

        //
        // Lock the working set and insert the VAD into the process VAD tree
        //
        MiLockProcessWorkingSet(Process, CurrentThread);
        Vad->ControlArea = NULL; // For Memory-Area hack
        MiInsertVad(Vad, Process);
        MiUnlockProcessWorkingSet(Process, CurrentThread);

        //
        // Update the virtual size of the process, and if this is now the highest
        // virtual size we have ever seen, update the peak virtual size to reflect
        // this.
        //
        Process->VirtualSize += PRegionSize;
        if (Process->VirtualSize > Process->PeakVirtualSize)
        {
            Process->PeakVirtualSize = Process->VirtualSize;
        }

        //
        // Release address space and detach and dereference the target process if
        // it was different from the current process
        //
        MmUnlockAddressSpace(AddressSpace);
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
        }
        _SEH2_END;
        return STATUS_SUCCESS;
    }

    //
    // This is a MEM_COMMIT on top of an existing address which must have been
    // MEM_RESERVED already. Compute the start and ending base addresses based
    // on the user input, and then compute the actual region size once all the
    // alignments have been done.
    //
    StartingAddress = (ULONG_PTR)PAGE_ALIGN(PBaseAddress);
    EndingAddress = (((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1));
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
    FoundVad = (PMMVAD)MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                                 EndingAddress >> PAGE_SHIFT,
                                                 &Process->VadRoot);
    if (!FoundVad)
    {
        DPRINT1("Could not find a VAD for this allocation\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
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
    if (((StartingAddress >> PAGE_SHIFT) < FoundVad->StartingVpn) &&
        ((EndingAddress >> PAGE_SHIFT) > FoundVad->EndingVpn))
    {
        DPRINT1("Address range does not fit into the VAD\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    //
    // If this is an existing section view, we call the old RosMm routine which
    // has the relevant code required to handle the section scenario. In the future
    // we will limit this even more so that there's almost nothing that the code
    // needs to do, and it will become part of section.c in RosMm
    //
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)PAGE_ROUND_DOWN(PBaseAddress));
    if (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3)
    {
        return MiRosAllocateVirtualMemory(ProcessHandle,
                                          Process,
                                          MemoryArea,
                                          AddressSpace,
                                          UBaseAddress,
                                          Attached,
                                          URegionSize,
                                          AllocationType,
                                          Protect);
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
        // We should make sure that the section's permissions aren't being messed with
        //
        if (FoundVad->u.VadFlags.NoChange)
        {
            DPRINT1("SEC_NO_CHANGE section being touched. Assuming this is ok\n");
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
    //MiLockWorkingSet(CurrentThread, AddressSpace);

    //
    // Make the current page table valid, and then loop each page within it
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= LastPte)
    {
        //
        // Have we crossed into a new page table?
        //
        if (!(((ULONG_PTR)PointerPte) & (SYSTEM_PD_SIZE - 1)))
        {
            //
            // Get the PDE and now make it valid too
            //
            PointerPde = MiAddressToPte(PointerPte);
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
            UsedPageTableEntries = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(MiPteToAddress(PointerPte))];
            (*UsedPageTableEntries)++;
            ASSERT((*UsedPageTableEntries) <= PTE_COUNT);

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
                ASSERT(PointerPte->u.Soft.PageFileHigh == 0);
            }

            //
            // There's a change in protection, remember this for later, but do
            // not yet handle it.
            //
            DPRINT1("Protection change to: 0x%lx not implemented\n", Protect);
            ChangeProtection = TRUE;
        }

        //
        // Move to the next PTE
        //
        PointerPte++;
    }

    //
    // This path is not yet handled
    //
    ASSERT(ChangeProtection == FALSE);

    //
    // Release the working set lock, unlock the address space, and detach from
    // the target process if it was not the current process. Also dereference the
    // target process if this wasn't the case.
    //
    //MiUnlockProcessWorkingSet(Process, CurrentThread);
    Status = STATUS_SUCCESS;
FailPath:
    MmUnlockAddressSpace(AddressSpace);
FailPathNoLock:
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

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
    ULONG_PTR CommitReduction = 0;
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
    // Only two flags are supported
    //
    if (!(FreeType & (MEM_RELEASE | MEM_DECOMMIT)))
    {
        DPRINT1("Invalid FreeType\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Check if no flag was used, or if both flags were used
    //
    if (!((FreeType & (MEM_DECOMMIT | MEM_RELEASE))) ||
         ((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == (MEM_DECOMMIT | MEM_RELEASE)))
    {
        DPRINT1("Invalid FreeType combination\n");
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
    // These ASSERTs are here because ReactOS ARM3 does not currently implement
    // any other kinds of VADs.
    //
    ASSERT(Vad->u.VadFlags.PrivateMemory == 1);
    ASSERT(Vad->u.VadFlags.NoChange == 0);
    ASSERT(Vad->u.VadFlags.VadType == VadNone);

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
            MiLockWorkingSet(CurrentThread, AddressSpace);
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
                    MiLockWorkingSet(CurrentThread, AddressSpace);
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
                    MiLockWorkingSet(CurrentThread, AddressSpace);
                    CommitReduction = MiCalculatePageCommitment(StartingAddress,
                                                                EndingAddress,
                                                                Vad,
                                                                Process);
                    Vad->u.VadFlags.CommitCharge -= CommitReduction;
                    // For ReactOS: shrink the corresponding memory area
                    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)StartingAddress);
                    ASSERT(Vad->StartingVpn << PAGE_SHIFT == (ULONG_PTR)MemoryArea->StartingAddress);
                    ASSERT((Vad->EndingVpn + 1) << PAGE_SHIFT == (ULONG_PTR)MemoryArea->EndingAddress);
                    Vad->EndingVpn = ((ULONG_PTR)StartingAddress - 1) >> PAGE_SHIFT;
                    MemoryArea->EndingAddress = (PVOID)(((Vad->EndingVpn + 1) << PAGE_SHIFT) - 1);
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
        MiUnlockWorkingSet(CurrentThread, AddressSpace);
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
    CommitReduction = MiAddressToPte(EndingAddress) -
                      MiAddressToPte(StartingAddress) +
                      1 -
                      MiDecommitPages((PVOID)StartingAddress,
                                      MiAddressToPte(EndingAddress),
                                      Process,
                                      Vad);
    ASSERT(CommitReduction >= 0);
    Vad->u.VadFlags.CommitCharge -= CommitReduction;
    ASSERT(Vad->u.VadFlags.CommitCharge >= 0);

    //
    // We are done, go to the exit path without freeing the VAD as it remains
    // valid since we have not released the allocation.
    //
    Vad = NULL;
    Status = STATUS_SUCCESS;
    goto FinalPath;

    //
    // In the failure path, we detach and derefernece the target process, and
    // return whatever failure code was sent.
    //
FailPath:
    MmUnlockAddressSpace(AddressSpace);
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
    return Status;
}

/* EOF */
