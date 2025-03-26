/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 asm implementation of fabs
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

#ifdef _USE_ML
/* fabs is not allowed as label name, so create _fabs instead and alias fabs to it */
ALIAS <fabs> = <_fabs>
PUBLIC _fabs
_fabs:
#else
PUBLIC fabs
fabs:
#endif
    /* Copy parameter into rcx */
#if !defined(_MSC_VER) || (_MSC_VER >= 1916)
    movq rcx, xmm0
#else
    /* Old ML64 version does not understand this form of movq and uses movd instead */
    movd rcx, xmm0
#endif

    /* Copy into rax */
    mov rax, rcx

    /* Clear sign bit in rax */
    btr rax, 63

    /* Check for NAN */
    mov r8, HEX(7FF0000000000000)
    cmp rax, r8

    /* If it is NAN, copy original value back to rax */
    cmova rax, rcx

#if !defined(_MSC_VER) || (_MSC_VER >= 1916)
    movq xmm0, rax
#else
    /* Old ML64 version does not understand this form of movq and uses movd instead */
    movd xmm0, rax
#endif

    ret

END
