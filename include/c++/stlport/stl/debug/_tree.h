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

#ifndef _STLP_INTERNAL_DBG_TREE_H
#define _STLP_INTERNAL_DBG_TREE_H

#ifndef _STLP_DBG_ITERATOR_H
#  include <stl/debug/_iterator.h>
#endif

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Key, class _Compare>
class _DbgCompare {
public:
  _DbgCompare() {}
  _DbgCompare(const _Compare& __cmp) : _M_non_dbg_cmp(__cmp) {}
  _DbgCompare(const _DbgCompare& __cmp) : _M_non_dbg_cmp(__cmp._M_non_dbg_cmp) {}

#if !defined (_STLP_USE_CONTAINERS_EXTENSION)
  bool operator () (const _Key& __lhs, const _Key& __rhs) const {
#else
  template <class _Kp1, class _Kp2>
  bool operator () (const _Kp1& __lhs, const _Kp2& __rhs) const {
#endif
    if (_M_non_dbg_cmp(__lhs, __rhs)) {
      return true;
    }
    return false;
  }

  _Compare non_dbg_key_comp() const { return _M_non_dbg_cmp; }
private:
  _Compare _M_non_dbg_cmp;
};

#define _STLP_NON_DBG_TREE _STLP_PRIV _STLP_NON_DBG_NAME(Rb_tree) <_Key, _STLP_PRIV _DbgCompare<_Key, _Compare>, _Value, _KeyOfValue, _Traits, _Alloc>

#if defined (_STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS)
_STLP_MOVE_TO_STD_NAMESPACE
template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc >
inline _Value*
value_type(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_TREE >&)
{ return (_Value*)0; }
template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc >
inline bidirectional_iterator_tag
iterator_category(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_TREE >&)
{ return bidirectional_iterator_tag(); }
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits,
          _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Value>) >
class _Rb_tree {
  typedef _STLP_NON_DBG_TREE _Base;
  typedef _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc> _Self;
  _Base _M_non_dbg_impl;
  _STLP_PRIV __owned_list _M_iter_list;

public:
  __IMPORT_CONTAINER_TYPEDEFS(_Base)
  typedef typename _Base::key_type key_type;

  typedef typename _Traits::_NonConstTraits _NonConstIteTraits;
  typedef typename _Traits::_ConstTraits    _ConstIteTraits;
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_NonConstIteTraits> > iterator;
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_ConstIteTraits> >    const_iterator;

  _STLP_DECLARE_BIDIRECTIONAL_REVERSE_ITERATORS;

private:
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)
  void _Invalidate_iterator(const iterator& __it)
  { _STLP_PRIV __invalidate_iterator(&_M_iter_list,__it); }
  void _Invalidate_iterators(const iterator& __first, const iterator& __last)
  { _STLP_PRIV __invalidate_range(&_M_iter_list, __first, __last); }

  typedef typename _Base::iterator _Base_iterator;
  typedef typename _Base::const_iterator _Base_const_iterator;

public:
  _Rb_tree()
    : _M_non_dbg_impl(), _M_iter_list(&_M_non_dbg_impl) {}
  _Rb_tree(const _Compare& __comp)
    : _M_non_dbg_impl(__comp), _M_iter_list(&_M_non_dbg_impl) {}
  _Rb_tree(const _Compare& __comp, const allocator_type& __a)
    : _M_non_dbg_impl(__comp, __a), _M_iter_list(&_M_non_dbg_impl) {}
  _Rb_tree(const _Self& __x)
    : _M_non_dbg_impl(__x._M_non_dbg_impl), _M_iter_list(&_M_non_dbg_impl) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  _Rb_tree(__move_source<_Self> src):
    _M_non_dbg_impl(__move_source<_Base>(src.get()._M_non_dbg_impl)),
    _M_iter_list(&_M_non_dbg_impl) {
#  if defined (_STLP_NO_EXTENSIONS) || (_STLP_DEBUG_LEVEL == _STLP_STANDARD_DBG_LEVEL)
    src.get()._M_iter_list._Invalidate_all();
#  else
    src.get()._M_iter_list._Set_owner(_M_iter_list);
#  endif
  }
#endif

  ~_Rb_tree() {}

  _Self& operator=(const _Self& __x) {
    if (this != &__x) {
      //Should not invalidate end iterator:
      _Invalidate_iterators(begin(), end());
      _M_non_dbg_impl = __x._M_non_dbg_impl;
    }
    return *this;
  }

  allocator_type get_allocator() const { return _M_non_dbg_impl.get_allocator(); }
  _Compare key_comp() const { return _M_non_dbg_impl.key_comp().non_dbg_key_comp(); }

  iterator begin() { return iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  const_iterator begin() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  iterator end() { return iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  const_iterator end() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.end()); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  bool empty() const { return _M_non_dbg_impl.empty(); }
  size_type size() const { return _M_non_dbg_impl.size(); }
  size_type max_size() const { return _M_non_dbg_impl.max_size(); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __x) const { return _M_non_dbg_impl.count(__x); }

  void swap(_Self& __t) {
    _M_non_dbg_impl.swap(__t._M_non_dbg_impl);
    _M_iter_list._Swap_owners(__t._M_iter_list);
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __k)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.find(__k)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __k) const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.find(__k)); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator lower_bound(const _KT& __x)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.lower_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator lower_bound(const _KT& __x) const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.lower_bound(__x)); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator upper_bound(const _KT& __x)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.upper_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator upper_bound(const _KT& __x) const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.upper_bound(__x)); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator,iterator> equal_range(const _KT& __x) {
    return pair<iterator, iterator>(iterator(&_M_iter_list, _M_non_dbg_impl.lower_bound(__x)),
                                    iterator(&_M_iter_list, _M_non_dbg_impl.upper_bound(__x)));
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __x) const {
    return pair<const_iterator,const_iterator>(const_iterator(&_M_iter_list, _M_non_dbg_impl.lower_bound(__x)),
                                               const_iterator(&_M_iter_list, _M_non_dbg_impl.upper_bound(__x)));
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator,iterator> equal_range_unique(const _KT& __x) {
    _STLP_STD::pair<_Base_iterator, _Base_iterator> __p;
    __p = _M_non_dbg_impl.equal_range_unique(__x);
    return pair<iterator, iterator>(iterator(&_M_iter_list, __p.first), iterator(&_M_iter_list, __p.second));
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range_unique(const _KT& __x) const {
    _STLP_STD::pair<_Base_const_iterator, _Base_const_iterator> __p;
    __p = _M_non_dbg_impl.equal_range_unique(__x);
    return pair<const_iterator, const_iterator>(const_iterator(&_M_iter_list, __p.first),
                                                const_iterator(&_M_iter_list, __p.second));
  }

  pair<iterator,bool> insert_unique(const value_type& __x) {
    _STLP_STD::pair<_Base_iterator, bool> __res = _M_non_dbg_impl.insert_unique(__x);
    return pair<iterator, bool>(iterator(&_M_iter_list, __res.first), __res.second);
  }
  iterator insert_equal(const value_type& __x)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.insert_equal(__x)); }

  iterator insert_unique(iterator __pos, const value_type& __x) {
    _STLP_DEBUG_CHECK(__check_if_owner(&_M_iter_list,__pos))
    return iterator(&_M_iter_list, _M_non_dbg_impl.insert_unique(__pos._M_iterator, __x));
  }
  iterator insert_equal(iterator __pos, const value_type& __x) {
    _STLP_DEBUG_CHECK(__check_if_owner(&_M_iter_list, __pos))
    return iterator(&_M_iter_list, _M_non_dbg_impl.insert_equal(__pos._M_iterator, __x));
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template<class _InputIterator>
  void insert_equal(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _M_non_dbg_impl.insert_equal(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
  template<class _InputIterator>
  void insert_unique(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _M_non_dbg_impl.insert_unique(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#else
  void insert_unique(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _M_non_dbg_impl.insert_unique(__first._M_iterator, __last._M_iterator);
  }
  void insert_unique(const value_type* __first, const value_type* __last) {
    _STLP_DEBUG_CHECK(__check_ptr_range(__first,__last))
    _M_non_dbg_impl.insert_unique(__first, __last);
  }
  void insert_equal(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _M_non_dbg_impl.insert_equal(__first._M_iterator, __last._M_iterator);
  }
  void insert_equal(const value_type* __first, const value_type* __last) {
    _STLP_DEBUG_CHECK(__check_ptr_range(__first,__last))
    _M_non_dbg_impl.insert_equal(__first, __last);
  }
#endif

  void erase(iterator __pos) {
    _STLP_DEBUG_CHECK(__check_if_owner(&_M_iter_list,__pos))
    _STLP_DEBUG_CHECK(_Dereferenceable(__pos))
    _Invalidate_iterator(__pos);
    _M_non_dbg_impl.erase(__pos._M_iterator);
  }
  size_type erase(const key_type& __x) {
    pair<iterator, iterator> __p = equal_range(__x);
    size_type __n = _STLP_STD::distance(__p.first._M_iterator, __p.second._M_iterator);
    _Invalidate_iterators(__p.first, __p.second);
    _M_non_dbg_impl.erase(__p.first._M_iterator, __p.second._M_iterator);
    return __n;
  }
  size_type erase_unique(const key_type& __x) {
    _Base_iterator __i = _M_non_dbg_impl.find(__x);
    if (__i != _M_non_dbg_impl.end()) {
      _Invalidate_iterator(iterator(&_M_iter_list, __i));
      _M_non_dbg_impl.erase(__i);
      return 1;
    }
    return 0;
  }

  void erase(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first, __last, begin(), end()))
    _Invalidate_iterators(__first, __last);
    _M_non_dbg_impl.erase(__first._M_iterator, __last._M_iterator);
  }
  void erase(const key_type* __first, const key_type* __last) {
    while (__first != __last) erase(*__first++);
  }

  void clear() {
    //should not invalidate end:
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.clear();
  }
};

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#undef _STLP_NON_DBG_TREE

#endif /* _STLP_INTERNAL_DBG_TREE_H */

// Local Variables:
// mode:C++
// End:
