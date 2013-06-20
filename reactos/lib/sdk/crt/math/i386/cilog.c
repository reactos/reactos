#include <precomp.h>

/*
 * @implemented
 */
double	CDECL	_CIlog(void)
{
	FPU_DOUBLE(x);
	return log(x);
}
