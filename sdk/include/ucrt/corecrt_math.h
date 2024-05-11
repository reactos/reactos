//
// corecrt_math.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The majority of the C Standard Library <math.h> functionality.
//
#pragma once
#ifndef _INC_MATH // include guard for 3rd party interop
#define _INC_MATH

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

#ifndef __assembler
    // Definition of the _exception struct, which is passed to the matherr function
    // when a floating point exception is detected:
    struct _exception
    {
        int    type;   // exception type - see below
        char*  name;   // name of function where error occurred
        double arg1;   // first argument to function
        double arg2;   // second argument (if any) to function
        double retval; // value to be returned by function
    };

    // Definition of the _complex struct to be used by those who use the complex
    // functions and want type checking.
    #ifndef _COMPLEX_DEFINED
        #define _COMPLEX_DEFINED

        struct _complex
        {
            double x, y; // real and imaginary parts
        };

        #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES && !defined __cplusplus
            // Non-ANSI name for compatibility
            #define complex _complex
        #endif
    #endif
#endif // __assembler



// On x86, when not using /arch:SSE2 or greater, floating point operations
// are performed using the x87 instruction set and FLT_EVAL_METHOD is 2.
// (When /fp:fast is used, floating point operations may be consistent, so
// we use the default types.)
#if defined _M_IX86 && _M_IX86_FP < 2 && !defined _M_FP_FAST
    typedef long double float_t;
    typedef long double double_t;
#else
    typedef float  float_t;
    typedef double double_t;
#endif



// Constant definitions for the exception type passed in the _exception struct
#define _DOMAIN     1   // argument domain error
#define _SING       2   // argument singularity
#define _OVERFLOW   3   // overflow range error
#define _UNDERFLOW  4   // underflow range error
#define _TLOSS      5   // total loss of precision
#define _PLOSS      6   // partial loss of precision

// Definitions of _HUGE and HUGE_VAL - respectively the XENIX and ANSI names
// for a value returned in case of error by a number of the floating point
// math routines.
#ifndef __assembler
    #ifndef _M_CEE_PURE
        extern double const _HUGE;
    #else
        double const _HUGE = System::Double::PositiveInfinity;
    #endif
#endif

#ifndef _HUGE_ENUF
    #define _HUGE_ENUF  1e+300  // _HUGE_ENUF*_HUGE_ENUF must overflow
#endif

#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))
#define HUGE_VAL   ((double)INFINITY)
#define HUGE_VALF  ((float)INFINITY)
#define HUGE_VALL  ((long double)INFINITY)
#ifndef _UCRT_NEGATIVE_NAN
// This operation creates a negative NAN adding a - to make it positive
#define NAN        (-(float)(INFINITY * 0.0F))
#else
// Keep this for backwards compatibility
#define NAN        ((float)(INFINITY * 0.0F))
#endif

#define _DENORM    (-2)
#define _FINITE    (-1)
#define _INFCODE   1
#define _NANCODE   2

#define FP_INFINITE  _INFCODE
#define FP_NAN       _NANCODE
#define FP_NORMAL    _FINITE
#define FP_SUBNORMAL _DENORM
#define FP_ZERO      0

#define _C2          1  // 0 if not 2's complement
#define FP_ILOGB0   (-0x7fffffff - _C2)
#define FP_ILOGBNAN 0x7fffffff

#define MATH_ERRNO        1
#define MATH_ERREXCEPT    2
#define math_errhandling  (MATH_ERRNO | MATH_ERREXCEPT)

// Values for use as arguments to the _fperrraise function
#define _FE_DIVBYZERO 0x04
#define _FE_INEXACT   0x20
#define _FE_INVALID   0x01
#define _FE_OVERFLOW  0x08
#define _FE_UNDERFLOW 0x10

#define _D0_C  3 // little-endian, small long doubles
#define _D1_C  2
#define _D2_C  1
#define _D3_C  0

#define _DBIAS 0x3fe
#define _DOFF  4

#define _F0_C  1 // little-endian
#define _F1_C  0

#define _FBIAS 0x7e
#define _FOFF  7
#define _FRND  1

#define _L0_C  3 // little-endian, 64-bit long doubles
#define _L1_C  2
#define _L2_C  1
#define _L3_C  0

#define _LBIAS 0x3fe
#define _LOFF  4

// IEEE 754 double properties
#define _DFRAC  ((unsigned short)((1 << _DOFF) - 1))
#define _DMASK  ((unsigned short)(0x7fff & ~_DFRAC))
#define _DMAX   ((unsigned short)((1 << (15 - _DOFF)) - 1))
#define _DSIGN  ((unsigned short)0x8000)

// IEEE 754 float properties
#define _FFRAC  ((unsigned short)((1 << _FOFF) - 1))
#define _FMASK  ((unsigned short)(0x7fff & ~_FFRAC))
#define _FMAX   ((unsigned short)((1 << (15 - _FOFF)) - 1))
#define _FSIGN  ((unsigned short)0x8000)

// IEEE 754 long double properties
#define _LFRAC  ((unsigned short)(-1))
#define _LMASK  ((unsigned short)0x7fff)
#define _LMAX   ((unsigned short)0x7fff)
#define _LSIGN  ((unsigned short)0x8000)

#define _DHUGE_EXP (int)(_DMAX * 900L / 1000)
#define _FHUGE_EXP (int)(_FMAX * 900L / 1000)
#define _LHUGE_EXP (int)(_LMAX * 900L / 1000)

#define _DSIGN_C(_Val)  (((_double_val *)(char*)&(_Val))->_Sh[_D0_C] & _DSIGN)
#define _FSIGN_C(_Val)  (((_float_val  *)(char*)&(_Val))->_Sh[_F0_C] & _FSIGN)
#define _LSIGN_C(_Val)  (((_ldouble_val*)(char*)&(_Val))->_Sh[_L0_C] & _LSIGN)

void __cdecl _fperrraise(_In_ int _Except);

_Check_return_ _ACRTIMP short __cdecl _dclass(_In_ double _X);
_Check_return_ _ACRTIMP short __cdecl _ldclass(_In_ long double _X);
_Check_return_ _ACRTIMP short __cdecl _fdclass(_In_ float _X);

_Check_return_ _ACRTIMP int __cdecl _dsign(_In_ double _X);
_Check_return_ _ACRTIMP int __cdecl _ldsign(_In_ long double _X);
_Check_return_ _ACRTIMP int __cdecl _fdsign(_In_ float _X);

_Check_return_ _ACRTIMP int __cdecl _dpcomp(_In_ double _X, _In_ double _Y);
_Check_return_ _ACRTIMP int __cdecl _ldpcomp(_In_ long double _X, _In_ long double _Y);
_Check_return_ _ACRTIMP int __cdecl _fdpcomp(_In_ float _X, _In_ float _Y);

_Check_return_ _ACRTIMP short __cdecl _dtest(_In_ double* _Px);
_Check_return_ _ACRTIMP short __cdecl _ldtest(_In_ long double* _Px);
_Check_return_ _ACRTIMP short __cdecl _fdtest(_In_ float* _Px);

_ACRTIMP short __cdecl _d_int(_Inout_ double* _Px, _In_ short _Xexp);
_ACRTIMP short __cdecl _ld_int(_Inout_ long double* _Px, _In_ short _Xexp);
_ACRTIMP short __cdecl _fd_int(_Inout_ float* _Px, _In_ short _Xexp);

_ACRTIMP short __cdecl _dscale(_Inout_ double* _Px, _In_ long _Lexp);
_ACRTIMP short __cdecl _ldscale(_Inout_ long double* _Px, _In_ long _Lexp);
_ACRTIMP short __cdecl _fdscale(_Inout_ float* _Px, _In_ long _Lexp);

_ACRTIMP short __cdecl _dunscale(_Out_ short* _Pex, _Inout_ double* _Px);
_ACRTIMP short __cdecl _ldunscale(_Out_ short* _Pex, _Inout_ long double* _Px);
_ACRTIMP short __cdecl _fdunscale(_Out_ short* _Pex, _Inout_ float* _Px);

_Check_return_ _ACRTIMP short __cdecl _dexp(_Inout_ double* _Px, _In_ double _Y, _In_ long _Eoff);
_Check_return_ _ACRTIMP short __cdecl _ldexp(_Inout_ long double* _Px, _In_ long double _Y, _In_ long _Eoff);
_Check_return_ _ACRTIMP short __cdecl _fdexp(_Inout_ float* _Px, _In_ float _Y, _In_ long _Eoff);

_Check_return_ _ACRTIMP short __cdecl _dnorm(_Inout_updates_(4) unsigned short* _Ps);
_Check_return_ _ACRTIMP short __cdecl _fdnorm(_Inout_updates_(2) unsigned short* _Ps);

_Check_return_ _ACRTIMP double __cdecl _dpoly(_In_ double _X, _In_reads_(_N) double const* _Tab, _In_ int _N);
_Check_return_ _ACRTIMP long double __cdecl _ldpoly(_In_ long double _X, _In_reads_(_N) long double const* _Tab, _In_ int _N);
_Check_return_ _ACRTIMP float __cdecl _fdpoly(_In_ float _X, _In_reads_(_N) float const* _Tab, _In_ int _N);

_Check_return_ _ACRTIMP double __cdecl _dlog(_In_ double _X, _In_ int _Baseflag);
_Check_return_ _ACRTIMP long double __cdecl _ldlog(_In_ long double _X, _In_ int _Baseflag);
_Check_return_ _ACRTIMP float __cdecl _fdlog(_In_ float _X, _In_ int _Baseflag);

_Check_return_ _ACRTIMP double __cdecl _dsin(_In_ double _X, _In_ unsigned int _Qoff);
_Check_return_ _ACRTIMP long double __cdecl _ldsin(_In_ long double _X, _In_ unsigned int _Qoff);
_Check_return_ _ACRTIMP float __cdecl _fdsin(_In_ float _X, _In_ unsigned int _Qoff);

// double declarations
typedef union
{   // pun floating type as integer array
    unsigned short _Sh[4];
    double _Val;
} _double_val;

// float declarations
typedef union
{   // pun floating type as integer array
    unsigned short _Sh[2];
    float _Val;
} _float_val;

// long double declarations
typedef union
{   // pun floating type as integer array
    unsigned short _Sh[4];
    long double _Val;
} _ldouble_val;

typedef union
{   // pun float types as integer array
    unsigned short _Word[4];
    float _Float;
    double _Double;
    long double _Long_double;
} _float_const;

extern const _float_const _Denorm_C,  _Inf_C,  _Nan_C,  _Snan_C, _Hugeval_C;
extern const _float_const _FDenorm_C, _FInf_C, _FNan_C, _FSnan_C;
extern const _float_const _LDenorm_C, _LInf_C, _LNan_C, _LSnan_C;

extern const _float_const _Eps_C,  _Rteps_C;
extern const _float_const _FEps_C, _FRteps_C;
extern const _float_const _LEps_C, _LRteps_C;

extern const double      _Zero_C,  _Xbig_C;
extern const float       _FZero_C, _FXbig_C;
extern const long double _LZero_C, _LXbig_C;

#define _FP_LT  1
#define _FP_EQ  2
#define _FP_GT  4

#ifndef __cplusplus

    #define _CLASS_ARG(_Val)                                  __pragma(warning(suppress:6334))(sizeof ((_Val) + (float)0) == sizeof (float) ? 'f' : sizeof ((_Val) + (double)0) == sizeof (double) ? 'd' : 'l')
    #define _CLASSIFY(_Val, _FFunc, _DFunc, _LDFunc)          (_CLASS_ARG(_Val) == 'f' ? _FFunc((float)(_Val)) : _CLASS_ARG(_Val) == 'd' ? _DFunc((double)(_Val)) : _LDFunc((long double)(_Val)))
    #define _CLASSIFY2(_Val1, _Val2, _FFunc, _DFunc, _LDFunc) (_CLASS_ARG((_Val1) + (_Val2)) == 'f' ? _FFunc((float)(_Val1), (float)(_Val2)) : _CLASS_ARG((_Val1) + (_Val2)) == 'd' ? _DFunc((double)(_Val1), (double)(_Val2)) : _LDFunc((long double)(_Val1), (long double)(_Val2)))

    #define fpclassify(_Val)      (_CLASSIFY(_Val, _fdclass, _dclass, _ldclass))
    #define _FPCOMPARE(_Val1, _Val2) (_CLASSIFY2(_Val1, _Val2, _fdpcomp, _dpcomp, _ldpcomp))

    #define isfinite(_Val)      (fpclassify(_Val) <= 0)
    #define isinf(_Val)         (fpclassify(_Val) == FP_INFINITE)
    #define isnan(_Val)         (fpclassify(_Val) == FP_NAN)
    #define isnormal(_Val)      (fpclassify(_Val) == FP_NORMAL)
    #define signbit(_Val)       (_CLASSIFY(_Val, _fdsign, _dsign, _ldsign))

    #define isgreater(x, y)      ((_FPCOMPARE(x, y) & _FP_GT) != 0)
    #define isgreaterequal(x, y) ((_FPCOMPARE(x, y) & (_FP_EQ | _FP_GT)) != 0)
    #define isless(x, y)         ((_FPCOMPARE(x, y) & _FP_LT) != 0)
    #define islessequal(x, y)    ((_FPCOMPARE(x, y) & (_FP_LT | _FP_EQ)) != 0)
    #define islessgreater(x, y)  ((_FPCOMPARE(x, y) & (_FP_LT | _FP_GT)) != 0)
    #define isunordered(x, y)    (_FPCOMPARE(x, y) == 0)

