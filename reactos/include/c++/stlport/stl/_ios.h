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
#ifndef _STLP_INTERNAL_IOS_H
#define _STLP_INTERNAL_IOS_H


#ifndef _STLP_IOS_BASE_H
# include <stl/_ios_base.h>
#endif

#ifndef _STLP_INTERNAL_CTYPE_H
# include <stl/_ctype.h>
#endif

#ifndef _STLP_INTERNAL_NUMPUNCT_H
# include <stl/_numpunct.h>
#endif

_STLP_BEGIN_NAMESPACE

// ----------------------------------------------------------------------

// Class basic_ios, a subclass of ios_base.  The only important difference
// between the two is that basic_ios is a class template, parameterized
// by the character type.  ios_base exists to factor out all of the
// common properties that don't depend on the character type.

// The second template parameter, _Traits, defaults to char_traits<_CharT>.
// The default is declared in header <iosfwd>, and it isn't declared here
// because C++ language rules do not allow it to be declared twice.

template <class _CharT, class _Traits>
class basic_ios : public ios_base {
  friend class ios_base;
public:                         // Synonyms for types.
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

public:                         // Constructor, destructor.
  explicit basic_ios(basic_streambuf<_CharT, _Traits>* __streambuf);
  virtual ~basic_ios() {}

public:                         // Members from clause 27.4.4.2
  basic_ostream<_CharT, _Traits>* tie() const {
    return _M_tied_ostream;
  }
  basic_ostream<_CharT, _Traits>*
  tie(basic_ostream<char_type, traits_type>* __new_tied_ostream) {
    basic_ostream<char_type, traits_type>* __tmp = _M_tied_ostream;
    _M_tied_ostream = __new_tied_ostream;
    return __tmp;
  }

  basic_streambuf<_CharT, _Traits>* rdbuf() const
    { return _M_streambuf; }

  basic_streambuf<_CharT, _Traits>*
  rdbuf(basic_streambuf<char_type, traits_type>*);

  // Copies __x's state to *this.
  basic_ios<_CharT, _Traits>& copyfmt(const basic_ios<_CharT, _Traits>& __x);

  char_type fill() const { return _M_fill; }
  char_type fill(char_type __fill) {
    char_type __tmp(_M_fill);
    _M_fill = __fill;
    return __tmp;
  }

public:                         // Members from 27.4.4.3.  These four functions
                                // can almost be defined in ios_base.

  void clear(iostate __state = goodbit) {
    _M_clear_nothrow(this->rdbuf() ? __state : iostate(__state|ios_base::badbit));
    _M_check_exception_mask();
  }
  void setstate(iostate __state) { this->clear(rdstate() | __state); }

  iostate exceptions() const { return this->_M_get_exception_mask(); }
  void exceptions(iostate __mask) {
    this->_M_set_exception_mask(__mask);
    this->clear(this->rdstate());
  }

public:                         // Locale-related member functions.
  locale imbue(const locale&);

  inline char narrow(_CharT, char) const ;
  inline _CharT widen(char) const;

  // Helper function that makes testing for EOF more convenient.
  static bool _STLP_CALL _S_eof(int_type __c) {
    const int_type __eof = _Traits::eof();
    return _Traits::eq_int_type(__c, __eof);
  }

protected:
  // Cached copy of the curent locale's ctype facet.  Set by init() and imbue().
  const ctype<char_type>* _M_cached_ctype;

public:
  // Equivalent to &use_facet< Facet >(getloc()), but faster.
  const ctype<char_type>* _M_ctype_facet() const { return _M_cached_ctype; }

protected:
  basic_ios();

  void init(basic_streambuf<_CharT, _Traits>* __streambuf);

public:

  // Helper function used in istream and ostream.  It is called only from
  // a catch clause.
  void _M_handle_exception(ios_base::iostate __flag);

private:                        // Data members
  char_type _M_fill;            // The fill character, used for padding.

  basic_streambuf<_CharT, _Traits>* _M_streambuf;
  basic_ostream<_CharT, _Traits>*   _M_tied_ostream;

};


template <class _CharT, class _Traits>
inline char
basic_ios<_CharT, _Traits>::narrow(_CharT __c, char __default) const
{ return _M_ctype_facet()->narrow(__c, __default); }

template <class _CharT, class _Traits>
inline _CharT
basic_ios<_CharT, _Traits>::widen(char __c) const
{ return _M_ctype_facet()->widen(__c); }

# if !defined (_STLP_NO_METHOD_SPECIALIZATION)
_STLP_TEMPLATE_NULL
inline char
basic_ios<char, char_traits<char> >::narrow(char __c, char) const
{
  return __c;
}

_STLP_TEMPLATE_NULL
inline char
basic_ios<char, char_traits<char> >::widen(char __c) const
{
  return __c;
}
# endif /* _STLP_NO_METHOD_SPECIALIZATION */

# if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_ios<char, char_traits<char> >;
#  if ! defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_ios<wchar_t, char_traits<wchar_t> >;
#  endif
# endif /* _STLP_USE_TEMPLATE_EXPORT */

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_ios.c>
#endif

#endif /* _STLP_IOS */

// Local Variables:
// mode:C++
// End:

