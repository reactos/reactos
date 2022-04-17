/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/arch/transfer.asm
 * PURPOSE:         Boot Library i386 Transfer Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

EXTERN _GdtRegister:FWORD
EXTERN _IdtRegister:FWORD
EXTERN _BootAppGdtRegister:FWORD
EXTERN _BootAppIdtRegister:FWORD
EXTERN _BootApp32Stack:DWORD
EXTERN _BootApp32EntryRoutine:DWORD
EXTERN _BootApp32Parameters:DWORD

/* FUNCTIONS ****************************************************************/
.code

PUBLIC _Archx86TransferTo32BitApplicationAsm
_Archx86TransferTo32BitApplicationAsm:

    /* Save non-volatile registers */
    push ebp
    push esi
    push edi
    push ebx

    /* Save data segments */
    push es
    push ds

    /* Save the old stack */
    mov ebx, esp

    /* Save current GDT/IDT, then load new one */
    sgdt _GdtRegister+2
    sidt _IdtRegister+2
    lgdt _BootAppGdtRegister+2
    lidt _BootAppIdtRegister+2

    /* Load the new stack */
    xor ebp, ebp
    mov esp, _BootApp32Stack

    /* Push old stack onto new stack */
    push ebx

    /* Call the entry routine, passing the parameters */
    mov eax, _BootApp32Parameters
    push eax
    mov eax, _BootApp32EntryRoutine
    call eax

    /* Retore old stack */
    pop ebx
    mov esp, ebx

    /* Restore old GDT/IDT */
    lgdt _GdtRegister+2
    lidt _IdtRegister+2

    /* Retore old segments */
    pop ds
    pop es

    /* Retore non-volatiles */
    pop ebx
    pop edi
    pop esi
    pop ebp

    /* All done */
    ret

END
