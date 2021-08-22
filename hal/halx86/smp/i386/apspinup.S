/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for protected mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>
#include <ks386.inc>

PUBLIC _APSpinup
PUBLIC _APSpinupEnd
PUBLIC _APGDT
PUBLIC _APIDT
PUBLIC _APFinalOffset
PUBLIC _TempPageTableLoc
EXTERN _APEntry:DWORD

.code32
_APSpinup:
    cli

    mov ax, HEX(10)
	mov ds, ax
	mov ss, ax
    mov fs, ax
    mov gs, ax

    mov eax, ([ebx] + [_TempPageTableLoc - _APSpinup])
    mov cr3, eax

    mov eax, cr0
    or  eax, HEX(8001000)
    mov cr0, eax

    hlt

    /* Load the Final GDT And IDT */
#ifdef _USE_ML
    lgdt fword ptr ([ebx] + [gdtptr - _APSpinup])
    lidt fword ptr ([ebx] + [idtptr - _APSpinup])
#else
    lgdt ([ebx] + [gdtptr - _APSpinup])
    lidt ([ebx] + [idtptr - _APSpinup])
#endif

    .byte HEX(EA)
_APFinalOffset:
    .long HEX(0)
    .word HEX(08)
    nop
    hlt


/* Write the main GDT */
.align 4 /* force 4-byte alignment */
_APGDT:
    .word HEX(0) /* Padding */
gdtptr:
    .word HEX(0) /* Limit */
    .long HEX(0) /* Address */

/* Write the main IDT */
.align 4 /* force 4-byte alignment */
_APIDT:
    .word HEX(0) /* Padding */
idtptr:
    .word HEX(0) /* Limit */
    .long HEX(0) /* Address */
_TempPageTableLoc:
    .long HEX(0)
_APSpinupEnd:
END
