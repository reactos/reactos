#include <roscfg.h>
#include <ndk/asm.h>

#define AP_MAGIC (0x12481020)

.global _kernel_stack
.global _kernel_stack_top
.global _kernel_trap_stack
.global _kernel_trap_stack_top

	.section .bss
	.align 12


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
	.globl NtProcessStartup
NtProcessStartup:
    lis		0,AP_MAGIC@ha
    ori		0,0,AP_MAGIC@l
    cmpw	0,0,3
    bne		.m1
     
    /*
     *  Reserve space for the floating point save area.
     */
    addi  1,1,-SIZEOF_FX_SAVE_AREA

    /*
     * Call the application processor initialization code
     */
    bl  _KiSystemStartup
    
.m1:     
    /* Load the initial kernel stack */
    lis		1,_kernel_stack_top@ha
    ori		1,1,_kernel_stack_top@l
    addi	1,1,-SIZEOF_FX_SAVE_AREA
    
    /* Call the main kernel initialization */
    mr		3,5
    mr		4,6
    bl		__main

