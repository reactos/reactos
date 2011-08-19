#include <precomp.h>
#include <math.h>

double CDECL _CIsin(double x);
double CDECL _CIcos(double x);
double CDECL _CItan(double x);
double CDECL _CIsinh(double x);
double CDECL _CIcosh(double x);
double CDECL _CItanh(double x);
double CDECL _CIasin(double x);
double CDECL _CIacos(double x);
double CDECL _CIatan(double x);
double CDECL _CIatan2(double y, double x);
double CDECL _CIexp(double x);
double CDECL _CIlog(double x);
double CDECL _CIlog10(double x);
double CDECL _CIpow(double x, double y);
double CDECL _CIsqrt(double x);
double CDECL _CIfmod(double x, double y);


/*
 * @implemented
 */
double	CDECL	_CIsin(double x)
{
	return sin(x);
}
/*
 * @implemented
 */
double	CDECL	_CIcos(double x)
{
	return cos(x);
}
/*
 * @implemented
 */
double	CDECL	_CItan(double x)
{
	return tan(x);
}
/*
 * @implemented
 */
double	CDECL	_CIsinh(double x)
{
	return sinh(x);
}
/*
 * @implemented
 */
double	CDECL	_CIcosh(double x)
{
	return cosh(x);
}
/*
 * @implemented
 */
double	CDECL	_CItanh(double x)
{
	return tanh(x);
}
/*
 * @implemented
 */
double	CDECL	_CIasin(double x)
{
	return asin(x);
}
/*
 * @implemented
 */
double	CDECL	_CIacos(double x)
{
	return acos(x);
}
/*
 * @implemented
 */
double	CDECL	_CIatan(double x)
{
	return atan(x);
}
/*
 * @implemented
 */
double	CDECL	_CIatan2(double x, double y)
{
	return atan2(y, x);
}
/*
 * @implemented
 */
double	CDECL	_CIexp(double x)
{
	return exp(x);
}
/*
 * @implemented
 */
double	CDECL	_CIlog(double x)
{
	return log(x);
}
/*
 * @implemented
 */
double	CDECL	_CIlog10(double x)
{
	return log10(x);
}
/*
 * @implemented
 */
double	CDECL	_CIpow(double x, double y)
{
	return pow(x, y);
}
/*
 * @implemented
 */
double	CDECL	_CIsqrt(double x)
{
	return sqrt(x);
}
/*
 * @implemented
 */
double	CDECL	_CIfmod(double x, double y)
{
	return fmod(x, y);
}

