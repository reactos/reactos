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

#line 15 "ARM³::PAGFAULT"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

PMMPTE
NTAPI
MiCheckVirtualAddress(IN PVOID VirtualAddress,
                      OUT PULONG ProtectCode,
                      OUT PMMVAD *ProtoVad)
{
    PMMVAD Vad;
    
    /* No prototype/section support for now */
    *ProtoVad = NULL;
    
    /* Only valid for user VADs for now */
    ASSERT(VirtualAddress <= MM_HIGHEST_USER_ADDRESS);
    
    /* Special case for shared data */
    if (PAGE_ALIGN(VirtualAddress) == (PVOID)USER_SHARED_DATA)
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
    
    /* This must be a TEB/PEB VAD */
    ASSERT(Vad->u.VadFlags.PrivateMemory == TRUE);
    ASSERT(Vad->u.VadFlags.MemCommit == TRUE);
    ASSERT(Vad->u.VadFlags.VadType == VadNone);
    
    /* Return the protection on it */
    *ProtectCode = Vad->u.VadFlags.Protection;
    return NULL;
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
    BOOLEAN NeedZero = FALSE;
    ULONG Color;
    DPRINT("ARM3 Demand Zero Page Fault Handler for address: %p in process: %p\n",
            Address,
            Process);
    
    /* Must currently only be called by paging path */
    ASSERT(OldIrql == MM_NOIRQL);
    if (Process)
    {
        /* Sanity check */
        ASSERT(MI_IS_PAGE_TABLE_ADDRESS(PointerPte));

        /* No forking yet */
        ASSERT(Process->ForkInProgress == NULL);
        
        /* Get process color */
        Color = MI_GET_NEXT_PROCESS_COLOR(Process);
        
        /* We'll need a zero page */
        NeedZero = TRUE;
    }
    else
    {
        /* Get the next system page color */
        Color = MI_GET_NEXT_COLOR();
    }
        
    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    
    /* Do we need a zero page? */
    if (NeedZero)
    {
        /* Try to get one, if we couldn't grab a free page and zero it */
        PageFrameNumber = MiRemoveZeroPageSafe(Color);
        if (PageFrameNumber) NeedZero = FALSE;
    }
    
    /* Did we get a page? */
    if (!PageFrameNumber)
    {
        /* We either failed to find a zero page, or this is a system request */
        PageFrameNumber = MiRemoveAnyPage(Color);
        DPRINT("New pool page: %lx\n", PageFrameNumber);
    }
    
    /* Initialize it */
    MiInitializePfn(PageFrameNumber, PointerPte, TRUE);
    
    //
    // Release PFN lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
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
    if (TempPte.u.Hard.Write) TempPte.u.Hard.Dirty = TRUE;
    
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
    PFN_NUMBER PageFrameIndex;
    
    /* Must be called with an valid prototype PTE, with the PFN lock held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(PointerProtoPte->u.Hard.Valid == 1);
    
    /* Quick-n-dirty */
    ASSERT(PointerPte->u.Soft.PageFileHigh == 0xFFFFF);
    
    /* Get the page */
    PageFrameIndex = PFN_FROM_PTE(PointerProtoPte);

    /* Release the PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
    /* Build the user PTE */
    ASSERT(Address < MmSystemRangeStart);
    MI_MAKE_HARDWARE_PTE_USER(&TempPte, PointerPte, MM_READONLY, PageFrameIndex);
    
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
    
    /* Must be called with an invalid, prototype PTE, with the PFN lock held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(PointerPte->u.Soft.Prototype == 1);

    /* Read the prototype PTE -- it must be valid since we only handle shared data */
    TempPte = *PointerProtoPte;
    ASSERT(TempPte.u.Hard.Valid == 1);
    
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
            
        /* We currently only handle the shared user data PTE path */
        ASSERT(Address < MmSystemRangeStart);
        ASSERT(PointerPte->u.Soft.Prototype == 1);
        ASSERT(PointerPte->u.Soft.PageFileHigh == 0xFFFFF);
        ASSERT(Vad == NULL);
        
        /* Lock the PFN database */
        LockIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        
        /* For the shared data page, this should be true */
        SuperProtoPte = MiAddressToPte(PointerProtoPte);
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
    
    //
    // The PTE must be invalid, but not totally blank
    //
    ASSERT(TempPte.u.Hard.Valid == 0);
    ASSERT(TempPte.u.Long != 0);
    
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
    ASSERT(KeAreAllApcsDisabled () == TRUE);
    if (NT_SUCCESS(Status))
    {
        //
        // Make sure we're returning in a sane state and pass the status down
        //
        ASSERT(OldIrql == KeGetCurrentIrql ());
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
    PMMPTE PointerPte, ProtoPte;
    PMMPDE PointerPde;
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
    // Get the PTE and PDE
    //
    PointerPte = MiAddressToPte(Address);
    PointerPde = MiAddressToPde(Address);
#if (_MI_PAGING_LEVELS >= 3)
    /* We need the PPE and PXE addresses */
    ASSERT(FALSE);
#endif

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
    // Check for kernel fault
    //
    if (Address >= MmSystemRangeStart)
    {
        //
        // What are you even DOING here?
        //
        if (Mode == UserMode) return STATUS_ACCESS_VIOLATION;
        
#if (_MI_PAGING_LEVELS >= 3)
        /* Need to check PXE and PDE validity */
        ASSERT(FALSE);
#endif

        //
        // Is the PDE valid?
        //
        if (!PointerPde->u.Hard.Valid == 0)
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
            if (StoreInstruction)
            {
                DPRINT1("Should NEVER happen on ARM3!!!\n");
                return STATUS_ACCESS_VIOLATION;
            }
            
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
            //
            // This might happen...not sure yet
            //
            DPRINT1("FAULT ON PAGE TABLES: %p %lx %lx!\n", Address, *PointerPte, *PointerPde);
#if (_MI_PAGING_LEVELS == 2) 
            //
            // Map in the page table
            //
            if (MiCheckPdeForPagedPool(Address) == STATUS_WAIT_1)
            {
                DPRINT1("PAGE TABLES FAULTED IN!\n");
                return STATUS_SUCCESS;
            }
#endif
            //
            // Otherwise the page table doesn't actually exist
            //
            DPRINT1("FAILING\n");
            return STATUS_ACCESS_VIOLATION;
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
            if (StoreInstruction)
            {
                DPRINT1("Should NEVER happen on ARM3!!!\n");
                return STATUS_ACCESS_VIOLATION;
            }
            
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
            /* The one used for protected pool... */
            ASSERT(MmProtectFreedNonPagedPool == TRUE);
            
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
        }
        
        //
        // We don't implement transition PTEs
        //
        ASSERT(TempPte.u.Soft.Transition == 0);
        
        //
        // Now do the real fault handling
        //
        Status = MiDispatchFault(StoreInstruction,
                                 Address,
                                 PointerPte,
                                 NULL,
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
    
    /* This is a user fault */
    CurrentThread = PsGetCurrentThread();
    CurrentProcess = PsGetCurrentProcess();
    
    /* Lock the working set */
    MiLockProcessWorkingSet(CurrentProcess, CurrentThread);
    
#if (_MI_PAGING_LEVELS >= 3)
    /* Need to check/handle PPE and PXE validity too */
    ASSERT(FALSE);
#endif

    /* First things first, is the PDE valid? */
    ASSERT(PointerPde != MiAddressToPde(PTE_BASE));
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
        MI_WRITE_INVALID_PTE(PointerPde, DemandZeroPde);

        /* And go dispatch the fault on the PDE. This should handle the demand-zero */
        Status = MiDispatchFault(TRUE,
                                 PointerPte,
                                 PointerPde,
                                 NULL,
                                 FALSE,
                                 PsGetCurrentProcess(),
                                 TrapInformation,
                                 NULL);

        /* We should come back with APCs enabled, and with a valid PDE */
        ASSERT(KeAreAllApcsDisabled() == TRUE);
#if (_MI_PAGING_LEVELS >= 3)
        /* Need to check/handle PPE and PXE validity too */
        ASSERT(FALSE);
#endif
        ASSERT(PointerPde->u.Hard.Valid == 1);
    }

    /* Now capture the PTE. We only handle cases where it's totally empty */
    TempPte = *PointerPte;
    ASSERT(TempPte.u.Long == 0);

    /* Check if this address range belongs to a valid allocation (VAD) */
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
    
    /* Did we get a prototype PTE back? */
    if (!ProtoPte)
    {
        /* No, create a new PTE. First, write the protection */
        PointerPte->u.Soft.Protection = ProtectionCode;

        /* Lock the PFN database since we're going to grab a page */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        
        /* Try to get a zero page */
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
        if (TempPte.u.Hard.Write) TempPte.u.Hard.Dirty = TRUE;

        /* And now write down the PTE, making the address valid */
        MI_WRITE_VALID_PTE(PointerPte, TempPte);
        
        /* Demand zero */
        Status = STATUS_PAGE_FAULT_DEMAND_ZERO;
    }
    else
    {
        /* The only "prototype PTE" we support is the shared user data path */
        ASSERT(ProtectionCode == MM_READONLY);
        
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
        ASSERT(PointerPte->u.Hard.PageFrameNumber == MmSharedUserDataPte->u.Hard.PageFrameNumber);
    }
    
    /* Release the working set */
    MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
    return Status;
}

/* EOF */
