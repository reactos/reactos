// STLport config file for Apogee 4.x

#define _STLP_COMPILER "Apogee"

#define _STLP_NO_NEW_NEW_HEADER 1
#define _STLP_HAS_NO_NEW_C_HEADERS 1

#if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
# define _STLP_RAND48 1
#endif
// #  define _STLP_RAND48 1
#define _STLP_LONG_LONG long long
#define _STLP_NO_BAD_ALLOC 1
#define _STLP_NO_MEMBER_TEMPLATE_KEYWORD 1
#define _STLP_NON_TYPE_TMPL_PARAM_BUG 1
// #  define _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS 1
#define _STLP_NO_EXCEPTION_HEADER 1

#undef  _STLP_LINK_TIME_INSTANTIATION
#define _STLP_LINK_TIME_INSTANTIATION 1

#ifdef __STDLIB
#  undef _STLP_NO_NEW_C_HEADERS
#  undef _STLP_NO_NEW_NEW_HEADER
#  undef _STLP_NO_BAD_ALLOC
#  undef _STLP_LONG_LONG
#else
#  undef  _STLP_NO_EXCEPTION_SPEC
#  define _STLP_NO_EXCEPTION_SPEC 1
#endif
