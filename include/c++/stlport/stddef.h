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

#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x262
#  include <stl/_cprolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x262) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if defined (_MSC_VER) || defined (__DMC__)
/* Native stddef.h contains errno macro definition making inclusion of native
 * errno.h in STLport errno.h impossible. We are then forced to include errno.h
 * first.
 */
#  include "errno.h"
#endif

#if defined (_STLP_HAS_INCLUDE_NEXT)
#  include_next <stddef.h>
#else
#  include _STLP_NATIVE_C_HEADER(stddef.h)
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x262)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif
