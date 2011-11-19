/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/float/fpclass.c
 * PURPOSE:     Floating-point classes
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <float.h>
#include <internal/ieee.h>

/*
 * @implemented
 */
int _fpclass(double __d)
{
	union
	{
		double*	  __d;
		double_s*   d;
	} d;
	d.__d = &__d;

	if ( d.d->exponent == 0 ) {
		if ( d.d->mantissah == 0 && d.d->mantissal == 0 ) {
			if ( d.d->sign == 0 )
				return _FPCLASS_PZ;
			else
				return _FPCLASS_NZ;
		} else {
			if ( d.d->sign == 0 )
				return _FPCLASS_PD;
			else
				return _FPCLASS_ND;
		}
	}
	else if (d.d->exponent == 0x7ff ) {
		if ( d.d->mantissah == 0 && d.d->mantissal == 0 ) {
			if ( d.d->sign == 0 )
				return _FPCLASS_PINF;
			else
				return _FPCLASS_NINF;
		}
		else if ( (d.d->mantissah & 0x80000) != 0 ) {
			return _FPCLASS_QNAN;
		}
	}
	return _FPCLASS_QNAN;
}
