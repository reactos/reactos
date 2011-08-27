/*
 * FILE:            hal/halx86/generic/timer.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Timer Interrupt and Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* GLOBALS *******************************************************************/

.data

PUBLIC MsgUnimplemented
MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"


/* FUNCTIONS *****************************************************************/

.code64

PUBLIC HalpCalibrateStallExecution@0
HalpCalibrateStallExecution@0:


PUBLIC HalpProfileInterrupt
HalpProfileInterrupt:


PUBLIC KeStallExecutionProcessor
KeStallExecutionProcessor:

    /* Get the number of microseconds required */
    jecxz Done

    /* Multiply by the stall factor */
    mov eax, gs:[PcStallScaleFactor]
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


PUBLIC HalpQuery8254Counter
HalpQuery8254Counter:

    /* Save EFLAGS and disable interrupts */
    pushfq
    cli

    /* Set timer data */
    mov al, 0
    out HEX(43), al
    jmp $+2

    /* Read current timer */
    in al, HEX(40)
    jmp $+2
    movzx ecx, al
    in al, HEX(40)
    mov ch, al

    /* Return it and restore interrupt state */
    mov eax, ecx
    popfq
    ret

PUBLIC HalpClockInterrupt
HalpClockInterrupt:
    UNIMPLEMENTED _HalpClockInterrupt
    iret

END
