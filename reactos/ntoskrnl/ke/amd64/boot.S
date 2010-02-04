/*
 * FILE:            ntoskrnl/ke/i386/boot.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         FreeLDR Wrapper Bootstrap Code and Bootstrap Trampoline
 * PROGRAMMERs:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller <w3seek@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/amd64/asm.h>


/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

.code64
.text

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
    mov ax, 0x18
    mov ss, ax
    mov rsp, r8
    sub rsp, 0x300 // FIXME

    /* Copy stack parameters to the new stack */
    sub rsp, 0x38
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

