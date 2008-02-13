/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/arm/stubs.c
 * PURPOSE:         ARM Memory Manager
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
/* GLOBALS ********************************************************************/

//
// METAFIXME: We need to stop using 1MB Section Entry TTEs!
//

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
MmUpdatePageDir(IN PEPROCESS Process,
                IN PVOID Address,
                IN ULONG Size)
{
    //
    // Nothing to do
    //
    UNIMPLEMENTED;
    return;
}

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(IN PEPROCESS Process)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PLARGE_INTEGER DirectoryTableBase)
{
    //
    // Share the directory base with the idle process
    //
    *DirectoryTableBase = PsGetCurrentProcess()->Pcb.DirectoryTableBase;
    
    //
    // Initialize the Addresss Space
    //
    MmInitializeAddressSpace(Process, (PMADDRESS_SPACE)&Process->VadRoot);
    
    //
    // The process now has an address space
    //
    Process->HasAddressSpace = TRUE;
    return STATUS_SUCCESS;
}

PULONG
MmGetPageDirectory(VOID)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}


BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PLARGE_INTEGER DirectoryTableBase)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
MmDeletePageTable(IN PEPROCESS Process,
                  IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

PFN_TYPE
NTAPI
MmGetPfnForProcess(IN PEPROCESS Process,
                   IN PVOID Address)
{
    PFN_TYPE Pfn = {0};
    
    //
    // TODO
    //
    UNIMPLEMENTED;
    return Pfn;
}

VOID
NTAPI
MmDisableVirtualMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        OUT PBOOLEAN WasDirty,
                        OUT PPFN_TYPE Page)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

VOID
NTAPI
MmRawDeleteVirtualMapping(IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

VOID
NTAPI
MmDeleteVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN BOOLEAN FreePage,
                       OUT PBOOLEAN WasDirty,
                       OUT PPFN_TYPE Page)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

VOID
NTAPI
MmDeletePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY *SwapEntry)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
MmIsDirtyPage(IN PEPROCESS Process,
              IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
MmSetCleanPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

VOID
NTAPI
MmSetDirtyPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

VOID
NTAPI
MmEnableVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
MmIsPagePresent(IN PEPROCESS Process,
                IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(IN PEPROCESS Process,
                  IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(IN PVOID Address,
                                IN ULONG flProtect,
                                IN PPFN_TYPE Pages,
                                IN ULONG PageCount)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY SwapEntry)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(IN PEPROCESS Process,
                             IN PVOID Address,
                             IN ULONG flProtect,
                             IN PPFN_TYPE Pages,
                             IN ULONG PageCount)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN ULONG flProtect,
                       IN PPFN_TYPE Pages,
                       IN ULONG PageCount)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
MmGetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
MmSetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address,
                 IN ULONG flProtect)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(IN PVOID vaddr)
{
    PHYSICAL_ADDRESS p;

    //
    // TODO
    //
    UNIMPLEMENTED;
    return p;
}

PVOID
NTAPI
MmCreateHyperspaceMapping(IN PFN_TYPE Page)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(IN PVOID Address)
{
    PFN_TYPE Pfn = {0};
      
    //
    // TODO
    //
    UNIMPLEMENTED;
    return Pfn;
}

VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
MiInitPageDirectoryMap(VOID)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}
