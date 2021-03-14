/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     HAL PIC Management and Control Code
 * COPYRIGHT:   Copyright 2016-2018 Thomas Faber <thomas.faber@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#include <ks386.inc>

/* FUNCTIONS *****************************************************************/

.code

MACRO(DEFINE_END_INTERRUPT_WRAPPER, WrapperName, HandlerName)
EXTERN @&HandlerName&@8:PROC
PUBLIC _&WrapperName&@8
.PROC _&WrapperName&@8
    FPO 0, 2, 0, 0, 0, FRAME_FPO

    /* Call the C function with the same arguments we got */
    mov ecx, [esp+4]
    mov edx, [esp+8]
    call @&HandlerName&@8

    /* Check if we got a pointer back */
    test eax, eax
    jnz WrapperName&_CallIntHandler

    /* No? Just return */
    ret 8

WrapperName&_CallIntHandler:
    /* We got a pointer to call. Since it won't return, reset the stack to
       the location of the stack frame. This frees up our own stack as well
       as that of the functions above us, and avoids an overflow due to
       excessive recursion.
       The next function takes the trap frame as its (fastcall) argument. */
    mov ecx, [esp+8]
    mov esp, ecx
    mov ebp, esp
    jmp eax
.ENDP
ENDM

MACRO(DEFINE_INTERRUPT_WRAPPER, WrapperName, HandlerName)
EXTERN _&HandlerName:PROC
PUBLIC _&WrapperName
.PROC _&WrapperName
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Call the C function */
    call _&HandlerName

    /* Check if we got a pointer back */
    test eax, eax
    jnz WrapperName&_CallIntHandler

    /* No? Just return */
    ret

WrapperName&_CallIntHandler:
    /* Optimize the tail call to avoid stack overflow */
    jmp eax
.ENDP
ENDM


DEFINE_END_INTERRUPT_WRAPPER HalpEndSoftwareInterrupt, HalpEndSoftwareInterrupt2
DEFINE_END_INTERRUPT_WRAPPER HalEndSystemInterrupt, HalEndSystemInterrupt2

DEFINE_INTERRUPT_WRAPPER HalpDispatchInterrupt, HalpDispatchInterrupt2
DEFINE_INTERRUPT_WRAPPER HalpHardwareInterruptLevel, HalpHardwareInterruptLevel2

END
