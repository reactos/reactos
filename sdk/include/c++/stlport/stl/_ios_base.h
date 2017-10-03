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
#ifndef _STLP_IOS_BASE_H
#define _STLP_IOS_BASE_H

#ifndef _STLP_INTERNAL_STDEXCEPT_BASE
#  include <stl/_stdexcept_base.h>
#endif

#ifndef _STLP_INTERNAL_PAIR_H
#  include <stl/_pair.h>
#endif

#ifndef _STLP_INTERNAL_LOCALE_H
#  include <stl/_locale.h>
#endif

#ifndef _STLP_INTERNAL_STRING_H
#  include <stl/_string.h>
#endif

_STLP_BEGIN_NAMESPACE

// ----------------------------------------------------------------------

// Class ios_base.  This is the base class of the ios hierarchy, which
// includes basic_istream and basic_ostream.  Classes in the ios
// hierarchy are actually quite simple: they are just glorified
// wrapper classes.  They delegate buffering and physical character
// manipulation to the streambuf classes, and they delegate most
// formatting tasks to a locale.

class _STLP_CLASS_DECLSPEC ios_base {
public:

  class _STLP_CLASS_DECLSPEC failure : public __Named_exception {
  public:
    explicit failure(const string&);
    virtual ~failure() _STLP_NOTHROW_INHERENTLY;
  };

  typedef int fmtflags;
  typedef int iostate;
  typedef int openmode;
  typedef int seekdir;

# ifndef _STLP_NO_ANACHRONISMS
  typedef fmtflags fmt_flags;
# endif

  // Formatting flags.
  _STLP_STATIC_CONSTANT(int, left = 0x0001);
  _STLP_STATIC_CONSTANT(int, right = 0x0002);
  _STLP_STATIC_CONSTANT(int, internal   = 0x0004);
  _STLP_STATIC_CONSTANT(int, dec        = 0x0008);
  _STLP_STATIC_CONSTANT(int, hex        = 0x0010);
  _STLP_STATIC_CONSTANT(int, oct        = 0x0020);
  _STLP_STATIC_CONSTANT(int, fixed      = 0x0040);
  _STLP_STATIC_CONSTANT(int, scientific = 0x0080);
  _STLP_STATIC_CONSTANT(int, boolalpha  = 0x0100);
  _STLP_STATIC_CONSTANT(int, showbase   = 0x0200);
  _STLP_STATIC_CONSTANT(int, showpoint  = 0x0400);
  _STLP_STATIC_CONSTANT(int, showpos    = 0x0800);
  _STLP_STATIC_CONSTANT(int, skipws     = 0x1000);
  _STLP_STATIC_CONSTANT(int, unitbuf    = 0x2000);
  _STLP_STATIC_CONSTANT(int, uppercase  = 0x4000);
  _STLP_STATIC_CONSTANT(int, adjustfield = left | right | internal);
  _STLP_STATIC_CONSTANT(int, basefield   = dec | hex | oct);
  _STLP_STATIC_CONSTANT(int, floatfield  = scientific | fixed);

  // State flags.
  _STLP_STATIC_CONSTANT(int, goodbit = 0x00);
  _STLP_STATIC_CONSTANT(int, badbit  = 0x01);
  _STLP_STATIC_CONSTANT(int, eofbit  = 0x02);
  _STLP_STATIC_CONSTANT(int, failbit = 0x04);

  // Openmode flags.
  _STLP_STATIC_CONSTANT(int, __default_mode = 0x0); /* implementation detail */
  _STLP_STATIC_CONSTANT(int, app    = 0x01);
  _STLP_STATIC_CONSTANT(int, ate    = 0x02);
  _STLP_STATIC_CONSTANT(int, binary = 0x04);
  _STLP_STATIC_CONSTANT(int, in     = 0x08);
  _STLP_STATIC_CONSTANT(int, out    = 0x10);
  _STLP_STATIC_CONSTANT(int, trunc  = 0x20);

  // Seekdir flags
  _STLP_STATIC_CONSTANT(int, beg = 0x01);
  _STLP_STATIC_CONSTANT(int, cur = 0x02);
  _STLP_STATIC_CONSTANT(int, end = 0x04);

public:                         // Flag-manipulation functions.
  fmtflags flags() const { return _M_fmtflags; }
  fmtflags flags(fmtflags __flags) {
    fmtflags __tmp = _M_fmtflags;
    _M_fmtflags = __flags;
    return __tmp;
  }

  fmtflags setf(fmtflags __flag) {
    fmtflags __tmp = _M_fmtflags;
    _M_fmtflags |= __flag;
    return __tmp;
  }
  fmtflags setf(fmtflags __flag, fmtflags __mask) {
    fmtflags __tmp = _M_fmtflags;
    _M_fmtflags &= ~__mask;
    _M_fmtflags |= __flag & __mask;
    return __tmp;
  }
  void unsetf(fmtflags __mask) { _M_fmtflags &= ~__mask; }

  streamsize precision() const { return _M_precision; }
  streamsize precision(streamsize __newprecision) {
    streamsize __tmp = _M_precision;
    _M_precision = __newprecision;
    return __tmp;
  }

  streamsize width() const { return _M_width; }
  streamsize width(streamsize __newwidth) {
    streamsize __tmp = _M_width;
    _M_width = __newwidth;
    return __tmp;
  }

public:                         // Locales
  locale imbue(const locale&);
  locale getloc() const { return _M_locale; }

public:                         // Auxiliary storage.
  static int _STLP_CALL xalloc();
  long&  iword(int __index);
  void*& pword(int __index);

public:                         // Destructor.
  virtual ~ios_base();

public:                         // Callbacks.
  enum event { erase_event, imbue_event, copyfmt_event };
  typedef void (*event_callback)(event, ios_base&, int __index);
  void register_callback(event_callback __fn, int __index);

public:                         // This member function affects only
                                // the eight predefined ios objects:
                                // cin, cout, etc.
  static bool _STLP_CALL sync_with_stdio(bool __sync = true);

public:                         // The C++ standard requires only that these
                                // member functions be defined in basic_ios.
                                // We define them in the non-template
                                // base class to avoid code duplication.
  operator void*() const { return !fail() ? (void*) __CONST_CAST(ios_base*,this) : (void*) 0; }
  bool operator!() const { return fail(); }

  iostate rdstate() const { return _M_iostate; }

  bool good() const { return _M_iostate == 0; }
  bool eof() const { return (_M_iostate & eofbit) != 0; }
  bool fail() const { return (_M_iostate & (failbit | badbit)) != 0; }
  bool bad() const { return (_M_iostate & badbit) != 0; }

protected:                      // The functional protected interface.

  // Copies the state of __x to *this.  This member function makes it
  // possible to implement basic_ios::copyfmt without having to expose
  // ios_base's private data members.  Does not copy _M_exception_mask
  // or _M_iostate.
  void _M_copy_state(const ios_base& __x);

  void _M_setstate_nothrow(iostate __state) { _M_iostate |= __state; }
  void _M_clear_nothrow(iostate __state) { _M_iostate = __state; }
  iostate _M_get_exception_mask() const { return _M_exception_mask; }
  void _M_set_exception_mask(iostate __mask) { _M_exception_mask = __mask; }
  void _M_check_exception_mask() {
    if (_M_iostate & _M_exception_mask)
      _M_throw_failure();
  }

  void _M_invoke_callbacks(event);
  void _STLP_FUNCTION_THROWS _M_throw_failure();

  ios_base();                   // Default constructor.

protected:                        // Initialization of the I/O system
  static void _STLP_CALL _S_initialize();
  static void _STLP_CALL _S_uninitialize();
  static bool _S_is_synced;

private:                        // Invalidate the copy constructor and
                                // assignment operator.
  ios_base(const ios_base&);
  void operator=(const ios_base&);

private:                        // Data members.

  fmtflags _M_fmtflags;         // Flags
  iostate _M_iostate;
  openmode _M_openmode;
  seekdir _M_seekdir;
  iostate _M_exception_mask;

  streamsize _M_precision;
  streamsize _M_width;

  locale _M_locale;

  pair<event_callback, int>* _M_callbacks;
  size_t _M_num_callbacks;      // Size of the callback array.
  size_t _M_callback_index;     // Index of the next available callback;
                                // initially zero.

  long* _M_iwords;              // Auxiliary storage.  The count is zero
  size_t _M_num_iwords;         // if and only if the pointer is null.

  void** _M_pwords;
  size_t _M_num_pwords;

public:
  // ----------------------------------------------------------------------
  // Nested initializer class.  This is an implementation detail, but it's
  // prescribed by the standard.  The static initializer object (on
  // implementations where such a thing is required) is declared in
  // <iostream>
  class _STLP_CLASS_DECLSPEC Init
  {
    public:
      Init();
      ~Init();
    private:
      static long _S_count;
      friend class ios_base;
  };

  friend class Init;

public:
# ifndef _STLP_NO_ANACHRONISMS
  //  31.6  Old iostreams members                         [depr.ios.members]
  typedef iostate  io_state;
  typedef openmode open_mode;
  typedef seekdir  seek_dir;
  typedef _STLP_STD::streamoff  streamoff;
  typedef _STLP_STD::streampos  streampos;
# endif
};

// ----------------------------------------------------------------------
// ios_base manipulator functions, from section 27.4.5 of the C++ standard.
// All of them are trivial one-line wrapper functions.

// fmtflag manipulators, section 27.4.5.1
inline ios_base& _STLP_CALL boolalpha(ios_base& __s)
  { __s.setf(ios_base::boolalpha); return __s;}

inline ios_base& _STLP_CALL noboolalpha(ios_base& __s)
  { __s.unsetf(ios_base::boolalpha); return __s;}

inline ios_base& _STLP_CALL showbase(ios_base& __s)
  { __s.setf(ios_base::showbase); return __s;}

inline ios_base& _STLP_CALL noshowbase(ios_base& __s)
  { __s.unsetf(ios_base::showbase); return __s;}

inline ios_base& _STLP_CALL showpoint(ios_base& __s)
  { __s.setf(ios_base::showpoint); return __s;}

inline ios_base& _STLP_CALL noshowpoint(ios_base& __s)
  { __s.unsetf(ios_base::showpoint); return __s;}

inline ios_base& _STLP_CALL showpos(ios_base& __s)
  { __s.setf(ios_base::showpos); return __s;}

inline ios_base& _STLP_CALL noshowpos(ios_base& __s)
  { __s.unsetf(ios_base::showpos); return __s;}

inline ios_base& _STLP_CALL skipws(ios_base& __s)
  { __s.setf(ios_base::skipws); return __s;}

inline ios_base& _STLP_CALL noskipws(ios_base& __s)
  { __s.unsetf(ios_base::skipws); return __s;}

inline ios_base& _STLP_CALL uppercase(ios_base& __s)
  { __s.setf(ios_base::uppercase); return __s;}

inline ios_base& _STLP_CALL nouppercase(ios_base& __s)
  { __s.unsetf(ios_base::uppercase); return __s;}

inline ios_base& _STLP_CALL unitbuf(ios_base& __s)
  { __s.setf(ios_base::unitbuf); return __s;}

inline ios_base& _STLP_CALL nounitbuf(ios_base& __s)
  { __s.unsetf(ios_base::unitbuf); return __s;}


// adjustfield manipulators, section 27.4.5.2
inline ios_base& _STLP_CALL internal(ios_base& __s)
  { __s.setf(ios_base::internal, ios_base::adjustfield); return __s; }

inline ios_base& _STLP_CALL left(ios_base& __s)
  { __s.setf(ios_base::left, ios_base::adjustfield); return __s; }

inline ios_base& _STLP_CALL right(ios_base& __s)
  { __s.setf(ios_base::right, ios_base::adjustfield); return __s; }

// basefield manipulators, section 27.4.5.3
inline ios_base& _STLP_CALL dec(ios_base& __s)
  { __s.setf(ios_base::dec, ios_base::basefield); return __s; }

inline ios_base& _STLP_CALL hex(ios_base& __s)
  { __s.setf(ios_base::hex, ios_base::basefield); return __s; }

inline ios_base& _STLP_CALL oct(ios_base& __s)
  { __s.setf(ios_base::oct, ios_base::basefield); return __s; }


// floatfield manipulators, section 27.4.5.3
inline ios_base& _STLP_CALL fixed(ios_base& __s)
  { __s.setf(ios_base::fixed, ios_base::floatfield); return __s; }

inline ios_base& _STLP_CALL scientific(ios_base& __s)
  { __s.setf(ios_base::scientific, ios_base::floatfield); return __s; }

_STLP_END_NAMESPACE

#endif /* _STLP_IOS_BASE */

// Local Variables:
// mode:C++
// End:

