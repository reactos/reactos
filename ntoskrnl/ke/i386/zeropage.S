/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fast zeroing of pages
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <asm.inc>

/* FUNCTIONS ****************************************************************/
.code

PUBLIC @KeZeroPages@8
@KeZeroPages@8:
    push edi
    mov edi, ecx
    mov ecx, edx
    shr ecx, 2
    xor eax, eax
    rep stosd
    pop edi
    ret

END
