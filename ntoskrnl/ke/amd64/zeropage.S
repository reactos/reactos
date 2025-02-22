/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fast zeroing of pages
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <asm.inc>

/* FUNCTIONS ****************************************************************/
.code

/* Benchmarking from Timo on some AMD machine:
    rep movsq : 128
    movaps 175
    movnti 620
    movntdq: 620
    movntps: 620
    MS KeZeroPages (movnti unrolled): 883
    MS KeZeroSinglePage (mov): 346

    whole discussion in https://github.com/reactos/reactos/pull/3765
    We stick with rep stosq.
*/

/*
 * VOID
 * KeZeroPages(PVOID Ptr, ULONG Size);
 */
PUBLIC KeZeroPages
FUNC KeZeroPages
    push rdi
    .PUSHREG rdi
    .ENDPROLOG

    mov rdi, rcx
    mov ecx, edx
    shr ecx, 3
    xor rax, rax
    rep stosq
    pop rdi
    ret
ENDFUNC

END
