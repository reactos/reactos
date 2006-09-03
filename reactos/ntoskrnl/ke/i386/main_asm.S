#include <roscfg.h>
#include <ndk/asm.h>

#define AP_MAGIC (0x12481020)

.global _kernel_stack
.global _kernel_stack_top
.global _kernel_trap_stack
.global _kernel_trap_stack_top

.globl _NtProcessStartup

.bss
.align 4096


/* guard page for the kernel stack */
.fill 4096, 1, 0

_kernel_stack:
.fill 3*4096, 1, 0
_kernel_stack_top:

/* guard page for the trap stack */
.fill 4096, 1, 0

_kernel_trap_stack:
.fill 3*4096, 1, 0
_kernel_trap_stack_top:
	
.text

_NtProcessStartup:

    /* FIXME: Application processors should have their own GDT/IDT */
    lgdt _KiGdtDescriptor
    lidt _KiIdtDescriptor

    /* Load the initial kernel stack */
    lea _kernel_stack_top, %eax
    sub $(SIZEOF_FX_SAVE_AREA), %eax
    movl %eax, %esp

    /* Call the main kernel initialization */
    pushl %edx
    call _KiRosPrepareForSystemStartup@4
