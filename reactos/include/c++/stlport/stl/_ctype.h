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

#ifndef _STLP_INTERNAL_CTYPE_H
#define _STLP_INTERNAL_CTYPE_H

#ifndef _STLP_C_LOCALE_H
#  include <stl/c_locale.h>
#endif

#ifndef _STLP_INTERNAL_LOCALE_H
#  include <stl/_locale.h>
#endif

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

_STLP_BEGIN_NAMESPACE

class _STLP_CLASS_DECLSPEC ctype_base {
public:
  enum mask {
    space   = _Locale_SPACE,
    print   = _Locale_PRINT,
    cntrl   = _Locale_CNTRL,
    upper   = _Locale_UPPER,
    lower   = _Locale_LOWER,
    alpha   = _Locale_ALPHA,
    digit   = _Locale_DIGIT,
    punct   = _Locale_PUNCT,
    xdigit  = _Locale_XDIGIT,
    alnum   = alpha | digit,
    graph   = alnum | punct
  };
};

// ctype<> template

template <class charT> class ctype {};
template <class charT> class ctype_byname {};

//ctype specializations

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC ctype<char> : public locale::facet, public ctype_base {
#ifndef _STLP_NO_WCHAR_T
#  ifdef _STLP_MSVC
    typedef ctype<wchar_t> _Wctype;
    friend _Wctype;
#  else
    friend class ctype<wchar_t>;
#  endif
#endif
public:

  typedef char char_type;

  explicit ctype(const mask* __tab = 0, bool __del = false, size_t __refs = 0);
  bool is(mask __m, char __c) const
  { return ((*(_M_ctype_table+(unsigned char)__c)) & __m) != 0; }

  const char* is(const char* __low, const char* __high, mask* __vec) const {
    for (const char* __p = __low;__p != __high; ++__p, ++__vec) {
      *__vec = _M_ctype_table[(unsigned char)*__p];
    }
    return __high;
  }

  const char* scan_is(mask __m, const char* __low, const char* __high) const;
  const char* scan_not(mask __m, const char* __low, const char* __high) const;

  char        (toupper)(char __c) const { return do_toupper(__c); }
  const char* (toupper)(char* __low, const char* __high) const {
    return do_toupper(__low, __high);
  }

  char        (tolower)(char __c) const { return do_tolower(__c); }
  const char* (tolower)(char* __low, const char* __high) const {
    return do_tolower(__low, __high);
  }

  char        widen(char __c) const { return do_widen(__c); }
  const char* widen(const char* __low, const char* __high, char* __to) const {
    return do_widen(__low, __high, __to);
  }

  char        narrow(char __c, char __dfault) const {
    return do_narrow(__c, __dfault);
  }
  const char* narrow(const char* __low, const char* __high,
                     char __dfault, char* __to) const {
    return do_narrow(__low, __high, __dfault, __to);
  }

  static _STLP_STATIC_DECLSPEC locale::id id;
  _STLP_STATIC_CONSTANT(size_t, table_size = 256);

protected:
  const mask* table() const _STLP_NOTHROW { return _M_ctype_table; }
  static const mask* _STLP_CALL classic_table() _STLP_NOTHROW;

  ~ctype();

  virtual char        do_toupper(char __c) const;
  virtual char        do_tolower(char __c) const;
  virtual const char* do_toupper(char* __low, const char* __high) const;
  virtual const char* do_tolower(char* __low, const char* __high) const;
  virtual char        do_widen(char __c) const;
  virtual const char* do_widen(const char* __low, const char* __high,
                               char* __to) const;
  virtual char        do_narrow(char __c, char /* dfault */ ) const;
  virtual const char* do_narrow(const char* __low, const char* __high,
                                char /* dfault */, char* __to) const;
private:
  struct _Is_mask {
    mask __m;
    _Is_mask(mask __x): __m(__x) {}
   bool operator()(char __c) {return (__m & (unsigned char) __c) != 0;}
  };

protected:
  const mask* _M_ctype_table;
private:
  bool _M_delete;
};

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC ctype_byname<char>: public ctype<char> {
  friend class _Locale_impl;
public:
  explicit ctype_byname(const char*, size_t = 0);
  ~ctype_byname();

  virtual char        do_toupper(char __c) const;
  virtual char        do_tolower(char __c) const;

  virtual const char* do_toupper(char*, const char*) const;
  virtual const char* do_tolower(char*, const char*) const;

private:
  ctype_byname(_Locale_ctype* __ctype)
    : _M_ctype(__ctype)
  { _M_init(); }

  void _M_init();

  //explicitely defined as private to avoid warnings:
  typedef ctype_byname<char> _Self;
  ctype_byname(_Self const&);
  _Self& operator = (_Self const&);

  mask _M_byname_table[table_size];
  _Locale_ctype* _M_ctype;
};

