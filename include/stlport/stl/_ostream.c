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
#ifndef _STLP_OSTREAM_C
#define _STLP_OSTREAM_C

#ifndef _STLP_INTERNAL_OSTREAM_H
#  include <stl/_ostream.h>
#endif

#if !defined (_STLP_INTERNAL_NUM_PUT_H)
#  include <stl/_num_put.h>            // For basic_streambuf and iterators
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// Definitions of non-inline member functions.

// Constructor, destructor

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>::basic_ostream(basic_streambuf<_CharT, _Traits>* __buf)
    : basic_ios<_CharT, _Traits>() {
  this->init(__buf);
}

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>::~basic_ostream()
{}

// Output directly from a streambuf.
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>&
basic_ostream<_CharT, _Traits>::operator<<(basic_streambuf<_CharT, _Traits>* __from) {
  sentry __sentry(*this);
  if (__sentry) {
    if (__from) {
      bool __any_inserted = __from->gptr() != __from->egptr()
        ? this->_M_copy_buffered(__from, this->rdbuf())
        : this->_M_copy_unbuffered(__from, this->rdbuf());
      if (!__any_inserted)
        this->setstate(ios_base::failbit);
    }
    else
      this->setstate(ios_base::badbit);
  }

  return *this;
}

// Helper functions for the streambuf version of operator<<.  The
// exception-handling code is complicated because exceptions thrown
// while extracting characters are treated differently than exceptions
// thrown while inserting characters.

template <class _CharT, class _Traits>
bool basic_ostream<_CharT, _Traits>
  ::_M_copy_buffered(basic_streambuf<_CharT, _Traits>* __from,
                     basic_streambuf<_CharT, _Traits>* __to) {
  bool __any_inserted = false;

  while (__from->egptr() != __from->gptr()) {
    const ptrdiff_t __avail = __from->egptr() - __from->gptr();

    streamsize __nwritten;
    _STLP_TRY {
      __nwritten = __to->sputn(__from->gptr(), __avail);
      __from->gbump((int)__nwritten);
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
      return __any_inserted;
    }

    if (__nwritten == __avail) {
      _STLP_TRY {
        if (this->_S_eof(__from->sgetc()))
          return true;
        else
          __any_inserted = true;
      }
      _STLP_CATCH_ALL {
        this->_M_handle_exception(ios_base::failbit);
        return false;
      }
    }
    else if (__nwritten != 0)
      return true;
    else
      return __any_inserted;
  }

  // No characters are in the buffer, but we aren't at EOF.  Switch to
  // unbuffered mode.
  return __any_inserted || this->_M_copy_unbuffered(__from, __to);
}

/*
 * Helper struct (guard) to put back a character in a streambuf
 * whenever an exception or an eof occur.
 */
template <class _CharT, class _Traits>
struct _SPutBackC {
  typedef basic_streambuf<_CharT, _Traits> _StreamBuf;
  typedef typename _StreamBuf::int_type int_type;
  _SPutBackC(_StreamBuf *pfrom)
    : __pfrom(pfrom), __c(0), __do_guard(false) {}
  ~_SPutBackC() {
    if (__do_guard) {
      __pfrom->sputbackc(_Traits::to_char_type(__c));
    }
  }

  void guard(int_type c) {
    __c = c;
    __do_guard = true;
  }
  void release() {
    __do_guard = false;
  }

private:
  _StreamBuf *__pfrom;
  int_type __c;
  bool __do_guard;
};

template <class _CharT, class _Traits>
bool basic_ostream<_CharT, _Traits>
  ::_M_copy_unbuffered(basic_streambuf<_CharT, _Traits>* __from,
                       basic_streambuf<_CharT, _Traits>* __to) {
  typedef _SPutBackC<_CharT, _Traits> _SPutBackCGuard;
  bool __any_inserted = false;
  int_type __c;

  _STLP_TRY {
    _SPutBackCGuard __cguard(__from);
    for (;;) {
      _STLP_TRY {
        __c = __from->sbumpc();
      }
      _STLP_CATCH_ALL {
        this->_M_handle_exception(ios_base::failbit);
        break;
      }

      if (this->_S_eof(__c))
        break;

      __cguard.guard(__c);
#if defined (__DMC__)
      _STLP_TRY {
#endif
      if (this->_S_eof(__to->sputc(_Traits::to_char_type(__c))))
        break;

#if defined (__DMC__)
      }
      _STLP_CATCH_ALL {
        this->_M_handle_exception(ios_base::badbit);
        break;
      }
#endif
      __cguard.release();
      __any_inserted = true;
    }
  }
  _STLP_CATCH_ALL {
    this->_M_handle_exception(ios_base::badbit);
  }
  return __any_inserted;
}

_STLP_MOVE_TO_PRIV_NAMESPACE

// Helper function for numeric output.
template <class _CharT, class _Traits, class _Number>
basic_ostream<_CharT, _Traits>&  _STLP_CALL
__put_num(basic_ostream<_CharT, _Traits>& __os, _Number __x) {
  typedef typename basic_ostream<_CharT, _Traits>::sentry _Sentry;
  _Sentry __sentry(__os);
  bool __failed = true;

  if (__sentry) {
    _STLP_TRY {
      typedef num_put<_CharT, ostreambuf_iterator<_CharT, _Traits> > _NumPut;
      __failed = (use_facet<_NumPut>(__os.getloc())).put(ostreambuf_iterator<_CharT, _Traits>(__os.rdbuf()),
                                                         __os, __os.fill(),
                                                         __x).failed();
    }
    _STLP_CATCH_ALL {
      __os._M_handle_exception(ios_base::badbit);
    }
  }
  if (__failed)
    __os.setstate(ios_base::badbit);
  return __os;
}

_STLP_MOVE_TO_STD_NAMESPACE

/*
 * In the following operators we try to limit code bloat by limiting the
 * number of __put_num instanciations.
 */
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(short __x) {
  _STLP_STATIC_ASSERT( sizeof(short) <= sizeof(long) )
  long __tmp = ((this->flags() & _Basic_ios::basefield) != ios_base::dec) ?
                  __STATIC_CAST(long, __STATIC_CAST(unsigned short, __x)): __x;
  return _STLP_PRIV __put_num(*this, __tmp);
}

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(unsigned short __x) {
  _STLP_STATIC_ASSERT( sizeof(unsigned short) <= sizeof(unsigned long) )
  return _STLP_PRIV __put_num(*this, __STATIC_CAST(unsigned long,__x));
}

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(int __x) {
  _STLP_STATIC_ASSERT( sizeof(int) <= sizeof(long) )
  long __tmp = ((this->flags() & _Basic_ios::basefield) != ios_base::dec) ?
                  __STATIC_CAST(long, __STATIC_CAST(unsigned int, __x)): __x;
  return _STLP_PRIV __put_num(*this, __tmp);
}

template <class _CharT, class _Traits>
#if defined (_WIN64) || !defined (_STLP_MSVC) || (_STLP_MSVC < 1300)
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(unsigned int __x) {
  _STLP_STATIC_ASSERT( sizeof(unsigned int) <= sizeof(unsigned long) )
#else
/* We define this operator with size_t rather than unsigned int to avoid
 * 64 bits warning.
 */
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(size_t __x) {
  _STLP_STATIC_ASSERT( sizeof(size_t) <= sizeof(unsigned long) )
#endif
  return _STLP_PRIV __put_num(*this,  __STATIC_CAST(unsigned long,__x));
}

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(long __x)
{ return _STLP_PRIV __put_num(*this,  __x); }

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(unsigned long __x)
{ return _STLP_PRIV __put_num(*this,  __x); }

#ifdef _STLP_LONG_LONG
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<< (_STLP_LONG_LONG __x)
{ return _STLP_PRIV __put_num(*this,  __x); }

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<< (unsigned _STLP_LONG_LONG __x)
{ return _STLP_PRIV __put_num(*this,  __x); }
#endif

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(float __x)
{ return _STLP_PRIV __put_num(*this,  __STATIC_CAST(double,__x)); }

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(double __x)
{ return _STLP_PRIV __put_num(*this,  __x); }

#ifndef _STLP_NO_LONG_DOUBLE
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(long double __x)
{ return _STLP_PRIV __put_num(*this,  __x); }
#endif

template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(const void* __x)
{ return _STLP_PRIV __put_num(*this,  __x); }

#ifndef _STLP_NO_BOOL
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>& basic_ostream<_CharT, _Traits>::operator<<(bool __x)
{ return _STLP_PRIV __put_num(*this,  __x); }
#endif

template <class _CharT, class _Traits>
void basic_ostream<_CharT, _Traits>::_M_put_char(_CharT __c) {
  sentry __sentry(*this);
  if (__sentry) {
    bool __failed = true;
    _STLP_TRY {
      streamsize __npad = this->width() > 0 ? this->width() - 1 : 0;
      //      if (__npad <= 1)
      if (__npad == 0)
        __failed = this->_S_eof(this->rdbuf()->sputc(__c));
      else if ((this->flags() & ios_base::adjustfield) == ios_base::left) {
        __failed = this->_S_eof(this->rdbuf()->sputc(__c));
        __failed = __failed ||
                   this->rdbuf()->_M_sputnc(this->fill(), __npad) != __npad;
      }
      else {
        __failed = this->rdbuf()->_M_sputnc(this->fill(), __npad) != __npad;
        __failed = __failed || this->_S_eof(this->rdbuf()->sputc(__c));
      }

      this->width(0);
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }

    if (__failed)
      this->setstate(ios_base::badbit);
  }
}

template <class _CharT, class _Traits>
void basic_ostream<_CharT, _Traits>::_M_put_nowiden(const _CharT* __s) {
  sentry __sentry(*this);
  if (__sentry) {
    bool __failed = true;
    streamsize __n = _Traits::length(__s);
    streamsize __npad = this->width() > __n ? this->width() - __n : 0;

    _STLP_TRY {
      if (__npad == 0)
        __failed = this->rdbuf()->sputn(__s, __n) != __n;
      else if ((this->flags() & ios_base::adjustfield) == ios_base::left) {
        __failed = this->rdbuf()->sputn(__s, __n) != __n;
        __failed = __failed ||
                   this->rdbuf()->_M_sputnc(this->fill(), __npad) != __npad;
      }
      else {
        __failed = this->rdbuf()->_M_sputnc(this->fill(), __npad) != __npad;
        __failed = __failed || this->rdbuf()->sputn(__s, __n) != __n;
      }

      this->width(0);
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }

    if (__failed)
      this->setstate(ios_base::failbit);
  }
}

template <class _CharT, class _Traits>
void basic_ostream<_CharT, _Traits>::_M_put_widen(const char* __s) {
  sentry __sentry(*this);
  if (__sentry) {
    bool __failed = true;
    streamsize __n = char_traits<char>::length(__s);
    streamsize __npad = this->width() > __n ? this->width() - __n : 0;

    _STLP_TRY {
      if (__npad == 0)
        __failed = !this->_M_put_widen_aux(__s, __n);
      else if ((this->flags() & ios_base::adjustfield) == ios_base::left) {
        __failed = !this->_M_put_widen_aux(__s, __n);
        __failed = __failed ||
                   this->rdbuf()->_M_sputnc(this->fill(), __npad) != __npad;
      }
      else {
        __failed = this->rdbuf()->_M_sputnc(this->fill(), __npad) != __npad;
        __failed = __failed || !this->_M_put_widen_aux(__s, __n);
      }

      this->width(0);
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }

    if (__failed)
      this->setstate(ios_base::failbit);
  }
}

template <class _CharT, class _Traits>
bool basic_ostream<_CharT, _Traits>::_M_put_widen_aux(const char* __s,
                                                      streamsize __n) {
  basic_streambuf<_CharT, _Traits>* __buf = this->rdbuf();

  for ( ; __n > 0 ; --__n)
    if (this->_S_eof(__buf->sputc(this->widen(*__s++))))
      return false;
  return true;
}

// Unformatted output of a single character.
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>&
basic_ostream<_CharT, _Traits>::put(char_type __c) {
  sentry __sentry(*this);
  bool __failed = true;

  if (__sentry) {
    _STLP_TRY {
      __failed = this->_S_eof(this->rdbuf()->sputc(__c));
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }
  }

  if (__failed)
    this->setstate(ios_base::badbit);

  return *this;
}

// Unformatted output of a single character.
template <class _CharT, class _Traits>
basic_ostream<_CharT, _Traits>&
basic_ostream<_CharT, _Traits>::write(const char_type* __s, streamsize __n) {
  sentry __sentry(*this);
  bool __failed = true;

  if (__sentry) {
    _STLP_TRY {
      __failed = this->rdbuf()->sputn(__s, __n) != __n;
    }
    _STLP_CATCH_ALL {
      this->_M_handle_exception(ios_base::badbit);
    }
  }

  if (__failed)
    this->setstate(ios_base::badbit);

  return *this;
}

_STLP_END_NAMESPACE

#endif /* _STLP_OSTREAM_C */

// Local Variables:
// mode:C++
// End:
