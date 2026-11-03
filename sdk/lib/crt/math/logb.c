/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of logb.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/logb.c
 *              blob: 7f8bdfae19f8ca0a85b611d38a73695ccf588e43
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

#include "libm.h"

/*
special cases:
	logb(+-0) = -inf, and raise divbyzero
	logb(+-inf) = +inf
	logb(nan) = nan
*/

double logb(double x)
{
	if (!isfinite(x))
		return x * x;
	if (x == 0)
		return -1/(x*x);
	return ilogb(x);
}
