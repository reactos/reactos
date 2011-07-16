/*
 *
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


#ifndef _STLP_TYPE_MANIPS_H
#define _STLP_TYPE_MANIPS_H

_STLP_BEGIN_NAMESPACE

struct __true_type {};
struct __false_type {};

#if defined (_STLP_USE_NAMESPACES) && !defined (_STLP_DONT_USE_PRIV_NAMESPACE)
_STLP_MOVE_TO_PRIV_NAMESPACE
using _STLP_STD::__true_type;
using _STLP_STD::__false_type;
_STLP_MOVE_TO_STD_NAMESPACE
#endif

//bool to type
template <int _Is>
struct __bool2type
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL
struct __bool2type<1> { typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL
struct __bool2type<0> { typedef __false_type _Ret; };

//type to bool
template <class __bool_type>
struct __type2bool { enum {_Ret = 1}; };

_STLP_TEMPLATE_NULL
struct __type2bool<__true_type> { enum {_Ret = 1}; };

_STLP_TEMPLATE_NULL
struct __type2bool<__false_type> { enum {_Ret = 0}; };

//Negation
template <class _BoolType>
struct _Not { typedef __false_type _Ret; };

_STLP_TEMPLATE_NULL
struct _Not<__false_type> { typedef __true_type _Ret; };

// logical and of 2 predicated
template <class _P1, class _P2>
struct _Land2 { typedef __false_type _Ret; };

_STLP_TEMPLATE_NULL
struct _Land2<__true_type, __true_type> { typedef __true_type _Ret; };

// logical and of 3 predicated
template <class _P1, class _P2, class _P3>
struct _Land3 { typedef __false_type _Ret; };

_STLP_TEMPLATE_NULL
struct _Land3<__true_type, __true_type, __true_type> { typedef __true_type _Ret; };

//logical or of 2 predicated
template <class _P1, class _P2>
struct _Lor2 { typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL
struct _Lor2<__false_type, __false_type> { typedef __false_type _Ret; };

// logical or of 3 predicated
template <class _P1, class _P2, class _P3>
struct _Lor3 { typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL
struct _Lor3<__false_type, __false_type, __false_type> { typedef __false_type _Ret; };

////////////////////////////////////////////////////////////////////////////////
// class template __select
// Selects one of two types based upon a boolean constant
// Invocation: __select<_Cond, T, U>::Result
// where:
// flag is a compile-time boolean constant
// T and U are types
// Result evaluates to T if flag is true, and to U otherwise.
////////////////////////////////////////////////////////////////////////////////
// BEWARE: If the compiler do not support partial template specialization or nested template
//classes the default behavior of the __select is to consider the condition as false and so return
//the second template type!!

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
#  if defined (__BORLANDC__) 
template <class _CondT, class _Tp1, class _Tp2>
struct __selectT { typedef _Tp1 _Ret; };

template <class _Tp1, class _Tp2>
struct __selectT<__false_type, _Tp1, _Tp2> { typedef _Tp2 _Ret; };
#  endif

#  if !defined (__BORLANDC__) || (__BORLANDC__ >= 0x590)
template <bool _Cond, class _Tp1, class _Tp2>
struct __select { typedef _Tp1 _Ret; };

template <class _Tp1, class _Tp2>
struct __select<false, _Tp1, _Tp2> { typedef _Tp2 _Ret; };
#  else
template <bool _Cond, class _Tp1, class _Tp2>
struct __select 
{ typedef __selectT<typename __bool2type<_Cond>::_Ret, _Tp1, _Tp2>::_Ret _Ret; };
#  endif

#else

#  if defined (_STLP_MEMBER_TEMPLATE_CLASSES)
template <int _Cond>
struct __select_aux {
  template <class _Tp1, class _Tp2>
  struct _In {
    typedef _Tp1 _Ret;
  };
};

_STLP_TEMPLATE_NULL
struct __select_aux<0> {
  template <class _Tp1, class _Tp2>
  struct _In {
    typedef _Tp2 _Ret;
  };
};

template <int _Cond, class _Tp1, class _Tp2>
struct __select {
  typedef typename __select_aux<_Cond>::_STLP_TEMPLATE _In<_Tp1, _Tp2>::_Ret _Ret;
};
#  else /* _STLP_MEMBER_TEMPLATE_CLASSES */
//default behavior
template <int _Cond, class _Tp1, class _Tp2>
struct __select {
  typedef _Tp2 _Ret;
};
#  endif /* _STLP_MEMBER_TEMPLATE_CLASSES */

#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

/* Rather than introducing a new macro for the following constrution we use
 * an existing one (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) that
 * is used for a similar feature.
 */
#if !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) && \
    (!defined (__GNUC__) || (__GNUC__ > 2))
// Helper struct that will forbid volatile qualified types:
#  if !defined (__BORLANDC__)
struct _NoVolatilePointerShim { _NoVolatilePointerShim(const void*); };
template <class _Tp>
char _STLP_CALL _IsCopyableFun(bool, _NoVolatilePointerShim, _Tp const*, _Tp*); // no implementation is required
char* _STLP_CALL _IsCopyableFun(bool, ...);       // no implementation is required

