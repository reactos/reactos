#ifndef _STLP_PTRS_SPECIALIZE_H
#define _STLP_PTRS_SPECIALIZE_H

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) || \
   (defined (_STLP_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) && !defined (_STLP_NO_ARROW_OPERATOR))
#  define _STLP_POINTERS_SPECIALIZE( _TpP )
#  define _STLP_DEFINE_ARROW_OPERATOR  pointer operator->() const { return &(operator*()); }
#else
#  ifndef _STLP_TYPE_TRAITS_H
#    include <stl/type_traits.h>
#  endif

// the following is a workaround for arrow operator problems
#  if defined  ( _STLP_NO_ARROW_OPERATOR )
// User wants to disable proxy -> operators
#    define _STLP_DEFINE_ARROW_OPERATOR
#  else
// Compiler can handle generic -> operator.
#    if defined (__BORLANDC__)
#      define _STLP_DEFINE_ARROW_OPERATOR  pointer operator->() const { return &(*(*this)); }
#    elif defined(__WATCOMC__)
#      define _STLP_DEFINE_ARROW_OPERATOR pointer operator->() const { reference x = operator*(); return &x; }
#    else
#      define _STLP_DEFINE_ARROW_OPERATOR  pointer operator->() const { return &(operator*()); }
#    endif
#  endif /* _STLP_NO_ARROW_OPERATOR */

// Important pointers specializations

#  ifdef _STLP_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS
#    define _STLP_TYPE_TRAITS_POD_SPECIALIZE_V(_Type)
#    define _STLP_TYPE_TRAITS_POD_SPECIALIZE(_Type)
#  else
#    define _STLP_TYPE_TRAITS_POD_SPECIALIZE(_Type) _STLP_TEMPLATE_NULL struct __type_traits<_Type> : __type_traits_aux<__true_type> {};
#    define _STLP_TYPE_TRAITS_POD_SPECIALIZE_V(_Type) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(_Type*) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(const _Type*) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(_Type**) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(_Type* const *) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(const _Type**) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(_Type***) \
_STLP_TYPE_TRAITS_POD_SPECIALIZE(const _Type***)
#  endif

#  define _STLP_POINTERS_SPECIALIZE(_Type) _STLP_TYPE_TRAITS_POD_SPECIALIZE_V(_Type)

_STLP_BEGIN_NAMESPACE

#  if !defined ( _STLP_NO_BOOL )
_STLP_POINTERS_SPECIALIZE( bool )
#  endif
_STLP_TYPE_TRAITS_POD_SPECIALIZE_V(void)
#  ifndef _STLP_NO_SIGNED_BUILTINS
  _STLP_POINTERS_SPECIALIZE( signed char )
#  endif
  _STLP_POINTERS_SPECIALIZE( char )
  _STLP_POINTERS_SPECIALIZE( unsigned char )
  _STLP_POINTERS_SPECIALIZE( short )
  _STLP_POINTERS_SPECIALIZE( unsigned short )
  _STLP_POINTERS_SPECIALIZE( int )
  _STLP_POINTERS_SPECIALIZE( unsigned int )
  _STLP_POINTERS_SPECIALIZE( long )
  _STLP_POINTERS_SPECIALIZE( unsigned long )
  _STLP_POINTERS_SPECIALIZE( float )
  _STLP_POINTERS_SPECIALIZE( double )
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  _STLP_POINTERS_SPECIALIZE( long double )
#  endif
#  if defined ( _STLP_LONG_LONG)
  _STLP_POINTERS_SPECIALIZE( _STLP_LONG_LONG )
  _STLP_POINTERS_SPECIALIZE( unsigned _STLP_LONG_LONG )
#  endif
#  if defined ( _STLP_HAS_WCHAR_T ) && ! defined (_STLP_WCHAR_T_IS_USHORT)
  _STLP_POINTERS_SPECIALIZE( wchar_t )
#  endif

_STLP_END_NAMESPACE

#  undef _STLP_ARROW_SPECIALIZE
#  undef _STLP_TYPE_TRAITS_POD_SPECIALIZE_V

#endif
#endif
