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
#  define _STLP_OUTERMOST_HEADER_ID 0x848
#  include <stl/_prolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x848) && ! defined (_STLP_DONT_POP_HEADER_ID)
#  define _STLP_DONT_POP_HEADER_ID
#endif

#if !defined(_STLP_NO_NEW_HEADER)
#  if defined (__BORLANDC__)
#    include <new>
#  elif defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <new.h>
#  elif defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800 && !defined(_MSC_VER))
#    include _STLP_NATIVE_OLD_STREAMS_HEADER(new.h)
#  else
#    if defined (__GNUC__) && (__GNUC__ >= 3)
#      include _STLP_NATIVE_OLD_STREAMS_HEADER(new.h)
#    else
#      include _STLP_NATIVE_CPP_RUNTIME_HEADER(new.h)
#    endif
#  endif
#endif /* !defined(_STLP_NO_NEW_HEADER) */

#if (_STLP_OUTERMOST_HEADER_ID == 0x848)
#  if ! defined (_STLP_DONT_POP_HEADER_ID)
#    include <stl/_epilog.h>
#    undef  _STLP_OUTERMOST_HEADER_ID
#  else
#    undef  _STLP_DONT_POP_HEADER_ID
#  endif
#endif

// Local Variables:
// mode:C++
// End:
