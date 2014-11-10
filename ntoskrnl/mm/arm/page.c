/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/arm/page.c
 * PURPOSE:         Old-school Page Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

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

ULONG MmGlobalKernelPageDirectory[4096];

/* Template PTE and PDE for a kernel page */
MMPDE ValidKernelPde = {.u.Hard.Valid = 1};
MMPTE ValidKernelPte = {.u.Hard.Valid = 1, .u.Hard.Sbo = 1};

/* Template PDE for a demand-zero page */
MMPDE DemandZeroPde  = {.u.Long = (MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS)};
MMPTE DemandZeroPte  = {.u.Long = (MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS)};

/* Template PTE for prototype page */
MMPTE PrototypePte = {.u.Long = (MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS) | PTE_PROTOTYPE | (MI_PTE_LOOKUP_NEEDED << PAGE_SHIFT)};

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiFlushTlb(IN PMMPTE PointerPte,
           IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
}

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PULONG DirectoryTableBase)
{
    UNIMPLEMENTED_DBGBREAK();
    return FALSE;
}

PULONG
NTAPI
MmGetPageDirectory(VOID)
{
    /* Return the TTB */
    return (PULONG)KeArmTranslationTableRegisterGet().AsUlong;
}

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(IN PEPROCESS Process,
                             IN PVOID Address,
                             IN ULONG Protection,
                             IN PPFN_NUMBER Pages,
                             IN ULONG PageCount)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN ULONG Protection,
                       IN PPFN_NUMBER Pages,
                       IN ULONG PageCount)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

VOID
NTAPI
MmRawDeleteVirtualMapping(IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
MmDeleteVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       OUT PBOOLEAN WasDirty,
                       OUT PPFN_NUMBER Page)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
MmDeletePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY *SwapEntry)
{
    UNIMPLEMENTED_DBGBREAK();
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY SwapEntry)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

PFN_NUMBER
NTAPI
MmGetPfnForProcess(IN PEPROCESS Process,
                   IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

BOOLEAN
NTAPI
MmIsDirtyPage(IN PEPROCESS Process,
              IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
    return FALSE;
}

VOID
NTAPI
MmSetCleanPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
MmSetDirtyPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
}

BOOLEAN
NTAPI
MmIsPagePresent(IN PEPROCESS Process,
                IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
    return FALSE;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(IN PEPROCESS Process,
                  IN PVOID Address)
{
    UNIMPLEMENTED_DBGBREAK();
    return FALSE;
}

ULONG
NTAPI
MmGetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address)
{
    /* We don't enforce any protection on the pages -- they are all RWX */
    return PAGE_READWRITE;
}

VOID
NTAPI
MmSetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address,
                 IN ULONG Protection)
{
    /* We don't enforce any protection on the pages -- they are all RWX */
    return;
}

VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    ULONG i;
    PULONG CurrentPageDirectory = (PULONG)PDE_BASE;

    /* Loop the 2GB of address space which belong to the kernel */
    for (i = MiGetPdeOffset(MmSystemRangeStart); i < 2048; i++)
    {
        /* Check if we have an entry for this already */
        if ((i != MiGetPdeOffset(PTE_BASE)) &&
            (i != MiGetPdeOffset(HYPER_SPACE)) &&
            (!MmGlobalKernelPageDirectory[i]) &&
            (CurrentPageDirectory[i]))
        {
            /* We don't, link it in our global page directory */
            MmGlobalKernelPageDirectory[i] = CurrentPageDirectory[i];
        }
    }
}

