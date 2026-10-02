/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of trunc.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/trunc.c
 *              blob: d13711b5015e073fbe69598cdf61f5126cb89df4
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include "libm.h"

double trunc(double x)
{
	union {double f; uint64_t i;} u = {x};
	int e = (int)(u.i >> 52 & 0x7ff) - 0x3ff + 12;
	uint64_t m;

	if (e >= 52 + 12)
		return x;
	if (e < 12)
		e = 1;
	m = -1ULL >> e;
	if ((u.i & m) == 0)
		return x;
	FORCE_EVAL(x + 0x1p120f);
	u.i &= ~m;
	return u.f;
}
