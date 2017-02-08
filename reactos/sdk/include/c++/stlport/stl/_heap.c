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
#ifndef _STLP_HEAP_C
#define _STLP_HEAP_C

#ifndef _STLP_INTERNAL_HEAP_H
# include <stl/_heap.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
# include <stl/_iterator_base.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _RandomAccessIterator, class _Distance, class _Tp>
_STLP_INLINE_LOOP
void
__push_heap(_RandomAccessIterator __first,
            _Distance __holeIndex, _Distance __topIndex, _Tp __val)
{
  _Distance __parent = (__holeIndex - 1) / 2;
  while (__holeIndex > __topIndex && *(__first + __parent) < __val) {
    *(__first + __holeIndex) = *(__first + __parent);
    __holeIndex = __parent;
    __parent = (__holeIndex - 1) / 2;
  }
  *(__first + __holeIndex) = __val;
}

template <class _RandomAccessIterator, class _Distance, class _Tp>
inline void
__push_heap_aux(_RandomAccessIterator __first,
                _RandomAccessIterator __last, _Distance*, _Tp*)
{
  __push_heap(__first, _Distance((__last - __first) - 1), _Distance(0),
              _Tp(*(__last - 1)));
}

template <class _RandomAccessIterator>
void
push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
{
  __push_heap_aux(__first, __last,
                  _STLP_DISTANCE_TYPE(__first, _RandomAccessIterator), _STLP_VALUE_TYPE(__first, _RandomAccessIterator));
}


template <class _RandomAccessIterator, class _Distance, class _Tp,
          class _Compare>
_STLP_INLINE_LOOP
void
__push_heap(_RandomAccessIterator __first, _Distance __holeIndex,
            _Distance __topIndex, _Tp __val, _Compare __comp)
{
  _Distance __parent = (__holeIndex - 1) / 2;
  while (__holeIndex > __topIndex && __comp(*(__first + __parent), __val)) {
    _STLP_VERBOSE_ASSERT(!__comp(__val, *(__first + __parent)), _StlMsg_INVALID_STRICT_WEAK_PREDICATE)
    *(__first + __holeIndex) = *(__first + __parent);
    __holeIndex = __parent;
    __parent = (__holeIndex - 1) / 2;
  }
  *(__first + __holeIndex) = __val;
}

template <class _RandomAccessIterator, class _Compare,
          class _Distance, class _Tp>
inline void
__push_heap_aux(_RandomAccessIterator __first,
                _RandomAccessIterator __last, _Compare __comp,
                _Distance*, _Tp*)
{
  __push_heap(__first, _Distance((__last - __first) - 1), _Distance(0),
              _Tp(*(__last - 1)), __comp);
}

template <class _RandomAccessIterator, class _Compare>
void
push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
          _Compare __comp)
{
  __push_heap_aux(__first, __last, __comp,
                  _STLP_DISTANCE_TYPE(__first, _RandomAccessIterator), _STLP_VALUE_TYPE(__first, _RandomAccessIterator));
}

template <class _RandomAccessIterator, class _Distance, class _Tp>
void
__adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex,
              _Distance __len, _Tp __val) {
  _Distance __topIndex = __holeIndex;
  _Distance __secondChild = 2 * __holeIndex + 2;
  while (__secondChild < __len) {
    if (*(__first + __secondChild) < *(__first + (__secondChild - 1)))
      __secondChild--;
    *(__first + __holeIndex) = *(__first + __secondChild);
    __holeIndex = __secondChild;
    __secondChild = 2 * (__secondChild + 1);
  }
  if (__secondChild == __len) {
    *(__first + __holeIndex) = *(__first + (__secondChild - 1));
    __holeIndex = __secondChild - 1;
  }
  __push_heap(__first, __holeIndex, __topIndex, __val);
}


template <class _RandomAccessIterator, class _Tp>
inline void
__pop_heap_aux(_RandomAccessIterator __first, _RandomAccessIterator __last, _Tp*) {
  __pop_heap(__first, __last - 1, __last - 1,
             _Tp(*(__last - 1)), _STLP_DISTANCE_TYPE(__first, _RandomAccessIterator));
}

template <class _RandomAccessIterator>
void pop_heap(_RandomAccessIterator __first,
        _RandomAccessIterator __last) {
  __pop_heap_aux(__first, __last, _STLP_VALUE_TYPE(__first, _RandomAccessIterator));
}

template <class _RandomAccessIterator, class _Distance,
          class _Tp, class _Compare>
void
__adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex,
              _Distance __len, _Tp __val, _Compare __comp)
{
  _Distance __topIndex = __holeIndex;
  _Distance __secondChild = 2 * __holeIndex + 2;
  while (__secondChild < __len) {
    if (__comp(*(__first + __secondChild), *(__first + (__secondChild - 1)))) {
      _STLP_VERBOSE_ASSERT(!__comp(*(__first + (__secondChild - 1)), *(__first + __secondChild)),
                           _StlMsg_INVALID_STRICT_WEAK_PREDICATE)
      __secondChild--;
    }
    *(__first + __holeIndex) = *(__first + __secondChild);
    __holeIndex = __secondChild;
    __secondChild = 2 * (__secondChild + 1);
  }
  if (__secondChild == __len) {
    *(__first + __holeIndex) = *(__first + (__secondChild - 1));
    __holeIndex = __secondChild - 1;
  }
  __push_heap(__first, __holeIndex, __topIndex, __val, __comp);
}


template <class _RandomAccessIterator, class _Tp, class _Compare>
inline void
__pop_heap_aux(_RandomAccessIterator __first,
               _RandomAccessIterator __last, _Tp*, _Compare __comp)
{
  __pop_heap(__first, __last - 1, __last - 1, _Tp(*(__last - 1)), __comp,
             _STLP_DISTANCE_TYPE(__first, _RandomAccessIterator));
}


template <class _RandomAccessIterator, class _Compare>
void
pop_heap(_RandomAccessIterator __first,
         _RandomAccessIterator __last, _Compare __comp)
{
    __pop_heap_aux(__first, __last, _STLP_VALUE_TYPE(__first, _RandomAccessIterator), __comp);
}

template <class _RandomAccessIterator, class _Tp, class _Distance>
_STLP_INLINE_LOOP
void
__make_heap(_RandomAccessIterator __first,
            _RandomAccessIterator __last, _Tp*, _Distance*)
{
  if (__last - __first < 2) return;
  _Distance __len = __last - __first;
  _Distance __parent = (__len - 2)/2;

  for (;;) {
    __adjust_heap(__first, __parent, __len, _Tp(*(__first + __parent)));
    if (__parent == 0) return;
    __parent--;
  }
}

template <class _RandomAccessIterator>
void
make_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
{
  __make_heap(__first, __last,
              _STLP_VALUE_TYPE(__first, _RandomAccessIterator), _STLP_DISTANCE_TYPE(__first, _RandomAccessIterator));
}

template <class _RandomAccessIterator, class _Compare,
          class _Tp, class _Distance>
_STLP_INLINE_LOOP
void
__make_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
            _Compare __comp, _Tp*, _Distance*)
{
  if (__last - __first < 2) return;
  _Distance __len = __last - __first;
  _Distance __parent = (__len - 2)/2;

  for (;;) {
    __adjust_heap(__first, __parent, __len, _Tp(*(__first + __parent)),
                  __comp);
    if (__parent == 0) return;
    __parent--;
  }
}

template <class _RandomAccessIterator, class _Compare>
void
make_heap(_RandomAccessIterator __first,
          _RandomAccessIterator __last, _Compare __comp)
{
  __make_heap(__first, __last, __comp,
              _STLP_VALUE_TYPE(__first, _RandomAccessIterator), _STLP_DISTANCE_TYPE(__first, _RandomAccessIterator));
}

_STLP_END_NAMESPACE

#endif /*  _STLP_HEAP_C */

// Local Variables:
// mode:C++
// End:
