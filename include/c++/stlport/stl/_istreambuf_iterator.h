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
// WARNING: This is an internal header file, included by other C++
// standard library headers.  You should not attempt to use this header
// file directly.


#ifndef _STLP_INTERNAL_ISTREAMBUF_ITERATOR_H
#define _STLP_INTERNAL_ISTREAMBUF_ITERATOR_H

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
# include <stl/_iterator_base.h>
#endif

#ifndef _STLP_INTERNAL_STREAMBUF
# include <stl/_streambuf.h>
#endif

_STLP_BEGIN_NAMESPACE

// defined in _istream.h
template <class _CharT, class _Traits>
extern basic_streambuf<_CharT, _Traits>* _STLP_CALL _M_get_istreambuf(basic_istream<_CharT, _Traits>& ) ;

// We do not read any characters until operator* is called. operator* calls sgetc
// unless the iterator is unchanged from the last call in which case a cached value is
// used. Calls to operator++ use sbumpc.

template<class _CharT, class _Traits>
class istreambuf_iterator :
  public iterator<input_iterator_tag, _CharT, typename _Traits::off_type, _CharT*, _CharT&>
{
public:
  typedef _CharT                           char_type;
  typedef _Traits                          traits_type;
  typedef typename _Traits::int_type       int_type;
  typedef basic_streambuf<_CharT, _Traits> streambuf_type;
  typedef basic_istream<_CharT, _Traits>   istream_type;

  typedef input_iterator_tag               iterator_category;
  typedef _CharT                           value_type;
  typedef typename _Traits::off_type       difference_type;
  typedef const _CharT*                    pointer;
  typedef const _CharT&                    reference;

public:
  istreambuf_iterator(streambuf_type* __p = 0) { this->_M_init(__p); }
  //  istreambuf_iterator(basic_istream<_CharT, _Traits>& __is) { this->_M_init(_M_get_istreambuf(__is)); }
  inline istreambuf_iterator(basic_istream<_CharT, _Traits>& __is);

  char_type operator*() const { this->_M_getc(); return _M_c; }
  istreambuf_iterator<_CharT, _Traits>& operator++() {
    _M_buf->sbumpc();
    _M_have_c = false;
    return *this;
  }
  istreambuf_iterator<_CharT, _Traits>  operator++(int);

  bool equal(const istreambuf_iterator<_CharT, _Traits>& __i) const {
    if (this->_M_buf)
      this->_M_getc();
    if (__i._M_buf)
      __i._M_getc();
    return this->_M_eof == __i._M_eof;
  }

private:
  void _M_init(streambuf_type* __p) {
    _M_buf = __p;
    _M_eof = (__p == 0);
    _M_have_c = false;
  }

  void _M_getc() const {
    if (_M_have_c)
      return;
    int_type __c = _M_buf->sgetc();
    _STLP_MUTABLE(_Self, _M_c) = traits_type::to_char_type(__c);
    _STLP_MUTABLE(_Self, _M_eof) = traits_type::eq_int_type(__c, traits_type::eof());
    _STLP_MUTABLE(_Self, _M_have_c) = true;
  }

private:
  streambuf_type* _M_buf;
  mutable _CharT _M_c;
  mutable bool _M_eof;
  mutable bool _M_have_c;
};

template<class _CharT, class _Traits>
inline istreambuf_iterator<_CharT, _Traits>::istreambuf_iterator(basic_istream<_CharT, _Traits>& __is)
{ this->_M_init(_M_get_istreambuf(__is)); }

template<class _CharT, class _Traits>
inline bool _STLP_CALL operator==(const istreambuf_iterator<_CharT, _Traits>& __x,
                                  const istreambuf_iterator<_CharT, _Traits>& __y) {
  return __x.equal(__y);
}

#ifdef _STLP_USE_SEPARATE_RELOPS_NAMESPACE

template<class _CharT, class _Traits>
inline bool _STLP_CALL operator!=(const istreambuf_iterator<_CharT, _Traits>& __x,
                                  const istreambuf_iterator<_CharT, _Traits>& __y) {
  return !__x.equal(__y);
}

#endif /* _STLP_USE_SEPARATE_RELOPS_NAMESPACE */

# if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS istreambuf_iterator<char, char_traits<char> >;
#  if defined (INSTANTIATE_WIDE_STREAMS)
_STLP_EXPORT_TEMPLATE_CLASS istreambuf_iterator<wchar_t, char_traits<wchar_t> >;
#  endif
# endif /* _STLP_USE_TEMPLATE_EXPORT */

# ifdef _STLP_USE_OLD_HP_ITERATOR_QUERIES
template <class _CharT, class _Traits>
inline input_iterator_tag _STLP_CALL iterator_category(const istreambuf_iterator<_CharT, _Traits>&) { return input_iterator_tag(); }
template <class _CharT, class _Traits>
inline streamoff* _STLP_CALL
distance_type(const istreambuf_iterator<_CharT, _Traits>&) { return (streamoff*)0; }
template <class _CharT, class _Traits>
inline _CharT* _STLP_CALL value_type(const istreambuf_iterator<_CharT, _Traits>&) { return (_CharT*)0; }
# endif

template <class _CharT, class _Traits>
istreambuf_iterator<_CharT, _Traits>
istreambuf_iterator<_CharT, _Traits>::operator++(int) {
  _M_getc(); // __tmp should avoid any future actions under
  // underlined buffer---during call of operator *()
  // (due to buffer for *this and __tmp are the same).
  istreambuf_iterator<_CharT, _Traits> __tmp = *this;
  _M_buf->sbumpc();
  _M_have_c = false;
  return __tmp;
}

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_ISTREAMBUF_ITERATOR_H */

// Local Variables:
// mode:C++
// End:

