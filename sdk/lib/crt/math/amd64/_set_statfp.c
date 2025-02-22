/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 implementation of _set_statfp
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <xmmintrin.h>

__ATTRIBUTE_SSE__
void _set_statfp(uintptr_t mask)
{
    unsigned int csr = _mm_getcsr();
    _mm_setcsr((mask & _MM_EXCEPT_MASK) | csr);
}
