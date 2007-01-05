// Debugging support implementation -*- C++ -*-

// Copyright (C) 2003
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#ifndef _GLIBCXX_DEBUG_DEBUG_H
#define _GLIBCXX_DEBUG_DEBUG_H 1

/**
 * Macros used by the implementation to verify certain
 * properties. These macros may only be used directly by the debug
 * wrappers. Note that these are macros (instead of the more obviously
 * "correct" choice of making them functions) because we need line and
 * file information at the call site, to minimize the distance between
 * the user error and where the error is reported.
 *
 */
#define _GLIBCXX_DEBUG_VERIFY(_Condition,_ErrorMessage)		\
  do {									\
    if (! (_Condition))							\
      ::__gnu_debug::_Error_formatter::_M_at(__FILE__, __LINE__)	\
	  ._ErrorMessage._M_error();					\
  } while (false)

// Verify that [_First, _Last) forms a valid iterator range.
#define __glibcxx_check_valid_range(_First,_Last)			\
_GLIBCXX_DEBUG_VERIFY(::__gnu_debug::__valid_range(_First, _Last),	\
		      _M_message(::__gnu_debug::__msg_valid_range)	\
		      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last))

/** Verify that we can insert into *this with the iterator _Position.
 *  Insertion into a container at a specific position requires that
 *  the iterator be nonsingular (i.e., either dereferenceable or
 *  past-the-end) and that it reference the sequence we are inserting
 *  into. Note that this macro is only valid when the container is a
 *  _Safe_sequence and the iterator is a _Safe_iterator.
*/
#define __glibcxx_check_insert(_Position)				\
_GLIBCXX_DEBUG_VERIFY(!_Position._M_singular(),				\
		      _M_message(::__gnu_debug::__msg_insert_singular) \
		      ._M_sequence(*this, "this")			\
		      ._M_iterator(_Position, #_Position));		\
_GLIBCXX_DEBUG_VERIFY(_Position._M_attached_to(this),			\
		      _M_message(::__gnu_debug::__msg_insert_different) \
		      ._M_sequence(*this, "this")			\
		      ._M_iterator(_Position, #_Position))

/** Verify that we can insert the values in the iterator range
 *  [_First, _Last) into *this with the iterator _Position.  Insertion
 *  into a container at a specific position requires that the iterator
 *  be nonsingular (i.e., either dereferenceable or past-the-end),
 *  that it reference the sequence we are inserting into, and that the
 *  iterator range [_First, Last) is a valid (possibly empty)
 *  range. Note that this macro is only valid when the container is a
 *  _Safe_sequence and the iterator is a _Safe_iterator.
 *
 *  @tbd We would like to be able to check for noninterference of
 *  _Position and the range [_First, _Last), but that can't (in
 *  general) be done.
*/
#define __glibcxx_check_insert_range(_Position,_First,_Last)		\
__glibcxx_check_valid_range(_First,_Last);				\
_GLIBCXX_DEBUG_VERIFY(!_Position._M_singular(),				\
		      _M_message(::__gnu_debug::__msg_insert_singular) \
                      ._M_sequence(*this, "this")			\
		      ._M_iterator(_Position, #_Position));		\
_GLIBCXX_DEBUG_VERIFY(_Position._M_attached_to(this),			\
		      _M_message(::__gnu_debug::__msg_insert_different) \
		      ._M_sequence(*this, "this")			\
		      ._M_iterator(_Position, #_Position))

/** Verify that we can erase the element referenced by the iterator
 * _Position. We can erase the element if the _Position iterator is
 * dereferenceable and references this sequence.
*/
#define __glibcxx_check_erase(_Position)				\
_GLIBCXX_DEBUG_VERIFY(_Position._M_dereferenceable(),			\
		      _M_message(::__gnu_debug::__msg_erase_bad)	\
                      ._M_sequence(*this, "this")			\
		      ._M_iterator(_Position, #_Position));		\
_GLIBCXX_DEBUG_VERIFY(_Position._M_attached_to(this),			\
		      _M_message(::__gnu_debug::__msg_erase_different) \
		      ._M_sequence(*this, "this")			\
		      ._M_iterator(_Position, #_Position))

/** Verify that we can erase the elements in the iterator range
 *  [_First, _Last). We can erase the elements if [_First, _Last) is a
 *  valid iterator range within this sequence.
*/
#define __glibcxx_check_erase_range(_First,_Last)			\
__glibcxx_check_valid_range(_First,_Last);				\
_GLIBCXX_DEBUG_VERIFY(_First._M_attached_to(this),			\
		      _M_message(::__gnu_debug::__msg_erase_different) \
                      ._M_sequence(*this, "this")			\
		      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last))

// Verify that the subscript _N is less than the container's size.
#define __glibcxx_check_subscript(_N)					\
_GLIBCXX_DEBUG_VERIFY(_N < this->size(),				\
		      _M_message(::__gnu_debug::__msg_subscript_oob) \
                      ._M_sequence(*this, "this")			\
		      ._M_integer(_N, #_N)				\
		      ._M_integer(this->size(), "size"))

// Verify that the container is nonempty
#define __glibcxx_check_nonempty()					\
_GLIBCXX_DEBUG_VERIFY(! this->empty(),					\
		      _M_message(::__gnu_debug::__msg_empty)	\
                      ._M_sequence(*this, "this"))

// Verify that the < operator for elements in the sequence is a
// StrictWeakOrdering by checking that it is irreflexive.
#define __glibcxx_check_strict_weak_ordering(_First,_Last)	\
_GLIBCXX_DEBUG_ASSERT(_First == _Last || !(*_First < *_First))

// Verify that the predicate is StrictWeakOrdering by checking that it
// is irreflexive.
#define __glibcxx_check_strict_weak_ordering_pred(_First,_Last,_Pred)	\
_GLIBCXX_DEBUG_ASSERT(_First == _Last || !_Pred(*_First, *_First))


// Verify that the iterator range [_First, _Last) is sorted
#define __glibcxx_check_sorted(_First,_Last)				\
__glibcxx_check_valid_range(_First,_Last);				\
__glibcxx_check_strict_weak_ordering(_First,_Last);			\
_GLIBCXX_DEBUG_VERIFY(::__gnu_debug::__check_sorted(_First, _Last),	\
		      _M_message(::__gnu_debug::__msg_unsorted)	\
                      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last))

/** Verify that the iterator range [_First, _Last) is sorted by the
    predicate _Pred. */
#define __glibcxx_check_sorted_pred(_First,_Last,_Pred)			\
__glibcxx_check_valid_range(_First,_Last);				\
__glibcxx_check_strict_weak_ordering_pred(_First,_Last,_Pred);	        \
_GLIBCXX_DEBUG_VERIFY(::__gnu_debug::__check_sorted(_First, _Last, _Pred), \
		      _M_message(::__gnu_debug::__msg_unsorted_pred) \
                      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last)			\
		      ._M_string(#_Pred))

/** Verify that the iterator range [_First, _Last) is partitioned
    w.r.t. the value _Value. */
#define __glibcxx_check_partitioned(_First,_Last,_Value)		\
__glibcxx_check_valid_range(_First,_Last);				\
_GLIBCXX_DEBUG_VERIFY(::__gnu_debug::__check_partitioned(_First, _Last,	\
							 _Value),	\
		      _M_message(::__gnu_debug::__msg_unpartitioned) \
		      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last)			\
		      ._M_string(#_Value))

/** Verify that the iterator range [_First, _Last) is partitioned
    w.r.t. the value _Value and predicate _Pred. */
#define __glibcxx_check_partitioned_pred(_First,_Last,_Value,_Pred)	\
__glibcxx_check_valid_range(_First,_Last);				\
_GLIBCXX_DEBUG_VERIFY(::__gnu_debug::__check_partitioned(_First, _Last,	\
							 _Value, _Pred), \
		      _M_message(::__gnu_debug::__msg_unpartitioned_pred) \
		      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last)			\
		      ._M_string(#_Pred)				\
                      ._M_string(#_Value))

// Verify that the iterator range [_First, _Last) is a heap
#define __glibcxx_check_heap(_First,_Last)				\
__glibcxx_check_valid_range(_First,_Last);				\
_GLIBCXX_DEBUG_VERIFY(::std::__is_heap(_First, _Last),		\
		      _M_message(::__gnu_debug::__msg_not_heap)	\
		      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last))

/** Verify that the iterator range [_First, _Last) is a heap
    w.r.t. the predicate _Pred. */
#define __glibcxx_check_heap_pred(_First,_Last,_Pred)			\
__glibcxx_check_valid_range(_First,_Last);				\
_GLIBCXX_DEBUG_VERIFY(::std::__is_heap(_First, _Last, _Pred),		\
		      _M_message(::__gnu_debug::__msg_not_heap_pred) \
                      ._M_iterator(_First, #_First)			\
		      ._M_iterator(_Last, #_Last)			\
		      ._M_string(#_Pred))

#ifdef _GLIBCXX_DEBUG_PEDANTIC
#  define __glibcxx_check_string(_String) _GLIBCXX_DEBUG_ASSERT(_String != 0)
#  define __glibcxx_check_string_len(_String,_Len) \
       _GLIBCXX_DEBUG_ASSERT(_String != 0 || _Len == 0)
#else
#  define __glibcxx_check_string(_String)
#  define __glibcxx_check_string_len(_String,_Len)
#endif

/** Macros used by the implementation outside of debug wrappers to
 *  verify certain properties. The __glibcxx_requires_xxx macros are
 *  merely wrappers around the __glibcxx_check_xxx wrappers when we
 *  are compiling with debug mode, but disappear when we are in
 *  release mode so that there is no checking performed in, e.g., the
 *  standard library algorithms.
*/
#ifdef _GLIBCXX_DEBUG
#  define _GLIBCXX_DEBUG_ASSERT(_Condition) assert(_Condition)

#  ifdef _GLIBXX_DEBUG_PEDANTIC
#    define _GLIBCXX_DEBUG_PEDASSERT(_Condition) assert(_Condition)
#  else
#    define _GLIBCXX_DEBUG_PEDASSERT(_Condition)
#  endif

#  define __glibcxx_requires_cond(_Cond,_Msg) _GLIBCXX_DEBUG_VERIFY(_Cond,_Msg)
#  define __glibcxx_requires_valid_range(_First,_Last) \
     __glibcxx_check_valid_range(_First,_Last)
#  define __glibcxx_requires_sorted(_First,_Last) \
     __glibcxx_check_sorted(_First,_Last)
#  define __glibcxx_requires_sorted_pred(_First,_Last,_Pred) \
     __glibcxx_check_sorted_pred(_First,_Last,_Pred)
#  define __glibcxx_requires_partitioned(_First,_Last,_Value)	\
     __glibcxx_check_partitioned(_First,_Last,_Value)
#  define __glibcxx_requires_partitioned_pred(_First,_Last,_Value,_Pred) \
     __glibcxx_check_partitioned_pred(_First,_Last,_Value,_Pred)
#  define __glibcxx_requires_heap(_First,_Last) \
     __glibcxx_check_heap(_First,_Last)
#  define __glibcxx_requires_heap_pred(_First,_Last,_Pred) \
     __glibcxx_check_heap_pred(_First,_Last,_Pred)
#  define __glibcxx_requires_nonempty() __glibcxx_check_nonempty()
#  define __glibcxx_requires_string(_String) __glibcxx_check_string(_String)
#  define __glibcxx_requires_string_len(_String,_Len)	\
     __glibcxx_check_string_len(_String,_Len)
#  define __glibcxx_requires_subscript(_N) __glibcxx_check_subscript(_N)
#else
#  define _GLIBCXX_DEBUG_ASSERT(_Condition)
#  define _GLIBCXX_DEBUG_PEDASSERT(_Condition)
#  define __glibcxx_requires_cond(_Cond,_Msg)
#  define __glibcxx_requires_valid_range(_First,_Last)
#  define __glibcxx_requires_sorted(_First,_Last)
#  define __glibcxx_requires_sorted_pred(_First,_Last,_Pred)
#  define __glibcxx_requires_partitioned(_First,_Last,_Value)
#  define __glibcxx_requires_partitioned_pred(_First,_Last,_Value,_Pred)
#  define __glibcxx_requires_heap(_First,_Last)
#  define __glibcxx_requires_heap_pred(_First,_Last,_Pred)
#  define __glibcxx_requires_nonempty()
#  define __glibcxx_requires_string(_String)
#  define __glibcxx_requires_string_len(_String,_Len)
#  define __glibcxx_requires_subscript(_N)
#endif

#include <cassert> // TBD: temporary

#include <stddef.h>                       // for ptrdiff_t
#include <bits/stl_iterator_base_types.h> // for iterator_traits, categories
#include <bits/type_traits.h>             // for _Is_integer

namespace __gnu_debug
{
  template<typename _Iterator, typename _Sequence>
    class _Safe_iterator;

  // An arbitrary iterator pointer is not singular.
  inline bool
  __check_singular_aux(const void*) { return false; }

  // We may have an iterator that derives from _Safe_iterator_base but isn't
  // a _Safe_iterator.
  template<typename _Iterator>
    inline bool
    __check_singular(_Iterator& __x)
    { return __gnu_debug::__check_singular_aux(&__x); }

  /** Non-NULL pointers are nonsingular. */
  template<typename _Tp>
    inline bool
    __check_singular(const _Tp* __ptr)
    { return __ptr == 0; }

  /** Safe iterators know if they are singular. */
  template<typename _Iterator, typename _Sequence>
    inline bool
    __check_singular(const _Safe_iterator<_Iterator, _Sequence>& __x)
    { return __x._M_singular(); }

  /** Assume that some arbitrary iterator is dereferenceable, because we
      can't prove that it isn't. */
  template<typename _Iterator>
    inline bool
    __check_dereferenceable(_Iterator&)
    { return true; }

  /** Non-NULL pointers are dereferenceable. */
  template<typename _Tp>
    inline bool
    __check_dereferenceable(const _Tp* __ptr)
    { return __ptr; }

  /** Safe iterators know if they are singular. */
  template<typename _Iterator, typename _Sequence>
    inline bool
    __check_dereferenceable(const _Safe_iterator<_Iterator, _Sequence>& __x)
    { return __x._M_dereferenceable(); }

  /** If the distance between two random access iterators is
   *  nonnegative, assume the range is valid.
  */
  template<typename _RandomAccessIterator>
    inline bool
    __valid_range_aux2(const _RandomAccessIterator& __first,
		       const _RandomAccessIterator& __last,
		       std::random_access_iterator_tag)
    { return __last - __first >= 0; }

  /** Can't test for a valid range with input iterators, because
   *  iteration may be destructive. So we just assume that the range
   *  is valid.
  */
  template<typename _InputIterator>
    inline bool
    __valid_range_aux2(const _InputIterator&, const _InputIterator&,
		       std::input_iterator_tag)
    { return true; }

  /** We say that integral types for a valid range, and defer to other
   *  routines to realize what to do with integral types instead of
   *  iterators.
  */
  template<typename _Integral>
    inline bool
    __valid_range_aux(const _Integral&, const _Integral&, __true_type)
    { return true; }

  /** We have iterators, so figure out what kind of iterators that are
   *  to see if we can check the range ahead of time.
  */
  template<typename _InputIterator>
    inline bool
    __valid_range_aux(const _InputIterator& __first,
		      const _InputIterator& __last, __false_type)
  {
    typedef typename std::iterator_traits<_InputIterator>::iterator_category
      _Category;
    return __gnu_debug::__valid_range_aux2(__first, __last, _Category());
  }

  /** Don't know what these iterators are, or if they are even
   *  iterators (we may get an integral type for InputIterator), so
   *  see if they are integral and pass them on to the next phase
   *  otherwise.
  */
  template<typename _InputIterator>
    inline bool
    __valid_range(const _InputIterator& __first, const _InputIterator& __last)
    {
      typedef typename _Is_integer<_InputIterator>::_Integral _Integral;
      return __gnu_debug::__valid_range_aux(__first, __last, _Integral());
    }

  /** Safe iterators know how to check if they form a valid range. */
  template<typename _Iterator, typename _Sequence>
    inline bool
    __valid_range(const _Safe_iterator<_Iterator, _Sequence>& __first,
		  const _Safe_iterator<_Iterator, _Sequence>& __last)
    { return __first._M_valid_range(__last); }

  /* Checks that [first, last) is a valid range, and then returns
   * __first. This routine is useful when we can't use a separate
   * assertion statement because, e.g., we are in a constructor.
  */
  template<typename _InputIterator>
    inline _InputIterator
    __check_valid_range(const _InputIterator& __first,
			const _InputIterator& __last)
    {
      _GLIBCXX_DEBUG_ASSERT(__gnu_debug::__valid_range(__first, __last));
      return __first;
    }

  /** Checks that __s is non-NULL or __n == 0, and then returns __s. */
  template<typename _CharT, typename _Integer>
    inline const _CharT*
    __check_string(const _CharT* __s, const _Integer& __n)
    {
#ifdef _GLIBCXX_DEBUG_PEDANTIC
      _GLIBCXX_DEBUG_ASSERT(__s != 0 || __n == 0);
#endif
      return __s;
    }

  /** Checks that __s is non-NULL and then returns __s. */
  template<typename _CharT>
    inline const _CharT*
    __check_string(const _CharT* __s)
    {
#ifdef _GLIBCXX_DEBUG_PEDANTIC
      _GLIBCXX_DEBUG_ASSERT(__s != 0);
#endif
      return __s;
    }

  // Can't check if an input iterator sequence is sorted, because we
  // can't step through the sequence.
  template<typename _InputIterator>
    inline bool
    __check_sorted_aux(const _InputIterator&, const _InputIterator&,
                       std::input_iterator_tag)
    { return true; }

  // Can verify if a forward iterator sequence is in fact sorted using
  // std::__is_sorted
  template<typename _ForwardIterator>
    inline bool
    __check_sorted_aux(_ForwardIterator __first, _ForwardIterator __last,
                       std::forward_iterator_tag)
    {
      if (__first == __last)
        return true;

      _ForwardIterator __next = __first;
      for (++__next; __next != __last; __first = __next, ++__next) {
        if (*__next < *__first)
          return false;
      }

      return true;
    }

  // Can't check if an input iterator sequence is sorted, because we can't step
  // through the sequence.
  template<typename _InputIterator, typename _Predicate>
    inline bool
    __check_sorted_aux(const _InputIterator&, const _InputIterator&,
                       _Predicate, std::input_iterator_tag)
    { return true; }

  // Can verify if a forward iterator sequence is in fact sorted using
  // std::__is_sorted
  template<typename _ForwardIterator, typename _Predicate>
    inline bool
    __check_sorted_aux(_ForwardIterator __first, _ForwardIterator __last,
                       _Predicate __pred, std::forward_iterator_tag)
    {
      if (__first == __last)
        return true;

      _ForwardIterator __next = __first;
      for (++__next; __next != __last; __first = __next, ++__next) {
        if (__pred(*__next, *__first))
          return false;
      }

      return true;
    }

  // Determine if a sequence is sorted.
  template<typename _InputIterator>
    inline bool
    __check_sorted(const _InputIterator& __first, const _InputIterator& __last)
    {
      typedef typename std::iterator_traits<_InputIterator>::iterator_category
        _Category;
      return __gnu_debug::__check_sorted_aux(__first, __last, _Category());
    }

  template<typename _InputIterator, typename _Predicate>
    inline bool
    __check_sorted(const _InputIterator& __first, const _InputIterator& __last,
                   _Predicate __pred)
    {
      typedef typename std::iterator_traits<_InputIterator>::iterator_category
        _Category;
      return __gnu_debug::__check_sorted_aux(__first, __last, __pred,
					     _Category());
    }

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 270. Binary search requirements overly strict
  // Determine if a sequence is partitioned w.r.t. this element.
  template<typename _ForwardIterator, typename _Tp>
    inline bool
    __check_partitioned(_ForwardIterator __first, _ForwardIterator __last,
			const _Tp& __value)
    {
      while (__first != __last && *__first < __value)
	++__first;
      while (__first != __last && !(*__first < __value))
	++__first;
      return __first == __last;
    }

  // Determine if a sequence is partitioned w.r.t. this element.
  template<typename _ForwardIterator, typename _Tp, typename _Pred>
    inline bool
    __check_partitioned(_ForwardIterator __first, _ForwardIterator __last,
			const _Tp& __value, _Pred __pred)
    {
      while (__first != __last && __pred(*__first, __value))
	++__first;
      while (__first != __last && !__pred(*__first, __value))
	++__first;
      return __first == __last;
    }
} // namespace __gnu_debug

#ifdef _GLIBCXX_DEBUG
// We need the error formatter
#  include <debug/formatter.h>
#endif

#endif
