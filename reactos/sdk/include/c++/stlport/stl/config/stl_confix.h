/*
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

/*
 * STLport configuration file
 * It is internal STLport header - DO NOT include it directly
 * Purpose of this file : to define STLport settings that depend on
 * compiler flags or can be otherwise missed
 *
 */

#ifndef _STLP_CONFIX_H
#define _STLP_CONFIX_H

/* If, by any chance, C compiler gets there, try to help it to pass smoothly */
#if ! defined (__cplusplus) && ! defined (_STLP_HAS_NO_NAMESPACES)
#  define _STLP_HAS_NO_NAMESPACES
#endif

#if defined (__MINGW32__)
#  define _STLP_NO_DRAND48
#endif

/* Modena C++ library  */
#if defined (__MWERKS__) && __MWERKS__ <= 0x2303 || (defined (__KCC) && __KCC_VERSION < 3400)
#  include <mcompile.h>
#  define _STLP_USE_MSIPL 1
#  if defined (__KCC) || (defined(__MSL_CPP__) && \
       ( (__MSL_CPP__ >= 0x5000 && defined( _MSL_NO_MESSAGE_FACET )) || \
       (__MSL_CPP__ < 0x5000 && defined( MSIPL_NL_TYPES ))))
#    define _STLP_NO_NATIVE_MESSAGE_FACET 1
#  endif
#endif

/* common switches for EDG front-end */
/* __EDG_SWITCHES do not seem to be an official EDG macro.
 * We keep it for historical reason. */
#if defined (__EDG_SWITCHES)
#  if !(defined(_TYPENAME) || defined (_TYPENAME_IS_KEYWORD))
#    undef  _STLP_NEED_TYPENAME
#    define _STLP_NEED_TYPENAME 1
#  endif
#  ifndef _WCHAR_T_IS_KEYWORD
#    undef _STLP_NO_WCHAR_T
#    define _STLP_NO_WCHAR_T 1
#  endif
#  ifndef _PARTIAL_SPECIALIZATION_OF_CLASS_TEMPLATES
#    undef _STLP_NO_CLASS_PARTIAL_SPECIALIZATION
#    define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION 1
#  endif
#  ifndef _MEMBER_TEMPLATES
#    undef _STLP_NO_MEMBER_TEMPLATES
#    define _STLP_NO_MEMBER_TEMPLATES 1
#    undef _STLP_NO_MEMBER_TEMPLATE_CLASSES
#    define _STLP_NO_MEMBER_TEMPLATE_CLASSES 1
#  endif
#  ifndef _MEMBER_TEMPLATE_KEYWORD
#    undef  _STLP_NO_MEMBER_TEMPLATE_KEYWORD
#    define _STLP_NO_MEMBER_TEMPLATE_KEYWORD 1
#  endif
#  if !defined (__EXCEPTIONS) && ! defined (_EXCEPTIONS)
#    undef  _STLP_HAS_NO_EXCEPTIONS
#    define _STLP_HAS_NO_EXCEPTIONS
#  endif
#  undef __EDG_SWITCHES
#endif /* EDG */

/* __EDG_VERSION__ is an official EDG macro, compilers based
 * on EDG have to define it. */
#if defined (__EDG_VERSION__)
#  if (__EDG_VERSION__ >= 244) && !defined (_STLP_HAS_INCLUDE_NEXT)
#    define _STLP_HAS_INCLUDE_NEXT
#  endif
#  if (__EDG_VERSION__ <= 240) && !defined (_STLP_DONT_RETURN_VOID)
#    define _STLP_DONT_RETURN_VOID
#  endif
#  if !defined (__EXCEPTIONS) && !defined (_STLP_HAS_NO_EXCEPTIONS)
#    define _STLP_HAS_NO_EXCEPTIONS
#  endif
#  if !defined (__NO_LONG_LONG) && !defined (_STLP_LONG_LONG)
#    define _STLP_LONG_LONG long long
#  endif
#endif

#endif
