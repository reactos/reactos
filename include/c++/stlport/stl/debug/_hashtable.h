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

#ifndef _STLP_INTERNAL_DBG_HASHTABLE_H
#define _STLP_INTERNAL_DBG_HASHTABLE_H

// Hashtable class, used to implement the hashed associative containers
// hash_set, hash_map, hash_multiset, and hash_multimap,
// unordered_set, unordered_map, unordered_multiset, unordered_multimap

#ifndef _STLP_DBG_ITERATOR_H
#  include <stl/debug/_iterator.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Key, class _Equal>
class _DbgEqual {
public:
  _DbgEqual() {}
  _DbgEqual(const _Equal& __eq) : _M_non_dbg_eq(__eq) {}
  _DbgEqual(const _DbgEqual& __eq) : _M_non_dbg_eq(__eq._M_non_dbg_eq) {}

#if !defined (_STLP_USE_CONTAINERS_EXTENSION)
  bool operator () (const _Key& __lhs, const _Key& __rhs) const
#else
  template <class _Kp1, class _Kp2>
  bool operator () (const _Kp1& __lhs, const _Kp2& __rhs) const
#endif
      {
#if !defined (_STLP_USE_CONTAINERS_EXTENSION)
        _STLP_VERBOSE_ASSERT(_M_non_dbg_eq(__rhs, __lhs) == _M_non_dbg_eq(__lhs, __rhs), _StlMsg_INVALID_EQUIVALENT_PREDICATE)
#endif
        return _M_non_dbg_eq(__lhs, __rhs) ? true : false;
      }

  _Equal non_dbg_key_eq() const { return _M_non_dbg_eq; }
private:
  _Equal _M_non_dbg_eq;
};

_STLP_MOVE_TO_STD_NAMESPACE

#define _STLP_NON_DBG_HT \
_STLP_PRIV _STLP_NON_DBG_NAME(hashtable) <_Val, _Key, _HF, _Traits, _ExK, _STLP_PRIV _DbgEqual<_Key, _EqK>, _All>

#if defined (_STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS)
template <class _Val, class _Key, class _HF,
          class _ExK, class _EqK, class _All>
inline _Val*
value_type(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_HT >&)
{ return (_Val*)0; }

template <class _Val, class _Key, class _HF,
          class _ExK, class _EqK, class _All>
inline forward_iterator_tag
iterator_category(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_HT >&)
{ return forward_iterator_tag(); }
#endif

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
class hashtable {
  typedef hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All> _Self;
  typedef _STLP_NON_DBG_HT _Base;

  typedef typename _Traits::_NonConstTraits _NonConstTraits;
  typedef typename _Traits::_ConstTraits _ConstTraits;
  typedef typename _Traits::_NonConstLocalTraits _NonConstLocalTraits;
  typedef typename _Traits::_ConstLocalTraits _ConstLocalTraits;

  _Base _M_non_dbg_impl;
  _STLP_PRIV __owned_list _M_iter_list;

public:
  typedef _Key key_type;
  typedef _HF hasher;
  typedef _EqK key_equal;

  __IMPORT_CONTAINER_TYPEDEFS(_Base)

  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_NonConstTraits> > iterator;
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _DbgTraits<_ConstTraits> >    const_iterator;
  //typedef _STLP_PRIV _DBG_iter<_Base, _DbgTraits<_NonConstLocalTraits> > local_iterator;
  typedef iterator local_iterator;
  //typedef _STLP_PRIV _DBG_iter<_Base, _DbgTraits<_ConstLocalTraits> >    const_local_iterator;
  typedef const_iterator const_local_iterator;

  typedef typename _Base::iterator _Base_iterator;
  typedef typename _Base::const_iterator _Base_const_iterator;

  hasher hash_funct() const { return _M_non_dbg_impl.hash_funct(); }
  key_equal key_eq() const { return _M_non_dbg_impl.key_eq().non_dbg_key_eq(); }

private:
  void _Invalidate_iterator(const const_iterator& __it)
  { _STLP_PRIV __invalidate_iterator(&_M_iter_list, __it); }
  void _Invalidate_iterators(const const_iterator& __first, const const_iterator& __last)
  { _STLP_PRIV __invalidate_range(&_M_iter_list, __first, __last); }

  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

public:
  allocator_type get_allocator() const { return _M_non_dbg_impl.get_allocator(); }

  hashtable(size_type __n,
            const _HF&  __hf,
            const _EqK& __eql,
            const _ExK& __ext,
            const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(__n, __hf, __eql, __ext, __a),
      _M_iter_list(&_M_non_dbg_impl) {}

  hashtable(size_type __n,
            const _HF&    __hf,
            const _EqK&   __eql,
            const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(__n, __hf, __eql, __a),
      _M_iter_list(&_M_non_dbg_impl) {}

  hashtable(const _Self& __ht)
    : _M_non_dbg_impl(__ht._M_non_dbg_impl),
      _M_iter_list(&_M_non_dbg_impl) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  hashtable(__move_source<_Self> src)
    : _M_non_dbg_impl(__move_source<_Base>(src.get()._M_non_dbg_impl)),
      _M_iter_list(&_M_non_dbg_impl) {
#  if defined (_STLP_NO_EXTENSIONS) || (_STLP_DEBUG_LEVEL == _STLP_STANDARD_DBG_LEVEL)
    src.get()._M_iter_list._Invalidate_all();
#  else
    src.get()._M_iter_list._Set_owner(_M_iter_list);
#  endif
  }
#endif

  size_type size() const { return _M_non_dbg_impl.size(); }
  size_type max_size() const { return _M_non_dbg_impl.max_size(); }
  bool empty() const { return _M_non_dbg_impl.empty(); }

  _Self& operator=(const _Self& __ht) {
    if (this != &__ht) {
      //Should not invalidate end iterator
      _Invalidate_iterators(begin(), end());
      _M_non_dbg_impl = __ht._M_non_dbg_impl;
    }
    return *this;
  }

  void swap(_Self& __ht) {
   _M_iter_list._Swap_owners(__ht._M_iter_list);
   _M_non_dbg_impl.swap(__ht._M_non_dbg_impl);
  }

  iterator begin() { return iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  iterator end()   { return iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  local_iterator begin(size_type __n) {
    //TODO: Add checks for iterator locality -> avoids comparison between different bucket iterators
    _STLP_VERBOSE_ASSERT((__n < bucket_count()), _StlMsg_INVALID_ARGUMENT)
    return local_iterator(&_M_iter_list, _M_non_dbg_impl.begin(__n));
  }
  local_iterator end(size_type __n) {
    //TODO: Add checks for iterator locality -> avoids comparison between different bucket iterators
    _STLP_VERBOSE_ASSERT((__n < bucket_count()), _StlMsg_INVALID_ARGUMENT)
    return local_iterator(&_M_iter_list, _M_non_dbg_impl.end(__n));
  }

  const_iterator begin() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  const_iterator end() const { return const_iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  const_local_iterator begin(size_type __n) const {
    //TODO: Add checks for iterator locality -> avoids comparison between different bucket iterators
    _STLP_VERBOSE_ASSERT((__n < bucket_count()), _StlMsg_INVALID_ARGUMENT)
    return const_local_iterator(&_M_iter_list, _M_non_dbg_impl.begin(__n));
  }
  const_local_iterator end(size_type __n) const {
    //TODO: Add checks for iterator locality -> avoids comparison between different bucket iterators
    _STLP_VERBOSE_ASSERT((__n < bucket_count()), _StlMsg_INVALID_ARGUMENT)
    return const_local_iterator(&_M_iter_list, _M_non_dbg_impl.end(__n));
  }

  pair<iterator, bool> insert_unique(const value_type& __obj) {
    pair<_Base_iterator, bool> __res = _M_non_dbg_impl.insert_unique(__obj);
    return pair<iterator, bool>(iterator(&_M_iter_list, __res.first), __res.second);
  }

  iterator insert_equal(const value_type& __obj)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.insert_equal(__obj)); }

  pair<iterator, bool> insert_unique_noresize(const value_type& __obj) {
    pair<_Base_iterator, bool> __res = _M_non_dbg_impl.insert_unique_noresize(__obj);
    return pair<iterator, bool>(iterator(&_M_iter_list, __res.first), __res.second);
  }

  iterator insert_equal_noresize(const value_type& __obj)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.insert_equal_noresize(__obj)); }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert_unique(_InputIterator __f, _InputIterator __l) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__f, __l))
    _M_non_dbg_impl.insert_unique(_STLP_PRIV _Non_Dbg_iter(__f), _STLP_PRIV _Non_Dbg_iter(__l));
  }

  template <class _InputIterator>
  void insert_equal(_InputIterator __f, _InputIterator __l){
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__f, __l))
    _M_non_dbg_impl.insert_equal(_STLP_PRIV _Non_Dbg_iter(__f), _STLP_PRIV _Non_Dbg_iter(__l));
  }

#else
  void insert_unique(const value_type* __f, const value_type* __l) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__f, __l))
    _M_non_dbg_impl.insert_unique(__f, __l);
  }

  void insert_equal(const value_type* __f, const value_type* __l) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__f, __l))
    _M_non_dbg_impl.insert_equal(__f, __l);
  }

  void insert_unique(const_iterator __f, const_iterator __l) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__f, __l))
    _M_non_dbg_impl.insert_unique(__f._M_iterator, __l._M_iterator);
  }

  void insert_equal(const_iterator __f, const_iterator __l) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__f, __l))
    _M_non_dbg_impl.insert_equal(__f._M_iterator, __l._M_iterator);
  }
#endif

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __key)
  { return iterator(&_M_iter_list, _M_non_dbg_impl.find(__key)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __key) const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.find(__key)); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __key) const { return _M_non_dbg_impl.count(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator, iterator> equal_range(const _KT& __key) {
    pair<_Base_iterator, _Base_iterator> __res = _M_non_dbg_impl.equal_range(__key);
    return pair<iterator,iterator> (iterator(&_M_iter_list,__res.first),
                                    iterator(&_M_iter_list,__res.second));
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __key) const {
    pair <_Base_const_iterator, _Base_const_iterator> __res = _M_non_dbg_impl.equal_range(__key);
    return pair<const_iterator,const_iterator> (const_iterator(&_M_iter_list,__res.first),
                                                const_iterator(&_M_iter_list,__res.second));
  }

  size_type erase(const key_type& __key) {
    pair<iterator, iterator> __p = equal_range(__key);
    size_type __n = _STLP_STD::distance(__p.first, __p.second);
    _Invalidate_iterators(__p.first, __p.second);
    _M_non_dbg_impl.erase(__p.first._M_iterator, __p.second._M_iterator);
    return __n;
  }

  void erase(const const_iterator& __it) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__it))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __it))
    _Invalidate_iterator(__it);
    _M_non_dbg_impl.erase(__it._M_iterator);
  }
  void erase(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last,
                                               const_iterator(begin()), const_iterator(end())))
    _Invalidate_iterators(__first, __last);
    _M_non_dbg_impl.erase(__first._M_iterator, __last._M_iterator);
  }

  void rehash(size_type __num_buckets_hint) { _M_non_dbg_impl.rehash(__num_buckets_hint); }
  void resize(size_type __num_elements_hint) { _M_non_dbg_impl.resize(__num_elements_hint); }

  void clear() {
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.clear();
  }

  reference _M_insert(const value_type& __obj) { return _M_non_dbg_impl._M_insert(__obj); }

  size_type bucket_count() const { return _M_non_dbg_impl.bucket_count(); }
  size_type max_bucket_count() const { return _M_non_dbg_impl.max_bucket_count(); }
  size_type elems_in_bucket(size_type __n) const {
    _STLP_VERBOSE_ASSERT((__n < bucket_count()), _StlMsg_INVALID_ARGUMENT)
    return _M_non_dbg_impl.elems_in_bucket(__n);
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type bucket(const _KT& __k) const { return _M_non_dbg_impl.bucket(__k); }

  float load_factor() const { return _M_non_dbg_impl.load_factor(); }
  float max_load_factor() const { return _M_non_dbg_impl.max_load_factor(); }
  void max_load_factor(float __z) {
    _STLP_VERBOSE_ASSERT((__z > 0.0f), _StlMsg_INVALID_ARGUMENT)
    _M_non_dbg_impl.max_load_factor(__z);
  }
};

_STLP_END_NAMESPACE

#undef _STLP_NON_DBG_HT

#endif /* _STLP_INTERNAL_HASHTABLE_H */

// Local Variables:
// mode:C++
// End:
