/*
 *
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
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
#ifndef _STLP_NUMERIC_C
#define _STLP_NUMERIC_C

#ifndef _STLP_INTERNAL_NUMERIC_H
# include <stl/_numeric.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIterator, class _OutputIterator, class _Tp,
          class _BinaryOperation>
_OutputIterator
__partial_sum(_InputIterator __first, _InputIterator __last,
              _OutputIterator __result, _Tp*, _BinaryOperation __binary_op) {
  _STLP_DEBUG_CHECK(__check_range(__first, __last))
  if (__first == __last) return __result;
  *__result = *__first;

  _Tp __val = *__first;
  while (++__first != __last) {
    __val = __binary_op(__val, *__first);
    *++__result = __val;
  }
  return ++__result;
}

template <class _InputIterator, class _OutputIterator, class _Tp,
          class _BinaryOperation>
_OutputIterator
__adjacent_difference(_InputIterator __first, _InputIterator __last,
                      _OutputIterator __result, _Tp*,
                      _BinaryOperation __binary_op) {
  _STLP_DEBUG_CHECK(__check_range(__first, __last))
  if (__first == __last) return __result;
  *__result = *__first;
  _Tp __val = *__first;
  while (++__first != __last) {
    _Tp __tmp = *__first;
    *++__result = __binary_op(__tmp, __val);
    __val = __tmp;
  }
  return ++__result;
}


template <class _Tp, class _Integer, class _MonoidOperation>
_Tp __power(_Tp __x, _Integer __n, _MonoidOperation __opr) {
  _STLP_MPWFIX_TRY
  if (__n == 0)
    return __identity_element(__opr);
  else {
    while ((__n & 1) == 0) {
      __n >>= 1;
      __x = __opr(__x, __x);
    }
    _Tp __result = __x;
  _STLP_MPWFIX_TRY
    __n >>= 1;
    while (__n != 0) {
      __x = __opr(__x, __x);
      if ((__n & 1) != 0)
        __result = __opr(__result, __x);
      __n >>= 1;
    }
    return __result;
  _STLP_MPWFIX_CATCH
  }
  _STLP_MPWFIX_CATCH_ACTION(__x = _Tp())
}

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /*  _STLP_NUMERIC_C */

// Local Variables:
// mode:C++
// End:
