#include <msvcrt/math.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/ieee.h>

/*
 * @implemented
 */
double
frexp(double __x, int *exptr)
{
	double_t *x = (double_t *)&__x;
	
	if ( exptr != NULL )
		*exptr = x->exponent - 0x3FE;
		
	
	x->exponent = 0x3FE;
	
	return __x; 
}



