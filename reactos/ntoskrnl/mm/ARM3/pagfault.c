/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/pagfault.c
 * PURPOSE:         ARM Memory Manager Page Fault Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

#if MI_TRACE_PFNS
BOOLEAN UserPdeFault = FALSE;
#endif

/* PRIVATE FUNCTIONS **********************************************************/

PMMPTE
NTAPI
MiCheckVirtualAddress(IN PVOID VirtualAddress,
                      OUT PULONG ProtectCode,
                      OUT PMMVAD *ProtoVad)
{
    PMMVAD Vad;
    PMMPTE PointerPte;

    /* No prototype/section support for now */
    *ProtoVad = NULL;

    /* Check if this is a page table address */
    if (MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress))
    {
        /* This should never happen, as these addresses are handled by the double-maping */
        if (((PMMPTE)VirtualAddress >= MiAddressToPte(MmPagedPoolStart)) &&
            ((PMMPTE)VirtualAddress <= MmPagedPoolInfo.LastPteForPagedPool))
        {
            /* Fail such access */
            *ProtectCode = MM_NOACCESS;
            return NULL;
        }

        /* Return full access rights */
        *ProtectCode = MM_READWRITE;
        return NULL;
    }

    /* Should not be a session address */
    ASSERT(MI_IS_SESSION_ADDRESS(VirtualAddress) == FALSE);

    /* Special case for shared data */
    if (PAGE_ALIGN(VirtualAddress) == (PVOID)MM_SHARED_USER_DATA_VA)
    {
        /* It's a read-only page */
        *ProtectCode = MM_READONLY;
        return MmSharedUserDataPte;
    }

    /* Find the VAD, it might not exist if the address is bogus */
    Vad = MiLocateAddress(VirtualAddress);
    if (!Vad)
    {
        /* Bogus virtual address */
        *ProtectCode = MM_NOACCESS;
        return NULL;
    }

    /* This must be a VM VAD */
    ASSERT(Vad->u.VadFlags.VadType == VadNone);

    /* Check if it's a section, or just an allocation */
    if (Vad->u.VadFlags.PrivateMemory == TRUE)
    {
        /* This must be a TEB/PEB VAD */
        ASSERT(Vad->u.VadFlags.MemCommit == TRUE);
        *ProtectCode = (ULONG)Vad->u.VadFlags.Protection;
        return NULL;
    }
    else
    {
        /* Return the proto VAD */
        ASSERT(Vad->u2.VadFlags2.ExtendableFile == 0);
        *ProtoVad = Vad;

        /* Get the prototype PTE for this page */
        PointerPte = (((ULONG_PTR)VirtualAddress >> PAGE_SHIFT) - Vad->StartingVpn) + Vad->FirstPrototypePte;
        ASSERT(PointerPte <= Vad->LastContiguousPte);
        ASSERT(PointerPte != NULL);

        /* Return the Prototype PTE and the protection for the page mapping */
        *ProtectCode = (ULONG)Vad->u.VadFlags.Protection;
        return PointerPte;
    }
}

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool(IN PVOID Address)
{
    PMMPDE PointerPde;
    NTSTATUS Status = STATUS_SUCCESS;

    /* No session support in ReactOS yet */
    ASSERT(MI_IS_SESSION_ADDRESS(Address) == FALSE);
    ASSERT(MI_IS_SESSION_PTE(Address) == FALSE);

    //
    // Check if this is a fault while trying to access the page table itself
    //
    if (MI_IS_SYSTEM_PAGE_TABLE_ADDRESS(Address))
    {
        //
        // Send a hint to the page fault handler that this is only a valid fault
        // if we already detected this was access within the page table range
        //
        PointerPde = (PMMPDE)MiAddressToPte(Address);
        Status = STATUS_WAIT_1;
    }
    else if (Address < MmSystemRangeStart)
    {
        //
        // This is totally illegal
        //
        return STATUS_ACCESS_VIOLATION;
    }
    else
    {
        //
        // Get the PDE for the address
        //
        PointerPde = MiAddressToPde(Address);
    }

    //
    // Check if it's not valid
    //
    if (PointerPde->u.Hard.Valid == 0)
    {
#ifdef _M_AMD64
        ASSERT(FALSE);
#else
        //
        // Copy it from our double-mapped system page directory
        //
        InterlockedExchangePte(PointerPde,
                               MmSystemPagePtes[((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE)].u.Long);
#endif
    }

    //
    // Return status
    //
    return Status;
}

VOID
NTAPI
MiZeroPfn(IN PFN_NUMBER PageFrameNumber)
{
    PMMPTE ZeroPte;
    MMPTE TempPte;
    PMMPFN Pfn1;
    PVOID ZeroAddress;

    /* Get the PFN for this page */
    Pfn1 = MiGetPfnEntry(PageFrameNumber);
    ASSERT(Pfn1);

    /* Grab a system PTE we can use to zero the page */
    ZeroPte = MiReserveSystemPtes(1, SystemPteSpace);
    ASSERT(ZeroPte);

    /* Initialize the PTE for it */
    TempPte = ValidKernelPte;
    TempPte.u.Hard.PageFrameNumber = PageFrameNumber;

    /* Setup caching */
    if (Pfn1->u3.e1.CacheAttribute == MiWriteCombined)
    {
        /* Write combining, no caching */
        MI_PAGE_DISABLE_CACHE(&TempPte);
        MI_PAGE_WRITE_COMBINED(&TempPte);
    }
    else if (Pfn1->u3.e1.CacheAttribute == MiNonCached)
    {
        /* Write through, no caching */
        MI_PAGE_DISABLE_CACHE(&TempPte);
        MI_PAGE_WRITE_THROUGH(&TempPte);
    }

    /* Make the system PTE valid with our PFN */
    MI_WRITE_VALID_PTE(ZeroPte, TempPte);

    /* Get the address it maps to, and zero it out */
    ZeroAddress = MiPteToAddress(ZeroPte);
    KeZeroPages(ZeroAddress, PAGE_SIZE);

    /* Now get rid of it */
    MiReleaseSystemPtes(ZeroPte, 1, SystemPteSpace);
}

NTSTATUS
NTAPI
MiResolveDemandZeroFault(IN PVOID Address,
                         IN PMMPTE PointerPte,
                         IN PEPROCESS Process,
                         IN KIRQL OldIrql)
{
    PFN_NUMBER PageFrameNumber = 0;
    MMPTE TempPte;
    BOOLEAN NeedZero = FALSE, HaveLock = FALSE;
    ULONG Color;
    DPRINT("ARM3 Demand Zero Page Fault Handler for address: %p in process: %p\n",
            Address,
            Process);

    /* Must currently only be called by paging path */
    if ((Process) && (OldIrql == MM_NOIRQL))
    {
        /* Sanity check */
        ASSERT(MI_IS_PAGE_TABLE_ADDRESS(PointerPte));

        /* No forking yet */
        ASSERT(Process->ForkInProgress == NULL);

        /* Get process color */
        Color = MI_GET_NEXT_PROCESS_COLOR(Process);
        ASSERT(Color != 0xFFFFFFFF);

        /* We'll need a zero page */
        NeedZero = TRUE;
    }
    else
    {
        /* Check if we need a zero page */
        NeedZero = (OldIrql != MM_NOIRQL);

        /* Get the next system page color */
        Color = MI_GET_NEXT_COLOR();
    }

    /* Check if the PFN database should be acquired */
    if (OldIrql == MM_NOIRQL)
    {
        /* Acquire it and remember we should release it after */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        HaveLock = TRUE;
    }

    /* We either manually locked the PFN DB, or already came with it locked */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Do we need a zero page? */
    ASSERT(PointerPte->u.Hard.Valid == 0);
#if MI_TRACE_PFNS
    if (UserPdeFault) MI_SET_USAGE(MI_USAGE_PAGE_TABLE);
    if (!UserPdeFault) MI_SET_USAGE(MI_USAGE_DEMAND_ZERO);
#endif
    if (Process) MI_SET_PROCESS2(Process->ImageFileName);
    if (!Process) MI_SET_PROCESS2("Kernel Demand 0");
    if ((NeedZero) && (Process))
    {
        /* Try to get one, if we couldn't grab a free page and zero it */
        PageFrameNumber = MiRemoveZeroPageSafe(Color);
        if (PageFrameNumber)
        {
            /* We got a genuine zero page, stop worrying about it */
            NeedZero = FALSE;
        }
        else
        {
            /* We'll need a free page and zero it manually */
            PageFrameNumber = MiRemoveAnyPage(Color);
        }
    }
    else if (!NeedZero)
    {
        /* Process or system doesn't want a zero page, grab anything */
        PageFrameNumber = MiRemoveAnyPage(Color);
    }
    else
    {
        /* System wants a zero page, obtain one */
        PageFrameNumber = MiRemoveZeroPage(Color);
    }

    /* Initialize it */
    MiInitializePfn(PageFrameNumber, PointerPte, TRUE);

    /* Release PFN lock if needed */
    if (HaveLock) KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    //
    // Increment demand zero faults
    //
    InterlockedIncrement(&KeGetCurrentPrcb()->MmDemandZeroCount);

    /* Zero the page if need be */
    if (NeedZero) MiZeroPfn(PageFrameNumber);

    /* Build the PTE */
    if (PointerPte <= MiHighestUserPte)
    {
        /* For user mode */
        MI_MAKE_HARDWARE_PTE_USER(&TempPte,
                                  PointerPte,
                                  PointerPte->u.Soft.Protection,
                                  PageFrameNumber);
    }
    else
    {
        /* For kernel mode */
        MI_MAKE_HARDWARE_PTE(&TempPte,
                             PointerPte,
                             PointerPte->u.Soft.Protection,
                             PageFrameNumber);
    }

    /* Set it dirty if it's a writable page */
    if (MI_IS_PAGE_WRITEABLE(&TempPte)) MI_MAKE_DIRTY_PAGE(&TempPte);

    /* Write it */
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // It's all good now
    //
    DPRINT("Paged pool page has now been paged in\n");
    return STATUS_PAGE_FAULT_DEMAND_ZERO;
}

NTSTATUS
NTAPI
MiCompleteProtoPteFault(IN BOOLEAN StoreInstruction,
                        IN PVOID Address,
                        IN PMMPTE PointerPte,
                        IN PMMPTE PointerProtoPte,
                        IN KIRQL OldIrql,
                        IN PMMPFN Pfn1)
{
    MMPTE TempPte;
    PMMPTE OriginalPte;
    ULONG_PTR Protection;
    PFN_NUMBER PageFrameIndex;

    /* Must be called with an valid prototype PTE, with the PFN lock held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(PointerProtoPte->u.Hard.Valid == 1);

    /* Get the page */
    PageFrameIndex = PFN_FROM_PTE(PointerProtoPte);

    /* Get the PFN entry and set it as a prototype PTE */
    Pfn1 = MiGetPfnEntry(PageFrameIndex);
    Pfn1->u3.e1.PrototypePte = 1;

    /* FIXME: Increment the share count for the page table */

    /* Check where we should be getting the protection information from */
    if (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)
    {
        /* Get the protection from the PTE, there's no real Proto PTE data */
        Protection = PointerPte->u.Soft.Protection;
    }
    else
    {
        /* Get the protection from the original PTE link */
        OriginalPte = &Pfn1->OriginalPte;
        Protection = OriginalPte->u.Soft.Protection;
    }

    /* Release the PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Remove caching bits */
    Protection &= ~(MM_NOCACHE | MM_NOACCESS);

    /* Check if this is a kernel or user address */
    if (Address < MmSystemRangeStart)
    {
        /* Build the user PTE */
        MI_MAKE_HARDWARE_PTE_USER(&TempPte, PointerPte, Protection, PageFrameIndex);
    }
    else
    {
        /* Build the kernel PTE */
        MI_MAKE_HARDWARE_PTE(&TempPte, PointerPte, Protection, PageFrameIndex);
    }

    /* Write the PTE */
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiResolveProtoPteFault(IN BOOLEAN StoreInstruction,
                       IN PVOID Address,
                       IN PMMPTE PointerPte,
                       IN PMMPTE PointerProtoPte,
                       IN OUT PMMPFN *OutPfn,
                       OUT PVOID *PageFileData,
                       OUT PMMPTE PteValue,
                       IN PEPROCESS Process,
                       IN KIRQL OldIrql,
                       IN PVOID TrapInformation)
{
    MMPTE TempPte;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    NTSTATUS Status;

    /* Must be called with an invalid, prototype PTE, with the PFN lock held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(PointerPte->u.Soft.Prototype == 1);

    /* Read the prototype PTE and check if it's valid */
    TempPte = *PointerProtoPte;
    if (TempPte.u.Hard.Valid == 1)
    {
        /* One more user of this mapped page */
        PageFrameIndex = PFN_FROM_PTE(&TempPte);
        Pfn1 = MiGetPfnEntry(PageFrameIndex);
        Pfn1->u2.ShareCount++;

        /* Call it a transition */
        InterlockedIncrement(&KeGetCurrentPrcb()->MmTransitionCount);

        /* Complete the prototype PTE fault -- this will release the PFN lock */
        return MiCompleteProtoPteFault(StoreInstruction,
                                       Address,
                                       PointerPte,
                                       PointerProtoPte,
                                       OldIrql,
                                       NULL);
    }

    /* Make sure there's some protection mask */
    if (TempPte.u.Long == 0)
    {
        /* Release the lock */
        DPRINT1("Access on reserved section?\n");
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        return STATUS_ACCESS_VIOLATION;
    }

    /* This is the only thing we support right now */
    ASSERT(TempPte.u.Soft.PageFileHigh == 0);
    ASSERT(TempPte.u.Proto.ReadOnly == 0);
    ASSERT(PointerPte > MiHighestUserPte);
    ASSERT(TempPte.u.Soft.Prototype == 0);
    ASSERT(TempPte.u.Soft.Transition == 0);

    /* Resolve the demand zero fault */
    Status = MiResolveDemandZeroFault(Address, PointerProtoPte, Process, OldIrql);
    ASSERT(NT_SUCCESS(Status));

    /* Complete the prototype PTE fault -- this will release the PFN lock */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    return MiCompleteProtoPteFault(StoreInstruction,
                                   Address,
                                   PointerPte,
                                   PointerProtoPte,
                                   OldIrql,
                                   NULL);
}

NTSTATUS
NTAPI
MiDispatchFault(IN BOOLEAN StoreInstruction,
                IN PVOID Address,
                IN PMMPTE PointerPte,
                IN PMMPTE PointerProtoPte,
                IN BOOLEAN Recursive,
                IN PEPROCESS Process,
                IN PVOID TrapInformation,
                IN PVOID Vad)
{
    MMPTE TempPte;
    KIRQL OldIrql, LockIrql;
    NTSTATUS Status;
    PMMPTE SuperProtoPte;
    DPRINT("ARM3 Page Fault Dispatcher for address: %p in process: %p\n",
             Address,
             Process);

    //
    // Make sure APCs are off and we're not at dispatch
    //
    OldIrql = KeGetCurrentIrql();
    ASSERT(OldIrql <= APC_LEVEL);
    ASSERT(KeAreAllApcsDisabled() == TRUE);

    //
    // Grab a copy of the PTE
    //
    TempPte = *PointerPte;

    /* Do we have a prototype PTE? */
    if (PointerProtoPte)
    {
        /* This should never happen */
        ASSERT(!MI_IS_PHYSICAL_ADDRESS(PointerProtoPte));

        /* Check if this is a kernel-mode address */
        SuperProtoPte = MiAddressToPte(PointerProtoPte);
        if (Address >= MmSystemRangeStart)
        {
            /* Lock the PFN database */
            LockIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

            /* Has the PTE been made valid yet? */
            if (!SuperProtoPte->u.Hard.Valid)
            {
                UNIMPLEMENTED;
                while (TRUE);
            }
            else
            {
                /* Resolve the fault -- this will release the PFN lock */
                ASSERT(PointerPte->u.Hard.Valid == 0);
                Status = MiResolveProtoPteFault(StoreInstruction,
                                                Address,
                                                PointerPte,
                                                PointerProtoPte,
                                                NULL,
                                                NULL,
                                                NULL,
                                                Process,
                                                LockIrql,
                                                TrapInformation);
                ASSERT(Status == STATUS_SUCCESS);

                /* Complete this as a transition fault */
                ASSERT(OldIrql == KeGetCurrentIrql());
                ASSERT(OldIrql <= APC_LEVEL);
                ASSERT(KeAreAllApcsDisabled() == TRUE);
                return Status;
            }
        }
        else
        {
            /* We currently only handle very limited paths */
            ASSERT(PointerPte->u.Soft.Prototype == 1);
            ASSERT(PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED);

            /* Lock the PFN database */
            LockIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

            /* For our current usage, this should be true */
            ASSERT(SuperProtoPte->u.Hard.Valid == 1);
            ASSERT(TempPte.u.Hard.Valid == 0);

            /* Resolve the fault -- this will release the PFN lock */
            Status = MiResolveProtoPteFault(StoreInstruction,
                                            Address,
                                            PointerPte,
                                            PointerProtoPte,
                                            NULL,
                                            NULL,
                                            NULL,
                                            Process,
                                            LockIrql,
                                            TrapInformation);
            ASSERT(Status == STATUS_SUCCESS);

            /* Complete this as a transition fault */
            ASSERT(OldIrql == KeGetCurrentIrql());
            ASSERT(OldIrql <= APC_LEVEL);
            ASSERT(KeAreAllApcsDisabled() == TRUE);
            return STATUS_PAGE_FAULT_TRANSITION;
        }
    }

    //
    // The PTE must be invalid
    //
    ASSERT(TempPte.u.Hard.Valid == 0);

    /* Check if the PTE is completely empty */
    if (TempPte.u.Long == 0)
    {
        KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                     (ULONG_PTR)Address,
                     StoreInstruction,
                     (ULONG_PTR)TrapInformation,
                     2);
    }

    //
    // No prototype, transition or page file software PTEs in ARM3 yet
    //
    ASSERT(TempPte.u.Soft.Prototype == 0);
    ASSERT(TempPte.u.Soft.Transition == 0);
    ASSERT(TempPte.u.Soft.PageFileHigh == 0);

    //
    // If we got this far, the PTE can only be a demand zero PTE, which is what
    // we want. Go handle it!
    //
    Status = MiResolveDemandZeroFault(Address,
                                      PointerPte,
                                      Process,
                                      MM_NOIRQL);
    ASSERT(KeAreAllApcsDisabled() == TRUE);
    if (NT_SUCCESS(Status))
    {
        //
        // Make sure we're returning in a sane state and pass the status down
        //
        ASSERT(OldIrql == KeGetCurrentIrql());
        ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        return Status;
    }

    //
    // Generate an access fault
    //
    return STATUS_ACCESS_VIOLATION;
}

NTSTATUS
NTAPI
MmArmAccessFault(IN BOOLEAN StoreInstruction,
                 IN PVOID Address,
                 IN KPROCESSOR_MODE Mode,
                 IN PVOID TrapInformation)
{
    KIRQL OldIrql = KeGetCurrentIrql(), LockIrql;
    PMMPTE ProtoPte = NULL;
    PMMPTE PointerPte = MiAddressToPte(Address);
    PMMPDE PointerPde = MiAddressToPde(Address);
#if (_MI_PAGING_LEVELS >= 3)
    PMMPDE PointerPpe = MiAddressToPpe(Address);
#if (_MI_PAGING_LEVELS == 4)
    PMMPDE PointerPxe = MiAddressToPxe(Address);
#endif
#endif
    MMPTE TempPte;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;
    NTSTATUS Status;
    PMMSUPPORT WorkingSet;
    ULONG ProtectionCode;
    PMMVAD Vad;
    PFN_NUMBER PageFrameIndex;
    ULONG Color;

    DPRINT("ARM3 FAULT AT: %p\n", Address);

    //
    // Check for dispatch-level snafu
    //
    if (OldIrql > APC_LEVEL)
    {
        //
        // There are some special cases where this is okay, but not in ARM3 yet
        //
        DbgPrint("MM:***PAGE FAULT AT IRQL > 1  Va %p, IRQL %lx\n",
                 Address,
                 OldIrql);
        ASSERT(OldIrql <= APC_LEVEL);
    }

    //
    // Check for kernel fault address
    //
    while (Address >= MmSystemRangeStart)
    {
        //
        // What are you even DOING here?
        //
        if (Mode == UserMode) return STATUS_ACCESS_VIOLATION;

#if (_MI_PAGING_LEVELS >= 3)
        if (
#if (_MI_PAGING_LEVELS == 4)
            (PointerPxe->u.Hard.Valid == 0) ||
#endif
            (PointerPpe->u.Hard.Valid == 0))
        {
            KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                         (ULONG_PTR)Address,
                         StoreInstruction,
                         (ULONG_PTR)TrapInformation,
                         2);
        }
#endif

        //
        // Is the PDE valid?
        //
        if (PointerPde->u.Hard.Valid == 0)
        {
            //
            // Debug spew (eww!)
            //
            DPRINT("Invalid PDE\n");
#if (_MI_PAGING_LEVELS == 2)
            //
            // Handle mapping in "Special" PDE directoreis
            //
            MiCheckPdeForPagedPool(Address);
#endif
            //
            // Now we SHOULD be good
            //
            if (PointerPde->u.Hard.Valid == 0)
            {
                //
                // FIXFIX: Do the S-LIST hack
                //

                //
                // Kill the system
                //
                KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                             (ULONG_PTR)Address,
                             StoreInstruction,
                             (ULONG_PTR)TrapInformation,
                             2);
            }
        }

        //
        // The PDE is valid, so read the PTE
        //
        TempPte = *PointerPte;
        if (TempPte.u.Hard.Valid == 1)
        {
            //
            // Only two things can go wrong here:
            // Executing NX page (we couldn't care less)
            // Writing to a read-only page (the stuff ARM3 works with is write,
            // so again, moot point).
            //

            //
            // Otherwise, the PDE was probably invalid, and all is good now
            //
            return STATUS_SUCCESS;
        }

        //
        // Check for a fault on the page table or hyperspace itself
        //
        if (MI_IS_PAGE_TABLE_OR_HYPER_ADDRESS(Address))
        {
#if (_MI_PAGING_LEVELS == 2)
            /* Could be paged pool access from a new process -- synchronize the page directories */
            if (MiCheckPdeForPagedPool(Address) == STATUS_WAIT_1)
            {
                DPRINT1("PAGE TABLES FAULTED IN!\n");
                return STATUS_SUCCESS;
            }
#endif
            /* Otherwise this could be a commit of a virtual address */
            break;
        }

        /* In this path, we are using the system working set */
        CurrentThread = PsGetCurrentThread();
        WorkingSet = &MmSystemCacheWs;

        /* Acquire it */
        KeRaiseIrql(APC_LEVEL, &LockIrql);
        MiLockWorkingSet(CurrentThread, WorkingSet);

        //
        // Re-read PTE now that the IRQL has been raised
        //
        TempPte = *PointerPte;
        if (TempPte.u.Hard.Valid == 1)
        {
            //
            // Only two things can go wrong here:
            // Executing NX page (we couldn't care less)
            // Writing to a read-only page (the stuff ARM3 works with is write,
            // so again, moot point.
            //

            /* Release the working set */
            MiUnlockWorkingSet(CurrentThread, WorkingSet);
            KeLowerIrql(LockIrql);

            //
            // Otherwise, the PDE was probably invalid, and all is good now
            //
            return STATUS_SUCCESS;
        }

        /* Check one kind of prototype PTE */
        if (TempPte.u.Soft.Prototype)
        {
            /* Make sure protected pool is on, and that this is a pool address */
            if ((MmProtectFreedNonPagedPool) &&
                (((Address >= MmNonPagedPoolStart) &&
                  (Address < (PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                     MmSizeOfNonPagedPoolInBytes))) ||
                 ((Address >= MmNonPagedPoolExpansionStart) &&
                  (Address < MmNonPagedPoolEnd))))
            {
                /* Bad boy, bad boy, whatcha gonna do, whatcha gonna do when ARM3 comes for you! */
                KeBugCheckEx(DRIVER_CAUGHT_MODIFYING_FREED_POOL,
                             (ULONG_PTR)Address,
                             StoreInstruction,
                             Mode,
                             4);
            }

            /* Get the prototype PTE! */
            ProtoPte = MiProtoPteToPte(&TempPte);
        }
        else
        {
            //
            // We don't implement transition PTEs
            //
            ASSERT(TempPte.u.Soft.Transition == 0);

            /* Check for no-access PTE */
            if (TempPte.u.Soft.Protection == MM_NOACCESS)
            {
                /* Bad boy, bad boy, whatcha gonna do, whatcha gonna do when ARM3 comes for you! */
                KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                             (ULONG_PTR)Address,
                             StoreInstruction,
                             (ULONG_PTR)TrapInformation,
                             1);
            }
        }

        /* Check for demand page */
        if ((StoreInstruction) && !(ProtoPte) && !(TempPte.u.Hard.Valid))
        {
            /* Get the protection code */
            if (!(TempPte.u.Soft.Protection & MM_READWRITE))
            {
                /* Bad boy, bad boy, whatcha gonna do, whatcha gonna do when ARM3 comes for you! */
                KeBugCheckEx(ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                             (ULONG_PTR)Address,
                             TempPte.u.Long,
                             (ULONG_PTR)TrapInformation,
                             14);
            }
        }

        //
        // Now do the real fault handling
        //
        Status = MiDispatchFault(StoreInstruction,
                                 Address,
                                 PointerPte,
                                 ProtoPte,
                                 FALSE,
                                 NULL,
                                 TrapInformation,
                                 NULL);

        /* Release the working set */
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        MiUnlockWorkingSet(CurrentThread, WorkingSet);
        KeLowerIrql(LockIrql);

        //
        // We are done!
        //
        DPRINT("Fault resolved with status: %lx\n", Status);
        return Status;
    }

#if (_MI_PAGING_LEVELS == 4)
    /* On these systems we have PXEs and PPEs ready for everything we need */
    if (PointerPxe->u.Hard.Valid == 0) return STATUS_ACCESS_VIOLATION;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    if (PointerPpe->u.Hard.Valid == 0) return STATUS_ACCESS_VIOLATION;
#endif

    /* This is a user fault (<- And this is a lie!) */
    CurrentThread = PsGetCurrentThread();
    CurrentProcess = PsGetCurrentProcess();

    /* Lock the working set */
    MiLockProcessWorkingSet(CurrentProcess, CurrentThread);

    /* First things first, is the PDE valid? */
    ASSERT(PointerPde->u.Hard.LargePage == 0);
    if (PointerPde->u.Hard.Valid == 0)
    {
        /* Right now, we only handle scenarios where the PDE is totally empty */
        ASSERT(PointerPde->u.Long == 0);

        /* Check if this address range belongs to a valid allocation (VAD) */
        MiCheckVirtualAddress(Address, &ProtectionCode, &Vad);

        /* Right now, we expect a valid protection mask on the VAD */
        ASSERT(ProtectionCode != MM_NOACCESS);

        /* Make the PDE demand-zero */
        MI_WRITE_INVALID_PDE(PointerPde, DemandZeroPde);

        /* And go dispatch the fault on the PDE. This should handle the demand-zero */
#if MI_TRACE_PFNS
        UserPdeFault = TRUE;
#endif
        Status = MiDispatchFault(TRUE,
                                 PointerPte,
                                 (PMMPTE)PointerPde,
                                 NULL,
                                 FALSE,
                                 PsGetCurrentProcess(),
                                 TrapInformation,
                                 NULL);
#if MI_TRACE_PFNS
        UserPdeFault = FALSE;
#endif
        /* We should come back with APCs enabled, and with a valid PDE */
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        ASSERT(PointerPde->u.Hard.Valid == 1);
    }

    /* Now capture the PTE. Ignore virtual faults for now */
    TempPte = *PointerPte;
    ASSERT(TempPte.u.Hard.Valid == 0);

    /* Quick check for demand-zero */
    if (TempPte.u.Long == (MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS))
    {
        /* Resolve the fault */
        MiResolveDemandZeroFault(Address,
                                 PointerPte,
                                 CurrentProcess,
                                 MM_NOIRQL);

        /* Return the status */
        MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
        return STATUS_PAGE_FAULT_DEMAND_ZERO;
    }

    /* Get protection and check if it's a prototype PTE */
    ProtectionCode = (ULONG)TempPte.u.Soft.Protection;
    ASSERT(TempPte.u.Soft.Prototype == 0);

    /* Check for non-demand zero PTE */
    if (TempPte.u.Long != 0)
    {
        /* This is a page fault, check for valid protection */
        ASSERT(ProtectionCode != 0x100);

        /* FIXME: Run MiAccessCheck */

        /* Dispatch the fault */
        Status = MiDispatchFault(StoreInstruction,
                                 Address,
                                 PointerPte,
                                 NULL,
                                 FALSE,
                                 PsGetCurrentProcess(),
                                 TrapInformation,
                                 NULL);

        /* Return the status */
        ASSERT(NT_SUCCESS(Status));
        ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
        return Status;
    }

    /* Check if this address range belongs to a valid allocation (VAD) */
    ASSERT(TempPte.u.Long == 0);
    ProtoPte = MiCheckVirtualAddress(Address, &ProtectionCode, &Vad);
    if (ProtectionCode == MM_NOACCESS)
    {
        /* This is a bogus VA */
        Status = STATUS_ACCESS_VIOLATION;

        /* Could be a not-yet-mapped paged pool page table */
#if (_MI_PAGING_LEVELS == 2)
        MiCheckPdeForPagedPool(Address);
#endif
        /* See if that fixed it */
        if (PointerPte->u.Hard.Valid == 1) Status = STATUS_SUCCESS;

        /* Return the status */
        MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
        return Status;
    }

    /* Is this a user address? */
    if (Address <= MM_HIGHEST_USER_ADDRESS)
    {
        /* Add an additional page table reference */
        MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)]++;
        ASSERT(MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)] <= PTE_COUNT);
    }

    /* Did we get a prototype PTE back? */
    if (!ProtoPte)
    {
        /* No, create a new PTE. First, write the protection */
        PointerPte->u.Soft.Protection = ProtectionCode;

        /* Lock the PFN database since we're going to grab a page */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* Try to get a zero page */
        MI_SET_USAGE(MI_USAGE_PEB_TEB);
        MI_SET_PROCESS2(CurrentProcess->ImageFileName);
        Color = MI_GET_NEXT_PROCESS_COLOR(CurrentProcess);
        PageFrameIndex = MiRemoveZeroPageSafe(Color);
        if (!PageFrameIndex)
        {
            /* Grab a page out of there. Later we should grab a colored zero page */
            PageFrameIndex = MiRemoveAnyPage(Color);
            ASSERT(PageFrameIndex);

            /* Release the lock since we need to do some zeroing */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

            /* Zero out the page, since it's for user-mode */
            MiZeroPfn(PageFrameIndex);

            /* Grab the lock again so we can initialize the PFN entry */
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        }

        /* Initialize the PFN entry now */
        MiInitializePfn(PageFrameIndex, PointerPte, 1);

        /* And we're done with the lock */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

        /* One more demand-zero fault */
        InterlockedIncrement(&KeGetCurrentPrcb()->MmDemandZeroCount);

        /* Was the fault on an actual user page, or a kernel page for the user? */
        if (PointerPte <= MiHighestUserPte)
        {
            /* User fault, build a user PTE */
            MI_MAKE_HARDWARE_PTE_USER(&TempPte,
                                      PointerPte,
                                      PointerPte->u.Soft.Protection,
                                      PageFrameIndex);
        }
        else
        {
            /* Session, kernel, or user PTE, figure it out and build it */
            MI_MAKE_HARDWARE_PTE(&TempPte,
                                 PointerPte,
                                 PointerPte->u.Soft.Protection,
                                 PageFrameIndex);
        }

        /* Write the dirty bit for writeable pages */
        if (MI_IS_PAGE_WRITEABLE(&TempPte)) MI_MAKE_DIRTY_PAGE(&TempPte);

        /* And now write down the PTE, making the address valid */
        MI_WRITE_VALID_PTE(PointerPte, TempPte);

        /* Demand zero */
        Status = STATUS_PAGE_FAULT_DEMAND_ZERO;
    }
    else
    {
        /* No guard page support yet */
        ASSERT((ProtectionCode & MM_DECOMMIT) == 0);
        ASSERT(ProtectionCode != 0x100);

        /* Write the prototype PTE */
        TempPte = PrototypePte;
        TempPte.u.Soft.Protection = ProtectionCode;
        MI_WRITE_INVALID_PTE(PointerPte, TempPte);

        /* Handle the fault */
        Status = MiDispatchFault(StoreInstruction,
                                 Address,
                                 PointerPte,
                                 ProtoPte,
                                 FALSE,
                                 CurrentProcess,
                                 TrapInformation,
                                 Vad);
        ASSERT(Status == STATUS_PAGE_FAULT_TRANSITION);
        ASSERT(PointerPte->u.Hard.Valid == 1);
        ASSERT(PointerPte->u.Hard.PageFrameNumber != 0);
    }

    /* Release the working set */
    MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
    return Status;
}

/* EOF */
