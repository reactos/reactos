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

#ifndef _STLP_MOVE_CONSTRUCT_FWK_H
#define _STLP_MOVE_CONSTRUCT_FWK_H

#ifndef _STLP_TYPE_TRAITS_H
#  include <stl/type_traits.h>
#endif

_STLP_BEGIN_NAMESPACE

/*************************************************************
 * Move constructor framework
 *************************************************************/

/*************************************************************
 *Partial move:
 *The source HAS to be a valid instance after the move!
 *************************************************************/
template <class _Tp>
class __move_source {
public:
  explicit __move_source (_Tp &_src) : _M_data(_src)
  {}

  _Tp& get() const
  { return _M_data; }
private:
  _Tp &_M_data;

  //We explicitely forbid assignment to avoid warning:
  typedef __move_source<_Tp> _Self;
  _Self& operator = (_Self const&);
};

//Class used to signal move constructor support, implementation and type.
template <class _Tp>
struct __move_traits {
  /*
   * implemented tells if a the special move constructor has to be called or the classic
   * copy constructor is just fine. Most of the time the copy constructor is fine only
   * if the following info is true.
   */
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && \
   !defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && \
   !defined (_STLP_NO_MOVE_SEMANTIC)
  typedef typename _IsSTLportClass<_Tp>::_Ret implemented;
#else
  typedef __false_type implemented;
#endif
  /*
   * complete tells if the move is complete or partial, that is to say, does the source
   * needs to be destroyed once it has been moved.
   */
#  if defined (__BORLANDC__) && (__BORLANDC__ >= 0x564)
  typedef __type_traits<_Tp>::has_trivial_destructor _TpMoveComplete;
  typedef typename __bool2type<__type2bool<_TpMoveComplete>::_Ret>::_Ret complete;
#  else
  typedef typename __type_traits<_Tp>::has_trivial_destructor complete;
#  endif
};

_STLP_MOVE_TO_PRIV_NAMESPACE

/*
 * This struct should never be used if the user has not explicitely stipulated
 * that its class support the full move concept. To check that the return type
 * in such a case will be __invalid_source<_Tp> to generate a compile error
 * revealing the configuration problem.
 */
template <class _Tp>
struct _MoveSourceTraits {
  typedef typename __move_traits<_Tp>::implemented _MvImpRet;
#if defined (__BORLANDC__)
  typedef typename __selectT<_MvImpRet,
#else
  enum {_MvImp = __type2bool<_MvImpRet>::_Ret};
  typedef typename __select<_MvImp,
#endif
                            __move_source<_Tp>,
                            _Tp const&>::_Ret _Type;
};

//The helper function
template <class _Tp>
inline _STLP_TYPENAME_ON_RETURN_TYPE _MoveSourceTraits<_Tp>::_Type
_AsMoveSource (_Tp &src) {
  typedef typename _MoveSourceTraits<_Tp>::_Type _SrcType;
  return _SrcType(src);
}

//Helper structs used for many class.
template <class _Tp>
struct __move_traits_aux {
  typedef typename __move_traits<_Tp>::implemented implemented;
  typedef typename __move_traits<_Tp>::complete complete;
};

template <class _Tp1, class _Tp2>
struct __move_traits_aux2 {
  typedef __move_traits<_Tp1> _MoveTraits1;
  typedef __move_traits<_Tp2> _MoveTraits2;

  typedef typename _Lor2<typename _MoveTraits1::implemented,
                         typename _MoveTraits2::implemented>::_Ret implemented;
  typedef typename _Land2<typename _MoveTraits1::complete,
                          typename _MoveTraits2::complete>::_Ret complete;
};

/*
 * Most of the time a class implement a move constructor but its use depends
 * on a third party, this is what the following struct are for.
 */
template <class _Tp>
struct __move_traits_help {
  typedef __true_type implemented;
  typedef typename __move_traits<_Tp>::complete complete;
};

template <class _Tp1, class _Tp2>
struct __move_traits_help1 {
  typedef __move_traits<_Tp1> _MoveTraits1;
  typedef __move_traits<_Tp2> _MoveTraits2;

  typedef typename _Lor2<typename _MoveTraits1::implemented,
                         typename _MoveTraits2::implemented>::_Ret implemented;
  typedef typename _Land2<typename _MoveTraits1::complete,
                          typename _MoveTraits2::complete>::_Ret complete;
};

template <class _Tp1, class _Tp2>
struct __move_traits_help2 {
  typedef __move_traits<_Tp1> _MoveTraits1;
  typedef __move_traits<_Tp2> _MoveTraits2;

  typedef __true_type implemented;
  typedef typename _Land2<typename _MoveTraits1::complete,
                          typename _MoveTraits2::complete>::_Ret complete;
};

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /* _STLP_MOVE_CONSTRUCT_FWK_H */
