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

#ifndef _STLP_INTERNAL_CCTYPE
#define _STLP_INTERNAL_CCTYPE

#if defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cctype>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cctype)
#  endif
#else
#  include <ctype.h>
#endif /* _STLP_USE_NEW_C_HEADERS */

#if ! defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
#  if defined ( _STLP_IMPORT_VENDOR_CSTD )
_STLP_BEGIN_NAMESPACE
using _STLP_VENDOR_CSTD::isalnum;
using _STLP_VENDOR_CSTD::isalpha;
using _STLP_VENDOR_CSTD::iscntrl;
using _STLP_VENDOR_CSTD::isdigit;
using _STLP_VENDOR_CSTD::isgraph;
using _STLP_VENDOR_CSTD::islower;
using _STLP_VENDOR_CSTD::isprint;
using _STLP_VENDOR_CSTD::ispunct;
using _STLP_VENDOR_CSTD::isspace;
using _STLP_VENDOR_CSTD::isupper;
using _STLP_VENDOR_CSTD::isxdigit;
using _STLP_VENDOR_CSTD::tolower;
using _STLP_VENDOR_CSTD::toupper;
_STLP_END_NAMESPACE
#  endif /* _STLP_IMPORT_VENDOR_CSTD*/
#endif /* _STLP_NO_CSTD_FUNCTION_IMPORTS */

#endif
