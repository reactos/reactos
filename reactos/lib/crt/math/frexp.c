#include <math.h>
#include <stdlib.h>
#include <internal/ieee.h>

/*
 * @implemented
 */
double
frexp(double __x, int *exptr)
{
	union
	{
		double*   __x;
		double_t*   x;
	} x;

	x.__x = &__x;

	if ( exptr != NULL )
		*exptr = x.x->exponent - 0x3FE;


	x.x->exponent = 0x3FE;

	return __x;
}



