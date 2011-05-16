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

#ifndef _STLP_INTERNAL_CSTDDEF
#define _STLP_INTERNAL_CSTDDEF

#  if (__GNUC__ >= 3) && defined (__CYGWIN__) // this total HACK is the only expedient way I could cygwin to work with GCC 3.0
#    define __need_wint_t // mostly because wint_t didn't seem to get defined otherwise :(
#    define __need_wchar_t
#    define __need_size_t
#    define __need_ptrdiff_t
#    define __need_NULL
#  endif

#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <cstddef>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(cstddef)
#    endif
#  else
#    include <stddef.h>
#  endif

#  ifdef _STLP_IMPORT_VENDOR_CSTD
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::ptrdiff_t;
using _STLP_VENDOR_CSTD::size_t;
_STLP_END_NAMESPACE
#  endif /* _STLP_IMPORT_VENDOR_CSTD */

#endif /* _STLP_INTERNAL_CSTDDEF */
