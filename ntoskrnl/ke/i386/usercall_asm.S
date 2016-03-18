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

    /* Allocate space for the initial stack */
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



PUBLIC @KiCallbackReturn@8
@KiCallbackReturn@8:

    /* Restore the stack */
    mov esp, ecx

    /* Set status and return */
    mov eax, edx
    pop edi
    pop esi
    pop ebx
    pop ebp

    /* Clean stack and return */
    ret 8

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
