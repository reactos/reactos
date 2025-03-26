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

.code64

// void _exituefi(VOID)
PUBLIC _exituefi
_exituefi:
    /* Save non-volatile registers */
    push rbp
    push rsi
    push rdi
    push rbx

    /* Save the old stack */
    mov rbx, rsp

    /* Load the new stack */
    xor rbp, rbp
    mov rsp, qword ptr UefiServiceStack[rip]

    /* Call the entry routine, passing the parameters */
    mov rax, UefiExitBootServices[rip]
    call rax

    /* Retore old stack */
    mov rsp, rbx

    /* Retore non-volatiles */
    pop rbx
    pop rdi
    pop rsi
    pop rbp

#ifdef _USE_ML
    lgdt fword ptr [_gdtptr]
#else
    lgdt cs:[_gdtptr][rip] /* GAS isn't my friend - avoid letting it generate absolute addressing */
#endif

    /* All done */
    ret

// void _changestack(VOID)
PUBLIC _changestack
_changestack:
    mov rax, rsp
    mov rsp, BasicStack[rip]
    push rax
    call ExecuteLoaderCleanly[rip]
    ret

.align 8
gdt:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00: NULL descriptor */
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 08: */
    .word HEX(0000), HEX(0000), HEX(9800), HEX(0020) /* 10: long mode CS */
    .word HEX(FFFF), HEX(0000), HEX(F300), HEX(00CF) /* 18: long mode DS */
    .word HEX(FFFF), HEX(0000), HEX(9E00), HEX(0000) /* 20: 16-bit real mode CS */
    .word HEX(FFFF), HEX(0000), HEX(9200), HEX(0000) /* 28: 16-bit real mode DS */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(00CF) /* 30: compat mode CS */

/* GDT table pointer */
_gdtptr:
    .word HEX(37) /* Limit */
#ifdef _USE_ML
    .quad gdt     /* Base Address */
#else
    .quad gdt, 0  /* Base Address */
#endif

END
