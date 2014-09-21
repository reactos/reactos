// STLport configuration file
// It is internal STLport header - DO NOT include it directly

#define _STLP_COMPILER "CC"

#define _STLP_HAS_SPECIFIC_PROLOG_EPILOG

// define native include path before trying to include anything

#define _STLP_NATIVE_HEADER(__x) </usr/include/CC/##__x>
#define _STLP_NATIVE_C_HEADER(__x) </usr/include/##__x>
#define _STLP_NATIVE_OLD_STREAMS_HEADER(__x) </usr/include/CC/##__x>
#define _STLP_NATIVE_CPP_C_HEADER(__x) </usr/include/CC/##__x>
#define _STLP_NATIVE_CPP_RUNTIME_HEADER(__x) </usr/include/CC/##__x>

#define _STLP_NO_NATIVE_MBSTATE_T

#define _STLP_NO_USING_FOR_GLOBAL_FUNCTIONS
#define _STLP_NO_NATIVE_WIDE_FUNCTIONS
#define _STLP_NO_MEMBER_TEMPLATE_CLASSES

// #define _STLP_NO_BAD_ALLOC

#define _STL_HAS_NAMESPACES

#if ! defined (__EXCEPTIONS) && ! defined (_STLP_NO_EXCEPTIONS)
#  define _STLP_NO_EXCEPTIONS
#endif

#define __EDG_SWITCHES

#define _STLP_USE_SGI_STRING         1

#define _STLP_HAS_NO_NEW_C_HEADERS 1
// #  define _STLP_VENDOR_GLOBAL_EXCEPT_STD

#define _STLP_NO_POST_COMPATIBLE_SECTION

#include <standards.h>

#if !(_COMPILER_VERSION >= 730)
#  define _STLP_NO_NEW_NEW_HEADER 1
#endif

#if (_COMPILER_VERSION >= 730 && defined(_STANDARD_C_PLUS_PLUS))
#  define _STLP_EXTERN_RANGE_ERRORS
#endif

#if !defined(_BOOL)
#  define _STLP_NO_BOOL
#endif
#if defined(_MIPS_SIM) && _MIPS_SIM == _ABIO32
#  define _STLP_STATIC_CONST_INIT_BUG
#endif

#if (_COMPILER_VERSION < 720) || (defined(_MIPS_SIM) && _MIPS_SIM == _ABIO32)
#  define _STLP_DEF_CONST_PLCT_NEW_BUG
#  define _STLP_DEF_CONST_DEF_PARAM_BUG
#endif
#if !((_COMPILER_VERSION >= 730) && defined(_MIPS_SIM) && _MIPS_SIM != _ABIO32)
#  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD
#endif
#if !defined(_STANDARD_C_PLUS_PLUS)
#  define _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS
#endif
#if !((_COMPILER_VERSION >= 721) && defined(_NAMESPACES))
#  define _STLP_HAS_NO_NAMESPACES
#endif
#if (_COMPILER_VERSION < 721) || !defined(_STL_HAS_NAMESPACES) || defined(_STLP_NO_NAMESPACES)
#  define _STLP_NO_EXCEPTION_HEADER
#endif
#if _COMPILER_VERSION < 730 || !defined(_STANDARD_C_PLUS_PLUS) || !defined(_NAMESPACES)
#  define _STLP_NO_BAD_ALLOC
#endif
#if defined(_LONGLONG) && defined(_SGIAPI) && _SGIAPI
#  define _STLP_LONG_LONG long long
#endif
#if !(_COMPILER_VERSION >= 730 && defined(_STANDARD_C_PLUS_PLUS))
#  define _STLP_USE_NO_IOSTREAMS
#endif
#if !(_COMPILER_VERSION >= 730 && defined(_STANDARD_C_PLUS_PLUS))
#  define _STLP_NO_AT_MEMBER_FUNCTION
#endif
// #   if !(_COMPILER_VERSION >= 730 && defined(_STANDARD_C_PLUS_PLUS))
#if !(_COMPILER_VERSION >= 721 && defined(_STANDARD_C_PLUS_PLUS))
#  define _STLP_NO_TEMPLATE_CONVERSIONS
#endif
#if !((_COMPILER_VERSION >= 730) && defined(_MIPS_SIM) && _MIPS_SIM != _ABIO32)
#  define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER
#endif

#if !defined (_NOTHREADS) && !defined (_STLP_THREADS_DEFINED) && !defined (__GNUC__)
#  define _STLP_SGI_THREADS
#endif
