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

#ifndef _STLP_SSTREAM_C
#define _STLP_SSTREAM_C

#ifndef _STLP_INTERNAL_SSTREAM
#  include <stl/_sstream.h>
#endif

#if defined ( _STLP_NESTED_TYPE_PARAM_BUG )
// no wint_t is supported for this mode
#  define __BSB_int_type__ int
#  define __BSB_pos_type__ streampos
#else
#  define __BSB_int_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_stringbuf<_CharT, _Traits, _Alloc>::int_type
#  define __BSB_pos_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_stringbuf<_CharT, _Traits, _Alloc>::pos_type
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// Non-inline stringbuf member functions.

// Constructors.  Note that the base class constructor sets all of the
// get and area pointers to null.

template <class _CharT, class _Traits, class _Alloc>
basic_stringbuf<_CharT, _Traits, _Alloc>
  ::basic_stringbuf(ios_base::openmode __mode)
    : basic_streambuf<_CharT, _Traits>(), _M_mode(__mode), _M_str()
{}

template <class _CharT, class _Traits, class _Alloc>
basic_stringbuf<_CharT, _Traits, _Alloc>
  ::basic_stringbuf(const basic_string<_CharT, _Traits, _Alloc>& __s, ios_base::openmode __mode)
    : basic_streambuf<_CharT, _Traits>(), _M_mode(__mode), _M_str(__s)
{
  _M_set_ptrs();
}

template <class _CharT, class _Traits, class _Alloc>
basic_stringbuf<_CharT, _Traits, _Alloc>::~basic_stringbuf()
{}

// Set the underlying string to a new value.
template <class _CharT, class _Traits, class _Alloc>
void
basic_stringbuf<_CharT, _Traits, _Alloc>::str(const basic_string<_CharT, _Traits, _Alloc>& __s)
{
  _M_str = __s;
  _M_set_ptrs();
}

template <class _CharT, class _Traits, class _Alloc>
void
basic_stringbuf<_CharT, _Traits, _Alloc>::_M_set_ptrs()
{
  _CharT* __data_ptr = _S_start(_M_str);
  _CharT* __data_end = _S_finish(_M_str);
  // The initial read position is the beginning of the string.
  if (_M_mode & ios_base::in) {
    this->setg(__data_ptr, (_M_mode & ios_base::ate) ? __data_end : __data_ptr, __data_end);
  }

  // The initial write position is the beginning of the string.
  if (_M_mode & ios_base::out) {
    if ( _M_mode & (ios_base::app | ios_base::ate) ) {
      this->setp( __data_end, __data_end );
    } else {
      this->setp( __data_ptr, __data_end );
      this->pbump((int)_M_str.size()); // initial write position, if we initialized with string 
    }
    // this->setp((_M_mode & (ios_base::app | ios_base::ate))? __data_end : __data_ptr, __data_end);
  }
}

// Precondition: gptr() >= egptr().  Returns a character, if one is available.
template <class _CharT, class _Traits, class _Alloc>
__BSB_int_type__
basic_stringbuf<_CharT, _Traits, _Alloc>::underflow() {
  return this->gptr() != this->egptr()
    ? _Traits::to_int_type(*this->gptr())
    : _Traits::eof();
}

// Precondition: gptr() >= egptr().
template <class _CharT, class _Traits, class _Alloc>
__BSB_int_type__
basic_stringbuf<_CharT, _Traits, _Alloc>::uflow() {
  if (this->gptr() != this->egptr()) {
    int_type __c = _Traits::to_int_type(*this->gptr());
    this->gbump(1);
    return __c;
  }
  else
    return _Traits::eof();
}

template <class _CharT, class _Traits, class _Alloc>
__BSB_int_type__
basic_stringbuf<_CharT, _Traits, _Alloc>::pbackfail(int_type __c) {
  if (this->gptr() != this->eback()) {
    if (!_Traits::eq_int_type(__c, _Traits::eof())) {
      if (_Traits::eq(_Traits::to_char_type(__c), this->gptr()[-1])) {
        this->gbump(-1);
        return __c;
      }
      else if (_M_mode & ios_base::out) {
        this->gbump(-1);
        *this->gptr() = _Traits::to_char_type(__c);
        return __c;
      }
      else
        return _Traits::eof();
    }
    else {
      this->gbump(-1);
      return _Traits::not_eof(__c);
    }
  }
  else
    return _Traits::eof();
}

template <class _CharT, class _Traits, class _Alloc>
__BSB_int_type__ basic_stringbuf<_CharT, _Traits, _Alloc>::overflow(int_type __c)
{
  if (!_Traits::eq_int_type(__c, _Traits::eof())) {
    if (_M_mode & ios_base::out) {
      if ( this->pptr() < this->epptr() ) { // just put back in any case
        _M_str.push_back( _Traits::to_char_type(__c) );
        this->pbump(1);
      } else if ( _M_mode & ios_base::in ) {
        ptrdiff_t __offset = this->gptr() - this->eback();
        _M_str.push_back(_Traits::to_char_type(__c));
        _CharT* __data_ptr = _S_start(_M_str);
        this->setg(__data_ptr, __data_ptr + __offset, _S_finish(_M_str));
        this->setp(__data_ptr, _S_finish(_M_str));
        this->pbump((int)_M_str.size());
      } else {
        _M_str.push_back( _Traits::to_char_type(__c) );
        this->setp(_S_start(_M_str), _S_finish(_M_str));
        this->pbump((int)_M_str.size());
      }
      return __c;
    }
    return _Traits::eof(); // Overflow always fails if it's read-only
  }
  return _Traits::not_eof(__c); // __c is EOF, so we don't have to do anything
}

template <class _CharT, class _Traits, class _Alloc>
streamsize
basic_stringbuf<_CharT, _Traits, _Alloc>::xsputn(const char_type* __s,
                                                 streamsize __n) {
  streamsize __nwritten = 0;

  if ((_M_mode & ios_base::out) && __n > 0) {
    // If the put pointer is somewhere in the middle of the string,
    // then overwrite instead of append.
    if ( !_M_str.empty() && this->pbase() == _S_start(_M_str)) {
      ptrdiff_t __avail = _S_finish(_M_str) - this->pptr();
      if (__avail > __n) {
        _Traits::copy(this->pptr(), __s, __STATIC_CAST(size_t, __n));
        this->pbump((int)__n);
        return __n;
      } else {
        _Traits::copy(this->pptr(), __s, __avail);
        __nwritten += __avail;
        __n -= __avail;
        __s += __avail;
      }
    }

    // At this point we know we're appending.
    _CharT* __data_ptr;
    if (_M_mode & ios_base::in) {
      ptrdiff_t __get_offset = this->gptr() - this->eback();
      _M_str.append(__s, __s + __STATIC_CAST(ptrdiff_t, __n));
      __data_ptr = _S_start(_M_str);
      this->setg(__data_ptr, __data_ptr + __get_offset, _S_finish(_M_str));
    } else {
      _M_str.append(__s, __s + __STATIC_CAST(ptrdiff_t, __n));
      __data_ptr = _S_start(_M_str);
    }

    this->setp(__data_ptr, _S_finish(_M_str));
    this->pbump((int)_M_str.size());
    __nwritten += __n;
  }

  return __nwritten;
}

template <class _CharT, class _Traits, class _Alloc>
streamsize
basic_stringbuf<_CharT, _Traits, _Alloc>::_M_xsputnc(char_type __c,
                                                     streamsize __n) {
  streamsize __nwritten = 0;

  if ((_M_mode & ios_base::out) && __n > 0) {
    // If the put pointer is somewhere in the middle of the string,
    // then overwrite instead of append.
    if (this->pbase() == _S_start(_M_str)) {
      ptrdiff_t __avail = _S_finish(_M_str) - this->pptr();
      if (__avail > __n) {
        _Traits::assign(this->pptr(), __STATIC_CAST(size_t, __n), __c);
        this->pbump(__STATIC_CAST(int, __n));
        return __n;
      }
      else {
        _Traits::assign(this->pptr(), __avail, __c);
        __nwritten += __avail;
        __n -= __avail;
      }
    }

    // At this point we know we're appending.
    size_t __app_size = sizeof(streamsize) > sizeof(size_t) ? __STATIC_CAST(size_t, (min)(__n, __STATIC_CAST(streamsize, _M_str.max_size())))
                                                            : __STATIC_CAST(size_t, __n);
    _CharT* __data_ptr;
    if (this->_M_mode & ios_base::in) {
      ptrdiff_t __get_offset = this->gptr() - this->eback();
      _M_str.append(__app_size, __c);
      __data_ptr = _S_start(_M_str);
      this->setg(__data_ptr, __data_ptr + __get_offset, _S_finish(_M_str));
    } else {
      _M_str.append(__app_size, __c);
      __data_ptr = _S_start(_M_str);
    }

    this->setp(__data_ptr, _S_finish(_M_str));
    this->pbump((int)_M_str.size());
    __nwritten += __app_size;
  }

  return __nwritten;
}

// According to the C++ standard the effects of setbuf are implementation
// defined, except that setbuf(0, 0) has no effect.  In this implementation,
// setbuf(<anything>, n), for n > 0, calls reserve(n) on the underlying
// string.
template <class _CharT, class _Traits, class _Alloc>
basic_streambuf<_CharT, _Traits>*
basic_stringbuf<_CharT, _Traits, _Alloc>::setbuf(_CharT*, streamsize __n) {
  if (__n > 0) {
    bool __do_get_area = false;
    bool __do_put_area = false;
    ptrdiff_t __offg = 0;
    ptrdiff_t __offp = 0;

    if (this->pbase() == _S_start(_M_str)) {
      __do_put_area = true;
      __offp = this->pptr() - this->pbase();
    }

    if (this->eback() == _S_start(_M_str)) {
      __do_get_area = true;
      __offg = this->gptr() - this->eback();
    }

    _M_str.reserve(sizeof(streamsize) > sizeof(size_t) ? __STATIC_CAST(size_t, (min)(__n, __STATIC_CAST(streamsize, _M_str.max_size())))
                                                       : __STATIC_CAST(size_t, __n));

    _CharT* __data_ptr = _S_start(_M_str);

    if (__do_get_area) {
      this->setg(__data_ptr, __data_ptr + __offg, _S_finish(_M_str));
    }

    if (__do_put_area) {
      this->setp(__data_ptr, _S_finish(_M_str));
      this->pbump((int)__offp);
    }
  }

  return this;
}

template <class _CharT, class _Traits, class _Alloc>
__BSB_pos_type__
basic_stringbuf<_CharT, _Traits, _Alloc>
  ::seekoff(off_type __off,
            ios_base::seekdir __dir,
            ios_base::openmode __mode) {
  __mode &= _M_mode;

  bool __imode  = (__mode & ios_base::in) != 0;
  bool __omode = (__mode & ios_base::out) != 0;

  if ( !(__imode || __omode) )
    return pos_type(off_type(-1));

  if ( (__imode && (this->gptr() == 0)) || (__omode && (this->pptr() == 0)) )
    return pos_type(off_type(-1));

  streamoff __newoff;
  switch(__dir) {
    case ios_base::beg:
      __newoff = 0;
      break;
    case ios_base::end:
      __newoff = _M_str.size();
      break;
    case ios_base::cur:
      __newoff = __imode ? this->gptr() - this->eback() : this->pptr() - this->pbase();
      if ( __off == 0 ) {
        return pos_type(__newoff);
      }
      break;
    default:
      return pos_type(off_type(-1));
  }

  __off += __newoff;

  if (__imode) {
    ptrdiff_t __n = this->egptr() - this->eback();

    if (__off < 0 || __off > __n)
      return pos_type(off_type(-1));
    this->setg(this->eback(), this->eback() + __STATIC_CAST(ptrdiff_t, __off),
                              this->eback() + __STATIC_CAST(ptrdiff_t, __n));
  }

  if (__omode) {
    ptrdiff_t __n = this->epptr() - this->pbase();

    if (__off < 0 || __off > __n)
      return pos_type(off_type(-1));
    this->setp(this->pbase(), this->pbase() + __n);
    this->pbump((int)__off);
  }

  return pos_type(__off);
}

