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

#ifndef _STLP_INTERNAL_UNINITIALIZED_H
#define _STLP_INTERNAL_UNINITIALIZED_H

#ifndef _STLP_INTERNAL_CSTRING
#  include <stl/_cstring.h>
#endif

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_CONSTRUCT_H
#  include <stl/_construct.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

// uninitialized_copy

template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __ucopy(_InputIter __first, _InputIter __last,
                           _OutputIter __result, _Distance*) {
  _OutputIter __cur = __result;
  _STLP_TRY {
    for ( ; __first != __last; ++__first, ++__cur)
      _Param_Construct(&*__cur, *__first);
    return __cur;
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__result, __cur))
  _STLP_RET_AFTER_THROW(__cur)
}

template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __ucopy(_InputIter __first, _InputIter __last,
                           _OutputIter __result, const input_iterator_tag &, _Distance* __d)
{ return __ucopy(__first, __last, __result, __d); }

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __ucopy(_InputIter __first, _InputIter __last,
                           _OutputIter __result, const forward_iterator_tag &, _Distance* __d)
{ return __ucopy(__first, __last, __result, __d); }

template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __ucopy(_InputIter __first, _InputIter __last,
                           _OutputIter __result, const bidirectional_iterator_tag &, _Distance* __d)
{ return __ucopy(__first, __last, __result, __d); }
#endif

template <class _RandomAccessIter, class _OutputIter, class _Distance>
inline _OutputIter __ucopy(_RandomAccessIter __first, _RandomAccessIter __last,
                           _OutputIter __result, const random_access_iterator_tag &, _Distance*) {
  _OutputIter __cur = __result;
  _STLP_TRY {
    for (_Distance __n = __last - __first; __n > 0; --__n) {
      _Param_Construct(&*__cur, *__first);
      ++__first;
      ++__cur;
    }
    return __cur;
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__result, __cur))
  _STLP_RET_AFTER_THROW(__cur)
}

//Used internaly
template <class _RandomAccessIter, class _OutputIter>
inline _OutputIter __ucopy(_RandomAccessIter __first, _RandomAccessIter __last, _OutputIter __result)
{ return __ucopy(__first, __last, __result, random_access_iterator_tag(), (ptrdiff_t*)0); }

inline void*
__ucopy_trivial(const void* __first, const void* __last, void* __result) {
  //dums: this version can use memcpy (__copy_trivial can't)
  return (__last == __first) ? __result :
    ((char*)memcpy(__result, __first, ((const char*)__last - (const char*)__first))) +
    ((const char*)__last - (const char*)__first);
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __ucopy_ptrs(_InputIter __first, _InputIter __last, _OutputIter __result,
                                const __false_type& /*TrivialUCopy*/)
{ return __ucopy(__first, __last, __result, random_access_iterator_tag(), (ptrdiff_t*)0); }

template <class _InputIter, class _OutputIter>
inline _OutputIter __ucopy_ptrs(_InputIter __first, _InputIter __last, _OutputIter __result,
                                const __true_type& /*TrivialUCopy*/) {
  // we know they all pointers, so this cast is OK
  //  return (_OutputIter)__copy_trivial(&(*__first), &(*__last), &(*__result));
  return (_OutputIter)__ucopy_trivial(__first, __last, __result);
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __ucopy_aux(_InputIter __first, _InputIter __last, _OutputIter __result,
                               const __true_type& /*BothPtrType*/) {
  return __ucopy_ptrs(__first, __last, __result,
                      _UseTrivialUCopy(_STLP_VALUE_TYPE(__first, _InputIter),
                                       _STLP_VALUE_TYPE(__result, _OutputIter))._Answer());
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __ucopy_aux(_InputIter __first, _InputIter __last, _OutputIter __result,
                               const __false_type& /*BothPtrType*/) {
  return __ucopy(__first, __last, __result,
                 _STLP_ITERATOR_CATEGORY(__first, _InputIter),
                 _STLP_DISTANCE_TYPE(__first, _InputIter));
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter, class _ForwardIter>
inline _ForwardIter
uninitialized_copy(_InputIter __first, _InputIter __last, _ForwardIter __result)
{ return _STLP_PRIV __ucopy_aux(__first, __last, __result, _BothPtrType< _InputIter, _ForwardIter>::_Answer()); }

inline char*
uninitialized_copy(const char* __first, const char* __last, char* __result)
{ return  (char*)_STLP_PRIV __ucopy_trivial(__first, __last, __result); }

#  if defined (_STLP_HAS_WCHAR_T) // dwa 8/15/97
inline wchar_t*
uninitialized_copy(const wchar_t* __first, const wchar_t* __last, wchar_t* __result)
{ return  (wchar_t*)_STLP_PRIV __ucopy_trivial (__first, __last, __result); }
#  endif

// uninitialized_copy_n (not part of the C++ standard)
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter, class _Size, class _ForwardIter>
_STLP_INLINE_LOOP
pair<_InputIter, _ForwardIter>
__ucopy_n(_InputIter __first, _Size __count, _ForwardIter __result,
          const input_iterator_tag &) {
  _ForwardIter __cur = __result;
  _STLP_TRY {
    for ( ; __count > 0 ; --__count, ++__first, ++__cur)
      _Param_Construct(&*__cur, *__first);
    return pair<_InputIter, _ForwardIter>(__first, __cur);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__result, __cur))
  _STLP_RET_AFTER_THROW((pair<_InputIter, _ForwardIter>(__first, __cur)))
}

#  if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _InputIter, class _Size, class _ForwardIterator>
inline pair<_InputIter, _ForwardIterator>
__ucopy_n(_InputIter __first, _Size __count,
                       _ForwardIterator __result,
                       const forward_iterator_tag &)
{ return __ucopy_n(__first, __count, __result, input_iterator_tag()); }

template <class _InputIter, class _Size, class _ForwardIterator>
inline pair<_InputIter, _ForwardIterator>
__ucopy_n(_InputIter __first, _Size __count,
                       _ForwardIterator __result,
                       const bidirectional_iterator_tag &)
{ return __ucopy_n(__first, __count, __result, input_iterator_tag()); }
#  endif

template <class _RandomAccessIter, class _Size, class _ForwardIter>
inline pair<_RandomAccessIter, _ForwardIter>
__ucopy_n(_RandomAccessIter __first, _Size __count, _ForwardIter __result,
                       const random_access_iterator_tag &) {
  _RandomAccessIter __last = __first + __count;
  return pair<_RandomAccessIter, _ForwardIter>(__last, uninitialized_copy(__first, __last, __result));
}

// This is used internally in <rope> , which is extension itself.
template <class _InputIter, class _Size, class _ForwardIter>
inline pair<_InputIter, _ForwardIter>
__ucopy_n(_InputIter __first, _Size __count, _ForwardIter __result)
{ return _STLP_PRIV __ucopy_n(__first, __count, __result, _STLP_ITERATOR_CATEGORY(__first, _InputIter)); }

#if !defined (_STLP_NO_EXTENSIONS)

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter, class _Size, class _ForwardIter>
inline pair<_InputIter, _ForwardIter>
uninitialized_copy_n(_InputIter __first, _Size __count, _ForwardIter __result)
{ return _STLP_PRIV __ucopy_n(__first, __count, __result); }

_STLP_MOVE_TO_PRIV_NAMESPACE

#endif

template <class _ForwardIter, class _Tp, class _Distance>
inline void __ufill(_ForwardIter __first, _ForwardIter __last, const _Tp& __x, _Distance*) {
  _ForwardIter __cur = __first;
  _STLP_TRY {
    for ( ; __cur != __last; ++__cur)
      _Param_Construct(&*__cur, __x);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__first, __cur))
}

template <class _ForwardIter, class _Tp, class _Distance>
inline void __ufill(_ForwardIter __first, _ForwardIter __last,
                    const _Tp& __x, const input_iterator_tag &, _Distance* __d)
{ __ufill(__first, __last, __x, __d); }

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _ForwardIter, class _Tp, class _Distance>
inline void __ufill(_ForwardIter __first, _ForwardIter __last,
                    const _Tp& __x, const forward_iterator_tag &, _Distance* __d)
{ __ufill(__first, __last, __x, __d); }

template <class _ForwardIter, class _Tp, class _Distance>
inline void __ufill(_ForwardIter __first, _ForwardIter __last,
                    const _Tp& __x, const bidirectional_iterator_tag &, _Distance* __d)
{ __ufill(__first, __last, __x, __d); }
#endif

template <class _ForwardIter, class _Tp, class _Distance>
inline void __ufill(_ForwardIter __first, _ForwardIter __last,
                    const _Tp& __x, const random_access_iterator_tag &, _Distance*) {
  _ForwardIter __cur = __first;
  _STLP_TRY {
    for (_Distance __n = __last - __first; __n > 0; --__n, ++__cur)
      _Param_Construct(&*__cur, __x);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__first, __cur))
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter, class _Tp>
inline void uninitialized_fill(_ForwardIter __first, _ForwardIter __last,  const _Tp& __x) {
  _STLP_PRIV __ufill(__first, __last, __x,
                     _STLP_ITERATOR_CATEGORY(__first, _ForwardIter),
                     _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

// Specialization: for one-byte types we can use memset.
inline void uninitialized_fill(unsigned char* __first, unsigned char* __last,
                               const unsigned char& __val) {
  unsigned char __tmp = __val;
  memset(__first, __tmp, __last - __first);
}
#if !defined (_STLP_NO_SIGNED_BUILTINS)
inline void uninitialized_fill(signed char* __first, signed char* __last,
                               const signed char& __val) {
  signed char __tmp = __val;
  memset(__first, __STATIC_CAST(unsigned char,__tmp), __last - __first);
}
#endif
inline void uninitialized_fill(char* __first, char* __last, const char& __val) {
  char __tmp = __val;
  memset(__first, __STATIC_CAST(unsigned char,__tmp), __last - __first);
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __ufill_n(_ForwardIter __first, _Size __n, const _Tp& __x) {
  _ForwardIter __cur = __first;
  _STLP_TRY {
    for ( ; __n > 0; --__n, ++__cur)
      _Param_Construct(&*__cur, __x);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__first, __cur))
  return __cur;
}

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __ufill_n(_ForwardIter __first, _Size __n, const _Tp& __x,
                              const input_iterator_tag &)
{ return __ufill_n(__first, __n, __x); }

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __ufill_n(_ForwardIter __first, _Size __n, const _Tp& __x,
                              const forward_iterator_tag &)
{ return __ufill_n(__first, __n, __x); }

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __ufill_n(_ForwardIter __first, _Size __n, const _Tp& __x,
                              const bidirectional_iterator_tag &)
{ return __ufill_n(__first, __n, __x); }
#endif

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __uninitialized_fill_n(_ForwardIter __first, _Size __n, const _Tp& __x) {
  _ForwardIter __last = __first + __n;
  __ufill(__first, __last, __x, random_access_iterator_tag(), (ptrdiff_t*)0);
  return __last;
}

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __ufill_n(_ForwardIter __first, _Size __n, const _Tp& __x,
                              const random_access_iterator_tag &)
{ return __uninitialized_fill_n(__first, __n, __x); }

/* __uninitialized_init is an internal algo to init a range with a value
 * built using default constructor. It is only called with pointer as
 * iterator.
 */
template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __uinit_aux_aux(_ForwardIter __first, _Size __n, const _Tp& __val,
                                    const __false_type& /*_HasDefaultZero*/)
{ return __uninitialized_fill_n(__first, __n, __val); }

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __uinit_aux_aux(_ForwardIter __first, _Size __n, const _Tp& /* __val */,
                                    const __true_type& /*_HasDefaultZero*/) {
  memset((unsigned char*)__first, 0, __n * sizeof(_Tp));
  return __first + __n;
}

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __uinit_aux(_ForwardIter __first, _Size __n, const _Tp&,
                                const __true_type& /*_TrivialInit*/)
{ return __first + __n; }

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __uinit_aux(_ForwardIter __first, _Size __n, const _Tp& __val,
                                const __false_type& /*_TrivialInit*/)
{ return __uinit_aux_aux(__first, __n, __val, _HasDefaultZeroValue(__first)._Answer()); }

template <class _ForwardIter, class _Size, class _Tp>
inline _ForwardIter __uninitialized_init(_ForwardIter __first, _Size __n, const _Tp& __val)
{ return __uinit_aux(__first, __n, __val, _UseTrivialInit(__first)._Answer()); }

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter, class _Size, class _Tp>
inline void
uninitialized_fill_n(_ForwardIter __first, _Size __n, const _Tp& __x)
{ _STLP_PRIV __ufill_n(__first, __n, __x, _STLP_ITERATOR_CATEGORY(__first, _ForwardIter)); }

// Extensions: __uninitialized_copy_copy, __uninitialized_copy_fill,
// __uninitialized_fill_copy.

// __uninitialized_copy_copy
// Copies [first1, last1) into [result, result + (last1 - first1)), and
//  copies [first2, last2) into
//  [result + (last1 - first1), result + (last1 - first1) + (last2 - first2)).

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter1, class _InputIter2, class _ForwardIter>
inline _ForwardIter
__uninitialized_copy_copy(_InputIter1 __first1, _InputIter1 __last1,
                          _InputIter2 __first2, _InputIter2 __last2,
                          _ForwardIter __result) {
  _ForwardIter __new_result = uninitialized_copy(__first1, __last1, __result);
  _STLP_TRY {
    return uninitialized_copy(__first2, __last2, __new_result);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__result, __new_result))
  _STLP_RET_AFTER_THROW(__result)
}

// __uninitialized_fill_copy
// Fills [result, mid) with x, and copies [first, last) into
//  [mid, mid + (last - first)).
template <class _ForwardIter, class _Tp, class _InputIter>
inline _ForwardIter
__uninitialized_fill_copy(_ForwardIter __result, _ForwardIter __mid, const _Tp& __x,
                          _InputIter __first, _InputIter __last) {
  uninitialized_fill(__result, __mid, __x);
  _STLP_TRY {
    return uninitialized_copy(__first, __last, __mid);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__result, __mid))
  _STLP_RET_AFTER_THROW(__result)
}

// __uninitialized_copy_fill
// Copies [first1, last1) into [first2, first2 + (last1 - first1)), and
//  fills [first2 + (last1 - first1), last2) with x.
template <class _Iter, class _Tp>
inline void
__uninitialized_copy_fill(_Iter __first1, _Iter __last1, _Iter __first2, _Iter __last2,
                          const _Tp& __x) {
  _Iter __mid2 = uninitialized_copy(__first1, __last1, __first2);
  _STLP_TRY {
    uninitialized_fill(__mid2, __last2, __x);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(__first2, __mid2))
}

/* __uninitialized_move:
 * This function is used internaly and only with pointers as iterators.
 */
template <class _InputIter, class _ForwardIter, class _TrivialUCpy>
inline _ForwardIter
__uninitialized_move(_InputIter __first, _InputIter __last, _ForwardIter __result,
                     _TrivialUCpy __trivial_ucpy, const __false_type& /*_Movable*/)
{ return __ucopy_ptrs(__first, __last, __result, __trivial_ucpy); }

template <class _InputIter, class _ForwardIter, class _TrivialUCpy>
_STLP_INLINE_LOOP
_ForwardIter
__uninitialized_move(_InputIter __first, _InputIter __last, _ForwardIter __result,
                     _TrivialUCpy , const __true_type& /*_Movable*/) {
  //Move constructor should not throw so we do not need to take care of exceptions here.
  for (ptrdiff_t __n = __last - __first ; __n > 0; --__n) {
    _Move_Construct(&*__result, *__first);
    ++__first; ++__result;
  }
  return __result;
}

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_UNINITIALIZED_H */

// Local Variables:
// mode:C++
// End:
