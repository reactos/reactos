/* STLport configuration file
 * It is internal STLport header - DO NOT include it directly
 */

#if defined (__IBMCPP__) || defined (__IBMC__)
#  define _STLP_COMPILER "Visual Age C++"
#elif defined (__xlC__)
#  define _STLP_COMPILER "xlc"
#else
#  error "Unknown compiler"
#endif

#if !defined(__IBMCPP__) || (__IBMCPP__ < 500)
# define _STLP_HAS_NO_NEW_C_HEADERS 1
#endif

/* string literal problem, same as with SUN and aCC */
# define _STLP_INLINE_STRING_LITERAL_BUG 1
# define _STLP_HAS_NATIVE_FLOAT_ABS

# define _STLP_DEF_CONST_PLCT_NEW_BUG 1
# define _STLP_DEF_CONST_DEF_PARAM_BUG 1


# ifdef __IBMCPP__
#  define _STLP_HAS_SPECIFIC_PROLOG_EPILOG
# endif

/* #  if ( defined (__MULTI__) && defined (__WINDOWS__))
 * #   define  _STLP_WIN32THREADS 1           Only Visual Age 3.5 for Windows
 * #  endif
 */

#  if ( defined (__MULTI__) && defined (__OS2__))
#   define _STLP_OS2THREADS 1
#  endif

/* __TEMPINC__ is set when /Ft+ option is used */
#  ifdef __TEMPINC__
#    define _STLP_LINK_TIME_INSTANTIATION 1
#  endif

# if defined (__MVS__)
/* long long support is buggy - reported by Tinny Ng
 * #  if __EXTENDED__ && __COMPILER_VER__ >= 0x22060000
 * #   define _STLP_LONG_LONG long long
 * #  endif
 */
/* boris : hstash reported it can be treated like UNIX */
#  define _STLP_NO_TYPEINFO 1
#  undef _STLP_NATIVE_INCLUDE_PATH
#  define _STLP_NATIVE_INCLUDE_PATH /usr/lpp/ioclib/include
/* same for C headers like <string.h> */
#  undef _STLP_NATIVE_C_INCLUDE_PATH
#  define _STLP_NATIVE_C_INCLUDE_PATH /usr/include
#  define _STLP_NATIVE_CPP_RUNTIME_INCLUDE_PATH /usr/include

# elif (defined (__WINDOWS__) || defined (_AIX) || defined (__OS2__) ) && (__IBMCPP__ >= 350)

#  define _STLP_LONG_LONG long long

#endif

#if !( defined( __xlC__ ) && __xlC__ >= 0x500 )

/* AIX xlC 3.1 , 3.0.1 ==0x301
 * Visual Age C++ 3.x
 * OS-390 C++
 * fbp : should be more version-specific!
 */

#  define _STLP_NO_BOOL 1
#  define _STLP_DONT_USE_BOOL_TYPEDEF 1
#  define _STLP_LIMITED_DEFAULT_TEMPLATES 1
#  define _STLP_HAS_NO_NAMESPACES 1
#  define _STLP_NEED_TYPENAME 1
#  define _STLP_NEED_EXPLICIT 1
#  define _STLP_NO_ARROW_OPERATOR 1
#  define _STLP_NO_NEW_STYLE_CASTS 1
#  define _STLP_NO_WCHAR_T 1
#  define _STLP_NEED_MUTABLE 1
#  define _STLP_NO_PARTIAL_SPECIALIZATION_SYNTAX 1
#  define _STLP_NO_BAD_ALLOC 1

#  define _STLP_NO_MEMBER_TEMPLATES 1
#  define _STLP_NO_MEMBER_TEMPLATE_CLASSES 1
#  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD 1
#  define _STLP_NO_QUALIFIED_FRIENDS 1
#  define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION 1
#  define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER 1

#  define _STLP_NO_DEFAULT_NON_TYPE_PARAM 1
#  define _STLP_NO_METHOD_SPECIALIZATION 1
#  define _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS 1
#  define _STLP_NO_EXCEPTION_HEADER 1

#  define _STLP_NO_NEW_NEW_HEADER 1

#  if defined (__OS2__) && (__IBMCPP__ <= 350)
#   define _STLP_NO_TYPEINFO 1
#  endif
#  define _STLP_NO_NEW_NEW_HEADER 1

#  define _STLP_STATIC_CONST_INIT_BUG 1
/* #  define _STLP_THROW_RETURN_BUG 1 */


#  define _STLP_NO_TEMPLATE_CONVERSIONS 1
#  define _STLP_BASE_TYPEDEF_OUTSIDE_BUG 1

/* AIX xlC, Visual Age 3.0 for OS/2 and MS */
#  define _STLP_TRIVIAL_DESTRUCTOR_BUG

#  define _STLP_NON_TYPE_TMPL_PARAM_BUG 1
#  define _STLP_NONTEMPL_BASE_MATCH_BUG 1

#if __IBMCPP__ <= 350
#  define _STLP_NEED_UNREACHABLE_RETURN 1
#endif

#if __IBMCPP__ < 350
#  define _STLP_NO_LONG_DOUBLE 1
#endif

#if __IBMCPP__ >= 365
/* does not have new C headers, but has div() */
#  define _STLP_LDIV
#endif

#endif /* xlC 5 */

/* detect xlC5 by: */
#if defined(__IBMCPP__) && (500 <= __IBMCPP__) && (__IBMCPP__ < 700)
#  define _STLP_USE_EXCEPTIONS 1
#  define _STLP_STATIC_CONST_INIT_BUG 1
/* #pragma report(disable,CPPC1500029)
 * unsigned 32-bit integer type
 */
#  define _STLP_UINT32_T unsigned int
#  if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
#    define _STLP_RAND48 1
#  endif
/* #  define _STLP_RAND48 1 */
#endif /* __IBMCPP__ == 500 */
