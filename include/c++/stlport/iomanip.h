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

#ifndef _STLP_IOMANIP_H
#define _STLP_IOMANIP_H

#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x2031
#  include <stl/_prolog.h>
#endif

#include <stl/_ioserr.h>

#include <iomanip>

#ifndef _STLP_HAS_NO_NAMESPACES
#  ifdef _STLP_BROKEN_USING_DIRECTIVE
_STLP_USING_NAMESPACE(stlport)
#  else
using _STLP_STD::setiosflags;
using _STLP_STD::resetiosflags;
using _STLP_STD::setbase;
using _STLP_STD::setfill;
using _STLP_STD::setprecision;
using _STLP_STD::setw;
#  endif
#endif /* _STLP_HAS_NO_NAMESPACES */

// get all the pollution we want
#include <iostream.h>

#if (_STLP_OUTERMOST_HEADER_ID == 0x2031)
#  include <stl/_epilog.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#endif

#endif /* _STLP_IOMANIP_H */

// Local Variables:
// mode:C++
// End:
