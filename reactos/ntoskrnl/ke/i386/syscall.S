/*
 * FILE:            ntoskrnl/ke/i386/syscall.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Call Handler
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#include <asm.h>
#include <internal/i386/asmmacro.S>

.globl _KiServiceExit
.globl _KiServiceExit2
.globl _KiFastCallEntry
.globl _KiSystemService
.globl _KiDebugService
.intel_syntax noprefix

/*
  * FIXMEs:
  *         - Figure out why ES/DS gets messed up in VMWare, when doing KiServiceExit only,
  *           and only when called from user-mode, and returning to user-mode.
  *         - Implement SYSCALL_PROLOG macro.
  */

 /*** This file is a mess; it is being worked on. Please contact Alex:
  *** alex@relsoft.net if you want to make any changes to it before this
  *** message goes away
  */

/* FUNCTIONS ***************************************************************/

BadStack:

    /* Restore ESP0 stack */
    mov ecx, [fs:KPCR_TSS]
    mov esp, ss:[ecx+KTSS_ESP0]

    /* Generate V86M Stack for Trap 6 */
    push 0
    push 0
    push 0
    push 0

    /* Generate interrupt stack for Trap 6 */
    push KGDT_R3_DATA + RPL_MASK
    push 0
    push 0x20202
    push KGDT_R3_CODE + RPL_MASK
    push 0
    jmp _KiTrap6

.func KiFastCallEntry
_KiFastCallEntry:

    /* Set FS to PCR */
    mov ecx, KGDT_R0_PCR
    mov fs, cx

    /* Set DS/ES to Kernel Selector */
    mov ecx, KGDT_R0_DATA
    mov ds, cx
    mov es, cx

    /* Set the current stack to Kernel Stack */
    mov ecx, [fs:KPCR_TSS]
    mov esp, ss:[ecx+KTSS_ESP0]

    /* Set up a fake INT Stack. */
    push KGDT_R3_DATA + RPL_MASK
    push edx                            /* Ring 3 SS:ESP */
    pushf                               /* Ring 3 EFLAGS */
    push 2                              /* Ring 0 EFLAGS */
    add edx, 8                          /* Skip user parameter list */
    popf                                /* Set our EFLAGS */
    or dword ptr [esp], EFLAGS_INTERRUPT_MASK   /* Re-enable IRQs in EFLAGS, to fake INT */
    push KGDT_R3_CODE + RPL_MASK
    push KUSER_SHARED_SYSCALL_RET

    /* Setup the Trap Frame stack */
    push 0
    push ebp
    push ebx
    push esi
    push edi
    push KGDT_R3_TEB + RPL_MASK

    /* Save pointer to our PCR */
    mov ebx, [fs:KPCR_SELF]

    /* Get a pointer to the current thread */
    mov esi, [ebx+KPCR_CURRENT_THREAD]

    /* Set the exception handler chain terminator */
    push [ebx+KPCR_EXCEPTION_LIST]
    mov dword ptr [ebx+KPCR_EXCEPTION_LIST], -1

    /* Use the thread's stack */
    mov ebp, [esi+KTHREAD_INITIAL_STACK]

    /* Push previous mode */
    push UserMode

    /* Skip the other registers */
    sub esp, 0x48

    /* Hack: it seems that on VMWare someone damages ES/DS on exit. Investigate! */
    mov dword ptr [esp+KTRAP_FRAME_DS], KGDT_R3_DATA + RPL_MASK
    mov dword ptr [esp+KTRAP_FRAME_ES], KGDT_R3_DATA + RPL_MASK

    /* Make space for us on the stack */
    sub ebp, 0x29C

    /* Write the previous mode */
    mov byte ptr [esi+KTHREAD_PREVIOUS_MODE], UserMode

    /* Sanity check */
    cmp ebp, esp
    jnz BadStack

    /* Flush DR7 */
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0

    /* Check if the thread was being debugged */
    test byte ptr [esi+KTHREAD_DEBUG_ACTIVE], 0xFF

    /* Jump to shared code or DR Save */
    //jnz Dr_FastCallDrSave
    jmp SharedCode
.endfunc

.func KiSystemService
_KiSystemService:

// ==================== COMMON SYSTEM CALL CODE ============//
    /* Create a trap frame */
    push 0
    push ebp
    push ebx
    push esi
    push edi
    push fs

    /* Load PCR Selector into fs */
    mov ebx, KGDT_R0_PCR
    mov fs, bx

    /* Get a pointer to the current thread */
    mov esi, [fs:KPCR_CURRENT_THREAD]

    /* Save the previous exception list */
    push [fs:KPCR_EXCEPTION_LIST]

    /* Set the exception handler chain terminator */
    mov dword ptr [fs:KPCR_EXCEPTION_LIST], -1

    /* Save the old previous mode */
    push ss:[esi+KTHREAD_PREVIOUS_MODE]
    
    /* Skip the other registers */
    sub esp, 0x48

    /* Hack: it seems that on VMWare someone damages ES/DS on exit. Investigate! */
    mov [esp+KTRAP_FRAME_DS], ds
    mov [esp+KTRAP_FRAME_ES], es

    /* Set the new previous mode based on the saved CS selector */
    mov ebx, [esp+0x6C]
    and ebx, 1
    mov byte ptr ss:[esi+KTHREAD_PREVIOUS_MODE], bl

    /* Go on the Kernel stack frame */
    mov ebp, esp

    /* Save the old trap frame pointer where EDX would be saved */
    mov ebx, [esi+KTHREAD_TRAP_FRAME]
    mov [ebp+KTRAP_FRAME_EDX], ebx

    /* Flush DR7 */
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0

    /* Check if the thread was being debugged */
    test byte ptr [esi+KTHREAD_DEBUG_ACTIVE], 0xFF
    //jnz Dr_kss_a

SharedCode:
    mov [esi+KTHREAD_TRAP_FRAME], ebp

    /* Set the trap frame debug header */
    SET_TF_DEBUG_HEADER

#ifdef DBG // FIXME: Is this for GDB? Can it be moved in the stub?
    /*
     * We want to know the address from where the syscall stub was called.
     * If PrevMode is KernelMode, that address is stored in our own (kernel)
     * stack, at location KTRAP_FRAME_ESP.
     * If we're coming from UserMode, we load the usermode stack pointer
     * and go back two frames (first frame is the syscall stub, second call
     * is the caller of the stub).
     */
    mov edi, [ebp+KTRAP_FRAME_ESP]
    test byte ptr [esi+KTHREAD_PREVIOUS_MODE], 0x01
    jz PrevWasKernelMode
    mov edi, [edi+4]
PrevWasKernelMode:
    mov [ebp+KTRAP_FRAME_DEBUGEIP], edi
#endif

    /* Enable interrupts */
    sti

CheckValidCall:

    /*
     * Find out which table offset to use. Converts 0x1124 into 0x10.
     * The offset is related to the Table Index as such: Offset = TableIndex x 10
     */
    mov edi, eax
    shr edi, SERVICE_TABLE_SHIFT
    and edi, SERVICE_TABLE_MASK
    mov ecx, edi

    /* Now add the thread's base system table to the offset */
    add edi, [esi+KTHREAD_SERVICE_TABLE]

    /* Get the true syscall ID and check it */
    mov ebx, eax
    and eax, SERVICE_NUMBER_MASK
    cmp eax, [edi+SERVICE_DESCRIPTOR_LIMIT]

    /* Invalid ID, try to load Win32K Table */
    jnb KiBBTUnexpectedRange

#if 0 // <== Disabled for two reasons: We don't save TEB in 0x18, but KPCR.
      // <== We don't have a KeGdiFlushUserBatch callback yet (needs to be
      //     sent through the PsInitializeWin32Callouts structure)
    /* Check if this was Win32K */
    cmp ecx, SERVICE_TABLE_TEST
    jnz NotWin32K

    /* Get the TEB */
    mov ecx, [fs:KPCR_TEB]

    /* Check if we should flush the User Batch */
    xor ebx, ebx
    or ebx, [ecx+TEB_GDI_BATCH_COUNT]
    jz NoWin32K

    /* Flush it */
    push edx
    push eax
    call [_KeGdiFlushUserBatch]
    pop eax
    pop edx
#endif

NotWin32K:
    /* Increase total syscall count */
    inc dword ptr fs:[KPCR_SYSTEM_CALLS]

#ifdef DBG
    /* Increase per-syscall count */
    mov ecx, [edi+SERVICE_DESCRIPTOR_COUNT]
    jecxz NoCountTable
    inc dword ptr [ecx+eax*4]
#endif

    /* Users's current stack frame pointer is source */
NoCountTable:
    mov esi, edx

    /* Allocate room for argument list from kernel stack */
    mov ebx, [edi+SERVICE_DESCRIPTOR_NUMBER]
    xor ecx, ecx
    mov cl, [eax+ebx]

    /* Get pointer to function */
    mov edi, [edi+SERVICE_DESCRIPTOR_BASE]
    mov ebx, [edi+eax*4]

    /* Allocate space on our stack */
    sub esp, ecx

    /* 
     * Copy the arguments from the user stack to our stack
     * FIXME: This needs to be probed with MmSystemRangeStart
     */
    shr ecx, 2
    mov edi, esp
    rep movsd

#ifdef DBG

    /* Make sure this isn't a user-mode call at elevated IRQL */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz SkipCheck
    call _KeGetCurrentIrql@0
    or al, al
    jnz InvalidIrql

    /*
     * The following lines are for the benefit of GDB. It will see the return
     * address of the "call ebx" below, find the last label before it and
     * thinks that that's the start of the function. It will then check to see
     * if it starts with a standard function prolog (push ebp, mov ebp,esp).
     * When that standard function prolog is not found, it will stop the
     * stack backtrace. Since we do want to backtrace into usermode, let's
     * make GDB happy and create a standard prolog.
     */
SkipCheck:
KiSystemService:
    push ebp
    mov ebp,esp
    pop ebp
#endif

    /* Do the System Call */
    call ebx

#ifdef DBG
    /* Make sure the user-mode call didn't return at elevated IRQL */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz SkipCheck2
    mov esi, eax                /* We need to save the syscall's return val */
    call _KeGetCurrentIrql@0
    or al, al
    jnz InvalidIrql
    mov eax, esi                /* Restore it */

    /* Get our temporary current thread pointer for sanity check */
    mov ecx, fs:[KPCR_CURRENT_THREAD]

    /* Make sure that we are not attached and that APCs are not disabled */
    mov dl, [ecx+KTHREAD_APC_STATE_INDEX]
    or dl, dl
    jnz InvalidIndex
    mov edx, [ecx+KTHREAD_COMBINED_APC_DISABLE]
    or edx, edx
    jnz InvalidIndex
#endif

SkipCheck2:

    /* Deallocate the kernel stack frame  */
    mov esp, ebp

KeReturnFromSystemCall:

    /* Get the Current Thread */
    mov ecx, [fs:KPCR_CURRENT_THREAD]

    /* Restore the old trap frame pointer */
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ecx+KTHREAD_TRAP_FRAME], edx
.endfunc

.func KiServiceExit
_KiServiceExit:
    /* Disable interrupts */
    cli

    /* Check for, and deliver, User-Mode APCs if needed */
    CHECK_FOR_APC_DELIVER 1

    /* Hack for VMWare: Sometimes ES/DS seem to be invalid when returning to user-mode. Investigate! */
    mov es, [ebp+KTRAP_FRAME_ES]
    mov ds, [ebp+KTRAP_FRAME_DS]

    /* Exit and cleanup */
    TRAP_EPILOG FromSystemCall, DoRestorePreviousMode, DoNotRestoreSegments, DoNotRestoreVolatiles, DoRestoreEverything
.endfunc

KiBBTUnexpectedRange:

    /* If this isn't a Win32K call, fail */
    cmp ecx, 0x10
    jne InvalidCall

    /* Set up Win32K Table */
    push edx
    push ebx
    call _PsConvertToGuiThread@0

    /* FIXME: Handle failure */
    pop eax
    pop edx

    /* Reset trap frame address */
    mov ebp, esp
    mov [esi+KTHREAD_TRAP_FRAME], ebp

    /* Try the Call again */
    jmp CheckValidCall

InvalidCall:

    /* Invalid System Call */
    mov eax, STATUS_INVALID_SYSTEM_SERVICE
    jmp KeReturnFromSystemCall
    
#ifdef DBG
InvalidIrql:
    /* Save current IRQL */
    push fs:[KPCR_IRQL]

    /* Set us at passive */
    mov dword ptr fs:[KPCR_IRQL], 0
    cli

    /* Bugcheck */
    push 0
    push 0
    push eax
    push ebx
    push IRQL_GT_ZERO_AT_SYSTEM_SERVICE
    call _KeBugCheckEx@20

InvalidIndex:

    /* Get the index and APC state */
    movzx eax, byte ptr [ecx+KTHREAD_APC_STATE_INDEX]
    mov edx, [ecx+KTHREAD_COMBINED_APC_DISABLE]

    /* Bugcheck */
    push 0
    push edx
    push eax
    push ebx
    push APC_INDEX_MISMATCH
    call _KeBugCheckEx@20
    ret
#endif

