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

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitGlobalKernelPageDirectory)
#endif

#define ADDR_TO_PDE_OFFSET MiAddressToPdeOffset
#define ADDR_TO_PAGE_TABLE(v)  (((ULONG)(v)) / (1024 * PAGE_SIZE))

/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)
#define PA_BIT_GLOBAL    (8)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_GLOBAL    (1 << PA_BIT_GLOBAL)

#define IS_HYPERSPACE(v)    (((ULONG)(v) >= HYPER_SPACE && (ULONG)(v) <= HYPER_SPACE_END))

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)

#define PAGE_MASK(x)		((x)&(~0xfff))

const
ULONG
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

static BOOLEAN MmUnmapPageTable(PULONG Pt);

VOID
MiFlushTlb(PULONG Pt, PVOID Address)
{
    if ((Pt && MmUnmapPageTable(Pt)) || Address >= MmSystemRangeStart)
    {
        KeInvalidateTlbEntry(Address);
    }
}

static ULONG
ProtectToPTE(ULONG flProtect)
{
    ULONG Attributes = 0;

    if (flProtect & (PAGE_NOACCESS|PAGE_GUARD))
    {
        Attributes = 0;
    }
    else if (flProtect & PAGE_IS_WRITABLE)
    {
        Attributes = PA_PRESENT | PA_READWRITE;
    }
    else if (flProtect & (PAGE_IS_READABLE | PAGE_IS_EXECUTABLE))
    {
        Attributes = PA_PRESENT;
    }
    else
    {
        DPRINT1("Unknown main protection type.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    if (flProtect & PAGE_SYSTEM)
    {
    }
    else
    {
        Attributes = Attributes | PA_USER;
    }
    if (flProtect & PAGE_NOCACHE)
    {
        Attributes = Attributes | PA_CD;
    }
    if (flProtect & PAGE_WRITETHROUGH)
    {
        Attributes = Attributes | PA_WT;
    }
    return(Attributes);
}

/* Taken from ARM3/pagfault.c */
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
    return SystemPde.u.Hard.Valid != 0;
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
                IN PVOID Vad);

NTSTATUS
NTAPI
MiFillSystemPageDirectory(IN PVOID Base,
                          IN SIZE_T NumberOfBytes);

static PULONG
MmGetPageTableForProcess(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
    PFN_NUMBER Pfn;
    PULONG Pt;
    PMMPDE PointerPde;

    if (Address < MmSystemRangeStart)
    {
        /* We should have a process for user land addresses */
        ASSERT(Process != NULL);

        if(Process != PsGetCurrentProcess())
        {
            PMMPDE PdeBase;
            ULONG PdeOffset = MiGetPdeOffset(Address);

            /* Nobody but page fault should ask for creating the PDE,
             * Which imples that Process is the current one */
            ASSERT(Create == FALSE);

            PdeBase = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase[0]));
            if (PdeBase == NULL)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            PointerPde = PdeBase + PdeOffset;
            if (PointerPde->u.Hard.Valid == 0)
            {
                MmDeleteHyperspaceMapping(PdeBase);
                return NULL;
            }
            else
            {
                Pfn = PointerPde->u.Hard.PageFrameNumber;
            }
            MmDeleteHyperspaceMapping(PdeBase);
            Pt = MmCreateHyperspaceMapping(Pfn);
            if (Pt == NULL)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            return Pt + MiAddressToPteOffset(Address);
        }
        /* This is for our process */
        PointerPde = MiAddressToPde(Address);
        Pt = (PULONG)MiAddressToPte(Address);
        if (PointerPde->u.Hard.Valid == 0)
        {
            NTSTATUS Status;
            if (Create == FALSE)
            {
                return NULL;
            }
            ASSERT(PointerPde->u.Long == 0);

            MI_WRITE_INVALID_PTE(PointerPde, DemandZeroPde);
            Status = MiDispatchFault(TRUE,
                                     Pt,
                                     PointerPde,
                                     NULL,
                                     FALSE,
                                     PsGetCurrentProcess(),
                                     NULL,
                                     NULL);
            DBG_UNREFERENCED_LOCAL_VARIABLE(Status);
            ASSERT(KeAreAllApcsDisabled() == TRUE);
            ASSERT(PointerPde->u.Hard.Valid == 1);
        }
        return (PULONG)MiAddressToPte(Address);
    }

    /* This is for kernel land address */
    ASSERT(Process == NULL);
    PointerPde = MiAddressToPde(Address);
    Pt = (PULONG)MiAddressToPte(Address);
    if (PointerPde->u.Hard.Valid == 0)
    {
        /* Let ARM3 synchronize the PDE */
        if(!MiSynchronizeSystemPde(PointerPde))
        {
            /* PDE (still) not valid, let ARM3 allocate one if asked */
            if(Create == FALSE)
                return NULL;
            MiFillSystemPageDirectory(Address, PAGE_SIZE);
        }
    }
    return Pt;
}

static BOOLEAN MmUnmapPageTable(PULONG Pt)
{
    if (!IS_HYPERSPACE(Pt))
    {
        return TRUE;
    }

    if (Pt)
    {
        MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pt));
    }
    return FALSE;
}

static ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
    ULONG Pte;
    PULONG Pt;

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt)
    {
        Pte = *Pt;
        MmUnmapPageTable(Pt);
        return Pte;
    }
    return 0;
}

PFN_NUMBER
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
    ULONG Entry;
    Entry = MmGetPageEntryForProcess(Process, Address);
    if (!(Entry & PA_PRESENT))
    {
        return 0;
    }
    return(PTE_TO_PFN(Entry));
}

VOID
NTAPI
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address,
                       BOOLEAN* WasDirty, PPFN_NUMBER Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    BOOLEAN WasValid = FALSE;
    PFN_NUMBER Pfn;
    ULONG Pte;
    PULONG Pt;

    DPRINT("MmDeleteVirtualMapping(%p, %p, %p, %p)\n",
           Process, Address, WasDirty, Page);

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);

    if (Pt == NULL)
    {
        if (WasDirty != NULL)
        {
            *WasDirty = FALSE;
        }
        if (Page != NULL)
        {
            *Page = 0;
        }
        return;
    }

    /*
     * Atomically set the entry to zero and get the old value.
     */
    Pte = InterlockedExchangePte(Pt, 0);

    /* We count a mapping as valid if it's a present page, or it's a nonzero pfn with
     * the swap bit unset, indicating a valid page protected to PAGE_NOACCESS. */
    WasValid = (Pte & PA_PRESENT) || ((Pte >> PAGE_SHIFT) && !(Pte & 0x800));
    if (WasValid)
    {
        /* Flush the TLB since we transitioned this PTE
         * from valid to invalid so any stale translations
         * are removed from the cache */
        MiFlushTlb(Pt, Address);

		if (Address < MmSystemRangeStart)
		{
			/* Remove PDE reference */
			Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)]--;
			ASSERT(Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)] < PTE_COUNT);
		}

        Pfn = PTE_TO_PFN(Pte);
    }
    else
    {
        MmUnmapPageTable(Pt);
        Pfn = 0;
    }

    /*
     * Return some information to the caller
     */
    if (WasDirty != NULL)
    {
        *WasDirty = ((Pte & PA_DIRTY) && (Pte & PA_PRESENT)) ? TRUE : FALSE;
    }
    if (Page != NULL)
    {
        *Page = Pfn;
    }
}

VOID
NTAPI
MmGetPageFileMapping(PEPROCESS Process, PVOID Address,
                     SWAPENTRY* SwapEntry)
/*
 * FUNCTION: Get a page file mapping
 */
{
    ULONG Entry = MmGetPageEntryForProcess(Process, Address);
    *SwapEntry = Entry >> 1;
}

VOID
NTAPI
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    ULONG Pte;
    PULONG Pt;

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);

    if (Pt == NULL)
    {
        *SwapEntry = 0;
        return;
    }

    /*
     * Atomically set the entry to zero and get the old value.
     */
    Pte = InterlockedExchangePte(Pt, 0);

	if (Address < MmSystemRangeStart)
	{
		/* Remove PDE reference */
		Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)]--;
		ASSERT(Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)] < PTE_COUNT);
	}

    /* We don't need to flush here because page file entries
     * are invalid translations, so the processor won't cache them */
    MmUnmapPageTable(Pt);

    if ((Pte & PA_PRESENT) || !(Pte & 0x800))
    {
        DPRINT1("Pte %x (want not 1 and 0x800)\n", Pte);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /*
     * Return some information to the caller
     */
    *SwapEntry = Pte >> 1;
}

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID Address)
{
    PMMPDE PointerPde = MiAddressToPde(Address);
    PMMPTE PointerPte = MiAddressToPte(Address);

    if (PointerPde->u.Hard.Valid == 0)
    {
        if(!MiSynchronizeSystemPde(PointerPde))
            return FALSE;
        return PointerPte->u.Hard.Valid != 0;
    }
    return FALSE;
}

