/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Relocate freeldr to the expected address.
 * COPYRIGHT:   Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#include <asm.inc>
#include <arch/pc/x86common.h>

#ifdef _M_IX86
#define rax eax
#define rsi esi
#define rsp esp
#define rdi edi
#define rcx ecx
#define qword dword
#endif

#ifdef _USE_ML
EXTERN __bss_start__:DWORD
EXTERN __bss_end__:DWORD
#endif

#ifdef _M_IX86
.code32
ASSUME ES:NOTHING, FS:NOTHING, GS:NOTHING
#define PointerSize 4
#else
.code64
#define PointerSize 8
#endif

PUBLIC RelocateFreeldr
RelocateFreeldr:
    /* Skip the entire relocation code if BSS_CurrentBaseAddress is same as FREELDR_BASE */
    cmp qword ptr [BSS_CurrentBaseAddress], FREELDR_BASE
    jz .IsNotNtldr

.FirstStep:
    /* Save the registers */
    push rsi
    push rdi
    push rcx

    /* Relocate Freeldr to TEMPCODE_BASE */
    mov rsi, qword ptr [BSS_CurrentBaseAddress]
    mov rdi, TEMPCODE_BASE
    mov rcx, offset __bss_start__ - FREELDR_BASE
    rep movsb

    /* Jump to the final step relocation */
    mov rdi, offset .FinalStep + TEMPCODE_BASE - FREELDR_BASE
    push rdi
    ret

.FinalStep:
    /* Relocate Freeldr back to FREELDR_BASE */
    mov rsi, TEMPCODE_BASE
    mov rdi, FREELDR_BASE
    mov rcx, offset __bss_start__ - FREELDR_BASE

    /* Start the relocation */
    rep movsb

    /* Restore the registers */
    pop rcx
    pop rdi
    pop rsi

    /* Save source register */
    push rsi

    /* Store the base address in the source register */
    mov rsi, qword ptr [BSS_CurrentBaseAddress]

    /* Set the base address with FREELDR base address */
    mov qword ptr [BSS_CurrentBaseAddress], FREELDR_BASE

    /* Subtract the return address from source register to get offset */
    sub qword ptr [rsp + PointerSize], rsi

    /* Sum the return address with FREELDR base address */
    add qword ptr [rsp + PointerSize], FREELDR_BASE

    /* Restore source register */
    pop rsi

.IsNotNtldr:
    /* Return to the relocated address */
    ret

END
