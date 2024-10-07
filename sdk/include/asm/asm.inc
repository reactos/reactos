/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            include/asm/asm.inc
 * PURPOSE:         ASM macros for GAS and MASM/ML64
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifndef __ASM_INC__
#define __ASM_INC__

/*
 * Common definitions for the FPO macro.
 * See https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_fpo_data
 */
#define FRAME_FPO    0
#define FRAME_TRAP   1
#define FRAME_TSS    2
#define FRAME_NONFPO 3

#ifdef _USE_ML

/* Allow ".name" identifiers */
OPTION DOTNAME

#ifdef _M_IX86
.686P
.XMM
.MODEL FLAT
ASSUME FS:NOTHING, GS:NOTHING
#endif

/* Explicit radix in MASM syntax  */
#define BIN(x) x##y
#define OCT(x) x##q
#define DEC(x) x##t
#define HEX(x) 0##x##h

/* Macro values need not be marked */
#define VAL(x) x

/* MASM/ML doesn't want explicit [rip] addressing */
rip = 0

/* Due to MASM's reverse syntax, we are forced to use a precompiler macro */
#define MACRO(name, ...) name MACRO __VA_ARGS__

/* To avoid reverse syntax we provide a new macro .PROC, replacing PROC... */
.PROC MACRO name
__current_function_name EQU %name
#ifdef _M_IX86
    %name PROC
#else
    %name PROC FRAME
#endif
ENDM
#define FUNC .PROC

/* ... and .ENDP, replacing ENDP */
.ENDP MACRO
    %__current_function_name ENDP
ENDM
#define ENDFUNC .ENDP

/* Global labels need an extra colon */
GLOBAL_LABEL MACRO label
    %label::
ENDM

/*
 * See https://docs.microsoft.com/en-us/cpp/assembler/masm/dot-fpo
 * and https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_fpo_data
 */
FPO MACRO cdwLocals, cdwParams, cbProlog, cbRegs, fUseBP, cbFrame
    .FPO (cdwLocals, cdwParams, cbProlog, cbRegs, fUseBP, cbFrame)
ENDM

/* MASM doesn't have an ASCII macro */
.ASCII MACRO text:VARARG
    DB text
ENDM
.ascii MACRO text:VARARG
    DB text
ENDM

/* MASM doesn't have an ASCIZ macro */
.ASCIZ MACRO text:VARARG
    DB text
    DB 0
ENDM
.asciz MACRO text:VARARG
    DB text
    DB 0
ENDM

.code64 MACRO
    .code
ENDM

.code32 MACRO
    .code
    .586P
ENDM

.code16 MACRO
    ASSUME nothing
    .text SEGMENT use16 PUBLIC 'CODE'
    .586P
ENDM

.endcode16 MACRO
    .text ENDS
ENDM

.bss MACRO
    .DATA?
    ASSUME nothing
ENDM

//.text MACRO
//ENDM

.align MACRO alignment
    ALIGN alignment
ENDM

.byte MACRO args:VARARG
    db args
ENDM

.short MACRO args:VARARG
    dw args
ENDM

.word MACRO args:VARARG
    dw args
ENDM

.long MACRO args:VARARG
    dd args
ENDM

.quad MACRO args:VARARG
    dq args
ENDM

.double MACRO args:VARARG
    real8 args
ENDM

.org MACRO value
    ORG value
ENDM

.fill MACRO count, size, value
    REPEAT count
        if (size EQ 1)
            DB value
        elseif (size EQ 2)
            DW value
        elseif (size EQ 4)
            DD value
        endif
    ENDM
ENDM

.skip MACRO size, fill:=<0>
    DB size DUP (fill)
ENDM

.space MACRO size, fill:=<0>
    .skip size, fill
ENDM

#define lXdtPrefix fword ptr

ljmp MACRO segment, offset
    DB 0EAh
    DD offset
    DW segment
ENDM

ljmp16 MACRO segment, offset
    DB 0EAh
    DW offset
    DW segment
ENDM

data32 MACRO opcode:VARARG
    DB 66h
    opcode
ENDM

UNIMPLEMENTED2 MACRO file, line, func
    jmp UNIMPLEMENTED2_IMPL
UNIMPLEMENTED_MSG: .ascii "WARNING:  %s at %s:%d is UNIMPLEMENTED!", 10, 0
UNIMPLEMENTED_FUNC: .ascii "&func&", 0
UNIMPLEMENTED_FILE: .ascii file, 0
EXTERN DbgPrint:PROC
UNIMPLEMENTED2_IMPL:
    sub rsp, 28h
    mov r9, line
    lea r8, UNIMPLEMENTED_FILE
    lea rdx, UNIMPLEMENTED_FUNC
    lea rcx, UNIMPLEMENTED_MSG
    call DbgPrint
    add rsp, 28h
    xor eax, eax
ENDM
#define UNIMPLEMENTED UNIMPLEMENTED2 __FILE__, __LINE__,

absolute MACRO address
    __absolute__address__ = address
ENDM

resb MACRO name, size
    name = __absolute__address__
    __absolute__address__ = __absolute__address__ + size
ENDM

/* We need this to distinguish repeat from macros */
#define ENDR ENDM

#define CR 13
#define LF 10
#define NUL 0

/* For compatibility with GAS */
CFI_STARTPROC MACRO start
ENDM
CFI_ENDPROC MACRO
ENDM
CFI_DEF_CFA MACRO reg:REQ, offset:REQ
ENDM
CFI_DEF_CFA_OFFSET MACRO offset:REQ
ENDM
CFI_DEF_CFA_REGISTER MACRO reg:REQ
ENDM
CFI_ADJUST_CFA_OFFSET MACRO offset:REQ
ENDM
CFI_OFFSET MACRO reg:REQ, offset:REQ
ENDM
CFI_REGISTER MACRO reg1:REQ, reg2:REQ
ENDM
CFI_REL_OFFSET MACRO reg:REQ, offset:REQ
ENDM
CFI_SAME_VALUE MACRO reg:REQ
ENDM

#else /***********************************************************************/

/* Force intel syntax */
.intel_syntax noprefix

/* Put dwarf debug info in the .dwarf_debug section, which will be properly stripped */
.cfi_sections .debug_frame

.altmacro

/* Explicit radix in GAS syntax */
#define BIN(x) 0b##x
#define OCT(x) 0##x
#define DEC(x) x
#define HEX(x) 0x##x

/* Macro values need to be marked */
#define VAL(x) \x

#define CR  "\r"
#define LF  "\n"
#define NUL "\0"

/* Due to MASM's reverse syntax, we are forced to use a precompiler macro */
#define MACRO(...) .macro __VA_ARGS__
#define ENDM .endm

/* To avoid reverse syntax we provide a new macro .PROC, replacing PROC... */
.macro .PROC name
    .func \name
    \name:
#ifdef _X86_
    .cfi_startproc
#else
    .seh_proc \name
#endif
.endm
#define FUNC .PROC

/* ... and .ENDP, replacing ENDP */
.macro .ENDP
#ifdef _X86_
    .cfi_endproc
#else
    .seh_endproc
#endif
    .endfunc
.endm
#define ENDFUNC .ENDP

/* MASM compatible PUBLIC */
.macro PUBLIC symbol
    .global \symbol
.endm

/* No special marking of global labels */
.macro GLOBAL_LABEL label
    \label:
.endm

/* Dummy ASSUME */
.macro ASSUME p1 p2 p3 p4 p5 p6 p7 p8
.endm

/* MASM needs an end tag for segments */
.macro .endcode16
.endm

/* MASM compatible ALIGN */
#define ALIGN .align

/* MASM compatible REPEAT, additional ENDR */
#define REPEAT .rept
#define ENDR .endr

#define lXdtPrefix

.macro ljmp segment, offset
    .byte 0xEA
    .long offset
    .word segment
.endm

.macro ljmp16 segment, offset
    .byte 0xEA
    .word offset
    .word segment
.endm

.macro data32 opcode:vararg
    .byte 0x66
    opcode
.endm

.macro retf
    lret
.endm

/* MASM compatible EXTERN */
.macro EXTERN name
.endm

/* MASM needs an END tag */
#define END

.macro .MODEL model
.endm

.macro .code
    .text
.endm

.macro .const
    .section .rdata
.endm

/*
 * See https://docs.microsoft.com/en-us/cpp/assembler/masm/dot-fpo
 * and https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_fpo_data
 */
.macro FPO cdwLocals, cdwParams, cbProlog, cbRegs, fUseBP, cbFrame
    .if (cbFrame == FRAME_TRAP)
        .cfi_signal_frame
    .endif
.endm

/* Macros for x64 stack unwind OPs */
.macro .allocstack size
    .seh_stackalloc \size
.endm

.macro .pushframe param
    /*
     * FIXME. .seh_pushframe doesn't accept code argument.
     * Patch sent.
     */
    .seh_pushframe \param
.endm

.macro .pushreg reg
    .seh_pushreg \reg
.endm

.macro .savereg reg, offset
    .seh_savereg \reg, \offset
.endm

.macro .savexmm128 reg, offset
    .seh_savexmm \reg, \offset
.endm

.macro .setframe reg, offset
    .seh_setframe \reg, \offset
.endm

.macro .endprolog
    .seh_endprologue
.endm

.macro absolute address
    __absolute__address__ = \address
.endm

.macro resb name, size
    \name = __absolute__address__
    __absolute__address__ = __absolute__address__ + \size
.endm

.macro UNIMPLEMENTED2 file, line, func
    jmp 4f
1:  .ascii "Unimplemented %s (%s:%d)", CR, LF, NUL
2:  .asciz "\func"
3:  .asciz \file
4:
    sub rsp, 0x20
    lea rcx, 1b[rip]
    lea rdx, 2b[rip]
    lea r8, 3b[rip]
    mov r9, \line
    call DbgPrint
    add rsp, 0x20
.endm
#define UNIMPLEMENTED UNIMPLEMENTED2 __FILE__, __LINE__,

/* MASM/ML uses ".if" for runtime conditionals, and "if" for compile time
   conditionals. We therefore use "if", too. .if shouldn't be used at all */
#define if .if
#define endif .endif
#define else .else
#define elseif .elseif

/* CFI annotations */
#define CFI_STARTPROC .cfi_startproc
#define CFI_ENDPROC .cfi_endproc
#define CFI_DEF_CFA .cfi_def_cfa
#define CFI_DEF_CFA_OFFSET .cfi_def_cfa_offset
#define CFI_DEF_CFA_REGISTER .cfi_def_cfa_register
#define CFI_ADJUST_CFA_OFFSET .cfi_adjust_cfa_offset
#define CFI_OFFSET .cfi_offset
#define CFI_REGISTER .cfi_register
#define CFI_REL_OFFSET .cfi_rel_offset
#define CFI_SAME_VALUE .cfi_same_value

#endif

#endif /* __ASM_INC__ */
