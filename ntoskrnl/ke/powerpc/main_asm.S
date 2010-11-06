#include <ndk/asm.h>

#define AP_MAGIC (0x12481020)

.global P0BootStack
.global	KiDoubleFaultStack
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
P0BootStack:	
_kernel_stack_top:

/* guard page for the trap stack */
.fill 4096, 1, 0

_kernel_trap_stack:
.fill 3*4096, 1, 0
_kernel_trap_stack_top:

.fill 3*4096, 1, 0
KiDoubleFaultStack:	
	
	.text
	.globl KiSystemStartup
	.globl KiRosPrepareForSystemStartup
	.globl DrawNumber
	
KiSystemStartup:
    /*
     * Set a normal MSR value
     */
    xor		0,0,0     
    ori		30,0,0x3030
    mtmsr	30

    /*
     *  Reserve space for the floating point save area.
     */
    addi  1,1,-SIZEOF_FX_SAVE_AREA

    /* Bye bye asm land! */
    mr		4,3
		
    /* Load the initial kernel stack */
    lis		1,_kernel_stack_top@ha
    ori		1,1,_kernel_stack_top@l
    addi	1,1,-SIZEOF_FX_SAVE_AREA
    
    /* Call the main kernel initialization */
    bl		KiRosPrepareForSystemStartup

	.global NtCurrentTeb
NtCurrentTeb:
	mr 3,12
	blr	

        .globl KeSynchronizeExecution

KeSynchronizeExecution: 
        blr

        .globl PearPCDebug
PearPCDebug:
//        .long 0x00333303
        blr
