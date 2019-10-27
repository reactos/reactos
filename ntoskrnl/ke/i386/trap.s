/*
 * FILE:            ntoskrnl/ke/i386/trap.s
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>
#include <internal/i386/asmmacro.S>

MACRO(GENERATE_IDT_STUB, Vector)
idt _KiUnexpectedInterrupt&Vector, INT_32_DPL0
ENDM

/*
 * The INT stub handlers must all have the same size (stored in _KiUnexpectedEntrySize)
 * in all situations. Therefore, we force using a push instruction with 32-bit operand
 * and a non-short near jump.
 */
MACRO(GENERATE_INT_HANDLER, Vector)
_KiUnexpectedInterrupt&Vector:
#ifdef _USE_ML
    push dword ptr (&Vector)
    jmp near ptr _KiEndUnexpectedRange@0
#else
    /*
     * NOTE: GAS does not take the explicit 'dword ptr' / 'near ptr' overrides
     * into account, and will use e.g. the 8-bit push for values <= 0x7F.
     * We therefore need to hardcode the explicit instruction opcodes.
     */
    .byte HEX(68)  // push dword ptr (&Vector)
    .long (&Vector)
    .byte HEX(0E9) // jmp near ptr _KiEndUnexpectedRange@0
    .long _KiEndUnexpectedRange@0 - 1f
1:
#endif
ENDM

/* GLOBALS *******************************************************************/

.data
ASSUME nothing

.align 16

PUBLIC _KiIdt
_KiIdt:
/* This is the Software Interrupt Table that we handle in this file:        */
idt _KiTrap00,         INT_32_DPL0  /* INT 00: Divide Error (#DE)           */
idt _KiTrap01,         INT_32_DPL0  /* INT 01: Debug Exception (#DB)        */
idt _KiTrap02,         INT_32_DPL0  /* INT 02: NMI Interrupt                */
idt _KiTrap03,         INT_32_DPL3  /* INT 03: Breakpoint Exception (#BP)   */
idt _KiTrap04,         INT_32_DPL3  /* INT 04: Overflow Exception (#OF)     */
idt _KiTrap05,         INT_32_DPL0  /* INT 05: BOUND Range Exceeded (#BR)   */
idt _KiTrap06,         INT_32_DPL0  /* INT 06: Invalid Opcode Code (#UD)    */
idt _KiTrap07,         INT_32_DPL0  /* INT 07: Device Not Available (#NM)   */
idt _KiTrap08,         INT_32_DPL0  /* INT 08: Double Fault Exception (#DF) */
idt _KiTrap09,         INT_32_DPL0  /* INT 09: RESERVED                     */
idt _KiTrap0A,         INT_32_DPL0  /* INT 0A: Invalid TSS Exception (#TS)  */
idt _KiTrap0B,         INT_32_DPL0  /* INT 0B: Segment Not Present (#NP)    */
idt _KiTrap0C,         INT_32_DPL0  /* INT 0C: Stack Fault Exception (#SS)  */
idt _KiTrap0D,         INT_32_DPL0  /* INT 0D: General Protection (#GP)     */
idt _KiTrap0E,         INT_32_DPL0  /* INT 0E: Page-Fault Exception (#PF)   */
idt _KiTrap0F,         INT_32_DPL0  /* INT 0F: RESERVED                     */
idt _KiTrap10,         INT_32_DPL0  /* INT 10: x87 FPU Error (#MF)          */
idt _KiTrap11,         INT_32_DPL0  /* INT 11: Align Check Exception (#AC)  */
idt _KiTrap0F,         INT_32_DPL0  /* INT 12: Machine Check Exception (#MC)*/
idt _KiTrap0F,         INT_32_DPL0  /* INT 13: SIMD FPU Exception (#XF)     */
REPEAT 21
idt _KiTrap0F,         INT_32_DPL0  /* INT 14-28: UNDEFINED INTERRUPTS      */
ENDR
idt _KiRaiseSecurityCheckFailure, INT_32_DPL3
                                    /* INT 29: Handler for __fastfail       */
idt _KiGetTickCount,   INT_32_DPL3  /* INT 2A: Get Tick Count Handler       */
idt _KiCallbackReturn, INT_32_DPL3  /* INT 2B: User-Mode Callback Return    */
idt _KiRaiseAssertion, INT_32_DPL3  /* INT 2C: Debug Assertion Handler      */
idt _KiDebugService,   INT_32_DPL3  /* INT 2D: Debug Service Handler        */
idt _KiSystemService,  INT_32_DPL3  /* INT 2E: System Call Service Handler  */
idt _KiTrap0F,         INT_32_DPL0  /* INT 2F: RESERVED                     */
i = HEX(30)
REPEAT 208
    GENERATE_IDT_STUB %i
    i = i + 1
ENDR

PUBLIC _KiIdtDescriptor
_KiIdtDescriptor:
    .short 0
    .short HEX(7FF)
    .long _KiIdt

PUBLIC _KiUnexpectedEntrySize
_KiUnexpectedEntrySize:
    .long _KiUnexpectedInterrupt49 - _KiUnexpectedInterrupt48

/******************************************************************************/

.code

PUBLIC _KiStartUnexpectedRange@0
_KiStartUnexpectedRange@0:
i = HEX(30)
REPEAT 208
    GENERATE_INT_HANDLER %i
    i = i + 1
ENDR

PUBLIC _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0:
    jmp _KiUnexpectedInterruptTail


MACRO(TRAP_ENTRY_EX, Vector, Trap, Flags)
#ifdef _USE_ML
.trap_seg&Vector SEGMENT 'CODE' ALIAS(".inthdlr$&Vector")
#else
.section .inthdlr$\()\Vector
#endif
    TRAP_ENTRY &Trap, &Flags
#ifdef _USE_ML
.trap_seg&Vector ENDS
#endif
ENDM

MACRO(TASK_ENTRY_EX, Vector, Trap, Flags)
#ifdef _USE_ML
.trap_seg&Vector SEGMENT 'CODE' ALIAS(".inthdlr$&Vector")
#else
.section .inthdlr$\()\Vector
#endif
    TASK_ENTRY &Trap, &Flags
#ifdef _USE_ML
.trap_seg&Vector ENDS
#endif
ENDM


TRAP_ENTRY_EX 00, KiTrap00, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 01, KiTrap01, KI_PUSH_FAKE_ERROR_CODE
TASK_ENTRY_EX 02, KiTrap02, KI_NMI
TRAP_ENTRY_EX 03, KiTrap03, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 04, KiTrap04, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 05, KiTrap05, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 06, KiTrap06, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 07, KiTrap07, KI_PUSH_FAKE_ERROR_CODE
TASK_ENTRY_EX 08, KiTrap08, 0
TRAP_ENTRY_EX 09, KiTrap09, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 0A, KiTrap0A, 0
TRAP_ENTRY_EX 0B, KiTrap0B, 0
TRAP_ENTRY_EX 0C, KiTrap0C, 0
TRAP_ENTRY_EX 0D_C, KiTrap0D, 0
TRAP_ENTRY_EX 0E, KiTrap0E, 0
TRAP_ENTRY_EX 0F, KiTrap0F, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 10, KiTrap10, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 11, KiTrap11, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 13, KiTrap13, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 29, KiRaiseSecurityCheckFailure, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 2A, KiGetTickCount, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 2B, KiCallbackReturn, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 2C, KiRaiseAssertion, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY_EX 2D, KiDebugService, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiUnexpectedInterruptTail, 0

.align 4
EXTERN @KiInterruptTemplateHandler@8:PROC
PUBLIC _KiInterruptTemplate
_KiInterruptTemplate:
    KiEnterTrap KI_PUSH_FAKE_ERROR_CODE
PUBLIC _KiInterruptTemplate2ndDispatch
_KiInterruptTemplate2ndDispatch:
    mov edx, 0
PUBLIC _KiInterruptTemplateObject
_KiInterruptTemplateObject:
    mov eax, offset @KiInterruptTemplateHandler@8
    jmp eax
PUBLIC _KiInterruptTemplateDispatch
_KiInterruptTemplateDispatch:

EXTERN @KiSystemServiceHandler@8:PROC
PUBLIC _KiSystemService
.PROC _KiSystemService
    FPO 0, 0, 0, 0, 1, FRAME_TRAP
    KiEnterTrap (KI_PUSH_FAKE_ERROR_CODE OR KI_NONVOLATILES_ONLY OR KI_DONT_SAVE_SEGS)
    KiCallHandler @KiSystemServiceHandler@8
.ENDP

PUBLIC _KiFastCallEntry
.PROC _KiFastCallEntry
    FPO 0, 0, 0, 0, 1, FRAME_TRAP
    KiEnterTrap (KI_FAST_SYSTEM_CALL OR KI_NONVOLATILES_ONLY OR KI_DONT_SAVE_SEGS)
    KiCallHandler @KiSystemServiceHandler@8
.ENDP

PUBLIC _KiFastCallEntryWithSingleStep
.PROC _KiFastCallEntryWithSingleStep
    FPO 0, 0, 0, 0, 1, FRAME_TRAP
    KiEnterTrap (KI_FAST_SYSTEM_CALL OR KI_NONVOLATILES_ONLY OR KI_DONT_SAVE_SEGS)
    or dword ptr [ecx + KTRAP_FRAME_EFLAGS], EFLAGS_TF
    KiCallHandler @KiSystemServiceHandler@8
.ENDP


/* EXIT CODE *****************************************************************/

KiTrapExitStub KiSystemCallReturn,        (KI_RESTORE_EAX OR KI_RESTORE_EFLAGS OR KI_EXIT_JMP)
KiTrapExitStub KiSystemCallSysExitReturn, (KI_RESTORE_EAX OR KI_RESTORE_FS OR KI_RESTORE_EFLAGS OR KI_EXIT_SYSCALL)
KiTrapExitStub KiSystemCallTrapReturn,    (KI_RESTORE_EAX OR KI_RESTORE_FS OR KI_EXIT_IRET)

KiTrapExitStub KiEditedTrapReturn,        (KI_RESTORE_VOLATILES OR KI_RESTORE_EFLAGS OR KI_EDITED_FRAME OR KI_EXIT_RET)
KiTrapExitStub KiTrapReturn,              (KI_RESTORE_VOLATILES OR KI_RESTORE_SEGMENTS OR KI_EXIT_IRET)
KiTrapExitStub KiTrapReturnNoSegments,    (KI_RESTORE_VOLATILES OR KI_EXIT_IRET)
KiTrapExitStub KiTrapReturnNoSegmentsRet8,(KI_RESTORE_VOLATILES OR KI_RESTORE_EFLAGS OR KI_EXIT_RET8)

.code

EXTERN _PsConvertToGuiThread@0:PROC

PUBLIC _KiConvertToGuiThread@0
_KiConvertToGuiThread@0:

    /*
     * Converting to a GUI thread safely updates ESP in-place as well as the
     * current Thread->TrapFrame and EBP when KeSwitchKernelStack is called.
     *
     * However, PsConvertToGuiThread "helpfully" restores EBP to the original
     * caller's value, since it is considered a nonvolatile register. As such,
     * as soon as we're back after the conversion and we try to store the result
     * which will probably be in some stack variable (EBP-based), we'll crash as
     * we are touching the de-allocated non-expanded stack.
     *
     * Thus we need a way to update our EBP before EBP is touched, and the only
     * way to guarantee this is to do the call itself in assembly, use the EAX
     * register to store the result, fixup EBP, and then let the C code continue
     * on its merry way.
     *
     */

    /* Save ebx */
    push ebx

    /* Calculate the stack frame offset in ebx */
    mov ebx, ebp
    sub ebx, esp

    /* Call the worker function */
    call _PsConvertToGuiThread@0

    /* Adjust ebp to the new stack */
    mov ebp, esp
    add ebp, ebx

    /* Restore ebx */
    pop ebx

    /* return to the caller */
    ret

END
