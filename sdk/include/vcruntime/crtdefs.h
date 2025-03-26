/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#include <vcruntime.h>
#include <specstrings.h>

#ifndef _INC_CRTDEFS
#define _INC_CRTDEFS

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#ifdef _USE_32BIT_TIME_T
#ifdef _WIN64
#error You cannot use 32-bit time_t (_USE_32BIT_TIME_T) with _WIN64
#undef _USE_32BIT_TIME_T
#endif
#else
#if _INTEGRAL_MAX_BITS < 64
#define _USE_32BIT_TIME_T
#endif
#endif

#pragma pack(push,_CRT_PACKING)

/* Disable non-ANSI C definitions if compiling with __STDC__ */
#if (!defined _CRT_DECLARE_NONSTDC_NAMES || !_CRT_DECLARE_NONSTDC_NAMES) && (defined _CRT_DECLARE_NONSTDC_NAMES || __STDC__)
#define NO_OLDNAMES
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #define _CRT_DISABLE_GCC_WARNINGS \
            _Pragma("GCC diagnostic push") \
            _Pragma("GCC diagnostic ignored \"-Wbuiltin-declaration-mismatch\"") \
            _Pragma("GCC diagnostic ignored \"-Wunknown-pragmas\"")
  #define _CRT_RESTORE_GCC_WARNINGS _Pragma("GCC diagnostic pop")
#else // __GNUC__
  #define _CRT_DISABLE_GCC_WARNINGS
  #define _CRT_RESTORE_GCC_WARNINGS
#endif // __GNUC__

/** Properties ***************************************************************/


#ifndef _CRT_DEFER_MACRO
#define _CRT_DEFER_MACRO(M,...) M(__VA_ARGS__)
#endif

#ifndef _W64
 #if !defined(_midl) && defined(_X86_) && _MSC_VER >= 1300
  #define _W64 __w64
 #else
  #define _W64
 #endif
#endif

//#define _CRT_ALTERNATIVE_INLINES

#ifndef _CRTIMP_ALT
 #ifdef _DLL
  #ifdef _CRT_ALTERNATIVE_INLINES
   #define _CRTIMP_ALT
  #else
   #define _CRTIMP_ALT _CRTIMP
   #define _CRT_ALTERNATIVE_IMPORTED
  #endif
 #else
  #define _CRTIMP_ALT
 #endif
#endif

#ifndef _CRTDATA
 #ifdef _M_CEE_PURE
  #define _CRTDATA(x) x
 #else
  #define _CRTDATA(x) _CRTIMP x
 #endif
#endif

#ifndef _CRTIMP2
 #define _CRTIMP2 _CRTIMP
#endif

#ifndef _CRTIMP_PURE
 #define _CRTIMP_PURE _CRTIMP
#endif

#ifndef _CRTIMP_ALTERNATIVE
 #define _CRTIMP_ALTERNATIVE _CRTIMP
 #define _CRT_ALTERNATIVE_IMPORTED
#endif

#ifndef _CRTIMP_NOIA64
 #ifdef __ia64__
  #define _CRTIMP_NOIA64
 #else
  #define _CRTIMP_NOIA64 _CRTIMP
 #endif
#endif

#ifndef _MRTIMP2
 #define _MRTIMP2  _CRTIMP
#endif

#ifndef _MCRTIMP
 #define _MCRTIMP _CRTIMP
#endif

#ifndef _PGLOBAL
 #define _PGLOBAL
#endif

#ifndef _AGLOBAL
 #define _AGLOBAL
#endif

#ifndef UNALIGNED
#if defined(__ia64__) || defined(__x86_64) || defined(__arm__) || defined(__arm64__)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

#ifndef _CRTNOALIAS
#define _CRTNOALIAS
#endif

#ifndef _CRT_UNUSED
#define _CRT_UNUSED(x) (void)x
#endif

#define __crt_typefix(ctype)

#ifndef _STATIC_ASSERT
  #ifdef __cplusplus
    #define _STATIC_ASSERT(expr) static_assert((expr), #expr)
  #elif defined(__clang__) || defined(__GNUC__)
    #define _STATIC_ASSERT(expr) _Static_assert((expr), #expr)
  #else
    #define _STATIC_ASSERT(expr) extern char (*__static_assert__(void)) [(expr) ? 1 : -1]
  #endif
#endif /* _STATIC_ASSERT */

/** Deprecated ***************************************************************/

#ifndef __STDC_WANT_SECURE_LIB__
#define __STDC_WANT_SECURE_LIB__ 1
#endif

#ifndef _CRT_INSECURE_DEPRECATE_CORE
# ifdef _CRT_SECURE_NO_DEPRECATE_CORE
#  define _CRT_INSECURE_DEPRECATE_CORE(_Replacement)
# else
#  define _CRT_INSECURE_DEPRECATE_CORE(_Replacement) \
    _CRT_DEPRECATE_TEXT("This may be unsafe, Try " #_Replacement " instead! Enable _CRT_SECURE_NO_DEPRECATE to avoid thie warning.")
# endif
#endif

#ifndef _CRT_NONSTDC_DEPRECATE
# ifdef _CRT_NONSTDC_NO_DEPRECATE
#  define _CRT_NONSTDC_DEPRECATE(_Replacement)
# else
#  define _CRT_NONSTDC_DEPRECATE(_Replacement) \
    _CRT_DEPRECATE_TEXT("Deprecated POSIX name, Try " #_Replacement " instead!")
# endif
#endif

#ifndef _CRT_INSECURE_DEPRECATE_GLOBALS
#define _CRT_INSECURE_DEPRECATE_GLOBALS(_Replacement)
#endif

#ifndef _CRT_MANAGED_HEAP_DEPRECATE
#define _CRT_MANAGED_HEAP_DEPRECATE
#endif

#ifndef _CRT_OBSOLETE
#define _CRT_OBSOLETE(_NewItem)
#endif

#ifndef _CRT_JIT_INTRINSIC
#define _CRT_JIT_INTRINSIC
#endif


/** Constants ****************************************************************/

#define _ARGMAX 100

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

#ifndef __REACTOS__
#define __STDC_SECURE_LIB__ 200411L
#define __GOT_SECURE_LIB__ __STDC_SECURE_LIB__
#define _SECURECRT_FILL_BUFFER_PATTERN 0xFD
#endif


/** Type definitions *********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCTYPE_T_DEFINED
#define _WCTYPE_T_DEFINED
  typedef unsigned short wint_t;
  typedef unsigned short wctype_t;
#endif

#ifndef _ERRCODE_DEFINED
#define _ERRCODE_DEFINED
  typedef int errcode;
  typedef int errno_t;
#endif

#ifndef _TIME32_T_DEFINED
#define _TIME32_T_DEFINED
  typedef long __time32_t;
#endif

#ifndef _TIME64_T_DEFINED
#define _TIME64_T_DEFINED
#if _INTEGRAL_MAX_BITS >= 64
  __MINGW_EXTENSION typedef __int64 __time64_t;
#endif
#endif

#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
#ifdef _USE_32BIT_TIME_T
  typedef __time32_t time_t;
#else
  typedef __time64_t time_t;
#endif
#endif

struct threadmbcinfostruct;
typedef struct threadmbcinfostruct *pthreadmbcinfo;

#ifndef _TAGLC_ID_DEFINED
#define _TAGLC_ID_DEFINED
  typedef struct tagLC_ID {
    unsigned short wLanguage;
    unsigned short wCountry;
    unsigned short wCodePage;
  } LC_ID,*LPLC_ID;
#endif

#ifndef _THREADLOCALEINFO
#define _THREADLOCALEINFO
  typedef struct threadlocaleinfostruct {
    int refcount;
    unsigned int lc_codepage;
    unsigned int lc_collate_cp;
    unsigned long lc_handle[6];
    LC_ID lc_id[6];
    struct {
      char *locale;
      wchar_t *wlocale;
      int *refcount;
      int *wrefcount;
    } lc_category[6];
    int lc_clike;
    int mb_cur_max;
    int *lconv_intl_refcount;
    int *lconv_num_refcount;
    int *lconv_mon_refcount;
    struct lconv *lconv;
    int *ctype1_refcount;
    unsigned short *ctype1;
    const unsigned short *pctype;
    const unsigned char *pclmap;
    const unsigned char *pcumap;
    struct __lc_time_data *lc_time_curr;
  } threadlocinfo, *pthreadlocinfo;
#endif

struct __lc_time_data;

#ifdef __cplusplus
}
#endif

#if defined(_PREFAST_) && defined(_PFT_SHOULD_CHECK_RETURN)
#define _Check_return_opt_ _Check_return_
#else
#define _Check_return_opt_
#endif

#if defined(_PREFAST_) && defined(_PFT_SHOULD_CHECK_RETURN_WAT)
#define _Check_return_wat_ _Check_return_
#else
#define _Check_return_wat_
#endif

#pragma pack(pop)

/* GCC-style diagnostics */
#ifndef PRAGMA_DIAGNOSTIC_IGNORED
# ifdef __clang__
#  define PRAGMA_DIAGNOSTIC_PUSH() _Pragma("clang diagnostic push")
#  define PRAGMA_DIAGNOSTIC_IGNORED(__x) \
    _Pragma(_CRT_STRINGIZE(clang diagnostic ignored _CRT_DEFER_MACRO(_CRT_STRINGIZE,__x)))
#  define PRAGMA_DIAGNOSTIC_POP() _Pragma("clang diagnostic pop")
# elif defined (__GNUC__)
#  define PRAGMA_DIAGNOSTIC_PUSH() _Pragma("GCC diagnostic push")
#  define PRAGMA_DIAGNOSTIC_IGNORED(__x) \
    _Pragma("GCC diagnostic ignored \"-Wpragmas\"") /* This allows us to use it for unkonwn warnings */ \
    _Pragma(_CRT_STRINGIZE(GCC diagnostic ignored _CRT_DEFER_MACRO(_CRT_STRINGIZE,__x))) \
    _Pragma("GCC diagnostic error \"-Wpragmas\"") /* This makes sure that we don't have side effects because we disabled it for our own use. This will be popped anyway. */
#  define PRAGMA_DIAGNOSTIC_POP() _Pragma("GCC diagnostic pop")
# else
#  define PRAGMA_DIAGNOSTIC_PUSH()
#  define PRAGMA_DIAGNOSTIC_IGNORED(__x)
#  define PRAGMA_DIAGNOSTIC_POP()
# endif
#endif

#endif /* !_INC_CRTDEFS */