template <class _CharT, class _Traits, class _Alloc>
__BSB_pos_type__
basic_stringbuf<_CharT, _Traits, _Alloc>
  ::seekpos(pos_type __pos, ios_base::openmode __mode) {
  __mode &= _M_mode;

  bool __imode = (__mode & ios_base::in) != 0;
  bool __omode = (__mode & ios_base::out) != 0;

  if ( !(__imode || __omode) )
    return pos_type(off_type(-1));

  if ( (__imode && (this->gptr() == 0)) || (__omode && (this->pptr() == 0)) )
    return pos_type(off_type(-1));

  const off_type __n = __pos - pos_type(off_type(0));

  if (__imode) {
    if (__n < 0 || __n > this->egptr() - this->eback())
      return pos_type(off_type(-1));
    this->setg(this->eback(), this->eback() + __STATIC_CAST(ptrdiff_t, __n), this->egptr());
  }

  if (__omode) {
    if (__n < 0 || size_t(__n) > _M_str.size())
      return pos_type(off_type(-1));

    this->setp(_S_start(_M_str), _S_finish(_M_str));
    this->pbump((int)__n);
  }

  return __pos;
}

//----------------------------------------------------------------------
// Non-inline istringstream member functions.

template <class _CharT, class _Traits, class _Alloc>
basic_istringstream<_CharT, _Traits, _Alloc>
  ::basic_istringstream(ios_base::openmode __mode)
    : basic_istream<_CharT, _Traits>(0),
      _M_buf(__mode | ios_base::in) {
  this->init(&_M_buf);
}

template <class _CharT, class _Traits, class _Alloc>
basic_istringstream<_CharT, _Traits, _Alloc>
  ::basic_istringstream(const _String& __str,ios_base::openmode __mode)
    : basic_istream<_CharT, _Traits>(0),
      _M_buf(__str, __mode | ios_base::in) {
  this->init(&_M_buf);
}

template <class _CharT, class _Traits, class _Alloc>
basic_istringstream<_CharT, _Traits, _Alloc>::~basic_istringstream()
{}

//----------------------------------------------------------------------
// Non-inline ostringstream member functions.

template <class _CharT, class _Traits, class _Alloc>
basic_ostringstream<_CharT, _Traits, _Alloc>
  ::basic_ostringstream(ios_base::openmode __mode)
    : basic_ostream<_CharT, _Traits>(0),
      _M_buf(__mode | ios_base::out) {
  this->init(&_M_buf);
}

template <class _CharT, class _Traits, class _Alloc>
basic_ostringstream<_CharT, _Traits, _Alloc>
  ::basic_ostringstream(const _String& __str, ios_base::openmode __mode)
    : basic_ostream<_CharT, _Traits>(0),
      _M_buf(__str, __mode | ios_base::out) {
  this->init(&_M_buf);
}

template <class _CharT, class _Traits, class _Alloc>
basic_ostringstream<_CharT, _Traits, _Alloc>::~basic_ostringstream()
{}

//----------------------------------------------------------------------
// Non-inline stringstream member functions.

template <class _CharT, class _Traits, class _Alloc>
basic_stringstream<_CharT, _Traits, _Alloc>
  ::basic_stringstream(ios_base::openmode __mode)
    : basic_iostream<_CharT, _Traits>(0), _M_buf(__mode) {
   this->init(&_M_buf);
}

template <class _CharT, class _Traits, class _Alloc>
basic_stringstream<_CharT, _Traits, _Alloc>
  ::basic_stringstream(const _String& __str, ios_base::openmode __mode)
    : basic_iostream<_CharT, _Traits>(0), _M_buf(__str, __mode) {
  this->init(&_M_buf);
}

template <class _CharT, class _Traits, class _Alloc>
basic_stringstream<_CharT, _Traits, _Alloc>::~basic_stringstream()
{}

_STLP_END_NAMESPACE

# undef __BSB_int_type__
# undef __BSB_pos_type__

#endif /* _STLP_SSTREAM_C */

// Local Variables:
// mode:C++
// End:
