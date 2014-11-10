/*
 * Copyright (c) 1997-1999
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

#ifndef _STLP_INTERNAL_STRING_H
#define _STLP_INTERNAL_STRING_H

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

#ifndef _STLP_STRING_FWD_H
#  include <stl/_string_fwd.h>
#endif

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_H
#  include <stl/_iterator.h>
#endif

#ifndef _STLP_INTERNAL_UNINITIALIZED_H
#  include <stl/_uninitialized.h>
#endif

#if defined (_STLP_USE_TEMPLATE_EXPRESSION)
#  include <stl/_string_sum.h>
#endif /* _STLP_USE_TEMPLATE_EXPRESSION */

#if defined (__MWERKS__) && ! defined (_STLP_USE_OWN_NAMESPACE)

// MSL implementation classes expect to see the definition of streampos
// when this header is included. We expect this to be fixed in later MSL
// implementations
#  if !defined( __MSL_CPP__ ) || __MSL_CPP__ < 0x4105
#    include <stl/msl_string.h>
#  endif
#endif // __MWERKS__

/*
 * Standard C++ string class.  This class has performance
 * characteristics very much like vector<>, meaning, for example, that
 * it does not perform reference-count or copy-on-write, and that
 * concatenation of two strings is an O(N) operation.

 * There are three reasons why basic_string is not identical to
 * vector.
 * First, basic_string always stores a null character at the end;
 * this makes it possible for c_str to be a fast operation.
 * Second, the C++ standard requires basic_string to copy elements
 * using char_traits<>::assign, char_traits<>::copy, and
 * char_traits<>::move.  This means that all of vector<>'s low-level
 * operations must be rewritten.  Third, basic_string<> has a lot of
 * extra functions in its interface that are convenient but, strictly
 * speaking, redundant.
 */

#include <stl/_string_base.h>

_STLP_BEGIN_NAMESPACE

// ------------------------------------------------------------
// Class basic_string.

// Class invariants:
// (1) [start, finish) is a valid range.
// (2) Each iterator in [start, finish) points to a valid object
//     of type value_type.
// (3) *finish is a valid object of type value_type; when
//     value_type is not a POD it is value_type().
// (4) [finish + 1, end_of_storage) is a valid range.
// (5) Each iterator in [finish + 1, end_of_storage) points to
//     unininitialized memory.

// Note one important consequence: a string of length n must manage
// a block of memory whose size is at least n + 1.

_STLP_MOVE_TO_PRIV_NAMESPACE
struct _String_reserve_t {};
_STLP_MOVE_TO_STD_NAMESPACE

#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#  define basic_string _STLP_NO_MEM_T_NAME(str)
#elif defined (_STLP_DEBUG)
#  define basic_string _STLP_NON_DBG_NAME(str)
#endif

#if defined (basic_string)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

#if defined (__DMC__)
#  define _STLP_PRIVATE public
#elif defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#  define _STLP_PRIVATE protected
#else
#  define _STLP_PRIVATE private
#endif

template <class _CharT, class _Traits, class _Alloc>
class basic_string : _STLP_PRIVATE _STLP_PRIV _String_base<_CharT,_Alloc>
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (basic_string)
                   , public __stlport_class<basic_string<_CharT, _Traits, _Alloc> >
#endif
{
_STLP_PRIVATE:                        // Private members inherited from base.
  typedef _STLP_PRIV _String_base<_CharT,_Alloc> _Base;
  typedef basic_string<_CharT, _Traits, _Alloc> _Self;

public:
  typedef _CharT value_type;
  typedef _Traits traits_type;

  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef typename _Base::size_type size_type;
  typedef ptrdiff_t difference_type;
  typedef random_access_iterator_tag _Iterator_category;

  typedef const value_type* const_iterator;
  typedef value_type*       iterator;

  _STLP_DECLARE_RANDOM_ACCESS_REVERSE_ITERATORS;

#include <stl/_string_npos.h>

  typedef _STLP_PRIV _String_reserve_t _Reserve_t;

public:                         // Constructor, destructor, assignment.
  typedef typename _Base::allocator_type allocator_type;

  allocator_type get_allocator() const
  { return _STLP_CONVERT_ALLOCATOR((const allocator_type&)this->_M_start_of_storage, _CharT); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit basic_string(const allocator_type& __a = allocator_type())
#else
  basic_string()
      : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type(), _Base::_DEFAULT_SIZE)
  { _M_terminate_string(); }
  explicit basic_string(const allocator_type& __a)
#endif
      : _STLP_PRIV _String_base<_CharT,_Alloc>(__a, _Base::_DEFAULT_SIZE)
  { _M_terminate_string(); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  basic_string(_Reserve_t, size_t __n,
               const allocator_type& __a = allocator_type())
#else
  basic_string(_Reserve_t, size_t __n)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type(), __n + 1)
  { _M_terminate_string(); }
  basic_string(_Reserve_t, size_t __n, const allocator_type& __a)
#endif
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__a, __n + 1)
  { _M_terminate_string(); }

  basic_string(const _Self&);

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  basic_string(const _Self& __s, size_type __pos, size_type __n = npos,
               const allocator_type& __a = allocator_type())
#else
  basic_string(const _Self& __s, size_type __pos)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type()) {
    if (__pos > __s.size())
      this->_M_throw_out_of_range();
    else
      _M_range_initialize(__s._M_Start() + __pos, __s._M_Finish());
  }
  basic_string(const _Self& __s, size_type __pos, size_type __n)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type()) {
    if (__pos > __s.size())
      this->_M_throw_out_of_range();
    else
      _M_range_initialize(__s._M_Start() + __pos,
                          __s._M_Start() + __pos + (min) (__n, __s.size() - __pos));
  }
  basic_string(const _Self& __s, size_type __pos, size_type __n,
               const allocator_type& __a)
#endif
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__a) {
    if (__pos > __s.size())
      this->_M_throw_out_of_range();
    else
      _M_range_initialize(__s._M_Start() + __pos,
                          __s._M_Start() + __pos + (min) (__n, __s.size() - __pos));
  }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  basic_string(const _CharT* __s, size_type __n,
               const allocator_type& __a = allocator_type())
#else
  basic_string(const _CharT* __s, size_type __n)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type()) {
      _STLP_FIX_LITERAL_BUG(__s)
      _M_range_initialize(__s, __s + __n);
    }
  basic_string(const _CharT* __s, size_type __n, const allocator_type& __a)
#endif
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__a) {
      _STLP_FIX_LITERAL_BUG(__s)
      _M_range_initialize(__s, __s + __n);
    }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  basic_string(const _CharT* __s,
               const allocator_type& __a = allocator_type());
#else
  basic_string(const _CharT* __s);
  basic_string(const _CharT* __s, const allocator_type& __a);
#endif

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  basic_string(size_type __n, _CharT __c,
               const allocator_type& __a = allocator_type())
#else
  basic_string(size_type __n, _CharT __c)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type(), __n + 1) {
    this->_M_finish = _STLP_PRIV __uninitialized_fill_n(this->_M_Start(), __n, __c);
    _M_terminate_string();
  }
  basic_string(size_type __n, _CharT __c, const allocator_type& __a)
#endif
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__a, __n + 1) {
    this->_M_finish = _STLP_PRIV __uninitialized_fill_n(this->_M_Start(), __n, __c);
    _M_terminate_string();
  }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  basic_string(__move_source<_Self> src)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__move_source<_Base>(src.get())) {}
#endif

  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _InputIterator>
  basic_string(_InputIterator __f, _InputIterator __l,
               const allocator_type & __a _STLP_ALLOCATOR_TYPE_DFL)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__a) {
    typedef typename _IsIntegral<_InputIterator>::_Ret _Integral;
    _M_initialize_dispatch(__f, __l, _Integral());
  }
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  basic_string(_InputIterator __f, _InputIterator __l)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type()) {
    typedef typename _IsIntegral<_InputIterator>::_Ret _Integral;
    _M_initialize_dispatch(__f, __l, _Integral());
  }
#  endif
#else
#  if !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  basic_string(const _CharT* __f, const _CharT* __l,
               const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(__a) {
    _STLP_FIX_LITERAL_BUG(__f)  _STLP_FIX_LITERAL_BUG(__l)
    _M_range_initialize(__f, __l);
  }
#    if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  basic_string(const _CharT* __f, const _CharT* __l)
    : _STLP_PRIV _String_base<_CharT,_Alloc>(allocator_type()) {
    _STLP_FIX_LITERAL_BUG(__f)  _STLP_FIX_LITERAL_BUG(__l)
    _M_range_initialize(__f, __l);
  }
#    endif
#  endif
#  if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  /* We need an additionnal constructor to build an empty string without
   * any allocation or termination char*/
protected:
  struct _CalledFromWorkaround_t {};
  basic_string(_CalledFromWorkaround_t, const allocator_type &__a)
    : _String_base<_CharT,_Alloc>(__a) {}
#  endif
#endif

_STLP_PRIVATE:
  size_type _M_compute_next_size(size_type __n) {
    const size_type __size = size();
    if (__n > max_size() - __size)
      this->_M_throw_length_error();
    size_type __len = __size + (max)(__n, __size) + 1;
    if (__len > max_size() || __len < __size)
      __len = max_size(); // overflow
    return __len;
  }

  template <class _InputIter>
  void _M_range_initialize(_InputIter __f, _InputIter __l,
                           const input_iterator_tag &__tag) {
    this->_M_allocate_block();
    _M_construct_null(this->_M_Finish());
    _M_appendT(__f, __l, __tag);
  }

  template <class _ForwardIter>
  void _M_range_initialize(_ForwardIter __f, _ForwardIter __l,
                           const forward_iterator_tag &) {
    difference_type __n = _STLP_STD::distance(__f, __l);
    this->_M_allocate_block(__n + 1);
    this->_M_finish = uninitialized_copy(__f, __l, this->_M_Start());
    this->_M_terminate_string();
  }

  template <class _InputIter>
  void _M_range_initializeT(_InputIter __f, _InputIter __l) {
    _M_range_initialize(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
  }

  template <class _Integer>
  void _M_initialize_dispatch(_Integer __n, _Integer __x, const __true_type& /*_Integral*/) {
    this->_M_allocate_block(__n + 1);
    this->_M_finish = _STLP_PRIV __uninitialized_fill_n(this->_M_Start(), __n, __x);
    this->_M_terminate_string();
  }

  template <class _InputIter>
  void _M_initialize_dispatch(_InputIter __f, _InputIter __l, const __false_type& /*_Integral*/) {
    _M_range_initializeT(__f, __l);
  }

public:
  _Self& operator=(const _Self& __s) {
    if (&__s != this)
      _M_assign(__s._M_Start(), __s._M_Finish());
    return *this;
  }

  _Self& operator=(const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    return _M_assign(__s, __s + traits_type::length(__s));
  }

  _Self& operator=(_CharT __c)
  { return assign(__STATIC_CAST(size_type,1), __c); }

private:
  static _CharT _STLP_CALL _M_null()
  { return _STLP_DEFAULT_CONSTRUCTED(_CharT); }

_STLP_PRIVATE:                     // Helper functions used by constructors
                                   // and elsewhere.
  void _M_construct_null(_CharT* __p) const
  { _STLP_STD::_Construct(__p); }
  void _M_terminate_string()
  { _M_construct_null(this->_M_Finish()); }
  bool _M_inside(const _CharT* __s) const {
    _STLP_FIX_LITERAL_BUG(__s)
    return (__s >= this->_M_Start()) && (__s < this->_M_Finish());
  }

  void _M_range_initialize(const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    ptrdiff_t __n = __l - __f;
    this->_M_allocate_block(__n + 1);
    this->_M_finish = uninitialized_copy(__f, __l, this->_M_Start());
    _M_terminate_string();
  }

public:                         // Iterators.
  iterator begin()             { return this->_M_Start(); }
  iterator end()               { return this->_M_Finish(); }
  const_iterator begin() const { return this->_M_Start(); }
  const_iterator end()   const { return this->_M_Finish(); }

  reverse_iterator rbegin()
  { return reverse_iterator(this->_M_Finish()); }
  reverse_iterator rend()
  { return reverse_iterator(this->_M_Start()); }
  const_reverse_iterator rbegin() const
  { return const_reverse_iterator(this->_M_Finish()); }
  const_reverse_iterator rend()   const
  { return const_reverse_iterator(this->_M_Start()); }

public:                         // Size, capacity, etc.
  size_type size() const     { return this->_M_Finish() - this->_M_Start(); }
  size_type length() const   { return size(); }
  size_type max_size() const { return _Base::max_size(); }

  void resize(size_type __n, _CharT __c) {
    if (__n <= size())
      erase(begin() + __n, end());
    else
      append(__n - size(), __c);
  }

  void resize(size_type __n) { resize(__n, _M_null()); }

private:
  void _M_reserve(size_type);
public:
  void reserve(size_type = 0);

  size_type capacity() const
  { return this->_M_capacity() - 1; }

  void clear() {
    if (!empty()) {
      _Traits::assign(*(this->_M_Start()), _M_null());
      this->_M_finish = this->_M_Start();
    }
  }

  bool empty() const { return this->_M_Start() == this->_M_Finish(); }

public:                         // Element access.

  const_reference operator[](size_type __n) const
  { return *(this->_M_Start() + __n); }
  reference operator[](size_type __n)
  { return *(this->_M_Start() + __n); }

  const_reference at(size_type __n) const {
    if (__n >= size())
      this->_M_throw_out_of_range();
    return *(this->_M_Start() + __n);
  }

  reference at(size_type __n) {
    if (__n >= size())
      this->_M_throw_out_of_range();
    return *(this->_M_Start() + __n);
  }

public:                         // Append, operator+=, push_back.

  _Self& operator+=(const _Self& __s) { return append(__s); }
  _Self& operator+=(const _CharT* __s) { _STLP_FIX_LITERAL_BUG(__s) return append(__s); }
  _Self& operator+=(_CharT __c) { push_back(__c); return *this; }

private:
  _Self& _M_append(const _CharT* __first, const _CharT* __last);

#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _InputIter>
  _Self& _M_appendT(_InputIter __first, _InputIter __last,
                    const input_iterator_tag &) {
    for ( ; __first != __last ; ++__first)
      push_back(*__first);
    return *this;
  }

  template <class _ForwardIter>
  _Self& _M_appendT(_ForwardIter __first, _ForwardIter __last,
                    const forward_iterator_tag &) {
    if (__first != __last) {
      size_type __n = __STATIC_CAST(size_type, _STLP_STD::distance(__first, __last));
      if (__n >= this->_M_rest()) {
        size_type __len = _M_compute_next_size(__n);
        pointer __new_start = this->_M_start_of_storage.allocate(__len, __len);
        pointer __new_finish = uninitialized_copy(this->_M_Start(), this->_M_Finish(), __new_start);
        __new_finish = uninitialized_copy(__first, __last, __new_finish);
        _M_construct_null(__new_finish);
        this->_M_deallocate_block();
        this->_M_reset(__new_start, __new_finish, __new_start + __len);
      }
      else {
        _Traits::assign(*this->_M_finish, *__first++);
        uninitialized_copy(__first, __last, this->_M_Finish() + 1);
        _M_construct_null(this->_M_Finish() + __n);
        this->_M_finish += __n;
      }
    }
    return *this;
  }

  template <class _Integer>
  _Self& _M_append_dispatch(_Integer __n, _Integer __x, const __true_type& /*Integral*/)
  { return append((size_type) __n, (_CharT) __x); }

  template <class _InputIter>
  _Self& _M_append_dispatch(_InputIter __f, _InputIter __l, const __false_type& /*Integral*/)
  { return _M_appendT(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter)); }

