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

#ifndef _STLP_INTERNAL_CWCHAR
#define _STLP_INTERNAL_CWCHAR

#if defined (_STLP_WCE_EVC3)
#  ifndef _STLP_INTERNAL_MBSTATE_T
#    include <stl/_mbstate_t.h>
#  endif
#else
#  if defined (__GNUC__)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <cstddef>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(cstddef)
#    endif
#  endif

#  if !defined (_STLP_NO_CWCHAR) && defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <cwchar>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(cwchar)
#    endif
#    if defined (__OpenBSD__)
typedef _BSD_WINT_T_ wint_t;
#    endif /* __OpenBSD__ */

#    if defined (__DMC__)
#      define __STDC_LIMIT_MACROS
#      include <stdint.h> // WCHAR_MIN, WCHAR_MAX
#    endif
#  elif defined (_STLP_NO_WCHAR_T) || \
       (defined (__BORLANDC__) && (__BORLANDC__ < 0x570)) || \
        defined (__OpenBSD__) || defined (__FreeBSD__) || \
       (defined (__GNUC__) && (defined (__APPLE__) || defined ( __Lynx__ )))
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <stddef.h>
#    else
#      include _STLP_NATIVE_C_HEADER(stddef.h)
#    endif
#    if defined (__Lynx__)
#      ifndef _WINT_T
typedef long int wint_t;
#        define _WINT_T
#      endif /* _WINT_T */
#    endif
#    if defined(__OpenBSD__)
typedef _BSD_WINT_T_ wint_t;
#    endif /* __OpenBSD__ */
#  else
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <wchar.h>
#    else
#      include _STLP_NATIVE_C_HEADER(wchar.h)
#    endif

#    if defined (__sun) && (defined (_XOPEN_SOURCE) || (_XOPEN_VERSION - 0 == 4))
extern wint_t   btowc();
extern int      fwprintf();
extern int      fwscanf();
extern int      fwide();
extern int      mbsinit();
extern size_t   mbrlen();
extern size_t   mbrtowc();
extern size_t   mbsrtowcs();
extern int      swprintf();
extern int      swscanf();
extern int      vfwprintf();
extern int      vwprintf();
extern int      vswprintf();
extern size_t   wcrtomb();
extern size_t   wcsrtombs();
extern wchar_t  *wcsstr();
extern int      wctob();
extern wchar_t  *wmemchr();
extern int      wmemcmp();
extern wchar_t  *wmemcpy();
extern wchar_t  *wmemmove();
extern wchar_t  *wmemset();
extern int      wprintf();
extern int      wscanf();
#    endif
#  endif

#  if defined (__MSL__) && (__MSL__ <= 0x51FF)  /* dwa 2/28/99 - not yet implemented by MSL  */
#    define _STLP_WCHAR_MSL_EXCLUDE 1
namespace std {
  extern "C" size_t wcsftime(wchar_t * str, size_t max_size, const wchar_t * format_str, const struct tm * timeptr);
}
#    define _STLP_NO_NATIVE_MBSTATE_T 1
#  elif defined (__BORLANDC__)
#    if !defined (_STLP_USE_NO_IOSTREAMS)
#      define _STLP_NO_NATIVE_MBSTATE_T
#    endif
#    define _STLP_WCHAR_BORLAND_EXCLUDE 1
#  endif

#  ifndef _STLP_INTERNAL_MBSTATE_T
#    include <stl/_mbstate_t.h>
#  endif

#  if !defined (_STLP_NO_WCHAR_T)
#    ifndef WCHAR_MIN
#      define WCHAR_MIN 0
/* SUNpro has some bugs with casts. wchar_t is size of int there anyway. */
#      if defined (__SUNPRO_CC) || defined (__DJGPP)
#        define WCHAR_MAX (~0)
#      else
#        define WCHAR_MAX ((wchar_t)~0)
#      endif
#    endif
#    if defined (__DMC__) || (defined (_STLP_MSVC_LIB) && (_STLP_MSVC_LIB < 1400)) || defined(_WIN32_WCE)
/* Compilers that do not define WCHAR_MIN and WCHAR_MAX to be testable at
 * preprocessing time. */
#      undef WCHAR_MIN
#      define WCHAR_MIN 0
#      undef WCHAR_MAX
#      define WCHAR_MAX 0xffff
#    endif
#    if defined (__GNUC__) && defined (__alpha__)
/* Definition of WCHAR_MIN and MAX are wrong for alpha platform
 * as gcc consider wchar_t as an unsigned type but WCHAR_MIN is defined as
 * a negative value. Static assertion is here to check that a future alpha
 * SDK or a future gcc won't change the situation making this workaround
 * useless.
 */
/* Check that gcc still consider wchar_t as unsigned */
_STLP_STATIC_ASSERT(((wchar_t)-1 > 0))
/* Check that WCHAR_MIN value hasn't been fixed */
_STLP_STATIC_ASSERT((WCHAR_MIN < 0))
#      undef WCHAR_MIN
#      define WCHAR_MIN 0
#      undef WCHAR_MAX
#      define WCHAR_MAX 0xffffffff
#    endif
#    if defined(__HP_aCC) && (__HP_aCC >= 60000)
/* Starting with B.11.31, HP-UX/ia64 provides C99-compliant definitions
 * of WCHAR_MIN/MAX macros without having to define
 * _INCLUDE_STDC__SOURCE_199901 macro (which aCC compiler does not
 * predefine). Let STLport provide B.11.31 definitions on any version of
 * HP-UX/ia64.
 */
#      undef WCHAR_MIN
#      define WCHAR_MIN 0
#      undef WCHAR_MAX
#      define WCHAR_MAX UINT_MAX
#    endif
#  endif

#  if defined (_STLP_IMPORT_VENDOR_CSTD)

#    if defined (__SUNPRO_CC) && !defined (_STLP_HAS_NO_NEW_C_HEADERS)
using _STLP_VENDOR_CSTD::wint_t;
#    endif

_STLP_BEGIN_NAMESPACE
#    if defined (_STLP_NO_WCHAR_T)
typedef int wint_t;
#    else
// gcc 3.0 has a glitch : wint_t only sucked into the global namespace if _GLIBCPP_USE_WCHAR_T is defined
// __MWERKS__ has definition in wchar_t.h (MSL C++), but ones differ from definition
// in stdio.h; I prefer settings from last file.
#      if (defined (__GNUC__) && ! defined (_GLIBCPP_USE_WCHAR_T))
using ::wint_t;
#      else
using _STLP_VENDOR_CSTD::wint_t;
#      endif
#    endif

using _STLP_VENDOR_CSTD::size_t;

#    if !defined (_STLP_NO_NATIVE_MBSTATE_T) && !defined (_STLP_USE_OWN_MBSTATE_T)
using _STLP_VENDOR_MB_NAMESPACE::mbstate_t;

#      if !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS) && !defined(_STLP_WCHAR_BORLAND_EXCLUDE) && \
         (!defined(__MSL__) || __MSL__ > 0x6001)
#        if defined (__MINGW32__) && ((__MINGW32_MAJOR_VERSION > 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION >= 8))) || \
          !(defined (__KCC) || defined (__GNUC__)) && !defined(_STLP_WCE_NET)
using _STLP_VENDOR_MB_NAMESPACE::btowc;
#          if (!defined(__MSL__) || __MSL__ > 0x7001)
using _STLP_VENDOR_MB_NAMESPACE::mbsinit;
#          endif
#        endif
#        if defined (__MINGW32__) && ((__MINGW32_MAJOR_VERSION > 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION >= 8))) || \
           !defined (__GNUC__) && !defined(_STLP_WCE_NET)
using _STLP_VENDOR_MB_NAMESPACE::mbrlen;
using _STLP_VENDOR_MB_NAMESPACE::mbrtowc;
using _STLP_VENDOR_MB_NAMESPACE::mbsrtowcs;
using _STLP_VENDOR_MB_NAMESPACE::wcrtomb;
using _STLP_VENDOR_MB_NAMESPACE::wcsrtombs;
#        endif
#      endif /* BORLAND && !__MSL__ || __MSL__ > 0x6001 */

#    endif /* _STLP_NO_NATIVE_MBSTATE_T */

#    if !defined (_STLP_NO_NATIVE_WIDE_FUNCTIONS) && ! defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)

#      if !defined (_STLP_WCHAR_BORLAND_EXCLUDE) && ! defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
using _STLP_VENDOR_CSTD::fgetwc;
using _STLP_VENDOR_CSTD::fgetws;
using _STLP_VENDOR_CSTD::fputwc;
using _STLP_VENDOR_CSTD::fputws;
#      endif

#      if !(defined (_STLP_WCHAR_SUNPRO_EXCLUDE) || defined (_STLP_WCHAR_BORLAND_EXCLUDE) || \
            defined(_STLP_WCHAR_HPACC_EXCLUDE) )
#        if !defined (__DECCXX)
using _STLP_VENDOR_CSTD::fwide;
#        endif
using _STLP_VENDOR_CSTD::fwprintf;
using _STLP_VENDOR_CSTD::fwscanf;
using _STLP_VENDOR_CSTD::getwchar;
#      endif

#      if !defined(_STLP_WCHAR_BORLAND_EXCLUDE)
#        ifndef _STLP_WCE_NET
using _STLP_VENDOR_CSTD::getwc;
#        endif
using _STLP_VENDOR_CSTD::ungetwc;
#        ifndef _STLP_WCE_NET
using _STLP_VENDOR_CSTD::putwc;
#        endif
using _STLP_VENDOR_CSTD::putwchar;
#      endif

#      if !(defined (_STLP_WCHAR_SUNPRO_EXCLUDE) || defined (_STLP_WCHAR_BORLAND_EXCLUDE) || \
            defined (_STLP_WCHAR_HPACC_EXCLUDE) )
#        if defined (_STLP_MSVC_LIB) && (_STLP_MSVC_LIB <= 1300) || \
            defined (__MINGW32__)
