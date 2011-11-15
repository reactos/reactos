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

#ifndef _STLP_INTERNAL_HASH_SET_H
#define _STLP_INTERNAL_HASH_SET_H

#ifndef _STLP_INTERNAL_HASHTABLE_H
#  include <stl/_hashtable.h>
#endif

_STLP_BEGIN_NAMESPACE

//Specific iterator traits creation
_STLP_CREATE_HASH_ITERATOR_TRAITS(HashSetTraitsT, Const_traits)

template <class _Value, _STLP_DFL_TMPL_PARAM(_HashFcn,hash<_Value>),
          _STLP_DFL_TMPL_PARAM(_EqualKey, equal_to<_Value>),
          _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Value>) >
class hash_set
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
               : public __stlport_class<hash_set<_Value, _HashFcn, _EqualKey, _Alloc> >
#endif
{
  typedef hash_set<_Value, _HashFcn, _EqualKey, _Alloc> _Self;
  //Specific iterator traits creation
  typedef _STLP_PRIV _HashSetTraitsT<_Value> _HashSetTraits;
public:
  typedef hashtable<_Value, _Value, _HashFcn,
                    _HashSetTraits, _STLP_PRIV _Identity<_Value>, _EqualKey, _Alloc> _Ht;
public:
  typedef typename _Ht::key_type key_type;
  typedef typename _Ht::value_type value_type;
  typedef typename _Ht::hasher hasher;
  typedef typename _Ht::key_equal key_equal;

  typedef typename _Ht::size_type size_type;
  typedef typename _Ht::difference_type difference_type;
  typedef typename _Ht::pointer         pointer;
  typedef typename _Ht::const_pointer   const_pointer;
  typedef typename _Ht::reference       reference;
  typedef typename _Ht::const_reference const_reference;

  typedef typename _Ht::iterator iterator;
  typedef typename _Ht::const_iterator const_iterator;

  typedef typename _Ht::allocator_type allocator_type;

  hasher hash_funct() const { return _M_ht.hash_funct(); }
  key_equal key_eq() const { return _M_ht.key_eq(); }
  allocator_type get_allocator() const { return _M_ht.get_allocator(); }

private:
  _Ht _M_ht;
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

public:
  hash_set()
    : _M_ht(0, hasher(), key_equal(), allocator_type()) {}
  explicit hash_set(size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type()) {}
  hash_set(size_type __n, const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type()) {}
#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  hash_set(size_type __n, const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a = allocator_type())
#else
  hash_set(size_type __n, const hasher& __hf, const key_equal& __eql)
    : _M_ht(__n, __hf, __eql, allocator_type()) {}
  hash_set(size_type __n, const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a)
#endif
    : _M_ht(__n, __hf, __eql, __a) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  hash_set(__move_source<_Self> src)
    : _M_ht(__move_source<_Ht>(src.get()._M_ht)) {}
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l)
    : _M_ht(0, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n,
           const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n,
           const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_unique(__f, __l); }
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n,
           const hasher& __hf, const key_equal& __eql)
    : _M_ht(__n, __hf, __eql, allocator_type())
    { _M_ht.insert_unique(__f, __l); }
#  endif
#else
  hash_set(const value_type* __f, const value_type* __l)
    : _M_ht(0, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  hash_set(const value_type* __f, const value_type* __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  hash_set(const value_type* __f, const value_type* __l, size_type __n,
           const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  hash_set(const value_type* __f, const value_type* __l, size_type __n,
           const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_unique(__f, __l); }

  hash_set(const_iterator __f, const_iterator __l)
    : _M_ht(0, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  hash_set(const_iterator __f, const_iterator __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  hash_set(const_iterator __f, const_iterator __l, size_type __n,
           const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  hash_set(const_iterator __f, const_iterator __l, size_type __n,
           const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_unique(__f, __l); }
#endif /*_STLP_MEMBER_TEMPLATES */

public:
  size_type size() const { return _M_ht.size(); }
  size_type max_size() const { return _M_ht.max_size(); }
  bool empty() const { return _M_ht.empty(); }
  void swap(_Self& __hs) { _M_ht.swap(__hs._M_ht); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  iterator begin() { return _M_ht.begin(); }
  iterator end() { return _M_ht.end(); }
  const_iterator begin() const { return _M_ht.begin(); }
  const_iterator end() const { return _M_ht.end(); }

public:
  pair<iterator, bool> insert(const value_type& __obj)
  { return _M_ht.insert_unique(__obj); }
#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(_InputIterator __f, _InputIterator __l)
#else
  void insert(const_iterator __f, const_iterator __l)
  {_M_ht.insert_unique(__f, __l); }
  void insert(const value_type* __f, const value_type* __l)
#endif
  { _M_ht.insert_unique(__f,__l); }

  pair<iterator, bool> insert_noresize(const value_type& __obj)
  { return _M_ht.insert_unique_noresize(__obj); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __key) { return _M_ht.find(__key); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __key) const { return _M_ht.find(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __key) const { return _M_ht.count(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator, iterator> equal_range(const _KT& __key)
  { return _M_ht.equal_range(__key); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __key) const
  { return _M_ht.equal_range(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type erase(const _KT& __key) {return _M_ht.erase(__key); }
  void erase(iterator __it) { _M_ht.erase(__it); }
  void erase(iterator __f, iterator __l) { _M_ht.erase(__f, __l); }
  void clear() { _M_ht.clear(); }

public:
  void resize(size_type __hint) { _M_ht.resize(__hint); }
  size_type bucket_count() const { return _M_ht.bucket_count(); }
  size_type max_bucket_count() const { return _M_ht.max_bucket_count(); }
  size_type elems_in_bucket(size_type __n) const
  { return _M_ht.elems_in_bucket(__n); }
};

//Specific iterator traits creation
_STLP_CREATE_HASH_ITERATOR_TRAITS(HashMultisetTraitsT, Const_traits)

template <class _Value, _STLP_DFL_TMPL_PARAM(_HashFcn,hash<_Value>),
          _STLP_DFL_TMPL_PARAM(_EqualKey, equal_to<_Value>),
          _STLP_DFL_TMPL_PARAM(_Alloc, allocator<_Value>) >
class hash_multiset
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
                    : public __stlport_class<hash_multiset<_Value, _HashFcn, _EqualKey, _Alloc> >
#endif
{
  typedef hash_multiset<_Value, _HashFcn, _EqualKey, _Alloc> _Self;
  //Specific iterator traits creation
  typedef _STLP_PRIV _HashMultisetTraitsT<_Value> _HashMultisetTraits;
public:
  typedef hashtable<_Value, _Value, _HashFcn,
                    _HashMultisetTraits, _STLP_PRIV _Identity<_Value>, _EqualKey, _Alloc> _Ht;

  typedef typename _Ht::key_type key_type;
  typedef typename _Ht::value_type value_type;
  typedef typename _Ht::hasher hasher;
  typedef typename _Ht::key_equal key_equal;

  typedef typename _Ht::size_type size_type;
  typedef typename _Ht::difference_type difference_type;
  typedef typename _Ht::pointer       pointer;
  typedef typename _Ht::const_pointer const_pointer;
  typedef typename _Ht::reference reference;
  typedef typename _Ht::const_reference const_reference;

  typedef typename _Ht::iterator iterator;
  typedef typename _Ht::const_iterator const_iterator;

  typedef typename _Ht::allocator_type allocator_type;

  hasher hash_funct() const { return _M_ht.hash_funct(); }
  key_equal key_eq() const { return _M_ht.key_eq(); }
  allocator_type get_allocator() const { return _M_ht.get_allocator(); }

private:
  _Ht _M_ht;
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

public:
  hash_multiset()
    : _M_ht(0, hasher(), key_equal(), allocator_type()) {}
  explicit hash_multiset(size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type()) {}
  hash_multiset(size_type __n, const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type()) {}
  hash_multiset(size_type __n, const hasher& __hf, const key_equal& __eql)
    : _M_ht(__n, __hf, __eql, allocator_type()) {}
  hash_multiset(size_type __n, const hasher& __hf, const key_equal& __eql,
                const allocator_type& __a)
    : _M_ht(__n, __hf, __eql, __a) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  hash_multiset(__move_source<_Self> src)
    : _M_ht(__move_source<_Ht>(src.get()._M_ht)) {}
#endif

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l)
    : _M_ht(0, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n,
                const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }

  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n,
                const hasher& __hf, const key_equal& __eql,
                const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_equal(__f, __l); }
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n,
                const hasher& __hf, const key_equal& __eql)
    : _M_ht(__n, __hf, __eql, allocator_type())
    { _M_ht.insert_equal(__f, __l); }
#  endif
#else
  hash_multiset(const value_type* __f, const value_type* __l)
    : _M_ht(0, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  hash_multiset(const value_type* __f, const value_type* __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  hash_multiset(const value_type* __f, const value_type* __l, size_type __n,
                const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  hash_multiset(const value_type* __f, const value_type* __l, size_type __n,
                const hasher& __hf, const key_equal& __eql,
                const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_equal(__f, __l); }

  hash_multiset(const_iterator __f, const_iterator __l)
    : _M_ht(0, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  hash_multiset(const_iterator __f, const_iterator __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  hash_multiset(const_iterator __f, const_iterator __l, size_type __n,
                const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  hash_multiset(const_iterator __f, const_iterator __l, size_type __n,
                const hasher& __hf, const key_equal& __eql,
                const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_equal(__f, __l); }
#endif /*_STLP_MEMBER_TEMPLATES */

public:
  size_type size() const { return _M_ht.size(); }
  size_type max_size() const { return _M_ht.max_size(); }
  bool empty() const { return _M_ht.empty(); }
  void swap(_Self& hs) { _M_ht.swap(hs._M_ht); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  iterator begin() { return _M_ht.begin(); }
  iterator end() { return _M_ht.end(); }
  const_iterator begin() const { return _M_ht.begin(); }
  const_iterator end() const { return _M_ht.end(); }

public:
  iterator insert(const value_type& __obj) { return _M_ht.insert_equal(__obj); }
#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(_InputIterator __f, _InputIterator __l)
  { _M_ht.insert_equal(__f,__l); }
#else
  void insert(const value_type* __f, const value_type* __l)
  { _M_ht.insert_equal(__f,__l); }
  void insert(const_iterator __f, const_iterator __l)
  { _M_ht.insert_equal(__f, __l); }
#endif /*_STLP_MEMBER_TEMPLATES */
  iterator insert_noresize(const value_type& __obj)
  { return _M_ht.insert_equal_noresize(__obj); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __key) { return _M_ht.find(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __key) const { return _M_ht.find(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __key) const { return _M_ht.count(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator, iterator> equal_range(const _KT& __key)
  { return _M_ht.equal_range(__key); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __key) const
  { return _M_ht.equal_range(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type erase(const _KT& __key) {return _M_ht.erase(__key); }
  void erase(iterator __it) { _M_ht.erase(__it); }
  void erase(iterator __f, iterator __l) { _M_ht.erase(__f, __l); }
  void clear() { _M_ht.clear(); }

public:
  void resize(size_type __hint) { _M_ht.resize(__hint); }
  size_type bucket_count() const { return _M_ht.bucket_count(); }
  size_type max_bucket_count() const { return _M_ht.max_bucket_count(); }
  size_type elems_in_bucket(size_type __n) const
  { return _M_ht.elems_in_bucket(__n); }
};

#define _STLP_TEMPLATE_HEADER template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
#define _STLP_TEMPLATE_CONTAINER hash_set<_Value,_HashFcn,_EqualKey,_Alloc>

#include <stl/_relops_hash_cont.h>

#undef _STLP_TEMPLATE_CONTAINER
#define _STLP_TEMPLATE_CONTAINER hash_multiset<_Value,_HashFcn,_EqualKey,_Alloc>
#include <stl/_relops_hash_cont.h>

#undef _STLP_TEMPLATE_CONTAINER
#undef _STLP_TEMPLATE_HEADER

// Specialization of insert_iterator so that it will work for hash_set
// and hash_multiset.

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
#  if !defined (_STLP_NO_MOVE_SEMANTIC)
template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
struct __move_traits<hash_set<_Value, _HashFcn, _EqualKey, _Alloc> > :
  _STLP_PRIV __move_traits_aux<typename hash_set<_Value, _HashFcn, _EqualKey, _Alloc>::_Ht>
{};

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
struct __move_traits<hash_multiset<_Value, _HashFcn, _EqualKey, _Alloc> > :
  _STLP_PRIV __move_traits_aux<typename hash_multiset<_Value, _HashFcn, _EqualKey, _Alloc>::_Ht>
{};
#  endif

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
class insert_iterator<hash_set<_Value, _HashFcn, _EqualKey, _Alloc> > {
protected:
  typedef hash_set<_Value, _HashFcn, _EqualKey, _Alloc> _Container;
  _Container* container;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  insert_iterator(_Container& __x) : container(&__x) {}
  insert_iterator(_Container& __x, typename _Container::iterator)
    : container(&__x) {}
  insert_iterator<_Container>&
  operator=(const typename _Container::value_type& __val) {
    container->insert(__val);
    return *this;
  }
  insert_iterator<_Container>& operator*() { return *this; }
  insert_iterator<_Container>& operator++() { return *this; }
  insert_iterator<_Container>& operator++(int) { return *this; }
};

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
class insert_iterator<hash_multiset<_Value, _HashFcn, _EqualKey, _Alloc> > {
protected:
  typedef hash_multiset<_Value, _HashFcn, _EqualKey, _Alloc> _Container;
  _Container* container;
  typename _Container::iterator iter;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  insert_iterator(_Container& __x) : container(&__x) {}
  insert_iterator(_Container& __x, typename _Container::iterator)
    : container(&__x) {}
  insert_iterator<_Container>&
  operator=(const typename _Container::value_type& __val) {
    container->insert(__val);
    return *this;
  }
  insert_iterator<_Container>& operator*() { return *this; }
  insert_iterator<_Container>& operator++() { return *this; }
  insert_iterator<_Container>& operator++(int) { return *this; }
};
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_HASH_SET_H */

// Local Variables:
// mode:C++
// End:
