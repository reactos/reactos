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

#ifndef _STLP_INTERNAL_DBG_DEQUE_H
#define _STLP_INTERNAL_DBG_DEQUE_H

#ifndef _STLP_DBG_ITERATOR_H
#  include <stl/debug/_iterator.h>
#endif

#define _STLP_NON_DBG_DEQUE _STLP_PRIV _STLP_NON_DBG_NAME(deque) <_Tp,_Alloc>

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS)
template <class _Tp, class _Alloc>
inline _Tp* value_type(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_DEQUE >&)
{ return (_Tp*)0; }
template <class _Tp, class _Alloc>
inline random_access_iterator_tag iterator_category(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_DEQUE >&)
{ return random_access_iterator_tag(); }
#endif

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class deque :
#if !defined (__DMC__)
             private
#endif
                     _STLP_PRIV __construct_checker<_STLP_NON_DBG_DEQUE >
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
            , public __stlport_class<deque<_Tp, _Alloc> >
#endif
{
  typedef deque<_Tp,_Alloc> _Self;
  typedef _STLP_NON_DBG_DEQUE _Base;
  typedef _STLP_PRIV __construct_checker<_STLP_NON_DBG_DEQUE > _ConstructCheck;

public:
  // Basic types
  __IMPORT_CONTAINER_TYPEDEFS(_Base)

  // Iterators
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_Nonconst_traits<value_type> > > iterator;
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_Const_traits<value_type> > >    const_iterator;

  _STLP_DECLARE_RANDOM_ACCESS_REVERSE_ITERATORS;

protected:
  _Base _M_non_dbg_impl;
  _STLP_PRIV __owned_list _M_iter_list;

  void _Invalidate_all()
  { _M_iter_list._Invalidate_all(); }
  void _Invalidate_iterator(const iterator& __it)
  { _STLP_PRIV __invalidate_iterator(&_M_iter_list,__it); }
  void _Invalidate_iterators(const iterator& __first, const iterator& __last)
  { _STLP_PRIV __invalidate_range(&_M_iter_list, __first, __last); }

public:
  // Basic accessors
  allocator_type get_allocator() const { return _M_non_dbg_impl.get_allocator(); }

  iterator begin() { return iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  iterator end() { return iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  const_iterator begin() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  const_iterator end() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.end()); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

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

  // Constructor, destructor.
  explicit deque(const allocator_type& __a = allocator_type()) :
    _M_non_dbg_impl(__a), _M_iter_list(&_M_non_dbg_impl) {}
  deque(const _Self& __x) :
    _ConstructCheck(__x), _M_non_dbg_impl(__x._M_non_dbg_impl),
    _M_iter_list(&_M_non_dbg_impl) {}

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit deque(size_type __n, const value_type& __x = _Tp(),
#else
  deque(size_type __n, const value_type& __x,
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
            const allocator_type& __a = allocator_type()) :
    _M_non_dbg_impl(__n, __x, __a), _M_iter_list(&_M_non_dbg_impl) {}
#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit deque(size_type __n) :
    _M_non_dbg_impl(__n), _M_iter_list(&_M_non_dbg_impl) {}
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  deque(__move_source<_Self> src)
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
  deque(_InputIterator __first, _InputIterator __last,
        const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last), __a),
      _M_iter_list(&_M_non_dbg_impl) {
    }
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  deque(_InputIterator __first, _InputIterator __last)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last)),
      _M_iter_list(&_M_non_dbg_impl) {
    }
#  endif
#else
  deque(const value_type* __first, const value_type* __last,
        const allocator_type& __a = allocator_type())
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first, __last, __a),
      _M_iter_list(&_M_non_dbg_impl) {
    }

  deque(const_iterator __first, const_iterator __last,
        const allocator_type& __a = allocator_type())
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first._M_iterator, __last._M_iterator, __a),
      _M_iter_list(&_M_non_dbg_impl) {
    }
#endif

  _Self& operator=(const _Self& __x) {
    if (this != &__x) {
      _Invalidate_all();
      _M_non_dbg_impl = __x._M_non_dbg_impl;
    }
    return *this;
  }

  bool empty() const { return _M_non_dbg_impl.empty(); }
  size_type size() const { return _M_non_dbg_impl.size(); }
  size_type max_size() const { return _M_non_dbg_impl.max_size(); }

  void swap(_Self& __x) {
    _M_iter_list._Swap_owners(__x._M_iter_list);
    _M_non_dbg_impl.swap(__x._M_non_dbg_impl);
  }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

public:
  void assign(size_type __n, const _Tp& __val) {
    _Invalidate_all();
    _M_non_dbg_impl.assign(__n, __val);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _Invalidate_all();
    _M_non_dbg_impl.assign(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#else
  void assign(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _Invalidate_all();
    _M_non_dbg_impl.assign(__first._M_iterator, __last._M_iterator);
  }
  void assign(const value_type *__first, const value_type *__last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    _Invalidate_all();
    _M_non_dbg_impl.assign(__first, __last);
  }
#endif

public:                         // push_* and pop_*

#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_back(const value_type& __t = _Tp()) {
#else
  void push_back(const value_type& __t) {
#endif
    _Invalidate_all();
    _M_non_dbg_impl.push_back(__t);
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_back() {
    _Invalidate_all();
    _M_non_dbg_impl.push_back();
  }
#endif

#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_front(const value_type& __t = _Tp()) {
#else
  void push_front(const value_type& __t) {
#endif
    _Invalidate_all();
    _M_non_dbg_impl.push_front(__t);
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_front() {
    _Invalidate_all();
    _M_non_dbg_impl.push_front();
  }
#endif

  void pop_back() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    _Invalidate_iterator(end());
    _M_non_dbg_impl.pop_back();
  }

  void pop_front() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    _Invalidate_iterator(begin());
    _M_non_dbg_impl.pop_front();
  }

public:                         // Insert

#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos, const value_type& __x = _Tp()) {
#else
  iterator insert(iterator __pos, const value_type& __x) {
#endif
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _Invalidate_all();
    return iterator(&_M_iter_list, _M_non_dbg_impl.insert(__pos._M_iterator, __x));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _Invalidate_all();
    return iterator(&_M_iter_list, _M_non_dbg_impl.insert(__pos._M_iterator));
  }
#endif

  void insert(iterator __pos, size_type __n, const value_type& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    if (__n != 0) _Invalidate_all();
    _M_non_dbg_impl.insert(__pos._M_iterator, __n, __x);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(iterator __pos, _InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    // We perform invalidate first to detect self referencing in __check_range as __first and __last
    // will have been invalidated.
    if (__first != __last) _Invalidate_all();
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.insert(__pos._M_iterator,
                           _STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#endif

#if !defined (_STLP_MEMBER_TEMPLATES)
  void insert(iterator __pos,
              const value_type* __first, const value_type* __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    if (__first != __last) _Invalidate_all();
    _M_non_dbg_impl.insert(__pos._M_iterator, __first, __last);
  }
#endif

#if !defined (_STLP_MEMBER_TEMPLATES) || !defined (_STLP_NO_METHOD_SPECIALIZATION)
  void insert(iterator __pos,
              const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    //Sequence requirements 23.1.1 Table 67:
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_not_owner(&_M_iter_list, __first));
    if (__first != __last) _Invalidate_all();
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
  }

  void insert(iterator __pos,
              iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    //Sequence requirements 23.1.1 Table 67:
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_not_owner(&_M_iter_list, __first));
    if (__first != __last) _Invalidate_all();
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
  }
#endif

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const value_type& __x = _Tp()) {
#else
  void resize(size_type __new_size, const value_type& __x) {
#endif
    if (__new_size != size()) {
      if ((__new_size > size()) || (__new_size < size() - 1))
        _Invalidate_all();
      else
        _Invalidate_iterator(end());
    }
    _M_non_dbg_impl.resize(__new_size, __x);
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type new_size) { resize(new_size, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif

  // Erase
  iterator erase(iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    if (__pos._M_iterator == _M_non_dbg_impl.begin()) {
      _Invalidate_iterator(__pos);
    } else {
      typename _Base::iterator tmp = --(_M_non_dbg_impl.end());
      if (__pos._M_iterator == tmp)
        _Invalidate_iterator(__pos);
      else
        _Invalidate_all();
    }
    return iterator (&_M_iter_list, _M_non_dbg_impl.erase(__pos._M_iterator));
  }

  iterator erase(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last, begin(), end()))
    if (!empty()) {
      if (__first._M_iterator == _M_non_dbg_impl.begin() ||
          __last._M_iterator == _M_non_dbg_impl.end())
        _Invalidate_iterators(__first, __last);
      else
        _Invalidate_all();
    }
    return iterator (&_M_iter_list, _M_non_dbg_impl.erase(__first._M_iterator, __last._M_iterator));
  }

  void clear() {
    _Invalidate_all();
    _M_non_dbg_impl.clear();
  }
};

_STLP_END_NAMESPACE

#undef _STLP_NON_DBG_DEQUE

#endif /* _STLP_INTERNAL_DEQUE_H */

// Local Variables:
// mode:C++
// End:
