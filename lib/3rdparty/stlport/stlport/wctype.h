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
#  define _STLP_OUTERMOST_HEADER_ID 0x279
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x279) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

/* evc3 doesn't have wctype.h */
#if !defined(_STLP_WCE_EVC3)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    if defined (__hpux)
#      include_next <stdarg.h>
#      include_next <wchar.h>
#    endif
#    include_next <wctype.h>
#  else
#    if defined (__hpux)
#      include _STLP_NATIVE_C_HEADER(stdarg.h)
#      include _STLP_NATIVE_C_HEADER(wchar.h)
#    endif
#    include _STLP_NATIVE_C_HEADER(wctype.h)
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x279)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
