/*
 * COPYRIGHT:       See COPYING in the top level directory
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


/* GLOBALS *****************************************************************/

ULONG64 MmGlobalKernelPageDirectory[512];


/* PRIVATE FUNCTIONS *******************************************************/


/* FUNCTIONS ***************************************************************/

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(PEPROCESS Process)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PULONG_PTR DirectoryTableBase)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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

PFN_TYPE
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
    UNIMPLEMENTED;
    return 0;
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
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN FreePage,
                       BOOLEAN* WasDirty, PPFN_TYPE Page)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
    return 0;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
    return 0;
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

ULONG
NTAPI
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
    UNIMPLEMENTED;
}

PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(PVOID vaddr)
{
	PHYSICAL_ADDRESS ret = {{0}};
    UNIMPLEMENTED;
    return ret;
}

VOID
NTAPI
MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
    ULONG StartIndex, EndIndex, Index;
    PULONG64 Pde;

    /* Sanity check */
    if (Address < MmSystemRangeStart)
    {
        KeBugCheck(0);
    }

    /* Get pointer to the page directory to update */
    if (Process != NULL && Process != PsGetCurrentProcess())
    {
//       Pde = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase[0]));
    }
    else
    {
        Pde = (PULONG64)PXE_BASE;
    }

    /* Update PML4 entries */
    StartIndex = VAtoPXI(Address);
    EndIndex = VAtoPXI((ULONG64)Address + Size);
    for (Index = StartIndex; Index <= EndIndex; Index++)
    {
        if (Index != VAtoPXI(PXE_BASE))
        {
            (void)InterlockedCompareExchangePointer((PVOID*)&Pde[Index],
                                                    (PVOID)MmGlobalKernelPageDirectory[Index],
                                                    0);
        }
    }
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    UNIMPLEMENTED;
}


/* EOF */
