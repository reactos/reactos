#include <crtdll/math.h>
#include <crtdll/float.h>

int isnan(double x)
{
	if ( x>= 0.0 && x < HUGE_VAL ) 
		return FALSE;
	else if ( x <= 0.0 && x > HUGE_VAL )
		return FALSE;

	return TRUE;
}