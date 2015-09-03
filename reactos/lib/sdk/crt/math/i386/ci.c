#include <precomp.h>

/*
 * @implemented
 */
double	CDECL	_CItan(void)
{
	FPU_DOUBLE(x);
	return tan(x);
}
/*
 * @implemented
 */
double	CDECL	_CIsinh(void)
{
	FPU_DOUBLE(x);
	return sinh(x);
}
/*
 * @implemented
 */
double	CDECL	_CIcosh(void)
{
	FPU_DOUBLE(x);
	return cosh(x);
}
/*
 * @implemented
 */
double	CDECL	_CItanh(void)
{
	FPU_DOUBLE(x);
	return tanh(x);
}
/*
 * @implemented
 */
double	CDECL	_CIasin(void)
{
	FPU_DOUBLE(x);
	return asin(x);
}
/*
 * @implemented
 */
double	CDECL	_CIacos(void)
{
	FPU_DOUBLE(x);
	return acos(x);
}
/*
 * @implemented
 */
double	CDECL	_CIatan(void)
{
	FPU_DOUBLE(x);
	return atan(x);
}
/*
 * @implemented
 */
double	CDECL	_CIatan2(void)
{
	FPU_DOUBLES(x, y);
	return atan2(y, x);
}
/*
 * @implemented
 */
double	CDECL	_CIexp(void)
{
	FPU_DOUBLE(x);
	return exp(x);
}
/*
 * @implemented
 */
double	CDECL	_CIlog10(void)
{
	FPU_DOUBLE(x);
	return log10(x);
}
/*
 * @implemented
 */
double	CDECL	_CIfmod(void)
{
	FPU_DOUBLES(x, y);
	return fmod(x, y);
}
