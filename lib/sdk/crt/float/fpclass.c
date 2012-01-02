/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/float/fpclass.c
 * PURPOSE:     Floating-point classes
 * PROGRAMER:   Pierre Schweitzer (pierre@reactos.org)
 * REFERENCE:   http://babbage.cs.qc.cuny.edu/IEEE-754/References.xhtml
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


    /* With 0x7ff, it can only be infinity or NaN */
    if (d.d->exponent == 0x7ff)
    {
        if (d.d->mantissah == 0 && d.d->mantissal == 0)
        {
            return (d.d->sign == 0) ? _FPCLASS_PINF : _FPCLASS_NINF;
        }
        /* Windows will never return Signaling NaN */
        else
        {
            return _FPCLASS_QNAN;
        }
    }

    /* With 0, it can only be zero or denormalized number */
    if (d.d->exponent == 0)
    {
        if (d.d->mantissah == 0 && d.d->mantissal == 0)
        {
            return (d.d->sign == 0) ? _FPCLASS_PZ : _FPCLASS_NZ;
        }
        else
        {
            return (d.d->sign == 0) ? _FPCLASS_PD : _FPCLASS_ND;
        }
    }
    /* Only remain normalized numbers */
    else
    {
        return (d.d->sign == 0) ? _FPCLASS_PN : _FPCLASS_NN;
    }
}
