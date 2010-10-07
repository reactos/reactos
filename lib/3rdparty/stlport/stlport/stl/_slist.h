/*
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

#ifndef _STLP_INTERNAL_SLIST_H
#define _STLP_INTERNAL_SLIST_H

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

#ifndef _STLP_INTERNAL_SLIST_BASE_H
#  include <stl/_slist_base.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp>
class _Slist_node : public _Slist_node_base {
public:
  _Tp _M_data;
  __TRIVIAL_STUFF(_Slist_node)
};

struct _Slist_iterator_base {
  typedef size_t               size_type;
  typedef ptrdiff_t            difference_type;
  typedef forward_iterator_tag iterator_category;

  _Slist_node_base *_M_node;

  _Slist_iterator_base(_Slist_node_base *__x) : _M_node(__x) {}

  void _M_incr() {
    _M_node = _M_node->_M_next;
  }
};

template <class _Tp, class _Traits>
class _Slist_iterator : public _Slist_iterator_base {
public:
  typedef typename _Traits::value_type value_type;
  typedef typename _Traits::pointer    pointer;
  typedef typename _Traits::reference  reference;
  typedef forward_iterator_tag iterator_category;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef _Slist_iterator<_Tp, _Traits>         _Self;
  typedef typename _Traits::_NonConstTraits     _NonConstTraits;
  typedef _Slist_iterator<_Tp, _NonConstTraits> iterator;
  typedef typename _Traits::_ConstTraits        _ConstTraits;
  typedef _Slist_iterator<_Tp, _ConstTraits>    const_iterator;

  typedef _Slist_node<value_type> _Node;

  explicit _Slist_iterator(_Slist_node_base *__x) : _Slist_iterator_base(__x) {}
  _Slist_iterator() : _Slist_iterator_base(0) {}
  //copy constructor for iterator and constructor from iterator for const_iterator
  _Slist_iterator(const iterator& __x) : _Slist_iterator_base(__x._M_node) {}

  reference operator*() const { return __STATIC_CAST(_Node*, this->_M_node)->_M_data; }

  _STLP_DEFINE_ARROW_OPERATOR

  _Self& operator++() {
    _M_incr();
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    _M_incr();
    return __tmp;
  }

  bool operator==(const_iterator __y ) const {
    return this->_M_node == __y._M_node;
  }
  bool operator!=(const_iterator __y ) const {
    return this->_M_node != __y._M_node;
  }
};

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
_STLP_MOVE_TO_STD_NAMESPACE
template <class _Tp, class _Traits>
struct __type_traits<_STLP_PRIV _Slist_iterator<_Tp, _Traits> > {
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
template <class _Tp, class _Traits>
inline _Tp* _STLP_CALL value_type(const _STLP_PRIV _Slist_iterator<_Tp, _Traits>&) { return __STATIC_CAST(_Tp*, 0); }
inline ptrdiff_t* _STLP_CALL distance_type(const _STLP_PRIV _Slist_iterator_base&) { return 0; }
inline forward_iterator_tag _STLP_CALL iterator_category(const _STLP_PRIV _Slist_iterator_base&) { return forward_iterator_tag(); }
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif /* OLD_QUERIES */

// Base class that encapsulates details of allocators and simplifies EH
template <class _Tp, class _Alloc>
class _Slist_base {
protected:
  typedef _Slist_node<_Tp> _Node;
  typedef typename _Alloc_traits<_Node,_Alloc>::allocator_type _M_node_allocator_type;
  typedef _Slist_base<_Tp, _Alloc> _Self;

public:
  typedef _STLP_alloc_proxy<_Slist_node_base, _Node, _M_node_allocator_type> _AllocProxy;

  _STLP_FORCE_ALLOCATORS(_Tp, _Alloc)
  typedef _Alloc allocator_type;

  _Slist_base(const allocator_type& __a) :
    _M_head(_STLP_CONVERT_ALLOCATOR(__a, _Node), _Slist_node_base() )
  { _M_head._M_data._M_next = 0; }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  _Slist_base(__move_source<_Self> src) :
    _M_head(__move_source<_AllocProxy>(src.get()._M_head))
  { src.get()._M_head._M_data._M_next = 0; }
#endif

  ~_Slist_base() { _M_erase_after(&_M_head._M_data, 0); }

protected:
  _Slist_node_base* _M_erase_after(_Slist_node_base* __pos) {
    _Node* __next = __STATIC_CAST(_Node*, __pos->_M_next);
    _Slist_node_base* __next_next = __next->_M_next;
    __pos->_M_next = __next_next;
    _STLP_STD::_Destroy(&__next->_M_data);
    _M_head.deallocate(__next,1);
    return __next_next;
  }
  _Slist_node_base* _M_erase_after(_Slist_node_base*, _Slist_node_base*);

public:
  allocator_type get_allocator() const
  { return _STLP_CONVERT_ALLOCATOR((const _M_node_allocator_type&)_M_head, _Tp); }
  _AllocProxy _M_head;
};

#if defined (_STLP_USE_PTR_SPECIALIZATIONS)
#  define slist _STLP_PTR_IMPL_NAME(slist)
#elif defined (_STLP_DEBUG)
#  define slist _STLP_NON_DBG_NAME(slist)
#else
_STLP_MOVE_TO_STD_NAMESPACE
#endif

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class slist;

#if !defined (slist)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

// helper functions to reduce code duplication
template <class _Tp, class _Alloc, class _BinaryPredicate>
void _Slist_unique(slist<_Tp, _Alloc>& __that, _BinaryPredicate __binary_pred);

template <class _Tp, class _Alloc, class _StrictWeakOrdering>
void _Slist_merge(slist<_Tp, _Alloc>& __that, slist<_Tp, _Alloc>& __x,
                  _StrictWeakOrdering __comp);

template <class _Tp, class _Alloc, class _StrictWeakOrdering>
void _Slist_sort(slist<_Tp, _Alloc>& __that, _StrictWeakOrdering __comp);

#if !defined (slist)
_STLP_MOVE_TO_STD_NAMESPACE
#endif

template <class _Tp, class _Alloc>
class slist : protected _STLP_PRIV _Slist_base<_Tp,_Alloc>
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (slist)
            , public __stlport_class<slist<_Tp, _Alloc> >
#endif
{
private:
  typedef _STLP_PRIV _Slist_base<_Tp,_Alloc> _Base;
  typedef slist<_Tp,_Alloc> _Self;
public:
  typedef _Tp                value_type;

  typedef value_type*       pointer;
  typedef const value_type* const_pointer;
  typedef value_type&       reference;
  typedef const value_type& const_reference;
  typedef size_t            size_type;
  typedef ptrdiff_t         difference_type;
  typedef forward_iterator_tag _Iterator_category;

  typedef _STLP_PRIV _Slist_iterator<_Tp, _Nonconst_traits<_Tp> > iterator;
  typedef _STLP_PRIV _Slist_iterator<_Tp, _Const_traits<_Tp> >    const_iterator;

  _STLP_FORCE_ALLOCATORS(_Tp, _Alloc)
  typedef typename _Base::allocator_type allocator_type;

private:
  typedef _STLP_PRIV _Slist_node<_Tp> _Node;
  typedef _STLP_PRIV _Slist_node_base _Node_base;

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  _Node* _M_create_node(const value_type& __x = _Tp()) {
#else
  _Node* _M_create_node(const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    _Node* __node = this->_M_head.allocate(1);
    _STLP_TRY {
      _Copy_Construct(&__node->_M_data, __x);
      __node->_M_next = 0;
    }
    _STLP_UNWIND(this->_M_head.deallocate(__node, 1))
    return __node;
  }

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  _Node* _M_create_node() {
    _Node* __node = this->_M_head.allocate(1);
    _STLP_TRY {
      _STLP_STD::_Construct(&__node->_M_data);
      __node->_M_next = 0;
    }
    _STLP_UNWIND(this->_M_head.deallocate(__node, 1))
    return __node;
  }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

public:

  allocator_type get_allocator() const { return _Base::get_allocator(); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit slist(const allocator_type& __a = allocator_type())
#else
  slist()
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(allocator_type()) {}
  slist(const allocator_type& __a)
#endif
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(__a) {}

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit slist(size_type __n, const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(_Tp),
                 const allocator_type& __a =  allocator_type())
#else
  explicit slist(size_type __n)
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(allocator_type())
    { _M_insert_after_fill(&this->_M_head._M_data, __n, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
  slist(size_type __n, const value_type& __x)
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(allocator_type())
    { _M_insert_after_fill(&this->_M_head._M_data, __n, __x); }
  slist(size_type __n, const value_type& __x, const allocator_type& __a)
#endif
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(__a)
    { _M_insert_after_fill(&this->_M_head._M_data, __n, __x); }

#if defined (_STLP_MEMBER_TEMPLATES)
  // We don't need any dispatching tricks here, because _M_insert_after_range
  // already does them.
  template <class _InputIterator>
  slist(_InputIterator __first, _InputIterator __last,
        const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(__a)
    { _M_insert_after_range(&this->_M_head._M_data, __first, __last); }
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  // VC++ needs this crazyness
  template <class _InputIterator>
  slist(_InputIterator __first, _InputIterator __last)
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(allocator_type())
    { _M_insert_after_range(&this->_M_head._M_data, __first, __last); }
# endif
#else /* _STLP_MEMBER_TEMPLATES */
  slist(const_iterator __first, const_iterator __last,
        const allocator_type& __a =  allocator_type() )
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(__a)
    { _M_insert_after_range(&this->_M_head._M_data, __first, __last); }
  slist(const value_type* __first, const value_type* __last,
        const allocator_type& __a =  allocator_type())
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(__a)
    { _M_insert_after_range(&this->_M_head._M_data, __first, __last); }
#endif /* _STLP_MEMBER_TEMPLATES */

  slist(const _Self& __x)
    : _STLP_PRIV _Slist_base<_Tp,_Alloc>(__x.get_allocator())
    { _M_insert_after_range(&this->_M_head._M_data, __x.begin(), __x.end()); }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  slist(__move_source<_Self> src)
    : _STLP_PRIV _Slist_base<_Tp, _Alloc>(__move_source<_Base>(src.get())) {}
#endif

  _Self& operator= (const _Self& __x);

  ~slist() {}

public:
  // assign(), a generalized assignment member function.  Two
  // versions: one that takes a count, and one that takes a range.
  // The range version is a member template, so we dispatch on whether
  // or not the type is an integer.

  void assign(size_type __n, const _Tp& __val) { _M_fill_assign(__n, __val); }

private:
  void _M_fill_assign(size_type __n, const _Tp& __val);

#if defined (_STLP_MEMBER_TEMPLATES)
public:
  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last) {
    typedef typename _IsIntegral<_InputIterator>::_Ret _Integral;
    _M_assign_dispatch(__first, __last, _Integral());
  }

private:
  template <class _Integer>
  void _M_assign_dispatch(_Integer __n, _Integer __val,
                          const __true_type& /*_IsIntegral*/) {
    _M_fill_assign((size_type) __n, (_Tp) __val);
  }

  template <class _InputIter>
  void _M_assign_dispatch(_InputIter __first, _InputIter __last,
                          const __false_type& /*_IsIntegral*/) {
#else
public:
  void assign(const_pointer __first, const_pointer __last) {
    _Node_base* __prev = &this->_M_head._M_data;
    _Node_base* __node = this->_M_head._M_data._M_next;
    while (__node != 0 && __first != __last) {
      __STATIC_CAST(_Node*, __node)->_M_data = *__first;
      __prev = __node;
      __node = __node->_M_next;
      ++__first;
    }
    if (__first != __last)
      _M_insert_after_range(__prev, __first, __last);
    else
      this->_M_erase_after(__prev, 0);
  }
  void assign(const_iterator __first, const_iterator __last) {
#endif /* _STLP_MEMBER_TEMPLATES */
    _Node_base* __prev = &this->_M_head._M_data;
    _Node_base* __node = this->_M_head._M_data._M_next;
    while (__node != 0 && __first != __last) {
      __STATIC_CAST(_Node*, __node)->_M_data = *__first;
      __prev = __node;
      __node = __node->_M_next;
      ++__first;
    }
    if (__first != __last)
      _M_insert_after_range(__prev, __first, __last);
    else
      this->_M_erase_after(__prev, 0);
  }

public:

  // Experimental new feature: before_begin() returns a
  // non-dereferenceable iterator that, when incremented, yields
  // begin().  This iterator may be used as the argument to
  // insert_after, erase_after, etc.  Note that even for an empty
  // slist, before_begin() is not the same iterator as end().  It
  // is always necessary to increment before_begin() at least once to
  // obtain end().
  iterator before_begin() { return iterator(&this->_M_head._M_data); }
  const_iterator before_begin() const
    { return const_iterator(__CONST_CAST(_Node_base*, &this->_M_head._M_data)); }

  iterator begin() { return iterator(this->_M_head._M_data._M_next); }
  const_iterator begin() const
    { return const_iterator(this->_M_head._M_data._M_next);}

  iterator end() { return iterator(); }
  const_iterator end() const { return const_iterator(); }

  size_type size() const
  { return _STLP_PRIV _Sl_global_inst::size(this->_M_head._M_data._M_next); }

  size_type max_size() const { return size_type(-1); }

  bool empty() const { return this->_M_head._M_data._M_next == 0; }

  void swap(_Self& __x)
  { this->_M_head.swap(__x._M_head); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

public:
  reference front()             { return *begin(); }
  const_reference front() const { return *begin(); }
#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_front(const value_type& __x = _Tp())   {
#else
  void push_front(const value_type& __x)   {
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
    _STLP_PRIV __slist_make_link(&this->_M_head._M_data, _M_create_node(__x));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_front() { _STLP_PRIV __slist_make_link(&this->_M_head._M_data, _M_create_node());}
#endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

  void pop_front() {
    _Node* __node = __STATIC_CAST(_Node*, this->_M_head._M_data._M_next);
    this->_M_head._M_data._M_next = __node->_M_next;
    _STLP_STD::_Destroy(&__node->_M_data);
    this->_M_head.deallocate(__node, 1);
  }

  iterator previous(const_iterator __pos) {
    return iterator(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node));
  }
  const_iterator previous(const_iterator __pos) const {
    return const_iterator(__CONST_CAST(_Node_base*,
                                       _STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data,
                                                                               __pos._M_node)));
  }

private:
#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  _Node* _M_insert_after(_Node_base* __pos, const value_type& __x = _Tp()) {
#else
  _Node* _M_insert_after(_Node_base* __pos, const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    return __STATIC_CAST(_Node*, _STLP_PRIV __slist_make_link(__pos, _M_create_node(__x)));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  _Node* _M_insert_after(_Node_base* __pos) {
    return __STATIC_CAST(_Node*, _STLP_PRIV __slist_make_link(__pos, _M_create_node()));
  }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void _M_insert_after_fill(_Node_base* __pos,
                            size_type __n, const value_type& __x) {
    for (size_type __i = 0; __i < __n; ++__i)
      __pos = _STLP_PRIV __slist_make_link(__pos, _M_create_node(__x));
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  // Check whether it's an integral type.  If so, it's not an iterator.
  template <class _InIter>
  void _M_insert_after_range(_Node_base* __pos,
                             _InIter __first, _InIter __last) {
    typedef typename _IsIntegral<_InIter>::_Ret _Integral;
    _M_insert_after_range(__pos, __first, __last, _Integral());
  }

  template <class _Integer>
  void _M_insert_after_range(_Node_base* __pos, _Integer __n, _Integer __x,
                             const __true_type&) {
    _M_insert_after_fill(__pos, __n, __x);
  }

  template <class _InIter>
  void _M_insert_after_range(_Node_base* __pos,
                             _InIter __first, _InIter __last,
                             const __false_type&) {
#else /* _STLP_MEMBER_TEMPLATES */
  void _M_insert_after_range(_Node_base* __pos,
                             const value_type* __first,
                             const value_type* __last) {
    while (__first != __last) {
      __pos = _STLP_PRIV __slist_make_link(__pos, _M_create_node(*__first));
      ++__first;
    }
  }
  void _M_insert_after_range(_Node_base* __pos,
                             const_iterator __first, const_iterator __last) {
#endif /* _STLP_MEMBER_TEMPLATES */
    while (__first != __last) {
      __pos = _STLP_PRIV __slist_make_link(__pos, _M_create_node(*__first));
      ++__first;
    }
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  // Check whether it's an integral type.  If so, it's not an iterator.
  template <class _InIter>
  void _M_splice_after_range(_Node_base* __pos,
                             _InIter __first, _InIter __last) {
    typedef typename _IsIntegral<_InIter>::_Ret _Integral;
    _M_splice_after_range(__pos, __first, __last, _Integral());
  }

  template <class _Integer>
  void _M_splice_after_range(_Node_base* __pos, _Integer __n, _Integer __x,
                             const __true_type&) {
    _M_insert_after_fill(__pos, __n, __x);
  }

  template <class _InIter>
  void _M_splice_after_range(_Node_base* __pos,
                             _InIter __first, _InIter __last,
                             const __false_type&) {
#else /* _STLP_MEMBER_TEMPLATES */
  void _M_splice_after_range(_Node_base* __pos,
                             const value_type* __first,
                             const value_type* __last) {
    while (__first != __last) {
      __pos = _STLP_PRIV __slist_make_link(__pos, _M_create_node(*__first));
      ++__first;
    }
  }
  void _M_splice_after_range(_Node_base* __pos,
                             const_iterator __first, const_iterator __last) {
#endif /* _STLP_MEMBER_TEMPLATES */
    //We use a temporary slist to avoid the auto reference troubles (infinite loop)
    _Self __tmp(__first, __last, this->get_allocator());
    splice_after(iterator(__pos), __tmp);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  // Check whether it's an integral type.  If so, it's not an iterator.
  template <class _InIter>
  void _M_splice_range(_Node_base* __pos,
                       _InIter __first, _InIter __last) {
    typedef typename _IsIntegral<_InIter>::_Ret _Integral;
    _M_splice_range(__pos, __first, __last, _Integral());
  }

  template <class _Integer>
  void _M_splice_range(_Node_base* __pos, _Integer __n, _Integer __x,
                       const __true_type&) {
    _M_insert_after_fill(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos),
                         __n, __x);
  }

  template <class _InIter>
  void _M_splice_range(_Node_base* __pos,
                       _InIter __first, _InIter __last,
                       const __false_type&) {
#else /* _STLP_MEMBER_TEMPLATES */
  void _M_splice_range(_Node_base* __pos,
                       const value_type* __first,
                       const value_type* __last) {
    while (__first != __last) {
      __pos = _STLP_PRIV __slist_make_link(__pos, _M_create_node(*__first));
      ++__first;
    }
  }
  void _M_splice_range(_Node_base* __pos,
                       const_iterator __first, const_iterator __last) {
#endif /* _STLP_MEMBER_TEMPLATES */
    //We use a temporary slist to avoid the auto reference troubles (infinite loop)
    _Self __tmp(__first, __last, this->get_allocator());
    splice(iterator(__pos), __tmp);
  }

public:

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert_after(iterator __pos, const value_type& __x = _Tp()) {
#else
  iterator insert_after(iterator __pos, const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    return iterator(_M_insert_after(__pos._M_node, __x));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert_after(iterator __pos) {
    return insert_after(__pos, _STLP_DEFAULT_CONSTRUCTED(_Tp));
  }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void insert_after(iterator __pos, size_type __n, const value_type& __x) {
    _M_insert_after_fill(__pos._M_node, __n, __x);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  // We don't need any dispatching tricks here, because _M_insert_after_range
  // already does them.
  template <class _InIter>
  void insert_after(iterator __pos, _InIter __first, _InIter __last) {
#else /* _STLP_MEMBER_TEMPLATES */
  void insert_after(iterator __pos,
                    const value_type* __first, const value_type* __last) {
    _M_insert_after_range(__pos._M_node, __first, __last);
  }
  void insert_after(iterator __pos,
                    const_iterator __first, const_iterator __last) {
#endif /* _STLP_MEMBER_TEMPLATES */
    _M_splice_after_range(__pos._M_node, __first, __last);
  }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert(iterator __pos, const value_type& __x = _Tp()) {
#else
  iterator insert(iterator __pos, const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    return iterator(_M_insert_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node),
                    __x));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert(iterator __pos) {
    return iterator(_M_insert_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node),
                                    _STLP_DEFAULT_CONSTRUCTED(_Tp)));
  }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void insert(iterator __pos, size_type __n, const value_type& __x) {
    _M_insert_after_fill(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node), __n, __x);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  // We don't need any dispatching tricks here, because _M_insert_after_range
  // already does them.
  template <class _InIter>
  void insert(iterator __pos, _InIter __first, _InIter __last) {
#else /* _STLP_MEMBER_TEMPLATES */
  void insert(iterator __pos, const value_type* __first,
                              const value_type* __last) {
    _M_insert_after_range(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node),
                          __first, __last);
  }
  void insert(iterator __pos, const_iterator __first, const_iterator __last) {
#endif /* _STLP_MEMBER_TEMPLATES */
    _M_splice_range(__pos._M_node, __first, __last);
  }

public:
  iterator erase_after(iterator __pos)
  { return iterator(this->_M_erase_after(__pos._M_node)); }
  iterator erase_after(iterator __before_first, iterator __last)
  { return iterator(this->_M_erase_after(__before_first._M_node, __last._M_node)); }

  iterator erase(iterator __pos)
  { return iterator(this->_M_erase_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node))); }
  iterator erase(iterator __first, iterator __last)
  { return iterator(this->_M_erase_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __first._M_node), __last._M_node)); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type new_size, const value_type& __x = _Tp());
#else
  void resize(size_type new_size, const value_type& __x);
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type new_size) { resize(new_size, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void clear()
  { this->_M_erase_after(&this->_M_head._M_data, 0); }

public:
  // Moves the range [__before_first + 1, __before_last + 1) to *this,
  //  inserting it immediately after __pos.  This is constant time.
  void splice_after(iterator __pos, _Self& __x,
                    iterator __before_first, iterator __before_last) {
    if (__before_first != __before_last) {
      if (this->get_allocator() == __x.get_allocator()) {
        _STLP_PRIV _Sl_global_inst::__splice_after(__pos._M_node,
                                                   __before_first._M_node, __before_last._M_node);
      }
      else {
        this->insert_after(__pos, iterator(__before_first._M_node->_M_next), iterator(__before_last._M_node->_M_next));
        __x.erase_after(__before_first, ++__before_last);
      }
    }
  }

  // Moves the element that follows __prev to *this, inserting it immediately
  //  after __pos.  This is constant time.
  void splice_after(iterator __pos, _Self& __x, iterator __prev) {
    if (this->get_allocator() == __x.get_allocator()) {
      _STLP_PRIV _Sl_global_inst::__splice_after(__pos._M_node,
                                                 __prev._M_node, __prev._M_node->_M_next);
    }
    else {
      this->insert_after(__pos, __STATIC_CAST(_Node*, __prev._M_node->_M_next)->_M_data);
      __x.erase_after(__prev);
    }
  }

  // Removes all of the elements from the list __x to *this, inserting
  // them immediately after __pos.  __x must not be *this.  Complexity:
  // linear in __x.size().
  void splice_after(iterator __pos, _Self& __x) {
    if (this->get_allocator() == __x.get_allocator())
      _STLP_PRIV _Sl_global_inst::__splice_after(__pos._M_node, &__x._M_head._M_data);
    else {
      this->insert_after(__pos, __x.begin(), __x.end());
      __x.clear();
    }
  }

  // Linear in distance(begin(), __pos), and linear in __x.size().
  void splice(iterator __pos, _Self& __x) {
    if (__x._M_head._M_data._M_next) {
      if (this->get_allocator() == __x.get_allocator()) {
        _STLP_PRIV _Sl_global_inst::__splice_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node),
                                                   &__x._M_head._M_data,
                                                   _STLP_PRIV _Sl_global_inst::__previous(&__x._M_head._M_data, 0));
      }
      else {
        insert(__pos, __x.begin(), __x.end());
        __x.clear();
      }
    }
  }

  // Linear in distance(begin(), __pos), and in distance(__x.begin(), __i).
  void splice(iterator __pos, _Self& __x, iterator __i) {
    if (this->get_allocator() == __x.get_allocator()) {
      _STLP_PRIV _Sl_global_inst::__splice_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node),
                                                 _STLP_PRIV _Sl_global_inst::__previous(&__x._M_head._M_data, __i._M_node),
                                                 __i._M_node);
    }
    else {
      insert(__pos, *__i);
      __x.erase(__i);
    }
  }

  // Linear in distance(begin(), __pos), in distance(__x.begin(), __first),
  // and in distance(__first, __last).
  void splice(iterator __pos, _Self& __x, iterator __first, iterator __last) {
    if (__first != __last) {
      if (this->get_allocator() == __x.get_allocator()) {
        _STLP_PRIV _Sl_global_inst::__splice_after(_STLP_PRIV _Sl_global_inst::__previous(&this->_M_head._M_data, __pos._M_node),
                                                   _STLP_PRIV _Sl_global_inst::__previous(&__x._M_head._M_data, __first._M_node),
                                                   _STLP_PRIV _Sl_global_inst::__previous(__first._M_node, __last._M_node));
      }
      else {
        insert(__pos, __first, __last);
        __x.erase(__first, __last);
      }
    }
  }

public:
  void reverse() {
    if (this->_M_head._M_data._M_next)
      this->_M_head._M_data._M_next = _STLP_PRIV _Sl_global_inst::__reverse(this->_M_head._M_data._M_next);
  }

  void remove(const _Tp& __val);

  void unique() { _STLP_PRIV _Slist_unique(*this, equal_to<value_type>()); }
  void merge(_Self& __x) { _STLP_PRIV _Slist_merge(*this, __x, less<value_type>()); }
  void sort() { _STLP_PRIV _Slist_sort(*this, less<value_type>()); }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _Predicate>
  void remove_if(_Predicate __pred) {
    _Node_base* __cur = &this->_M_head._M_data;
    while (__cur->_M_next) {
      if (__pred(__STATIC_CAST(_Node*, __cur->_M_next)->_M_data))
        this->_M_erase_after(__cur);
      else
        __cur = __cur->_M_next;
    }
  }

  template <class _BinaryPredicate>
  void unique(_BinaryPredicate __pred)
  { _STLP_PRIV _Slist_unique(*this, __pred); }

  template <class _StrictWeakOrdering>
  void merge(_Self& __x, _StrictWeakOrdering __comp)
  { _STLP_PRIV _Slist_merge(*this, __x, __comp); }

  template <class _StrictWeakOrdering>
  void sort(_StrictWeakOrdering __comp)
  { _STLP_PRIV _Slist_sort(*this, __comp); }
#endif /* _STLP_MEMBER_TEMPLATES */
};

#if defined (slist)
#  undef slist
_STLP_MOVE_TO_STD_NAMESPACE
#endif

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_slist.c>
#endif

#if defined (_STLP_USE_PTR_SPECIALIZATIONS)
#  include <stl/pointers/_slist.h>
#endif

#if defined (_STLP_DEBUG)
#  include <stl/debug/_slist.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _Tp, class _Alloc>
inline bool  _STLP_CALL
operator == (const slist<_Tp,_Alloc>& _SL1, const slist<_Tp,_Alloc>& _SL2) {
  typedef typename slist<_Tp,_Alloc>::const_iterator const_iterator;
  const_iterator __end1 = _SL1.end();
  const_iterator __end2 = _SL2.end();

  const_iterator __i1 = _SL1.begin();
  const_iterator __i2 = _SL2.begin();
  while (__i1 != __end1 && __i2 != __end2 && *__i1 == *__i2) {
    ++__i1;
    ++__i2;
  }
  return __i1 == __end1 && __i2 == __end2;
}

#define _STLP_EQUAL_OPERATOR_SPECIALIZED
#define _STLP_TEMPLATE_HEADER    template <class _Tp, class _Alloc>
#define _STLP_TEMPLATE_CONTAINER slist<_Tp, _Alloc>
#include <stl/_relops_cont.h>
#undef _STLP_TEMPLATE_CONTAINER
#undef _STLP_TEMPLATE_HEADER
#undef _STLP_EQUAL_OPERATOR_SPECIALIZED

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
#  if !defined (_STLP_NO_MOVE_SEMANTIC)
template <class _Tp, class _Alloc>
struct __move_traits<slist<_Tp, _Alloc> > {
  typedef __true_type implemented;
  typedef typename __move_traits<_Alloc>::complete complete;
};
#  endif

// Specialization of insert_iterator so that insertions will be constant
// time rather than linear time.
template <class _Tp, class _Alloc>
class insert_iterator<slist<_Tp, _Alloc> > {
protected:
  typedef slist<_Tp, _Alloc> _Container;
  _Container* _M_container;
  typename _Container::iterator _M_iter;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  insert_iterator(_Container& __x, typename _Container::iterator __i)
    : _M_container(&__x) {
    if (__i == __x.begin())
      _M_iter = __x.before_begin();
    else
      _M_iter = __x.previous(__i);
  }

  insert_iterator<_Container>&
  operator = (const typename _Container::value_type& __val) {
    _M_iter = _M_container->insert_after(_M_iter, __val);
    return *this;
  }

  insert_iterator<_Container>& operator*() { return *this; }
  insert_iterator<_Container>& operator++() { return *this; }
  insert_iterator<_Container>& operator++(int) { return *this; }
};
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_SLIST_H */

// Local Variables:
// mode:C++
// End:
