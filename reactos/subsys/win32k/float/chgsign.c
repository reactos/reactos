#include <crtdll/float.h>
#include <crtdll/internal/ieee.h>

double _chgsign( double __x )
{
	double_t *x = (double_t *)&x;
	if ( x->sign == 1 )
		x->sign = 0;
	else 
		x->sign = 1;

	return __x;
}
