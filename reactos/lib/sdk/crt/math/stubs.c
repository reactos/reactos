#include <precomp.h>
#include <math.h>

#define FPU_DOUBLE(var) double var; \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )

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

/* The following functions are likely workarounds for the pentium fdiv bug */
void __stdcall _adj_fdiv_m32( unsigned int arg )
{
    FIXME("_adj_fdiv_m32 stub\n");
}
void __stdcall _adj_fdiv_m32i( int arg )
{
    FIXME("_adj_fdiv_m32i stub\n");
}

void __stdcall _adj_fdiv_m64( unsigned __int64 arg )
{
    FIXME("_adj_fdiv_m64 stub\n");
}

void _adj_fdiv_r(void)
{
    FIXME("_adj_fdiv_r stub\n");
}

void __stdcall _adj_fdivr_m32( unsigned int arg )
{
    FIXME("_adj_fdivr_m32i stub\n");
}

void __stdcall _adj_fdivr_m32i( int arg )
{
    FIXME("_adj_fdivr_m32i stub\n");
}

void __stdcall _adj_fdivr_m64( unsigned __int64 arg )
{
    FIXME("_adj_fdivr_m64 stub\n");
}

void _adj_fpatan(void)
{
    FIXME("_adj_fpatan stub\n");
}

void __stdcall _adj_fdiv_m16i( short arg )
{
    FIXME("_adj_fdiv_m16i stub\n");
}

void __stdcall _adj_fdivr_m16i( short arg )
{
    FIXME("_adj_fdivr_m16i stub\n");
}

void _adj_fprem(void)
{
    FIXME("_adj_fprem stub\n");
}

void _adj_fprem1(void)
{
    FIXME("_adj_fprem1 stub\n");
}

void _adj_fptan(void)
{
    FIXME("_adj_fptan stub\n");
}

void _safe_fdiv(void)
{
    FIXME("_safe_fdiv stub\n");
}

void _safe_fdivr(void)
{
    FIXME("_safe_fdivr stub\n");
}

void _safe_fprem(void)
{
    FIXME("_safe_fprem stub\n");
}

void _safe_fprem1(void)
{
    FIXME("_safe_fprem1 stub\n");
}


