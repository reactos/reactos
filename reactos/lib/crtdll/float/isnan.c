#include <crtdll/math.h>
#include <crtdll/float.h>


int _isnan(double x)
{
	if ( x>= 0.0 && x < HUGE_VAL ) 
		return 0;
	else if ( x <= 0.0 && x > HUGE_VAL )
		return 0;

	return 1;
}

int _isinf(double x)
{
	if ( fabs(x) == HUGE_VAL ) 
		return 1;
	
	return 0;
}