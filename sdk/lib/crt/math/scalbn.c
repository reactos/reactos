/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of scalbn.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/scalbn.c
 *              blob: 182f561068fda7bc8321dfc7991df8b6d58e1b56
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include <math.h>
#include <stdint.h>

double scalbn(double x, int n)
{
	union {double f; uint64_t i;} u;
	double_t y = x;

	if (n > 1023) {
		y *= 0x1p1023;
		n -= 1023;
		if (n > 1023) {
			y *= 0x1p1023;
			n -= 1023;
			if (n > 1023)
				n = 1023;
		}
	} else if (n < -1022) {
		/* make sure final n < -53 to avoid double
		   rounding in the subnormal range */
		y *= 0x1p-1022 * 0x1p53;
		n += 1022 - 53;
		if (n < -1022) {
			y *= 0x1p-1022 * 0x1p53;
			n += 1022 - 53;
			if (n < -1022)
				n = -1022;
		}
	}
	u.i = (uint64_t)(0x3ff+n)<<52;
	x = y * u.f;
	return x;
}
