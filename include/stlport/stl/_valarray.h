/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
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

#ifndef _STLP_VALARRAY_H
#define _STLP_VALARRAY_H

#ifndef _STLP_INTERNAL_CMATH
#  include <stl/_cmath.h>
#endif

#ifndef _STLP_INTERNAL_NEW
#  include <stl/_new.h>
#endif

#ifndef _STLP_INTERNAL_ALGO_H
#  include <stl/_algo.h>
#endif

#ifndef _STLP_INTERNAL_NUMERIC_H
#  include <stl/_numeric.h>
#endif

#ifndef _STLP_INTERNAL_LIMITS
#  include <stl/_limits.h>
#endif

_STLP_BEGIN_NAMESPACE

class slice;
class gslice;

template <class _Tp> class valarray;
typedef valarray<bool>    _Valarray_bool;
typedef valarray<size_t>  _Valarray_size_t;

template <class _Tp> class slice_array;
template <class _Tp> class gslice_array;
template <class _Tp> class mask_array;
template <class _Tp> class indirect_array;

//----------------------------------------------------------------------
// class valarray

// Base class to handle memory allocation and deallocation.  We can't just
// use vector<>, because vector<bool> would be unsuitable as an internal
// representation for valarray<bool>.

template <class _Tp>
struct _Valarray_base {
  _Tp*   _M_first;
  size_t _M_size;

  _Valarray_base() : _M_first(0), _M_size(0) {}
  _Valarray_base(size_t __n) : _M_first(0), _M_size(0) { _M_allocate(__n); }
  ~_Valarray_base() { _M_deallocate(); }

  void _M_allocate(size_t __n) {
    if (__n != 0) {
      _M_first = __STATIC_CAST(_Tp*, __stl_new(__n * sizeof(_Tp)));
      _M_size  = __n;
    }
    else {
      _M_first = 0;
      _M_size = 0;
    }
  }

  void _M_deallocate() {
    __stl_delete(_M_first);
    _M_first = 0;
    _M_size = 0;
  }
};

template <class _Tp>
class valarray : private _Valarray_base<_Tp>
{
  friend class gslice;

public:
  typedef _Tp value_type;

  // Basic constructors
  valarray() : _Valarray_base<_Tp>() {}
  explicit valarray(size_t __n) : _Valarray_base<_Tp>(__n)
    { uninitialized_fill_n(this->_M_first, this->_M_size, _STLP_DEFAULT_CONSTRUCTED(value_type)); }
  valarray(const value_type& __x, size_t __n) : _Valarray_base<_Tp>(__n)
    { uninitialized_fill_n(this->_M_first, this->_M_size, __x); }
  valarray(const value_type* __p, size_t __n) : _Valarray_base<_Tp>(__n)
    { uninitialized_copy(__p, __p + __n, this->_M_first); }
  valarray(const valarray<_Tp>& __x) : _Valarray_base<_Tp>(__x._M_size) {
    uninitialized_copy(__x._M_first, __x._M_first + __x._M_size,
                       this->_M_first);
  }

  // Constructors from auxiliary array types
  valarray(const slice_array<_Tp>&);
  valarray(const gslice_array<_Tp>&);
  valarray(const mask_array<_Tp>&);
  valarray(const indirect_array<_Tp>&);

  // Destructor
  ~valarray() { _STLP_STD::_Destroy_Range(this->_M_first, this->_M_first + this->_M_size); }

  // Extension: constructor that doesn't initialize valarray elements to a
  // specific value.  This is faster for types such as int and double.
private:
  void _M_initialize(const __true_type&) {}
  void _M_initialize(const __false_type&)
    { uninitialized_fill_n(this->_M_first, this->_M_size, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }

public:
  struct _NoInit {};
  valarray(size_t __n, _NoInit) : _Valarray_base<_Tp>(__n) {
    typedef typename __type_traits<_Tp>::has_trivial_default_constructor _Is_Trivial;
    _M_initialize(_Is_Trivial());
  }

public:                         // Assignment
  // Basic assignment.  Note that 'x = y' is undefined if x.size() != y.size()
  valarray<_Tp>& operator=(const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    if (this != &__x)
      copy(__x._M_first, __x._M_first + __x._M_size, this->_M_first);
    return *this;
  }

  // Scalar assignment
  valarray<_Tp>& operator=(const value_type& __x) {
    fill_n(this->_M_first, this->_M_size, __x);
    return *this;
  }

  // Assignment of auxiliary array types
  valarray<_Tp>& operator=(const slice_array<_Tp>&);
  valarray<_Tp>& operator=(const gslice_array<_Tp>&);
  valarray<_Tp>& operator=(const mask_array<_Tp>&);
  valarray<_Tp>& operator=(const indirect_array<_Tp>&);

public:                         // Element access
  value_type  operator[](size_t __n) const {
    _STLP_ASSERT(__n < this->size())
    return this->_M_first[__n];
  }
  value_type& operator[](size_t __n) {
    _STLP_ASSERT(__n < this->size())
    return this->_M_first[__n];
  }
  size_t size() const { return this->_M_size; }

public:                         // Subsetting operations with auxiliary type
  valarray<_Tp>       operator[](slice) const;
  slice_array<_Tp>    operator[](slice);
  valarray<_Tp>       operator[](const gslice&) const;
  gslice_array<_Tp>   operator[](const gslice&);
  valarray<_Tp>       operator[](const _Valarray_bool&) const;
  mask_array<_Tp>     operator[](const _Valarray_bool&);
  valarray<_Tp>       operator[](const _Valarray_size_t&) const;
  indirect_array<_Tp> operator[](const _Valarray_size_t&);

public:                         // Unary operators.
  valarray<_Tp> operator+() const { return *this; }

  valarray<_Tp> operator-() const {
    valarray<_Tp> __tmp(this->size(), _NoInit());
    for (size_t __i = 0; __i < this->size(); ++__i)
      __tmp[__i] = -(*this)[__i];
    return __tmp;
  }

  valarray<_Tp> operator~() const {
    valarray<_Tp> __tmp(this->size(), _NoInit());
    for (size_t __i = 0; __i < this->size(); ++__i)
      __tmp[__i] = ~(*this)[__i];
    return __tmp;
  }

  _Valarray_bool operator!() const;

public:                         // Scalar computed assignment.
  valarray<_Tp>& operator*= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] *= __x;
    return *this;
  }

  valarray<_Tp>& operator/= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] /= __x;
    return *this;
  }

  valarray<_Tp>& operator%= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] %= __x;
    return *this;
  }

  valarray<_Tp>& operator+= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] += __x;
    return *this;
  }

  valarray<_Tp>& operator-= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] -= __x;
    return *this;
  }

  valarray<_Tp>& operator^= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] ^= __x;
    return *this;
  }

  valarray<_Tp>& operator&= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] &= __x;
    return *this;
  }

  valarray<_Tp>& operator|= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] |= __x;
    return *this;
  }

  valarray<_Tp>& operator<<= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] <<= __x;
    return *this;
  }

  valarray<_Tp>& operator>>= (const value_type& __x) {
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] >>= __x;
    return *this;
  }

public:                         // Array computed assignment.
  valarray<_Tp>& operator*= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] *= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator/= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] /= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator%= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] %= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator+= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] += __x[__i];
    return *this;
  }

  valarray<_Tp>& operator-= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] -= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator^= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] ^= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator&= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] &= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator|= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] |= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator<<= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] <<= __x[__i];
    return *this;
  }

  valarray<_Tp>& operator>>= (const valarray<_Tp>& __x) {
    _STLP_ASSERT(__x.size() == this->size())
    for (size_t __i = 0; __i < this->size(); ++__i)
      (*this)[__i] >>= __x[__i];
    return *this;
  }

public:                         // Other member functions.

  // The result is undefined for zero-length arrays
  value_type sum() const {
    _STLP_ASSERT(this->size() != 0)
    return accumulate(this->_M_first + 1, this->_M_first + this->_M_size,
                      (*this)[0]);
  }

  // The result is undefined for zero-length arrays
  value_type (min) () const {
    _STLP_ASSERT(this->size() != 0)
    return *min_element(this->_M_first + 0, this->_M_first + this->_M_size);
  }

  value_type (max) () const {
    _STLP_ASSERT(this->size() != 0)
    return *max_element(this->_M_first + 0, this->_M_first + this->_M_size);
  }

  valarray<_Tp> shift(int __n) const;
  valarray<_Tp> cshift(int __n) const;

  valarray<_Tp> apply(value_type __f(value_type)) const {
    valarray<_Tp> __tmp(this->size());
    transform(this->_M_first + 0, this->_M_first + this->_M_size, __tmp._M_first,
              __f);
    return __tmp;
  }
  valarray<_Tp> apply(value_type __f(const value_type&)) const {
    valarray<_Tp> __tmp(this->size());
    transform(this->_M_first + 0, this->_M_first + this->_M_size, __tmp._M_first,
              __f);
    return __tmp;
  }

  void resize(size_t __n, value_type __x = value_type()) {
    _STLP_STD::_Destroy_Range(this->_M_first, this->_M_first + this->_M_size);
    _Valarray_base<_Tp>::_M_deallocate();
    _Valarray_base<_Tp>::_M_allocate(__n);
    uninitialized_fill_n(this->_M_first, this->_M_size, __x);
  }
};

//----------------------------------------------------------------------
// valarray non-member functions.

// Binary arithmetic operations between two arrays.  Behavior is
// undefined if the two arrays do not have the same length.

template <class _Tp>
inline valarray<_Tp>  _STLP_CALL operator*(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] * __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp>  _STLP_CALL operator/(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] / __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp>  _STLP_CALL operator%(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] % __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp>  _STLP_CALL operator+(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] + __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp>  _STLP_CALL operator-(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] - __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator^(const valarray<_Tp>& __x,
                                          const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] ^ __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator&(const valarray<_Tp>& __x,
                                          const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] & __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator|(const valarray<_Tp>& __x,
                                          const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] | __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator<<(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] << __y[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator>>(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] >> __y[__i];
  return __tmp;
}

// Binary arithmetic operations between an array and a scalar.

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator*(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  * __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator*(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c * __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator/(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  / __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator/(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c / __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator%(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  % __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator%(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c % __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator+(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  + __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator+(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c + __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator-(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  - __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator-(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c - __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator^(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  ^ __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator^(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c ^ __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator&(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  & __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator&(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c & __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator|(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  | __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator|(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c | __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator<<(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  << __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator<<(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c << __x[__i];
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator>>(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  >> __c;
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> _STLP_CALL operator>>(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c >> __x[__i];
  return __tmp;
}

// Binary logical operations between two arrays.  Behavior is undefined
// if the two arrays have different lengths.  Note that operator== does
// not do what you might at first expect.

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator==(const valarray<_Tp>& __x,
                                            const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] == __y[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator<(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] < __y[__i];
  return __tmp;
}

#ifdef _STLP_USE_SEPARATE_RELOPS_NAMESPACE

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator!=(const valarray<_Tp>& __x,
                                            const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] != __y[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator>(const valarray<_Tp>& __x,
                                           const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] > __y[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator<=(const valarray<_Tp>& __x,
                                            const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] <= __y[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator>=(const valarray<_Tp>& __x,
                                            const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] >= __y[__i];
  return __tmp;
}

#endif /* _STLP_USE_SEPARATE_RELOPS_NAMESPACE */
// fbp : swap ?

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator&&(const valarray<_Tp>& __x,
                                            const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] && __y[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator||(const valarray<_Tp>& __x,
                                            const valarray<_Tp>& __y) {
  _STLP_ASSERT(__x.size() == __y.size())
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] || __y[__i];
  return __tmp;
}

// Logical operations between an array and a scalar.

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator==(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] == __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator==(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c == __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator!=(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] != __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator!=(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c != __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator<(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] < __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator<(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c < __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator>(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] > __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator>(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c > __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator<=(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i]  <= __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator<=(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c <= __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator>=(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] >= __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator>=(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c >= __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator&&(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] && __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator&&(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c && __x[__i];
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator||(const valarray<_Tp>& __x, const _Tp& __c) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __x[__i] || __c;
  return __tmp;
}

template <class _Tp>
inline _Valarray_bool _STLP_CALL operator||(const _Tp& __c, const valarray<_Tp>& __x) {
  _Valarray_bool __tmp(__x.size(), _Valarray_bool::_NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = __c || __x[__i];
  return __tmp;
}

// valarray "transcendentals" (the list includes abs and sqrt, which,
// of course, are not transcendental).

template <class _Tp>
inline valarray<_Tp> abs(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::abs(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> acos(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::acos(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> asin(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::asin(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> atan(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::atan(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> atan2(const valarray<_Tp>& __x,
                           const valarray<_Tp>& __y) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::atan2(__x[__i], __y[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> atan2(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::atan2(__x[__i], __c);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> atan2(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::atan2(__c, __x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> cos(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::cos(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> cosh(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::cosh(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> exp(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::exp(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> log(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::log(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> log10(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::log10(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> pow(const valarray<_Tp>& __x,
                         const valarray<_Tp>& __y) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::pow(__x[__i], __y[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> pow(const valarray<_Tp>& __x, const _Tp& __c) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::pow(__x[__i], __c);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> pow(const _Tp& __c, const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::pow(__c, __x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> sin(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::sin(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> sinh(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::sinh(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> sqrt(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::sqrt(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> tan(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::tan(__x[__i]);
  return __tmp;
}

template <class _Tp>
inline valarray<_Tp> tanh(const valarray<_Tp>& __x) {
  typedef typename valarray<_Tp>::_NoInit _NoInit;
  valarray<_Tp> __tmp(__x.size(), _NoInit());
  for (size_t __i = 0; __i < __x.size(); ++__i)
    __tmp[__i] = ::tanh(__x[__i]);
  return __tmp;
}

//----------------------------------------------------------------------
// slice and slice_array

class slice {
public:
  slice() : _M_start(0), _M_length(0), _M_stride(0) {}
  slice(size_t __start, size_t __length, size_t __stride)
    : _M_start(__start), _M_length(__length), _M_stride(__stride)
    {}
  __TRIVIAL_DESTRUCTOR(slice)

  size_t start()  const { return _M_start; }
  size_t size()   const { return _M_length; }
  size_t stride() const { return _M_stride; }

private:
  size_t _M_start;
  size_t _M_length;
  size_t _M_stride;
};

template <class _Tp>
class slice_array {
  friend class valarray<_Tp>;
public:
  typedef _Tp value_type;

  void operator=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] = __x[__i];
  }

  void operator*=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] *= __x[__i];
  }

  void operator/=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] /= __x[__i];
  }

  void operator%=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] %= __x[__i];
  }

  void operator+=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] += __x[__i];
  }

  void operator-=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] -= __x[__i];
  }

  void operator^=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] ^= __x[__i];
  }

  void operator&=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] &= __x[__i];
  }

  void operator|=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] |= __x[__i];
  }

  void operator<<=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] <<= __x[__i];
  }

  void operator>>=(const valarray<value_type>& __x) const {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] >>= __x[__i];
  }

  void operator=(const value_type& __c) /*const could be const but standard says NO (26.3.5.4-1)*/ {
    size_t __index = _M_slice.start();
    for (size_t __i = 0;
         __i < _M_slice.size();
         ++__i, __index += _M_slice.stride())
      _M_array[__index] = __c;
  }

  // C++ Standard defect 253, copy constructor must be public.
  slice_array(const slice_array &__x)
    : _M_slice(__x._M_slice), _M_array(__x._M_array)
    {}

  ~slice_array() {}

private:
  slice_array(const slice& __slice, valarray<_Tp> &__array)
    : _M_slice(__slice), _M_array(__array)
    {}

  slice          _M_slice;
  valarray<_Tp>& _M_array;

private:
  // Disable default constructor and assignment
  slice_array();
  slice_array& operator=(const slice_array&);
};

// valarray member functions dealing with slice and slice_array

template <class _Tp>
inline valarray<_Tp>::valarray(const slice_array<_Tp>& __x)
  : _Valarray_base<_Tp>(__x._M_slice.size()) {
  typedef typename __type_traits<_Tp>::has_trivial_default_constructor
          _Is_Trivial;
  _M_initialize(_Is_Trivial());
  *this = __x;
}


template <class _Tp>
inline slice_array<_Tp> valarray<_Tp>::operator[](slice __slice)
{ return slice_array<_Tp>(__slice, *this); }

//----------------------------------------------------------------------
// gslice and gslice_array

template <class _Size>
struct _Gslice_Iter_tmpl;

class gslice {
  friend struct _Gslice_Iter_tmpl<size_t>;
public:
  gslice() : _M_start(0), _M_lengths(), _M_strides() {}
  gslice(size_t __start,
         const _Valarray_size_t& __lengths, const _Valarray_size_t& __strides)
    : _M_start(__start), _M_lengths(__lengths), _M_strides(__strides)
    {}
  __TRIVIAL_DESTRUCTOR(gslice)

  size_t start()            const { return _M_start; }
  _Valarray_size_t size()   const { return _M_lengths; }
  _Valarray_size_t stride() const { return _M_strides; }

  // Extension: check for an empty gslice.
  bool _M_empty() const { return _M_lengths.size() == 0; }

  // Extension: number of indices this gslice represents.  (For a degenerate
  // gslice, they're not necessarily all distinct.)
  size_t _M_size() const {
    return !this->_M_empty()
      ? accumulate(_M_lengths._M_first + 1,
                   _M_lengths._M_first + _M_lengths._M_size,
                   _M_lengths[0],
                   multiplies<size_t>())
      : 0;
  }

# ifndef __HP_aCC
private:
# endif

  size_t _M_start;
  _Valarray_size_t _M_lengths;
  _Valarray_size_t _M_strides;
};

// This is not an STL iterator.  It is constructed from a gslice, and it
// steps through the gslice indices in sequence.  See 23.3.6 of the C++
// standard, paragraphs 2-3, for an explanation of the sequence.  At
// each step we get two things: the ordinal (i.e. number of steps taken),
// and the one-dimensional index.

template <class _Size>
struct _Gslice_Iter_tmpl {
  _Gslice_Iter_tmpl(const gslice& __gslice)
    : _M_step(0), _M_1d_idx(__gslice.start()),
      _M_indices(size_t(0), __gslice._M_lengths.size()),
      _M_gslice(__gslice)
    {}

  bool _M_done() const { return _M_indices[0] == _M_gslice._M_lengths[0]; }

  bool _M_incr();

  _Size _M_step;
  _Size _M_1d_idx;

  valarray<_Size> _M_indices;
  const gslice& _M_gslice;
};

typedef _Gslice_Iter_tmpl<size_t> _Gslice_Iter;

template <class _Tp>
class gslice_array {
  friend class valarray<_Tp>;
public:
  typedef _Tp value_type;

  void operator= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] = __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator*= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] *= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator/= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] /= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator%= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] %= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator+= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] += __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator-= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] -= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator^= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] ^= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator&= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] &= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator|= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] |= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator<<= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] <<= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator>>= (const valarray<value_type>& __x) const {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] >>= __x[__i._M_step]; while(__i._M_incr());
    }
  }

  void operator= (const value_type& __c) /*const could be const but standard says NO (26.3.7.4-1)*/ {
    if (!_M_gslice._M_empty()) {
      _Gslice_Iter __i(_M_gslice);
      do _M_array[__i._M_1d_idx] = __c; while(__i._M_incr());
    }
  }

  // C++ Standard defect 253, copy constructor must be public.
  gslice_array(const gslice_array& __x)
    : _M_gslice(__x._M_gslice), _M_array(__x._M_array)
    {}

  ~gslice_array() {}

private:
  gslice_array(const gslice &__gslice, valarray<_Tp> &__array)
    : _M_gslice(__gslice), _M_array(__array)
    {}

  gslice                _M_gslice;
  valarray<value_type>& _M_array;

private:
  // Disable default constructor and assignment
  gslice_array();
  void operator=(const gslice_array<_Tp>&);
};

// valarray member functions dealing with gslice and gslice_array.  Note
// that it is illegal (behavior is undefined) to construct a gslice_array
// from a degenerate gslice.

template <class _Tp>
inline valarray<_Tp>::valarray(const gslice_array<_Tp>& __x)
  : _Valarray_base<_Tp>(__x._M_gslice._M_size()) {
  typedef typename __type_traits<_Tp>::has_trivial_default_constructor
          _Is_Trivial;
  _M_initialize(_Is_Trivial());
  *this = __x;
}

template <class _Tp>
inline gslice_array<_Tp> valarray<_Tp>::operator[](const gslice& __slice)
{ return gslice_array<_Tp>(__slice, *this); }


//----------------------------------------------------------------------
// mask_array

template <class _Tp>
class mask_array {
  friend class valarray<_Tp>;
public:
  typedef _Tp value_type;

  void operator=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] = __x[__idx++];
  }

  void operator*=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] *= __x[__idx++];
  }

  void operator/=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] /= __x[__idx++];
  }

  void operator%=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] %= __x[__idx++];
  }

  void operator+=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] += __x[__idx++];
  }

  void operator-=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] -= __x[__idx++];
  }

  void operator^=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] ^= __x[__idx++];
  }

  void operator&=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] &= __x[__idx++];
  }

  void operator|=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] |= __x[__idx++];
  }

  void operator<<=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] <<= __x[__idx++];
  }

  void operator>>=(const valarray<value_type>& __x) const {
    size_t __idx = 0;
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] >>= __x[__idx++];
  }

  void operator=(const value_type& __c) const {
    for (size_t __i = 0; __i < _M_array.size(); ++__i)
      if (_M_mask[__i]) _M_array[__i] = __c;
  }

  // Extension: number of true values in the mask
  size_t _M_num_true() const {
    size_t __result = 0;
    for (size_t __i = 0; __i < _M_mask.size(); ++__i)
      if (_M_mask[__i]) ++__result;
    return __result;
  }

  // C++ Standard defect 253, copy constructor must be public.
  mask_array(const mask_array& __x)
    : _M_mask(__x._M_mask), _M_array(__x._M_array)
    {}

  ~mask_array() {}

private:
  mask_array(const _Valarray_bool& __mask, valarray<_Tp>& __array)
    : _M_mask(__mask), _M_array(__array)
    {}
  _Valarray_bool _M_mask;
  valarray<_Tp>& _M_array;

private:
  // Disable default constructor and assignment
  mask_array();
  void operator=(const mask_array<_Tp>&);
};

// valarray member functions dealing with mask_array

template <class _Tp>
inline valarray<_Tp>::valarray(const mask_array<_Tp>& __x)
  : _Valarray_base<_Tp>(__x._M_num_true()) {
  typedef typename __type_traits<_Tp>::has_trivial_default_constructor
          _Is_Trivial;
  _M_initialize(_Is_Trivial());
  *this = __x;
}

// Behavior is undefined if __x._M_num_true() != this->size()
template <class _Tp>
inline valarray<_Tp>& valarray<_Tp>::operator=(const mask_array<_Tp>& __x) {
  size_t __idx = 0;
  for (size_t __i = 0; __i < __x._M_array.size(); ++__i)
    if (__x._M_mask[__i]) (*this)[__idx++] = __x._M_array[__i];
  return *this;
}

template <class _Tp>
inline mask_array<_Tp> valarray<_Tp>::operator[](const _Valarray_bool& __mask) {
  _STLP_ASSERT(__mask.size() == this->size())
  return mask_array<_Tp>(__mask, *this);
}

//----------------------------------------------------------------------
// indirect_array

template <class _Tp>
class indirect_array {
  friend class valarray<_Tp>;
public:
  typedef _Tp value_type;

  void operator=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] = __x[__i];
  }

  void operator*=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] *= __x[__i];
  }

  void operator/=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] /= __x[__i];
  }

  void operator%=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] %= __x[__i];
  }

  void operator+=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] += __x[__i];
  }

  void operator-=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] -= __x[__i];
  }

  void operator^=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] ^= __x[__i];
  }

  void operator&=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] &= __x[__i];
  }

  void operator|=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] |= __x[__i];
  }

  void operator<<=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] <<= __x[__i];
  }

  void operator>>=(const valarray<value_type>& __x) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] >>= __x[__i];
  }

  void operator=(const value_type& __c) const {
    for (size_t __i = 0; __i < _M_addr.size(); ++__i)
      _M_array[_M_addr[__i]] = __c;
  }

  // C++ Standard defect 253, copy constructor must be public.
  indirect_array(const indirect_array& __x)
    : _M_addr(__x._M_addr), _M_array(__x._M_array)
    {}

  ~indirect_array() {}

private:
  indirect_array(const _Valarray_size_t& __addr, valarray<_Tp>& __array)
    : _M_addr(__addr), _M_array(__array)
  {}

  _Valarray_size_t _M_addr;
  valarray<_Tp>&   _M_array;

private:
  // Disable default constructor and assignment
  indirect_array();
  void operator=(const indirect_array<_Tp>&);
};

// valarray member functions dealing with indirect_array

template <class _Tp>
inline valarray<_Tp>::valarray(const indirect_array<_Tp>& __x)
  : _Valarray_base<_Tp>(__x._M_addr.size()) {
  typedef typename __type_traits<_Tp>::has_trivial_default_constructor
          _Is_Trivial;
  _M_initialize(_Is_Trivial());
  *this = __x;
}


template <class _Tp>
inline indirect_array<_Tp>
valarray<_Tp>::operator[](const _Valarray_size_t& __addr)
{ return indirect_array<_Tp>(__addr, *this); }

_STLP_END_NAMESPACE

# if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_valarray.c>
# endif

#endif /* _STLP_VALARRAY */


// Local Variables:
// mode:C++
// End:
