

#include <asm.inc>

.code32


// globals
PUBLIC _PageDirectoryStart
_PageDirectoryStart:
PUBLIC _PageDirectoryEnd
_PageDirectoryEnd:

PUBLIC _startup_pagedirectory
_startup_pagedirectory:
PUBLIC _apic_pagetable
_apic_pagetable:
PUBLIC _kernel_pagetable
_kernel_pagetable:
PUBLIC _lowmem_pagetable
_lowmem_pagetable:

PUBLIC _BootOldLinuxKernel
_BootOldLinuxKernel:

PUBLIC _BootNewLinuxKernel
_BootNewLinuxKernel:

PUBLIC _DriveMapOldInt13HandlerAddress
_DriveMapOldInt13HandlerAddress:

PUBLIC _DriveMapInt13HandlerStart
_DriveMapInt13HandlerStart:

PUBLIC _DriveMapInt13HandlerEnd
_DriveMapInt13HandlerEnd:

PUBLIC _DriveMapInt13HandlerMapList
_DriveMapInt13HandlerMapList:

PUBLIC _PnpBiosGetDeviceNode
_PnpBiosGetDeviceNode:

PUBLIC _PnpBiosGetDeviceNodeCount
_PnpBiosGetDeviceNodeCount:

PUBLIC _PnpBiosSupported
_PnpBiosSupported:

END
