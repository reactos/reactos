//
// complex.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The complex math library.
//
#pragma once
#ifndef _COMPLEX
#define _COMPLEX

#include <corecrt.h>

#if (_CRT_HAS_CXX17 == 1) && !defined(_CRT_USE_C_COMPLEX_H)
#include <ccomplex>
#else // ^^^^ /std:c++17 ^^^^ // vvvv _CRT_USE_C_COMPLEX_H vvvv

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _C_COMPLEX_T
    #define _C_COMPLEX_T
    typedef struct _C_double_complex
    {
        double _Val[2];
    } _C_double_complex;

    typedef struct _C_float_complex
    {
        float _Val[2];
    } _C_float_complex;

    typedef struct _C_ldouble_complex
    {
        long double _Val[2];
    } _C_ldouble_complex;
#endif

typedef _C_double_complex  _Dcomplex;
typedef _C_float_complex   _Fcomplex;
typedef _C_ldouble_complex _Lcomplex;



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define _DCOMPLEX_(re, im)  _Cbuild(re, im)
#define _FCOMPLEX_(re, im)  _FCbuild(re, im)
#define _LCOMPLEX_(re, im)  _LCbuild(re, im)

#define _Complex_I _FCbuild(0.0F, 1.0F)
#define I          _Complex_I



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_ACRTIMP double    __cdecl cabs(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl cacos(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl cacosh(_In_ _Dcomplex _Z);
_ACRTIMP double    __cdecl carg(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl casin(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl casinh(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl catan(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl catanh(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl ccos(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl ccosh(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl cexp(_In_ _Dcomplex _Z);
_ACRTIMP double    __cdecl cimag(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl clog(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl clog10(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl conj(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl cpow(_In_ _Dcomplex _X, _In_ _Dcomplex _Y);
_ACRTIMP _Dcomplex __cdecl cproj(_In_ _Dcomplex _Z);
_ACRTIMP double    __cdecl creal(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl csin(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl csinh(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl csqrt(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl ctan(_In_ _Dcomplex _Z);
_ACRTIMP _Dcomplex __cdecl ctanh(_In_ _Dcomplex _Z);
_ACRTIMP double    __cdecl norm(_In_ _Dcomplex _Z);

_ACRTIMP float     __cdecl cabsf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl cacosf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl cacoshf(_In_ _Fcomplex _Z);
_ACRTIMP float     __cdecl cargf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl casinf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl casinhf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl catanf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl catanhf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl ccosf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl ccoshf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl cexpf(_In_ _Fcomplex _Z);
_ACRTIMP float     __cdecl cimagf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl clogf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl clog10f(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl conjf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl cpowf(_In_ _Fcomplex _X, _In_ _Fcomplex _Y);
_ACRTIMP _Fcomplex __cdecl cprojf(_In_ _Fcomplex _Z);
_ACRTIMP float     __cdecl crealf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl csinf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl csinhf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl csqrtf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl ctanf(_In_ _Fcomplex _Z);
_ACRTIMP _Fcomplex __cdecl ctanhf(_In_ _Fcomplex _Z);
_ACRTIMP float     __cdecl normf(_In_ _Fcomplex _Z);

_ACRTIMP long double __cdecl cabsl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl cacosl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl cacoshl(_In_ _Lcomplex _Z);
_ACRTIMP long double __cdecl cargl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl casinl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl casinhl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl catanl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl catanhl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl ccosl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl ccoshl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl cexpl(_In_ _Lcomplex _Z);
_ACRTIMP long double __cdecl cimagl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl clogl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl clog10l(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl conjl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl cpowl(_In_ _Lcomplex _X, _In_ _Lcomplex _Y);
_ACRTIMP _Lcomplex   __cdecl cprojl(_In_ _Lcomplex _Z);
_ACRTIMP long double __cdecl creall(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl csinl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl csinhl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl csqrtl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl ctanl(_In_ _Lcomplex _Z);
_ACRTIMP _Lcomplex   __cdecl ctanhl(_In_ _Lcomplex _Z);
_ACRTIMP long double __cdecl norml(_In_ _Lcomplex _Z);

_ACRTIMP _Dcomplex __cdecl _Cbuild(_In_ double _Re, _In_ double _Im);
_ACRTIMP _Dcomplex __cdecl _Cmulcc(_In_ _Dcomplex _X, _In_ _Dcomplex _Y);
_ACRTIMP _Dcomplex __cdecl _Cmulcr(_In_ _Dcomplex _X, _In_ double _Y);

_ACRTIMP _Fcomplex __cdecl _FCbuild(_In_ float _Re, _In_ float _Im);
_ACRTIMP _Fcomplex __cdecl _FCmulcc(_In_ _Fcomplex _X, _In_ _Fcomplex _Y);
_ACRTIMP _Fcomplex __cdecl _FCmulcr(_In_ _Fcomplex _X, _In_ float _Y);

_ACRTIMP _Lcomplex __cdecl _LCbuild(_In_ long double _Re, _In_ long double _Im);
_ACRTIMP _Lcomplex __cdecl _LCmulcc(_In_ _Lcomplex _X, _In_ _Lcomplex _Y);
_ACRTIMP _Lcomplex __cdecl _LCmulcr(_In_ _Lcomplex _X, _In_ long double _Y);



#ifdef __cplusplus
extern "C++"
{
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // double complex overloads
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    inline _Dcomplex __CRTDECL acos(_In_ _Dcomplex _X) throw()
    {
        return cacos(_X);
    }

    inline _Dcomplex __CRTDECL acosh(_In_ _Dcomplex _X) throw()
    {
        return cacosh(_X);
    }

    inline _Dcomplex __CRTDECL asin(_In_ _Dcomplex _X) throw()
    {
        return casin(_X);
    }

    inline _Dcomplex __CRTDECL asinh(_In_ _Dcomplex _X) throw()
    {
        return casinh(_X);
    }

    inline _Dcomplex __CRTDECL atan(_In_ _Dcomplex _X) throw()
    {
        return catan(_X);
    }

    inline _Dcomplex __CRTDECL atanh(_In_ _Dcomplex _X) throw()
    {
        return catanh(_X);
    }

    inline _Dcomplex __CRTDECL cos(_In_ _Dcomplex _X) throw()
    {
        return ccos(_X);
    }

    inline _Dcomplex __CRTDECL cosh(_In_ _Dcomplex _X) throw()
    {
        return ccosh(_X);
    }

    inline _Dcomplex __CRTDECL proj(_In_ _Dcomplex _X) throw()
    {
        return cproj(_X);
    }

    inline _Dcomplex __CRTDECL exp(_In_ _Dcomplex _X) throw()
    {
        return cexp(_X);
    }

    inline _Dcomplex __CRTDECL log(_In_ _Dcomplex _X) throw()
    {
        return clog(_X);
    }

    inline _Dcomplex __CRTDECL log10(_In_ _Dcomplex _X) throw()
    {
        return clog10(_X);
    }

    inline _Dcomplex __CRTDECL pow(_In_ _Dcomplex _X, _In_ _Dcomplex _Y) throw()
    {
        return cpow(_X, _Y);
    }

    inline _Dcomplex __CRTDECL sin(_In_ _Dcomplex _X) throw()
    {
        return csin(_X);
    }

    inline _Dcomplex __CRTDECL sinh(_In_ _Dcomplex _X) throw()
    {
        return csinh(_X);
    }

    inline _Dcomplex __CRTDECL sqrt(_In_ _Dcomplex _X) throw()
    {
        return csqrt(_X);
    }

    inline _Dcomplex __CRTDECL tan(_In_ _Dcomplex _X) throw()
    {
        return ctan(_X);
    }

    inline _Dcomplex __CRTDECL tanh(_In_ _Dcomplex _X) throw()
    {
        return ctanh(_X);
    }

    inline double __CRTDECL abs(_In_ _Dcomplex _X) throw()
    {
        return cabs(_X);
    }

    inline double __CRTDECL arg(_In_ _Dcomplex _X) throw()
    {
        return carg(_X);
    }

    inline double __CRTDECL imag(_In_ _Dcomplex _X) throw()
    {
        return cimag(_X);
    }

    inline double __CRTDECL real(_In_ _Dcomplex _X) throw()
    {
        return creal(_X);
    }



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // float complex overloads
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    inline _Fcomplex __CRTDECL acos(_In_ _Fcomplex _X) throw()
    {
        return cacosf(_X);
    }

    inline _Fcomplex __CRTDECL acosh(_In_ _Fcomplex _X) throw()
    {
        return cacoshf(_X);
    }

    inline _Fcomplex __CRTDECL asin(_In_ _Fcomplex _X) throw()
    {
        return casinf(_X);
    }

    inline _Fcomplex __CRTDECL asinh(_In_ _Fcomplex _X) throw()
    {
        return casinhf(_X);
    }

    inline _Fcomplex __CRTDECL atan(_In_ _Fcomplex _X) throw()
    {
        return catanf(_X);
    }

    inline _Fcomplex __CRTDECL atanh(_In_ _Fcomplex _X) throw()
    {
        return catanhf(_X);
    }

    inline _Fcomplex __CRTDECL conj(_In_ _Fcomplex _X) throw()
    {
        return conjf(_X);
    }

    inline _Fcomplex __CRTDECL cos(_In_ _Fcomplex _X) throw()
    {
        return ccosf(_X);
    }

    inline _Fcomplex __CRTDECL cosh(_In_ _Fcomplex _X) throw()
    {
        return ccoshf(_X);
    }

    inline _Fcomplex __CRTDECL cproj(_In_ _Fcomplex _X) throw()
    {
        return cprojf(_X);
    }

    inline _Fcomplex __CRTDECL proj(_In_ _Fcomplex _X) throw()
    {
        return cprojf(_X);
    }

    inline _Fcomplex __CRTDECL exp(_In_ _Fcomplex _X) throw()
    {
        return cexpf(_X);
    }

    inline _Fcomplex __CRTDECL log(_In_ _Fcomplex _X) throw()
    {
        return clogf(_X);
    }

    inline _Fcomplex __CRTDECL log10(_In_ _Fcomplex _X) throw()
    {
        return clog10f(_X);
    }

    inline float __CRTDECL norm(_In_ _Fcomplex _X) throw()
    {
        return normf(_X);
    }

    inline _Fcomplex __CRTDECL pow(_In_ _Fcomplex _X, _In_ _Fcomplex _Y) throw()
    {
        return cpowf(_X, _Y);
    }

    inline _Fcomplex __CRTDECL sin(_In_ _Fcomplex _X) throw()
    {
        return csinf(_X);
    }

    inline _Fcomplex __CRTDECL sinh(_In_ _Fcomplex _X) throw()
    {
        return csinhf(_X);
    }

    inline _Fcomplex __CRTDECL sqrt(_In_ _Fcomplex _X) throw()
    {
        return csqrtf(_X);
    }

    inline _Fcomplex __CRTDECL tan(_In_ _Fcomplex _X) throw()
    {
        return ctanf(_X);
    }

    inline _Fcomplex __CRTDECL tanh(_In_ _Fcomplex _X) throw()
    {
        return ctanhf(_X);
    }

    inline float __CRTDECL abs(_In_ _Fcomplex _X) throw()
    {
        return cabsf(_X);
    }

    inline float __CRTDECL arg(_In_ _Fcomplex _X) throw()
    {
        return cargf(_X);
    }

    inline float __CRTDECL carg(_In_ _Fcomplex _X) throw()
    {
        return cargf(_X);
    }

    inline float __CRTDECL cimag(_In_ _Fcomplex _X) throw()
    {
        return cimagf(_X);
    }

    inline float __CRTDECL creal(_In_ _Fcomplex _X) throw()
    {
        return crealf(_X);
    }

    inline float __CRTDECL imag(_In_ _Fcomplex _X) throw()
    {
        return cimagf(_X);
    }

    inline float __CRTDECL real(_In_ _Fcomplex _X) throw()
    {
        return crealf(_X);
    }



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // long double complex overloads
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    inline _Lcomplex __CRTDECL acos(_In_ _Lcomplex _X) throw()
    {
        return cacosl(_X);
    }

    inline _Lcomplex __CRTDECL acosh(_In_ _Lcomplex _X) throw()
    {
        return cacoshl(_X);
    }

    inline _Lcomplex __CRTDECL asin(_In_ _Lcomplex _X) throw()
    {
        return casinl(_X);
    }

    inline _Lcomplex __CRTDECL asinh(_In_ _Lcomplex _X) throw()
    {
        return casinhl(_X);
    }

    inline _Lcomplex __CRTDECL atan(_In_ _Lcomplex _X) throw()
    {
        return catanl(_X);
    }

    inline _Lcomplex __CRTDECL atanh(_In_ _Lcomplex _X) throw()
    {
        return catanhl(_X);
    }

    inline _Lcomplex __CRTDECL conj(_In_ _Lcomplex _X) throw()
    {
        return conjl(_X);
    }

    inline _Lcomplex __CRTDECL cos(_In_ _Lcomplex _X) throw()
    {
        return ccosl(_X);
    }

    inline _Lcomplex __CRTDECL cosh(_In_ _Lcomplex _X) throw()
    {
        return ccoshl(_X);
    }

    inline _Lcomplex __CRTDECL cproj(_In_ _Lcomplex _X) throw()
    {
        return cprojl(_X);
    }

    inline _Lcomplex __CRTDECL proj(_In_ _Lcomplex _X) throw()
    {
        return cprojl(_X);
    }

    inline _Lcomplex __CRTDECL exp(_In_ _Lcomplex _X) throw()
    {
        return cexpl(_X);
    }

    inline _Lcomplex __CRTDECL log(_In_ _Lcomplex _X) throw()
    {
        return clogl(_X);
    }

    inline _Lcomplex __CRTDECL log10(_In_ _Lcomplex _X) throw()
    {
        return clog10l(_X);
    }

    inline long double __CRTDECL norm(_In_ _Lcomplex _X) throw()
    {
        return norml(_X);
    }

    inline _Lcomplex __CRTDECL pow(_In_ _Lcomplex _X, _In_ _Lcomplex _Y) throw()
    {
        return cpowl(_X, _Y);
    }

    inline _Lcomplex __CRTDECL sin(_In_ _Lcomplex _X) throw()
    {
        return csinl(_X);
    }

    inline _Lcomplex __CRTDECL sinh(_In_ _Lcomplex _X) throw()
    {
        return csinhl(_X);
    }

    inline _Lcomplex __CRTDECL sqrt(_In_ _Lcomplex _X) throw()
    {
        return csqrtl(_X);
    }

    inline _Lcomplex __CRTDECL tan(_In_ _Lcomplex _X) throw()
    {
        return ctanl(_X);
    }

    inline _Lcomplex __CRTDECL tanh(_In_ _Lcomplex _X) throw()
    {
        return ctanhl(_X);
    }

    inline long double __CRTDECL abs(_In_ _Lcomplex _X) throw()
    {
        return cabsl(_X);
    }

    inline long double __CRTDECL arg(_In_ _Lcomplex _X) throw()
    {
        return cargl(_X);
    }

    inline long double __CRTDECL carg(_In_ _Lcomplex _X) throw()
    {
        return cargl(_X);
    }

    inline long double __CRTDECL cimag(_In_ _Lcomplex _X) throw()
    {
        return cimagl(_X);
    }

    inline long double __CRTDECL creal(_In_ _Lcomplex _X) throw()
    {
        return creall(_X);
    }

    inline long double __CRTDECL imag(_In_ _Lcomplex _X) throw()
    {
        return cimagl(_X);
    }

    inline long double __CRTDECL real(_In_ _Lcomplex _X) throw()
    {
        return creall(_X);
    }

} // extern "C++"
#endif // __cplusplus

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // (_CRT_HAS_CXX17 == 1) && !defined(_CRT_USE_C_COMPLEX_H)
#endif // _COMPLEX
