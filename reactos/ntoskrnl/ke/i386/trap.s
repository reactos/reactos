/*
 * FILE:            ntoskrnl/ke/i386/trap.S
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

MACRO(GENERATE_INT_HANDLER, Vector)
//.func KiUnexpectedInterrupt&Number
_KiUnexpectedInterrupt&Vector:
    /* This is a push instruction with 8bit operand. Since the instruction
       sign extends the value to 32 bits, we need to offset it */
    push (Vector - 128)
    jmp _KiEndUnexpectedRange@0
//.endfunc
ENDM

EXTERN _KiTrap02:PROC

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

TRAP_ENTRY KiTrap00, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap01, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap03, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap04, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap05, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap06, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap07, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap08, 0
TRAP_ENTRY KiTrap09, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap0A, 0
TRAP_ENTRY KiTrap0B, 0
TRAP_ENTRY KiTrap0C, 0
TRAP_ENTRY KiTrap0D, 0
TRAP_ENTRY KiTrap0E, 0
TRAP_ENTRY KiTrap0F, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap10, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap11, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiTrap13, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiRaiseSecurityCheckFailure, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiGetTickCount, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiCallbackReturn, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiRaiseAssertion, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiDebugService, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiUnexpectedInterruptTail, 0

ALIGN 4
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

PUBLIC _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0:
    add dword ptr[esp], 128
    jmp _KiUnexpectedInterruptTail


/* EXIT CODE *****************************************************************/

KiTrapExitStub KiSystemCallReturn,        (KI_RESTORE_EAX OR KI_RESTORE_EFLAGS OR KI_EXIT_JMP)
KiTrapExitStub KiSystemCallSysExitReturn, (KI_RESTORE_EAX OR KI_RESTORE_FS OR KI_RESTORE_EFLAGS OR KI_EXIT_SYSCALL)
KiTrapExitStub KiSystemCallTrapReturn,    (KI_RESTORE_EAX OR KI_RESTORE_FS OR KI_EXIT_IRET)

KiTrapExitStub KiEditedTrapReturn,        (KI_RESTORE_VOLATILES OR KI_RESTORE_EFLAGS OR KI_EDITED_FRAME OR KI_EXIT_RET)
KiTrapExitStub KiTrapReturn,              (KI_RESTORE_VOLATILES OR KI_RESTORE_SEGMENTS OR KI_EXIT_IRET)
KiTrapExitStub KiTrapReturnNoSegments,    (KI_RESTORE_VOLATILES OR KI_EXIT_IRET)
KiTrapExitStub KiTrapReturnNoSegmentsRet8,(KI_RESTORE_VOLATILES OR KI_RESTORE_EFLAGS OR KI_EXIT_RET8)

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
