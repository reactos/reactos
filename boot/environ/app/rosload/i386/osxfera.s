/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI OS Loader
 * FILE:            boot/environ/i386/osxfera.asm
 * PURPOSE:         OS Loader i386 Transfer Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

EXTERN _OslKernelGdt:FWORD
EXTERN _OslKernelIdt:FWORD

/* FUNCTIONS ****************************************************************/
.code

PUBLIC _OslArchTransferToKernel
_OslArchTransferToKernel:

    /* Load new GDT and IDT */
    lgdt _OslKernelGdt+2
    lidt _OslKernelIdt+2

    /* Set the Ring 0 DS/ES/SS Segment */
    mov ax, KGDT_R0_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov gs, ax

    /* Get the Ring 0 TSS */
    mov ax, KGDT_TSS
    ltr ax

    /* Save loader block and entrypoint */
    mov ecx, [esp+4]
    mov eax, [esp+8]

    /* Create initial interrupt frame */
    xor edx, edx
    push ecx
    push edx
    push KGDT_R0_CODE
    push eax

    /* Jump to KGDT_R0_CODE:[EAX] */
    retf

    /* We should never make it here */
    ret 8
END
