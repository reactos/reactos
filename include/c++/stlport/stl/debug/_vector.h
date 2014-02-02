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

#ifndef _STLP_INTERNAL_DBG_VECTOR_H
#define _STLP_INTERNAL_DBG_VECTOR_H

#ifndef _STLP_DBG_ITERATOR_H
#  include <stl/debug/_iterator.h>
#endif

#define _STLP_NON_DBG_VECTOR _STLP_PRIV _STLP_NON_DBG_NAME(vector) <_Tp, _Alloc>

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS)
template <class _Tp, class _Alloc>
inline _Tp*
value_type(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_VECTOR >&)
{ return (_Tp*)0; }
template <class _Tp, class _Alloc>
inline random_access_iterator_tag
iterator_category(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_VECTOR >&)
{ return random_access_iterator_tag(); }
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp, class _NcIt>
struct _Vector_const_traits;

template <class _Tp, class _NcIt>
struct _Vector_nonconst_traits {
  typedef _Nonconst_traits<_Tp> _BaseT;
  typedef _Tp value_type;
  typedef _Tp& reference;
  typedef _Tp* pointer;
  typedef _Vector_const_traits<_Tp, _NcIt> _ConstTraits;
  typedef _Vector_nonconst_traits<_Tp, _NcIt> _NonConstTraits;
};

template <class _Tp, class _NcIt>
struct _Vector_const_traits {
  typedef _Const_traits<_Tp> _BaseT;
  typedef _Tp value_type;
  typedef const _Tp& reference;
  typedef const _Tp* pointer;
  typedef _Vector_const_traits<_Tp, _NcIt> _ConstTraits;
  typedef _Vector_nonconst_traits<_Tp, _NcIt> _NonConstTraits;
};

_STLP_TEMPLATE_NULL
struct _Vector_nonconst_traits<bool, _Bit_iterator> {
  typedef _Bit_iterator::value_type value_type;
  typedef _Bit_iterator::reference reference;
  typedef _Bit_iterator::pointer pointer;
  typedef _Vector_const_traits<bool, _Bit_iterator> _ConstTraits;
  typedef _Vector_nonconst_traits<bool, _Bit_iterator> _NonConstTraits;
};

_STLP_TEMPLATE_NULL
struct _Vector_const_traits<bool, _Bit_iterator> {
  typedef _Bit_const_iterator::value_type value_type;
  typedef _Bit_const_iterator::reference reference;
  typedef _Bit_const_iterator::pointer pointer;
  typedef _Vector_const_traits<bool, _Bit_iterator> _ConstTraits;
  typedef _Vector_nonconst_traits<bool, _Bit_iterator> _NonConstTraits;
};

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class vector :
#if !defined (__DMC__)
             private
#endif
                     _STLP_PRIV __construct_checker< _STLP_NON_DBG_VECTOR >
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
             , public __stlport_class<vector<_Tp, _Alloc> >
#endif
{
private:
  typedef _STLP_NON_DBG_VECTOR _Base;
  typedef vector<_Tp, _Alloc> _Self;
  typedef _STLP_PRIV __construct_checker<_STLP_NON_DBG_VECTOR > _ConstructCheck;
  _Base _M_non_dbg_impl;
  _STLP_PRIV __owned_list _M_iter_list;

public:
  __IMPORT_CONTAINER_TYPEDEFS(_Base)

  typedef _STLP_PRIV _DBG_iter<_Base,
    _STLP_PRIV _DbgTraits<_STLP_PRIV _Vector_nonconst_traits<value_type, typename _Base::iterator> > > iterator;

  typedef _STLP_PRIV _DBG_iter<_Base,
    _STLP_PRIV _DbgTraits<_STLP_PRIV _Vector_const_traits<value_type, typename _Base::iterator> > > const_iterator;

private:
  void _Invalidate_all()
  { _M_iter_list._Invalidate_all(); }
  void _Invalidate_iterator(const iterator& __it)
  { _STLP_PRIV __invalidate_iterator(&_M_iter_list, __it); }
  void _Invalidate_iterators(const iterator& __first, const iterator& __last)
  { _STLP_PRIV __invalidate_range(&_M_iter_list, __first, __last); }

  void _Check_Overflow(size_type __nb) {
    if (size() + __nb > capacity())
      _Invalidate_all();
  }
  void _Compare_Capacity (size_type __old_capacity) {
    if (capacity() > __old_capacity) {
      _Invalidate_all();
    }
  }

public:
  _STLP_DECLARE_RANDOM_ACCESS_REVERSE_ITERATORS;

  allocator_type get_allocator() const { return _M_non_dbg_impl.get_allocator(); }

  iterator begin()             { return iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  const_iterator begin() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  iterator end()               { return iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  const_iterator end() const   { return const_iterator(&_M_iter_list, _M_non_dbg_impl.end()); }

  reverse_iterator rbegin()             { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  reverse_iterator rend()               { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const   { return const_reverse_iterator(begin()); }

  size_type size() const        { return _M_non_dbg_impl.size(); }
  size_type max_size() const    { return _M_non_dbg_impl.max_size(); }
  size_type capacity() const    { return _M_non_dbg_impl.capacity(); }
  bool empty() const            { return _M_non_dbg_impl.empty(); }

  reference operator[](size_type __n) {
    _STLP_VERBOSE_ASSERT(__n < size(), _StlMsg_OUT_OF_BOUNDS)
    return _M_non_dbg_impl[__n];
  }

  const_reference operator[](size_type __n) const {
    _STLP_VERBOSE_ASSERT(__n < size(), _StlMsg_OUT_OF_BOUNDS)
    return _M_non_dbg_impl[__n];
  }

  reference at(size_type __n) { return _M_non_dbg_impl.at(__n); }
  const_reference at(size_type __n) const { return _M_non_dbg_impl.at(__n); }

  explicit vector(const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(__a), _M_iter_list(&_M_non_dbg_impl)  {}

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit vector(size_type __n, const _Tp& __x = _Tp(),
#else
  vector(size_type __n, const _Tp& __x,
#endif
         const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(__n, __x, __a), _M_iter_list(&_M_non_dbg_impl) {}

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit vector(size_type __n)
    : _M_non_dbg_impl(__n), _M_iter_list(&_M_non_dbg_impl) {}
#endif

  vector(const _Self& __x)
    : _ConstructCheck(__x), _M_non_dbg_impl(__x._M_non_dbg_impl), _M_iter_list(&_M_non_dbg_impl) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  vector(__move_source<_Self> src)
    : _M_non_dbg_impl(__move_source<_Base>(src.get()._M_non_dbg_impl)),
      _M_iter_list(&_M_non_dbg_impl) {
#  if defined (_STLP_NO_EXTENSIONS) || (_STLP_DEBUG_LEVEL == _STLP_STANDARD_DBG_LEVEL)
    src.get()._M_iter_list._Invalidate_all();
#  else
    src.get()._M_iter_list._Set_owner(_M_iter_list);
#  endif
  }
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  vector(_InputIterator __first, _InputIterator __last,
         const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last), __a),
      _M_iter_list(&_M_non_dbg_impl) {}

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  vector(_InputIterator __first, _InputIterator __last)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last)),
      _M_iter_list(&_M_non_dbg_impl) {}
#  endif
#else
  vector(const _Tp* __first, const _Tp* __last,
         const allocator_type& __a = allocator_type())
    : _ConstructCheck(__first, __last), _M_non_dbg_impl(__first, __last, __a),
    _M_iter_list(&_M_non_dbg_impl) {}

  // mysterious VC++ bug ?
  vector(const_iterator __first, const_iterator __last ,
         const allocator_type& __a = allocator_type())
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first._M_iterator, __last._M_iterator, __a),
    _M_iter_list(&_M_non_dbg_impl) {}
#endif /* _STLP_MEMBER_TEMPLATES */

  _Self& operator=(const _Self& __x) {
    if (this != &__x) {
      _Invalidate_all();
      _M_non_dbg_impl = __x._M_non_dbg_impl;
    }
    return *this;
  }

  void reserve(size_type __n) {
    if (capacity() < __n)
      _Invalidate_all();
    _M_non_dbg_impl.reserve(__n);
  }

  reference front() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    return *begin();
  }
  const_reference front() const {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    return *begin();
  }
  reference back() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    return *(--end());
  }
  const_reference back() const {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    return *(--end());
  }

  void swap(_Self& __x) {
    _M_iter_list._Swap_owners(__x._M_iter_list);
    _M_non_dbg_impl.swap(__x._M_non_dbg_impl);
  }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert(iterator __pos, const _Tp& __x = _Tp()) {
#else
  iterator insert(iterator __pos, const _Tp& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _Check_Overflow(1);
    return iterator(&_M_iter_list, _M_non_dbg_impl.insert(__pos._M_iterator, __x));
  }

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert(iterator __pos)
  { return insert(__pos, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if defined (_STLP_MEMBER_TEMPLATES)
  // Check whether it's an integral type.  If so, it's not an iterator.
  template <class _InputIterator>
  void insert(iterator __pos,
              _InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    size_type __old_capacity = capacity();
    _M_non_dbg_impl.insert(__pos._M_iterator,
                           _STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
    _Compare_Capacity(__old_capacity);
  }
#endif
#if !defined (_STLP_MEMBER_TEMPLATES)
  void insert (iterator __pos,
               const value_type *__first, const value_type *__last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first,__last))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    size_type __old_capacity = capacity();
    _M_non_dbg_impl.insert(__pos._M_iterator, __first, __last);
    _Compare_Capacity(__old_capacity);
  }
#endif

#if !defined (_STLP_MEMBER_TEMPLATES) || !defined (_STLP_NO_METHOD_SPECIALIZATION)
  void insert(iterator __pos,
              const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first,__last))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    //Sequence requirements 23.1.1 Table 67:
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_not_owner(&_M_iter_list, __first));
    size_type __old_capacity = capacity();
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
    _Compare_Capacity(__old_capacity);
  }
  void insert(iterator __pos,
              iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first,__last))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    //Sequence requirements 23.1.1 Table 67:
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_not_owner(&_M_iter_list, __first));
    size_type __old_capacity = capacity();
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
    _Compare_Capacity(__old_capacity);
  }
#endif

  void insert (iterator __pos, size_type __n, const _Tp& __x){
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _Check_Overflow(__n);
    _M_non_dbg_impl.insert(__pos._M_iterator, __n, __x);
  }

  void pop_back() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    _Invalidate_iterator(end());
    _M_non_dbg_impl.pop_back();
  }
  iterator erase(iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _Invalidate_iterators(__pos, end());
    return iterator(&_M_iter_list, _M_non_dbg_impl.erase(__pos._M_iterator));
  }
  iterator erase(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last, begin(), end()))
    _Invalidate_iterators(__first, end());
    return iterator(&_M_iter_list, _M_non_dbg_impl.erase(__first._M_iterator, __last._M_iterator));
  }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const _Tp& __x = _STLP_DEFAULT_CONSTRUCTED(_Tp)) {
#else
  void resize(size_type __new_size, const _Tp& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    if (__new_size > capacity()) {
      _Invalidate_all();
    }
    else if (__new_size < size()) {
      _Invalidate_iterators(begin() + __new_size, end());
    }
    _M_non_dbg_impl.resize(__new_size, __x);
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size) { resize(__new_size, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first,__last))
    _Invalidate_all();
    _M_non_dbg_impl.assign(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#else
private:
  void _M_assign(const value_type *__first, const value_type *__last) {
    _Invalidate_all();
    _M_non_dbg_impl.assign(__first, __last);
  }
public:
  void assign(const value_type *__first, const value_type *__last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first,__last))
    _M_assign(__first, __last);
  }

  void assign(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first,__last))
    _M_assign(__first._M_iterator, __last._M_iterator);
  }
#endif

public:
  void assign(size_type __n, const _Tp& __val) {
    _Invalidate_all();
    _M_non_dbg_impl.assign(__n, __val);
  }

  void clear() {
    _Invalidate_all();
    _M_non_dbg_impl.clear();
  }
  void push_back(const _Tp& __x) {
    _Check_Overflow(1);
    _M_non_dbg_impl.push_back(__x);
  }
};

_STLP_END_NAMESPACE

#undef _STLP_NON_DBG_VECTOR

#endif /* _STLP_DBG_VECTOR_H */

// Local Variables:
// mode:C++
// End:
