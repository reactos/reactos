/* Copyright (C) 1991, 1992, 1995 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <precomp.h>

#if defined(_MSC_VER) && defined(_M_ARM)
#pragma function(_isnan)
#endif /* _MSC_VER */

/*
 * @implemented
 */
int _isnan(double __x)
{
	union
	{
		double*   __x;
		double_s*   x;
	} x;
    	x.__x = &__x;
	return ( x.x->exponent == 0x7ff  && ( x.x->mantissah != 0 || x.x->mantissal != 0 ));
}

/*
 * @implemented
 */
int _finite(double __x)
{
	union
	{
		double*   __x;
		double_s*   x;
	} x;

	x.__x = &__x;

    return ((x.x->exponent & 0x7ff) != 0x7ff);
}
