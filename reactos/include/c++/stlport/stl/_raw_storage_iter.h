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
 * You should not attempt to use it directly.
 */

#ifndef _STLP_INTERNAL_RAW_STORAGE_ITERATOR_H
#define _STLP_INTERNAL_RAW_STORAGE_ITERATOR_H

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#  include <stl/_iterator_base.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _ForwardIterator, class _Tp>
class raw_storage_iterator
      : public iterator<output_iterator_tag,void,void,void,void>
{
protected:
  _ForwardIterator _M_iter;
public:
  typedef output_iterator_tag iterator_category;
# ifdef _STLP_CLASS_PARTIAL_SPECIALIZATION
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;
# endif
  explicit raw_storage_iterator(_ForwardIterator __x) : _M_iter(__x) {}
  raw_storage_iterator<_ForwardIterator, _Tp>& operator*() { return *this; }
  raw_storage_iterator<_ForwardIterator, _Tp>& operator=(const _Tp& __element) {
    _Param_Construct(&*_M_iter, __element);
    return *this;
  }
  raw_storage_iterator<_ForwardIterator, _Tp>& operator++() {
    ++_M_iter;
    return *this;
  }
  raw_storage_iterator<_ForwardIterator, _Tp> operator++(int) {
    raw_storage_iterator<_ForwardIterator, _Tp> __tmp = *this;
    ++_M_iter;
    return __tmp;
  }
};

# ifdef _STLP_USE_OLD_HP_ITERATOR_QUERIES
template <class _ForwardIterator, class _Tp>
inline output_iterator_tag iterator_category(const raw_storage_iterator<_ForwardIterator, _Tp>&) { return output_iterator_tag(); }
#endif
_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_RAW_STORAGE_ITERATOR_H */

// Local Variables:
// mode:C++
// End:
