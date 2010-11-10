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

#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x278
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x278) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if !defined (_STLP_WCE_EVC3) && !defined (_STLP_NO_WCHAR_T)

#  if defined (__BORLANDC__) && !defined (__linux__)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <_str.h>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(_str.h)
#    endif
#    ifdef __cplusplus
using _STLP_VENDOR_CSTD::strlen;
using _STLP_VENDOR_CSTD::strspn;
#    endif
#  endif

#  if (((__GNUC__ < 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ < 3))) && defined (__APPLE__)) || defined (__OpenBSD__)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <stddef.h>
#    else
#      include _STLP_NATIVE_C_HEADER(stddef.h)
#    endif
#  else
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <wchar.h>
#    else
#      include _STLP_NATIVE_C_HEADER(wchar.h)
#    endif
#  endif
#endif /* !defined (_STLP_WCE_EVC3) && !defined (_STLP_NO_WCHAR_T) */

#ifndef _STLP_INTERNAL_MBSTATE_T
#  include <stl/_mbstate_t.h>
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x278)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif

