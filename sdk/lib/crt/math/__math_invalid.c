/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __math_invalid.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/__math_invalid.c
 *              blob: 177404900d161e4224c1424ec59f08353269d114
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include "libm.h"

double __math_invalid(double x)
{
	return (x - x) / (x - x);
}
