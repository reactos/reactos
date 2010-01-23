/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <internal/ieee.h>

/*
 * @implemented
 */
double _scalb( double __x, long e )
{
	union
	{
		double*   __x;
                double_t*   x;
	} x;

	x.__x = &__x;

	x.x->exponent += e;

	return __x;
}
