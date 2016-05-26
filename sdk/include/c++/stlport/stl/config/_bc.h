/* STLport configuration file
 * It is internal STLport header - DO NOT include it directly */

#define _STLP_COMPILER "Borland"

#if (__BORLANDC__ < 0x551)
#  error - Borland compilers below version 5.5.1 not supported.
#endif

#pragma defineonoption _STLP_NO_RTTI -RT-

#define _STLP_DONT_SUP_DFLT_PARAM

#if (__BORLANDC__ >= 0x580)
#  define _STLP_HAS_INCLUDE_NEXT
#  define _STLP_NATIVE_HEADER(header) <../include/##header>
#  define _STLP_NATIVE_CPP_RUNTIME_HEADER(header) <../include/dinkumware/##header>
#  define _STLP_NO_NEW_C_HEADERS
#  define _STLP_NO_FORCE_INSTANTIATE
#endif

#if (__BORLANDC__ >= 0x570) && (__BORLANDC__ < 0x580)
#  define _STLP_NO_NEW_C_HEADERS
#  define _STLP_NO_FORCE_INSTANTIATE
#  define _STLP_DEF_CONST_DEF_PARAM_BUG
#  define _STLP_USE_DEFAULT_FILE_OFFSET

#  if defined (__cplusplus)
#    define _STLP_NATIVE_CPP_C_INCLUDE_PATH ../include/c++/ ## GCC_VERSION
#    define _STLP_NATIVE_CPP_RUNTIME_INCLUDE_PATH ../include/c++/ ## GCC_VERSION
#  endif

#  undef _SYS_CDEFS_H
#  include </usr/include/sys/cdefs.h>

#  ifdef __MT__
#    define _PTHREADS
#    if !defined (_RTLDLL)
#      define _STLP_DONT_USE_PTHREAD_SPINLOCK
#    endif
#  else
#    define _NOTHREADS
#  endif

#  pragma defineonoption _CPPUNWIND -xd
#  define _STLP_NO_EXCEPTION_HEADER
#  define _STLP_DONT_USE_EXCEPTIONS
#endif 

#if (__BORLANDC__ >= 0x560) && (__BORLANDC__ < 0x570)
#  define _USE_OLD_RW_STL
#endif

#if (__BORLANDC__ >= 0x560)
#  if !defined (__BUILDING_STLPORT)
#    define NOWINBASEINTERLOCK  
#  endif
#  define _STLP_LANG_INVARIANT_DEFINED
#endif

#if (__BORLANDC__ < 0x590)
#  define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER
#  define _STLP_DLLEXPORT_NEEDS_PREDECLARATION
   // <bitset> problems
#  define _STLP_MEMBER_SPECIALIZATION_BUG 1
#  ifdef __cplusplus
#    define _STLP_TR1 _STLP_STD_NAME::tr1::
#  endif
#endif

#if (__BORLANDC__ < 0x564)
#  define _STLP_QUALIFIED_SPECIALIZATION_BUG
#  define _STLP_NO_MOVE_SEMANTIC
#endif

#define _STLP_DONT_USE_PRIV_NAMESPACE
#define _STLP_NO_TYPENAME_BEFORE_NAMESPACE
#define _STLP_NO_VENDOR_STDLIB_L
#define _STLP_NO_VENDOR_MATH_F
#define _STLP_DONT_USE_SHORT_STRING_OPTIM 1

#if (__BORLANDC__ < 0x570) || (__BORLANDC__ >= 0x580)
#define _STLP_NO_NATIVE_MBSTATE_T
#undef _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT
#endif

#if (__BORLANDC__ < 0x580) && !defined (_RTLDLL)
#  define _UNCAUGHT_EXCEPTION 1
#endif

// <limits> problem
#define _STLP_STATIC_CONST_INIT_BUG

#define _STLP_HAS_SPECIFIC_PROLOG_EPILOG 1

#define _STLP_LONG_LONG  __int64

// auto enable thread safety and exceptions:
#ifndef _CPPUNWIND
#  define _STLP_HAS_NO_EXCEPTIONS
#endif

#if defined (__MT__) && !defined (_NOTHREADS)
#  define _STLP_THREADS
#endif

#define _STLP_EXPORT_DECLSPEC __declspec(dllexport)
#define _STLP_IMPORT_DECLSPEC __declspec(dllimport)

#define _STLP_CLASS_EXPORT_DECLSPEC __declspec(dllexport)
#define _STLP_CLASS_IMPORT_DECLSPEC __declspec(dllimport)

#if defined (_DLL)
#  define _STLP_DLL
#endif
#if defined (_RTLDLL)
#  define _STLP_RUNTIME_DLL
#endif
#include <stl/config/_detect_dll_or_lib.h>
#undef _STLP_RUNTIME_DLL
#undef _STLP_DLL

#if defined (_STLP_USE_DYNAMIC_LIB)
#  define _STLP_USE_DECLSPEC 1
#  if defined (__BUILDING_STLPORT)
#    define _STLP_CALL __cdecl __export
#  else
#      define  _STLP_CALL __cdecl
#    endif
#else
#  define  _STLP_CALL __cdecl
#endif

#if !defined (__linux__)
#  include <stl/config/_auto_link.h>
#endif

#include <stl/config/_feedback.h>
