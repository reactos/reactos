#include <precomp.h>

/*
 * @implemented
 */
double	CDECL	_CIsqrt(void)
{
	FPU_DOUBLE(x);
	return sqrt(x);
}
