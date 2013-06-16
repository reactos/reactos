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

#ifndef _STLP_INTERNAL_CWCTYPE
#define _STLP_INTERNAL_CWCTYPE

#if defined (__BORLANDC__) && !defined (_STLP_INTERNAL_CCTYPE)
#  include <stl/_cctype.h>
#endif

#if !defined (_STLP_WCE_EVC3)
#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <cwctype>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(cwctype)
#    endif
#    if defined (__MSL__)
namespace std {
  typedef wchar_t wctrans_t;
  wint_t towctrans(wint_t c, wctrans_t value);
  wctrans_t wctrans(const char *name);
}
using std::wctrans_t;
using std::towctrans;
using std::wctrans;
#    endif
#  else
#    include <wctype.h>
#  endif

#  if defined (_STLP_IMPORT_VENDOR_CSTD) && !defined (__hpux)

#    if defined (_STLP_USE_GLIBC) && !(defined (_GLIBCPP_USE_WCHAR_T) || defined (_GLIBCXX_USE_WCHAR_T)) || \
        defined (__sun) || defined (__FreeBSD__) || \
        defined (__CYGWIN__) || \
        defined (__MINGW32__) && ((__MINGW32_MAJOR_VERSION < 3) || (__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION <= 0))
//We take wide functions from global namespace:
#      define _STLP_VENDOR_CSTD_WFUNC
#    else
#      define _STLP_VENDOR_CSTD_WFUNC _STLP_VENDOR_CSTD
#    endif

_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD_WFUNC::wctype_t;
using _STLP_VENDOR_CSTD_WFUNC::wint_t;
#    if !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
#      if !defined (__BORLANDC__) && !defined (__MSL__)
using _STLP_VENDOR_CSTD_WFUNC::wctrans_t;
#        if !defined (__DMC__) && (!defined(_WIN32_WCE) || (_WIN32_WCE < 400))
using _STLP_VENDOR_CSTD_WFUNC::towctrans;
using _STLP_VENDOR_CSTD_WFUNC::wctrans;
using _STLP_VENDOR_CSTD_WFUNC::wctype;
#        endif
using _STLP_VENDOR_CSTD_WFUNC::iswctype;
#      endif
using _STLP_VENDOR_CSTD_WFUNC::iswalnum;
using _STLP_VENDOR_CSTD_WFUNC::iswalpha;
using _STLP_VENDOR_CSTD_WFUNC::iswcntrl;

using _STLP_VENDOR_CSTD_WFUNC::iswdigit;
using _STLP_VENDOR_CSTD_WFUNC::iswgraph;
using _STLP_VENDOR_CSTD_WFUNC::iswlower;
using _STLP_VENDOR_CSTD_WFUNC::iswprint;
using _STLP_VENDOR_CSTD_WFUNC::iswpunct;
using _STLP_VENDOR_CSTD_WFUNC::iswspace;
using _STLP_VENDOR_CSTD_WFUNC::iswupper;
using _STLP_VENDOR_CSTD_WFUNC::iswxdigit;

using _STLP_VENDOR_CSTD_WFUNC::towlower;
using _STLP_VENDOR_CSTD_WFUNC::towupper;
#    endif /* _STLP_NO_CSTD_FUNCTION_IMPORTS */
_STLP_END_NAMESPACE
#  endif /* _STLP_IMPORT_VENDOR_CSTD */
#endif /* _STLP_WCE_EVC3 */

#endif /* _STLP_INTERNAL_CWCTYPE */