.func KiServiceExit2
_KiServiceExit2:

    /* Disable interrupts */
    cli

    /* Check for, and deliver, User-Mode APCs if needed */
    CHECK_FOR_APC_DELIVER 0

    /* Exit and cleanup */
    TRAP_EPILOG NotFromSystemCall, DoRestorePreviousMode, DoRestoreSegments, DoRestoreVolatiles, DoNotRestoreEverything
.endfunc

.globl _Kei386EoiHelper@0
.func Kei386EoiHelper@0
_Kei386EoiHelper@0:

    /* Disable interrupts */
    cli

    /* Check for, and deliver, User-Mode APCs if needed */
    CHECK_FOR_APC_DELIVER 0

    /* Exit and cleanup */
    TRAP_EPILOG NotFromSystemCall, DoNotRestorePreviousMode, DoRestoreSegments, DoRestoreVolatiles, DoNotRestoreEverything
.endfunc

V86_Exit:
    /* Move to EDX position */
    add esp, KTRAP_FRAME_EDX

    /* Restore volatiles */
    pop edx
    pop ecx
    pop eax

    /* Move to non-volatiles */
    lea esp, [ebp+KTRAP_FRAME_EDI]
    pop edi
    pop esi
    pop ebx
    pop ebp

    /* Skip error code and return */
    add esp, 4
    iret

AbiosExit:
    /* Not yet supported */
    int 3

_KiDebugService:

    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(kids)

    /* Increase EIP so we skip the INT3 */
    //inc dword ptr [ebp+KTRAP_FRAME_EIP]

    /* Call debug service dispatcher */
    mov eax, [ebp+KTRAP_FRAME_EAX]
    mov ecx, [ebp+KTRAP_FRAME_ECX]
    mov edx, [ebp+KTRAP_FRAME_EAX]

    /* Check for V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz NotUserMode

    /* Check if this is kernel or user-mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], 1
    jz CallDispatch
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jnz NotUserMode

    /* Re-enable interrupts */
VdmProc:
    sti

    /* Call the debug routine */
CallDispatch:
    mov esi, ecx
    mov edi, edx
    mov edx, eax
    mov ecx, 3
    push edi
    push esi
    push edx
    call _KdpServiceDispatcher@12

NotUserMode:

    /* Get the current process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]

    /* Check if this is a VDM Process */
    //cmp dword ptr [ebx+KPROCESS_VDM_OBJECTS], 0
    //jz VdmProc

    /* Exit through common routine */
    jmp _Kei386EoiHelper@0

.globl _NtRaiseException@12
_NtRaiseException@12:

    /* NOTE: We -must- be called by Zw* to have the right frame! */
    /* Push the stack frame */
    push ebp

    /* Get the current thread and restore its trap frame */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ebx+KTHREAD_TRAP_FRAME], edx

    /* Set up stack frame */
    mov ebp, esp

    /* Get the Trap Frame in EBX */
    mov ebx, [ebp+0]

    /* Get the exception list and restore */
    mov eax, [ebx+KTRAP_FRAME_EXCEPTION_LIST]
    mov [fs:KPCR_EXCEPTION_LIST], eax

    /* Get the parameters */
    mov edx, [ebp+16] /* Search frames */
    mov ecx, [ebp+12] /* Context */
    mov eax, [ebp+8]  /* Exception Record */

    /* Raise the exception */
    push edx
    push ebx
    push 0
    push ecx
    push eax
    call _KiRaiseException@20

    /* Restore trap frame in EBP */
    pop ebp
    mov esp, ebp

    /* Check the result */
    or eax, eax
    jz _KiServiceExit2

    /* Restore debug registers too */
    jmp _KiServiceExit

.globl _NtContinue@8
_NtContinue@8:

    /* NOTE: We -must- be called by Zw* to have the right frame! */
    /* Push the stack frame */
    push ebp

    /* Get the current thread and restore its trap frame */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ebx+KTHREAD_TRAP_FRAME], edx

    /* Set up stack frame */
    mov ebp, esp

    /* Save the parameters */
    mov eax, [ebp+0]
    mov ecx, [ebp+8]

    /* Call KiContinue */
    push eax
    push 0
    push ecx
    call _KiContinue@12

    /* Check if we failed (bad context record) */
    or eax, eax
    jnz Error

    /* Check if test alert was requested */
    cmp dword ptr [ebp+12], 0
    je DontTest

    /* Test alert for the thread */
    mov al, [ebx+KTHREAD_PREVIOUS_MODE]
    push eax
    call _KeTestAlertThread@4

DontTest:
    /* Return to previous context */
    pop ebp
    mov esp, ebp
    jmp _KiServiceExit2

Error:
    pop ebp
    mov esp, ebp
    jmp _KiServiceExit

