#include <msvcrt/float.h>


double _nextafter(double x, double y)
{
	if (x == y)
		return x;

	if (isnan(x) || isnan(y))
		return x;

	return x;
}
