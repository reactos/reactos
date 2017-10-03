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

#ifndef _STLP_INTERNAL_ALGOBASE_H
#define _STLP_INTERNAL_ALGOBASE_H

#ifndef _STLP_INTERNAL_CSTDDEF
#  include <stl/_cstddef.h>
#endif

#ifndef _STLP_INTERNAL_CSTRING
#  include <stl/_cstring.h>
#endif

#ifndef _STLP_CLIMITS
#  include <climits>
#endif

#ifndef _STLP_INTERNAL_CSTDLIB
#  include <stl/_cstdlib.h>
#endif

#ifndef _STLP_INTERNAL_PAIR_H
#  include <stl/_pair.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#  include <stl/_iterator_base.h>
#endif

#ifndef _STLP_TYPE_TRAITS_H
#  include <stl/type_traits.h>
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
_STLP_MOVE_TO_PRIV_NAMESPACE
template <class _Tp>
inline void __swap_aux(_Tp& __a, _Tp& __b, const __true_type& /*SwapImplemented*/) {
  __a._M_swap_workaround(__b);
}

template <class _Tp>
inline void __swap_aux(_Tp& __a, _Tp& __b, const __false_type& /*SwapImplemented*/) {
  _Tp __tmp = __a;
  __a = __b;
  __b = __tmp;
}
_STLP_MOVE_TO_STD_NAMESPACE
#endif

// swap and iter_swap
template <class _Tp>
inline void swap(_Tp& __a, _Tp& __b) {
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
#  if !defined(__BORLANDC__)
  typedef typename _SwapImplemented<_Tp>::_Ret _Implemented;
#  else
  enum { _Is = _SwapImplemented<_Tp>::_Is };
  typedef typename __bool2type<_Is>::_Ret _Implemented;
#  endif
  _STLP_PRIV __swap_aux(__a, __b, _Implemented());
#else
  _Tp __tmp = __a;
  __a = __b;
  __b = __tmp;
#endif
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter1, class _ForwardIter2, class _Value>
inline void __iter_swap_aux_aux(_ForwardIter1& __i1, _ForwardIter2& __i2, _Value *) {
  _Value tmp = *__i1;
  *__i1 = *__i2;
  *__i2 = tmp;
}

template <class _ForwardIter1, class _ForwardIter2>
inline void __iter_swap_aux(_ForwardIter1& __i1, _ForwardIter2& __i2, const __true_type& /*OKToSwap*/) {
  /* namespace specification breaks access to the right swap template overload (at least for gcc) */
  /*_STLP_STD::*/ swap(*__i1, *__i2);
}

template <class _ForwardIter1, class _ForwardIter2>
inline void __iter_swap_aux(_ForwardIter1& __i1, _ForwardIter2& __i2, const __false_type& /*OKToSwap*/) {
  _STLP_PRIV __iter_swap_aux_aux( __i1, __i2, _STLP_VALUE_TYPE(__i1,_ForwardIter1) );
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter1, class _ForwardIter2>
inline void iter_swap(_ForwardIter1 __i1, _ForwardIter2 __i2) {
  _STLP_PRIV __iter_swap_aux( __i1, __i2, _IsOKToSwap(_STLP_VALUE_TYPE(__i1, _ForwardIter1), _STLP_VALUE_TYPE(__i2, _ForwardIter2),
                                                      _STLP_IS_REF_TYPE_REAL_REF(__i1, _ForwardIter1),
                                                      _STLP_IS_REF_TYPE_REAL_REF(__i2, _ForwardIter2))._Answer());
}

//--------------------------------------------------
// min and max

#if !defined (__BORLANDC__) || defined (_STLP_USE_OWN_NAMESPACE)
#  if (defined (__BORLANDC__) && (__BORLANDC__ < 0x580)) && !defined (__STDC__)
//In not ANSI mode Borland import min/max in global namespace which conflict
//with STLport min/max when user does a 'using namespace std' in its code
//(see test/unit/alg_test.cpp). To avoid this clash we simply import Borland min/max
//in STLport namespace.
using _STLP_VENDOR_STD::min;
using _STLP_VENDOR_STD::max;
#  else
template <class _Tp>
inline const _Tp& (min)(const _Tp& __a, const _Tp& __b) { return __b < __a ? __b : __a; }
template <class _Tp>
inline const _Tp& (max)(const _Tp& __a, const _Tp& __b) {  return  __a < __b ? __b : __a; }
#  endif
#endif

# if defined (__BORLANDC__) && defined (_STLP_USE_OWN_NAMESPACE)
inline unsigned long (min) (unsigned long __a, unsigned long __b) { return __b < __a ? __b : __a; }
inline unsigned long (max) (unsigned long __a, unsigned long __b) {  return  __a < __b ? __b : __a; }
# endif

#  if !defined (__BORLANDC__) || (__BORLANDC__ < 0x590)
template <class _Tp, class _Compare>
inline const _Tp& (min)(const _Tp& __a, const _Tp& __b, _Compare __comp) {
  return __comp(__b, __a) ? __b : __a;
}

template <class _Tp, class _Compare>
inline const _Tp& (max)(const _Tp& __a, const _Tp& __b, _Compare __comp) {
  return __comp(__a, __b) ? __b : __a;
}
#  else
template <class _Tp, class _Compare>
inline const _Tp (min)(const _Tp __a, const _Tp __b, _Compare __comp) {
  return __comp(__b, __a) ? __b : __a;
}

template <class _Tp, class _Compare>
inline const _Tp (max)(const _Tp __a, const _Tp __b, _Compare __comp) {
  return __comp(__a, __b) ? __b : __a;
}
#  endif

//--------------------------------------------------
// copy

// All of these auxiliary functions serve two purposes.  (1) Replace
// calls to copy with memmove whenever possible.  (Memmove, not memcpy,
// because the input and output ranges are permitted to overlap.)
// (2) If we're using random access iterators, then write the loop as
// a for loop with an explicit count.

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __copy(_InputIter __first, _InputIter __last,
                          _OutputIter __result, const input_iterator_tag &, _Distance*) {
  for ( ; __first != __last; ++__result, ++__first)
    *__result = *__first;
  return __result;
}

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __copy(_InputIter __first, _InputIter __last,
                          _OutputIter __result, const forward_iterator_tag &, _Distance* ) {
  for ( ; __first != __last; ++__result, ++__first)
    *__result = *__first;
  return __result;
}

template <class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __copy(_InputIter __first, _InputIter __last,
                          _OutputIter __result, const bidirectional_iterator_tag &, _Distance* ) {
  for ( ; __first != __last; ++__result, ++__first)
    *__result = *__first;
  return __result;
}
#endif

template <class _RandomAccessIter, class _OutputIter, class _Distance>
inline _OutputIter
__copy(_RandomAccessIter __first, _RandomAccessIter __last,
       _OutputIter __result, const random_access_iterator_tag &, _Distance*) {
  for (_Distance __n = __last - __first; __n > 0; --__n) {
    *__result = *__first;
    ++__first;
    ++__result;
  }
  return __result;
}

inline void*
__copy_trivial(const void* __first, const void* __last, void* __result) {
  size_t __n = (const char*)__last - (const char*)__first;
  return __n ? (void *)((char*)memmove(__result, __first, __n) + __n) : __result;
}

//--------------------------------------------------
// copy_backward auxiliary functions

template <class _BidirectionalIter1, class _BidirectionalIter2,
          class _Distance>
inline _BidirectionalIter2 __copy_backward(_BidirectionalIter1 __first,
                                           _BidirectionalIter1 __last,
                                           _BidirectionalIter2 __result,
                                           const bidirectional_iterator_tag &,
                                           _Distance*) {
  while (__first != __last)
    *--__result = *--__last;
  return __result;
}

template <class _RandomAccessIter, class _BidirectionalIter, class _Distance>
inline _BidirectionalIter __copy_backward(_RandomAccessIter __first,
                                          _RandomAccessIter __last,
                                          _BidirectionalIter __result,
                                          const random_access_iterator_tag &,
                                          _Distance*) {
  for (_Distance __n = __last - __first; __n > 0; --__n)
    *--__result = *--__last;
  return __result;
}

inline void*
__copy_trivial_backward(const void* __first, const void* __last, void* __result) {
  const ptrdiff_t _Num = (const char*)__last - (const char*)__first;
  return (_Num > 0) ? memmove((char*)__result - _Num, __first, _Num) : __result ;
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_ptrs(_InputIter __first, _InputIter __last, _OutputIter __result,
                               const __false_type& /*IsOKToMemCpy*/) {
  return _STLP_PRIV __copy(__first, __last, __result, random_access_iterator_tag(), (ptrdiff_t*)0);
}
template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_ptrs(_InputIter __first, _InputIter __last, _OutputIter __result,
                               const __true_type& /*IsOKToMemCpy*/) {
  // we know they all pointers, so this cast is OK
  //  return (_OutputIter)__copy_trivial(&(*__first), &(*__last), &(*__result));
  return (_OutputIter)_STLP_PRIV __copy_trivial(__first, __last, __result);
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_aux(_InputIter __first, _InputIter __last, _OutputIter __result,
                              const __true_type& /*BothPtrType*/) {
  return _STLP_PRIV __copy_ptrs(__first, __last, __result,
                                _UseTrivialCopy(_STLP_VALUE_TYPE(__first, _InputIter),
                                                _STLP_VALUE_TYPE(__result, _OutputIter))._Answer());
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_aux(_InputIter __first, _InputIter __last, _OutputIter __result,
                              const __false_type& /*BothPtrType*/) {
  return _STLP_PRIV __copy(__first, __last, __result,
                           _STLP_ITERATOR_CATEGORY(__first, _InputIter),
                           _STLP_DISTANCE_TYPE(__first, _InputIter));
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter, class _OutputIter>
inline _OutputIter copy(_InputIter __first, _InputIter __last, _OutputIter __result) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __copy_aux(__first, __last, __result, _BothPtrType< _InputIter, _OutputIter>::_Answer());
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_backward_ptrs(_InputIter __first, _InputIter __last,
                                        _OutputIter __result, const __false_type& /*TrivialAssignment*/) {
  return _STLP_PRIV __copy_backward(__first, __last, __result,
                                    _STLP_ITERATOR_CATEGORY(__first, _InputIter),
                                    _STLP_DISTANCE_TYPE(__first, _InputIter));
}
template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_backward_ptrs(_InputIter __first, _InputIter __last,
                                        _OutputIter __result, const __true_type& /*TrivialAssignment*/) {
  return (_OutputIter)_STLP_PRIV __copy_trivial_backward(__first, __last, __result);
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_backward_aux(_InputIter __first, _InputIter __last, _OutputIter __result, const __false_type&) {
  return _STLP_PRIV __copy_backward(__first, __last, __result,
                                    _STLP_ITERATOR_CATEGORY(__first,_InputIter),
                                    _STLP_DISTANCE_TYPE(__first, _InputIter));
}

template <class _InputIter, class _OutputIter>
inline _OutputIter __copy_backward_aux(_InputIter __first, _InputIter __last, _OutputIter __result, const __true_type&) {
  return _STLP_PRIV __copy_backward_ptrs(__first, __last, __result,
                                         _UseTrivialCopy(_STLP_VALUE_TYPE(__first, _InputIter),
                                                         _STLP_VALUE_TYPE(__result, _OutputIter))._Answer());
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter, class _OutputIter>
inline _OutputIter copy_backward(_InputIter __first, _InputIter __last, _OutputIter __result) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  return _STLP_PRIV __copy_backward_aux(__first, __last, __result, _BothPtrType< _InputIter, _OutputIter>::_Answer() );
}

#if !defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS)
#  define _STLP_DECLARE_COPY_TRIVIAL(_Tp)                                       \
inline _Tp* copy(const _Tp* __first, const _Tp* __last, _Tp* __result)          \
{ return (_Tp*)_STLP_PRIV __copy_trivial(__first, __last, __result); }          \
inline _Tp* copy_backward(const _Tp* __first, const _Tp* __last, _Tp* __result) \
{ return (_Tp*)_STLP_PRIV __copy_trivial_backward(__first, __last, __result); }

#  if !defined (_STLP_NO_BOOL)
_STLP_DECLARE_COPY_TRIVIAL(bool)
#  endif
_STLP_DECLARE_COPY_TRIVIAL(char)
#  if !defined (_STLP_NO_SIGNED_BUILTINS)
_STLP_DECLARE_COPY_TRIVIAL(signed char)
#  endif
_STLP_DECLARE_COPY_TRIVIAL(unsigned char)
_STLP_DECLARE_COPY_TRIVIAL(short)
_STLP_DECLARE_COPY_TRIVIAL(unsigned short)
_STLP_DECLARE_COPY_TRIVIAL(int)
_STLP_DECLARE_COPY_TRIVIAL(unsigned int)
_STLP_DECLARE_COPY_TRIVIAL(long)
_STLP_DECLARE_COPY_TRIVIAL(unsigned long)
#  if !defined(_STLP_NO_WCHAR_T) && !defined (_STLP_WCHAR_T_IS_USHORT)
_STLP_DECLARE_COPY_TRIVIAL(wchar_t)
#  endif
#  if defined (_STLP_LONG_LONG)
_STLP_DECLARE_COPY_TRIVIAL(_STLP_LONG_LONG)
_STLP_DECLARE_COPY_TRIVIAL(unsigned _STLP_LONG_LONG)
#  endif
_STLP_DECLARE_COPY_TRIVIAL(float)
_STLP_DECLARE_COPY_TRIVIAL(double)
#  if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLARE_COPY_TRIVIAL(long double)
#  endif
#  undef _STLP_DECLARE_COPY_TRIVIAL
#endif

//--------------------------------------------------
// copy_n (not part of the C++ standard)

#if !defined (_STLP_NO_EXTENSIONS)
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter, class _Size, class _OutputIter>
_STLP_INLINE_LOOP _STLP_STD::pair<_InputIter, _OutputIter>
__copy_n(_InputIter __first, _Size __count, _OutputIter __result,
         const input_iterator_tag &) {
  for ( ; __count > 0; --__count) {
    *__result = *__first;
    ++__first;
    ++__result;
  }
  return _STLP_STD::pair<_InputIter, _OutputIter>(__first, __result);
}

template <class _RAIter, class _Size, class _OutputIter>
inline _STLP_STD::pair<_RAIter, _OutputIter>
__copy_n(_RAIter __first, _Size __count, _OutputIter __result,
         const random_access_iterator_tag &) {
  _RAIter __last = __first + __count;
  return _STLP_STD::pair<_RAIter, _OutputIter>(__last, _STLP_STD::copy(__first, __last, __result));
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIter, class _Size, class _OutputIter>
inline pair<_InputIter, _OutputIter>
copy_n(_InputIter __first, _Size __count, _OutputIter __result) {
  _STLP_FIX_LITERAL_BUG(__first)
  return _STLP_PRIV __copy_n(__first, __count, __result, _STLP_ITERATOR_CATEGORY(__first, _InputIter));
}
#endif

//--------------------------------------------------
// fill and fill_n
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _Tp>
_STLP_INLINE_LOOP
void __fill_fwd(_ForwardIter __first, _ForwardIter __last, const _Tp& __val) {
  for ( ; __first != __last; ++__first)
    *__first = __val;
}

template <class _ForwardIter, class _Tp, class _Distance>
inline void __fill(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
                   const input_iterator_tag &, _Distance*) {
  _STLP_PRIV __fill_fwd(__first, __last, __val);
}

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _ForwardIter, class _Tp, class _Distance>
_STLP_INLINE_LOOP
void __fill(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
            const forward_iterator_tag &, _Distance*) {
  _STLP_PRIV __fill_fwd(__first, __last, __val);
}

template <class _ForwardIter, class _Tp, class _Distance>
_STLP_INLINE_LOOP
void __fill(_ForwardIter __first, _ForwardIter __last, const _Tp& __val,
            const bidirectional_iterator_tag &, _Distance*) {
  _STLP_PRIV __fill_fwd(__first, __last, __val);
}
#endif

template <class _RandomAccessIter, class _Tp, class _Distance>
_STLP_INLINE_LOOP
void __fill(_RandomAccessIter __first, _RandomAccessIter __last, const _Tp& __val,
            const random_access_iterator_tag &, _Distance*) {
  for (_Distance __n = __last - __first ; __n > 0; ++__first, --__n)
    *__first = __val;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter, class _Tp>
inline void fill(_ForwardIter __first, _ForwardIter __last, const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  _STLP_PRIV __fill(__first, __last, __val,
                    _STLP_ITERATOR_CATEGORY(__first, _ForwardIter),
                    _STLP_DISTANCE_TYPE(__first, _ForwardIter));
}

// Specialization: for one-byte types we can use memset.
inline void fill(unsigned char* __first, unsigned char* __last,
                 const unsigned char& __val) {
  unsigned char __tmp = __val;
  memset(__first, __tmp, __last - __first);
}
#if !defined (_STLP_NO_SIGNED_BUILTINS)
inline void fill(signed char* __first, signed char* __last,
                 const signed char& __val) {
  signed char __tmp = __val;
  memset(__first, __STATIC_CAST(unsigned char,__tmp), __last - __first);
}
#endif
inline void fill(char* __first, char* __last, const char& __val) {
  char __tmp = __val;
  memset(__first, __STATIC_CAST(unsigned char,__tmp), __last - __first);
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _OutputIter, class _Size, class _Tp>
_STLP_INLINE_LOOP
_OutputIter __fill_n(_OutputIter __first, _Size __n, const _Tp& __val) {
  _STLP_FIX_LITERAL_BUG(__first)
  for ( ; __n > 0; --__n, ++__first)
    *__first = __val;
  return __first;
}

#if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
template <class _Size>
inline unsigned char* __fill_n(unsigned char* __first, _Size __n,
                               const unsigned char& __val) {
  _STLP_STD::fill(__first, __first + __n, __val);
  return __first + __n;
}
#if !defined (_STLP_NO_SIGNED_BUILTINS)
template <class _Size>
inline signed char* __fill_n(signed char* __first, _Size __n,
                             const signed char& __val) {
  _STLP_STD::fill(__first, __first + __n, __val);
  return __first + __n;
}
#endif
template <class _Size>
inline char* __fill_n(char* __first, _Size __n,
                      const char& __val) {
  _STLP_STD::fill(__first, __first + __n, __val);
  return __first + __n;
}
#endif

_STLP_MOVE_TO_STD_NAMESPACE

template <class _OutputIter, class _Size, class _Tp>
inline void fill_n(_OutputIter __first, _Size __n, const _Tp& __val) {
  _STLP_FIX_LITERAL_BUG(__first)
  _STLP_PRIV __fill_n(__first, __n, __val);
}


//--------------------------------------------------
// equal and mismatch

template <class _InputIter1, class _InputIter2>
_STLP_INLINE_LOOP
_STLP_STD::pair<_InputIter1, _InputIter2> mismatch(_InputIter1 __first1,
                                                   _InputIter1 __last1,
                                                   _InputIter2 __first2) {
  _STLP_FIX_LITERAL_BUG(__first2)
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  while (__first1 != __last1 && *__first1 == *__first2) {
    ++__first1;
    ++__first2;
  }
  return _STLP_STD::pair<_InputIter1, _InputIter2>(__first1, __first2);
}

template <class _InputIter1, class _InputIter2, class _BinaryPredicate>
_STLP_INLINE_LOOP
_STLP_STD::pair<_InputIter1, _InputIter2> mismatch(_InputIter1 __first1,
                                                   _InputIter1 __last1,
                                                   _InputIter2 __first2,
                                                   _BinaryPredicate __binary_pred) {
  _STLP_FIX_LITERAL_BUG(__first2)
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  while (__first1 != __last1 && __binary_pred(*__first1, *__first2)) {
    ++__first1;
    ++__first2;
  }
  return _STLP_STD::pair<_InputIter1, _InputIter2>(__first1, __first2);
}

template <class _InputIter1, class _InputIter2>
_STLP_INLINE_LOOP
bool equal(_InputIter1 __first1, _InputIter1 __last1,
           _InputIter2 __first2) {
  _STLP_FIX_LITERAL_BUG(__first1) _STLP_FIX_LITERAL_BUG(__last1)  _STLP_FIX_LITERAL_BUG(__first2)
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    if (!(*__first1 == *__first2))
      return false;
  return true;
}

template <class _InputIter1, class _InputIter2, class _BinaryPredicate>
_STLP_INLINE_LOOP
bool equal(_InputIter1 __first1, _InputIter1 __last1,
           _InputIter2 __first2, _BinaryPredicate __binary_pred) {
  _STLP_FIX_LITERAL_BUG(__first2)
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    if (!__binary_pred(*__first1, *__first2))
      return false;
  return true;
}

//--------------------------------------------------
// lexicographical_compare and lexicographical_compare_3way.
// (the latter is not part of the C++ standard.)

template <class _InputIter1, class _InputIter2>
bool lexicographical_compare(_InputIter1 __first1, _InputIter1 __last1,
                             _InputIter2 __first2, _InputIter2 __last2);

template <class _InputIter1, class _InputIter2, class _Compare>
bool lexicographical_compare(_InputIter1 __first1, _InputIter1 __last1,
                             _InputIter2 __first2, _InputIter2 __last2,
                             _Compare __comp);

inline bool
lexicographical_compare(const unsigned char* __first1,
                        const unsigned char* __last1,
                        const unsigned char* __first2,
                        const unsigned char* __last2) {
  const size_t __len1 = __last1 - __first1;
  const size_t __len2 = __last2 - __first2;
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))

  const int __result = memcmp(__first1, __first2, (min) (__len1, __len2));
  return __result != 0 ? (__result < 0) : (__len1 < __len2);
}


#if !(CHAR_MAX == SCHAR_MAX)
inline bool lexicographical_compare(const char* __first1, const char* __last1,
                                    const char* __first2, const char* __last2) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first2, __last2))

  return lexicographical_compare((const unsigned char*) __first1,
                                 (const unsigned char*) __last1,
                                 (const unsigned char*) __first2,
                                 (const unsigned char*) __last2);
}
#endif /* CHAR_MAX == SCHAR_MAX */

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter1, class _InputIter2>
int __lexicographical_compare_3way(_InputIter1 __first1, _InputIter1 __last1,
                                   _InputIter2 __first2, _InputIter2 __last2);

inline int
__lexicographical_compare_3way(const unsigned char* __first1,
                               const unsigned char* __last1,
                               const unsigned char* __first2,
                               const unsigned char* __last2) {
  const ptrdiff_t __len1 = __last1 - __first1;
  const ptrdiff_t __len2 = __last2 - __first2;
  const int __result = memcmp(__first1, __first2, (min) (__len1, __len2));
  return __result != 0 ? __result
                       : (__len1 == __len2 ? 0 : (__len1 < __len2 ? -1 : 1));
}


#if !(CHAR_MAX == SCHAR_MAX)
inline int
__lexicographical_compare_3way(const char* __first1, const char* __last1,
                               const char* __first2, const char* __last2) {
  return __lexicographical_compare_3way((const unsigned char*) __first1,
                                        (const unsigned char*) __last1,
                                        (const unsigned char*) __first2,
                                        (const unsigned char*) __last2);
}
#endif

_STLP_MOVE_TO_STD_NAMESPACE

#if !defined (_STLP_NO_EXTENSIONS)
template <class _InputIter1, class _InputIter2>
int lexicographical_compare_3way(_InputIter1 __first1, _InputIter1 __last1,
                                 _InputIter2 __first2, _InputIter2 __last2);

#endif

// count
template <class _InputIter, class _Tp>
_STLP_INLINE_LOOP _STLP_DIFFERENCE_TYPE(_InputIter)
count(_InputIter __first, _InputIter __last, const _Tp& __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  _STLP_DIFFERENCE_TYPE(_InputIter) __n = 0;
  for ( ; __first != __last; ++__first)
    if (*__first == __val)
      ++__n;
  return __n;
}

// find and find_if. Note find may be expressed in terms of find_if if appropriate binder was available.
template <class _InputIter, class _Tp>
_InputIter find(_InputIter __first, _InputIter __last, const _Tp& __val);

template <class _InputIter, class _Predicate>
_InputIter find_if(_InputIter __first, _InputIter __last, _Predicate __pred);

// search.
template <class _ForwardIter1, class _ForwardIter2, class _BinaryPred>
_ForwardIter1 search(_ForwardIter1 __first1, _ForwardIter1 __last1,
                     _ForwardIter2 __first2, _ForwardIter2 __last2, _BinaryPred  __predicate);

_STLP_MOVE_TO_PRIV_NAMESPACE

// find_first_of
template <class _InputIter, class _ForwardIter>
_InputIter __find_first_of(_InputIter __first1, _InputIter __last1,
                           _ForwardIter __first2, _ForwardIter __last2);

template <class _InputIter, class _ForwardIter, class _BinaryPredicate>
_InputIter __find_first_of(_InputIter __first1, _InputIter __last1,
                           _ForwardIter __first2, _ForwardIter __last2,
                           _BinaryPredicate __comp);

_STLP_MOVE_TO_STD_NAMESPACE

template <class _ForwardIter1, class _ForwardIter2,
          class _BinaryPredicate>
_ForwardIter1
find_end(_ForwardIter1 __first1, _ForwardIter1 __last1,
         _ForwardIter2 __first2, _ForwardIter2 __last2,
         _BinaryPredicate __comp);

// replace
template <class _ForwardIter, class _Tp>
_STLP_INLINE_LOOP void
replace(_ForwardIter __first, _ForwardIter __last,
        const _Tp& __old_value, const _Tp& __new_value) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first)
    if (*__first == __old_value)
      *__first = __new_value;
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _ForwardIter, class _Tp, class _Compare1, class _Compare2, class _Distance>
_ForwardIter __lower_bound(_ForwardIter __first, _ForwardIter __last,
                           const _Tp& __val, _Compare1 __comp1, _Compare2 __comp2, _Distance*);

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_algobase.c>
#endif

#endif /* _STLP_INTERNAL_ALGOBASE_H */

// Local Variables:
// mode:C++
// End:

