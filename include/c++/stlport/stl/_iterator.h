/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996-1998
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

#ifndef _STLP_INTERNAL_ITERATOR_H
#define _STLP_INTERNAL_ITERATOR_H

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#  include <stl/_iterator_base.h>
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
// This is the new version of reverse_iterator, as defined in the
//  draft C++ standard.  It relies on the iterator_traits template,
//  which in turn relies on partial specialization.  The class
//  reverse_bidirectional_iterator is no longer part of the draft
//  standard, but it is retained for backward compatibility.

template <class _Iterator>
class reverse_iterator :
  public iterator<typename iterator_traits<_Iterator>::iterator_category,
                  typename iterator_traits<_Iterator>::value_type,
                  typename iterator_traits<_Iterator>::difference_type,
                  typename iterator_traits<_Iterator>::pointer,
                  typename iterator_traits<_Iterator>::reference> {
protected:
  _Iterator current;
  typedef reverse_iterator<_Iterator> _Self;
public:
  typedef typename iterator_traits<_Iterator>::difference_type difference_type;
  // pointer type required for arrow operator hidden behind _STLP_DEFINE_ARROW_OPERATOR:
  typedef typename iterator_traits<_Iterator>::pointer pointer;
  typedef typename iterator_traits<_Iterator>::reference reference;
  typedef _Iterator iterator_type;
public:
  reverse_iterator() {}
  explicit reverse_iterator(iterator_type __x) : current(__x) {}
  reverse_iterator(const _Self& __x) : current(__x.current) {}
  _Self& operator = (const _Self& __x) { current = __x.base(); return *this; }
#  if defined (_STLP_MEMBER_TEMPLATES)
  template <class _Iter>
  reverse_iterator(const reverse_iterator<_Iter>& __x) : current(__x.base()) {}
  template <class _Iter>
  _Self& operator = (const reverse_iterator<_Iter>& __x) { current = __x.base(); return *this; }
#  endif /* _STLP_MEMBER_TEMPLATES */

  iterator_type base() const { return current; }
  reference operator*() const {
    _Iterator __tmp = current;
    return *--__tmp;
  }
  _STLP_DEFINE_ARROW_OPERATOR
  _Self& operator++() {
    --current;
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    --current;
    return __tmp;
  }
  _Self& operator--() {
    ++current;
    return *this;
  }
  _Self operator--(int) {
    _Self __tmp = *this;
    ++current;
    return __tmp;
  }

  _Self operator+(difference_type __n) const { return _Self(current - __n); }
  _Self& operator+=(difference_type __n) {
    current -= __n;
    return *this;
  }
  _Self operator-(difference_type __n) const { return _Self(current + __n); }
  _Self& operator-=(difference_type __n) {
    current += __n;
    return *this;
  }
  reference operator[](difference_type __n) const { return *(*this + __n); }
};

template <class _Iterator>
inline bool  _STLP_CALL operator==(const reverse_iterator<_Iterator>& __x,
                                   const reverse_iterator<_Iterator>& __y)
{ return __x.base() == __y.base(); }

template <class _Iterator>
inline bool _STLP_CALL operator<(const reverse_iterator<_Iterator>& __x,
                                 const reverse_iterator<_Iterator>& __y)
{ return __y.base() < __x.base(); }

#  if defined (_STLP_USE_SEPARATE_RELOPS_NAMESPACE)
template <class _Iterator>
inline bool _STLP_CALL operator!=(const reverse_iterator<_Iterator>& __x,
                                  const reverse_iterator<_Iterator>& __y)
{ return !(__x == __y); }

template <class _Iterator>
inline bool _STLP_CALL operator>(const reverse_iterator<_Iterator>& __x,
                                 const reverse_iterator<_Iterator>& __y)
{ return __y < __x; }

template <class _Iterator>
inline bool _STLP_CALL operator<=(const reverse_iterator<_Iterator>& __x,
                                  const reverse_iterator<_Iterator>& __y)
{ return !(__y < __x); }

template <class _Iterator>
inline bool _STLP_CALL operator>=(const reverse_iterator<_Iterator>& __x,
                                  const reverse_iterator<_Iterator>& __y)
{ return !(__x < __y); }
#  endif /* _STLP_USE_SEPARATE_RELOPS_NAMESPACE */

template <class _Iterator>
#  if defined (__SUNPRO_CC)
inline ptrdiff_t _STLP_CALL
#  else
inline typename reverse_iterator<_Iterator>::difference_type _STLP_CALL
#  endif
operator-(const reverse_iterator<_Iterator>& __x,
          const reverse_iterator<_Iterator>& __y)
{ return __y.base() - __x.base(); }

template <class _Iterator, class _DifferenceType>
inline reverse_iterator<_Iterator>  _STLP_CALL
operator+(_DifferenceType n,const reverse_iterator<_Iterator>& x)
{ return x.operator+(n); }
#endif

template <class _Container>
class back_insert_iterator
  : public iterator<output_iterator_tag, void, void, void, void> {
  typedef back_insert_iterator<_Container> _Self;
protected:
  //c is a Standard name (24.4.2.1), do no make it STLport naming convention compliant.
  _Container *container;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;

  explicit back_insert_iterator(_Container& __x) : container(&__x) {}

  _Self& operator=(const _Self& __other) {
    container = __other.container;
    return *this;
  }
  _Self& operator=(const typename _Container::value_type& __val) {
    container->push_back(__val);
    return *this;
  }
  _Self& operator*() { return *this; }
  _Self& operator++() { return *this; }
  _Self  operator++(int) { return *this; }
};

template <class _Container>
inline back_insert_iterator<_Container>  _STLP_CALL back_inserter(_Container& __x)
{ return back_insert_iterator<_Container>(__x); }

template <class _Container>
class front_insert_iterator
  : public iterator<output_iterator_tag, void, void, void, void> {
  typedef front_insert_iterator<_Container> _Self;
protected:
  //c is a Standard name (24.4.2.3), do no make it STLport naming convention compliant.
  _Container *container;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  explicit front_insert_iterator(_Container& __x) : container(&__x) {}

  _Self& operator=(const _Self& __other) {
    container = __other.container;
    return *this;
  }
  _Self& operator=(const typename _Container::value_type& __val) {
    container->push_front(__val);
    return *this;
  }
  _Self& operator*() { return *this; }
  _Self& operator++() { return *this; }
  _Self  operator++(int) { return *this; }
};

template <class _Container>
inline front_insert_iterator<_Container>  _STLP_CALL front_inserter(_Container& __x)
{ return front_insert_iterator<_Container>(__x); }

template <class _Container>
class insert_iterator
  : public iterator<output_iterator_tag, void, void, void, void> {
  typedef insert_iterator<_Container> _Self;
protected:
  //container is a Standard name (24.4.2.5), do no make it STLport naming convention compliant.
  _Container *container;
  typename _Container::iterator _M_iter;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  insert_iterator(_Container& __x, typename _Container::iterator __i)
    : container(&__x), _M_iter(__i) {}

  _Self& operator=(_Self const& __other) {
    container = __other.container;
    _M_iter = __other._M_iter;
    return *this;
  }
  _Self& operator=(const typename _Container::value_type& __val) {
    _M_iter = container->insert(_M_iter, __val);
    ++_M_iter;
    return *this;
  }
  _Self& operator*() { return *this; }
  _Self& operator++() { return *this; }
  _Self& operator++(int) { return *this; }
};

template <class _Container, class _Iterator>
inline insert_iterator<_Container>  _STLP_CALL
inserter(_Container& __x, _Iterator __i) {
  typedef typename _Container::iterator __iter;
  return insert_iterator<_Container>(__x, __iter(__i));
}

_STLP_END_NAMESPACE

#if ! defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) || defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
#  include <stl/_iterator_old.h>
#endif

#endif /* _STLP_INTERNAL_ITERATOR_H */

// Local Variables:
// mode:C++
// End:
