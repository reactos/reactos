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

/* FUNCTIONS *****************************************************************/

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
    mov eax, 100000
    jmp _KeUpdateSystemTime@0

Spurious:

    /* Exit the interrupt */
    add esp, 8
    mov esi, $
    jmp _Kei386EoiHelper@0
.endfunc

