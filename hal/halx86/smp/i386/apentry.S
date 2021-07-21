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
PUBLIC _PageTableLocation

.code16
_APEntry:
    cli

    xor ax, ax
	mov ds, ax
	mov ss, ax
    mov fs, ax
    mov gs, ax

    /* bitshift 4 to get the address */
    mov bx, cs
    shl ebx, HEX(4)
    add ebx, gdt
    sub ebx, _APEntry
    mov dword ptr cs:[gdtoffset - _APEntry], ebx

    /* Load the GDT and blank IDT */
#ifdef _USE_ML
    lgdt fword ptr cs:[gdtptr - _APEntry]
    lidt fword ptr cs:[idtdesc - _APEntry]
#else
    lgdt cs:[gdtptr - _APEntry]
    lidt cs:[idtdesc - _APEntry]
#endif
    /* Enable Protected mode */
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax

    xor eax, eax
    mov eax, (HEX(4000) + _PageTableLocation - _APEntry)
    mov cr3, eax

/* Jump to APSpinup */
    .byte HEX(EA)
_APJumpOffset:
    .word HEX(0)
    .word HEX(08)

    hlt

_PageTableLocation:
    .word HEX(0)
gdtptr:
    .word (gdtend - gdt - 1)
gdtoffset:
    .long gdt - _APEntry
idtdesc:
    .word HEX(0)
    .long HEX(0)
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
gdtend:
    hlt
_APEntryEnd:
END
