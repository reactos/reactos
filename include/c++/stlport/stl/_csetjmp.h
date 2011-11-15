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

#ifndef _STLP_INTERNAL_CSETJMP
#define _STLP_INTERNAL_CSETJMP

// if the macro is on, the header is already there
#if !defined (setjmp)
#  if defined (_STLP_USE_NEW_C_HEADERS)
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <csetjmp>
#    else
#      include _STLP_NATIVE_CPP_C_HEADER(csetjmp)
#    endif
#  else
#    include <setjmp.h>
#  endif
#endif

#if defined (_STLP_IMPORT_VENDOR_CSTD)

#  if defined (__BORLANDC__) && defined (_STLP_USE_NEW_C_HEADERS)
/* For Borland, even if stdjmp.h is included symbols won't be in global namespace
 * so we need to reach them in vendor namespace:
 */
#    undef _STLP_NATIVE_SETJMP_H_INCLUDED
#  endif

_STLP_BEGIN_NAMESPACE
#  if !defined (_STLP_NATIVE_SETJMP_H_INCLUDED)
using _STLP_VENDOR_CSTD::jmp_buf;
#  else
// if setjmp.h was included first, this is in global namespace, not in
// vendor's std.  - 2005-08-04, ptr
using ::jmp_buf;
#  endif
#  if !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
#    if !defined (setjmp)
#      if !defined (__MSL__) || ((__MSL__ > 0x7001) && (__MSL__ < 0x8000))
#        ifndef _STLP_NATIVE_SETJMP_H_INCLUDED
using _STLP_VENDOR_CSTD::setjmp;
#        else
using ::setjmp;
#        endif
#      endif
#    endif
#    if !defined (_STLP_NATIVE_SETJMP_H_INCLUDED)
using _STLP_VENDOR_CSTD::longjmp;
#    else
using ::longjmp;
#    endif
#  endif
_STLP_END_NAMESPACE
#endif /* _STLP_IMPORT_VENDOR_CSTD */

#endif
