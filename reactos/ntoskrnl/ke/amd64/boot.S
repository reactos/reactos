/*
 * FILE:            ntoskrnl/ke/i386/boot.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         FreeLDR Wrapper Bootstrap Code and Bootstrap Trampoline
 * PROGRAMMERs:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller <w3seek@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
.intel_syntax noprefix
.code64

/* GLOBALS *******************************************************************/

.bss
.align 16

/* Kernel Boot Stack */
.globl _P0BootStack
.space KERNEL_STACK_SIZE
_P0BootStack:

/* Kernel Double-Fault and Temporary DPC Stack */
.globl _KiDoubleFaultStack
.space KERNEL_STACK_SIZE
_KiDoubleFaultStack:

/* FUNCTIONS *****************************************************************/

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
.globl _KiSetupStackAndInitializeKernel
.func KiSetupStackAndInitializeKernel
_KiSetupStackAndInitializeKernel:

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

    jmp _KiInitializeKernelAndGotoIdleLoop

.endfunc
