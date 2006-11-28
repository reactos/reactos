/*
 * FILE:            hal/halx86/generic/timer.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Timer Interrupt and Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix
.extern _HalpCurrentTimeIncrement

/* FUNCTIONS *****************************************************************/

.globl _KeStallExecutionProcessor@4
.func KeStallExecutionProcessor@4
_KeStallExecutionProcessor@4:

    /* Get the number of microseconds required */
    mov ecx, [esp+4]
    jecxz Done

    /* Multiply by the stall factor */
    mov eax, fs:[KPCR_STALL_SCALE_FACTOR]
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

.globl _HalpClockInterrupt@0
.func HalpClockInterrupt@0
_HalpClockInterrupt@0:

    /* Enter trap */
    INT_PROLOG Hci, DoPushFakeErrorCode

    /* Push vector and make stack for IRQL */
    push 0x30
    sub esp, 4

    /* Begin the interrupt */
    push esp
    push 0x30
    push CLOCK2_LEVEL
    call _HalBeginSystemInterrupt@12

    /* Check if it's spurious */
    or al, al
    jz Spurious

    /* Do a tick */
    mov eax, _HalpCurrentTimeIncrement
    xor ebx, ebx
    jmp _KeUpdateSystemTime@0

Spurious:

    /* Exit the interrupt */
    add esp, 8
    mov esi, $
    jmp _Kei386EoiHelper@0
.endfunc

