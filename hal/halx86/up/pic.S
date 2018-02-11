/*
 * FILE:            hal/halx86/up/pic.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         HAL PIC Management and Control Code
 * PROGRAMMER:      Thomas Faber (thomas.faber@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#include <ks386.inc>

EXTERN @HalpEndSoftwareInterrupt2@8:PROC

/* GLOBALS *******************************************************************/

.data
ASSUME CS:NOTHING, DS:NOTHING, ES:NOTHING, FS:NOTHING, GS:NOTHING

/* FUNCTIONS *****************************************************************/

.code
PUBLIC _HalpEndSoftwareInterrupt@8
.PROC _HalpEndSoftwareInterrupt@8
    FPO 0, 2, 0, 0, 0, FRAME_FPO

    /* Call the C function with the same arguments we got */
    mov ecx, [esp+4]
    mov edx, [esp+8]
    call @HalpEndSoftwareInterrupt2@8

    /* Check if we got a pointer back */
    test eax, eax
    jnz CallIntHandler

    /* No? Just return */
    ret 8

CallIntHandler:
    /* We got a pointer to call. Since it won't return, free up our stack
       space. Otherwise we could end up with some nasty deep recursion.
       The next function takes the trap frame as its (fastcall) argument. */
    mov ecx, [esp+8]
    add esp, 12
    jmp eax
.ENDP

END
