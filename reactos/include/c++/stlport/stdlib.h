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

/* Workaround for a "misbehaviour" when compiling resource scripts using
 * eMbedded Visual C++. The standard .rc file includes windows header files,
 * which in turn include stdlib.h, which results in warnings and errors
 */
#if !defined (RC_INVOKED)

#  if !defined (_STLP_OUTERMOST_HEADER_ID)
#    define _STLP_OUTERMOST_HEADER_ID 0x265
#    include <stl/_cprolog.h>
#  elif (_STLP_OUTERMOST_HEADER_ID == 0x265) && !defined (_STLP_DONT_POP_HEADER_ID)
#    define _STLP_DONT_POP_HEADER_ID
#  endif

#  if defined (_STLP_MSVC_LIB) || (defined (__GNUC__) && defined (__MINGW32__)) || \
       defined (__BORLANDC__) || defined (__DMC__) || \
       (defined (__HP_aCC) && defined (_REENTRANT))
/* Native stdlib.h contains errno macro definition making inclusion of native
 * errno.h in STLport errno.h impossible. We are then forced to include errno.h
 * first.
 */
#    include "errno.h"
#  endif

/*
 forward-declaration for _exception struct; prevents warning message
 ../include/stdlib.h(817) : warning C4115: '_exception' : named type definition in parentheses
*/
#  if defined(_STLP_WCE_EVC3)
struct _exception;
#  endif

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <stdlib.h>
#  else
#    include _STLP_NATIVE_C_HEADER(stdlib.h)
#  endif

/* on evc3/evc4 including stdlib.h also defines setjmp macro */
#  if defined (_STLP_WCE)
#    define _STLP_NATIVE_SETJMP_H_INCLUDED
#  endif

#  if (_STLP_OUTERMOST_HEADER_ID == 0x265)
#    if ! defined (_STLP_DONT_POP_HEADER_ID)
#      include <stl/_epilog.h>
#      undef  _STLP_OUTERMOST_HEADER_ID
#    else
#      undef  _STLP_DONT_POP_HEADER_ID
#    endif
#  endif

#endif /* RC_INVOKED */