#else // __cplusplus
extern "C++"
{
    _Check_return_ inline int fpclassify(_In_ float _X) throw()
    {
        return _fdtest(&_X);
    }

    _Check_return_ inline int fpclassify(_In_ double _X) throw()
    {
        return _dtest(&_X);
    }

    _Check_return_ inline int fpclassify(_In_ long double _X) throw()
    {
        return _ldtest(&_X);
    }

    _Check_return_ inline bool signbit(_In_ float _X) throw()
    {
        return _fdsign(_X) != 0;
    }

    _Check_return_ inline bool signbit(_In_ double _X) throw()
    {
        return _dsign(_X) != 0;
    }

    _Check_return_ inline bool signbit(_In_ long double _X) throw()
    {
        return _ldsign(_X) != 0;
    }

    _Check_return_ inline int _fpcomp(_In_ float _X, _In_ float _Y) throw()
    {
        return _fdpcomp(_X, _Y);
    }

    _Check_return_ inline int _fpcomp(_In_ double _X, _In_ double _Y) throw()
    {
        return _dpcomp(_X, _Y);
    }

    _Check_return_ inline int _fpcomp(_In_ long double _X, _In_ long double _Y) throw()
    {
        return _ldpcomp(_X, _Y);
    }

    template <class _Trc, class _Tre> struct _Combined_type
    {   // determine combined type
        typedef float _Type;
    };

    template <> struct _Combined_type<float, double>
    {   // determine combined type
        typedef double _Type;
    };

    template <> struct _Combined_type<float, long double>
    {   // determine combined type
        typedef long double _Type;
    };

    template <class _Ty, class _T2> struct _Real_widened
    {   // determine widened real type
        typedef long double _Type;
    };

    template <> struct _Real_widened<float, float>
    {   // determine widened real type
        typedef float _Type;
    };

    template <> struct _Real_widened<float, double>
    {   // determine widened real type
        typedef double _Type;
    };

    template <> struct _Real_widened<double, float>
    {   // determine widened real type
        typedef double _Type;
    };

    template <> struct _Real_widened<double, double>
    {   // determine widened real type
        typedef double _Type;
    };

    template <class _Ty> struct _Real_type
    {   // determine equivalent real type
        typedef double _Type;   // default is double
    };

    template <> struct _Real_type<float>
    {   // determine equivalent real type
        typedef float _Type;
    };

    template <> struct _Real_type<long double>
    {   // determine equivalent real type
        typedef long double _Type;
    };

    template <class _T1, class _T2>
    _Check_return_ inline int _fpcomp(_In_ _T1 _X, _In_ _T2 _Y) throw()
    {   // compare _Left and _Right
        typedef typename _Combined_type<float,
            typename _Real_widened<
            typename _Real_type<_T1>::_Type,
            typename _Real_type<_T2>::_Type>::_Type>::_Type _Tw;
        return _fpcomp((_Tw)_X, (_Tw)_Y);
    }

    template <class _Ty>
    _Check_return_ inline bool isfinite(_In_ _Ty _X) throw()
    {
        return fpclassify(_X) <= 0;
    }

    template <class _Ty>
    _Check_return_ inline bool isinf(_In_ _Ty _X) throw()
    {
        return fpclassify(_X) == FP_INFINITE;
    }

    template <class _Ty>
    _Check_return_ inline bool isnan(_In_ _Ty _X) throw()
    {
        return fpclassify(_X) == FP_NAN;
    }

    template <class _Ty>
    _Check_return_ inline bool isnormal(_In_ _Ty _X) throw()
    {
        return fpclassify(_X) == FP_NORMAL;
    }

    template <class _Ty1, class _Ty2>
    _Check_return_ inline bool isgreater(_In_ _Ty1 _X, _In_ _Ty2 _Y) throw()
    {
        return (_fpcomp(_X, _Y) & _FP_GT) != 0;
    }

    template <class _Ty1, class _Ty2>
    _Check_return_ inline bool isgreaterequal(_In_ _Ty1 _X, _In_ _Ty2 _Y) throw()
    {
        return (_fpcomp(_X, _Y) & (_FP_EQ | _FP_GT)) != 0;
    }

    template <class _Ty1, class _Ty2>
    _Check_return_ inline bool isless(_In_ _Ty1 _X, _In_ _Ty2 _Y) throw()
    {
        return (_fpcomp(_X, _Y) & _FP_LT) != 0;
    }

    template <class _Ty1, class _Ty2>
    _Check_return_ inline bool islessequal(_In_ _Ty1 _X, _In_ _Ty2 _Y) throw()
    {
        return (_fpcomp(_X, _Y) & (_FP_LT | _FP_EQ)) != 0;
    }

    template <class _Ty1, class _Ty2>
    _Check_return_ inline bool islessgreater(_In_ _Ty1 _X, _In_ _Ty2 _Y) throw()
    {
        return (_fpcomp(_X, _Y) & (_FP_LT | _FP_GT)) != 0;
    }

    template <class _Ty1, class _Ty2>
    _Check_return_ inline bool isunordered(_In_ _Ty1 _X, _In_ _Ty2 _Y) throw()
    {
        return _fpcomp(_X, _Y) == 0;
    }
}  // extern "C++"
#endif // __cplusplus



#if _CRT_FUNCTIONS_REQUIRED

    _Check_return_ int       __cdecl abs(_In_ int _X);
    _Check_return_ long      __cdecl labs(_In_ long _X);
    _Check_return_ long long __cdecl llabs(_In_ long long _X);

    _Check_return_ double __cdecl acos(_In_ double _X);
    _Check_return_ double __cdecl asin(_In_ double _X);
    _Check_return_ double __cdecl atan(_In_ double _X);
    _Check_return_ double __cdecl atan2(_In_ double _Y, _In_ double _X);

    _Check_return_ double __cdecl cos(_In_ double _X);
    _Check_return_ double __cdecl cosh(_In_ double _X);
    _Check_return_ double __cdecl exp(_In_ double _X);
    _Check_return_ _CRT_JIT_INTRINSIC double __cdecl fabs(_In_ double _X);
    _Check_return_ double __cdecl fmod(_In_ double _X, _In_ double _Y);
    _Check_return_ double __cdecl log(_In_ double _X);
    _Check_return_ double __cdecl log10(_In_ double _X);
    _Check_return_ double __cdecl pow(_In_ double _X, _In_ double _Y);
    _Check_return_ double __cdecl sin(_In_ double _X);
    _Check_return_ double __cdecl sinh(_In_ double _X);
    _Check_return_ _CRT_JIT_INTRINSIC double __cdecl sqrt(_In_ double _X);
    _Check_return_ double __cdecl tan(_In_ double _X);
    _Check_return_ double __cdecl tanh(_In_ double _X);

    _Check_return_ _ACRTIMP double    __cdecl acosh(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl asinh(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl atanh(_In_ double _X);
    _Check_return_ _ACRTIMP  double    __cdecl atof(_In_z_ char const* _String);
    _Check_return_ _ACRTIMP  double    __cdecl _atof_l(_In_z_ char const* _String, _In_opt_ _locale_t _Locale);
    _Check_return_ _ACRTIMP double    __cdecl _cabs(_In_ struct _complex _Complex_value);
    _Check_return_ _ACRTIMP double    __cdecl cbrt(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl ceil(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl _chgsign(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl copysign(_In_ double _Number, _In_ double _Sign);
    _Check_return_ _ACRTIMP double    __cdecl _copysign(_In_ double _Number, _In_ double _Sign);
    _Check_return_ _ACRTIMP double    __cdecl erf(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl erfc(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl exp2(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl expm1(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl fdim(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double    __cdecl floor(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl fma(_In_ double _X, _In_ double _Y, _In_ double _Z);
    _Check_return_ _ACRTIMP double    __cdecl fmax(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double    __cdecl fmin(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double    __cdecl frexp(_In_ double _X, _Out_ int* _Y);
    _Check_return_ _ACRTIMP double    __cdecl hypot(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double    __cdecl _hypot(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP int       __cdecl ilogb(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl ldexp(_In_ double _X, _In_ int _Y);
    _Check_return_ _ACRTIMP double    __cdecl lgamma(_In_ double _X);
    _Check_return_ _ACRTIMP long long __cdecl llrint(_In_ double _X);
    _Check_return_ _ACRTIMP long long __cdecl llround(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl log1p(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl log2(_In_ double _X);
    _Check_return_ _ACRTIMP double    __cdecl logb(_In_ double _X);
    _Check_return_ _ACRTIMP long      __cdecl lrint(_In_ double _X);
    _Check_return_ _ACRTIMP long      __cdecl lround(_In_ double _X);

    int __CRTDECL _matherr(_Inout_ struct _exception* _Except);

    _Check_return_ _ACRTIMP double __cdecl modf(_In_ double _X, _Out_ double* _Y);
    _Check_return_ _ACRTIMP double __cdecl nan(_In_ char const* _X);
    _Check_return_ _ACRTIMP double __cdecl nearbyint(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl nextafter(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double __cdecl nexttoward(_In_ double _X, _In_ long double _Y);
    _Check_return_ _ACRTIMP double __cdecl remainder(_In_ double _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double __cdecl remquo(_In_ double _X, _In_ double _Y, _Out_ int* _Z);
    _Check_return_ _ACRTIMP double __cdecl rint(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl round(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl scalbln(_In_ double _X, _In_ long _Y);
    _Check_return_ _ACRTIMP double __cdecl scalbn(_In_ double _X, _In_ int _Y);
    _Check_return_ _ACRTIMP double __cdecl tgamma(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl trunc(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl _j0(_In_ double _X );
    _Check_return_ _ACRTIMP double __cdecl _j1(_In_ double _X );
    _Check_return_ _ACRTIMP double __cdecl _jn(int _X, _In_ double _Y);
    _Check_return_ _ACRTIMP double __cdecl _y0(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl _y1(_In_ double _X);
    _Check_return_ _ACRTIMP double __cdecl _yn(_In_ int _X, _In_ double _Y);

    _Check_return_ _ACRTIMP float     __cdecl acoshf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl asinhf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl atanhf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl cbrtf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl _chgsignf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl copysignf(_In_ float _Number, _In_ float _Sign);
    _Check_return_ _ACRTIMP float     __cdecl _copysignf(_In_ float _Number, _In_ float _Sign);
    _Check_return_ _ACRTIMP float     __cdecl erff(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl erfcf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl expm1f(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl exp2f(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl fdimf(_In_ float _X, _In_ float _Y);
    _Check_return_ _ACRTIMP float     __cdecl fmaf(_In_ float _X, _In_ float _Y, _In_ float _Z);
    _Check_return_ _ACRTIMP float     __cdecl fmaxf(_In_ float _X, _In_ float _Y);
    _Check_return_ _ACRTIMP float     __cdecl fminf(_In_ float _X, _In_ float _Y);
    _Check_return_ _ACRTIMP float     __cdecl _hypotf(_In_ float _X, _In_ float _Y);
    _Check_return_ _ACRTIMP int       __cdecl ilogbf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl lgammaf(_In_ float _X);
    _Check_return_ _ACRTIMP long long __cdecl llrintf(_In_ float _X);
    _Check_return_ _ACRTIMP long long __cdecl llroundf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl log1pf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl log2f(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl logbf(_In_ float _X);
    _Check_return_ _ACRTIMP long      __cdecl lrintf(_In_ float _X);
    _Check_return_ _ACRTIMP long      __cdecl lroundf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl nanf(_In_ char const* _X);
    _Check_return_ _ACRTIMP float     __cdecl nearbyintf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl nextafterf(_In_ float _X, _In_ float _Y);
    _Check_return_ _ACRTIMP float     __cdecl nexttowardf(_In_ float _X, _In_ long double _Y);
    _Check_return_ _ACRTIMP float     __cdecl remainderf(_In_ float _X, _In_ float _Y);
    _Check_return_ _ACRTIMP float     __cdecl remquof(_In_ float _X, _In_ float _Y, _Out_ int* _Z);
    _Check_return_ _ACRTIMP float     __cdecl rintf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl roundf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl scalblnf(_In_ float _X, _In_ long _Y);
    _Check_return_ _ACRTIMP float     __cdecl scalbnf(_In_ float _X, _In_ int _Y);
    _Check_return_ _ACRTIMP float     __cdecl tgammaf(_In_ float _X);
    _Check_return_ _ACRTIMP float     __cdecl truncf(_In_ float _X);

    #if defined _M_IX86

        _Check_return_ _ACRTIMP int  __cdecl _set_SSE2_enable(_In_ int _Flag);

    #endif

    #if defined _M_X64

        _Check_return_ _ACRTIMP float __cdecl _logbf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl _nextafterf(_In_ float _X, _In_ float _Y);
        _Check_return_ _ACRTIMP int   __cdecl _finitef(_In_ float _X);
        _Check_return_ _ACRTIMP int   __cdecl _isnanf(_In_ float _X);
        _Check_return_ _ACRTIMP int   __cdecl _fpclassf(_In_ float _X);

        _Check_return_ _ACRTIMP int   __cdecl _set_FMA3_enable(_In_ int _Flag);
        _Check_return_ _ACRTIMP int   __cdecl _get_FMA3_enable(void);

    #elif defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID_X86_ARM64

        _Check_return_ _ACRTIMP int   __cdecl _finitef(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl _logbf(_In_ float _X);

    #endif



    #if defined _M_X64 || defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID_X86_ARM64 || defined _CORECRT_BUILD_APISET || defined _M_ARM64EC

        _Check_return_ _ACRTIMP float __cdecl acosf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl asinf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl atan2f(_In_ float _Y, _In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl atanf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl ceilf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl cosf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl coshf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl expf(_In_ float _X);

    #else

        _Check_return_ __inline float __CRTDECL acosf(_In_ float _X)
        {
            return (float)acos(_X);
        }

        _Check_return_ __inline float __CRTDECL asinf(_In_ float _X)
        {
            return (float)asin(_X);
        }

        _Check_return_ __inline float __CRTDECL atan2f(_In_ float _Y, _In_ float _X)
        {
            return (float)atan2(_Y, _X);
        }

        _Check_return_ __inline float __CRTDECL atanf(_In_ float _X)
        {
            return (float)atan(_X);
        }

        _Check_return_ __inline float __CRTDECL ceilf(_In_ float _X)
        {
            return (float)ceil(_X);
        }

        _Check_return_ __inline float __CRTDECL cosf(_In_ float _X)
        {
            return (float)cos(_X);
        }

        _Check_return_ __inline float __CRTDECL coshf(_In_ float _X)
        {
            return (float)cosh(_X);
        }

        _Check_return_ __inline float __CRTDECL expf(_In_ float _X)
        {
            return (float)exp(_X);
        }

    #endif

    #if defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID_X86_ARM64 || defined _M_ARM64EC

        _Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP float __cdecl fabsf(_In_ float  _X);

    #else

        _Check_return_ __inline float __CRTDECL fabsf(_In_ float _X)
        {
            return (float)fabs(_X);
        }

    #endif

    #if defined _M_X64 || defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID_X86_ARM64 || defined _M_ARM64EC

        _Check_return_ _ACRTIMP float __cdecl floorf(_In_ float _X);
        _Check_return_ _ACRTIMP float __cdecl fmodf(_In_ float _X, _In_ float _Y);

    #else

        _Check_return_ __inline float __CRTDECL floorf(_In_ float _X)
        {
            return (float)floor(_X);
        }

        _Check_return_ __inline float __CRTDECL fmodf(_In_ float _X, _In_ float _Y)
        {
            return (float)fmod(_X, _Y);
        }

    #endif

    _Check_return_ __inline float __CRTDECL frexpf(_In_ float _X, _Out_ int *_Y)
    {
        return (float)frexp(_X, _Y);
    }

    _Check_return_ __inline float __CRTDECL hypotf(_In_ float _X, _In_ float _Y)
    {
        return _hypotf(_X, _Y);
    }

    _Check_return_ __inline float __CRTDECL ldexpf(_In_ float _X, _In_ int _Y)
    {
        return (float)ldexp(_X, _Y);
    }

    #if defined _M_X64 || defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID_X86_ARM64 || defined _CORECRT_BUILD_APISET || defined _M_ARM64EC

        _Check_return_ _ACRTIMP float  __cdecl log10f(_In_ float _X);
        _Check_return_ _ACRTIMP float  __cdecl logf(_In_ float _X);
        _Check_return_ _ACRTIMP float  __cdecl modff(_In_ float _X, _Out_ float *_Y);
        _Check_return_ _ACRTIMP float  __cdecl powf(_In_ float _X, _In_ float _Y);
        _Check_return_ _ACRTIMP float  __cdecl sinf(_In_ float _X);
        _Check_return_ _ACRTIMP float  __cdecl sinhf(_In_ float _X);
        _Check_return_ _ACRTIMP float  __cdecl sqrtf(_In_ float _X);
        _Check_return_ _ACRTIMP float  __cdecl tanf(_In_ float _X);
        _Check_return_ _ACRTIMP float  __cdecl tanhf(_In_ float _X);

    #else

        _Check_return_ __inline float __CRTDECL log10f(_In_ float _X)
        {
            return (float)log10(_X);
        }

        _Check_return_ __inline float __CRTDECL logf(_In_ float _X)
        {
            return (float)log(_X);
        }

        _Check_return_ __inline float __CRTDECL modff(_In_ float _X, _Out_ float* _Y)
        {
            double _F, _I;
            _F = modf(_X, &_I);
            *_Y = (float)_I;
            return (float)_F;
        }

        _Check_return_ __inline float __CRTDECL powf(_In_ float _X, _In_ float _Y)
        {
            return (float)pow(_X, _Y);
        }

        _Check_return_ __inline float __CRTDECL sinf(_In_ float _X)
        {
            return (float)sin(_X);
        }

        _Check_return_ __inline float __CRTDECL sinhf(_In_ float _X)
        {
            return (float)sinh(_X);
        }

        _Check_return_ __inline float __CRTDECL sqrtf(_In_ float _X)
        {
            return (float)sqrt(_X);
        }

        _Check_return_ __inline float __CRTDECL tanf(_In_ float _X)
        {
            return (float)tan(_X);
        }

        _Check_return_ __inline float __CRTDECL tanhf(_In_ float _X)
        {
            return (float)tanh(_X);
        }

    #endif

    _Check_return_ _ACRTIMP long double __cdecl acoshl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL acosl(_In_ long double _X)
    {
        return acos((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl asinhl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL asinl(_In_ long double _X)
    {
        return asin((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL atan2l(_In_ long double _Y, _In_ long double _X)
    {
        return atan2((double)_Y, (double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl atanhl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL atanl(_In_ long double _X)
    {
        return atan((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl cbrtl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL ceill(_In_ long double _X)
    {
        return ceil((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL _chgsignl(_In_ long double _X)
    {
        return _chgsign((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl copysignl(_In_ long double _Number, _In_ long double _Sign);

    _Check_return_ __inline long double __CRTDECL _copysignl(_In_ long double _Number, _In_ long double _Sign)
    {
        return _copysign((double)_Number, (double)_Sign);
    }

    _Check_return_ __inline long double __CRTDECL coshl(_In_ long double _X)
    {
        return cosh((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL cosl(_In_ long double _X)
    {
        return cos((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl erfl(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl erfcl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL expl(_In_ long double _X)
    {
        return exp((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl exp2l(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl expm1l(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL fabsl(_In_ long double _X)
    {
        return fabs((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl fdiml(_In_ long double _X, _In_ long double _Y);

    _Check_return_ __inline long double __CRTDECL floorl(_In_ long double _X)
    {
        return floor((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl fmal(_In_ long double _X, _In_ long double _Y, _In_ long double _Z);
    _Check_return_ _ACRTIMP long double __cdecl fmaxl(_In_ long double _X, _In_ long double _Y);
    _Check_return_ _ACRTIMP long double __cdecl fminl(_In_ long double _X, _In_ long double _Y);

    _Check_return_ __inline long double __CRTDECL fmodl(_In_ long double _X, _In_ long double _Y)
    {
        return fmod((double)_X, (double)_Y);
    }

    _Check_return_ __inline long double __CRTDECL frexpl(_In_ long double _X, _Out_ int *_Y)
    {
        return frexp((double)_X, _Y);
    }

    _Check_return_ _ACRTIMP int __cdecl ilogbl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL _hypotl(_In_ long double _X, _In_ long double _Y)
    {
        return _hypot((double)_X, (double)_Y);
    }

    _Check_return_ __inline long double __CRTDECL hypotl(_In_ long double _X, _In_ long double _Y)
    {
        return _hypot((double)_X, (double)_Y);
    }

    _Check_return_ __inline long double __CRTDECL ldexpl(_In_ long double _X, _In_ int _Y)
    {
        return ldexp((double)_X, _Y);
    }

    _Check_return_ _ACRTIMP long double __cdecl lgammal(_In_ long double _X);
    _Check_return_ _ACRTIMP long long __cdecl llrintl(_In_ long double _X);
    _Check_return_ _ACRTIMP long long __cdecl llroundl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL logl(_In_ long double _X)
    {
        return log((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL log10l(_In_ long double _X)
    {
        return log10((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl log1pl(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl log2l(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl logbl(_In_ long double _X);
    _Check_return_ _ACRTIMP long __cdecl lrintl(_In_ long double _X);
    _Check_return_ _ACRTIMP long __cdecl lroundl(_In_ long double _X);

    _Check_return_ __inline long double __CRTDECL modfl(_In_ long double _X, _Out_ long double* _Y)
    {
        double _F, _I;
        _F = modf((double)_X, &_I);
        *_Y = _I;
        return _F;
    }

    _Check_return_ _ACRTIMP long double __cdecl nanl(_In_ char const* _X);
    _Check_return_ _ACRTIMP long double __cdecl nearbyintl(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl nextafterl(_In_ long double _X, _In_ long double _Y);
    _Check_return_ _ACRTIMP long double __cdecl nexttowardl(_In_ long double _X, _In_ long double _Y);

    _Check_return_ __inline long double __CRTDECL powl(_In_ long double _X, _In_ long double _Y)
    {
        return pow((double)_X, (double)_Y);
    }

    _Check_return_ _ACRTIMP long double __cdecl remainderl(_In_ long double _X, _In_ long double _Y);
    _Check_return_ _ACRTIMP long double __cdecl remquol(_In_ long double _X, _In_ long double _Y, _Out_ int* _Z);
    _Check_return_ _ACRTIMP long double __cdecl rintl(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl roundl(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl scalblnl(_In_ long double _X, _In_ long _Y);
    _Check_return_ _ACRTIMP long double __cdecl scalbnl(_In_ long double _X, _In_ int _Y);

    _Check_return_ __inline long double __CRTDECL sinhl(_In_ long double _X)
    {
        return sinh((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL sinl(_In_ long double _X)
    {
        return sin((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL sqrtl(_In_ long double _X)
    {
        return sqrt((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL tanhl(_In_ long double _X)
    {
        return tanh((double)_X);
    }

    _Check_return_ __inline long double __CRTDECL tanl(_In_ long double _X)
    {
        return tan((double)_X);
    }

    _Check_return_ _ACRTIMP long double __cdecl tgammal(_In_ long double _X);
    _Check_return_ _ACRTIMP long double __cdecl truncl(_In_ long double _X);

    #ifndef __cplusplus
        #define _matherrl _matherr
    #endif

#endif // _CRT_FUNCTIONS_REQUIRED

#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

    #define DOMAIN      _DOMAIN
    #define SING        _SING
    #define OVERFLOW    _OVERFLOW
    #define UNDERFLOW   _UNDERFLOW
    #define TLOSS       _TLOSS
    #define PLOSS       _PLOSS

    #define matherr     _matherr

    #ifndef __assembler
        #ifndef _M_CEE_PURE
            extern double HUGE;
        #else
            double const HUGE = _HUGE;
        #endif

        _CRT_NONSTDC_DEPRECATE(_j0) _Check_return_ _ACRTIMP double __cdecl j0(_In_ double _X);
        _CRT_NONSTDC_DEPRECATE(_j1) _Check_return_ _ACRTIMP double __cdecl j1(_In_ double _X);
        _CRT_NONSTDC_DEPRECATE(_jn) _Check_return_ _ACRTIMP double __cdecl jn(_In_ int _X, _In_ double _Y);
        _CRT_NONSTDC_DEPRECATE(_y0) _Check_return_ _ACRTIMP double __cdecl y0(_In_ double _X);
        _CRT_NONSTDC_DEPRECATE(_y1) _Check_return_ _ACRTIMP double __cdecl y1(_In_ double _X);
        _CRT_NONSTDC_DEPRECATE(_yn) _Check_return_ _ACRTIMP double __cdecl yn(_In_ int _X, _In_ double _Y);
    #endif // !__assembler

#endif // _CRT_INTERNAL_NONSTDC_NAMES

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif /* _INC_MATH */
