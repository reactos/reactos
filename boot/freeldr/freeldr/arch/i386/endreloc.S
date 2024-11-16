/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Finish the partial relocation to the expected address.
 * COPYRIGHT:   Copyright 2024 Daniel Victor <ilauncherdeveloper@gmail.com>
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

PUBLIC FinishRelocation
FinishRelocation:
    /* Check if it's loaded with NTLDR */
    cmp byte ptr [BSS_BootedViaNtldr], 0
    jz .IsNotNtldr

.FirstStep:
    /* Save the registers */
    push rax
    push rsi
    push rdi
    push rcx

    /* Clear the rax because it's will be used as offset */
    xor rax, rax

    /* Relocate Freeldr to NTLDR_TMP_BASE */
    mov rsi, qword ptr [BSS_NtldrAddress]
    mov rdi, NTLDR_TMP_BASE
    mov rcx, offset __bss_start__ - FREELDR_BASE
    rep movsb

    /* Jump to the final step relocation */
    mov rdi, offset .FinalStep + NTLDR_TMP_BASE - FREELDR_BASE
    push rdi
    ret

.FinalStep:
    /* If the NTLDR address is below than NTLDR_BASE then keep offset value */
    cmp qword ptr [BSS_NtldrAddress], NTLDR_BASE
    jb .IsChainloadedFreeldr

    /* Set the offset to 63kb to avoid data/code overwrite */
    mov rax, HEX(FFFF)

.IsChainloadedFreeldr:
    /* Relocate Freeldr back to FREELDR_BASE */
    mov rsi, NTLDR_TMP_BASE
    mov rdi, FREELDR_BASE
    mov rcx, offset __bss_start__ - FREELDR_BASE

    /* Add the offset from source and destination registers */
    add rsi, rax
    add rdi, rax

    /* Subtract the offset from counter register */
    sub rcx, rax

    /* Start the relocation */
    rep movsb

    /* Freeldr has now been relocated, reset the flag */
    mov byte ptr [BSS_BootedViaNtldr], 0

    /* Restore the registers */
    pop rcx
    pop rdi
    pop rsi
    pop rax

    /* Save source register */
    push rsi

    /* Store the base address in the source register */
    mov rsi, qword ptr [BSS_BaseAddress]

    /* Set the base address with FREELDR base address */
    mov qword ptr [BSS_BaseAddress], FREELDR_BASE

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
