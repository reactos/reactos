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
    
    /* Find the VAD, it must exist, since we only handle PEB/TEB */
    Vad = MiLocateAddress(VirtualAddress);
    ASSERT(Vad);
    
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
#ifndef _M_AMD64
        /* This seems to be making the assumption that one PDE is one page long */
        C_ASSERT(PAGE_SIZE == (PD_COUNT * (sizeof(MMPTE) * PDE_COUNT)));
#endif
        
        //
        // Copy it from our double-mapped system page directory
        //
        InterlockedExchangePte(PointerPde,
                               MmSystemPagePtes[((ULONG_PTR)PointerPde &
                                                 (PAGE_SIZE - 1)) /
                                                sizeof(MMPTE)].u.Long);
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
    PFN_NUMBER PageFrameNumber;
    MMPTE TempPte;
    BOOLEAN NeedZero = FALSE;
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
        
        /* We'll need a zero page */
        NeedZero = TRUE;
    }
        
    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    
    /* Get a page */
    PageFrameNumber = MiRemoveAnyPage(0);
    DPRINT("New pool page: %lx\n", PageFrameNumber);
    
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
MiDispatchFault(IN BOOLEAN StoreInstruction,
                IN PVOID Address,
                IN PMMPTE PointerPte,
                IN PMMPTE PrototypePte,
                IN BOOLEAN Recursive,
                IN PEPROCESS Process,
                IN PVOID TrapInformation,
                IN PVOID Vad)
{
    MMPTE TempPte;
    KIRQL OldIrql;
    NTSTATUS Status;
    DPRINT("ARM3 Page Fault Dispatcher for address: %p in process: %p\n",
             Address,
             Process);
    
    //
    // Make sure APCs are off and we're not at dispatch
    //
    OldIrql = KeGetCurrentIrql ();
    ASSERT(OldIrql <= APC_LEVEL);
    ASSERT(KeAreAllApcsDisabled () == TRUE);
    
    //
    // Grab a copy of the PTE
    //
    TempPte = *PointerPte;
    
    /* No prototype */
    ASSERT(PrototypePte == NULL);
    
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
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    MMPTE TempPte;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;
    NTSTATUS Status;
    PMMSUPPORT WorkingSet;
    ULONG ProtectionCode;
    PMMVAD Vad;
    PFN_NUMBER PageFrameIndex;
    DPRINT("ARM3 FAULT AT: %p\n", Address);
    
    //
    // Get the PTE and PDE
    //
    PointerPte = MiAddressToPte(Address);
    PointerPde = MiAddressToPde(Address);
    
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
        
        //
        // Is the PDE valid?
        //
        if (!PointerPde->u.Hard.Valid == 0)
        {
            //
            // Debug spew (eww!)
            //
            DPRINT("Invalid PDE\n");
            
            //
            // Handle mapping in "Special" PDE directoreis
            //
            MiCheckPdeForPagedPool(Address);
            
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
            
            //
            // Map in the page table
            //
            if (MiCheckPdeForPagedPool(Address) == STATUS_WAIT_1)
            {
                DPRINT1("PAGE TABLES FAULTED IN!\n");
                return STATUS_SUCCESS;
            }
            
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
        
        //
        // We don't implement prototype PTEs
        //
        ASSERT(TempPte.u.Soft.Prototype == 0);
        
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
        ASSERT(PointerPde->u.Hard.Valid == 1);
    }

    /* Now capture the PTE. We only handle cases where it's totally empty */
    TempPte = *PointerPte;
    ASSERT(TempPte.u.Long == 0);

    /* Check if this address range belongs to a valid allocation (VAD) */
    MiCheckVirtualAddress(Address, &ProtectionCode, &Vad);
    
    /* Right now, we expect a valid protection mask on the VAD */
    ASSERT(ProtectionCode != MM_NOACCESS);
    PointerPte->u.Soft.Protection = ProtectionCode;

    /* Lock the PFN database since we're going to grab a page */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Grab a page out of there. Later we should grab a colored zero page */
    PageFrameIndex = MiRemoveAnyPage(0);
    ASSERT(PageFrameIndex);

    /* Release the lock since we need to do some zeroing */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Zero out the page, since it's for user-mode */
    MiZeroPfn(PageFrameIndex);

    /* Grab the lock again so we can initialize the PFN entry */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

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
    
    /* Release the working set */
    MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
    return STATUS_PAGE_FAULT_DEMAND_ZERO;
}

/* EOF */
