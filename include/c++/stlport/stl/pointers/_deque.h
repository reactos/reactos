/*
 * Copyright (c) 2004
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

#ifndef _STLP_SPECIALIZED_DEQUE_H
#define _STLP_SPECIALIZED_DEQUE_H

#ifndef _STLP_POINTERS_SPEC_TOOLS_H
#  include <stl/pointers/_tools.h>
#endif

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

/*
 * struct helper to cast deque iterators:
 */
template <class _StorageT, class _ValueT>
struct _DequeIteCast {
  typedef _Deque_iterator<_ValueT, _Nonconst_traits<_ValueT> > iterator;
  typedef _Deque_iterator<_ValueT, _Const_traits<_ValueT> >    const_iterator;
  typedef _Deque_iterator<_StorageT, _Nonconst_traits<_StorageT> > storage_iterator;
  typedef _Deque_iterator<_StorageT, _Const_traits<_StorageT> > const_storage_iterator;
  typedef _CastTraits<_StorageT, _ValueT> cast_traits;

  static iterator to_value_type_ite (storage_iterator const& __ite) {
    iterator tmp;
    tmp._M_cur = cast_traits::to_value_type_ptr(__ite._M_cur);
    tmp._M_first = cast_traits::to_value_type_ptr(__ite._M_first);
    tmp._M_last = cast_traits::to_value_type_ptr(__ite._M_last);
    tmp._M_node = cast_traits::to_value_type_pptr(__ite._M_node);
    return tmp;
  }
  static storage_iterator to_storage_type_ite (iterator const& __ite) {
    storage_iterator tmp;
    tmp._M_cur = cast_traits::to_storage_type_ptr(__ite._M_cur);
    tmp._M_first = cast_traits::to_storage_type_ptr(__ite._M_first);
    tmp._M_last = cast_traits::to_storage_type_ptr(__ite._M_last);
    tmp._M_node = cast_traits::to_storage_type_pptr(__ite._M_node);
    return tmp;
  }

  static const_iterator to_value_type_cite (const_storage_iterator const& __ite) {
    const_iterator tmp;
    tmp._M_cur = cast_traits::to_value_type_ptr(__ite._M_cur);
    tmp._M_first = cast_traits::to_value_type_ptr(__ite._M_first);
    tmp._M_last = cast_traits::to_value_type_ptr(__ite._M_last);
    tmp._M_node = cast_traits::to_value_type_pptr(__ite._M_node);
    return tmp;
  }

  static const_storage_iterator to_storage_type_cite (const_iterator const& __ite) {
    const_storage_iterator tmp;
    tmp._M_cur = cast_traits::to_storage_type_ptr(__ite._M_cur);
    tmp._M_first = cast_traits::to_storage_type_ptr(__ite._M_first);
    tmp._M_last = cast_traits::to_storage_type_ptr(__ite._M_last);
    tmp._M_node = cast_traits::to_storage_type_pptr(__ite._M_node);
    return tmp;
  }
};

#define DEQUE_IMPL _STLP_PTR_IMPL_NAME(deque)
#if defined (__BORLANDC__) || defined (__DMC__)
#  define typename
#endif

#if defined (_STLP_USE_TEMPLATE_EXPORT) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
_STLP_EXPORT_TEMPLATE_CLASS _STLP_alloc_proxy<size_t, void*,  allocator<void*> >;
_STLP_EXPORT_TEMPLATE_CLASS _STLP_alloc_proxy<void***, void**,  allocator<void**> >;
_STLP_EXPORT template struct _STLP_CLASS_DECLSPEC _Deque_iterator<void*, _Nonconst_traits<void*> >;
_STLP_EXPORT_TEMPLATE_CLASS _Deque_base<void*,allocator<void*> >;
_STLP_EXPORT_TEMPLATE_CLASS DEQUE_IMPL<void*,allocator<void*> >;
#endif

#if defined (_STLP_DEBUG)
#  define deque _STLP_NON_DBG_NAME(deque)
#else
_STLP_MOVE_TO_STD_NAMESPACE
#endif

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class deque
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (deque)
            : public __stlport_class<deque<_Tp, _Alloc> >
#endif
{
  typedef _STLP_TYPENAME _STLP_PRIV _StorageType<_Tp>::_Type _StorageType;
  typedef typename _Alloc_traits<_StorageType, _Alloc>::allocator_type _StorageTypeAlloc;
  typedef _STLP_PRIV DEQUE_IMPL<_StorageType, _StorageTypeAlloc> _Base;
  typedef deque<_Tp, _Alloc> _Self;

  typedef _STLP_PRIV _CastTraits<_StorageType, _Tp> cast_traits;
  typedef _STLP_PRIV _DequeIteCast<_StorageType, _Tp> ite_cast_traits;

public:
  typedef _Tp value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef random_access_iterator_tag _Iterator_category;
  _STLP_FORCE_ALLOCATORS(value_type, _Alloc)
  typedef typename _Alloc_traits<value_type, _Alloc>::allocator_type allocator_type;
  typedef _STLP_PRIV _Deque_iterator<value_type, _Nonconst_traits<value_type> > iterator;
  typedef _STLP_PRIV _Deque_iterator<value_type, _Const_traits<value_type> >    const_iterator;

  _STLP_DECLARE_RANDOM_ACCESS_REVERSE_ITERATORS;

public:                         // Basic accessors
  iterator begin() { return ite_cast_traits::to_value_type_ite(_M_impl.begin()); }
  iterator end()   { return ite_cast_traits::to_value_type_ite(_M_impl.end()); }
  const_iterator begin() const { return ite_cast_traits::to_value_type_cite(_M_impl.begin()); }
  const_iterator end() const   { return ite_cast_traits::to_value_type_cite(_M_impl.end()); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend()   { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const
  { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const
  { return const_reverse_iterator(begin()); }

  reference operator[](size_type __n)
  { return cast_traits::to_value_type_ref(_M_impl[__n]); }
  const_reference operator[](size_type __n) const
  { return cast_traits::to_value_type_cref(_M_impl[__n]); }

  reference at(size_type __n)
  { return cast_traits::to_value_type_ref(_M_impl.at(__n)); }
  const_reference at(size_type __n) const
  { return cast_traits::to_value_type_cref(_M_impl.at(__n)); }

  reference front() { return cast_traits::to_value_type_ref(_M_impl.front()); }
  reference back()  { return cast_traits::to_value_type_ref(_M_impl.back()); }
  const_reference front() const { return cast_traits::to_value_type_cref(_M_impl.front()); }
  const_reference back() const  { return cast_traits::to_value_type_cref(_M_impl.back()); }

  size_type size() const     { return _M_impl.size(); }
  size_type max_size() const { return _M_impl.max_size(); }
  bool empty() const         { return _M_impl.empty(); }
  allocator_type get_allocator() const { return _STLP_CONVERT_ALLOCATOR(_M_impl.get_allocator(), value_type); }

  explicit deque(const allocator_type& __a = allocator_type())
    : _M_impl(_STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

  deque(const _Self& __x) : _M_impl(__x._M_impl) {}

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit deque(size_type __n, const value_type& __val = _STLP_DEFAULT_CONSTRUCTED(value_type),
#else
  deque(size_type __n, const value_type& __val,
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
        const allocator_type& __a = allocator_type())
    : _M_impl(__n, cast_traits::to_storage_type_cref(__val), _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}
  // int,long variants may be needed
#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit deque(size_type __n) : _M_impl(__n) {}
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  deque(_InputIterator __first, _InputIterator __last,
        const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
#if !defined (_STLP_USE_ITERATOR_WRAPPER)
  : _M_impl(__first, __last,
            _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {
#else
  : _M_impl(_STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {
#endif
#if defined (_STLP_USE_ITERATOR_WRAPPER)
    insert(end(), __first, __last);
#endif
  }

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  deque(_InputIterator __first, _InputIterator __last)
#    if !defined (_STLP_USE_ITERATOR_WRAPPER)
    : _M_impl(__first, __last) {}
#    else
  { insert(end(), __first, __last); }
#    endif
#  endif

#else
  deque(const_pointer __first, const_pointer __last,
        const allocator_type& __a = allocator_type() )
    : _M_impl(cast_traits::to_storage_type_cptr(__first),
              cast_traits::to_storage_type_cptr(__last),
              _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

  deque(const_iterator __first, const_iterator __last,
        const allocator_type& __a = allocator_type() )
    : _M_impl(ite_cast_traits::to_storage_type_cite(__first),
              ite_cast_traits::to_storage_type_cite(__last),
              _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}
#endif /* _STLP_MEMBER_TEMPLATES */

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  deque(__move_source<_Self> src)
    : _M_impl(__move_source<_Base>(src.get()._M_impl)) {}
#endif

  _Self& operator= (const _Self& __x) { _M_impl = __x._M_impl; return *this; }

  void swap(_Self& __x) { _M_impl.swap(__x._M_impl); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  void assign(size_type __n, const value_type& __val) {
    _M_impl.assign(__n, cast_traits::to_storage_type_cref(__val));
  }

#if defined (_STLP_MEMBER_TEMPLATES)
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
private:
  template <class _Integer>
  void _M_assign_dispatch(_Integer __n, _Integer __val,
                          const __true_type&)
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
  void assign(const_pointer __first, const_pointer __last)
  { _M_impl.assign(cast_traits::to_storage_type_cptr(__first),
                   cast_traits::to_storage_type_cptr(__last)); }
  void assign(const_iterator __first, const_iterator __last)
  { _M_impl.assign(ite_cast_traits::to_storage_type_cite(__first),
                   ite_cast_traits::to_storage_type_cite(__last)); }
#endif /* _STLP_MEMBER_TEMPLATES */

#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_back(const value_type& __t = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  void push_back(const value_type& __t)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
  { _M_impl.push_back(cast_traits::to_storage_type_cref(__t)); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_front(const value_type& __t = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  void push_front(const value_type& __t)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
  { _M_impl.push_front(cast_traits::to_storage_type_cref(__t)); }

# if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  void push_back()  { _M_impl.push_back(); }
  void push_front() { _M_impl.push_front(); }
# endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

  void pop_back()  { _M_impl.pop_back(); }
  void pop_front() { _M_impl.pop_front(); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos, const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  iterator insert(iterator __pos, const value_type& __x)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
  { return ite_cast_traits::to_value_type_ite(_M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos),
                                                             cast_traits::to_storage_type_cref(__x))); }

#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos) { return insert(__pos, _STLP_DEFAULT_CONSTRUCTED(value_type)); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

  void insert(iterator __pos, size_type __n, const value_type& __x)
  { _M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos), __n, cast_traits::to_storage_type_cref(__x)); }

#if defined (_STLP_MEMBER_TEMPLATES)
#  if defined (_STLP_USE_ITERATOR_WRAPPER)
private:
  template <class _Integer>
  void _M_insert_dispatch(iterator __pos, _Integer __n, _Integer __val,
                          const __true_type&) {
    _M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos), __n, __val);
  }

  template <class _InputIterator>
  void _M_insert_dispatch(iterator __pos,
                          _InputIterator __first, _InputIterator __last,
                          const __false_type&) {
    _M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos),
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
    _M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos), __first, __last);
#  endif
  }

#else /* _STLP_MEMBER_TEMPLATES */
  void insert(iterator __pos,
              const_pointer __first, const_pointer __last) {
    _M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos),
                   cast_traits::to_storage_type_cptr(__first),
                   cast_traits::to_storage_type_cptr(__last));
  }
  void insert(iterator __pos,
              const_iterator __first, const_iterator __last) {
    _M_impl.insert(ite_cast_traits::to_storage_type_ite(__pos),
                   ite_cast_traits::to_storage_type_cite(__first),
                   ite_cast_traits::to_storage_type_cite(__last));
  }

#endif /* _STLP_MEMBER_TEMPLATES */

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  void resize(size_type __new_size, const value_type& __x)
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
  { _M_impl.resize(__new_size, cast_traits::to_storage_type_cref(__x)); }

#if defined (_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size) { _M_impl.resize(__new_size); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  iterator erase(iterator __pos)
  { return ite_cast_traits::to_value_type_ite(_M_impl.erase(ite_cast_traits::to_storage_type_ite(__pos))); }

  iterator erase(iterator __first, iterator __last)
  { return ite_cast_traits::to_value_type_ite(_M_impl.erase(ite_cast_traits::to_storage_type_ite(__first),
                                                            ite_cast_traits::to_storage_type_ite(__last))); }
  void clear() { _M_impl.clear(); }

private:
  _Base _M_impl;
};

#if defined (deque)
#  undef deque
_STLP_MOVE_TO_STD_NAMESPACE
#endif

#undef DEQUE_IMPL
#if defined (__BORLANDC__) || defined (__DMC__)
#  undef typename
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_SPECIALIZED_DEQUE_H */

// Local Variables:
// mode:C++
// End:
