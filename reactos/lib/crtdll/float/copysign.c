#include <crtdll/float.h>
#include <crtdll/internal/ieee.h>

double _copysign (double __d, double __s)
{
	double_t *d = (double_t *)&__d;
	double_t *s = (double_t *)&__s;

	d->sign = s->sign;

	return __d;
}

