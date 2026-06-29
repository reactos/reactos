/*
 * sincos implementation
 *
 * Copyright 2021 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep implib
#endif

#include <math.h>

/* GCC may optimize a pair of sin(), cos() calls to a single sincos() call,
 * which is not exported by any msvcrt version. */

void sincos(double x, double *s, double *c)
{
    *s = sin(x);
    *c = cos(x);
}

void sincosf(float x, float *s, float *c)
{
    *s = sinf(x);
    *c = cosf(x);
}
