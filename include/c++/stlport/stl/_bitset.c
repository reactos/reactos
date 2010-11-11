/*
 * Copyright (c) 1998
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

#ifndef _STLP_BITSET_C
#define _STLP_BITSET_C

#ifndef _STLP_BITSET_H
#  include <stl/_bitset.h>
#endif

#define __BITS_PER_WORD (CHAR_BIT * sizeof(unsigned long))

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE
//
// Definitions of non-inline functions from _Base_bitset.
//
template<size_t _Nw>
void _Base_bitset<_Nw>::_M_do_left_shift(size_t __shift) {
  if (__shift != 0) {
    const size_t __wshift = __shift / __BITS_PER_WORD;
    const size_t __offset = __shift % __BITS_PER_WORD;

    if (__offset == 0)
      for (size_t __n = _Nw - 1; __n >= __wshift; --__n)
        _M_w[__n] = _M_w[__n - __wshift];

    else {
      const size_t __sub_offset = __BITS_PER_WORD - __offset;
      for (size_t __n = _Nw - 1; __n > __wshift; --__n)
        _M_w[__n] = (_M_w[__n - __wshift] << __offset) |
                    (_M_w[__n - __wshift - 1] >> __sub_offset);
      _M_w[__wshift] = _M_w[0] << __offset;
    }

    fill(_M_w + 0, _M_w + __wshift, __STATIC_CAST(_WordT,0));
  }
}

template<size_t _Nw>
void _Base_bitset<_Nw>::_M_do_right_shift(size_t __shift) {
  if (__shift != 0) {
    const size_t __wshift = __shift / __BITS_PER_WORD;
    const size_t __offset = __shift % __BITS_PER_WORD;
    const size_t __limit = _Nw - __wshift - 1;

    if (__offset == 0)
      for (size_t __n = 0; __n <= __limit; ++__n)
        _M_w[__n] = _M_w[__n + __wshift];

    else {
      const size_t __sub_offset = __BITS_PER_WORD - __offset;
      for (size_t __n = 0; __n < __limit; ++__n)
        _M_w[__n] = (_M_w[__n + __wshift] >> __offset) |
                    (_M_w[__n + __wshift + 1] << __sub_offset);
      _M_w[__limit] = _M_w[_Nw-1] >> __offset;
    }

    fill(_M_w + __limit + 1, _M_w + _Nw, __STATIC_CAST(_WordT,0));
  }
}

template<size_t _Nw>
unsigned long _Base_bitset<_Nw>::_M_do_to_ulong() const {
  for (size_t __i = 1; __i < _Nw; ++__i)
    if (_M_w[__i])
      __stl_throw_overflow_error("bitset");
  return _M_w[0];
} // End _M_do_to_ulong

template<size_t _Nw>
size_t _Base_bitset<_Nw>::_M_do_find_first(size_t __not_found) const {
  for ( size_t __i = 0; __i < _Nw; __i++ ) {
    _WordT __thisword = _M_w[__i];
    if ( __thisword != __STATIC_CAST(_WordT,0) ) {
      // find byte within word
      for ( size_t __j = 0; __j < sizeof(_WordT); __j++ ) {
        unsigned char __this_byte
          = __STATIC_CAST(unsigned char,(__thisword & (~(unsigned char)0)));
        if ( __this_byte )
          return __i*__BITS_PER_WORD + __j*CHAR_BIT +
            _Bs_G::_S_first_one(__this_byte);

        __thisword >>= CHAR_BIT;
      }
    }
  }
  // not found, so return an indication of failure.
  return __not_found;
}

template<size_t _Nw>
size_t
_Base_bitset<_Nw>::_M_do_find_next(size_t __prev,
                                   size_t __not_found) const {
  // make bound inclusive
  ++__prev;

  // check out of bounds
  if ( __prev >= _Nw * __BITS_PER_WORD )
    return __not_found;

    // search first word
  size_t __i = _S_whichword(__prev);
  _WordT __thisword = _M_w[__i];

    // mask off bits below bound
  __thisword &= (~__STATIC_CAST(_WordT,0)) << _S_whichbit(__prev);

  if ( __thisword != __STATIC_CAST(_WordT,0) ) {
    // find byte within word
    // get first byte into place
    __thisword >>= _S_whichbyte(__prev) * CHAR_BIT;
    for ( size_t __j = _S_whichbyte(__prev); __j < sizeof(_WordT); ++__j ) {
      unsigned char __this_byte
        = __STATIC_CAST(unsigned char,(__thisword & (~(unsigned char)0)));
      if ( __this_byte )
        return __i*__BITS_PER_WORD + __j*CHAR_BIT +
          _Bs_G::_S_first_one(__this_byte);

      __thisword >>= CHAR_BIT;
    }
  }

  // check subsequent words
  ++__i;
  for ( ; __i < _Nw; ++__i ) {
    /* _WordT */ __thisword = _M_w[__i];
    if ( __thisword != __STATIC_CAST(_WordT,0) ) {
      // find byte within word
      for ( size_t __j = 0; __j < sizeof(_WordT); ++__j ) {
        unsigned char __this_byte
          = __STATIC_CAST(unsigned char,(__thisword & (~(unsigned char)0)));
        if ( __this_byte )
          return __i*__BITS_PER_WORD + __j*CHAR_BIT +
            _Bs_G::_S_first_one(__this_byte);

        __thisword >>= CHAR_BIT;
      }
    }
  }

  // not found, so return an indication of failure.
  return __not_found;
} // end _M_do_find_next

_STLP_MOVE_TO_STD_NAMESPACE

#if !defined (_STLP_NON_TYPE_TMPL_PARAM_BUG)

#  if !defined (_STLP_USE_NO_IOSTREAMS)

_STLP_END_NAMESPACE

#ifndef _STLP_STRING_IO_H
#  include <stl/_string_io.h> //includes _istream.h and _ostream.h
#endif

_STLP_BEGIN_NAMESPACE

template <class _CharT, class _Traits, size_t _Nb>
basic_istream<_CharT, _Traits>& _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __is, bitset<_Nb>& __x) {
  basic_string<_CharT, _Traits> __tmp;
  __tmp.reserve(_Nb);

  // Skip whitespace
  typename basic_istream<_CharT, _Traits>::sentry __sentry(__is);
  if (__sentry) {
    basic_streambuf<_CharT, _Traits>* __buf = __is.rdbuf();
    for (size_t __i = 0; __i < _Nb; ++__i) {
      static typename _Traits::int_type __eof = _Traits::eof();

      typename _Traits::int_type __c1 = __buf->sbumpc();
      if (_Traits::eq_int_type(__c1, __eof)) {
        __is.setstate(ios_base::eofbit);
        break;
      }
      else {
        typename _Traits::char_type __c2 = _Traits::to_char_type(__c1);
        char __c = __is.narrow(__c2, '*');

        if (__c == '0' || __c == '1')
          __tmp.push_back(__c);
        else if (_Traits::eq_int_type(__buf->sputbackc(__c2), __eof)) {
          __is.setstate(ios_base::failbit);
          break;
        }
      }
    }

    if (__tmp.empty())
      __is.setstate(ios_base::failbit);
    else
      __x._M_copy_from_string(__tmp, __STATIC_CAST(size_t,0), _Nb);
  }

  return __is;
}

template <class _CharT, class _Traits, size_t _Nb>
basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os,
           const bitset<_Nb>& __x) {
  basic_string<_CharT, _Traits> __tmp;
  __x._M_copy_to_string(__tmp);
  return __os << __tmp;
}

#  endif /* !_STLP_USE_NO_IOSTREAMS */

#endif /* _STLP_NON_TYPE_TMPL_PARAM_BUG */

_STLP_END_NAMESPACE

#undef __BITS_PER_WORD
#undef bitset

#endif /*  _STLP_BITSET_C */
