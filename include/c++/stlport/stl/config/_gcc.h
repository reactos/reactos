/* STLport configuration file
 * It is internal STLport header - DO NOT include it directly
 */

#define _STLP_COMPILER "gcc"

#define _STLP_HAS_INCLUDE_NEXT 1

#if (__GNUC__ < 2) || ((__GNUC__ < 3) && ((__GNUC_MINOR__ < 95) || (__GNUC_MINOR__ == 96)))
/* We do not support neither the unofficial 2.96 gcc version. */
#  error GNU compilers before 2.95 are not supported anymore.
#endif

/* Systems having GLIBC installed have different traits */
#if defined (__linux__)
#  ifndef _STLP_USE_GLIBC
#    define _STLP_USE_GLIBC 1
#  endif
#  if defined (__UCLIBC__) && !defined (_STLP_USE_UCLIBC)
#    define _STLP_USE_UCLIBC 1
#  endif
#endif

#if defined (__CYGWIN__) && \
     (__GNUC__ >= 3) && (__GNUC_MINOR__ >= 3) && !defined (_GLIBCPP_USE_C99)
#  define _STLP_NO_VENDOR_MATH_L
#  define _STLP_NO_VENDOR_STDLIB_L
#endif

#if (__GNUC__ < 3)
#  define _STLP_NO_VENDOR_STDLIB_L
#endif

#if (__GNUC__ < 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ < 4))
/* define for gcc versions before 3.4.0. */
#  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD
#endif

#if !defined (_REENTRANT) && (defined (_THREAD_SAFE) || \
                             (defined (__OpenBSD__) && defined (_POSIX_THREADS)) || \
                             (defined (__MINGW32__) && defined (_MT)))
#  define _REENTRANT
#endif

#if defined (__DJGPP)
#  define _STLP_RAND48    1
#  define _NOTHREADS    1
#  undef  _PTHREADS
#  define _STLP_LITTLE_ENDIAN
#endif

#if defined (__MINGW32__)
/* Mingw32, egcs compiler using the Microsoft C runtime */
#  if (__GNUC__ >= 3)
/* For gcc before version 3 this macro is defined below */
#    define _STLP_VENDOR_GLOBAL_CSTD
#  endif
#  undef  _STLP_NO_DRAND48
#  define _STLP_NO_DRAND48
#  define _STLP_CALL
#endif /* __MINGW32__ */

#if defined (__CYGWIN__) || defined (__MINGW32__)
#  if !defined (_STLP_USE_STATIC_LIB)
#    define _STLP_USE_DECLSPEC 1
#    if !defined (_STLP_USE_DYNAMIC_LIB)
#      define _STLP_USE_DYNAMIC_LIB
#    endif
#    define _STLP_EXPORT_DECLSPEC __declspec(dllexport)
#    define _STLP_CLASS_EXPORT_DECLSPEC __declspec(dllexport)
#    define _STLP_CLASS_IMPORT_DECLSPEC __declspec(dllimport)
#  endif
/* The following is defined independently of _STLP_USE_STATIC_LIB because it is also
 * used to import symbols from PSDK under MinGW
 */
#  define _STLP_IMPORT_DECLSPEC __declspec(dllimport)
#else
#  if (__GNUC__ >= 4)
#    if !defined (_STLP_USE_STATIC_LIB)
#      if !defined (_STLP_USE_DYNAMIC_LIB)
#        define _STLP_USE_DYNAMIC_LIB
#      endif
#      define _STLP_USE_DECLSPEC 1
#      define _STLP_EXPORT_DECLSPEC __attribute__((visibility("default")))
#      define _STLP_IMPORT_DECLSPEC __attribute__((visibility("default")))
#      define _STLP_CLASS_EXPORT_DECLSPEC __attribute__((visibility("default")))
#      define _STLP_CLASS_IMPORT_DECLSPEC __attribute__((visibility("default")))
#    endif
#  endif
#endif

#if defined (__CYGWIN__) || defined (__MINGW32__) || !(defined (_STLP_USE_GLIBC) || defined (__sun) || defined(__APPLE__))
#  if !defined (__MINGW32__) && !defined (__CYGWIN__)
#    define _STLP_NO_NATIVE_MBSTATE_T    1
#  endif
#  if !defined (__MINGW32__) || (__GNUC__ < 3) || (__GNUC__ == 3) && (__GNUC_MINOR__ < 4)
#    define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#  endif
#  define _STLP_NO_NATIVE_WIDE_STREAMS   1
#endif

#define _STLP_NORETURN_FUNCTION __attribute__((noreturn))

/* Mac OS X is a little different with namespaces and cannot instantiate
 * static data members in template classes */
#if defined (__APPLE__)
#  if ((__GNUC__ < 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ < 3)))
/* Mac OS X is missing a required typedef and standard macro */
typedef unsigned int wint_t;
#  endif

#  define __unix

#  define _STLP_NO_LONG_DOUBLE

/* Mac OS X needs all "::" scope references to be "std::" */
#  define _STLP_USE_NEW_C_HEADERS

#  define _STLP_NO_VENDOR_STDLIB_L

#endif /* __APPLE__ */

/* g++ 2.7.x and above */
#define _STLP_LONG_LONG long long

#ifdef _STLP_USE_UCLIBC
  /* No *f math fuctions variants (i.e. sqrtf, fabsf, etc.) */
#  define _STLP_NO_VENDOR_MATH_F
  /* No *l math fuctions variants (i.e. sqrtl, fabsl, etc.) */
#  define _STLP_NO_VENDOR_MATH_L
#  define _STLP_NO_LONG_DOUBLE
#endif

#if defined (__OpenBSD__) || defined (__FreeBSD__)
#  define _STLP_NO_VENDOR_MATH_L
#  define _STLP_NO_VENDOR_STDLIB_L /* no llabs */
#  ifndef __unix
#    define __unix
#  endif
#endif

#if defined (__alpha__)
#  define _STLP_NO_VENDOR_MATH_L
#endif

#if defined (__hpux)
#  define _STLP_VENDOR_GLOBAL_CSTD 1
#  define _STLP_NO_VENDOR_STDLIB_L /* no llabs */
  /* No *f math fuctions variants (i.e. sqrtf, fabsf, etc.) */
#  define _STLP_NO_VENDOR_MATH_F
#endif

#if (__GNUC__ >= 3)
#  ifndef _STLP_HAS_NO_NEW_C_HEADERS
/*
#    ifndef _STLP_USE_UCLIBC
*/
#    define _STLP_HAS_NATIVE_FLOAT_ABS
/*
#    endif
*/
#  else
#    ifdef _STLP_USE_GLIBC
#      define _STLP_VENDOR_LONG_DOUBLE_MATH  1
#    endif
#  endif
#endif

#if (__GNUC__ < 3)
#  define _STLP_HAS_NO_NEW_C_HEADERS     1
#  define _STLP_VENDOR_GLOBAL_CSTD       1
#  define _STLP_DONT_USE_PTHREAD_SPINLOCK 1
#  ifndef __HONOR_STD
#    define _STLP_VENDOR_GLOBAL_EXCEPT_STD 1
#  endif
/* egcs fails to initialize builtin types in expr. like this : new(p) char();  */
#  define _STLP_DEF_CONST_PLCT_NEW_BUG 1
#endif

#undef _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#undef _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT

/* strict ANSI prohibits "long long" ( gcc) */
#if defined ( __STRICT_ANSI__ )
#  undef _STLP_LONG_LONG 
#endif

#ifndef __EXCEPTIONS
#  undef  _STLP_DONT_USE_EXCEPTIONS
#  define _STLP_DONT_USE_EXCEPTIONS 1
#endif

#if (__GNUC__ >= 3)
/* Instantiation scheme that used (default) in gcc 3 made void of sense explicit
   instantiation within library: nothing except increased library size. - ptr
 */
#  define _STLP_NO_FORCE_INSTANTIATE
#endif
