//
// lconv_unsigned_char_initialization.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// lconv initialization used to support an unsigned 'char' type (enabled by the
// -J flag during compilation).  Per the specification, some members of the lconv
// structure must be initialized to CHAR_MAX.  The value of CHAR_MAX, however,
// depends on whether the 'char' type is signed or unsigned (it is 127 for a
// signed 'char'; 255 for an unsigned 'char').
//
// By default, these members of lconv are initialized to SCHAR_MAX (127).  If an
// unsigned 'char' type is used, we call this function to update those members
// to UCHAR_MAX (255).  Note that this is not done for DLLs that are linked to
// the CRT DLL, because we do not want such DLLs to override the -J setting for
// an EXE linked to the CRT DLL.
//
// There is code in several other files that is required to make all of this work:
//
// * locale.h:  When -J is used to compile a source file that includes <locale.h>,
//   it generates an unresolved external to __do_unsigned_char_lconv_initialization.
//
// * lconv_unsigned_char_static.cpp:  This file defines a global variable named
//   __do_unsigned_char_lconv_initialization, and is used during the link to
//   satisfy the unresolved external.  This file adds a call to
//   __initialize_lconv_for_unsigned_char to the list of initializers to be
//   executed.  That function (defined below) updates the fields of the lconv
//   structure to be set to UCHAR_MAX.
//
// * localeconv.cpp:  This file defines the __acrt_lconv_c structure.
//
#include <corecrt_internal.h>
#include <limits.h>
#include <locale.h>

#pragma warning(disable: 4310) // cast truncates constant value



extern "C" int __cdecl __initialize_lconv_for_unsigned_char()
{
    __acrt_lconv_c.int_frac_digits = (char)UCHAR_MAX;
    __acrt_lconv_c.frac_digits     = (char)UCHAR_MAX;
    __acrt_lconv_c.p_cs_precedes   = (char)UCHAR_MAX;
    __acrt_lconv_c.p_sep_by_space  = (char)UCHAR_MAX;
    __acrt_lconv_c.n_cs_precedes   = (char)UCHAR_MAX;
    __acrt_lconv_c.n_sep_by_space  = (char)UCHAR_MAX;
    __acrt_lconv_c.p_sign_posn     = (char)UCHAR_MAX;
    __acrt_lconv_c.n_sign_posn     = (char)UCHAR_MAX;

    return 0;
}
