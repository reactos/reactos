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
#ifndef _STLP_ISTREAM_C
#define _STLP_ISTREAM_C

#ifndef _STLP_INTERNAL_ISTREAM
#  include <stl/_istream.h>
#endif

#ifndef _STLP_INTERNAL_LIMITS
#  include <stl/_limits.h>
#endif

#ifndef _STLP_INTERNAL_NUM_GET_H
#  include <stl/_num_get.h>
#endif

#if defined ( _STLP_NESTED_TYPE_PARAM_BUG )
// no wchar_t is supported for this mode
#  define __BIS_int_type__ int
#  define __BIS_pos_type__ streampos
#  define __BIS_off_type__ streamoff
#else
#  define __BIS_int_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_istream<_CharT, _Traits>::int_type
#  define __BIS_pos_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_istream<_CharT, _Traits>::pos_type
#  define __BIS_off_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_istream<_CharT, _Traits>::off_type
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// Function object structs used by some member functions.

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Traits>
struct _Is_not_wspace {
  typedef typename _Traits::char_type argument_type;
  typedef bool                        result_type;

  const ctype<argument_type>* _M_ctype;

  _Is_not_wspace(const ctype<argument_type>* __c_type) : _M_ctype(__c_type) {}
  bool operator()(argument_type __c) const
    { return !_M_ctype->is(ctype_base::space, __c); }
};

template <class _Traits>
struct _Is_wspace_null {
  typedef typename _Traits::char_type argument_type;
  typedef bool                        result_type;

  const ctype<argument_type>* _M_ctype;

  _Is_wspace_null(const ctype<argument_type>* __c_type) : _M_ctype(__c_type) {}
  bool operator()(argument_type __c) const {
    return _Traits::eq(__c, argument_type()) ||
           _M_ctype->is(ctype_base::space, __c);
  }
};

template <class _Traits>
struct _Scan_for_wspace {
  typedef typename _Traits::char_type  char_type;
  typedef char_type*                   first_argument_type;
  typedef char_type*                   second_argument_type;
  typedef char_type*                   result_type;

  const ctype<char_type>* _M_ctype;

  _Scan_for_wspace(const ctype<char_type>* __ctype) : _M_ctype(__ctype) {}
  const char_type*
  operator()(const char_type* __first, const char_type* __last) const {
    return _M_ctype->scan_is(ctype_base::space, __first, __last);
  }
};

template <class _Traits>
struct _Scan_wspace_null {
  typedef typename _Traits::char_type  char_type;
  typedef char_type*                   first_argument_type;
  typedef char_type*                   second_argument_type;
  typedef char_type*                   result_type;

  const ctype<char_type>* _M_ctype;

  _Scan_wspace_null(const ctype<char_type>* __c_type) : _M_ctype(__c_type) {}
  const char_type*
  operator()(const char_type* __first, const char_type* __last) const {
    __last = find_if(__first, __last,
                     _Eq_char_bound<_Traits>(char_type()));
    return _M_ctype->scan_is(ctype_base::space, __first, __last);
  }
};

template <class _Traits>
struct _Scan_for_not_wspace {
  typedef typename _Traits::char_type  char_type;
  typedef char_type*                   first_argument_type;
  typedef char_type*                   second_argument_type;
  typedef char_type*                   result_type;

  const ctype<char_type>* _M_ctype;

  _Scan_for_not_wspace(const ctype<char_type>* __c_type) : _M_ctype(__c_type) {}
  const char_type*
  operator()(const char_type* __first, const char_type* __last) const {
    return _M_ctype->scan_not(ctype_base::space, __first, __last);
  }
};

template <class _Traits>
struct _Scan_for_char_val {
  typedef typename _Traits::char_type char_type;
  typedef char_type*                  first_argument_type;
  typedef char_type*                  second_argument_type;
  typedef char_type*                  result_type;

  char_type _M_val;

  _Scan_for_char_val(char_type __val) : _M_val(__val) {}

  const char_type*
  operator()(const char_type* __first, const char_type* __last) const {
    return find_if(__first, __last, _Eq_char_bound<_Traits>(_M_val));
  }
};

template <class _Traits>
struct _Scan_for_int_val {
  typedef typename _Traits::char_type char_type;
  typedef typename _Traits::int_type  int_type;
  typedef char_type*                  first_argument_type;
  typedef char_type*                  second_argument_type;
  typedef char_type*                  result_type;

  int_type _M_val;

  _Scan_for_int_val(int_type __val) : _M_val(__val) {}

  const char_type*
  operator()(const char_type* __first, const char_type* __last) const {
    return find_if(__first, __last,
                   _Eq_int_bound<_Traits>(_M_val));
  }
};

// Helper function: try to push back a character to a streambuf,
// return true if the pushback succeeded.  Does not throw.

template <class _CharT, class _Traits>
bool _STLP_CALL
__pushback(basic_streambuf<_CharT, _Traits>* __buf, _CharT __c) {
  bool ret;
  _STLP_TRY {
    const typename _Traits::int_type __eof = _Traits::eof();
    ret = !_Traits::eq_int_type(__buf->sputbackc(__c), __eof);
  }
  _STLP_CATCH_ALL {
    ret = false;
  }
  return ret;
}

//----------------------------------------------------------------------
// Definitions of basic_istream<>'s noninline member functions.

// Helper function for formatted input of numbers.
template <class _CharT, class _Traits, class _Number>
ios_base::iostate _STLP_CALL
__get_num(basic_istream<_CharT, _Traits>& __that, _Number& __val) {
  typedef typename basic_istream<_CharT, _Traits>::sentry _Sentry;
  ios_base::iostate __err = 0;
  _Sentry __sentry( __that );     // Skip whitespace.
  if (__sentry) {
    typedef num_get<_CharT, istreambuf_iterator<_CharT, _Traits> > _Num_get;
    _STLP_TRY {
      // Do not remove additional parenthesis around use_facet instanciation, some compilers (VC6)
      // require it when building the library.
      (use_facet<_Num_get>(__that.getloc())).get(istreambuf_iterator<_CharT, _Traits>(__that.rdbuf()),
                                               0, __that, __err, __val);
    }
    _STLP_CATCH_ALL {
      __that._M_handle_exception(ios_base::badbit);
    }
    if (__err) __that.setstate(__err);
  }
  return __err;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (short& __val) {
  long __lval;
  _STLP_PRIV __get_num(*this, __lval);
  if ( this->fail() ) {
    return *this;
  }
  short __tmp = __STATIC_CAST(short, __lval);
  unsigned short __uval = __STATIC_CAST(unsigned short, __lval);
  // check if we lose digits
  //    if ((__val != __lval) && ((unsigned short)__val != __lval))
  if ((__tmp != __lval) && ((long)__uval != __lval))
    this->setstate(ios_base::failbit);
  else
    __val = __tmp;
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (int& __val) {
  long __lval;
  _STLP_PRIV __get_num(*this, __lval);
  if ( this->fail() ) {
    return *this;
  }
  int __tmp = __lval;
  unsigned int __uval = __lval;
  // check if we lose digits
  //    if ((__val != __lval) && ((unsigned int)__val != __lval))
  if ((__tmp != __lval) && ((long)__uval != __lval))
    this->setstate(ios_base::failbit);
  else
    __val = __tmp;
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (unsigned short& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (unsigned int& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (long& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (unsigned long& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}

#if defined (_STLP_LONG_LONG)
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (_STLP_LONG_LONG& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (unsigned _STLP_LONG_LONG& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}
#endif
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (float& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (double& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}
#if !defined (_STLP_NO_LONG_DOUBLE)
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (long double& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}
#endif
#if !defined (_STLP_NO_BOOL)
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (bool& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}
#endif

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::operator>> (void*& __val) {
  _STLP_PRIV __get_num(*this, __val);
  return *this;
}

// Unformatted input

template <class _CharT, class _Traits>
__BIS_int_type__
basic_istream<_CharT, _Traits>::peek() {
  typename _Traits::int_type __tmp = _Traits::eof();

  this->_M_gcount = 0;
  sentry __sentry(*this, _No_Skip_WS());

  if (__sentry) {
    _STLP_TRY {
      __tmp = this->rdbuf()->sgetc();
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }
    if (this->_S_eof(__tmp))
      this->setstate(ios_base::eofbit);
  }

  return __tmp;
}


template <class _CharT, class _Traits>
__BIS_int_type__
basic_istream<_CharT, _Traits>::get() {
  typename _Traits::int_type __tmp = _Traits::eof();
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    _STLP_TRY {
      __tmp = this->rdbuf()->sbumpc();
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }

    if (!this->_S_eof(__tmp))
      this->_M_gcount = 1;
  }

  if (_M_gcount == 0)
    this->setstate(ios_base::eofbit | ios_base::failbit);

  return __tmp;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::get(_CharT& __c) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    typename _Traits::int_type __tmp = _Traits::eof();
    _STLP_TRY {
      __tmp = this->rdbuf()->sbumpc();
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }

    if (!this->_S_eof(__tmp)) {
      this->_M_gcount = 1;
      __c = _Traits::to_char_type(__tmp);
    }
  }

  if (this->_M_gcount == 0)
    this->setstate(ios_base::eofbit | ios_base::failbit);

  return *this;
}


// Read characters and discard them.  The standard specifies a single
// function with two arguments, each with a default.  We instead use
// three overloded functions, because it's possible to implement the
// first two more efficiently than the fully general third version.
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::ignore() {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    int_type __c;
    _STLP_TRY {
      __c = this->rdbuf()->sbumpc();
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
      return *this;
    }

    if (!this->_S_eof(__c))
      this->_M_gcount = 1;
    else
      this->setstate(ios_base::eofbit);
  }

  return *this;
}

// Putback

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::putback(_CharT __c) {
  this->_M_gcount = 0;
  sentry __sentry(*this, _No_Skip_WS());

  if (__sentry) {
    typename _Traits::int_type __tmp = _Traits::eof();
    basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
//    if (!__buf || this->_S_eof(__buf->sputbackc(__c)))
    if (__buf) {
      _STLP_TRY {
        __tmp = __buf->sputbackc(__c);
      }
      _STLP_CATCH_ALL {
        this->_M_handle_exception(ios_base::badbit);
      }
    }
    if (this->_S_eof(__tmp))
      this->setstate(ios_base::badbit);
  }
  else
    this->setstate(ios_base::failbit);

  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>& basic_istream<_CharT, _Traits>::unget() {
  this->_M_gcount = 0;

  sentry __sentry(*this, _No_Skip_WS());

  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
    //     if (!__buf || _Traits::eq_int_type(__buf->sungetc(), _Traits::eof()))
    if (__buf) {
      _STLP_TRY {
        if (this->_S_eof(__buf->sungetc()))
          this->setstate(ios_base::badbit);
      }
      _STLP_CATCH_ALL {
        this->_M_handle_exception(ios_base::badbit);
      }
    } else
      this->setstate(ios_base::badbit);
  }
  else
    this->setstate(ios_base::failbit);

  return *this;
}

// Positioning and buffer control.

template <class _CharT, class _Traits>
int basic_istream<_CharT, _Traits>::sync() {
  sentry __sentry(*this, _No_Skip_WS());

  basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
  if (__buf) {
    if (__buf->pubsync() == -1) {
      this->setstate(ios_base::badbit);
      return -1;
    }
    else
      return 0;
  }
  else
    return -1;
}

template <class _CharT, class _Traits>
__BIS_pos_type__
basic_istream<_CharT, _Traits>::tellg() {
  sentry __sentry(*this, _No_Skip_WS());

  basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
  return (__buf && !this->fail()) ? __buf->pubseekoff(0, ios_base::cur, ios_base::in)
    : pos_type(-1);
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::seekg(pos_type __pos) {
  sentry __sentry(*this, _No_Skip_WS());

  basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
  if (!this->fail() && __buf) {
    if (__buf->pubseekpos(__pos, ios_base::in) == pos_type(-1)) {
      this->setstate(ios_base::failbit);
    }
  }
  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::seekg(off_type __off, ios_base::seekdir __dir) {
  sentry __sentry(*this, _No_Skip_WS());

  basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
  if (!this->fail() && __buf)
    __buf->pubseekoff(__off, __dir, ios_base::in);
  return *this;
}

// Formatted input of characters and character arrays.

template <class _CharT, class _Traits>
void basic_istream<_CharT, _Traits>::_M_formatted_get(_CharT& __c) {
//  typename _Traits::int_type __tmp = _Traits::eof();

  sentry __sentry(*this); // Skip whitespace.

  if (__sentry) {
    typename _Traits::int_type __tmp;// = _Traits::eof();

    _STLP_TRY {
      __tmp = this->rdbuf()->sbumpc();
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
      return;
    }

    if (!this->_S_eof(__tmp))
      __c = _Traits::to_char_type(__tmp);
    else
      this->setstate(ios_base::eofbit | ios_base::failbit);
  }
}


//---------------------------------------------------------------------------
// istream's helper functions.

// A generic function for unbuffered input.  We stop when we reach EOF,
// or when we have extracted _Num characters, or when the function object
// __is_delim return true.  In the last case, it extracts the character
// for which __is_delim is true, if and only if __extract_delim is true.
// It appends a null character to the end of the string; this means that
// it may store up to _Num + 1 characters.
//
// __is_getline governs two corner cases: reading _Num characters without
// encountering delim or eof (in which case failbit is set if __is_getline
// is true); and reading _Num characters where the _Num+1'st character is
// eof (in which case eofbit is set if __is_getline is true).
//
// It is assumed that __is_delim never throws.
//
// Return value is the number of characters extracted, including the
// delimiter if it is extracted.  Note that the number of characaters
// extracted isn't necessarily the same as the number stored.

_STLP_MOVE_TO_PRIV_NAMESPACE

template < class _CharT, class _Traits, class _Is_Delim>
streamsize _STLP_CALL
__read_unbuffered(basic_istream<_CharT, _Traits>* __that, basic_streambuf<_CharT, _Traits>* __buf,
                  streamsize _Num, _CharT* __s,
                  _Is_Delim __is_delim,
                  bool __extract_delim, bool __append_null,
                  bool __is_getline)
{
  streamsize __n = 0;
  ios_base::iostate __status = 0;

  typedef typename basic_istream<_CharT, _Traits>::int_type int_type;
  // The operations that can potentially throw are sbumpc, snextc, and sgetc.
  _STLP_TRY {
    for (;;) {
      if (__n == _Num) {
        if (__is_getline) // didn't find delimiter as one of the _Num chars
          __status |= ios_base::failbit;
        break;
      }
      int_type __c = __buf->sbumpc(); // sschwarz

      if (__that->_S_eof(__c)) {
        if (__n < _Num || __is_getline)
          __status |= ios_base::eofbit;
        break;
      } else if (__is_delim(_Traits::to_char_type(__c))) {
        if (__extract_delim) { // Extract and discard current character.
          ++__n;
        } else if ( !__pushback(__buf, _Traits::to_char_type(__c)) ) { // leave delimiter
          __status |= ios_base::failbit;
        }
        break;
      }
      // regular character
      *__s++ = _Traits::to_char_type(__c);
      ++__n;
    }
  }
  _STLP_CATCH_ALL {
    __that->_M_handle_exception(ios_base::badbit);
    *__s = _STLP_DEFAULT_CONSTRUCTED(_CharT);
    return __n;
  }

  if (__append_null)
    *__s =  _STLP_DEFAULT_CONSTRUCTED(_CharT);
  if (__status)
    __that->setstate(__status);    // This might throw.
  return __n;
}

// Much like __read_unbuffered, but with one additional function object:
// __scan_delim(first, last) returns the first pointer p in [first, last)
// such that __is_delim(p) is true.

template < class _CharT, class _Traits, class _Is_Delim, class _Scan_Delim>
streamsize _STLP_CALL
__read_buffered(basic_istream<_CharT, _Traits>* __that, basic_streambuf<_CharT, _Traits>* __buf,
                 streamsize _Num, _CharT* __s,
                 _Is_Delim __is_delim, _Scan_Delim __scan_delim,
                 bool __extract_delim, bool __append_null,
                 bool __is_getline) {
  streamsize __n = 0;
  ios_base::iostate __status = 0;
  bool __done    = false;

  _STLP_TRY {
    while (__buf->_M_egptr() != __buf->_M_gptr() && !__done) {
      const _CharT* __first = __buf->_M_gptr();
      const _CharT* __last  = __buf->_M_egptr();
      //casting numeric_limits<ptrdiff_t>::max to streamsize only works is ptrdiff_t is signed or streamsize representation
      //is larger than ptrdiff_t one.
      _STLP_STATIC_ASSERT((sizeof(streamsize) > sizeof(ptrdiff_t)) ||
                          ((sizeof(streamsize) == sizeof(ptrdiff_t)) && numeric_limits<ptrdiff_t>::is_signed))
      ptrdiff_t __request = __STATIC_CAST(ptrdiff_t, (min) (__STATIC_CAST(streamsize, (numeric_limits<ptrdiff_t>::max)()), _Num - __n));

      const _CharT* __p  = __scan_delim(__first, __last);
      ptrdiff_t __chunk = (min) (ptrdiff_t(__p - __first), __request);
      _Traits::copy(__s, __first, __chunk);
      __s += __chunk;
      __n += __chunk;
      __buf->_M_gbump((int)__chunk);

      // We terminated by finding delim.
      if (__p != __last && __p - __first <= __request) {
        if (__extract_delim) {
          __n += 1;
          __buf->_M_gbump(1);
        }
        __done = true;
      }

      // We terminated by reading all the characters we were asked for.
      else if (__n == _Num) {

        // Find out if we have reached eof.  This matters for getline.
        if (__is_getline) {
          if (__chunk == __last - __first) {
            if (__that->_S_eof(__buf->sgetc()))
              __status |= ios_base::eofbit;
          }
          else
            __status |= ios_base::failbit;
        }
        __done   = true;
      }

      // The buffer contained fewer than _Num - __n characters.  Either we're
      // at eof, or we should refill the buffer and try again.
      else {
        if (__that->_S_eof(__buf->sgetc())) {
          __status |= ios_base::eofbit;
          __done = true;
        }
      }
    } // Close the while loop.
  }
  _STLP_CATCH_ALL {
    __that->_M_handle_exception(ios_base::badbit);
    __done = true;
  }

  if (__done) {
    if (__append_null)
        *__s =  _STLP_DEFAULT_CONSTRUCTED(_CharT);
    if (__status != 0)
      __that->setstate(__status);   // This might throw.
    return __n;
  }

  // If execution has reached this point, then we have an empty buffer but
  // we have not reached eof.  What that means is that the streambuf has
  // decided to switch from buffered to unbuffered input.  We switch to
  // to __read_unbuffered.

  return __n + __read_unbuffered(__that,  __buf, _Num - __n, __s, __is_delim,
                                 __extract_delim,__append_null,__is_getline);
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::get(_CharT* __s, streamsize __n,
                                    _CharT __delim) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    if (__n > 0) {
      basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();

      if (__buf->egptr() != __buf->gptr())
        this->_M_gcount =
          _STLP_PRIV __read_buffered(this,  __buf, __n - 1, __s,
                                     _STLP_PRIV _Eq_char_bound<_Traits>(__delim),
                                     _STLP_PRIV _Scan_for_char_val<_Traits>(__delim),
                                     false, true, false);
      else
        this->_M_gcount =
          _STLP_PRIV __read_unbuffered(this,  __buf, __n - 1, __s,
                                       _STLP_PRIV _Eq_char_bound<_Traits>(__delim),
                                       false, true, false);
    }
  }

  if (this->_M_gcount == 0)
    this->setstate(ios_base::failbit);

  return *this;
}

// Getline is essentially identical to get, except that it extracts
// the delimiter.
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::getline(_CharT* __s, streamsize __n,
                                        _CharT __delim) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    if (__n > 0) {
      basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
      this->_M_gcount = __buf->egptr() != __buf->gptr()
        ? _STLP_PRIV __read_buffered(this,  __buf, __n - 1, __s,
                                     _STLP_PRIV _Eq_char_bound<_Traits>(__delim),
                                     _STLP_PRIV _Scan_for_char_val<_Traits>(__delim),
                                     true, true, true)
        : _STLP_PRIV __read_unbuffered(this,  __buf, __n - 1, __s,
                                       _STLP_PRIV _Eq_char_bound<_Traits>(__delim),
                                       true, true, true);
    }
  }

  if (this->_M_gcount == 0)
    this->setstate(ios_base::failbit);

  return *this;
}

// Read n characters.  We don't look for any delimiter, and we don't
// put in a terminating null character.
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::read(char_type* __s, streamsize __n) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry && !this->eof()) {
    basic_streambuf<_CharT, _Traits>*__buf = this->rdbuf();
    if (__buf->gptr() != __buf->egptr())
      _M_gcount
        = _STLP_PRIV __read_buffered(this,  __buf, __n, __s,
                                     _STLP_PRIV _Constant_unary_fun<bool, int_type>(false),
                                     _STLP_PRIV _Project2nd<const _CharT*, const _CharT*>(),
                                     false, false, false);
    else
      _M_gcount
        = _STLP_PRIV __read_unbuffered(this,  __buf, __n, __s,
                                       _STLP_PRIV _Constant_unary_fun<bool, int_type>(false),
                                       false, false, false);
  }
  else
    this->setstate(ios_base::failbit);

  if (this->eof())
    this->setstate(ios_base::eofbit | ios_base::failbit);

  return *this;
}


// Read n or fewer characters.  We don't look for any delimiter, and
// we don't put in a terminating null character.
template <class _CharT, class _Traits>
streamsize
basic_istream<_CharT, _Traits>::readsome(char_type* __s, streamsize __nmax) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry && !this->eof() && __nmax >= 0) {

    basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
    streamsize __avail = __buf->in_avail();

    // fbp : isn't full-blown setstate required here ?
    if (__avail == -1)
      this->_M_setstate_nothrow(ios_base::eofbit);

    else if (__avail != 0) {

      if (__buf->gptr() != __buf->egptr())
        _M_gcount
          = _STLP_PRIV __read_buffered(this,  __buf, (min) (__avail, __nmax), __s,
                                       _STLP_PRIV _Constant_unary_fun<bool, int_type>(false),
                                       _STLP_PRIV _Project2nd<const _CharT*, const _CharT*>(),
                                       false, false, false);
      else
        _M_gcount
          = _STLP_PRIV __read_unbuffered(this,  __buf, (min) (__avail, __nmax), __s,
                                         _STLP_PRIV _Constant_unary_fun<bool, int_type>(false),
                                         false, false, false);
    }
  }
  else {
    // fbp : changed so that failbit is set only there, to pass Dietmar's test
    if (this->eof())
      this->setstate(ios_base::eofbit | ios_base::failbit);
    else
      this->setstate(ios_base::failbit);
  }

  //  if (this->eof())
  //    this->setstate(ios_base::eofbit | ios_base::failbit);

  return _M_gcount;
}

template <class _CharT, class _Traits>
void basic_istream<_CharT, _Traits>::_M_formatted_get(_CharT* __s) {
  sentry __sentry(*this); // Skip whitespace.

  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
    streamsize __nmax = this->width() > 0
      ? this->width() - 1
      : ((numeric_limits<streamsize>::max)() / sizeof(_CharT)) - 1;

    streamsize __n = __buf->gptr() != __buf->egptr()
      ? _STLP_PRIV __read_buffered(this,  __buf, __nmax, __s,
                                   _STLP_PRIV _Is_wspace_null<_Traits>(this->_M_ctype_facet()),
                                   _STLP_PRIV _Scan_wspace_null<_Traits>(this->_M_ctype_facet()),
                                   false, true, false)
      : _STLP_PRIV __read_unbuffered(this,  __buf, __nmax, __s,
                                     _STLP_PRIV _Is_wspace_null<_Traits>(this->_M_ctype_facet()),
                                     false, true, false);
    if (__n == 0)
      this->setstate(ios_base::failbit);
  }
  this->width(0);
}

// A generic unbuffered function for ignoring characters.  We stop
// when we reach EOF, or when the function object __is_delim returns
// true.  In the last case, it extracts the character for which
// __is_delim is true, if and only if __extract_delim is true.

template < class _CharT, class _Traits, class _Is_Delim>
void _STLP_CALL
_M_ignore_unbuffered(basic_istream<_CharT, _Traits>* __that,
                     basic_streambuf<_CharT, _Traits>* __buf,
                     _Is_Delim __is_delim,
                     bool __extract_delim, bool __set_failbit) {
  bool __done = false;
  ios_base::iostate __status = 0;
  typedef typename basic_istream<_CharT, _Traits>::int_type int_type;

  _STLP_TRY {
    while (!__done) {
      int_type __c = __buf->sbumpc();

      if (__that->_S_eof(__c)) {
        __done = true;
        __status |= __set_failbit ? ios_base::eofbit | ios_base::failbit
                                  : ios_base::eofbit;
      }

      else if (__is_delim(_Traits::to_char_type(__c))) {
        __done = true;
        if (!__extract_delim)
          if (__that->_S_eof(__buf->sputbackc(_Traits::to_char_type(__c))))
            __status |= ios_base::failbit;
      }
    }
  }
  _STLP_CATCH_ALL {
    __that->_M_handle_exception(ios_base::badbit);
  }

  __that->setstate(__status);
}

// A generic buffered function for ignoring characters.  Much like
// _M_ignore_unbuffered, but with one additional function object:
// __scan_delim(first, last) returns the first pointer p in [first,
// last) such that __is_delim(p) is true.

template < class _CharT, class _Traits, class _Is_Delim, class _Scan_Delim>
void _STLP_CALL
_M_ignore_buffered(basic_istream<_CharT, _Traits>* __that,
                   basic_streambuf<_CharT, _Traits>* __buf,
                   _Is_Delim __is_delim, _Scan_Delim __scan_delim,
                   bool __extract_delim, bool __set_failbit) {
  bool __at_eof      = false;
  bool __found_delim = false;

  _STLP_TRY {
    while (__buf->_M_egptr() != __buf->_M_gptr() && !__at_eof && !__found_delim) {
      const _CharT* __p = __scan_delim(__buf->_M_gptr(), __buf->_M_egptr());
      __buf->_M_gbump((int)(__p - __buf->_M_gptr()));

      if (__p != __buf->_M_egptr()) { // We found delim, so we're done.
        if (__extract_delim)
          __buf->_M_gbump(1);
        __found_delim = true;
      }

      else                         // No delim.  Try to refil the buffer.
        __at_eof = __that->_S_eof(__buf->sgetc());
    }                              // Close the while loop.
  }
  _STLP_CATCH_ALL {
    __that->_M_handle_exception(ios_base::badbit);
    return;
  }

  if (__at_eof) {
    __that->setstate(__set_failbit ? ios_base::eofbit | ios_base::failbit
                                   : ios_base::eofbit);
    return;
  }
  if (__found_delim)
    return;

  // If execution has reached this point, then we have an empty buffer but
  // we have not reached eof.  What that means is that the streambuf has
  // decided to switch from a buffered to an unbuffered mode.  We switch
  // to _M_ignore_unbuffered.
  _M_ignore_unbuffered(__that,  __buf, __is_delim, __extract_delim, __set_failbit);
}

// Overloaded versions of _M_ignore_unbuffered and _M_ignore_unbuffered
// with an explicit count _Num.  Return value is the number of
// characters extracted.
//
// The function object __max_chars takes two arguments, _Num and __n
// (the latter being the number of characters we have already read),
// and returns the maximum number of characters to read from the buffer.
// We parameterize _M_ignore_buffered so that we can use it for both
// bounded and unbounded input; for the former the function object should
// be minus<>, and for the latter it should return a constant maximum value.

template < class _CharT, class _Traits, class _Max_Chars, class _Is_Delim>
streamsize _STLP_CALL
_M_ignore_unbuffered(basic_istream<_CharT, _Traits>* __that,
                     basic_streambuf<_CharT, _Traits>* __buf,
                     streamsize _Num, _Max_Chars __max_chars,
                     _Is_Delim __is_delim,
                     bool __extract_delim, bool __set_failbit) {
  streamsize __n = 0;
  ios_base::iostate __status = 0;
  typedef typename basic_istream<_CharT, _Traits>::int_type int_type;

  _STLP_TRY {
    while (__max_chars(_Num, __n) > 0) {
      int_type __c = __buf->sbumpc();

      if (__that->_S_eof(__c)) {
        __status |= __set_failbit ? ios_base::eofbit | ios_base::failbit
                                  : ios_base::eofbit;
        break;
      }

      else if (__is_delim(_Traits::to_char_type(__c))) {
        if (__extract_delim)
          ++__n;
        else if (__that->_S_eof(__buf->sputbackc(_Traits::to_char_type(__c))))
          __status |= ios_base::failbit;

        break;
      }
      // fbp : added counter increment to pass Dietmar's test
      ++__n;
    }
  }
  _STLP_CATCH_ALL {
    __that->_M_handle_exception(ios_base::badbit);
  }

  if (__status)
    __that->setstate(__status);   // This might throw.
  return __n;
}

template < class _CharT, class _Traits, class _Max_Chars, class _Is_Delim, class _Scan_Delim>
streamsize _STLP_CALL
_M_ignore_buffered(basic_istream<_CharT, _Traits>* __that,
                   basic_streambuf<_CharT, _Traits>* __buf,
                   streamsize _Num,
                   _Max_Chars __max_chars,
                   _Is_Delim __is_delim, _Scan_Delim __scan_delim,
                   bool __extract_delim, bool __set_failbit) {
  streamsize __n = 0;
  bool __at_eof = false;
  bool __done   = false;

  _STLP_TRY {
    while (__buf->_M_egptr() != __buf->_M_gptr() && !__done) {
      ptrdiff_t __avail = __buf->_M_egptr() - __buf->_M_gptr();
      streamsize __m = __max_chars(_Num, __n);

      if (__avail >= __m) {       // We have more characters than we need.
        const _CharT* __last = __buf->_M_gptr() + __STATIC_CAST(ptrdiff_t, __m);
        const _CharT* __p = __scan_delim(__buf->_M_gptr(), __last);
        ptrdiff_t __chunk = __p - __buf->_M_gptr();
        __n += __chunk;
        __buf->_M_gbump((int)__chunk);

        if (__extract_delim && __p != __last) {
          __n += 1;
          __buf->_M_gbump(1);
        }

        __done = true;
      }

      else {
        const _CharT* __p = __scan_delim(__buf->_M_gptr(), __buf->_M_egptr());
        ptrdiff_t __chunk = __p - __buf->_M_gptr();
        __n += __chunk;
        __buf->_M_gbump((int)__chunk);

        if (__p != __buf->_M_egptr()) { // We found delim.
          if (__extract_delim) {
            __n += 1;
            __buf->_M_gbump(1);
          }

          __done = true;
        }

        // We didn't find delim.  Try to refill the buffer.
        else if (__that->_S_eof(__buf->sgetc())) {
          __done   = true;
          __at_eof = true;
        }
      }
    } // Close the while loop.
  }
  _STLP_CATCH_ALL {
    __that->_M_handle_exception(ios_base::badbit);
    return __n;
  }

  if (__at_eof)
    __that->setstate(__set_failbit ? ios_base::eofbit | ios_base::failbit
                                   : ios_base::eofbit);

  if (__done)
    return __n;

  // If execution has reached this point, then we have an empty buffer but
  // we have not reached eof.  What that means is that the streambuf has
  // decided to switch from buffered to unbuffered input.  We switch to
  // to _M_ignore_unbuffered.

  return __n + _M_ignore_unbuffered(__that,  __buf, _Num, __max_chars,
                                    __is_delim, __extract_delim, __set_failbit);
}


template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::ignore(streamsize __n) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
    typedef _STLP_PRIV _Constant_unary_fun<bool, int_type> _Const_bool;
    typedef _STLP_PRIV _Constant_binary_fun<streamsize, streamsize, streamsize> _Const_streamsize;
    const streamsize __maxss = (numeric_limits<streamsize>::max)();

    if (__n == (numeric_limits<int>::max)()) {
      if (__buf->gptr() != __buf->egptr())
        _M_gcount = _M_ignore_buffered(this,  __buf,
                                       __maxss, _Const_streamsize(__maxss),
                                       _Const_bool(false),
                                       _STLP_PRIV _Project2nd<const _CharT*, const _CharT*>(),
                                       false, false);
      else
        _M_gcount = _M_ignore_unbuffered(this,  __buf,
                                         __maxss, _Const_streamsize(__maxss),
                                         _Const_bool(false), false, false);
    }
    else {
      if (__buf->gptr() != __buf->egptr())
        _M_gcount = _M_ignore_buffered(this,  __buf,
                                       __n, minus<streamsize>(),
                                       _Const_bool(false),
                                       _STLP_PRIV _Project2nd<const _CharT*, const _CharT*>(),
                                       false, false);
      else
        _M_gcount = _M_ignore_unbuffered(this,  __buf, __n, minus<streamsize>(),
                                         _Const_bool(false), false, false);
    }
  }

  return *this;
}

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>::ignore(streamsize __n, int_type __delim) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
    typedef _STLP_PRIV _Constant_unary_fun<bool, int_type> _Const_bool;
    typedef _STLP_PRIV _Constant_binary_fun<streamsize, streamsize, streamsize>
      _Const_streamsize;
    const streamsize __maxss = (numeric_limits<streamsize>::max)();

    if (__n == (numeric_limits<int>::max)()) {
      if (__buf->gptr() != __buf->egptr())
        _M_gcount = _M_ignore_buffered(this,  __buf,
                                       __maxss, _Const_streamsize(__maxss),
                                       _STLP_PRIV _Eq_int_bound<_Traits>(__delim),
                                       _STLP_PRIV _Scan_for_int_val<_Traits>(__delim),
                                       true, false);
      else
        _M_gcount = _M_ignore_unbuffered(this,  __buf,
                                         __maxss, _Const_streamsize(__maxss),
                                         _STLP_PRIV _Eq_int_bound<_Traits>(__delim),
                                         true, false);
    }
    else {
      if (__buf->gptr() != __buf->egptr())
        _M_gcount = _M_ignore_buffered(this,  __buf,
                                       __n, minus<streamsize>(),
                                       _STLP_PRIV _Eq_int_bound<_Traits>(__delim),
                                       _STLP_PRIV _Scan_for_int_val<_Traits>(__delim),
                                       true, false);
      else
        _M_gcount = _M_ignore_unbuffered(this,  __buf, __n, minus<streamsize>(),
                                         _STLP_PRIV _Eq_int_bound<_Traits>(__delim),
                                         true, false);
    }
  }

  return *this;
}

// This member function does not construct a sentry object, because
// it is called from sentry's constructor.
template <class _CharT, class _Traits>
void basic_istream<_CharT, _Traits>::_M_skip_whitespace(bool __set_failbit) {
  basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();
  if (!__buf)
    this->setstate(ios_base::badbit);
  else if (__buf->gptr() != __buf->egptr())
    _M_ignore_buffered(this,  __buf,
                       _STLP_PRIV _Is_not_wspace<_Traits>(this->_M_ctype_facet()),
                       _STLP_PRIV _Scan_for_not_wspace<_Traits>(this->_M_ctype_facet()),
                       false, __set_failbit);
  else
    _M_ignore_unbuffered(this,  __buf,
                         _STLP_PRIV _Is_not_wspace<_Traits>(this->_M_ctype_facet()),
                         false, __set_failbit);
}


// This is a very simple loop that reads characters from __src and puts
// them into __dest.  It looks complicated because of the (standard-
// mandated) exception handling policy.
//
// We stop when we get an exception, when we fail to insert into the
// output streambuf, or when __is_delim is true.

_STLP_MOVE_TO_PRIV_NAMESPACE

template < class _CharT, class _Traits, class _Is_Delim>
streamsize _STLP_CALL
__copy_unbuffered(basic_istream<_CharT, _Traits>* __that, basic_streambuf<_CharT, _Traits>* __src,
                  basic_streambuf<_CharT, _Traits>* __dest,
                  _Is_Delim __is_delim,
                  bool __extract_delim, bool __rethrow) {
  streamsize __extracted = 0;
  ios_base::iostate __status = 0;
  typedef typename basic_istream<_CharT, _Traits>::int_type int_type;
  int_type __c;

  _STLP_TRY {
    for (;;) {
      // Get a character. If there's an exception, catch and (maybe) rethrow it.
      __c = __src->sbumpc();

      // If we failed to get a character, then quit.
      if (__that->_S_eof(__c)) {
        __status |= ios_base::eofbit;
        break;
      }
      // If it's the delimiter, then quit.
      else if (__is_delim(_Traits::to_char_type(__c))) {
        if (!__extract_delim && !__pushback(__src, _Traits::to_char_type(__c)))
          __status |= ios_base::failbit;
        break;
      }
      else {
        // Try to put the character in the output streambuf.
        bool __failed = false;
        _STLP_TRY {
          if (!__that->_S_eof(__dest->sputc(_Traits::to_char_type(__c))))
            ++__extracted;
          else
            __failed = true;
        }
        _STLP_CATCH_ALL {
          __failed = true;
        }

        // If we failed to put the character in the output streambuf, then
        // try to push it back to the input streambuf.
        if (__failed && !__pushback(__src, _Traits::to_char_type(__c)))
          __status |= ios_base::failbit;

        // fbp : avoiding infinite loop in io-27-6-1-2-3.exp
        if (__failed)
          break;
      }

    } /* for (;;) */

  }
  // fbp : this try/catch moved here in reasonable assumption
  // __is_delim never throw (__pushback is guaranteed not to)
  _STLP_CATCH_ALL {
    // See 27.6.1.2.3, paragraph 13.
    if (__rethrow && __extracted == 0)
      __that->_M_handle_exception(ios_base::failbit);
  }
  __that->setstate(__status);
  return __extracted;
}

// Buffered copying from one streambuf to another.  We copy the characters
// in chunks, rather than one at a time.  We still have to worry about all
// of the error conditions we checked in __copy_unbuffered, plus one more:
// the streambuf might decide to switch from a buffered to an unbuffered mode.

template < class _CharT, class _Traits, class _Is_Delim, class _Scan_Delim>
streamsize _STLP_CALL
__copy_buffered(basic_istream<_CharT, _Traits>* __that, basic_streambuf<_CharT, _Traits>* __src,
                basic_streambuf<_CharT, _Traits>* __dest,
                _Scan_Delim __scan_delim, _Is_Delim __is_delim,
                bool __extract_delim, bool __rethrow) {
  streamsize __extracted = 0;
  ios_base::iostate __status = 0;
  typedef typename basic_istream<_CharT, _Traits>::int_type int_type;
  //Borland compiler generates a warning if assignment because value is never used:
  int_type __c /*= _Traits::eof()*/;
  _CharT* __first = __src->_M_gptr();
  ptrdiff_t __avail = __src->_M_egptr() - __first;
  // fbp : introduced to move catch/try blocks out of the loop
  bool __do_handle_exceptions = false;

  _STLP_TRY {
    for (;;) {
      const _CharT* __last = __scan_delim(__first, __src->_M_egptr());

      // Try to copy the entire input buffer to the output buffer.
      streamsize __n = __dest->sputn(__first, __extract_delim && __last != __src->_M_egptr()
                                     ? (__last - __first) + 1
                                     : (__last - __first));
      __src->_M_gbump((int)__n);
      __extracted += __n;

      // from this on, catch() will call _M_handle_exceptions()
      __do_handle_exceptions = true;

      if (__n < __avail)          // We found the delimiter, or else failed to
        break;                    // copy some characters.

      __c = __src->sgetc();

      // Three possibilities: we succeeded in refilling the buffer, or
      // we got EOF, or the streambuf has switched to unbuffered mode.
      __first = __src->_M_gptr();
      __avail = __src->_M_egptr() - __first;

      if (__avail > 0)
        {}  // dwa 1/16/00 -- suppress a Metrowerks warning
      else if (__that->_S_eof(__c)) {
        __status |= ios_base::eofbit;
        break;
      }
      else {
        return __extracted + __copy_unbuffered(__that,  __src, __dest, __is_delim,
                                                __extract_delim, __rethrow);
      }

      __do_handle_exceptions = false;
    }
  }

  _STLP_CATCH_ALL {
    // See 27.6.1.2.3, paragraph 13.
    if (__rethrow && __do_handle_exceptions &&  __extracted == 0)
      __that->_M_handle_exception(ios_base::failbit);
  }

  if (__status)
    __that->setstate(__status);   // This might throw.
  return __extracted;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>
  ::get(basic_streambuf<_CharT, _Traits>& __dest, _CharT __delim) {
  sentry __sentry(*this, _No_Skip_WS());
  this->_M_gcount = 0;

  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __src = this->rdbuf();

    if (__src)
      this->_M_gcount = __src->egptr() != __src->gptr()
        ? _STLP_PRIV __copy_buffered(this,  __src, &__dest,
                                     _STLP_PRIV _Scan_for_char_val<_Traits>(__delim),
                                     _STLP_PRIV _Eq_char_bound<_Traits>(__delim),
                                     false, false)
        : _STLP_PRIV __copy_unbuffered(this,  __src, &__dest,
                                       _STLP_PRIV _Eq_char_bound<_Traits>(__delim),
                                       false, false);
  }

  if (this->_M_gcount == 0)
    this->setstate(ios_base::failbit);

  return *this;
}

// Copying characters into a streambuf.
template <class _CharT, class _Traits>
basic_istream<_CharT, _Traits>&
basic_istream<_CharT, _Traits>
  ::operator>>(basic_streambuf<_CharT, _Traits>* __dest) {
  streamsize __n = 0;
  typedef typename basic_istream<_CharT, _Traits>::sentry _Sentry;
  _Sentry __sentry(*this);
  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __src = this->rdbuf();
    if (__src && __dest)
      __n = __src->egptr() != __src->gptr()
        ? _STLP_PRIV __copy_buffered(this,  __src, __dest,
                                     _STLP_PRIV _Project2nd<const _CharT*, const _CharT*>(),
                                     _STLP_PRIV _Constant_unary_fun<bool, int_type>(false),
                                     false, true)
        : _STLP_PRIV __copy_unbuffered(this,  __src, __dest,
                                       _STLP_PRIV _Constant_unary_fun<bool, int_type>(false),
                                       false, true);
  }

  if (__n == 0)
    this->setstate(ios_base::failbit);

  return *this;
}

// ----------------------------------------------------------------
// basic_iostream<> class
// ----------------------------------------------------------------

template <class _CharT, class _Traits>
basic_iostream<_CharT, _Traits>
  ::basic_iostream(basic_streambuf<_CharT, _Traits>* __buf)
    : basic_ios<_CharT, _Traits>(),
      basic_istream<_CharT, _Traits>(__buf),
      basic_ostream<_CharT, _Traits>(__buf) {
  this->init(__buf);
}

template <class _CharT, class _Traits>
basic_iostream<_CharT, _Traits>::~basic_iostream()
{}

_STLP_END_NAMESPACE

#undef __BIS_int_type__
#undef __BIS_pos_type__
#undef __BIS_off_type__

#endif /* _STLP_ISTREAM_C */

// Local Variables:
// mode:C++
// End:
