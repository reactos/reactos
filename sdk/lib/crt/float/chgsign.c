/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/float/chgsign.c
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
double _chgsign( double __x )
{
	union
	{
	    double* __x;
	    double_s *x;
	} u;
	u.__x = &__x;

	if ( u.x->sign == 1 )
		u.x->sign = 0;
	else
		u.x->sign = 1;

	return __x;
}