#          undef swprintf
#          define swprintf _snwprintf
#          undef vswprintf
#          define vswprintf _vsnwprintf
using ::swprintf;
using ::vswprintf;
#        else
using _STLP_VENDOR_CSTD::swprintf;
using _STLP_VENDOR_CSTD::vswprintf;
#        endif
using _STLP_VENDOR_CSTD::swscanf;
using _STLP_VENDOR_CSTD::vfwprintf;
using _STLP_VENDOR_CSTD::vwprintf;

#        if (!defined(__MSL__) || __MSL__ > 0x7001 ) && !defined(_STLP_WCE_NET) && \
             !defined(_STLP_USE_UCLIBC) /* at least in uClibc 0.9.26 */

using _STLP_VENDOR_CSTD::wcsftime;
#        endif
using _STLP_VENDOR_CSTD::wcstok;

#      endif

#      if !defined (_STLP_WCE_NET)
using _STLP_VENDOR_CSTD::wcscoll;
using _STLP_VENDOR_CSTD::wcsxfrm;
#      endif
using _STLP_VENDOR_CSTD::wcscat;
using _STLP_VENDOR_CSTD::wcsrchr;
using _STLP_VENDOR_CSTD::wcscmp;

using _STLP_VENDOR_CSTD::wcscpy;
using _STLP_VENDOR_CSTD::wcscspn;

using _STLP_VENDOR_CSTD::wcslen;
using _STLP_VENDOR_CSTD::wcsncat;
using _STLP_VENDOR_CSTD::wcsncmp;
using _STLP_VENDOR_CSTD::wcsncpy;
using _STLP_VENDOR_CSTD::wcspbrk;
using _STLP_VENDOR_CSTD::wcschr;

using _STLP_VENDOR_CSTD::wcsspn;

#      if !defined (_STLP_WCHAR_BORLAND_EXCLUDE)
using _STLP_VENDOR_CSTD::wcstod;
using _STLP_VENDOR_CSTD::wcstol;
#      endif

#      if !(defined (_STLP_WCHAR_SUNPRO_EXCLUDE) || defined (_STLP_WCHAR_HPACC_EXCLUDE) )
using _STLP_VENDOR_CSTD::wcsstr;
using _STLP_VENDOR_CSTD::wmemchr;

#        if !defined (_STLP_WCHAR_BORLAND_EXCLUDE)
#            if !defined (_STLP_WCE_NET)
using _STLP_VENDOR_CSTD::wctob;
#            endif
#          if !defined (__DMC__)
using _STLP_VENDOR_CSTD::wmemcmp;
using _STLP_VENDOR_CSTD::wmemmove;
#          endif
using _STLP_VENDOR_CSTD::wprintf;
using _STLP_VENDOR_CSTD::wscanf;
#        endif

#        if defined (__BORLANDC__) && !defined (__linux__)
inline wchar_t* _STLP_wmemcpy(wchar_t* __wdst, const wchar_t* __wsrc, size_t __n)
{ return __STATIC_CAST(wchar_t*, _STLP_VENDOR_CSTD::wmemcpy(__wdst, __wsrc, __n)); }
inline wchar_t* _STLP_wmemset(wchar_t* __wdst, wchar_t __wc, size_t __n)
{ return __STATIC_CAST(wchar_t*, _STLP_VENDOR_CSTD::memset(__wdst, __wc, __n)); }
#          undef wmemcpy
#          undef wmemset
inline wchar_t* wmemcpy(wchar_t* __wdst, const wchar_t* __wsrc, size_t __n)
{ return _STLP_wmemcpy(__wdst, __wsrc, __n); }
inline wchar_t* wmemset(wchar_t* __wdst, wchar_t __wc, size_t __n)
{ return _STLP_wmemset(__wdst, __wc, __n); }
#        elif defined (__DMC__)
inline wchar_t* wmemcpy(wchar_t* __RESTRICT __wdst, const wchar_t* __RESTRICT __wsrc, size_t __n)
{ return __STATIC_CAST(wchar_t*, memcpy(__wdst, __wsrc, __n * sizeof(wchar_t))); }
inline wchar_t* wmemmove(wchar_t* __RESTRICT __wdst, const wchar_t * __RESTRICT __wc, size_t __n)
{ return __STATIC_CAST(wchar_t*, memmove(__wdst, __wc, __n * sizeof(wchar_t))); }
inline wchar_t* wmemset(wchar_t* __wdst, wchar_t __wc, size_t __n)
{ for (size_t i = 0; i < __n; i++) __wdst[i] = __wc; return __wdst; }
#        else
using _STLP_VENDOR_CSTD::wmemcpy;
using _STLP_VENDOR_CSTD::wmemset;
#        endif
#      endif

#    endif /* _STLP_NO_NATIVE_WIDE_FUNCTIONS */
_STLP_END_NAMESPACE

#  endif /* _STLP_IMPORT_VENDOR_CSTD */

#  undef _STLP_WCHAR_SUNPRO_EXCLUDE
#  undef _STLP_WCHAR_MSL_EXCLUDE

#  endif /* !defined(_STLP_WCE_EVC3) */

#endif /* _STLP_INTERNAL_CWCHAR */
