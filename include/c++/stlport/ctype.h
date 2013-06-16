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

#ifndef _STLP_CTYPE_H

/* Workaround for a "misbehaviour" when compiling resource scripts using
 * eMbedded Visual C++. The standard .rc file includes windows header files,
 * which in turn include ctype.h, which results in warnings and errors
 */
#if !defined(RC_INVOKED)

#  if !defined (_STLP_OUTERMOST_HEADER_ID)
#    define _STLP_OUTERMOST_HEADER_ID 0x219
#    include <stl/_prolog.h>
#  elif (_STLP_OUTERMOST_HEADER_ID == 0x219)
#    define _STLP_DONT_POP_HEADER_ID
#    define _STLP_CTYPE_H
#  endif

#  if defined(_STLP_WCE_EVC3)
struct _exception;
#  endif

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <ctype.h>
#  else
#    include _STLP_NATIVE_C_HEADER(ctype.h)
#  endif

/* on evc4 including ctype.h also defines setjmp macro */
#  if defined (_STLP_WCE)
#    define _STLP_NATIVE_SETJMP_H_INCLUDED
#  endif

#  ifndef _STLP_CTYPE_H_SEEN
#    define _STLP_CTYPE_H_SEEN

/* Undef convenience interfaces */
#    undef isspace
#    undef isprint
#    undef iscntrl
#    undef isupper
#    undef islower
#    undef isalpha
#    undef isdigit
#    undef ispunct
#    undef isxdigit
#    undef isalnum
#    undef isgraph
#    undef toupper
#    undef tolower

#    if defined (UNDER_CE)

#      if (_WIN32_WCE < 300)     /* Only wide chars for older versions */
#        define _isctype iswctype
#      endif

__inline int (isalpha)(int c) { return _isctype(c, _ALPHA); }
__inline int (isupper)(int c) { return _isctype(c, _UPPER); }
__inline int (islower)(int c) { return _isctype(c, _LOWER); }
__inline int (isdigit)(int c) { return _isctype(c, _DIGIT); }
__inline int (isxdigit)(int c) { return _isctype(c, _HEX); }
__inline int (isspace)(int c) { return _isctype(c, _SPACE); }
__inline int (ispunct)(int c) { return _isctype(c, _PUNCT); }
__inline int (isalnum)(int c) { return _isctype(c, _ALPHA|_DIGIT); }
__inline int (isprint)(int c) { return _isctype(c, _BLANK|_PUNCT|_ALPHA|_DIGIT); }
__inline int (isgraph)(int c) { return _isctype(c, _PUNCT|_ALPHA|_DIGIT); }
__inline int (iscntrl)(int c) { return _isctype(c, _CONTROL); }
__inline int (isascii)(int c) { return ((unsigned)(c) < 0x80); }

#      undef _isctype

__inline int (iswalpha)(int c) { return iswctype((unsigned short)(c), _ALPHA); }
__inline int (iswupper)(int c) { return iswctype((unsigned short)(c), _UPPER); }
__inline int (iswlower)(int c) { return iswctype((unsigned short)(c), _LOWER); }
__inline int (iswdigit)(int c) { return iswctype((unsigned short)(c), _DIGIT); }
__inline int (iswxdigit)(int c) { return iswctype((unsigned short)(c), _HEX); }
__inline int (iswspace)(int c) { return iswctype((unsigned short)(c), _SPACE); }
__inline int (iswpunct)(int c) { return iswctype((unsigned short)(c), _PUNCT); }
__inline int (iswalnum)(int c) { return iswctype((unsigned short)(c), _ALPHA|_DIGIT); }
__inline int (iswprint)(int c) { return iswctype((unsigned short)(c), _BLANK|_PUNCT|_ALPHA|_DIGIT); }
__inline int (iswgraph)(int c) { return iswctype((unsigned short)(c), _PUNCT|_ALPHA|_DIGIT); }
__inline int (iswcntrl)(int c) { return iswctype((unsigned short)(c), _CONTROL); }
__inline int (iswascii)(int c) { return ((unsigned)(c) < 0x80); }

#    endif /* UNDER_CE */

#  endif /* _STLP_CTYPE_H_SEEN */

#  if (_STLP_OUTERMOST_HEADER_ID == 0x219)
#    if ! defined (_STLP_DONT_POP_HEADER_ID)
#      include <stl/_epilog.h>
#      undef  _STLP_OUTERMOST_HEADER_ID
#    else
#      undef  _STLP_DONT_POP_HEADER_ID
#    endif
#  endif

#endif /* RC_INVOKED */

#endif /* _STLP_CTYPE_H */
