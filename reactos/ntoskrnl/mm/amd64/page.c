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

#undef InterlockedExchangePte
#define InterlockedExchangePte(pte1, pte2) \
    InterlockedExchange64((LONG64*)&pte1->u.Long, pte2.u.Long)

#define PAGE_EXECUTE_ANY (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)
#define PAGE_WRITE_ANY (PAGE_EXECUTE_READWRITE|PAGE_READWRITE|PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY)
#define PAGE_WRITECOPY_ANY (PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY)

extern MMPTE HyperTemplatePte;

/* GLOBALS *****************************************************************/


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
        return NULL;
    }
    else if (Create)
    {
        TmplPte.u.Long = 0;
        TmplPte.u.Flush.Valid = 1;
        TmplPte.u.Flush.Write = 1;

        /* Get the PXE */
        Pte = MiAddressToPxe(Address);
        if (!Pte->u.Hard.Valid)
        {
//            TmplPte.u.Hard.PageFrameNumber = MiAllocPage(TRUE);
            InterlockedExchangePte(Pte, TmplPte);
        }

        /* Get the PPE */
        Pte = MiAddressToPpe(Address);
        if (!Pte->u.Hard.Valid)
        {
//            TmplPte.u.Hard.PageFrameNumber = MiAllocPage(TRUE);
            InterlockedExchangePte(Pte, TmplPte);
        }

        /* Get the PDE */
        Pte = MiAddressToPde(Address);
        if (!Pte->u.Hard.Valid)
        {
//            TmplPte.u.Hard.PageFrameNumber = MiAllocPage(TRUE);
            InterlockedExchangePte(Pte, TmplPte);
        }
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


NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(PEPROCESS Process)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN* WasDirty, PPFN_NUMBER Page)
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
MmCreateVirtualMappingUnsafe(
    PEPROCESS Process,
    PVOID Address,
    ULONG PageProtection,
    PPFN_NUMBER Pages,
    ULONG PageCount)
{
    ULONG i;
    MMPTE TmplPte, *Pte;

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

//__debugbreak();

    for (i = 0; i < PageCount; i++)
    {
        TmplPte.u.Hard.PageFrameNumber = Pages[i];

        Pte = MiGetPteForProcess(Process, Address, TRUE);

DPRINT1("MmCreateVirtualMappingUnsafe, Address=%p, TmplPte=%p, Pte=%p\n", 
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
