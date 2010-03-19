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

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool(IN PVOID Address)
{
    PMMPDE PointerPde;
    NTSTATUS Status = STATUS_SUCCESS;
    
    //
    // Check if this is a fault while trying to access the page table itself
    //
    if ((Address >= (PVOID)MiAddressToPte(MmSystemRangeStart)) &&
        (Address < (PVOID)PTE_TOP))
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

NTSTATUS
NTAPI
MiResolveDemandZeroFault(IN PVOID Address,
                         IN PMMPTE PointerPte,
                         IN PEPROCESS Process,
                         IN KIRQL OldIrql)
{
    PFN_NUMBER PageFrameNumber;
    MMPTE TempPte;
    DPRINT("ARM3 Demand Zero Page Fault Handler for address: %p in process: %p\n",
            Address,
            Process);
    
    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    
    //
    // Get a page
    //
    PageFrameNumber = MmAllocPage(MC_PPOOL);
    DPRINT("New pool page: %lx\n", PageFrameNumber);
    
    //
    // Release PFN lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
    //
    // Increment demand zero faults
    //
    InterlockedIncrement(&KeGetCurrentPrcb()->MmDemandZeroCount);
    
    //
    // Build the PTE
    //
    TempPte = ValidKernelPte;
    TempPte.u.Hard.PageFrameNumber = PageFrameNumber;
    *PointerPte = TempPte;
    ASSERT(PointerPte->u.Hard.Valid == 1);
    
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
                                      -1);
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
    NTSTATUS Status;
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
        if ((Address >= (PVOID)PTE_BASE) && (Address <= MmHyperSpaceEnd))
        {
            //
            // This might happen...not sure yet
            //
            DPRINT1("FAULT ON PAGE TABLES!\n");
            
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
        
        //
        // Now we must raise to APC_LEVEL and mark the thread as owner
        // We don't actually implement a working set pushlock, so this is only
        // for internal consistency (and blocking APCs)
        //
        KeRaiseIrql(APC_LEVEL, &LockIrql);
        CurrentThread = PsGetCurrentThread();
        KeEnterGuardedRegion();
        ASSERT((CurrentThread->OwnsSystemWorkingSetExclusive == 0) &&
               (CurrentThread->OwnsSystemWorkingSetShared == 0));
        CurrentThread->OwnsSystemWorkingSetExclusive = 1; 
        
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
        
        //
        // Re-enable APCs
        //
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        CurrentThread->OwnsSystemWorkingSetExclusive = 0;
        KeLeaveGuardedRegion();
        KeLowerIrql(LockIrql);
        
        //
        // We are done!
        //
        DPRINT("Fault resolved with status: %lx\n", Status);
        return Status;
    }
    
    //
    // DIE DIE DIE
    //
    DPRINT1("WARNING: USER MODE FAULT IN ARM3???\n");
    return STATUS_ACCESS_VIOLATION;
}

/* EOF */
