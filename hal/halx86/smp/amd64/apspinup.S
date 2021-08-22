/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for long mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC APSpinup
PUBLIC APSpinupEnd
PUBLIC APGDT
PUBLIC APIDT
PUBLIC TempPageTableLoc
PUBLIC APFinalOffset

.code64
APSpinup:
    hlt

APFinalOffset:

/* Write the main GDT */
.align 4 /* force 4-byte alignment */
APGDT:
    .word HEX(0) /* Padding */
gdtptr:
    .word HEX(0) /* Limit */
    .long HEX(0) /* Address */

/* Write the main IDT */
.align 4 /* force 4-byte alignment */
APIDT:
    .word HEX(0) /* Padding */
idtptr:
    .word HEX(0) /* Limit */
    .long HEX(0) /* Address */
TempPageTableLoc:
    .long HEX(0)
APSpinupEnd:
END
