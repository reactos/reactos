

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

END
