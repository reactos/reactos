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

#ifndef _STLP_INTERNAL_DBG_SLIST_H
#define _STLP_INTERNAL_DBG_SLIST_H

#ifndef _STLP_DBG_ITERATOR_H
#  include <stl/debug/_iterator.h>
#endif

#define _STLP_NON_DBG_SLIST _STLP_PRIV _STLP_NON_DBG_NAME(slist) <_Tp, _Alloc>

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS)
template <class _Tp, class _Alloc>
inline _Tp*
value_type(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_SLIST >&)
{ return (_Tp*)0; }

template <class _Tp, class _Alloc>
inline forward_iterator_tag
iterator_category(const _STLP_PRIV _DBG_iter_base< _STLP_NON_DBG_SLIST >&)
{ return forward_iterator_tag(); }
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

/*
 * slist special debug traits version.
 */
template <class _Traits>
struct _SlistDbgTraits : _Traits {
  typedef _SlistDbgTraits<typename _Traits::_ConstTraits> _ConstTraits;
  typedef _SlistDbgTraits<typename _Traits::_NonConstTraits> _NonConstTraits;

  /*
   * We don't want the before_begin iterator to return false at _Dereferenceable
   * call to do not break the current debug framework but calling * operator should
   * fail.
   */
  template <class _Iterator>
  static bool _Check(const _Iterator& __it)
  { return !(__it._M_iterator == (__it._Get_container_ptr())->before_begin()); }
};

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class slist :
#if !defined (__DMC__)
             private
#endif
                     _STLP_PRIV __construct_checker<_STLP_NON_DBG_SLIST >
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
            , public __stlport_class<slist<_Tp, _Alloc> >
#endif
{
private:
  typedef _STLP_NON_DBG_SLIST _Base;
  typedef slist<_Tp,_Alloc> _Self;
  typedef _STLP_PRIV __construct_checker<_STLP_NON_DBG_SLIST > _ConstructCheck;

public:

  __IMPORT_CONTAINER_TYPEDEFS(_Base)

  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _SlistDbgTraits<_Nonconst_traits<value_type> > > iterator;
  typedef _STLP_PRIV _DBG_iter<_Base, _STLP_PRIV _SlistDbgTraits<_Const_traits<value_type> > >    const_iterator;

  allocator_type get_allocator() const { return _M_non_dbg_impl.get_allocator(); }
private:
  _Base _M_non_dbg_impl;
  _STLP_PRIV __owned_list _M_iter_list;

  void _Invalidate_iterator(const iterator& __it)
  { _STLP_PRIV __invalidate_iterator(&_M_iter_list, __it); }
  void _Invalidate_iterators(const iterator& __first, const iterator& __last)
  { _STLP_PRIV __invalidate_range(&_M_iter_list, __first, __last); }

  typedef typename _Base::iterator _Base_iterator;

public:
  explicit slist(const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(__a) , _M_iter_list(&_M_non_dbg_impl) {}

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit slist(size_type __n, const value_type& __x = _Tp(),
#else
  slist(size_type __n, const value_type& __x,
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
        const allocator_type& __a =  allocator_type())
    : _M_non_dbg_impl(__n, __x, __a), _M_iter_list(&_M_non_dbg_impl) {}

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit slist(size_type __n) : _M_non_dbg_impl(__n) , _M_iter_list(&_M_non_dbg_impl) {}
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  slist(__move_source<_Self> src)
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
  // We don't need any dispatching tricks here, because _M_insert_after_range
  // already does them.
  template <class _InputIterator>
  slist(_InputIterator __first, _InputIterator __last,
        const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last), __a),
      _M_iter_list(&_M_non_dbg_impl) {}
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  slist(_InputIterator __first, _InputIterator __last)
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last)),
      _M_iter_list(&_M_non_dbg_impl) {}
#  endif
#else

  slist(const value_type* __first, const value_type* __last,
        const allocator_type& __a =  allocator_type())
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first, __last, __a),
      _M_iter_list(&_M_non_dbg_impl) {}

  slist(const_iterator __first, const_iterator __last,
        const allocator_type& __a = allocator_type() )
    : _ConstructCheck(__first, __last),
      _M_non_dbg_impl(__first._M_iterator, __last._M_iterator, __a),
      _M_iter_list(&_M_non_dbg_impl) {}
#endif

  slist(const _Self& __x) :
    _ConstructCheck(__x),
    _M_non_dbg_impl(__x._M_non_dbg_impl), _M_iter_list(&_M_non_dbg_impl) {}

  _Self& operator= (const _Self& __x) {
    if (this != &__x) {
      _Invalidate_iterators(begin(), end());
      _M_non_dbg_impl = __x._M_non_dbg_impl;
    }
    return *this;
  }

  ~slist() {}

  void assign(size_type __n, const value_type& __val) {
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.assign(__n, __val);
  }

  iterator before_begin()
  { return iterator(&_M_iter_list, _M_non_dbg_impl.before_begin()); }
  const_iterator before_begin() const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.before_begin()); }

  iterator begin()
  { return iterator(&_M_iter_list, _M_non_dbg_impl.begin()); }
  const_iterator begin() const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.begin());}

  iterator end()
  { return iterator(&_M_iter_list, _M_non_dbg_impl.end()); }
  const_iterator end() const
  { return const_iterator(&_M_iter_list, _M_non_dbg_impl.end()); }

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

  reference front() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    return _M_non_dbg_impl.front();
  }
  const_reference front() const {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    return _M_non_dbg_impl.front();
  }
  void push_front(const_reference __x) { _M_non_dbg_impl.push_front(__x); }
  void pop_front() {
    _STLP_VERBOSE_ASSERT(!empty(), _StlMsg_EMPTY_CONTAINER)
    _M_non_dbg_impl.pop_front();
  }
  iterator previous(const_iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    return iterator(&_M_iter_list, _M_non_dbg_impl.previous(__pos._M_iterator));
  }
  const_iterator previous(const_iterator __pos) const {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    return const_iterator(&_M_iter_list, _M_non_dbg_impl.previous(__pos._M_iterator));
  }

public:

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert_after(iterator __pos, const value_type& __x = _Tp()) {
#else
  iterator insert_after(iterator __pos, const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    return iterator(&_M_iter_list,_M_non_dbg_impl.insert_after(__pos._M_iterator, __x));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert_after(iterator __pos) {
    return insert_after(__pos, _STLP_DEFAULT_CONSTRUCTED(_Tp));
  }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void insert_after(iterator __pos, size_type __n, const value_type& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _M_non_dbg_impl.insert_after(__pos._M_iterator, __n, __x);
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.assign(_STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#else
  void assign(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.assign(__first._M_iterator, __last._M_iterator);
  }
  void assign(const value_type *__first, const value_type *__last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.assign(__first, __last);
  }
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InIter>
  void insert_after(iterator __pos, _InIter __first, _InIter __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.insert_after(__pos._M_iterator,
                                 _STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }

  template <class _InIter>
  void insert(iterator __pos, _InIter __first, _InIter __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.insert(__pos._M_iterator,
                           _STLP_PRIV _Non_Dbg_iter(__first), _STLP_PRIV _Non_Dbg_iter(__last));
  }
#else
  void insert_after(iterator __pos,
                    const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.insert_after(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
  }
  void insert_after(iterator __pos,
                    const value_type* __first, const value_type* __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    _M_non_dbg_impl.insert_after(__pos._M_iterator, __first, __last);
  }

  void insert(iterator __pos, const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
    _M_non_dbg_impl.insert(__pos._M_iterator, __first._M_iterator, __last._M_iterator);
  }
  void insert(iterator __pos, const value_type* __first,
                              const value_type* __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_ptr_range(__first, __last))
    _M_non_dbg_impl.insert(__pos._M_iterator, __first, __last);
  }
#endif

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert(iterator __pos, const value_type& __x = _Tp()) {
#else
  iterator insert(iterator __pos, const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    return iterator(&_M_iter_list, _M_non_dbg_impl.insert(__pos._M_iterator, __x));
  }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  iterator insert(iterator __pos) {
    return insert(__pos, _STLP_DEFAULT_CONSTRUCTED(_Tp));
  }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void insert(iterator __pos, size_type __n, const value_type& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    _M_non_dbg_impl.insert(__pos._M_iterator, __n, __x);
  }

public:
  iterator erase_after(iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    iterator __tmp = __pos; ++__tmp;
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__tmp))
    _Invalidate_iterator(__tmp);
    return iterator(&_M_iter_list, _M_non_dbg_impl.erase_after(__pos._M_iterator));
  }
  iterator erase_after(iterator __before_first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__before_first))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__before_first, __last, begin(), end()))
    iterator __tmp = __before_first; ++__tmp;
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__tmp))
    _Invalidate_iterators(__tmp, __last);
    return iterator(&_M_iter_list, _M_non_dbg_impl.erase_after(__before_first._M_iterator, __last._M_iterator));
  }

  iterator erase(iterator __pos) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_VERBOSE_ASSERT(__pos._M_iterator != _M_non_dbg_impl.before_begin(), _StlMsg_INVALID_ARGUMENT)
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _Invalidate_iterator(__pos);
    return iterator(&_M_iter_list, _M_non_dbg_impl.erase(__pos._M_iterator));
  }
  iterator erase(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last, begin(), end()))
    _Invalidate_iterators(__first, __last);
    return iterator(&_M_iter_list, _M_non_dbg_impl.erase(__first._M_iterator, __last._M_iterator));
  }

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const value_type& __x = _Tp()) {
#else
  void resize(size_type __new_size, const value_type& __x) {
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
    _M_non_dbg_impl.resize(__new_size, __x);
  }

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size) { resize(__new_size, _STLP_DEFAULT_CONSTRUCTED(_Tp)); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void clear() {
    _Invalidate_iterators(begin(), end());
    _M_non_dbg_impl.clear();
  }

public:
  // Removes all of the elements from the list __x to *this, inserting
  // them immediately after __pos.  __x must not be *this.  Complexity:
  // linear in __x.size().
  void splice_after(iterator __pos, _Self& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_VERBOSE_ASSERT(!(&__x == this), _StlMsg_INVALID_ARGUMENT)
    _M_non_dbg_impl.splice_after(__pos._M_iterator, __x._M_non_dbg_impl);
    if (get_allocator() == __x.get_allocator()) {
      __x._M_iter_list._Set_owner(_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__x.begin(), __x.end());
    }
  }

  // Moves the element that follows __prev to *this, inserting it immediately
  //  after __pos.  This is constant time.
  void splice_after(iterator __pos, _Self& __x, iterator __prev) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list, __pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__prev))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&__x._M_iter_list, __prev))
    iterator __elem = __prev; ++__elem;
    _M_non_dbg_impl.splice_after(__pos._M_iterator, __x._M_non_dbg_impl, __prev._M_iterator);
    if (get_allocator() == __x.get_allocator()) {
      _STLP_PRIV __change_ite_owner(__elem, &_M_iter_list);
    }
    else {
      __x._Invalidate_iterator(__elem);
    }
  }

  // Moves the range [__before_first + 1, __before_last + 1) to *this,
  //  inserting it immediately after __pos.  This is constant time.
  void splice_after(iterator __pos, _Self& __x,
                    iterator __before_first, iterator __before_last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__before_first, __before_last))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&__x._M_iter_list, __before_first))
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__before_first))
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__before_last))
    iterator __first = __before_first; ++__first;
    iterator __last = __before_last; ++__last;
    if (get_allocator() == __x.get_allocator()) {
      _STLP_PRIV __change_range_owner(__first, __last, &_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__first, __last);
    }
    _M_non_dbg_impl.splice_after(__pos._M_iterator, __x._M_non_dbg_impl,
                                 __before_first._M_iterator, __before_last._M_iterator);
  }

  void splice(iterator __pos, _Self& __x) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(!(&__x == this), _StlMsg_INVALID_ARGUMENT)
    _M_non_dbg_impl.splice(__pos._M_iterator, __x._M_non_dbg_impl);
    if (get_allocator() == __x.get_allocator()) {
      __x._M_iter_list._Set_owner(_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__x.begin(), __x.end());
    }
  }

  void splice(iterator __pos, _Self& __x, iterator __i) {
    //__pos should be owned by *this and not be the before_begin iterator
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    //__i should be dereferenceable, not before_begin and be owned by __x
    _STLP_DEBUG_CHECK(_STLP_PRIV _Dereferenceable(__i))
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&__x._M_iter_list ,__i))
    _STLP_VERBOSE_ASSERT(!(__i == __x.before_begin()), _StlMsg_INVALID_ARGUMENT)
    if (get_allocator() == __x.get_allocator()) {
      _STLP_PRIV __change_ite_owner(__i, &_M_iter_list);
    }
    else {
      __x._Invalidate_iterator(__i);
    }
    _M_non_dbg_impl.splice(__pos._M_iterator, __x._M_non_dbg_impl, __i._M_iterator);
  }

  void splice(iterator __pos, _Self& __x, iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_if_owner(&_M_iter_list,__pos))
    _STLP_VERBOSE_ASSERT(!(__pos._M_iterator == _M_non_dbg_impl.before_begin()), _StlMsg_INVALID_ARGUMENT)
    //_STLP_VERBOSE_ASSERT(&__x != this, _StlMsg_INVALID_ARGUMENT)
    _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last, __x.begin(), __x.end()))
    if (get_allocator() == __x.get_allocator()) {
      _STLP_PRIV __change_range_owner(__first, __last, &_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__first, __last);
    }
    _M_non_dbg_impl.splice(__pos._M_iterator, __x._M_non_dbg_impl,
                           __first._M_iterator, __last._M_iterator);
  }

  void reverse()
  { _M_non_dbg_impl.reverse(); }

  void remove(const value_type& __val) {
    _Base_iterator __first = _M_non_dbg_impl.begin(), __last = _M_non_dbg_impl.end();
    while (__first != __last) {
      _Base_iterator __next = __first;
      ++__next;
      if (__val == *__first) {
        _Invalidate_iterator(iterator(&_M_iter_list, __first));
        _M_non_dbg_impl.erase(__first);
      }
      __first = __next;
    }
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
  void merge(_Self& __x) {
    _STLP_VERBOSE_ASSERT(&__x != this, _StlMsg_INVALID_ARGUMENT)
#if !defined (_STLP_NO_EXTENSIONS)
    /* comments below due to bug in GCC compilers: ones eat all memory  and die if see
     * something like namespace_name::func_name() - ptr
     */
    _STLP_DEBUG_CHECK( /* _STLP_STD:: */ is_sorted(_M_non_dbg_impl.begin(), _M_non_dbg_impl.end()))
    _STLP_DEBUG_CHECK( /* _STLP_STD:: */ is_sorted(__x.begin()._M_iterator, __x.end()._M_iterator))
#endif
    _M_non_dbg_impl.merge(__x._M_non_dbg_impl);
    if (get_allocator() == __x.get_allocator()) {
      __x._M_iter_list._Set_owner(_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__x.begin(), __x.end());
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
  void unique(_BinaryPredicate __pred) {
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
  void merge(_Self& __x, _StrictWeakOrdering __ord) {
    _STLP_VERBOSE_ASSERT(&__x != this, _StlMsg_INVALID_ARGUMENT)
#if !defined (_STLP_NO_EXTENSIONS)
    /* comments below due to bug in GCC compilers: ones eat all memory  and die if see
     * something like namespace_name::func_name() - ptr
     */
    _STLP_DEBUG_CHECK( /* _STLP_STD:: */ is_sorted(_M_non_dbg_impl.begin(), _M_non_dbg_impl.end(), __ord))
    _STLP_DEBUG_CHECK( /* _STLP_STD:: */ is_sorted(__x.begin()._M_iterator, __x.end()._M_iterator, __ord))
#endif
    _M_non_dbg_impl.merge(__x._M_non_dbg_impl, __ord);
    if (get_allocator() == __x.get_allocator()) {
      __x._M_iter_list._Set_owner(_M_iter_list);
    }
    else {
      __x._Invalidate_iterators(__x.begin(), __x.end());
    }
  }

  template <class _StrictWeakOrdering>
  void sort(_StrictWeakOrdering __comp)
  { _M_non_dbg_impl.sort(__comp); }
#endif
};

_STLP_END_NAMESPACE

#undef _STLP_NON_DBG_SLIST

#endif /* _STLP_INTERNAL_DBG_SLIST_H */

// Local Variables:
// mode:C++
// End:
