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
.globl _KiSystemStartup
.func KiSystemStartup
_KiSystemStartup:

    /* NTLDR Boot: Call the main kernel initialization */
    test dword ptr [esp+4], 0x80000000
    jnz _KiSystemStartupReal@4

    /* FREELDR Boot: Call the FreeLDR wrapper */
    jmp @KiRosPrepareForSystemStartup@8
.endfunc

.globl _KiSetupStackAndInitializeKernel@24
.func KiSetupStackAndInitializeKernel@24
_KiSetupStackAndInitializeKernel@24:

    /* Save current stack */
    mov esi, esp

    /* Setup the new stack */
    mov esp, [esp+12]
    sub esp, NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH
    push CR0_EM + CR0_TS + CR0_MP

    /* Copy all parameters to the new stack */
    push [esi+24]
    push [esi+20]
    push [esi+16]
    push [esi+12]
    push [esi+8]
    push [esi+4]
    xor ebp, ebp
    call _KiInitializeKernel@24

    /* Set the priority of this thread to 0 */
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov byte ptr [ebx+KTHREAD_PRIORITY], 0

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    sti
    mov ecx, DISPATCH_LEVEL
    call @KfLowerIrql@4

    /* Set the right wait IRQL */
    mov byte ptr [ebx+KTHREAD_WAIT_IRQL], DISPATCH_LEVEL;

    /* Jump into the idle loop */
    jmp @KiIdleLoop@0
.endfunc
