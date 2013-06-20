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

#ifndef _STLP_SPECIALIZED_VECTOR_H
#define _STLP_SPECIALIZED_VECTOR_H

#ifndef _STLP_POINTERS_SPEC_TOOLS_H
#  include <stl/pointers/_tools.h>
#endif

_STLP_BEGIN_NAMESPACE

#define VECTOR_IMPL _STLP_PTR_IMPL_NAME(vector)

#if defined (_STLP_USE_TEMPLATE_EXPORT) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
_STLP_EXPORT_TEMPLATE_CLASS _STLP_PRIV _Vector_base<void*,allocator<void*> >;
_STLP_EXPORT_TEMPLATE_CLASS _STLP_PRIV VECTOR_IMPL<void*, allocator<void*> >;
#endif

#if defined (_STLP_DEBUG)
#  define vector _STLP_NON_DBG_NAME(vector)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

template <class _Tp, _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Tp>) >
class vector
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (vector)
             : public __stlport_class<vector<_Tp, _Alloc> >
#endif
{
  /* In the vector implementation iterators are pointer which give a number
   * of opportunities for optimization. To not break those optimizations
   * iterators passed to template should not be wrapped for casting purpose.
   * So vector implementation will always use a qualified void pointer type and
   * won't use iterator wrapping.
   */
  typedef _STLP_TYPENAME _STLP_PRIV _StorageType<_Tp>::_QualifiedType _StorageType;
  typedef typename _Alloc_traits<_StorageType, _Alloc>::allocator_type _StorageTypeAlloc;
  typedef _STLP_PRIV VECTOR_IMPL<_StorageType, _StorageTypeAlloc> _Base;
  typedef vector<_Tp, _Alloc> _Self;

  typedef _STLP_PRIV _CastTraits<_StorageType, _Tp> cast_traits;

public:
  typedef _Tp value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef random_access_iterator_tag _Iterator_category;

  _STLP_DECLARE_RANDOM_ACCESS_REVERSE_ITERATORS;
  _STLP_FORCE_ALLOCATORS(value_type, _Alloc)
  typedef typename _Alloc_traits<value_type, _Alloc>::allocator_type allocator_type;

  allocator_type get_allocator() const
  { return _STLP_CONVERT_ALLOCATOR(_M_impl.get_allocator(), value_type); }

  iterator begin()             { return cast_traits::to_value_type_ptr(_M_impl.begin()); }
  const_iterator begin() const { return cast_traits::to_value_type_cptr(_M_impl.begin()); }
  iterator end()               { return cast_traits::to_value_type_ptr(_M_impl.end()); }
  const_iterator end() const   { return cast_traits::to_value_type_cptr(_M_impl.end()); }

  reverse_iterator rbegin()              { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const  { return const_reverse_iterator(end()); }
  reverse_iterator rend()                { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const    { return const_reverse_iterator(begin()); }

  size_type size() const        { return _M_impl.size(); }
  size_type max_size() const    { return _M_impl.max_size(); }

  size_type capacity() const    { return _M_impl.capacity(); }
  bool empty() const            { return _M_impl.empty(); }

  reference operator[](size_type __n) { return cast_traits::to_value_type_ref(_M_impl[__n]); }
  const_reference operator[](size_type __n) const { return cast_traits::to_value_type_cref(_M_impl[__n]); }

  reference front()             { return cast_traits::to_value_type_ref(_M_impl.front()); }
  const_reference front() const { return cast_traits::to_value_type_cref(_M_impl.front()); }
  reference back()              { return cast_traits::to_value_type_ref(_M_impl.back()); }
  const_reference back() const  { return cast_traits::to_value_type_cref(_M_impl.back()); }

  reference at(size_type __n) { return cast_traits::to_value_type_ref(_M_impl.at(__n)); }
  const_reference at(size_type __n) const { return cast_traits::to_value_type_cref(_M_impl.at(__n)); }

  explicit vector(const allocator_type& __a = allocator_type())
    : _M_impl(_STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit vector(size_type __n, const value_type& __val = _STLP_DEFAULT_CONSTRUCTED(value_type),
#else
  vector(size_type __n, const value_type& __val,
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
         const allocator_type& __a = allocator_type())
    : _M_impl(__n, cast_traits::to_storage_type_cref(__val),
      _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  explicit vector(size_type __n)
    : _M_impl(__n, allocator_type() ) {}
#endif

  vector(const _Self& __x)
    : _M_impl(__x._M_impl) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  explicit vector(__move_source<_Self> src)
    : _M_impl(__move_source<_Base>(src.get()._M_impl)) {}
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  vector(_InputIterator __first, _InputIterator __last,
         const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL )
  : _M_impl(__first, __last,
            _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  vector(_InputIterator __first, _InputIterator __last)
    : _M_impl(__first, __last) {}
#  endif

#else
  vector(const_iterator __first, const_iterator __last,
         const allocator_type& __a = allocator_type())
    : _M_impl(cast_traits::to_storage_type_cptr(__first), cast_traits::to_storage_type_cptr(__last),
              _STLP_CONVERT_ALLOCATOR(__a, _StorageType)) {}
#endif /* _STLP_MEMBER_TEMPLATES */

  _Self& operator=(const _Self& __x) { _M_impl = __x._M_impl; return *this; }

  void reserve(size_type __n) {_M_impl.reserve(__n);}
  void assign(size_type __n, const value_type& __val)
  { _M_impl.assign(__n, cast_traits::to_storage_type_cref(__val)); }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void assign(_InputIterator __first, _InputIterator __last)
  { _M_impl.assign(__first, __last); }
#else
  void assign(const_iterator __first, const_iterator __last) {
    _M_impl.assign(cast_traits::to_storage_type_cptr(__first),
                   cast_traits::to_storage_type_cptr(__last));
  }
#endif /* _STLP_MEMBER_TEMPLATES */

#if !defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
  void push_back(const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  void push_back(const value_type& __x)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
  { _M_impl.push_back(cast_traits::to_storage_type_cref(__x)); }

#if !defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
  iterator insert(iterator __pos, const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  iterator insert(iterator __pos, const value_type& __x)
#endif /*!_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/
  { return cast_traits::to_value_type_ptr(_M_impl.insert(cast_traits::to_storage_type_ptr(__pos),
                                                         cast_traits::to_storage_type_cref(__x))); }

#if defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
  void push_back() { _M_impl.push_back(); }
  iterator insert(iterator __pos)
  { return _M_impl.insert(cast_traits::to_storage_type_ptr(__pos)); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

  void swap(_Self& __x) { _M_impl.swap(__x._M_impl); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(iterator __pos, _InputIterator __first, _InputIterator __last)
  { _M_impl.insert(cast_traits::to_storage_type_ptr(__pos), __first, __last); }
#else
  void insert(iterator __pos, const_iterator __first, const_iterator __last) {
    _M_impl.insert(cast_traits::to_storage_type_ptr(__pos), cast_traits::to_storage_type_cptr(__first),
                                                            cast_traits::to_storage_type_cptr(__last));
  }
#endif

  void insert (iterator __pos, size_type __n, const value_type& __x) {
    _M_impl.insert(cast_traits::to_storage_type_ptr(__pos), __n, cast_traits::to_storage_type_cref(__x));
  }

  void pop_back() {_M_impl.pop_back();}
  iterator erase(iterator __pos)
  {return cast_traits::to_value_type_ptr(_M_impl.erase(cast_traits::to_storage_type_ptr(__pos)));}
  iterator erase(iterator __first, iterator __last) {
    return cast_traits::to_value_type_ptr(_M_impl.erase(cast_traits::to_storage_type_ptr(__first),
                                                        cast_traits::to_storage_type_ptr(__last)));
  }

#if !defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size, const value_type& __x = _STLP_DEFAULT_CONSTRUCTED(value_type))
#else
  void resize(size_type __new_size, const value_type& __x)
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/
  { _M_impl.resize(__new_size, cast_traits::to_storage_type_cref(__x)); }

#if defined(_STLP_DONT_SUP_DFLT_PARAM)
  void resize(size_type __new_size) { _M_impl.resize(__new_size); }
#endif /*_STLP_DONT_SUP_DFLT_PARAM*/

  void clear() { _M_impl.clear(); }

private:
  _Base _M_impl;
};

#if defined (vector)
#  undef vector
_STLP_MOVE_TO_STD_NAMESPACE
#endif

#undef VECTOR_IMPL

_STLP_END_NAMESPACE

#endif /* _STLP_SPECIALIZED_VECTOR_H */
