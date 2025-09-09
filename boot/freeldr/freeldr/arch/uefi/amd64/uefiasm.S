/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     x64 assembly UEFI escape helper functions
 * COPYRIGHT:   Copyright 2023 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

EXTERN UefiServiceStack:QWORD
EXTERN BasicStack:QWORD
EXTERN ExecuteLoaderCleanly:PROC
EXTERN UefiExitBootServices:PROC
/* If needed: */
/* EXTERN EfiImageHandle:QWORD */
/* EXTERN EfiMapKey:QWORD */

.code64

// void _exituefi(VOID)
PUBLIC _exituefi
_exituefi:
    /* Save non-volatile registers */
    push    rbp
    push    rsi
    push    rdi
    push    rbx

    /* Save the old stack */
    mov     rbx, rsp

    /* Load the new UEFI service stack */
    xor     rbp, rbp
    mov     rsp, qword ptr [rip + UefiServiceStack]
    
    /* Ensure 16-byte alignment (defensive programming) */
    and     rsp, -16

    /* Call UefiExitBootServices (wrapper) with proper shadow space + alignment */
    /* If calling firmware directly, set RCX=ImageHandle, RDX=MapKey first. */
    /* mov   rcx, qword ptr [rip + EfiImageHandle] */
    /* mov   rdx, qword ptr [rip + EfiMapKey]      */

    sub     rsp, 0x20                /* 32-byte shadow space for Win x64 ABI */
    call    UefiExitBootServices
    
    /* Check return value (EFI_SUCCESS = 0) */
    test    rax, rax
    jz      .exit_success
    
    /* Error case: you may want to handle this differently */
    /* For now, continue anyway, but could retry or halt */
    
.exit_success:
    add     rsp, 0x20

    /* Restore old stack */
    mov     rsp, rbx

    /* Restore non-volatiles */
    pop     rbx
    pop     rdi
    pop     rsi
    pop     rbp

    /* Disable interrupts before loading new GDT */
    cli
    
#ifdef _USE_ML
    lgdt    _gdtptr                  /* ml64: 10-byte descriptor inferred */
#else
    lgdt    [rip + _gdtptr]          /* GAS: RIP-relative load of 10-byte GDTR */
#endif
#endif

    /* Re-enable interrupts if appropriate for your environment */
    /* sti */

    /* All done */
    ret

// void _changestack(VOID)
PUBLIC _changestack
_changestack:
    /* Save current stack pointer and switch to BasicStack */
    mov     rax, rsp
    mov     rsp, qword ptr [rip + BasicStack]
    
    /* Ensure 16-byte alignment (defensive programming) */
    and     rsp, -16

    /* x64 ABI: shadow space + alignment before call. */
    /* Pass old RSP in RCX if ExecuteLoaderCleanly expects it (common pattern). */
    sub     rsp, 0x20                /* 32-byte shadow space */
    mov     rcx, rax                 /* RCX = previous RSP (argument 1) */
    call    ExecuteLoaderCleanly
    add     rsp, 0x20
    
    /* Note: ExecuteLoaderCleanly should not return in normal operation */
    /* If it does return, this is likely an error condition */
    
    ret

.align 8
gdt:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00: NULL descriptor */
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 08: unused/reserved */
    .word HEX(0000), HEX(0000), HEX(9A00), HEX(0020) /* 10: long mode CS (access=0x9A, L-bit=1) */
    .word HEX(FFFF), HEX(0000), HEX(9200), HEX(0000) /* 18: long mode DS (RW, DPL=0) */
    .word HEX(FFFF), HEX(0000), HEX(9E00), HEX(0000) /* 20: 16-bit real mode CS */
    .word HEX(FFFF), HEX(0000), HEX(9200), HEX(0000) /* 28: 16-bit real mode DS */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(00CF) /* 30: 32-bit compat mode CS */

    /* GDT table pointer: 10 bytes (limit:2 + base:8) */
_gdtptr:
    .word HEX(0037)                  /* Limit = 7 entries * 8 - 1 = 0x37 */
    .quad gdt                        /* Base Address */

END