/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of scalbn.
 * COPYRIGHT:   Imported from musl nexttowardf
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/nexttowardf.c
 *              blob: bbf172f9e65573e9059d26ea574aa8a96dfdef43
 */

#include "libm.h"

float nexttowardf(float x, long double y)
{
	union {float f; uint32_t i;} ux = {x};
	uint32_t e;

	if (_isnan(x) || _isnan(y))
		return x + y;
	if (x == y)
		return y;
	if (x == 0) {
		ux.i = 1;
		if (signbit(y))
			ux.i |= 0x80000000;
	} else if (x < y) {
		if (signbit(x))
			ux.i--;
		else
			ux.i++;
	} else {
		if (signbit(x))
			ux.i++;
		else
			ux.i--;
	}
	e = ux.i & 0x7f800000;
	/* raise overflow if ux.f is infinite and x is finite */
	if (e == 0x7f800000)
		FORCE_EVAL(x+x);
	/* raise underflow if ux.f is subnormal or zero */
	if (e == 0)
		FORCE_EVAL(x*x + ux.f*ux.f);
	return ux.f;
}
