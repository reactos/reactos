/*
 * Copyright (c) 2005
 * Francois Dumont
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_PTR_SPECIALIZED_SET_H
#define _STLP_PTR_SPECIALIZED_SET_H

#ifndef _STLP_POINTERS_SPEC_TOOLS_H
#  include <stl/pointers/_tools.h>
#endif

_STLP_BEGIN_NAMESPACE

#if defined (__BORLANDC__) || defined (__DMC__)
#  define typename
#endif

//Specific iterator traits creation
_STLP_CREATE_ITERATOR_TRAITS(SetTraitsT, Const_traits)

#if defined (_STLP_USE_TEMPLATE_EXPORT) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
_STLP_EXPORT template struct _STLP_CLASS_DECLSPEC less<void*>;

_STLP_MOVE_TO_PRIV_NAMESPACE

typedef _Rb_tree_node<void*> _Node;
_STLP_EXPORT_TEMPLATE_CLASS _STLP_alloc_proxy<_Rb_tree_node_base, _Node,  allocator<_Node> >;
_STLP_EXPORT_TEMPLATE_CLASS _Rb_tree_base<void*, allocator<void*> >;
#  if defined (_STLP_DEBUG)
_STLP_EXPORT_TEMPLATE_CLASS _DbgCompare<void*, less<void*> >;
#    define _Rb_tree _STLP_NON_DBG_NAME(Rb_tree)
_STLP_EXPORT_TEMPLATE_CLASS _Rb_tree<void*, _DbgCompare<void*, less<void*> >, void*, _Identity<void*>,
                                     _SetTraitsT<void*>, allocator<void*> >;
#    undef _Rb_tree
#  endif
_STLP_EXPORT_TEMPLATE_CLASS _Rb_tree<void*, less<void*>, void*, _Identity<void*>,
                                     _SetTraitsT<void*>, allocator<void*> >;
_STLP_MOVE_TO_STD_NAMESPACE
#endif

template <class _Key, _STLP_DFL_TMPL_PARAM(_Compare, less<_Key>),
                      _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Key>) >
class set
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
          : public __stlport_class<set<_Key, _Compare, _Alloc> >
#endif
{
#if !defined (__BORLANDC__)
  typedef _STLP_PRIV _AssocStorageTypes<_Key, _Compare> _AssocStorageTypes;
  typedef typename _AssocStorageTypes::_KeyStorageType _KeyStorageType;
  typedef typename _AssocStorageTypes::_CompareStorageType _CompareStorageType;
#else
  typedef _STLP_PRIV _AssocStorageTypes<_Key, _Compare>::_KeyStorageType _KeyStorageType;
  typedef _STLP_PRIV _AssocStorageTypes<_Key, _Compare>::_CompareStorageType _CompareStorageType;
#endif
  typedef typename _Alloc_traits<_KeyStorageType, _Alloc>::allocator_type _StorageTypeAlloc;
  typedef _STLP_PRIV _CastTraits<_KeyStorageType, _Key> cast_traits;

  typedef set<_Key, _Compare, _Alloc> _Self;
public:
  typedef _Key     key_type;
  typedef _Key     value_type;
  typedef _Compare key_compare;
  typedef _Compare value_compare;

protected:
  //Specific iterator traits creation
  typedef _STLP_PRIV _SetTraitsT<value_type> _SetTraits;
  typedef _STLP_PRIV _Rb_tree<key_type, key_compare,
                              value_type, _STLP_PRIV _Identity<value_type>,
                              _SetTraits, _Alloc> _Priv_Rep_type;

  typedef _STLP_PRIV _SetTraitsT<_KeyStorageType> _SetStorageTraits;

public:
  //dums: need the following public for the __move_traits framework
  typedef _STLP_PRIV _Rb_tree<_KeyStorageType, _CompareStorageType,
                              _KeyStorageType, _STLP_PRIV _Identity<_KeyStorageType>,
                              _SetStorageTraits, _StorageTypeAlloc> _Rep_type;

private:
  typedef typename _Rep_type::iterator base_iterator;
  typedef typename _Rep_type::const_iterator const_base_iterator;

public:
  typedef typename _Priv_Rep_type::pointer pointer;
  typedef typename _Priv_Rep_type::const_pointer const_pointer;
  typedef typename _Priv_Rep_type::reference reference;
  typedef typename _Priv_Rep_type::const_reference const_reference;
  typedef typename _Priv_Rep_type::iterator iterator;
  typedef typename _Priv_Rep_type::const_iterator const_iterator;
  typedef typename _Priv_Rep_type::reverse_iterator reverse_iterator;
  typedef typename _Priv_Rep_type::const_reverse_iterator const_reverse_iterator;
  typedef typename _Priv_Rep_type::size_type size_type;
  typedef typename _Priv_Rep_type::difference_type difference_type;
  typedef typename _Priv_Rep_type::allocator_type allocator_type;

private:
  _Rep_type _M_t;  // red-black tree representing set
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

#if defined (_STLP_DEBUG)
  static iterator _S_to_value_ite(const_base_iterator __ite)
  { return iterator(__ite._Owner(), __ite._M_iterator._M_node); }
  static base_iterator _S_to_storage_ite(const_iterator __ite)
  { return base_iterator(__ite._Owner(), __ite._M_iterator._M_node); }
#else
  static iterator _S_to_value_ite(const_base_iterator __ite)
  { return iterator(__ite._M_node); }
  static base_iterator _S_to_storage_ite(const_iterator __ite)
  { return base_iterator(__ite._M_node); }
#endif

public:
  set() : _M_t(_CompareStorageType(), _StorageTypeAlloc()) {}
  explicit set(const _Compare& __comp,
               const allocator_type& __a = allocator_type())
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType)) {}

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  set(_InputIterator __first, _InputIterator __last)
    : _M_t(_Compare(), _StorageTypeAlloc()) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_unique(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                       _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#  else
    _M_t.insert_unique(__first, __last);
#  endif
  }

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  set(_InputIterator __first, _InputIterator __last, const _Compare& __comp)
    : _M_t(__comp, _StorageTypeAlloc()) {
#    if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_unique(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                       _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#    else
    _M_t.insert_unique(__first, __last);
#    endif
  }
#  endif
  template <class _InputIterator>
  set(_InputIterator __first, _InputIterator __last, const _Compare& __comp,
      const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType)) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_unique(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                       _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#  else
    _M_t.insert_unique(__first, __last);
#  endif
  }
#else
  set(const value_type* __first, const value_type* __last)
    : _M_t(_Compare(), _StorageTypeAlloc()) {
    _M_t.insert_unique(cast_traits::to_storage_type_cptr(__first),
                       cast_traits::to_storage_type_cptr(__last));
  }

  set(const value_type* __first, const value_type* __last,
      const _Compare& __comp, const allocator_type& __a = allocator_type())
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType)) {
    _M_t.insert_unique(cast_traits::to_storage_type_cptr(__first),
                       cast_traits::to_storage_type_cptr(__last));
  }

  set(const_iterator __first, const_iterator __last)
    : _M_t(_Compare(), _StorageTypeAlloc())
  { _M_t.insert_unique(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }

  set(const_iterator __first, const_iterator __last,
      const _Compare& __comp, const allocator_type& __a = allocator_type())
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType))
  { _M_t.insert_unique(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }
#endif /* _STLP_MEMBER_TEMPLATES */

  set(const _Self& __x) : _M_t(__x._M_t) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  set(__move_source<_Self> src)
    : _M_t(__move_source<_Rep_type>(src.get()._M_t)) {}
#endif

  _Self& operator=(const _Self& __x) {
    _M_t = __x._M_t;
    return *this;
  }

  // accessors:
  key_compare key_comp() const { return _M_t.key_comp(); }
  value_compare value_comp() const { return _M_t.key_comp(); }
  allocator_type get_allocator() const
  { return _STLP_CONVERT_ALLOCATOR(_M_t.get_allocator(), value_type); }

  iterator begin() { return _S_to_value_ite(_M_t.begin()); }
  iterator end() { return _S_to_value_ite(_M_t.end()); }
  const_iterator begin() const { return _S_to_value_ite(_M_t.begin()); }
  const_iterator end() const { return _S_to_value_ite(_M_t.end()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  bool empty() const { return _M_t.empty(); }
  size_type size() const { return _M_t.size(); }
  size_type max_size() const { return _M_t.max_size(); }
  void swap(_Self& __x) { _M_t.swap(__x._M_t); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  // insert/erase
  pair<iterator,bool> insert(const value_type& __x) {
    pair<base_iterator, bool> ret = _M_t.insert_unique(cast_traits::to_storage_type_cref(__x));
    return pair<iterator, bool>(_S_to_value_ite(ret.first), ret.second);
  }
  iterator insert(iterator __pos, const value_type& __x)
  { return _S_to_value_ite(_M_t.insert_unique(_S_to_storage_ite(__pos), cast_traits::to_storage_type_cref(__x))); }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(_InputIterator __first, _InputIterator __last) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_unique(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                       _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#  else
    _M_t.insert_unique(__first, __last);
#  endif
  }
#else
  void insert(const_iterator __first, const_iterator __last)
  { _M_t.insert_unique(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }
  void insert(const value_type* __first, const value_type* __last) {
    _M_t.insert_unique(cast_traits::to_storage_type_cptr(__first),
                       cast_traits::to_storage_type_cptr(__last));
  }
#endif
  void erase(iterator __pos)
  { _M_t.erase(_S_to_storage_ite(__pos)); }
  size_type erase(const key_type& __x)
  { return _M_t.erase_unique(cast_traits::to_storage_type_cref(__x)); }
  void erase(iterator __first, iterator __last)
  { _M_t.erase(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }
  void clear() { _M_t.clear(); }

  // set operations:
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __x) const
  { return _S_to_value_ite(_M_t.find(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __x)
  { return _S_to_value_ite(_M_t.find(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __x) const
  { return _M_t.find(cast_traits::to_storage_type_crefT(__x)) == _M_t.end() ? 0 : 1; }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator lower_bound(const _KT& __x)
  { return _S_to_value_ite(_M_t.lower_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator lower_bound(const _KT& __x) const
  { return _S_to_value_ite(_M_t.lower_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator upper_bound(const _KT& __x)
  { return _S_to_value_ite(_M_t.upper_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator upper_bound(const _KT& __x) const
  { return _S_to_value_ite(_M_t.upper_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator, iterator> equal_range(const _KT& __x) {
    pair<base_iterator, base_iterator> __ret;
    __ret = _M_t.equal_range(cast_traits::to_storage_type_crefT(__x));
    return pair<iterator, iterator>(_S_to_value_ite(__ret.first),
                                    _S_to_value_ite(__ret.second));
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __x) const {
    pair<const_base_iterator, const_base_iterator> __ret;
    __ret = _M_t.equal_range_unique(cast_traits::to_storage_type_crefT(__x));
    return pair<const_iterator, const_iterator>(_S_to_value_ite(__ret.first),
                                                _S_to_value_ite(__ret.second));
  }
};

//Specific iterator traits creation
_STLP_CREATE_ITERATOR_TRAITS(MultisetTraitsT, Const_traits)

template <class _Key, _STLP_DFL_TMPL_PARAM(_Compare, less<_Key>),
                     _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Key>) >
class multiset
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
               : public __stlport_class<multiset<_Key, _Compare, _Alloc> >
#endif
{
#if !defined (__BORLANDC__)
  typedef _STLP_PRIV _AssocStorageTypes<_Key, _Compare> _AssocStorageTypes;
  typedef typename _AssocStorageTypes::_KeyStorageType _KeyStorageType;
  typedef typename _AssocStorageTypes::_CompareStorageType _CompareStorageType;
#else
  typedef _STLP_PRIV _AssocStorageTypes<_Key, _Compare>::_KeyStorageType _KeyStorageType;
  typedef _STLP_PRIV _AssocStorageTypes<_Key, _Compare>::_CompareStorageType _CompareStorageType;
#endif
  typedef typename _Alloc_traits<_KeyStorageType, _Alloc>::allocator_type _StorageTypeAlloc;
  typedef _STLP_PRIV _CastTraits<_KeyStorageType, _Key> cast_traits;

  typedef multiset<_Key, _Compare, _Alloc> _Self;
public:
  // typedefs:
  typedef _Key     key_type;
  typedef _Key     value_type;
  typedef _Compare key_compare;
  typedef _Compare value_compare;

protected:
  //Specific iterator traits creation
  typedef _STLP_PRIV _MultisetTraitsT<value_type> _MultisetTraits;
  typedef _STLP_PRIV _Rb_tree<key_type, key_compare,
                              value_type, _STLP_PRIV _Identity<value_type>,
                              _MultisetTraits, _Alloc> _Priv_Rep_type;

  typedef _STLP_PRIV _MultisetTraitsT<_KeyStorageType> _MultisetStorageTraits;
public:
  //dums: need the following public for the __move_traits framework
  typedef _STLP_PRIV _Rb_tree<_KeyStorageType, _CompareStorageType,
                              _KeyStorageType, _STLP_PRIV _Identity<_KeyStorageType>,
                              _MultisetStorageTraits, _StorageTypeAlloc> _Rep_type;

private:
  typedef typename _Rep_type::iterator base_iterator;
  typedef typename _Rep_type::const_iterator const_base_iterator;

public:
  typedef typename _Priv_Rep_type::pointer pointer;
  typedef typename _Priv_Rep_type::const_pointer const_pointer;
  typedef typename _Priv_Rep_type::reference reference;
  typedef typename _Priv_Rep_type::const_reference const_reference;
  typedef typename _Priv_Rep_type::iterator iterator;
  typedef typename _Priv_Rep_type::const_iterator const_iterator;
  typedef typename _Priv_Rep_type::reverse_iterator reverse_iterator;
  typedef typename _Priv_Rep_type::const_reverse_iterator const_reverse_iterator;
  typedef typename _Priv_Rep_type::size_type size_type;
  typedef typename _Priv_Rep_type::difference_type difference_type;
  typedef typename _Priv_Rep_type::allocator_type allocator_type;

private:
  _Rep_type _M_t;  // red-black tree representing multiset
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

#if defined (_STLP_DEBUG)
  static iterator _S_to_value_ite(const_base_iterator __ite)
  { return iterator(__ite._Owner(), __ite._M_iterator._M_node); }
  static base_iterator _S_to_storage_ite(const_iterator __ite)
  { return base_iterator(__ite._Owner(), __ite._M_iterator._M_node); }
#else
  static iterator _S_to_value_ite(const_base_iterator __ite)
  { return iterator(__ite._M_node); }
  static base_iterator _S_to_storage_ite(const_iterator __ite)
  { return base_iterator(__ite._M_node); }
#endif

public:
  multiset() : _M_t(_Compare(), _StorageTypeAlloc()) {}
  explicit multiset(const _Compare& __comp,
                    const allocator_type& __a = allocator_type())
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType)) {}

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  multiset(_InputIterator __first, _InputIterator __last)
    : _M_t(_Compare(), _StorageTypeAlloc()) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_equal(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                      _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#  else
    _M_t.insert_equal(__first, __last);
#  endif
  }

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  multiset(_InputIterator __first, _InputIterator __last,
           const _Compare& __comp)
    : _M_t(__comp, _StorageTypeAlloc()) {
#    if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_equal(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                      _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#    else
    _M_t.insert_equal(__first, __last);
#    endif
  }
#  endif
  template <class _InputIterator>
  multiset(_InputIterator __first, _InputIterator __last,
           const _Compare& __comp,
           const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType)) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_equal(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                      _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#  else
    _M_t.insert_equal(__first, __last);
#  endif
  }

#else
  multiset(const value_type* __first, const value_type* __last)
    : _M_t(_Compare(), _StorageTypeAlloc()) {
    _M_t.insert_equal(cast_traits::to_storage_type_cptr(__first),
                      cast_traits::to_storage_type_cptr(__last));
  }

  multiset(const value_type* __first, const value_type* __last,
           const _Compare& __comp,
           const allocator_type& __a = allocator_type())
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType)) {
    _M_t.insert_equal(cast_traits::to_storage_type_cptr(__first),
                      cast_traits::to_storage_type_cptr(__last));
  }

  multiset(const_iterator __first, const_iterator __last)
    : _M_t(_Compare(), _StorageTypeAlloc())
  { _M_t.insert_equal(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }

  multiset(const_iterator __first, const_iterator __last,
           const _Compare& __comp,
           const allocator_type& __a = allocator_type())
    : _M_t(__comp, _STLP_CONVERT_ALLOCATOR(__a, _KeyStorageType))
  { _M_t.insert_equal(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }
#endif /* _STLP_MEMBER_TEMPLATES */

  multiset(const _Self& __x)
    : _M_t(__x._M_t) {}

  _Self& operator=(const _Self& __x) {
    _M_t = __x._M_t;
    return *this;
  }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  multiset(__move_source<_Self> src)
    : _M_t(__move_source<_Rep_type>(src.get()._M_t)) {}
#endif

  // accessors:
  key_compare key_comp() const { return _M_t.key_comp(); }
  value_compare value_comp() const { return _M_t.key_comp(); }
  allocator_type get_allocator() const
  { return _STLP_CONVERT_ALLOCATOR(_M_t.get_allocator(), value_type); }

  iterator begin() { return _S_to_value_ite(_M_t.begin()); }
  iterator end() { return _S_to_value_ite(_M_t.end()); }
  const_iterator begin() const { return _S_to_value_ite(_M_t.begin()); }
  const_iterator end() const { return _S_to_value_ite(_M_t.end()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  bool empty() const { return _M_t.empty(); }
  size_type size() const { return _M_t.size(); }
  size_type max_size() const { return _M_t.max_size(); }
  void swap(_Self& __x) { _M_t.swap(__x._M_t); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  // insert/erase
  iterator insert(const value_type& __x)
  { return _S_to_value_ite(_M_t.insert_equal(cast_traits::to_storage_type_cref(__x))); }
  iterator insert(iterator __pos, const value_type& __x) {
    return _S_to_value_ite(_M_t.insert_equal(_S_to_storage_ite(__pos),
                                             cast_traits::to_storage_type_cref(__x)));
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(_InputIterator __first, _InputIterator __last) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    _M_t.insert_equal(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__first),
                      _STLP_TYPENAME _STLP_PRIV _IteWrapper<_KeyStorageType, _Key, _InputIterator>::_Ite(__last));
#  else
    _M_t.insert_equal(__first, __last);
#  endif
  }
#else
  void insert(const value_type* __first, const value_type* __last) {
    _M_t.insert_equal(cast_traits::to_storage_type_cptr(__first),
                      cast_traits::to_storage_type_cptr(__last));
  }
  void insert(const_iterator __first, const_iterator __last)
  { _M_t.insert_equal(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }
#endif /* _STLP_MEMBER_TEMPLATES */

  void erase(iterator __pos)
  { _M_t.erase(_S_to_storage_ite(__pos)); }
  size_type erase(const key_type& __x)
  { return _M_t.erase(cast_traits::to_storage_type_cref(__x)); }
  void erase(iterator __first, iterator __last)
  { _M_t.erase(_S_to_storage_ite(__first), _S_to_storage_ite(__last)); }
  void clear() { _M_t.clear(); }

  // multiset operations:

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __x)
  { return _S_to_value_ite(_M_t.find(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __x) const
  { return _S_to_value_ite(_M_t.find(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __x) const
  { return _M_t.count(cast_traits::to_storage_type_crefT(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator lower_bound(const _KT& __x)
  { return _S_to_value_ite(_M_t.lower_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator lower_bound(const _KT& __x) const
  { return _S_to_value_ite(_M_t.lower_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator upper_bound(const _KT& __x)
  { return _S_to_value_ite(_M_t.upper_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator upper_bound(const _KT& __x) const
  { return _S_to_value_ite(_M_t.upper_bound(cast_traits::to_storage_type_crefT(__x))); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator, iterator> equal_range(const _KT& __x) {
    pair<base_iterator, base_iterator> __ret;
    __ret = _M_t.equal_range(cast_traits::to_storage_type_crefT(__x));
    return pair<iterator, iterator>(_S_to_value_ite(__ret.first),
                                    _S_to_value_ite(__ret.second));
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __x) const {
    pair<const_base_iterator, const_base_iterator> __ret;
    __ret = _M_t.equal_range(cast_traits::to_storage_type_crefT(__x));
    return pair<const_iterator, const_iterator>(_S_to_value_ite(__ret.first),
                                                _S_to_value_ite(__ret.second));
  }
};

#if defined (__BORLANDC__) || defined (__DMC__)
#  undef typename
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_PTR_SPECIALIZED_SET_H */

// Local Variables:
// mode:C++
// End:
