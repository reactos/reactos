#include <float.h>
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
