#include <msvcrt/math.h>


double	_CIsin(double x);
double	_CIcos(double x);
double	_CItan(double x);
double	_CIsinh(double x);
double	_CIcosh(double x);
double	_CItanh(double x);
double	_CIasin(double x);
double	_CIacos(double x);
double	_CIatan(double x);
double	_CIatan2(double y, double x);
double	_CIexp(double x);
double	_CIlog(double x);
double	_CIlog10(double x);
double	_CIpow(double x, double y);
double	_CIsqrt(double x);
double	_CIfmod(double x, double y);


double	_CIsin(double x)
{
	return sin(x);
}
double	_CIcos(double x)
{
	return cos(x);
}
double	_CItan(double x)
{
	return tan(x);
}
double	_CIsinh(double x)
{
	return sinh(x);
}
double	_CIcosh(double x)
{
	return cosh(x);
}
double	_CItanh(double x)
{
	return tanh(x);
}
double	_CIasin(double x)
{
	return asin(x);
}
double	_CIacos(double x)
{
	return acos(x);
}
double	_CIatan(double x)
{
	return atan(x);
}
double	_CIatan2(double y, double x)
{
	return atan2(y, x);
}
double	_CIexp(double x)
{
	return exp(x);
}
double	_CIlog(double x)
{
	return log(x);
}
double	_CIlog10(double x)
{
	return log10(x);
}
double	_CIpow(double x, double y)
{
	return pow(x, y);
}
double	_CIsqrt(double x)
{
	return sqrt(x);
}
double	_CIfmod(double x, double y)
{
	return fmod(x, y);
}
