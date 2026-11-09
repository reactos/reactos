/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __math_uflow.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/__math_uflow.c
 *              blob: b90594aee14e54144ad4c1695a446fbba12395a4
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include "libm.h"

double __math_uflow(uint32_t sign)
{
	return __math_xflow(sign, 0x1p-767);
}
