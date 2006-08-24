/*
 * FILE:            ntoskrnl/ke/i386/clock.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Clock Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

#include <ndk/asm.h>
#include <../hal/halx86/include/halirq.h>

.global _irq_handler_0
_irq_handler_0:
    pusha
    cld
    push ds
    push es
    push fs
    push gs
    push 0xCEAFBEEF
    mov eax, KGDT_R0_DATA
    mov ds, eax
    mov es, eax
    mov gs, eax
    mov eax, KGDT_R0_PCR
    mov fs, eax

    /* Increase interrupt count */
    inc dword ptr [fs:KPCR_PRCB_INTERRUPT_COUNT]

    /* Put vector in EBX and make space for KIRQL */
    sub esp, 4

    /* Begin interrupt */
    push esp
    push 0x30
    push HIGH_LEVEL
    call _HalBeginSystemInterrupt@12

    cli
    call _HalEndSystemInterrupt@8

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret
