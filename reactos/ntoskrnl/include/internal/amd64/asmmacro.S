/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/internal/amd64/asmmacro.S
 * PURPOSE:         Macros for x64 assembly
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

.intel_syntax noprefix
.code64

/* Macros for x64 stack unwind OPs */

.macro .proc name
    .func name
    .global _\name
    _\name:
    .cfi_startproc
    .equ cfa_current_offset, -8
.endm

.macro .endproc
    .cfi_endproc
    .endfunc
.endm

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

.macro UNIMPLEMENTED2 line, func
    jmp 3f
    .equ expr, 12
1:  .asciz "\func"
2:  .asciz __FILE__
3:
    sub rsp, 0x20
    lea rcx, _MsgUnimplemented[rip]
    lea rdx, 1b[rip]
    lea r8, 2b[rip]
    mov r9, \line
    call _DbgPrint
    add rsp, 0x20
.endm
#define UNIMPLEMENTED UNIMPLEMENTED2 __LINE__,

