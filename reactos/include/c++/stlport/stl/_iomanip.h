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

#ifndef _STLP_INTERNAL_IOMANIP
#define _STLP_INTERNAL_IOMANIP

#ifndef _STLP_INTERNAL_ISTREAM
#  include <stl/_istream.h>              // Includes <ostream> and <ios>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

//----------------------------------------------------------------------
// Machinery for defining manipulators.

// Class that calls one of ios_base's single-argument member functions.
template <class _Arg>
struct _Ios_Manip_1 {
   typedef _Arg (ios_base::*__f_ptr_type)(_Arg);

  _Ios_Manip_1(__f_ptr_type __f, const _Arg& __arg)
    : _M_f(__f), _M_arg(__arg) {}

  void operator()(ios_base& __ios) const
  { (__ios.*_M_f)(_M_arg); }

  __f_ptr_type _M_f;
  _Arg _M_arg;
};

// Class that calls one of ios_base's two-argument member functions.
struct _Ios_Setf_Manip {
  ios_base::fmtflags _M_flag;
  ios_base::fmtflags _M_mask;
  bool _M_two_args;

  _Ios_Setf_Manip(ios_base::fmtflags __f)
    : _M_flag(__f), _M_mask(0), _M_two_args(false) {}

  _Ios_Setf_Manip(ios_base::fmtflags __f, ios_base::fmtflags __m)
    : _M_flag(__f), _M_mask(__m), _M_two_args(true) {}

  void operator()(ios_base& __ios) const {
    if (_M_two_args)
      __ios.setf(_M_flag, _M_mask);
    else
      __ios.setf(_M_flag);
  }
};

_STLP_MOVE_TO_STD_NAMESPACE

template <class _CharT, class _Traits, class _Arg>
inline basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __istr,
           const _STLP_PRIV _Ios_Manip_1<_Arg>& __f) {
  __f(__istr);
  return __istr;
}

template <class _CharT, class _Traits, class _Arg>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os,
           const _STLP_PRIV _Ios_Manip_1<_Arg>& __f) {
  __f(__os);
  return __os;
}

template <class _CharT, class _Traits>
inline basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __istr,
           const _STLP_PRIV _Ios_Setf_Manip& __f) {
  __f(__istr);
  return __istr;
}

template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os,
           const _STLP_PRIV _Ios_Setf_Manip& __f) {
  __f(__os);
  return __os;
}

//----------------------------------------------------------------------
// The ios_base manipulators.
inline _STLP_PRIV _Ios_Setf_Manip _STLP_CALL resetiosflags(ios_base::fmtflags __mask)
{ return _STLP_PRIV _Ios_Setf_Manip(0, __mask); }

inline _STLP_PRIV _Ios_Setf_Manip _STLP_CALL setiosflags(ios_base::fmtflags __flag)
{ return _STLP_PRIV _Ios_Setf_Manip(__flag); }

inline _STLP_PRIV _Ios_Setf_Manip _STLP_CALL setbase(int __n) {
  ios_base::fmtflags __base = __n == 8  ? ios_base::oct :
                              __n == 10 ? ios_base::dec :
                              __n == 16 ? ios_base::hex :
                              ios_base::fmtflags(0);
  return _STLP_PRIV _Ios_Setf_Manip(__base, ios_base::basefield);
}

inline _STLP_PRIV _Ios_Manip_1<streamsize> _STLP_CALL
setprecision(int __n) {
  _STLP_PRIV _Ios_Manip_1<streamsize>::__f_ptr_type __f = &ios_base::precision;
  return _STLP_PRIV _Ios_Manip_1<streamsize>(__f, __n);
}

inline _STLP_PRIV _Ios_Manip_1<streamsize>  _STLP_CALL
setw(int __n) {
  _STLP_PRIV _Ios_Manip_1<streamsize>::__f_ptr_type __f = &ios_base::width;  
  return _STLP_PRIV _Ios_Manip_1<streamsize>(__f, __n);
}

//----------------------------------------------------------------------
// setfill, a manipulator that operates on basic_ios<> instead of ios_base.

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _CharT>
struct _Setfill_Manip {
  _Setfill_Manip(_CharT __c) : _M_c(__c) {}
  _CharT _M_c;
};

_STLP_MOVE_TO_STD_NAMESPACE

template <class _CharT, class _CharT2, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os,
           const _STLP_PRIV _Setfill_Manip<_CharT2>& __m) {
  __os.fill(__m._M_c);
  return __os;
}

template <class _CharT, class _CharT2, class _Traits>
inline basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __is,
           const _STLP_PRIV _Setfill_Manip<_CharT2>& __m) {
  __is.fill(__m._M_c);
  return __is;
}

template <class _CharT>
inline _STLP_PRIV _Setfill_Manip<_CharT> _STLP_CALL setfill(_CharT __c)
{ return _STLP_PRIV _Setfill_Manip<_CharT>(__c); }

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_IOMANIP */
