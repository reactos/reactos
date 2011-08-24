/*
 *
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

#ifndef _STLP_BOOST_TYPE_TRAITS_H
#define _STLP_BOOST_TYPE_TRAITS_H

#ifndef BOOST_CONFIG_SUFFIX_HPP
#  ifdef BOOST_CONFIG_HPP
#    undef BOOST_CONFIG_HPP
#  endif
#  include <boost/config.hpp>
#endif

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_float.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/type_traits/has_trivial_copy.hpp>
#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_same.hpp>

/*
 * This file mostly wraps boost type_traits in the STLport type_traits.
 * When checking a type traits like trivial assign operator for instance
 * both the boost value and STLport values has to be taken into account
 * as we don't know what the user might have prefer, specializing the boost
 * type traits or the STLport one.
 */
_STLP_BEGIN_NAMESPACE

template <class _Tp> struct _IsRef {
  enum { _Is = ::boost::is_reference<_Tp>::value };
  typedef typename __bool2type<_Is>::_Ret _Ret;
};

template <class _Tp> struct _IsPtr {
  enum { is_pointer = ::boost::is_pointer<_Tp>::value };
  typedef typename __bool2type<is_pointer>::_Ret _Ret;
};

template <class _Tp> struct _IsIntegral {
  enum { is_integral = ::boost::is_integral<_Tp>::value };
  typedef typename __bool2type<is_integral>::_Ret _Ret;
};

template <class _Tp> struct _IsRational {
  enum { is_float = ::boost::is_float<_Tp>::value };
  typedef typename __bool2type<is_float>::_Ret _Ret;
};

template <class _Tp>
struct __type_traits {
  enum { trivial_constructor = ::boost::has_trivial_constructor<_Tp>::value };
  typedef typename __bool2type<trivial_constructor>::_Ret has_trivial_default_constructor;

  enum { trivial_copy = ::boost::has_trivial_copy<_Tp>::value };
  typedef typename __bool2type<trivial_copy>::_Ret has_trivial_copy_constructor;

  enum { trivial_assign = ::boost::has_trivial_assign<_Tp>::value };
  typedef typename __bool2type<trivial_assign>::_Ret has_trivial_assignment_operator;

  enum { trivial_destructor = ::boost::has_trivial_destructor<_Tp>::value };
  typedef typename __bool2type<trivial_destructor>::_Ret has_trivial_destructor;

  enum { pod = ::boost::is_pod<_Tp>::value };
  typedef typename __bool2type<pod>::_Ret is_POD_type;
};

template <class _Tp1, class _Tp2>
struct _TrivialCopy {
  typedef typename ::boost::remove_cv<_Tp1>::type uncv1;
  typedef typename ::boost::remove_cv<_Tp2>::type uncv2;

  enum { same = ::boost::is_same<uncv1, uncv2>::value };
  typedef typename __bool2type<same>::_Ret _Same;

  enum { boost_trivial_assign = ::boost::has_trivial_assign<uncv1>::value };
  typedef typename __bool2type<boost_trivial_assign>::_Ret _BoostTrivialAssign;
  typedef typename __type_traits<uncv1>::has_trivial_assignment_operator _STLPTrivialAssign;
  typedef typename _Lor2<_BoostTrivialAssign, _STLPTrivialAssign>::_Ret _TrivialAssign;

  typedef typename _Land2<_Same, _TrivialAssign>::_Ret _Type;
  static _Type _Answer() { return _Type(); }
};

template <class _Tp1, class _Tp2>
struct _TrivialUCopy {
  typedef typename ::boost::remove_cv<_Tp1>::type uncv1;
  typedef typename ::boost::remove_cv<_Tp2>::type uncv2;

  enum { same = ::boost::is_same<uncv1, uncv2>::value };
  typedef typename __bool2type<same>::_Ret _Same;

  enum { boost_trivial_copy = ::boost::has_trivial_copy<uncv1>::value };
  typedef typename __bool2type<boost_trivial_copy>::_Ret _BoostTrivialCopy;
  typedef typename __type_traits<uncv1>::has_trivial_copy_constructor _STLPTrivialCopy;
  typedef typename _Lor2<_BoostTrivialCopy, _STLPTrivialCopy>::_Ret _TrivialCopy;

  typedef typename _Land2<_Same, _TrivialCopy>::_Ret _Type;
  static _Type _Answer() { return _Type(); }
};

template <class _Tp>
struct _DefaultZeroValue {
  enum { is_integral = ::boost::is_integral<_Tp>::value };
  typedef typename __bool2type<is_integral>::_Ret _IsIntegral;
  enum { is_float = ::boost::is_float<_Tp>::value };
  typedef typename __bool2type<is_float>::_Ret _IsFloat;
  enum { is_pointer = ::boost::is_pointer<_Tp>::value };
  typedef typename __bool2type<is_pointer>::_Ret _IsPointer;

  typedef typename _Lor3<_IsIntegral, _IsFloat, _IsPointer>::_Ret _Ret;
};

template <class _Tp>
struct _TrivialInit {
  typedef typename ::boost::remove_cv<_Tp>::type uncv;

  enum { boost_trivial_constructor = ::boost::has_trivial_constructor<uncv>::value };
  typedef typename __bool2type<boost_trivial_constructor>::_Ret _BoostTrivialInit;
  typedef typename __type_traits<uncv>::has_trivial_default_constructor _STLPTrivialInit;
  typedef typename _Lor2<_BoostTrivialInit, _STLPTrivialInit>::_Ret _Tr1;

  typedef typename _DefaultZeroValue<_Tp>::_Ret _Tr2;
  typedef typename _Not<_Tr2>::_Ret _Tr3;

  typedef typename _Land2<_Tr1, _Tr3>::_Ret _Ret;
  static _Ret _Answer() { return _Ret(); }
};

_STLP_END_NAMESPACE

#endif /* _STLP_BOOST_TYPE_TRAITS_H */
