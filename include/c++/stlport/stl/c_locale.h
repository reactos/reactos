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

#ifndef _STLP_C_LOCALE_H
#define _STLP_C_LOCALE_H

/*
 * Implementation dependent definitions.
 * Beware: This header is not a purely internal header, it is also included
 * from the outside world when building the STLport library. So this header
 * should not reference internal headers (stlport/stl/_*.h) directly.
 */
#if defined (__sgi)
#  if defined (ROOT_65) /* IRIX 6.5.x */
#    include <sgidefs.h>
#    include <standards.h>
#    include <wchar.h>
#    include <ctype.h>
#  else /* IRIX pre-6.5 */
#    include <sgidefs.h>
#    include <standards.h>
#    if !defined(_SIZE_T) && !defined(_SIZE_T_)
#      define _SIZE_T
#      if (_MIPS_SZLONG == 32)
typedef unsigned int size_t;
#      endif
#      if (_MIPS_SZLONG == 64)
typedef unsigned long size_t;
#      endif
#    endif
#    if !defined (_WCHAR_T)
#      define _WCHAR_T
#      if (_MIPS_SZLONG == 32)
typedef long wchar_t;
#      endif
#      if (_MIPS_SZLONG == 64)
typedef __int32_t wchar_t;
#      endif
#    endif /* _WCHAR_T */
#    if !defined (_WINT_T)
#      define _WINT_T
#      if (_MIPS_SZLONG == 32)
typedef long wint_t;
#      endif
#      if (_MIPS_SZLONG == 64)
typedef __int32_t wint_t;
#      endif
#    endif /* _WINT_T */
#    if !defined (_MBSTATE_T)
#      define _MBSTATE_T
/* _MSC_VER check is here for historical reason and seems wrong as it is the macro defined
 * by Microsoft compilers to give their version. But we are in a SGI platform section so it
 * is weird. However _MSC_VER might also be a SGI compiler macro so we keep it this way.*/
#      if defined (_MSC_VER)
typedef int mbstate_t;
#      else
typedef char mbstate_t;
#      endif
#    endif /* _MBSTATE_T */
#  endif /* ROOT65 */
#elif defined (_STLP_USE_GLIBC)
#  include <ctype.h>
#endif

/*
 * GENERAL FRAMEWORK
 */

/*
 * Opaque types, implementation (if there is one) depends
 * on platform localisation API.
 */
struct _Locale_ctype;
struct _Locale_codecvt;
struct _Locale_numeric;
struct _Locale_time;
struct _Locale_collate;
struct _Locale_monetary;
struct _Locale_messages;

/*
  Bitmask macros.
*/

/*
 * For narrow characters, we expose the lookup table interface.
 */

#if defined (_STLP_USE_GLIBC)
/* This section uses macros defined in the gnu libc ctype.h header */
#  define _Locale_CNTRL  _IScntrl
#  define _Locale_UPPER  _ISupper
#  define _Locale_LOWER  _ISlower
#  define _Locale_DIGIT  _ISdigit
#  define _Locale_XDIGIT _ISxdigit
#  define _Locale_PUNCT  _ISpunct
#  define _Locale_SPACE  _ISspace
#  define _Locale_PRINT  _ISprint
#  define _Locale_ALPHA  _ISalpha
#else
/* Default values based on C++ Standard 22.2.1.
 * Under Windows the localisation implementation take care of mapping its
 * mask values to those internal values. For other platforms without real
 * localization support we are free to use the most suitable values.*/
#  define _Locale_SPACE  0x0001
#  define _Locale_PRINT  0x0002
#  define _Locale_CNTRL  0x0004
#  define _Locale_UPPER  0x0008
#  define _Locale_LOWER  0x0010
#  define _Locale_ALPHA  0x0020
#  define _Locale_DIGIT  0x0040
#  define _Locale_PUNCT  0x0080
#  define _Locale_XDIGIT 0x0100
#endif

#endif /* _STLP_C_LOCALE_H */
