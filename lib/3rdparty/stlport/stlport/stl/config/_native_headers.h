 /*
  *
  * Copyright (c) 2006
  * Francois Dumont
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

#if !defined (_STLP_MAKE_HEADER)
#  define _STLP_MAKE_HEADER(path, header) <path/header>
#endif

#if !defined (_STLP_NATIVE_HEADER)
#  if !defined (_STLP_NATIVE_INCLUDE_PATH)
#    define _STLP_NATIVE_INCLUDE_PATH ../include
#  endif
#  define _STLP_NATIVE_HEADER(header) _STLP_MAKE_HEADER(_STLP_NATIVE_INCLUDE_PATH,header)
#endif

/* For some compilers, C headers like <stdio.h> are located in separate directory */
#if !defined (_STLP_NATIVE_C_HEADER)
#  if !defined (_STLP_NATIVE_C_INCLUDE_PATH)
#    define _STLP_NATIVE_C_INCLUDE_PATH _STLP_NATIVE_INCLUDE_PATH
#  endif
#  define _STLP_NATIVE_C_HEADER(header)  _STLP_MAKE_HEADER(_STLP_NATIVE_C_INCLUDE_PATH,header)
#endif

/* For some compilers, C-library headers like <cstdio> are located in separate directory */
#if !defined (_STLP_NATIVE_CPP_C_HEADER)
#  if !defined (_STLP_NATIVE_CPP_C_INCLUDE_PATH)
#    define _STLP_NATIVE_CPP_C_INCLUDE_PATH _STLP_NATIVE_INCLUDE_PATH
#  endif
#  define _STLP_NATIVE_CPP_C_HEADER(header)  _STLP_MAKE_HEADER(_STLP_NATIVE_CPP_C_INCLUDE_PATH,header)
#endif

/* Some compilers locate basic C++ runtime support headers (<new>, <typeinfo>, <exception>) in separate directory */
#if !defined ( _STLP_NATIVE_CPP_RUNTIME_HEADER )
#  if !defined (_STLP_NATIVE_CPP_RUNTIME_INCLUDE_PATH)
#    define _STLP_NATIVE_CPP_RUNTIME_INCLUDE_PATH _STLP_NATIVE_INCLUDE_PATH
#  endif
#  define _STLP_NATIVE_CPP_RUNTIME_HEADER(header)  _STLP_MAKE_HEADER(_STLP_NATIVE_CPP_RUNTIME_INCLUDE_PATH,header)
#endif
