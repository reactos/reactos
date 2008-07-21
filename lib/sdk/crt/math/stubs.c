#include <precomp.h>
#include <math.h>

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


/*
 * @implemented
 */
double	_CIsin(double x)
{
	return sin(x);
}
/*
 * @implemented
 */
double	_CIcos(double x)
{
	return cos(x);
}
/*
 * @implemented
 */
double	_CItan(double x)
{
	return tan(x);
}
/*
 * @implemented
 */
double	_CIsinh(double x)
{
	return sinh(x);
}
/*
 * @implemented
 */
double	_CIcosh(double x)
{
	return cosh(x);
}
/*
 * @implemented
 */
double	_CItanh(double x)
{
	return tanh(x);
}
/*
 * @implemented
 */
double	_CIasin(double x)
{
	return asin(x);
}
/*
 * @implemented
 */
double	_CIacos(double x)
{
	return acos(x);
}
/*
 * @implemented
 */
double	_CIatan(double x)
{
	return atan(x);
}
/*
 * @implemented
 */
double	_CIatan2(double y, double x)
{
	return atan2(y, x);
}
/*
 * @implemented
 */
double	_CIexp(double x)
{
	return exp(x);
}
/*
 * @implemented
 */
double	_CIlog(double x)
{
	return log(x);
}
/*
 * @implemented
 */
double	_CIlog10(double x)
{
	return log10(x);
}
/*
 * @implemented
 */
double	_CIpow(double x, double y)
{
	return pow(x, y);
}
/*
 * @implemented
 */
double	_CIsqrt(double x)
{
	return sqrt(x);
}
/*
 * @implemented
 */
double	_CIfmod(double x, double y)
{
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


