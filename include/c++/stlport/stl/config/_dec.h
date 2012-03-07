#define _STLP_COMPILER "Dec"

# define _STLP_HAS_SPECIFIC_PROLOG_EPILOG

# define _STLP_NATIVE_HEADER(header) <../cxx/##header>
# define _STLP_NATIVE_C_HEADER(x) <../include/##x>

#if (__DECCXX_VER < 60300000)
# define _STLP_NATIVE_CPP_C_HEADER(header) <../cxx/##header>
#else
# define _STLP_NATIVE_CPP_C_HEADER(header) </usr/include/cxx_cname/##header>
#endif

# define _STLP_NATIVE_OLD_STREAMS_HEADER(header) <../cxx/##header>
# define _STLP_NATIVE_CPP_RUNTIME_HEADER(header) <../cxx/##header>

/* Alpha is little-endian */
# define _STLP_LITTLE_ENDIAN

// collisions
# define _STLP_DONT_PUT_STLPORT_IN_STD

#if (__DECCXX_VER < 60000000)

/*
 automatic template instantiation does not
 work with namespaces ;(
*/
# define _STLP_HAS_NO_NAMESPACES 1

# define _STLP_NO_NEW_NEW_HEADER 1

# define _STLP_NO_WCHAR_T  1
# define _STLP_NEED_EXPLICIT  1

# define _STLP_NO_BOOL  1
# define _STLP_NEED_TYPENAME 1
# define _STLP_NO_NEW_STYLE_CASTS 1
# define _STLP_NEED_MUTABLE 1
# define _STLP_NO_BAD_ALLOC 1


# define _STLP_NO_PARTIAL_SPECIALIZATION_SYNTAX 1

# define _STLP_NO_MEMBER_TEMPLATES 1
# define _STLP_NO_MEMBER_TEMPLATE_CLASSES 1
# define _STLP_NO_MEMBER_TEMPLATE_KEYWORD 1
# define _STLP_NO_QUALIFIED_FRIENDS 1
# define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION 1
# define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER 1
# define _STLP_NON_TYPE_TMPL_PARAM_BUG 1
# define _STLP_BROKEN_USING_DIRECTIVE 1
# define _STLP_NO_EXCEPTION_HEADER 1
# define _STLP_DEF_CONST_PLCT_NEW_BUG 1
# define _STLP_DEF_CONST_DEF_PARAM_BUG 1

#endif


#ifndef __NO_USE_STD_IOSTREAM
/* default is to use new iostreams, anyway */
# ifndef __USE_STD_IOSTREAM
#  define __USE_STD_IOSTREAM
# endif
#endif

/*
# ifndef __STD_STRICT_ANSI_ERRORS
# endif
*/

#ifndef __EXCEPTIONS
# define _STLP_HAS_NO_EXCEPTIONS 1
#endif

# ifdef __IMPLICIT_INCLUDE_ENABLED

/* but, works with ours ;). */
#  define _STLP_LINK_TIME_INSTANTIATION 1
# else
#  undef _STLP_LINK_TIME_INSTANTIATION
# endif

# if defined (__IMPLICIT_USING_STD) && !defined (__NO_USE_STD_IOSTREAM)
/*
 we should ban that !
 #  error "STLport won't work with new iostreams and std:: being implicitly included. Please use -std strict_ansi[_errors] or specify __NO_USE_STD_IOSTREAM"
*/
# endif

# if (defined (__STD_STRICT_ANSI) || defined (__STD_STRICT_ANSI_ERRORS))
#  define _STLP_STRICT_ANSI 1
# else
/* we want to enforce it */
#  define _STLP_LONG_LONG long long
# endif

/* unsigned 32-bit integer type */
#  define _STLP_UINT32_T unsigned int
#if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
# define _STLP_RAND48 1
#endif
/* #  define _STLP_RAND48 1 */

#  define _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS 1

# if (__DECCXX_VER <= 60600000)
#  define _STLP_HAS_NO_NEW_C_HEADERS 1
# endif

#if !defined (_NOTHREADS) && !defined (_STLP_THREADS_DEFINED)
#  define _STLP_DEC_THREADS
#endif
