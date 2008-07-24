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
.global _KiSystemStartup
.text
.func KiSystemStartup
_KiSystemStartup:

    /* NTLDR Boot: Call the main kernel initialization */
    test rcx, 0x80000000
    jnz _KiSystemStartupReal

    /* FREELDR Boot: Cal the FreeLDR wrapper */
    jmp _KiRosPrepareForSystemStartup
.endfunc

/**
 * VOID
 * KiSetupStackAndInitializeKernel(
 *             esp+4 = ? -> rcx
 *             esp+8 = ? -> rdx
 *        PVOID pNewstack     // esp+12 = new stack -> r8
 *               esp+16 -> r9
 *               esp+20 -> rsp + 8
 *               esp+24 -> rsp + 16?
 */
.globl _KiSetupStackAndInitializeKernel
.func KiSetupStackAndInitializeKernel
_KiSetupStackAndInitializeKernel:

    /* Save current stack */
    mov rsi, rsp

    /* Setup the new stack */
    mov rsp, r8
    sub rsp, NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH
    push CR0_EM + CR0_TS + CR0_MP

    /* Copy stack parameters to the new stack */
    push [rsi + 16]
    push [rsi + 8]
    xor rbp, rbp
    call _KiInitializeKernel

    /* Set the priority of this thread to 0 */
    mov rbx, PCR[KPCR_CURRENT_THREAD]
    mov byte ptr [rbx+KTHREAD_PRIORITY], 0

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    sti
    mov rcx, DISPATCH_LEVEL
    call _KfLowerIrql

    /* Set the right wait IRQL */
    mov byte ptr [rbx+KTHREAD_WAIT_IRQL], DISPATCH_LEVEL;

    /* Jump into the idle loop */
    jmp _KiIdleLoop
.endfunc
