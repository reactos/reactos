/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/i386/asmmacro.S
 * PURPOSE:         Assembly Macros for Spinlocks and common Trap Code
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */
 
/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

// Arguments for TRAP_EPILOG
#define FromSystemCall              1
#define DoRestorePreviousMode       1
#define DoRestoreEverything         1
#define DoRestoreSegments           1
#define DoRestoreVolatiles          1
#define NotFromSystemCall           0
#define DoNotRestorePreviousMode    0
#define DoNotRestoreEverything      0
#define DoNotRestoreSegments        0
#define DoNotRestoreVolatiles       0

.intel_syntax noprefix

//
// These macros are inlined equivalents of KiAcquire/ReleaseSpinlock, that is,
// they will not be compiled into non-SMP builds. Usage is as follows:
//
// .BeginYourFunction
//      mov reg, lockaddr
//      ACQUIRE_SPINLOCK(reg, .spin)
//      <thread-safe code here>
//      RELEASE_SPINLOCK(reg)
//      <misc code here>
//  retn
//  #IFDEF CONFIG_SMP
//  .spin
//      <any necessary steps to be able to jump back safely>
/       SPIN_ON_LOCK(reg, .BeginYourFunction)
//  #ENDIF
//
#ifdef CONFIG_SMP
#define LOCK lock
#define ACQUIRE_SPINLOCK(x, y) \
    lock bts dword ptr [x], 0; \
    jb y
#define RELEASE_SPINLOCK(x) mov byte ptr [x], 0
#define SPIN_ON_LOCK(x, y) \
1: \
    test dword ptr [x], 1; \
    jz y; \
    pause; \
    jmp 1b
#else
#define LOCK
#define ACQUIRE_SPINLOCK(x, y)
#define RELEASE_SPINLOCK(x) 
#endif

//
// @name SET_TF_DEBUG_HEADER
//
// This macro sets up the debug header in the trap frame.
//
// @param None.
//
// @remark ebp = PKTRAP_FRAME.
//         edi/ebx = Have been saved and can be used.
//
.macro SET_TF_DEBUG_HEADER
    /* Get the Debug Trap Frame EBP/EIP */
    mov ebx, [ebp+KTRAP_FRAME_EBP]
    mov edi, [ebp+KTRAP_FRAME_EIP]

    /* Write the debug data */
    mov [ebp+KTRAP_FRAME_DEBUGPOINTER], edx
    mov dword ptr [ebp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00
    mov [ebp+KTRAP_FRAME_DEBUGEBP], ebx
    mov [ebp+KTRAP_FRAME_DEBUGEIP], edi
.endm

//
// @name CHECK_FOR_APC_DELIVER
// 
// This macro checks if the trapframe indicates a return to user-mode,
// and, if so, checks if user-mode APCs should be delivered.
//
// @param PreserveEax
//        Determines if EAX should be preserved. Implies that the segment
//        registers will also be saved.
//
// @remark ebp = PKTRAP_FRAME.
//         ebx = Saved and will be used.
//
.macro CHECK_FOR_APC_DELIVER PreserveEax
    /* Check for V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz 1f

    /* Deliver APCs only if we were called from user mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], 1
    je 2f

    /* Get the current thread */
1:
    mov ebx, [fs:KPCR_CURRENT_THREAD]

    /* Make it non-alerted */
    mov byte ptr [ebx+KTHREAD_ALERTED], 0

    /* And only if any are actually pending */
    cmp byte ptr [ebx+KTHREAD_PENDING_USER_APC], 0
    je 2f

    /* Save pointer to Trap Frame */
    mov ebx, ebp

.if \PreserveEax
    /* Save some stuff that raising IRQL will kill */
    mov [ebx+KTRAP_FRAME_EAX], eax
    mov dword ptr [ebx+KTRAP_FRAME_FS], KGDT_R3_TEB + RPL_MASK
    mov dword ptr [ebx+KTRAP_FRAME_DS], KGDT_R3_DATA + RPL_MASK
    mov dword ptr [ebx+KTRAP_FRAME_ES], KGDT_R3_DATA + RPL_MASK
    mov dword ptr [ebx+KTRAP_FRAME_GS], 0
.endif

    /* Raise IRQL to APC_LEVEL */
    mov ecx, 1
    call @KfRaiseIrql@4

    /* Save old IRQL */
    push eax

    /* Deliver APCs */
    sti
    push ebx
    push 0
    push UserMode
    call _KiDeliverApc@12

    /* Return to old IRQL */
    pop ecx
    call @KfLowerIrql@4

    /* Restore EAX (only in volatile case) */
.if \PreserveEax
    mov eax, [ebx+KTRAP_FRAME_EAX]
.endif
    cli
    jmp 1b
2:
.endm

//
// @name TRAP_PROLOG
// 
// This macro creates a standard trap entry prologue.
// It should be used for entry into any kernel trap (KiTrapXx), but not for
// system calls, which require special handling.
//
// @param Label
//        Identifying name of the caller function; will be used to append
//        to the name V86 and DR helper functions, which must already exist.
//
// @remark Use as follows:
//          _KiTrap00:
//              /* Push fake error code */
//              push 0
//
//              /* Enter common prologue */
//              TRAP_PROLOG(0)
//
//              /* Handle trap */
//              <Your Trap Code Here>
//
#define TRAP_PROLOG(Label) \
    /* Just to be safe, clear out the HIWORD, since it's reserved */ \
    mov word ptr [esp+2], 0; \
\
    /* Save the non-volatiles */ \
    push ebp; \
    push ebx; \
    push esi; \
    push edi; \
\
    /* Save FS and set it to PCR */ \
    push fs; \
    mov ebx, KGDT_R0_PCR; \
    mov fs, bx; \
\
    /* Save exception list and bogus previous mode */ \
    push fs:[KPCR_EXCEPTION_LIST]; \
    push -1; \
\
    /* Save volatiles and segment registers */ \
    push eax; \
    push ecx; \
    push edx; \
    push ds; \
    push es; \
    push gs; \
\
    /* Set the R3 data segment */ \
    mov ax, KGDT_R3_DATA + RPL_MASK; \
\
    /* Skip debug registers and debug stuff */ \
    sub esp, 0x30; \
\
    /* Load the segment registers */ \
    mov ds, ax; \
    mov es, ax; \
\
    /* Set up frame */ \
    mov ebp, esp; \
\
    /* Check if this was from V86 Mode */ \
    /* test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK; */ \
    /* jnz V86_Label; */ \
\
    /* Get current thread */ \
    mov ecx, [fs:KPCR_CURRENT_THREAD]; \
    cld; \
\
    /* Flush DR7 */ \
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0; \
\
    /* Check if the thread was being debugged */ \
    /* test byte ptr [ecx+KTHREAD_DEBUG_ACTIVE], 0xFF; */ \
    /* jnz Dr_Label; */ \
\
    /* Set the Trap Frame Debug Header */ \
    SET_TF_DEBUG_HEADER

//
// @name SYSCALL_PROLOG
// 
// This macro creates a system call entry prologue.
// It should be used for entry into any fast-system call (KiGetTickCount,
// KiCallbackReturn, KiRaiseAssertion) and the generic system call handler
// (KiSystemService)
//
// @param Label
//        Unique label identifying the name of the caller function; will be
//        used to append to the name of the DR helper function, which must
//        already exist.
//
// @remark None.
//
.macro SYSCALL_PROLOG
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

    /* Set the thread's trap frame and clear direction flag */
    mov [esi+KTHREAD_TRAP_FRAME], ebp
    cld

    /* Save DR registers if needed */
    //jnz Dr_kss_&Label

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
    jz 0f
    mov edi, [edi+4]
0:
    mov [ebp+KTRAP_FRAME_DEBUGEIP], edi
#endif

    /* Enable interrupts */
    sti
.endm

//
// @name TRAP_EPILOG
// 
// This macro creates an epilogue for leaving any system trap.
// It is used for exiting system calls, exceptions, interrupts and generic
// traps.
//
// @param SystemCall
//        Specifies whether this trap will exit a system call. If so, special
//        code will be assembled to potentially use SYSEXIT instead of IRETD.
//
// @param RestorePreviousMode
//        Specifies if the previous mode should be restored.
//
// @param RestoreSegments
//        Specifies if the segment registers should be restored.
//
// @param RestoreVolatiles
//        Specifies if the volatile registers should be restored.
//
// @param RestoreAllRegs
//        Specifies if volatiles and segments should both be restored.
//
// @remark
//
.macro TRAP_EPILOG SystemCall, RestorePreviousMode, RestoreSegments, RestoreVolatiles, RestoreAllRegs
#ifdef DBG
    /* Assert the flags */
    pushfd
    pop edx
    test edx, EFLAGS_INTERRUPT_MASK
    jnz 1f

    /* Assert the stack */
    cmp esp, ebp
    jnz 1f

    /* Assert the trap frame */
0:
    //sub dword ptr [esp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00
    //jnz 2f

    /* Assert FS */
    mov bx, fs
    cmp bx, KGDT_R0_PCR
    jnz 1f

    /* Assert exception list */
    cmp dword ptr fs:[KPCR_EXCEPTION_LIST], 0
    jnz 4f
2:
    add dword ptr [esp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00
1:
    int 3
    jmp 0b
4:
#endif

    /* Get exception list */
    mov edx, [esp+KTRAP_FRAME_EXCEPTION_LIST]

#ifdef DBG
    /* Assert the saved exception list */
    or edx, edx
    jnz 5f
    int 3
5:
#endif

    /* Restore it */
    mov [fs:KPCR_EXCEPTION_LIST], edx

.if \RestorePreviousMode
    /* Get previous mode */
    mov ecx, [esp+KTRAP_FRAME_PREVIOUS_MODE]

#ifdef DBG
    /* Assert the saved previous mode */
    cmp ecx, -1
    jnz 6f
    int 3
6:
#endif

    /* Restore the previous mode */
    mov esi, [fs:KPCR_CURRENT_THREAD]
    mov byte ptr [esi+KTHREAD_PREVIOUS_MODE], cl
.endif

    /* Check for V86 */
    test dword ptr [esp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86_Exit

    /* Check if the frame was edited */
    test word ptr [esp+KTRAP_FRAME_CS], FRAME_EDITED
    jz 7f

.ifeq \RestoreAllRegs
    /* Check the old mode */
    cmp word ptr [esp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    bt word ptr [esp+KTRAP_FRAME_CS], 0
    cmc
    ja 8f
.endif

.if \RestoreVolatiles
    /* Restore volatiles */
    mov edx, [esp+KTRAP_FRAME_EDX]
    mov ecx, [esp+KTRAP_FRAME_ECX]
    mov eax, [esp+KTRAP_FRAME_EAX]
.endif

    /* Check if we were called from kernel mode */
    cmp dword ptr [ebp+KTRAP_FRAME_CS], KGDT_R0_CODE
    jz 9f

.if \RestoreSegments
    /* Restore segment registers */
    lea esp, [ebp+KTRAP_FRAME_GS]
    pop gs
    pop es
    pop ds
.endif

    /* Restore FS */
3:
    lea esp, [ebp+KTRAP_FRAME_FS]
    pop fs

9:
    /* Skip debug information and unsaved registers */
    lea esp, [ebp+KTRAP_FRAME_EDI]
    pop edi
    pop esi
    pop ebx
    pop ebp

    /* Check for ABIOS */
    cmp word ptr [esp+8], 0x80
    ja AbiosExit

    /* Pop error code */
    add esp, 4

.if \SystemCall
    /* Check if previous CS is from user-mode */
    test dword ptr [esp+4], 1

    /* It is, so use Fast Exit */
    jnz FastExit

    /* Jump back to stub */
    pop edx
    pop ecx
    popf
    jmp edx

.ret:
.endif
    iret

.if \SystemCall
FastExit:
    /* Is SYSEXIT Supported/Wanted? */
    cmp dword ptr ss:[_KiFastSystemCallDisable], 0
    jnz .ret
    test dword ptr [esp+8], EFLAGS_TF
    jnz .ret

    /* Restore FS to TIB */
    mov ecx, KGDT_R3_TEB + RPL_MASK
    mov fs, ecx

    /* We will be cleaning up the stack ourselves */
    pop edx                                 /* New Ring 3 EIP */
    add esp, 4                              /* Skip Ring 3 DS */
    and dword ptr [esp], 0xfffffdff         /* Remove EFLAGS_INTERRUPT_MASK from EFLAGS */
    popf                                    /* Restore old EFLAGS */
    pop ecx                                 /* Old Ring 3 SS:ESP */

    /*
     * At this point:
     *     ECX points to the old User Stack.
     *     EDX points to the instruction to execute in usermode after the sysenter
     */
    sti
    sysexit
.endif

8:
    /* Restore EAX */
    mov eax, [esp+KTRAP_FRAME_EAX]

    /* Skip registers */
    add esp, 0x30

    /* Restore segments and volatiles */
    pop gs
    pop es
    pop ds
    pop edx
    pop ecx

    /* Jump back to mainline code */
    jmp 3b

7:
    /* Restore real CS value */
    mov ebx, [esp+KTRAP_FRAME_TEMPCS]
    mov [esp+KTRAP_FRAME_CS], ebx

    /*
     * If ESP was modified, then a special interrupt exit stack
     * must be created to "update" ESP's value in a legal manner
     */
    mov ebx, [esp+KTRAP_FRAME_TEMPESP]
    sub ebx, 0xC
    mov [esp+KTRAP_FRAME_ERROR_CODE], ebx

    /* Copy Interrupt Stack */
    mov esi, [esp+KTRAP_FRAME_EFLAGS]
    mov [ebx+8], esi
    mov esi, [esp+KTRAP_FRAME_CS]
    mov [ebx+4], esi
    mov esi, [esp+KTRAP_FRAME_EIP]
    mov [ebx], esi

    /* Return */
    add esp, KTRAP_FRAME_EDI
    pop edi
    pop esi
    pop ebx
    pop ebp
    mov esp, [esp]
    iret
.endm

