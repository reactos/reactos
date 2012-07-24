#include <precomp.h>

/*
 * @implemented
 */
double	CDECL	_CIcos(void)
{
	FPU_DOUBLE(x);
	return cos(x);
}
