/*
 * FILE:            ntoskrnl/ke/i386/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/i386/asm.h>
#include <internal/i386/asmmacro.S>

MACRO(GENERATE_IDT_STUB, Number)
idt _KiUnexpectedInterrupt&Number, INT_32_DPL0
ENDM

MACRO(GENERATE_INT_HANDLER, Number)
.func KiUnexpectedInterrupt&Number
_KiUnexpectedInterrupt&Number:
    push PRIMARY_VECTOR_BASE + Number
    jmp _KiEndUnexpectedRange@0
.endfunc
ENDM

/* GLOBALS *******************************************************************/

.data

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
.rept 22
idt _KiTrap0F,         INT_32_DPL0  /* INT 14-29: UNDEFINED INTERRUPTS      */
.endr
idt _KiGetTickCount,   INT_32_DPL3  /* INT 2A: Get Tick Count Handler       */
idt _KiCallbackReturn, INT_32_DPL3  /* INT 2B: User-Mode Callback Return    */
idt _KiRaiseAssertion, INT_32_DPL3  /* INT 2C: Debug Assertion Handler      */
idt _KiDebugService,   INT_32_DPL3  /* INT 2D: Debug Service Handler        */
idt _KiSystemService,  INT_32_DPL3  /* INT 2E: System Call Service Handler  */
idt _KiTrap0F,         INT_32_DPL0  /* INT 2F: RESERVED                     */
i = 0
.rept 208
    GENERATE_IDT_STUB %i
    i = i + 1
.endr

PUBLIC _KiIdtDescriptor
_KiIdtDescriptor:
    .short 0
    .short 0x7FF
    .long _KiIdt

PUBLIC _KiUnexpectedEntrySize
_KiUnexpectedEntrySize:
    .long _KiUnexpectedInterrupt1 - _KiUnexpectedInterrupt0

/******************************************************************************/
.code32
.text

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
TRAP_ENTRY KiGetTickCount, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiCallbackReturn, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiRaiseAssertion, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiDebugService, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY KiUnexpectedInterruptTail, 0

ALIGN 4
EXTERN @KiInterruptTemplateHandler@8
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

EXTERN @KiFastCallEntryHandler@8:PROC
PUBLIC _KiFastCallEntry
_KiFastCallEntry:
    KiEnterTrap (KI_FAST_SYSTEM_CALL OR KI_NONVOLATILES_ONLY OR KI_DONT_SAVE_SEGS)
    KiCallHandler @KiFastCallEntryHandler@8


EXTERN @KiSystemServiceHandler@8:PROC
PUBLIC _KiSystemService
_KiSystemService:
    KiEnterTrap (KI_PUSH_FAKE_ERROR_CODE OR KI_NONVOLATILES_ONLY OR KI_DONT_SAVE_SEGS)
    KiCallHandler @KiSystemServiceHandler@8

PUBLIC _KiStartUnexpectedRange@0
_KiStartUnexpectedRange@0:
i = 0
.rept 208
    GENERATE_INT_HANDLER %i
    i = i + 1
.endr
PUBLIC _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0:
    jmp _KiUnexpectedInterruptTail


/* EXIT CODE *****************************************************************/

KiTrapExitStub KiSystemCallReturn,        (KI_RESTORE_EAX OR KI_RESTORE_EFLAGS OR KI_EXIT_JMP)
KiTrapExitStub KiSystemCallSysExitReturn, (KI_RESTORE_EAX OR KI_RESTORE_FS OR KI_RESTORE_EFLAGS OR KI_EXIT_SYSCALL)
KiTrapExitStub KiSystemCallTrapReturn,    (KI_RESTORE_EAX OR KI_RESTORE_FS OR KI_EXIT_IRET)

KiTrapExitStub KiEditedTrapReturn,        (KI_RESTORE_VOLATILES OR KI_RESTORE_EFLAGS OR KI_EDITED_FRAME OR KI_EXIT_RET)
KiTrapExitStub KiTrapReturn,              (KI_RESTORE_VOLATILES OR KI_RESTORE_SEGMENTS OR KI_EXIT_IRET)
KiTrapExitStub KiTrapReturnNoSegments,    (KI_RESTORE_VOLATILES OR KI_EXIT_IRET)

END
