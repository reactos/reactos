/*
 * COPYRIGHT:       GPL, See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/page.c
 * PURPOSE:         Low level memory managment manipulation
 *
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitGlobalKernelPageDirectory)
#pragma alloc_text(INIT, MiInitPageDirectoryMap)
#endif

extern MMPTE HyperTemplatePte;

/* GLOBALS *****************************************************************/

ULONG64 MmGlobalKernelPageDirectory[512];
ULONG Ke386GlobalPagesEnabled = 0;


/* PRIVATE FUNCTIONS *******************************************************/

BOOLEAN
FORCEINLINE
MiIsHyperspaceAddress(PVOID Address)
{
    return ((ULONG64)Address >= MI_HYPER_SPACE_START && 
            (ULONG64)Address <= MI_HYPER_SPACE_END);
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
    PMMPTE Pte;

    /* Check if we need hypersapce mapping */
    if (Address < MmSystemRangeStart && 
        Process && Process != PsGetCurrentProcess())
    {
        UNIMPLEMENTED;
        return NULL;
    }
    else if (Create)
    {
        /* Get the PXE */
        Pte = MiAddressToPxe(Address);
        if (!Pte->u.Hard.Valid)
            InterlockedBitTestAndSet64(&Pte->u.Long, 0);

        /* Get the PPE */
        Pte = MiAddressToPpe(Address);
        if (!Pte->u.Hard.Valid)
            InterlockedBitTestAndSet64(&Pte->u.Long, 0);

        /* Get the PDE */
        Pte = MiAddressToPde(Address);
        if (!Pte->u.Hard.Valid)
            InterlockedBitTestAndSet64(&Pte->u.Long, 0);

        /* Get the PTE */
        Pte = MiAddressToPte(Address);

        return Pte;
    }
    else
    {
        /* Get the PXE */
        Pte = MiAddressToPxe(Address);
        if (!Pte->u.Hard.Valid)
            return NULL;

        /* Get the PPE */
        Pte = MiAddressToPpe(Address);
        if (Pte->u.Hard.Valid)
            return NULL;

        /* Get the PDE */
        Pte = MiAddressToPde(Address);
        if (Pte->u.Hard.Valid)
            return NULL;

        /* Get the PTE */
        Pte = MiAddressToPte(Address);

        return Pte;
    }

    return 0;
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


/* FUNCTIONS ***************************************************************/

PFN_TYPE
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return Pte.u.Hard.Valid ? Pte.u.Hard.PageFrameNumber : 0;
}

PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(PVOID Address)
{
    PHYSICAL_ADDRESS p;
    MMPTE Pte;

    Pte.u.Long = MiGetPteValueForProcess(NULL, Address);
    if (Pte.u.Hard.Valid)
    {
        p.QuadPart = Pte.u.Hard.PageFrameNumber * PAGE_SIZE;
        p.u.LowPart |= (ULONG_PTR)Address & (PAGE_SIZE - 1);
    }
    else
    {
        p.QuadPart = 0;
    }

    return p;
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return Pte.u.Hard.Valid;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    MMPTE Pte;
    Pte.u.Long = MiGetPteValueForProcess(Process, Address);
    return Pte.u.Hard.Valid && Pte.u.Soft.Transition;
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
    ULONG Protect;

    Pte.u.Long = MiGetPteValueForProcess(Process, Address);

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

#define PAGE_EXECUTE_ANY (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
    PMMPTE Pte;
    MMPTE NewPte;

    if (!(flProtect & PAGE_EXECUTE_ANY))
        NewPte.u.Flush.NoExecute = 1;

    if (flProtect & (PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY))
    {
        NewPte.u.Flush.Write = 1;
        NewPte.u.Flush.CopyOnWrite = 1;
    }

    if (flProtect & (PAGE_EXECUTE_READWRITE|PAGE_READWRITE))
        NewPte.u.Flush.Write = 1;

    if (flProtect & PAGE_NOCACHE)
        NewPte.u.Flush.CacheDisable = 1;

    if (flProtect & PAGE_WRITETHROUGH)
        NewPte.u.Flush.WriteThrough = 1;

    Pte = MiGetPteForProcess(Process, Address, FALSE);

    InterlockedExchange64(&Pte->u.Long, NewPte.u.Long);

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


NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(PEPROCESS Process)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN* WasDirty, PPFN_TYPE Page)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
MmDeleteVirtualMapping(
    PEPROCESS Process,
    PVOID Address,
    BOOLEAN FreePage,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page)
{
    PFN_NUMBER Pfn;
    PMMPTE Pte;
    MMPTE OldPte;

    Pte = MiGetPteForProcess(Process, Address, FALSE);

    if (Pte)
    {
        /* Atomically set the entry to zero and get the old value. */
        OldPte.u.Long = InterlockedExchange64(&Pte->u.Long, 0);

        if (OldPte.u.Hard.Valid)
        {
            Pfn = OldPte.u.Hard.PageFrameNumber;

            if (FreePage)
                MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
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
        *WasDirty = OldPte.u.Hard.Dirty;;

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


VOID
NTAPI
MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
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
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_TYPE Pages,
                             ULONG PageCount)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PPFN_TYPE Pages,
                       ULONG PageCount)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
    ULONG StartIndex, EndIndex, Index;
    PMMPTE Pte;

    /* Sanity check */
    if (Address < MmSystemRangeStart)
    {
        KeBugCheck(0);
    }

    /* Get pointer to the page directory to update */
    if (Process && Process != PsGetCurrentProcess())
    {
//       Pte = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase[0]));
    }
    else
    {
        Pte = (PMMPTE)PXE_BASE;
    }

    /* Update PML4 entries */
    StartIndex = VAtoPXI(Address);
    EndIndex = VAtoPXI((ULONG64)Address + Size - 1);
    for (Index = StartIndex; Index <= EndIndex; Index++)
    {
        if (Index != VAtoPXI(PXE_BASE))
        {
            InterlockedCompareExchange64(&Pte[Index].u.Long,
                                         MmGlobalKernelPageDirectory[Index],
                                         0);
            if (!MiIsHyperspaceAddress(Pte))
                __invlpg((PVOID)((ULONG64)Index * PAGE_SIZE));
        }
    }

    if (MiIsHyperspaceAddress(Pte))
        MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pte));
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    PULONG64 CurrentPageDirectory = (PULONG64)PXE_BASE;
    MMPTE Pte;
    ULONG i;

    /* Setup template pte */
    HyperTemplatePte.u.Long = 0;
    HyperTemplatePte.u.Hard.Valid = 1;
    HyperTemplatePte.u.Hard.Write = 1;
    HyperTemplatePte.u.Hard.Dirty = 1;
    HyperTemplatePte.u.Hard.Accessed = 1;
    if (Ke386GlobalPagesEnabled)
        HyperTemplatePte.u.Hard.Global = 1;

    for (i = VAtoPXI(MmSystemRangeStart); i < 512; i++)
    {
        if ((i < VAtoPXI(PTE_BASE) || i > VAtoPXI(PTE_TOP)) &&
            (i < VAtoPXI(MI_HYPER_SPACE_START) || i > VAtoPXI(MI_HYPER_SPACE_END)) &&
            MmGlobalKernelPageDirectory[i] == 0 && 
            CurrentPageDirectory[i] != 0)
        {
            Pte.u.Long = CurrentPageDirectory[i];
            if (Ke386GlobalPagesEnabled)
            {
                Pte.u.Hard.Global = 1;
                CurrentPageDirectory[i] = Pte.u.Hard.Global;
            }
            MmGlobalKernelPageDirectory[i] = Pte.u.Hard.Global;
        }
    }
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PULONG_PTR DirectoryTableBase)
{
    /* Share the directory base with the idle process */
    DirectoryTableBase[0] = PsGetCurrentProcess()->Pcb.DirectoryTableBase[0];
    DirectoryTableBase[1] = PsGetCurrentProcess()->Pcb.DirectoryTableBase[1];

    /* Initialize the Addresss Space */
    KeInitializeGuardedMutex(&Process->AddressCreationLock);
    Process->Vm.WorkingSetExpansionLinks.Flink = NULL;
    ASSERT(Process->VadRoot.NumberGenericTableElements == 0);
    Process->VadRoot.BalancedRoot.u1.Parent = &Process->VadRoot.BalancedRoot;

    /* The process now has an address space */
    Process->HasAddressSpace = TRUE;
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PULONG_PTR DirectoryTableBase)
{
    UNIMPLEMENTED;
    return 0;
}


/* EOF */