BOOLEAN
NTAPI
MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
    return MmGetPageEntryForProcess(Process, Address) & PA_DIRTY ? TRUE : FALSE;
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
    PULONG Pt;
    ULONG Pte;

    if (Address < MmSystemRangeStart && Process == NULL)
    {
        DPRINT1("MmSetCleanPage is called for user space without a process.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    do
    {
        Pte = *Pt;
    } while (Pte != InterlockedCompareExchangePte(Pt, Pte & ~PA_DIRTY, Pte));

    if (!(Pte & PA_PRESENT))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    else if (Pte & PA_DIRTY)
    {
        MiFlushTlb(Pt, Address);
    }
    else
    {
        MmUnmapPageTable(Pt);
    }
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
    PULONG Pt;
    ULONG Pte;

    if (Address < MmSystemRangeStart && Process == NULL)
    {
        DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    do
    {
        Pte = *Pt;
    } while (Pte != InterlockedCompareExchangePte(Pt, Pte | PA_DIRTY, Pte));

    if (!(Pte & PA_PRESENT))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    else
    {
        /* The processor will never clear this bit itself, therefore
         * we do not need to flush the TLB here when setting it */
        MmUnmapPageTable(Pt);
    }
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
    return MmGetPageEntryForProcess(Process, Address) & PA_PRESENT;
}

BOOLEAN
NTAPI
MmIsDisabledPage(PEPROCESS Process, PVOID Address)
{
    ULONG_PTR Entry = MmGetPageEntryForProcess(Process, Address);
    return !(Entry & PA_PRESENT) && !(Entry & 0x800) && (Entry >> PAGE_SHIFT);
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    ULONG Entry;
    Entry = MmGetPageEntryForProcess(Process, Address);
    return !(Entry & PA_PRESENT) && (Entry & 0x800);
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
    PULONG Pt;
    ULONG Pte;

    if (Process == NULL && Address < MmSystemRangeStart)
    {
        DPRINT1("No process\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    if (Process != NULL && Address >= MmSystemRangeStart)
    {
        DPRINT1("Setting kernel address with process context\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    if (SwapEntry & (1 << 31))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        /* Nobody should page out an address that hasn't even been mapped */
        /* But we might place a wait entry first, requiring the page table */
        if (SwapEntry != MM_WAIT_ENTRY)
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        Pt = MmGetPageTableForProcess(Process, Address, TRUE);
    }
    Pte = InterlockedExchangePte(Pt, SwapEntry << 1);
    if (Pte != 0)
    {
        KeBugCheckEx(MEMORY_MANAGEMENT, SwapEntry, (ULONG_PTR)Process, (ULONG_PTR)Address, 0);
    }

	if (Address < MmSystemRangeStart)
	{
		/* Add PDE reference */
		Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)]++;
		ASSERT(Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)] <= PTE_COUNT);
	}

    /* We don't need to flush the TLB here because it
     * only caches valid translations and a zero PTE
     * is not a valid translation */
    MmUnmapPageTable(Pt);

    return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_NUMBER Pages,
                             ULONG PageCount)
{
    ULONG Attributes;
    PVOID Addr;
    ULONG i;
    ULONG oldPdeOffset, PdeOffset;
    PULONG Pt = NULL;
    ULONG Pte;
    DPRINT("MmCreateVirtualMappingUnsafe(%p, %p, %lu, %p (%x), %lu)\n",
           Process, Address, flProtect, Pages, *Pages, PageCount);

    ASSERT(((ULONG_PTR)Address % PAGE_SIZE) == 0);

    if (Process == NULL)
    {
        if (Address < MmSystemRangeStart)
        {
            DPRINT1("No process\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        if (PageCount > 0x10000 ||
            (ULONG_PTR) Address / PAGE_SIZE + PageCount > 0x100000)
        {
            DPRINT1("Page count too large\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }
    else
    {
        if (Address >= MmSystemRangeStart)
        {
            DPRINT1("Setting kernel address with process context\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        if (PageCount > (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE ||
            (ULONG_PTR) Address / PAGE_SIZE + PageCount >
            (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE)
        {
            DPRINT1("Page Count too large\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

    Attributes = ProtectToPTE(flProtect);
    Attributes &= 0xfff;
    if (Address >= MmSystemRangeStart)
    {
        Attributes &= ~PA_USER;
    }
    else
    {
        Attributes |= PA_USER;
    }

    Addr = Address;
    /* MmGetPageTableForProcess should be called on the first run, so
     * let this trigger it */
    oldPdeOffset = ADDR_TO_PDE_OFFSET(Addr) + 1;
    for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
    {
        if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
        {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%p with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        PdeOffset = ADDR_TO_PDE_OFFSET(Addr);
        if (oldPdeOffset != PdeOffset)
        {
            if(Pt) MmUnmapPageTable(Pt);
            Pt = MmGetPageTableForProcess(Process, Addr, TRUE);
            if (Pt == NULL)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
        }
        else
        {
            Pt++;
        }
        oldPdeOffset = PdeOffset;

        Pte = InterlockedExchangePte(Pt, PFN_TO_PTE(Pages[i]) | Attributes);

        /* There should not be anything valid here */
        if (Pte != 0)
        {
            DPRINT1("Bad PTE %lx\n", Pte);
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        /* We don't need to flush the TLB here because it only caches valid translations
         * and we're moving this PTE from invalid to valid so it can't be cached right now */

		if (Addr < MmSystemRangeStart)
		{
			/* Add PDE reference */
			Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Addr)]++;
			ASSERT(Process->Vm.VmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Addr)] <= PTE_COUNT);
		}
    }

    ASSERT(Addr > Address);
    MmUnmapPageTable(Pt);

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PPFN_NUMBER Pages,
                       ULONG PageCount)
{
    ULONG i;

    ASSERT((ULONG_PTR)Address % PAGE_SIZE == 0);
    for (i = 0; i < PageCount; i++)
    {
        if (!MmIsPageInUse(Pages[i]))
        {
            DPRINT1("Page at address %x not in use\n", PFN_TO_PTE(Pages[i]));
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

    return(MmCreateVirtualMappingUnsafe(Process,
                                        Address,
                                        flProtect,
                                        Pages,
                                        PageCount));
}

ULONG
NTAPI
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
    ULONG Entry;
    ULONG Protect;

    Entry = MmGetPageEntryForProcess(Process, Address);


    if (!(Entry & PA_PRESENT))
    {
        Protect = PAGE_NOACCESS;
    }
    else
    {
        if (Entry & PA_READWRITE)
        {
            Protect = PAGE_READWRITE;
        }
        else
        {
            Protect = PAGE_EXECUTE_READ;
        }
        if (Entry & PA_CD)
        {
            Protect |= PAGE_NOCACHE;
        }
        if (Entry & PA_WT)
        {
            Protect |= PAGE_WRITETHROUGH;
        }
        if (!(Entry & PA_USER))
        {
            Protect |= PAGE_SYSTEM;
        }

    }
    return(Protect);
}

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
    ULONG Attributes = 0;
    PULONG Pt;
    ULONG Pte;

    DPRINT("MmSetPageProtect(Process %p  Address %p  flProtect %x)\n",
           Process, Address, flProtect);

    Attributes = ProtectToPTE(flProtect);

    Attributes &= 0xfff;
    if (Address >= MmSystemRangeStart)
    {
        Attributes &= ~PA_USER;
    }
    else
    {
        Attributes |= PA_USER;
    }

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Pte = InterlockedExchangePte(Pt, PAGE_MASK(*Pt) | Attributes | (*Pt & (PA_ACCESSED|PA_DIRTY)));

    // We should be able to bring a page back from PAGE_NOACCESS
    if ((Pte & 0x800) || !(Pte >> PAGE_SHIFT))
    {
        DPRINT1("Invalid Pte %lx\n", Pte);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    if((Pte & Attributes) != Attributes)
        MiFlushTlb(Pt, Address);
    else
        MmUnmapPageTable(Pt);
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    /* Nothing to do here */
}

/* EOF */
