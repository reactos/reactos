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

/*
 * @implemented
 */
double _nextafter( double x, double y )
{
    WARN("This function is not implemented correctly\n");
	if ( x == y)
		return x;

	if ( _isnan(x) || _isnan(y) )
		return x;

	return x;
}
