/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/amd64/asmmacro.S
 * PURPOSE:         ASM macros for GAS and MASM/ML64
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifndef __ASM_INC__
#define __ASM_INC__

#ifdef _USE_ML

/* Allow ".name" identifiers */
OPTION DOTNAME

#ifdef _M_IX86
.686P
.XMM
.MODEL FLAT
ASSUME CS:NOTHING, DS:NOTHING, ES:NOTHING, FS:NOTHING, GS:NOTHING
#endif

/* Hex numbers need to be in 01ABh format */
#define HEX(x) 0##x##h

/* Macro values need to be marked */
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

/* check http://msdn.microsoft.com/en-us/library/9c9k076y%28VS.80%29.aspx
   and http://msdn.microsoft.com/en-us/library/ms679352%28VS.85%29.aspx */
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
    .text SEGMENT use16
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

.double MACRO args:VARARG
    dq args
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

UNIMPLEMENTED MACRO name
ENDM

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

#else /***********************************************************************/

/* Force intel syntax */
.intel_syntax noprefix

.altmacro

/* Hex numbers need to be in 0x1AB format */
#define HEX(y) 0x##y

/* Macro values need to be marked */
#define VAL(x) \x

/* Due to MASM's reverse syntax, we are forced to use a precompiler macro */
#define MACRO(...) .macro __VA_ARGS__
#define ENDM .endm

/* To avoid reverse syntax we provide a new macro .PROC, replacing PROC... */
.macro .PROC name
    .func \name
#ifdef _X86_
    /* x86 gas expects a label with _ prefix */
    _\name:
#endif
    \name:
    .cfi_startproc
    .equ cfa_current_offset, -8
.endm
#define FUNC .PROC

/* ... and .ENDP, replacing ENDP */
.macro .ENDP
    .cfi_endproc
    .endfunc
.endm
#define ENDFUNC .ENDP

/* MASM compatible PUBLIC */
.macro PUBLIC symbol
    .global \symbol
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

.macro ljmp segment, offset
    jmp far ptr \segment:\offset
.endm

.macro ljmp16 segment, offset
    jmp far ptr \segment:\offset
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

/* check http://msdn.microsoft.com/en-us/library/9c9k076y%28VS.80%29.aspx
   and http://msdn.microsoft.com/en-us/library/ms679352%28VS.85%29.aspx */
.macro FPO cdwLocals, cdwParams, cbProlog, cbRegs, fUseBP, cbFrame
    /* dummy */
.endm

/* Macros for x64 stack unwind OPs */

.macro .allocstack size
    .cfi_adjust_cfa_offset \size
    .set cfa_current_offset, cfa_current_offset - \size
.endm

code = 1
.macro .pushframe param=0
    .if (\param)
        .cfi_adjust_cfa_offset 0x30
        .set cfa_current_offset, cfa_current_offset - 0x30
    .else
        .cfi_adjust_cfa_offset 0x28
        .set cfa_current_offset, cfa_current_offset - 0x28
    .endif
.endm

.macro .pushreg reg
    .cfi_adjust_cfa_offset 8
    .equ cfa_current_offset, cfa_current_offset - 8
    .cfi_offset \reg, cfa_current_offset
.endm

.macro .savereg reg, offset
    // checkme!!!
    .cfi_offset \reg, \offset
.endm

.macro .savexmm128 reg, offset
    // checkme!!!
    .cfi_offset \reg, \offset
.endm

.macro .setframe reg, offset
    .cfi_def_cfa reg, \offset
    .equ cfa_current_offset, \offset
.endm

.macro .endprolog
.endm

.macro absolute address
    __absolute__address__ = \address
ENDM

.macro resb name, size
    \name = __absolute__address__
    __absolute__address__ = __absolute__address__ + \size
ENDM

.macro UNIMPLEMENTED2 file, line, func

    jmp 3f
1:  .asciz "\func"
2:  .asciz \file
3:
    sub rsp, 0x20
    lea rcx, MsgUnimplemented[rip]
    lea rdx, 1b[rip]
    lea r8, 2b[rip]
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

#define CR "\r"
#define LF "\n"
#define NUL "\0"

#endif

/* Common definitions for FPO macro
   see http://msdn.microsoft.com/en-us/library/ms679352%28VS.85%29.aspx */
#define FRAME_FPO 0
#define FRAME_TRAP 1
#define FRAME_TSS 2
#define FRAME_NONFPO 3

#endif /* __ASM_INC__ */
