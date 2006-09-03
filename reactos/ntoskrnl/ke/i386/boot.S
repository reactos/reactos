/*
 * FILE:            ntoskrnl/ke/i386/boot.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Bootstrap Code
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

.bss
.align 16

/* Kernel Boot Stack */
.globl _P0BootStack
.space KERNEL_STACK_SIZE
_P0BootStack:

/* Kernel Double-Fault and Temporary DPC Stack */
.globl _KiDoubleFaultStack
.space KERNEL_STACK_SIZE
_KiDoubleFaultStack:

/* FUNCTIONS *****************************************************************/

.text
.func NtProcessStartup
_NtProcessStartup:

    /* Load the initial kernel stack */
    lea eax, _P0BootStack
    sub eax, (NPX_FRAME_LENGTH + KTRAP_FRAME_LENGTH + KTRAP_FRAME_ALIGN)
    mov esp, eax

    /* Save initial CR0 state */
    push CR0_EM + CR0_TS + CR0_MP

    /* Call the main kernel initialization */
    push edx
    call _KiRosPrepareForSystemStartup@4
.endfunc
