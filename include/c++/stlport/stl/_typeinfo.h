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

#ifndef _STLP_INTERNAL_TYPEINFO
#define _STLP_INTERNAL_TYPEINFO

#if !defined (_STLP_NO_TYPEINFO)

#  if defined (_STLP_NO_NEW_NEW_HEADER)
#    include <typeinfo.h>
#  else
#    if defined (_STLP_HAS_INCLUDE_NEXT)
#      include_next <typeinfo>
#    else
#      include _STLP_NATIVE_CPP_RUNTIME_HEADER(typeinfo)
#    endif
#  endif

#  if (defined(_STLP_MSVC) && (_STLP_MSVC >= 1300)) || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800))
// In .NET, <typeinfo> actually includes <typeinfo.h>
#    undef _STLP_OLDSTD_typeinfo
#  endif

// if <typeinfo.h> already included, do not import anything
#  if defined(_STLP_USE_NAMESPACES) && !defined(_STLP_OLDSTD_typeinfo) && \
      (defined(_STLP_VENDOR_GLOBAL_EXCEPT_STD) || \
       defined(_STLP_USE_OWN_NAMESPACE) || defined (_STLP_DEBUG))
#    if defined(_STLP_MSVC) && (_STLP_MSVC < 1300) && !defined(_STLP_WCE_NET)
class bad_cast : public exception {};
#    endif

_STLP_BEGIN_NAMESPACE
// VC 6 and eVC 4 have type_info in the global namespace
#    if (defined(_STLP_MSVC) && (_STLP_MSVC < 1300)) || defined(_STLP_WCE_NET)
using ::type_info;
#    else
using _STLP_VENDOR_EXCEPT_STD::type_info;
#    endif

#    if !defined (__DMC__)
using _STLP_VENDOR_EXCEPT_STD::bad_typeid;
#    endif

#    if defined (_STLP_MSVC) && (_STLP_MSVC < 1300) && !defined (_STLP_WCE_NET)
using ::bad_cast;
#    else
using _STLP_VENDOR_EXCEPT_STD::bad_cast;
#    endif

_STLP_END_NAMESPACE

#  endif

#else

#  ifndef _STLP_INTERNAL_EXCEPTION
#    include <stl/_exception.h>
#  endif

_STLP_BEGIN_NAMESPACE
#  if !defined (__DMC__)
struct bad_cast : exception {};
#  endif
_STLP_END_NAMESPACE
#endif

#endif
