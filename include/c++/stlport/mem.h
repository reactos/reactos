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

#ifndef _STLP_mem_h

#if !defined (_STLP_OUTERMOST_HEADER_ID)
#  define _STLP_OUTERMOST_HEADER_ID 0x245
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x245) && !defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if (_STLP_OUTERMOST_HEADER_ID != 0x245) || defined (_STLP_DONT_POP_HEADER_ID)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <mem.h>
#  else
#    include _STLP_NATIVE_C_HEADER(mem.h)
#  endif
#else
#  if defined (__BORLANDC__) && defined (__USING_CNAME__)
#    define _USING_CNAME_WAS_UNDEFINED
#    undef __USING_CNAME__
#  endif

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <mem.h>
#  else
#    include _STLP_NATIVE_C_HEADER(mem.h)
#  endif

#  if defined (__BORLANDC__) && defined (_USING_CNAME_WAS_UNDEFINED)
#    define __USING_CNAME__
#    define _STLP_mem_h 1
#    undef _USING_CNAME_WAS_UNDEFINED
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x245)
#  if !defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  endif
#  undef  _STLP_DONT_POP_HEADER_ID
#endif

#endif /* _STLP_mem_h */