#  ifndef _STLP_NO_WCHAR_T
_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC ctype<wchar_t> : public locale::facet, public ctype_base {
public:
  typedef wchar_t char_type;

  explicit ctype(size_t __refs = 0) : locale::facet(__refs) {}

  bool is(mask __m, wchar_t __c) const
    { return do_is(__m, __c); }

  const wchar_t* is(const wchar_t* __low, const wchar_t* __high,
                    mask* __vec) const
    { return do_is(__low, __high, __vec); }

  const wchar_t* scan_is(mask __m,
                         const wchar_t* __low, const wchar_t* __high) const
    { return do_scan_is(__m, __low, __high); }

  const wchar_t* scan_not (mask __m,
                           const wchar_t* __low, const wchar_t* __high) const
    { return do_scan_not(__m, __low, __high); }

  wchar_t (toupper)(wchar_t __c) const { return do_toupper(__c); }
  const wchar_t* (toupper)(wchar_t* __low, const wchar_t* __high) const
    { return do_toupper(__low, __high); }

  wchar_t (tolower)(wchar_t __c) const { return do_tolower(__c); }
  const wchar_t* (tolower)(wchar_t* __low, const wchar_t* __high) const
    { return do_tolower(__low, __high); }

  wchar_t widen(char __c) const { return do_widen(__c); }
  const char* widen(const char* __low, const char* __high,
                    wchar_t* __to) const
    { return do_widen(__low, __high, __to); }

  char narrow(wchar_t __c, char __dfault) const
    { return do_narrow(__c, __dfault); }
  const wchar_t* narrow(const wchar_t* __low, const wchar_t* __high,
                        char __dfault, char* __to) const
    { return do_narrow(__low, __high, __dfault, __to); }

  static _STLP_STATIC_DECLSPEC locale::id id;

protected:
  ~ctype();

  virtual bool           do_is(mask __m, wchar_t __c) const;
  virtual const wchar_t* do_is(const wchar_t*, const wchar_t*, mask*) const;
  virtual const wchar_t* do_scan_is(mask,
                                    const wchar_t*, const wchar_t*) const;
  virtual const wchar_t* do_scan_not(mask,
                                     const wchar_t*, const wchar_t*) const;
  virtual wchar_t do_toupper(wchar_t __c) const;
  virtual const wchar_t* do_toupper(wchar_t*, const wchar_t*) const;
  virtual wchar_t do_tolower(wchar_t c) const;
  virtual const wchar_t* do_tolower(wchar_t*, const wchar_t*) const;
  virtual wchar_t do_widen(char c) const;
  virtual const char* do_widen(const char*, const char*, wchar_t*) const;
  virtual char  do_narrow(wchar_t __c, char __dfault) const;
  virtual const wchar_t* do_narrow(const wchar_t*, const wchar_t*,
                                   char, char*) const;
};

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC ctype_byname<wchar_t>: public ctype<wchar_t> {
  friend class _Locale_impl;
public:
  explicit ctype_byname(const char* __name, size_t __refs = 0);

protected:
  ~ctype_byname();

  virtual bool           do_is(mask __m, wchar_t __c) const;
  virtual const wchar_t* do_is(const wchar_t*, const wchar_t*, mask*) const;
  virtual const wchar_t* do_scan_is(mask,
                                    const wchar_t*, const wchar_t*) const;
  virtual const wchar_t* do_scan_not(mask,
                                     const wchar_t*, const wchar_t*) const;
  virtual wchar_t do_toupper(wchar_t __c) const;
  virtual const wchar_t* do_toupper(wchar_t*, const wchar_t*) const;
  virtual wchar_t do_tolower(wchar_t c) const;
  virtual const wchar_t* do_tolower(wchar_t*, const wchar_t*) const;

private:
  ctype_byname(_Locale_ctype* __ctype)
    : _M_ctype(__ctype) {}

  //explicitely defined as private to avoid warnings:
  typedef ctype_byname<wchar_t> _Self;
  ctype_byname(_Self const&);
  _Self& operator = (_Self const&);

  _Locale_ctype* _M_ctype;
};

#  endif /* WCHAR_T */

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_CTYPE_H */

// Local Variables:
// mode:C++
// End:

