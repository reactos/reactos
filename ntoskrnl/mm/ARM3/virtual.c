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

/* PRIVATE FUNCTIONS **********************************************************/

ULONG
NTAPI
MiMakeSystemAddressValid(IN PVOID PageTableVirtualAddress,
                         IN PEPROCESS CurrentProcess)
{
    NTSTATUS Status;
    BOOLEAN LockChange = FALSE;

    /* Must be a non-pool page table, since those are double-mapped already */
    ASSERT(PageTableVirtualAddress > MM_HIGHEST_USER_ADDRESS);
    ASSERT((PageTableVirtualAddress < MmPagedPoolStart) ||
           (PageTableVirtualAddress > MmPagedPoolEnd));

    /* Working set lock or PFN lock should be held */
    ASSERT(KeAreAllApcsDisabled() == TRUE);

    /* Check if the page table is valid */
    while (!MmIsAddressValid(PageTableVirtualAddress))
    {
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
    PMMPFN Pfn1;
    //PMMPFN Pfn2;
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
                //Pfn2 = MiGetPfnEntry(PageTableIndex);

                /* Lock the PFN database */
                OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

                /* Delete it the page */
                MI_SET_PFN_DELETED(Pfn1);
                MiDecrementShareCount(Pfn1, PageFrameIndex);

                /* Decrement the page table too */
                DPRINT("FIXME: ARM3 should decrement the pool PDE refcount for: %p\n", PageTableIndex);
                #if 0 // ARM3: Dont't trust this yet
                MiDecrementShareCount(Pfn2, PageTableIndex);
                #endif

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
        /* Drop the reference on the page table. */
        MiDecrementShareCount(MiGetPfnEntry(PFN_FROM_PTE(PointerPde)), PFN_FROM_PTE(PointerPde));

        /* Drop the share count */
        MiDecrementShareCount(Pfn1, PageFrameIndex);

        /* Either a fork, or this is the shared user data page */
        if (PointerPte <= MiHighestUserPte)
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

        /* FIXME: Drop the reference on the page table. For now, leak it until RosMM is gone */
        //DPRINT1("Dropping a ref...\n");
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
        return MmProtectToValue[Pfn->OriginalPte.u.Soft.Protection];
    }

    /* This is hardware PTE */
    UNIMPLEMENTED;
    ASSERT(FALSE);

    return PAGE_NOACCESS;
}

ULONG
NTAPI
MiQueryAddressState(IN PVOID Va,
                    IN PMMVAD Vad,
                    IN PEPROCESS TargetProcess,
                    OUT PULONG ReturnedProtect,
                    OUT PVOID *NextVa)
{

    PMMPTE PointerPte;
    PMMPDE PointerPde;
    MMPTE TempPte;
    BOOLEAN DemandZeroPte = TRUE, ValidPte = FALSE;
    ULONG State = MEM_RESERVE, Protect = 0, LockChange;
    ASSERT((Vad->StartingVpn <= ((ULONG_PTR)Va >> PAGE_SHIFT)) &&
           (Vad->EndingVpn >= ((ULONG_PTR)Va >> PAGE_SHIFT)));

    /* Only normal VADs supported */
    ASSERT(Vad->u.VadFlags.VadType == VadNone);

    /* Get the PDE and PTE for the address */
    PointerPde = MiAddressToPde(Va);
    PointerPte = MiAddressToPte(Va);

    /* Return the next range */
    *NextVa = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);

    /* Loop to make sure the PDE is valid */
    do
    {
        /* Try again */
        LockChange = 0;

        /* Is the PDE empty? */
        if (!PointerPde->u.Long)
        {
            /* No address in this range used yet, move to the next PDE range */
            *NextVa = MiPdeToAddress(PointerPde + 1);
            break;
        }

        /* The PDE is not empty, but is it faulted in? */
        if (!PointerPde->u.Hard.Valid)
        {
            /* It isn't, go ahead and do the fault */
            LockChange = MiMakeSystemAddressValid(MiPdeToPte(PointerPde),
                                                  TargetProcess);
        }

        /* Check if the PDE was faulted in, making the PTE readable */
        if (!LockChange) ValidPte = TRUE;
    } while (LockChange);

    /* Is it safe to try reading the PTE? */
    if (ValidPte)
    {
        /* FIXME: watch out for large pages */

        /* Capture the PTE */
        TempPte = *PointerPte;
        if (TempPte.u.Long)
        {
            /* The PTE is valid, so it's not zeroed out */
            DemandZeroPte = FALSE;

            /* Check if it's valid or has a valid protection mask */
            ASSERT(TempPte.u.Soft.Prototype == 0);
            if ((TempPte.u.Soft.Protection != MM_DECOMMIT) ||
                (TempPte.u.Hard.Valid == 1))
            {
                /* This means it's committed */
                State = MEM_COMMIT;

                /* Get protection state of this page */
                Protect = MiGetPageProtection(PointerPte);
            }
            else
            {
                /* Otherwise our defaults should hold */
                ASSERT(Protect == 0);
                ASSERT(State == MEM_RESERVE);
            }
        }
    }

    /* Check if this was a demand-zero PTE, since we need to find the state */
    if (DemandZeroPte)
    {
        /* Check if the VAD is for committed memory */
        if (Vad->u.VadFlags.MemCommit)
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

    /* This must be a VM VAD */
    ASSERT(Vad->u.VadFlags.PrivateMemory);

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
MiQueryMemorySectionName(IN HANDLE ProcessHandle,
                         IN PVOID BaseAddress,
                         OUT PVOID MemoryInformation,
                         IN SIZE_T MemoryInformationLength,
                         OUT PSIZE_T ReturnLength)
{
    PEPROCESS Process;
    NTSTATUS Status;
    WCHAR ModuleFileNameBuffer[MAX_PATH] = {0};
    UNICODE_STRING ModuleFileName;
    PMEMORY_SECTION_NAME SectionName = NULL;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       NULL,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("MiQueryMemorySectionName: ObReferenceObjectByHandle returned %x\n",Status);
        return Status;
    }

    RtlInitEmptyUnicodeString(&ModuleFileName, ModuleFileNameBuffer, sizeof(ModuleFileNameBuffer));
    Status = MmGetFileNameForAddress(BaseAddress, &ModuleFileName);

    if (NT_SUCCESS(Status))
    {
        SectionName = MemoryInformation;
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
                SectionName->SectionFileName.MaximumLength = (USHORT)MemoryInformationLength;
                RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

                if (ReturnLength) *ReturnLength = ModuleFileName.Length;

            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
            SectionName->SectionFileName.MaximumLength = (USHORT)MemoryInformationLength;
            RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

            if (ReturnLength) *ReturnLength = ModuleFileName.Length;

        }
    }
    ObDereferenceObject(Process);
    return Status;
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

#ifdef __USE_ARM3__
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
    ULONG Type;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID BaseAddress;
    ULONG RegionSize;
    PMMVAD Vad;
    PMMADDRESS_NODE ParentNode;
    ULONG_PTR StartVpn, EndVpn;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    KAPC_STATE ApcState;
    ULONG ProtectionMask;
    BOOLEAN Attached = FALSE;
    BoundaryAddressMultiple.QuadPart = 0;
    TABLE_SEARCH_RESULT Result;
    
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
        BaseAddress = *UBaseAddress;
        RegionSize = *URegionSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
    
    /* Make sure there's a size specified */
    if (!RegionSize)
    {
        DPRINT1("Region size is invalid (zero)\n");
        return STATUS_INVALID_PARAMETER_4;
    }
    
    RegionSize = PAGE_ROUND_UP((ULONG_PTR)BaseAddress + RegionSize) -
        PAGE_ROUND_DOWN(BaseAddress);
    BaseAddress = (PVOID)PAGE_ROUND_DOWN(BaseAddress);
    StartVpn = (ULONG_PTR)BaseAddress >> PAGE_SHIFT;
    EndVpn = ((ULONG_PTR)BaseAddress + RegionSize - 1) >> PAGE_SHIFT;

    /* Make sure the allocation isn't past the VAD area */
    if (BaseAddress >= MM_HIGHEST_VAD_ADDRESS)
    {
        DPRINT1("Virtual allocation base above User Space\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Make sure the allocation wouldn't overflow past the VAD area */
    if ((((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) - (ULONG_PTR)BaseAddress) < RegionSize)
    {
        DPRINT1("Region size would overflow into kernel-memory\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Check if this is for the current process */
    if (ProcessHandle == NtCurrentProcess())
    {
        /* We already have the current process, no need to go through Ob */
        Process = CurrentProcess;
    }
    else
    {
        /* Reference the handle for correct permissions */
        Status = ObReferenceObjectByHandle(ProcessHandle,
            PROCESS_VM_OPERATION,
            PsProcessType,
            PreviousMode,
            (PVOID*)&Process,
            NULL);
        if (!NT_SUCCESS(Status)) return Status;

        /* Check if not running in the current process */
        if (CurrentProcess != Process)
        {
            /* Attach to it */
            KeStackAttachProcess(&Process->Pcb, &ApcState);
            Attached = TRUE;
        }
    }

    /* Check for large page allocations */
    if (AllocationType & MEM_LARGE_PAGES)
    {
        /* The lock memory privilege is required */
        if (!SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode))
        {
            /* Fail without it */
            DPRINT1("Privilege not held for MEM_LARGE_PAGES\n");
            if (Attached) KeUnstackDetachProcess(&ApcState);
            if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }


    /*
    * Copy on Write is reserved for system use. This case is a certain failure
    * but there may be other cases...needs more testing
    */
    if ((!BaseAddress || (AllocationType & MEM_RESERVE)) &&
            (Protect & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)))
    {
        DPRINT1("Copy on write is not supported by VirtualAlloc\n");
        if (Attached) KeUnstackDetachProcess(&ApcState);
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    Type = (AllocationType & MEM_COMMIT) ? MEM_COMMIT : MEM_RESERVE;
    DPRINT("Type %x\n", Type);

    /* Lock the process address space */
    KeAcquireGuardedMutex(&Process->AddressCreationLock);
    
    if(BaseAddress != 0)
    {
        /* 
         * An address was provided. Let's see if we've already
         * something there 
         */
        if(MiCheckForConflictingNode(StartVpn, EndVpn, &Process->VadRoot) != NULL)
        {
            /* Can't reserve twice the same range */
            if(AllocationType & MEM_RESERVE)
            {
                Status = STATUS_CONFLICTING_ADDRESSES;
                DPRINT1("Trying to reserve twice the same range.\n");
                goto cleanup;
            }
            /* Great there's already something there. What shall we do ? */
            if(AllocationType == MEM_RESET)
            {
                UNIMPLEMENTED;
                /* Reset the dirty bits for each PTEs */
                goto cleanup;
            }
            else
            {
                ASSERT(AllocationType & MEM_COMMIT);
                UNIMPLEMENTED;
                /* Mark the VAD as committed */
                goto cleanup;
            }
        }
        
        /* There's nothing */
        if(!(AllocationType & MEM_RESERVE))
        {
            Status = STATUS_ACCESS_DENIED;
            goto cleanup;
        }
        
        /* Now we can reserve our chunk of memory */
        goto buildVad;
    }
    
    /* No base address was given. */
    if(!(AllocationType & MEM_RESERVE))
    {
        DPRINT1("Providing NULL base address witout MEM_RESERVE.\n");
        ASSERT(FALSE);
        Status = STATUS_INVALID_PARAMETER_5;
        goto cleanup;
    }
    
    /* Find an empty range in Address Space */
    if(AllocationType & MEM_TOP_DOWN)
    {
        /* Top down allocation */
        Result = MiFindEmptyAddressRangeDownTree(RegionSize,
            (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS,
            (ZeroBits > PAGE_SHIFT) ? 1 << ZeroBits : PAGE_SIZE,
            &Process->VadRoot,
            (PULONG_PTR)&BaseAddress,
            &ParentNode);
        
        if(Result == TableFoundNode)
        {
            /* This means failure */
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }
    }
    else
    {
        /* Good old bottom up allocation */
        Status = MiFindEmptyAddressRangeInTree(RegionSize,
            (ZeroBits > PAGE_SHIFT) ? 1 << ZeroBits : PAGE_SIZE,
            &Process->VadRoot,
            &ParentNode,
            (PULONG_PTR)&BaseAddress);
        if(!NT_SUCCESS(Status))
        {
            /* Failed... */
            goto cleanup;
        }
    }
    StartVpn = (ULONG_PTR)BaseAddress >> PAGE_SHIFT;
    EndVpn = ((ULONG_PTR)BaseAddress + RegionSize - 1) >> PAGE_SHIFT;
    
    /* Build the Vad */
buildVad:
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD), TAG_MVAD);
    if(!Vad)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    RtlZeroMemory(Vad, sizeof(MMVAD));
    
    /* Set min/max */
    Vad->StartingVpn = StartVpn;
    Vad->EndingVpn = EndVpn;
    /* Set protection */
    Vad->u.VadFlags.Protection = ProtectionMask;
    /* Should it be already marked as committed ? */
    if(AllocationType & MEM_COMMIT)
        Vad->u.VadFlags.MemCommit = 1;
    if(AllocationType & MEM_PHYSICAL)
    {
        UNIMPLEMENTED;
        Vad->u.VadFlags.VadType = VadAwe;
    }
    /* Add it */
    MiLockProcessWorkingSet(Process, PsGetCurrentThread());
    MiInsertVad(Vad, Process);
    MiUnlockProcessWorkingSet(Process, PsGetCurrentThread());

    /* we're done */
cleanup:
    KeReleaseGuardedMutex(&Process->AddressCreationLock);
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

    *UBaseAddress = BaseAddress;
    *URegionSize = RegionSize;
    DPRINT("*UBaseAddress %x  *URegionSize %x\n", BaseAddress, RegionSize);

    return Status;
}
#endif
/* EOF */
