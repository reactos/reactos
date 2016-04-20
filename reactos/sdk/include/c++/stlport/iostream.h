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

#ifndef _STLP_IOSTREAM_H
#define _STLP_IOSTREAM_H

#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x2035
#  include <stl/_prolog.h>
#endif

#include <iostream>

// Those should be included all separately, as they do contain using declarations
#include <streambuf.h>
#include <ostream.h>
#include <istream.h>

#ifndef _STLP_HAS_NO_NAMESPACES

#  ifdef _STLP_BROKEN_USING_DIRECTIVE
_STLP_USING_NAMESPACE(stlport)
#  else
using _STLP_STD::cin;
using _STLP_STD::cout;
using _STLP_STD::clog;
using _STLP_STD::cerr;
using _STLP_STD::iostream;
#    ifndef _STLP_NO_WCHAR_T
using _STLP_STD::wcin;
using _STLP_STD::wcout;
using _STLP_STD::wclog;
using _STLP_STD::wcerr;
#    endif
#  endif
#endif /* _STLP_HAS_NO_NAMESPACES */

// Obsolete classes for old-style backwards compatibility


class istream_withassign : public istream {
 public:
  istream_withassign() : istream((streambuf*)0) {}
  ~istream_withassign() {}

  istream_withassign& operator=(istream& __s) {
    ios::init(__s.rdbuf());
    return *this;
  }
  istream_withassign& operator=(streambuf* __s) {
    ios::init(__s);
    return *this;
  }
};

class ostream_withassign : public ostream {
 public:
  ostream_withassign() : ostream((streambuf*)0) {}
  ~ostream_withassign() {}

  ostream_withassign& operator=(ostream& __s) {
    ios::init(__s.rdbuf());
    return *this;
  }
  ostream_withassign& operator=(streambuf* __s) {
    ios::init(__s);
    return *this;
  }
};

class iostream_withassign : public iostream {
 public:
  iostream_withassign() : iostream((streambuf*)0) {}
  ~iostream_withassign() {}
  iostream_withassign & operator=(ios& __i) {
    ios::init(__i.rdbuf());
    return *this;
  }
  iostream_withassign & operator=(streambuf* __s) {
    ios::init(__s);
    return *this;
  }
} ;

#if (_STLP_OUTERMOST_HEADER_ID == 0x2035)
#  include <stl/_epilog.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#endif

#endif /* _STLP_IOSTREAM_H */

// Local Variables:
// mode:C++
// End:
