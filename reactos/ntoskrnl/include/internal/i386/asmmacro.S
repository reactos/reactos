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
#define DoPushFakeErrorCode         1
#define DoFixupV86                  1
#define DoFixupAbios                1
#define NotFromSystemCall           0
#define DoNotRestorePreviousMode    0
#define DoNotRestoreEverything      0
#define DoNotRestoreSegments        0
#define DoNotRestoreVolatiles       0
#define DoNotPushFakeErrorCode      0
#define DoNotFixupV86               0
#define DoNotFixupAbios             0

// Arguments for idt
#define INT_32_DPL0                 0x8E00
#define INT_32_DPL3                 0xEE00

//
// This table contains the prefix flags that are used by V86 emulation
//
.equ PREFIX_FLAG_ES,                0x00000100
.equ PREFIX_FLAG_CS,                0x00000200
.equ PREFIX_FLAG_SS,                0x00000400
.equ PREFIX_FLAG_DS,                0x00000800
.equ PREFIX_FLAG_FS,                0x00001000
.equ PREFIX_FLAG_GS,                0x00002000
.equ PREFIX_FLAG_OPER32,            0x00004000
.equ PREFIX_FLAG_ADDR32,            0x00008000
.equ PREFIX_FLAG_LOCK,              0x00010000
.equ PREFIX_FLAG_REPNE,             0x00020000
.equ PREFIX_FLAG_REP,               0x00040000

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
// @name UNHANDLED_PATH
//
// This macro TODO
//
// @param None
//
// @remark None.
//
.macro UNHANDLED_PATH
    /* Get EIP */
    call $+5

    /* Print debug message */
    push offset _UnhandledMsg
    call _DbgPrint
    add esp, 8

    /* Loop indefinitely */
    jmp $
.endm

//
// @name IDT
//
// This macro creates an IDT entry for the given handler
//
// @param Handler
//        Pointer to the IDT handler
//
// @param Bits
//        Descriptor Bits to associate
//
// @remark None.
//
.macro idt Handler, Bits
    .long \Handler
    .short \Bits
    .short KGDT_R0_CODE
.endm

//
// @name GENERATE_IDT_STUB
//
// This macro creates an IDT entry for an unexpected interrupt handler.
//
// @param None.
//
// @remark None.
//
.macro GENERATE_IDT_STUB Number
idt _KiUnexpectedInterrupt&Number, INT_32_DPL0
.endm

//
// @name GENERATE_IDT_STUBS
//
// This macro creates unexpected interrupt IDT entries.
//
// @param None.
//
// @remark None.
//
.altmacro
.macro GENERATE_IDT_STUBS
.set i, 0
.rept 208
    GENERATE_IDT_STUB %i
    .set i, i + 1
.endr
.endm

//
// @name GENERATE_INT_HANDLER
//
// This macro creates an unexpected interrupt handler.
//
// @param None.
//
// @remark None.
//
.macro GENERATE_INT_HANDLER Number
.func KiUnexpectedInterrupt&Number
_KiUnexpectedInterrupt&Number:
    push PRIMARY_VECTOR_BASE + Number
    jmp _KiEndUnexpectedRange@0
.endfunc
.endm

//
// @name GENERATE_INT_HANDLERS
//
// This macro creates the unexpected interrupt handlers.
//
// @param None.
//
// @remark None.
//
.altmacro
.macro GENERATE_INT_HANDLERS
.set i, 0
.rept 208
    GENERATE_INT_HANDLER %i
    .set i, i + 1
.endr
.endm

//
// @name INVALID_V86_OPCODE
//
// This macro creates one or more entries for unhandled V86 Opcodes
// in the V86 Opcode Table.
//
// @param count.
//        Number of entries to generate.
//
// @remark None.
//
.macro INVALID_V86_OPCODE count
    .rept \count
        .byte 0
    .endr
.endm

//
// @name GENERATE_PREFIX_HANDLER
//
// This macro creates a prefix opcode handler.
//
// @param None.
//
// @remark None.
//
.macro GENERATE_PREFIX_HANDLER Name
.func Opcode&Name&PrefixV86
_Opcode&Name&PrefixV86:
    or ebx, PREFIX_FLAG_&Name
    jmp _OpcodeGenericPrefixV86
.endfunc
.endm

//
// @name INVALID_V86_OPCODE
//
// This macro prints out visible message and hangs the computer.
//
// @param None.
//
// @remark Temporary debugging use.
//
.macro UNHANDLED_V86_OPCODE
    /* Print debug message, breakpoint and freeze */
    push ecx
    push offset V86DebugMsg
    call _DbgPrint
    add esp, 8
    jmp $
.endm

//
// @name TRAP_FIXUPS
//
// This macro contains out-of-line code for various Trap Frame Fixups, such as:
//
//  - DR Fixup: Loads and restores DR registers.
//  - V86 Fixup: Loads and restores V86 segments.
//  - ABIOS Fixup: Loads and restores the ABIOS state and stack.
//
// @param None.
//
// @remark ebp = PKTRAP_FRAME
//
.macro TRAP_FIXUPS Label, EndLabel, V86Fix, AbiosFix
Dr_&Label:

    /* Check if this was V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz 2f

    /* Check if it was user mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz Dr_&EndLabel

2:
    /* Get DR0, 1, 2 */
    mov ebx, dr0
    mov ecx, dr1
    mov edi, dr2

    /* Save them */
    mov [ebp+KTRAP_FRAME_DR0], ebx
    mov [ebp+KTRAP_FRAME_DR1], ecx
    mov [ebp+KTRAP_FRAME_DR2], edi

    /* Get DR3, 6, 7 */
    mov ebx, dr3
    mov ecx, dr6
    mov edi, dr7

    /* Save them */
    mov [ebp+KTRAP_FRAME_DR3], ebx
    mov [ebp+KTRAP_FRAME_DR6], ecx
    mov [ebp+KTRAP_FRAME_DR7], edi

    /* Clear DR7 */
    xor ebx, ebx
    mov dr7, ebx

    /* Get the PRCB */
    mov edi, fs:[KPCR_PRCB]

    /* Get DR0, 1 */
    mov ebx, [edi+KPRCB_DR0]
    mov ecx, [edi+KPRCB_DR1]

    /* Set them */
    mov dr0, ebx
    mov dr1, ecx

    /* Get DR2, 3 */
    mov ebx, [edi+KPRCB_DR2]
    mov ecx, [edi+KPRCB_DR3]

    /* Set them */
    mov dr2, ebx
    mov dr3, ecx

    /* Get DR6, 7 */
    mov ebx, [edi+KPRCB_DR6]
    mov ecx, [edi+KPRCB_DR7]

    /* Set them */
    mov dr6, ebx
    mov dr7, ecx
    jmp Dr_&EndLabel

.if \AbiosFix
Abios_&Label:
    UNHANDLED_PATH
.endif

.if \V86Fix
V86_&Label:

    /* Get V86 segment registers */
    mov eax, [ebp+KTRAP_FRAME_V86_FS]
    mov ebx, [ebp+KTRAP_FRAME_V86_GS]
    mov ecx, [ebp+KTRAP_FRAME_V86_ES]
    mov edx, [ebp+KTRAP_FRAME_V86_DS]

    /* Restore them into Protected Mode trap frame */
    mov [ebp+KTRAP_FRAME_FS], ax
    mov [ebp+KTRAP_FRAME_GS], bx
    mov [ebp+KTRAP_FRAME_ES], cx
    mov [ebp+KTRAP_FRAME_DS], dx

    /* Go back to mainline code */
    jmp V86_&EndLabel
