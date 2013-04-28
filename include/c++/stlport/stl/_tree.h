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

#ifndef _STLP_INTERNAL_TREE_H
#define _STLP_INTERNAL_TREE_H

/*

Red-black tree class, designed for use in implementing STL
associative containers (set, multiset, map, and multimap). The
insertion and deletion algorithms are based on those in Cormen,
Leiserson, and Rivest, Introduction to Algorithms (MIT Press, 1990),
except that

(1) the header cell is maintained with links not only to the root
but also to the leftmost node of the tree, to enable constant time
begin(), and to the rightmost node of the tree, to enable linear time
performance when used with the generic set algorithms (set_union,
etc.);

(2) when a node being deleted has two children its successor node is
relinked into its place, rather than copied, so that the only
iterators invalidated are those referring to the deleted node.

*/

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_H
#  include <stl/_iterator.h>
#endif

#ifndef _STLP_INTERNAL_CONSTRUCT_H
#  include <stl/_construct.h>
#endif

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

typedef bool _Rb_tree_Color_type;
//const _Rb_tree_Color_type _S_rb_tree_red = false;
//const _Rb_tree_Color_type _S_rb_tree_black = true;

#define _S_rb_tree_red false
#define _S_rb_tree_black true

struct _Rb_tree_node_base {
  typedef _Rb_tree_Color_type _Color_type;
  typedef _Rb_tree_node_base* _Base_ptr;

  _Color_type _M_color;
  _Base_ptr _M_parent;
  _Base_ptr _M_left;
  _Base_ptr _M_right;

  static _Base_ptr _STLP_CALL _S_minimum(_Base_ptr __x) {
    while (__x->_M_left != 0) __x = __x->_M_left;
    return __x;
  }

  static _Base_ptr _STLP_CALL _S_maximum(_Base_ptr __x) {
    while (__x->_M_right != 0) __x = __x->_M_right;
    return __x;
  }
};

template <class _Value>
struct _Rb_tree_node : public _Rb_tree_node_base {
  _Value _M_value_field;
  __TRIVIAL_STUFF(_Rb_tree_node)
};

struct _Rb_tree_base_iterator;

template <class _Dummy>
class _Rb_global {
public:
  typedef _Rb_tree_node_base* _Base_ptr;
  // those used to be global functions
  static void _STLP_CALL _Rebalance(_Base_ptr __x, _Base_ptr& __root);
  static _Base_ptr _STLP_CALL _Rebalance_for_erase(_Base_ptr __z,
                                                   _Base_ptr& __root,
                                                   _Base_ptr& __leftmost,
                                                   _Base_ptr& __rightmost);
  // those are from _Rb_tree_base_iterator - moved here to reduce code bloat
  // moved here to reduce code bloat without templatizing _Rb_tree_base_iterator
  static _Base_ptr  _STLP_CALL _M_increment (_Base_ptr);
  static _Base_ptr  _STLP_CALL _M_decrement (_Base_ptr);
  static void       _STLP_CALL _Rotate_left (_Base_ptr __x, _Base_ptr& __root);
  static void       _STLP_CALL _Rotate_right(_Base_ptr __x, _Base_ptr& __root);
};

# if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS _Rb_global<bool>;
# endif

typedef _Rb_global<bool> _Rb_global_inst;

struct _Rb_tree_base_iterator {
  typedef _Rb_tree_node_base*        _Base_ptr;
  typedef bidirectional_iterator_tag iterator_category;
  typedef ptrdiff_t                  difference_type;
  _Base_ptr _M_node;
  _Rb_tree_base_iterator() : _M_node(0) {}
  _Rb_tree_base_iterator(_Base_ptr __x) : _M_node(__x) {}
};

template <class _Value, class _Traits>
struct _Rb_tree_iterator : public _Rb_tree_base_iterator {
  typedef _Value value_type;
  typedef typename _Traits::reference  reference;
  typedef typename _Traits::pointer    pointer;
  typedef _Rb_tree_iterator<_Value, _Traits> _Self;
  typedef _Rb_tree_node_base*    _Base_ptr;
  typedef _Rb_tree_node<_Value>* _Link_type;

  typedef typename _Traits::_NonConstTraits _NonConstTraits;
  typedef _Rb_tree_iterator<_Value, _NonConstTraits> iterator;
  typedef typename _Traits::_ConstTraits _ConstTraits;
  typedef _Rb_tree_iterator<_Value, _ConstTraits> const_iterator;

  _Rb_tree_iterator() {}
#if !defined (_STLP_DEBUG)
  /* In STL debug mode we need this constructor implicit for the pointer
   * specialization implementation.
   */
  explicit
#endif
  _Rb_tree_iterator(_Base_ptr __x) : _Rb_tree_base_iterator(__x) {}
  //copy constructor for iterator and constructor from iterator for const_iterator
  _Rb_tree_iterator(const iterator& __it) : _Rb_tree_base_iterator(__it._M_node) {}

  reference operator*() const {
    return __STATIC_CAST(_Link_type, _M_node)->_M_value_field;
  }

  _STLP_DEFINE_ARROW_OPERATOR

  _Self& operator++() {
    _M_node = _Rb_global_inst::_M_increment(_M_node);
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    ++(*this);
    return __tmp;
  }

  _Self& operator--() {
    _M_node = _Rb_global_inst::_M_decrement(_M_node);
    return *this;
  }
  _Self operator--(int) {
    _Self __tmp = *this;
    --(*this);
    return __tmp;
  }

  bool operator == (const_iterator __rhs) const {
    return _M_node == __rhs._M_node;
  }
  bool operator != (const_iterator __rhs) const {
    return _M_node != __rhs._M_node;
  }
};

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
_STLP_MOVE_TO_STD_NAMESPACE
template <class _Value, class _Traits>
struct __type_traits<_STLP_PRIV _Rb_tree_iterator<_Value, _Traits> > {
  typedef __false_type   has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __false_type   is_POD_type;
};
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

#if defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
_STLP_MOVE_TO_STD_NAMESPACE
template <class _Value, class _Traits>
inline _Value* value_type(const _STLP_PRIV _Rb_tree_iterator<_Value, _Traits>&)
{ return (_Value*)0; }
inline bidirectional_iterator_tag iterator_category(const _STLP_PRIV _Rb_tree_base_iterator&)
{ return bidirectional_iterator_tag(); }
inline ptrdiff_t* distance_type(const _STLP_PRIV _Rb_tree_base_iterator&)
{ return (ptrdiff_t*) 0; }
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

// Base class to help EH

template <class _Tp, class _Alloc>
class _Rb_tree_base {
public:
  typedef _Rb_tree_node_base _Node_base;
  typedef _Rb_tree_node<_Tp> _Node;
  _STLP_FORCE_ALLOCATORS(_Tp, _Alloc)
  typedef _Alloc allocator_type;
private:
  typedef _Rb_tree_base<_Tp, _Alloc> _Self;
  typedef typename _Alloc_traits<_Node, _Alloc>::allocator_type _M_node_allocator_type;
  typedef _STLP_alloc_proxy<_Node_base, _Node, _M_node_allocator_type> _AllocProxy;

public:
  allocator_type get_allocator() const {
    return _STLP_CONVERT_ALLOCATOR(_M_header, _Tp);
  }

protected:
  _Rb_tree_base(const allocator_type& __a) :
    _M_header(_STLP_CONVERT_ALLOCATOR(__a, _Node), _Node_base() ) {
    _M_empty_initialize();
  }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  _Rb_tree_base(__move_source<_Self> src) :
    _M_header(__move_source<_AllocProxy>(src.get()._M_header)) {
    _M_rebind(&src.get()._M_header._M_data);
    src.get()._M_empty_initialize();
  }
#endif

  void _M_empty_initialize() {
    _M_header._M_data._M_color = _S_rb_tree_red; // used to distinguish header from
                                                 // __root, in iterator.operator++
    _M_header._M_data._M_parent = 0;
    _M_header._M_data._M_left = &_M_header._M_data;
    _M_header._M_data._M_right = &_M_header._M_data;
  }

  void _M_rebind(_Node_base *__static_node) {
    if (_M_header._M_data._M_parent != 0) {
      _M_header._M_data._M_parent->_M_parent = &_M_header._M_data;
    }
    if (_M_header._M_data._M_right == __static_node) {
      _M_header._M_data._M_right = &_M_header._M_data;
    }
    if (_M_header._M_data._M_left == __static_node) {
      _M_header._M_data._M_left = &_M_header._M_data;
    }
  }

  _AllocProxy _M_header;
};

#if defined (_STLP_DEBUG)
#  define _Rb_tree _STLP_NON_DBG_NAME(Rb_tree)
#endif

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits,
          _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Value>) >
class _Rb_tree : public _Rb_tree_base<_Value, _Alloc> {
  typedef _Rb_tree_base<_Value, _Alloc> _Base;
  typedef _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc> _Self;
protected:
  typedef _Rb_tree_node_base * _Base_ptr;
  typedef _Rb_tree_node<_Value> _Node;
  typedef _Node* _Link_type;
  typedef _Rb_tree_Color_type _Color_type;
public:
  typedef _Key key_type;
  typedef _Value value_type;
  typedef typename _Traits::pointer pointer;
  typedef const value_type* const_pointer;
  typedef typename _Traits::reference reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef bidirectional_iterator_tag _Iterator_category;
  typedef typename _Base::allocator_type allocator_type;

protected:

  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)
  _Base_ptr _M_create_node(const value_type& __x) {
    _Link_type __tmp = this->_M_header.allocate(1);
    _STLP_TRY {
      _Copy_Construct(&__tmp->_M_value_field, __x);
    }
    _STLP_UNWIND(this->_M_header.deallocate(__tmp,1))
    _S_left(__tmp) = 0;
    _S_right(__tmp) = 0;
    return __tmp;
  }

  _Base_ptr _M_clone_node(_Base_ptr __x) {
    _Base_ptr __tmp = _M_create_node(_S_value(__x));
    _S_color(__tmp) = _S_color(__x);
    return __tmp;
  }

  size_type _M_node_count; // keeps track of size of tree
  _Compare _M_key_compare;

  _Base_ptr _M_root() const
  { return this->_M_header._M_data._M_parent; }
  _Base_ptr _M_leftmost() const
  { return this->_M_header._M_data._M_left; }
  _Base_ptr _M_rightmost() const
  { return this->_M_header._M_data._M_right; }

  _Base_ptr& _M_root()
  { return this->_M_header._M_data._M_parent; }
  _Base_ptr& _M_leftmost()
  { return this->_M_header._M_data._M_left; }
  _Base_ptr& _M_rightmost()
  { return this->_M_header._M_data._M_right; }

  static _Base_ptr& _STLP_CALL _S_left(_Base_ptr __x)
  { return __x->_M_left; }
  static _Base_ptr& _STLP_CALL _S_right(_Base_ptr __x)
  { return __x->_M_right; }
  static _Base_ptr& _STLP_CALL _S_parent(_Base_ptr __x)
  { return __x->_M_parent; }
  static value_type& _STLP_CALL _S_value(_Base_ptr __x)
  { return __STATIC_CAST(_Link_type, __x)->_M_value_field; }
  static const _Key& _STLP_CALL _S_key(_Base_ptr __x)
  { return _KeyOfValue()(_S_value(__x));}
  static _Color_type& _STLP_CALL _S_color(_Base_ptr __x)
  { return (_Color_type&)(__x->_M_color); }

  static _Base_ptr _STLP_CALL _S_minimum(_Base_ptr __x)
  { return _Rb_tree_node_base::_S_minimum(__x); }

  static _Base_ptr _STLP_CALL _S_maximum(_Base_ptr __x)
  { return _Rb_tree_node_base::_S_maximum(__x); }

public:
  typedef typename _Traits::_NonConstTraits _NonConstTraits;
  typedef typename _Traits::_ConstTraits _ConstTraits;
  typedef _Rb_tree_iterator<value_type, _NonConstTraits> iterator;
  typedef _Rb_tree_iterator<value_type, _ConstTraits> const_iterator;
  _STLP_DECLARE_BIDIRECTIONAL_REVERSE_ITERATORS;

private:
  iterator _M_insert(_Base_ptr __parent, const value_type& __val, _Base_ptr __on_left = 0, _Base_ptr __on_right = 0);
  _Base_ptr _M_copy(_Base_ptr __x, _Base_ptr __p);
  void _M_erase(_Base_ptr __x);

public:
                                // allocation/deallocation
  _Rb_tree()
    : _Rb_tree_base<_Value, _Alloc>(allocator_type()), _M_node_count(0), _M_key_compare(_Compare())
    {}

  _Rb_tree(const _Compare& __comp)
    : _Rb_tree_base<_Value, _Alloc>(allocator_type()), _M_node_count(0), _M_key_compare(__comp)
    {}

  _Rb_tree(const _Compare& __comp, const allocator_type& __a)
    : _Rb_tree_base<_Value, _Alloc>(__a), _M_node_count(0), _M_key_compare(__comp)
    {}

  _Rb_tree(const _Self& __x)
    : _Rb_tree_base<_Value, _Alloc>(__x.get_allocator()),
      _M_node_count(0), _M_key_compare(__x._M_key_compare) {
    if (__x._M_root() != 0) {
      _S_color(&this->_M_header._M_data) = _S_rb_tree_red;
      _M_root() = _M_copy(__x._M_root(), &this->_M_header._M_data);
      _M_leftmost() = _S_minimum(_M_root());
      _M_rightmost() = _S_maximum(_M_root());
    }
    _M_node_count = __x._M_node_count;
  }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  _Rb_tree(__move_source<_Self> src)
    : _Rb_tree_base<_Value, _Alloc>(__move_source<_Base>(src.get())),
      _M_node_count(src.get()._M_node_count),
      _M_key_compare(_AsMoveSource(src.get()._M_key_compare))
  { src.get()._M_node_count = 0; }
#endif

  ~_Rb_tree() { clear(); }
  _Self& operator=(const _Self& __x);

public:
                                // accessors:
  _Compare key_comp() const { return _M_key_compare; }

  iterator begin() { return iterator(_M_leftmost()); }
  const_iterator begin() const { return const_iterator(_M_leftmost()); }
  iterator end() { return iterator(&this->_M_header._M_data); }
  const_iterator end() const { return const_iterator(__CONST_CAST(_Base_ptr, &this->_M_header._M_data)); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const
  { return const_reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const
  { return const_reverse_iterator(begin()); }
  bool empty() const { return _M_node_count == 0; }
  size_type size() const { return _M_node_count; }
  size_type max_size() const { return size_type(-1); }

  void swap(_Self& __t) {
    if (__t.empty()) {
      if (this->empty()) return;
      __t._M_header.swap(this->_M_header);
      __t._M_rebind(&this->_M_header._M_data);
      this->_M_empty_initialize();
    }
    else if (this->empty()) {
      __t.swap(*this);
      return;
    }
    else {
      this->_M_header.swap(__t._M_header);
      this->_M_rebind(&__t._M_header._M_data);
      __t._M_rebind(&this->_M_header._M_data);
    }
    _STLP_STD::swap(_M_node_count, __t._M_node_count);
    _STLP_STD::swap(_M_key_compare, __t._M_key_compare);
  }

public:
                                // insert/erase
  pair<iterator,bool> insert_unique(const value_type& __x);
  iterator insert_equal(const value_type& __x);

  iterator insert_unique(iterator __pos, const value_type& __x);
  iterator insert_equal(iterator __pos, const value_type& __x);

#if defined (_STLP_MEMBER_TEMPLATES)
  template<class _II> void insert_equal(_II __first, _II __last) {
    for ( ; __first != __last; ++__first)
      insert_equal(*__first);
  }
  template<class _II> void insert_unique(_II __first, _II __last) {
    for ( ; __first != __last; ++__first)
      insert_unique(*__first);
  }
#else
  void insert_unique(const_iterator __first, const_iterator __last) {
    for ( ; __first != __last; ++__first)
      insert_unique(*__first);
  }
  void insert_unique(const value_type* __first, const value_type* __last) {
    for ( ; __first != __last; ++__first)
      insert_unique(*__first);
  }
  void insert_equal(const_iterator __first, const_iterator __last) {
    for ( ; __first != __last; ++__first)
      insert_equal(*__first);
  }
  void insert_equal(const value_type* __first, const value_type* __last) {
    for ( ; __first != __last; ++__first)
      insert_equal(*__first);
  }
#endif

  void erase(iterator __pos) {
    _Base_ptr __x = _Rb_global_inst::_Rebalance_for_erase(__pos._M_node,
                                                          this->_M_header._M_data._M_parent,
                                                          this->_M_header._M_data._M_left,
                                                          this->_M_header._M_data._M_right);
    _STLP_STD::_Destroy(&_S_value(__x));
    this->_M_header.deallocate(__STATIC_CAST(_Link_type, __x), 1);
    --_M_node_count;
  }

  size_type erase(const key_type& __x) {
    pair<iterator,iterator> __p = equal_range(__x);
    size_type __n = _STLP_STD::distance(__p.first, __p.second);
    erase(__p.first, __p.second);
    return __n;
  }

  size_type erase_unique(const key_type& __x) {
    iterator __i = find(__x);
    if (__i._M_node != &this->_M_header._M_data) {
      erase(__i);
      return 1;
    }
    return 0;
  }

  void erase(iterator __first, iterator __last) {
    if (__first._M_node == this->_M_header._M_data._M_left && // begin()
        __last._M_node == &this->_M_header._M_data)           // end()
      clear();
    else
      while (__first != __last) erase(__first++);
  }

  void erase(const key_type* __first, const key_type* __last) {
    while (__first != __last) erase(*__first++);
  }

  void clear() {
    if (_M_node_count != 0) {
      _M_erase(_M_root());
      _M_leftmost() = &this->_M_header._M_data;
      _M_root() = 0;
      _M_rightmost() = &this->_M_header._M_data;
      _M_node_count = 0;
    }
  }

public:
                                // set operations:
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __k) { return iterator(_M_find(__k)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __k) const { return const_iterator(_M_find(__k)); }
private:
  _STLP_TEMPLATE_FOR_CONT_EXT
  _Base_ptr _M_find(const _KT& __k) const {
    _Base_ptr __y = __CONST_CAST(_Base_ptr, &this->_M_header._M_data);      // Last node which is not less than __k.
    _Base_ptr __x = _M_root();      // Current node.

    while (__x != 0)
      if (!_M_key_compare(_S_key(__x), __k))
        __y = __x, __x = _S_left(__x);
      else
        __x = _S_right(__x);

    if (__y != &this->_M_header._M_data) {
      if (_M_key_compare(__k, _S_key(__y))) {
        __y = __CONST_CAST(_Base_ptr, &this->_M_header._M_data);
      }
    }
    return __y;
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  _Base_ptr _M_lower_bound(const _KT& __k) const {
    _Base_ptr __y = __CONST_CAST(_Base_ptr, &this->_M_header._M_data); /* Last node which is not less than __k. */
    _Base_ptr __x = _M_root(); /* Current node. */

    while (__x != 0)
      if (!_M_key_compare(_S_key(__x), __k))
        __y = __x, __x = _S_left(__x);
      else
        __x = _S_right(__x);

    return __y;
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  _Base_ptr _M_upper_bound(const _KT& __k) const {
    _Base_ptr __y = __CONST_CAST(_Base_ptr, &this->_M_header._M_data); /* Last node which is greater than __k. */
    _Base_ptr __x = _M_root(); /* Current node. */

    while (__x != 0)
      if (_M_key_compare(__k, _S_key(__x)))
        __y = __x, __x = _S_left(__x);
      else
        __x = _S_right(__x);

    return __y;
  }

public:
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __x) const {
    pair<const_iterator, const_iterator> __p = equal_range(__x);
    return _STLP_STD::distance(__p.first, __p.second);
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator lower_bound(const _KT& __x) { return iterator(_M_lower_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator lower_bound(const _KT& __x) const { return const_iterator(_M_lower_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator upper_bound(const _KT& __x) { return iterator(_M_upper_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator upper_bound(const _KT& __x) const { return const_iterator(_M_upper_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator,iterator> equal_range(const _KT& __x)
  { return pair<iterator, iterator>(lower_bound(__x), upper_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __x) const
  { return pair<const_iterator, const_iterator>(lower_bound(__x), upper_bound(__x)); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator,iterator> equal_range_unique(const _KT& __x) {
    pair<iterator, iterator> __p;
    __p.second = lower_bound(__x);
    if (__p.second._M_node != &this->_M_header._M_data &&
        !_M_key_compare(__x, _S_key(__p.second._M_node))) {
      __p.first = __p.second++;
    }
    else {
      __p.first = __p.second;
    }
    return __p;
  }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range_unique(const _KT& __x) const {
    pair<const_iterator, const_iterator> __p;
    __p.second = lower_bound(__x);
    if (__p.second._M_node != &this->_M_header._M_data &&
        !_M_key_compare(__x, _S_key(__p.second._M_node))) {
      __p.first = __p.second++;
    }
    else {
      __p.first = __p.second;
    }
    return __p;
  }

#if defined (_STLP_DEBUG)
public:
  // Debugging.
  bool __rb_verify() const;
#endif //_STLP_DEBUG
};

#if defined (_STLP_DEBUG)
#  undef _Rb_tree
#endif

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_tree.c>
#endif

#if defined (_STLP_DEBUG)
#  include <stl/debug/_tree.h>
#endif

_STLP_BEGIN_NAMESPACE

#define _STLP_TEMPLATE_HEADER template <class _Key, class _Compare, class _Value, class _KeyOfValue, class _Traits, class _Alloc>
#define _STLP_TEMPLATE_CONTAINER _STLP_PRIV _Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>
#include <stl/_relops_cont.h>
#undef _STLP_TEMPLATE_CONTAINER
#undef _STLP_TEMPLATE_HEADER

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_NO_MOVE_SEMANTIC)
template <class _Key, class _Compare, class _Value, class _KeyOfValue, class _Traits, class _Alloc>
struct __move_traits<_STLP_PRIV _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc> >
  : _STLP_PRIV __move_traits_help2<_Compare, _Alloc> {};
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_TREE_H */

// Local Variables:
// mode:C++
// End:
