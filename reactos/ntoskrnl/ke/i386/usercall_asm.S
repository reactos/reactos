/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/i386/usercall_asm.S
 * PURPOSE:         User-Mode callbacks and return.
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
//#include <bugcodes.h>
#define APC_INDEX_MISMATCH 1
#define IRQL_GT_ZERO_AT_SYSTEM_SERVICE 0x4A
#define STATUS_NO_CALLBACK_ACTIVE 0xC0000258
.intel_syntax noprefix

// This file is a work in progress. Most of the code is currently disabled.

/* GLOBALS ****************************************************************/
.extern PVOID _KeUserCallbackDispatcher

/* FUNCTIONS ****************************************************************/

.globl _KiGetUserModeStackAddress@0
.func KiGetUserModeStackAddress@0
_KiGetUserModeStackAddress@0:

    /* Get the current thread's trapframe and return the esp */
    mov eax, fs:[KPCR_CURRENT_THREAD]
    mov eax, [eax+KTHREAD_TRAP_FRAME]
    lea eax, [eax+KTRAP_FRAME_ESP]
    ret

.endfunc

/*++
 * @name KiCallUserMode 
 *
 *     The KiSwitchToUserMode routine sets up a Trap Frame and a Callback stack
 *     for the purpose of switching to user mode. The actual final jump is done
 *     by KiServiceExit which will treat this as a syscall return.
 *
 * @param OutputBuffer
 *        Pointer to a caller-allocated buffer where to receive the return data
 *        from the user-mode function
 *
 * @param OutputLength
 *        Size of the Output Buffer described above.
 *
 * @return None. Jumps into KiServiceExit.
 *
 * @remark If there is not enough Kernel Stack space, the routine will increase the
 *         Kernel Stack.
 *
 *         User mode execution resumes at ntdll!KiUserCallbackDispatcher.
 *
 *         This call MUST be paired by interrupt 0x2B or NtCallbackReturn.
 *
 *--*/
.globl _KiCallUserMode@8
.func KiCallUserMode@8
_KiCallUserMode@8:

    /* Save volatile registers */
    push ebp
    push ebx
    push esi
    push edi

    /* Get the current thread */
    mov ebx, fs:[KPCR_CURRENT_THREAD]

    /* Make sure we're at passive */
#if DBG
    call _KeGetCurrentIrql@0
    or al, al
    jz AtPassive

    /* We're not, bugcheck! */
    push 0
    push 0
    push eax
    push 0
    push IRQL_GT_ZERO_AT_SYSTEM_SERVICE
    call _KeBugCheckEx@20

AtPassive:

    /* Make sure that we are not attached and that APCs are not disabled */
    movzx eax, byte ptr [ebx+KTHREAD_APC_STATE_INDEX]
    mov edx, [ebx+KTHREAD_COMBINED_APC_DISABLE]
    or eax, eax
    jnz InvalidIndex
    or edx, edx
    jz ApcsEnabled

InvalidIndex:

    push 0
    push edx
    push eax
    push 0
    push APC_INDEX_MISMATCH
    call _KeBugCheckEx@20

ApcsEnabled:
#endif

    /* Get the lowest stack limit and check if we can handle it */
    lea eax, [esp-0x3000]
    cmp eax, [ebx+KTHREAD_STACK_LIMIT]
    jnb StackOk

    /* We can't, we'll have to grow our stack */
    push esp
    call _MmGrowKernelStack@4

    /* Quit if we failed */
    or eax, eax
    jnz GrowFailed

    /* Save the current callback stack */
StackOk:
    push [ebx+KTHREAD_CALLBACK_STACK]

    /* Get and save the trap frame */
    mov edx, [ebx+KTHREAD_TRAP_FRAME]
    push edx

    /* Get and save the initial stack */
    mov esi, [ebx+KTHREAD_INITIAL_STACK]
    push esi

    /* Set the new callback stack */
    mov [ebx+KTHREAD_CALLBACK_STACK], esp

    /* Align stack on 16-byte boundary */
    and esp, ~15
    mov edi, esp

    /* Set destination and origin NPX Areas */
    sub esp, NPX_FRAME_LENGTH
    sub esi, NPX_FRAME_LENGTH

    /* Disable interrupts so we can fill the NPX State */
    cli

    /* Now copy the NPX State */
    mov ecx, [esi+FN_CONTROL_WORD]
    mov [esp+FN_CONTROL_WORD], ecx
    mov ecx, [esi+FN_STATUS_WORD]
    mov [esp+FN_STATUS_WORD], ecx
    mov ecx, [esi+FN_TAG_WORD]
    mov [esp+FN_TAG_WORD], ecx
    mov ecx, [esi+FN_DATA_SELECTOR]
    mov [esp+FN_DATA_SELECTOR], ecx
    mov ecx, [esi+FN_CR0_NPX_STATE]
    mov [esp+FN_CR0_NPX_STATE], ecx

    /* Get TSS */
    mov esi, fs:[KPCR_TSS]

    /* Set the stack address */
    mov [ebx+KTHREAD_INITIAL_STACK], edi

    /* Bias the stack for V86 mode */
    mov ecx, esp
    sub esp, 16
    test dword ptr [edx+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz DontBias
    mov ecx, esp

DontBias:
    /* Set new stack address in TSS */
    mov [esi+KTSS_ESP0], ecx

    /* Allocate the trap frame and set it */
    sub esp, KTRAP_FRAME_V86_ES
    mov ebp, esp

    /* Set copy iterator and dest/origin parameters and do the copy */
    mov ecx, (KTRAP_FRAME_V86_ES - KTRAP_FRAME_FS) / 4
    lea edi, [esp+KTRAP_FRAME_FS]
    lea esi, [edx+KTRAP_FRAME_FS]
    rep movsd

    /* FIXME: Copy debug registers if needed */

    /* Get user-mode dispatcher address and set it as EIP */
    mov eax, _KeUserCallbackDispatcher
    mov [esp+KTRAP_FRAME_EIP], eax

    /* Set the exception list */
    mov eax, fs:[KPCR_EXCEPTION_LIST]
    mov [esp+KTRAP_FRAME_EXCEPTION_LIST], eax

    /* Set the previous mode */
    mov eax, [edx+KTRAP_FRAME_PREVIOUS_MODE]
    mov [esp+KTRAP_FRAME_PREVIOUS_MODE], eax

    /* Bring interrupts back */
    sti

    /* Write the debug data */
    mov edi, [ebp+KTRAP_FRAME_EBP]
    mov edx, [ebp+KTRAP_FRAME_EIP]
    mov [ebp+KTRAP_FRAME_DEBUGPOINTER], edx
    mov dword ptr [ebp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00
    mov [ebp+KTRAP_FRAME_DEBUGEBP], ebx
    mov [ebp+KTRAP_FRAME_DEBUGEIP], edi

    /* Exit to user-mode */
    jmp _KiServiceExit

GrowFailed:
    /* Restore registers */
    pop edi
    pop esi
    pop ebx
    pop ebp

    /* Return */
    ret 8

.endfunc

/*++
 * @name NtCallbackReturn
 *
 *     The NtCallbackReturn routine returns to kernel mode after a user-mode
 *     callback was done through KeUserModeCallback. It uses the callback frame
 *     which was setup in order to return the information, restores the stack,
 *     and resumes execution where it was left off.
 *
 * @param Result
 *        Pointer to a caller-allocated buffer where the return data
 *               from the user-mode function is located.
 *
 * @param ResultLength
 *        Size of the Output Buffer described above.
 *
 * @param CallbackStatus
 *        Status code of the callback operation.
 *
 * @return Status code of the callback operation.
 *
 * @remark This call MUST be paired with KeUserModeCallback.
 *
 *--*/
.globl _NtCallbackReturn@12
.func NtCallbackReturn@12
_NtCallbackReturn@12:

    /* Get the current thread and make sure we have a callback stack */
    mov eax, fs:[KPCR_CURRENT_THREAD]
    mov ecx, [eax+KTHREAD_CALLBACK_STACK]
    test ecx, ecx
    jz NoStack

    /* Get the trap frame */
    mov ebx, [eax+KTHREAD_TRAP_FRAME]

    /* Restore the exception list */
    mov edx, [ebx+KTRAP_FRAME_EXCEPTION_LIST]
    mov fs:[KPCR_EXCEPTION_LIST], edx

    /* Get the result, the result length and the status */
    mov edi, [esp+4]
    mov esi, [esp+8]
    mov ebp, [esp+12]

    /* Store the results in the callback stack */
    mov ebx, [ecx+CBSTACK_RESULT]
    mov [ebx], edi
    mov ebx, [ecx+CBSTACK_RESULT_LENGTH]
    mov [ebx], esi

    /* Get the previous stack */
    mov ebx, [ecx]

    /* Disable interrupts for NPX save and stack switch */
    cli

    /* Get the initial stack and restore it */
    mov esi, [eax+KTHREAD_INITIAL_STACK]
    mov [eax+KTHREAD_INITIAL_STACK], ebx

    /* Set desination and origin NPX Frames */
    sub esi, NPX_FRAME_LENGTH
    sub ebx, NPX_FRAME_LENGTH

    /* Copy NPX Data */
    mov edx, [esi+FN_CONTROL_WORD]
    mov [ebx+FN_CONTROL_WORD], edx
    mov edx, [esi+FN_STATUS_WORD]
    mov [ebx+FN_STATUS_WORD], edx
    mov edx, [esi+FN_TAG_WORD]
    mov [ebx+FN_TAG_WORD], edx
    mov edx, [esi+FN_DATA_SELECTOR]
    mov [ebx+FN_DATA_SELECTOR], edx
    mov edx, [esi+FN_CR0_NPX_STATE]
    mov [ebx+FN_CR0_NPX_STATE], edx

    /* Get saved trap frame and clear DR7 */
    mov edi, [ecx+CBSTACK_TRAP_FRAME]
    and dword ptr [edi+KTRAP_FRAME_DR7], 0

    /* FIXME: Restore debug regs */

    /* Get TSS */
    mov edx, fs:[KPCR_TSS]

    /* Restore stack pointer */
    lea esp, [ecx+CBSTACK_CALLBACK_STACK]

    /* Check if we were in V86 mode */
    test dword ptr [edi+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Ret
    sub ebx, 16

V86Ret:
    /* Restore the ESP in TSS */
    mov [edx+KTSS_ESP0], ebx

    /* Restore the trap frame */
    mov [eax+KTHREAD_TRAP_FRAME], edi

    /* Bring interrupts back */
    sti

    /* Restore the callback stack*/
    pop [eax+KTHREAD_CALLBACK_STACK]

    /* Set status and return */
    mov eax, ebp
    pop edi
    pop esi
    pop ebx
    pop ebp
    pop edx

    /* Clean stack and jump back */
    add esp, 8
    jmp edx

NoStack:

    /* Return failure */
    mov eax, STATUS_NO_CALLBACK_ACTIVE
    ret 12

.endfunc

.globl _KeSwitchKernelStack@8
.func KeSwitchKernelStack@8
_KeSwitchKernelStack@8:

    /* Save volatiles */
    push esi
    push edi

    /* Get current thread */
    mov edx, fs:[KPCR_CURRENT_THREAD]

    /* Get new and current base */
    mov edi, [esp+12]
    mov ecx, [edx+KTHREAD_STACK_BASE]

    /* Fixup the frame pointer */
    sub ebp, ecx
    add ebp, edi

    /* Fixup the trap frame */
    mov eax, [edx+KTHREAD_TRAP_FRAME]
    sub eax, ecx
    add eax, edi
    mov [edx+KTHREAD_TRAP_FRAME], eax

    /* Calculate stack size */
    sub ecx, esp

    /* Get desination and origin */
    sub edi, ecx
    mov esi, esp

    /* Save stack pointer */
    push edi

    /* Copy stack */
    rep movsb

    /* Restore stack pointer */
    pop edi

    /* Save old stack base and get new limit/base */
    mov eax, [edx+KTHREAD_STACK_BASE]
    mov ecx, [esp+12]
    mov esi, [esp+16]

    /* Disable interrupts for stack switch */
    cli

    /* Set new base/limit */
    mov [edx+KTHREAD_STACK_BASE], ecx
    mov [edx+KTHREAD_STACK_LIMIT], esi

    /* Set LargeStack */
    mov byte ptr [edx+KTHREAD_LARGE_STACK], 1

    /* Set new initial stack */
    mov [edx+KTHREAD_INITIAL_STACK], ecx

    /* Get trap frame */
    mov esi, [edx+KTHREAD_TRAP_FRAME]

    /* Get TSS */
    mov edx, fs:[KPCR_TSS]

    /* Check if we came from V86 mode */
    test dword ptr [esi+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK

    /* Bias for NPX Area */
    lea ecx, [ecx-NPX_FRAME_LENGTH]
    jnz V86Switch
    sub ecx, 16

V86Switch:

    /* Update ESP in TSS */
    mov [edx+KTSS_ESP0], ecx

    /* Update stack pointer */
    mov esp, edi

    /* Bring back interrupts and return */
    sti
    pop edi
    pop esi
    ret 8

.endfunc
