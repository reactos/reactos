#include <crtdll/math.h>

double _cabs( struct _complex z )
{
	return hypot(z.x,z.y);
}




