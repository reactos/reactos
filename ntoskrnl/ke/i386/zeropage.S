/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fast zeroing of pages
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <asm.inc>

/* FUNCTIONS ****************************************************************/
.code

KeZeroPagesNoSSE:
    push edi
    mov edi, ecx
    mov ecx, edx
    shr ecx, 2
    xor eax, eax
    rep stosd
    pop edi
    ret

PUBLIC @KeZeroPages@8
@KeZeroPages@8:
    /* Kernel will overwrite this with nop if SSE is available */
    jmp KeZeroPagesNoSSE
    /* Save XMM0 & zero it */
    sub esp, 31
    mov eax, esp
    and eax, HEX(FFFFFFF0)
    movaps xmmword ptr [eax], xmm0
    pxor xmm0, xmm0
    /* Calculate buffer end */
    add edx, ecx
    /* Loop */
do_zero:
    movntps xmmword ptr [ecx], xmm0
    add ecx, 16
    cmp ecx, edx
    jne do_zero
#ifdef CONFIG_SMP
    sfence
#endif
    /* Restore */
    movaps xmm0, xmmword ptr [eax]
    add esp, 31
    ret

END
