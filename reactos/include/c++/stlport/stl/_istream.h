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
#ifndef _STLP_INTERNAL_ISTREAM
#define _STLP_INTERNAL_ISTREAM

// this block is included by _ostream.h, we include it here to lower #include level
#if defined (_STLP_HAS_WCHAR_T) && !defined (_STLP_INTERNAL_CWCHAR)
#  include <stl/_cwchar.h>
#endif

#ifndef _STLP_INTERNAL_IOS_H
#  include <stl/_ios.h>                  // For basic_ios<>.  Includes <iosfwd>.
#endif

#ifndef _STLP_INTERNAL_OSTREAM_H
#  include <stl/_ostream.h>              // Needed as a base class of basic_iostream.
#endif

#ifndef _STLP_INTERNAL_ISTREAMBUF_ITERATOR_H
#  include <stl/_istreambuf_iterator.h>
#endif

#include <stl/_ctraits_fns.h>    // Helper functions that allow char traits
                                // to be used as function objects.
_STLP_BEGIN_NAMESPACE

#if defined (_STLP_USE_TEMPLATE_EXPORT)
template <class _CharT, class _Traits>
class _Isentry;
#endif

struct _No_Skip_WS {};        // Dummy class used by sentry.

template <class _CharT, class _Traits>
bool _M_init_skip(basic_istream<_CharT, _Traits>& __istr);
template <class _CharT, class _Traits>
bool _M_init_noskip(basic_istream<_CharT, _Traits>& __istr);

//----------------------------------------------------------------------
// Class basic_istream, a class that performs formatted input through
// a stream buffer.

// The second template parameter, _Traits, defaults to char_traits<_CharT>.
// The default is declared in header <iosfwd>, and it isn't declared here
// because C++ language rules do not allow it to be declared twice.

template <class _CharT, class _Traits>
class basic_istream : virtual public basic_ios<_CharT, _Traits> {
  typedef basic_istream<_CharT, _Traits> _Self;

#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1300 && _STLP_MSVC <= 1310)
  //explicitely defined as private to avoid warnings:
  basic_istream(_Self const&);
  _Self& operator = (_Self const&);
#endif

public:
                         // Types
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;
  typedef basic_ios<_CharT, _Traits>     _Basic_ios;

  typedef basic_ios<_CharT, _Traits>& (_STLP_CALL *__ios_fn)(basic_ios<_CharT, _Traits>&);
  typedef ios_base& (_STLP_CALL *__ios_base_fn)(ios_base&);
  typedef _Self& (_STLP_CALL *__istream_fn)(_Self&);

public:                         // Constructor and destructor.
  explicit basic_istream(basic_streambuf<_CharT, _Traits>* __buf) :
    basic_ios<_CharT, _Traits>(), _M_gcount(0) {
    this->init(__buf);
  }
  ~basic_istream() {};

public:                         // Nested sentry class.

public:                         // Hooks for manipulators.  The arguments are
                                // function pointers.
  _Self& operator>> (__istream_fn __f) { return __f(*this); }
  _Self& operator>> (__ios_fn __f) {  __f(*this); return *this; }
  _Self& operator>> (__ios_base_fn __f) { __f(*this); return *this; }

public:                         // Formatted input of numbers.
  _Self& operator>> (short& __val);
  _Self& operator>> (int& __val);
  _Self& operator>> (unsigned short& __val);
  _Self& operator>> (unsigned int& __val);
  _Self& operator>> (long& __val);
  _Self& operator>> (unsigned long& __val);
#ifdef _STLP_LONG_LONG
  _Self& operator>> (_STLP_LONG_LONG& __val);
  _Self& operator>> (unsigned _STLP_LONG_LONG& __val);
#endif
  _Self& operator>> (float& __val);
  _Self& operator>> (double& __val);
# ifndef _STLP_NO_LONG_DOUBLE
  _Self& operator>> (long double& __val);
# endif
# ifndef _STLP_NO_BOOL
  _Self& operator>> (bool& __val);
# endif
  _Self& operator>> (void*& __val);

public:                         // Copying characters into a streambuf.
  _Self& operator>>(basic_streambuf<_CharT, _Traits>*);

public:                         // Unformatted input.
  streamsize gcount() const { return _M_gcount; }
  int_type peek();

public:                         // get() for single characters
  int_type get();
  _Self& get(char_type& __c);

public:                         // get() for character arrays.
  _Self& get(char_type* __s, streamsize __n, char_type __delim);
  _Self& get(char_type* __s, streamsize __n)
    { return get(__s, __n, this->widen('\n')); }

public:                         // get() for streambufs
  _Self& get(basic_streambuf<_CharT, _Traits>& __buf,
                     char_type __delim);
  _Self& get(basic_streambuf<_CharT, _Traits>& __buf)
    { return get(__buf, this->widen('\n')); }

public:                         // getline()
  _Self& getline(char_type* __s, streamsize __n, char_type delim);
  _Self& getline(char_type* __s, streamsize __n)
    { return getline(__s, __n, this->widen('\n')); }

public:                         // read(), readsome(), ignore()
  _Self& ignore();
  _Self& ignore(streamsize __n);
  _Self& ignore(streamsize __n, int_type __delim);

  _Self& read(char_type* __s, streamsize __n);
  streamsize readsome(char_type* __s, streamsize __n);

public:                         // putback
  _Self& putback(char_type __c);
  _Self& unget();

public:                         // Positioning and buffer control.
  int sync();

  pos_type tellg();
  _Self& seekg(pos_type __pos);
  _Self& seekg(off_type, ios_base::seekdir);

public:                         // Helper functions for non-member extractors.
  void _M_formatted_get(_CharT& __c);
  void _M_formatted_get(_CharT* __s);
  void _M_skip_whitespace(bool __set_failbit);

private:                        // Number of characters extracted by the
  streamsize _M_gcount;         // most recent unformatted input function.

public:

#if defined (_STLP_USE_TEMPLATE_EXPORT)
  // If we are using DLL specs, we have not to use inner classes
  // end class declaration here
  typedef _Isentry<_CharT, _Traits>      sentry;
};
#  define sentry _Isentry
template <class _CharT, class _Traits>
class _Isentry {
  typedef _Isentry<_CharT, _Traits> _Self;
# else
  class sentry {
    typedef sentry _Self;
#endif

  private:
    const bool _M_ok;
    //    basic_streambuf<_CharT, _Traits>* _M_buf;

  public:
    typedef _Traits traits_type;

    explicit sentry(basic_istream<_CharT, _Traits>& __istr,
                    bool __noskipws = false) :
      _M_ok((__noskipws || !(__istr.flags() & ios_base::skipws)) ? _M_init_noskip(__istr) : _M_init_skip(__istr) )
      /* , _M_buf(__istr.rdbuf()) */
      {}

    // Calling this constructor is the same as calling the previous one with
    // __noskipws = true, except that it doesn't require a runtime test.
    sentry(basic_istream<_CharT, _Traits>& __istr, _No_Skip_WS) : /* _M_buf(__istr.rdbuf()), */
      _M_ok(_M_init_noskip(__istr)) {}

    ~sentry() {}

    operator bool() const { return _M_ok; }

  private:                        // Disable assignment and copy constructor.
    //Implementation is here only to avoid warning with some compilers.
    sentry(const _Self&) : _M_ok(false) {}
    _Self& operator=(const _Self&) { return *this; }
  };

# if defined (_STLP_USE_TEMPLATE_EXPORT)
#  undef sentry
# else
  // close basic_istream class definition here
};
# endif

# if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS _Isentry<char, char_traits<char> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_istream<char, char_traits<char> >;
#  if ! defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS _Isentry<wchar_t, char_traits<wchar_t> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_istream<wchar_t, char_traits<wchar_t> >;
#  endif
# endif /* _STLP_USE_TEMPLATE_EXPORT */

// Non-member character and string extractor functions.
template <class _CharT, class _Traits>
inline basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __in_str, _CharT& __c) {
  __in_str._M_formatted_get(__c);
  return __in_str;
}

template <class _Traits>
inline basic_istream<char, _Traits>& _STLP_CALL
operator>>(basic_istream<char, _Traits>& __in_str, unsigned char& __c) {
  __in_str._M_formatted_get(__REINTERPRET_CAST(char&,__c));
  return __in_str;
}

template <class _Traits>
inline basic_istream<char, _Traits>& _STLP_CALL
operator>>(basic_istream<char, _Traits>& __in_str, signed char& __c) {
  __in_str._M_formatted_get(__REINTERPRET_CAST(char&,__c));
  return __in_str;
}

template <class _CharT, class _Traits>
inline basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __in_str, _CharT* __s) {
  __in_str._M_formatted_get(__s);
  return __in_str;
}

template <class _Traits>
inline basic_istream<char, _Traits>& _STLP_CALL
operator>>(basic_istream<char, _Traits>& __in_str, unsigned char* __s) {
  __in_str._M_formatted_get(__REINTERPRET_CAST(char*,__s));
  return __in_str;
}

template <class _Traits>
inline basic_istream<char, _Traits>& _STLP_CALL
operator>>(basic_istream<char, _Traits>& __in_str, signed char* __s) {
  __in_str._M_formatted_get(__REINTERPRET_CAST(char*,__s));
  return __in_str;
}

//----------------------------------------------------------------------
// istream manipulator.
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& _STLP_CALL
ws(basic_istream<_CharT, _Traits>& __istr) {
  if (!__istr.eof()) {
    typedef typename basic_istream<_CharT, _Traits>::sentry      _Sentry;
    _Sentry __sentry(__istr, _No_Skip_WS()); // Don't skip whitespace.
    if (__sentry)
      __istr._M_skip_whitespace(false);
  }
  return __istr;
}

// Helper functions for istream<>::sentry constructor.
template <class _CharT, class _Traits>
inline bool _M_init_skip(basic_istream<_CharT, _Traits>& __istr) {
  if (__istr.good()) {
    if (__istr.tie())
      __istr.tie()->flush();

    __istr._M_skip_whitespace(true);
  }

  if (!__istr.good()) {
    __istr.setstate(ios_base::failbit);
    return false;
  } else
    return true;
}

template <class _CharT, class _Traits>
inline bool _M_init_noskip(basic_istream<_CharT, _Traits>& __istr) {
  if (__istr.good()) {
    if (__istr.tie())
      __istr.tie()->flush();

    if (!__istr.rdbuf())
      __istr.setstate(ios_base::badbit);
  }
  else
    __istr.setstate(ios_base::failbit);
  return __istr.good();
}

//----------------------------------------------------------------------
// Class iostream.
template <class _CharT, class _Traits>
class basic_iostream
  : public basic_istream<_CharT, _Traits>,
    public basic_ostream<_CharT, _Traits>
{
public:
  typedef basic_ios<_CharT, _Traits> _Basic_ios;

  explicit basic_iostream(basic_streambuf<_CharT, _Traits>* __buf);
  virtual ~basic_iostream();
};

# if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_iostream<char, char_traits<char> >;

#  if ! defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_iostream<wchar_t, char_traits<wchar_t> >;
#  endif
# endif /* _STLP_USE_TEMPLATE_EXPORT */

template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>* _STLP_CALL _M_get_istreambuf(basic_istream<_CharT, _Traits>& __istr)
{ return __istr.rdbuf(); }

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_istream.c>
#endif

#endif /* _STLP_INTERNAL_ISTREAM */

// Local Variables:
// mode:C++
// End:
