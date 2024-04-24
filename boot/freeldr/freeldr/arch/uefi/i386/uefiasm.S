/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     i386 assembly UEFI escape helper functions
 * COPYRIGHT:   Copyright 2023 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

PUBLIC _gdtptr
PUBLIC _i386idtptr
PUBLIC __changestack
EXTERN _UefiServiceStack:DWORD
EXTERN _i386Idt:DWORD
EXTERN _ExecuteLoaderCleanly:PROC
EXTERN _UefiExitBootServices:PROC
EXTERN _BasicStack:DWORD
.code32

// void __exituefi(VOID)
PUBLIC __exituefi
__exituefi:
    push ebp
    push esi
    push edi
    push ebx

    /* Save the old stack */
    mov ebx, esp

    /* Load the new stack */
    xor ebp, ebp
    mov esp, _UefiServiceStack

    /* Call the entry routine, passing the parameters */
    call _UefiExitBootServices

    /* Retore old stack */
    mov esp, ebx

    /* Retore non-volatiles */
    pop ebx
    pop edi
    pop esi
    pop ebp

#ifdef _USE_ML
    lidt fword ptr ds:[_i386idtptr]
    lgdt fword ptr [_gdtptr]
#else
    lgdt cs:[_gdtptr]
    lidt _i386idtptr
#endif

    /* All done */
    ret

// void __reloadsegment(VOID)
PUBLIC __changestack
__changestack:
    mov eax, esp
    mov esp, _BasicStack
    push eax
    call _ExecuteLoaderCleanly
    ret

    .align 4    /* force 4-byte alignment */
gdt:
    /* NULL Descriptor */
   .word HEX(0000)
   .word HEX(0000)
   .word HEX(0000)
   .word HEX(0000)

    /* 32-bit flat CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9A00)
    .word HEX(00CF)

    /* 32-bit flat DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(00CF)

    /* 16-bit real mode CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9E00)
    .word HEX(0000)

    /* 16-bit real mode DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(0000)

/* GDT table pointer */
_gdtptr:
    .word HEX(27)  /* Limit */
    .long gdt, 0   /* Base Address */

_i386idtptr:
    .word 255      /* Limit */
    .long _i386Idt /* Base Address */
END
