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


#ifndef _STLP_INTERNAL_NUMPUNCT_H
#define _STLP_INTERNAL_NUMPUNCT_H

#ifndef _STLP_IOS_BASE_H
# include <stl/_ios_base.h>
#endif

# ifndef _STLP_C_LOCALE_H
#  include <stl/c_locale.h>
# endif

#ifndef _STLP_INTERNAL_STRING_H
# include <stl/_string.h>
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// numpunct facets

template <class _CharT> class numpunct {};
template <class _CharT> class numpunct_byname {};
template <class _Ch, class _InIt> class num_get;

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC numpunct<char> : public locale::facet {
public:
  typedef char               char_type;
  typedef string             string_type;

  explicit numpunct(size_t __refs = 0)
    : locale::facet(__refs) {}

  char decimal_point() const { return do_decimal_point(); }
  char thousands_sep() const { return do_thousands_sep(); }
  string grouping() const { return do_grouping(); }
  string truename() const { return do_truename(); }
  string falsename() const { return do_falsename(); }

  static _STLP_STATIC_DECLSPEC locale::id id;

protected:
  ~numpunct();

  virtual char do_decimal_point() const;
  virtual char do_thousands_sep() const;
  virtual string do_grouping() const;
  virtual string do_truename() const;
  virtual string do_falsename()  const;
};

# if ! defined (_STLP_NO_WCHAR_T)

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC numpunct<wchar_t> : public locale::facet {
public:
  typedef wchar_t               char_type;
  typedef wstring               string_type;

  explicit numpunct(size_t __refs = 0)
    : locale::facet(__refs) {}

  wchar_t decimal_point() const { return do_decimal_point(); }
  wchar_t thousands_sep() const { return do_thousands_sep(); }
  string grouping() const { return do_grouping(); }
  wstring truename() const { return do_truename(); }
  wstring falsename() const { return do_falsename(); }

  static _STLP_STATIC_DECLSPEC locale::id id;

protected:
  ~numpunct();

  virtual wchar_t do_decimal_point() const;
  virtual wchar_t do_thousands_sep() const;
  virtual string do_grouping() const;
  virtual wstring do_truename() const;
  virtual wstring do_falsename()  const;
};

# endif /* WCHAR_T */

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC numpunct_byname<char> : public numpunct<char> {
  friend class _Locale_impl;
public:
  typedef char                char_type;
  typedef string              string_type;

  explicit numpunct_byname(const char* __name, size_t __refs = 0);

protected:

  ~numpunct_byname();

  virtual char   do_decimal_point() const;
  virtual char   do_thousands_sep() const;
  virtual string do_grouping()      const;
  virtual string do_truename()      const;
  virtual string do_falsename()     const;

private:
  numpunct_byname(_Locale_numeric *__numeric)
    : _M_numeric(__numeric) {}

  //explicitely defined as private to avoid warnings:
  typedef numpunct_byname<char> _Self;
  numpunct_byname(_Self const&);
  _Self& operator = (_Self const&);

  _Locale_numeric* _M_numeric;
};

# ifndef _STLP_NO_WCHAR_T
_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC numpunct_byname<wchar_t>: public numpunct<wchar_t> {
  friend class _Locale_impl;
public:
  typedef wchar_t               char_type;
  typedef wstring               string_type;

  explicit numpunct_byname(const char* __name, size_t __refs = 0);

protected:
  ~numpunct_byname();

  virtual wchar_t   do_decimal_point() const;
  virtual wchar_t   do_thousands_sep() const;
  virtual string do_grouping() const;
  virtual wstring do_truename() const;
  virtual wstring do_falsename() const;

private:
  numpunct_byname(_Locale_numeric *__numeric)
    : _M_numeric(__numeric) {}

  //explicitely defined as private to avoid warnings:
  typedef numpunct_byname<wchar_t> _Self;
  numpunct_byname(_Self const&);
  _Self& operator = (_Self const&);

  _Locale_numeric* _M_numeric;
};

# endif /* WCHAR_T */

_STLP_END_NAMESPACE

#endif /* _STLP_NUMPUNCT_H */

// Local Variables:
// mode:C++
// End:

