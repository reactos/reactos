/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of ldexp.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/ldexp.c
 *              blob: f4d1cd6af5d8b815064251c9d3f062709f2d87d0
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include "libm.h"

double ldexp(double x, int n)
{
	return scalbn(x, n);
}
