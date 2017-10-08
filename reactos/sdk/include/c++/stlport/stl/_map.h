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

#ifndef _STLP_INTERNAL_MAP_H
#define _STLP_INTERNAL_MAP_H

#ifndef _STLP_INTERNAL_TREE_H
#  include <stl/_tree.h>
#endif

_STLP_BEGIN_NAMESPACE

//Specific iterator traits creation
_STLP_CREATE_ITERATOR_TRAITS(MapTraitsT, traits)

template <class _Key, class _Tp, _STLP_DFL_TMPL_PARAM(_Compare, less<_Key> ),
          _STLP_DEFAULT_PAIR_ALLOCATOR_SELECT(_STLP_CONST _Key, _Tp) >
class map
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
          : public __stlport_class<map<_Key, _Tp, _Compare, _Alloc> >
#endif
{
  typedef map<_Key, _Tp, _Compare, _Alloc> _Self;
public:

// typedefs:

  typedef _Key                  key_type;
  typedef _Tp                   data_type;
  typedef _Tp                   mapped_type;
  typedef pair<_STLP_CONST _Key, _Tp> value_type;
  typedef _Compare              key_compare;

  class value_compare
    : public binary_function<value_type, value_type, bool> {
  friend class map<_Key,_Tp,_Compare,_Alloc>;
  protected :
    //c is a Standard name (23.3.1), do no make it STLport naming convention compliant.
    _Compare comp;
    value_compare(_Compare __c) : comp(__c) {}
  public:
    bool operator()(const value_type& __x, const value_type& __y) const
    { return comp(__x.first, __y.first); }
  };

protected:
  typedef _STLP_PRIV _MapTraitsT<value_type> _MapTraits;

public:
  //Following typedef have to be public for __move_traits specialization.
  typedef _STLP_PRIV _Rb_tree<key_type, key_compare,
                              value_type, _STLP_SELECT1ST(value_type, _Key),
                              _MapTraits, _Alloc> _Rep_type;

  typedef typename _Rep_type::pointer pointer;
  typedef typename _Rep_type::const_pointer const_pointer;
  typedef typename _Rep_type::reference reference;
  typedef typename _Rep_type::const_reference const_reference;
  typedef typename _Rep_type::iterator iterator;
  typedef typename _Rep_type::const_iterator const_iterator;
  typedef typename _Rep_type::reverse_iterator reverse_iterator;
  typedef typename _Rep_type::const_reverse_iterator const_reverse_iterator;
  typedef typename _Rep_type::size_type size_type;
  typedef typename _Rep_type::difference_type difference_type;
  typedef typename _Rep_type::allocator_type allocator_type;

private:
  _Rep_type _M_t;  // red-black tree representing map
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

public:
  // allocation/deallocation
  map() : _M_t(_Compare(), allocator_type()) {}
#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  explicit map(const _Compare& __comp,
               const allocator_type& __a = allocator_type())
#else
  explicit map(const _Compare& __comp)
    : _M_t(__comp, allocator_type()) {}
  explicit map(const _Compare& __comp, const allocator_type& __a)
#endif
    : _M_t(__comp, __a) {}

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  map(_InputIterator __first, _InputIterator __last)
    : _M_t(_Compare(), allocator_type())
    { _M_t.insert_unique(__first, __last); }

  template <class _InputIterator>
  map(_InputIterator __first, _InputIterator __last, const _Compare& __comp,
      const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _M_t(__comp, __a) { _M_t.insert_unique(__first, __last); }

#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator>
  map(_InputIterator __first, _InputIterator __last, const _Compare& __comp)
    : _M_t(__comp, allocator_type()) { _M_t.insert_unique(__first, __last); }
#  endif

#else
  map(const value_type* __first, const value_type* __last)
    : _M_t(_Compare(), allocator_type())
    { _M_t.insert_unique(__first, __last); }

  map(const value_type* __first,
      const value_type* __last, const _Compare& __comp,
      const allocator_type& __a = allocator_type())
    : _M_t(__comp, __a) { _M_t.insert_unique(__first, __last); }

  map(const_iterator __first, const_iterator __last)
    : _M_t(_Compare(), allocator_type())
    { _M_t.insert_unique(__first, __last); }

  map(const_iterator __first, const_iterator __last, const _Compare& __comp,
      const allocator_type& __a = allocator_type())
    : _M_t(__comp, __a) { _M_t.insert_unique(__first, __last); }
#endif /* _STLP_MEMBER_TEMPLATES */

  map(const _Self& __x) : _M_t(__x._M_t) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  map(__move_source<_Self> src)
    : _M_t(__move_source<_Rep_type>(src.get()._M_t)) {}
#endif

  _Self& operator=(const _Self& __x) {
    _M_t = __x._M_t;
    return *this;
  }

  // accessors:
  key_compare key_comp() const { return _M_t.key_comp(); }
  value_compare value_comp() const { return value_compare(_M_t.key_comp()); }
  allocator_type get_allocator() const { return _M_t.get_allocator(); }

  iterator begin() { return _M_t.begin(); }
  const_iterator begin() const { return _M_t.begin(); }
  iterator end() { return _M_t.end(); }
  const_iterator end() const { return _M_t.end(); }
  reverse_iterator rbegin() { return _M_t.rbegin(); }
  const_reverse_iterator rbegin() const { return _M_t.rbegin(); }
  reverse_iterator rend() { return _M_t.rend(); }
  const_reverse_iterator rend() const { return _M_t.rend(); }
  bool empty() const { return _M_t.empty(); }
  size_type size() const { return _M_t.size(); }
  size_type max_size() const { return _M_t.max_size(); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  _Tp& operator[](const _KT& __k) {
    iterator __i = lower_bound(__k);
    // __i->first is greater than or equivalent to __k.
    if (__i == end() || key_comp()(__k, (*__i).first))
      __i = insert(__i, value_type(__k, _STLP_DEFAULT_CONSTRUCTED(_Tp)));
    return (*__i).second;
  }
  void swap(_Self& __x) { _M_t.swap(__x._M_t); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  // insert/erase
  pair<iterator,bool> insert(const value_type& __x)
  { return _M_t.insert_unique(__x); }
  iterator insert(iterator __pos, const value_type& __x)
  { return _M_t.insert_unique(__pos, __x); }
#ifdef _STLP_MEMBER_TEMPLATES
  template <class _InputIterator>
  void insert(_InputIterator __first, _InputIterator __last)
  { _M_t.insert_unique(__first, __last); }
#else
  void insert(const value_type* __first, const value_type* __last)
  { _M_t.insert_unique(__first, __last); }
  void insert(const_iterator __first, const_iterator __last)
  { _M_t.insert_unique(__first, __last); }
#endif /* _STLP_MEMBER_TEMPLATES */

  void erase(iterator __pos) { _M_t.erase(__pos); }
  size_type erase(const key_type& __x) { return _M_t.erase_unique(__x); }
  void erase(iterator __first, iterator __last) { _M_t.erase(__first, __last); }
  void clear() { _M_t.clear(); }

  // map operations:
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __x) { return _M_t.find(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __x) const { return _M_t.find(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __x) const { return _M_t.find(__x) == _M_t.end() ? 0 : 1; }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator lower_bound(const _KT& __x) { return _M_t.lower_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator lower_bound(const _KT& __x) const { return _M_t.lower_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator upper_bound(const _KT& __x) { return _M_t.upper_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator upper_bound(const _KT& __x) const { return _M_t.upper_bound(__x); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator,iterator> equal_range(const _KT& __x)
  { return _M_t.equal_range_unique(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator,const_iterator> equal_range(const _KT& __x) const
  { return _M_t.equal_range_unique(__x); }
};

//Specific iterator traits creation
_STLP_CREATE_ITERATOR_TRAITS(MultimapTraitsT, traits)

template <class _Key, class _Tp, _STLP_DFL_TMPL_PARAM(_Compare, less<_Key> ),
          _STLP_DEFAULT_PAIR_ALLOCATOR_SELECT(_STLP_CONST _Key, _Tp) >
class multimap
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
               : public __stlport_class<multimap<_Key, _Tp, _Compare, _Alloc> >
#endif
{
  typedef multimap<_Key, _Tp, _Compare, _Alloc> _Self;
public:

// typedefs:

  typedef _Key                  key_type;
  typedef _Tp                   data_type;
  typedef _Tp                   mapped_type;
  typedef pair<_STLP_CONST _Key, _Tp> value_type;
  typedef _Compare              key_compare;

  class value_compare : public binary_function<value_type, value_type, bool> {
    friend class multimap<_Key,_Tp,_Compare,_Alloc>;
  protected:
    //comp is a Standard name (23.3.2), do no make it STLport naming convention compliant.
    _Compare comp;
    value_compare(_Compare __c) : comp(__c) {}
  public:
    bool operator()(const value_type& __x, const value_type& __y) const
    { return comp(__x.first, __y.first); }
  };

protected:
  //Specific iterator traits creation
  typedef _STLP_PRIV _MultimapTraitsT<value_type> _MultimapTraits;

public:
  //Following typedef have to be public for __move_traits specialization.
  typedef _STLP_PRIV _Rb_tree<key_type, key_compare,
                              value_type, _STLP_SELECT1ST(value_type, _Key),
                              _MultimapTraits, _Alloc> _Rep_type;

  typedef typename _Rep_type::pointer pointer;
  typedef typename _Rep_type::const_pointer const_pointer;
  typedef typename _Rep_type::reference reference;
  typedef typename _Rep_type::const_reference const_reference;
  typedef typename _Rep_type::iterator iterator;
  typedef typename _Rep_type::const_iterator const_iterator;
  typedef typename _Rep_type::reverse_iterator reverse_iterator;
  typedef typename _Rep_type::const_reverse_iterator const_reverse_iterator;
  typedef typename _Rep_type::size_type size_type;
  typedef typename _Rep_type::difference_type difference_type;
  typedef typename _Rep_type::allocator_type allocator_type;

private:
  _Rep_type _M_t;  // red-black tree representing multimap
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

public:
  // allocation/deallocation
  multimap() : _M_t(_Compare(), allocator_type()) { }
  explicit multimap(const _Compare& __comp,
                    const allocator_type& __a = allocator_type())
    : _M_t(__comp, __a) { }

#ifdef _STLP_MEMBER_TEMPLATES
  template <class _InputIterator>
  multimap(_InputIterator __first, _InputIterator __last)
    : _M_t(_Compare(), allocator_type())
    { _M_t.insert_equal(__first, __last); }
# ifdef _STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS
  template <class _InputIterator>
  multimap(_InputIterator __first, _InputIterator __last,
           const _Compare& __comp)
    : _M_t(__comp, allocator_type()) { _M_t.insert_equal(__first, __last); }
#  endif
  template <class _InputIterator>
  multimap(_InputIterator __first, _InputIterator __last,
           const _Compare& __comp,
           const allocator_type& __a _STLP_ALLOCATOR_TYPE_DFL)
    : _M_t(__comp, __a) { _M_t.insert_equal(__first, __last); }
#else
  multimap(const value_type* __first, const value_type* __last)
    : _M_t(_Compare(), allocator_type())
    { _M_t.insert_equal(__first, __last); }
  multimap(const value_type* __first, const value_type* __last,
           const _Compare& __comp,
           const allocator_type& __a = allocator_type())
    : _M_t(__comp, __a) { _M_t.insert_equal(__first, __last); }

  multimap(const_iterator __first, const_iterator __last)
    : _M_t(_Compare(), allocator_type())
    { _M_t.insert_equal(__first, __last); }
  multimap(const_iterator __first, const_iterator __last,
           const _Compare& __comp,
           const allocator_type& __a = allocator_type())
    : _M_t(__comp, __a) { _M_t.insert_equal(__first, __last); }
#endif /* _STLP_MEMBER_TEMPLATES */

  multimap(const _Self& __x) : _M_t(__x._M_t) {}

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  multimap(__move_source<_Self> src)
    : _M_t(__move_source<_Rep_type>(src.get()._M_t)) {}
#endif

  _Self& operator=(const _Self& __x) {
    _M_t = __x._M_t;
    return *this;
  }

  // accessors:

  key_compare key_comp() const { return _M_t.key_comp(); }
  value_compare value_comp() const { return value_compare(_M_t.key_comp()); }
  allocator_type get_allocator() const { return _M_t.get_allocator(); }

  iterator begin() { return _M_t.begin(); }
  const_iterator begin() const { return _M_t.begin(); }
  iterator end() { return _M_t.end(); }
  const_iterator end() const { return _M_t.end(); }
  reverse_iterator rbegin() { return _M_t.rbegin(); }
  const_reverse_iterator rbegin() const { return _M_t.rbegin(); }
  reverse_iterator rend() { return _M_t.rend(); }
  const_reverse_iterator rend() const { return _M_t.rend(); }
  bool empty() const { return _M_t.empty(); }
  size_type size() const { return _M_t.size(); }
  size_type max_size() const { return _M_t.max_size(); }
  void swap(_Self& __x) { _M_t.swap(__x._M_t); }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(_Self& __x) { swap(__x); }
#endif

  // insert/erase
  iterator insert(const value_type& __x) { return _M_t.insert_equal(__x); }
  iterator insert(iterator __pos, const value_type& __x) { return _M_t.insert_equal(__pos, __x); }
#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert(_InputIterator __first, _InputIterator __last)
  { _M_t.insert_equal(__first, __last); }
#else
  void insert(const value_type* __first, const value_type* __last)
  { _M_t.insert_equal(__first, __last); }
  void insert(const_iterator __first, const_iterator __last)
  { _M_t.insert_equal(__first, __last); }
#endif /* _STLP_MEMBER_TEMPLATES */
  void erase(iterator __pos) { _M_t.erase(__pos); }
  size_type erase(const key_type& __x) { return _M_t.erase(__x); }
  void erase(iterator __first, iterator __last) { _M_t.erase(__first, __last); }
  void clear() { _M_t.clear(); }

  // multimap operations:

  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __x) { return _M_t.find(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __x) const { return _M_t.find(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __x) const { return _M_t.count(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator lower_bound(const _KT& __x) { return _M_t.lower_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator lower_bound(const _KT& __x) const { return _M_t.lower_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator upper_bound(const _KT& __x) { return _M_t.upper_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator upper_bound(const _KT& __x) const { return _M_t.upper_bound(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator,iterator> equal_range(const _KT& __x)
  { return _M_t.equal_range(__x); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator,const_iterator> equal_range(const _KT& __x) const
  { return _M_t.equal_range(__x); }
};

#define _STLP_TEMPLATE_HEADER template <class _Key, class _Tp, class _Compare, class _Alloc>
#define _STLP_TEMPLATE_CONTAINER map<_Key,_Tp,_Compare,_Alloc>
#include <stl/_relops_cont.h>
#undef  _STLP_TEMPLATE_CONTAINER
#define _STLP_TEMPLATE_CONTAINER multimap<_Key,_Tp,_Compare,_Alloc>
#include <stl/_relops_cont.h>
#undef  _STLP_TEMPLATE_CONTAINER
#undef  _STLP_TEMPLATE_HEADER

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_NO_MOVE_SEMANTIC)
template <class _Key, class _Tp, class _Compare, class _Alloc>
struct __move_traits<map<_Key,_Tp,_Compare,_Alloc> > :
  _STLP_PRIV __move_traits_aux<typename map<_Key,_Tp,_Compare,_Alloc>::_Rep_type>
{};

template <class _Key, class _Tp, class _Compare, class _Alloc>
struct __move_traits<multimap<_Key,_Tp,_Compare,_Alloc> > :
  _STLP_PRIV __move_traits_aux<typename multimap<_Key,_Tp,_Compare,_Alloc>::_Rep_type>
{};
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_MAP_H */

// Local Variables:
// mode:C++
// End:

