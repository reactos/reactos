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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_INTERNAL_ALGO_H
#define _STLP_INTERNAL_ALGO_H

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_HEAP_H
#  include <stl/_heap.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_H
#  include <stl/_iterator.h>
#endif

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

#if defined (__SUNPRO_CC) && !defined (_STLP_INTERNAL_CSTDIO)
// remove() conflict
#  include <stl/_cstdio.h>
#endif

_STLP_BEGIN_NAMESPACE

// for_each.  Apply a function to every element of a range.
template <class _InputIter, class _Function>
_STLP_INLINE_LOOP _Function
for_each(_InputIter __first, _InputIter __last, _Function __f) {
  for ( ; __first != __last; ++__first)
    __f(*__first);
  return __f;
}

// count_if
template <class _InputIter, class _Predicate>
_STLP_INLINE_LOOP _STLP_DIFFERENCE_TYPE(_InputIter)
count_if(_InputIter __first, _InputIter __last, _Predicate __pred) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  _STLP_DIFFERENCE_TYPE(_InputIter) __n = 0;
  for ( ; __first != __last; ++__first) {
    if (__pred(*__first))
      ++__n;
  }
  return __n;
}

// adjacent_find.

template <class _ForwardIter, class _BinaryPredicate>
_STLP_INLINE_LOOP _ForwardIter
adjacent_find(_ForwardIter __first, _ForwardIter __last,
              _BinaryPredicate __binary_pred) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  if (__first == __last)
    return __last;
  _ForwardIter __next = __first;
  while(++__next != __last) {
    if (__binary_pred(*__first, *__next))
      return __first;
    __first = __next;
  }
  return __last;
}

template <class _ForwardIter>
_STLP_INLINE_LOOP _ForwardIter
adjacent_find(_ForwardIter __first, _ForwardIter __last) {
  return adjacent_find(__first, __last,
                       _STLP_PRIV __equal_to(_STLP_VALUE_TYPE(__first, _ForwardIter)));
}

#if !defined (_STLP_NO_ANACHRONISMS)
template <class _InputIter, class _Tp, class _Size>
_STLP_INLINE_LOOP void
count(_InputIter __first, _InputIter __last, const _Tp& __val, _Size& __n) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    for ( ; __first != __last; ++__first)
      if (*__first == __val)
        ++__n;
}

template <class _InputIter, class _Predicate, class _Size>
_STLP_INLINE_LOOP void
count_if(_InputIter __first, _InputIter __last, _Predicate __pred, _Size& __n) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first)
    if (__pred(*__first))
      ++__n;
}
#endif

template <class _ForwardIter1, class _ForwardIter2>
_ForwardIter1 search(_ForwardIter1 __first1, _ForwardIter1 __last1,
                     _ForwardIter2 __first2, _ForwardIter2 __last2);

// search_n.  Search for __count consecutive copies of __val.
template <class _ForwardIter, class _Integer, class _Tp>
_ForwardIter search_n(_ForwardIter __first, _ForwardIter __last,
                      _Integer __count, const _Tp& __val);
template <class _ForwardIter, class _Integer, class _Tp, class _BinaryPred>
_ForwardIter search_n(_ForwardIter __first, _ForwardIter __last,
                      _Integer __count, const _Tp& __val, _BinaryPred __binary_pred);

template <class _InputIter, class _ForwardIter>
inline _InputIter find_first_of(_InputIter __first1, _InputIter __last1,
                                _ForwardIter __first2, _ForwardIter __last2) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  return _STLP_PRIV __find_first_of(__first1, __last1, __first2, __last2);
}

template <class _InputIter, class _ForwardIter, class _BinaryPredicate>
inline _InputIter
find_first_of(_InputIter __first1, _InputIter __last1,
              _ForwardIter __first2, _ForwardIter __last2, _BinaryPredicate __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))
  return _STLP_PRIV __find_first_of(__first1, __last1, __first2, __last2, __comp);
}

template <class _ForwardIter1, class _ForwardIter2>
_ForwardIter1
find_end(_ForwardIter1 __first1, _ForwardIter1 __last1,
         _ForwardIter2 __first2, _ForwardIter2 __last2);

// swap_ranges
template <class _ForwardIter1, class _ForwardIter2>
_STLP_INLINE_LOOP _ForwardIter2
swap_ranges(_ForwardIter1 __first1, _ForwardIter1 __last1, _ForwardIter2 __first2) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    iter_swap(__first1, __first2);
  return __first2;
}

// transform
template <class _InputIter, class _OutputIter, class _UnaryOperation>
_STLP_INLINE_LOOP _OutputIter
transform(_InputIter __first, _InputIter __last, _OutputIter __result, _UnaryOperation __opr) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first, ++__result)
    *__result = __opr(*__first);
  return __result;
}
template <class _InputIter1, class _InputIter2, class _OutputIter, class _BinaryOperation>
_STLP_INLINE_LOOP _OutputIter
transform(_InputIter1 __first1, _InputIter1 __last1,
          _InputIter2 __first2, _OutputIter __result,_BinaryOperation __binary_op) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  for ( ; __first1 != __last1; ++__first1, ++__first2, ++__result)
    *__result = __binary_op(*__first1, *__first2);
  return __result;
}

// replace_if, replace_copy, replace_copy_if

template <class _ForwardIter, class _Predicate, class _Tp>
_STLP_INLINE_LOOP void
replace_if(_ForwardIter __first, _ForwardIter __last, _Predicate __pred, const _Tp& __new_value) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first)
    if (__pred(*__first))
      *__first = __new_value;
}

template <class _InputIter, class _OutputIter, class _Tp>
_STLP_INLINE_LOOP  _OutputIter
replace_copy(_InputIter __first, _InputIter __last,_OutputIter __result,
             const _Tp& __old_value, const _Tp& __new_value) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first, ++__result)
    *__result = *__first == __old_value ? __new_value : *__first;
  return __result;
}

template <class _Iterator, class _OutputIter, class _Predicate, class _Tp>
_STLP_INLINE_LOOP _OutputIter
replace_copy_if(_Iterator __first, _Iterator __last,
                _OutputIter __result,
                _Predicate __pred, const _Tp& __new_value) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first, ++__result)
    *__result = __pred(*__first) ? __new_value : *__first;
  return __result;
}

// generate and generate_n

template <class _ForwardIter, class _Generator>
_STLP_INLINE_LOOP void
generate(_ForwardIter __first, _ForwardIter __last, _Generator __gen) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first)
    *__first = __gen();
}

template <class _OutputIter, class _Size, class _Generator>
_STLP_INLINE_LOOP void
generate_n(_OutputIter __first, _Size __n, _Generator __gen) {
  for ( ; __n > 0; --__n, ++__first)
    *__first = __gen();
}

// remove, remove_if, remove_copy, remove_copy_if

template <class _InputIter, class _OutputIter, class _Tp>
_STLP_INLINE_LOOP _OutputIter
remove_copy(_InputIter __first, _InputIter __last,_OutputIter __result, const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first) {
    if (!(*__first == __val)) {
      *__result = *__first;
      ++__result;
    }
  }
  return __result;
}

template <class _InputIter, class _OutputIter, class _Predicate>
_STLP_INLINE_LOOP _OutputIter
remove_copy_if(_InputIter __first, _InputIter __last, _OutputIter __result, _Predicate __pred) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first) {
    if (!__pred(*__first)) {
      *__result = *__first;
      ++__result;
    }
  }
  return __result;
}

template <class _ForwardIter, class _Tp>
_STLP_INLINE_LOOP _ForwardIter
remove(_ForwardIter __first, _ForwardIter __last, const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  __first = find(__first, __last, __val);
  if (__first == __last)
    return __first;
  else {
    _ForwardIter __next = __first;
    return remove_copy(++__next, __last, __first, __val);
  }
}

template <class _ForwardIter, class _Predicate>
_STLP_INLINE_LOOP _ForwardIter
remove_if(_ForwardIter __first, _ForwardIter __last, _Predicate __pred) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  __first = find_if(__first, __last, __pred);
  if ( __first == __last )
    return __first;
  else {
    _ForwardIter __next = __first;
    return remove_copy_if(++__next, __last, __first, __pred);
  }
}

// unique and unique_copy
template <class _InputIter, class _OutputIter>
_OutputIter unique_copy(_InputIter __first, _InputIter __last, _OutputIter __result);

template <class _InputIter, class _OutputIter, class _BinaryPredicate>
_OutputIter unique_copy(_InputIter __first, _InputIter __last,_OutputIter __result,
                        _BinaryPredicate __binary_pred);

template <class _ForwardIter>
inline _ForwardIter unique(_ForwardIter __first, _ForwardIter __last) {
  __first = adjacent_find(__first, __last);
  return unique_copy(__first, __last, __first);
}

template <class _ForwardIter, class _BinaryPredicate>
inline _ForwardIter unique(_ForwardIter __first, _ForwardIter __last,
                           _BinaryPredicate __binary_pred) {
  __first = adjacent_find(__first, __last, __binary_pred);
  return unique_copy(__first, __last, __first, __binary_pred);
}

// reverse and reverse_copy, and their auxiliary functions

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _BidirectionalIter>
_STLP_INLINE_LOOP void
__reverse(_BidirectionalIter __first, _BidirectionalIter __last, const bidirectional_iterator_tag &) {
  for (; __first != __last && __first != --__last; ++__first)
    _STLP_STD::iter_swap(__first,__last);
}

template <class _RandomAccessIter>
_STLP_INLINE_LOOP void
__reverse(_RandomAccessIter __first, _RandomAccessIter __last, const random_access_iterator_tag &) {
  for (; __first < __last; ++__first)
    _STLP_STD::iter_swap(__first, --__last);
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _BidirectionalIter>
inline void
reverse(_BidirectionalIter __first, _BidirectionalIter __last) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  _STLP_PRIV __reverse(__first, __last, _STLP_ITERATOR_CATEGORY(__first, _BidirectionalIter));
}

template <class _BidirectionalIter, class _OutputIter>
_STLP_INLINE_LOOP
_OutputIter reverse_copy(_BidirectionalIter __first,
                         _BidirectionalIter __last,
                         _OutputIter __result) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  while (__first != __last) {
    --__last;
    *__result = *__last;
    ++__result;
  }
  return __result;
}

template <class _ForwardIter>
void rotate(_ForwardIter __first, _ForwardIter __middle, _ForwardIter __last);

template <class _ForwardIter, class _OutputIter>
inline _OutputIter rotate_copy(_ForwardIter __first, _ForwardIter __middle,
                               _ForwardIter __last, _OutputIter __result) {
  return _STLP_STD::copy(__first, __middle, copy(__middle, __last, __result));
}

// random_shuffle

template <class _RandomAccessIter>
void random_shuffle(_RandomAccessIter __first, _RandomAccessIter __last);

template <class _RandomAccessIter, class _RandomNumberGenerator>
void random_shuffle(_RandomAccessIter __first, _RandomAccessIter __last,
                    _RandomNumberGenerator& __rand);

#if !defined (_STLP_NO_EXTENSIONS)
// random_sample and random_sample_n (extensions, not part of the standard).

template <class _ForwardIter, class _OutputIter, class _Distance>
_OutputIter random_sample_n(_ForwardIter __first, _ForwardIter __last,
                            _OutputIter __out_ite, const _Distance __n);

template <class _ForwardIter, class _OutputIter, class _Distance,
          class _RandomNumberGenerator>
_OutputIter random_sample_n(_ForwardIter __first, _ForwardIter __last,
                            _OutputIter __out_ite, const _Distance __n,
                            _RandomNumberGenerator& __rand);

template <class _InputIter, class _RandomAccessIter>
_RandomAccessIter
random_sample(_InputIter __first, _InputIter __last,
              _RandomAccessIter __out_first, _RandomAccessIter __out_last);

template <class _InputIter, class _RandomAccessIter,
          class _RandomNumberGenerator>
_RandomAccessIter
random_sample(_InputIter __first, _InputIter __last,
              _RandomAccessIter __out_first, _RandomAccessIter __out_last,
              _RandomNumberGenerator& __rand);

#endif /* _STLP_NO_EXTENSIONS */

// partition, stable_partition, and their auxiliary functions

template <class _ForwardIter, class _Predicate>
_ForwardIter partition(_ForwardIter __first, _ForwardIter __last, _Predicate   __pred);

template <class _ForwardIter, class _Predicate>
_ForwardIter
stable_partition(_ForwardIter __first, _ForwardIter __last, _Predicate __pred);

// sort() and its auxiliary functions.
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Size>
inline _Size __lg(_Size __n) {
  _Size __k;
  for (__k = 0; __n != 1; __n >>= 1) ++__k;
  return __k;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _RandomAccessIter>
void sort(_RandomAccessIter __first, _RandomAccessIter __last);
template <class _RandomAccessIter, class _Compare>
void sort(_RandomAccessIter __first, _RandomAccessIter __last, _Compare __comp);

// stable_sort() and its auxiliary functions.
template <class _RandomAccessIter>
void stable_sort(_RandomAccessIter __first,
                 _RandomAccessIter __last);

template <class _RandomAccessIter, class _Compare>
void stable_sort(_RandomAccessIter __first,
                 _RandomAccessIter __last, _Compare __comp);

// partial_sort, partial_sort_copy, and auxiliary functions.

template <class _RandomAccessIter>
void partial_sort(_RandomAccessIter __first, _RandomAccessIter __middle,
                  _RandomAccessIter __last);

template <class _RandomAccessIter, class _Compare>
void partial_sort(_RandomAccessIter __first,_RandomAccessIter __middle,
                  _RandomAccessIter __last, _Compare __comp);

template <class _InputIter, class _RandomAccessIter>
_RandomAccessIter
partial_sort_copy(_InputIter __first, _InputIter __last,
                  _RandomAccessIter __result_first, _RandomAccessIter __result_last);

template <class _InputIter, class _RandomAccessIter, class _Compare>
_RandomAccessIter
partial_sort_copy(_InputIter __first, _InputIter __last,
                  _RandomAccessIter __result_first,
                  _RandomAccessIter __result_last, _Compare __comp);

// nth_element() and its auxiliary functions.
template <class _RandomAccessIter>
void nth_element(_RandomAccessIter __first, _RandomAccessIter __nth,
                 _RandomAccessIter __last);

template <class _RandomAccessIter, class _Compare>
void nth_element(_RandomAccessIter __first, _RandomAccessIter __nth,
                 _RandomAccessIter __last, _Compare __comp);

// auxiliary class for lower_bound, etc.
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _T1, class _T2>
struct __less_2 {
  bool operator() (const _T1& __x, const _T2& __y) const { return __x < __y ; }
};

template <class _T1, class _T2>
__less_2<_T1,_T2> __less2(_T1*, _T2* ) { return __less_2<_T1, _T2>(); }

#if defined (_STLP_FUNCTION_PARTIAL_ORDER)
template <class _Tp>
less<_Tp> __less2(_Tp*, _Tp* ) { return less<_Tp>(); }
#endif

_STLP_MOVE_TO_STD_NAMESPACE

// Binary search (lower_bound, upper_bound, equal_range, binary_search).
template <class _ForwardIter, class _Tp>
inline _ForwardIter lower_bound(_ForwardIter __first, _ForwardIter __last,
                                   const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __lower_bound(__first, __last, __val,
                                  _STLP_PRIV __less2(_STLP_VALUE_TYPE(__first, _ForwardIter), (_Tp*)0),
                                  _STLP_PRIV __less2((_Tp*)0, _STLP_VALUE_TYPE(__first, _ForwardIter)),
                                  _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

template <class _ForwardIter, class _Tp, class _Compare>
inline _ForwardIter lower_bound(_ForwardIter __first, _ForwardIter __last,
                                const _Tp& __val, _Compare __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __lower_bound(__first, __last, __val, __comp, __comp,
                                  _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _Tp, class _Compare1, class _Compare2, class _Distance>
_ForwardIter __upper_bound(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
                           _Compare1 __comp1, _Compare2 __comp2, _Distance*);

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter, class _Tp>
inline _ForwardIter upper_bound(_ForwardIter __first, _ForwardIter __last,
                                const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __upper_bound(__first, __last, __val,
                                  _STLP_PRIV __less2(_STLP_VALUE_TYPE(__first, _ForwardIter), (_Tp*)0),
                                  _STLP_PRIV __less2((_Tp*)0, _STLP_VALUE_TYPE(__first, _ForwardIter)),
                                  _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

template <class _ForwardIter, class _Tp, class _Compare>
inline _ForwardIter upper_bound(_ForwardIter __first, _ForwardIter __last,
                                const _Tp& __val, _Compare __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __upper_bound(__first, __last, __val, __comp, __comp,
                                  _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _Tp, class _Compare1, class _Compare2, class _Distance>
pair<_ForwardIter, _ForwardIter>
__equal_range(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
              _Compare1 __comp1, _Compare2 __comp2, _Distance*);

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter, class _Tp>
inline pair<_ForwardIter, _ForwardIter>
equal_range(_ForwardIter __first, _ForwardIter __last, const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __equal_range(__first, __last, __val,
                                  _STLP_PRIV __less2(_STLP_VALUE_TYPE(__first, _ForwardIter), (_Tp*)0),
                                  _STLP_PRIV __less2((_Tp*)0, _STLP_VALUE_TYPE(__first, _ForwardIter)),
                                  _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

template <class _ForwardIter, class _Tp, class _Compare>
inline pair<_ForwardIter, _ForwardIter>
equal_range(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
            _Compare __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __equal_range(__first, __last, __val, __comp, __comp,
                                  _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

template <class _ForwardIter, class _Tp>
inline bool binary_search(_ForwardIter __first, _ForwardIter __last,
                   const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  _ForwardIter __i = _STLP_PRIV __lower_bound(__first, __last, __val,
                                              _STLP_PRIV __less2(_STLP_VALUE_TYPE(__first, _ForwardIter), (_Tp*)0),
                                              _STLP_PRIV __less2((_Tp*)0, _STLP_VALUE_TYPE(__first, _ForwardIter)),
                                              _STLP_DISTANCE_TYPE(__first, _ForwardIter));
  return __i != __last && !(__val < *__i);
}

template <class _ForwardIter, class _Tp, class _Compare>
inline bool binary_search(_ForwardIter __first, _ForwardIter __last,
                          const _Tp& __val,
                          _Compare __comp) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  _ForwardIter __i = _STLP_PRIV __lower_bound(__first, __last, __val, __comp, __comp,
                                              _STLP_DISTANCE_TYPE(__first, _ForwardIter));
  return __i != __last && !__comp(__val, *__i);
}

// merge, with and without an explicitly supplied comparison function.

template <class _InputIter1, class _InputIter2, class _OutputIter>
_OutputIter merge(_InputIter1 __first1, _InputIter1 __last1,
                  _InputIter2 __first2, _InputIter2 __last2,
                  _OutputIter __result);

template <class _InputIter1, class _InputIter2, class _OutputIter,
          class _Compare>
_OutputIter merge(_InputIter1 __first1, _InputIter1 __last1,
                  _InputIter2 __first2, _InputIter2 __last2,
                  _OutputIter __result, _Compare __comp);


// inplace_merge and its auxiliary functions.


template <class _BidirectionalIter>
void inplace_merge(_BidirectionalIter __first,
                   _BidirectionalIter __middle,
                   _BidirectionalIter __last) ;

template <class _BidirectionalIter, class _Compare>
void inplace_merge(_BidirectionalIter __first,
                   _BidirectionalIter __middle,
                   _BidirectionalIter __last, _Compare __comp);

// Set algorithms: includes, set_union, set_intersection, set_difference,
// set_symmetric_difference.  All of these algorithms have the precondition
// that their input ranges are sorted and the postcondition that their output
// ranges are sorted.

template <class _InputIter1, class _InputIter2>
bool includes(_InputIter1 __first1, _InputIter1 __last1,
              _InputIter2 __first2, _InputIter2 __last2);

template <class _InputIter1, class _InputIter2, class _Compare>
bool includes(_InputIter1 __first1, _InputIter1 __last1,
              _InputIter2 __first2, _InputIter2 __last2, _Compare __comp);

template <class _InputIter1, class _InputIter2, class _OutputIter>
_OutputIter set_union(_InputIter1 __first1, _InputIter1 __last1,
                      _InputIter2 __first2, _InputIter2 __last2,
                      _OutputIter __result);

template <class _InputIter1, class _InputIter2, class _OutputIter,
          class _Compare>
_OutputIter set_union(_InputIter1 __first1, _InputIter1 __last1,
                      _InputIter2 __first2, _InputIter2 __last2,
                      _OutputIter __result, _Compare __comp);

template <class _InputIter1, class _InputIter2, class _OutputIter>
_OutputIter set_intersection(_InputIter1 __first1, _InputIter1 __last1,
                             _InputIter2 __first2, _InputIter2 __last2,
                             _OutputIter __result);

template <class _InputIter1, class _InputIter2, class _OutputIter,
          class _Compare>
_OutputIter set_intersection(_InputIter1 __first1, _InputIter1 __last1,
                             _InputIter2 __first2, _InputIter2 __last2,
                             _OutputIter __result, _Compare __comp);



template <class _InputIter1, class _InputIter2, class _OutputIter>
_OutputIter set_difference(_InputIter1 __first1, _InputIter1 __last1,
                           _InputIter2 __first2, _InputIter2 __last2,
                           _OutputIter __result);

template <class _InputIter1, class _InputIter2, class _OutputIter,
          class _Compare>
_OutputIter set_difference(_InputIter1 __first1, _InputIter1 __last1,
                           _InputIter2 __first2, _InputIter2 __last2,
                           _OutputIter __result, _Compare __comp);

template <class _InputIter1, class _InputIter2, class _OutputIter>
_OutputIter
set_symmetric_difference(_InputIter1 __first1, _InputIter1 __last1,
                         _InputIter2 __first2, _InputIter2 __last2,
                         _OutputIter __result);


template <class _InputIter1, class _InputIter2, class _OutputIter,
          class _Compare>
_OutputIter
set_symmetric_difference(_InputIter1 __first1, _InputIter1 __last1,
                         _InputIter2 __first2, _InputIter2 __last2,
                         _OutputIter __result,
                         _Compare __comp);


// min_element and max_element, with and without an explicitly supplied
// comparison function.

template <class _ForwardIter>
_ForwardIter max_element(_ForwardIter __first, _ForwardIter __last);
template <class _ForwardIter, class _Compare>
_ForwardIter max_element(_ForwardIter __first, _ForwardIter __last,
                            _Compare __comp);

template <class _ForwardIter>
_ForwardIter min_element(_ForwardIter __first, _ForwardIter __last);

template <class _ForwardIter, class _Compare>
_ForwardIter min_element(_ForwardIter __first, _ForwardIter __last,
                            _Compare __comp);

// next_permutation and prev_permutation, with and without an explicitly
// supplied comparison function.

template <class _BidirectionalIter>
bool next_permutation(_BidirectionalIter __first, _BidirectionalIter __last);

template <class _BidirectionalIter, class _Compare>
bool next_permutation(_BidirectionalIter __first, _BidirectionalIter __last,
                      _Compare __comp);


template <class _BidirectionalIter>
bool prev_permutation(_BidirectionalIter __first, _BidirectionalIter __last);


template <class _BidirectionalIter, class _Compare>
bool prev_permutation(_BidirectionalIter __first, _BidirectionalIter __last,
                      _Compare __comp);

#if !defined (_STLP_NO_EXTENSIONS)
// is_heap, a predicate testing whether or not a range is
// a heap.  This function is an extension, not part of the C++
// standard.

template <class _RandomAccessIter>
bool is_heap(_RandomAccessIter __first, _RandomAccessIter __last);

template <class _RandomAccessIter, class _StrictWeakOrdering>
bool is_heap(_RandomAccessIter __first, _RandomAccessIter __last,
             _StrictWeakOrdering __comp);

// is_sorted, a predicated testing whether a range is sorted in
// nondescending order.  This is an extension, not part of the C++
// standard.
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _StrictWeakOrdering>
bool __is_sorted(_ForwardIter __first, _ForwardIter __last,
                 _StrictWeakOrdering __comp);

_STLP_MOVE_TO_STD_NAMESPACE
template <class _ForwardIter>
inline bool is_sorted(_ForwardIter __first, _ForwardIter __last) {
  return _STLP_PRIV __is_sorted(__first, __last,
                                _STLP_PRIV __less(_STLP_VALUE_TYPE(__first, _ForwardIter)));
}

template <class _ForwardIter, class _StrictWeakOrdering>
inline bool is_sorted(_ForwardIter __first, _ForwardIter __last,
                      _StrictWeakOrdering __comp) {
  return _STLP_PRIV __is_sorted(__first, __last, __comp);
}
#endif

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_algo.c>
#endif

#endif /* _STLP_INTERNAL_ALGO_H */

// Local Variables:
// mode:C++
// End:

