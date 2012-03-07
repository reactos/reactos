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
#ifndef _STLP_VALARRAY_C
#define _STLP_VALARRAY_C

#ifndef _STLP_VALARRAY_H
# include <stl/_valarray.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _Tp>
_Valarray_bool valarray<_Tp>:: operator!() const {
  _Valarray_bool __tmp(this->size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < this->size(); ++__i)
    __tmp[__i] = !(*this)[__i];
  return __tmp;
}

// Behavior is undefined if __x and *this have different sizes
template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const slice_array<_Tp>& __x) {
  _STLP_ASSERT(__x._M_slice.size() == this->size())
  size_t __index = __x._M_slice.start();
  for (size_t __i = 0;
       __i < __x._M_slice.size();
       ++__i, __index += __x._M_slice.stride())
    (*this)[__i] = __x._M_array[__index];
  return *this;
}

template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](slice __slice) const {
  valarray<_Tp> __tmp(__slice.size(), _NoInit());
  size_t __index = __slice.start();
  for (size_t __i = 0;
       __i < __slice.size();
       ++__i, __index += __slice.stride())
    __tmp[__i] = (*this)[__index];
  return __tmp;
}

template <class _Size>
bool _Gslice_Iter_tmpl<_Size>::_M_incr() {
  size_t __dim = _M_indices.size() - 1;
  ++_M_step;
  for (;;) {
    _M_1d_idx += _M_gslice._M_strides[__dim];
    if (++_M_indices[__dim] != _M_gslice._M_lengths[__dim])
      return true;
    else if (__dim != 0) {
      _M_1d_idx -= _M_gslice._M_strides[__dim] * _M_gslice._M_lengths[__dim];
      _M_indices[__dim] = 0;
      --__dim;
    }
    else
      return false;
  }
}

// Behavior is undefined if __x and *this have different sizes, or if
// __x was constructed from a degenerate gslice.
template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const gslice_array<_Tp>& __x) {
  if (this->size() != 0) {
    _Gslice_Iter __i(__x._M_gslice);
    do
      (*this)[__i._M_step] = __x._M_array[__i._M_1d_idx];
    while(__i._M_incr());
  }
  return *this;
}

template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](const gslice& __slice) const {
  valarray<_Tp> __tmp(__slice._M_size(), _NoInit());
  if (__tmp.size() != 0) {
    _Gslice_Iter __i(__slice);
    do __tmp[__i._M_step] = (*this)[__i._M_1d_idx]; while(__i._M_incr());
  }
  return __tmp;
}

template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](const _Valarray_bool& __mask) const {
  size_t _p_size = 0;
  {
    for (size_t __i = 0; __i < __mask.size(); ++__i)
      if (__mask[__i]) ++_p_size;
  }

  valarray<_Tp> __tmp(_p_size, _NoInit());
  size_t __idx = 0;
  {
    for (size_t __i = 0; __i < __mask.size(); ++__i)
      if (__mask[__i]) __tmp[__idx++] = (*this)[__i];
  }

  return __tmp;
}

template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const indirect_array<_Tp>& __x) {
  _STLP_ASSERT(__x._M_addr.size() == this->size())
  for (size_t __i = 0; __i < __x._M_addr.size(); ++__i)
    (*this)[__i] = __x._M_array[__x._M_addr[__i]];
  return *this;
}

template <class _Tp>
valarray<_Tp>
valarray<_Tp>::operator[](const _Valarray_size_t& __addr) const {
  valarray<_Tp> __tmp(__addr.size(), _NoInit());
  for (size_t __i = 0; __i < __addr.size(); ++__i)
    __tmp[__i] = (*this)[__addr[__i]];
  return __tmp;
}

//----------------------------------------------------------------------
// Other valarray noninline member functions

// Shift and cshift

template <class _Tp>
valarray<_Tp> valarray<_Tp>::shift(int __n) const {
  valarray<_Tp> __tmp(this->size());

  if (__n >= 0) {
    if (__n < this->size())
      copy(this->_M_first + __n, this->_M_first + this->size(),
           __tmp._M_first);
  }
  else {
    if (-__n < this->size())
      copy(this->_M_first, this->_M_first + this->size() + __n,
           __tmp._M_first - __n);
  }
  return __tmp;
}

template <class _Tp>
valarray<_Tp> valarray<_Tp>::cshift(int __m) const {
  valarray<_Tp> __tmp(this->size());

  // Reduce __m to an equivalent number in the range [0, size()).  We
  // have to be careful with negative numbers, since the sign of a % b
  // is unspecified when a < 0.
  long __n = __m;
  if (this->size() < (numeric_limits<long>::max)())
    __n %= long(this->size());
  if (__n < 0)
    __n += this->size();

  copy(this->_M_first,       this->_M_first + __n,
       __tmp._M_first + (this->size() - __n));
  copy(this->_M_first + __n, this->_M_first + this->size(),
       __tmp._M_first);

  return __tmp;
}

_STLP_END_NAMESPACE

#endif /*  _STLP_VALARRAY_C */

// Local Variables:
// mode:C++
// End:
