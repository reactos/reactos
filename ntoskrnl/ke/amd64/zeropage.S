/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fast zeroing of pages
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <asm.inc>

/* FUNCTIONS ****************************************************************/
.code

PUBLIC KeZeroPages
KeZeroPages:
    pxor xmm0, xmm0
    /* Calculate buffer end */
    add rdx, rcx
    /* Loop */
do_zero:
    movntps xmmword ptr [rcx], xmm0
    add rcx, 16
    cmp rcx, rdx
    jne do_zero
#ifdef CONFIG_SMP
    sfence
#endif
    ret

END
