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

// DMC has hardcoded inclusion of typeinfo.h at the begining of any translation unit.
// So inclusion of this header will directly reference the native header. This is not
// a problem as typeinfo.h is neither a C nor C++ Standard header, this header should
// never be used in user code.
#if defined (__DMC__)
// We define _STLP_OUTERMOST_HEADER_ID to signal to other STLport headers that inclusion
// is done from native typeinfo.h (see exception header).
#  define _STLP_OUTERMOST_HEADER_ID 0x874
#  include <../include/typeinfo.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#else
#  ifndef _STLP_OLDSTD_typeinfo
#  define _STLP_OLDSTD_typeinfo

#  ifndef _STLP_OUTERMOST_HEADER_ID
#    define _STLP_OUTERMOST_HEADER_ID 0x874
#    include <stl/_prolog.h>
#  endif

#  ifndef _STLP_NO_TYPEINFO

#    if defined (__GNUC__)
#      undef _STLP_OLDSTD_typeinfo
#      include <typeinfo>
#      define _STLP_OLDSTD_typeinfo
#    else
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <typeinfo.h>
#      elif !defined (__BORLANDC__) || (__BORLANDC__ < 0x580)
#        include _STLP_NATIVE_CPP_RUNTIME_HEADER(typeinfo.h)
#      else
#        include _STLP_NATIVE_CPP_C_HEADER(typeinfo.h)
#      endif
#      if defined (__BORLANDC__) && (__BORLANDC__ >= 0x580) || \
          defined (__DMC__)
using std::type_info;
using std::bad_typeid;
using std::bad_cast;
#      endif
#    endif

// if <typeinfo> already included, do not import anything

#    if defined (_STLP_USE_OWN_NAMESPACE) && !(defined (_STLP_TYPEINFO) && !defined (_STLP_NO_NEW_NEW_HEADER))

_STLP_BEGIN_NAMESPACE

using /*_STLP_VENDOR_EXCEPT_STD */ :: type_info;
#      if !(defined(__MRC__) || (defined(__SC__) && !defined(__DMC__)))
using /* _STLP_VENDOR_EXCEPT_STD */ :: bad_typeid;
#      endif

using /* _STLP_VENDOR_EXCEPT_STD */ :: bad_cast;

_STLP_END_NAMESPACE

#    endif /* _STLP_OWN_NAMESPACE */

#  endif /* _STLP_NO_TYPEINFO */

#  if (_STLP_OUTERMOST_HEADER_ID == 0x874)
#    include <stl/_epilog.h>
#    undef _STLP_OUTERMOST_HEADER_ID
#  endif

#  endif /* _STLP_OLDSTD_typeinfo */

#endif /* __DMC__ */

// Local Variables:
// mode:C++
// End:
