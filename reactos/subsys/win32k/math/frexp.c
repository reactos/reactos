#include <crtdll/math.h>
#include <crtdll/stdlib.h>
#include <crtdll/internal/ieee.h>

double
frexp(double __x, int *exptr)
{
	double_t *x = (double_t *)&__x;
	
	if ( exptr != NULL )
		*exptr = x->exponent - 0x3FE;
		
	
	x->exponent = 0x3FE;
	
	return __x; 
}



