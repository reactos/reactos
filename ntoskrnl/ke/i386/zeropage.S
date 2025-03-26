/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fast zeroing of pages
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <asm.inc>

/* FUNCTIONS ****************************************************************/
.code

/*
 * VOID
 * FASTCALL
 * KeZeroPages(void* ptr, ULONG Size)
 */
PUBLIC @KeZeroPages@8
FUNC @KeZeroPages@8
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    push edi
    mov edi, ecx
    mov ecx, edx
    shr ecx, 2
    xor eax, eax
    rep stosd
    pop edi
    ret
ENDFUNC

END
