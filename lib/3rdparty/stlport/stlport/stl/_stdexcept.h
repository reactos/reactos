/*
 * Copyright (c) 1996,1997
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

#ifndef _STLP_INTERNAL_STDEXCEPT
#define _STLP_INTERNAL_STDEXCEPT

#ifndef _STLP_INTERNAL_STDEXCEPT_BASE
#  include <stl/_stdexcept_base.h>
#endif

#if !defined (_STLP_USE_NATIVE_STDEXCEPT) || defined (_STLP_USE_OWN_NAMESPACE)

#  if defined(_STLP_USE_EXCEPTIONS) || \
    !(defined(_MIPS_SIM) && defined(_ABIO32) && (_MIPS_SIM == _ABIO32))

_STLP_BEGIN_NAMESPACE

class _STLP_CLASS_DECLSPEC logic_error : public __Named_exception {
public:
  logic_error(const string& __s) : __Named_exception(__s) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~logic_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC runtime_error : public __Named_exception {
public:
  runtime_error(const string& __s) : __Named_exception(__s) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~runtime_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC domain_error : public logic_error {
public:
  domain_error(const string& __arg) : logic_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~domain_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC invalid_argument : public logic_error {
public:
  invalid_argument(const string& __arg) : logic_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~invalid_argument() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC length_error : public logic_error {
public:
  length_error(const string& __arg) : logic_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~length_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC out_of_range : public logic_error {
public:
  out_of_range(const string& __arg) : logic_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~out_of_range() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC range_error : public runtime_error {
public:
  range_error(const string& __arg) : runtime_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~range_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC overflow_error : public runtime_error {
public:
  overflow_error(const string& __arg) : runtime_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~overflow_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

class _STLP_CLASS_DECLSPEC underflow_error : public runtime_error {
public:
  underflow_error(const string& __arg) : runtime_error(__arg) {}
#    ifndef _STLP_USE_NO_IOSTREAMS
  ~underflow_error() _STLP_NOTHROW_INHERENTLY;
#    endif
};

_STLP_END_NAMESPACE

#  endif
#endif

#endif /* _STLP_INTERNAL_STDEXCEPT */
