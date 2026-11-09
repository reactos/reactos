/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __math_oflow.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/__math_oflow.c
 *              blob: c85dbf982a0db036a7edf0a1bb32c74de208b185
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include "libm.h"

double __math_oflow(uint32_t sign)
{
	return __math_xflow(sign, 0x1p769);
}
