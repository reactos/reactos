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


#ifndef _STLP_INTERNAL_OSTREAM_H
#define _STLP_INTERNAL_OSTREAM_H

#ifndef _STLP_INTERNAL_IOS_H
#  include <stl/_ios.h>                  // For basic_ios<>.  Includes <iosfwd>.
#endif

#ifndef _STLP_INTERNAL_OSTREAMBUF_ITERATOR_H
#  include <stl/_ostreambuf_iterator.h>
#endif

#if !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT) && !defined (_STLP_INTERNAL_EXCEPTION)
#  include <stl/_exception.h>
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_USE_TEMPLATE_EXPORT)
template <class _CharT, class _Traits>
class _Osentry;
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _CharT, class _Traits>
bool __init_bostr(basic_ostream<_CharT, _Traits>& __str);

_STLP_MOVE_TO_STD_NAMESPACE

//----------------------------------------------------------------------
// class basic_ostream<>

template <class _CharT, class _Traits>
class basic_ostream : virtual public basic_ios<_CharT, _Traits> {
  typedef basic_ostream<_CharT, _Traits> _Self;

#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1300 && _STLP_MSVC <= 1310)
  //explicitely defined as private to avoid warnings:
  basic_ostream(_Self const&);
  _Self& operator = (_Self const&);
#endif

public:                         // Types
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;
  typedef basic_ios<_CharT, _Traits> _Basic_ios;

public:                         // Constructor and destructor.
  explicit basic_ostream(basic_streambuf<_CharT, _Traits>* __buf);
  ~basic_ostream();

public:                         // Hooks for manipulators.
  typedef basic_ios<_CharT, _Traits>& (_STLP_CALL *__ios_fn)(basic_ios<_CharT, _Traits>&);
  typedef ios_base& (_STLP_CALL *__ios_base_fn)(ios_base&);
  typedef _Self& (_STLP_CALL *__ostream_fn)(_Self&);
  _Self& operator<< (__ostream_fn __f) { return __f(*this); }
  _Self & operator<< (__ios_base_fn __f) { __f(*this); return *this; }
  _Self& operator<< (__ios_fn __ff) { __ff(*this); return *this; }

private:
  bool _M_copy_buffered(basic_streambuf<_CharT, _Traits>* __from,
                        basic_streambuf<_CharT, _Traits>* __to);
  bool _M_copy_unbuffered(basic_streambuf<_CharT, _Traits>* __from,
                          basic_streambuf<_CharT, _Traits>* __to);

public:
  void _M_put_char(_CharT __c);

  void _M_put_nowiden(const _CharT* __s);
  void _M_put_widen(const char* __s);
  bool _M_put_widen_aux(const char* __s, streamsize __n);

public:                         // Unformatted output.
  _Self& put(char_type __c);
  _Self& write(const char_type* __s, streamsize __n);

public:                         // Formatted output.
  // Formatted output from a streambuf.
  _Self& operator<<(basic_streambuf<_CharT, _Traits>* __buf);
# ifndef _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER
  // this is needed for compiling with option char = unsigned
  _Self& operator<<(unsigned char __x) { _M_put_char(__x); return *this; }
# endif
  _Self& operator<<(short __x);
  _Self& operator<<(unsigned short __x);
  _Self& operator<<(int __x);
#if defined (_WIN64) || !defined (_STLP_MSVC) || (_STLP_MSVC < 1300)
  _Self& operator<<(unsigned int __x);
#else
/* We define this operator with size_t rather than unsigned int to avoid
 * 64 bits warning.
 */
  _Self& operator<<(size_t __x);
#endif
  _Self& operator<<(long __x);
  _Self& operator<<(unsigned long __x);
#ifdef _STLP_LONG_LONG
  _Self& operator<< (_STLP_LONG_LONG __x);
  _Self& operator<< (unsigned _STLP_LONG_LONG __x);
#endif
  _Self& operator<<(float __x);
  _Self& operator<<(double __x);
# ifndef _STLP_NO_LONG_DOUBLE
  _Self& operator<<(long double __x);
# endif
  _Self& operator<<(const void* __x);
# ifndef _STLP_NO_BOOL
  _Self& operator<<(bool __x);
# endif

public:                         // Buffer positioning and manipulation.
  _Self& flush() {
    if (this->rdbuf())
      if (this->rdbuf()->pubsync() == -1)
        this->setstate(ios_base::badbit);
    return *this;
  }

  pos_type tellp() {
    return this->rdbuf() && !this->fail()
      ? this->rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out)
      : pos_type(-1);
  }

  _Self& seekp(pos_type __pos) {
    if (this->rdbuf() && !this->fail()) {
      if (this->rdbuf()->pubseekpos(__pos, ios_base::out) == pos_type(-1)) {
        this->setstate(ios_base::failbit);
      }
    }
    return *this;
  }

  _Self& seekp(off_type __off, ios_base::seekdir __dir) {
    if (this->rdbuf() && !this->fail())
      this->rdbuf()->pubseekoff(__off, __dir, ios_base::out);
    return *this;
  }

