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
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

#define HYDRA_PROCESS (PEPROCESS)1
#if MI_TRACE_PFNS
BOOLEAN UserPdeFault = FALSE;
#endif

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
MiCheckForUserStackOverflow(IN PVOID Address,
                            IN PVOID TrapInformation)
{
    PETHREAD CurrentThread = PsGetCurrentThread();
    PTEB Teb = CurrentThread->Tcb.Teb;
    PVOID StackBase, DeallocationStack, NextStackAddress;
    SIZE_T GuranteedSize;
    NTSTATUS Status;

    /* Do we own the address space lock? */
    if (CurrentThread->AddressSpaceOwner == 1)
    {
        /* This isn't valid */
        DPRINT1("Process owns address space lock\n");
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        return STATUS_GUARD_PAGE_VIOLATION;
    }

    /* Are we attached? */
    if (KeIsAttachedProcess())
    {
        /* This isn't valid */
        DPRINT1("Process is attached\n");
        return STATUS_GUARD_PAGE_VIOLATION;
    }

    /* Read the current settings */
    StackBase = Teb->NtTib.StackBase;
    DeallocationStack = Teb->DeallocationStack;
    GuranteedSize = Teb->GuaranteedStackBytes;
    DPRINT("Handling guard page fault with Stacks Addresses 0x%p and 0x%p, guarantee: %lx\n",
            StackBase, DeallocationStack, GuranteedSize);

    /* Guarantees make this code harder, for now, assume there aren't any */
    ASSERT(GuranteedSize == 0);

    /* So allocate only the minimum guard page size */
    GuranteedSize = PAGE_SIZE;

    /* Does this faulting stack address actually exist in the stack? */
    if ((Address >= StackBase) || (Address < DeallocationStack))
    {
        /* That's odd... */
        DPRINT1("Faulting address outside of stack bounds. Address=%p, StackBase=%p, DeallocationStack=%p\n",
                Address, StackBase, DeallocationStack);
        return STATUS_GUARD_PAGE_VIOLATION;
    }

    /* This is where the stack will start now */
    NextStackAddress = (PVOID)((ULONG_PTR)PAGE_ALIGN(Address) - GuranteedSize);

    /* Do we have at least one page between here and the end of the stack? */
    if (((ULONG_PTR)NextStackAddress - PAGE_SIZE) <= (ULONG_PTR)DeallocationStack)
    {
        /* We don't -- Windows would try to make this guard page valid now */
        DPRINT1("Close to our death...\n");
        return STATUS_STACK_OVERFLOW;
    }

    /* Don't handle this flag yet */
    ASSERT((PsGetCurrentProcess()->Peb->NtGlobalFlag & FLG_DISABLE_STACK_EXTENSION) == 0);

    /* Update the stack limit */
    Teb->NtTib.StackLimit = (PVOID)((ULONG_PTR)NextStackAddress + GuranteedSize);

    /* Now move the guard page to the next page */
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &NextStackAddress,
                                     0,
                                     &GuranteedSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE | PAGE_GUARD);
    if ((NT_SUCCESS(Status) || (Status == STATUS_ALREADY_COMMITTED)))
    {
        /* We did it! */
        DPRINT("Guard page handled successfully for %p\n", Address);
        return STATUS_PAGE_FAULT_GUARD_PAGE;
    }

    /* Fail, we couldn't move the guard page */
    DPRINT1("Guard page failure: %lx\n", Status);
    ASSERT(FALSE);
    return STATUS_STACK_OVERFLOW;
}

FORCEINLINE
BOOLEAN
MiIsAccessAllowed(
    _In_ ULONG ProtectionMask,
    _In_ BOOLEAN Write,
    _In_ BOOLEAN Execute)
{
    #define _BYTE_MASK(Bit0, Bit1, Bit2, Bit3, Bit4, Bit5, Bit6, Bit7) \
        (Bit0) | ((Bit1) << 1) | ((Bit2) << 2) | ((Bit3) << 3) | \
        ((Bit4) << 4) | ((Bit5) << 5) | ((Bit6) << 6) | ((Bit7) << 7)
    static const UCHAR AccessAllowedMask[2][2] =
    {
        {   // Protect 0  1  2  3  4  5  6  7
            _BYTE_MASK(0, 1, 1, 1, 1, 1, 1, 1), // READ
            _BYTE_MASK(0, 0, 1, 1, 0, 0, 1, 1), // EXECUTE READ
        },
        {
            _BYTE_MASK(0, 0, 0, 0, 1, 1, 1, 1), // WRITE
            _BYTE_MASK(0, 0, 0, 0, 0, 0, 1, 1), // EXECUTE WRITE
        }
    };

    /* We want only the lower access bits */
    ProtectionMask &= MM_PROTECT_ACCESS;

    /* Look it up in the table */
    return (AccessAllowedMask[Write != 0][Execute != 0] >> ProtectionMask) & 1;
}

NTSTATUS
NTAPI
MiAccessCheck(IN PMMPTE PointerPte,
              IN BOOLEAN StoreInstruction,
              IN KPROCESSOR_MODE PreviousMode,
              IN ULONG_PTR ProtectionMask,
              IN PVOID TrapFrame,
              IN BOOLEAN LockHeld)
{
    MMPTE TempPte;

    /* Check for invalid user-mode access */
    if ((PreviousMode == UserMode) && (PointerPte > MiHighestUserPte))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    /* Capture the PTE -- is it valid? */
    TempPte = *PointerPte;
    if (TempPte.u.Hard.Valid)
    {
        /* Was someone trying to write to it? */
        if (StoreInstruction)
        {
            /* Is it writable?*/
            if (MI_IS_PAGE_WRITEABLE(&TempPte) ||
                MI_IS_PAGE_COPY_ON_WRITE(&TempPte))
            {
                /* Then there's nothing to worry about */
                return STATUS_SUCCESS;
            }

            /* Oops! This isn't allowed */
            return STATUS_ACCESS_VIOLATION;
        }

        /* Someone was trying to read from a valid PTE, that's fine too */
        return STATUS_SUCCESS;
    }

    /* Check if the protection on the page allows what is being attempted */
    if (!MiIsAccessAllowed(ProtectionMask, StoreInstruction, FALSE))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    /* Check if this is a guard page */
    if ((ProtectionMask & MM_PROTECT_SPECIAL) == MM_GUARDPAGE)
    {
        NT_ASSERT(ProtectionMask != MM_DECOMMIT);

        /* Attached processes can't expand their stack */
        if (KeIsAttachedProcess()) return STATUS_ACCESS_VIOLATION;

        /* No support for transition PTEs yet */
        ASSERT(((TempPte.u.Soft.Transition == 1) &&
                (TempPte.u.Soft.Prototype == 0)) == FALSE);

        /* Remove the guard page bit, and return a guard page violation */
        TempPte.u.Soft.Protection = ProtectionMask & ~MM_GUARDPAGE;
        NT_ASSERT(TempPte.u.Long != 0);
        MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        return STATUS_GUARD_PAGE_VIOLATION;
    }

    /* Nothing to do */
    return STATUS_SUCCESS;
}

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

    /* User or kernel fault? */
    if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS)
    {
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

        /* ReactOS does not handle physical memory VADs yet */
        ASSERT(Vad->u.VadFlags.VadType != VadDevicePhysicalMemory);

        /* Check if it's a section, or just an allocation */
        if (Vad->u.VadFlags.PrivateMemory)
        {
            /* ReactOS does not handle AWE VADs yet */
            ASSERT(Vad->u.VadFlags.VadType != VadAwe);

            /* This must be a TEB/PEB VAD */
            if (Vad->u.VadFlags.MemCommit)
            {
                /* It's committed, so return the VAD protection */
                *ProtectCode = (ULONG)Vad->u.VadFlags.Protection;
            }
            else
            {
                /* It has not yet been committed, so return no access */
                *ProtectCode = MM_NOACCESS;
            }

            /* In both cases, return no PTE */
            return NULL;
        }
        else
        {
            /* ReactOS does not supoprt these VADs yet */
            ASSERT(Vad->u.VadFlags.VadType != VadImageMap);
            ASSERT(Vad->u2.VadFlags2.ExtendableFile == 0);

            /* Return the proto VAD */
            *ProtoVad = Vad;

            /* Get the prototype PTE for this page */
            PointerPte = (((ULONG_PTR)VirtualAddress >> PAGE_SHIFT) - Vad->StartingVpn) + Vad->FirstPrototypePte;
            ASSERT(PointerPte != NULL);
            ASSERT(PointerPte <= Vad->LastContiguousPte);

            /* Return the Prototype PTE and the protection for the page mapping */
            *ProtectCode = (ULONG)Vad->u.VadFlags.Protection;
            return PointerPte;
        }
    }
    else if (MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress))
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
    else if (MI_IS_SESSION_ADDRESS(VirtualAddress))
    {
        /* ReactOS does not have an image list yet, so bail out to failure case */
        ASSERT(IsListEmpty(&MmSessionSpace->ImageList));
    }

    /* Default case -- failure */
    *ProtectCode = MM_NOACCESS;
    return NULL;
}

#if (_MI_PAGING_LEVELS == 2)
FORCEINLINE
BOOLEAN
MiSynchronizeSystemPde(PMMPDE PointerPde)
{
    MMPDE SystemPde;
    ULONG Index;

    /* Get the Index from the PDE */
    Index = ((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE);

    /* Copy the PDE from the double-mapped system page directory */
    SystemPde = MmSystemPagePtes[Index];
    *PointerPde = SystemPde;

    /* Make sure we re-read the PDE and PTE */
    KeMemoryBarrierWithoutFence();

    /* Return, if we had success */
    return (BOOLEAN)SystemPde.u.Hard.Valid;
}

NTSTATUS
FASTCALL
MiCheckPdeForSessionSpace(IN PVOID Address)
{
    MMPTE TempPde;
    PMMPDE PointerPde;
    PVOID SessionAddress;
    ULONG Index;

    /* Is this a session PTE? */
    if (MI_IS_SESSION_PTE(Address))
    {
        /* Make sure the PDE for session space is valid */
        PointerPde = MiAddressToPde(MmSessionSpace);
        if (!PointerPde->u.Hard.Valid)
        {
            /* This means there's no valid session, bail out */
            DbgPrint("MiCheckPdeForSessionSpace: No current session for PTE %p\n",
                     Address);
            DbgBreakPoint();
            return STATUS_ACCESS_VIOLATION;
        }

        /* Now get the session-specific page table for this address */
        SessionAddress = MiPteToAddress(Address);
        PointerPde = MiAddressToPte(Address);
        if (PointerPde->u.Hard.Valid) return STATUS_WAIT_1;

        /* It's not valid, so find it in the page table array */
        Index = ((ULONG_PTR)SessionAddress - (ULONG_PTR)MmSessionBase) >> 22;
        TempPde.u.Long = MmSessionSpace->PageTables[Index].u.Long;
        if (TempPde.u.Hard.Valid)
        {
            /* The copy is valid, so swap it in */
            InterlockedExchange((PLONG)PointerPde, TempPde.u.Long);
            return STATUS_WAIT_1;
        }

        /* We don't seem to have allocated a page table for this address yet? */
        DbgPrint("MiCheckPdeForSessionSpace: No Session PDE for PTE %p, %p\n",
                 PointerPde->u.Long, SessionAddress);
        DbgBreakPoint();
        return STATUS_ACCESS_VIOLATION;
    }

    /* Is the address also a session address? If not, we're done */
    if (!MI_IS_SESSION_ADDRESS(Address)) return STATUS_SUCCESS;

    /* It is, so again get the PDE for session space */
    PointerPde = MiAddressToPde(MmSessionSpace);
    if (!PointerPde->u.Hard.Valid)
    {
        /* This means there's no valid session, bail out */
        DbgPrint("MiCheckPdeForSessionSpace: No current session for VA %p\n",
                    Address);
        DbgBreakPoint();
        return STATUS_ACCESS_VIOLATION;
    }

    /* Now get the PDE for the address itself */
    PointerPde = MiAddressToPde(Address);
    if (!PointerPde->u.Hard.Valid)
    {
        /* Do the swap, we should be good to go */
        Index = ((ULONG_PTR)Address - (ULONG_PTR)MmSessionBase) >> 22;
        PointerPde->u.Long = MmSessionSpace->PageTables[Index].u.Long;
        if (PointerPde->u.Hard.Valid) return STATUS_WAIT_1;

        /* We had not allocated a page table for this session address yet, fail! */
        DbgPrint("MiCheckPdeForSessionSpace: No Session PDE for VA %p, %p\n",
                 PointerPde->u.Long, Address);
        DbgBreakPoint();
        return STATUS_ACCESS_VIOLATION;
    }

    /* It's valid, so there's nothing to do */
    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool(IN PVOID Address)
{
    PMMPDE PointerPde;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check session PDE */
    if (MI_IS_SESSION_ADDRESS(Address)) return MiCheckPdeForSessionSpace(Address);
    if (MI_IS_SESSION_PTE(Address)) return MiCheckPdeForSessionSpace(Address);

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
        //
        // Copy it from our double-mapped system page directory
        //
        InterlockedExchangePte(PointerPde,
                               MmSystemPagePtes[((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE)].u.Long);
    }

    //
    // Return status
    //
    return Status;
}
#else
NTSTATUS
FASTCALL
MiCheckPdeForPagedPool(IN PVOID Address)
{
    return STATUS_ACCESS_VIOLATION;
}
#endif

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
    PMMPFN Pfn1;
    DPRINT("ARM3 Demand Zero Page Fault Handler for address: %p in process: %p\n",
            Address,
            Process);

    /* Must currently only be called by paging path */
    if ((Process > HYDRA_PROCESS) && (OldIrql == MM_NOIRQL))
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

        /* Session-backed image views must be zeroed */
        if ((Process == HYDRA_PROCESS) &&
            ((MI_IS_SESSION_IMAGE_ADDRESS(Address)) ||
             ((Address >= MiSessionViewStart) && (Address < MiSessionSpaceWs))))
        {
            NeedZero = TRUE;
        }

        /* Hardcode unknown color */
        Color = 0xFFFFFFFF;
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
    ASSERT(PointerPte->u.Hard.Valid == 0);

    /* Assert we have enough pages */
    ASSERT(MmAvailablePages >= 32);

#if MI_TRACE_PFNS
    if (UserPdeFault) MI_SET_USAGE(MI_USAGE_PAGE_TABLE);
    if (!UserPdeFault) MI_SET_USAGE(MI_USAGE_DEMAND_ZERO);
#endif
    if (Process) MI_SET_PROCESS2(Process->ImageFileName);
    if (!Process) MI_SET_PROCESS2("Kernel Demand 0");

    /* Do we need a zero page? */
    if (Color != 0xFFFFFFFF)
    {
        /* Try to get one, if we couldn't grab a free page and zero it */
        PageFrameNumber = MiRemoveZeroPageSafe(Color);
        if (!PageFrameNumber)
        {
            /* We'll need a free page and zero it manually */
            PageFrameNumber = MiRemoveAnyPage(Color);
            NeedZero = TRUE;
        }
    }
    else
    {
        /* Get a color, and see if we should grab a zero or non-zero page */
        Color = MI_GET_NEXT_COLOR();
        if (!NeedZero)
        {
            /* Process or system doesn't want a zero page, grab anything */
            PageFrameNumber = MiRemoveAnyPage(Color);
        }
        else
        {
            /* System wants a zero page, obtain one */
            PageFrameNumber = MiRemoveZeroPage(Color);
        }
    }

    /* Initialize it */
    MiInitializePfn(PageFrameNumber, PointerPte, TRUE);

    /* Do we have the lock? */
    if (HaveLock)
    {
        /* Release it */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

        /* Update performance counters */
        if (Process > HYDRA_PROCESS) Process->NumberOfPrivatePages++;
    }

    /* Increment demand zero faults */
    InterlockedIncrement(&KeGetCurrentPrcb()->MmDemandZeroCount);

    /* Zero the page if need be */
    if (NeedZero) MiZeroPfn(PageFrameNumber);

    /* Fault on user PDE, or fault on user PTE? */
    if (PointerPte <= MiHighestUserPte)
    {
        /* User fault, build a user PTE */
        MI_MAKE_HARDWARE_PTE_USER(&TempPte,
                                  PointerPte,
                                  PointerPte->u.Soft.Protection,
                                  PageFrameNumber);
    }
    else
    {
        /* This is a user-mode PDE, create a kernel PTE for it */
        MI_MAKE_HARDWARE_PTE(&TempPte,
                             PointerPte,
                             PointerPte->u.Soft.Protection,
                             PageFrameNumber);
    }

    /* Set it dirty if it's a writable page */
    if (MI_IS_PAGE_WRITEABLE(&TempPte)) MI_MAKE_DIRTY_PAGE(&TempPte);

    /* Write it */
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    /* Did we manually acquire the lock */
    if (HaveLock)
    {
        /* Get the PFN entry */
        Pfn1 = MI_PFN_ELEMENT(PageFrameNumber);

        /* Windows does these sanity checks */
        ASSERT(Pfn1->u1.Event == 0);
        ASSERT(Pfn1->u3.e1.PrototypePte == 0);
    }

    //
    // It's all good now
    //
    DPRINT("Demand zero page has now been paged in\n");
    return STATUS_PAGE_FAULT_DEMAND_ZERO;
}

NTSTATUS
NTAPI
MiCompleteProtoPteFault(IN BOOLEAN StoreInstruction,
                        IN PVOID Address,
                        IN PMMPTE PointerPte,
                        IN PMMPTE PointerProtoPte,
                        IN KIRQL OldIrql,
                        IN PMMPFN* LockedProtoPfn)
{
    MMPTE TempPte;
    PMMPTE OriginalPte, PageTablePte;
    ULONG_PTR Protection;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1, Pfn2;
    BOOLEAN OriginalProtection, DirtyPage;

    /* Must be called with an valid prototype PTE, with the PFN lock held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(PointerProtoPte->u.Hard.Valid == 1);

    /* Get the page */
    PageFrameIndex = PFN_FROM_PTE(PointerProtoPte);

    /* Get the PFN entry and set it as a prototype PTE */
    Pfn1 = MiGetPfnEntry(PageFrameIndex);
    Pfn1->u3.e1.PrototypePte = 1;

    /* Increment the share count for the page table */
    PageTablePte = MiAddressToPte(PointerPte);
    Pfn2 = MiGetPfnEntry(PageTablePte->u.Hard.PageFrameNumber);
    Pfn2->u2.ShareCount++;

    /* Check where we should be getting the protection information from */
    if (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)
    {
        /* Get the protection from the PTE, there's no real Proto PTE data */
        Protection = PointerPte->u.Soft.Protection;

        /* Remember that we did not use the proto protection */
        OriginalProtection = FALSE;
    }
    else
    {
        /* Get the protection from the original PTE link */
        OriginalPte = &Pfn1->OriginalPte;
        Protection = OriginalPte->u.Soft.Protection;

        /* Remember that we used the original protection */
        OriginalProtection = TRUE;

        /* Check if this was a write on a read only proto */
        if ((StoreInstruction) && !(Protection & MM_READWRITE))
        {
            /* Clear the flag */
            StoreInstruction = 0;
        }
    }

    /* Check if this was a write on a non-COW page */
    DirtyPage = FALSE;
    if ((StoreInstruction) && ((Protection & MM_WRITECOPY) != MM_WRITECOPY))
    {
        /* Then the page should be marked dirty */
        DirtyPage = TRUE;

        /* ReactOS check */
        ASSERT(Pfn1->OriginalPte.u.Soft.Prototype != 0);
    }

    /* Did we get a locked incoming PFN? */
    if (*LockedProtoPfn)
    {
        /* Drop a reference */
        ASSERT((*LockedProtoPfn)->u3.e2.ReferenceCount >= 1);
        MiDereferencePfnAndDropLockCount(*LockedProtoPfn);
        *LockedProtoPfn = NULL;
    }

    /* Release the PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Remove special/caching bits */
    Protection &= ~MM_PROTECT_SPECIAL;

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

    /* Set the dirty flag if needed */
    if (DirtyPage) MI_MAKE_DIRTY_PAGE(&TempPte);

    /* Write the PTE */
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    /* Reset the protection if needed */
    if (OriginalProtection) Protection = MM_ZERO_ACCESS;

    /* Return success */
    ASSERT(PointerPte == MiAddressToPte(Address));
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiResolvePageFileFault(_In_ BOOLEAN StoreInstruction,
                       _In_ PVOID FaultingAddress,
                       _In_ PMMPTE PointerPte,
                       _In_ PEPROCESS CurrentProcess,
                       _Inout_ KIRQL *OldIrql)
{
    ULONG Color;
    PFN_NUMBER Page;
    NTSTATUS Status;
    MMPTE TempPte = *PointerPte;
    KEVENT Event;
    PMMPFN Pfn1;
    ULONG PageFileIndex = TempPte.u.Soft.PageFileLow;
    ULONG_PTR PageFileOffset = TempPte.u.Soft.PageFileHigh;

    /* Things we don't support yet */
    ASSERT(CurrentProcess > HYDRA_PROCESS);
    ASSERT(*OldIrql != MM_NOIRQL);

    /* We must hold the PFN lock */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Some sanity checks */
    ASSERT(TempPte.u.Hard.Valid == 0);
    ASSERT(TempPte.u.Soft.PageFileHigh != 0);
    ASSERT(TempPte.u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED);

    /* Get any page, it will be overwritten */
    Color = MI_GET_NEXT_PROCESS_COLOR(CurrentProcess);
    Page = MiRemoveAnyPage(Color);

    /* Initialize this PFN */
    MiInitializePfn(Page, PointerPte, StoreInstruction);

    /* Sets the PFN as being in IO operation */
    Pfn1 = MI_PFN_ELEMENT(Page);
    ASSERT(Pfn1->u1.Event == NULL);
    ASSERT(Pfn1->u3.e1.ReadInProgress == 0);
    ASSERT(Pfn1->u3.e1.WriteInProgress == 0);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Pfn1->u1.Event = &Event;
    Pfn1->u3.e1.ReadInProgress = 1;

    /* We must write the PTE now as the PFN lock will be released while performing the IO operation */
    TempPte.u.Soft.Transition = 1;
    TempPte.u.Soft.PageFileLow = 0;
    TempPte.u.Soft.Prototype = 0;
    TempPte.u.Trans.PageFrameNumber = Page;

    MI_WRITE_INVALID_PTE(PointerPte, TempPte);

    /* Release the PFN lock while we proceed */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, *OldIrql);

    /* Do the paging IO */
    Status = MiReadPageFile(Page, PageFileIndex, PageFileOffset);

    /* Lock the PFN database again */
    *OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Nobody should have changed that while we were not looking */
    ASSERT(Pfn1->u1.Event == &Event);
    ASSERT(Pfn1->u3.e1.ReadInProgress == 1);
    ASSERT(Pfn1->u3.e1.WriteInProgress == 0);

    if (!NT_SUCCESS(Status))
    {
        /* Malheur! */
        ASSERT(FALSE);
        Pfn1->u4.InPageError = 1;
        Pfn1->u1.ReadStatus = Status;
    }

    /* This is now a nice and normal PFN */
    Pfn1->u1.Event = NULL;
    Pfn1->u3.e1.ReadInProgress = 0;

    /* And the PTE can finally be valid */
    MI_MAKE_HARDWARE_PTE(&TempPte, PointerPte, TempPte.u.Trans.Protection, Page);
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    /* Waiters gonna wait */
    KeSetEvent(&Event, IO_NO_INCREMENT, FALSE);

    return Status;
}

NTSTATUS
NTAPI
MiResolveTransitionFault(IN PVOID FaultingAddress,
                         IN PMMPTE PointerPte,
                         IN PEPROCESS CurrentProcess,
                         IN KIRQL OldIrql,
                         OUT PVOID *InPageBlock)
{
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    MMPTE TempPte;
    PMMPTE PointerToPteForProtoPage;
    DPRINT("Transition fault on 0x%p with PTE 0x%p in process %s\n",
            FaultingAddress, PointerPte, CurrentProcess->ImageFileName);

    /* Windowss does this check */
    ASSERT(*InPageBlock == NULL);

    /* ARM3 doesn't support this path */
    ASSERT(OldIrql != MM_NOIRQL);

    /* Capture the PTE and make sure it's in transition format */
    TempPte = *PointerPte;
    ASSERT((TempPte.u.Soft.Valid == 0) &&
           (TempPte.u.Soft.Prototype == 0) &&
           (TempPte.u.Soft.Transition == 1));

    /* Get the PFN and the PFN entry */
    PageFrameIndex = TempPte.u.Trans.PageFrameNumber;
    DPRINT("Transition PFN: %lx\n", PageFrameIndex);
    Pfn1 = MiGetPfnEntry(PageFrameIndex);

    /* One more transition fault! */
    InterlockedIncrement(&KeGetCurrentPrcb()->MmTransitionCount);

    /* This is from ARM3 -- Windows normally handles this here */
    ASSERT(Pfn1->u4.InPageError == 0);

    /* See if we should wait before terminating the fault */
    if (Pfn1->u3.e1.ReadInProgress == 1)
    {
        DPRINT1("The page is currently being read!\n");
        ASSERT(Pfn1->u1.Event != NULL);
        *InPageBlock = Pfn1->u1.Event;
        if (PointerPte == Pfn1->PteAddress)
        {
            DPRINT1("And this if for this particular PTE.\n");
            /* The PTE will be made valid by the thread serving the fault */
            return STATUS_SUCCESS; // FIXME: Maybe something more descriptive
        }
    }

    /* Windows checks there's some free pages and this isn't an in-page error */
    ASSERT(MmAvailablePages > 0);
    ASSERT(Pfn1->u4.InPageError == 0);

    /* ReactOS checks for this */
    ASSERT(MmAvailablePages > 32);

    /* Was this a transition page in the valid list, or free/zero list? */
    if (Pfn1->u3.e1.PageLocation == ActiveAndValid)
    {
        /* All Windows does here is a bunch of sanity checks */
        DPRINT("Transition in active list\n");
        ASSERT((Pfn1->PteAddress >= MiAddressToPte(MmPagedPoolStart)) &&
               (Pfn1->PteAddress <= MiAddressToPte(MmPagedPoolEnd)));
        ASSERT(Pfn1->u2.ShareCount != 0);
        ASSERT(Pfn1->u3.e2.ReferenceCount != 0);
    }
    else
    {
        /* Otherwise, the page is removed from its list */
        DPRINT("Transition page in free/zero list\n");
        MiUnlinkPageFromList(Pfn1);
        MiReferenceUnusedPageAndBumpLockCount(Pfn1);
    }

    /* At this point, there should no longer be any in-page errors */
    ASSERT(Pfn1->u4.InPageError == 0);

    /* Check if this was a PFN with no more share references */
    if (Pfn1->u2.ShareCount == 0) MiDropLockCount(Pfn1);

    /* Bump the share count and make the page valid */
    Pfn1->u2.ShareCount++;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;

    /* Prototype PTEs are in paged pool, which itself might be in transition */
    if (FaultingAddress >= MmSystemRangeStart)
    {
        /* Check if this is a paged pool PTE in transition state */
        PointerToPteForProtoPage = MiAddressToPte(PointerPte);
        TempPte = *PointerToPteForProtoPage;
        if ((TempPte.u.Hard.Valid == 0) && (TempPte.u.Soft.Transition == 1))
        {
            /* This isn't yet supported */
            DPRINT1("Double transition fault not yet supported\n");
            ASSERT(FALSE);
        }
    }

    /* Build the transition PTE -- maybe a macro? */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(PointerPte->u.Trans.Prototype == 0);
    ASSERT(PointerPte->u.Trans.Transition == 1);
    TempPte.u.Long = (PointerPte->u.Long & ~0xFFF) |
                     (MmProtectToPteMask[PointerPte->u.Trans.Protection]) |
                     MiDetermineUserGlobalPteMask(PointerPte);

    /* Is the PTE writeable? */
    if ((Pfn1->u3.e1.Modified) &&
        MI_IS_PAGE_WRITEABLE(&TempPte) &&
        !MI_IS_PAGE_COPY_ON_WRITE(&TempPte))
    {
        /* Make it dirty */
        MI_MAKE_DIRTY_PAGE(&TempPte);
    }
    else
    {
        /* Make it clean */
        MI_MAKE_CLEAN_PAGE(&TempPte);
    }

    /* Write the valid PTE */
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    /* Return success */
    return STATUS_PAGE_FAULT_TRANSITION;
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
    MMPTE TempPte, PteContents;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    NTSTATUS Status;
    PVOID InPageBlock = NULL;

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
                                       OutPfn);
    }

    /* Make sure there's some protection mask */
    if (TempPte.u.Long == 0)
    {
        /* Release the lock */
        DPRINT1("Access on reserved section?\n");
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        return STATUS_ACCESS_VIOLATION;
    }

    /* There is no such thing as a decommitted prototype PTE */
    NT_ASSERT(TempPte.u.Long != MmDecommittedPte.u.Long);

    /* Check for access rights on the PTE proper */
    PteContents = *PointerPte;
    if (PteContents.u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED)
    {
        if (!PteContents.u.Proto.ReadOnly)
        {
            /* Check for page acess in software */
            Status = MiAccessCheck(PointerProtoPte,
                                   StoreInstruction,
                                   KernelMode,
                                   TempPte.u.Soft.Protection,
                                   TrapInformation,
                                   TRUE);
            ASSERT(Status == STATUS_SUCCESS);

            /* Check for copy on write page */
            if ((TempPte.u.Soft.Protection & MM_WRITECOPY) == MM_WRITECOPY)
            {
                /* Not yet supported */
                ASSERT(FALSE);
            }
        }
    }
    else
    {
        /* Check for copy on write page */
        if ((PteContents.u.Soft.Protection & MM_WRITECOPY) == MM_WRITECOPY)
        {
            /* Not yet supported */
            ASSERT(FALSE);
        }
    }

    /* Check for clone PTEs */
    if (PointerPte <= MiHighestUserPte) ASSERT(Process->CloneRoot == NULL);

    /* We don't support mapped files yet */
    ASSERT(TempPte.u.Soft.Prototype == 0);

    /* We might however have transition PTEs */
    if (TempPte.u.Soft.Transition == 1)
    {
        /* Resolve the transition fault */
        ASSERT(OldIrql != MM_NOIRQL);
        Status = MiResolveTransitionFault(Address,
                                          PointerProtoPte,
                                          Process,
                                          OldIrql,
                                          &InPageBlock);
        ASSERT(NT_SUCCESS(Status));
    }
    else
    {
        /* We also don't support paged out pages */
        ASSERT(TempPte.u.Soft.PageFileHigh == 0);

        /* Resolve the demand zero fault */
        Status = MiResolveDemandZeroFault(Address,
                                          PointerProtoPte,
                                          Process,
                                          OldIrql);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Complete the prototype PTE fault -- this will release the PFN lock */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    return MiCompleteProtoPteFault(StoreInstruction,
                                   Address,
                                   PointerPte,
                                   PointerProtoPte,
                                   OldIrql,
                                   OutPfn);
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
                IN PMMVAD Vad)
{
    MMPTE TempPte;
    KIRQL OldIrql, LockIrql;
    NTSTATUS Status;
    PMMPTE SuperProtoPte;
    PMMPFN Pfn1, OutPfn = NULL;
    PFN_NUMBER PageFrameIndex;
    PFN_COUNT PteCount, ProcessedPtes;
    DPRINT("ARM3 Page Fault Dispatcher for address: %p in process: %p\n",
             Address,
             Process);

    /* Make sure the addresses are ok */
    ASSERT(PointerPte == MiAddressToPte(Address));

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
                ASSERT(FALSE);
            }
            else if (PointerPte->u.Hard.Valid == 1)
            {
                ASSERT(FALSE);
            }

            /* Resolve the fault -- this will release the PFN lock */
            Status = MiResolveProtoPteFault(StoreInstruction,
                                            Address,
                                            PointerPte,
                                            PointerProtoPte,
                                            &OutPfn,
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
        else
        {
            /* We only handle the lookup path */
            ASSERT(PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED);

            /* Is there a non-image VAD? */
            if ((Vad) &&
                (Vad->u.VadFlags.VadType != VadImageMap) &&
                !(Vad->u2.VadFlags2.ExtendableFile))
            {
                /* One day, ReactOS will cluster faults */
                ASSERT(Address <= MM_HIGHEST_USER_ADDRESS);
                DPRINT("Should cluster fault, but won't\n");
            }

            /* Only one PTE to handle for now */
            PteCount = 1;
            ProcessedPtes = 0;

            /* Lock the PFN database */
            LockIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

            /* We only handle the valid path */
            ASSERT(SuperProtoPte->u.Hard.Valid == 1);

            /* Capture the PTE */
            TempPte = *PointerProtoPte;

            /* Loop to handle future case of clustered faults */
            while (TRUE)
            {
                /* For our current usage, this should be true */
                if (TempPte.u.Hard.Valid == 1)
                {
                    /* Bump the share count on the PTE */
                    PageFrameIndex = PFN_FROM_PTE(&TempPte);
                    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
                    Pfn1->u2.ShareCount++;
                }
                else if ((TempPte.u.Soft.Prototype == 0) &&
                         (TempPte.u.Soft.Transition == 1))
                {
                    /* This is a standby page, bring it back from the cache */
                    PageFrameIndex = TempPte.u.Trans.PageFrameNumber;
                    DPRINT("oooh, shiny, a soft fault! 0x%lx\n", PageFrameIndex);
                    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
                    ASSERT(Pfn1->u3.e1.PageLocation != ActiveAndValid);

                    /* Should not yet happen in ReactOS */
                    ASSERT(Pfn1->u3.e1.ReadInProgress == 0);
                    ASSERT(Pfn1->u4.InPageError == 0);

                    /* Get the page */
                    MiUnlinkPageFromList(Pfn1);

                    /* Bump its reference count */
                    ASSERT(Pfn1->u2.ShareCount == 0);
                    InterlockedIncrement16((PSHORT)&Pfn1->u3.e2.ReferenceCount);
                    Pfn1->u2.ShareCount++;

                    /* Make it valid again */
                    /* This looks like another macro.... */
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;
                    ASSERT(PointerProtoPte->u.Hard.Valid == 0);
                    ASSERT(PointerProtoPte->u.Trans.Prototype == 0);
                    ASSERT(PointerProtoPte->u.Trans.Transition == 1);
                    TempPte.u.Long = (PointerProtoPte->u.Long & ~0xFFF) |
                                     MmProtectToPteMask[PointerProtoPte->u.Trans.Protection];
                    TempPte.u.Hard.Valid = 1;
                    MI_MAKE_ACCESSED_PAGE(&TempPte);

                    /* Is the PTE writeable? */
                    if ((Pfn1->u3.e1.Modified) &&
                        MI_IS_PAGE_WRITEABLE(&TempPte) &&
                        !MI_IS_PAGE_COPY_ON_WRITE(&TempPte))
                    {
                        /* Make it dirty */
                        MI_MAKE_DIRTY_PAGE(&TempPte);
                    }
                    else
                    {
                        /* Make it clean */
                        MI_MAKE_CLEAN_PAGE(&TempPte);
                    }

                    /* Write the valid PTE */
                    MI_WRITE_VALID_PTE(PointerProtoPte, TempPte);
                    ASSERT(PointerPte->u.Hard.Valid == 0);
                }
                else
                {
                    /* Page is invalid, get out of the loop */
                    break;
                }

                /* One more done, was it the last? */
                if (++ProcessedPtes == PteCount)
                {
                    /* Complete the fault */
                    MiCompleteProtoPteFault(StoreInstruction,
                                            Address,
                                            PointerPte,
                                            PointerProtoPte,
                                            LockIrql,
                                            &OutPfn);

                    /* THIS RELEASES THE PFN LOCK! */
                    break;
                }

                /* No clustered faults yet */
                ASSERT(FALSE);
            }

            /* Did we resolve the fault? */
            if (ProcessedPtes)
            {
                /* Bump the transition count */
                InterlockedExchangeAddSizeT(&KeGetCurrentPrcb()->MmTransitionCount, ProcessedPtes);
                ProcessedPtes--;

                /* Loop all the processing we did */
                ASSERT(ProcessedPtes == 0);

                /* Complete this as a transition fault */
                ASSERT(OldIrql == KeGetCurrentIrql());
                ASSERT(OldIrql <= APC_LEVEL);
                ASSERT(KeAreAllApcsDisabled() == TRUE);
                return STATUS_PAGE_FAULT_TRANSITION;
            }

            /* We did not -- PFN lock is still held, prepare to resolve prototype PTE fault */
            OutPfn = MI_PFN_ELEMENT(SuperProtoPte->u.Hard.PageFrameNumber);
            MiReferenceUsedPageAndBumpLockCount(OutPfn);
            ASSERT(OutPfn->u3.e2.ReferenceCount > 1);
            ASSERT(PointerPte->u.Hard.Valid == 0);

            /* Resolve the fault -- this will release the PFN lock */
            Status = MiResolveProtoPteFault(StoreInstruction,
                                            Address,
                                            PointerPte,
                                            PointerProtoPte,
                                            &OutPfn,
                                            NULL,
                                            NULL,
                                            Process,
                                            LockIrql,
                                            TrapInformation);
            //ASSERT(Status != STATUS_ISSUE_PAGING_IO);
            //ASSERT(Status != STATUS_REFAULT);
            //ASSERT(Status != STATUS_PTE_CHANGED);

            /* Did the routine clean out the PFN or should we? */
            if (OutPfn)
            {
                /* We had a locked PFN, so acquire the PFN lock to dereference it */
                ASSERT(PointerProtoPte != NULL);
                OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

                /* Dereference the locked PFN */
                MiDereferencePfnAndDropLockCount(OutPfn);
                ASSERT(OutPfn->u3.e2.ReferenceCount >= 1);

                /* And now release the lock */
                KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
            }

            /* Complete this as a transition fault */
            ASSERT(OldIrql == KeGetCurrentIrql());
            ASSERT(OldIrql <= APC_LEVEL);
            ASSERT(KeAreAllApcsDisabled() == TRUE);
            return Status;
        }
    }

    /* Is this a transition PTE */
    if (TempPte.u.Soft.Transition)
    {
        PVOID InPageBlock = NULL;
        /* Lock the PFN database */
        LockIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* Resolve */
        Status = MiResolveTransitionFault(Address, PointerPte, Process, LockIrql, &InPageBlock);

        NT_ASSERT(NT_SUCCESS(Status));

        /* And now release the lock and leave*/
        KeReleaseQueuedSpinLock(LockQueuePfnLock, LockIrql);

        if (InPageBlock != NULL)
        {
            /* The page is being paged in by another process */
            KeWaitForSingleObject(InPageBlock, WrPageIn, KernelMode, FALSE, NULL);
        }

        ASSERT(OldIrql == KeGetCurrentIrql());
        ASSERT(OldIrql <= APC_LEVEL);
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        return Status;
    }

    /* Should we page the data back in ? */
    if (TempPte.u.Soft.PageFileHigh != 0)
    {
        /* Lock the PFN database */
        LockIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* Resolve */
        Status = MiResolvePageFileFault(StoreInstruction, Address, PointerPte, Process, &LockIrql);

        /* And now release the lock and leave*/
        KeReleaseQueuedSpinLock(LockQueuePfnLock, LockIrql);

        ASSERT(OldIrql == KeGetCurrentIrql());
        ASSERT(OldIrql <= APC_LEVEL);
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        return Status;
    }

    //
    // The PTE must be invalid but not completely empty. It must also not be a
    // prototype a transition or a paged-out PTE as those scenarii should've been handled above.
    // These are all Windows checks
    //
    ASSERT(TempPte.u.Hard.Valid == 0);
    ASSERT(TempPte.u.Soft.Prototype == 0);
    ASSERT(TempPte.u.Soft.Transition == 0);
    ASSERT(TempPte.u.Soft.PageFileHigh == 0);
    ASSERT(TempPte.u.Long != 0);

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
    PMMVAD Vad = NULL;
    PFN_NUMBER PageFrameIndex;
    ULONG Color;
    BOOLEAN IsSessionAddress;
    PMMPFN Pfn1;
    DPRINT("ARM3 FAULT AT: %p\n", Address);

    /* Check for page fault on high IRQL */
    if (OldIrql > APC_LEVEL)
    {
#if (_MI_PAGING_LEVELS < 3)
        /* Could be a page table for paged pool, which we'll allow */
        if (MI_IS_SYSTEM_PAGE_TABLE_ADDRESS(Address)) MiSynchronizeSystemPde((PMMPDE)PointerPte);
        MiCheckPdeForPagedPool(Address);
#endif
        /* Check if any of the top-level pages are invalid */
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
            /* This fault is not valid, print out some debugging help */
            DbgPrint("MM:***PAGE FAULT AT IRQL > 1  Va %p, IRQL %lx\n",
                     Address,
                     OldIrql);
            if (TrapInformation)
            {
                PKTRAP_FRAME TrapFrame = TrapInformation;
#ifdef _M_IX86
                DbgPrint("MM:***EIP %p, EFL %p\n", TrapFrame->Eip, TrapFrame->EFlags);
                DbgPrint("MM:***EAX %p, ECX %p EDX %p\n", TrapFrame->Eax, TrapFrame->Ecx, TrapFrame->Edx);
                DbgPrint("MM:***EBX %p, ESI %p EDI %p\n", TrapFrame->Ebx, TrapFrame->Esi, TrapFrame->Edi);
#elif defined(_M_AMD64)
                DbgPrint("MM:***RIP %p, EFL %p\n", TrapFrame->Rip, TrapFrame->EFlags);
                DbgPrint("MM:***RAX %p, RCX %p RDX %p\n", TrapFrame->Rax, TrapFrame->Rcx, TrapFrame->Rdx);
                DbgPrint("MM:***RBX %p, RSI %p RDI %p\n", TrapFrame->Rbx, TrapFrame->Rsi, TrapFrame->Rdi);
#elif defined(_M_ARM)
                DbgPrint("MM:***PC %p\n", TrapFrame->Pc);
                DbgPrint("MM:***R0 %p, R1 %p R2 %p, R3 %p\n", TrapFrame->R0, TrapFrame->R1, TrapFrame->R2, TrapFrame->R3);
                DbgPrint("MM:***R11 %p, R12 %p SP %p, LR %p\n", TrapFrame->R11, TrapFrame->R12, TrapFrame->Sp, TrapFrame->Lr);
#endif
            }

            /* Tell the trap handler to fail */
            return STATUS_IN_PAGE_ERROR | 0x10000000;
        }

        /* Not yet implemented in ReactOS */
        ASSERT(MI_IS_PAGE_LARGE(PointerPde) == FALSE);
        ASSERT(((StoreInstruction) && MI_IS_PAGE_COPY_ON_WRITE(PointerPte)) == FALSE);

        /* Check if this was a write */
        if (StoreInstruction)
        {
            /* Was it to a read-only page? */
            Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
            if (!(PointerPte->u.Long & PTE_READWRITE) &&
                !(Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE))
            {
                /* Crash with distinguished bugcheck code */
                KeBugCheckEx(ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                             (ULONG_PTR)Address,
                             PointerPte->u.Long,
                             (ULONG_PTR)TrapInformation,
                             10);
            }
        }

        /* Nothing is actually wrong */
        DPRINT1("Fault at IRQL %u is ok (%p)\n", OldIrql, Address);
        return STATUS_SUCCESS;
    }

    /* Check for kernel fault address */
    if (Address >= MmSystemRangeStart)
    {
        /* Bail out, if the fault came from user mode */
        if (Mode == UserMode) return STATUS_ACCESS_VIOLATION;

#if (_MI_PAGING_LEVELS == 2)
        if (MI_IS_SYSTEM_PAGE_TABLE_ADDRESS(Address)) MiSynchronizeSystemPde((PMMPDE)PointerPte);
        MiCheckPdeForPagedPool(Address);
#endif

        /* Check if the higher page table entries are invalid */
        if (
#if (_MI_PAGING_LEVELS == 4)
            /* AMD64 system, check if PXE is invalid */
            (PointerPxe->u.Hard.Valid == 0) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
            /* PAE/AMD64 system, check if PPE is invalid */
            (PointerPpe->u.Hard.Valid == 0) ||
#endif
            /* Always check if the PDE is valid */
            (PointerPde->u.Hard.Valid == 0))
        {
            /* PXE/PPE/PDE (still) not valid, kill the system */
            KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                         (ULONG_PTR)Address,
                         StoreInstruction,
                         (ULONG_PTR)TrapInformation,
                         2);
        }

        /* Not handling session faults yet */
        IsSessionAddress = MI_IS_SESSION_ADDRESS(Address);

        /* The PDE is valid, so read the PTE */
        TempPte = *PointerPte;
        if (TempPte.u.Hard.Valid == 1)
        {
            /* Check if this was system space or session space */
            if (!IsSessionAddress)
            {
                /* Check if the PTE is still valid under PFN lock */
                OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
                TempPte = *PointerPte;
                if (TempPte.u.Hard.Valid)
                {
                    /* Check if this was a write */
                    if (StoreInstruction)
                    {
                        /* Was it to a read-only page? */
                        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        if (!(PointerPte->u.Long & PTE_READWRITE) &&
                            !(Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE))
                        {
                            /* Crash with distinguished bugcheck code */
                            KeBugCheckEx(ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                         (ULONG_PTR)Address,
                                         PointerPte->u.Long,
                                         (ULONG_PTR)TrapInformation,
                                         11);
                        }
                    }
                }

                /* Release PFN lock and return all good */
                KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
                return STATUS_SUCCESS;
            }
        }
#if (_MI_PAGING_LEVELS == 2)
        /* Check if this was a session PTE that needs to remap the session PDE */
        if (MI_IS_SESSION_PTE(Address))
        {
            /* Do the remapping */
            Status = MiCheckPdeForSessionSpace(Address);
            if (!NT_SUCCESS(Status))
            {
                /* It failed, this address is invalid */
                KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                             (ULONG_PTR)Address,
                             StoreInstruction,
                             (ULONG_PTR)TrapInformation,
                             6);
            }
        }
#else

_WARN("Session space stuff is not implemented yet!")

#endif

        /* Check for a fault on the page table or hyperspace */
        if (MI_IS_PAGE_TABLE_OR_HYPER_ADDRESS(Address))
        {
#if (_MI_PAGING_LEVELS < 3)
            /* Windows does this check but I don't understand why -- it's done above! */
            ASSERT(MiCheckPdeForPagedPool(Address) != STATUS_WAIT_1);
#endif
            /* Handle this as a user mode fault */
            goto UserFault;
        }

        /* Get the current thread */
        CurrentThread = PsGetCurrentThread();

        /* What kind of address is this */
        if (!IsSessionAddress)
        {
            /* Use the system working set */
            WorkingSet = &MmSystemCacheWs;
            CurrentProcess = NULL;

            /* Make sure we don't have a recursive working set lock */
            if ((CurrentThread->OwnsProcessWorkingSetExclusive) ||
                (CurrentThread->OwnsProcessWorkingSetShared) ||
                (CurrentThread->OwnsSystemWorkingSetExclusive) ||
                (CurrentThread->OwnsSystemWorkingSetShared) ||
                (CurrentThread->OwnsSessionWorkingSetExclusive) ||
                (CurrentThread->OwnsSessionWorkingSetShared))
            {
                /* Fail */
                return STATUS_IN_PAGE_ERROR | 0x10000000;
            }
        }
        else
        {
            /* Use the session process and working set */
            CurrentProcess = HYDRA_PROCESS;
            WorkingSet = &MmSessionSpace->GlobalVirtualAddress->Vm;

            /* Make sure we don't have a recursive working set lock */
            if ((CurrentThread->OwnsSessionWorkingSetExclusive) ||
                (CurrentThread->OwnsSessionWorkingSetShared))
            {
                /* Fail */
                return STATUS_IN_PAGE_ERROR | 0x10000000;
            }
        }

        /* Acquire the working set lock */
        KeRaiseIrql(APC_LEVEL, &LockIrql);
        MiLockWorkingSet(CurrentThread, WorkingSet);

        /* Re-read PTE now that we own the lock */
        TempPte = *PointerPte;
        if (TempPte.u.Hard.Valid == 1)
        {
            /* Check if this was a write */
            if (StoreInstruction)
            {
                /* Was it to a read-only page that is not copy on write? */
                Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                if (!(TempPte.u.Long & PTE_READWRITE) &&
                    !(Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) &&
                    !MI_IS_PAGE_COPY_ON_WRITE(&TempPte))
                {
                    /* Case not yet handled */
                    ASSERT(!IsSessionAddress);

                    /* Crash with distinguished bugcheck code */
                    KeBugCheckEx(ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                 (ULONG_PTR)Address,
                                 TempPte.u.Long,
                                 (ULONG_PTR)TrapInformation,
                                 12);
                }
            }

            /* Check for read-only write in session space */
            if ((IsSessionAddress) &&
                (StoreInstruction) &&
                !MI_IS_PAGE_WRITEABLE(&TempPte))
            {
                /* Sanity check */
                ASSERT(MI_IS_SESSION_IMAGE_ADDRESS(Address));

                /* Was this COW? */
                if (!MI_IS_PAGE_COPY_ON_WRITE(&TempPte))
                {
                    /* Then this is not allowed */
                    KeBugCheckEx(ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                 (ULONG_PTR)Address,
                                 (ULONG_PTR)TempPte.u.Long,
                                 (ULONG_PTR)TrapInformation,
                                 13);
                }

                /* Otherwise, handle COW */
                ASSERT(FALSE);
            }

            /* Release the working set */
            MiUnlockWorkingSet(CurrentThread, WorkingSet);
            KeLowerIrql(LockIrql);

            /* Otherwise, the PDE was probably invalid, and all is good now */
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

            /* Do we need to locate the prototype PTE in session space? */
            if ((IsSessionAddress) &&
                (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED))
            {
                /* Yep, go find it as well as the VAD for it */
                ProtoPte = MiCheckVirtualAddress(Address,
                                                 &ProtectionCode,
                                                 &Vad);
                ASSERT(ProtoPte != NULL);
            }
        }
        else
        {
            /* We don't implement transition PTEs */
            ASSERT(TempPte.u.Soft.Transition == 0);

            /* Check for no-access PTE */
            if (TempPte.u.Soft.Protection == MM_NOACCESS)
            {
                /* Bugcheck the system! */
                KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                             (ULONG_PTR)Address,
                             StoreInstruction,
                             (ULONG_PTR)TrapInformation,
                             1);
            }

            /* Check for no protecton at all */
            if (TempPte.u.Soft.Protection == MM_ZERO_ACCESS)
            {
                /* Bugcheck the system! */
                KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                             (ULONG_PTR)Address,
                             StoreInstruction,
                             (ULONG_PTR)TrapInformation,
                             0);
            }
        }

        /* Check for demand page */
        if ((StoreInstruction) &&
            !(ProtoPte) &&
            !(IsSessionAddress) &&
            !(TempPte.u.Hard.Valid))
        {
            /* Get the protection code */
            ASSERT(TempPte.u.Soft.Transition == 0);
            if (!(TempPte.u.Soft.Protection & MM_READWRITE))
            {
                /* Bugcheck the system! */
                KeBugCheckEx(ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                             (ULONG_PTR)Address,
                             TempPte.u.Long,
                             (ULONG_PTR)TrapInformation,
                             14);
            }
        }

        /* Now do the real fault handling */
        Status = MiDispatchFault(StoreInstruction,
                                 Address,
                                 PointerPte,
                                 ProtoPte,
                                 FALSE,
                                 CurrentProcess,
                                 TrapInformation,
                                 NULL);

        /* Release the working set */
        ASSERT(KeAreAllApcsDisabled() == TRUE);
        MiUnlockWorkingSet(CurrentThread, WorkingSet);
        KeLowerIrql(LockIrql);

        /* We are done! */
        DPRINT("Fault resolved with status: %lx\n", Status);
        return Status;
    }

    /* This is a user fault */
UserFault:
    CurrentThread = PsGetCurrentThread();
    CurrentProcess = (PEPROCESS)CurrentThread->Tcb.ApcState.Process;

    /* Lock the working set */
    MiLockProcessWorkingSet(CurrentProcess, CurrentThread);

#if (_MI_PAGING_LEVELS == 4)
// Note to Timo: You should call MiCheckVirtualAddress and also check if it's zero pte
// also this is missing the page count increment
    /* Check if the PXE is valid */
    if (PointerPxe->u.Hard.Valid == 0)
    {
        /* Right now, we only handle scenarios where the PXE is totally empty */
        ASSERT(PointerPxe->u.Long == 0);
#if 0
        /* Resolve a demand zero fault */
        Status = MiResolveDemandZeroFault(PointerPpe,
                                          MM_READWRITE,
                                          CurrentProcess,
                                          MM_NOIRQL);
#endif
        /* We should come back with a valid PXE */
        ASSERT(PointerPxe->u.Hard.Valid == 1);
    }
#endif

#if (_MI_PAGING_LEVELS >= 3)
// Note to Timo: You should call MiCheckVirtualAddress and also check if it's zero pte
// also this is missing the page count increment
    /* Check if the PPE is valid */
    if (PointerPpe->u.Hard.Valid == 0)
    {
        /* Right now, we only handle scenarios where the PPE is totally empty */
        ASSERT(PointerPpe->u.Long == 0);
#if 0
        /* Resolve a demand zero fault */
        Status = MiResolveDemandZeroFault(PointerPde,
                                          MM_READWRITE,
                                          CurrentProcess,
                                          MM_NOIRQL);
#endif
        /* We should come back with a valid PPE */
        ASSERT(PointerPpe->u.Hard.Valid == 1);
    }
#endif

    /* Check if the PDE is valid */
    if (PointerPde->u.Hard.Valid == 0)
    {
        /* Right now, we only handle scenarios where the PDE is totally empty */
        ASSERT(PointerPde->u.Long == 0);

        /* And go dispatch the fault on the PDE. This should handle the demand-zero */
#if MI_TRACE_PFNS
        UserPdeFault = TRUE;
#endif
        MiCheckVirtualAddress(Address, &ProtectionCode, &Vad);
        if (ProtectionCode == MM_NOACCESS)
        {
#if (_MI_PAGING_LEVELS == 2)
            /* Could be a page table for paged pool */
            MiCheckPdeForPagedPool(Address);
#endif
            /* Has the code above changed anything -- is this now a valid PTE? */
            Status = (PointerPde->u.Hard.Valid == 1) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION;

            /* Either this was a bogus VA or we've fixed up a paged pool PDE */
            MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
            return Status;
        }

        /* Write a demand-zero PDE */
        MI_WRITE_INVALID_PDE(PointerPde, DemandZeroPde);

        /* Dispatch the fault */
        Status = MiDispatchFault(TRUE,
                                 PointerPte,
                                 PointerPde,
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
    else
    {
        /* Not yet implemented in ReactOS */
        ASSERT(MI_IS_PAGE_LARGE(PointerPde) == FALSE);
    }

    /* Now capture the PTE. */
    TempPte = *PointerPte;

    /* Check if the PTE is valid */
    if (TempPte.u.Hard.Valid)
    {
        /* Check if this is a write on a readonly PTE */
        if (StoreInstruction)
        {
            /* Is this a copy on write PTE? */
            if (MI_IS_PAGE_COPY_ON_WRITE(&TempPte))
            {
                /* Not supported yet */
                ASSERT(FALSE);
            }

            /* Is this a read-only PTE? */
            if (!MI_IS_PAGE_WRITEABLE(&TempPte))
            {
                /* Return the status */
                MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
                return STATUS_ACCESS_VIOLATION;
            }
        }

        /* FIXME: Execution is ignored for now, since we don't have no-execute pages yet */

        /* The fault has already been resolved by a different thread */
        MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
        return STATUS_SUCCESS;
    }

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

    /* Check for zero PTE */
    if (TempPte.u.Long == 0)
    {
        /* Check if this address range belongs to a valid allocation (VAD) */
        ProtoPte = MiCheckVirtualAddress(Address, &ProtectionCode, &Vad);
        if (ProtectionCode == MM_NOACCESS)
        {
#if (_MI_PAGING_LEVELS == 2)
            /* Could be a page table for paged pool */
            MiCheckPdeForPagedPool(Address);
#endif
            /* Has the code above changed anything -- is this now a valid PTE? */
            Status = (PointerPte->u.Hard.Valid == 1) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION;

            /* Either this was a bogus VA or we've fixed up a paged pool PDE */
            MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
            return Status;
        }

        /*
         * Check if this is a real user-mode address or actually a kernel-mode
         * page table for a user mode address
         */
        if (Address <= MM_HIGHEST_USER_ADDRESS)
        {
            /* Add an additional page table reference */
            MiIncrementPageTableReferences(Address);
        }

        /* Is this a guard page? */
        if ((ProtectionCode & MM_PROTECT_SPECIAL) == MM_GUARDPAGE)
        {
            /* The VAD protection cannot be MM_DECOMMIT! */
            NT_ASSERT(ProtectionCode != MM_DECOMMIT);

            /* Remove the bit */
            TempPte.u.Soft.Protection = ProtectionCode & ~MM_GUARDPAGE;
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);

            /* Not supported */
            ASSERT(ProtoPte == NULL);
            ASSERT(CurrentThread->ApcNeeded == 0);

            /* Drop the working set lock */
            MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
            ASSERT(KeGetCurrentIrql() == OldIrql);

            /* Handle stack expansion */
            return MiCheckForUserStackOverflow(Address, TrapInformation);
        }

        /* Did we get a prototype PTE back? */
        if (!ProtoPte)
        {
            /* Is this PTE actually part of the PDE-PTE self-mapping directory? */
            if (PointerPde == MiAddressToPde(PTE_BASE))
            {
                /* Then it's really a demand-zero PDE (on behalf of user-mode) */
#ifdef _M_ARM
                _WARN("This is probably completely broken!");
                MI_WRITE_INVALID_PDE((PMMPDE)PointerPte, DemandZeroPde);
#else
                MI_WRITE_INVALID_PTE(PointerPte, DemandZeroPde);
#endif
            }
            else
            {
                /* No, create a new PTE. First, write the protection */
                TempPte.u.Soft.Protection = ProtectionCode;
                MI_WRITE_INVALID_PTE(PointerPte, TempPte);
            }

            /* Lock the PFN database since we're going to grab a page */
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

            /* Make sure we have enough pages */
            ASSERT(MmAvailablePages >= 32);

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

            /* Increment the count of pages in the process */
            CurrentProcess->NumberOfPrivatePages++;

            /* One more demand-zero fault */
            InterlockedIncrement(&KeGetCurrentPrcb()->MmDemandZeroCount);

            /* Fault on user PDE, or fault on user PTE? */
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
                /* This is a user-mode PDE, create a kernel PTE for it */
                MI_MAKE_HARDWARE_PTE(&TempPte,
                                     PointerPte,
                                     PointerPte->u.Soft.Protection,
                                     PageFrameIndex);
            }

            /* Write the dirty bit for writeable pages */
            if (MI_IS_PAGE_WRITEABLE(&TempPte)) MI_MAKE_DIRTY_PAGE(&TempPte);

            /* And now write down the PTE, making the address valid */
            MI_WRITE_VALID_PTE(PointerPte, TempPte);
            Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
            ASSERT(Pfn1->u1.Event == NULL);

            /* Demand zero */
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
            return STATUS_PAGE_FAULT_DEMAND_ZERO;
        }

        /* We should have a valid protection here */
        ASSERT(ProtectionCode != 0x100);

        /* Write the prototype PTE */
        TempPte = PrototypePte;
        TempPte.u.Soft.Protection = ProtectionCode;
        NT_ASSERT(TempPte.u.Long != 0);
        MI_WRITE_INVALID_PTE(PointerPte, TempPte);
    }
    else
    {
        /* Get the protection code and check if this is a proto PTE */
        ProtectionCode = (ULONG)TempPte.u.Soft.Protection;
        if (TempPte.u.Soft.Prototype)
        {
            /* Do we need to go find the real PTE? */
            if (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)
            {
                /* Get the prototype pte and VAD for it */
                ProtoPte = MiCheckVirtualAddress(Address,
                                                 &ProtectionCode,
                                                 &Vad);
                if (!ProtoPte)
                {
                    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
                    MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
                    return STATUS_ACCESS_VIOLATION;
                }
            }
            else
            {
                /* Get the prototype PTE! */
                ProtoPte = MiProtoPteToPte(&TempPte);

                /* Is it read-only */
                if (TempPte.u.Proto.ReadOnly)
                {
                    /* Set read-only code */
                    ProtectionCode = MM_READONLY;
                }
                else
                {
                    /* Set unknown protection */
                    ProtectionCode = 0x100;
                    ASSERT(CurrentProcess->CloneRoot != NULL);
                }
            }
        }
    }

    /* Do we have a valid protection code? */
    if (ProtectionCode != 0x100)
    {
        /* Run a software access check first, including to detect guard pages */
        Status = MiAccessCheck(PointerPte,
                               StoreInstruction,
                               Mode,
                               ProtectionCode,
                               TrapInformation,
                               FALSE);
        if (Status != STATUS_SUCCESS)
        {
            /* Not supported */
            ASSERT(CurrentThread->ApcNeeded == 0);

            /* Drop the working set lock */
            MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
            ASSERT(KeGetCurrentIrql() == OldIrql);

            /* Did we hit a guard page? */
            if (Status == STATUS_GUARD_PAGE_VIOLATION)
            {
                /* Handle stack expansion */
                return MiCheckForUserStackOverflow(Address, TrapInformation);
            }

            /* Otherwise, fail back to the caller directly */
            return Status;
        }
    }

    /* Dispatch the fault */
    Status = MiDispatchFault(StoreInstruction,
                             Address,
                             PointerPte,
                             ProtoPte,
                             FALSE,
                             CurrentProcess,
                             TrapInformation,
                             Vad);

    /* Return the status */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    MiUnlockProcessWorkingSet(CurrentProcess, CurrentThread);
    return Status;
}

NTSTATUS
NTAPI
MmGetExecuteOptions(IN PULONG ExecuteOptions)
{
    PKPROCESS CurrentProcess = &PsGetCurrentProcess()->Pcb;
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    *ExecuteOptions = 0;

    if (CurrentProcess->Flags.ExecuteDisable)
    {
        *ExecuteOptions |= MEM_EXECUTE_OPTION_DISABLE;
    }

    if (CurrentProcess->Flags.ExecuteEnable)
    {
        *ExecuteOptions |= MEM_EXECUTE_OPTION_ENABLE;
    }

    if (CurrentProcess->Flags.DisableThunkEmulation)
    {
        *ExecuteOptions |= MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION;
    }

    if (CurrentProcess->Flags.Permanent)
    {
        *ExecuteOptions |= MEM_EXECUTE_OPTION_PERMANENT;
    }

    if (CurrentProcess->Flags.ExecuteDispatchEnable)
    {
        *ExecuteOptions |= MEM_EXECUTE_OPTION_EXECUTE_DISPATCH_ENABLE;
    }

    if (CurrentProcess->Flags.ImageDispatchEnable)
    {
        *ExecuteOptions |= MEM_EXECUTE_OPTION_IMAGE_DISPATCH_ENABLE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmSetExecuteOptions(IN ULONG ExecuteOptions)
{
    PKPROCESS CurrentProcess = &PsGetCurrentProcess()->Pcb;
    KLOCK_QUEUE_HANDLE ProcessLock;
    NTSTATUS Status = STATUS_ACCESS_DENIED;
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    /* Only accept valid flags */
    if (ExecuteOptions & ~MEM_EXECUTE_OPTION_VALID_FLAGS)
    {
        /* Fail */
        DPRINT1("Invalid no-execute options\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Change the NX state in the process lock */
    KiAcquireProcessLock(CurrentProcess, &ProcessLock);

    /* Don't change anything if the permanent flag was set */
    if (!CurrentProcess->Flags.Permanent)
    {
        /* Start by assuming it's not disabled */
        CurrentProcess->Flags.ExecuteDisable = FALSE;

        /* Now process each flag and turn the equivalent bit on */
        if (ExecuteOptions & MEM_EXECUTE_OPTION_DISABLE)
        {
            CurrentProcess->Flags.ExecuteDisable = TRUE;
        }
        if (ExecuteOptions & MEM_EXECUTE_OPTION_ENABLE)
        {
            CurrentProcess->Flags.ExecuteEnable = TRUE;
        }
        if (ExecuteOptions & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
        {
            CurrentProcess->Flags.DisableThunkEmulation = TRUE;
        }
        if (ExecuteOptions & MEM_EXECUTE_OPTION_PERMANENT)
        {
            CurrentProcess->Flags.Permanent = TRUE;
        }
        if (ExecuteOptions & MEM_EXECUTE_OPTION_EXECUTE_DISPATCH_ENABLE)
        {
            CurrentProcess->Flags.ExecuteDispatchEnable = TRUE;
        }
        if (ExecuteOptions & MEM_EXECUTE_OPTION_IMAGE_DISPATCH_ENABLE)
        {
            CurrentProcess->Flags.ImageDispatchEnable = TRUE;
        }

        /* These are turned on by default if no-execution is also eanbled */
        if (CurrentProcess->Flags.ExecuteEnable)
        {
            CurrentProcess->Flags.ExecuteDispatchEnable = TRUE;
            CurrentProcess->Flags.ImageDispatchEnable = TRUE;
        }

        /* All good */
        Status = STATUS_SUCCESS;
    }

    /* Release the lock and return status */
    KiReleaseProcessLock(&ProcessLock);
    return Status;
}

/* EOF */
