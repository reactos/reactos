/*
 * FILE:            ntoskrnl/ke/i386/boot.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         FreeLDR Wrapper Bootstrap Code and Bootstrap Trampoline
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

EXTERN KiInitializeKernelAndGotoIdleLoop:PROC

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

.code64

/**
 * VOID
 * KiSetupStackAndInitializeKernel(
 *     IN PKPROCESS InitProcess,               <rsp + 0x08, rcx>
 *     IN PKTHREAD InitThread,                 <rsp + 0x10, rdx>
 *     IN PVOID IdleStack,                     <rsp + 0x18, r8>
 *     IN PKPRCB Prcb,                         <rsp + 0x20, r9>
 *     IN CCHAR Number,                        <rsp + 0x28>
 *     IN PLOADER_PARAMETER_BLOCK LoaderBlock) <rsp + 0x30>
 */
PUBLIC KiSetupStackAndInitializeKernel
.PROC KiSetupStackAndInitializeKernel

    /* Save current stack */
    mov rsi, rsp

    /* Setup the new stack */
    mov ax, HEX(18)
    mov ss, ax
    mov rsp, r8
    sub rsp, HEX(300) // FIXME

    /* Copy stack parameters to the new stack */
    sub rsp, HEX(38)
    .ENDPROLOG

    mov rdi, rsp
    movsq
    movsq
    movsq
    movsq
    movsq
    movsq
    movsq

    jmp KiInitializeKernelAndGotoIdleLoop

.ENDP KiSetupStackAndInitializeKernel

END

