/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

// WARNING: This is an internal header file, included by other C++
// standard library headers.  You should not attempt to use this header
// file directly.

#ifndef _STLP_INTERNAL_CTRAITS_FUNCTIONS_H
#define _STLP_INTERNAL_CTRAITS_FUNCTIONS_H

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

// This file contains a few small adapters that allow a character
// traits class to be used as a function object.

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Traits>
struct _Eq_traits
  : public binary_function<typename _Traits::char_type,
                           typename _Traits::char_type,
                           bool> {
  bool operator()(const typename _Traits::char_type& __x,
                  const typename _Traits::char_type& __y) const
  { return _Traits::eq(__x, __y); }
};

template <class _Traits>
struct _Eq_char_bound
  : public unary_function<typename _Traits::char_type, bool> {
  typename _Traits::char_type __val;
  _Eq_char_bound(typename _Traits::char_type __c) : __val(__c) {}
  bool operator()(const typename _Traits::char_type& __x) const
  { return _Traits::eq(__x, __val); }
};

template <class _Traits>
struct _Neq_char_bound
  : public unary_function<typename _Traits::char_type, bool>
{
  typename _Traits::char_type __val;
  _Neq_char_bound(typename _Traits::char_type __c) : __val(__c) {}
  bool operator()(const typename _Traits::char_type& __x) const
  { return !_Traits::eq(__x, __val); }
};

template <class _Traits>
struct _Eq_int_bound
  : public unary_function<typename _Traits::char_type, bool> {
  typename _Traits::int_type __val;

  _Eq_int_bound(typename _Traits::int_type __c) : __val(__c) {}
  bool operator()(const typename _Traits::char_type& __x) const
  { return _Traits::eq_int_type(_Traits::to_int_type(__x), __val); }
};

#if 0
template <class _Traits>
struct _Lt_traits
  : public binary_function<typename _Traits::char_type,
                           typename _Traits::char_type,
                           bool> {
  bool operator()(const typename _Traits::char_type& __x,
                  const typename _Traits::char_type& __y) const
  { return _Traits::lt(__x, __y); }
};
#endif

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_CTRAITS_FUNCTIONS_H */

// Local Variables:
// mode:C++
// End:
