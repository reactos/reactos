/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */
#include "stlport_prefix.h"


// Trigonometric and hyperbolic functions for complex<float>,
// complex<double>, and complex<long double>
#include <complex>
#include <cfloat>
#include <cmath>

_STLP_BEGIN_NAMESPACE


//----------------------------------------------------------------------
// helpers
#if defined (__sgi)
  static const union { unsigned int i; float f; } float_ulimit = { 0x42b2d4fc };
  static const float float_limit = float_ulimit.f;
  static union {
    struct { unsigned int h; unsigned int l; } w;
    double d;
  } double_ulimit = { 0x408633ce, 0x8fb9f87d };
  static const double double_limit = double_ulimit.d;
  static union {
    struct { unsigned int h[2]; unsigned int l[2]; } w;
    long double ld;
  } ldouble_ulimit = {0x408633ce, 0x8fb9f87e, 0xbd23b659, 0x4e9bd8b1};
#  if !defined (_STLP_NO_LONG_DOUBLE)
  static const long double ldouble_limit = ldouble_ulimit.ld;
#  endif
#else
#  if defined (M_LN2) && defined (FLT_MAX_EXP)
  static const float float_limit = float(M_LN2 * FLT_MAX_EXP);
  static const double double_limit = M_LN2 * DBL_MAX_EXP;
#  else
  static const float float_limit = ::log(FLT_MAX);
  static const double double_limit = ::log(DBL_MAX);
#  endif
#  if !defined (_STLP_NO_LONG_DOUBLE)
#    if defined (M_LN2l)
  static const long double ldouble_limit = M_LN2l * LDBL_MAX_EXP;
#    else
  static const long double ldouble_limit = ::log(LDBL_MAX);
#    endif
#  endif
#endif


//----------------------------------------------------------------------
// sin
template <class _Tp>
static complex<_Tp> sinT(const complex<_Tp>& z) {
  return complex<_Tp>(::sin(z._M_re) * ::cosh(z._M_im),
                      ::cos(z._M_re) * ::sinh(z._M_im));
}

_STLP_DECLSPEC complex<float> _STLP_CALL sin(const complex<float>& z)
{ return sinT(z); }

_STLP_DECLSPEC complex<double> _STLP_CALL sin(const complex<double>& z)
{ return sinT(z); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL sin(const complex<long double>& z)
{ return sinT(z); }
#endif

//----------------------------------------------------------------------
// cos
template <class _Tp>
static complex<_Tp> cosT(const complex<_Tp>& z) {
  return complex<_Tp>(::cos(z._M_re) * ::cosh(z._M_im),
                     -::sin(z._M_re) * ::sinh(z._M_im));
}

_STLP_DECLSPEC complex<float> _STLP_CALL cos(const complex<float>& z)
{ return cosT(z); }

_STLP_DECLSPEC complex<double> _STLP_CALL cos(const complex<double>& z)
{ return cosT(z); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL cos(const complex<long double>& z)
{ return cosT(z); }
#endif

//----------------------------------------------------------------------
// tan
template <class _Tp>
static complex<_Tp> tanT(const complex<_Tp>& z, const _Tp& Tp_limit) {
  _Tp re2 = 2.f * z._M_re;
  _Tp im2 = 2.f * z._M_im;

  if (::abs(im2) > Tp_limit)
    return complex<_Tp>(0.f, (im2 > 0 ? 1.f : -1.f));
  else {
    _Tp den = ::cos(re2) + ::cosh(im2);
    return complex<_Tp>(::sin(re2) / den, ::sinh(im2) / den);
  }
}

_STLP_DECLSPEC complex<float> _STLP_CALL tan(const complex<float>& z)
{ return tanT(z, float_limit); }

_STLP_DECLSPEC complex<double> _STLP_CALL tan(const complex<double>& z)
{ return tanT(z, double_limit); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL tan(const complex<long double>& z)
{ return tanT(z, ldouble_limit); }
#endif

//----------------------------------------------------------------------
// sinh
template <class _Tp>
static complex<_Tp> sinhT(const complex<_Tp>& z) {
  return complex<_Tp>(::sinh(z._M_re) * ::cos(z._M_im),
                      ::cosh(z._M_re) * ::sin(z._M_im));
}

_STLP_DECLSPEC complex<float> _STLP_CALL sinh(const complex<float>& z)
{ return sinhT(z); }

_STLP_DECLSPEC complex<double> _STLP_CALL sinh(const complex<double>& z)
{ return sinhT(z); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL sinh(const complex<long double>& z)
{ return sinhT(z); }
#endif

//----------------------------------------------------------------------
// cosh
template <class _Tp>
static complex<_Tp> coshT(const complex<_Tp>& z) {
  return complex<_Tp>(::cosh(z._M_re) * ::cos(z._M_im),
                      ::sinh(z._M_re) * ::sin(z._M_im));
}

_STLP_DECLSPEC complex<float> _STLP_CALL cosh(const complex<float>& z)
{ return coshT(z); }

_STLP_DECLSPEC complex<double> _STLP_CALL cosh(const complex<double>& z)
{ return coshT(z); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL cosh(const complex<long double>& z)
{ return coshT(z); }
#endif

//----------------------------------------------------------------------
// tanh
template <class _Tp>
static complex<_Tp> tanhT(const complex<_Tp>& z, const _Tp& Tp_limit) {
  _Tp re2 = 2.f * z._M_re;
  _Tp im2 = 2.f * z._M_im;
  if (::abs(re2) > Tp_limit)
    return complex<_Tp>((re2 > 0 ? 1.f : -1.f), 0.f);
  else {
    _Tp den = ::cosh(re2) + ::cos(im2);
    return complex<_Tp>(::sinh(re2) / den, ::sin(im2) / den);
  }
}

_STLP_DECLSPEC complex<float> _STLP_CALL tanh(const complex<float>& z)
{ return tanhT(z, float_limit); }

_STLP_DECLSPEC complex<double> _STLP_CALL tanh(const complex<double>& z)
{ return tanhT(z, double_limit); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL tanh(const complex<long double>& z)
{ return tanhT(z, ldouble_limit); }
#endif

_STLP_END_NAMESPACE
