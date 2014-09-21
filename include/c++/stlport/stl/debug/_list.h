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

#ifndef _STLP_INTERNAL_DBG_LIST_H
#define _STLP_INTERNAL_DBG_LIST_H

#ifndef _STLP_INTERNAL_ALGO_H
#  include <stl/_algo.h>
#endif

#ifndef _STLP_DBG_ITERATOR_H
#  include <stl/debug/_iterator.h>
#endif

#define _STLP_NON_DBG_LIST _STLP_PRIV _STLP_NON_DBG_NAME(list) <_Tp, _Alloc>

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS)
template <class _Tp, class _Alloc>
inline _Tp*
value_type(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_LIST >&)
{ return (_Tp*)0; }
template <class _Tp, class _Alloc>
inline bidirectional_iterator_tag
iterator_category(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_LIST >&)
{ return bidirectional_iterator_tag(); }
#endif

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class list :
#if !defined (__DMC__)
             private
#endif
                     _STLP_PRIV __construct_checker<_STLP_NON_DBG_LIST >
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
           , public __stlport_class<list<_Tp, _Alloc> >
#endif
{
  typedef _STLP_NON_DBG_LIST _Base;
  typedef list<_Tp, _Alloc> _Self;
  typedef _STLP_PRIV __construct_checker<_STLP_NON_DBG_LIST > _ConstructCheck;

public:
  __IMPORT_CONTAINER_TYPEDEFS(_Base)

public:
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_Nonconst_traits<value_type> > > iterator;
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_Const_traits<value_type> > >    const_iterator;

  _STLP_DECLARE_BIDIRECTIONAL_REVERSE_ITERATORS;

private:
  _Base _M_non_dbg_impl;
  _STLP_PRIV __owned_list _M_iter_list;

  void _Invalidate_iterator(const iterator& __it)
  { _STLP_PRIV __invalidate_iterator(&_M_iter_list, __it); }
  void _Invalidate_iterators(const iterator& __first, const iterator& __last)
  { _STLP_PRIV __invalidate_range(&_M_iter_list, __first, __last); }

  typedef typename _Base::iterator _Base_iterator;

public:
  explicit list(const allocator_type& __a = allocator_type()) :
    _M_non_dbg_impl(__a), _M_iter_list(&_M_non_dbg_impl) {}

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit list(size_type __n, const _Tp& __x = _Tp(),
#else
  list(size_type __n, const _Tp& __x,
#endif /*!_STLP_DONT_SUP_DFLT_PARAM*/
            const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(__n, __x, __a), _M_iter_list(&_M_non_dbg_impl) {}

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit list(size_type __n)
    : _M_non_dbg_impl(__n), _M_iter_list(&_M_non_dbg_impl) {}
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  list(__move_source<_Self> src)
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
  list(_InputIterator __first, _InputIterator __last,
       const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last), __a),
      _M_iter_list(&_M_non_dbg_impl) {}
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  list(_InputIterator __first, _InputIterator __last)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last)),
      _M_iter_list(&_M_non_dbg_impl) {}
#  endif
#else

  list(const value_type* __first, const value_type* __last,
       const allocator_type& __a = allocator_type())
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first, __last, __a),
      _M_iter_list(&_M_non_dbg_impl) {}
  list(const_iterator __first, const_iterator __last,
       const allocator_type& __a = allocator_type())
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first._M_iterator, __last._M_iterator, __a),
      _M_iter_list(&_M_non_dbg_impl) {}

#endif

  list(const _Self& __x) :
    _ConstructCheck(__x),
    _M_non_dbg_impl(__x._M_non_dbg_impl) , _M_iter_list(&_M_non_dbg_impl) {}

  _Self& operator=(const _Self& __x) {
    if (this != &__x) {
      //Should not invalidate end iterator
      _Invalidate_iterators(begin(), end());
      _M_non_dbg_impl = __x._M_non_dbg_impl;
    }
    return *this;
  }

  allocator_type get_allocator() const { return _M_non_dbg_impl.get_allocator(); }

  iterator begin()             { return iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  const_iterator begin() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }

  iterator end()               { return iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  const_iterator end() const   { return const_iterator(&_M_iter_list, _M_non_dbg_impl.end()); }

  reverse_iterator rbegin()    { return reverse_iterator(end()); }
  reverse_iterator rend()      { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  size_type size() const { return _M_non_dbg_impl.size(); }
  size_type max_size() const { return _M_non_dbg_impl.max_size(); }
  bool empty() const { return _M_non_dbg_impl.empty(); }

  // those are here to enforce checking
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

#if !defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos, const _Tp& __x = _Tp()) {
#else
  iterator insert(iterator __pos, const _Tp& __x) {
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    return iterator(&_M_iter_list,_M_non_dbg_impl.insert(__pos._M_iterator, __x) );
  }

#if defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos) { return insert(__pos, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(iterator __pos, _InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.insert(__pos._M_iterator,
                           _STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#endif

#if !defined (_STLP_MEMBER_TEMPLATES)
  void insert(iterator __pos, const _Tp* __first, const _Tp* __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    _M_non_dbg_impl.insert(__pos._M_iterator, __first, __last);
  }
#endif

#if !defined (_STLP_MEMBER_TEMPLATES) || !defined (_STLP_NO_METHOD_SPECIALIZATION)
  void insert(iterator __pos,
              const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
#  if (_STLP_DEBUG_LEVEL == _STLP_STANDARD_DBG_LEVEL)
    _STLP_STD_DEBUG_CHECK(__check_if_not_owner(&_M_iter_list, __first))
#  endif
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
  }
  void insert(iterator __pos,
              iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
#  if (_STLP_DEBUG_LEVEL == _STLP_STANDARD_DBG_LEVEL)
    _STLP_STD_DEBUG_CHECK(__check_if_not_owner(&_M_iter_list, __first))
#  endif
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
  }
#endif

  void insert(iterator __pos, size_type __n, const _Tp& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _M_non_dbg_impl.insert(__pos._M_iterator, __n, __x);
  }

  void push_back(const_reference __x) { _M_non_dbg_impl.push_back(__x); }
  void pop_back() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    _Invalidate_iterator(end());
    _M_non_dbg_impl.pop_back();
  }

  void push_front(const_reference __x) { _M_non_dbg_impl.push_front(__x); }
  void pop_front() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    _Invalidate_iterator(begin());
    _M_non_dbg_impl.pop_front();
  }

  iterator erase(iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _Invalidate_iterator(__pos);
    return iterator(&_M_iter_list,_M_non_dbg_impl.erase(__pos._M_iterator));
  }
  iterator erase(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last, begin(), end()))
    _Invalidate_iterators(__first, __last);
    return iterator (&_M_iter_list, _M_non_dbg_impl.erase(__first._M_iterator, __last._M_iterator));
  }

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const _Tp& __x = _Tp()) {
#else
  void resize(size_type __new_size, const _Tp& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    _Base_iterator __i = _M_non_dbg_impl.begin();
    size_type __len = 0;
    for ( ; __i != _M_non_dbg_impl.end() && __len < __new_size; ++__i, ++__len);

    if (__len == __new_size)
      erase(iterator(&_M_iter_list, __i), end());
    else                          // __i == end()
      _M_non_dbg_impl.insert(_M_non_dbg_impl.end(), __new_size - __len, __x);
  }

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size) { resize(__new_size, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.assign(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
    _Invalidate_iterators(begin(), end());
  }
#else
  void assign(const _Tp* __first, const _Tp* __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    _M_non_dbg_impl.assign(__first, __last);
    _Invalidate_iterators(begin(), end());
  }

  void assign(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.assign(__first._M_iterator, __last._M_iterator);
    _Invalidate_iterators(begin(), end());
  }

  void assign(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.assign(__first._M_iterator, __last._M_iterator);
    _Invalidate_iterators(begin(), end());
  }
#endif

  void assign(size_type __n, const _Tp& __val) {
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.assign(__n, __val);
  }

  void remove(const _Tp& __x) {
    _Base_iterator __first = _M_non_dbg_impl.begin(), __last = _M_non_dbg_impl.end();
    while (__first != __last) {
      _Base_iterator __next = __first;
      ++__next;
      if (__x == *__first) {
        _Invalidate_iterator(iterator(&_M_iter_list, __first));
        _M_non_dbg_impl.erase(__first);
      }
      __first = __next;
    }
  }

  void clear() {
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.clear();
  }

public:
  void splice(iterator __pos, _Self& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _M_non_dbg_impl.splice(__pos._M_iterator, __x._M_non_dbg_impl);
#if (_STLP_DEBUG_LEVEL != _STLP_STANDARD_DBG_LEVEL)
    if (get_allocator() == __x.get_allocator())
      __x._M_iter_list._Set_owner(_M_iter_list);
    else
#endif
      // Std: 23.2.2.4:4
      // end iterator is not invalidated:
      __x._Invalidate_iterators(__x.begin(), __x.end());
  }

  void splice(iterator __pos, _Self& __x, iterator __i) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__i))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&(__x._M_iter_list),__i))
    _M_non_dbg_impl.splice(__pos._M_iterator, __x._M_non_dbg_impl, __i._M_iterator);
#if (_STLP_DEBUG_LEVEL != _STLP_STANDARD_DBG_LEVEL)
    if (get_allocator() == __x.get_allocator())
      _STLP_PRIV __change_ite_owner(__i, &_M_iter_list);
    else
#endif
      // Std: 23.2.2.4:7
      __x._Invalidate_iterator(__i);
  }

  void splice(iterator __pos, _Self& __x, iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last, __x.begin(), __x.end()))
    _STLP_DEBUG_CHECK(this == &__x ? !_STLP_PRIV __check_range(__pos, __first, __last) : true)
#if (_STLP_DEBUG_LEVEL != _STLP_STANDARD_DBG_LEVEL)
    if (this->get_allocator() == __x.get_allocator())
      _STLP_PRIV __change_range_owner(__first, __last, &_M_iter_list);
    else
#endif
      // Std: 23.2.2.4:12
      __x._Invalidate_iterators(__first, __last);
    _M_non_dbg_impl.splice(__pos._M_iterator, __x._M_non_dbg_impl, __first._M_iterator, __last._M_iterator);
  }

  void merge(_Self& __x) {
#if !defined (_STLP_NO_EXTENSIONS)
    _STLP_DEBUG_CHECK(_STLP_STD::is_sorted(begin()._M_iterator, end()._M_iterator))
    _STLP_DEBUG_CHECK(_STLP_STD::is_sorted(__x.begin()._M_iterator, __x.end()._M_iterator))
#endif
    _M_non_dbg_impl.merge(__x._M_non_dbg_impl);
    if (this->get_allocator() == __x.get_allocator()) {
      __x._M_iter_list._Set_owner(_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__x.begin(), __x.end());
    }
  }
  void reverse() {
    _M_non_dbg_impl.reverse();
  }
  void unique() {
    _Base_iterator __first = _M_non_dbg_impl.begin(), __last = _M_non_dbg_impl.end();
    if (__first == __last) return;
    _Base_iterator __next = __first;
    while (++__next != __last) {
      if (*__first == *__next) {
        _Invalidate_iterator(iterator(&_M_iter_list, __next));
        _M_non_dbg_impl.erase(__next);
      }
      else
        __first = __next;
      __next = __first;
    }
  }
  void sort() {
    _M_non_dbg_impl.sort();
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _Predicate>
  void remove_if(_Predicate __pred) {
    _Base_iterator __first = _M_non_dbg_impl.begin(), __last = _M_non_dbg_impl.end();
    while (__first != __last) {
      _Base_iterator __next = __first;
      ++__next;
      if (__pred(*__first)) {
        _Invalidate_iterator(iterator(&_M_iter_list, __first));
        _M_non_dbg_impl.erase(__first);
      }
      __first = __next;
    }
  }

  template <class _BinaryPredicate>
  void unique(_BinaryPredicate __binary_pred) {
    _Base_iterator __first = _M_non_dbg_impl.begin(), __last = _M_non_dbg_impl.end();
    if (__first == __last) return;
    _Base_iterator __next = __first;
    while (++__next != __last) {
      if (__binary_pred(*__first, *__next)) {
        _Invalidate_iterator(iterator(&_M_iter_list, __next));
        _M_non_dbg_impl.erase(__next);
      }
      else
        __first = __next;
      __next = __first;
    }
  }

  template <class _StrictWeakOrdering>
  void merge(_Self& __x, _StrictWeakOrdering __comp) {
#if !defined (_STLP_NO_EXTENSIONS)
    _STLP_DEBUG_CHECK(_STLP_STD::is_sorted(_M_non_dbg_impl.begin(), _M_non_dbg_impl.end(), __comp))
    _STLP_DEBUG_CHECK(_STLP_STD::is_sorted(__x.begin()._M_iterator, __x.end()._M_iterator, __comp))
#endif
    _M_non_dbg_impl.merge(__x._M_non_dbg_impl, __comp);
    if (this->get_allocator() == __x.get_allocator()) {
      __x._M_iter_list._Set_owner(_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__x.begin(), __x.end());
    }
  }

  template <class _StrictWeakOrdering>
  void sort(_StrictWeakOrdering __comp) {
    _M_non_dbg_impl.sort(__comp);
  }
#endif
};


_STLP_END_NAMESPACE

#undef _STLP_NON_DBG_LIST

#endif /* _STLP_INTERNAL_LIST_H */

// Local Variables:
// mode:C++
// End:
