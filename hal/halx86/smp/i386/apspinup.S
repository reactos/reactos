/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for protected mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC _APSpinup
PUBLIC _APSpinupEnd

.code32
_APSpinup:
    mov eax, cr0
    or  eax, HEX(80000000)
    mov cr0, eax

    hlt
_APSpinupEnd:
END
 