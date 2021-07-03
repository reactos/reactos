/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for real mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC _APEntry
PUBLIC _APEntryEnd

.code16
_APEntry:
    cli

    xor ax, ax
	mov ds, ax
	mov ss, ax
    mov fs, ax
    mov gs, ax

    /* Load the GDT and blank IDT */
#ifdef _USE_ML
    lgdt fword ptr cs:[gdtptr - _APEntry]
    lidt fword ptr cs:[idtdesc - _APEntry]
#else
    lgdt cs:[gdtptr - _APEntry]
    lidt cs:[idtdesc - _APEntry]
#endif

    mov eax, cr0
    or  eax, HEX(00000001)
    mov cr0, eax

    ljmp16 HEX(08), (HEX(4000) + _APEntryEnd - _APEntry)

    hlt

gdtptr:
     .word (gdtend - gdt - 1)
     .long HEX(4000) + gdt - _APEntry
idtdesc:
    .word HEX(0)
    .long HEX(0)
gdt: 
_NULLDesc:
      /* NULL Descriptor */
     .word HEX(0000)
     .word HEX(0000)
     .word HEX(0000)
     .word HEX(0000)
_KernelCS:
      /* 32-bit flat CS */
     .word HEX(FFFF)
     .word HEX(0000)
     .word HEX(9A00)
     .word HEX(00CF)
_KernelDS:
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
