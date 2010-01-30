/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/amd64/asmmacro.S
 * PURPOSE:         ASM macros for for GAS and MASM/ML64
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifdef _MSC_VER

/* Allow ".name" identifiers */
OPTION DOTNAME

/* Hex numbers need to be in 01ABh format */
#define HEX(x) 0##x##h

/* Macro values need to be marked */
#define VAL(x) x

/* MASM/ML doesn't want explicit [rip] addressing */
#define RIP(address) address

/* Due to MASM's reverse syntax, we are forced to use a precompiler macro */
#define MACRO(name, ...) name MACRO __VA_ARGS__

/* To avoid reverse syntax we provide a new macro .PROC, replacing PROC... */
.PROC MACRO namex
    namex PROC FRAME
ENDM

/* ... and .ENDP, replacing ENDP */
.ENDP MACRO name
    name ENDP
ENDM

/* MASM doesn't have an ASCIIZ macro */
.ASCIIZ MACRO text
    DB text, 0
ENDM

/* We need this to distinguish repeat from macros */
#define ENDR ENDM

#else /***********************************************************************/

/* Force intel syntax */
.intel_syntax noprefix
.code64

/* Hex numbers need to be in 0x1AB format */
#define HEX(x) 0x##x

/* Macro values need to be marked */
#define VAL(x) \x

/* GAS needs explicit [rip] addressing */
#define RIP(address) address##[rip]

/* Due to MASM's reverse syntax, we are forced to use a precompiler macro */
#define MACRO(...) .macro __VA_ARGS__
#define ENDM .endm

/* To avoid reverse syntax we provide a new macro .PROC, replacing PROC... */
.macro .PROC name
    .func \name
    \name:
    .cfi_startproc
    .equ cfa_current_offset, -8
.endm

/* ... and .ENDP, replacing ENDP */
.macro .ENDP name
    .cfi_endproc
    .endfunc
.endm

/* MASM compatible PUBLIC */
#define PUBLIC .global

/* MASM compatible ALIGN */
#define ALIGN .align

/* MASM compatible REPEAT, additional ENDR */
#define REPEAT .rept
#define ENDR .endr

/* MASM compatible EXTERN */
.macro EXTERN name
.endm

/* MASM needs an END tag */
#define END

/* Macros for x64 stack unwind OPs */

.macro .allocstack size
    .cfi_adjust_cfa_offset \size
    .set cfa_current_offset, cfa_current_offset - \size
.endm

.macro .pushframe code
    .if (\code == 0)
        .cfi_adjust_cfa_offset 0x28
        .set cfa_current_offset, cfa_current_offset - 0x28
    .else
        .cfi_adjust_cfa_offset 0x30
        .set cfa_current_offset, cfa_current_offset - 0x30
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

.macro UNIMPLEMENTED2 file, line, func
    jmp 3f
    .equ expr, 12
1:  .asciz "\func"
2:  .asciz "\file"
3:
    sub rsp, 0x20
    lea rcx, _MsgUnimplemented[rip]
    lea rdx, 1b[rip]
    lea r8, 2b[rip]
    mov r9, \line
    call _DbgPrint
    add rsp, 0x20
.endm
#define UNIMPLEMENTED UNIMPLEMENTED2 __FILE__, __LINE__,

/* MASM/ML uses ".if" for runtime conditionals, and "if" for compile time
   conditionals. We therefore use "if", too. .if shouldn't be used at all */
#define if .if
#define endif .endif

#endif
