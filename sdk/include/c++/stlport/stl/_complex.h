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
#ifndef _STLP_INTERNAL_COMPLEX
#define _STLP_INTERNAL_COMPLEX

// This header declares the template class complex, as described in
// in the draft C++ standard.  Single-precision complex numbers
// are complex<float>, double-precision are complex<double>, and
// quad precision are complex<long double>.

// Note that the template class complex is declared within namespace
// std, as called for by the draft C++ standard.

#ifndef _STLP_INTERNAL_CMATH
#  include <stl/_cmath.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _Tp>
struct complex {
  typedef _Tp value_type;
  typedef complex<_Tp> _Self;

  // Constructors, destructor, assignment operator.
  complex() : _M_re(0), _M_im(0) {}
  complex(const value_type& __x)
    : _M_re(__x), _M_im(0) {}
  complex(const value_type& __x, const value_type& __y)
    : _M_re(__x), _M_im(__y) {}
  complex(const _Self& __z)
    : _M_re(__z._M_re), _M_im(__z._M_im) {}

  _Self& operator=(const _Self& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

#if defined (_STLP_MEMBER_TEMPLATES) && defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  template <class _Tp2>
  explicit complex(const complex<_Tp2>& __z)
    : _M_re(__z._M_re), _M_im(__z._M_im) {}

  template <class _Tp2>
  _Self& operator=(const complex<_Tp2>& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }
#endif /* _STLP_MEMBER_TEMPLATES */

  // Element access.
  value_type real() const { return _M_re; }
  value_type imag() const { return _M_im; }

  // Arithmetic op= operations involving one real argument.

  _Self& operator= (const value_type& __x) {
    _M_re = __x;
    _M_im = 0;
    return *this;
  }
  _Self& operator+= (const value_type& __x) {
    _M_re += __x;
    return *this;
  }
  _Self& operator-= (const value_type& __x) {
    _M_re -= __x;
    return *this;
  }
  _Self& operator*= (const value_type& __x) {
    _M_re *= __x;
    _M_im *= __x;
    return *this;
  }
  _Self& operator/= (const value_type& __x) {
    _M_re /= __x;
    _M_im /= __x;
    return *this;
  }

  // Arithmetic op= operations involving two complex arguments.

  static void  _STLP_CALL _div(const value_type& __z1_r, const value_type& __z1_i,
                               const value_type& __z2_r, const value_type& __z2_i,
                               value_type& __res_r, value_type& __res_i);

  static void _STLP_CALL _div(const value_type& __z1_r,
                              const value_type& __z2_r, const value_type& __z2_i,
                              value_type& __res_r, value_type& __res_i);

#if defined (_STLP_MEMBER_TEMPLATES) // && defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)

  template <class _Tp2> _Self& operator+= (const complex<_Tp2>& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  template <class _Tp2> _Self& operator-= (const complex<_Tp2>& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  template <class _Tp2> _Self& operator*= (const complex<_Tp2>& __z) {
    value_type __r = _M_re * __z._M_re - _M_im * __z._M_im;
    value_type __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  template <class _Tp2> _Self& operator/= (const complex<_Tp2>& __z) {
    value_type __r;
    value_type __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }
#endif /* _STLP_MEMBER_TEMPLATES */

  _Self& operator+= (const _Self& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  _Self& operator-= (const _Self& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  _Self& operator*= (const _Self& __z) {
    value_type __r = _M_re * __z._M_re - _M_im * __z._M_im;
    value_type __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  _Self& operator/= (const _Self& __z) {
    value_type __r;
    value_type __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  // Data members.
  value_type _M_re;
  value_type _M_im;
};

// Explicit specializations for float, double, long double.  The only
// reason for these specializations is to enable automatic conversions
// from complex<float> to complex<double>, and complex<double> to
// complex<long double>.

_STLP_TEMPLATE_NULL
struct _STLP_CLASS_DECLSPEC complex<float> {
  typedef float value_type;
  typedef complex<float> _Self;
  // Constructors, destructor, assignment operator.

  complex(value_type __x = 0.0f, value_type __y = 0.0f)
    : _M_re(__x), _M_im(__y) {}

  complex(const complex<float>& __z)    : _M_re(__z._M_re), _M_im(__z._M_im) {}

  inline explicit complex(const complex<double>& __z);
#ifndef _STLP_NO_LONG_DOUBLE
  inline explicit complex(const complex<long double>& __z);
#endif
  // Element access.
  value_type real() const { return _M_re; }
  value_type imag() const { return _M_im; }

  // Arithmetic op= operations involving one real argument.

  _Self& operator= (value_type __x) {
    _M_re = __x;
    _M_im = 0.0f;
    return *this;
  }
  _Self& operator+= (value_type __x) {
    _M_re += __x;
    return *this;
  }
  _Self& operator-= (value_type __x) {
    _M_re -= __x;
    return *this;
  }
  _Self& operator*= (value_type __x) {
    _M_re *= __x;
    _M_im *= __x;
    return *this;
  }
  _Self& operator/= (value_type __x) {
    _M_re /= __x;
    _M_im /= __x;
    return *this;
  }

  // Arithmetic op= operations involving two complex arguments.

  static void _STLP_CALL _div(const float& __z1_r, const float& __z1_i,
                              const float& __z2_r, const float& __z2_i,
                              float& __res_r, float& __res_i);

  static void _STLP_CALL _div(const float& __z1_r,
                              const float& __z2_r, const float& __z2_i,
                              float& __res_r, float& __res_i);

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _Tp2>
  complex<float>& operator=(const complex<_Tp2>& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<float>& operator+= (const complex<_Tp2>& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<float>& operator-= (const complex<_Tp2>& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<float>& operator*= (const complex<_Tp2>& __z) {
    float __r = _M_re * __z._M_re - _M_im * __z._M_im;
    float __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  template <class _Tp2>
  complex<float>& operator/= (const complex<_Tp2>& __z) {
    float __r;
    float __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

#endif /* _STLP_MEMBER_TEMPLATES */

  _Self& operator=(const _Self& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

  _Self& operator+= (const _Self& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  _Self& operator-= (const _Self& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  _Self& operator*= (const _Self& __z) {
    value_type __r = _M_re * __z._M_re - _M_im * __z._M_im;
    value_type __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  _Self& operator/= (const _Self& __z) {
    value_type __r;
    value_type __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  // Data members.
  value_type _M_re;
  value_type _M_im;
};

_STLP_TEMPLATE_NULL
struct _STLP_CLASS_DECLSPEC complex<double> {
  typedef double value_type;
  typedef complex<double> _Self;

  // Constructors, destructor, assignment operator.

  complex(value_type __x = 0.0, value_type __y = 0.0)
    : _M_re(__x), _M_im(__y) {}

  complex(const complex<double>& __z)
    : _M_re(__z._M_re), _M_im(__z._M_im) {}
  inline complex(const complex<float>& __z);
#if !defined (_STLP_NO_LONG_DOUBLE)
  explicit inline complex(const complex<long double>& __z);
#endif
  // Element access.
  value_type real() const { return _M_re; }
  value_type imag() const { return _M_im; }

  // Arithmetic op= operations involving one real argument.

  _Self& operator= (value_type __x) {
    _M_re = __x;
    _M_im = 0.0;
    return *this;
  }
  _Self& operator+= (value_type __x) {
    _M_re += __x;
    return *this;
  }
  _Self& operator-= (value_type __x) {
    _M_re -= __x;
    return *this;
  }
  _Self& operator*= (value_type __x) {
    _M_re *= __x;
    _M_im *= __x;
    return *this;
  }
  _Self& operator/= (value_type __x) {
    _M_re /= __x;
    _M_im /= __x;
    return *this;
  }

  // Arithmetic op= operations involving two complex arguments.

  static void _STLP_CALL _div(const double& __z1_r, const double& __z1_i,
                              const double& __z2_r, const double& __z2_i,
                              double& __res_r, double& __res_i);
  static void _STLP_CALL _div(const double& __z1_r,
                              const double& __z2_r, const double& __z2_i,
                              double& __res_r, double& __res_i);

#if defined (_STLP_MEMBER_TEMPLATES) && defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  template <class _Tp2>
  complex<double>& operator=(const complex<_Tp2>& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<double>& operator+= (const complex<_Tp2>& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<double>& operator-= (const complex<_Tp2>& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<double>& operator*= (const complex<_Tp2>& __z) {
    double __r = _M_re * __z._M_re - _M_im * __z._M_im;
    double __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  template <class _Tp2>
  complex<double>& operator/= (const complex<_Tp2>& __z) {
    double __r;
    double __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

#endif /* _STLP_MEMBER_TEMPLATES */

  _Self& operator=(const _Self& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

  _Self& operator+= (const _Self& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  _Self& operator-= (const _Self& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  _Self& operator*= (const _Self& __z) {
    value_type __r = _M_re * __z._M_re - _M_im * __z._M_im;
    value_type __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  _Self& operator/= (const _Self& __z) {
    value_type __r;
    value_type __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  // Data members.
  value_type _M_re;
  value_type _M_im;
};

#if !defined (_STLP_NO_LONG_DOUBLE)

_STLP_TEMPLATE_NULL
struct _STLP_CLASS_DECLSPEC complex<long double> {
  typedef long double value_type;
  typedef complex<long double> _Self;

  // Constructors, destructor, assignment operator.
  complex(value_type __x = 0.0l, value_type __y = 0.0l)
    : _M_re(__x), _M_im(__y) {}

  complex(const complex<long double>& __z)
    : _M_re(__z._M_re), _M_im(__z._M_im) {}
  inline complex(const complex<float>& __z);
  inline complex(const complex<double>& __z);

  // Element access.
  value_type real() const { return _M_re; }
  value_type imag() const { return _M_im; }

  // Arithmetic op= operations involving one real argument.

  _Self& operator= (value_type __x) {
    _M_re = __x;
    _M_im = 0.0l;
    return *this;
  }
  _Self& operator+= (value_type __x) {
    _M_re += __x;
    return *this;
  }
  _Self& operator-= (value_type __x) {
    _M_re -= __x;
    return *this;
  }
  _Self& operator*= (value_type __x) {
    _M_re *= __x;
    _M_im *= __x;
    return *this;
  }
  _Self& operator/= (value_type __x) {
    _M_re /= __x;
    _M_im /= __x;
    return *this;
  }

  // Arithmetic op= operations involving two complex arguments.

  static void _STLP_CALL _div(const long double& __z1_r, const long double& __z1_i,
                              const long double& __z2_r, const long double& __z2_i,
                              long double& __res_r, long double& __res_i);

  static void _STLP_CALL _div(const long double& __z1_r,
                              const long double& __z2_r, const long double& __z2_i,
                              long double& __res_r, long double& __res_i);

#  if defined (_STLP_MEMBER_TEMPLATES) && defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)

  template <class _Tp2>
  complex<long double>& operator=(const complex<_Tp2>& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<long double>& operator+= (const complex<_Tp2>& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<long double>& operator-= (const complex<_Tp2>& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  template <class _Tp2>
  complex<long double>& operator*= (const complex<_Tp2>& __z) {
    long double __r = _M_re * __z._M_re - _M_im * __z._M_im;
    long double __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  template <class _Tp2>
  complex<long double>& operator/= (const complex<_Tp2>& __z) {
    long double __r;
    long double __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

#  endif /* _STLP_MEMBER_TEMPLATES */

  _Self& operator=(const _Self& __z) {
    _M_re = __z._M_re;
    _M_im = __z._M_im;
    return *this;
  }

  _Self& operator+= (const _Self& __z) {
    _M_re += __z._M_re;
    _M_im += __z._M_im;
    return *this;
  }

  _Self& operator-= (const _Self& __z) {
    _M_re -= __z._M_re;
    _M_im -= __z._M_im;
    return *this;
  }

  _Self& operator*= (const _Self& __z) {
    value_type __r = _M_re * __z._M_re - _M_im * __z._M_im;
    value_type __i = _M_re * __z._M_im + _M_im * __z._M_re;
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  _Self& operator/= (const _Self& __z) {
    value_type __r;
    value_type __i;
    _div(_M_re, _M_im, __z._M_re, __z._M_im, __r, __i);
    _M_re = __r;
    _M_im = __i;
    return *this;
  }

  // Data members.
  value_type _M_re;
  value_type _M_im;
};

#endif /* _STLP_NO_LONG_DOUBLE */

// Converting constructors from one of these three specialized types
// to another.

inline complex<float>::complex(const complex<double>& __z)
  : _M_re((float)__z._M_re), _M_im((float)__z._M_im) {}
inline complex<double>::complex(const complex<float>& __z)
  : _M_re(__z._M_re), _M_im(__z._M_im) {}
#ifndef _STLP_NO_LONG_DOUBLE
inline complex<float>::complex(const complex<long double>& __z)
  : _M_re((float)__z._M_re), _M_im((float)__z._M_im) {}
inline complex<double>::complex(const complex<long double>& __z)
  : _M_re((double)__z._M_re), _M_im((double)__z._M_im) {}
inline complex<long double>::complex(const complex<float>& __z)
  : _M_re(__z._M_re), _M_im(__z._M_im) {}
inline complex<long double>::complex(const complex<double>& __z)
  : _M_re(__z._M_re), _M_im(__z._M_im) {}
#endif

// Unary non-member arithmetic operators.

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator+(const complex<_Tp>& __z)
{ return __z; }

template <class _Tp>
inline complex<_Tp> _STLP_CALL  operator-(const complex<_Tp>& __z)
{ return complex<_Tp>(-__z._M_re, -__z._M_im); }

// Non-member arithmetic operations involving one real argument.

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator+(const _Tp& __x, const complex<_Tp>& __z)
{ return complex<_Tp>(__x + __z._M_re, __z._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator+(const complex<_Tp>& __z, const _Tp& __x)
{ return complex<_Tp>(__z._M_re + __x, __z._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator-(const _Tp& __x, const complex<_Tp>& __z)
{ return complex<_Tp>(__x - __z._M_re, -__z._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator-(const complex<_Tp>& __z, const _Tp& __x)
{ return complex<_Tp>(__z._M_re - __x, __z._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator*(const _Tp& __x, const complex<_Tp>& __z)
{ return complex<_Tp>(__x * __z._M_re, __x * __z._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator*(const complex<_Tp>& __z, const _Tp& __x)
{ return complex<_Tp>(__z._M_re * __x, __z._M_im * __x); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator/(const _Tp& __x, const complex<_Tp>& __z) {
  complex<_Tp> __result;
  complex<_Tp>::_div(__x,
                     __z._M_re, __z._M_im,
                     __result._M_re, __result._M_im);
  return __result;
}

template <class _Tp>
inline complex<_Tp> _STLP_CALL operator/(const complex<_Tp>& __z, const _Tp& __x)
{ return complex<_Tp>(__z._M_re / __x, __z._M_im / __x); }

// Non-member arithmetic operations involving two complex arguments

template <class _Tp>
inline complex<_Tp> _STLP_CALL
operator+(const complex<_Tp>& __z1, const complex<_Tp>& __z2)
{ return complex<_Tp>(__z1._M_re + __z2._M_re, __z1._M_im + __z2._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL
operator-(const complex<_Tp>& __z1, const complex<_Tp>& __z2)
{ return complex<_Tp>(__z1._M_re - __z2._M_re, __z1._M_im - __z2._M_im); }

template <class _Tp>
inline complex<_Tp> _STLP_CALL
operator*(const complex<_Tp>& __z1, const complex<_Tp>& __z2) {
  return complex<_Tp>(__z1._M_re * __z2._M_re - __z1._M_im * __z2._M_im,
                      __z1._M_re * __z2._M_im + __z1._M_im * __z2._M_re);
}

template <class _Tp>
inline complex<_Tp> _STLP_CALL
operator/(const complex<_Tp>& __z1, const complex<_Tp>& __z2) {
  complex<_Tp> __result;
  complex<_Tp>::_div(__z1._M_re, __z1._M_im,
                     __z2._M_re, __z2._M_im,
                     __result._M_re, __result._M_im);
  return __result;
}

// Comparison operators.

template <class _Tp>
inline bool _STLP_CALL operator==(const complex<_Tp>& __z1, const complex<_Tp>& __z2)
{ return __z1._M_re == __z2._M_re && __z1._M_im == __z2._M_im; }

template <class _Tp>
inline bool _STLP_CALL operator==(const complex<_Tp>& __z, const _Tp& __x)
{ return __z._M_re == __x && __z._M_im == 0; }

template <class _Tp>
inline bool _STLP_CALL operator==(const _Tp& __x, const complex<_Tp>& __z)
{ return __x == __z._M_re && 0 == __z._M_im; }

//04/27/04 dums: removal of this check, if it is restablish
//please explain why the other operators are not macro guarded
//#ifdef _STLP_FUNCTION_TMPL_PARTIAL_ORDER

template <class _Tp>
inline bool _STLP_CALL operator!=(const complex<_Tp>& __z1, const complex<_Tp>& __z2)
{ return __z1._M_re != __z2._M_re || __z1._M_im != __z2._M_im; }

//#endif /* _STLP_FUNCTION_TMPL_PARTIAL_ORDER */

template <class _Tp>
inline bool _STLP_CALL operator!=(const complex<_Tp>& __z, const _Tp& __x)
{ return __z._M_re != __x || __z._M_im != 0; }

template <class _Tp>
inline bool _STLP_CALL operator!=(const _Tp& __x, const complex<_Tp>& __z)
{ return __x != __z._M_re || 0 != __z._M_im; }

// Other basic arithmetic operations
template <class _Tp>
inline _Tp _STLP_CALL real(const complex<_Tp>& __z)
{ return __z._M_re; }

template <class _Tp>
inline _Tp _STLP_CALL imag(const complex<_Tp>& __z)
{ return __z._M_im; }

template <class _Tp>
_Tp _STLP_CALL abs(const complex<_Tp>& __z);

template <class _Tp>
_Tp _STLP_CALL arg(const complex<_Tp>& __z);

template <class _Tp>
inline _Tp _STLP_CALL norm(const complex<_Tp>& __z)
{ return __z._M_re * __z._M_re + __z._M_im * __z._M_im; }

template <class _Tp>
inline complex<_Tp> _STLP_CALL conj(const complex<_Tp>& __z)
{ return complex<_Tp>(__z._M_re, -__z._M_im); }

template <class _Tp>
complex<_Tp> _STLP_CALL polar(const _Tp& __rho)
{ return complex<_Tp>(__rho, 0); }

template <class _Tp>
complex<_Tp> _STLP_CALL polar(const _Tp& __rho, const _Tp& __phi);

_STLP_TEMPLATE_NULL
_STLP_DECLSPEC float _STLP_CALL abs(const complex<float>&);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC double _STLP_CALL abs(const complex<double>&);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC float _STLP_CALL arg(const complex<float>&);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC double _STLP_CALL arg(const complex<double>&);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC complex<float> _STLP_CALL polar(const float& __rho, const float& __phi);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC complex<double> _STLP_CALL polar(const double& __rho, const double& __phi);

template <class _Tp>
_Tp _STLP_CALL abs(const complex<_Tp>& __z)
{ return _Tp(abs(complex<double>(double(__z.real()), double(__z.imag())))); }

template <class _Tp>
_Tp _STLP_CALL arg(const complex<_Tp>& __z)
{ return _Tp(arg(complex<double>(double(__z.real()), double(__z.imag())))); }

template <class _Tp>
complex<_Tp> _STLP_CALL polar(const _Tp& __rho, const _Tp& __phi) {
  complex<double> __tmp = polar(double(__rho), double(__phi));
  return complex<_Tp>(_Tp(__tmp.real()), _Tp(__tmp.imag()));
}

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC long double _STLP_CALL arg(const complex<long double>&);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC long double _STLP_CALL abs(const complex<long double>&);
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC complex<long double> _STLP_CALL polar(const long double&, const long double&);
#endif


#if !defined (_STLP_USE_NO_IOSTREAMS)

_STLP_END_NAMESPACE

#  ifndef _STLP_INTERNAL_IOSFWD
#    include <stl/_iosfwd.h>
#  endif

_STLP_BEGIN_NAMESPACE

// Complex output, in the form (re,im).  We use a two-step process
// involving stringstream so that we get the padding right.
template <class _Tp, class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>&  _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, const complex<_Tp>& __z);

template <class _Tp, class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __is, complex<_Tp>& __z);

// Specializations for narrow characters; lets us avoid widen.

_STLP_OPERATOR_TEMPLATE
_STLP_DECLSPEC basic_istream<char, char_traits<char> >& _STLP_CALL
operator>>(basic_istream<char, char_traits<char> >& __is, complex<float>& __z);

_STLP_OPERATOR_TEMPLATE
_STLP_DECLSPEC basic_istream<char, char_traits<char> >& _STLP_CALL
operator>>(basic_istream<char, char_traits<char> >& __is, complex<double>& __z);

_STLP_OPERATOR_TEMPLATE
_STLP_DECLSPEC basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __is, const complex<float>& __z);

_STLP_OPERATOR_TEMPLATE
_STLP_DECLSPEC basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __is, const complex<double>& __z);

#  if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_OPERATOR_TEMPLATE
_STLP_DECLSPEC basic_istream<char, char_traits<char> >& _STLP_CALL
operator>>(basic_istream<char, char_traits<char> >& __is, complex<long double>& __z);

_STLP_OPERATOR_TEMPLATE
_STLP_DECLSPEC basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __is, const complex<long double>& __z);

#  endif

#  if defined (_STLP_USE_TEMPLATE_EXPORT) && ! defined (_STLP_NO_WCHAR_T)

_STLP_EXPORT_TEMPLATE basic_istream<wchar_t, char_traits<wchar_t> >& _STLP_CALL
operator>>(basic_istream<wchar_t, char_traits<wchar_t> >&, complex<double>&);
_STLP_EXPORT_TEMPLATE basic_ostream<wchar_t, char_traits<wchar_t> >& _STLP_CALL
operator<<(basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<double>&);
_STLP_EXPORT_TEMPLATE basic_istream<wchar_t, char_traits<wchar_t> >& _STLP_CALL
operator>>(basic_istream<wchar_t, char_traits<wchar_t> >&, complex<float>&);
_STLP_EXPORT_TEMPLATE basic_ostream<wchar_t, char_traits<wchar_t> >& _STLP_CALL
operator<<(basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<float>&);

#    if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_EXPORT_TEMPLATE basic_istream<wchar_t, char_traits<wchar_t> >& _STLP_CALL
operator>>(basic_istream<wchar_t, char_traits<wchar_t> >&, complex<long double>&);
_STLP_EXPORT_TEMPLATE basic_ostream<wchar_t, char_traits<wchar_t> >& _STLP_CALL
operator<<(basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<long double>&);
#    endif
#  endif
#endif


// Transcendental functions.  These are defined only for float,
//  double, and long double.  (Sqrt isn't transcendental, of course,
//  but it's included in this section anyway.)

_STLP_DECLSPEC complex<float> _STLP_CALL sqrt(const complex<float>&);

_STLP_DECLSPEC complex<float> _STLP_CALL exp(const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL  log(const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL log10(const complex<float>&);

_STLP_DECLSPEC complex<float> _STLP_CALL pow(const complex<float>&, int);
_STLP_DECLSPEC complex<float> _STLP_CALL pow(const complex<float>&, const float&);
_STLP_DECLSPEC complex<float> _STLP_CALL pow(const float&, const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL pow(const complex<float>&, const complex<float>&);

_STLP_DECLSPEC complex<float> _STLP_CALL sin(const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL cos(const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL tan(const complex<float>&);

_STLP_DECLSPEC complex<float> _STLP_CALL sinh(const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL cosh(const complex<float>&);
_STLP_DECLSPEC complex<float> _STLP_CALL tanh(const complex<float>&);

_STLP_DECLSPEC complex<double> _STLP_CALL sqrt(const complex<double>&);

_STLP_DECLSPEC complex<double> _STLP_CALL exp(const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL log(const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL log10(const complex<double>&);

_STLP_DECLSPEC complex<double> _STLP_CALL pow(const complex<double>&, int);
_STLP_DECLSPEC complex<double> _STLP_CALL pow(const complex<double>&, const double&);
_STLP_DECLSPEC complex<double> _STLP_CALL pow(const double&, const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL pow(const complex<double>&, const complex<double>&);

_STLP_DECLSPEC complex<double> _STLP_CALL sin(const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL cos(const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL tan(const complex<double>&);

_STLP_DECLSPEC complex<double> _STLP_CALL sinh(const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL cosh(const complex<double>&);
_STLP_DECLSPEC complex<double> _STLP_CALL tanh(const complex<double>&);

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL sqrt(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL exp(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL log(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL log10(const complex<long double>&);

_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const complex<long double>&, int);
_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const complex<long double>&, const long double&);
_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const long double&, const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const complex<long double>&,
                                                   const complex<long double>&);

_STLP_DECLSPEC complex<long double> _STLP_CALL sin(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL cos(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL tan(const complex<long double>&);

_STLP_DECLSPEC complex<long double> _STLP_CALL sinh(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL cosh(const complex<long double>&);
_STLP_DECLSPEC complex<long double> _STLP_CALL tanh(const complex<long double>&);
#endif

_STLP_END_NAMESPACE

#ifndef _STLP_LINK_TIME_INSTANTIATION
#  include <stl/_complex.c>
#endif

#endif

// Local Variables:
// mode:C++
// End:
