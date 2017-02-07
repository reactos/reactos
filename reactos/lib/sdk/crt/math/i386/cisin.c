#include <precomp.h>

/*
 * @implemented
 */
double	CDECL	_CIsin(void)
{
	FPU_DOUBLE(x);
	return sin(x);
}
