/*
 * Copyright (c) 2005
 * Francois Dumont
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_CARRAY_H
#define _STLP_CARRAY_H

/* Purpose: Mimic a pur C array with the additionnal feature of
 * being able to be used with type not default constructible.
 */

#ifndef _STLP_INTERNAL_CONSTRUCT_H
#  include <stl/_construct.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp, size_t _Nb>
struct _CArray {
  _CArray (const _Tp& __val) {
    for (size_t __i = 0; __i < _Nb; ++__i) {
      _Copy_Construct(__REINTERPRET_CAST(_Tp*, _M_data + __i * sizeof(_Tp)), __val);
    }
  }

  ~_CArray() {
    _Destroy_Range(__REINTERPRET_CAST(_Tp*, _M_data + 0),
                   __REINTERPRET_CAST(_Tp*, _M_data + _Nb * sizeof(_Tp)));
  }

  _Tp& operator [] (size_t __i) {
    _STLP_ASSERT(__i < _Nb)
    return *__REINTERPRET_CAST(_Tp*, _M_data + __i * sizeof(_Tp));
  }

private:
  char _M_data[sizeof(_Tp) * _Nb];
};

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif //_STLP_CARRAY_H
