/*
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
#ifndef _STLP_ALGOBASE_C
#define _STLP_ALGOBASE_C

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _InputIter1, class _InputIter2>
bool lexicographical_compare(_InputIter1 __first1, _InputIter1 __last1,
                             _InputIter2 __first2, _InputIter2 __last2) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  for ( ; __first1 != __last1 && __first2 != __last2
        ; ++__first1, ++__first2) {
    if (*__first1 < *__first2) {
      _STLP_VERBOSE_ASSERT(!(*__first2 < *__first1), _StlMsg_INVALID_STRICT_WEAK_PREDICATE)
      return true;
    }
    if (*__first2 < *__first1)
      return false;
  }
  return __first1 == __last1 && __first2 != __last2;
}

template <class _InputIter1, class _InputIter2, class _Compare>
bool lexicographical_compare(_InputIter1 __first1, _InputIter1 __last1,
                             _InputIter2 __first2, _InputIter2 __last2,
                             _Compare __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  for ( ; __first1 != __last1 && __first2 != __last2
        ; ++__first1, ++__first2) {
    if (__comp(*__first1, *__first2)) {
      _STLP_VERBOSE_ASSERT(!__comp(*__first2, *__first1),
                           _StlMsg_INVALID_STRICT_WEAK_PREDICATE)
      return true;
    }
    if (__comp(*__first2, *__first1))
      return false;
  }
  return __first1 == __last1 && __first2 != __last2;
}

#if !defined (_STLP_NO_EXTENSIONS)
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter1, class _InputIter2>
int __lexicographical_compare_3way(_InputIter1 __first1, _InputIter1 __last1,
                                   _InputIter2 __first2, _InputIter2 __last2) {
  while (__first1 != __last1 && __first2 != __last2) {
    if (*__first1 < *__first2) {
      _STLP_VERBOSE_ASSERT(!(*__first2 < *__first1), _StlMsg_INVALID_STRICT_WEAK_PREDICATE)
      return -1;
    }
    if (*__first2 < *__first1)
      return 1;
    ++__first1;
    ++__first2;
  }
  if (__first2 == __last2) {
    return !(__first1 == __last1);
  }
  else {
    return -1;
  }
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter1, class _InputIter2>
int lexicographical_compare_3way(_InputIter1 __first1, _InputIter1 __last1,
                                 _InputIter2 __first2, _InputIter2 __last2) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  return _STLP_PRIV __lexicographical_compare_3way(__first1, __last1, __first2, __last2);
}
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _RandomAccessIter, class _Tp>
_STLP_INLINE_LOOP _RandomAccessIter __find(_RandomAccessIter __first, _RandomAccessIter __last,
                                           const _Tp& __val,
                                           const random_access_iterator_tag &) {
  _STLP_DIFFERENCE_TYPE(_RandomAccessIter) __trip_count = (__last - __first) >> 2;

  for ( ; __trip_count > 0 ; --__trip_count) {
    if (*__first == __val) return __first;
    ++__first;

    if (*__first == __val) return __first;
    ++__first;

    if (*__first == __val) return __first;
    ++__first;

    if (*__first == __val) return __first;
    ++__first;
  }

  switch (__last - __first) {
  case 3:
    if (*__first == __val) return __first;
    ++__first;
  case 2:
    if (*__first == __val) return __first;
    ++__first;
  case 1:
    if (*__first == __val) return __first;
    //++__first;
  case 0:
  default:
    return __last;
  }
}

inline char*
__find(char* __first, char* __last, char __val, const random_access_iterator_tag &) {
  void *res =  memchr(__first, __val, __last - __first);
  return res != 0 ? __STATIC_CAST(char*, res) : __last;
}
inline const char*
__find(const char* __first, const char* __last, char __val, const random_access_iterator_tag &) {
  const void *res =  memchr(__first, __val, __last - __first);
  return res != 0 ? __STATIC_CAST(const char*, res) : __last;
}

template <class _RandomAccessIter, class _Predicate>
_STLP_INLINE_LOOP _RandomAccessIter __find_if(_RandomAccessIter __first, _RandomAccessIter __last,
                                              _Predicate __pred,
                                              const random_access_iterator_tag &) {
  _STLP_DIFFERENCE_TYPE(_RandomAccessIter) __trip_count = (__last - __first) >> 2;

  for ( ; __trip_count > 0 ; --__trip_count) {
    if (__pred(*__first)) return __first;
    ++__first;

    if (__pred(*__first)) return __first;
    ++__first;

    if (__pred(*__first)) return __first;
    ++__first;

    if (__pred(*__first)) return __first;
    ++__first;
  }

  switch(__last - __first) {
  case 3:
    if (__pred(*__first)) return __first;
    ++__first;
  case 2:
    if (__pred(*__first)) return __first;
    ++__first;
  case 1:
    if (__pred(*__first)) return __first;
      //++__first;
  case 0:
  default:
    return __last;
  }
}

template <class _InputIter, class _Tp>
_STLP_INLINE_LOOP _InputIter __find(_InputIter __first, _InputIter __last,
                                    const _Tp& __val,
                                    const input_iterator_tag &) {
  while (__first != __last && !(*__first == __val)) ++__first;
  return __first;
}

template <class _InputIter, class _Predicate>
_STLP_INLINE_LOOP _InputIter __find_if(_InputIter __first, _InputIter __last,
                                       _Predicate __pred,
                                       const input_iterator_tag &) {
  while (__first != __last && !__pred(*__first))
    ++__first;
  return __first;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter, class _Predicate>
_InputIter find_if(_InputIter __first, _InputIter __last,
                   _Predicate __pred) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __find_if(__first, __last, __pred, _STLP_ITERATOR_CATEGORY(__first, _InputIter));
}

template <class _InputIter, class _Tp>
_InputIter find(_InputIter __first, _InputIter __last, const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __find(__first, __last, __val, _STLP_ITERATOR_CATEGORY(__first, _InputIter));
}

template <class _ForwardIter1, class _ForwardIter2, class _BinaryPred>
_ForwardIter1 search(_ForwardIter1 __first1, _ForwardIter1 __last1,
                     _ForwardIter2 __first2, _ForwardIter2 __last2,
                     _BinaryPred  __pred) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  // Test for empty ranges
  if (__first1 == __last1 || __first2 == __last2)
    return __first1;

  // Test for a pattern of length 1.
  _ForwardIter2 __p1(__first2);

  if ( ++__p1 == __last2 ) {
    while (__first1 != __last1 && !__pred(*__first1, *__first2)) {
      ++__first1;
    }
    return __first1;
  }

  // General case.

  for ( ; ; ) { // __first1 != __last1 will be checked below
    while (__first1 != __last1 && !__pred(*__first1, *__first2)) {
      ++__first1;
    }
    if (__first1 == __last1) {
      return __last1;
    }
    _ForwardIter2 __p = __p1;
    _ForwardIter1 __current = __first1;
    if (++__current == __last1) return __last1;

    while (__pred(*__current, *__p)) {
      if (++__p == __last2)
        return __first1;
      if (++__current == __last1)
        return __last1;
    }
    ++__first1;
  }
  return __first1;
}

_STLP_MOVE_TO_PRIV_NAMESPACE
template <class _Tp>
struct _IsCharLikeType
{ typedef __false_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsCharLikeType<char>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsCharLikeType<unsigned char>
{ typedef __true_type _Ret; };

#  ifndef _STLP_NO_SIGNED_BUILTINS
_STLP_TEMPLATE_NULL struct _IsCharLikeType<signed char>
{ typedef __true_type _Ret; };
#  endif

template <class _Tp1, class _Tp2>
inline bool __stlp_eq(_Tp1 __val1, _Tp2 __val2)
{ return __val1 == __val2; }

#if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
template <class _Tp>
inline bool __stlp_eq(_Tp, _Tp)
{ return true; }
#endif

template <class _InputIter, class _ForwardIter, class _Tp2, class _Predicate>
inline _InputIter __find_first_of_aux2(_InputIter __first1, _InputIter __last1,
                                       _ForwardIter __first2, _ForwardIter __last2,
                                       _Tp2*, _Predicate __pred,
                                       const __true_type& /* _UseStrcspnLikeAlgo */) {
  unsigned char __hints[(UCHAR_MAX + 1) / CHAR_BIT];
  memset(__hints, 0, sizeof(__hints) / sizeof(unsigned char));
  for (; __first2 != __last2; ++__first2) {
    unsigned char __tmp = (unsigned char)*__first2;
    __hints[__tmp / CHAR_BIT] |= (1 << (__tmp % CHAR_BIT));
  }

  for (; __first1 != __last1; ++__first1) {
    _Tp2 __tmp = (_Tp2)*__first1;
    if (__stlp_eq(*__first1, __tmp) &&
        __pred((__hints[(unsigned char)__tmp / CHAR_BIT] & (1 << ((unsigned char)__tmp % CHAR_BIT))) != 0))
      break;
  }
  return __first1;
}

template <class _InputIter, class _ForwardIter, class _Tp2, class _Predicate>
inline _InputIter __find_first_of_aux2(_InputIter __first1, _InputIter __last1,
                                       _ForwardIter __first2, _ForwardIter __last2,
                                       _Tp2* /* __dummy */, _Predicate /* __pred */,
                                       const __false_type& /* _UseStrcspnLikeAlgo */) {
  return _STLP_PRIV __find_first_of(__first1, __last1, __first2, __last2,
                                    _STLP_PRIV __equal_to(_STLP_VALUE_TYPE(__first1, _InputIter)));
}

template <class _InputIter, class _ForwardIter, class _Tp1, class _Tp2>
inline _InputIter __find_first_of_aux1(_InputIter __first1, _InputIter __last1,
                                       _ForwardIter __first2, _ForwardIter __last2,
                                       _Tp1* __pt1, _Tp2* __pt2) {
  typedef _STLP_TYPENAME _STLP_STD::_IsIntegral<_Tp1>::_Ret _IsIntegral;
  typedef _STLP_TYPENAME _STLP_PRIV _IsCharLikeType<_Tp2>::_Ret _IsCharLike;
  typedef _STLP_TYPENAME _STLP_STD::_Land2<_IsIntegral, _IsCharLike>::_Ret _UseStrcspnLikeAlgo;
  return _STLP_PRIV __find_first_of_aux2(__first1, __last1,
                                         __first2, __last2,
                                         __pt2, _Identity<bool>(), _UseStrcspnLikeAlgo());
}

template <class _InputIter, class _ForwardIter>
inline _InputIter __find_first_of(_InputIter __first1, _InputIter __last1,
                                  _ForwardIter __first2, _ForwardIter __last2) {
  return _STLP_PRIV __find_first_of_aux1(__first1, __last1, __first2, __last2,
                                         _STLP_VALUE_TYPE(__first1, _InputIter),
                                         _STLP_VALUE_TYPE(__first2, _ForwardIter));
}

// find_first_of, with and without an explicitly supplied comparison function.
template <class _InputIter, class _ForwardIter, class _BinaryPredicate>
_InputIter __find_first_of(_InputIter __first1, _InputIter __last1,
                           _ForwardIter __first2, _ForwardIter __last2,
                           _BinaryPredicate __comp) {
  for ( ; __first1 != __last1; ++__first1) {
    for (_ForwardIter __iter = __first2; __iter != __last2; ++__iter) {
      if (__comp(*__first1, *__iter)) {
        return __first1;
      }
    }
  }
  return __last1;
}

// find_end, with and without an explicitly supplied comparison function.
// Search [first2, last2) as a subsequence in [first1, last1), and return
// the *last* possible match.  Note that find_end for bidirectional iterators
// is much faster than for forward iterators.

// find_end for forward iterators.
template <class _ForwardIter1, class _ForwardIter2,
  class _BinaryPredicate>
_ForwardIter1 __find_end(_ForwardIter1 __first1, _ForwardIter1 __last1,
                         _ForwardIter2 __first2, _ForwardIter2 __last2,
                         const forward_iterator_tag &, const forward_iterator_tag &,
                         _BinaryPredicate __comp) {
  if (__first2 == __last2)
    return __last1;
  else {
    _ForwardIter1 __result = __last1;
    for (;;) {
      _ForwardIter1 __new_result = _STLP_STD::search(__first1, __last1, __first2, __last2, __comp);
      if (__new_result == __last1)
        return __result;
      else {
        __result = __new_result;
        __first1 = __new_result;
        ++__first1;
      }
    }
  }
}

_STLP_MOVE_TO_STD_NAMESPACE

// find_end for bidirectional iterators.  Requires partial specialization.
#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)

#  ifndef _STLP_INTERNAL_ITERATOR_H
_STLP_END_NAMESPACE
#    include <stl/_iterator.h>
_STLP_BEGIN_NAMESPACE
#  endif /*_STLP_INTERNAL_ITERATOR_H*/

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _BidirectionalIter1, class _BidirectionalIter2,
          class _BinaryPredicate>
_BidirectionalIter1
__find_end(_BidirectionalIter1 __first1, _BidirectionalIter1 __last1,
           _BidirectionalIter2 __first2, _BidirectionalIter2 __last2,
           const bidirectional_iterator_tag &, const bidirectional_iterator_tag &,
           _BinaryPredicate __comp) {
  typedef _STLP_STD::reverse_iterator<_BidirectionalIter1> _RevIter1;
  typedef _STLP_STD::reverse_iterator<_BidirectionalIter2> _RevIter2;

  _RevIter1 __rlast1(__first1);
  _RevIter2 __rlast2(__first2);
  _RevIter1 __rresult = _STLP_STD::search(_RevIter1(__last1), __rlast1,
                                          _RevIter2(__last2), __rlast2,
                                          __comp);

  if (__rresult == __rlast1)
    return __last1;
  else {
    _BidirectionalIter1 __result = __rresult.base();
    _STLP_STD::advance(__result, -_STLP_STD::distance(__first2, __last2));
    return __result;
  }
}

_STLP_MOVE_TO_STD_NAMESPACE
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

template <class _ForwardIter1, class _ForwardIter2,
          class _BinaryPredicate>
_ForwardIter1
find_end(_ForwardIter1 __first1, _ForwardIter1 __last1,
         _ForwardIter2 __first2, _ForwardIter2 __last2,
         _BinaryPredicate __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  return _STLP_PRIV __find_end(__first1, __last1, __first2, __last2,
#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
                               _STLP_ITERATOR_CATEGORY(__first1, _ForwardIter1),
                               _STLP_ITERATOR_CATEGORY(__first2, _ForwardIter2),
#else
                               forward_iterator_tag(),
                               forward_iterator_tag(),
#endif
                               __comp);
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _Tp, class _Compare1, class _Compare2, class _Distance>
_ForwardIter __lower_bound(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
                           _Compare1 __comp1, _Compare2 __comp2, _Distance*) {
  _Distance __len = _STLP_STD::distance(__first, __last);
  _Distance __half;
  _ForwardIter __middle;

  while (__len > 0) {
    __half = __len >> 1;
    __middle = __first;
    _STLP_STD::advance(__middle, __half);
    if (__comp1(*__middle, __val)) {
      _STLP_VERBOSE_ASSERT(!__comp2(__val, *__middle), _StlMsg_INVALID_STRICT_WEAK_PREDICATE)
      __first = __middle;
      ++__first;
      __len = __len - __half - 1;
    }
    else
      __len = __half;
  }
  return __first;
}

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /* _STLP_ALGOBASE_C */

// Local Variables:
// mode:C++
// End:
