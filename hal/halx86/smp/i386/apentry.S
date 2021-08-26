/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for real mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>
#include <ks386.inc>

PUBLIC _APEntry
PUBLIC _APEntryEnd
PUBLIC _APJumpOffset
PUBLIC _APGDT
PUBLIC _APIDT
PUBLIC _TempPageTableLoc
EXTERN _APSpinupEnd:DWORD

.code16
_APEntry:
    cli

    xor ax, ax
	mov ds, ax
	mov ss, ax
    mov fs, ax
    mov gs, ax

    /* Disable paging, Just in case */
    xor eax, eax
    mov cr0, eax

    /* Load offset of APStub for use later */
    xor ebx, ebx
    mov bx, cs
    shl ebx, HEX(4)
    add ebx, (_APEntryEnd - _APEntry)

    /* Load temp page tables and enable paging */
    mov eax, cs:[_TempPageTableLoc - _APEntry]
    mov cr3, eax

    mov eax, cr0
    or  eax, CR0_PG
    mov cr0, eax

    /* Load GDT */
    .byte HEX(66)
#ifdef _USE_ML
    lgdt fword ptr cs:[gdtptr - _APEntry]
    .byte HEX(66)
    lidt fword ptr cs:[idtptr - _APEntry]
#else
    lgdt cs:[gdtptr - _APEnry]
    .byte HEX(66)
    lidt cs:[idtptr - _APEnry]
#endif

    /* Enable Protected mode */
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax

    .byte HEX(66)
    .byte HEX(EA)
_APJumpOffset:
    .word HEX(0)
    .word HEX(08)

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
_APEntryEnd:
.endcode16
END
