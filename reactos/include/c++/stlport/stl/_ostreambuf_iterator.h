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


#ifndef _STLP_INTERNAL_OSTREAMBUF_ITERATOR_H
#define _STLP_INTERNAL_OSTREAMBUF_ITERATOR_H

#ifndef _STLP_INTERNAL_STREAMBUF
# include <stl/_streambuf.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template<class _CharT, class _Traits>
extern basic_streambuf<_CharT, _Traits>* _STLP_CALL __get_ostreambuf(basic_ostream<_CharT, _Traits>&);

_STLP_MOVE_TO_STD_NAMESPACE

// The default template argument is declared in iosfwd
template <class _CharT, class _Traits>
class ostreambuf_iterator :
  public iterator<output_iterator_tag, void, void, void, void> {
public:
  typedef _CharT                           char_type;
  typedef _Traits                          traits_type;
  typedef typename _Traits::int_type       int_type;
  typedef basic_streambuf<_CharT, _Traits> streambuf_type;
  typedef basic_ostream<_CharT, _Traits>   ostream_type;

  typedef output_iterator_tag              iterator_category;
  typedef void                             value_type;
  typedef void                             difference_type;
  typedef void                             pointer;
  typedef void                             reference;

public:
  ostreambuf_iterator(streambuf_type* __buf) _STLP_NOTHROW : _M_buf(__buf), _M_ok(__buf!=0) {}
  //  ostreambuf_iterator(ostream_type& __o) _STLP_NOTHROW : _M_buf(__get_ostreambuf(__o)), _M_ok(_M_buf != 0) {}
  inline ostreambuf_iterator(ostream_type& __o) _STLP_NOTHROW;

  ostreambuf_iterator<_CharT, _Traits>& operator=(char_type __c) {
    _M_ok = _M_ok && !traits_type::eq_int_type(_M_buf->sputc(__c),
                                               traits_type::eof());
    return *this;
  }

  ostreambuf_iterator<_CharT, _Traits>& operator*()     { return *this; }
  ostreambuf_iterator<_CharT, _Traits>& operator++()    { return *this; }
  ostreambuf_iterator<_CharT, _Traits>& operator++(int) { return *this; }

  bool failed() const { return !_M_ok; }

private:
  streambuf_type* _M_buf;
  bool _M_ok;
};

template <class _CharT, class _Traits>
inline ostreambuf_iterator<_CharT, _Traits>::ostreambuf_iterator(basic_ostream<_CharT, _Traits>& __o) _STLP_NOTHROW
  : _M_buf(_STLP_PRIV __get_ostreambuf(__o)), _M_ok(_M_buf != 0) {}

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS ostreambuf_iterator<char, char_traits<char> >;
#  if defined (INSTANTIATE_WIDE_STREAMS)
_STLP_EXPORT_TEMPLATE_CLASS ostreambuf_iterator<wchar_t, char_traits<wchar_t> >;
#  endif
#endif /* _STLP_USE_TEMPLATE_EXPORT */

#if defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
template <class _CharT, class _Traits>
inline output_iterator_tag _STLP_CALL
iterator_category(const ostreambuf_iterator<_CharT, _Traits>&) { return output_iterator_tag(); }
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_OSTREAMBUF_ITERATOR_H */

// Local Variables:
// mode:C++
// End:

