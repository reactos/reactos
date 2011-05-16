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

#ifndef _STLP_SETJMP_H

#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x256
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x256) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#  define _STLP_SETJMP_H
#endif

#if defined(_STLP_WCE_EVC3)
struct _exception;
#endif

#if !defined (setjmp)
#  define _STLP_NATIVE_SETJMP_H_INCLUDED
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <setjmp.h>
#  else
#    include _STLP_NATIVE_C_HEADER(setjmp.h)
#  endif
#endif

#if !defined (_STLP_NATIVE_SETJMP_H_INCLUDED)
/* See errno.h file for a description of this problem. */
#  error setjmp has been defined before inclusion of setjmp.h header.
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x256)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
#endif /* _STLP_SETJMP_H */