template <class _Src, class _Dst>
struct _Copyable {
  static _Src* __null_src();
  static _Dst* __null_dst();
  enum { _Ret = (sizeof(_IsCopyableFun(false, __null_src(), __null_src(), __null_dst())) == sizeof(char)) };
  typedef typename __bool2type<_Ret>::_Ret _RetT;
};
#  else
template <class _Tp1, class _Tp2> struct _AreSameTypes;
template <class _Tp> struct _IsUnQual;
template <class _Src, class _Dst>
struct _Copyable {
  typedef typename _AreSameTypes<_Src, _Dst>::_Ret _Tr1;
  typedef typename _IsUnQual<_Dst>::_Ret _Tr2;
  typedef typename _Land2<_Tr1, _Tr2>::_Ret _RetT;
  enum { _Ret = __type2bool<_RetT>::_Ret };
};
#  endif
#else
template <class _Src, class _Dst>
struct _Copyable {
  enum { _Ret = 0 };
  typedef __false_type _RetT;
};
#endif

/*
 * The following struct will tell you if 2 types are the same and if copying memory
 * from the _Src type to the _Dst type is right considering qualifiers. If _Src and
 * _Dst types are the same unqualified types _Ret will be false if:
 *  - any of the type has the volatile qualifier
 *  - _Dst is const qualified
 */
template <class _Src, class _Dst>
struct _AreCopyable {
  enum { _Same = _Copyable<_Src, _Dst>::_Ret };
  typedef typename _Copyable<_Src, _Dst>::_RetT _Ret;
};

template <class _Tp1, class _Tp2>
struct _AreSameTypes {
  enum { _Same = 0 };
  typedef __false_type _Ret;
};

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _Tp>
struct _AreSameTypes<_Tp, _Tp> {
  enum { _Same = 1 };
  typedef __true_type _Ret;
};
#endif

#if !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS)
template <class _Src, class _Dst>
struct _ConversionHelper {
  static char _Test(bool, _Dst);
  static char* _Test(bool, ...);
  static _Src _MakeSource();
};

template <class _Src, class _Dst>
struct _IsConvertible {
  typedef _ConversionHelper<_Src*, const volatile _Dst*> _H;
  enum { value = (sizeof(char) == sizeof(_H::_Test(false, _H::_MakeSource()))) };
  typedef typename __bool2type<value>::_Ret _Ret;
};

#  if defined (__BORLANDC__)
#    if (__BORLANDC__ < 0x590)
template<class _Tp>
struct _UnConstPtr { typedef _Tp _Type; };

template<class _Tp>
struct _UnConstPtr<_Tp*> { typedef _Tp _Type; };

template<class _Tp>
struct _UnConstPtr<const _Tp*> { typedef _Tp _Type; };
#    endif

#    if !defined (_STLP_QUALIFIED_SPECIALIZATION_BUG)
template <class _Tp>
struct _IsConst { typedef __false_type _Ret; };
#    else
template <class _Tp>
struct _IsConst { typedef _AreSameTypes<_Tp, const _Tp>::_Ret _Ret; };
#    endif

#    if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_QUALIFIED_SPECIALIZATION_BUG)
template <class _Tp>
struct _IsConst <const _Tp> { typedef __true_type _Ret; };
#    endif

#    if (__BORLANDC__ < 0x590)
template<class _Tp>
struct _IsConst<_Tp*> { typedef _AreSameTypes<_Tp*, const _Tp*>::_Ret _Ret; };
#    endif
template <class _Tp>
struct _IsVolatile { typedef _AreSameTypes<_Tp, volatile _Tp>::_Ret _Ret; };

template<class _Tp>
struct _IsUnQual {
  typedef _IsConst<_Tp>::_Ret _Tr1;
  typedef _IsVolatile<_Tp>::_Ret _Tr2;
  typedef _Not<_Tr1>::_Ret _NotCon;
  typedef _Not<_Tr2>::_Ret _NotVol;
  typedef _Land2<_NotCon, _NotVol>::_Ret _Ret;
};

#    if !defined (_STLP_QUALIFIED_SPECIALIZATION_BUG)
template <class _Tp> struct _UnQual { typedef _Tp _Type; };
template <class _Tp> struct _UnQual<const _Tp> { typedef _Tp _Type; };
template <class _Tp> struct _UnQual<volatile _Tp> { typedef _Tp _Type; };
template <class _Tp> struct _UnQual<const volatile _Tp> { typedef _Tp _Type; };
#    endif
#  endif

/* This struct is intended to say if a pointer can be convertible to an other
 * taking into account cv qualifications. It shouldn't be instanciated with
 * something else than pointer type as it uses pass by value parameter that
 * results in compilation error when parameter type has a special memory
 * alignment
 */
template <class _Src, class _Dst>
struct _IsCVConvertible {
#  if !defined (__BORLANDC__) || (__BORLANDC__ >= 0x590)
  typedef _ConversionHelper<_Src, _Dst> _H;
  enum { value = (sizeof(char) == sizeof(_H::_Test(false, _H::_MakeSource()))) };
#  else
  enum { _Is1 = __type2bool<_IsConst<_Src>::_Ret>::_Ret };
  enum { _Is2 = _IsConvertible<_UnConstPtr<_Src>::_Type, _UnConstPtr<_Dst>::_Type>::value };
  enum { value = _Is1 ? 0 : _Is2 };
#  endif
  typedef typename __bool2type<value>::_Ret _Ret;
};

#else
template <class _Src, class _Dst>
struct _IsConvertible {
  enum { value = 0 };
  typedef __false_type _Ret;
};

template <class _Src, class _Dst>
struct _IsCVConvertible {
  enum { value = 0 };
  typedef __false_type _Ret;
};
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_TYPE_MANIPS_H */
