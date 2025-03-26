/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 asm implementation of fabsf
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

PUBLIC fabsf
fabsf:
    /* Copy parameter into rcx */
#if !defined(_MSC_VER) || (_MSC_VER >= 1916)
    movq rcx, xmm0
#else
    /* Old ML64 version does not understand this form of movq and uses movd instead */
    movd rcx, xmm0
#endif

    /* Copy into eax */
    mov eax, ecx

    /* Clear sign bit in eax */
    btr eax, 31

    /* Set error bit in rcx */
    bts ecx, 22

    /* Check for NAN */
    cmp eax, HEX(7F800000)

    /* If eax is NAN, copy error result to rax */
    cmova eax, ecx

#if !defined(_MSC_VER) || (_MSC_VER >= 1916)
    movq xmm0, rax
#else
    /* Old ML64 version does not understand this form of movq and uses movd instead */
    movd xmm0, rax
#endif

    ret

END
