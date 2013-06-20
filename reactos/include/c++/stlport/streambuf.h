/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
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
#ifndef _STLP_STREAMBUF_H
#define _STLP_STREAMBUF_H

#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x2067
#  include <stl/_prolog.h>
#endif

#include <stl/_ioserr.h>

#include <streambuf>

#include <ios.h>

#ifndef _STLP_HAS_NO_NAMESPACES
#  ifdef _STLP_BROKEN_USING_DIRECTIVE
_STLP_USING_NAMESPACE(stlport)
#  else
using _STLP_STD::basic_streambuf;
using _STLP_STD::streambuf;
#    ifndef _STLP_NO_WCHAR_T
using _STLP_STD::wstreambuf;
#    endif
#  endif
#endif /* _STLP_HAS_NO_NAMESPACES */

#if (_STLP_OUTERMOST_HEADER_ID == 0x2067)
#  include <stl/_epilog.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#endif

#endif /* _STLP_STREAMBUF_H */

// Local Variables:
// mode:C++
// End:
