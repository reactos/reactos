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


#ifndef _STLP_INTERNAL_NUM_GET_H
#define _STLP_INTERNAL_NUM_GET_H

#ifndef _STLP_INTERNAL_ISTREAMBUF_ITERATOR_H
#  include <stl/_istreambuf_iterator.h>
#endif

#ifndef _STLP_C_LOCALE_H
#  include <stl/c_locale.h>
#endif

#ifndef _STLP_INTERNAL_NUMPUNCT_H
#  include <stl/_numpunct.h>
#endif

#ifndef _STLP_INTERNAL_CTYPE_H
#  include <stl/_ctype.h>
#endif

#ifndef _STLP_INTERNAL_IOSTREAM_STRING_H
#  include <stl/_iostream_string.h>
#endif

#ifndef _STLP_FACETS_FWD_H
#  include <stl/_facets_fwd.h>
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// num_get facets

template <class _CharT, class _InputIter>
class num_get: public locale::facet {
public:
  typedef _CharT     char_type;
  typedef _InputIter iter_type;

  explicit num_get(size_t __refs = 0): locale::facet(__refs) {}

#if !defined (_STLP_NO_BOOL)
  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, bool& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }
#endif

#if defined (_STLP_FIX_LIBRARY_ISSUES)
  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, short& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, int& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }
#endif

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, long& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, unsigned short& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, unsigned int& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, unsigned long& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

#if defined (_STLP_LONG_LONG)
  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, _STLP_LONG_LONG& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, unsigned _STLP_LONG_LONG& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }
#endif /* _STLP_LONG_LONG */

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, float& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, double& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

#if !defined (_STLP_NO_LONG_DOUBLE)
  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, long double& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }
# endif

  _InputIter get(_InputIter __ii, _InputIter __end, ios_base& __str,
                 ios_base::iostate& __err, void*& __val) const
  { return do_get(__ii, __end, __str, __err, __val); }

  static locale::id id;

protected:
  ~num_get() {}

  typedef string               string_type;
  typedef ctype<_CharT>        _Ctype;
  typedef numpunct<_CharT>     _Numpunct;

#if !defined (_STLP_NO_BOOL)
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, bool& __val) const;
#endif

  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, long& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, unsigned short& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, unsigned int& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, unsigned long& __val) const;

#if defined (_STLP_FIX_LIBRARY_ISSUES)
  // issue 118 : those are actually not supposed to be here
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, short& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, int& __val) const;
#endif

  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, float& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, double& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, void*& __p) const;

#if !defined (_STLP_NO_LONG_DOUBLE)
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, long double& __val) const;
#endif

#if defined (_STLP_LONG_LONG)
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, _STLP_LONG_LONG& __val) const;
  virtual _InputIter do_get(_InputIter __ii, _InputIter __end, ios_base& __str,
                            ios_base::iostate& __err, unsigned _STLP_LONG_LONG& __val) const;
#endif

};


#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS num_get<char, istreambuf_iterator<char, char_traits<char> > >;
// _STLP_EXPORT_TEMPLATE_CLASS num_get<char, const char*>;
#  if !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
// _STLP_EXPORT_TEMPLATE_CLASS num_get<wchar_t, const wchar_t*>;
#  endif
#endif

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION)

_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_DECLSPEC bool _STLP_CALL __valid_grouping(const char*, const char*, const char*, const char*);

template <class _InputIter, class _Integer, class _CharT>
bool _STLP_CALL
__get_decimal_integer(_InputIter& __first, _InputIter& __last, _Integer& __val, _CharT*);

#  if !defined (_STLP_NO_WCHAR_T)
bool _STLP_DECLSPEC _STLP_CALL __get_fdigit(wchar_t&, const wchar_t*);
bool _STLP_DECLSPEC _STLP_CALL __get_fdigit_or_sep(wchar_t&, wchar_t, const wchar_t*);
#  endif

inline void  _STLP_CALL
_Initialize_get_float(const ctype<char>&,
                       char& Plus, char& Minus,
                       char& pow_e, char& pow_E,
                       char*) {
  Plus = '+';
  Minus = '-';
  pow_e = 'e';
  pow_E = 'E';
}

#  if !defined (_STLP_NO_WCHAR_T)
void _STLP_DECLSPEC _STLP_CALL _Initialize_get_float(const ctype<wchar_t>&,
                                                     wchar_t&, wchar_t&, wchar_t&, wchar_t&, wchar_t*);
#  endif
void _STLP_DECLSPEC _STLP_CALL __string_to_float(const __iostring&, float&);
void _STLP_DECLSPEC _STLP_CALL __string_to_float(const __iostring&, double&);
#  if !defined (_STLP_NO_LONG_DOUBLE)
void _STLP_DECLSPEC _STLP_CALL __string_to_float(const __iostring&, long double&);
#  endif

_STLP_MOVE_TO_STD_NAMESPACE

#endif /* _STLP_EXPOSE_STREAM_IMPLEMENTATION */


_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_num_get.c>
#endif

#endif /* _STLP_INTERNAL_NUM_GET_H */

// Local Variables:
// mode:C++
// End:

