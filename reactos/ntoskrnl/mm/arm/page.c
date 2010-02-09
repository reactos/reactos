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

/* GLOBALS ********************************************************************/

ULONG MmGlobalKernelPageDirectory[4096];
MMPDE HyperTemplatePde;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiFlushTlb(IN PMMPTE PointerPte,
           IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PULONG DirectoryTableBase)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
MmUpdatePageDir(IN PEPROCESS Process,
                IN PVOID Address,
                IN ULONG Size)
{
    /* Nothing to do */
    return;
}

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(IN PEPROCESS Process)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PULONG DirectoryTableBase)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

PULONG
NTAPI
MmGetPageDirectory(VOID)
{
    /* Return the TTB */
    return (PULONG)KeArmTranslationTableRegisterGet().AsUlong;
}

VOID
NTAPI
MmDisableVirtualMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        OUT PBOOLEAN WasDirty,
                        OUT PPFN_TYPE Page)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
MmEnableVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(IN PEPROCESS Process,
                             IN PVOID Address,
                             IN ULONG Protection,
                             IN PPFN_TYPE Pages,
                             IN ULONG PageCount)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN ULONG Protection,
                       IN PPFN_TYPE Pages,
                       IN ULONG PageCount)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
NTAPI
MmRawDeleteVirtualMapping(IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
MmDeleteVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN BOOLEAN FreePage,
                       OUT PBOOLEAN WasDirty,
                       OUT PPFN_TYPE Page)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
MmDeletePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY *SwapEntry)
{
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY SwapEntry)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

PFN_TYPE
NTAPI
MmGetPfnForProcess(IN PEPROCESS Process,
                   IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

BOOLEAN
NTAPI
MmIsDirtyPage(IN PEPROCESS Process,
              IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

VOID
NTAPI
MmSetCleanPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
MmSetDirtyPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
MmIsPagePresent(IN PEPROCESS Process,
                IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(IN PEPROCESS Process,
                  IN PVOID Address)
{
    UNIMPLEMENTED;
    while (TRUE);
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
    extern MMPTE HyperTemplatePte;
    
    /* Setup PTE template */
    HyperTemplatePte.u.Long = 0;
    HyperTemplatePte.u.Hard.Valid = 1;
    HyperTemplatePte.u.Hard.Access = 1;

    /* Setup PDE template */
    HyperTemplatePde.u.Long = 0;
    HyperTemplatePde.u.Hard.Valid = 1;
        
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

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(IN PVOID Address)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    PhysicalAddress.QuadPart = 0;

    UNIMPLEMENTED;
    while (TRUE);

    return PhysicalAddress;
}
