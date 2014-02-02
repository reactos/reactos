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

// This header defines two streambufs:
// stdio_istreambuf, a read-only streambuf synchronized with a C stdio
// FILE object
// stdio_ostreambuf, a write-only streambuf synchronized with a C stdio
// FILE object.
// Note that neither stdio_istreambuf nor stdio_ostreambuf is a template;
// both classes are derived from basic_streambuf<char, char_traits<char> >.

// Note: the imbue() member function is a no-op.  In particular, these
// classes assume that codecvt<char, char, mbstate_t> is always an identity
// transformation.  This is true of the default locale, and of all locales
// defined for the C I/O library.  If you need to use a locale where
// the codecvt<char, char, mbstate_t> facet performs a nontrivial
// conversion, then you should use basic_filebuf<> instead of stdio_istreambuf
// or stdio_ostreambuf.  (If you don't understand what any of this means,
// then it's not a feature you need to worry about.  Locales where
// codecvt<char, char, mbstate_t> does something nontrivial are a rare
// corner case.)


#ifndef _STLP_STDIO_STREAMBUF
#define _STLP_STDIO_STREAMBUF

#include <streambuf>
#include <cstdio>              // For FILE.

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

// Base class for features common to stdio_istreambuf and stdio_ostreambuf
class stdio_streambuf_base :
  public basic_streambuf<char, char_traits<char> > /* FILE_basic_streambuf */ {
public:                         // Constructor, destructor.
  // The argument may not be null.  It must be an open file pointer.
  stdio_streambuf_base(FILE*);

  // The destructor flushes the stream, but does not close it.
  ~stdio_streambuf_base();

protected:                      // Virtual functions from basic_streambuf.
  streambuf* setbuf(char*, streamsize);

  pos_type seekoff(off_type, ios_base::seekdir,
                   ios_base::openmode
                          = ios_base::in | ios_base::out);
  pos_type seekpos(pos_type,
                   ios_base::openmode
                          = ios_base::in | ios_base::out);
  int sync();

protected:
  FILE* _M_file;
};

class stdio_istreambuf : public stdio_streambuf_base {
public:                         // Constructor, destructor.
  stdio_istreambuf(FILE* __f) : stdio_streambuf_base(__f) {}
  ~stdio_istreambuf();

protected:                      // Virtual functions from basic_streambuf.
  streamsize showmanyc();
  int_type underflow();
  int_type uflow();
  virtual int_type pbackfail(int_type c = traits_type::eof());
};

class stdio_ostreambuf : public stdio_streambuf_base {
public:                         // Constructor, destructor.
  stdio_ostreambuf(FILE* __f) : stdio_streambuf_base(__f) {}
  ~stdio_ostreambuf();

protected:                      // Virtual functions from basic_streambuf.
  streamsize showmanyc();
  int_type overflow(int_type c = traits_type::eof());
};

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#endif /* _STLP_STDIO_STREAMBUF */

// Local Variables:
// mode:C++
// End:
