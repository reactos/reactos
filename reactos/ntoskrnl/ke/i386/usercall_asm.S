/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/i386/usercall_asm.S
 * PURPOSE:         User-Mode callbacks and return.
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>
#include <internal/i386/asmmacro.S>

EXTERN _MmGrowKernelStack@4:PROC
EXTERN _KeUserCallbackDispatcher:DWORD
EXTERN @KiServiceExit@8:PROC
EXTERN _KeGetCurrentIrql@0:PROC
EXTERN _KeBugCheckEx@20:PROC
EXTERN @KiUserModeCallout@4:PROC

/* FUNCTIONS ****************************************************************/
.code

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
PUBLIC _KiCallUserMode@8
_KiCallUserMode@8:

    /* Push non-volatile registers on the stack.
       This is part of the KCALLOUT_FRAME */
    push ebp
    push ebx
    push esi
    push edi

    /* load the address of the callout frame into ecx */
    lea ecx, [esp - 12]

    /* Allocate space for the inital stack */
    sub esp, 12 + NPX_FRAME_LENGTH + KTRAP_FRAME_LENGTH + 16

    call @KiUserModeCallout@4

    add esp, 12 + NPX_FRAME_LENGTH + KTRAP_FRAME_LENGTH + 16

    /* Restore registers */
    pop edi
    pop esi
    pop ebx
    pop ebp

    /* Return */
    ret 8


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
PUBLIC _NtCallbackReturn@12
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
    mov edx, [esi+FP_CONTROL_WORD]
    mov [ebx+FP_CONTROL_WORD], edx
    mov edx, [esi+FP_STATUS_WORD]
    mov [ebx+FP_STATUS_WORD], edx
    mov edx, [esi+FP_TAG_WORD]
    mov [ebx+FP_TAG_WORD], edx
    mov edx, [esi+FP_DATA_SELECTOR]
    mov [ebx+FP_DATA_SELECTOR], edx
    mov edx, [esi+FN_CR0_NPX_STATE]
    mov [ebx+FN_CR0_NPX_STATE], edx

    /* Check if we failed in user mode */
    cmp ebp, STATUS_CALLBACK_POP_STACK
    mov edi, [ecx+CBSTACK_TRAP_FRAME]
    jz UserFault

CheckDebug:

    /* Clear DR7 */
    and dword ptr [edi+KTRAP_FRAME_DR7], 0

    /* Check if debugging was active */
    test byte ptr [eax+KTHREAD_DEBUG_ACTIVE], HEX(0FF)
    jnz RestoreDebug

RestoreStack:

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

UserFault:
    /* Set size to copy */
    mov ecx, (KTRAP_FRAME_V86_ES - KTRAP_FRAME_FS) / 4

    /* Check if this was V86 mode */
    mov esi, [eax+KTHREAD_TRAP_FRAME]
    test dword ptr [esi+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK

    /* Save EDI and load destination */
    mov edx, edi
    lea edi, [edi+KTRAP_FRAME_FS]
    jz NotV86
    add ecx, 16 / 4

NotV86:
    /* Set source and copy */
    lea esi, [esi+KTRAP_FRAME_FS]
    rep movsd

    /* Restore ECX and ECX */
    mov ecx, [eax+KTHREAD_CALLBACK_STACK]
    mov edi, edx
    jmp CheckDebug

RestoreDebug:
    /* Get a pointer to thread's trap frame */
    mov esi, [eax+KTHREAD_TRAP_FRAME]

    /* Copy debug registers data from it */
    mov edx, [esi+KTRAP_FRAME_DR0]
    mov [edi+KTRAP_FRAME_DR0], edx
    mov edx, [esi+KTRAP_FRAME_DR1]
    mov [edi+KTRAP_FRAME_DR1], edx
    mov edx, [esi+KTRAP_FRAME_DR2]
    mov [edi+KTRAP_FRAME_DR2], edx
    mov edx, [esi+KTRAP_FRAME_DR3]
    mov [edi+KTRAP_FRAME_DR3], edx
    mov edx, [esi+KTRAP_FRAME_DR6]
    mov [edi+KTRAP_FRAME_DR6], edx
    mov edx, [esi+KTRAP_FRAME_DR7]
    mov [edi+KTRAP_FRAME_DR7], edx

    /* Jump back */
    jmp RestoreStack

NoStack:

    /* Return failure */
    mov eax, STATUS_NO_CALLBACK_ACTIVE
    ret 12

/*++
 * @name KeSwitchKernelStack
 *
 *     The KeSwitchKernelStack routine switches from the current thread's stack
 *     to the new specified base and limit.
 *
 * @param StackBase
 *        Pointer to the new Stack Base of the thread.
 *
 * @param StackLimit
 *        Pointer to the new Stack Limit of the thread.
 *
 * @return The previous Stack Base of the thread.
 *
 * @remark This routine should typically only be used when converting from a
 *         non-GUI to a GUI Thread. The caller is responsible for freeing the
 *         previous stack. The new stack values MUST be valid before calling
 *         this routine.
 *
 *--*/
PUBLIC _KeSwitchKernelStack@8
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

END
