#include <msvcrti.h>


double _nextafter( double x, double y )
{
	if ( x == y)
		return x;

	if ( _isnan(x) || _isnan(y) )
		return x;

	return x;
}