.endif
.endm

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
    mov ebx, PCR[KPCR_CURRENT_THREAD]

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
.macro TRAP_PROLOG Label EndLabel
    /* Just to be safe, clear out the HIWORD, since it's reserved */
    mov word ptr [esp+2], 0

    /* Save the non-volatiles */
    push ebp
    push ebx
    push esi
    push edi

    /* Save FS and set it to PCR */
    push fs
    mov ebx, KGDT_R0_PCR
    .byte 0x66
    mov fs, bx

    /* Save exception list and bogus previous mode */
    push fs:[KPCR_EXCEPTION_LIST]
    push -1

    /* Save volatiles and segment registers */
    push eax
    push ecx
    push edx
    push ds
    push es
    push gs

    /* Set the R3 data segment */
    mov ax, KGDT_R3_DATA + RPL_MASK

    /* Skip debug registers and debug stuff */
    sub esp, 0x30

    /* Load the segment registers */
    .byte 0x66
    mov ds, ax
    .byte 0x66
    mov es, ax

    /* Check if this interrupt happened in 16-bit mode */
    cmp esp, 0x10000
    jb _Ki16BitStackException

    /* Set up frame */
    mov ebp, esp

    /* Check if this was from V86 Mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86_&Label

V86_&EndLabel:
    /* Get current thread */
    mov ecx, fs:[KPCR_CURRENT_THREAD]
    cld

    /* Flush DR7 */
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0

    /* Check if the thread was being debugged */
    test byte ptr [ecx+KTHREAD_DEBUG_ACTIVE], 0xFF
    jnz Dr_&Label

    /* Set the Trap Frame Debug Header */
Dr_&EndLabel:
    SET_TF_DEBUG_HEADER
.endm

//
// @name INT_PROLOG
// 
// This macro creates a standard interrupt entry prologue.
// It should be used for entry into any interrupt, including software.
//
// @param Label
//        Identifying name of the caller function; will be used to append
//        to the name V86, ABIOS and DR helper functions, which must exist.
//
// @remark For software interrupts, make sure that a fake INT stack is created.
//
.macro INT_PROLOG Label EndLabel FakeErrorCode

.if \FakeErrorCode
    /* Save fake error code */
    push esp
.endif

    /* Save the non-volatiles */
    push ebp
    push ebx
    push esi
    push edi

    /* Skip debug registers and other stuff */
    sub esp, 0x54

    /* Set up frame */
    mov ebp, esp

    /* Save volatiles */ 
    mov [esp+KTRAP_FRAME_EAX], eax
    mov [esp+KTRAP_FRAME_ECX], ecx
    mov [esp+KTRAP_FRAME_EDX], edx
    mov dword ptr [esp+KTRAP_FRAME_PREVIOUS_MODE], -1

    /* Check if this was from V86 Mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86_&Label

    /* Check if this was kernel mode */
V86_&EndLabel:
    cmp word ptr [esp+KTRAP_FRAME_CS], KGDT_R0_CODE
    jz 1f

    /* Set segments */
    mov word ptr [esp+KTRAP_FRAME_FS], fs
    mov word ptr [esp+KTRAP_FRAME_DS], ds
    mov word ptr [esp+KTRAP_FRAME_ES], es
    mov [esp+KTRAP_FRAME_GS], gs

    /* Load the segment registers (use OVERRIDE (0x66)) */
    mov ebx, KGDT_R0_PCR
    mov eax, KGDT_R3_DATA | RPL_MASK
    .byte 0x66
    mov fs, bx
    .byte 0x66
    mov ds, ax
    .byte 0x66
    mov es, ax

1: 
    /* Get the previous exception list */
    mov ebx, fs:[KPCR_EXCEPTION_LIST]

    /* Set the exception handler chain terminator */
    mov dword ptr fs:[KPCR_EXCEPTION_LIST], -1

    /* Save the previous exception list */
    mov [esp+KTRAP_FRAME_EXCEPTION_LIST], ebx

.ifeq \FakeErrorCode
    /* Setup the 16-bit stack */
    lea eax, [esp+KTRAP_FRAME_ERROR_CODE]
    lea ecx, [esp+KTRAP_FRAME_EIP]
    mov ebx, ss:[eax]
    mov ss:[eax], ecx
.endif

    /* Check if this is the ABIOS stack */
    /* cmp esp, 0x10000*/
    /* jb Abios_Label*/

    /* Delete error code */
    and dword ptr [esp+KTRAP_FRAME_ERROR_CODE], 0

    /* Get the current thread and clear direction flag */
    mov ecx, PCR[KPCR_CURRENT_THREAD]
    cld

    /* Flush DR7 */
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0

    /* Check if the thread was being debugged */
    test byte ptr [ecx+KTHREAD_DEBUG_ACTIVE], 0xFF

.ifeq \FakeErrorCode
    /* Push parameter */
    push ebx
.endif

    /* Save DR registers if needed */
    jnz Dr_&Label

    /* Set the trap frame debug header */
Dr_&EndLabel:
    SET_TF_DEBUG_HEADER
.endm

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
.macro SYSCALL_PROLOG Label EndLabel
    /* Create a trap frame */
    push 0
    push ebp
    push ebx
    push esi
    push edi
    push fs

    /* Load PCR Selector into fs */
    mov ebx, KGDT_R0_PCR
    .byte 0x66
    mov fs, bx

    /* Get a pointer to the current thread */
    mov esi, PCR[KPCR_CURRENT_THREAD]

    /* Save the previous exception list */
    push PCR[KPCR_EXCEPTION_LIST]

    /* Set the exception handler chain terminator */
    mov dword ptr PCR[KPCR_EXCEPTION_LIST], -1

    /* Save the old previous mode */
    push [esi+KTHREAD_PREVIOUS_MODE]

    /* Skip the other registers */
    sub esp, 0x48

    /* Set the new previous mode based on the saved CS selector */
    mov ebx, [esp+0x6C]
    and ebx, 1
    mov byte ptr [esi+KTHREAD_PREVIOUS_MODE], bl

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
    jnz Dr_&Label

    /* Set the trap frame debug header */
Dr_&EndLabel:
    SET_TF_DEBUG_HEADER

    /* Enable interrupts */
    sti
.endm

//
// @name FASTCALL_PROLOG
// 
// TODO
//
// @param Label
//        Unique label identifying the name of the caller function; will be
//        used to append to the name of the DR helper function, which must
//        already exist.
//
// @remark None.
//
.macro FASTCALL_PROLOG Label EndLabel

    /* Set user selector */
    mov ecx, KGDT_R3_DATA | RPL_MASK

    /* Set FS to PCR */
    push KGDT_R0_PCR
    pop fs

    /* Set DS/ES to User Selector */
    mov ds, cx
    mov es, cx

    /* Set the current stack to Kernel Stack */
    mov ecx, PCR[KPCR_TSS]
    mov esp, [ecx+KTSS_ESP0]

    /* Set up a fake INT Stack. */
    push KGDT_R3_DATA + RPL_MASK
    push edx                            /* Ring 3 SS:ESP */
    pushf                               /* Ring 3 EFLAGS */
    push 2                              /* Ring 0 EFLAGS */
    add edx, 8                          /* Skip user parameter list */
    popf                                /* Set our EFLAGS */
    or dword ptr [esp], EFLAGS_INTERRUPT_MASK   /* Re-enable IRQs in EFLAGS, to fake INT */
    push KGDT_R3_CODE + RPL_MASK
    push dword ptr ds:KUSER_SHARED_SYSCALL_RET

    /* Setup the Trap Frame stack */
    push 0
    push ebp
    push ebx
    push esi
    push edi
    push KGDT_R3_TEB + RPL_MASK

    /* Save pointer to our PCR */
    mov ebx, PCR[KPCR_SELF]

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

    /* Set the thread's trap frame */
    mov [esi+KTHREAD_TRAP_FRAME], ebp

    /* Save DR registers if needed */
    jnz Dr_&Label

    /* Set the trap frame debug header */
Dr_&EndLabel:
    SET_TF_DEBUG_HEADER

    /* Enable interrupts */
    sti
.endm

//
// @name V86_TRAP_PROLOG
// 
// This macro creates a V86 Trap entry prologue.
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
.macro V86_TRAP_PROLOG Label EndLabel

    /* Skip everything to the error code */
    sub esp, KTRAP_FRAME_ERROR_CODE

    /* Clear the error code */
    mov word ptr [esp+KTRAP_FRAME_ERROR_CODE+2], 0

    /* Save the registers we'll trample */
    mov [esp+KTRAP_FRAME_EBX], ebx
    mov [esp+KTRAP_FRAME_EAX], eax
    mov [esp+KTRAP_FRAME_EBP], ebp
    mov [esp+KTRAP_FRAME_ESI], esi
    mov [esp+KTRAP_FRAME_EDI], edi

    /* Save PCR and Ring 3 segments */
    mov ebx, KGDT_R0_PCR
    mov eax, KGDT_R3_DATA + RPL_MASK

    /* Save ECX and EDX too */
    mov [esp+KTRAP_FRAME_ECX], ecx
    mov [esp+KTRAP_FRAME_EDX], edx

    /* Set debugging markers */
    mov dword ptr [esp+KTRAP_FRAME_PREVIOUS_MODE], -1
    mov dword ptr [esp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00

    /* Now set segments (use OVERRIDE, 0x66) */
    .byte 0x66
    mov fs, bx
    .byte 0x66
    mov ds, ax
    .byte 0x66
    mov es, ax

    /* Set the trap frame in the stack and clear the direction flag */
    mov ebp, esp
    cld

    /* Save the exception list */
    mov eax, fs:[KPCR_EXCEPTION_LIST]
    mov [esp+KTRAP_FRAME_EXCEPTION_LIST], eax

    /* Check if we need debugging */
    mov eax, dr7
    test eax, ~DR7_RESERVED_MASK
    mov [esp+KTRAP_FRAME_DR7], eax
    jnz Dr_&Label

Dr_&EndLabel:
.endm

//
// @name V86_TRAP_EPILOG
// 
// This macro creates an epilogue for leaving V86 traps
//
// @param None.
//
// @remark None.
//
.macro V86_TRAP_EPILOG

    /* Get the current thread and make it unalerted */
ExitBegin:
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov byte ptr [ebx+KTHREAD_ALERTED], 0

    /* Check if it has User-mode APCs pending */
    cmp byte ptr [ebx+KTHREAD_PENDING_USER_APC], 0
    jne PendingUserApc

    /* It doesn't, pop the frame */
    add esp, KTRAP_FRAME_EDX
    pop edx
    pop ecx
    pop eax

    /* Check if DR registers should be restored */
    test dword ptr [ebp+KTRAP_FRAME_DR7], ~DR7_RESERVED_MASK
    jnz V86DebugRestore

    /* Finish popping the rest of the frame, and return to P-mode */
V86DebugContinue:
    add esp, 12
    pop edi
    pop esi
    pop ebx
    pop ebp
    add esp, 4
    iretd

V86DebugRestore:

    /* Get DR0, 1 */
    xor ebx, ebx
    mov esi, [ebp+KTRAP_FRAME_DR0]
    mov edi, [ebp+KTRAP_FRAME_DR1]

    /* Clear DR 7 */
    mov dr7, ebx

    /* Get DR2 and load DR0-2 */
    mov ebx, [ebp+KTRAP_FRAME_DR2]
    mov dr0, esi
    mov dr1, edi
    mov dr2, ebx

    /* Get DR3-7 */
    mov esi, [ebp+KTRAP_FRAME_DR0]
    mov edi, [ebp+KTRAP_FRAME_DR1]
    mov ebx, [ebp+KTRAP_FRAME_DR7]

    /* Load them */
    mov dr3, esi
    mov dr6, edi
    mov dr7, ebx
    jmp V86DebugContinue

PendingUserApc:

    /* Raise to APC level */
    mov ecx, APC_LEVEL
    call @KfRaiseIrql@4

    /* Save KIRQL and deliver APCs */
    push eax
    sti
    push ebp
    push 0
    push UserMode
    call _KiDeliverApc@12

    /* Restore IRQL */
    pop ecx
    call @KfLowerIrql@4
    cli

    /* Check if we're not in V86 anymore */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz ExitBegin
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
    jnz 6f

    /* Assert the stack */
    cmp esp, ebp
    jnz 6f

    /* Assert the trap frame */
#endif
5:
#ifdef DBG
    sub dword ptr [esp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00
    jnz 0f

    /* Assert FS */
    mov bx, fs
    cmp bx, KGDT_R0_PCR
    jnz 1f

    /* Assert exception list */
    cmp dword ptr PCR[KPCR_EXCEPTION_LIST], 0
    jnz 2f

1:
    push -1
    call _KeBugCheck@4
#endif

2:
    /* Get exception list */
    mov edx, [esp+KTRAP_FRAME_EXCEPTION_LIST]

#ifdef DBG
    /* Assert the saved exception list */
    or edx, edx
    jnz 1f
    UNHANDLED_PATH
1:
#endif

    /* Restore it */
    mov PCR[KPCR_EXCEPTION_LIST], edx

.if \RestorePreviousMode
    /* Get previous mode */
    mov ecx, [esp+KTRAP_FRAME_PREVIOUS_MODE]

#ifdef DBG
    /* Assert the saved previous mode */
    cmp ecx, -1
    jnz 1f
    UNHANDLED_PATH
1:
#endif

    /* Restore the previous mode */
    mov esi, PCR[KPCR_CURRENT_THREAD]
    mov byte ptr [esi+KTHREAD_PREVIOUS_MODE], cl
.else

#ifdef DBG
    /* Assert the saved previous mode */
    mov ecx, [esp+KTRAP_FRAME_PREVIOUS_MODE]
    cmp ecx, -1
    jz 1f
    UNHANDLED_PATH
1:
#endif
.endif

    /* Check for debug registers */
    test dword ptr [esp+KTRAP_FRAME_DR7], ~DR7_RESERVED_MASK
    jnz 2f

    /* Check for V86 */
4:
    test dword ptr [esp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86_Exit

    /* Check if the frame was edited */
    test word ptr [esp+KTRAP_FRAME_CS], FRAME_EDITED
    jz 7f

.if \RestoreAllRegs
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
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R0_CODE
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

.if \RestoreAllRegs
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
.endif

#if DBG
0:
    /* Fix up the mask */
    add dword ptr [esp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00

6:
    UNHANDLED_PATH
    jmp 5b
#endif

2:
    /* Check if this was V86 mode */
    test dword ptr [esp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz 1f

    /* Check if it was user mode */
    test word ptr [esp+KTRAP_FRAME_CS], MODE_MASK
    jz 4b

1:
    /* Clear DR7 */
    xor ebx, ebx
    mov dr7, ebx

    /* Get DR0, 1, 2 */
    mov esi, [ebp+KTRAP_FRAME_DR0]
    mov edi, [ebp+KTRAP_FRAME_DR1]
    mov ebx, [ebp+KTRAP_FRAME_DR2]

    /* Set them */
    mov dr0, esi
    mov dr1, edi
    mov dr2, ebx

    /* Get DR3, 6, 7 */
    mov esi, [ebp+KTRAP_FRAME_DR3]
    mov edi, [ebp+KTRAP_FRAME_DR6]
    mov ebx, [ebp+KTRAP_FRAME_DR7]

    /* Set them */
    mov dr3, esi
    mov dr6, edi
    mov dr7, ebx
    jmp 4b

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

.if \RestoreVolatiles
    /* Restore volatiles */
    mov eax, [esp+KTRAP_FRAME_EAX]
    mov edx, [esp+KTRAP_FRAME_EDX]
    mov ecx, [esp+KTRAP_FRAME_ECX]
.endif

    /* Return */
    add esp, KTRAP_FRAME_EDI
    pop edi
    pop esi
    pop ebx
    pop ebp
    mov esp, [esp]
    iret
.endm

//
// @name INT_EPILOG
// 
// This macro creates an epilogue for leaving any system trap.
// It is used for exiting system calls, exceptions, interrupts and generic
// traps.
//
// @param Spurious - TRUE if the interrupt was unexpected and spurious.
//
// @remark None.
//
.macro INT_EPILOG Spurious

.if \Spurious
    /* Just exit the trap */
    jmp _Kei386EoiHelper@0
.else
    /* Disable interrupts */
    cli

    /* End the interrupt and do EOI */
    call _HalEndSystemInterrupt@8
    jmp _Kei386EoiHelper@0
.endif
.endm

#ifdef DBG

.macro VERIFY_INT Label
    /* Get the current time and mask it to 192 ticks */
    mov eax, _KeTickCount
    and eax, 0xC0

    /* Check if we're in the same tick area */
    cmp eax, dword ptr [edi+KINTERRUPT_TICK_COUNT]
    jg VfRst_&Label
    jl VfWrap_&Label

    /* If we got here, then our count is too large */
    dec word ptr [edi+KINTERRUPT_DISPATCH_COUNT]
    jz VfOvr_&Label
Vf_&Label:
.endm

.macro VERIFY_INT_END Label, Info
VfOvr_&Label:

    /* Decrement the dispatch count and check if we should bug check */
    dec word ptr [edi+KINTERRUPT_DISPATCH_COUNT+2]
    jz 1f

    /* Update the tick count */
    add eax, 0x40
    mov [edi+KINTERRUPT_TICK_COUNT], eax
    jmp VfRstDef_&Label

.1:
    /* Check if the debugger is enabled */
    cmp byte ptr __KdDebuggerEnabled, 0
    jnz 1f

    /* It isn't, bugcheck */
    push Info
    push edi
    push [edi+KINTERRUPT_SERVICE_CONTEXT]
    push [edi+KINTERRUPT_SERVICE_ROUTINE]
    push HARDWARE_INTERRUPT_STORM
    call _KeBugCheckEx@20

1:
    /* Debugger enabled, do a debug print + break instead */
    push [edi+KINTERRUPT_SERVICE_ROUTINE]
    push offset _IsrOverflowMsg
    call _DbgPrint
    add esp, 8
    int 3

    /* Breakpoint handled, get the new tick count */
    mov eax, _KeTickCount
    and eax, 0xC0

VfRst_&Label:
    /* Reset tick count */
    mov dword ptr [edi+KINTERRUPT_TICK_COUNT], eax
    mov word ptr [edi+KINTERRUPT_DISPATCH_COUNT+2], 64

VfRstDef_&Label:
    /* Put default overflow count and continue */
    mov ax, _KiISROverflow
    mov word ptr [edi+KINTERRUPT_DISPATCH_COUNT], ax
    jmp Vf_&Label

VfWrap_&Label:
    /* Check if we wrapped */
    add eax, 0x40
    cmp eax, [edi+KINTERRUPT_TICK_COUNT]
    je Vf_&Label

    /* We did, start over */
    mov eax, _KeTickCount
    jmp VfRst_&Label
.endm

#else

/* We don't verify interrupts on retail builds */
.macro VERIFY_INT Label
.endm
.macro VERIFY_INT_END Label, Info
.endm

#endif