#if defined (_STLP_USE_TEMPLATE_EXPORT)
  // If we are using DLL specs, we have not to use inner classes
  // end class declaration here
  typedef _Osentry<_CharT, _Traits>  sentry;
};
#  define sentry _Osentry
  template <class _CharT, class _Traits>
  class _Osentry {
    typedef _Osentry<_CharT, _Traits> _Self;
#else
    class sentry {
      typedef sentry _Self;
#endif
    private:
      basic_ostream<_CharT, _Traits>& _M_str;
      //      basic_streambuf<_CharT, _Traits>* _M_buf;
      bool _M_ok;
    public:
      explicit sentry(basic_ostream<_CharT, _Traits>& __str)
        : _M_str(__str), /* _M_buf(__str.rdbuf()), */ _M_ok(_STLP_PRIV __init_bostr(__str))
      {}

      ~sentry() {
        if (_M_str.flags() & ios_base::unitbuf)
#if !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
          if (!uncaught_exception())
#endif
            _M_str.flush();
      }

      operator bool() const { return _M_ok; }
    private:                        // Disable assignment and copy constructor.
      //Implementation is here only to avoid warning with some compilers.
      sentry(const _Self& __s) : _M_str(__s._M_str) {}
      _Self& operator=(const _Self&) { return *this; }
    };
#if defined (_STLP_USE_TEMPLATE_EXPORT)
#  undef sentry
#else
  // close basic_ostream class definition here
};
#endif

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_ostream<char, char_traits<char> >;
_STLP_EXPORT_TEMPLATE_CLASS _Osentry<char, char_traits<char> >;
#  if !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_ostream<wchar_t, char_traits<wchar_t> >;
_STLP_EXPORT_TEMPLATE_CLASS _Osentry<wchar_t, char_traits<wchar_t> >;
#  endif
#endif /* _STLP_USE_TEMPLATE_EXPORT */

_STLP_MOVE_TO_PRIV_NAMESPACE

// Helper functions for istream<>::sentry constructor.
template <class _CharT, class _Traits>
bool __init_bostr(basic_ostream<_CharT, _Traits>& __str) {
  if (__str.good()) {
    // boris : check if this is needed !
    if (!__str.rdbuf())
      __str.setstate(ios_base::badbit);
    if (__str.tie())
      __str.tie()->flush();
    return __str.good();
  }
  else
    return false;
}

template <class _CharT, class _Traits>
inline basic_streambuf<_CharT, _Traits>* _STLP_CALL
__get_ostreambuf(basic_ostream<_CharT, _Traits>& __St)
{ return __St.rdbuf(); }

_STLP_MOVE_TO_STD_NAMESPACE

// Non-member functions.
template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, _CharT __c){
  __os._M_put_char(__c);
  return __os;
}

template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, const _CharT* __s) {
  __os._M_put_nowiden(__s);
  return __os;
}

#if defined (_STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER)
// some specializations

inline basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __os, char __c) {
  __os._M_put_char(__c);
  return __os;
}

inline basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __os, signed char __c) {
  __os._M_put_char(__c);
  return __os;
}

inline basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __os, unsigned char __c) {
  __os._M_put_char(__c);
  return __os;
}

inline basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __os, const char* __s) {
  __os._M_put_nowiden(__s);
  return __os;
}

inline basic_ostream<char, char_traits<char> >& _STLP_CALL
operator<<(basic_ostream<char, char_traits<char> >& __os, const signed char* __s) {
  __os._M_put_nowiden(__REINTERPRET_CAST(const char*,__s));
  return __os;
}

inline basic_ostream<char, char_traits<char> >&
operator<<(basic_ostream<char, char_traits<char> >& __os, const unsigned char* __s) {
  __os._M_put_nowiden(__REINTERPRET_CAST(const char*,__s));
  return __os;
}

#else

// also for compilers who might use that
template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, char __c) {
  __os._M_put_char(__os.widen(__c));
  return __os;
}

template <class _Traits>
inline basic_ostream<char, _Traits>& _STLP_CALL
operator<<(basic_ostream<char, _Traits>& __os, char __c) {
  __os._M_put_char(__c);
  return __os;
}

template <class _Traits>
inline basic_ostream<char, _Traits>& _STLP_CALL
operator<<(basic_ostream<char, _Traits>& __os, signed char __c) {
  __os._M_put_char(__c);
  return __os;
}

template <class _Traits>
inline basic_ostream<char, _Traits>& _STLP_CALL
operator<<(basic_ostream<char, _Traits>& __os, unsigned char __c) {
  __os._M_put_char(__c);
  return __os;
}

template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, const char* __s) {
  __os._M_put_widen(__s);
  return __os;
}

template <class _Traits>
inline basic_ostream<char, _Traits>& _STLP_CALL
operator<<(basic_ostream<char, _Traits>& __os, const char* __s) {
  __os._M_put_nowiden(__s);
  return __os;
}

template <class _Traits>
inline basic_ostream<char, _Traits>& _STLP_CALL
operator<<(basic_ostream<char, _Traits>& __os, const signed char* __s) {
  __os._M_put_nowiden(__REINTERPRET_CAST(const char*,__s));
  return __os;
}

template <class _Traits>
inline basic_ostream<char, _Traits>&
operator<<(basic_ostream<char, _Traits>& __os, const unsigned char* __s) {
  __os._M_put_nowiden(__REINTERPRET_CAST(const char*,__s));
  return __os;
}
#endif /* _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER */

//----------------------------------------------------------------------
// basic_ostream manipulators.

template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
endl(basic_ostream<_CharT, _Traits>& __os) {
  __os.put(__os.widen('\n'));
  __os.flush();
  return __os;
}

template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
ends(basic_ostream<_CharT, _Traits>& __os) {
  __os.put(_STLP_DEFAULT_CONSTRUCTED(_CharT));
  return __os;
}

template <class _CharT, class _Traits>
inline basic_ostream<_CharT, _Traits>& _STLP_CALL
flush(basic_ostream<_CharT, _Traits>& __os) {
  __os.flush();
  return __os;
}

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_ostream.c>
#endif

#endif /* _STLP_INTERNAL_OSTREAM_H */

// Local Variables:
// mode:C++
// End:
