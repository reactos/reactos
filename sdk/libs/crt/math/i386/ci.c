#include <precomp.h>
#include <math.h>

#if defined(__GNUC__)
#define FPU_DOUBLE(var) double var; \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )
#elif defined(_MSC_VER)
#define FPU_DOUBLE(var) double var; \
	__asm { fstp [var] }; __asm { fwait };
#define FPU_DOUBLES(var1,var2) double var1,var2; \
	__asm { fstp [var1] }; __asm { fwait }; \
	__asm { fstp [var2] }; __asm { fwait };
#endif

/*
 * @implemented
 */
double	CDECL	_CIsin(void)
{
	FPU_DOUBLE(x);
	return sin(x);
}
/*
 * @implemented
 */
double	CDECL	_CIcos(void)
{
	FPU_DOUBLE(x);
	return cos(x);
}
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
	return atan2(x, y);
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
double	CDECL	_CIlog(void)
{
	FPU_DOUBLE(x);
	return log(x);
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
double	CDECL	_CIpow(void)
{
	FPU_DOUBLES(x, y);
	return pow(x, y);
}
/*
 * @implemented
 */
double	CDECL	_CIsqrt(void)
{
	FPU_DOUBLE(x);
	return sqrt(x);
}
/*
 * @implemented
 */
double	CDECL	_CIfmod(void)
{
	FPU_DOUBLES(x, y);
	return fmod(x, y);
}