public:
  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  _Self& append(_InputIter __first, _InputIter __last) {
    typedef typename _IsIntegral<_InputIter>::_Ret _Integral;
    return _M_append_dispatch(__first, __last, _Integral());
  }
#else
public:
  _Self& append(const _CharT* __first, const _CharT* __last) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    return _M_append(__first, __last);
  }
#endif

public:
  _Self& append(const _Self& __s)
  { return _M_append(__s._M_Start(), __s._M_Finish()); }

  _Self& append(const _Self& __s,
                size_type __pos, size_type __n) {
    if (__pos > __s.size())
      this->_M_throw_out_of_range();
    return _M_append(__s._M_Start() + __pos,
                     __s._M_Start() + __pos + (min) (__n, __s.size() - __pos));
  }

  _Self& append(const _CharT* __s, size_type __n)
  { _STLP_FIX_LITERAL_BUG(__s) return _M_append(__s, __s+__n); }
  _Self& append(const _CharT* __s)
  { _STLP_FIX_LITERAL_BUG(__s) return _M_append(__s, __s + traits_type::length(__s)); }
  _Self& append(size_type __n, _CharT __c);

public:
  void push_back(_CharT __c) {
    if (this->_M_rest() == 1 )
      _M_reserve(_M_compute_next_size(1));
    _M_construct_null(this->_M_Finish() + 1);
    _Traits::assign(*(this->_M_Finish()), __c);
    ++this->_M_finish;
  }

  void pop_back() {
    _Traits::assign(*(this->_M_Finish() - 1), _M_null());
    --this->_M_finish;
  }

public:                         // Assign
  _Self& assign(const _Self& __s)
  { return _M_assign(__s._M_Start(), __s._M_Finish()); }

  _Self& assign(const _Self& __s,
                size_type __pos, size_type __n) {
    if (__pos > __s.size())
      this->_M_throw_out_of_range();
    return _M_assign(__s._M_Start() + __pos,
                     __s._M_Start() + __pos + (min) (__n, __s.size() - __pos));
  }

  _Self& assign(const _CharT* __s, size_type __n)
  { _STLP_FIX_LITERAL_BUG(__s) return _M_assign(__s, __s + __n); }

  _Self& assign(const _CharT* __s)
  { _STLP_FIX_LITERAL_BUG(__s) return _M_assign(__s, __s + _Traits::length(__s)); }

  _Self& assign(size_type __n, _CharT __c);

private:
  _Self& _M_assign(const _CharT* __f, const _CharT* __l);

#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  // Helper functions for assign.
  template <class _Integer>
  _Self& _M_assign_dispatch(_Integer __n, _Integer __x, const __true_type& /*_Integral*/)
  { return assign((size_type) __n, (_CharT) __x); }

  template <class _InputIter>
  _Self& _M_assign_dispatch(_InputIter __f, _InputIter __l, const __false_type& /*_Integral*/) {
    pointer __cur = this->_M_Start();
    while (__f != __l && __cur != this->_M_Finish()) {
      _Traits::assign(*__cur, *__f);
      ++__f;
      ++__cur;
    }
    if (__f == __l)
      erase(__cur, this->end());
    else
      _M_appendT(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
    return *this;
  }

public:
  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  _Self& assign(_InputIter __first, _InputIter __last) {
    typedef typename _IsIntegral<_InputIter>::_Ret _Integral;
    return _M_assign_dispatch(__first, __last, _Integral());
  }
#else
public:
  _Self& assign(const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    return _M_assign(__f, __l);
  }
#endif

public:                         // Insert
  _Self& insert(size_type __pos, const _Self& __s) {
    if (__pos > size())
      this->_M_throw_out_of_range();
    if (__s.size() > max_size() - size())
      this->_M_throw_length_error();
    _M_insert(begin() + __pos, __s._M_Start(), __s._M_Finish(), &__s == this);
    return *this;
  }

  _Self& insert(size_type __pos, const _Self& __s,
                size_type __beg, size_type __n) {
    if (__pos > size() || __beg > __s.size())
      this->_M_throw_out_of_range();
    size_type __len = (min) (__n, __s.size() - __beg);
    if (__len > max_size() - size())
      this->_M_throw_length_error();
    _M_insert(begin() + __pos,
              __s._M_Start() + __beg, __s._M_Start() + __beg + __len, &__s == this);
    return *this;
  }
  _Self& insert(size_type __pos, const _CharT* __s, size_type __n) {
    _STLP_FIX_LITERAL_BUG(__s)
    if (__pos > size())
      this->_M_throw_out_of_range();
    if (__n > max_size() - size())
      this->_M_throw_length_error();
    _M_insert(begin() + __pos, __s, __s + __n, _M_inside(__s));
    return *this;
  }

  _Self& insert(size_type __pos, const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    if (__pos > size())
      this->_M_throw_out_of_range();
    size_type __len = _Traits::length(__s);
    if (__len > max_size() - size())
      this->_M_throw_length_error();
    _M_insert(this->_M_Start() + __pos, __s, __s + __len, _M_inside(__s));
    return *this;
  }

  _Self& insert(size_type __pos, size_type __n, _CharT __c) {
    if (__pos > size())
      this->_M_throw_out_of_range();
    if (__n > max_size() - size())
      this->_M_throw_length_error();
    insert(begin() + __pos, __n, __c);
    return *this;
  }

  iterator insert(iterator __p, _CharT __c) {
    _STLP_FIX_LITERAL_BUG(__p)
    if (__p == end()) {
      push_back(__c);
      return this->_M_Finish() - 1;
    }
    else
      return _M_insert_aux(__p, __c);
  }

  void insert(iterator __p, size_t __n, _CharT __c);

_STLP_PRIVATE:  // Helper functions for insert.
  void _M_insert(iterator __p, const _CharT* __first, const _CharT* __last, bool __self_ref);

  pointer _M_insert_aux(pointer, _CharT);

  void _M_copy(const _CharT* __f, const _CharT* __l, _CharT* __res) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    _STLP_FIX_LITERAL_BUG(__res)
    _Traits::copy(__res, __f, __l - __f);
  }

  void _M_move(const _CharT* __f, const _CharT* __l, _CharT* __res) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    _Traits::move(__res, __f, __l - __f);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
#  if !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _ForwardIter>
  void _M_insert_overflow(iterator __pos, _ForwardIter __first, _ForwardIter __last,
                          size_type __n) {
    size_type __len = _M_compute_next_size(__n);
    pointer __new_start = this->_M_start_of_storage.allocate(__len, __len);
    pointer __new_finish = uninitialized_copy(this->_M_Start(), __pos, __new_start);
    __new_finish = uninitialized_copy(__first, __last, __new_finish);
    __new_finish = uninitialized_copy(__pos, this->_M_Finish(), __new_finish);
    _M_construct_null(__new_finish);
    this->_M_deallocate_block();
    this->_M_reset(__new_start, __new_finish, __new_start + __len);
  }

  template <class _InputIter>
  void _M_insertT(iterator __p, _InputIter __first, _InputIter __last,
                  const input_iterator_tag &) {
    for ( ; __first != __last; ++__first) {
      __p = insert(__p, *__first);
      ++__p;
    }
  }

  template <class _ForwardIter>
  void _M_insertT(iterator __pos, _ForwardIter __first, _ForwardIter __last,
                  const forward_iterator_tag &) {
    if (__first != __last) {
      size_type __n = _STLP_STD::distance(__first, __last);
      if (__n < this->_M_rest()) {
        const size_type __elems_after = this->_M_finish - __pos;
        if (__elems_after >= __n) {
          uninitialized_copy((this->_M_Finish() - __n) + 1, this->_M_Finish() + 1, this->_M_Finish() + 1);
          this->_M_finish += __n;
          _Traits::move(__pos + __n, __pos, (__elems_after - __n) + 1);
          _M_copyT(__first, __last, __pos);
        }
        else {
          pointer __old_finish = this->_M_Finish();
          _ForwardIter __mid = __first;
          _STLP_STD::advance(__mid, __elems_after + 1);
          _STLP_STD::uninitialized_copy(__mid, __last, this->_M_Finish() + 1);
          this->_M_finish += __n - __elems_after;
          uninitialized_copy(__pos, __old_finish + 1, this->_M_Finish());
          this->_M_finish += __elems_after;
          _M_copyT(__first, __mid, __pos);
        }
      }
      else {
        _M_insert_overflow(__pos, __first, __last, __n);
      }
    }
  }

  template <class _Integer>
  void _M_insert_dispatch(iterator __p, _Integer __n, _Integer __x,
                          const __true_type& /*Integral*/)
  { insert(__p, (size_type) __n, (_CharT) __x); }

  template <class _InputIter>
  void _M_insert_dispatch(iterator __p, _InputIter __first, _InputIter __last,
                          const __false_type& /*Integral*/) {
    _STLP_FIX_LITERAL_BUG(__p)
    /* We are forced to do a temporary string to avoid the self referencing issue. */
    const _Self __self(__first, __last, get_allocator());
    _M_insertT(__p, __self.begin(), __self.end(), forward_iterator_tag());
  }

  template <class _InputIterator>
  void _M_copyT(_InputIterator __first, _InputIterator __last, pointer __result) {
    _STLP_FIX_LITERAL_BUG(__result)
    for ( ; __first != __last; ++__first, ++__result)
      _Traits::assign(*__result, *__first);
  }

#    if !defined (_STLP_NO_METHOD_SPECIALIZATION)
  void _M_copyT(const _CharT* __f, const _CharT* __l, _CharT* __res) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    _STLP_FIX_LITERAL_BUG(__res)
    _Traits::copy(__res, __f, __l - __f);
  }
#    endif
public:
  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  void insert(iterator __p, _InputIter __first, _InputIter __last) {
    typedef typename _IsIntegral<_InputIter>::_Ret _Integral;
    _M_insert_dispatch(__p, __first, __last, _Integral());
  }
#  endif
#endif

#if !defined (_STLP_MEMBER_TEMPLATES) || !defined (_STLP_NO_METHOD_SPECIALIZATION)
public:
  void insert(iterator __p, const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    _M_insert(__p, __f, __l, _M_inside(__f));
  }
#endif

public:                         // Erase.
  _Self& erase(size_type __pos = 0, size_type __n = npos) {
    if (__pos > size())
      this->_M_throw_out_of_range();
    erase(begin() + __pos, begin() + __pos + (min) (__n, size() - __pos));
    return *this;
  }

  iterator erase(iterator __pos) {
    // The move includes the terminating _CharT().
    _Traits::move(__pos, __pos + 1, this->_M_Finish() - __pos);
    --this->_M_finish;
    return __pos;
  }

  iterator erase(iterator __first, iterator __last) {
    if (__first != __last) {
      // The move includes the terminating _CharT().
      traits_type::move(__first, __last, (this->_M_Finish() - __last) + 1);
      this->_M_finish = this->_M_Finish() - (__last - __first);
    }
    return __first;
  }

public:                         // Replace.  (Conceptually equivalent
                                // to erase followed by insert.)
  _Self& replace(size_type __pos, size_type __n, const _Self& __s) {
    const size_type __size = size();
    if (__pos > __size)
      this->_M_throw_out_of_range();
    const size_type __len = (min) (__n, __size - __pos);
    if (__s.size() > max_size() - (__size - __len))
      this->_M_throw_length_error();
    return _M_replace(begin() + __pos, begin() + __pos + __len,
                      __s._M_Start(), __s._M_Finish(), &__s == this);
  }

  _Self& replace(size_type __pos1, size_type __n1, const _Self& __s,
                 size_type __pos2, size_type __n2) {
    const size_type __size1 = size();
    const size_type __size2 = __s.size();
    if (__pos1 > __size1 || __pos2 > __size2)
      this->_M_throw_out_of_range();
    const size_type __len1 = (min) (__n1, __size1 - __pos1);
    const size_type __len2 = (min) (__n2, __size2 - __pos2);
    if (__len2 > max_size() - (__size1 - __len1))
      this->_M_throw_length_error();
    return _M_replace(begin() + __pos1, begin() + __pos1 + __len1,
                      __s._M_Start() + __pos2, __s._M_Start() + __pos2 + __len2, &__s == this);
  }

  _Self& replace(size_type __pos, size_type __n1,
                 const _CharT* __s, size_type __n2) {
    _STLP_FIX_LITERAL_BUG(__s)
    const size_type __size = size();
    if (__pos > __size)
      this->_M_throw_out_of_range();
    const size_type __len = (min) (__n1, __size - __pos);
    if (__n2 > max_size() - (__size - __len))
      this->_M_throw_length_error();
    return _M_replace(begin() + __pos, begin() + __pos + __len,
                      __s, __s + __n2, _M_inside(__s));
  }

  _Self& replace(size_type __pos, size_type __n1, const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    return replace(__pos, __n1, __s, _Traits::length(__s));
  }

  _Self& replace(size_type __pos, size_type __n1,
                 size_type __n2, _CharT __c) {
    const size_type __size = size();
    if (__pos > __size)
      this->_M_throw_out_of_range();
    const size_type __len = (min) (__n1, __size - __pos);
    if (__n2 > max_size() - (__size - __len))
      this->_M_throw_length_error();
    return replace(begin() + __pos, begin() + __pos + __len, __n2, __c);
  }

  _Self& replace(iterator __first, iterator __last, const _Self& __s) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    return _M_replace(__first, __last, __s._M_Start(), __s._M_Finish(), &__s == this);
  }

  _Self& replace(iterator __first, iterator __last,
                 const _CharT* __s, size_type __n) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__s)
    return _M_replace(__first, __last, __s, __s + __n, _M_inside(__s));
  }

  _Self& replace(iterator __first, iterator __last,
                 const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__s)
    return _M_replace(__first, __last, __s, __s + _Traits::length(__s), _M_inside(__s));
  }

  _Self& replace(iterator __first, iterator __last, size_type __n, _CharT __c);

_STLP_PRIVATE:                        // Helper functions for replace.
  _Self& _M_replace(iterator __first, iterator __last,
                    const _CharT* __f, const _CharT* __l, bool __self_ref);

#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _Integer>
  _Self& _M_replace_dispatch(iterator __first, iterator __last,
                             _Integer __n, _Integer __x, const __true_type& /*IsIntegral*/) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    return replace(__first, __last, (size_type) __n, (_CharT) __x);
  }

  template <class _InputIter>
  _Self& _M_replace_dispatch(iterator __first, iterator __last,
                             _InputIter __f, _InputIter __l, const __false_type& /*IsIntegral*/) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    /* We are forced to do a temporary string to avoid the self referencing issue. */
    const _Self __self(__f, __l, get_allocator());
    return _M_replace(__first, __last, __self._M_Start(), __self._M_Finish(), false);
  }

public:
  // Check to see if _InputIter is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  _Self& replace(iterator __first, iterator __last,
                 _InputIter __f, _InputIter __l) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    typedef typename _IsIntegral<_InputIter>::_Ret _Integral;
    return _M_replace_dispatch(__first, __last, __f, __l,  _Integral());
  }
#endif

#if !defined (_STLP_MEMBER_TEMPLATES) || !defined (_STLP_NO_METHOD_SPECIALIZATION)
public:
  _Self& replace(iterator __first, iterator __last,
                 const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    return _M_replace(__first, __last, __f, __l, _M_inside(__f));
  }
#endif

public:                         // Other modifier member functions.

  size_type copy(_CharT* __s, size_type __n, size_type __pos = 0) const {
    _STLP_FIX_LITERAL_BUG(__s)
    if (__pos > size())
      this->_M_throw_out_of_range();
    const size_type __len = (min) (__n, size() - __pos);
    _Traits::copy(__s, this->_M_Start() + __pos, __len);
    return __len;
  }

  void swap(_Self& __s) { this->_M_swap(__s); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

public:                         // Conversion to C string.

  const _CharT* c_str() const { return this->_M_Start(); }
  const _CharT* data()  const { return this->_M_Start(); }

public: // find.
  size_type find(const _Self& __s, size_type __pos = 0) const
  { return find(__s._M_Start(), __pos, __s.size()); }

  size_type find(const _CharT* __s, size_type __pos = 0) const
  { _STLP_FIX_LITERAL_BUG(__s) return find(__s, __pos, _Traits::length(__s)); }

  size_type find(const _CharT* __s, size_type __pos, size_type __n) const;

  // WIE: Versant schema compiler 5.2.2 ICE workaround
  size_type find(_CharT __c) const { return find(__c, 0); }
  size_type find(_CharT __c, size_type __pos /* = 0 */) const;

public: // rfind.
  size_type rfind(const _Self& __s, size_type __pos = npos) const
  { return rfind(__s._M_Start(), __pos, __s.size()); }

  size_type rfind(const _CharT* __s, size_type __pos = npos) const
  { _STLP_FIX_LITERAL_BUG(__s) return rfind(__s, __pos, _Traits::length(__s)); }

  size_type rfind(const _CharT* __s, size_type __pos, size_type __n) const;
  size_type rfind(_CharT __c, size_type __pos = npos) const;

public: // find_first_of
  size_type find_first_of(const _Self& __s, size_type __pos = 0) const
  { return find_first_of(__s._M_Start(), __pos, __s.size()); }

  size_type find_first_of(const _CharT* __s, size_type __pos = 0) const
  { _STLP_FIX_LITERAL_BUG(__s) return find_first_of(__s, __pos, _Traits::length(__s)); }

  size_type find_first_of(const _CharT* __s, size_type __pos, size_type __n) const;

  size_type find_first_of(_CharT __c, size_type __pos = 0) const
  { return find(__c, __pos); }

public: // find_last_of
  size_type find_last_of(const _Self& __s, size_type __pos = npos) const
  { return find_last_of(__s._M_Start(), __pos, __s.size()); }

  size_type find_last_of(const _CharT* __s, size_type __pos = npos) const
  { _STLP_FIX_LITERAL_BUG(__s) return find_last_of(__s, __pos, _Traits::length(__s)); }

  size_type find_last_of(const _CharT* __s, size_type __pos, size_type __n) const;

  size_type find_last_of(_CharT __c, size_type __pos = npos) const
  { return rfind(__c, __pos); }

public: // find_first_not_of
  size_type find_first_not_of(const _Self& __s, size_type __pos = 0) const
  { return find_first_not_of(__s._M_Start(), __pos, __s.size()); }

  size_type find_first_not_of(const _CharT* __s, size_type __pos = 0) const
  { _STLP_FIX_LITERAL_BUG(__s) return find_first_not_of(__s, __pos, _Traits::length(__s)); }

  size_type find_first_not_of(const _CharT* __s, size_type __pos, size_type __n) const;

  size_type find_first_not_of(_CharT __c, size_type __pos = 0) const;

public: // find_last_not_of
  size_type find_last_not_of(const _Self& __s, size_type __pos = npos) const
  { return find_last_not_of(__s._M_Start(), __pos, __s.size()); }

  size_type find_last_not_of(const _CharT* __s, size_type __pos = npos) const
  { _STLP_FIX_LITERAL_BUG(__s) return find_last_not_of(__s, __pos, _Traits::length(__s)); }

  size_type find_last_not_of(const _CharT* __s, size_type __pos, size_type __n) const;

  size_type find_last_not_of(_CharT __c, size_type __pos = npos) const;

public: // Substring.
  _Self substr(size_type __pos = 0, size_type __n = npos) const
  { return _Self(*this, __pos, __n, get_allocator()); }

public: // Compare
  int compare(const _Self& __s) const
  { return _M_compare(this->_M_Start(), this->_M_Finish(), __s._M_Start(), __s._M_Finish()); }

  int compare(size_type __pos1, size_type __n1, const _Self& __s) const {
    if (__pos1 > size())
      this->_M_throw_out_of_range();
    return _M_compare(this->_M_Start() + __pos1,
                      this->_M_Start() + __pos1 + (min) (__n1, size() - __pos1),
                      __s._M_Start(), __s._M_Finish());
  }

  int compare(size_type __pos1, size_type __n1, const _Self& __s,
              size_type __pos2, size_type __n2) const {
    if (__pos1 > size() || __pos2 > __s.size())
      this->_M_throw_out_of_range();
    return _M_compare(this->_M_Start() + __pos1,
                      this->_M_Start() + __pos1 + (min) (__n1, size() - __pos1),
                      __s._M_Start() + __pos2,
                      __s._M_Start() + __pos2 + (min) (__n2, __s.size() - __pos2));
  }

  int compare(const _CharT* __s) const {
    _STLP_FIX_LITERAL_BUG(__s)
    return _M_compare(this->_M_Start(), this->_M_Finish(), __s, __s + _Traits::length(__s));
  }

  int compare(size_type __pos1, size_type __n1, const _CharT* __s) const {
    _STLP_FIX_LITERAL_BUG(__s)
    if (__pos1 > size())
      this->_M_throw_out_of_range();
    return _M_compare(this->_M_Start() + __pos1,
                      this->_M_Start() + __pos1 + (min) (__n1, size() - __pos1),
                      __s, __s + _Traits::length(__s));
  }

  int compare(size_type __pos1, size_type __n1, const _CharT* __s, size_type __n2) const {
    _STLP_FIX_LITERAL_BUG(__s)
    if (__pos1 > size())
      this->_M_throw_out_of_range();
    return _M_compare(this->_M_Start() + __pos1,
                      this->_M_Start() + __pos1 + (min) (__n1, size() - __pos1),
                      __s, __s + __n2);
  }

public: // Helper functions for compare.
  static int _STLP_CALL _M_compare(const _CharT* __f1, const _CharT* __l1,
                                   const _CharT* __f2, const _CharT* __l2) {
    const ptrdiff_t __n1 = __l1 - __f1;
    const ptrdiff_t __n2 = __l2 - __f2;
    const int cmp = _Traits::compare(__f1, __f2, (min) (__n1, __n2));
    return cmp != 0 ? cmp : (__n1 < __n2 ? -1 : (__n1 > __n2 ? 1 : 0));
  }
#if defined (_STLP_USE_TEMPLATE_EXPRESSION) && !defined (_STLP_DEBUG) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#  define _STLP_STRING_SUM_BASE(__reserve, __size, __alloc) _STLP_PRIV _String_base<_CharT,_Alloc>(__alloc, __size + 1)
#  include <stl/_string_sum_methods.h>
#  undef _STLP_STRING_SUM_BASE
#endif
};

#undef _STLP_PRIVATE

#if defined (__GNUC__) && (__GNUC__ == 2) && (__GNUC_MINOR__ == 96)
template <class _CharT, class _Traits, class _Alloc>
const size_t basic_string<_CharT, _Traits, _Alloc>::npos = ~(size_t) 0;
#endif

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_string<char, char_traits<char>, allocator<char> >;
#  if defined (_STLP_HAS_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;
#  endif
#endif /* _STLP_USE_TEMPLATE_EXPORT */

#if defined (basic_string)
_STLP_MOVE_TO_STD_NAMESPACE
#  undef basic_string
#endif

_STLP_END_NAMESPACE

#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#  include <stl/_string_workaround.h>
#endif

#if defined (_STLP_DEBUG)
#  include <stl/debug/_string.h>
#endif

_STLP_BEGIN_NAMESPACE

// ------------------------------------------------------------
// Non-member functions.
// Swap.
#if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
template <class _CharT, class _Traits, class _Alloc>
inline void _STLP_CALL
swap(basic_string<_CharT,_Traits,_Alloc>& __x,
     basic_string<_CharT,_Traits,_Alloc>& __y)
{ __x.swap(__y); }
#else
inline void _STLP_CALL swap(string& __x, string& __y)
{ __x.swap(__y); }
#  if defined (_STLP_HAS_WCHAR_T)
inline void _STLP_CALL swap(wstring& __x, wstring& __y)
{ __x.swap(__y); }
#  endif
#endif

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_NO_MOVE_SEMANTIC)
template <class _CharT, class _Traits, class _Alloc>
struct __move_traits<basic_string<_CharT, _Traits, _Alloc> > {
  typedef __true_type implemented;
  //Completness depends on the allocator:
  typedef typename __move_traits<_Alloc>::complete complete;
};
/*#else
 * There is no need to specialize for string and wstring in this case
 * as the default __move_traits will already tell that string is movable
 * but not complete. We cannot define it as complete as nothing guaranty
 * that the STLport user hasn't specialized std::allocator for char or
 * wchar_t.
 */
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _CharT, class _Traits, class _Alloc>
void _STLP_CALL _S_string_copy(const basic_string<_CharT,_Traits,_Alloc>& __s,
                               _CharT* __buf, size_t __n);

#if defined(_STLP_USE_WIDE_INTERFACE)
// A couple of functions to transfer between ASCII/Unicode
wstring __ASCIIToWide(const char *ascii);
string __WideToASCII(const wchar_t *wide);
#endif

inline const char* _STLP_CALL
__get_c_string(const string& __str) { return __str.c_str(); }

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#include <stl/_string_operators.h>

#if defined(_STLP_USE_NO_IOSTREAMS) || \
    (defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION))
#  include <stl/_string.c>
#endif

#endif /* _STLP_INTERNAL_STRING_H */

/*
 * Local Variables:
 * mode:C++
 * End:
 */
