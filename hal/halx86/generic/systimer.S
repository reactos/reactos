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

/* GLOBALS *******************************************************************/

_HalpLastPerfCounterLow:    .long 0
_HalpLastPerfCounterHigh:   .long 0
_HalpPerfCounterLow:        .long 0
_HalpPerfCounterHigh:       .long 0

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

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

.global _KeQueryPerformanceCounter@4
.func KeQueryPerformanceCounter@4
_KeQueryPerformanceCounter@4:

    /* Check if we were called too early */
    cmp dword ptr _HalpCurrentRollOver, 0
    je NoCount

    /* Save volatiles */
    push ebx
    push esi

LoopPreInt:

    /* Disable interrupts */
    pushf
    cli

LoopPostInt:

    /* Get the current value */
    mov ebx, _HalpPerfCounterLow
    mov esi, _HalpPerfCounterHigh

    /* Read 8254 timer */
    mov al, 0
    out 0x43, al
    jmp $+2
    in al, 0x40
    jmp $+2
    movzx ecx, al
    in al, 0x40
    mov ch, al

    /* Enable interrupts and do a short wait */
    popf
    nop
    jmp $+2

    /* Disable them again */
    pushf
    cli

    /* Get the counter value again */
    mov eax, _HalpPerfCounterLow
    mov edx, _HalpPerfCounterHigh

    /* Check if someone updated the counter */
    cmp eax, ebx
    jnz LoopPostInt
    cmp edx, esi
    jnz LoopPostInt

    /* Check if the current 8254 value causes rollover */
    neg ecx
    add ecx, _HalpCurrentRollOver
    jnb DoRollOver

SetSum:

    /* Calculate the sum */
    add eax, ecx
    adc edx, 0

    /* Check if we're above or below the last high value */
    cmp edx, _HalpLastPerfCounterHigh
    jb short BelowHigh
    jnz short BelowLow

    /* Check if we're above or below the last low value */
    cmp eax, _HalpLastPerfCounterLow
    jb BelowHigh

BelowLow:

    /* Update the last value and bring back interrupts */
    mov _HalpLastPerfCounterLow, eax
    mov _HalpLastPerfCounterHigh, edx
    popf

    /* Check if caller wants frequency */
    cmp dword ptr [esp+12], 0
    jz ReturnNoFreq

    /* Save hard-coded frequency */
    mov ecx, dword ptr [esp+12]
    mov dword ptr [ecx], 1193182
    mov dword ptr [ecx+4], 0

ReturnNoFreq:

    /* Restore volatiles */
    pop esi
    pop ebx
    ret 4

NoCount:

    /* Return empty, called too soon */
    mov eax, 0
    mov edx, 0
    ret 4

DoRollOver:

    /* We might have an incoming interrupt, save EFLAGS and reset rollover */
    mov esi, [esp]
    mov ecx, _HalpCurrentRollOver
    popf

    /* Check if interrupts were enabled and try again */
    test esi, EFLAGS_INTERRUPT_MASK
    jnz LoopPreInt

    /* They're not, continue where we left */
    pushf
    jmp SetSum

BelowHigh:

    /* Get the last counter values */
    mov ebx, _HalpLastPerfCounterLow
    mov esi, _HalpLastPerfCounterHigh

    /* Check if the previous value was 0 and go back if yes */
    mov ecx, ebx
    or ecx, esi
    jz BelowLow

    /* Make sure that the count is still valid */
    sub ebx, eax
    sbb esi, edx
    jnz InvalidCount
    cmp ebx, _HalpCurrentRollOver
    jg InvalidCount

    /* Fixup the count with the last known value */
    sub eax, ebx
    sbb edx, esi

    /* We might have an incoming interrupt, save EFLAGS */
    mov ecx, [esp]
    popf

    /* Check if interrupts were enabled and try again */
    test ecx, EFLAGS_INTERRUPT_MASK
    jnz LoopPreInt

    /* They're not, continue where we left */
    pushf
    jmp BelowLow

InvalidCount:
    popf
    xor eax, eax
    mov _HalpLastPerfCounterLow, eax
    mov _HalpLastPerfCounterHigh, eax
    jmp LoopPreInt
.endfunc

.globl _HalpClockInterrupt@0
.func HalpClockInterrupt@0
TRAP_FIXUPS hci_a, hci_t, DoFixupV86, DoFixupAbios
_HalpClockInterrupt@0:

    /* Enter trap */
    INT_PROLOG hci_a, hci_t, DoPushFakeErrorCode

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

    /* Update the performance counter */
    xor ebx, ebx
    mov eax, _HalpCurrentRollOver
    add _HalpPerfCounterLow, eax
    adc _HalpPerfCounterHigh, ebx

    /* Get the time increment and check if someone changed the clock rate */
    mov eax, _HalpCurrentTimeIncrement
    cmp _HalpClockSetMSRate, ebx
    jz _KeUpdateSystemTime@0

    /* FIXME: Someone did! */
    int 3

Spurious:

    /* Exit the interrupt */
    add esp, 8
    jmp _Kei386EoiHelper@0
.endfunc

