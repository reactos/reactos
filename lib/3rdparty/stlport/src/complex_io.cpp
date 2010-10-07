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

#include <complex>
#include <istream>

_STLP_BEGIN_NAMESPACE

// Specializations for narrow characters; lets us avoid the nuisance of
// widening.
_STLP_OPERATOR_SPEC
basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<< (basic_ostream<char, char_traits<char> >& __os, const complex<float>& __z)
{ return __os << '(' << (double)__z.real() << ',' << (double)__z.imag() << ')'; }

_STLP_OPERATOR_SPEC
basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<< (basic_ostream<char, char_traits<char> >& __os, const complex<double>& __z)
{ return __os << '(' << __z.real() << ',' << __z.imag() << ')'; }

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_OPERATOR_SPEC
basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<< (basic_ostream<char, char_traits<char> >& __os, const complex<long double>& __z)
{ return __os << '(' << __z.real() << ',' << __z.imag() << ')'; }
#endif

// Specialization for narrow characters; lets us avoid widen.
_STLP_OPERATOR_SPEC
basic_istream<char, char_traits<char> >& _STLP_CALL
operator>>(basic_istream<char, char_traits<char> >& __is, complex<float>& __z) {
  float  __re = 0;
  float  __im = 0;

  char __c;

  __is >> __c;
  if (__c == '(') {
    __is >> __re >> __c;
    if (__c == ',')
      __is >> __im >> __c;
    if (__c != ')')
      __is.setstate(ios_base::failbit);
  }
  else {
    __is.putback(__c);
    __is >> __re;
  }

  if (__is)
    __z = complex<float>(__re, __im);
  return __is;
}

_STLP_OPERATOR_SPEC
basic_istream<char, char_traits<char> >& _STLP_CALL
operator>>(basic_istream<char, char_traits<char> >& __is, complex<double>& __z) {
  double  __re = 0;
  double  __im = 0;

  char __c;

  __is >> __c;
  if (__c == '(') {
    __is >> __re >> __c;
    if (__c == ',')
      __is >> __im >> __c;
    if (__c != ')')
      __is.setstate(ios_base::failbit);
  }
  else {
    __is.putback(__c);
    __is >> __re;
  }

  if (__is)
    __z = complex<double>(__re, __im);
  return __is;
}

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_OPERATOR_SPEC
basic_istream<char, char_traits<char> >& _STLP_CALL
operator>>(basic_istream<char, char_traits<char> >& __is, complex<long double>& __z) {
  long double  __re = 0;
  long double  __im = 0;

  char __c;

  __is >> __c;
  if (__c == '(') {
    __is >> __re >> __c;
    if (__c == ',')
      __is >> __im >> __c;
    if (__c != ')')
      __is.setstate(ios_base::failbit);
  }
  else {
    __is.putback(__c);
    __is >> __re;
  }

  if (__is)
    __z = complex<long double>(__re, __im);
  return __is;
}
#endif

// Force instantiation of complex I/O functions
#if !(defined (_STLP_NO_FORCE_INSTANTIATE) || defined (_STLP_NO_WCHAR_T))

_STLP_OPERATOR_SPEC basic_istream<wchar_t, char_traits<wchar_t> >&  _STLP_CALL
operator>>(basic_istream<wchar_t, char_traits<wchar_t> >&, complex<float>&);

_STLP_OPERATOR_SPEC basic_istream<wchar_t, char_traits<wchar_t> >&  _STLP_CALL
operator>>(basic_istream<wchar_t, char_traits<wchar_t> >&, complex<double>&);

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_OPERATOR_SPEC basic_istream<wchar_t, char_traits<wchar_t> >&  _STLP_CALL
operator>>(basic_istream<wchar_t, char_traits<wchar_t> >&, complex<long double>&);

_STLP_OPERATOR_SPEC basic_ostream<wchar_t, char_traits<wchar_t> >&  _STLP_CALL
operator<<(basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<long double>&);
#endif

_STLP_OPERATOR_SPEC basic_ostream<wchar_t, char_traits<wchar_t> >&  _STLP_CALL
operator<<(basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<float>&);

_STLP_OPERATOR_SPEC basic_ostream<wchar_t, char_traits<wchar_t> >&  _STLP_CALL
operator<<(basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<double>&);

#endif /* _STLP_NO_WCHAR_T */

_STLP_END_NAMESPACE


// Local Variables:
// mode:C++
// End:

