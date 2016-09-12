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
#ifndef _STLP_STREAMBUF_C
#define _STLP_STREAMBUF_C

#ifndef _STLP_INTERNAL_STREAMBUF
#  include <stl/_streambuf.h>
#endif

_STLP_BEGIN_NAMESPACE
//----------------------------------------------------------------------
// Non-inline basic_streambuf<> member functions.

#if !defined (_STLP_MSVC) || (_STLP_MSVC >= 1300) || !defined (_STLP_USE_STATIC_LIB)
template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>::basic_streambuf()
  : _M_gbegin(0), _M_gnext(0), _M_gend(0),
    _M_pbegin(0), _M_pnext(0), _M_pend(0),
    _M_locale() {
  //  _M_lock._M_initialize();
}
#endif

template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>::~basic_streambuf()
{}

template <class _CharT, class _Traits>
locale
basic_streambuf<_CharT, _Traits>::pubimbue(const locale& __loc) {
  this->imbue(__loc);
  locale __tmp = _M_locale;
  _M_locale = __loc;
  return __tmp;
}

template <class _CharT, class _Traits>
streamsize
basic_streambuf<_CharT, _Traits>::xsgetn(_CharT* __s, streamsize __n) {
  streamsize __result = 0;
  const int_type __eof = _Traits::eof();

  while (__result < __n) {
    if (_M_gnext < _M_gend) {
      size_t __chunk = (min) (__STATIC_CAST(size_t,_M_gend - _M_gnext),
                              __STATIC_CAST(size_t,__n - __result));
      _Traits::copy(__s, _M_gnext, __chunk);
      __result += __chunk;
      __s += __chunk;
      _M_gnext += __chunk;
    }
    else {
      int_type __c = this->sbumpc();
      if (!_Traits::eq_int_type(__c, __eof)) {
        *__s = _Traits::to_char_type(__c);
        ++__result;
        ++__s;
      }
      else
        break;
    }
  }

  return __result;
}

template <class _CharT, class _Traits>
streamsize
basic_streambuf<_CharT, _Traits>::xsputn(const _CharT* __s, streamsize __n)
{
  streamsize __result = 0;
  const int_type __eof = _Traits::eof();

  while (__result < __n) {
    if (_M_pnext < _M_pend) {
      size_t __chunk = (min) (__STATIC_CAST(size_t,_M_pend - _M_pnext),
                           __STATIC_CAST(size_t,__n - __result));
      _Traits::copy(_M_pnext, __s, __chunk);
      __result += __chunk;
      __s += __chunk;
      _M_pnext += __chunk;
    }

    else if (!_Traits::eq_int_type(this->overflow(_Traits::to_int_type(*__s)),
                                   __eof)) {
      ++__result;
      ++__s;
    }
    else
      break;
  }
  return __result;
}

template <class _CharT, class _Traits>
streamsize
basic_streambuf<_CharT, _Traits>::_M_xsputnc(_CharT __c, streamsize __n)
{
  streamsize __result = 0;
  const int_type __eof = _Traits::eof();

  while (__result < __n) {
    if (_M_pnext < _M_pend) {
      size_t __chunk = (min) (__STATIC_CAST(size_t,_M_pend - _M_pnext),
                           __STATIC_CAST(size_t,__n - __result));
      _Traits::assign(_M_pnext, __chunk, __c);
      __result += __chunk;
      _M_pnext += __chunk;
    }

    else if (!_Traits::eq_int_type(this->overflow(_Traits::to_int_type(__c)),
                                   __eof))
      ++__result;
    else
      break;
  }
  return __result;
}

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::int_type
basic_streambuf<_CharT, _Traits>::_M_snextc_aux()
{
  int_type __eof = _Traits::eof();
  if (_M_gend == _M_gnext)
    return _Traits::eq_int_type(this->uflow(), __eof) ? __eof : this->sgetc();
  else {
    _M_gnext = _M_gend;
    return this->underflow();
  }
}

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::int_type
basic_streambuf<_CharT, _Traits>::pbackfail(int_type) {
 return _Traits::eof();
}

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::int_type
basic_streambuf<_CharT, _Traits>::overflow(int_type) {
  return _Traits::eof();
}

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::int_type
basic_streambuf<_CharT, _Traits>::uflow() {
    return ( _Traits::eq_int_type(this->underflow(),_Traits::eof()) ?
             _Traits::eof() :
             _Traits::to_int_type(*_M_gnext++));
}

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::int_type
basic_streambuf<_CharT, _Traits>::underflow()
{ return _Traits::eof(); }

template <class _CharT, class _Traits>
streamsize
basic_streambuf<_CharT, _Traits>::showmanyc()
{ return 0; }

template <class _CharT, class _Traits>
void
basic_streambuf<_CharT, _Traits>::imbue(const locale&) {}

template <class _CharT, class _Traits>
int
basic_streambuf<_CharT, _Traits>::sync() { return 0; }

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::pos_type
basic_streambuf<_CharT, _Traits>::seekpos(pos_type, ios_base::openmode)
{ return pos_type(-1); }

template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_streambuf<_CharT, _Traits>::pos_type
basic_streambuf<_CharT, _Traits>::seekoff(off_type, ios_base::seekdir,
                                          ios_base::openmode)
{ return pos_type(-1); }

template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>*
basic_streambuf<_CharT, _Traits>:: setbuf(char_type*, streamsize)
{ return this; }

_STLP_END_NAMESPACE

#endif

// Local Variables:
// mode:C++
// End:
