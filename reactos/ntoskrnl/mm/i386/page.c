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
#include "../ARM3/miarm.h"

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitGlobalKernelPageDirectory)
#pragma alloc_text(INIT, MiInitPageDirectoryMap)
#endif


/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)
#define PA_BIT_GLOBAL	 (8)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_GLOBAL    (1 << PA_BIT_GLOBAL)

#define HYPERSPACE		(0xc0400000)
#define IS_HYPERSPACE(v)	(((ULONG)(v) >= HYPERSPACE && (ULONG)(v) < HYPERSPACE + 0x400000))

ULONG MmGlobalKernelPageDirectory[1024];

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)

#if defined(__GNUC__)
#define PTE_TO_PAGE(X) ((LARGE_INTEGER)(LONGLONG)(PAGE_MASK(X)))
#else
__inline LARGE_INTEGER PTE_TO_PAGE(ULONG npage)
{
    LARGE_INTEGER dummy;
    dummy.QuadPart = (LONGLONG)(PAGE_MASK(npage));
    return dummy;
}
#endif

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

BOOLEAN MmUnmapPageTable(PULONG Pt);

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

static PULONG
MmGetPageTableForProcess(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
    ULONG PdeOffset = ADDR_TO_PDE_OFFSET(Address);
    NTSTATUS Status;
    PFN_NUMBER Pfn;
    ULONG Entry;
    PULONG Pt, PageDir;
    
    if (Address < MmSystemRangeStart)
    {
        /* We should have a process for user land addresses */
        ASSERT(Process != NULL);
        
        if(Process != PsGetCurrentProcess())
        {
            PageDir = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase[0]));
            if (PageDir == NULL)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            if (0 == InterlockedCompareExchangePte(&PageDir[PdeOffset], 0, 0))
            {
                if (Create == FALSE)
                {
                    MmDeleteHyperspaceMapping(PageDir);
                    return NULL;
                }
                MI_SET_USAGE(MI_USAGE_LEGACY_PAGE_DIRECTORY);
                if (Process) MI_SET_PROCESS2(Process->ImageFileName);
                if (!Process) MI_SET_PROCESS2("Kernel Legacy");
                Status = MmRequestPageMemoryConsumer(MC_SYSTEM, FALSE, &Pfn);
                if (!NT_SUCCESS(Status) || Pfn == 0)
                {
                    KeBugCheck(MEMORY_MANAGEMENT);
                }
                Entry = InterlockedCompareExchangePte(&PageDir[PdeOffset], PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
                if (Entry != 0)
                {
                    MmReleasePageMemoryConsumer(MC_SYSTEM, Pfn);
                    Pfn = PTE_TO_PFN(Entry);
                }
            }
            else
            {
                Pfn = PTE_TO_PFN(PageDir[PdeOffset]);
            }
            MmDeleteHyperspaceMapping(PageDir);
            Pt = MmCreateHyperspaceMapping(Pfn);
            if (Pt == NULL)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            return Pt + MiAddressToPteOffset(Address);
        }
        /* This is for our process */
        PageDir = (PULONG)MiAddressToPde(Address);
        if (0 == InterlockedCompareExchangePte(PageDir, 0, 0))
        {
            if (Create == FALSE)
            {
                return NULL;
            }
            MI_SET_USAGE(MI_USAGE_LEGACY_PAGE_DIRECTORY);
            if (Process) MI_SET_PROCESS2(Process->ImageFileName);
            if (!Process) MI_SET_PROCESS2("Kernel Legacy");
            Status = MmRequestPageMemoryConsumer(MC_SYSTEM, FALSE, &Pfn);
            if (!NT_SUCCESS(Status) || Pfn == 0)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            Entry = InterlockedCompareExchangePte(PageDir, PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
            if (Entry != 0)
            {
                MmReleasePageMemoryConsumer(MC_SYSTEM, Pfn);
            }
        }
        return (PULONG)MiAddressToPte(Address);
    }
    
    /* This is for kernel land address */
    PageDir = (PULONG)MiAddressToPde(Address);
    if (0 == InterlockedCompareExchangePte(PageDir, 0, 0))
    {
        if (0 == InterlockedCompareExchangePte(&MmGlobalKernelPageDirectory[PdeOffset], 0, 0))
        {
            if (Create == FALSE)
            {
                return NULL;
            }
            MI_SET_USAGE(MI_USAGE_LEGACY_PAGE_DIRECTORY);
            if (Process) MI_SET_PROCESS2(Process->ImageFileName);
            if (!Process) MI_SET_PROCESS2("Kernel Legacy");
            Status = MmRequestPageMemoryConsumer(MC_SYSTEM, FALSE, &Pfn);
            if (!NT_SUCCESS(Status) || Pfn == 0)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE;
            if(0 != InterlockedCompareExchangePte(&MmGlobalKernelPageDirectory[PdeOffset], Entry, 0))
            {
                MmReleasePageMemoryConsumer(MC_SYSTEM, Pfn);
            }
            InterlockedExchangePte(PageDir, MmGlobalKernelPageDirectory[PdeOffset]);
            RtlZeroMemory(MiPteToAddress(PageDir), PAGE_SIZE);
            return (PULONG)MiAddressToPte(Address);
        }
        InterlockedExchangePte(PageDir, MmGlobalKernelPageDirectory[PdeOffset]);
    }
    return (PULONG)MiAddressToPte(Address);
}

BOOLEAN MmUnmapPageTable(PULONG Pt)
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
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN* WasDirty, PPFN_NUMBER Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    BOOLEAN WasValid;
    ULONG Pte;
    PULONG Pt;
    
    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    /*
     * Atomically disable the present bit and get the old value.
     */
    do
    {
        Pte = *Pt;
    } while (Pte != InterlockedCompareExchangePte(Pt, Pte & ~PA_PRESENT, Pte));
    
    if(Pte & PA_PRESENT)
        MiFlushTlb(Pt, Address);
    else
        MmUnmapPageTable(Pt);
    
    WasValid = (PAGE_MASK(Pte) != 0);
    if (!WasValid)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    
    /*
     * Return some information to the caller
     */
    if (WasDirty != NULL)
    {
        *WasDirty = Pte & PA_DIRTY;
    }
    if (Page != NULL)
    {
        *Page = PTE_TO_PFN(Pte);
    }
}

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address)
{
    PULONG Pt;
    
    Pt = MmGetPageTableForProcess(NULL, Address, FALSE);
    if (Pt && *Pt)
    {
        /*
         * Set the entry to zero
         */
        InterlockedExchangePte(Pt, 0);
        MiFlushTlb(Pt, Address);
    }
}

VOID
NTAPI
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN FreePage,
                       BOOLEAN* WasDirty, PPFN_NUMBER Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    BOOLEAN WasValid = FALSE;
    PFN_NUMBER Pfn;
    ULONG Pte;
    PULONG Pt;
    
    DPRINT("MmDeleteVirtualMapping(%x, %x, %d, %x, %x)\n",
           Process, Address, FreePage, WasDirty, Page);
    
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
    
    
    WasValid = (PAGE_MASK(Pte) != 0);
    if (WasValid)
    {
        Pfn = PTE_TO_PFN(Pte);
        MiFlushTlb(Pt, Address);
    }
    else
    {
        Pfn = 0;
        MmUnmapPageTable(Pt);
    }
    
    if (FreePage && WasValid)
    {
        MmReleasePageMemoryConsumer(MC_SYSTEM, Pfn);
    }
    
    /*
     * Return some information to the caller
     */
    if (WasDirty != NULL)
    {
        *WasDirty = Pte & PA_DIRTY ? TRUE : FALSE;
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
    
    //MiFlushTlb(Pt, Address);
    MmUnmapPageTable(Pt);
    
    if(!(Pte & 0x800))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    
    /*
     * Return some information to the caller
     */
    *SwapEntry = Pte >> 1;
}

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
    PULONG Pt, Pde;
    Pde = (PULONG)MiAddressToPde(PAddress);
    if (*Pde == 0)
    {
        Pt = MmGetPageTableForProcess(NULL, PAddress, FALSE);
        if (Pt != NULL)
        {
            return TRUE;
        }
    }
    return(FALSE);
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
    
    if (Pte & PA_DIRTY)
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
    if (!(Pte & PA_DIRTY))
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
MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
    PULONG Pt;
    ULONG Pte;
    
    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        //HACK to get DPH working, waiting for MM rewrite :-/
        //KeBugCheck(MEMORY_MANAGEMENT);
        return;
    }
    
    /* Do not mark a 0 page as present */
    if(0 == InterlockedCompareExchangePte(Pt, 0, 0))
        return;
    
    do
    {
        Pte = *Pt;
    } while (Pte != InterlockedCompareExchangePte(Pt, Pte | PA_PRESENT, Pte));
    if (!(Pte & PA_PRESENT))
    {
        MiFlushTlb(Pt, Address);
    }
    else
    {
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
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    ULONG Entry;
    Entry = MmGetPageEntryForProcess(Process, Address);
    return !(Entry & PA_PRESENT) && (Entry & 0x800) && Entry != 0;
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
    
    Pt = MmGetPageTableForProcess(Process, Address, TRUE);
    if (Pt == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Pte = InterlockedExchangePte(Pt, SwapEntry << 1);
    if(PAGE_MASK(Pte))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    //MiFlushTlb(Pt, Address);
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
    DPRINT("MmCreateVirtualMappingUnsafe(%x, %x, %x, %x (%x), %d)\n",
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
                    "0x%.8X with attributes %x/%x.\n",
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
        
        Pte = InterlockedExchangePte(Pt, PFN_TO_PTE(Pages[i]) | Attributes);;
        /* There should not be anything valid here */
        if (PAGE_MASK(Pte) != 0)
        {
            PMMPFN Pfn1 = MiGetPfnEntry(PTE_TO_PFN(Pte));
            (void)Pfn1;
            DPRINT1("Bad PTE %lx\n", Pte);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        /* flush if currently mapped, just continue editing if hyperspace
         * NOTE : This check is similar to what is done in MiFlushTlb, but we 
         * don't use it because it would unmap the page table */
        if (Addr >= MmSystemRangeStart || (!IS_HYPERSPACE(Pt)))
        {
            KeInvalidateTlbEntry(Addr);
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
    
    DPRINT("MmSetPageProtect(Process %x  Address %x  flProtect %x)\n",
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
    
    if(!PAGE_MASK(Pte))
    {
        DPRINT1("Invalid Pte %lx\n", Pte);
        __debugbreak();
    }
    if((Pte & Attributes) != Attributes)
        MiFlushTlb(Pt, Address);
    else
        MmUnmapPageTable(Pt);
}

/*
 * @implemented
 */
PHYSICAL_ADDRESS NTAPI
MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
    PHYSICAL_ADDRESS p;
    ULONG Pte;
    
    DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
    Pte = MmGetPageEntryForProcess(NULL, vaddr);
    if (Pte != 0 && Pte & PA_PRESENT)
    {
        p.QuadPart = PAGE_MASK(Pte);
        p.u.LowPart |= (ULONG_PTR)vaddr & (PAGE_SIZE - 1);
    }
    else
    {
        p.QuadPart = 0;
    }
    return p;
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    ULONG i;
    PULONG CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;
    
    DPRINT("MmInitGlobalKernelPageDirectory()\n");
    
    for (i = ADDR_TO_PDE_OFFSET(MmSystemRangeStart); i < 1024; i++)
    {
        if (i != ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) &&
            i != ADDR_TO_PDE_OFFSET(HYPERSPACE) &&
            0 == MmGlobalKernelPageDirectory[i] && 0 != CurrentPageDirectory[i])
        {
            MmGlobalKernelPageDirectory[i] = CurrentPageDirectory[i];
        }
    }
}

/* EOF */
