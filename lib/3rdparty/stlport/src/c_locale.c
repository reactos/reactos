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

#include "stlport_prefix.h"

#include "c_locale.h"

#if defined (_STLP_WIN32) && !defined (_STLP_WCE)
#  include "c_locale_win32/c_locale_win32.c"
#elif defined (_STLP_USE_GLIBC2_LOCALIZATION)
#  include "c_locale_glibc/c_locale_glibc2.c" /* glibc 2.2 and newer */
#else
#  include "c_locale_dummy/c_locale_dummy.c"
#endif
