/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/i386/page.c
 * PURPOSE:         Low level memory managment manipulation
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <mm/ARM3/miarm.h>

#ifndef _MI_PAGING_LEVELS
#error "Dude, fix your stuff before using this file"
#endif

/* GLOBALS *****************************************************************/
const
ULONG_PTR
MmProtectToPteMask[32] =
{
    //
    // These are the base MM_ protection flags
    //
    0,
    PTE_READONLY            | PTE_ENABLE_CACHE,
    PTE_EXECUTE             | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_ENABLE_CACHE,
    PTE_READWRITE           | PTE_ENABLE_CACHE,
    PTE_WRITECOPY           | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_ENABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_ENABLE_CACHE,
    //
    // These OR in the MM_NOCACHE flag
    //
    0,
    PTE_READONLY            | PTE_DISABLE_CACHE,
    PTE_EXECUTE             | PTE_DISABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_DISABLE_CACHE,
    PTE_READWRITE           | PTE_DISABLE_CACHE,
    PTE_WRITECOPY           | PTE_DISABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_DISABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_DISABLE_CACHE,
    //
    // These OR in the MM_DECOMMIT flag, which doesn't seem supported on x86/64/ARM
    //
    0,
    PTE_READONLY            | PTE_ENABLE_CACHE,
    PTE_EXECUTE             | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_ENABLE_CACHE,
    PTE_READWRITE           | PTE_ENABLE_CACHE,
    PTE_WRITECOPY           | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_ENABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_ENABLE_CACHE,
    //
    // These OR in the MM_NOACCESS flag, which seems to enable WriteCombining?
    //
    0,
    PTE_READONLY            | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE             | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_READ        | PTE_WRITECOMBINED_CACHE,
    PTE_READWRITE           | PTE_WRITECOMBINED_CACHE,
    PTE_WRITECOPY           | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_WRITECOMBINED_CACHE,
};

const
ULONG MmProtectToValue[32] =
{
    PAGE_NOACCESS,
    PAGE_READONLY,
    PAGE_EXECUTE,
    PAGE_EXECUTE_READ,
    PAGE_READWRITE,
    PAGE_WRITECOPY,
    PAGE_EXECUTE_READWRITE,
    PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_NOCACHE | PAGE_READONLY,
    PAGE_NOCACHE | PAGE_EXECUTE,
    PAGE_NOCACHE | PAGE_EXECUTE_READ,
    PAGE_NOCACHE | PAGE_READWRITE,
    PAGE_NOCACHE | PAGE_WRITECOPY,
    PAGE_NOCACHE | PAGE_EXECUTE_READWRITE,
    PAGE_NOCACHE | PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_GUARD | PAGE_READONLY,
    PAGE_GUARD | PAGE_EXECUTE,
    PAGE_GUARD | PAGE_EXECUTE_READ,
    PAGE_GUARD | PAGE_READWRITE,
    PAGE_GUARD | PAGE_WRITECOPY,
    PAGE_GUARD | PAGE_EXECUTE_READWRITE,
    PAGE_GUARD | PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_WRITECOMBINE | PAGE_READONLY,
    PAGE_WRITECOMBINE | PAGE_EXECUTE,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_READ,
    PAGE_WRITECOMBINE | PAGE_READWRITE,
    PAGE_WRITECOMBINE | PAGE_WRITECOPY,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_READWRITE,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_WRITECOPY
};

/* FUNCTIONS ***************************************************************/

NTSTATUS
NTAPI
MiFillSystemPageDirectory(IN PVOID Base,
                          IN SIZE_T NumberOfBytes);

static
BOOLEAN
MiIsPageTablePresent(PVOID Address)
{
#if _MI_PAGING_LEVELS == 2
    return MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)] != 0;
#else
    PMMPDE PointerPde;
    PMMPPE PointerPpe;
#if _MI_PAGING_LEVELS == 4
    PMMPXE PointerPxe;
#endif
    PMMPFN Pfn;

    /* Make sure we're locked */
    ASSERT((PsGetCurrentThread()->OwnsProcessWorkingSetExclusive) || (PsGetCurrentThread()->OwnsProcessWorkingSetShared));

    /* Must not hold the PFN lock! */
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

        /* Check if PXE or PPE have references first. */
#if _MI_PAGING_LEVELS == 4
    PointerPxe = MiAddressToPxe(Address);
    if ((PointerPxe->u.Hard.Valid == 1) || (PointerPxe->u.Soft.Transition == 1))
    {
        Pfn = MiGetPfnEntry(PFN_FROM_PXE(PointerPxe));
        if (Pfn->OriginalPte.u.Soft.UsedPageTableEntries == 0)
            return FALSE;
    }
    else if (PointerPxe->u.Soft.UsedPageTableEntries == 0)
    {
        return FALSE;
    }

    if (PointerPxe->u.Hard.Valid == 0)
    {
        MiMakeSystemAddressValid(MiPteToAddress(PointerPxe), PsGetCurrentProcess());
    }
#endif

    PointerPpe = MiAddressToPpe(Address);
    if ((PointerPpe->u.Hard.Valid == 1) || (PointerPpe->u.Soft.Transition == 1))
    {
        Pfn = MiGetPfnEntry(PFN_FROM_PPE(PointerPpe));
        if (Pfn->OriginalPte.u.Soft.UsedPageTableEntries == 0)
            return FALSE;
    }
    else if (PointerPpe->u.Soft.UsedPageTableEntries == 0)
    {
        return FALSE;
    }

    if (PointerPpe->u.Hard.Valid == 0)
    {
        MiMakeSystemAddressValid(MiPteToAddress(PointerPpe), PsGetCurrentProcess());
    }

    PointerPde = MiAddressToPde(Address);
    if ((PointerPde->u.Hard.Valid == 0) && (PointerPde->u.Soft.Transition == 0))
    {
        return PointerPde->u.Soft.UsedPageTableEntries != 0;
    }

    /* This lies on the PFN */
    Pfn = MiGetPfnEntry(PFN_FROM_PDE(PointerPde));
    return Pfn->OriginalPte.u.Soft.UsedPageTableEntries != 0;
#endif
}

PFN_NUMBER
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
    PMMPTE PointerPte;
    PFN_NUMBER Page;

    /* Must be called for user mode only */
    ASSERT(Process != NULL);
    ASSERT(Address < MmSystemRangeStart);

    /* And for our process */
    ASSERT(Process == PsGetCurrentProcess());

    /* Lock for reading */
    MiLockProcessWorkingSetShared(Process, PsGetCurrentThread());

    if (!MiIsPageTablePresent(Address))
    {
        MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
        return 0;
    }

    /* Make sure we can read the PTE */
    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);
    Page = PointerPte->u.Hard.Valid ? PFN_FROM_PTE(PointerPte) : 0;

    MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
    return Page;
}

VOID
NTAPI
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address,
                       BOOLEAN* WasDirty, PPFN_NUMBER Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    PMMPTE PointerPte;
    MMPTE OldPte;

    DPRINT("MmDeleteVirtualMapping(%p, %p, %p, %p)\n", Process, Address, WasDirty, Page);

    ASSERT(((ULONG_PTR)Address % PAGE_SIZE) == 0);

    /* And we should be at low IRQL */
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    /* Make sure our PDE is valid, and that everything is going fine */
    if (Process == NULL)
    {
        if (Address < MmSystemRangeStart)
        {
            DPRINT1("NULL process given for user-mode mapping at %p\n", Address);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
#if (_MI_PAGING_LEVELS == 2)
        if (!MiSynchronizeSystemPde(MiAddressToPde(Address)))
#else
        if (!MiIsPdeForAddressValid(Address))
#endif
        {
            /* There can't be a page if there is no PDE */
            if (WasDirty)
                *WasDirty = FALSE;
            if (Page)
                *Page = 0;
            return;
        }
    }
    else
    {
        if ((Address >= MmSystemRangeStart) || Add2Ptr(Address, PAGE_SIZE) >= MmSystemRangeStart)
        {
            DPRINT1("Process %p given for kernel-mode mapping at %p\n", Process, Address);
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        /* Only for current process !!! */
        ASSERT(Process = PsGetCurrentProcess());
        MiLockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

        /* No PDE --> No page */
        if (!MiIsPageTablePresent(Address))
        {
            MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());
            if (WasDirty)
                *WasDirty = 0;
            if (Page)
                *Page = 0;
            return;
        }

        MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);
    }

    PointerPte = MiAddressToPte(Address);
    OldPte.u.Long = InterlockedExchangePte(PointerPte, 0);

    if (OldPte.u.Long == 0)
    {
        /* There was nothing here */
        if (Address < MmSystemRangeStart)
            MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());
        if (WasDirty)
            *WasDirty = 0;
        if (Page)
            *Page = 0;
        return;
    }

    /* It must have been present, or not a swap entry */
    ASSERT(OldPte.u.Hard.Valid || !FlagOn(OldPte.u.Long, 0x800));

    if (OldPte.u.Hard.Valid)
        KeInvalidateTlbEntry(Address);

    if (Address < MmSystemRangeStart)
    {
        /* Remove PDE reference */
        if (MiDecrementPageTableReferences(Address) == 0)
        {
            KIRQL OldIrql = MiAcquirePfnLock();
            MiDeletePde(MiAddressToPde(Address), Process);
            MiReleasePfnLock(OldIrql);
        }

        MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());
    }

    if (WasDirty)
        *WasDirty = !!OldPte.u.Hard.Dirty;
    if (Page)
        *Page = OldPte.u.Hard.PageFrameNumber;
}


VOID
NTAPI
MmDeletePageFileMapping(
    PEPROCESS Process,
    PVOID Address,
    SWAPENTRY* SwapEntry)
{
    PMMPTE PointerPte;
    MMPTE OldPte;

    /* This should not be called for kernel space anymore */
    ASSERT(Process != NULL);
    ASSERT(Address < MmSystemRangeStart);

    /* And we don't support deleting for other process */
    ASSERT(Process == PsGetCurrentProcess());

    /* And we should be at low IRQL */
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    /* We are tinkering with the PDE here. Ensure it will be there */
    MiLockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

    /* Callers must ensure there is actually something there */
    ASSERT(MiAddressToPde(Address)->u.Long != 0);

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);
    OldPte.u.Long = InterlockedExchangePte(PointerPte, 0);
    /* This must be a swap entry ! */
    if (!FlagOn(OldPte.u.Long, 0x800) || OldPte.u.Hard.Valid)
    {
        KeBugCheckEx(MEMORY_MANAGEMENT, OldPte.u.Long, (ULONG_PTR)Process, (ULONG_PTR)Address, 0);
    }

    /* This used to be a non-zero PTE, now we can let the PDE go. */
    if (MiDecrementPageTableReferences(Address) == 0)
    {
        /* We can let it go */
        KIRQL OldIrql = MiAcquirePfnLock();
        MiDeletePde(MiPteToPde(PointerPte), Process);
        MiReleasePfnLock(OldIrql);
    }

    MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

    *SwapEntry = OldPte.u.Long >> 1;
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
    BOOLEAN Ret;

    if (Address >= MmSystemRangeStart)
    {
        ASSERT(Process == NULL);
#if _MI_PAGING_LEVELS == 2
        if (!MiSynchronizeSystemPde(MiAddressToPde(Address)))
#else
        if (!MiIsPdeForAddressValid(Address))
#endif
        {
            /* It can't be present if there is no PDE */
            return FALSE;
        }

        return MiAddressToPte(Address)->u.Hard.Valid;
    }

    ASSERT(Process != NULL);
    ASSERT(Process == PsGetCurrentProcess());

    MiLockProcessWorkingSetShared(Process, PsGetCurrentThread());

    if (!MiIsPageTablePresent(Address))
    {
        /* It can't be present if there is no PDE */
        MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
        return FALSE;
    }

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    Ret = MiAddressToPte(Address)->u.Hard.Valid;

    MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());

    return Ret;
}

BOOLEAN
NTAPI
MmIsDisabledPage(PEPROCESS Process, PVOID Address)
{
    BOOLEAN Ret;
    PMMPTE PointerPte;

    if (Address >= MmSystemRangeStart)
    {
        ASSERT(Process == NULL);
#if _MI_PAGING_LEVELS == 2
        if (!MiSynchronizeSystemPde(MiAddressToPde(Address)))
#else
        if (!MiIsPdeForAddressValid(Address))
#endif
        {
            /* It's not disabled if it's not present */
            return FALSE;
        }
    }
    else
    {
        ASSERT(Process != NULL);
        ASSERT(Process == PsGetCurrentProcess());

        MiLockProcessWorkingSetShared(Process, PsGetCurrentThread());

        if (!MiIsPageTablePresent(Address))
        {
            /* It can't be disabled if there is no PDE */
            MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
            return FALSE;
        }

        MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);
    }

    PointerPte = MiAddressToPte(Address);
    Ret = !PointerPte->u.Hard.Valid
        && !FlagOn(PointerPte->u.Long, 0x800)
        && (PointerPte->u.Hard.PageFrameNumber != 0);

    if (Address < MmSystemRangeStart)
        MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());

    return Ret;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    BOOLEAN Ret;
    PMMPTE PointerPte;

    /* We never set swap entries for kernel addresses */
    if (Address >= MmSystemRangeStart)
    {
        ASSERT(Process == NULL);
        return FALSE;
    }

    ASSERT(Process != NULL);
    ASSERT(Process == PsGetCurrentProcess());

    MiLockProcessWorkingSetShared(Process, PsGetCurrentThread());

    if (!MiIsPageTablePresent(Address))
    {
        /* There can't be a swap entry if there is no PDE */
        MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
        return FALSE;
    }

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);
    Ret = !PointerPte->u.Hard.Valid && FlagOn(PointerPte->u.Long, 0x800);

    MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());

    return Ret;
}

VOID
NTAPI
MmGetPageFileMapping(PEPROCESS Process, PVOID Address, SWAPENTRY* SwapEntry)
{
    PMMPTE PointerPte;

    /* We never set swap entries for kernel addresses */
    if (Address >= MmSystemRangeStart)
    {
        ASSERT(Process == NULL);
        *SwapEntry = 0;
        return;
    }

    ASSERT(Process != NULL);
    ASSERT(Process == PsGetCurrentProcess());

    MiLockProcessWorkingSetShared(Process, PsGetCurrentThread());

    if (!MiIsPageTablePresent(Address))
    {
        /* There can't be a swap entry if there is no PDE */
        MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
        *SwapEntry = 0;
        return;
    }

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);
    if (!PointerPte->u.Hard.Valid && FlagOn(PointerPte->u.Long, 0x800))
        *SwapEntry = PointerPte->u.Long >> 1;
    else
        *SwapEntry = 0;

    MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
    PMMPTE PointerPte;
    ULONG_PTR Pte;

    /* This should not be called for kernel space anymore */
    ASSERT(Process != NULL);
    ASSERT(Address < MmSystemRangeStart);

    /* And we don't support creating for other process */
    ASSERT(Process == PsGetCurrentProcess());

    if (SwapEntry & (1 << 31))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /* We are tinkering with the PDE here. Ensure it will be there */
    ASSERT(Process == PsGetCurrentProcess());
    MiLockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);
    Pte = InterlockedExchangePte(PointerPte, SwapEntry << 1);
    if (Pte != 0)
    {
        KeBugCheckEx(MEMORY_MANAGEMENT, SwapEntry, (ULONG_PTR)Process, (ULONG_PTR)Address, 0);
    }

    /* This used to be a 0 PTE, now we need a valid PDE to keep it around */
    MiIncrementPageTableReferences(Address);
    MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PFN_NUMBER Page)
{
    ULONG ProtectionMask;
    PMMPTE PointerPte;
    MMPTE TempPte;
    ULONG_PTR Pte;

    DPRINT("MmCreateVirtualMappingUnsafe(%p, %p, %lu, %x)\n",
           Process, Address, flProtect, Page);

    ASSERT(((ULONG_PTR)Address % PAGE_SIZE) == 0);

    ProtectionMask = MiMakeProtectionMask(flProtect);
    /* Caller must have checked ! */
    ASSERT(ProtectionMask != MM_INVALID_PROTECTION);
    ASSERT(ProtectionMask != MM_NOACCESS);
    ASSERT(ProtectionMask != MM_ZERO_ACCESS);

    /* Make sure our PDE is valid, and that everything is going fine */
    if (Process == NULL)
    {
        /* We don't support this in legacy Mm for kernel mappings */
        ASSERT(ProtectionMask != MM_WRITECOPY);
        ASSERT(ProtectionMask != MM_EXECUTE_WRITECOPY);

        if (Address < MmSystemRangeStart)
        {
            DPRINT1("NULL process given for user-mode mapping at %p\n", Address);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
#if _MI_PAGING_LEVELS == 2
        if (!MiSynchronizeSystemPde(MiAddressToPde(Address)))
            MiFillSystemPageDirectory(Address, PAGE_SIZE);
#endif
    }
    else
    {
        if ((Address >= MmSystemRangeStart) || Add2Ptr(Address, PAGE_SIZE) >= MmSystemRangeStart)
        {
            DPRINT1("Process %p given for kernel-mode mapping at %p -- 1 page starting at %Ix\n",
                    Process, Address, Page);
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        /* Only for current process !!! */
        ASSERT(Process = PsGetCurrentProcess());
        MiLockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

        MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);
    }

    PointerPte = MiAddressToPte(Address);

    MI_MAKE_HARDWARE_PTE(&TempPte, PointerPte, ProtectionMask, Page);

    Pte = InterlockedExchangePte(PointerPte, TempPte.u.Long);
    /* There should not have been anything valid here */
    if (Pte != 0)
    {
        DPRINT1("Bad PTE %lx at %p for %p\n", Pte, PointerPte, Address);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /* We don't need to flush the TLB here because it only caches valid translations
     * and we're moving this PTE from invalid to valid so it can't be cached right now */

    if (Address < MmSystemRangeStart)
    {
        /* Add PDE reference */
        MiIncrementPageTableReferences(Address);
        MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PFN_NUMBER Page)
{
    ASSERT((ULONG_PTR)Address % PAGE_SIZE == 0);
    if (!MmIsPageInUse(Page))
    {
        DPRINT1("Page %lx is not in use\n", Page);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    return MmCreateVirtualMappingUnsafe(Process, Address, flProtect, Page);
}

ULONG
NTAPI
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
    PMMPTE PointerPte;
    ULONG Protect;

    if (Address >= MmSystemRangeStart)
    {
        ASSERT(Process == NULL);

#if _MI_PAGING_LEVELS == 2
        if (!MiSynchronizeSystemPde(MiAddressToPde(Address)))
#else
        if (!MiIsPdeForAddressValid(Address))
#endif
        {
            return PAGE_NOACCESS;
        }
    }
    else
    {
        ASSERT(Address < MmSystemRangeStart);
        ASSERT(Process != NULL);

        ASSERT(Process == PsGetCurrentProcess());

        MiLockProcessWorkingSetShared(Process, PsGetCurrentThread());

        if (!MiIsPageTablePresent(Address))
        {
            /* It can't be present if there is no PDE */
            MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());
            return PAGE_NOACCESS;
        }

        MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);
    }

    PointerPte = MiAddressToPte(Address);

    if (!PointerPte->u.Flush.Valid)
    {
        Protect = PAGE_NOACCESS;
    }
    else
    {
        if (PointerPte->u.Flush.CopyOnWrite)
            Protect = PAGE_WRITECOPY;
        else if (PointerPte->u.Flush.Write)
            Protect = PAGE_READWRITE;
        else
            Protect = PAGE_READONLY;
#if _MI_PAGING_LEVELS >= 3
        /* PAE & AMD64 long mode support NoExecute bit */
        if (!PointerPte->u.Flush.NoExecute)
            Protect <<= 4;
#endif
        if (PointerPte->u.Flush.CacheDisable)
            Protect |= PAGE_NOCACHE;
        if (PointerPte->u.Flush.WriteThrough)
            Protect |= PAGE_WRITETHROUGH;
    }

    if (Address < MmSystemRangeStart)
        MiUnlockProcessWorkingSetShared(Process, PsGetCurrentThread());

    return(Protect);
}

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
    ULONG ProtectionMask;
    PMMPTE PointerPte;
    MMPTE TempPte, OldPte;

    DPRINT("MmSetPageProtect(Process %p  Address %p  flProtect %x)\n",
           Process, Address, flProtect);

    ASSERT(Process != NULL);
    ASSERT(Address < MmSystemRangeStart);

    ASSERT(Process == PsGetCurrentProcess());

    ProtectionMask = MiMakeProtectionMask(flProtect);
    /* Caller must have checked ! */
    ASSERT(ProtectionMask != MM_INVALID_PROTECTION);

    MiLockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);

    /* Sanity check */
    ASSERT(PointerPte->u.Hard.Owner == 1);

    TempPte.u.Long = 0;
    TempPte.u.Hard.PageFrameNumber = PointerPte->u.Hard.PageFrameNumber;
    TempPte.u.Long |= MmProtectToPteMask[ProtectionMask];
    TempPte.u.Hard.Owner = 1;

    /* Only set valid bit if we have to */
    if ((ProtectionMask != MM_NOACCESS) && !FlagOn(ProtectionMask, MM_GUARDPAGE))
        TempPte.u.Hard.Valid = 1;

    /* Keep dirty & accessed bits */
    TempPte.u.Hard.Accessed = PointerPte->u.Hard.Accessed;
    TempPte.u.Hard.Dirty = PointerPte->u.Hard.Dirty;

    OldPte.u.Long = InterlockedExchangePte(PointerPte, TempPte.u.Long);

    // We should be able to bring a page back from PAGE_NOACCESS
    if (!OldPte.u.Hard.Valid && (FlagOn(OldPte.u.Long, 0x800) || (OldPte.u.Hard.PageFrameNumber == 0)))
    {
        DPRINT1("Invalid Pte %lx\n", OldPte.u.Long);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    if (OldPte.u.Long != TempPte.u.Long)
        KeInvalidateTlbEntry(Address);

    MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());
}

VOID
NTAPI
MmSetDirtyBit(PEPROCESS Process, PVOID Address, BOOLEAN Bit)
{
    PMMPTE PointerPte;

    DPRINT("MmSetDirtyBit(Process %p  Address %p  Bit %x)\n",
           Process, Address, Bit);

    ASSERT(Process != NULL);
    ASSERT(Address < MmSystemRangeStart);

    ASSERT(Process == PsGetCurrentProcess());

    MiLockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());

    MiMakePdeExistAndMakeValid(MiAddressToPde(Address), Process, MM_NOIRQL);

    PointerPte = MiAddressToPte(Address);
    // We shouldnl't set dirty bit on non-mapped adresses
    if (!PointerPte->u.Hard.Valid && (FlagOn(PointerPte->u.Long, 0x800) || (PointerPte->u.Hard.PageFrameNumber == 0)))
    {
        DPRINT1("Invalid Pte %lx\n", PointerPte->u.Long);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    PointerPte->u.Hard.Dirty = !!Bit;

    if (!Bit)
        KeInvalidateTlbEntry(Address);

    MiUnlockProcessWorkingSetUnsafe(Process, PsGetCurrentThread());
}

CODE_SEG("INIT")
VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    /* Nothing to do here */
}

#ifdef _M_IX86
BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID Address)
{
    PMMPDE PointerPde = MiAddressToPde(Address);
    PMMPTE PointerPte = MiAddressToPte(Address);

    if (PointerPde->u.Hard.Valid == 0)
    {
        if (!MiSynchronizeSystemPde(PointerPde))
            return FALSE;
        return PointerPte->u.Hard.Valid != 0;
    }
    return FALSE;
}
#endif

/* EOF */
