/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * PURPOSE:     Floating-point number scaling
 * PROGRAMER:   Pierre Schweitzer (pierre@reactos.org)
 */

#include <precomp.h>

/*
 * @implemented
 */
double _scalb(double x, long exp)
{
	return ldexp(x, exp);
}
