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

#include <memory>
#include <istream>
#include <fstream>
#if defined (_STLP_MSVC) || defined (__MWERKS__) || defined (__ICL) || defined (__ISCPP__)
#  define _STLP_USE_NOT_INIT_SEGMENT
#  include <iostream>
#endif

#include "stdio_streambuf.h"
#include "aligned_buffer.h"
#include "_stdio_file.h"
#include "c_locale.h"

// boris : note this is repeated in <iostream>
#ifndef _STLP_USE_NAMESPACES
// in case of SGI iostreams, we have to rename our streams not to clash with those
// provided in native lib
#  define cin _STLP_cin
#  define cout _STLP_cout
#  define cerr _STLP_cerr
#  define clog _STLP_clog
#endif

_STLP_BEGIN_NAMESPACE

// This file handles iostream initialization.  It is inherently
// nonportable, since the C++ language definition provides no mechanism
// for controlling order of initialization of nonlocal objects.
// Initialization has three parts, which must be performed in the following
// order:
//  (1) Initialize the locale system
//  (2) Call the constructors for the eight global stream objects.
//  (3) Create streambufs for the global stream objects, and initialize
//      the stream objects by calling the init() member function.


#if defined (_STLP_USE_NOT_INIT_SEGMENT)

// Definitions of the eight global I/O objects that are declared in
// <iostream>. For some compilers we use pragmas to put the global I/O
// objects into an initialization segment that will not
// be executed. We then explicitly invoke the constructors
// with placement new in ios_base::_S_initialize()

#  if defined (__MWERKS__)
#    pragma suppress_init_code on
#  else
#    pragma init_seg("STLPORT_NO_INIT")
#  endif

_STLP_DECLSPEC istream cin(0);
_STLP_DECLSPEC ostream cout(0);
_STLP_DECLSPEC ostream cerr(0);
_STLP_DECLSPEC ostream clog(0);

#  ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC wistream wcin(0);
_STLP_DECLSPEC wostream wcout(0);
_STLP_DECLSPEC wostream wcerr(0);
_STLP_DECLSPEC wostream wclog(0);
#  endif

#  if defined (__MWERKS__)
#    pragma suppress_init_code off
#  endif

#else

// Definitions of the eight global I/O objects that are declared in
// <iostream>.  Disgusting hack: we deliberately define them with the
// wrong types so that the constructors don't get run automatically.
// We need special tricks to make sure that these objects are struct-
// aligned rather than byte-aligned.

// This is not portable.  Declaring a variable with different types in
// two translations units is "undefined", according to the C++ standard.
// Most compilers, however, silently accept this instead of diagnosing
// it as an error.

#  ifndef __DMC__
_STLP_DECLSPEC _Stl_aligned_buffer<istream> cin;
_STLP_DECLSPEC _Stl_aligned_buffer<ostream> cout;
_STLP_DECLSPEC _Stl_aligned_buffer<ostream> cerr;
_STLP_DECLSPEC _Stl_aligned_buffer<ostream> clog;
#  else
_Stl_aligned_buffer<istream> cin;
_Stl_aligned_buffer<ostream> cout;
_Stl_aligned_buffer<ostream> cerr;
_Stl_aligned_buffer<ostream> clog;

#    pragma alias("?cin@std@@3V?$basic_istream@std@DV?$char_traits@std@D@1@@1@A", "?cin@std@@3T?$_Stl_aligned_buffer@std@V?$basic_istream@std@DV?$char_traits@std@D@1@@1@@1@A")
#    pragma alias("?cout@std@@3V?$basic_ostream@std@DV?$char_traits@std@D@1@@1@A", "?cout@std@@3T?$_Stl_aligned_buffer@std@V?$basic_ostream@std@DV?$char_traits@std@D@1@@1@@1@A")
#    pragma alias("?cerr@std@@3V?$basic_ostream@std@DV?$char_traits@std@D@1@@1@A", "?cerr@std@@3T?$_Stl_aligned_buffer@std@V?$basic_ostream@std@DV?$char_traits@std@D@1@@1@@1@A")
#    pragma alias("?clog@std@@3V?$basic_ostream@std@DV?$char_traits@std@D@1@@1@A", "?clog@std@@3T?$_Stl_aligned_buffer@std@V?$basic_ostream@std@DV?$char_traits@std@D@1@@1@@1@A")
#  endif

#  ifndef _STLP_NO_WCHAR_T

#    ifndef __DMC__
_STLP_DECLSPEC _Stl_aligned_buffer<wistream> wcin;
_STLP_DECLSPEC _Stl_aligned_buffer<wostream> wcout;
_STLP_DECLSPEC _Stl_aligned_buffer<wostream> wcerr;
_STLP_DECLSPEC _Stl_aligned_buffer<wostream> wclog;
#    else
_Stl_aligned_buffer<wistream> wcin;
_Stl_aligned_buffer<wostream> wcout;
_Stl_aligned_buffer<wostream> wcerr;
_Stl_aligned_buffer<wostream> wclog;

#      pragma alias("?wcin@std@@3V?$basic_istream@std@_YV?$char_traits@std@_Y@1@@1@A", "?wcin@std@@3T?$_Stl_aligned_buffer@std@V?$basic_istream@std@_YV?$char_traits@std@_Y@1@@1@@1@A")
#      pragma alias("?wcout@std@@3V?$basic_ostream@std@_YV?$char_traits@std@_Y@1@@1@A", "?wcout@std@@3T?$_Stl_aligned_buffer@std@V?$basic_ostream@std@_YV?$char_traits@std@_Y@1@@1@@1@A")
#      pragma alias("?wcerr@std@@3V?$basic_ostream@std@_YV?$char_traits@std@_Y@1@@1@A", "?wcerr@std@@3T?$_Stl_aligned_buffer@std@V?$basic_ostream@std@_YV?$char_traits@std@_Y@1@@1@@1@A")
#      pragma alias("?wclog@std@@3V?$basic_ostream@std@_YV?$char_traits@std@_Y@1@@1@A", "?wclog@std@@3T?$_Stl_aligned_buffer@std@V?$basic_ostream@std@_YV?$char_traits@std@_Y@1@@1@@1@A")
#    endif
#  endif
#endif /* STL_MSVC || __MWERKS__ */

// Member functions from class ios_base and ios_base::Init

long ios_base::Init::_S_count = 0;
// by default, those are synced
bool ios_base::_S_is_synced = true;

ios_base::Init::Init() {
  if (_S_count++ == 0) {
    _Locale_init();
    ios_base::_S_initialize();
    _Filebuf_base::_S_initialize();
  }
}

ios_base::Init::~Init() {
  if (--_S_count == 0) {
    ios_base::_S_uninitialize();
    _Locale_final();
  }
}

static int _Stl_extract_open_param(FILE* f)
{ return _FILE_fd(f); }

#ifdef _STLP_REDIRECT_STDSTREAMS
static const char* _Stl_extract_open_param(const char* name)
{ return name; }
#endif

template <class _Tp>
static filebuf*
_Stl_create_filebuf(_Tp x, ios_base::openmode mode ) {
  auto_ptr<filebuf> result(new basic_filebuf<char, char_traits<char> >());
  result->open(_Stl_extract_open_param(x), mode);

  if (result->is_open())
    return result.release();

  return 0;
}

#if !defined (_STLP_NO_WCHAR_T)
static wfilebuf*
_Stl_create_wfilebuf(FILE* f, ios_base::openmode mode) {
  auto_ptr<wfilebuf> result(new basic_filebuf<wchar_t, char_traits<wchar_t> >());
  result->_M_open(_FILE_fd(f), mode);

  if (result->is_open())
    return result.release();

  return 0;
}
#endif

void  _STLP_CALL ios_base::_S_initialize() {
#if !defined (_STLP_HAS_NO_NAMESPACES) && !defined (_STLP_DONT_USE_PRIV_NAMESPACE)
  using _STLP_PRIV stdio_istreambuf;
  using _STLP_PRIV stdio_ostreambuf;
#endif

  auto_ptr<streambuf> cin_buf;
  auto_ptr<streambuf> cout_buf;
  auto_ptr<streambuf> cerr_buf;
  auto_ptr<streambuf> clog_buf;

  if (_S_is_synced)
    cin_buf.reset(new stdio_istreambuf(stdin));
  else
    cin_buf.reset(_Stl_create_filebuf(stdin, ios_base::in));

  if (_S_is_synced) {
#ifdef _STLP_REDIRECT_STDSTREAMS
    cout_buf.reset(_Stl_create_filebuf("/stdout.txt", ios::out));
    cerr_buf.reset(_Stl_create_filebuf("/stderr.txt", ios::out));
    clog_buf.reset(_Stl_create_filebuf("/stdlog.txt", ios::out));
#else
    cout_buf.reset(new stdio_ostreambuf(stdout));
    cerr_buf.reset(new stdio_ostreambuf(stderr));
    clog_buf.reset(new stdio_ostreambuf(stderr));
#endif
  }
  else {
    cout_buf.reset(_Stl_create_filebuf(stdout, ios_base::out));
    cerr_buf.reset(_Stl_create_filebuf(stderr, ios_base::out));
    clog_buf.reset(_Stl_create_filebuf(stderr, ios_base::out));
  }

  istream* ptr_cin  = new(&cin)  istream(cin_buf.get()); cin_buf.release();
  ostream* ptr_cout = new(&cout) ostream(cout_buf.get()); cout_buf.release();
  ostream* ptr_cerr = new(&cerr) ostream(cerr_buf.get()); cerr_buf.release();
  /*ostream* ptr_clog = */ new(&clog) ostream(clog_buf.get()); clog_buf.release();
  ptr_cin->tie(ptr_cout);
  ptr_cerr->setf(ios_base::unitbuf);

#ifndef _STLP_NO_WCHAR_T
  auto_ptr<wfilebuf> win(_Stl_create_wfilebuf(stdin, ios_base::in));
  auto_ptr<wfilebuf> wout(_Stl_create_wfilebuf(stdout, ios_base::out));
  auto_ptr<wfilebuf> werr(_Stl_create_wfilebuf(stderr, ios_base::out));
  auto_ptr<wfilebuf> wlog(_Stl_create_wfilebuf(stderr, ios_base::out));

  // Run constructors for the four wide stream objects.
  wistream* ptr_wcin  = new(&wcin)  wistream(win.get()); win.release();
  wostream* ptr_wcout = new(&wcout) wostream(wout.get()); wout.release();
  wostream* ptr_wcerr = new(&wcerr) wostream(werr.get()); werr.release();
  /*wostream* ptr_wclog = */ new(&wclog) wostream(wlog.get()); wlog.release();

  ptr_wcin->tie(ptr_wcout);
  ptr_wcerr->setf(ios_base::unitbuf);
#endif
}

void _STLP_CALL ios_base::_S_uninitialize() {
  // Note that destroying output streambufs flushes the buffers.
  istream* ptr_cin  = &cin;
  ostream* ptr_cout = &cout;
  ostream* ptr_cerr = &cerr;
  ostream* ptr_clog = &clog;

  // We don't want any exceptions being thrown here
  ptr_cin->exceptions(0);
  ptr_cout->exceptions(0);
  ptr_cerr->exceptions(0);
  ptr_clog->exceptions(0);

  delete ptr_cin->rdbuf(0);
  delete ptr_cout->rdbuf(0);
  delete ptr_cerr->rdbuf(0);
  delete ptr_clog->rdbuf(0);

  _Destroy(ptr_cin);
  _Destroy(ptr_cout);
  _Destroy(ptr_cerr);
  _Destroy(ptr_clog);

#ifndef _STLP_NO_WCHAR_T
  wistream* ptr_wcin  = &wcin;
  wostream* ptr_wcout = &wcout;
  wostream* ptr_wcerr = &wcerr;
  wostream* ptr_wclog = &wclog;

  // We don't want any exceptions being thrown here
  ptr_wcin->exceptions(0);
  ptr_wcout->exceptions(0);
  ptr_wcerr->exceptions(0);
  ptr_wclog->exceptions(0);

  delete ptr_wcin->rdbuf(0);
  delete ptr_wcout->rdbuf(0);
  delete ptr_wcerr->rdbuf(0);
  delete ptr_wclog->rdbuf(0);

  _Destroy(ptr_wcin);
  _Destroy(ptr_wcout);
  _Destroy(ptr_wcerr);
  _Destroy(ptr_wclog);
#endif
}


bool _STLP_CALL ios_base::sync_with_stdio(bool sync) {
#  if !defined (_STLP_HAS_NO_NAMESPACES) && !defined (_STLP_DONT_USE_PRIV_NAMESPACE)
  using _STLP_PRIV stdio_istreambuf;
  using _STLP_PRIV stdio_ostreambuf;
#  endif

  if (sync == _S_is_synced) return sync;

  // if by any chance we got there before std streams initialization,
  // just set the sync flag and exit
  if (Init::_S_count == 0) {
    _S_is_synced = sync;
    return sync;
  }

  auto_ptr<streambuf> cin_buf;
  auto_ptr<streambuf> cout_buf;
  auto_ptr<streambuf> cerr_buf;
  auto_ptr<streambuf> clog_buf;

  if (sync)
    cin_buf.reset(new stdio_istreambuf(stdin));
  else
    cin_buf.reset(_Stl_create_filebuf(stdin, ios_base::in));

  if (sync) {
#ifdef _STLP_REDIRECT_STDSTREAMS
    cout_buf.reset(_Stl_create_filebuf("/stdout.txt", ios::out));
    cerr_buf.reset(_Stl_create_filebuf("/stderr.txt", ios::out));
    clog_buf.reset(_Stl_create_filebuf("/stdlog.txt", ios::out));
#else
    cout_buf.reset(new stdio_ostreambuf(stdout));
    cerr_buf.reset(new stdio_ostreambuf(stderr));
    clog_buf.reset(new stdio_ostreambuf(stderr));
#endif
  }
  else {
    cout_buf.reset(_Stl_create_filebuf(stdout, ios_base::out));
    cerr_buf.reset(_Stl_create_filebuf(stderr, ios_base::out));
    clog_buf.reset(_Stl_create_filebuf(stderr, ios_base::out));
  }

  if (cin_buf.get() != 0 && cout_buf.get() != 0 && cerr_buf.get() != 0 && clog_buf.get() != 0) {
    // When streambuf passed to rdbuf is not null, rdbuf is exception safe:
    delete (&cin)->rdbuf(cin_buf.release());
    delete (&cout)->rdbuf(cout_buf.release());
    delete (&cerr)->rdbuf(cerr_buf.release());
    delete (&clog)->rdbuf(clog_buf.release());
    _S_is_synced = sync;
  }

  return _S_is_synced;
}

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
