#include <msvcrt/float.h>
#include <msvcrt/internal/ieee.h>


/*
 * @implemented
 */
double _chgsign( double __x )
{
	union
	{
	    double* __x;
	    double_t *x;
	} u;
	u.__x = &__x;

	if ( u.x->sign == 1 )
		u.x->sign = 0;
	else 
		u.x->sign = 1;

	return __x;
}
