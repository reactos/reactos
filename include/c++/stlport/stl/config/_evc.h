/*
 * File to have Microsoft eMbedded Visual C++ 3.0 and .NET working with STLport
 * May 2004
 * Origin : Zdenek Nemec - zero@mapfactor.com
 *          Michael Fink - vividos@users.sourceforge.net
 */

#ifndef _STLP_EVC_H
#define _STLP_EVC_H

#define _STLP_COMPILER "eMbedded Visual C++"

// This flag is being used by STLport to support
// old-fashioned Windows CE SDK (see stl_wince.h)
// do not use with eMebedded Visual C++ 3 or 4!
#ifdef _STLP_WINCE
#  undef _STLP_WINCE
#endif

/* Compiler dependent define. The following defines may be available:
 * _STLP_WCE_EVC3 when compiling under eMbedded Visual C++ 3
 * _STLP_WCE_NET  when compiling under eMbedded Visual C++ .NET
 * _STLP_WCE      always defined when compiling with one of the above
 */
#undef _STLP_WCE_EVC3
#undef _STLP_WCE_NET

#if (_WIN32_WCE > 300)
#  define _STLP_WCE_NET UNDER_CE
#elif (_WIN32_WCE == 300)
#  define _STLP_WCE_EVC3 UNDER_CE
#else
#  error No support for Windows CE below 3.0!
#endif

// This is defined for all platforms using Windows CE
#define _STLP_WCE

/* All Windows CE versions up to at least version 5 are little-endian, even
 * if the hardware (like e.g. MIPS) can be configured for big-endian, too. */
#define _STLP_LITTLE_ENDIAN

// Ensure _DEBUG is defined.
#if defined (DEBUG) && !defined (_DEBUG)
#  define _DEBUG
#endif

// inherit all msvc6 options
#include <stl/config/_msvc.h>

// CE up to at least version 5 has no C locale support
#define _STLP_NO_LOCALE_SUPPORT

#if _WIN32_WCE >= 420
   // SDKs built with PB5 have terminate&co in namespace std...
#  define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_STD
#  define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_STD
   // ...and new_handler/set_new_handler in the global namespace.
#  define _STLP_GLOBAL_NEW_HANDLER 1
#endif

// Always threaded in eMbedded Visual C++ 3.0 and .NET
#ifndef _MT
#  define _MT
#endif

// we don't have a static native runtime library
#undef _STLP_USING_CROSS_NATIVE_RUNTIME_LIB

#if _WIN32_WCE < 400
// no long double under CE3 and older
#  define _STLP_NO_LONG_DOUBLE
#endif

// no *f and *l math functions available
#define _STLP_NO_VENDOR_MATH_F
#define _STLP_NO_VENDOR_MATH_L

/*
 * Redirect cout, cerr and clog:
 * If defined redirect cout, cerr and clog to
 * files stdout.txt, stderr.txt and stdlog.txt
 */
//# define _STLP_REDIRECT_STDSTREAMS

/*
 * Static class members may generate LNK1179:
 * Wrong COMDAT packing may cause LNK1179 error.
 * For details see http://groups.google.com/groups?th=8a05c82c4ffee280
 * example P78. This define is not used/needed at this moment
 * but may came handy in future...
 */
//# define _STLP_STATIC_MEMBERS_BUG

// Use wide-string interface of windows native functions (CreateFile...).
// Note that this should be defined all the time while under CE.
#if defined (UNICODE)
#  define _STLP_USE_WIDE_INTERFACE
#endif

// Force exception std to std instead of __std_alias.
#if defined (__cplusplus) && !defined (_STLP_HAS_NO_NAMESPACES)
#  ifdef _STLP_VENDOR_EXCEPT_STD
#    undef _STLP_VENDOR_EXCEPT_STD
#  endif
#  define _STLP_VENDOR_EXCEPT_STD std
#endif

// short string optimization bug under evc3, evc4 using ARM compiler
#if _MSC_VER < 1400 && (defined (ARM) || defined (_ARM_))
#  define _STLP_DONT_USE_SHORT_STRING_OPTIM
#endif

// when using MFC, disable another placement new declaration, since there is one in wcealt.h
#if !defined (__BUILDING_STLPORT) && defined (_MFC_VER)
#  define __PLACEMENT_NEW_INLINE
#endif

// threads
#undef _REENTRANT
#define _REENTRANT
#undef _NOTHREADS

// Use old fashioned headers (ctime vs. time.h).
#undef _STLP_NO_NEW_C_HEADERS
#define _STLP_NO_NEW_C_HEADERS

// exception handling support: only on evc4 and user added /GX to project settings
#if defined (_STLP_WCE_EVC3) || !defined (_CPPUNWIND)
#  define _STLP_NO_EXCEPTION_HEADER
#  define _STLP_NO_EXCEPTIONS
#  undef _STLP_USE_EXCEPTIONS
#  ifndef _STLP_THROW_BAD_ALLOC
#    define _STLP_THROW_BAD_ALLOC { _STLP_WINCE_TRACE(L"out of memory"); ExitThread(1); }
#  endif
#endif

#define _STLP_WINCE_TRACE(msg) OutputDebugString(msg)

/*
 * eMbedded Visual C++ .NET specific settings
 */
#if defined (_STLP_WCE_NET)

// evc4 has no locale and time support
#  define _STLP_NO_LOCALE_SUPPORT
#  define _STLP_NO_TIME_SUPPORT

// ptrdiff_t is not defined in evc4 headers
#  ifndef _PTRDIFF_T_DEFINED
   typedef int ptrdiff_t;
#    define _PTRDIFF_T_DEFINED
#  endif

/*
 * Helper macros for including the native headers in cases where a file with
 * the same name also exists in the STLport include folder. The idea behind
 * this is that we first go up one directory and then down into a dir that
 * is only present in the native install but not in STLport.
 *
 */
#  if !defined (_STLP_NATIVE_INCLUDE_PATH)
#    if defined (_X86_)
#      if defined (_STLP_WCE_TARGET_PROC_SUBTYPE_EMULATOR)
#        define _STLP_NATIVE_INCLUDE_PATH ../Emulator
#      else
#        define _STLP_NATIVE_INCLUDE_PATH ../X86
#      endif
#    elif defined (_ARM_)
#      if _MSC_VER < 1400
         // eVC3/4
#        if defined (ARMV4)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4
#        elif defined (ARMV4I)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4i
#        elif defined (ARMV4T)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4t
#        else
#          error Unknown ARM SDK.
#        endif
#      else
         // VC8 crosscompiling for CE
#        if defined (ARMV4)
           // VC8 bundled Pocket PC 2003 SDK don't have a target CPU subfolder.
#          if defined(WIN32_PLATFORM_PSPC)
#            define _STLP_NATIVE_INCLUDE_PATH ../Include
#          else
#            define _STLP_NATIVE_INCLUDE_PATH ../Armv4
#          endif
#        elif defined(ARMV4I) || defined(ARMV4T)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4i
#        else
#          error Unknown ARM SDK.
#        endif
#      endif
#    elif defined (_MIPS_)
#      if defined (MIPS16)
#        define _STLP_NATIVE_INCLUDE_PATH ../mips16
#      elif defined (MIPSII)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsII
#      elif defined (MIPSII_FP)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsII_fp
#      elif defined (MIPSIV)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsIV
#      elif defined (MIPSIV_FP)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsIV_fp
#      else
#        error Unknown MIPS SDK.
#      endif
#    elif defined (SHx)
#      if defined (SH3)
#        define _STLP_NATIVE_INCLUDE_PATH ../sh3
#      elif defined (SH4)
#        define _STLP_NATIVE_INCLUDE_PATH ../sh4
#      else
#        error Unknown SHx SDK.
#      endif
#    else
#      error Unknown SDK.
#    endif
#  endif /* !_STLP_NATIVE_INCLUDE_PATH */

/* Workaround when using MFCCE and using <new> together: MFCCE's wcealt.h doesn't
 * check for __PLACEMENT_NEW_INLINE before defining operator new, so when <new>
 * defines the operatore before, there will be an error C2084:
 * "function 'void *__cdecl operator new(unsigned int,void *)' already has a body".
 */
#  ifdef _STLP_USE_MFC
#    define __PLACEMENT_NEW_INLINE
#  endif

#endif /* _STLP_WCE_NET */

/*
 * eMbedded Visual C++ 3.0 specific settings
 */
#if defined (_STLP_WCE_EVC3)

#  define _STLP_NO_NATIVE_MBSTATE_T

// evc3 has no locale and time support
#  define _STLP_NO_LOCALE_SUPPORT
#  define _STLP_NO_TIME_SUPPORT

// evc3 has new, but no explicit header
#  define _STLP_NO_NEW_HEADER
#  define _STLP_NO_NEW_NEW_HEADER

// evc3 has no bad_alloc and no typeinfo
#  undef _STLP_NO_BAD_ALLOC
#  define _STLP_NO_BAD_ALLOC

#  undef _STLP_NO_TYPEINFO
#  define _STLP_NO_TYPEINFO

// missing things in eMbedded Visual C++ 3.0 headers
#  ifndef _SIZE_T_DEFINED
   typedef unsigned int size_t;
#    define _SIZE_T_DEFINED
#  endif

#  ifndef _WCHAR_T_DEFINED
   typedef unsigned short wchar_t;
#    define _WCHAR_T_DEFINED
#  endif

// ptrdiff_t is not defined in evc3 headers
#  ifndef _PTRDIFF_T_DEFINED
   typedef int ptrdiff_t;
#    define _PTRDIFF_T_DEFINED
#  endif

// clock_t is not defined in evc3 headers
#  ifndef _CLOCK_T_DEFINED
   typedef long clock_t;
#    define _CLOCK_T_DEFINED
#  endif

// Struct tm is not defined in evc3 headers
#  ifndef _TM_DEFINED
struct tm {
   int tm_sec;     /* seconds after the minute - [0,59] */
   int tm_min;     /* minutes after the hour - [0,59] */
   int tm_hour;    /* hours since midnight - [0,23] */
   int tm_mday;    /* day of the month - [1,31] */
   int tm_mon;     /* months since January - [0,11] */
   int tm_year;    /* years since 1900 */
   int tm_wday;    /* days since Sunday - [0,6] */
   int tm_yday;    /* days since January 1 - [0,365] */
   int tm_isdst;   /* daylight savings time flag */
};
#    define _TM_DEFINED
#  endif

// define placement new and delete operator
// note: when MFCCE headers are included first, don't define the new operator,
//       since it was already defined in wcealt.h
#  ifdef __cplusplus
#    ifndef __PLACEMENT_NEW_INLINE
#      ifndef _MFC_VER
inline void *__cdecl operator new(size_t, void *_P) { return (_P); }
#      endif /* _MFC_VER */
inline void __cdecl operator delete(void *, void *) { return; }
#      define __PLACEMENT_NEW_INLINE
#    endif
#  endif /* __cplusplus */

// evc3 doesn't have native wide functions, e.g. fgetwc, wmemmove
#  define _STLP_NO_NATIVE_WIDE_FUNCTIONS

// evc3 doesn't have assert.h
#  ifndef _ASSERT_DEFINED
#    define assert(expr) _STLP_ASSERT(expr)
#    define _ASSERT_DEFINED
#  endif

#endif /* _STLP_WCE_EVC3 */

/*
 * original call: TerminateProcess(GetCurrentProcess(), 0);
 * we substitute the GetCurrentProcess() with the result of the inline function
 * defined in kfuncs.h, since we then can avoid including <windows.h> at all.
 * all needed Win32 API functions are defined in <stl/_windows.h>
 */
#ifndef _ABORT_DEFINED
#  define _STLP_ABORT() TerminateProcess(reinterpret_cast<HANDLE>(66), 0)
#  define _ABORT_DEFINED
#endif

// Notice: windows.h isn't included here anymore; all needed defines are in
// stl/_windows.h now

#endif /* _STLP_EVC_H */
