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
#ifndef _STLP_IOS_C
#define _STLP_IOS_C

#ifndef _STLP_INTERNAL_IOS_H
# include <stl/_ios.h>
#endif

#ifndef _STLP_INTERNAL_STREAMBUF
# include <stl/_streambuf.h>
#endif

#ifndef _STLP_INTERNAL_NUMPUNCT_H
# include <stl/_numpunct.h>
#endif

_STLP_BEGIN_NAMESPACE

// basic_ios<>'s non-inline member functions

// Public constructor, taking a streambuf.
template <class _CharT, class _Traits>
basic_ios<_CharT, _Traits>
  ::basic_ios(basic_streambuf<_CharT, _Traits>* __streambuf)
    : ios_base(), _M_cached_ctype(0),
      _M_fill(_STLP_NULL_CHAR_INIT(_CharT)), _M_streambuf(0), _M_tied_ostream(0) {
  basic_ios<_CharT, _Traits>::init(__streambuf);
}

template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>*
basic_ios<_CharT, _Traits>::rdbuf(basic_streambuf<_CharT, _Traits>* __buf) {
  basic_streambuf<_CharT, _Traits>* __tmp = _M_streambuf;
  _M_streambuf = __buf;
  this->clear();
  return __tmp;
}

template <class _CharT, class _Traits>
basic_ios<_CharT, _Traits>&
basic_ios<_CharT, _Traits>::copyfmt(const basic_ios<_CharT, _Traits>& __x) {
  _M_invoke_callbacks(erase_event);
  _M_copy_state(__x);           // Inherited from ios_base.
  _M_cached_ctype = __x._M_cached_ctype;
  _M_fill = __x._M_fill;
  _M_tied_ostream = __x._M_tied_ostream;
  _M_invoke_callbacks(copyfmt_event);
  this->_M_set_exception_mask(__x.exceptions());
  return *this;
}

template <class _CharT, class _Traits>
locale basic_ios<_CharT, _Traits>::imbue(const locale& __loc) {
  locale __tmp = ios_base::imbue(__loc);
  _STLP_TRY {
    if (_M_streambuf)
      _M_streambuf->pubimbue(__loc);

    // no throwing here
    _M_cached_ctype = &use_facet<ctype<char_type> >(__loc);
  }
  _STLP_CATCH_ALL {
    __tmp = ios_base::imbue(__tmp);
    _M_handle_exception(ios_base::failbit);
  }
  return __tmp;
}

// Protected constructor and initialization functions. The default
// constructor creates an uninitialized basic_ios, and init() initializes
// all of the members to the values in Table 89 of the C++ standard.

template <class _CharT, class _Traits>
basic_ios<_CharT, _Traits>::basic_ios()
  : ios_base(),
    _M_fill(_STLP_NULL_CHAR_INIT(_CharT)), _M_streambuf(0), _M_tied_ostream(0)
{}

template <class _CharT, class _Traits>
void
basic_ios<_CharT, _Traits>::init(basic_streambuf<_CharT, _Traits>* __sb)
{
  this->rdbuf(__sb);
  this->imbue(locale());
  this->tie(0);
  this->_M_set_exception_mask(ios_base::goodbit);
  this->_M_clear_nothrow(__sb != 0 ? ios_base::goodbit : ios_base::badbit);
  ios_base::flags(ios_base::skipws | ios_base::dec);
  ios_base::width(0);
  ios_base::precision(6);
  this->fill(widen(' '));
  // We don't need to worry about any of the three arrays: they are
  // initialized correctly in ios_base's constructor.
}

// This is never called except from within a catch clause.
template <class _CharT, class _Traits>
void basic_ios<_CharT, _Traits>::_M_handle_exception(ios_base::iostate __flag)
{
  this->_M_setstate_nothrow(__flag);
  if (this->_M_get_exception_mask() & __flag)
    _STLP_RETHROW;
}

_STLP_END_NAMESPACE

#endif /* _STLP_IOS_C */

// Local Variables:
// mode:C++
// End:
