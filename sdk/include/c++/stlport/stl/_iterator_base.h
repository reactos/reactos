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

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#define _STLP_INTERNAL_ITERATOR_BASE_H

#ifndef _STLP_INTERNAL_CSTDDEF
#  include <stl/_cstddef.h>
#endif

//# if defined  (_STLP_IMPORT_VENDOR_CSTD) && ! defined (_STLP_VENDOR_GLOBAL_CSTD)
//_STLP_BEGIN_NAMESPACE
//using namespace _STLP_VENDOR_CSTD;
//_STLP_END_NAMESPACE
//#endif /* _STLP_IMPORT_VENDOR_CSTD */

#if !defined(_STLP_USE_OLD_HP_ITERATOR_QUERIES) && !defined(_STLP_CLASS_PARTIAL_SPECIALIZATION)
#  ifndef _STLP_TYPE_TRAITS_H
#    include <stl/type_traits.h>
#  endif
#endif

_STLP_BEGIN_NAMESPACE

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <class _Category, class _Tp, _STLP_DFL_TMPL_PARAM(_Distance,ptrdiff_t),
          _STLP_DFL_TMPL_PARAM(_Pointer,_Tp*), _STLP_DFL_TMPL_PARAM(_Reference,_Tp&) >
struct iterator {
  typedef _Category  iterator_category;
  typedef _Tp        value_type;
  typedef _Distance  difference_type;
  typedef _Pointer   pointer;
  typedef _Reference reference;
};
_STLP_TEMPLATE_NULL
struct iterator<output_iterator_tag, void, void, void, void> {
  typedef output_iterator_tag  iterator_category;
#ifdef _STLP_CLASS_PARTIAL_SPECIALIZATION
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;
#endif
};

#if defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
#  define _STLP_ITERATOR_CATEGORY(_It, _Tp) _STLP_STD::iterator_category(_It)
#  define _STLP_DISTANCE_TYPE(_It, _Tp)     _STLP_STD::distance_type(_It)
#  define _STLP_VALUE_TYPE(_It, _Tp)        _STLP_STD::value_type(_It)
//Old HP iterator queries do not give information about the iterator
//associated reference type so we consider that it is not a real reference.
#  define _STLP_IS_REF_TYPE_REAL_REF(_It, _Tp) __false_type()
#else
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
#    define _STLP_VALUE_TYPE(_It, _Tp)        (_STLP_TYPENAME _STLP_STD::iterator_traits< _Tp >::value_type*)0
#    define _STLP_DISTANCE_TYPE(_It, _Tp)     (_STLP_TYPENAME _STLP_STD::iterator_traits< _Tp >::difference_type*)0
#    if defined (__BORLANDC__) || defined (__SUNPRO_CC) || ( defined (__MWERKS__) && (__MWERKS__ <= 0x2303)) || \
       (defined (__sgi) && defined (_COMPILER_VERSION)) || defined (__DMC__)
#      define _STLP_ITERATOR_CATEGORY(_It, _Tp) _STLP_STD::iterator_traits< _Tp >::iterator_category()
#    else
#      define _STLP_ITERATOR_CATEGORY(_It, _Tp) _STLP_TYPENAME _STLP_STD::iterator_traits< _Tp >::iterator_category()
#    endif
#    define _STLP_IS_REF_TYPE_REAL_REF(_It, _Tp) _STLP_STD::_IsRefType< _STLP_TYPENAME _STLP_STD::iterator_traits< _Tp >::reference >::_Ret()
#  else
#    define _STLP_ITERATOR_CATEGORY(_It, _Tp)   _STLP_STD::__iterator_category(_It, _STLP_STD::_IsPtrType<_Tp>::_Ret())
#    define _STLP_DISTANCE_TYPE(_It, _Tp)       _STLP_STD::__distance_type(_It, _STLP_STD::_IsPtrType<_Tp>::_Ret())
#    define _STLP_VALUE_TYPE(_It, _Tp)          _STLP_STD::__value_type(_It, _STLP_STD::_IsPtrType<_Tp>::_Ret())
#    define _STLP_IS_REF_TYPE_REAL_REF(_It, _Tp) __false_type()
#  endif
#endif

#if defined (_STLP_DONT_REDEFINE_STD) && defined (_STLP_WHOLE_NATIVE_STD)
/* In this mode we will see both STLport implementation and native
 * one. To allow some interaction between both implementations through
 * iterators we have to map std iterator categories to stlport ones. This
 * way we will be able to initialize STLport containers with native
 * iterators, the other side won't work except when STLport iterators are
 * simple pointers. */

_STLP_END_NAMESPACE

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <iterator>
#  else
#    include _STLP_NATIVE_HEADER(iterator)
#  endif

_STLP_BEGIN_NAMESPACE

template <class _IteCat>
struct _CategoryMapping
{ typedef _IteCat _Tag; };

_STLP_TEMPLATE_NULL
struct _CategoryMapping<::std::input_iterator_tag>
{ typedef input_iterator_tag _Tag; };
_STLP_TEMPLATE_NULL
struct _CategoryMapping<::std::output_iterator_tag>
{ typedef output_iterator_tag _Tag; };
_STLP_TEMPLATE_NULL
struct _CategoryMapping<::std::forward_iterator_tag>
{ typedef forward_iterator_tag _Tag; };
_STLP_TEMPLATE_NULL
struct _CategoryMapping<::std::bidirectional_iterator_tag>
{ typedef bidirectional_iterator_tag _Tag; };
_STLP_TEMPLATE_NULL
struct _CategoryMapping<::std::random_access_iterator_tag>
{ typedef random_access_iterator_tag _Tag; };

template <class _Iterator>
struct iterator_traits {
  typedef typename _Iterator::iterator_category _OriginalTag;
  typedef typename _CategoryMapping<_OriginalTag>::_Tag iterator_category;
#else
template <class _Iterator>
struct iterator_traits {
  typedef typename _Iterator::iterator_category iterator_category;
#endif
  typedef typename _Iterator::value_type        value_type;
  typedef typename _Iterator::difference_type   difference_type;
  typedef typename _Iterator::pointer           pointer;
  typedef typename _Iterator::reference         reference;
};

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (__SUNPRO_CC)
#  define _STLP_DIFFERENCE_TYPE(_Iterator) typename iterator_traits<_Iterator>::difference_type
#else
#  define _STLP_DIFFERENCE_TYPE(_Iterator) ptrdiff_t
#endif

#ifdef _STLP_CLASS_PARTIAL_SPECIALIZATION

// fbp : this order keeps gcc happy
template <class _Tp>
struct iterator_traits<const _Tp*> {
  typedef random_access_iterator_tag  iterator_category;
  typedef _Tp                         value_type;
  typedef ptrdiff_t                   difference_type;
  typedef const _Tp*                  pointer;
  typedef const _Tp&                  reference;
};

template <class _Tp>
struct iterator_traits<_Tp*> {
  typedef random_access_iterator_tag  iterator_category;
  typedef _Tp                         value_type;
  typedef ptrdiff_t                   difference_type;
  typedef _Tp*                        pointer;
  typedef _Tp&                        reference;
};

#  if defined (__BORLANDC__)
template <class _Tp>
struct iterator_traits<_Tp* const> {
  typedef random_access_iterator_tag  iterator_category;
  typedef _Tp                         value_type;
  typedef ptrdiff_t                   difference_type;
  typedef const _Tp*                  pointer;
  typedef const _Tp&                  reference;
};
#  endif

#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

_STLP_END_NAMESPACE
#include <stl/_ptrs_specialize.h>
_STLP_BEGIN_NAMESPACE

#ifndef _STLP_USE_OLD_HP_ITERATOR_QUERIES
// The overloaded functions iterator_category, distance_type, and
// value_type are not part of the C++ standard.  (They have been
// replaced by struct iterator_traits.)  They are included for
// backward compatibility with the HP STL.
// We introduce internal names for these functions.

#  ifndef _STLP_CLASS_PARTIAL_SPECIALIZATION

template <class _Tp>
inline _STLP_STD::random_access_iterator_tag
__iterator_category(const _Tp*, const __true_type&)
{ return _STLP_STD::random_access_iterator_tag(); }

template <class _Iter>
inline _STLP_TYPENAME_ON_RETURN_TYPE _STLP_STD::iterator_traits<_Iter>::iterator_category
__iterator_category(const _Iter&, const __false_type&) {
  typedef _STLP_TYPENAME _STLP_STD::iterator_traits<_Iter>::iterator_category _Category;
  return _Category();
}

template <class _Tp>
inline ptrdiff_t*
__distance_type(const _Tp*, const __true_type&)
{ return __STATIC_CAST(ptrdiff_t*, 0); }

template <class _Iter>
inline _STLP_TYPENAME_ON_RETURN_TYPE _STLP_STD::iterator_traits<_Iter>::difference_type*
__distance_type(const _Iter&, const __false_type&) {
  typedef _STLP_TYPENAME _STLP_STD::iterator_traits<_Iter>::difference_type _diff_type;
  return __STATIC_CAST(_diff_type*,0);
}

template <class _Tp>
inline _Tp*
__value_type(const _Tp*, const __true_type&)
{ return __STATIC_CAST(_Tp*, 0); }

template <class _Iter>
inline _STLP_TYPENAME_ON_RETURN_TYPE _STLP_STD::iterator_traits<_Iter>::value_type*
__value_type(const _Iter&, const __false_type&) {
  typedef _STLP_TYPENAME _STLP_STD::iterator_traits<_Iter>::value_type _value_type;
  return __STATIC_CAST(_value_type*,0);
}

#  endif

#else /* _STLP_USE_OLD_HP_ITERATOR_QUERIES */
template <class _Category, class _Tp, class _Distance, class _Pointer, class _Reference>
inline _Category _STLP_CALL iterator_category(const iterator<_Category,_Tp,_Distance,_Pointer,_Reference>&) { return _Category(); }
template <class _Category, class _Tp, class _Distance, class _Pointer, class _Reference>
inline _Tp* _STLP_CALL value_type(const iterator<_Category,_Tp,_Distance,_Pointer,_Reference>&) { return __STATIC_CAST(_Tp*, 0); }
template <class _Category, class _Tp, class _Distance, class _Pointer, class _Reference>
inline _Distance* _STLP_CALL distance_type(const iterator<_Category,_Tp,_Distance,_Pointer,_Reference>&) { return __STATIC_CAST(_Distance*, 0); }
template <class _Tp>
inline random_access_iterator_tag _STLP_CALL iterator_category(const _Tp*) { return random_access_iterator_tag(); }
template <class _Tp>
inline _Tp* _STLP_CALL value_type(const _Tp*) { return __STATIC_CAST(_Tp*, 0); }
template <class _Tp>
inline ptrdiff_t* _STLP_CALL distance_type(const _Tp*) { return __STATIC_CAST(ptrdiff_t*, 0); }
#endif /* _STLP_USE_OLD_HP_ITERATOR_QUERIES */

#if !defined (_STLP_NO_ANACHRONISMS)
// The base classes input_iterator, output_iterator, forward_iterator,
// bidirectional_iterator, and random_access_iterator are not part of
// the C++ standard.  (They have been replaced by struct iterator.)
// They are included for backward compatibility with the HP STL.
template <class _Tp, class _Distance> struct input_iterator :
  public iterator <input_iterator_tag, _Tp, _Distance, _Tp*, _Tp&> {};
struct output_iterator : public iterator <output_iterator_tag, void, void, void, void> {};
template <class _Tp, class _Distance> struct forward_iterator :
  public iterator<forward_iterator_tag, _Tp, _Distance, _Tp*, _Tp&> {};
template <class _Tp, class _Distance> struct bidirectional_iterator :
  public iterator<bidirectional_iterator_tag, _Tp, _Distance, _Tp*, _Tp&> {};
template <class _Tp, class _Distance> struct random_access_iterator :
  public iterator<random_access_iterator_tag, _Tp, _Distance, _Tp*, _Tp&> {};

#  if defined (_STLP_BASE_MATCH_BUG) && defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
template <class _Tp, class _Distance>
inline input_iterator_tag _STLP_CALL
iterator_category(const input_iterator<_Tp, _Distance>&) { return input_iterator_tag(); }
inline output_iterator_tag _STLP_CALL
iterator_category(const output_iterator&) { return output_iterator_tag(); }
template <class _Tp, class _Distance>
inline forward_iterator_tag _STLP_CALL
iterator_category(const forward_iterator<_Tp, _Distance>&) { return forward_iterator_tag(); }
template <class _Tp, class _Distance>
inline bidirectional_iterator_tag _STLP_CALL
iterator_category(const bidirectional_iterator<_Tp, _Distance>&) { return bidirectional_iterator_tag(); }
template <class _Tp, class _Distance>
inline random_access_iterator_tag _STLP_CALL
iterator_category(const random_access_iterator<_Tp, _Distance>&) { return random_access_iterator_tag(); }
template <class _Tp, class _Distance>
inline _Tp*  _STLP_CALL value_type(const input_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Tp*, 0); }
template <class _Tp, class _Distance>
inline _Tp* _STLP_CALL value_type(const forward_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Tp*, 0); }
template <class _Tp, class _Distance>
inline _Tp* _STLP_CALL value_type(const bidirectional_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Tp*, 0); }
template <class _Tp, class _Distance>
inline _Tp* _STLP_CALL value_type(const random_access_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Tp*, 0); }
template <class _Tp, class _Distance>
inline _Distance* _STLP_CALL distance_type(const input_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Distance*, 0); }
template <class _Tp, class _Distance>
inline _Distance* _STLP_CALL distance_type(const forward_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Distance*, 0); }
template <class _Tp, class _Distance>
inline _Distance* _STLP_CALL distance_type(const bidirectional_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Distance*, 0);}
template <class _Tp, class _Distance>
inline _Distance* _STLP_CALL distance_type(const random_access_iterator<_Tp, _Distance>&) { return __STATIC_CAST(_Distance*, 0); }
#  endif
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIterator>
inline _STLP_DIFFERENCE_TYPE(_InputIterator) _STLP_CALL
__distance(const _InputIterator& __first, const _InputIterator& __last,
           const input_iterator_tag &) {
  _STLP_DIFFERENCE_TYPE(_InputIterator) __n = 0;
  _InputIterator __it(__first);
  while (__it != __last) {
    ++__it; ++__n;
  }
  return __n;
}

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _ForwardIterator>
inline _STLP_DIFFERENCE_TYPE(_ForwardIterator) _STLP_CALL
__distance(const _ForwardIterator& __first, const _ForwardIterator& __last,
           const forward_iterator_tag &) {
  _STLP_DIFFERENCE_TYPE(_ForwardIterator) __n = 0;
  _ForwardIterator __it(__first);
  while (__it != __last) {
    ++__it; ++__n;
  }
  return __n;
}

template <class _BidirectionalIterator>
_STLP_INLINE_LOOP _STLP_DIFFERENCE_TYPE(_BidirectionalIterator) _STLP_CALL
__distance(const _BidirectionalIterator& __first, const _BidirectionalIterator& __last,
           const bidirectional_iterator_tag &) {
  _STLP_DIFFERENCE_TYPE(_BidirectionalIterator) __n = 0;
  _BidirectionalIterator __it(__first);
  while (__it != __last) {
    ++__it; ++__n;
  }
  return __n;
}
#endif

template <class _RandomAccessIterator>
inline _STLP_DIFFERENCE_TYPE(_RandomAccessIterator) _STLP_CALL
__distance(const _RandomAccessIterator& __first, const _RandomAccessIterator& __last,
           const random_access_iterator_tag &)
{ return __last - __first; }

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIterator>
inline _STLP_DIFFERENCE_TYPE(_InputIterator) _STLP_CALL
distance(_InputIterator __first, _InputIterator __last)
{ return _STLP_PRIV __distance(__first, __last, _STLP_ITERATOR_CATEGORY(__first, _InputIterator)); }

#if !defined (_STLP_NO_ANACHRONISMS)
template <class _InputIterator, class _Distance>
inline void _STLP_CALL distance(const _InputIterator& __first,
                                const _InputIterator& __last, _Distance& __n)
{ __n += _STLP_STD::distance(__first, __last); }

#  if defined (_STLP_MSVC)
// MSVC specific
template <class _InputIterator, class _Dist>
inline void  _STLP_CALL _Distance(_InputIterator __first,
                                  _InputIterator __last, _Dist& __n)
{ __n += _STLP_STD::distance(__first, __last); }
#  endif
#endif

// fbp: those are being used for iterator/const_iterator definitions everywhere
template <class _Tp>
struct _Nonconst_traits;

template <class _Tp>
struct _Const_traits {
  typedef _Tp value_type;
  typedef const _Tp&  reference;
  typedef const _Tp*  pointer;
  typedef _Const_traits<_Tp> _ConstTraits;
  typedef _Nonconst_traits<_Tp> _NonConstTraits;
};

template <class _Tp>
struct _Nonconst_traits {
  typedef _Tp value_type;
  typedef _Tp& reference;
  typedef _Tp* pointer;
  typedef _Const_traits<_Tp> _ConstTraits;
  typedef _Nonconst_traits<_Tp> _NonConstTraits;
};

/*
 * dums: A special iterator/const_iterator traits for set and multiset for which even
 * the iterator is not mutable
 */
template <class _Tp>
struct _Nonconst_Const_traits;

template <class _Tp>
struct _Const_Const_traits {
  typedef _Tp value_type;
  typedef const _Tp&  reference;
  typedef const _Tp*  pointer;
  typedef _Const_Const_traits<_Tp> _ConstTraits;
  typedef _Nonconst_Const_traits<_Tp> _NonConstTraits;
};

template <class _Tp>
struct _Nonconst_Const_traits {
  typedef _Tp value_type;
  typedef const _Tp& reference;
  typedef const _Tp* pointer;
  typedef _Const_Const_traits<_Tp> _ConstTraits;
  typedef _Nonconst_Const_traits<_Tp> _NonConstTraits;
};

/*
 * A macro to generate a new iterator traits from one of the
 * previous one. Changing the iterator traits type make iterators
 * from different containers not comparable.
 */
#define _STLP_CREATE_ITERATOR_TRAITS_BASE(Motif, Traits)        \
template <class _Tp>                                            \
struct _##Motif;                                                \
template <class _Tp>                                            \
struct _Const##Motif : public _STLP_STD::_Const_##Traits<_Tp> {  \
  typedef _Const##Motif<_Tp> _ConstTraits;                      \
  typedef _##Motif<_Tp> _NonConstTraits;                        \
};                                                              \
template <class _Tp>                                            \
struct _##Motif : public _STLP_STD::_Nonconst_##Traits<_Tp> {    \
  typedef _Const##Motif<_Tp> _ConstTraits;                      \
  typedef _##Motif<_Tp> _NonConstTraits;                        \
};

#define _STLP_CREATE_ITERATOR_TRAITS(Motif, Traits)             \
_STLP_MOVE_TO_PRIV_NAMESPACE                                    \
_STLP_CREATE_ITERATOR_TRAITS_BASE(Motif, Traits)                \
_STLP_MOVE_TO_STD_NAMESPACE

#define _STLP_CREATE_HASH_ITERATOR_TRAITS(Motif, Traits)        \
_STLP_MOVE_TO_PRIV_NAMESPACE                                    \
_STLP_CREATE_ITERATOR_TRAITS_BASE(NonLocal##Motif, Traits)      \
_STLP_CREATE_ITERATOR_TRAITS_BASE(Local##Motif, Traits)         \
template <class _Tp>                                            \
struct _##Motif {                                               \
  typedef _ConstNonLocal##Motif<_Tp> _ConstTraits;              \
  typedef _NonLocal##Motif<_Tp> _NonConstTraits;                \
  typedef _ConstLocal##Motif<_Tp> _ConstLocalTraits;            \
  typedef _Local##Motif<_Tp> _NonConstLocalTraits;              \
};                                                              \
_STLP_MOVE_TO_STD_NAMESPACE

/*
#  if defined (_STLP_BASE_TYPEDEF_BUG)
// this workaround is needed for SunPro 4.0.1
template <class _Traits>
struct __cnst_traits_aux : private _Traits {
  typedef typename _Traits::value_type value_type;
};
#  define __TRAITS_VALUE_TYPE(_Traits) __cnst_traits_aux<_Traits>::value_type
#  else
#  define __TRAITS_VALUE_TYPE(_Traits) _Traits::value_type
#  endif
*/

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIter, class _Distance>
_STLP_INLINE_LOOP void _STLP_CALL
__advance(_InputIter& __i, _Distance __n, const input_iterator_tag &)
{ while (__n--) ++__i; }

// fbp : added output iterator tag variant
template <class _InputIter, class _Distance>
_STLP_INLINE_LOOP void _STLP_CALL
__advance(_InputIter& __i, _Distance __n, const output_iterator_tag &)
{ while (__n--) ++__i; }

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _ForwardIterator, class _Distance>
_STLP_INLINE_LOOP void _STLP_CALL
__advance(_ForwardIterator& i, _Distance n, const forward_iterator_tag &)
{ while (n--) ++i; }
#endif

template <class _BidirectionalIterator, class _Distance>
_STLP_INLINE_LOOP void _STLP_CALL
__advance(_BidirectionalIterator& __i, _Distance __n,
          const bidirectional_iterator_tag &) {
  if (__n > 0)
    while (__n--) ++__i;
  else
    while (__n++) --__i;
}

template <class _RandomAccessIterator, class _Distance>
inline void _STLP_CALL
__advance(_RandomAccessIterator& __i, _Distance __n,
          const random_access_iterator_tag &)
{ __i += __n; }

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIterator, class _Distance>
inline void _STLP_CALL advance(_InputIterator& __i, _Distance __n)
{ _STLP_PRIV __advance(__i, __n, _STLP_ITERATOR_CATEGORY(__i, _InputIterator)); }

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_ITERATOR_BASE_H */


// Local Variables:
// mode:C++
// End:
