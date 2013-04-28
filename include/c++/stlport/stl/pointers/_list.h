/*
 * Copyright (c) 2003
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
 *
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_PTR_SPECIALIZED_LIST_H
#define _STLP_PTR_SPECIALIZED_LIST_H

#ifndef _STLP_POINTERS_SPEC_TOOLS_H
#  include <stl/pointers/_tools.h>
#endif

_STLP_BEGIN_NAMESPACE

#define LIST_IMPL _STLP_PTR_IMPL_NAME(list)
#if defined (__BORLANDC__) || defined (__DMC__)
#  define typename
#endif

#if defined (_STLP_USE_TEMPLATE_EXPORT) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)

_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_EXPORT_TEMPLATE_CLASS _List_node<void*>;

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_EXPORT_TEMPLATE_CLASS allocator<_STLP_PRIV _List_node<void*> >;

_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_EXPORT_TEMPLATE_CLASS _STLP_alloc_proxy<_List_node_base, _List_node<void*>, allocator<_List_node<void*> > >;
_STLP_EXPORT_TEMPLATE_CLASS _List_base<void*, allocator<void*> >;
_STLP_EXPORT_TEMPLATE_CLASS LIST_IMPL<void*, allocator<void*> >;

_STLP_MOVE_TO_STD_NAMESPACE
#endif

#if defined (_STLP_DEBUG)
#  define list _STLP_NON_DBG_NAME(list)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class list
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (list)
           : public __stlport_class<list<_Tp, _Alloc> >
#endif
{
  typedef _STLP_TYPENAME _STLP_PRIV _StorageType<_Tp>::_Type _StorageType;
  typedef typename _Alloc_traits<_StorageType, _Alloc>::allocator_type _StorageTypeAlloc;
  typedef _STLP_PRIV LIST_IMPL<_StorageType, _StorageTypeAlloc> _Base;
  typedef typename _Base::iterator _BaseIte;
  typedef typename _Base::const_iterator _BaseConstIte;
  typedef _STLP_PRIV _CastTraits<_StorageType, _Tp> cast_traits;
  typedef list<_Tp, _Alloc> _Self;

public:
  typedef _Tp value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  _STLP_FORCE_ALLOCATORS(value_type, _Alloc)
  typedef typename _Alloc_traits<value_type, _Alloc>::allocator_type allocator_type;
  typedef bidirectional_iterator_tag _Iterator_category;

  typedef _STLP_PRIV _List_iterator<value_type, _Nonconst_traits<value_type> > iterator;
  typedef _STLP_PRIV _List_iterator<value_type, _Const_traits<value_type> >    const_iterator;

  _STLP_DECLARE_BIDIRECTIONAL_REVERSE_ITERATORS;

  allocator_type get_allocator() const
  { return _STLP_CONVERT_ALLOCATOR(_M_impl.get_allocator(), value_type); }

  explicit list(const allocator_type& __a = allocator_type())
    : _M_impl(_STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit list(size_type __n, const value_type& __val = _STLP_DEFAULT_CONSTRUCTED(value_type),
#else
  list(size_type __n, const value_type& __val,
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
       const allocator_type& __a = allocator_type())
    : _M_impl(__n, cast_traits::to_storage_type_cref(__val),
              _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit list(size_type __n)
    : _M_impl(__n) {}
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  list(_InputIterator __first, _InputIterator __last,
       const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
#  if !defined (_STLP_USE_ITERATOR_WRAPPER)
    : _M_impl(__first, __last, _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}
#  else
    : _M_impl(_STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {
    insert(begin(), __first, __last);
  }
#  endif

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  list(_InputIterator __first, _InputIterator __last)
#    if !defined (_STLP_USE_WRAPPER_ITERATOR)
    : _M_impl(__first, __last) {}
#    else
  { insert(begin(), __first, __last); }
#    endif
#  endif

#else /* _STLP_MEMBER_TEMPLATES */

  list(const value_type *__first, const value_type *__last,
       const allocator_type& __a = allocator_type())
    : _M_impl(cast_traits::to_storage_type_cptr(__first),
              cast_traits::to_storage_type_cptr(__last),
               _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}
  list(const_iterator __first, const_iterator __last,
       const allocator_type& __a = allocator_type())
    : _M_impl(_BaseConstIte(__first._M_node), _BaseConstIte(__last._M_node),
              _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

#endif /* _STLP_MEMBER_TEMPLATES */

  list(const _Self& __x) : _M_impl(__x._M_impl) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  list(__move_source<_Self> src)
    : _M_impl(__move_source<_Base>(src.get()._M_impl)) {}
#endif

  iterator begin()             { return iterator(_M_impl.begin()._M_node); }
  const_iterator begin() const { return const_iterator(_M_impl.begin()._M_node); }

  iterator end()               { return iterator(_M_impl.end()._M_node); }
  const_iterator end() const   { return const_iterator(_M_impl.end()._M_node); }

  reverse_iterator rbegin()             { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

  reverse_iterator rend()               { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const   { return const_reverse_iterator(begin()); }

  bool empty() const         { return _M_impl.empty(); }
  size_type size() const     { return _M_impl.size(); }
  size_type max_size() const { return _M_impl.max_size(); }

  reference front()             { return *begin(); }
  const_reference front() const { return *begin(); }
  reference back()              { return *(--end()); }
  const_reference back() const  { return *(--end()); }

  void swap(_Self &__x) { _M_impl.swap(__x._M_impl); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif
  void clear() { _M_impl.clear(); }

#if !defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos, const_reference __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  iterator insert(iterator __pos, const_reference __x)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
  { return iterator(_M_impl.insert(_BaseIte(__pos._M_node),
                                   cast_traits::to_storage_type_cref(__x))._M_node); }

#if defined (_STLP_MEMBER_TEMPLATES)
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
private:
  template <class _Integer>
  void _M_insert_dispatch(iterator __pos, _Integer __n, _Integer __val,
                          const __true_type&)
  { _M_impl.insert(_BaseIte(__pos._M_node), __n, __val); }

  template <class _InputIterator>
  void _M_insert_dispatch(iterator __pos,
                          _InputIterator __first, _InputIterator __last,
                          const __false_type&) {
    _M_impl.insert(_BaseIte(__pos._M_node),
                   _STLP_TYPENAME _STLP_PRIV _IteWrapper<_StorageType, _Tp, _InputIterator>::_Ite(__first),
                   _STLP_TYPENAME _STLP_PRIV _IteWrapper<_StorageType, _Tp, _InputIterator>::_Ite(__last));
  }

public:
#  endif

  template <class _InputIterator>
  void insert(iterator __pos, _InputIterator __first, _InputIterator __last) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    // Check whether it's an integral type.  If so, it's not an iterator.
    typedef typename _IsIntegral<_InputIterator>::_Ret _Integral;
    _M_insert_dispatch(__pos, __first, __last, _Integral());
#  else
    _M_impl.insert(_BaseIte(__pos._M_node), __first, __last);
#  endif
  }
#else /* _STLP_MEMBER_TEMPLATES */
  void insert(iterator __pos, const value_type *__first, const value_type *__last)
  { _M_impl.insert(_BaseIte(__pos._M_node), cast_traits::to_storage_type_cptr(__first),
                                            cast_traits::to_storage_type_cptr(__last)); }
  void insert(iterator __pos, const_iterator __first, const_iterator __last)
  { _M_impl.insert(_BaseIte(__pos._M_node), _BaseConstIte(__first._M_node), _BaseConstIte(__last._M_node)); }
#endif /* _STLP_MEMBER_TEMPLATES */

  void insert(iterator __pos, size_type __n, const value_type& __x)
  { _M_impl.insert(_BaseIte(__pos._M_node), __n, cast_traits::to_storage_type_cref(__x)); }

  void push_front(const value_type& __x) { _M_impl.push_front(cast_traits::to_storage_type_cref(__x)); }
  void push_back(const value_type& __x)  { _M_impl.push_back(cast_traits::to_storage_type_cref(__x)); }

#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos) { return iterator(_M_impl.insert(__pos._M_node)._M_node); }
  void push_front() { _M_impl.push_front();}
  void push_back()  { _M_impl.push_back();}
# endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

  iterator erase(iterator __pos)
  { return iterator(_M_impl.erase(_BaseIte(__pos._M_node))._M_node); }
  iterator erase(iterator __first, iterator __last)
  { return iterator(_M_impl.erase(_BaseIte(__first._M_node), _BaseIte(__last._M_node))._M_node); }

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  void resize(size_type __new_size) { _M_impl.resize(__new_size); }
  void resize(size_type __new_size, const value_type& __x)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM*/
  {_M_impl.resize(__new_size, cast_traits::to_storage_type_cref(__x));}

  void pop_front() { _M_impl.pop_front(); }
  void pop_back()  { _M_impl.pop_back(); }

  _Self& operator=(const _Self& __x)
  { _M_impl = __x._M_impl; return *this; }
  void assign(size_type __n, const value_type& __val)
  { _M_impl.assign(__n, cast_traits::to_storage_type_cref(__val)); }

#if defined (_STLP_MEMBER_TEMPLATES)
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
private:
  template <class _Integer>
  void _M_assign_dispatch(_Integer __n, _Integer __val, const __true_type&)
  { _M_impl.assign(__n, __val); }

  template <class _InputIterator>
  void _M_assign_dispatch(_InputIterator __first, _InputIterator __last,
                          const __false_type&) {
    _M_impl.assign(_STLP_TYPENAME _STLP_PRIV _IteWrapper<_StorageType, _Tp, _InputIterator>::_Ite(__first),
                   _STLP_TYPENAME _STLP_PRIV _IteWrapper<_StorageType, _Tp, _InputIterator>::_Ite(__last));
  }

public:
#  endif

  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last) {
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
    typedef typename _IsIntegral<_InputIterator>::_Ret _Integral;
    _M_assign_dispatch(__first, __last, _Integral());
#  else
    _M_impl.assign(__first, __last);
#  endif
  }
#else
  void assign(const value_type *__first, const value_type *__last) {
    _M_impl.assign(cast_traits::to_storage_type_cptr(__first),
                   cast_traits::to_storage_type_cptr(__last));
  }
  void assign(const_iterator __first, const_iterator __last)
  { _M_impl.assign(_BaseConstIte(__first._M_node), _BaseConstIte(__last._M_node)); }
#endif

  void splice(iterator __pos, _Self& __x)
  { _M_impl.splice(_BaseIte(__pos._M_node), __x._M_impl); }
  void splice(iterator __pos, _Self& __x, iterator __i)
  { _M_impl.splice(_BaseIte(__pos._M_node), __x._M_impl, _BaseIte(__i._M_node)); }
  void splice(iterator __pos, _Self& __x, iterator __first, iterator __last)
  { _M_impl.splice(_BaseIte(__pos._M_node), __x._M_impl,
                   _BaseIte(__first._M_node), _BaseIte(__last._M_node)); }

  void remove(const_reference __val)
  { _M_impl.remove(cast_traits::to_storage_type_cref(__val)); }
  void unique() { _M_impl.unique(); }
  void merge(_Self& __x) { _M_impl.merge(__x._M_impl); }
  void reverse() { _M_impl.reverse(); }
  void sort() { _M_impl.sort(); }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _Predicate>
  void remove_if(_Predicate __pred)
  { _M_impl.remove_if(_STLP_PRIV _UnaryPredWrapper<_StorageType, _Tp, _Predicate>(__pred)); }
  template <class _BinaryPredicate>
  void unique(_BinaryPredicate __bin_pred)
  { _M_impl.unique(_STLP_PRIV _BinaryPredWrapper<_StorageType, _Tp, _BinaryPredicate>(__bin_pred)); }

  template <class _StrictWeakOrdering>
  void merge(_Self &__x, _StrictWeakOrdering __comp)
  { _M_impl.merge(__x._M_impl, _STLP_PRIV _BinaryPredWrapper<_StorageType, _Tp, _StrictWeakOrdering>(__comp)); }

  template <class _StrictWeakOrdering>
  void sort(_StrictWeakOrdering __comp)
  { _M_impl.sort(_STLP_PRIV _BinaryPredWrapper<_StorageType, _Tp, _StrictWeakOrdering>(__comp)); }
#endif /* _STLP_MEMBER_TEMPLATES */

private:
  _Base _M_impl;
};

#if defined (list)
#  undef list
_STLP_MOVE_TO_STD_NAMESPACE
#endif

#undef LIST_IMPL
#if defined (__BORLANDC__) || defined (__DMC__)
#  undef typename
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_PTR_SPECIALIZED_LIST_H */

// Local Variables:
// mode:C++
// End:
