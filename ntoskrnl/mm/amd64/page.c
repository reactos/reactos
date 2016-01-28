/*
 * COPYRIGHT:       GPL, See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/page.c
 * PURPOSE:         Low level memory managment manipulation
 *
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  ReactOS Portable Systems Group
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <mm/ARM3/miarm.h>

#undef InterlockedExchangePte
#define InterlockedExchangePte(pte1, pte2) \
    InterlockedExchange64((LONG64*)&pte1->u.Long, pte2.u.Long)

#define PAGE_EXECUTE_ANY (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)
#define PAGE_WRITE_ANY (PAGE_EXECUTE_READWRITE|PAGE_READWRITE|PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY)
#define PAGE_WRITECOPY_ANY (PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY)

extern MMPTE HyperTemplatePte;

/* GLOBALS *****************************************************************/

const
ULONG64
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

/* PRIVATE FUNCTIONS *******************************************************/

BOOLEAN
FORCEINLINE
MiIsHyperspaceAddress(PVOID Address)
{
    return ((ULONG64)Address >= HYPER_SPACE &&
            (ULONG64)Address <= HYPER_SPACE_END);
}

VOID
MiFlushTlb(PMMPTE Pte, PVOID Address)
{
    if (MiIsHyperspaceAddress(Pte))
    {
        MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pte));
    }
    else
    {
        __invlpg(Address);
    }
}

static
PMMPTE
MiGetPteForProcess(
    PEPROCESS Process,
    PVOID Address,
    BOOLEAN Create)
{
    MMPTE TmplPte, *Pte;

    /* Check if we need hypersapce mapping */
    if (Address < MmSystemRangeStart &&
        Process && Process != PsGetCurrentProcess())
    {
        UNIMPLEMENTED;
        __debugbreak();
        return NULL;
    }
    else if (Create)
    {
        KIRQL OldIrql;
        TmplPte.u.Long = 0;
        TmplPte.u.Flush.Valid = 1;
        TmplPte.u.Flush.Write = 1;

        /* All page table levels of user pages are user owned */
        TmplPte.u.Flush.Owner = (Address < MmHighestUserAddress) ? 1 : 0;

        /* Lock the PFN database */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* Get the PXE */
        Pte = MiAddressToPxe(Address);
        if (!Pte->u.Hard.Valid)
        {
            TmplPte.u.Hard.PageFrameNumber = MiRemoveZeroPage(0);
            MI_WRITE_VALID_PTE(Pte, TmplPte);
        }

        /* Get the PPE */
        Pte = MiAddressToPpe(Address);
        if (!Pte->u.Hard.Valid)
        {
            TmplPte.u.Hard.PageFrameNumber = MiRemoveZeroPage(1);
            MI_WRITE_VALID_PTE(Pte, TmplPte);
        }

        /* Get the PDE */
        Pte = MiAddressToPde(Address);
        if (!Pte->u.Hard.Valid)
        {
            TmplPte.u.Hard.PageFrameNumber = MiRemoveZeroPage(2);
            MI_WRITE_VALID_PTE(Pte, TmplPte);
        }

        /* Unlock PFN database */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    }
    else
    {
        /* Get the PXE */
        Pte = MiAddressToPxe(Address);
        if (!Pte->u.Hard.Valid)
            return NULL;

        /* Get the PPE */
        Pte = MiAddressToPpe(Address);
        if (!Pte->u.Hard.Valid)
            return NULL;

        /* Get the PDE */
        Pte = MiAddressToPde(Address);
        if (!Pte->u.Hard.Valid)
            return NULL;
    }

    return MiAddressToPte(Address);
}

static
ULONG64
MiGetPteValueForProcess(
    PEPROCESS Process,
    PVOID Address)
{
    PMMPTE Pte;
    ULONG64 PteValue;

    Pte = MiGetPteForProcess(Process, Address, FALSE);
    PteValue = Pte ? Pte->u.Long : 0;

    if (MiIsHyperspaceAddress(Pte))
        MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pte));

    return PteValue;
}

ULONG
NTAPI
MiGetPteProtection(MMPTE Pte)
{
    ULONG Protect;

    if (!Pte.u.Flush.Valid)
    {
        Protect = PAGE_NOACCESS;
    }
    else if (Pte.u.Flush.NoExecute)
    {
        if (Pte.u.Flush.CopyOnWrite)
            Protect = PAGE_WRITECOPY;
        else if (Pte.u.Flush.Write)
            Protect = PAGE_READWRITE;
        else
            Protect = PAGE_READONLY;
    }
    else
    {
        if (Pte.u.Flush.CopyOnWrite)
            Protect = PAGE_EXECUTE_WRITECOPY;
        else if (Pte.u.Flush.Write)
            Protect = PAGE_EXECUTE_READWRITE;
        else
            Protect = PAGE_EXECUTE_READ;
    }

    if (Pte.u.Flush.CacheDisable)
        Protect |= PAGE_NOCACHE;

    if (Pte.u.Flush.WriteThrough)
        Protect |= PAGE_WRITETHROUGH;

    // PAGE_GUARD ?
    return Protect;
}

VOID
NTAPI
MiSetPteProtection(PMMPTE Pte, ULONG Protection)
{
    Pte->u.Flush.CopyOnWrite = (Protection & PAGE_WRITECOPY_ANY) ? 1 : 0;
    Pte->u.Flush.Write = (Protection & PAGE_WRITE_ANY) ? 1 : 0;
    Pte->u.Flush.CacheDisable = (Protection & PAGE_NOCACHE) ? 1 : 0;
    Pte->u.Flush.WriteThrough = (Protection & PAGE_WRITETHROUGH) ? 1 : 0;

    // FIXME: This doesn't work. Why?
//    Pte->u.Flush.NoExecute = (Protection & PAGE_EXECUTE_ANY) ? 0 : 1;
}

/* FUNCTIONS ***************************************************************/

PFN_NUMBER
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return Pte.u.Hard.Valid ? Pte.u.Hard.PageFrameNumber : 0;
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return (BOOLEAN)Pte.u.Hard.Valid;
}

BOOLEAN
NTAPI
MmIsDisabledPage(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    __debugbreak(); // FIXME
    return !Pte.u.Hard.Valid && !(Pte.u.Long & 0x800) && Pte.u.Hard.PageFrameNumber;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return Pte.u.Hard.Valid && Pte.u.Soft.Transition;
}

static PMMPTE
MmGetPageTableForProcess(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
    __debugbreak();
    return 0;
}

BOOLEAN MmUnmapPageTable(PMMPTE Pt)
{
    ASSERT(FALSE);
    return 0;
}

static ULONG64 MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte, *PointerPte;

    PointerPte = MmGetPageTableForProcess(Process, Address, FALSE);
    if (PointerPte)
    {
        Pte = *PointerPte;
        MmUnmapPageTable(PointerPte);
        return Pte.u.Long;
    }
    return 0;
}

VOID
NTAPI
MmGetPageFileMapping(
    PEPROCESS Process,
    PVOID Address,
    SWAPENTRY* SwapEntry)
{
	ULONG64 Entry = MmGetPageEntryForProcess(Process, Address);
	*SwapEntry = Entry >> 1;
}

BOOLEAN
NTAPI
MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return Pte.u.Hard.Valid && Pte.u.Hard.Dirty;
}

ULONG
NTAPI
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;

    Pte.u.Long = MiGetPteValueForProcess(Process, Address);

    return MiGetPteProtection(Pte);
}

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
    PMMPTE Pte;
    MMPTE NewPte;

    Pte = MiGetPteForProcess(Process, Address, FALSE);
    ASSERT(Pte != NULL);

    NewPte = *Pte;

    MiSetPteProtection(&NewPte, flProtect);

    InterlockedExchangePte(Pte, NewPte);

    MiFlushTlb(Pte, Address);
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
    PMMPTE Pte;

    Pte = MiGetPteForProcess(Process, Address, FALSE);
    if (!Pte)
    {
        KeBugCheckEx(MEMORY_MANAGEMENT, 0x1234, (ULONG64)Address, 0, 0);
    }

    /* Ckear the dirty bit */
    if (InterlockedBitTestAndReset64((PVOID)Pte, 6))
    {
        if (!MiIsHyperspaceAddress(Pte))
            __invlpg(Address);
    }

    MiFlushTlb(Pte, Address);
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
    PMMPTE Pte;

    Pte = MiGetPteForProcess(Process, Address, FALSE);
    if (!Pte)
    {
        KeBugCheckEx(MEMORY_MANAGEMENT, 0x1234, (ULONG64)Address, 0, 0);
    }

    /* Ckear the dirty bit */
    if (InterlockedBitTestAndSet64((PVOID)Pte, 6))
    {
        if (!MiIsHyperspaceAddress(Pte))
            __invlpg(Address);
    }

    MiFlushTlb(Pte, Address);
}

VOID
NTAPI
MmDeleteVirtualMapping(
    PEPROCESS Process,
    PVOID Address,
    BOOLEAN* WasDirty,
    PPFN_NUMBER Page)
{
    PFN_NUMBER Pfn;
    PMMPTE Pte;
    MMPTE OldPte;

    Pte = MiGetPteForProcess(Process, Address, FALSE);

    if (Pte)
    {
        /* Atomically set the entry to zero and get the old value. */
        OldPte.u.Long = InterlockedExchange64((LONG64*)&Pte->u.Long, 0);

        if (OldPte.u.Hard.Valid)
        {
            Pfn = OldPte.u.Hard.PageFrameNumber;
        }
        else
            Pfn = 0;
    }
    else
    {
        OldPte.u.Long = 0;
        Pfn = 0;
    }

    /* Return information to the caller */
    if (WasDirty)
        *WasDirty = (BOOLEAN)OldPte.u.Hard.Dirty;;

    if (Page)
        *Page = Pfn;

    MiFlushTlb(Pte, Address);
}

VOID
NTAPI
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(
    PEPROCESS Process,
    PVOID Address,
    ULONG PageProtection,
    PPFN_NUMBER Pages,
    ULONG PageCount)
{
    ULONG i;
    MMPTE TmplPte, *Pte;

    ASSERT((ULONG_PTR)Address % PAGE_SIZE == 0);

    /* Check if the range is valid */
    if ((Process == NULL && Address < MmSystemRangeStart) ||
        (Process != NULL && Address > MmHighestUserAddress))
    {
        DPRINT1("Address 0x%p is invalid for process %p\n", Address, Process);
        ASSERT(FALSE);
    }

    TmplPte.u.Long = 0;
    TmplPte.u.Hard.Valid = 1;
    MiSetPteProtection(&TmplPte, PageProtection);

    TmplPte.u.Flush.Owner = (Address < MmHighestUserAddress) ? 1 : 0;

//__debugbreak();

    for (i = 0; i < PageCount; i++)
    {
        TmplPte.u.Hard.PageFrameNumber = Pages[i];

        Pte = MiGetPteForProcess(Process, Address, TRUE);

DPRINT("MmCreateVirtualMappingUnsafe, Address=%p, TmplPte=%p, Pte=%p\n",
        Address, TmplPte.u.Long, Pte);

        if (InterlockedExchangePte(Pte, TmplPte))
        {
            KeInvalidateTlbEntry(Address);
        }

        if (MiIsHyperspaceAddress(Pte))
            MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pte));

        Address = (PVOID)((ULONG64)Address + PAGE_SIZE);
    }


    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG Protect,
                       PPFN_NUMBER Pages,
                       ULONG PageCount)
{
    ULONG i;

    ASSERT((ULONG_PTR)Address % PAGE_SIZE == 0);

    for (i = 0; i < PageCount; i++)
    {
        if (!MmIsPageInUse(Pages[i]))
        {
            DPRINT1("Page %x not in use\n", Pages[i]);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

    return MmCreateVirtualMappingUnsafe(Process, Address, Protect, Pages, PageCount);
}

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            OUT PULONG_PTR DirectoryTableBase)
{
    KIRQL OldIrql;
    PFN_NUMBER TableBasePfn, HyperPfn, HyperPdPfn, HyperPtPfn, WorkingSetPfn;
    PMMPTE SystemPte;
    MMPTE TempPte, PdePte;
    ULONG TableIndex;
    PMMPTE PageTablePointer;

    /* Make sure we don't already have a page directory setup */
    ASSERT(Process->Pcb.DirectoryTableBase[0] == 0);
    ASSERT(Process->Pcb.DirectoryTableBase[1] == 0);
    ASSERT(Process->WorkingSetPage == 0);

    /* Choose a process color */
    Process->NextPageColor = (USHORT)RtlRandom(&MmProcessColorSeed);

    /* Setup the hyperspace lock */
    KeInitializeSpinLock(&Process->HyperSpaceLock);

    /* Lock PFN database */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Get a page for the table base and one for hyper space. The PFNs for
       these pages will be initialized in MmInitializeProcessAddressSpace,
       when we are already attached to the process. */
    TableBasePfn = MiRemoveAnyPage(MI_GET_NEXT_PROCESS_COLOR(Process));
    HyperPfn = MiRemoveAnyPage(MI_GET_NEXT_PROCESS_COLOR(Process));
    HyperPdPfn = MiRemoveAnyPage(MI_GET_NEXT_PROCESS_COLOR(Process));
    HyperPtPfn = MiRemoveAnyPage(MI_GET_NEXT_PROCESS_COLOR(Process));
    WorkingSetPfn = MiRemoveAnyPage(MI_GET_NEXT_PROCESS_COLOR(Process));

    /* Release PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Zero pages */ /// FIXME:
    MiZeroPhysicalPage(HyperPfn);
    MiZeroPhysicalPage(WorkingSetPfn);

    /* Set the base directory pointers */
    Process->WorkingSetPage = WorkingSetPfn;
    DirectoryTableBase[0] = TableBasePfn << PAGE_SHIFT;
    DirectoryTableBase[1] = HyperPfn << PAGE_SHIFT;

    /* Get a PTE to map the page directory */
    SystemPte = MiReserveSystemPtes(1, SystemPteSpace);
    ASSERT(SystemPte != NULL);

    /* Get its address */
    PageTablePointer = MiPteToAddress(SystemPte);

    /* Build the PTE for the page directory and map it */
    PdePte = ValidKernelPte;
    PdePte.u.Hard.PageFrameNumber = TableBasePfn;
    *SystemPte = PdePte;

/// architecture specific
    //MiInitializePageDirectoryForProcess(

    /* Copy the kernel mappings and zero out the rest */
    TableIndex = PXE_PER_PAGE / 2;
    RtlZeroMemory(PageTablePointer, TableIndex * sizeof(MMPTE));
    RtlCopyMemory(PageTablePointer + TableIndex,
                  MiAddressToPxe(0) + TableIndex,
                  PAGE_SIZE - TableIndex * sizeof(MMPTE));

    /* Sanity check */
    ASSERT(MiAddressToPxi(MmHyperSpaceEnd) >= TableIndex);

    /* Setup a PTE for the page directory mappings */
    TempPte = ValidKernelPte;

    /* Update the self mapping of the PML4 */
    TableIndex = MiAddressToPxi((PVOID)PXE_SELFMAP);
    TempPte.u.Hard.PageFrameNumber = TableBasePfn;
    PageTablePointer[TableIndex] = TempPte;

    /* Write the PML4 entry for hyperspace */
    TableIndex = MiAddressToPxi((PVOID)HYPER_SPACE);
    TempPte.u.Hard.PageFrameNumber = HyperPfn;
    PageTablePointer[TableIndex] = TempPte;

    /* Map the hyperspace PDPT to the system PTE */
    PdePte.u.Hard.PageFrameNumber = HyperPfn;
    *SystemPte = PdePte;
    __invlpg(PageTablePointer);

    /* Write the hyperspace entry for the first PD */
    TempPte.u.Hard.PageFrameNumber = HyperPdPfn;
    PageTablePointer[0] = TempPte;

    /* Map the hyperspace PD to the system PTE */
    PdePte.u.Hard.PageFrameNumber = HyperPdPfn;
    *SystemPte = PdePte;
    __invlpg(PageTablePointer);

    /* Write the hyperspace entry for the first PT */
    TempPte.u.Hard.PageFrameNumber = HyperPtPfn;
    PageTablePointer[0] = TempPte;

    /* Map the hyperspace PT to the system PTE */
    PdePte.u.Hard.PageFrameNumber = HyperPtPfn;
    *SystemPte = PdePte;
    __invlpg(PageTablePointer);

    /* Write the hyperspace PTE for the working set list index */
    TempPte.u.Hard.PageFrameNumber = WorkingSetPfn;
    TableIndex = MiAddressToPti(MmWorkingSetList);
    PageTablePointer[TableIndex] = TempPte;

/// end architecture specific

    /* Release the system PTE */
    MiReleaseSystemPtes(SystemPte, 1, SystemPteSpace);

    /* Switch to phase 1 initialization */
    ASSERT(Process->AddressSpaceInitialized == 0);
    Process->AddressSpaceInitialized = 1;

    return TRUE;
}

/* EOF */
