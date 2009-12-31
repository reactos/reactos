/*
 * FILE:            hal/halx86/generic/timer.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Timer Interrupt and Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <ndk/amd64/asmmacro.S>

/* GLOBALS *******************************************************************/

.data

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

.global _MsgUnimplemented
_MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"


/* FUNCTIONS *****************************************************************/

.text
.code64

.global _HalpReleaseCmosSpinLock
.func HalpReleaseCmosSpinLock
_HalpReleaseCmosSpinLock:

.endfunc

.global _HalpAcquireSystemHardwareSpinLock
.func HalpAcquireSystemHardwareSpinLock
_HalpAcquireSystemHardwareSpinLock:

.endfunc

.global _HalpCalibrateStallExecution@0
.func HalpCalibrateStallExecution@0
_HalpCalibrateStallExecution@0:

.endfunc

.globl _HalpProfileInterrupt
.func HalpProfileInterrupt
_HalpProfileInterrupt:

.endfunc


.globl _KeStallExecutionProcessor
.func KeStallExecutionProcessor
_KeStallExecutionProcessor:

    /* Get the number of microseconds required */
    jecxz Done

    /* Multiply by the stall factor */
    mov eax, gs:[KPCR_STALL_SCALE_FACTOR]
    mul ecx

    /* Align to 16 bytes */
    .align 16

    /* Jump to subtraction loop */
    jmp SubtractLoop

    /* Align to 16 bytes */
    .align 16

    /* Subtract one count */
SubtractLoop:
    sub eax, 1
    jnz SubtractLoop

Done:
    /* Return */
    ret 4
.endfunc


.globl _HalpQuery8254Counter
.func HalpQuery8254Counter
_HalpQuery8254Counter:

    /* Save EFLAGS and disable interrupts */
    pushfq
    cli

    /* Set timer data */
    mov al, 0
    out 0x43, al
    jmp $+2

    /* Read current timer */
    in al, 0x40
    jmp $+2
    movzx ecx, al
    in al, 0x40
    mov ch, al

    /* Return it and restore interrupt state */
    mov eax, ecx
    popfq
    ret
.endfunc

.globl _HalpClockInterrupt
.func HalpClockInterrupt
_HalpClockInterrupt:
    UNIMPLEMENTED _HalpClockInterrupt
    iret
.endfunc

