#include <precomp.h>

/*
 * @implemented
 */
double	CDECL	_CIpow(void)
{
	FPU_DOUBLES(x, y);
	return pow(x, y);
}
